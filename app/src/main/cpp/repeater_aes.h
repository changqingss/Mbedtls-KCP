#ifndef REPEATER_AEC_H
#define REPEATER_AEC_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "wo_aes.h"
//#include "common/param_config/dev_param.h"

typedef struct quartuples_t {
  struct in_addr ip;
  uint8_t mac[6];
  uint8_t key[AES_KEY_SIZE];
  uint8_t iv[AES_KEY_SIZE];
} Quadruples, *PQuadruples;

#define MAX_AES_KEY_NUM (5)
typedef struct client_list_t {
  int count;
  uint8_t macs[MAX_AES_KEY_NUM][6];
} ClientList, *PClientList;

aes_128_cbc_encrypo_t *repeater_aes_enc_get(const uint8_t *mac);
aes_128_cbc_decrypo_t *repeater_aes_dec_get(const uint8_t *mac);
aes_128_cbc_encrypo_t *repeater_client_aes_enc_get(void);
aes_128_cbc_decrypo_t *repeater_client_aes_dec_get(void);

int repeater_update_quadruples(PQuadruples quadruples);
int repeater_del_quadruples(const uint8_t *mac);
int repeater_del_all_quadruples(void);
int repeater_dump_quadruples(void);
int repeater_sync_quadruples(char **mac, int mac_count);  // 从后台同步拓展器的MAC列表

void repeater_init_client_quadruples(PQuadruples quadruples);
void repeater_set_default_key(const uint8_t *key);
void repeater_get_default_key(uint8_t *key);

void repeater_aes_enc_deinit(aes_128_cbc_encrypo_t *enc);
void repeater_aes_dec_deinit(aes_128_cbc_decrypo_t *dec);

void repeater_save_quadruples(void);
void repeater_load_quadruples(void);
void repeater_get_client_list(PClientList list);

void repeater_save_client_quadruples(void);
void repeater_load_client_quadruples(void);

void repeater_set_master_mac(const uint8_t *mac);
void repeater_get_master_mac(uint8_t *mac);

void repeater_client_get_aes_key(uint8_t *key);
void repeater_client_get_aes_iv(uint8_t *iv);
void repeater_client_get_mac(uint8_t *mac);
void repeater_client_get_ip(struct in_addr *ip);
void repeater_client_set_ip(const struct in_addr *ip);

//inline int get_aes_enc_out_len(int in_len) { return AES_BLOCK_SIZE - (in_len % AES_BLOCK_SIZE) + in_len; }

// 基于repeaterId获取完整MAC地址
int repeater_get_mac_by_repeater_id(uint16_t repeater_id, uint8_t *mac);
/**
 * @brief 生成可读的ASCII字符串格式AES
 * @param iv 输出缓冲区，必须至少16字节
 * @param iv_len IV长度，应该为16
 * @param filename IV文件路径
 * @return 输出数据长度
 */
//int aes_generate_iv_string(char *iv, size_t iv_len);

#endif  // REPEATER_AEC_H