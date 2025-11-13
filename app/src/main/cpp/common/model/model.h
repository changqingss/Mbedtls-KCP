#ifndef __MODEL_H__
#define __MODEL_H__

#include <cjson/cJSON.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MODEL_SAVE_VALUE_INT(dst, src) \
  do {                                 \
    dst = *(int32_t *)src;             \
  } while (0);

#define MODEL_SAVE_VALUE_BOOL(dst, src) \
  do {                                  \
    dst = *(int32_t *)src;              \
  } while (0);

#define MODEL_SAVE_VALUE_STRING(dst, src)       \
  do {                                          \
    strncpy(dst, (char *)src, sizeof(dst) - 1); \
    dst[sizeof(dst) - 1] = '\0';                \
  } while (0);

#define MODEL_SAVE_VALUE_JSON(dst, object_json, size) \
  do {                                                \
    ptr = cJSON_Print(object_json);                   \
    if (ptr) {                                        \
      if (strlen(ptr) < (size)) {                     \
        strncpy(dst, ptr, (size));                    \
        dst[(size)-1] = '\0';                         \
      }                                               \
    }                                                 \
  } while (0);

typedef enum DOORBELL_CAMERA_STATE {
  DOORBELL_CAMERA_STATE_IDLE,  // 空闲
  DOORBELL_CAMERA_STATE_BUSY,  // 占用
} DOORBELL_CAMERA_STATE;

// https://ihqz5dyhwa.feishu.cn/wiki/F7iLwN5fAiVaQ4ksyCDcobMGn3d
#define FUNCTION_ID_SET_PROPERTY (1)  // 物模型设置属性FunctionID

// https://ihqz5dyhwa.feishu.cn/wiki/wikcngCS4i8jFwhDzKONzcEWzvd
#define MODEL_CODE_REPORT_PROPERTY (1)  // 物模型属性变更上报
#define MODEL_CODE_RESPONSE (4)         // 物模型回复

#define MAX_DOORBELL_CAM_NUM (16)

// 外机的物模型定义(属性), https://ihqz5dyhwa.feishu.cn/wiki/PjgcwKdy0i1uBek6QyfcrigSneb
typedef enum DOORBELL_OBJECT_MODEL {
  DOORBELL_OBJECT_MODEL_ONLINE = 66,            // 在线/离线状态(int), 0：离线， 1：在线
  DOORBELL_OBJECT_MODEL_BLE_FW = 802,           // 蓝牙固件版本号
  DOORBELL_OBJECT_MODEL_BATTERY_LEVEL = 820,    // 电量(int), 0~100
  DOORBELL_OBJECT_MODEL_BLE_MAC = 847,          // BLE MAC(string)
  DOORBELL_OBJECT_MODEL_FIRMWAREVERSION = 853,  // 固件版本(string)
  DOORBELL_OBJECT_MODEL_UPGRADESTATUS = 855,    // 升级状态(string)
  DOORBELL_OBJECT_MODEL_UPGRADEPROGRESS = 856,  // 升级进度(int)
  DOORBELL_OBJECT_MODEL_RING_TONE = 8402,       // 门铃音效(int), 可选值：1/2/3
  DOORBELL_OBJECT_MODEL_RING_VOLUME = 8403,     // 门铃音量(int), 0~10
  DOORBELL_OBJECT_MODEL_LIGHT_ENABLED = 8405,   // 补灯光开关(bool)
  DOORBELL_OBJECT_MODEL_MONITOR_ID = 8406,      // 绑定的内机(string), 内机的蓝牙mac
  DOORBELL_OBJECT_MODEL_DETECTION_ZONE = 8407,  // 侦测区域(json), {"x": 0, "y": 0, "width": 80, "height": 80}
  DOORBELL_OBJECT_MODEL_MOTION_DETECTION_ENABLED = 8408,  // 移动侦测开关(bool)
  DOORBELL_OBJECT_MODEL_DETECTION_TYPE = 8409,  // 侦测类型(json), 可选值：["human"] / ["human", "all"] / ["all"]
  DOORBELL_OBJECT_MODEL_DETECTION_SENSITIVITY = 8410,      // 侦测灵敏度(int), 1~10
  DOORBELL_OBJECT_MODEL_DETECTION_SCHEDULES = 8411,        // 侦测时段(json), 待定
  DOORBELL_OBJECT_MODEL_MAX_VIDEO_CLIP_LENGTH = 8412,      // 最大录制时长(int), 单位秒，10~60
  DOORBELL_OBJECT_MODEL_TRIGGER_INTERVAL = 8413,           // 侦测间隔(int), 单位秒，5~60
  DOORBELL_OBJECT_MODEL_INDICATOR_LIGHT_ENABLED = 8418,    // 状态指示灯开关(bool)
  DOORBELL_OBJECT_MODEL_NIGHT_VISION_MODE = 8419,          // 夜视模式(int), 可选值：0.auto 1.off 2.on
  DOORBELL_OBJECT_MODEL_NIGHT_VISION_DISPLAY_MODE = 8420,  // 夜视画面(int), 可选值：0.黑白 1.彩色
  DOORBELL_OBJECT_MODEL_DISMANTLING_ALARM_ENABLED = 8421,  // 拆除警报(bool)
  DOORBELL_OBJECT_MODEL_ANTI_FLICKER_MODE = 8422,          // 视频显示抗闪烁(int), 0. 50HZ 1. 60HZ
  DOORBELL_OBJECT_MODEL_TIME_WATERMARK_ENABLED = 8423,     // 时间水印开关(bool)
  DOORBELL_OBJECT_MODEL_LOGO_WATERMARK_ENABLED = 8424,     // logo水印开关(bool)
  DOORBELL_OBJECT_MODEL_FLIP_SCREEN_ENABLED = 8425,        // 画面翻转开关(bool)
  DOORBELL_OBJECT_MODEL_TALK_VOLUME = 8426,                // 对讲音量(int)
  DOORBELL_OBJECT_MODEL_LOCKBUTTONSETTINGS =
      8427,  // 上锁键设置(json), {"mode": 1, "effectiveDuration":
             // 120}，mode: 1.禁用，2.永久启用，3.非永久启用；effectiveDuration：关门后多少秒内会生效
  DOORBELL_OBJECT_MODEL_LOCK_ID = 8428,               // 锁的设备ID(string)
  DOORBELL_OBJECT_MODEL_WIFI_SSID = 8429,             // wifi热点的ssid(string)
  DOORBELL_OBJECT_MODEL_WIFI_PWD = 8430,              // wifi热点的pwd(string)
  DOORBELL_OBJECT_MODEL_LAST_EVENT_TIME = 8431,       // 最近事件时间(int)
  DOORBELL_OBJECT_MODEL_LAST_EVENT_TYPE = 8432,       // 最近事件类型(string)
  DOORBELL_OBJECT_MODEL_SNOOZE_MODE_END_TIME = 8433,  // 休眠模式的结束时间(string)
  DOORBELL_OBJECT_MODEL_KEYPAD_DISABLED = 8434,       // 键盘禁用开关(bool)
  DOORBELL_OBJECT_MODEL_BACK_LIGHT_ENABLED = 8435,    // 门铃背光开关 backlightEnabled(bool)
  DOORBELL_OBJECT_MODEL_SOUNDENABLED = 8436,          // 提示音开关 soundEnabled(bool)
  DOORBELL_OBJECT_MODEL_TALKVOICE = 8437,             // 对讲音色 talkingVoice(string)
  DOORBELL_OBJECT_MODEL_CUSTOM_QUICK_REPLIES =
      8440,  // 用户自定义快捷回复列表 customQuickReplies, [{"id": "c1e8feb5-e7e1-4330-b153-98c9db49b594", "title":
             // "Hello", "audioPath": ""}, {"id": "d23d5689-bc0a-4606-bb77-9e74cfaba5a8", "title": "World", "audioPath":
             // ""}]

  DOORBELL_OBJECT_MODEL_DETECT_ALARM_ENABLED = 8441,        // 侦测报警开关(bool)
  DOORBELL_OBJECT_MODEL_DETECT_ALARM_DURATION = 8442,       // 侦测报警持续时长(int)
  DOORBELL_OBJECT_MODEL_DETECT_ALARM_TONE = 8443,           // 侦测报警音效(int)
  DOORBELL_OBJECT_MODEL_DETECT_ALARM_VOLUME = 8444,         // 侦测报警音量(int)
  DOORBELL_OBJECT_MODEL_KEYBOARD_SOUND_ENABLED = 8447,      // 键盘提示音(bool)
  DOORBELL_OBJECT_MODEL_EMERGENCY_PASSCODE_USED = 8448,     // 紧急密码被触发(bool)
  DOORBELL_OBJECT_MODEL_EMERGENCY_FINGERPRINT_USED = 8449,  // 紧急指纹被触发(bool)
  DOORBELL_OBJECT_MODEL_PASSCODE_DISABLED = 8450,           // 密码被禁用(bool)
  DOORBELL_OBJECT_MODEL_FINGERPRINT_DISABLED = 8451,        // 指纹被禁用(bool)
  DOORBELL_OBJECT_MODEL_NFC_CARD_DISABLED = 8452,           // NFC卡被禁用(bool)
  DOORBELL_OBJECT_MODEL_WAS_REMOVED = 8453,                 // 被拆除(bool)
  DOORBELL_OBJECT_MODEL_CHARGING_STATUS =
      8455,  // 充电状态(int), 0: 未充电，1: 适配器充电，2: 太阳能板充电，3: 适配器连接已充满，4:
             // 太阳能板连接已充满，5:未充满状态下太阳能板连接未充电；6:硬件错误
  DOORBELL_OBJECT_MODEL_RECORDING_ENABLED = 8456,             // 录像开关(bool)
  DOORBELL_OBJECT_MODEL_MUTED_RECORDING_ENABLED = 8457,       // 录像静音开关(bool)
  DOORBELL_OBJECT_MODEL_DETECTION_ALARM_MONITOR_TONE = 8458,  // 侦测报警显示器音效(int)
  DOORBELL_OBJECT_MODEL_IS_WAKEUP = 8459,                     // 外机是否唤醒

  DOORBELL_OBJECT_MODEL_EMERGENCY_PASSCODE_CYCLE_NUM = 8461,     // 紧急密码循环次数(int)
  DOORBELL_OBJECT_MODEL_EMERGENCY_FINGERPRINT_CYCLE_NUM = 8462,  // 紧急指纹循环次数(int)
  DOORBELL_OBJECT_MODEL_EMERGENCY_CREDENTIAL_USED_NUM = 8464,    // 使用的紧急凭证emergencyCredentialUsed
  DOORBELL_OBJECT_MODEL_WIFI_INFO = 8465,                        // 外机的wifi信息(信号强度和信道), json
  DOORBELL_OBJECT_MODEL_AUTO_REPLY_ENABLED = 8466,               // 自动回复开关 autoReplyEnabled(bool)
  DOORBELL_OBJECT_MODEL_AUTO_REPLY_ID = 8467,                    // 自动回复ID autoReplyID(int)

  DOORBELL_OBJECT_MODEL_IS_AWAKE = 8459,  // 是否唤醒 isAwake

  DOORBELL_OBJECT_MODEL_RING_EVENT = 8500,                     // 响铃事件(json)
  DOORBELL_OBJECT_MODEL_HUMAN_MOVE_EVENT = 8501,               // 人移动事件(json)
  DOORBELL_OBJECT_MODEL_MOTION_DETECT_EVENT = 8502,            // 移动侦测事件(json)
  DOORBELL_OBJECT_MODEL_DISMANTLE_EVENT = 8503,                // 设备被拆除事件(json)
  DOORBELL_OBJECT_MODEL_UNLOCK_EVENT = 8504,                   // 解锁事件(json)
  DOORBELL_OBJECT_MODEL_LOCK_EVENT = 8505,                     // 上锁成功事件(json)
  DOORBELL_OBJECT_MODEL_EVENT_RECORD_END_EVENT = 8506,         // 事件录像结束事件 eventRecordingEnded
  DOORBELL_OBJECT_MODEL_VERIFICATION_FAIL_EVENT = 8507,        // 验证失败事件 verificationFailed
  DOORBELL_OBJECT_MODEL_PASSWORD_UNLOCK_DISABLE_EVENT = 8508,  // 密码解锁被禁用事件 passwordUnlockDisabled
  DOORBELL_OBJECT_MODEL_FINGER_UNLOCK_DISABLE_EVENT = 8509,    // 指纹解锁被禁用事件 fingerprintUnlockDisabled
  DOORBELL_OBJECT_MODEL_NFC_UNLOCK_DISABLE_EVENT = 8510,       // NFC卡解锁被禁用事件 nfcCardUnlockDisabled
  DOORBELL_OBJECT_MODEL_LOCKFAILED_EVENT = 8511,               // 上锁失败事件 lockedFailed
  DOORBELL_OBJECT_MODEL_NFC_CARD_DISABLED_EVENT = 8512,        // NFC卡被禁用 nfcCardDisabled

  DOORBELL_OBJECT_MODEL_HUB_ADDED = 8600,      // 内机添加事件(bool) hubAdded
  DOORBELL_OBJECT_MODEL_RTSP_ENABLE = 8800,    // RTSP开关(bool) rtspEnabled
  DOORBELL_OBJECT_MODEL_RTSP_CONFIG = 8801,    // RTSP配置(json) rtspConfig
  DOORBELL_OBJECT_MODEL_RTSP_USERNAME = 8802,  // RTSP用户名(string) rtspUserName
  DOORBELL_OBJECT_MODEL_RTSP_PASSWORD = 8803,  // RTSP密码(string) rtspPassword
} DOORBELL_OBJECT_MODEL;

// 内机的物模型定义(属性)
typedef enum {
  DOORBELL_OBJECT_MODEL_HUB_TIMEZONE = 100,     // 设置时区, string
  DOORBELL_OBJECT_MODEL_HUB_BLE_FW = 802,       // 内机BLE版本, int
  DOORBELL_OBJECT_MODEL_HUB_WIFI_MAC = 842,     // WIFI MAC, string
  DOORBELL_OBJECT_MODEL_HUB_IPADDRESS = 845,    // ipAddress, string
  DOORBELL_OBJECT_MODEL_HUB_WIFI_SSID = 846,    // 内机连接的SSID, wifiSsid, string
  DOORBELL_OBJECT_MODEL_HUB_BLE_MAC = 847,      // BLE MAC, string
  DOORBELL_OBJECT_MODEL_HUB_TIME_ZONEID = 852,  // 时区IANA格式 string timeZoneID Asia/Shanghai、American/New_York等
  DOORBELL_OBJECT_MODEL_HUB_FW_VERSION = 853,           // 固件版本 firmwareVersion
  DOORBELL_OBJECT_MODEL_HUB_AUTO_UPGRADE_ENABLE = 854,  // 自动升级开关 autoUpgradeEnabled
  DOORBELL_OBJECT_MODEL_HUB_UPGRADE_STATUS = 855,       // 升级状态 upgradeStatus
  DOORBELL_OBJECT_MODEL_HUB_UPGRADE_PROGRESS = 856,     // 升级进度 upgradeProgress
  DOORBELL_OBJECT_MODEL_HUB_WIFI_SIGNAL = 857,          // 内机连接的信号强度, string
  DOORBELL_OBJECT_MODEL_HUB_TIME_ZONE_POSIX = 859,      // 时区Posix格式 string timeZonePosix EST5EDT,M3.2.0,M11.1.0
  DOORBELL_OBJECT_MODEL_HUB_UPGRADE_START_TIME = 862,   // 升级开始时间 upgradeStartTime
  DOORBELL_OBJECT_MODEL_HUB_FLASHSCREENENABLED = 8200,  // 屏幕闪屏开关 , bool
  DOORBELL_OBJECT_MODEL_HUB_SCREENBRIGHTNESS = 8201,    // 屏幕亮度, int, 1-10
  DOORBELL_OBJECT_MODEL_HUB_INDICATORLIGHTENABLED = 8202,   // 屏幕指示灯开关, bool
  DOORBELL_OBJECT_MODEL_HUB_RINGTONEENABLED = 8203,         // 铃声开关, bool
  DOORBELL_OBJECT_MODEL_HUB_TONE = 8204,                    // 屏幕音效, int
  DOORBELL_OBJECT_MODEL_HUB_VOLUME = 8205,                  // 屏幕音量, int
  DOORBELL_OBJECT_MODEL_HUB_LANGUAGE = 8206,                // 屏幕显示语言, string
  DOORBELL_OBJECT_MODEL_HUB_DONOTDISTURBENABLED = 8207,     // 勿扰开关, bool
  DOORBELL_OBJECT_MODEL_HUB_DONOTDISTURBTIMERANGE = 8208,   // 勿扰时间范围, json
  DOORBELL_OBJECT_MODEL_HUB_DOORBELLIDS_DEPRECATED = 8209,  // 绑定外机列表, json
  DOORBELL_OBJECT_MODEL_HUB_SDCARDSTATUS = 8210,            // SD卡状态, int
  DOORBELL_OBJECT_MODEL_HUB_SDCARDINFO = 8211,  // SD卡信息，包括容量、已使用空间和可用空间, json
  DOORBELL_OBJECT_MODEL_HUB_SSID = 8213,        // wifi热点的SSID, string
  DOORBELL_OBJECT_MODEL_HUB_PWD = 8214,         // wifi热点的密码, string
  DOORBELL_OBJECT_MODEL_HUB_ANIMATEDPROMPTENABLED = 8215,  // 动画提示开关, bool
  DOORBELL_OBJECT_MODEL_HUB_SCREENTIMEOUT = 8216,          // 熄屏时间, int
  DOORBELL_OBJECT_MODEL_HUB_DETECTION_ALARM_TONE = 8217,   // 侦测报警显示器音效, int
  DOORBELL_OBJECT_MODEL_HUB_KEEP_SCREEN_ON = 8218,         // 永不熄屏开关, bool
  DOORBELL_OBJECT_MODEL_HUB_ENABLE24HOUR = 8219,           // 24小时制开关, bool
  DOORBELL_OBJECT_MODEL_HUB_DEBUG_MODE = 8220,             // 调试模式, int
  DOORBELL_OBJECT_MODEL_HUB_MATTER_SUPPORT = 8221,         // 是否支持matter supportsMatter, bool
  DOORBELL_OBJECT_MODEL_HUB_REPEATER_DEVICE_LIST = 8222,   // 拓展器设备列表, json数组
  DOORBELL_OBJECT_MODEL_HUB_DOORBELLLOCKBOUND = 8300,      // 外机和锁绑定事件, json
  DOORBELL_OBJECT_MODEL_HUB_DOORBELLLOCKUNBOUND = 8301,    // 外机和锁解绑事件, json

  DOORBELL_OBJECT_MODEL_HUB_DEFAULT_BIND_CAM_ID = 8566,  // 内机和外机绑定事件,string

} DOORBELL_OBJECT_MODEL_HUB;

// 内机设置方法
typedef enum {
  DOORBELL_OBJECT_HUB_FUNCTION_ID_RESET = 855,                 // 内机重置
  DOORBELL_OBJECT_HUB_FUNCTION_ID_LOCK = 8200,                 // 上锁
  DOORBELL_OBJECT_HUB_FUNCTION_ID_UNLOCK = 8201,               // 解锁
  DOORBELL_OBJECT_HUB_FUNCTION_ID_FORMATSDCARD = 8202,         // 格式化SD卡
  DOORBELL_OBJECT_HUB_FUNCTION_ID_OTA = 8203,                  // OTA
  DOORBELL_OBJECT_HUB_FUNCTION_ID_DISCONNCT_WEBRTC = 860,      // 断开webrtc连接
  DOORBELL_OBJECT_HUB_FUNCTION_ID_ADD_MATTER_SUBDEVICE = 882,  // 添加 matter 子设备
  DOORBELL_OBJECT_HUB_FUNCTION_ID_DEL_MATTER_SUBDEVICE = 883,  // 删除 matter 子设备
  DOORBELL_OBJECT_HUB_FUNCTION_ID_ENABLE_MATTER = 884,         // 查询设备是否启用 matter
  DOORBELL_OBJECT_HUB_FUNCTION_ID_ENTER_MATTER_MODE = 885,     // 进入添加 matter 模式
  DOORBELL_OBJECT_HUB_FUNCTION_ID_GET_HUB_INFO = 8206,         // 读取内机基础信息
  DOORBELL_OBJECT_HUB_FUNCTION_ID_SWITCH_BROKER = 5436         // 切换broker
} DOORBELL_OBJECT_HUB_FUNCTION_ID;

typedef enum {
  DOORBELL_OBJECT_CAM_FUNCTION_ID_ADD_QUICK_REPLY = 8400,                     // 新增自定义快捷回复
  DOORBELL_OBJECT_CAM_FUNCTION_ID_DELETE_QUICK_REPLY = 8401,                  // 删除自定义快捷回复
  DOORBELL_OBJECT_CAM_FUNCTION_ID_UPDATE_QUICK_REPLY_AUDIO = 8402,            // 更新自定义快捷回复音频
  DOORBELL_OBJECT_CAM_FUNCTION_ID_UPDATE_QUICK_REPLY_TITLE = 8403,            // 更新自定义快捷回复标题
  DOORBELL_OBJECT_CAM_FUNCTION_ID_UNBIND_DEVICE_MAC_KEY_ID = 8404,            // 解除设备Mac和Key ID的绑定
  DOORBELL_OBJECT_CAM_FUNCTION_ID_SOUND_ALARM = 8405,                         // 播放或停止声音告警
  DOORBELL_OBJECT_CAM_FUNCTION_ID_PLAY_QUICK_REPLY = 8406,                    // 播放或停止快捷回复
  DOORBELL_OBJECT_CAM_FUNCTION_ID_REMOVE_REMOVAL_ALERT = 8407,                // 解除被拆除告警
  DOORBELL_OBJECT_CAM_FUNCTION_ID_REMOVE_EMERGENCY_CREDENTIALS_ALERT = 8408,  // 解除紧急凭据告警
  DOORBELL_OBJECT_CAM_FUNCTION_ID_ACTIVATE_CLOUD_STORAGE = 851,               // activateCloudStorage
  DOORBELL_OBJECT_CAM_FUNCTION_ID_DEACTIVATE_CLOUD_STORAGE = 852,             // deactivateCloudStorage
  DOORBELL_OBJECT_CAM_FUNCTION_ID_SET_RTSP_USERNAME_PASSWD = 8410,            // 设置RTSP用户名和密码
  DOORBELL_OBJECT_CAM_FUNCTION_ID_GET_WIFI_INFO = 8411,                       // 获取wifi信息
} DOORBELL_OBJECT_CAM_FUNCTION_ID;

typedef struct detection_zone {
  int x;
  int y;
  int width;
  int height;
} DetectionZone;

// 外机的配置
typedef struct camera_dev_info {
  DOORBELL_CAMERA_STATE state;     // 状态机.0 : 空闲， 1：忙
  int32_t hubAdded;                // 内机是否添加，0：未添加，1：已添加
  char bleMac[32];                 // 外机蓝牙MAC
  int32_t online;                  // 在离线，0：离线，1：在线
  int32_t batteryLevel;            // 电量，0~100
  int32_t soundAlarmEnabled;       // 声音报警开关
  int32_t ringtone;                // 门铃音效，可选值：1/2/3
  int32_t ringVolume;              // 门铃音量，1~10
  int32_t alarmDuration;           // 报警时长，单位秒，可选值：5/10/30
  int32_t lightEnabled;            // 补光灯开关
  char monitorID[32];              // 绑定的内机，内机的蓝牙mac，没有绑定时为null
  char detectionZone[128];         // 侦测区域，json字符串
  int32_t motionDetectionEnabled;  // 移动侦测开关
  char detectionType[128];         // 侦测类型，可选值：["human"] / ["human", "all"] / ["all"]
  int32_t detType;                 // 侦测类型, 可选值：human:0x01 / all:0x02 / human+all:0x03
  int32_t detTypeInit;
  int32_t detectionSensitivity;     // 侦测灵敏度，1~10
  char detectionSchedules[256];     // 侦测时段，json字符串
  int32_t detSchedules[7];          // 侦测时段， 整形数组
  int32_t maxVideoClipLength;       // 最大录像时长，单位秒，10~60
  int32_t triggerInterval;          // 侦测间隔，单位秒，5~60
  int32_t batterySavingEnabled;     // 省电模式
  int32_t doNotDisturbEnabled;      // 勿扰开关
  char doNotDisturbTimeRange[64];   // 勿扰时间范围，json字符串
  int32_t keyboardDisabled;         // 键盘禁用(已经废弃，2024-11-13)
  int32_t indicatorLightEnabled;    // 状态指示灯开关
  int32_t nightVisionMode;          // 夜视模式，可选值：0.auto 1.off 2.on
  int32_t nightVisionDisplayMode;   // 夜视画面，可选值：0.黑白 1.彩色
  int32_t dismantlingAlarmEnabled;  // 拆除警报
  int32_t antiFlickerMode;          // 视频显示抗闪烁，0. 50HZ 1. 60HZ
  int32_t timeWatermarkEnabled;     // 时间水印开关
  int32_t logoWatermarkEnabled;     // logo水印开关
  int32_t flipScreenEnabled;        // 画面翻转开关
  int32_t talkingVolume;            // 对讲音量，1~10
  char lockButtonSettings[128];     // 上锁键设置，json字符串
  int32_t effectiveDuration;
  int32_t lockButtonMode;
  char lockID[32];                   // 锁的设备ID，内锁的蓝牙mac，没有绑定时为null
  char ssid[64];                     // wifi热点的ssid
  char pwd[64];                      // wifi热点的pwd
  int32_t stopRecordTime;            // 最近事件时间
  int32_t latestEventTime;           // 最近事件时间
  char latestEventType[32];          // 最近事件类型
  int32_t snoozeModeEndTime;         // 休眠模式结束时间
  int32_t keypadDisabled;            // 键盘禁用开关
  int32_t backlightEnabled;          // 门铃背光开关
  int32_t soundEnabled;              // 提示音开关
  char talkingVoice[32];             // 对讲音色，可选值：original/male/female
  int32_t talkTimbre;                // 对讲音色，可选值：0/1/2, 对应talkingVoice，方便用于对比
  int32_t quickRepliesEnabled;       // 快捷回复开关
  char customQuickReplies[2048];     // 用户自定义快捷回复列表，json字符串
  int32_t detectAlarmEnabled;        // 侦测报警开关
  int32_t detectAlarmDuration;       // 侦测报警持续时长
  int32_t detectAlarmFixedDuration;  // 侦测报警持续时长
  char triSrc[24];
  int32_t detectAlarmTone;              // 侦测报警音效，可选值：1/2/3
  int32_t detectAlarmVolume;            // 侦测报警音量
  int32_t batterySavingsEnabled;        // 节电续航开关
  int32_t continuousMonitoringEnabled;  // 持续监控开关
  int32_t keyboardSoundEnabled;         // 键盘提示音
  int32_t emergencyPasscodeUsed;        // 紧急密码被触发
  int32_t emergencyFingerprintUsed;     // 紧急指纹被触发
  int32_t passcodeDisabled;             // 密码被禁用
  int32_t fingerprintDisabled;          // 指纹被禁用
  int32_t nfcCardDisabled;              // NFC卡被禁用
  int32_t wasRemoved;                   // 被拆除
  int32_t removalAlertEnabled;          // 拆除报警开关
  int32_t autoReplyEnabled;             // 自动回复开关
  long long autoReplyID;                // 自动回复ID
  int32_t chargingStatus;  // 充电状态, 0: 未充电，1: 适配器充电，2: 太阳能板充电，3: 适配器连接已充满，4:
                           // 太阳能板连接已充满，5:未充满状态下太阳能板连接未充电；6:硬件错误
  int32_t recordingEnabled;       // 录像开关
  int32_t mutedRecordingEnabled;  // 录像静音开关
  int detectionAlarmMonitorTone;  // 侦测报警显示器音效
  int32_t rtspEnabled;            // RTSP开关

  char rtspUserName[32];  // RTSP用户名
  char rtspPassword[32];  // RTSP密码
  char rtspMainUrl[128];  // RTSP主URL
  char rtspSubUrl[128];   // RTSP子URL

  DetectionZone detectionZoneInfo;
} CameraDevInfo, *PCameraDevInfo;

typedef struct ImmediateParams {
  int32_t isReording;

} ImmediateParams_s, *PImmediateParams_s;

// 内机配置信息
typedef struct hub_dev_info {
  int32_t initState;              // 初始化状态，0：未初始化，1：初始化
  char timezone[64];              // 时区设置
  char timeZoneId[64];            // 时区ID设置
  char timeZonePosix[64];         // 时区Posix格式设置
  int bleVersion;                 // 内机蓝牙的BLE固件版本
  int bleHardVersion;             // 内机蓝牙的BLE硬件版本
  char hubMac[32];                // 内机蓝牙 MAC
  char wifiMac[32];               // 内机WIFI MAC
  char userId[128];               // 用户ID
  char firmwareVersion[64];       // 内机的版本
  int hubRssi;                    // 内机的WIFI RSSI
  int32_t autoupgradeEnabled;     // 自动升级开关 bool
  int32_t flashScreenEnabled;     // 屏幕闪屏开关 bool
  int32_t screenBrightness;       // 屏幕亮度 int
  int32_t indicatorLightEnabled;  // 屏幕指示灯开关 bool
  int32_t ringtoneEnabled;        // 铃声开关 bool
  int32_t tone;                   // 屏幕音效 int, 1/2/3
  int32_t volume;                 // 屏幕音量 int, 1~10
  int32_t detectionAlarmTone;     // 侦测报警显示器音效 int, 1/2/3
  int32_t keepScreenOn;           // 永不熄屏, bool
  int32_t enable24Hour;           // 24小时制开关 bool
  char language[32];              // 屏幕显示语言

  int32_t doNotDisturbEnabled;                 // 勿扰开关 bool
  char doNotDisturbTimeRange[64];              // 勿扰时间范围 json字符串
  char doorbellIDs[MAX_DOORBELL_CAM_NUM][32];  // 绑定的外机MAC列表
  int32_t recordingEnabled;                    // 录像开关 bool
  char ssid[128];                              // wifi热点的ssid
  char pwd[128];                               // wifi热点的pwd
  int32_t animatedPromptEnabled;               // 动画提示开关 bool
  int32_t screenTimeout;                       // 熄屏时间 int，单位秒
  int32_t mutedRecordingEnabled;               // 录像静音开关 bool

  ImmediateParams_s immedParams;
  CameraDevInfo cameras[MAX_DOORBELL_CAM_NUM];  // 摄像头信息

  int32_t isLoadCameraConfig;  // 是否加载摄像头配置 bool

  char lvglbindcamId[32];
  PCameraDevInfo sub_info;

} HubDevInfo, *PHubDevInfo;

typedef struct {
  int mapkey;
  int modelkey;
  int modelType;
  const char *keyName;
  int paramType;
  int (*cam_set_func)(void *dev_info, void *m_value, int Isrecv);
  int (*cam_get_func)(void *dev_info, void *m_value, void *m_value1);
} CameraParamsMap;

typedef struct hub_param_table {
  int attrkey;
  int attrType;
  char keyValue[64];
} hub_param_table_s, *Phub_param_table_s;

typedef enum PARAMS_TYPE {
  PARAMS_TYPE_INT = 0,
  PARAMS_TYPE_INT_ARRAY,
  PARAMS_TYPE_CHAR,
  PARAMS_TYPE_CHAR_ARRAY,
  PARAMS_TYPE_BOOL,
  PARAMS_TYPE_BOOL_ARRAY,
  PARAMS_TYPE_FALOT,
  PARAMS_TYPE_FALOT_ARRAY,
  PARAMS_TYPE_JSON_ELEMENT,

  PARAMS_TYPE_MAX
} PARAMS_TYPE_E;

typedef enum HUB_PARAMS_TABLE_CODE {
  HUB_PTABLE_INIT_STATE = 0,
  HUB_PTABLE_BLE_MAC,
  HUB_PTABLE_WIF_MAC,
  HUB_PTABLE_USER_ID,
  HUB_PTABLE_FW,
  HUB_PTABLE_RSSI,
  HUB_PTABLE_FLASH_SCREEN_ENABLE,
  HUB_PTABLE_SCREEN_BRIGHTNESS,
  HUB_PTABLE_INDICATOR_LIGHT_ENABLE,
  HUB_PTABLE_RING_TONE_ENABLE,
  HUB_PTABLE_TONE,
  HUB_PTABLE_VOLUME,
  HUB_PTABLE_LANGUAGE,

  HUB_PTABLE_DO_NOT_DISTURB_ENABLE,
  HUB_PTABLE_DO_NOT_DISTURB_TIME_RANGE,
  HUB_PTABLE_DOORBELL_LIST,
  HUB_PTABLE_RECORD_ENABLE,
  HUB_PTABLE_STA_SSID,
  HUB_PTABLE_STA_PASSWD,
  HUB_PTABLE_ANIMATE_PROMPT_ENABLE,
  HUB_PTABLE_SCREEN_TIMEOUT,
  HUB_PTABLE_MUTE_RECORD_ENABLE,
  HUB_PTABLE_LVGL_BIND_CAM_ID,
  HUB_PTABLE_DETECTION_ALARM_TONE,
  HUB_PTABLE_KEEP_SCREEN_ON,
  HUB_PTABLE_ENABLE24HOUR,

  /*Immediate Params start*/
  HUB_PTABLE_IMMED_IS_RECORDING,
  /*Immediate Params end*/

  /*外机参数*/
  SUB_PATABLE_DETECTALARMENABLED,
  SUB_PATABLE_DETECTALARMDURATION,
  SUB_PATABLE_DETECTALARMVOLUME,
  SUB_PATABLE_DETECTALARMTONE,
  SUB_PATABLE_LOCKBUTTONSETTINGS,
  SUB_PATABLE_SNOOZE_MODE_END_TIME,
  HUB_PARAMS_MAX
} HUB_PARAMS_TABLE_CODE_E;

char *mqtt_model_get_string(DOORBELL_OBJECT_MODEL model);
const char *mqtt_model_hub_get_string(DOORBELL_OBJECT_MODEL_HUB model);
int mqtt_model_hub_parse_param_json(cJSON *root, PHubDevInfo devInfo, int tableIndex);

PHubDevInfo hub_get_golbal_params(void);

#endif
