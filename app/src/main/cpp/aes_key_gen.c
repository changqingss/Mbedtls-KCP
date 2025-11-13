#include "aes_key_gen.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

//#include "../log/log_conf.h"
#include <android/log.h>

// 简化的日志宏定义
#define AES_LOG_I(fmt, ...)  __android_log_print(ANDROID_LOG_DEBUG, "AES_GEN", fmt, ##__VA_ARGS__)
#define AES_LOG_E(fmt, ...)  __android_log_print(ANDROID_LOG_ERROR, "AES_GEN", fmt, ##__VA_ARGS__)
#define AES_LOG_W(fmt, ...) LOG_W("AES_GEN", fmt, ##__VA_ARGS__)
// #define AES_LOG_D(fmt, ...) LOG_D("AES_GEN", fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##__VA_ARGS__)


#define AES_LOG_D(fmt, ...) \
  do {                      \
  } while (0)

/**
 * @brief 生成可读的ASCII字符串格式AES密钥
 */
int aes_generate_key_128_string(char *key_str, size_t key_str_len) {
  if (!key_str || key_str_len < 17) {
    AES_LOG_E("Invalid parameters for AES key string generation, key_str_len must be at least 17");
    return -1;
  }

  // 转换为可读的ASCII字符串格式
  // 使用自定义的字符集，每个字节映射到一个字符
  const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  int charset_len = strlen(charset);

  for (int i = 0; i < 16; i++) {
    // 将每个字节映射到可读字符
    int index = (rand() % charset_len);
    key_str[i] = charset[index];
  }

  key_str[16] = '\0';  // 确保字符串结束

  AES_LOG_I("Successfully generated readable AES key string");
  AES_LOG_D("Generated key string: %s", key_str);

  return 0;
}

/**
 * @brief 生成可读的ASCII字符串格式AES密钥并保存到文件
 */
int aes_generate_and_save_key_string(const char *filename) {
  if (!filename) {
    AES_LOG_E("Invalid filename for AES key string generation and save");
    return -1;
  }

  char key_str[17];  // 16字符 + 1个结束符

  // 生成可读的AES密钥字符串
  if (aes_generate_key_128_string(key_str, sizeof(key_str)) != 0) {
    AES_LOG_E("Failed to generate AES key string");
    return -1;
  }

  // 保存密钥字符串到文件
  FILE *fp = fopen(filename, "w");
  if (!fp) {
    AES_LOG_E("Failed to open file for writing: %s, error: %s", filename, strerror(errno));
    return -1;
  }

  size_t bytes_written = fwrite(key_str, 1, strlen(key_str), fp);
  fclose(fp);

  if (bytes_written != strlen(key_str)) {
    AES_LOG_E("Failed to write complete key string to file: %s, expected %zu, got %zu", filename, strlen(key_str),
              bytes_written);
    return -1;
  }

  AES_LOG_I("Successfully generated and saved readable AES key string to: %s", filename);
  return 0;
}

/**
 * @brief 从文件加载AES密钥
 */
int aes_load_key_from_file(uint8_t *key, size_t key_len, const char *filename) {
  if (!key || !filename || key_len < 16) {
    AES_LOG_E("Invalid parameters for loading AES key");
    return -1;
  }

  if (access(filename, F_OK)) {
    AES_LOG_E("AES key file does not exist: %s, error: %s", filename, strerror(errno));
    return -1;
  }

  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    AES_LOG_E("Failed to open file for reading: %s, error: %s", filename, strerror(errno));
    return -1;
  }

  size_t bytes_read = fread(key, 1, 16, fp);
  fclose(fp);

  if (bytes_read < 16) {
    AES_LOG_E("Failed to read complete key from file: %s, expected %zu, got %zu", filename, key_len, bytes_read);
    return -1;
  }

  AES_LOG_I("Successfully loaded AES key from file: %s", filename);
  return 0;
}

int aes_generate_iv_string(char *iv, size_t iv_len) {
  // 生成随机IV
  char iv_temp[17];
  if (aes_generate_key_128_string(iv_temp, sizeof(iv_temp)) != 0) {
    AES_LOG_E("Failed to generate AES IV string");
    return -1;
  }
  memcpy(iv, iv_temp, iv_len);
  return 0;
}