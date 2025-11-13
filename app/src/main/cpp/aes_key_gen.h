#ifndef __AES_KEY_GEN_H__
#define __AES_KEY_GEN_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 从文件加载AES密钥
 * @param key 输出缓冲区，必须至少16字节
 * @param key_len 密钥长度，应该为16
 * @param filename 密钥文件路径
 * @return 0成功，-1失败
 */
int aes_load_key_from_file(uint8_t *key, size_t key_len, const char *filename);

/**
 * @brief 生成并保存AES密钥到文件
 * @param filename 密钥文件路径
 * @return 0成功，-1失败
 */
int aes_generate_and_save_key(const char *filename);

/**
 * @brief 生成可读的ASCII字符串格式AES密钥并保存到文件
 * @param filename 密钥文件路径
 * @return 0成功，-1失败
 */
int aes_generate_and_save_key_string(const char *filename);

/**
 * @brief 生成可读的ASCII字符串格式AES
 * @param iv 输出缓冲区，必须至少16字节
 * @param iv_len IV长度，应该为16
 * @param filename IV文件路径
 * @return 输出数据长度
 */
int aes_generate_iv_string(char *iv, size_t iv_len);

#ifdef __cplusplus
}
#endif

#define CLIENT_HELLO "client hello"
#define CLIENT_HELLO_LEN strlen(CLIENT_HELLO)
#define SERVER_HELLO "server hello"
#define SERVER_HELLO_LEN strlen(SERVER_HELLO)
#define SERVER_HELLO_ACK "server hello ack"
#define SERVER_HELLO_ACK_LEN strlen(SERVER_HELLO_ACK)

#endif /* __AES_KEY_GEN_H__ */
