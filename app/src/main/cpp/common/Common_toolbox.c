#include "Common_toolbox.h"

#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <inttypes.h>
#include <linux/ethtool.h>
#include <linux/if_ether.h>
#include <linux/rtc.h>
#include <linux/sockios.h>
#include <math.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <netdb.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h> /* offsetof */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "common/log/log_conf.h"

int CommToolbox_get_utcsec(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  return (tv.tv_sec);
}

uint64_t CommToolbox_get_utcmsec(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  return ((uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000);
}

// 设置系统时间
void CommToolbox_set_system_time(uint32_t utc_time, int32_t timezone_offset) {
  struct timeval tv;
  time_t local_time = utc_time;  // + timezone_offset;

  tv.tv_sec = local_time;
  tv.tv_usec = 0;
  printf("++++++++++++ utc_time=%lu, timezone_offset=%ld +++++++++++++\n", utc_time, timezone_offset);
  if (settimeofday(&tv, NULL) != 0) {
    perror("settimeofday");
  } else {
    printf("system time set success\n");
  }
}

int32_t CommToolbox_get_timezone_offset(void) {
  time_t now;
  struct tm local_tm, gm_tm;
  double timezone_offset;

  // 获取当前时间
  time(&now);

  // printf("+++++++++++++ now=%ld\n", (long)now);

  // 获取本地时间
  localtime_r(&now, &local_tm);
  // 获取 UTC 时间

  // 将本地时间和 UTC 时间转换为 time_t 类型以便比较
  time_t local_seconds = mktime(&local_tm);

  // 计算时区偏移时间（以秒为单位）
  timezone_offset = local_tm.tm_gmtoff;
  // printf("+++++++++++++ timezone_offset=%.0f\n", timezone_offset);

  // 将偏移时间转换为 23 位有符号整数（范围: -12 * 60 * 60 到 +14 * 60 * 60）
  if (timezone_offset < -43200 || timezone_offset > 50400) {
    fprintf(stderr, "时区偏移时间超出范围\n");
    return 0;
  }

  return (int32_t)timezone_offset;
}

/*****************************************************
 * @fn	    	CommToolbox_SetTimeZonePosix
 * @brief		设置时区包含夏令时
 * @param		pTimezone:
 * @return 	false: 未绑定 true: 已绑定
 ******************************************************/
void CommToolbox_SetTimeZonePosix(char *pTimezone) {
  if (pTimezone == NULL || strlen(pTimezone) <= 0) {
    printf("CONNECT_SetTimeZonePosix pTimezone error");
    return;
  }
  COMMONLOG_I("set time zone (%s)", pTimezone);
  setenv("TZ", pTimezone, 1);
  tzset();

  return;
}

/*****************************************************
 * @fn	    	WIFI_GetIp
 * @brief		获取ip地址
 * @param		ip
 * @return 	解析成功返回0，失败返回-1
 ******************************************************/
int CommToolbox_get_ip(const char *interface_name, char *rvip, int ipSize) {
  struct ifaddrs *ifaddr, *ifa;
  char ip[INET_ADDRSTRLEN] = {0};

  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    return -1;
  }

  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL) continue;

    if (ifa->ifa_addr->sa_family == AF_INET) {
      if (strcmp(ifa->ifa_name, interface_name) == 0) {
        struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
        inet_ntop(AF_INET, &(sa->sin_addr), ip, INET_ADDRSTRLEN);
        // printf("Interface: %s\tIP Address: %s\n", ifa->ifa_name, ip);
        snprintf(rvip, ipSize - 1, "%s", ip);
        break;
      }
    }
  }

  freeifaddrs(ifaddr);

  return 0;
}

int CommToolbox_get_mac(const char *iface, char *rvmac, int macSize) {
  struct ifreq ifr;
  int sockfd;
  unsigned char *mac;

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd == -1) {
    perror("socket");
    return 1;
  }

  strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
  if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) == -1) {
    perror("ioctl");
    close(sockfd);
    return 1;
  }

  close(sockfd);

  mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;

  snprintf(rvmac, macSize - 1, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  return 0;
}

long CommToolbox_getFileSize(const char *filename) {
  FILE *file = fopen(filename, "rb");  // 以二进制模式打开文件
  if (file == NULL) {
    perror("Cannot open file");
    return -1;
  }

  if (fseek(file, 0, SEEK_END) != 0) {  // 将文件指针移动到文件末尾
    perror("fseek failed");
    fclose(file);
    return -1;
  }

  long fileSize = ftell(file);  // 获取文件指针当前位置（即文件大小）
  if (fileSize == -1L) {        // 检查 ftell 是否失败
    perror("ftell failed");
    fclose(file);
    return -1;
  }

  fclose(file);  // 关闭文件
  return fileSize;
}

int CommToolbox_touch_newFile(const char *filePath, const char *text) {
  if (filePath == NULL || strlen(filePath) == 0) {
    fprintf(stderr, "Invalid file path\n");
    return -1;
  }

  FILE *file = fopen(filePath, "w");

  if (!file) {
    perror("could not open file");
    printf("could not open %s, %s\n", filePath, strerror(errno));
    return -1;
  }

  if (text != NULL && strlen(text) > 0) {
    if (fprintf(file, "%s", text) < 0) {
      perror("fail to write to file");
      printf("COMM_NewFile fail to write to file %s\n", strerror(errno));
      fclose(file);
      return -1;
    }
    fflush(file);
  }

  if (fclose(file) == EOF) {
    perror("Failed to close file");
    printf("Failed to close file %s\n", strerror(errno));
    return -1;
  }
  return 0;
}

int CommToolbox_creat_newDir(const char *dirPath) {
  if (mkdir(dirPath, 0777) == 0) {
    printf("%s created successfully.\n", dirPath);
  } else {
    // perror("mkdir");
    return -1;
  }

  return 0;
}

int CommToolbox_writeFile(const char *path, const char *content) {
  if (!path || !content) {
    errno = EINVAL;
    return -1;
  }

  FILE *fp = fopen(path, "wb");  // 二进制写，跨平台一致
  if (!fp) {
    perror("fopen");
    return -1;
  }

  size_t len = strlen(content);
  if (fwrite(content, 1, len, fp) != len) {
    perror("fwrite");
    fclose(fp);
    return -1;
  }

  /* fclose 会隐式 fflush，这里单独 fflush 可视场景决定要不要留 */
  if (fclose(fp) == EOF) {
    perror("fclose");
    return -1;
  }
  return 0;
}

/*
 * CJSON START
 **/
cJSON *CommToolbox_parse_cjsonObject_from_file(const char *pathstr) {
  FILE *fd = NULL;
  char *pContent = NULL;
  size_t len;

  fd = fopen(pathstr, "rb");
  if (fd == NULL) {
    printf("fopen file:%s failed", pathstr);
    return NULL;
  }

  fseek(fd, 0, SEEK_END);
  len = ftell(fd);
  fseek(fd, 0, SEEK_SET);
  pContent = (char *)calloc(len + 4, 1);
  if (pContent == NULL) {
    printf("calloc error");
    fclose(fd);
    return NULL;
  }

  if (fread(pContent, 1, len, fd) != len) {
    free(pContent);
    fclose(fd);
    printf("fread file:%s failed!", pathstr);
    return NULL;
  }
  fclose(fd);

  cJSON *root = cJSON_Parse(pContent);
  if (!root) {
    printf("Not a json string");
    free(pContent);
    return NULL;
  }
  free(pContent);

  return root;
}

char *CommToolbox_cjson_GetStringValue(const cJSON *root, const char *m_key) {
  // Check if root or key is NULL
  if (!root || !m_key) {
    printf("Invalid root or key!\n");
    return NULL;
  }

  // Get the JSON object item
  cJSON *strJson = cJSON_GetObjectItem(root, m_key);
  if (!strJson) {
    printf("cJSON_GetObjectItem(%s) error!\n", m_key);
    return NULL;
  }

  // Get the string value from the JSON object
  char *strValue = cJSON_GetStringValue(strJson);
  if (!strValue) {
    printf("%s is not a string!\n", m_key);
    return NULL;
  }

  return strValue;
}

/**
 * 解析消息头部
{
    "module": "expandHub", // 由主内机发起
    "msgType": "verQuery", // 新的消息类型
    "seq": 1755743679,
    "mac": "拓展屏的MAC",
}
 */
int CommToolbox_repeater_msg_header_parse(cJSON *header, Prepeater_header_data_s pHeaderData) {
  cJSON *module = cJSON_GetObjectItem(header, "module");
  cJSON *msgType = cJSON_GetObjectItem(header, "msgType");
  cJSON *seq = cJSON_GetObjectItem(header, "seq");
  cJSON *mac = cJSON_GetObjectItem(header, "mac");

  if (!module || !msgType || !seq || !mac) {
    printf("Invalid header format");
    return -1;
  }

  strncpy(pHeaderData->m_module_name, module->valuestring, sizeof(pHeaderData->m_module_name) - 1);
  strncpy(pHeaderData->m_msgType, msgType->valuestring, sizeof(pHeaderData->m_msgType) - 1);
  pHeaderData->m_seq = seq->valueint;
  strncpy(pHeaderData->m_mac, mac->valuestring, sizeof(pHeaderData->m_mac) - 1);

  return 0;
}

int CommToolbox_repeater_msg_header_build(cJSON *header, Prepeater_header_data_s pHeaderData) {
  if (header == NULL || pHeaderData == NULL) {
    printf("Invalid parameters");
    return -1;
  }

  cJSON_AddStringToObject(header, "module", pHeaderData->m_module_name);
  cJSON_AddStringToObject(header, "msgType", pHeaderData->m_msgType);
  cJSON_AddNumberToObject(header, "seq", pHeaderData->m_seq);
  cJSON_AddStringToObject(header, "mac", pHeaderData->m_mac);

  return 0;
}

/*
 * CJSON END
 **/

#define MAX_LINE_LENGTH 1024

void CommToolbox_replaceLineWithString(const char *filename, const char *search_string, const char *new_content,
                                       const char *ignor_content, const char *ck_cont) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    perror("Error opening file");
    return;
  }

  FILE *temp_fp = fopen("/parameter/wifi/temp.conf", "w");
  if (temp_fp == NULL) {
    perror("Error opening temporary file");
    fclose(fp);
    return;
  }

  char buffer[MAX_LINE_LENGTH];
  int found = 0;
  int sameSsid = 0;

  while (fgets(buffer, sizeof(buffer), fp) != NULL) {
    // Check if the search_string is found in the current line
    if (strstr(buffer, search_string) != NULL && strstr(buffer, ck_cont) == NULL) {
      if (ignor_content != NULL && strstr(buffer, ignor_content) == NULL) {
        fprintf(temp_fp, "%s\n", new_content);
        found = 1;
      } else if (ignor_content == NULL) {
        fprintf(temp_fp, "%s\n", new_content);
        found = 1;
      }
    } else {
      if (strstr(buffer, ck_cont) != NULL) {
        sameSsid = 1;
      }
      fprintf(temp_fp, "%s", buffer);
    }
  }

  fclose(fp);
  fclose(temp_fp);

  // Replace the original file with the modified file
  if (found) {
    if (remove(filename) != 0) {
      perror("Error deleting the original file");
    } else if (rename("/parameter/wifi/temp.conf", filename) != 0) {
      perror("Error renaming the temporary file");
    } else {
      printf("Line replaced successfully.\n");
    }
  } else if (sameSsid) {
    remove("temp.conf");
    printf("ssid '%s' found in the file.\n", ck_cont);
  } else {
    remove("temp.conf");
    printf("String '%s' not found in the file.\n", search_string);
  }
}

// 提取指定字符最后一次出现后面的子字符串
char *CommToolbox_extract_after_last(const char *s, char delimiter) {
  // 查找分隔符最后一次出现的位置
  const char *last_occurrence = strrchr(s, delimiter);

  // 如果没有找到分隔符，则返回 NULL 或原字符串
  if (!last_occurrence) {
    return NULL;
  }

  // 返回分隔符最后一次出现后面的子字符串
  return (char *)(last_occurrence + 1);
}
