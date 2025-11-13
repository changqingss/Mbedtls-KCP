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
#ifndef MP4_LIST_H
#define MP4_LIST_H

//*****************************************************************************
#ifdef __cplusplus
extern "C" {
#endif

//*****************************************************************************
#include <stdbool.h>
#include <stdint.h>

//*****************************************************************************
#define REC_BASE_PATH "/mnt/sd/record"  // 录像文件存放的基础路径

//*****************************************************************************
#define LIST_DELIMITER ","  // 文件列表分隔符
#define TIME_DELIMITER "."  // 时间分隔符

//*****************************************************************************
/**
 * @brief 录像文件列表
 */
typedef struct fileListStruct {
  char fileName[128];            ///< 录像文件名, 此文件名包含了目录地址，如
                                 ///< /mnt/sd/video/2022/05/27/a.
  int recordEventType;           // 录像包含的事件类型
  uint32_t hubStartWriteTime;    ///< 开始时间戳，单位秒
  uint32_t eventStartTimestamp;  ///< 开始时间戳，单位秒
  uint32_t startTimestamp;       ///< 开始时间戳，单位秒
  uint32_t stopTimestamp;        ///< 结束时间戳，单位秒
  uint32_t durationTime;         ///< 录像持续时间，单位秒
  uint32_t preTimestamp;         ///< 预录时间，单位秒
  unsigned long long startUs;    ///< 开始时刻的us，单位us
  unsigned long long stopUs;     ///< 停止时刻的us，单位us
  unsigned long long wtimestamp;
} fileList;

//*****************************************************************************
/**
 * @brief 写文件列表，将录像文件信息记录到文件中
 */
void MP4_LIST_SetFileList(int recordMode, char* recordFile, fileList list);

/**
 * @brief 获取文件列表
 * @details 提取列表信息，解析出每个录像文件的名称、开始时间、结束时间信息
 * @param[in] char* recordFile, 记录每个录像信息的文件
 * @param[out] fileList list[],
 * 解析后得到的录像文件的名称、开始时间、结束时间信息
 * @return  int num 一共多少个文件录像文件
 */
int MP4_LIST_GetFileList(char* recordFile, fileList list[]);

/**
 * @brief 获取文件 timeBase 基准时间
 * @details 根据视频文件名，解析出对应的基准时间，如视频文件为
 * 1654071711.mp4，则对应的时间为 1654071711
 * @param[in] char* fileNames 需要提取时间的文件名称
 * @param[out] int64_t timeBase 基准时间值，单位为 us
 * @return 成功返回0，失败返回-1
 */
int MP4_LIST_GetTimeBase(const char* fileName, int len, int64_t* timeBase);
void MP4_LIST_Trim(char* str);

//*****************************************************************************
#ifdef __cplusplus
}
#endif

//*****************************************************************************
#endif /* MEDIA_LIST_H */

/** @} */
