#ifndef HTTPS_UTILS_H
#define HTTPS_UTILS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LOCAL_KEY_SIZE (16)

struct https_param_t {
  char secret[MAX_LOCAL_KEY_SIZE];
  char mac[32];
  char url[100];
  unsigned long long timestamp;  // 毫秒时间戳
};

struct https_token_t {
  char *token;
  int expiresIn;                 // 过期时间，单位为秒
  unsigned long long timestamp;  // 毫秒时间戳
};

struct https_data_t {
  char *data;
};

#define HTTPS_GET_METHOD "GET"
#define HTTPS_TOKEN_PATH "/device/v1/auth"
#define HTTPS_MATTER_DAC_PATH "/device/v1/device/matter"
#define HTTPS_URL_PREFIX "https://wonderlabs."
#define HTTPS_ONLINE_URL_SUFFIX ".api.switchbot.net"
#define HTTPS_OFFLINE_URL_SUFFIX ".api.woankeji.cn"

#define MATTER_DAC_CERT_FILE "/meta/matter/wlab.json"

int get_https_token(struct https_param_t *param, struct https_token_t *token);
int https_utils_read_region(char *region, int region_size, bool *isOnline);
int https_utils_read_param(struct https_param_t *param);
int get_https_data(struct https_param_t *param, struct https_token_t *token, struct https_data_t *data);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
