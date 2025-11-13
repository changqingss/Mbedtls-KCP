#ifndef __AES_128_CBC_H__
#define __AES_128_CBC_H__

#include <stdint.h>

#define USE_OPENSSL 0
#define USE_MBEDTLS 1

#if USE_OPENSSL
#include <openssl/aes.h>
#elif USE_MBEDTLS
#include "mbedtls/aes.h"
#endif

#define AES_KEY_SIZE (16)

#ifndef AES_BLOCK_SIZE
#define AES_BLOCK_SIZE (16)
#endif

typedef enum AES_OPT_TYPE {
  AES_OPT_TYPE_ENC_INIT,
  AES_OPT_TYPE_ENC_DEINIT,

  AES_OPT_TYPE_DEC_INIT,
  AES_OPT_TYPE_DEC_DEINIT,
} AES_OPT_TYPE;

typedef struct aes_128_cbc_encrypo_t {
  int32_t is_init;

#if USE_MBEDTLS
  mbedtls_aes_context aes;
#elif USE_OPENSSL
  AES_KEY aes_key_enc;
#endif

  unsigned char key[AES_KEY_SIZE];
  unsigned char iv[AES_KEY_SIZE];

} aes_128_cbc_encrypo_t;

typedef struct aes_128_cbc_decrypo_t {
  int32_t is_init;

#if USE_MBEDTLS
  mbedtls_aes_context aes;
#elif USE_OPENSSL
  AES_KEY aes_key_dec;
#endif

  unsigned char key[AES_KEY_SIZE];
  unsigned char iv[AES_KEY_SIZE];

} aes_128_cbc_decrypo_t;

inline int get_aes_enc_out_len(int in_len) { return AES_BLOCK_SIZE - (in_len % AES_BLOCK_SIZE) + in_len; }

int aes_encrypo_opt(aes_128_cbc_encrypo_t *enc, const unsigned char *key, const unsigned char *iv, AES_OPT_TYPE opt);
int aes_encrypo_data(aes_128_cbc_encrypo_t *enc, const char *in, int in_len, char *out);

int aes_decrypo_opt(aes_128_cbc_decrypo_t *dec, const unsigned char *key, const unsigned char *iv, AES_OPT_TYPE opt);
int aes_decrypo_data(aes_128_cbc_decrypo_t *dec, const char *in, int in_len, char *out);

#endif