
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "BLE-MAC"
#include "common/log/log_conf.h"

extern int WIFI_SetBleMac(const char *macHex);

// 将 MAC 地址保存为字符串格式
int ble_save_mac_address(const char *filename, const unsigned char *macHex) {
  if (NULL == filename || NULL == macHex) {
    COMMONLOG_E("param is invalid");
    return -1;
  }

  FILE *file = fopen(filename, "w");
  if (file == NULL) {
    COMMONLOG_E("Failed to open file for writing");
    return -1;
  }

  // 将 MAC 地址格式化为字符串并写入文件
  fprintf(file, "%02hhX %02hhX %02hhX %02hhX %02hhX %02hhX\n", macHex[0], macHex[1], macHex[2], macHex[3], macHex[4],
          macHex[5]);
  fclose(file);

  WIFI_SetBleMac(macHex);

  return 0;
}

// 从文件中读取 MAC 地址并存储到数组中
int ble_read_mac_address(const char *filename, unsigned char *macHex, unsigned char *macStr) {
  if (filename == NULL || NULL == macHex || NULL == macStr) {
    COMMONLOG_E("param is invalid");
    return -1;
  }

  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    COMMONLOG_E("Failed to open file for reading");
    return -1;
  }

  if (fscanf(file, "%02hhX %02hhX %02hhX %02hhX %02hhX %02hhX", &macHex[0], &macHex[1], &macHex[2], &macHex[3],
             &macHex[4], &macHex[5]) != 6) {
    perror("Failed to read MAC address from file");
    fclose(file);
    return -1;
  }

  sprintf(macStr, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX", macHex[0], macHex[1], macHex[2], macHex[3], macHex[4],
          macHex[5]);

  fclose(file);

  return 0;
}
