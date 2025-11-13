/***********************************************************************************
 * 文 件 名   : media_slice_packet.h
 * 负 责 人   : xwq
 * 创建日期   : 2024年1月15日
 * 文件描述   : media_slice_packet.c 的头文件
 * 版权说明   : Copyright (c) 2015-2024  SWITCHBOT SHENZHEN IOT
 * 字符编码   : UTF-8
 * 修改日志   :
 ***********************************************************************************/

#ifndef __MEDIA_SLICE_PACKET_H__
#define __MEDIA_SLICE_PACKET_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */
/*----------------------------------------------*
 * 包含头文件                                           *
 *----------------------------------------------*/
#include <stdint.h>

/*----------------------------------------------*
 * 宏定义                              *
 *----------------------------------------------*/

#define DATA_MAX_MTU_SIZE (1376)

#define FRAME_HEAD_IDENTIFIER 0xABCD
#define FRAME_TAIL_IDENTIFIER 0xDCBA

#define TAIL_FRAME_HEAD_IDENTIFIER 0xBA98
#define TAIL_FRAME_TAIL_IDENTIFIER 0x89ab

/*----------------------------------------------*
 * 外部变量说明                 *
 *----------------------------------------------*/
#define SLICE_FREAM_VERSION "V1.0"
/*----------------------------------------------*
 * 全局变量                        *
 *----------------------------------------------*/

typedef enum {
  FREAM_ENCODE_H265 = 0,
  FREAM_ENCODE_H264,
  FREAM_ENCODE_AAC,
  FREAM_ENCODE_G711A,
  FREAM_ENCODE_PCM,
  FREAM_ENCODE_JPG
} FREAM_ENCODE_TYPE_E;

typedef enum {
  FREAM_TYPE_VIDEO_I = 0,
  FREAM_TYPE_VIDEO_P,
  FREAM_TYPE_AUDIO,
  FREAM_TYPE_HEARTBEAT,
  FREAM_TYPE_PING,
  FREAM_TYPE_PONG,
  FREAM_TYPE_CTRL,
  FREAM_TYPE_FILE,
  FREAM_TYPE_REPEATER
} FREAM_TYPE_E;

typedef enum {
  FRAME_CTRL_TYPE_PUSHPLAY,  // 首次门铃

  FRAME_CTRL_TYPE_BOOTUP_REQ,  // bootup
  FRAME_CTRL_TYPE_BOOTUP_RESP,

  FRAME_CTRL_TYPE_SYNC_TIME_REQ,  // sync time, resp共用bootup resp

  FRAME_CTRL_TYPE_STATU_QUERY_REQ,  // statuquery
  FRAME_CTRL_TYPE_STATU_QUERY_RESP,

  FRAME_CTRL_TYPE_RING_EVENT_INFO,  // 门铃

  FRAME_CTRL_TYPE_POWER_OFF,  // 内机控制休眠

  FRAME_CTRL_TYPE_SYNC_PARAMS_RESP,  // 同步参数

  FRAME_CTRL_TYPE_SYNC_PARAMS_REPORT,  // 上报参数

  FRAME_CTRL_TYPE_PLAYRING,  // 非首次门铃

  FRAME_CTRL_TYPE_OTA_REPORT,     // OTA上报
  FRAME_CTRL_TYPE_OTA_REQ,        // OTA请求
  FRAME_CTRL_TYPE_SET_TIME_REQ,   // 设置时间请求
  FRAME_CTRL_TYPE_SET_TIME_RESP,  // 设置时间请求

  FRAME_CTRL_TYPE_CAM_AUTO_REPLY_LIST,  // 外机自动回复列表

  FRAME_CTRL_TYPE_CAM_AUTO_REPLY_UPDATE,  // 外机自动回复更新音频文件

  FRAME_CTRL_TYPE_KEEP_LIVE,  // 保活

  FRAME_CTRL_TYPE_LIGHT_ENABLE,  // 补光灯开关

  FRAME_CTRL_TYPE_MAX
} FRAME_CTRL_TYPE;

typedef enum {
  FRAME_CTRL_TYPE_REPEATER_JOIN = 0,
  FRAME_CTRL_TYPE_REPEATER_LEAVE,
  FRAME_CTRL_TYPE_REPEATER_PARAMS,
} FRAME_CTRL_TYPE_REPEATER_E;

typedef struct _frameTailInfo_ {
  uint16_t m_headidentifier;
  int m_fragmentLen;
  unsigned short m_fragmentIndex;
  uint16_t m_tailidentifier;
} frameTailInfo_s;

typedef enum {
  AUDIO_TYPE_IDLE = 0,             // 空闲状态
  AUDIO_TYPE_QUICK_REPLY = 1,      // 快捷回复
  AUDIO_TYPE_TALK_HUB_MASTER = 2,  // 主内机对讲
  AUDIO_TYPE_TALK_APP = 3,         // APP对讲
  AUDIO_TYPE_TALK_3RD = 4,         // 第三方对讲
  AUDIO_TYPE_TALK_HUB_SLAVE = 5,   // 子内机对讲
} AUDIO_TYPE_E;

typedef struct _frameInfo_ {
  uint16_t m_headidentifier;
  unsigned short m_frameType;      /* I freme | P freme | audio | jpg */
  unsigned short m_EncodeType;     /* h265 | h264 | aac | g711a | pcm */
  unsigned short m_width;          /* 分辨率 */
  unsigned short m_height;         /* 分辨率 */
  unsigned short m_frameRate;      /* 帧率，这个字段会被用于区分控制类型*/
  unsigned short m_frameGop;       /* gop长度， 这个字段会被用于是否加密*/
  unsigned int m_freamSize;        // 8 + 8 + 4 + 4*8 = 32 + 16 + 4
  unsigned long long m_frameIndex; /* 当前帧索引 */
  unsigned long long m_frmPts;  // 帧时间戳. 内机发送给外机的对讲时用来作为对讲数据类型 AUDIO_TYPE_E
  unsigned long long m_utcPts;
  unsigned short m_totalfragment;
  unsigned short m_fragmentIndex;
  uint16_t m_tailidentifier;
} frameInfo_s;

typedef struct _sliceDataHeader_ {
  frameInfo_s frameInfo;
  unsigned short m_sliceTotalNum; /* 切片的总数 */
  unsigned short m_sliceIndex;    /* 当前切片索引 */
  unsigned short m_packetLen;     /* 当前切片大小 */
} sliceDataHeader_s;

/*----------------------------------------------*
 * 常量定义                                          *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 内部函数原型说明                                *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 外部函数原型说明                             *
 *----------------------------------------------*/

extern int media_data_slice_unpack(frameInfo_s* frameInfo, char* srcdata, int srcLen, char** destData, int* destLen);
extern int media_data_slice_pack(frameInfo_s* frameInfo, char* srcdata, int srcLen, char** destData, int* destLen,
                                 int* packetNum);
extern const char* kcp_fream_type_to_string(FREAM_TYPE_E type);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __MEDIA_SLICE_PACKET_H__ */
