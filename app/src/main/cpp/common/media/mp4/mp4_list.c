/**
 * @addtogroup module_Storage
 * @{
 */

/**
 * @file
 * @brief
 * 多媒体文件列表，列表记录视频文件开始时间、结束时间、持续时间、文件名等信息
 * @details 用于统计所有的视频文件功能
 * @version 1.0.0
 * @author sky.houfei
 * @date 2022-5-230
 */

//*****************************************************************************
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//*****************************************************************************
#include "common/log/log_conf.h"
#include "mp4_list.h"

//*****************************************************************************
/**
 * @brief 修饰字符串，参考TrimLeft函数功能实现。
 * @details 删除空格符、换行符、制表符、回车符。
 * @param[in] char* str需要修饰的字符串。
 */
void MP4_LIST_Trim(char* str) {
  int i = 0;
  int n = 0;
  uint8_t len = strlen(str);
  char trimString[128];  // 经过修改的字符串，存储剔除了特殊字符的字符串

  for (i = 0; i < len; i++) {
    while ((str[i] == '\n') || (str[i] == ' ') || (str[i] == '\t') || (str[i] == '\r')) {
      // 跳过特殊字符
      i++;
    }
    trimString[n++] = str[i];
  }
  trimString[n++] = '\0';  // 增加结束符

  // 拷贝剔除特殊字符之后的内容
  for (i = 0; i < n; i++) {
    str[i] = trimString[i];
  }
}

/**
 * @brief 写文件列表，将录像文件信息记录到文件中
 */
void MP4_LIST_SetFileList(int recordMode, char* recordFile, fileList list) {
  FILE* fp = NULL;
  char buf[128];

  if (list.durationTime == 0) {
    COMMONLOG_I("file is empty");
    return;
  }

  fp = fopen(recordFile, "a");
  if (fp == NULL) {
    printf("failed opne %s\n", recordFile);
    return;
  }

  memset(buf, 0, sizeof(buf));
  if (recordMode == 2) {
    COMMONLOG_I("file:%s, starttime:%ld, stoptime:%ld, durt:%ld pre:%u event:%d\n", list.fileName, list.startTimestamp,
                list.stopTimestamp, list.durationTime, list.preTimestamp, list.recordEventType);
    sprintf(buf, "%s,%ld,%ld,%ld,%d", list.fileName, list.startTimestamp, list.stopTimestamp, list.durationTime,
            list.recordEventType);
  } else {
    COMMONLOG_I("file:%s, starttime:%ld, stoptime:%ld, durt:%ld event:%d\n", list.fileName, list.startTimestamp,
                list.stopTimestamp, list.durationTime, list.recordEventType);
    sprintf(buf, "%s,%ld,%ld,%ld,%d", list.fileName, list.startTimestamp, list.stopTimestamp, list.durationTime,
            list.recordEventType);
  }

  fwrite(buf, strlen(buf) + 1, 1, fp);
  fwrite("\r\n", 1, 2, fp);
  fclose(fp);
  return;
}

/**
 * @brief 获取文件列表
 * @details 提取列表信息，解析出每个录像文件的名称、开始时间、结束时间信息
 * @param[in] char* recordFile, 记录每个录像信息的文件
 * @param[out] fileList list[],
 * 解析后得到的录像文件的名称、开始时间、结束时间信息
 * @return  int num 一共多少个文件录像文件
 */
int MP4_LIST_GetFileList(char* recordFile, fileList list[]) {
  FILE* fp = NULL;
  char buffer[128];
  char* cRes = NULL;
  char* result = NULL;
  char* saveptr = NULL;
  uint64_t timestamp = 0;
  int num = 0;
  int len = 0;

  fp = fopen(recordFile, "r");
  if (fp == NULL) {
    printf("Open file %s fail\n", recordFile);
    return num;
  }

  // 读取
  while (!feof(fp)) {
    // 读取一行文件
    cRes = fgets(buffer, sizeof(buffer), fp);
    if (cRes == NULL) {
      break;
    }

    len = strlen(buffer);
    if (len > 0) {
      // 字符切割，提取文件名
      result = strtok_r(buffer, LIST_DELIMITER, &saveptr);
      if (result != NULL) {
        MP4_LIST_Trim(result);
        memcpy(list[num].fileName, result, strlen(result));

        result = strtok_r(NULL, LIST_DELIMITER, &saveptr);
        MP4_LIST_Trim(result);
        timestamp = atoi(result);
        list[num].startTimestamp = timestamp;

        result = strtok_r(NULL, LIST_DELIMITER, &saveptr);
        MP4_LIST_Trim(result);
        timestamp = atoi(result);
        list[num].stopTimestamp = timestamp;
      }
    }
    num++;
  }
  if (fp) {
    fclose(fp);
    fp = NULL;
  }

  return num;
}

/**
 * @brief 获取文件 timeBase 基准时间
 * @details 根据视频文件名，解析出对应的基准时间，如视频文件为
 * 1654071711.mp4，则对应的时间为 1654071711
 * @param[in] char* fileNames 需要提取时间的文件名称
 * @param[out] int64_t timeBase 基准时间值，单位为 us
 * @return 成功返回0，失败返回-1
 */
int MP4_LIST_GetTimeBase(const char* fileName, int len, int64_t* timeBase) {
  char* result = NULL;
  int64_t timeStamp = 0;
  int ret = -1;
  char fileTemp[128];

  if (fileName == NULL) {
    printf("fileName is null\n");
    return ret;
  }

  // 复制文件名称
  memset(fileTemp, 0, sizeof(fileTemp) / sizeof(fileTemp[0]));
  strncpy(fileTemp, fileName, len);

  // 字符切割，提取文件名
  result = strtok(fileTemp, TIME_DELIMITER);
  if (result != NULL) {
    MP4_LIST_Trim(result);
    timeStamp = atoll(result);  // 获取时间戳
    printf("fileName is %s, timeStamp = %lld\n", fileTemp, timeStamp);
    *timeBase = timeStamp;  // 单位转换为 us
    ret = 0;
  }

  return ret;
}

/** @} */
