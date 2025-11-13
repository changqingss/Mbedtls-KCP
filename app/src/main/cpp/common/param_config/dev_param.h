#ifndef __DEV_PARAM_H__
#define __DEV_PARAM_H__

#include <stdbool.h>
#include <stdint.h>

#include "common/api/comm_api.h"

#define CONFIG_DIR PARAMTER_ROOT_DIR "/config"                       // hub设备配置目录
#define BACKUP_CONFIG_DIR PARAMTER_ROOT_DIR "/config_backup"         // hub设备配置备份目录
#define CONFIG_CAMERA_DIR PARAMTER_ROOT_DIR "/config/sub"            // 子设备配置目录
#define CONFIG_FILE_PATH PARAMTER_ROOT_DIR "/config/app_param.json"  // 设备需保存的配置参数文件
#define CONFIG_FILE_BACKUP_PATH PARAMTER_ROOT_DIR "/config/app_param_backup.json"  // 设备需保存的配置参数文件备份
#define OLD_CONFIG_FILE_PATH PARAMTER_ROOT_DIR "/config/app_param.config"  // 设备需保存的配置参数文件
#define OLD_CONFIG_FILE_BACKUP_PATH PARAMTER_ROOT_DIR "/config/app_param_backup.config"  // 设备需保存的配置参数文件备份
#define CONFIG_FILE_REGION_PATH PARAMTER_ROOT_DIR "/config/region.json"  // 移动侦测区域配置文件

#define REPEATER_CONFIG_DIR PARAMTER_ROOT_DIR "/config/repeater"                 // 中继器配置目录
#define REPEATER_QUADRUPLES_CONFIG_FILE REPEATER_CONFIG_DIR "/quadrupples.json"  // 主机中继器四元组配置文件
#define REPEATER_CLIENT_CONFIG_FILE REPEATER_CONFIG_DIR "/client.json"  // 客户端中继器四元组配置文件

#define HUB_CONFIG_FILE_PATH PARAMTER_ROOT_DIR "/config/hub_param.json"  // 内机需保存的配置参数文件

#define ROOT_DIR "/vendor/ssl/certs"            // 证书路径
#define ROOT_CERT "/vendor/ssl/certs/cert.pem"  // 设备根证书

#define CERT_STORE_PATH PARAMTER_ROOT_DIR "/config/certs"  // 证书存储路径
#define CERT_MESSAGE_FILE CERT_STORE_PATH "/cert_message.txt"
// 包含certificate.pem、private.pem.key证书以及topic、THING_NAME
#define CERTIFICATE_FILE CERT_STORE_PATH "/certificate.pem"  // IOT 公钥证书
#define PRIVATE_KEY_FILE CERT_STORE_PATH "/private.pem.key"  // IOT 私钥证书

// --- 新下发证书的临时存放路径（待验证连接成功后再移动到正式目录） ---
#define NEW_CERT_TEMP_DIR "/tmp/new_cert_temp"
#define NEW_CERT_TEMP_MESSAGE_FILE NEW_CERT_TEMP_DIR "/cert_message.txt"
#define NEW_CERT_TEMP_CERTIFICATE_FILE NEW_CERT_TEMP_DIR "/certificate.pem"
#define NEW_CERT_TEMP_PRIVATE_KEY_FILE NEW_CERT_TEMP_DIR "/private.pem.key"

#define BIND_STATUS_FILE PARAMTER_ROOT_DIR "/config/bindStatus.txt"              // 设备绑定标记
#define ACTIVE_RESTART_STATUS_FILE PARAMTER_ROOT_DIR "/config/activeRstart.txt"  // 设备主动重启标记
#define HUB_BLE_MAC_FILE PARAMTER_ROOT_DIR "/hub_ble_mac.txt"                    // 内机蓝牙MAC地址文件
#define KVS_PROVIDER_FILE PARAMTER_ROOT_DIR "/config/kvs_provider.txt"
// KVS连接凭证文件，包括ChannelName、Region、KVS URL
#define TIMEZONE_FILE PARAMTER_ROOT_DIR "/config/timezone.txt"      // 时区配置文件
#define SECRET_KEY_FILE PARAMTER_ROOT_DIR "/config/secret_key.txt"  // 秘钥文件

#define REPEATER_BIND_FILE PARAMTER_ROOT_DIR "/config/repeater_bind.txt"      // 中继器绑定状态文件
#define REPEATER_KEY_FILE "/tmp/repeater_key.txt"                             // 中继器密钥文件
#define REPEATER_CONFIG_FILE PARAMTER_ROOT_DIR "/config/repeater_config.txt"  // 中继器配置文件
#define REPEATER_WIFI_CONFIG_FILE_TMP "/tmp/repeater_wifi_config.txt"         // 中继器配置文件临时文件

// /root/share/01_code/07_Doorbell/doorbell4/pkg/ad100/vendor/wifi/run_ap.sh
#define WIFI_CONFIG_DIR PARAMTER_ROOT_DIR "/config/wifi"                       // WIFI配置文件目录
#define WIFI_CONFIG_FILE PARAMTER_ROOT_DIR "/config/wifi/wpa_supplicant.conf"  // WIFI配置文件
#define AP_CONFIG_SCRIPT "/vendor/wifi/run_ap.sh"                              // AP启动脚本文件
#define CAMERA_CLIENT_BIND_CONF "/parameter/conf/cam_bind.conf"                // 外机的配置文件
#define HOST_APD_CONF "/parameter/config/wifi/hostapd.conf"                    // AP配置文件

#define REPEATER_CLIENT_BIND_FILE "/parameter/repeater_client_bind"  // 中继器客户端的绑定标志

#define QUICK_REPLY_DIR PARAMTER_ROOT_DIR "/config/quick_reply/"           // 快捷回复的配置目录
#define QUICK_REPLY_DEFAULT_DIR "/vendor/audio/"                           // 快捷回复的默认音频目录
#define QUICK_REPLY_COUSTUM_DIR QUICK_REPLY_DIR "custom/"                  // 快捷回复的自定义音频目录
#define QUICK_REPLY_CUSTOM_CONF QUICK_REPLY_DIR "quick_reply_custom.conf"  // 快捷回复的自定义配置文件
#define QUICK_REPLY_DEF_CONF QUICK_REPLY_DIR "quick_reply_def.conf"        // 快捷回复的默认配置文件

#define HUB_RESET_FLAG "/parameter/hubReset"
#define HUB_FIRST_ADD "/parameter/hubFirstAdd"

#define REFRESH_KVS_PROVIDER "/tmp/kvsProivder"
#define RESTART_KVS_APP "/tmp/outmem"

#define WPA_SUPPLICANT_PATH "/var/run/wpa_supplicant/wlan0"  // WPA的unix套接字

#ifndef T23ZN

#ifdef AD100
#define DOORBELL_HUB_PRODUCT_ID "W1050000"
#elif defined(AD102N)
#define DOORBELL_HUB_PRODUCT_ID "W1050000"
#elif defined(AD102P)
#define DOORBELL_HUB_PRODUCT_ID "W1050000"
#else
#error NO DEF CONFIG_HI_CHIP_TYPE
#endif

#endif

#define DOORBELL_HUB_PRODUCT_ID "W1050000"
#define DOORBELL_CAMERA_PRODUCT_ID "W1050001"
#define DOORBELL_EXTERN_HUB_PRODUCT_ID "W1050003"
#define DOORBELL_REPEATER_PRODUCT_ID "W1050003"
#define MAX_CAMERA_NUM (16)
#define NTP_CHECK_TIME_SEC (1719763200)  // 2024-7-1，NTP校时完的基准时间

#define UNIX_SOCKET_PATH_SPK "/tmp/speaker_unix_sock"
#define UNIX_SOCKET_PATH_KVS "/tmp/push_stream_kvs"

/***** 设备类型 *****/
#define DEV_TYPE_3MP "WoCamKvs"
#define DEV_TYPE_5MP "WoCamKvs5mp"

/* 音效文件 */
#define QRCODE_MUSIC_FILE "/vendor/resource/qrcode.pcm"
#define MUSIC1_FILE "/vendor/resource/music1.pcm"
#define MUSIC2_FILE "/vendor/resource/music2.pcm"
#define MUSIC3_FILE "/vendor/resource/music3.pcm"
#define SIREN1_FILE "/vendor/resource/siren_1.pcm"
#define SIREN2_FILE "/vendor/resource/siren_2.pcm"
#define SIREN3_FILE "/vendor/resource/siren_3.pcm"

#define ALARM_AUDIO_FILE "/vendor/resource/alarm.pcm"

#define DEFAULT_SSID "00000000"
#define DEFAULT_PASSWD "00000000"

#define END_POINT_OFFLINE "c21h83ofzvo45x.credentials.iot.us-east-1.amazonaws.com"  // 线下
#define ROLE_ALIAS_OFFLINE "KvsCameraIoTRoleAlias"                                  // 线下
// #define GET_IOT_CERT_URL_OFFLINE
//   "https://nmep8h3wn7.execute-api.us-east-1.amazonaws.com/prod/v1/getIotCert"  // 线下环境

// #define GET_IOT_CERT_URL_ONLINE
//   "https://igkqjo8693.execute-api.us-east-1.amazonaws.com/prod/v1/getIotCert"  //线上环境
#define END_POINT_ONLINE "c1uvqmjoxoa1ul.credentials.iot.us-east-1.amazonaws.com"  // 线上
#define ROLE_ALIAS_ONLINE "KvsCameraIoTRoleAlias"                                  // 线上

#define QRCODE_REGION_PROD_US_ID "p1"
#define QRCODE_REGION_PROD_US_CERT_URL "https://device.us.api.switchbot.net/kvs/v1/getIotCert"
#define QRCODE_REGION_PROD_AP_ID "p2"
#define QRCODE_REGION_PROD_AP_CERT_URL "https://device.ap.api.switchbot.net/kvs/v1/getIotCert"
#define QRCODE_REGION_PROD_EU_ID "p3"
#define QRCODE_REGION_PROD_EU_CERT_URL "https://device.eu.api.switchbot.net/kvs/v1/getIotCert"
#define QRCODE_REGION_PROD_CN_ID "p4"
#define QRCODE_REGION_PROD_CN_CERT_URL "https://device.cn.api.switchbot.net/kvs/v1/getIotCert"
#define QRCODE_REGION_TEST_US_ID "t1"
#define QRCODE_REGION_TEST_US_CERT_URL "https://device.us.api.woankeji.cn/kvs/v1/getIotCert"
#define QRCODE_REGION_TEST_AP_ID "t2"
#define QRCODE_REGION_TEST_AP_CERT_URL "https://device.ap.api.woankeji.cn/kvs/v1/getIotCert"
#define QRCODE_REGION_TEST_EU_ID "t3"
#define QRCODE_REGION_TEST_EU_CERT_URL "https://device.eu.api.woankeji.cn/kvs/v1/getIotCert"
#define QRCODE_REGION_TEST_CN_ID "t4"
#define QRCODE_REGION_TEST_CN_CERT_URL "https://device.cn.api.woankeji.cn/kvs/v1/getIotCert"

#define COUNTRY_CODE_JP "JP"
#define COUNTRY_CODE_US "US"
#define COUNTRY_CODE_EU "EU"

#define SETTINGS_COUNTDOWN_TIMESEC (15)
#define KVS_COUNTDOWN_TIMESEC (15)
#define HUB_LOG_UPLOAD_TIMESEC (60 * 60)  // 内机统计日志上传时间间隔，1小时
// #define HUB_LOG_UPLOAD_TIMESEC (10)  // 内机统计日志上传时间间隔，1小时
#define MIN_REQUIRED_LINES (2)  // 最少需要2行(表头+数据)

typedef enum BIND_STATE {
  BIND_STATE_UNBIND,                // 未绑定
  BIND_STATE_ENTER_PAIR_MODE_REQ,   // 进入配网模式中
  BIND_STATE_ENTER_PAIR_MODE_RESP,  // 进入配网模式成功
  BIND_STATE_OK,                    // 成功绑定
} BIND_STATE;

#define AAC_AUDIO_FRAME_HEAD_LEN (7)
#define MAX_CAM_SYNC_PARAM_SIZE (4096)

// 0: 未充电，1: 适配器充电，2: 太阳能板充电，3: 适配器连接已充满，4:
// 太阳能板连接已充满，5:未充满状态下太阳能板连接未充电；6:硬件错误
typedef enum {
  CAMERA_CHARGING_STATE_NONE,              // 未充电
  CAMERA_CHARGING_STATE_ADAPTER_CHARGING,  // 适配器充电中
  CAMERA_CHARGING_STATE_SOLAR_CHARGING,    // 太阳能充电中
  CAMERA_CHARGING_STATE_ADAPTER_FINISH,    // 适配器充满电
  CAMERA_CHARGING_STATE_SOLAR_FINISH,      // 太阳能充满电
  CAMERA_CHARGING_STATE_SOLAR_DISCONNECT,  // 未充满状态下太阳能板连接未充电
  CAMERA_CHARGING_STATE_HARDWARE_ERROR,    // 硬件错误
} CAMERA_CHARGING_STATE;

/* 设备基本信息 */
typedef struct DEV_BASE_INFO {
  char mac[13];        // 设备蓝牙 MAC 地址
  char userID[64];     // 设备用户 ID
  char requestID[64];  // 设备请求 ID
  int hwVersion;
} DEV_BASE_INFO_S;

/* 侦测报警参数 */
typedef struct DETECT_ALARM_PARAM {
  unsigned char detSwitch;  // 0:关闭   1:打开
  unsigned char tone;
  unsigned char volume;
  unsigned char duration;
} DETECT_ALARM_PARAM_S;

typedef struct DEV_POINT {
  int x;
  int y;
} DEV_POINT_S;

typedef struct DEV_RESOLUTION_INFO {
  unsigned char resolution;
  char id[64];
} DEV_RESOLUTION_INFO_S;

#define ENDPOINT_SIZE (100)
#define IOT_URL_SIZE (100)
#define ROLE_ALIAS_SIZE (50)
typedef struct AWS_CONF {
  char endpoint[ENDPOINT_SIZE];
  char iot_url[IOT_URL_SIZE];
  char role_alias[ROLE_ALIAS_SIZE];
} AWS_CONF_S;

typedef struct EVENT_SCHEDULE {
  bool enEventSchedule;
  int monSchedule;
  int tueSchedule;
  int wedSchedule;
  int thursSchedule;
  int friSchedule;
  int staSchedule;
  int sunSchedule;
} EVENT_SCHEDULE_S;

typedef struct REC_CONF {
  unsigned char enrecord;    // 录像模式 0:关闭录像 1:开启录像
  unsigned char recordType;  // 录像模式 1:全时录像 2:事件录像
  unsigned char muteRecord;  // 静音录像 0：关闭 1：打开
} REC_CONF_S;

typedef enum APP_PARAM_TYPE {
  PARAM_TYPE_CONTRAST = 0,
  PARAM_TYPE_BRIGHTNESS,
  PARAM_TYPE_SHARPNESS,
  PARAM_TYPE_SATURATION,
  PARAM_TYPE_MD,
  PARAM_TYPE_MD_SENSE,
  PARAM_TYPE_HUMAN_FILTER,
  PARAM_TYPE_RESOLUTION,
  PARAM_TYPE_INDICATOR,
  PARAM_TYPE_NIGHT_VISION,
  PARAM_TYPE_VOLUME,
  PARAM_TYPE_FLIP,
  PARAM_TYPE_PRIVATE_MODE,
  PARAM_TYPE_TIME_WATERMARK,
  PARAM_TYPE_TALK_MODE,
  PARAM_TYPE_TIMEZONE,
  PARAM_TYPE_TIMEZONEPOSIX,
  PARAM_TYPE_AUTO_TRACKING,
  PARAM_TYPE_TOKEN,
  PARAM_TYPE_STREAM_NAME,
  PARAM_TYPE_REGION,
  PARAM_TYPE_UPGRADE_FLAG,
  PARAM_TYPE_DEV_MODE,
  PARAM_TYPE_DET_ALARM,
  PARAM_TYPE_AUTO_UPGRADE,
  PARAM_TYPE_SOUND_ALARM,
  PARAM_TYPE_ANTI_FLICKER,
  PARAM_TYPE_CRUISE,
  PARAM_TYPE_PREPOINT,
  PARAM_TYPE_CLOUD_END_TIME,
  PARAM_TYPE_CLOUD_STAR_TIME,
  PARAM_TYPE_ENDPOINT,
  PARAM_TYPE_ROLEALIAS,
  PARAM_TYPE_IOT_URL,
  PARAM_TYPE_HW_VERSION,
  PARAM_TYPE_DARK_FULL_COLOR,
  PARAM_TYPE_EVENT_SCHEDULE,
  PARAM_TYPE_RECORD_INFO,
  PARAM_TYPE_EN_ALGOLOG,
  PARAM_TYPE_EN_KEYLOG,
  PARAM_TYPE_EN_DETAILLOG,
  PARAM_TYPE_SET_LOGLEVEL,
  PARAM_TYPE_MAX
} APP_PARAM_TYPE_E;

#pragma pack(1)
typedef struct speaker_frame_t {
  uint16_t head;       // 消息头
  char subMac[6];      // 子设备的MAC地址
  uint64_t timestamp;  // 时间戳
  int codecType;       // 保留字段
  int frameSize;       // 帧大小
  char frameData[];    // 不占据内存大小
} SpeakerFrame, *PSpeakerFrame;
#pragma pack()

#pragma pack(1)
typedef struct sub_mac_conf_t {
  char bleMac[64];   // 子设备的ble MAC地址
  char wifiMac[64];  // 子设备的wifi MAC地址
} sub_mac_conf, *Psub_mac_conf;
#pragma pack()

int DEV_PARAM_GetParam(APP_PARAM_TYPE_E type, void *pDataDst);
int DEV_PARAM_SetParam(APP_PARAM_TYPE_E type, void *pValue);
int DEV_PARAM_Init(void);
int DEV_PARAM_SaveToFile(void);
int DEV_PARAM_SetFactoryMode(void);
int DEV_PARAM_MakeConfEffect(APP_PARAM_TYPE_E type);
bool DEV_PARAM_CheckPrivateMode(void);
int syncParamToFile(void);
int DEV_PARAM_get_sub_ble_mac(const char *wifiMac, char *bleMacOut, size_t bleMacOutSize);
int DEV_PARAM_update_sub_mac(char *m_subBleMac, char *m_subWifiMac);

#endif
