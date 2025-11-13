#include "wo_aes.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <android/log.h>
#define LOG_TAG "REPEATER"
#define LOGD(fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##__VA_ARGS__)

// 0、打印16进制的数据
void print_hex(const char *s, const unsigned char *data, size_t len) {
  size_t i;
  printf("%s: ", s);
  for (i = 0; i < len; i++) printf("%02x", data[i]);
  printf("\n");
}

// 1、填充PKCS7数据，填充的原则是把填充的字节长度作为填充的内容
static int aes_padding_pkcs7_set(unsigned char *data, int data_len) {
  int padding_len = AES_BLOCK_SIZE - (data_len % AES_BLOCK_SIZE);
  int i;

  for (i = 0; i < padding_len; i++) {
    data[data_len + i] = (unsigned char)padding_len;
  }

  return data_len + padding_len;
}

// 2、获取PKCS7数据
static int aes_padding_pkcs7_get(unsigned char *data, int data_len) {
  int i;
  int pad_idx;
  unsigned char padding_len;
  unsigned char bad = 0;
  int len;

  padding_len = data[data_len - 1];
  for (i = 0; i < padding_len; i++) {
    if (data[data_len - 1 - i] != padding_len) {
      return 0;  // data_len;
    }
  }
  len = data_len - padding_len;

  /* Avoid logical || since it results in a branch */
  bad |= padding_len > data_len;
  bad |= padding_len == 0;

  /* The number of bytes checked must be independent of padding_len,
   * so pick input_len, which is usually 8 or 16 (one block) */
  pad_idx = data_len - padding_len;
  for (i = 0; i < data_len; i++) {
    bad |= (data[i] ^ padding_len) * (i >= pad_idx);
  }

  return (len * (bad == 0));
}

// 3、初始化或者反初始化解密器
int aes_encrypo_opt(aes_128_cbc_encrypo_t *enc, const unsigned char *key, const unsigned char *iv, AES_OPT_TYPE opt) {
  switch (opt) {
    case AES_OPT_TYPE_ENC_INIT: {
      if (enc->is_init == 0) {
        memcpy(enc->key, key, AES_KEY_SIZE);
        memcpy(enc->iv, iv, AES_KEY_SIZE);

#if USE_MBEDTLS
        mbedtls_aes_init(&enc->aes);
        mbedtls_aes_setkey_enc(&enc->aes, key, AES_KEY_SIZE * 8);
#elif USE_OPENSSL
        AES_set_encrypt_key(enc->key, AES_KEY_SIZE * 8, &enc->aes_key_enc);
#endif

        enc->is_init = 1;
      }
      break;
    }

    case AES_OPT_TYPE_ENC_DEINIT:
      if (enc->is_init) {
#if USE_MBEDTLS
        mbedtls_aes_free(&enc->aes);
#endif
        enc->is_init = 0;
      }
      break;

    default:
      break;
  }
  return enc->is_init ? 0 : -1;  // 返回是否初始化成功
}

// 2、加密数据
int aes_encrypo_data(aes_128_cbc_encrypo_t *enc, const char *in, int in_len, char *out) {
  if (enc->is_init) {
    int len = 0, ret = 0;
    unsigned char iv[AES_KEY_SIZE] = "0";
    memcpy(out, in, in_len);
    memcpy(iv, enc->iv, AES_KEY_SIZE);

    // 1.把输入的数据拷贝到out，然后对out进行pkcs7填充，pkcs7的填充原则是把填充的字节长度作为填充的内容
    len = aes_padding_pkcs7_set(out, in_len);

#if USE_MBEDTLS
    ret = mbedtls_aes_crypt_cbc(&enc->aes, MBEDTLS_AES_ENCRYPT, len, iv, out, out);
#elif USE_OPENSSL
    AES_cbc_encrypt(out, out, len, &enc->aes_key_enc, iv, AES_ENCRYPT);
#endif

      // 🔹 打印加密后的数据（16进制）
      char hexBuf[1024] = {0};
      char *p = hexBuf;
      for (int i = 0; i < len; i++) {
          p += sprintf(p, "%02X", (unsigned char)out[i]);
          if (i < len - 1) *p++ = ':';  // 美观分隔
      }
      *p = '\0';
      LOGD("AES Encrypt Output (len=%d): %s", len, hexBuf);

    return len;
  }
}

// 3、解密的初始化和反初始化
int aes_decrypo_opt(aes_128_cbc_decrypo_t *dec, const unsigned char *key, const unsigned char *iv, AES_OPT_TYPE opt) {
  switch (opt) {
    case AES_OPT_TYPE_DEC_INIT: {
      if (dec->is_init == 0) {
        memcpy(dec->key, key, AES_KEY_SIZE);
        memcpy(dec->iv, iv, AES_KEY_SIZE);

#if USE_MBEDTLS
        mbedtls_aes_init(&dec->aes);
        mbedtls_aes_setkey_dec(&dec->aes, key, AES_KEY_SIZE * 8);
#elif USE_OPENSSL
        AES_set_decrypt_key(dec->key, AES_KEY_SIZE * 8, &dec->aes_key_dec);
#endif

        dec->is_init = 1;
      }
      break;
    }

    case AES_OPT_TYPE_DEC_DEINIT:
      if (dec->is_init) {
#if USE_MBEDTLS
        mbedtls_aes_free(&dec->aes);
#endif
        dec->is_init = 0;
      }
      break;

    default:
      break;
  }
  return dec->is_init ? 0 : -1;  // 返回是否初始化成功
}

// 4、AES128解密数据
int aes_decrypo_data(aes_128_cbc_decrypo_t *dec, const char *in, int in_len, char *out) {
  if (!dec || !in || !dec->is_init) return -1;

  int ret = 0;
  int len = in_len;
  unsigned char iv[AES_KEY_SIZE] = "0";
  memcpy(iv, dec->iv, AES_KEY_SIZE);

#if USE_MBEDTLS
  ret = mbedtls_aes_crypt_cbc(&dec->aes, MBEDTLS_AES_DECRYPT, in_len, iv, in, out);
#elif USE_OPENSSL
  AES_cbc_encrypt(in, out, in_len, &dec->aes_key_dec, iv, AES_DECRYPT);
#endif

  len = aes_padding_pkcs7_get(out, len);
  return len;
}

// 5、初始化加密器
int init_enc_dec(aes_128_cbc_encrypo_t *enc, aes_128_cbc_decrypo_t *dec, char *key, char *iv) {
  aes_encrypo_opt(enc, key, iv, AES_OPT_TYPE_ENC_INIT);
  aes_decrypo_opt(dec, key, iv, AES_OPT_TYPE_DEC_INIT);
  return 0;
}

// 6、初始化解密器
int deinit_enc_dec(aes_128_cbc_encrypo_t *enc, aes_128_cbc_decrypo_t *dec) {
  aes_encrypo_opt(enc, NULL, NULL, AES_OPT_TYPE_ENC_DEINIT);
  aes_decrypo_opt(dec, NULL, NULL, AES_OPT_TYPE_DEC_DEINIT);
  return 0;
}

// 7、先加密再解密数据
int enc_dec_data(aes_128_cbc_encrypo_t *enc, aes_128_cbc_decrypo_t *dec) {
  // unsigned char src_data[] = "The quick brown ";
  unsigned char src_data[] = "Hello AES Data, Hello AES Data, Hello AES Data, Hello AES Data";
  unsigned char enc_data[256] = "";
  unsigned char dec_data[256] = "";
  int enc_len = 0, dec_len = 0;

  print_hex("src_data", src_data, strlen(src_data));

  enc_len = aes_encrypo_data(enc, src_data, strlen(src_data), enc_data);

  print_hex("enc_data", enc_data, enc_len);

  dec_len = aes_decrypo_data(dec, enc_data, enc_len, dec_data);

  print_hex("dec_data", dec_data, dec_len);

  return 0;
}

int wo_aes_test_main() {
  int test_cnt = 100, i = 0;
  aes_128_cbc_encrypo_t enc = {0};
  aes_128_cbc_decrypo_t dec = {0};

  unsigned char key[16] = "0123456789012345";
  unsigned char iv[16] = "0123456789012345";

  while (test_cnt-- > 0) {
    printf("=======%d=======\n", i);
    init_enc_dec(&enc, &dec, key, iv);
    enc_dec_data(&enc, &dec);
    deinit_enc_dec(&enc, &dec);
    printf("=======%d=======\n", i++);

    sleep(1);
  }

  return 0;
}
