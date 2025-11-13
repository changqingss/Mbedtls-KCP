#ifndef _WIFI_H_
#define _WIFI_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

#define WIFI_NAME_MAX_SIZE 128

typedef struct WIFI_CONFIG {
  char ssid[128];
  char passwd[128];
} WIFI_CONFIG_S;

typedef struct WIFI_QUALITY {
  int link;
  int level;
  int noise;
} WIFI_QUALITY_S;

int WIFI_GetIp(char *ip);
int WIFI_GetBleMac(char *mac);
int WIFI_GetWifiMac(char *mac);
int WIFI_GetName(char *name, bool retrieve);
bool WIFI_CheckNetwork(void);
int WIFI_GetWifiQuality(WIFI_QUALITY_S *pQuality);
bool WIFI_ApStaCheck(void);
bool WIFI_CheckStaConnect(void);

int WIFI_generate_local_net_info(char *m_ssid, int ssidMaxLen, char *m_passwd, int passwdMaxLen);
int WIFI_copy_wifi_conf_file(void);
int WIFI_ifconfig_up(char *m_interface);
int WIFI_GetLocalIp(const char *iface, char *local_ip);

int WIFI_GetSignal(void);

#ifdef __cplusplus
}
#endif

#endif /* NETWORK_MANAGER_H_ */

/** @} */
