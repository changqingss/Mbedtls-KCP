#include "model.h"

#include <stdlib.h>

#include "common/log/log_conf.h"

// 1、根据物模型的类型获取属性ID
char *mqtt_model_get_string(DOORBELL_OBJECT_MODEL model) {
  switch (model) {
    case DOORBELL_OBJECT_MODEL_ONLINE:
      return "66";  // 在线/离线状态(int), 0：离线， 1：在线
    case DOORBELL_OBJECT_MODEL_BLE_FW:
      return "802";
    case DOORBELL_OBJECT_MODEL_BATTERY_LEVEL:
      return "820";  // 电量(int), 0~100
    case DOORBELL_OBJECT_MODEL_BLE_MAC:
      return "847";  // BLE MAC(string)
    case DOORBELL_OBJECT_MODEL_FIRMWAREVERSION:
      return "853";  // 固件版本(string)
    case DOORBELL_OBJECT_MODEL_UPGRADESTATUS:
      return "855";  // 升级状态(string)
    case DOORBELL_OBJECT_MODEL_UPGRADEPROGRESS:
      return "856";  // 升级进度(int)
    case DOORBELL_OBJECT_MODEL_RING_TONE:
      return "8402";  // 报警音效(int), 可选值：1/2/3
    case DOORBELL_OBJECT_MODEL_RING_VOLUME:
      return "8403";  // 门铃音量(int), 0~10
    case DOORBELL_OBJECT_MODEL_LIGHT_ENABLED:
      return "8405";  // 灯光开关(bool)
    case DOORBELL_OBJECT_MODEL_MONITOR_ID:
      return "8406";  // 绑定的内机(string), 内机的蓝牙mac
    case DOORBELL_OBJECT_MODEL_DETECTION_ZONE:
      return "8407";  // 侦测区域(json), {"x": 0, "y": 0, "width": 80, "height": 80}
    case DOORBELL_OBJECT_MODEL_MOTION_DETECTION_ENABLED:
      return "8408";  // 移动侦测开关(bool)
    case DOORBELL_OBJECT_MODEL_DETECTION_TYPE:
      return "8409";  // 侦测类型(json), 可选值：["human"] / ["human", "all"] / ["all"]
    case DOORBELL_OBJECT_MODEL_DETECTION_SENSITIVITY:
      return "8410";  // 侦测灵敏度(int), 1~10
    case DOORBELL_OBJECT_MODEL_DETECTION_SCHEDULES:
      return "8411";  // 侦测时段(json), 待定
    case DOORBELL_OBJECT_MODEL_MAX_VIDEO_CLIP_LENGTH:
      return "8412";  // 最大录制时长(int), 单位秒，10~60
    case DOORBELL_OBJECT_MODEL_TRIGGER_INTERVAL:
      return "8413";  // 侦测间隔(int), 单位秒，5~60
    case DOORBELL_OBJECT_MODEL_INDICATOR_LIGHT_ENABLED:
      return "8418";  // 状态指示灯开关(bool)
    case DOORBELL_OBJECT_MODEL_NIGHT_VISION_MODE:
      return "8419";  // 夜视模式(int), 可选值：0.auto 1.off 2.on
    case DOORBELL_OBJECT_MODEL_NIGHT_VISION_DISPLAY_MODE:
      return "8420";  // 夜视画面(int), 可选值：0.黑白 1.彩色
    case DOORBELL_OBJECT_MODEL_DISMANTLING_ALARM_ENABLED:
      return "8421";  // 拆除警报(bool)
    case DOORBELL_OBJECT_MODEL_ANTI_FLICKER_MODE:
      return "8422";  // 视频显示抗闪烁(int), 0: 50HZ 1: 60HZ
    case DOORBELL_OBJECT_MODEL_TIME_WATERMARK_ENABLED:
      return "8423";  // 时间水印开关(bool)
    case DOORBELL_OBJECT_MODEL_LOGO_WATERMARK_ENABLED:
      return "8424";  // logo水印开关(bool)
    case DOORBELL_OBJECT_MODEL_FLIP_SCREEN_ENABLED:
      return "8425";  // 画面翻转开关(bool)
    case DOORBELL_OBJECT_MODEL_TALK_VOLUME:
      return "8426";  // 对讲音量(int), 1~10
    case DOORBELL_OBJECT_MODEL_LOCKBUTTONSETTINGS:
      return "8427";  // 上锁键设置(json), {"mode": 1, "effectiveDuration": 120},
                      // mode: 1.禁用，2.永久启用，3.非永久启用；effectiveDuration：关门后多少秒内会生效
    case DOORBELL_OBJECT_MODEL_LOCK_ID:
      return "8428";  // 锁的设备ID(string)
    case DOORBELL_OBJECT_MODEL_WIFI_SSID:
      return "8429";  // wifi热点的ssid(string)
    case DOORBELL_OBJECT_MODEL_WIFI_PWD:
      return "8430";  // wifi热点的pwd(string)
    case DOORBELL_OBJECT_MODEL_LAST_EVENT_TIME:
      return "8431";  // 最近事件时间(int)
    case DOORBELL_OBJECT_MODEL_LAST_EVENT_TYPE:
      return "8432";  // 最近事件类型(string)
    case DOORBELL_OBJECT_MODEL_SNOOZE_MODE_END_TIME:
      return "8433";  // 休眠模式的结束时间(int)
    case DOORBELL_OBJECT_MODEL_KEYPAD_DISABLED:
      return "8434";  // 键盘禁用开关(bool)
    case DOORBELL_OBJECT_MODEL_BACK_LIGHT_ENABLED:
      return "8435";  // 门铃背光开关(bool)
    case DOORBELL_OBJECT_MODEL_SOUNDENABLED:
      return "8436";  // 提示音开关(bool)
    case DOORBELL_OBJECT_MODEL_TALKVOICE:
      return "8437";  // 对讲音色(string), 可选值：original/male/female
    case DOORBELL_OBJECT_MODEL_CUSTOM_QUICK_REPLIES:
      return "8440";  // 用户自定义快捷回复列表(json), [{"id": "c1e8feb5-e7e1-4330-b153-98c9db49b594", "title": "Hello",
                      // "audioPath": ""}, {"id": "d23d5689-bc0a-4606-bb77-9e74cfaba5a8", "title": "World", "audioPath":
                      // ""}]
    case DOORBELL_OBJECT_MODEL_DETECT_ALARM_ENABLED:
      return "8441";  // 侦测报警开关(bool)
    case DOORBELL_OBJECT_MODEL_DETECT_ALARM_DURATION:
      return "8442";  // 侦测报警持续时长(int)
    case DOORBELL_OBJECT_MODEL_DETECT_ALARM_TONE:
      return "8443";  // 侦测报警音效(int)
    case DOORBELL_OBJECT_MODEL_DETECT_ALARM_VOLUME:
      return "8444";  // 侦测报警音量(int)
    case DOORBELL_OBJECT_MODEL_KEYBOARD_SOUND_ENABLED:
      return "8447";  // 键盘提示音(bool)
    case DOORBELL_OBJECT_MODEL_EMERGENCY_PASSCODE_USED:
      return "8448";  // 紧急密码被触发(bool)
    case DOORBELL_OBJECT_MODEL_EMERGENCY_FINGERPRINT_USED:
      return "8449";  // 紧急指纹被触发(bool)
    case DOORBELL_OBJECT_MODEL_PASSCODE_DISABLED:
      return "8450";  // 密码被禁用(bool)
    case DOORBELL_OBJECT_MODEL_FINGERPRINT_DISABLED:
      return "8451";  // 指纹被禁用(bool)
    case DOORBELL_OBJECT_MODEL_NFC_CARD_DISABLED:
      return "8452";  // NFC卡被禁用(bool)
    case DOORBELL_OBJECT_MODEL_WAS_REMOVED:
      return "8453";  // 被拆除(bool)
    case DOORBELL_OBJECT_MODEL_CHARGING_STATUS:
      return "8455";  // 充电状态(int)
    case DOORBELL_OBJECT_MODEL_RECORDING_ENABLED:
      return "8456";  // 录像开关(bool)
    case DOORBELL_OBJECT_MODEL_MUTED_RECORDING_ENABLED:
      return "8457";  // 录像静音开关(bool)
    case DOORBELL_OBJECT_MODEL_DETECTION_ALARM_MONITOR_TONE:
      return "8458";  // 侦测报警显示器音效(int)
    case DOORBELL_OBJECT_MODEL_IS_WAKEUP:
      return "8459";  // 外机是否唤醒(bool)
    case DOORBELL_OBJECT_MODEL_EMERGENCY_PASSCODE_CYCLE_NUM:
      return "8461";  // 紧急密码触发次数(int)
    case DOORBELL_OBJECT_MODEL_EMERGENCY_FINGERPRINT_CYCLE_NUM:
      return "8462";  // 紧急指纹触发次数(int)
    case DOORBELL_OBJECT_MODEL_EMERGENCY_CREDENTIAL_USED_NUM:
      return "8464";  // 使用的紧急凭证emergencyCredentialUsed
    case DOORBELL_OBJECT_MODEL_WIFI_INFO:
      return "8465";  // 外机的wifi信息(信号强度和信道)
    case DOORBELL_OBJECT_MODEL_AUTO_REPLY_ENABLED:
      return "8466";  // 自动回复开关 autoReplyEnabled(bool)
    case DOORBELL_OBJECT_MODEL_AUTO_REPLY_ID:
      return "8467";  // 自动回复ID autoReplyID(int)
    case DOORBELL_OBJECT_MODEL_RING_EVENT:
      return "8500";  // 响铃事件(json)
    case DOORBELL_OBJECT_MODEL_HUMAN_MOVE_EVENT:
      return "8501";  // 人移动事件(json)
    case DOORBELL_OBJECT_MODEL_MOTION_DETECT_EVENT:
      return "8502";  // 移动侦测事件(json)
    case DOORBELL_OBJECT_MODEL_DISMANTLE_EVENT:
      return "8503";  // 设备被拆除事件(json)
    case DOORBELL_OBJECT_MODEL_UNLOCK_EVENT:
      return "8504";  // 解锁事件(json)
    case DOORBELL_OBJECT_MODEL_LOCK_EVENT:
      return "8505";  // 上锁成功事件(json)
    case DOORBELL_OBJECT_MODEL_EVENT_RECORD_END_EVENT:
      return "8506";  // 事件录像结束事件(json)
    case DOORBELL_OBJECT_MODEL_VERIFICATION_FAIL_EVENT:
      return "8507";  // 验证失败事件(json)
    case DOORBELL_OBJECT_MODEL_PASSWORD_UNLOCK_DISABLE_EVENT:
      return "8508";  // 密码解锁被禁用事件(json)
    case DOORBELL_OBJECT_MODEL_FINGER_UNLOCK_DISABLE_EVENT:
      return "8509";  // 指纹解锁被禁用事件(json)
    case DOORBELL_OBJECT_MODEL_NFC_UNLOCK_DISABLE_EVENT:
      return "8510";  // NFC卡解锁被禁用事件(json)
    case DOORBELL_OBJECT_MODEL_LOCKFAILED_EVENT:
      return "8511";  // 上锁失败事件(json)
    case DOORBELL_OBJECT_MODEL_NFC_CARD_DISABLED_EVENT:
      return "8512";  // NFC卡被禁用 nfcCardDisabled
    case DOORBELL_OBJECT_MODEL_HUB_ADDED:
      return "8600";  // 内机添加事件(bool)
    case DOORBELL_OBJECT_MODEL_RTSP_ENABLE:
      return "8800";  // RTSP开关(bool)
    case DOORBELL_OBJECT_MODEL_RTSP_CONFIG:
      return "8801";  // RTSP配置(json)
    case DOORBELL_OBJECT_MODEL_RTSP_USERNAME:
      return "8802";  // RTSP用户名(string)
    case DOORBELL_OBJECT_MODEL_RTSP_PASSWORD:
      return "8803";  // RTSP密码(string)
    default:
      return "";  // 未知属性
  }
}

const char *mqtt_model_hub_get_string(DOORBELL_OBJECT_MODEL_HUB model) {
  switch (model) {
    case DOORBELL_OBJECT_MODEL_HUB_BLE_FW:
      return "802";
    case DOORBELL_OBJECT_MODEL_HUB_WIFI_MAC:
      return "842";  // WIFI MAC
    case DOORBELL_OBJECT_MODEL_HUB_IPADDRESS:
      return "845";  // 内机连接IP地址
    case DOORBELL_OBJECT_MODEL_HUB_WIFI_SSID:
      return "846";  // 内机连接WIFI的SSID
    case DOORBELL_OBJECT_MODEL_HUB_BLE_MAC:
      return "847";  // BLE MAC
    case DOORBELL_OBJECT_MODEL_HUB_FW_VERSION:
      return "853";
    case DOORBELL_OBJECT_MODEL_HUB_TIME_ZONEID:
      return "852";
    case DOORBELL_OBJECT_MODEL_HUB_AUTO_UPGRADE_ENABLE:
      return "854";
    case DOORBELL_OBJECT_MODEL_HUB_UPGRADE_STATUS:
      return "855";
    case DOORBELL_OBJECT_MODEL_HUB_UPGRADE_PROGRESS:
      return "856";
    case DOORBELL_OBJECT_MODEL_HUB_WIFI_SIGNAL:
      return "857";  // 内机连接WIFI的信号强度
    case DOORBELL_OBJECT_MODEL_HUB_TIME_ZONE_POSIX:
      return "859";
    case DOORBELL_OBJECT_MODEL_HUB_UPGRADE_START_TIME:
      return "862";  // 升级开始时间 upgradeStartTime
    case DOORBELL_OBJECT_MODEL_HUB_FLASHSCREENENABLED:
      return "8200";  // 屏幕闪屏开关
    case DOORBELL_OBJECT_MODEL_HUB_SCREENBRIGHTNESS:
      return "8201";  // 屏幕亮度
    case DOORBELL_OBJECT_MODEL_HUB_INDICATORLIGHTENABLED:
      return "8202";  // 指示灯开关
    case DOORBELL_OBJECT_MODEL_HUB_RINGTONEENABLED:
      return "8203";  // 铃声开关
    case DOORBELL_OBJECT_MODEL_HUB_TONE:
      return "8204";  // 屏幕音效
    case DOORBELL_OBJECT_MODEL_HUB_VOLUME:
      return "8205";  // 屏幕音量
    case DOORBELL_OBJECT_MODEL_HUB_LANGUAGE:
      return "8206";  // 屏幕显示语言
    case DOORBELL_OBJECT_MODEL_HUB_DONOTDISTURBENABLED:
      return "8207";  // 勿扰开关
    case DOORBELL_OBJECT_MODEL_HUB_DONOTDISTURBTIMERANGE:
      return "8208";  // 勿扰时间范围
    case DOORBELL_OBJECT_MODEL_HUB_DOORBELLIDS_DEPRECATED:
      return "8209";  // 绑定外机列表
    case DOORBELL_OBJECT_MODEL_HUB_SDCARDSTATUS:
      return "8210";  // SD卡状态
    case DOORBELL_OBJECT_MODEL_HUB_SDCARDINFO:
      return "8211";  // SD卡信息，包括容量、已使用空间和可用空间
    case DOORBELL_OBJECT_MODEL_HUB_SSID:
      return "8213";  // wifi热点的SSID
    case DOORBELL_OBJECT_MODEL_HUB_PWD:
      return "8214";  // wifi热点的密码
    case DOORBELL_OBJECT_MODEL_HUB_ANIMATEDPROMPTENABLED:
      return "8215";  // 动画提示开关
    case DOORBELL_OBJECT_MODEL_HUB_SCREENTIMEOUT:
      return "8216";  // 熄屏时间
    case DOORBELL_OBJECT_MODEL_HUB_DETECTION_ALARM_TONE:
      return "8217";  // 侦测报警显示器音效
    case DOORBELL_OBJECT_MODEL_HUB_MATTER_SUPPORT:
      return "8221";
    case DOORBELL_OBJECT_MODEL_HUB_DOORBELLLOCKBOUND:
      return "8300";  // 外机和锁绑定事件
    case DOORBELL_OBJECT_MODEL_HUB_KEEP_SCREEN_ON:
      return "8218";  // 永不熄屏开关
    case DOORBELL_OBJECT_MODEL_HUB_ENABLE24HOUR:
      return "8219";  // 24小时制开关
    case DOORBELL_OBJECT_MODEL_HUB_DOORBELLLOCKUNBOUND:
      return "8301";  // 外机和锁解绑事件
    case DOORBELL_OBJECT_MODEL_HUB_DEFAULT_BIND_CAM_ID:
      return "8566";  // 内机和外机绑定事件
    default:
      return "";
  }
}

hub_param_table_s defparamsTable[HUB_PARAMS_MAX] = {
    {HUB_PTABLE_INIT_STATE, PARAMS_TYPE_INT, "initState"},
    {HUB_PTABLE_BLE_MAC, PARAMS_TYPE_CHAR, "hubMac"},
    {HUB_PTABLE_WIF_MAC, PARAMS_TYPE_CHAR, "wifiMac"},
    {HUB_PTABLE_USER_ID, PARAMS_TYPE_CHAR, "userId"},
    {HUB_PTABLE_FW, PARAMS_TYPE_CHAR, "firmwareVersion"},
    {HUB_PTABLE_RSSI, PARAMS_TYPE_INT, "hubRssi"},
    {HUB_PTABLE_FLASH_SCREEN_ENABLE, PARAMS_TYPE_BOOL, "flashScreenEnabled"},
    {HUB_PTABLE_SCREEN_BRIGHTNESS, PARAMS_TYPE_INT, "screenBrightness"},
    {HUB_PTABLE_INDICATOR_LIGHT_ENABLE, PARAMS_TYPE_BOOL, "indicatorLightEnabled"},
    {HUB_PTABLE_RING_TONE_ENABLE, PARAMS_TYPE_BOOL, "ringtoneEnabled"},
    {HUB_PTABLE_TONE, PARAMS_TYPE_INT, "tone"},
    {HUB_PTABLE_VOLUME, PARAMS_TYPE_INT, "volume"},
    {HUB_PTABLE_LANGUAGE, PARAMS_TYPE_CHAR, "language"},
    {HUB_PTABLE_DO_NOT_DISTURB_ENABLE, PARAMS_TYPE_BOOL, "doNotDisturbEnabled"},
    {HUB_PTABLE_DO_NOT_DISTURB_TIME_RANGE, PARAMS_TYPE_CHAR, "doNotDisturbTimeRange"},
    {HUB_PTABLE_DOORBELL_LIST, PARAMS_TYPE_CHAR_ARRAY, "doorbellIDs"},
    {HUB_PTABLE_RECORD_ENABLE, PARAMS_TYPE_BOOL, "recordingEnabled"},
    {HUB_PTABLE_STA_SSID, PARAMS_TYPE_CHAR, "ssid"},
    {HUB_PTABLE_STA_PASSWD, PARAMS_TYPE_CHAR, "pwd"},
    {HUB_PTABLE_ANIMATE_PROMPT_ENABLE, PARAMS_TYPE_BOOL, "animatedPromptEnabled"},
    {HUB_PTABLE_SCREEN_TIMEOUT, PARAMS_TYPE_INT, "screenTimeout"},
    {HUB_PTABLE_MUTE_RECORD_ENABLE, PARAMS_TYPE_BOOL, "mutedRecordingEnabled"},
    {HUB_PTABLE_LVGL_BIND_CAM_ID, PARAMS_TYPE_CHAR, "lvglbindcamId"},
    {HUB_PTABLE_DETECTION_ALARM_TONE, PARAMS_TYPE_INT, "detectionAlarmTone"},
    {HUB_PTABLE_KEEP_SCREEN_ON, PARAMS_TYPE_BOOL, "keepScreenOn"},
    {HUB_PTABLE_ENABLE24HOUR, PARAMS_TYPE_BOOL, "enabel24Hour"},
    /*Immediate Params start*/
    {HUB_PTABLE_IMMED_IS_RECORDING, PARAMS_TYPE_BOOL, "isReording"},
    /*Immediate Params end*/

    {SUB_PATABLE_DETECTALARMENABLED, PARAMS_TYPE_INT, "detectAlarmEnabled"},
    {SUB_PATABLE_DETECTALARMDURATION, PARAMS_TYPE_INT, "detectAlarmDuration"},
    {SUB_PATABLE_DETECTALARMVOLUME, PARAMS_TYPE_INT, "detectAlarmVolume"},
    {SUB_PATABLE_DETECTALARMTONE, PARAMS_TYPE_INT, "detectAlarmTone"},
    {SUB_PATABLE_LOCKBUTTONSETTINGS, PARAMS_TYPE_CHAR, "lockButtonSettings"},
    {SUB_PATABLE_SNOOZE_MODE_END_TIME, PARAMS_TYPE_INT, "snoozeModeEndTime"},
};

static int fill_char_json(cJSON *root, hub_param_table_s *paramTable, char *value) {
  if (paramTable->attrType == PARAMS_TYPE_CHAR) {
    cJSON_AddItemToObject(root, paramTable->keyValue, cJSON_CreateString(value));
  } else {
    COMMONLOG_W("%s, attrType is not match(%d)", paramTable->keyValue, paramTable->attrType);
    return -1;
  }

  return 0;
}

static int fill_char_array_json(cJSON *root, hub_param_table_s *paramTable, char *value[], int size) {
  int i = 0;
  char Item[64] = {0};

  if (paramTable->attrType == PARAMS_TYPE_CHAR_ARRAY) {
    cJSON *Jarray = cJSON_CreateArray();
    if (Jarray) {
      for (i = 0; i < size; i++) {
        cJSON *JItem = cJSON_CreateObject();
        if (JItem) {
          memset(Item, 0, sizeof(Item));
          snprintf(Item, sizeof(Item), "Item_%d", i);

          if (value[i] != NULL) {
            cJSON_AddItemToObject(JItem, Item, cJSON_CreateString(value[i]));
          } else {
            COMMONLOG_W("String at index %d is NULL", i);
            cJSON_Delete(Jarray);
            return -1;
          }

          cJSON_AddItemToArray(Jarray, JItem);
        } else {
          COMMONLOG_W("Failed to create JSON item object");
          cJSON_Delete(Jarray);
          return -1;
        }
      }
      cJSON_AddItemToObject(root, paramTable->keyValue, Jarray);
    } else {
      COMMONLOG_W("Failed to create JSON array");
      return -1;
    }
  } else {
    COMMONLOG_W("%s, attrType is not match(%d)", paramTable->keyValue, paramTable->attrType);
    return -1;
  }

  return 0;
}

static int fill_int_json(cJSON *root, hub_param_table_s *paramTable, int value) {
  if (paramTable->attrType == PARAMS_TYPE_INT) {
    cJSON_AddItemToObject(root, paramTable->keyValue, cJSON_CreateNumber(value));
  } else {
    COMMONLOG_W("%s, attrType is not match(%d)", paramTable->keyValue, paramTable->attrType);
    return -1;
  }

  return 0;
}

static int fill_bool_json(cJSON *root, hub_param_table_s *paramTable, bool value) {
  if (paramTable->attrType == PARAMS_TYPE_BOOL) {
    cJSON_AddItemToObject(root, paramTable->keyValue, cJSON_CreateBool(value));
  } else {
    COMMONLOG_W("%s, attrType is not match(%d)", paramTable->keyValue, paramTable->attrType);
    return -1;
  }

  return 0;
}

static int fill_sub_param_json(cJSON *root, hub_param_table_s *paramTable, PCameraDevInfo devInfo) {
  int ret = 0;
  switch (paramTable->attrkey) {
    case SUB_PATABLE_DETECTALARMENABLED: {
      ret = fill_int_json(root, paramTable, devInfo->detectAlarmEnabled);
    } break;
    case SUB_PATABLE_DETECTALARMDURATION: {
      ret = fill_int_json(root, paramTable, devInfo->detectAlarmDuration);
    } break;
    case SUB_PATABLE_DETECTALARMVOLUME: {
      ret = fill_int_json(root, paramTable, devInfo->detectAlarmVolume);
    } break;
    case SUB_PATABLE_DETECTALARMTONE: {
      ret = fill_int_json(root, paramTable, devInfo->detectAlarmTone);
    } break;

    case SUB_PATABLE_LOCKBUTTONSETTINGS: {
      ret = fill_char_json(root, paramTable, devInfo->lockButtonSettings);
    } break;

    case SUB_PATABLE_SNOOZE_MODE_END_TIME: {
      ret = fill_int_json(root, paramTable, devInfo->snoozeModeEndTime);
    } break;
    default:
      ret = -1;
      break;
  }

  return ret;
}

static int fill_param_json(cJSON *root, hub_param_table_s *paramTable, PHubDevInfo devInfo) {
  int ret = 0;
  switch (paramTable->attrkey) {
    case HUB_PTABLE_INIT_STATE: {
      ret = fill_int_json(root, paramTable, devInfo->initState);
    } break;
    case HUB_PTABLE_BLE_MAC: {
      ret = fill_char_json(root, paramTable, devInfo->hubMac);
    } break;
    case HUB_PTABLE_WIF_MAC: {
      ret = fill_char_json(root, paramTable, devInfo->wifiMac);
    } break;
    case HUB_PTABLE_USER_ID: {
      ret = fill_char_json(root, paramTable, devInfo->userId);
    } break;
    case HUB_PTABLE_FW: {
      ret = fill_char_json(root, paramTable, devInfo->firmwareVersion);
    } break;
    case HUB_PTABLE_RSSI: {
      ret = fill_int_json(root, paramTable, devInfo->hubRssi);
    } break;
    case HUB_PTABLE_FLASH_SCREEN_ENABLE: {
      ret = fill_bool_json(root, paramTable, devInfo->flashScreenEnabled);
    } break;
    case HUB_PTABLE_SCREEN_BRIGHTNESS: {
      ret = fill_int_json(root, paramTable, devInfo->screenBrightness);
    } break;
    case HUB_PTABLE_INDICATOR_LIGHT_ENABLE: {
      ret = fill_bool_json(root, paramTable, devInfo->indicatorLightEnabled);
    } break;
    case HUB_PTABLE_RING_TONE_ENABLE: {
      ret = fill_bool_json(root, paramTable, devInfo->ringtoneEnabled);
    } break;
    case HUB_PTABLE_TONE: {
      ret = fill_int_json(root, paramTable, devInfo->tone);
    } break;
    case HUB_PTABLE_VOLUME: {
      ret = fill_int_json(root, paramTable, devInfo->volume);
    } break;
    case HUB_PTABLE_LANGUAGE: {
      ret = fill_char_json(root, paramTable, devInfo->language);
    } break;
    case HUB_PTABLE_DO_NOT_DISTURB_ENABLE: {
      ret = fill_bool_json(root, paramTable, devInfo->doNotDisturbEnabled);
    } break;
    case HUB_PTABLE_DO_NOT_DISTURB_TIME_RANGE: {
      ret = fill_char_json(root, paramTable, devInfo->doNotDisturbTimeRange);
    } break;
    case HUB_PTABLE_DOORBELL_LIST: {
      char *arrayOfPointers[MAX_DOORBELL_CAM_NUM];
      for (int i = 0; i < MAX_DOORBELL_CAM_NUM; i++) {
        arrayOfPointers[i] = devInfo->doorbellIDs[i];
      }
      ret = fill_char_array_json(root, paramTable, arrayOfPointers,
                                 sizeof(devInfo->doorbellIDs) / sizeof(devInfo->doorbellIDs[0]));
    } break;
    case HUB_PTABLE_RECORD_ENABLE: {
      ret = fill_bool_json(root, paramTable, devInfo->recordingEnabled);
    } break;
    case HUB_PTABLE_STA_SSID: {
      ret = fill_char_json(root, paramTable, devInfo->ssid);
    } break;
    case HUB_PTABLE_STA_PASSWD: {
      ret = fill_char_json(root, paramTable, devInfo->pwd);
    } break;
    case HUB_PTABLE_ANIMATE_PROMPT_ENABLE: {
      ret = fill_bool_json(root, paramTable, devInfo->animatedPromptEnabled);
    } break;
    case HUB_PTABLE_SCREEN_TIMEOUT: {
      ret = fill_int_json(root, paramTable, devInfo->screenTimeout);
    } break;
    case HUB_PTABLE_MUTE_RECORD_ENABLE: {
      ret = fill_bool_json(root, paramTable, devInfo->mutedRecordingEnabled);
    } break;
    case HUB_PTABLE_LVGL_BIND_CAM_ID: {
      ret = fill_char_json(root, paramTable, devInfo->lvglbindcamId);
    } break;
    case HUB_PTABLE_DETECTION_ALARM_TONE: {
      ret = fill_int_json(root, paramTable, devInfo->detectionAlarmTone);
    } break;
    case HUB_PTABLE_KEEP_SCREEN_ON: {
      ret = fill_bool_json(root, paramTable, devInfo->keepScreenOn);
    } break;
    case HUB_PTABLE_ENABLE24HOUR: {
      ret = fill_bool_json(root, paramTable, devInfo->enable24Hour);
    } break;

    /*Immediate Params start*/
    case HUB_PTABLE_IMMED_IS_RECORDING: {
      COMMONLOG_W("++++++++++++++++ PUB HUB_PTABLE_IMMED_IS_RECORDING(%d) +++++++++++++\n",
                  devInfo->immedParams.isReording);
      ret = fill_bool_json(root, paramTable, devInfo->immedParams.isReording);
    } break;
    /*Immediate Params end*/
    default:
      ret = -1;
      break;
  }

  return ret;
}

int mqtt_model_sub_fill_param_json(cJSON *root, PCameraDevInfo devInfo, int tableIndex) {
  int i = 0;
  int ret = 0;
  if (root == NULL) {
    return -1;
  }

  cJSON_AddItemToObject(root, "tableIndex", cJSON_CreateNumber(tableIndex));

  if (tableIndex == HUB_PARAMS_MAX) {
    for (i = 0; i < HUB_PARAMS_MAX; i++) {
      ret = fill_sub_param_json(root, &defparamsTable[i], devInfo);
    }
  } else if (tableIndex < HUB_PARAMS_MAX) {
    ret = fill_sub_param_json(root, &defparamsTable[tableIndex], devInfo);
  }

  return ret;
}

int mqtt_model_hub_fill_param_json(cJSON *root, PHubDevInfo devInfo, int tableIndex) {
  int i = 0;
  int ret = 0;
  if (root == NULL) {
    return -1;
  }

  cJSON_AddItemToObject(root, "tableIndex", cJSON_CreateNumber(tableIndex));

  if (tableIndex == HUB_PARAMS_MAX) {
    for (i = 0; i < HUB_PARAMS_MAX; i++) {
      ret = fill_param_json(root, &defparamsTable[i], devInfo);
    }
  } else if (tableIndex < HUB_PARAMS_MAX) {
    ret = fill_param_json(root, &defparamsTable[tableIndex], devInfo);
  }

  return ret;
}

static int parse_char_array_json(cJSON *root, hub_param_table_s *paramTable, char **value, int valuesize, int dataLen) {
  if (root && cJSON_IsArray(root)) {
    cJSON *sub_element = NULL;
    int index = 0;

    cJSON_ArrayForEach(sub_element, root) {
      if (index >= valuesize) {
        COMMONLOG_W("Array size exceeded at index %d", index);
        return -1;
      }

      if (!cJSON_IsString(sub_element)) {
        COMMONLOG_W("Array element at index %d is not a string", index);
        return -1;
      }

      const char *sub_value = cJSON_GetStringValue(sub_element);
      if (sub_value != NULL) {
        snprintf(value[index], dataLen, "%s", sub_value);
      } else {
        COMMONLOG_W("Failed to get string value from JSON at index %d", index);
        return -1;
      }
      index++;
    }
  } else {
    COMMONLOG_W("json %s is NULL or not match", paramTable->keyValue);
    return -1;
  }

  return 0;
}

static int parse_char_json(cJSON *root, hub_param_table_s *paramTable, char *value, int valueLen) {
  if (root && cJSON_IsString(root)) {
    snprintf(value, valueLen - 1, "%s", root->valuestring);
  } else {
    COMMONLOG_W("json %s is NULL or not match", paramTable->keyValue);
    return -1;
  }

  return 0;
}

static int parse_int_json(cJSON *root, hub_param_table_s *paramTable, int *value) {
  if (root && cJSON_IsNumber(root)) {
    *value = root->valueint;
  } else {
    COMMONLOG_W("json %s is NULL", paramTable->keyValue);
    return -1;
  }

  return 0;
}

static int parse_bool_json(cJSON *root, hub_param_table_s *paramTable, bool *value) {
  if (root && cJSON_IsBool(root)) {
    *value = cJSON_IsTrue(root);  // 使用 cJSON_IsTrue() 来获取布尔值
  } else {
    COMMONLOG_W("json %s is NULL or not a boolean", paramTable->keyValue);
    return -1;
  }

  return 0;
}

static int parse_dest_param(cJSON *root, hub_param_table_s *paramTable, PHubDevInfo devInfo) {
  switch (paramTable->attrkey) {
    case HUB_PTABLE_INIT_STATE: {
      parse_int_json(root, paramTable, &devInfo->initState);
    } break;
    case HUB_PTABLE_BLE_MAC: {
      parse_char_json(root, paramTable, devInfo->hubMac, sizeof(devInfo->hubMac));
    } break;
    case HUB_PTABLE_WIF_MAC: {
      parse_char_json(root, paramTable, devInfo->wifiMac, sizeof(devInfo->wifiMac));
    } break;
    case HUB_PTABLE_USER_ID: {
      parse_char_json(root, paramTable, devInfo->userId, sizeof(devInfo->userId));
    } break;
    case HUB_PTABLE_FW: {
      parse_char_json(root, paramTable, devInfo->firmwareVersion, sizeof(devInfo->firmwareVersion));
    } break;
    case HUB_PTABLE_RSSI: {
      parse_int_json(root, paramTable, &devInfo->hubRssi);
    } break;
    case HUB_PTABLE_FLASH_SCREEN_ENABLE: {
      parse_bool_json(root, paramTable, (bool *)&devInfo->flashScreenEnabled);
    } break;
    case HUB_PTABLE_SCREEN_BRIGHTNESS: {
      parse_int_json(root, paramTable, &devInfo->screenBrightness);
    } break;
    case HUB_PTABLE_INDICATOR_LIGHT_ENABLE: {
      parse_bool_json(root, paramTable, (bool *)&devInfo->indicatorLightEnabled);
    } break;
    case HUB_PTABLE_RING_TONE_ENABLE: {
      parse_bool_json(root, paramTable, (bool *)&devInfo->ringtoneEnabled);
    } break;
    case HUB_PTABLE_TONE: {
      parse_int_json(root, paramTable, &devInfo->tone);
    } break;
    case HUB_PTABLE_VOLUME: {
      parse_int_json(root, paramTable, &devInfo->volume);
    } break;
    case HUB_PTABLE_LANGUAGE: {
      parse_char_json(root, paramTable, devInfo->language, sizeof(devInfo->language));
    } break;
    case HUB_PTABLE_DO_NOT_DISTURB_ENABLE: {
      parse_bool_json(root, paramTable, (bool *)&devInfo->doNotDisturbEnabled);
    } break;
    case HUB_PTABLE_DO_NOT_DISTURB_TIME_RANGE: {
      parse_char_json(root, paramTable, devInfo->doNotDisturbTimeRange, sizeof(devInfo->doNotDisturbTimeRange));
    } break;
    case HUB_PTABLE_DOORBELL_LIST: {
      char *arrayOfPointers[MAX_DOORBELL_CAM_NUM];
      for (int i = 0; i < MAX_DOORBELL_CAM_NUM; i++) {
        arrayOfPointers[i] = devInfo->doorbellIDs[i];
      }
      parse_char_array_json(root, paramTable, arrayOfPointers,
                            sizeof(devInfo->doorbellIDs) / sizeof(devInfo->doorbellIDs[0]),
                            sizeof(devInfo->doorbellIDs[0]) / sizeof(devInfo->doorbellIDs[0][0]));
    } break;
    case HUB_PTABLE_RECORD_ENABLE: {
      parse_bool_json(root, paramTable, (bool *)&devInfo->recordingEnabled);
    } break;
    case HUB_PTABLE_STA_SSID: {
      parse_char_json(root, paramTable, devInfo->ssid, sizeof(devInfo->ssid));
    } break;
    case HUB_PTABLE_STA_PASSWD: {
      parse_char_json(root, paramTable, devInfo->pwd, sizeof(devInfo->pwd));
    } break;
    case HUB_PTABLE_ANIMATE_PROMPT_ENABLE: {
      parse_bool_json(root, paramTable, (bool *)&devInfo->animatedPromptEnabled);
    } break;
    case HUB_PTABLE_SCREEN_TIMEOUT: {
      parse_int_json(root, paramTable, &devInfo->screenTimeout);
    } break;
    case HUB_PTABLE_MUTE_RECORD_ENABLE: {
      parse_bool_json(root, paramTable, (bool *)&devInfo->mutedRecordingEnabled);
    } break;
    case HUB_PTABLE_LVGL_BIND_CAM_ID: {
      parse_char_json(root, paramTable, devInfo->lvglbindcamId, sizeof(devInfo->lvglbindcamId));
    } break;
    case HUB_PTABLE_DETECTION_ALARM_TONE: {
      parse_int_json(root, paramTable, &devInfo->detectionAlarmTone);
    } break;
    case HUB_PTABLE_KEEP_SCREEN_ON: {
      parse_bool_json(root, paramTable, (bool *)&devInfo->keepScreenOn);
    } break;
    case HUB_PTABLE_ENABLE24HOUR: {
      parse_bool_json(root, paramTable, (bool *)&devInfo->enable24Hour);
    } break;
    case SUB_PATABLE_DETECTALARMENABLED: {
      parse_int_json(root, paramTable, &devInfo->sub_info->detectAlarmEnabled);
    } break;
    case SUB_PATABLE_DETECTALARMDURATION: {
      parse_int_json(root, paramTable, &devInfo->sub_info->detectAlarmDuration);
    } break;
    case SUB_PATABLE_DETECTALARMVOLUME: {
      parse_int_json(root, paramTable, &devInfo->sub_info->detectAlarmVolume);
    } break;
    case SUB_PATABLE_DETECTALARMTONE: {
      parse_int_json(root, paramTable, &devInfo->sub_info->detectAlarmTone);
    } break;
    case SUB_PATABLE_LOCKBUTTONSETTINGS: {
      parse_char_json(root, paramTable, devInfo->sub_info->lockButtonSettings,
                      sizeof(devInfo->sub_info->lockButtonSettings));
    } break;
    case SUB_PATABLE_SNOOZE_MODE_END_TIME: {
      parse_int_json(root, paramTable, &devInfo->sub_info->snoozeModeEndTime);
    } break;

    /*Immediate Params start*/
    case HUB_PTABLE_IMMED_IS_RECORDING: {
      COMMONLOG_W("++++++++++++++++ SUB HUB_PTABLE_IMMED_IS_RECORDING(%d) +++++++++++++\n",
                  devInfo->immedParams.isReording);
      parse_bool_json(root, paramTable, (bool *)&devInfo->immedParams.isReording);
    } break;
    /*Immediate Params end*/
    default:
      break;
  }

  return 0;
}

static int parse_param_json(cJSON *root, hub_param_table_s *paramTable, PHubDevInfo devInfo) {
  cJSON *current_element = NULL;

  cJSON_ArrayForEach(current_element, root) {
    // 获取当前元素的 key 值
    const char *key = current_element->string;
    if (key == NULL) {
      continue;
    }

    if (strcmp(key, paramTable->keyValue) == 0) {
      parse_dest_param(current_element, paramTable, devInfo);
    }
#if 0
      // 判断当前元素的类型，并处理值
      if (cJSON_IsString(current_element)) {
          printf("Value: %s\n", current_element->valuestring);
      } else if (cJSON_IsNumber(current_element)) {
          printf("Value: %d\n", current_element->valueint);
      } else if (cJSON_IsBool(current_element)) {
          printf("Value: %s\n", cJSON_IsTrue(current_element) ? "true" : "false");
      } else if (cJSON_IsArray(current_element)) {
          printf("Value: Array\n");
          cJSON *sub_element = NULL;
          int index = 0;
          cJSON_ArrayForEach(sub_element, current_element) {
              printf("  [%d]: %s\n", index, cJSON_GetStringValue(sub_element));
              index++;
          }
      } else if (cJSON_IsObject(current_element)) {
          printf("Value: Object\n");
          // 递归处理嵌套对象
          print_json_keys(current_element);
      }
#endif
  }

  return 0;
}

int mqtt_model_hub_parse_param_json(cJSON *root, PHubDevInfo devInfo, int tableIndex) {
  int i = 0;
  if (root == NULL) {
    return 0;
  }

  if (tableIndex == HUB_PARAMS_MAX) {
    for (i = 0; i < HUB_PARAMS_MAX; i++) {
      parse_param_json(root, &defparamsTable[i], devInfo);
    }
  } else if (tableIndex < HUB_PARAMS_MAX) {
    parse_param_json(root, &defparamsTable[tableIndex], devInfo);
  }

  return 0;
}
