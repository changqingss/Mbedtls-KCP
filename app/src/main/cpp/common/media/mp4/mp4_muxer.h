/**
 * @addtogroup module_Storage
 * @{
 */

/**
 * @file
 * @brief MP4 编码器功能
 * @details 用于MP4文件编码操作
 * @version 1.0.0
 * @author sky.houfei
 * @date 2022-5-12
 */

//*****************************************************************************
#ifndef MP4_MUXER_H_
#define MP4_MUXER_H_

//*****************************************************************************
#ifdef __cplusplus
extern "C" {
#endif

//*****************************************************************************
#include <ctype.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

//*****************************************************************************
typedef enum MP4_TYPE { MP4_TYPE_G711A_H264 = 0, MP4_TYPE_AAC_H264, MP4_TYPE_AAC_H265 } MP4_TYPE_E;

typedef enum FRAMETYPE_e {
  FRAME_AUDIO,    ///< 音频帧
  FRAME_VIDEO_I,  ///< 视频关键帧
  FRAME_VIDEO     ///< 视频普通帧
} FRAMETYPE_e;

//*****************************************************************************
typedef struct AVRecord_t {
  AVFormatContext* m_fmtCtx;  ///< 视频文件上下文
  AVStream* pVStream;         ///< 视频流
  AVStream* pAStream;         ///< 音频流
  int64_t startTimestamp;     ///< 开始录制视频的起始时间
} AVRecord_t;

//*****************************************************************************
int MP4_MUXER_Open(struct AVRecord_t* pRecord, const char* pFileName);
int MP4_MUXER_Close(struct AVRecord_t* pRecord);
int MP4_MUXER_WriteFrame(struct AVRecord_t* pRecord, enum FRAMETYPE_e type, unsigned char* buf, int64_t timestamp,
                         unsigned int size, int muteFlag);
int MP4_MUXER_SetMute(int val);

struct mp4Muxer_struct {
  char* name;
  int (*mp4Muxer_open)(struct AVRecord_t* pRecord, const char* pFileName);
  int (*mp4Muxer_close)(struct AVRecord_t* pRecord);
  int (*mp4Muxer_write)(struct AVRecord_t* pRecord, enum FRAMETYPE_e type, unsigned char* buf, int64_t timestamp,
                        unsigned int size, int muteFlag);
};

// extern struct mp4Muxer_struct bal_mp4Muxer;

void* bal_mp4Muxer_new(void);
//*****************************************************************************
#ifdef __cplusplus
}
#endif

//*****************************************************************************
#endif /* MP4_MUXER_H_ */

/** @} */
