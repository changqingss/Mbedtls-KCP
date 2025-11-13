#include "wifi.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "common/api/comm_api.h"
#include "common/param_config/dev_param.h"
#define LOG_TAG "WIFI"
#include "ble_mac.h"
#include "common/log/log_conf.h"

#define WIFI_QUALITY_INFO_FILE "/proc/net/wireless"
#define WPA_SUPPLICANT "wpa_supplicant"
#define WPA_CLI "wpa_cli"
#define WLAN0 "wlan0"

// 全局变量存储蓝牙MAC地址
static char g_ble_mac[32] = {0};  // 12位MAC地址 + 1位结束符
static pthread_mutex_t g_ble_mac_mutex = PTHREAD_MUTEX_INITIALIZER;

/*****************************************************
 * @fn	    	WIFI_GetName
 * @brief		获取WIFI NAME
 * @param		name
 * @param		retrieve 是否重新获取
 * @return 	解析成功返回0，失败返回-1
 ******************************************************/
char gWiFiName[WIFI_NAME_MAX_SIZE];
int WIFI_GetName(char *name, bool retrieve) {
  FILE *fp;
  static bool first = false;
  char temp[WIFI_NAME_MAX_SIZE] = {0};

  if (!first || retrieve) {
    fp = popen("wpa_cli status | grep -v bssid | awk -F= '/ssid/ {print $2}'", "r");
    if (fp == NULL) {
      COMMONLOG_E("Error opening pipe %s", strerror(errno));
      return 1;
    }

    if (fgets(temp, WIFI_NAME_MAX_SIZE, fp) != NULL) {
      temp[strcspn(temp, "\n")] = '\0';
      memset(gWiFiName, 0, sizeof(gWiFiName));
      memcpy(gWiFiName, temp, WIFI_NAME_MAX_SIZE);
      COMMONLOG_D("SSID:%s", gWiFiName);
      first = true;
    } else {
      COMMONLOG_E("SSID not found");
    }

    pclose(fp);
  }
  memcpy(name, gWiFiName, WIFI_NAME_MAX_SIZE);
  return 0;
}

/*****************************************************
 * @fn	    	WIFI_GetMac
 * @brief		获取mac地址
 * @param		mac
 * @return 	解析成功返回0，失败返回-1
 ******************************************************/
int WIFI_GetBleMac(char *mac) {
  unsigned char macHex[6] = {0};
  static bool first_read = true;

  pthread_mutex_lock(&g_ble_mac_mutex);
  // 如果是第一次读取或者内存中的MAC为空，则从文件读取
  if (first_read || strlen(g_ble_mac) == 0) {
    if (COMM_API_CheckFileExist(HUB_BLE_MAC_FILE, F_OK) == 0) {
      if (ble_read_mac_address(HUB_BLE_MAC_FILE, macHex, g_ble_mac) == 0) {
        COMMONLOG_I("[WIFI_GetMac] get hub ble mac from file:%s", g_ble_mac);
        first_read = false;
        strcpy(mac, g_ble_mac);
        pthread_mutex_unlock(&g_ble_mac_mutex);
        return 0;
      }
    }
    COMMONLOG_E("WIFI_GetMac:error reading from file");
    pthread_mutex_unlock(&g_ble_mac_mutex);
    return 1;
  }

  // 从内存中获取MAC地址
  strcpy(mac, g_ble_mac);
  COMMONLOG_I("[WIFI_GetMac] get hub ble mac from memory:%s", mac);
  pthread_mutex_unlock(&g_ble_mac_mutex);
  return 0;
}

int WIFI_SetBleMac(const char *macHex) {
  char macStr[32] = {0};
  pthread_mutex_lock(&g_ble_mac_mutex);
  sprintf(macStr, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX", macHex[0], macHex[1], macHex[2], macHex[3], macHex[4],
          macHex[5]);
  strcpy(g_ble_mac, macStr);
  COMMONLOG_I("[WIFI_SetBleMac] set hub ble mac, %s", macStr);
  pthread_mutex_unlock(&g_ble_mac_mutex);
  return 0;
}

/*****************************************************
 * @fn	    	WIFI_GetMac
 * @brief		获取mac地址
 * @param		mac
 * @return 	解析成功返回0，失败返回-1
 ******************************************************/
int WIFI_GetWifiMac(char *mac) {
  // 获取 WIFI MAC 地址
  struct ifreq ifreq;
  int sock_fd;

  if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    COMMONLOG_E("Open AF_INET socket error!\n");
    return -1;
  }

  strcpy(ifreq.ifr_name, "wlan0");
  if (ioctl(sock_fd, SIOCGIFHWADDR, &ifreq) < 0) {
    COMMONLOG_E("ioctl error!\n");
    close(sock_fd);
    return -1;
  }

  sprintf(mac, "%02X%02X%02X%02X%02X%02X", (unsigned char)ifreq.ifr_hwaddr.sa_data[0],
          (unsigned char)ifreq.ifr_hwaddr.sa_data[1], (unsigned char)ifreq.ifr_hwaddr.sa_data[2],
          (unsigned char)ifreq.ifr_hwaddr.sa_data[3], (unsigned char)ifreq.ifr_hwaddr.sa_data[4],
          (unsigned char)ifreq.ifr_hwaddr.sa_data[5]);
  close(sock_fd);

  return 0;
}

/*****************************************************
 * @fn	    	WIFI_GetIp
 * @brief		获取ip地址
 * @param		ip
 * @return 	解析成功返回0，失败返回-1
 ******************************************************/
int WIFI_GetIp(char *ip) {
  int MAXINTERFACES = 16;
  int fd, intrface, ret = -1;
  struct ifreq buf[MAXINTERFACES];
  struct ifconf ifc;
  char *ipTmp = NULL;
  int ip_len = 0;

  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;
    if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc)) {
      intrface = ifc.ifc_len / sizeof(struct ifreq);
      while (intrface-- > 0) {
        if (!(ioctl(fd, SIOCGIFADDR, (char *)&buf[intrface]))) {
          ipTmp = (inet_ntoa(((struct sockaddr_in *)(&buf[intrface].ifr_addr))->sin_addr));
          ip_len = strlen(ipTmp);
          if (ip_len < 128) {
            strncpy(ip, ipTmp, ip_len);
            ip[ip_len] = '\0';
            ret = 0;
          } else {
            COMMONLOG_E("ip is too long");
            ret = -1;
          }
          break;
        }
      }
    }
    close(fd);
  }
  return ret;
}

/*****************************************************
 * @fn	    	WIFI_GetGatewayIp
 * @brief		获取ip地址
 * @param		ip
 * @return 	解析成功返回0，失败返回-1
 ******************************************************/
int WIFI_GetGatewayIp(char *ip) {
  char cmd[128] = {0};
  char readline[128] = {0};
  memset(cmd, 0, sizeof(cmd));
  sprintf(cmd, "route |grep default|awk \'{print $2}\'");
  FILE *fp = popen(cmd, "r");

  if (NULL == fp) {
    return -1;
  }

  memset(readline, 0, sizeof(readline));
  while (NULL != fgets(readline, sizeof(readline), fp)) {
    if (readline[strlen(readline) - 1] == '\n') {
      readline[strlen(readline) - 1] = 0;
    }
    printf("gateway=%s\n", readline);
    memcpy(ip, readline, strlen(readline));
    break;
  }
  pclose(fp);

  return 0;
}

/* 得到本机的mac地址和ip地址 */
int WIFI_GetLocalMac(const char *device, char *mac, char *ip) {
  int sockfd;
  struct ifreq req;
  struct sockaddr_in *sin;

  if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
    COMMONLOG_E("Sock Error:%s\n\a", strerror(errno));
    return (-1);
  }

  memset(&req, 0, sizeof(req));
  strcpy(req.ifr_name, device);
  if (ioctl(sockfd, SIOCGIFHWADDR, (char *)&req) == -1) {
    COMMONLOG_E("ioctl SIOCGIFHWADDR:%s\n\a", strerror(errno));
    close(sockfd);
    return (-1);
  }
  memcpy(mac, req.ifr_hwaddr.sa_data, 6);

  req.ifr_addr.sa_family = PF_INET;
  if (ioctl(sockfd, SIOCGIFADDR, (char *)&req) == -1) {
    COMMONLOG_E("ioctl SIOCGIFADDR:%s\n\a", strerror(errno));
    close(sockfd);
    return (-1);
  }
  sin = (struct sockaddr_in *)&req.ifr_addr;
  memcpy(ip, (char *)&sin->sin_addr, 4);

  close(sockfd);
  return (0);
}

char *mac_ntoa(const unsigned char *mac) {
  /* Linux 下有 ether_ntoa(),不过我们重新写一个也很简单 */
  static char buffer[18];
  memset(buffer, 0, sizeof(buffer));
  sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return (buffer);
}

/* 根据 RFC 0826 修改*/
typedef struct _Ether_pkg Ether_pkg;
struct _Ether_pkg {
  /* 前面是ethernet头 */
  unsigned char ether_dhost[6];  /* 目地硬件地址 */
  unsigned char ether_shost[6];  /* 源硬件地址 */
  unsigned short int ether_type; /* 网络类型 */

  /* 下面是arp协议 */
  unsigned short int ar_hrd; /* 硬件地址格式 */
  unsigned short int ar_pro; /* 协议地址格式 */
  unsigned char ar_hln;      /* 硬件地址长度(字节) */
  unsigned char ar_pln;      /* 协议地址长度(字节) */
  unsigned short int ar_op;  /* 操作代码 */
  unsigned char arp_sha[6];  /* 源硬件地址 */
  unsigned char arp_spa[4];  /* 源协议地址 */
  unsigned char arp_tha[6];  /* 目地硬件地址 */
  unsigned char arp_tpa[4];  /* 目地协议地址 */
};

void WIFI_parse_ether_package(const Ether_pkg *pkg) {
  COMMONLOG_I("src  IP=[%s] MAC=[%s]\n", inet_ntoa(*(struct in_addr *)pkg->arp_spa), mac_ntoa(pkg->arp_sha));
  COMMONLOG_I("dest IP=[%s] MAC=[%s]\n", inet_ntoa(*(struct in_addr *)pkg->arp_tpa), mac_ntoa(pkg->arp_tha));
}

int WIFI_sendpkg(char *mac, char *broad_mac, char *ip, int ipLen, char *dest) {
  int nRet = 0;
  Ether_pkg pkg;
  struct hostent *host = NULL;
  struct sockaddr sa;
  int sockfd, len;
  char buffer[255];
  memset((char *)&pkg, '\0', sizeof(pkg));

  /* 填充ethernet包文 */
  memcpy((char *)pkg.ether_shost, (char *)mac, 6);
  memcpy((char *)pkg.ether_dhost, (char *)broad_mac, 6);
  pkg.ether_type = htons(ETHERTYPE_ARP);

  /* 下面填充arp包文 */
  pkg.ar_hrd = htons(ARPHRD_ETHER);
  pkg.ar_pro = htons(ETHERTYPE_IP);
  pkg.ar_hln = 6;
  pkg.ar_pln = 4;
  pkg.ar_op = htons(ARPOP_REQUEST);
  memcpy((char *)pkg.arp_sha, (char *)mac, 6);
  memcpy((char *)pkg.arp_spa, (char *)ip, 4);
  memcpy((char *)pkg.arp_tha, (char *)broad_mac, 6);

  /*printf ( "Resolve [%s],Please Waiting...",dest );
   以欺骗方式发包，会造成IP冲突错误 */
  fflush(stdout);
  memset(ip, 0, ipLen);
  if (inet_aton(dest, (struct in_addr *)ip) == 0) {
    if ((host = gethostbyname(dest)) == NULL) {
      COMMONLOG_E("Fail! %s\n\a", hstrerror(h_errno));
      return (-1);
    }
    memcpy((char *)ip, host->h_addr, 4);
  }
  memcpy((char *)pkg.arp_tpa, (char *)ip, 4);

  /*unsigned char tip[5];
memset(tip,0,sizeof(tip));
  inet_aton(dest,(struct in_addr *)tip);
  memcpy((char *)pkg.arp_tpa,(char *)tip,4);*/

  /* 实际应该使用PF_PACKET */
  if ((sockfd = socket(PF_INET, SOCK_PACKET, htons(ETH_P_ALL))) == -1) {
    COMMONLOG_E("Socket Error:%s\n\a", strerror(errno));
    return (0);
  }

  memset(&sa, '\0', sizeof(sa));
  strcpy(sa.sa_data, "wlan0");

  len = sendto(sockfd, &pkg, sizeof(pkg), 0, &sa, sizeof(sa));
  if (len != sizeof(pkg)) {
    COMMONLOG_E("Sendto Error:%s\n\a", strerror(errno));
    return (0);
  }
  Ether_pkg *parse;
  parse = (Ether_pkg *)buffer;

  fd_set readfds;
  struct timeval tv;
  tv.tv_sec = 5;
  tv.tv_usec = 0; /*50毫秒*/
  FD_ZERO(&readfds);
  FD_SET(sockfd, &readfds);
  len = select(sockfd + 1, &readfds, 0, 0, &tv);
  if (len > -1) {
    if (FD_ISSET(sockfd, &readfds)) {
      memset(buffer, 0, sizeof(buffer));
      len = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, (socklen_t *)&len);
      if ((ntohs(parse->ether_type) == ETHERTYPE_ARP) && (ntohs(parse->ar_op) == ARPOP_REPLY)) {
        WIFI_parse_ether_package(parse);
        nRet = 1;
      }
    }
  }

  close(sockfd);

  return nRet;
}
#define PACKET_SIZE 64
#define MAX_WAIT_TIME 5

// 计算校验和
unsigned short calculate_checksum(unsigned short *buf, int length) {
  unsigned long sum = 0;
  for (; length > 1; length -= 2) {
    sum += *buf++;
  }
  if (length == 1) {
    sum += *(unsigned char *)buf;
  }
  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  return (unsigned short)~sum;
}

int WIFI_senficmp(char *argvip) {
  const char *target_ip = argvip;
  struct sockaddr_in target_addr;
  int sockfd;
  char send_buffer[PACKET_SIZE];
  char recv_buffer[PACKET_SIZE];
  int nRet = 0, recv_size = 0, len = 0;
  struct ip *ip_header;
  struct icmp *icmp_header;

  sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (sockfd < 0) {
    COMMONLOG_E("creat socket failed");
    return 0;
  }

  memset(&target_addr, 0, sizeof(target_addr));
  target_addr.sin_family = AF_INET;
  if (inet_pton(AF_INET, target_ip, &(target_addr.sin_addr)) <= 0) {
    COMMONLOG_E("inet_pton failed");
    return 0;
  }

  // 构造 ICMP 报文
  icmp_header = (struct icmp *)send_buffer;
  icmp_header->icmp_type = ICMP_ECHO;  // ICMP Echo Request
  icmp_header->icmp_code = 0;
  icmp_header->icmp_id = getpid();
  icmp_header->icmp_seq = 1;
  memset(icmp_header->icmp_data, 0xa5, PACKET_SIZE - sizeof(struct icmp));
  icmp_header->icmp_cksum = 0;
  icmp_header->icmp_cksum = calculate_checksum((unsigned short *)icmp_header, PACKET_SIZE);

  // 发送 ICMP 报文
  if (sendto(sockfd, send_buffer, PACKET_SIZE, 0, (struct sockaddr *)&target_addr, sizeof(target_addr)) < 0) {
    COMMONLOG_E("send ICMP packet failed");
    return 1;
  }

  // 接收 ICMP 响应
  struct timeval timeout;
  timeout.tv_sec = MAX_WAIT_TIME;
  timeout.tv_usec = 0;
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  nRet = 0;
  fd_set readfds;
  struct timeval tv;
  tv.tv_sec = 2;
  tv.tv_usec = 0; /*50毫秒*/
  FD_ZERO(&readfds);
  FD_SET(sockfd, &readfds);
  len = select(sockfd + 1, &readfds, 0, 0, &tv);
  if (len > -1) {
    if (FD_ISSET(sockfd, &readfds)) {
      recv_size = recv(sockfd, recv_buffer, PACKET_SIZE, 0);
      if (recv_size < 0) {
        COMMONLOG_W("recv ICMP packet failed");
        nRet = 0;
      } else {
        ip_header = (struct ip *)recv_buffer;
        icmp_header =
            (struct icmp *)(recv_buffer + (ip_header->ip_hl << 2));  // IP头部长度以32位字为单位，左移两位得到字节数

        if (icmp_header->icmp_type == ICMP_ECHOREPLY) {
          COMMONLOG_I("recv ICMP response from %s\n", inet_ntoa(ip_header->ip_src));
          nRet = 1;
        }
      }
    }
  }

  close(sockfd);

  return nRet;
}

bool WIFI_CheckStaConnect(void) {
  int nRet = -1;
  unsigned int ip_0 = 0;
  unsigned int ip_1 = 0;
  unsigned int ip_2 = 0;
  unsigned int ip_3 = 0;
  int ip_g = 0;
  int sec = 0;
  double msec = 0;

  bool routeConnect = false;
  unsigned char mac[7];
  unsigned char ip[5];
  char localIp[16] = {0};
  char dest[16] = {0};
  char errlog[256] = {0};
  unsigned char broad_mac[7] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00};
  char gatewayIp[64] = {0};
  char *tempIp = NULL;

  WIFI_GetGatewayIp(gatewayIp);
  if (strlen(gatewayIp) > 0) {
    tempIp = strrchr(gatewayIp, '.') + 1;
    ip_g = (int)atoi(tempIp);
  } else {
    COMMONLOG_I("+++++ Gateway can't get +++++");
    return (false);
  }

  // 以太网帧首部的硬件地址填FF:FF:FF:FF:FF:FF表示广播
  memset(mac, 0, sizeof(mac));
  memset(ip, 0, sizeof(ip));
  if (WIFI_GetLocalMac("wlan0", mac, ip) == -1) return (false);

  snprintf(localIp, sizeof(localIp), "%s", inet_ntoa(*(struct in_addr *)ip));
  sscanf(localIp, "%d.%d.%d.%d", &ip_0, &ip_1, &ip_2, &ip_3);
  COMMONLOG_I("local Mac=[%s] Ip=[%s]\n", mac_ntoa(mac), localIp);

  int i = 0;
  routeConnect = false;
  for (i = 0; i < 255; i++) {
    if (i != ip_3 && ip_g == i) {
      memset(dest, 0, sizeof(dest));
      snprintf(dest, sizeof(dest) - 1, "%d.%d.%d.%d", ip_0, ip_1, ip_2, i);

      nRet = WIFI_senficmp(gatewayIp);
      if (nRet > 0) {
        routeConnect = true;
      } else {
        nRet = WIFI_sendpkg(mac, broad_mac, ip, sizeof(ip), dest);
        if (nRet > 0) {
          routeConnect = true;
        }
      }
      break;
    }
  }

  return routeConnect;
}

/*****************************************************
 * @fn	    	WIFI_CheckNetwork
 * @brief		连接外网检测
 * @param
 * @return 	解析成功返回true，失败返回false
 ******************************************************/
bool WIFI_CheckNetwork(void) {
  pid_t pid;
  if ((pid = vfork()) < 0) {
    COMMONLOG_E("vfork error");
    return false;
  } else if (pid == 0) {
    if (execlp("ping", "ping", "-c", "1", "8.8.8.8", (char *)0) < 0) {
      COMMONLOG_E("execlp error");
      exit(1);
    }
  }
  int stat;
  waitpid(pid, &stat, 0);
  if (stat == 0) {
    return true;
  }

  return false;
}

/*****************************************************
 * @fn	    	WIFI_ApStaCheck
 * @brief		ap连接检测
 * @param
 * @return 	解析成功返回true，失败返回false
 ******************************************************/
bool WIFI_ApStaCheck(void) {
  char gatewayIp[64] = {0};
  pid_t pid;

  WIFI_GetGatewayIp(gatewayIp);
  if ((pid = vfork()) < 0) {
    COMMONLOG_E("vfork error");
    return false;
  } else if (pid == 0) {
    if (execlp("ping", "ping", "-c", "1", gatewayIp, (char *)0) < 0) {
      COMMONLOG_E("execlp error");
      exit(1);
    }
  }
  int stat;
  waitpid(pid, &stat, 0);
  if (stat == 0) {
    return true;
  }

  return false;
}

/*****************************************************
 * @fn	    	WIFI_GetWifiQuality
 * @brief		获取wifi质量参数
 * @param
 * @return 	解析成功返回0，失败返回-1
 ******************************************************/
int WIFI_GetWifiQuality(WIFI_QUALITY_S *pQuality) {
  int ret = -1, valueTmp = 0;
  char tmpBuf[512] = {0}, *p1 = NULL, *p2 = NULL, name[8] = {0}, link[8] = {0}, level[8] = {0}, noise[8] = {0},
       status[16] = {0};
  FILE *fp = 0;

  if ((fp = fopen(WIFI_QUALITY_INFO_FILE, "r")) != NULL) {
    ret = fread(tmpBuf, 1, sizeof(tmpBuf), fp);

    if ((p1 = strstr(tmpBuf, "wlan0")) != NULL) {
      sscanf(p1, "%s %s %s %s %s", name, status, link, level, noise);
      valueTmp = atoi(level);
      if (valueTmp < -100) {
        valueTmp = -100;
      } else if (valueTmp > 100) {
        valueTmp = 100;
      }
      pQuality->level = (valueTmp < 0) ? (valueTmp + 100) : valueTmp;
      pQuality->noise = atoi(noise);
      pQuality->link = atoi(link);
      COMMONLOG_I("pQuality->level:%d, pQuality->noise:%d, pQuality->link:%d!\n", pQuality->level, pQuality->noise,
                  pQuality->link);
      ret = 0;
    }
  }

  if (fp) {
    fclose(fp);
  }

  return ret;
}

int WIFI_Reload_hostapd_conf(char *m_interface, char *m_ssid, char *m_passwd) {
  char strcmd[128] = {0};

  memset(strcmd, 0, sizeof(strcmd));
  snprintf(strcmd, sizeof(strcmd) - 1, "hostapd_cli -i %s set ssid %s", m_interface, m_ssid);
  COMMONLOG_I("+++++++++++++ %s +++++++++++++", strcmd);
  system(strcmd);

  memset(strcmd, 0, sizeof(strcmd));
  snprintf(strcmd, sizeof(strcmd) - 1, "hostapd_cli -i %s set wpa_passphrase %s", m_interface, m_passwd);
  COMMONLOG_I("+++++++++++++ %s +++++++++++++", strcmd);
  system(strcmd);

  memset(strcmd, 0, sizeof(strcmd));
  snprintf(strcmd, sizeof(strcmd) - 1, "hostapd_cli reload");
  COMMONLOG_I("+++++++++++++ %s +++++++++++++", strcmd);
  system(strcmd);

  return 0;
}

int WIFI_generate_local_net_info(char *m_ssid, int ssidMaxLen, char *m_passwd, int passwdMaxLen) {
  unsigned char mac[32];
  unsigned char macHex[6] = {0};
  ble_read_mac_address(HUB_BLE_MAC_FILE, macHex, mac);
  snprintf(m_ssid, ssidMaxLen - 1, "IPC_%02hhX%02hhX", macHex[4], macHex[5]);
  snprintf(m_passwd, passwdMaxLen - 1, "123456789");
  return 0;
}

int WIFI_ifconfig_up(char *m_interface) {
  char strCmd[128] = {0};

  memset(strCmd, 0, sizeof(strCmd));
  snprintf(strCmd, sizeof(strCmd) - 1, "ifconfig %s up", m_interface);

  system(strCmd);

  return 0;
}

int WIFI_copy_wifi_conf_file(void) {
  char strCmd[128] = {0};

  if (access("/parameter/config/wifi/udhcpc.script", F_OK) != 0) {
    memset(strCmd, 0, sizeof(strCmd));
    snprintf(strCmd, sizeof(strCmd) - 1, "cp -fr /vendor/wifi/udhcpc.script /parameter/config/wifi/udhcpc.script");

    system(strCmd);
  }

  if (access("/parameter/config/wifi/wpa_action.sh", F_OK) != 0) {
    memset(strCmd, 0, sizeof(strCmd));
    snprintf(strCmd, sizeof(strCmd) - 1, "cp -fr /vendor/wifi/wpa_action.sh /parameter/config/wifi/wpa_action.sh");

    system(strCmd);
  }

  if (access("/parameter/config/wifi/run_sta.sh", F_OK) != 0) {
    memset(strCmd, 0, sizeof(strCmd));
    snprintf(strCmd, sizeof(strCmd) - 1, "cp -fr /vendor/wifi/run_sta.sh /parameter/config/wifi/run_sta.sh");

    system(strCmd);
  }

  if (access("/parameter/config/wifi/udhcpd.conf", F_OK) != 0) {
    memset(strCmd, 0, sizeof(strCmd));
    snprintf(strCmd, sizeof(strCmd) - 1, "cp -fr /vendor/wifi/udhcpd.conf /parameter/config/wifi/udhcpd.conf");

    system(strCmd);
  }

  return 0;
}

int WIFI_GetLocalIp(const char *iface, char *local_ip) {
  int sockfd;
  struct ifreq ifr;

  // 创建socket
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    COMMONLOG_E("socket error, errno:%d, eerinfo:%s", errno, strerror(errno));
    return -1;
  }

  // 准备ifreq结构体
  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
  ifr.ifr_name[IFNAMSIZ - 1] = '\0';  // 确保字符串以null终止

  // ioctl调用获取IP地址
  if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
    COMMONLOG_E("ioctl iface:%s error, errno:%d, errinfo:%s", iface, errno, strerror(errno));
    close(sockfd);
    return -1;
  }

  // 关闭socket
  close(sockfd);

  // 将获取到的IP地址复制到传入的ip参数中
  if (inet_ntop(AF_INET, &((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr, local_ip, INET_ADDRSTRLEN) == NULL) {
    COMMONLOG_E("inet_ntop error, errno:%d, eerinfo:%s", errno, strerror(errno));
    return -1;
  }

  COMMONLOG_I("local ip: %s", local_ip);

  return 0;
}

/*****************************************************
 * @fn	    	WIFI_GetSignal
 * @brief		获取WIFI信号质量
 * @param
 * @return 	解析成功返回信号值，失败返回0
 ******************************************************/
int WIFI_GetSignal(void) {
  int tmp = 0;

  // 从 /proc/net/rtl8192fu/wlan0/rx_signal 读取 signal_strength 字段
  FILE *fp = fopen("/proc/net/rtl8192fu/wlan0/rx_signal", "r");
  if (fp) {
    char line[128] = {0};
    while (fgets(line, sizeof(line), fp)) {
      // 查找 signal_strength 字段
      char *p = strstr(line, "signal_strength:");
      if (p) {
        int val = 0;
        if (sscanf(p, "signal_strength:%d", &val) == 1) {
          tmp = val;
          break;
        }
      }
    }
    fclose(fp);
  }

  return tmp;
}