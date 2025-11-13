/***********************************************************************************
 * 文 件 名   : CommonBase.h
 * 负 责 人   : xwq
 * 创建日期   : 2024年5月4日
 * 文件描述   : 公共头文件
 * 版权说明   : Copyright (c) 2015-2024  SWITCHBOT SHENZHEN IOT
 * 字符编码   : UTF-8
 * 修改日志   :
 ***********************************************************************************/

#ifndef __COMMONBASE_H__
#define __COMMONBASE_H__
/*----------------------------------------------*
 * 包含头文件                                           *
 *----------------------------------------------*/
#include <nng/nng.h>
#include <stdint.h>

/*----------------------------------------------*
 * 宏定义                              *
 *----------------------------------------------*/
#define MAX_APP_PROC 5

#define APP_MAX_SUB (MAX_APP_PROC - 1)

#define MAIN_APP_ID 0
#define HAL_APP_ID 1
#define MEDIA_APP_ID 2
#define KVS_APP_ID 3
#define LVGL_APP_ID 4

#define TALK_STATUS_OFF 0       // 对讲关闭
#define TALK_STATUS_ON_APP 1    // APP    对讲开启
#define TALK_STATUS_ON_3RD 2    // 第三方 对讲开启
#define TALK_STATUS_ON_SLAVE 3  // SLAVE 对讲开启

typedef enum {
  STREAM_TYPE_HD = 0,
  STREAM_TYPE_SD = 1,
  STREAM_TYPE_AUTO = 2,
  STREAM_TYPE_AUDIO = 3,
  STREAM_TYPE_MAX = 4,
} STREAM_TYPE;

#define PROXY_IPC_FRONT_URL "ipc:///tmp/front.ipc"
#define PROXY_IPC_BACK_URL "ipc:///tmp/back.ipc"

#define CAM_BIND_FIXED_IP "0.0.0.0"

#define PROXY_TCP_FRONT_URL "tcp://" CAM_BIND_FIXED_IP ":5533"
#define PROXY_TCP_BACK_URL "tcp://" CAM_BIND_FIXED_IP ":5534"

#define CAM_DEF_IP "222.111.112.100"
#define CAM_BIND_HUB_IP "222.111.112.1"
#define PROXY_HUB_FRONT_URL "tcp://" CAM_BIND_HUB_IP ":5533"
#define PROXY_HUB_BACK_URL "tcp://" CAM_BIND_HUB_IP ":5534"

#define CAM_PAIR_PORT 60000

#define CAM_KCP_CONV 0x1000;

#define TOPIC_HEAD_HUB "hub"
#define TOPIC_HEAD_BATCAM "batteryCam"

#define TOPIC_HEAD_MAIN "main"
#define TOPIC_HEAD_HAL "hal"
#define TOPIC_HEAD_MEDIA "media"
#define TOPIC_HEAD_KVS "kvs"
#define TOPIC_HEAD_LVGL "lvgl"
#define TOPIC_HEAD_QUICK_REPLY "quick_reply"

/* 休眠、唤醒 */
#define TOPIC_HEAD_WAKEUP_SLLEP "wakeupSleep"
/* 心跳 */
#define TOPIC_HEAD_HEATBEAT "heatbeat"

/* lvgl 按键对讲 */
#define TOPIC_LVGL_BUTTON_TALK "lvglButtonTalk"

/* 按键事件 */
#define METHOD_BUTTON "button"
/* BLE事件 */
#define METHOD_BLE "ble"
/* MQTT控制消息 */
#define METHOD_MQTT "mqtt"
/* SDP控制消息 */
#define METHOD_SDP "sdp"
/* 参数设置/获取 */
#define METHOD_PARAMS "params"
/* 控制 */
#define METHOD_CTRL "ctrl"
/* 通知 */
#define METHOD_NOTIFY "notify"
/* 上报，事件/参数配置 */
#define METHOD_REPORT "report"
/* 流状态 */
#define METHOD_STREAM "stream"
/* 同步参数配置 */
#define METHOD_SYNC "sync"
/* 同步时间 */
#define METHOD_SYNC_TIME "timeSync"
/* 正在直播 */
#define METHOD_LIVE_STREAM "liveStream"
/* 外机唤醒 */
#define METHOD_CAM_WAKEUP "wakeup"
/*同步设置给到外机*/
#define METHOD_CAM_SYNC_PARAM "syncParam"
/*同步设置给到外机*/
#define METHOD_CAM_REPORT_PARAM "reportParam"

/*保持外机连接*/
#define METHOD_KEEP_LIVE "keepLive"

#define METHOD_KVS_TALK "kvsTalk"

#define METHOD_KVS_CLOUD_ENDTIME "kvsCloudEndTime"

#define METHOD_RECORD_KEPP_ALVIE "recordKeepAlive"

/* 同步显示参数 */
#define METHOD_SYNC_DISPLAY_PARAM "displayParams"

/* 设备启动 */
#define METHOD_BOOTUP "bootup"

/* 设备关机 */
#define METHOD_POWER_OFF "poweroff"

/* 内机锁状态 */
#define MATHOD_HUB_LOCK_STATE "hubLockState"

/* 内机控锁 */
#define MATHOD_HUB_LOCK_CONTROL "hubLockControl"

/* 内机解锁结果 */
#define MATHOD_HUB_UNLOCK_EVENT "unlock_event"

/* 内机格式化sd卡 */
#define MATHOD_FORMAT "format"

/* 内机控灯 */
#define MATHOD_HUB_LED_CONTROL "hubLedContorl"

/* 事件录像信息通知 */
#define METHOD_EVENT_RECORD_INFO_NOTIFY "eventRecordInfo"

/* 播放快捷回复音频文件 */
#define METHOD_PLAY_QUICK_REPLY "play"

/* 更新快捷回复配置 */
#define METHOD_UPDATE_QUICK_REPLY "update"

/* 产测通知 */
#define MATHOD_FACTORY_NOTIFY "factoryNotify"

/* 内机状态查询 */
#define MATHOD_STATU_QUERY "statusQuery"

/* 内机LVGL OTA状态通知 */
#define MATHOD_OTA_UPGRADE "upgrade"

/* 设置配网模式 */
#define METHOD_CONFIGURATION_NETWORK "configure_network"

/* 外机ota结果上报 */
#define METHOD_CAM_OTA_REPORT "subOtaReport"

/* 外机ota请求 */
#define METHOD_CAM_OTA_REQ "subOtaReq"

/* 断开webrtc连接 */
#define METHOD_DISCONNCT_WEBRTC "disconnectWebrtc"

/* 获取RTSP配置请求 */
#define METHOD_GET_RTSP_CONFIG_REQ "getRtspConfigReq"

/* 获取RTSP配置响应 */
#define METHOD_GET_RTSP_CONFIG_RESP "getRtspConfigResp"

/* 内机对讲状态 */
#define METHOD_HUB_TALK_STATUS "hubTalkStatus"

/* 获取报警状态 */
#define METHOD_HUB_ALARM_STATUS "displayParams"

// 拓展器RingBuffer ID
#define REPEATER_RINGBUFFER_ID "987654321"

/*----------------------------------------------*
 * 外部变量说明                 *
 *----------------------------------------------*/
extern char *pubsuburls[MAX_APP_PROC];
extern char *surveyurls[MAX_APP_PROC];
extern char *proctopic[MAX_APP_PROC];

/*----------------------------------------------*
 * 全局变量                        *
 *----------------------------------------------*/

struct MODULE_COMMON {
  int (*module_open)(void *ctx_t);
  int (*module_close)(void *ctx_t);

  int (*module_start)(void *ctx_t);  // resume
  int (*module_stop)(void *ctx_t);   // pause

  int (*module_exit)(void *ctx_t);

  int (*module_get_error)(void *ctx_t);
};

// 定义回调函数指针类型
typedef void (*subMsgCallback)(void *, char *, int);

// 定义回调函数指针类型
typedef int (*msgHandleCallback)(void *, int, char *, int);

typedef struct COMMON_TOPIC {
  char method[32];
  char str[128];
} COMMON_TOPIC_S;

// 存储回调函数的结构体
typedef struct {
  msgHandleCallback msgHandleCb;  // 回调函数指针数组
  COMMON_TOPIC_S topic;           // 回调函数数量
} topicCallbackManager;

typedef struct {
  topicCallbackManager *topicCb;  // 回调函数指针数组
  int count;                      // 回调函数数量
  int capacity;                   // 回调函数指针数组的容量
} TopicManager;

typedef struct COMMON_PUB_CONF {
  uint8_t pubconnected; /* only use to proxy */

  char puburl[128];
  char pubTopic[64];
  nng_socket pubsock;
  TopicManager *topicManager;
} COMMON_PUB_CONF_S;

typedef struct COMMON_SUB_CONF {
  uint8_t g_subexit;
  uint8_t EnaioCb;
  uint8_t aioCb_exit;
  uint8_t subconnected;
  char suburl[128];
  char subTopic[64];

  nng_socket subsock;
  nng_aio *subaio;
  subMsgCallback submsgCb;
  TopicManager *topicManager;
} COMMON_SUB_CONF_S;

typedef struct COMMON_SURVEY_CONF {
  char surveyurl[128];
  char surveyTopic[64];

  nng_socket surverysock;
  nng_socket surveryrecvsock;
} COMMON_SURVEY_CONF_S;

typedef struct COMMON_SURVEY_RESP_CONF {
  char respurl[128];
  char respTopic[64];

  nng_socket respsock;
  nng_aio *surveyaio;
} COMMON_SURVEY_RESP_CONF_S;

typedef struct COMMON_APP_CONF {
  uint8_t g_exit;  // 录像模式 0:关闭录像 1:开启录像

  COMMON_PUB_CONF_S proxypub_conf;
  COMMON_SUB_CONF_S proxysub_conf;

} COMMON_APP_CONF_S;

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

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __COMMONBASE_H__ */
