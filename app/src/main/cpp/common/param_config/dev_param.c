#include "dev_param.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#define LOG_TAG "DEV_PARAM"
#include "cjson/cJSON.h"
#include "common/Common_toolbox.h"
#include "common/api/comm_api.h"

/*************************************************************
 * To add a parameter in the parameter server, the following
 * functions need change:
 *  DEV_PARAM_SetDefaultConf
 *  appsetConfToJson
 *  jsonToAppsetConf
 *  DEV_PARAM_GetParam
 *  DEV_PARAM_SetParam
 *  DEV_PARAM_MakeConfEffect(if necessary)
 **************************************************************/

#define CURRENT_VERSION (8)  // 20231025

/* app端设置相关参数 */
typedef struct APPSET_CONF {
  union {
    struct {
      unsigned int version;    // 参数版本
      char timezone[64];       // 时区ID
      char timeZoneId[64];     // 时区ID
      char timeZonePosix[64];  // 设置时区TZ
      char token[128];         // 扫码添加时的token
      char streamName[128];    //
      char region[32];
      char logLevel[32];
      unsigned char contrast;         // 对比度 0~255
      unsigned char brightness;       // 亮度 0~255
      unsigned char sharpness;        // 锐化度 0~255
      unsigned char saturation;       // 饱和度 0~255
      unsigned char md;               // 移动侦测 0：关闭 1：开启
      unsigned char mdSense;          // 移动侦测灵敏度
      unsigned char humanFilter;      // 人形过滤开关 0:关闭 1:打开
      unsigned char resolution;       // 用户分辨率设置 0：2k 1：360p 2:480p 默认2k
      unsigned char indicatorSwitch;  // 指示灯开关 	 0 关闭     1 开启
      unsigned char nightVision;      // 夜视模式 0:自动 1:关闭         2:打开
      unsigned char volume;           // 输出音量设置 0:高 1:中 2:低 (version:5)
      unsigned char flip;             // 画面翻转 0:正常 1:翻转
      unsigned char privateMode;      // 隐私模式 0:关闭 1:打开
      unsigned char timeWatermark;    // 时间水印 0:关闭 1:打开
      unsigned char talkMode;         // 对讲模式 0:单向 1:双向
      unsigned char autoTracking;     // 移动追踪
      unsigned char upgradeFlag;
      unsigned char devMode;            // 0:reset 1:bunding
      unsigned char soundAlarm;         // 声音报警 0：关闭 1：打开
      unsigned char autoUpgrade;        // 自动升级 0：关闭 1：打开
      unsigned char enUploadKeyLog;     // 关键日志上传 0：关闭 1：打开
      unsigned char enUploadDetailLog;  // 详细日志上传 0：关闭 1：打开
      unsigned char enAlgoLog;          // 算法日志 0：关闭 1：打开
      DETECT_ALARM_PARAM_S detAlarm;    // 事件告警设置 (version:3)
      unsigned char antiFlicker;        // 抗闪烁    0:关闭 1:50HZ   2:60HZ (version:2)
      unsigned char cruise;             // 巡航计划总开关
      DEV_POINT_S prePoint;             // 断电电机位置记录
      long long cloudStarTime;          //
      long long cloudEndTime;           //
      AWS_CONF_S aws_conf;
      unsigned char darkFullColor;  // 微光全彩 0：关闭 1：打开 (version:8)
      EVENT_SCHEDULE_S eventsch_conf;
      REC_CONF_S record_conf;
    } data;
    unsigned char raw[1024 * 4];
  };
} APPSET_CONF_S;

typedef struct DEV_INFO {
  DEV_BASE_INFO_S devBaseInfo;
  APPSET_CONF_S appSetInfo;
  int stopFlag;
  long lastMotorPosUpdateTime;
  bool isParamReady;
  pthread_mutex_t lock;
} DEV_INFO_S;

static DEV_INFO_S *pAppParam = NULL;
static DEV_RESOLUTION_INFO_S *pResolutionInfo = NULL;

/*****************************************************
 * @fn	    	DEV_PARAM_ReadHwVersion
 * @brief		读取硬件版本号
 * @param
 * @return 	-1 :fail > 0 : success
 ******************************************************/
static int DEV_PARAM_ReadHwVersion(void) {
  int len, fileSize = 0, hw_version = -1;
  char version[8] = {0};
  char connect[1024] = {0};
  FILE *file;
  char *hwStart, *hwEnd;

  file = fopen("/proc/cmdline", "rb");
  if (file == NULL) {
    printf("Failed to open /proc/cmdline\n");
    return -1;
  }
  fseek(file, 0, SEEK_SET);
  fileSize = fread(connect, 1, 1024, file);
  if (fileSize <= 0 || fileSize >= 1023) {
    printf("Failed to read /proc/cmdline\n");
    fclose(file);
    return -1;
  }
  printf("Contents of /proc/cmdline:%s", connect);

  hwStart = strstr(connect, "hw_version=");
  if (hwStart == NULL) {
    printf("Failed to get hw_version\n");
    fclose(file);
    return -1;
  }
  hwStart += strlen("hw_version=");
  if ((hwEnd = strchr(hwStart, ' ')) == NULL) {
    if ((hwEnd = strchr(hwStart, '\n')) == NULL) {
      if ((hwEnd = strchr(hwStart, '\0')) == NULL) {
        printf("Failed to get end\n");
        fclose(file);
        return -1;
      }
    }
  }
  len = hwEnd - hwStart;
  strncpy(version, hwStart, len);
  printf("Read hw_version: %s (len: %d)\n", version, len);
  hw_version = atoi(version);
  printf("Read hw_version: %d\n", hw_version);
  fclose(file);
  return hw_version;
}

/*****************************************************
 * @fn	    	DEV_PARAM_SetDefaultConf
 * @brief		设置默认参数
 * @param
 * @return 	N/A
 ******************************************************/
static void DEV_PARAM_SetDefaultConf(APPSET_CONF_S *pParam) {
  int val;
  if (pParam) {
    pParam->data.version = CURRENT_VERSION;
    pParam->data.contrast = 128;
    pParam->data.brightness = 128;
    pParam->data.sharpness = 128;
    pParam->data.saturation = 128;
    pParam->data.md = 1;                 /**移动侦测默认打开 */
    pParam->data.mdSense = 1;            /**移动侦测灵敏度默认为中 */
    pParam->data.humanFilter = 0;        /**人形过滤默认关闭 */
    pParam->data.resolution = 0;         /**分辨率默认2K,随APP拉流更改 */
    pParam->data.indicatorSwitch = 1;    /**指示灯默认打开 */
    pParam->data.nightVision = 0;        /**默认自动 */
    pParam->data.volume = 5;             /**对讲音量默认5 */
    pParam->data.flip = 0;               /**画面翻转默认关闭 */
    pParam->data.privateMode = 0;        /**隐私模式默认关闭 */
    pParam->data.timeWatermark = 1;      /**时间水印默认打开 */
    pParam->data.talkMode = 0;           /**对讲模式默认单向 */
    pParam->data.devMode = 0;            /**设备默认reset状态 */
    pParam->data.autoTracking = 0;       /**移动追踪默认关闭 */
    pParam->data.enAlgoLog = 0;          /**算法日志默认关闭 */
    pParam->data.enUploadDetailLog = 1;  /**详细日志上传默认打开 */
    pParam->data.enUploadKeyLog = 1;     /**关键日志上传默认打开 */
    pParam->data.detAlarm.detSwitch = 0; /**侦测报警默认关闭 */
    pParam->data.detAlarm.tone = 1;      /**侦测报警默认音效1 */
    pParam->data.detAlarm.volume = 5;    /**侦测报警默认音量5 */
    pParam->data.detAlarm.duration = 5;  /**侦测报警默认播放时长5s */
    pParam->data.autoUpgrade = 1;        /**自动升级默认打开 */
    pParam->data.soundAlarm = 0;         /**声音报警默认关闭 */
    pParam->data.antiFlicker = 2;        /**默认60HZ */
    pParam->data.cruise = 0;             /**默认关闭 */
    pParam->data.prePoint.x = 2047;
    pParam->data.prePoint.y = 458;
    pParam->data.darkFullColor = 1; /**微光全彩默认打开 */

    pParam->data.record_conf.enrecord = 1;   /**本地录像默认开启 */
    pParam->data.record_conf.recordType = 1; /**本地录像默认持续录像 */
    pParam->data.record_conf.muteRecord = 0; /**静音录像默认关闭 */

    val = DEV_PARAM_ReadHwVersion();
    if (val > 0) {
      pAppParam->devBaseInfo.hwVersion = val;
    }
    strncpy(pAppParam->appSetInfo.data.aws_conf.endpoint, END_POINT_ONLINE, ENDPOINT_SIZE);
    strncpy(pAppParam->appSetInfo.data.aws_conf.role_alias, ROLE_ALIAS_ONLINE, ROLE_ALIAS_SIZE);

    pAppParam->appSetInfo.data.eventsch_conf.enEventSchedule = true;
    pAppParam->appSetInfo.data.eventsch_conf.monSchedule = 16777215;
    pAppParam->appSetInfo.data.eventsch_conf.tueSchedule = 16777215;
    pAppParam->appSetInfo.data.eventsch_conf.wedSchedule = 16777215;
    pAppParam->appSetInfo.data.eventsch_conf.thursSchedule = 16777215;
    pAppParam->appSetInfo.data.eventsch_conf.friSchedule = 16777215;
    pAppParam->appSetInfo.data.eventsch_conf.staSchedule = 16777215;
    pAppParam->appSetInfo.data.eventsch_conf.sunSchedule = 16777215;
  }
}

static char *appsetConfToJson(APPSET_CONF_S *appsetConf) {
  cJSON *root = cJSON_CreateObject();

  cJSON_AddNumberToObject(root, "version", appsetConf->data.version);
  cJSON_AddStringToObject(root, "timezone", appsetConf->data.timezone);
  cJSON_AddStringToObject(root, "timeZoneId", appsetConf->data.timeZoneId);
  cJSON_AddStringToObject(root, "timeZonePosix", appsetConf->data.timeZonePosix);
  cJSON_AddStringToObject(root, "token", appsetConf->data.token);
  cJSON_AddStringToObject(root, "streamName", appsetConf->data.streamName);
  cJSON_AddStringToObject(root, "region", appsetConf->data.region);
  cJSON_AddStringToObject(root, "logLevel", appsetConf->data.logLevel);
  cJSON_AddNumberToObject(root, "contrast", appsetConf->data.contrast);
  cJSON_AddNumberToObject(root, "brightness", appsetConf->data.brightness);
  cJSON_AddNumberToObject(root, "sharpness", appsetConf->data.sharpness);
  cJSON_AddNumberToObject(root, "saturation", appsetConf->data.saturation);
  cJSON_AddNumberToObject(root, "md", appsetConf->data.md);
  cJSON_AddNumberToObject(root, "mdSense", appsetConf->data.mdSense);
  cJSON_AddNumberToObject(root, "humanFilter", appsetConf->data.humanFilter);
  cJSON_AddNumberToObject(root, "resolution", appsetConf->data.resolution);
  cJSON_AddNumberToObject(root, "indicatorSwitch", appsetConf->data.indicatorSwitch);
  cJSON_AddNumberToObject(root, "nightVision", appsetConf->data.nightVision);
  cJSON_AddNumberToObject(root, "volume", appsetConf->data.volume);
  cJSON_AddNumberToObject(root, "flip", appsetConf->data.flip);
  cJSON_AddNumberToObject(root, "privateMode", appsetConf->data.privateMode);
  cJSON_AddNumberToObject(root, "timeWatermark", appsetConf->data.timeWatermark);
  cJSON_AddNumberToObject(root, "talkMode", appsetConf->data.talkMode);
  cJSON_AddNumberToObject(root, "autoTracking", appsetConf->data.autoTracking);
  cJSON_AddNumberToObject(root, "enAlgoLog", appsetConf->data.enAlgoLog);
  cJSON_AddNumberToObject(root, "enUploadDetailLog", appsetConf->data.enUploadDetailLog);
  cJSON_AddNumberToObject(root, "enUploadKeyLog", appsetConf->data.enUploadKeyLog);
  cJSON_AddNumberToObject(root, "upgradeFlag", appsetConf->data.upgradeFlag);
  cJSON_AddNumberToObject(root, "devMode", appsetConf->data.devMode);
  cJSON_AddNumberToObject(root, "soundAlarm", appsetConf->data.soundAlarm);
  cJSON_AddNumberToObject(root, "autoUpgrade_v2", appsetConf->data.autoUpgrade);
  cJSON_AddNumberToObject(root, "antiFlicker", appsetConf->data.antiFlicker);
  cJSON_AddNumberToObject(root, "cruise", appsetConf->data.cruise);
  cJSON_AddNumberToObject(root, "cloudEndTime", appsetConf->data.cloudEndTime);
  cJSON_AddNumberToObject(root, "cloudStarTime", appsetConf->data.cloudStarTime);
  cJSON_AddNumberToObject(root, "darkFullColor", appsetConf->data.darkFullColor);

  cJSON *detAlarm = cJSON_CreateObject();
  cJSON_AddNumberToObject(detAlarm, "detSwitch", appsetConf->data.detAlarm.detSwitch);
  cJSON_AddNumberToObject(detAlarm, "tone", appsetConf->data.detAlarm.tone);
  cJSON_AddNumberToObject(detAlarm, "volume", appsetConf->data.detAlarm.volume);
  cJSON_AddNumberToObject(detAlarm, "duration", appsetConf->data.detAlarm.duration);
  cJSON_AddItemToObject(root, "detAlarm", detAlarm);

  cJSON *prePoint = cJSON_CreateObject();
  cJSON_AddNumberToObject(prePoint, "x", appsetConf->data.prePoint.x);
  cJSON_AddNumberToObject(prePoint, "y", appsetConf->data.prePoint.y);
  cJSON_AddItemToObject(root, "prePoint", prePoint);

  cJSON *awsConf = cJSON_CreateObject();
  cJSON_AddStringToObject(awsConf, "endpoint", appsetConf->data.aws_conf.endpoint);
  cJSON_AddStringToObject(awsConf, "iot_url", appsetConf->data.aws_conf.iot_url);
  cJSON_AddStringToObject(awsConf, "role_alias", appsetConf->data.aws_conf.role_alias);
  cJSON_AddItemToObject(root, "awsConf", awsConf);

  cJSON *alarmchConf = cJSON_CreateObject();
  cJSON_AddBoolToObject(alarmchConf, "enEventSchedule", appsetConf->data.eventsch_conf.enEventSchedule);
  cJSON_AddNumberToObject(alarmchConf, "monSchedule", appsetConf->data.eventsch_conf.monSchedule);
  cJSON_AddNumberToObject(alarmchConf, "tueSchedule", appsetConf->data.eventsch_conf.tueSchedule);
  cJSON_AddNumberToObject(alarmchConf, "wedSchedule", appsetConf->data.eventsch_conf.wedSchedule);
  cJSON_AddNumberToObject(alarmchConf, "thursSchedule", appsetConf->data.eventsch_conf.thursSchedule);
  cJSON_AddNumberToObject(alarmchConf, "friSchedule", appsetConf->data.eventsch_conf.friSchedule);
  cJSON_AddNumberToObject(alarmchConf, "staSchedule", appsetConf->data.eventsch_conf.staSchedule);
  cJSON_AddNumberToObject(alarmchConf, "sunSchedule", appsetConf->data.eventsch_conf.sunSchedule);
  cJSON_AddItemToObject(root, "alarmchConf", alarmchConf);

  cJSON *recordConf = cJSON_CreateObject();
  cJSON_AddNumberToObject(recordConf, "enrecord", appsetConf->data.record_conf.enrecord);
  cJSON_AddNumberToObject(recordConf, "recordType", appsetConf->data.record_conf.recordType);
  cJSON_AddNumberToObject(recordConf, "muteRecord", appsetConf->data.record_conf.muteRecord);
  cJSON_AddItemToObject(root, "recordConf", recordConf);

  char *jsonString = cJSON_Print(root);

  cJSON_Delete(root);

  return jsonString;
}

int jsonToAppsetConf(APPSET_CONF_S *appsetConf, const char *jsonString) {
  int oldrecordType = 0;
  int oldmuteRecord = 0;
  int syncFile = 0;
  DEV_PARAM_SetDefaultConf(appsetConf);
  cJSON *root = cJSON_Parse(jsonString);
  if (root == NULL) {
    printf("Failed to parse param json field\n");
    return -1;
  }

  cJSON *version = cJSON_GetObjectItem(root, "version");
  if (version != NULL) {
    appsetConf->data.version = version->valueint;
  } else {
    printf("Failed to get field: version from JSON file\n");
  }
  cJSON *timezone = cJSON_GetObjectItem(root, "timeZoneID");
  if (timezone != NULL) {
    strncpy(appsetConf->data.timezone, timezone->valuestring, sizeof(appsetConf->data.timezone));
  } else {
    printf("Failed to get field: timeZoneID from JSON file\n");
  }
  cJSON *timeZonePosix = cJSON_GetObjectItem(root, "timeZonePosix");
  if (timeZonePosix != NULL) {
    strncpy(appsetConf->data.timeZonePosix, timeZonePosix->valuestring, sizeof(appsetConf->data.timeZonePosix));
  } else {
    printf("Failed to get field: timeZonePosix from JSON file\n");
  }
  cJSON *token = cJSON_GetObjectItem(root, "token");
  if (token != NULL) {
    strncpy(appsetConf->data.token, token->valuestring, sizeof(appsetConf->data.token));
  } else {
    printf("Failed to get field: token from JSON file\n");
  }
  cJSON *streamName = cJSON_GetObjectItem(root, "streamName");
  if (streamName != NULL) {
    strncpy(appsetConf->data.streamName, streamName->valuestring, sizeof(appsetConf->data.streamName));
  } else {
    printf("Failed to get field: streamName from JSON file\n");
  }
  cJSON *region = cJSON_GetObjectItem(root, "region");
  if (region != NULL) {
    strncpy(appsetConf->data.region, region->valuestring, sizeof(appsetConf->data.region));
  } else {
    printf("Failed to get field: region from JSON file\n");
  }
  cJSON *logLevel = cJSON_GetObjectItem(root, "logLevel");
  if (logLevel != NULL) {
    strncpy(appsetConf->data.logLevel, logLevel->valuestring, sizeof(appsetConf->data.logLevel));
  } else {
    strncpy(appsetConf->data.logLevel, "info", strlen("info"));
    printf("Failed to get field: logLevel from JSON file\n");
  }
  cJSON *contrast = cJSON_GetObjectItem(root, "contrast");
  if (contrast != NULL) {
    appsetConf->data.contrast = contrast->valueint;
  } else {
    printf("Failed to get field: contrast from JSON file\n");
  }
  cJSON *brightness = cJSON_GetObjectItem(root, "brightness");
  if (brightness != NULL) {
    appsetConf->data.brightness = brightness->valueint;
  } else {
    printf("Failed to get field: brightness from JSON file\n");
  }
  cJSON *sharpness = cJSON_GetObjectItem(root, "sharpness");
  if (sharpness != NULL) {
    appsetConf->data.sharpness = sharpness->valueint;
  } else {
    printf("Failed to get field: sharpness from JSON file\n");
  }
  cJSON *saturation = cJSON_GetObjectItem(root, "saturation");
  if (saturation != NULL) {
    appsetConf->data.saturation = saturation->valueint;
  } else {
    printf("Failed to get field: saturation from JSON file\n");
  }
  cJSON *md = cJSON_GetObjectItem(root, "md");
  if (md != NULL) {
    appsetConf->data.md = md->valueint;
  } else {
    printf("Failed to get field: md from JSON file\n");
  }
  cJSON *mdSense = cJSON_GetObjectItem(root, "mdSense");
  if (mdSense != NULL) {
    appsetConf->data.mdSense = mdSense->valueint;
  } else {
    printf("Failed to get field: mdSense from JSON file\n");
  }
  cJSON *humanFilter = cJSON_GetObjectItem(root, "humanFilter");
  if (humanFilter != NULL) {
    appsetConf->data.humanFilter = humanFilter->valueint;
  } else {
    printf("Failed to get field: humanFilter from JSON file\n");
  }
  cJSON *resolution = cJSON_GetObjectItem(root, "resolution");
  if (resolution != NULL) {
    appsetConf->data.resolution = resolution->valueint;
  } else {
    printf("Failed to get field: resolution from JSON file\n");
  }
  cJSON *indicatorSwitch = cJSON_GetObjectItem(root, "indicatorSwitch");
  if (indicatorSwitch != NULL) {
    appsetConf->data.indicatorSwitch = indicatorSwitch->valueint;
  } else {
    printf("Failed to get field: indicatorSwitch from JSON file\n");
  }
  cJSON *recordType = cJSON_GetObjectItem(root, "recordType");
  if (recordType != NULL) {
    oldrecordType = recordType->valueint;
    syncFile = 1;
  } else {
    printf("Failed to get field: recordType from JSON file\n");
  }
  cJSON *nightVision = cJSON_GetObjectItem(root, "nightVision");
  if (nightVision != NULL) {
    appsetConf->data.nightVision = nightVision->valueint;
  } else {
    printf("Failed to get field: nightVision from JSON file\n");
  }
  cJSON *volume = cJSON_GetObjectItem(root, "volume");
  if (volume != NULL) {
    appsetConf->data.volume = volume->valueint;
  } else {
    printf("Failed to get field: volume from JSON file\n");
  }
  cJSON *flip = cJSON_GetObjectItem(root, "flip");
  if (flip != NULL) {
    appsetConf->data.flip = flip->valueint;
  } else {
    printf("Failed to get field: flip from JSON file\n");
  }
  cJSON *privateMode = cJSON_GetObjectItem(root, "privateMode");
  if (privateMode != NULL) {
    appsetConf->data.privateMode = privateMode->valueint;
  } else {
    printf("Failed to get field: privateMode from JSON file\n");
  }
  cJSON *timeWatermark = cJSON_GetObjectItem(root, "timeWatermark");
  if (timeWatermark != NULL) {
    appsetConf->data.timeWatermark = timeWatermark->valueint;
  } else {
    printf("Failed to get field: timeWatermark from JSON file\n");
  }
  cJSON *talkMode = cJSON_GetObjectItem(root, "talkMode");
  if (talkMode != NULL) {
    appsetConf->data.talkMode = talkMode->valueint;
  } else {
    printf("Failed to get field: talkMode from JSON file\n");
  }
  cJSON *autoTracking = cJSON_GetObjectItem(root, "autoTracking");
  if (autoTracking != NULL) {
    appsetConf->data.autoTracking = autoTracking->valueint;
  } else {
    printf("Failed to get field: autoTracking from JSON file\n");
  }
  cJSON *enAlgoLog = cJSON_GetObjectItem(root, "enAlgoLog");
  if (enAlgoLog != NULL) {
    appsetConf->data.enAlgoLog = enAlgoLog->valueint;
  } else {
    appsetConf->data.enAlgoLog = 0;
    printf("Failed to get field: enAlgoLog from JSON file\n");
  }
  cJSON *enUploadDetailLog = cJSON_GetObjectItem(root, "enUploadDetailLog");
  if (enUploadDetailLog != NULL) {
    appsetConf->data.enUploadDetailLog = enUploadDetailLog->valueint;
  } else {
    appsetConf->data.enUploadDetailLog = 1;
    printf("Failed to get field: enUploadDetailLog from JSON file\n");
  }
  cJSON *enUploadKeyLog = cJSON_GetObjectItem(root, "enUploadKeyLog");
  if (enUploadKeyLog != NULL) {
    appsetConf->data.enUploadKeyLog = enUploadKeyLog->valueint;
  } else {
    appsetConf->data.enUploadKeyLog = 1;
    printf("Failed to get field: enUploadKeyLog from JSON file\n");
  }
  cJSON *upgradeFlag = cJSON_GetObjectItem(root, "upgradeFlag");
  if (upgradeFlag != NULL) {
    appsetConf->data.upgradeFlag = upgradeFlag->valueint;
  } else {
    printf("Failed to get field: upgradeFlag from JSON file\n");
  }
  cJSON *devMode = cJSON_GetObjectItem(root, "devMode");
  if (devMode != NULL) {
    appsetConf->data.devMode = devMode->valueint;
  } else {
    printf("Failed to get field: devMode from JSON file\n");
  }
  cJSON *soundAlarm = cJSON_GetObjectItem(root, "soundAlarm");
  if (soundAlarm != NULL) {
    appsetConf->data.soundAlarm = soundAlarm->valueint;
  } else {
    printf("Failed to get field: soundAlarm from JSON file\n");
  }
  cJSON *autoUpgrade = cJSON_GetObjectItem(root, "autoUpgrade_v2");
  if (autoUpgrade != NULL) {
    appsetConf->data.autoUpgrade = autoUpgrade->valueint;
  } else {
    appsetConf->data.autoUpgrade = 1;
    syncFile = 1;
    printf("Failed to get field: autoUpgrade from JSON file\n");
  }
  cJSON *muteRecord = cJSON_GetObjectItem(root, "muteRecord");
  if (muteRecord != NULL) {
    oldmuteRecord = muteRecord->valueint;
    syncFile = 1;
  } else {
    printf("Failed to get field: muteRecord from JSON file\n");
  }
  cJSON *antiFlicker = cJSON_GetObjectItem(root, "antiFlicker");
  if (antiFlicker != NULL) {
    appsetConf->data.antiFlicker = antiFlicker->valueint;
  } else {
    printf("Failed to get field: antiFlicker from JSON file\n");
  }
  cJSON *cruise = cJSON_GetObjectItem(root, "cruise");
  if (cruise != NULL) {
    appsetConf->data.cruise = cruise->valueint;
  } else {
    printf("Failed to get field: cruise from JSON file\n");
  }
  cJSON *cloudEndTime = cJSON_GetObjectItem(root, "cloudEndTime");
  if (cloudEndTime != NULL) {
    appsetConf->data.cloudEndTime = cloudEndTime->valueint;
  } else {
    printf("Failed to get field: cloudEndTime from JSON file\n");
  }
  cJSON *cloudStarTime = cJSON_GetObjectItem(root, "cloudStarTime");
  if (cloudStarTime != NULL) {
    appsetConf->data.cloudStarTime = cloudStarTime->valueint;
  } else {
    printf("Failed to get field: cloudStarTime from JSON file\n");
  }
  cJSON *darkFullColor = cJSON_GetObjectItem(root, "darkFullColor");
  if (darkFullColor != NULL) {
    appsetConf->data.darkFullColor = darkFullColor->valueint;
  } else {
    printf("Failed to get field: darkFullColor from JSON file\n");
  }

  cJSON *awsConf = cJSON_GetObjectItem(root, "awsConf");
  if (awsConf != NULL) {
    cJSON *endpoint = cJSON_GetObjectItem(awsConf, "endpoint");
    if (endpoint != NULL) {
      strncpy(appsetConf->data.aws_conf.endpoint, endpoint->valuestring, sizeof(appsetConf->data.aws_conf.endpoint));
    } else {
      printf("Failed to get field: endpoint from JSON file\n");
    }
    cJSON *iot_url = cJSON_GetObjectItem(awsConf, "iot_url");
    if (iot_url != NULL) {
      strncpy(appsetConf->data.aws_conf.iot_url, iot_url->valuestring, sizeof(appsetConf->data.aws_conf.iot_url));
    } else {
      printf("Failed to get field: iot_url from JSON file\n");
    }
    cJSON *role_alias = cJSON_GetObjectItem(awsConf, "role_alias");
    if (role_alias != NULL) {
      strncpy(appsetConf->data.aws_conf.role_alias, role_alias->valuestring,
              sizeof(appsetConf->data.aws_conf.role_alias));
    } else {
      printf("Failed to get field: role_alias from JSON file\n");
    }
  } else {
    printf("Failed to get field: awsConf from JSON file\n");
  }

  cJSON *prePoint = cJSON_GetObjectItem(root, "prePoint");
  if (prePoint != NULL) {
    cJSON *x = cJSON_GetObjectItem(prePoint, "x");
    if (x != NULL) {
      appsetConf->data.prePoint.x = x->valueint;
    } else {
      printf("Failed to get field: x from JSON file\n");
    }
    cJSON *y = cJSON_GetObjectItem(prePoint, "y");
    if (y != NULL) {
      appsetConf->data.prePoint.y = y->valueint;
    } else {
      printf("Failed to get field: y from JSON file\n");
    }
  } else {
    printf("Failed to get field: prePoint from JSON file\n");
  }

  cJSON *detAlarm = cJSON_GetObjectItem(root, "detAlarm");
  if (detAlarm != NULL) {
    cJSON *detSwitch = cJSON_GetObjectItem(detAlarm, "detSwitch");
    if (detSwitch != NULL) {
      appsetConf->data.detAlarm.detSwitch = detSwitch->valueint;
    } else {
      printf("Failed to get field: detSwitch from JSON file\n");
    }
    cJSON *tone = cJSON_GetObjectItem(detAlarm, "tone");
    if (tone != NULL) {
      appsetConf->data.detAlarm.tone = tone->valueint;
    } else {
      printf("Failed to get field: tone from JSON file\n");
    }
    cJSON *volume = cJSON_GetObjectItem(detAlarm, "volume");
    if (volume != NULL) {
      appsetConf->data.detAlarm.volume = volume->valueint;
    } else {
      printf("Failed to get field: volume from JSON file\n");
    }
    cJSON *duration = cJSON_GetObjectItem(detAlarm, "duration");
    if (duration != NULL) {
      appsetConf->data.detAlarm.duration = duration->valueint;
    } else {
      printf("Failed to get field: duration from JSON file\n");
    }
  } else {
    printf("Failed to get field: detAlarm from JSON file\n");
  }

  cJSON *alarmchConf = cJSON_GetObjectItem(root, "alarmchConf");
  if (alarmchConf != NULL) {
    cJSON *enEventSch = cJSON_GetObjectItem(alarmchConf, "enEventSchedule");
    if (enEventSch != NULL) {
      appsetConf->data.eventsch_conf.enEventSchedule = enEventSch->valueint == 1 ? true : false;
    } else {
      printf("Failed to get field: enEventSch from JSON file\n");
    }

    cJSON *monSch = cJSON_GetObjectItem(alarmchConf, "monSchedule");
    if (monSch != NULL) {
      appsetConf->data.eventsch_conf.monSchedule = monSch->valueint;
    } else {
      printf("Failed to get field: monSch from JSON file\n");
    }

    cJSON *tueSch = cJSON_GetObjectItem(alarmchConf, "tueSchedule");
    if (tueSch != NULL) {
      appsetConf->data.eventsch_conf.tueSchedule = tueSch->valueint;
    } else {
      printf("Failed to get field: tueSch from JSON file\n");
    }

    cJSON *wedSch = cJSON_GetObjectItem(alarmchConf, "wedSchedule");
    if (wedSch != NULL) {
      appsetConf->data.eventsch_conf.wedSchedule = wedSch->valueint;
    } else {
      printf("Failed to get field: wedSch from JSON file\n");
    }

    cJSON *thursSch = cJSON_GetObjectItem(alarmchConf, "thursSchedule");
    if (thursSch != NULL) {
      appsetConf->data.eventsch_conf.thursSchedule = thursSch->valueint;
    } else {
      printf("Failed to get field: thursSch from JSON file\n");
    }

    cJSON *friSch = cJSON_GetObjectItem(alarmchConf, "friSchedule");
    if (friSch != NULL) {
      appsetConf->data.eventsch_conf.friSchedule = friSch->valueint;
    } else {
      printf("Failed to get field: friSch from JSON file\n");
    }

    cJSON *staSch = cJSON_GetObjectItem(alarmchConf, "staSchedule");
    if (staSch != NULL) {
      appsetConf->data.eventsch_conf.staSchedule = staSch->valueint;
      printf("=============== staSchedule=%d", appsetConf->data.eventsch_conf.staSchedule);
    } else {
      printf("Failed to get field: staSch from JSON file\n");
    }

    cJSON *sunSch = cJSON_GetObjectItem(alarmchConf, "sunSchedule");
    if (sunSch != NULL) {
      appsetConf->data.eventsch_conf.sunSchedule = sunSch->valueint;
    } else {
      printf("Failed to get field: sunSch from JSON file\n");
    }
  } else {
    printf("Failed to get field: alarmchConf from JSON file, use default value");
    appsetConf->data.eventsch_conf.enEventSchedule = true;
    appsetConf->data.eventsch_conf.monSchedule = 16777215;
    appsetConf->data.eventsch_conf.tueSchedule = 16777215;
    appsetConf->data.eventsch_conf.wedSchedule = 16777215;
    appsetConf->data.eventsch_conf.thursSchedule = 16777215;
    appsetConf->data.eventsch_conf.friSchedule = 16777215;
    appsetConf->data.eventsch_conf.staSchedule = 16777215;
    appsetConf->data.eventsch_conf.sunSchedule = 16777215;
  }

  cJSON *recordConf = cJSON_GetObjectItem(root, "recordConf");
  if (recordConf != NULL) {
    cJSON *enRecord = cJSON_GetObjectItem(recordConf, "enRecord");
    if (enRecord != NULL) {
      appsetConf->data.record_conf.enrecord = enRecord->valueint;
    } else {
      appsetConf->data.record_conf.enrecord = 0;
      printf("Failed to get field: enRecord from JSON file\n");
    }

    cJSON *recType = cJSON_GetObjectItem(recordConf, "recordType");
    if (recType != NULL) {
      appsetConf->data.record_conf.recordType = recType->valueint;
    } else {
      appsetConf->data.record_conf.recordType = 2;
      printf("Failed to get field: recType from JSON file\n");
    }

    cJSON *muteRec = cJSON_GetObjectItem(recordConf, "muteRecord");
    if (muteRec != NULL) {
      appsetConf->data.record_conf.muteRecord = muteRec->valueint;
    } else {
      appsetConf->data.record_conf.muteRecord = 0;
      printf("Failed to get field: muteRec from JSON file\n");
    }

  } else {
    if (oldrecordType) {
      appsetConf->data.record_conf.enrecord = 1;
      appsetConf->data.record_conf.recordType = oldrecordType;
    } else {
      appsetConf->data.record_conf.enrecord = 0;
      appsetConf->data.record_conf.recordType = 2;
    }

    if (oldmuteRecord) {
      appsetConf->data.record_conf.muteRecord = oldmuteRecord;
    } else {
      appsetConf->data.record_conf.muteRecord = 0;
    }
  }

  cJSON_Delete(root);

  if (syncFile) {
    syncParamToFile();
  }
  return 0;
}

static int readParamFromFile(const char *filePath, APPSET_CONF_S *param) {
  if (filePath == NULL || param == NULL) {
    printf("Invalid input\n");
    return -1;
  }
  FILE *file = fopen(filePath, "r");
  if (file == NULL) {
    printf("File: %s is not exist", filePath);
    return -1;
  }
  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);
  char *jsonString = (char *)malloc(fileSize + 1);
  if (jsonString == NULL) {
    printf("Malloc error\n");
    fclose(file);
    return -1;
  }
  size_t bytesRead = fread(jsonString, 1, fileSize, file);
  if (bytesRead != fileSize) {
    printf("Read param file:%s error", filePath);
    fclose(file);
    free(jsonString);
    return -1;
  }
  jsonString[fileSize] = '\0';
  fclose(file);
  int res = jsonToAppsetConf(param, jsonString);
  printf("Read param from file: %s ok, param:\n%s", filePath, jsonString);
  free(jsonString);
  return res;
}

static int writeParamToFile(const char *filePath, APPSET_CONF_S *param, bool printLog) {
  if (filePath == NULL || param == NULL) {
    printf("Invalid input\n");
    return -1;
  }
  FILE *file = fopen(filePath, "w+");
  if (file == NULL) {
    printf("File: %s is not exist", filePath);
    return -1;
  }
  char *jsonString = appsetConfToJson(param);
  if (jsonString == NULL) {
    printf("convert param to json string failed\n");
    fclose(file);
    return -1;
  }
  fputs(jsonString, file);
  fclose(file);
  if (printLog) {
    printf("Param has been written to file: %s ok, param:\n%s", filePath, jsonString);
  }
  free(jsonString);
  return 0;
}

static int readOldParamFile(const char *filePath, APPSET_CONF_S *param) {
  FILE *fp = NULL;
  int dataLen = 0, ret = 0;
  if (pAppParam == NULL) {
    printf("pAppParam is NULL\n");
    return -1;
  }
  if ((fp = fopen(filePath, "r+")) != NULL) {
    dataLen = sizeof(APPSET_CONF_S);
    if ((ret = fread(&(pAppParam->appSetInfo), 1, dataLen, fp)) == dataLen) {
      if (pAppParam->appSetInfo.data.version < CURRENT_VERSION) {
        if (pAppParam->appSetInfo.data.version < 2) {
          pAppParam->appSetInfo.data.antiFlicker = 1;
        }
        if (pAppParam->appSetInfo.data.version < 3) {
          pAppParam->appSetInfo.data.detAlarm.tone = 1;
          pAppParam->appSetInfo.data.detAlarm.volume = 5;
          pAppParam->appSetInfo.data.detAlarm.duration = 5;
        }
        if (pAppParam->appSetInfo.data.version < 5) {
          pAppParam->appSetInfo.data.volume = 7;
        }
        if (pAppParam->appSetInfo.data.version < 8) {
          pAppParam->appSetInfo.data.darkFullColor = 1;
        }
        pAppParam->appSetInfo.data.version = CURRENT_VERSION;
      }
      if (!strlen(pAppParam->appSetInfo.data.aws_conf.endpoint)) {
        strncpy(pAppParam->appSetInfo.data.aws_conf.endpoint, END_POINT_ONLINE, ENDPOINT_SIZE);
      }
      if (!strlen(pAppParam->appSetInfo.data.aws_conf.role_alias)) {
        strncpy(pAppParam->appSetInfo.data.aws_conf.role_alias, ROLE_ALIAS_ONLINE, ROLE_ALIAS_SIZE);
      }
      printf("len=%zu, endpoint:%s", strlen(pAppParam->appSetInfo.data.aws_conf.endpoint),
             pAppParam->appSetInfo.data.aws_conf.endpoint);
      printf("len=%zu, role_alias:%s", strlen(pAppParam->appSetInfo.data.aws_conf.role_alias),
             pAppParam->appSetInfo.data.aws_conf.role_alias);
      printf("Read app config form old param file %s ok", filePath);
      return 0;
    }
  }
  printf("Read app config form old param file %s failed", filePath);
  return -1;
}

int syncParamToFile(void) {
  int ret = -1;
  if (pAppParam == NULL) {
    printf("pAppParam is NULL\n");
    return -1;
  }
  if (!pAppParam->isParamReady) {
    printf("pAppParam is not ready\n");
    return -1;
  }
  if (pAppParam->stopFlag) {
    printf("pAppParam->stopFlag is true!\n");
    return ret;
  }
  if (writeParamToFile(CONFIG_FILE_BACKUP_PATH, &pAppParam->appSetInfo, false) == 0) {
    printf("update backup param file done\n");
  } else {
    printf("update backup param file failed\n");
  }
  if (writeParamToFile(CONFIG_FILE_PATH, &pAppParam->appSetInfo, true) == 0) {
    printf("update param file done\n");
    ret = 0;
  } else {
    printf("update param file failed\n");
  }
  return ret;
}

static int syncParamFromFile(void) {
  if (pAppParam == NULL) {
    printf("pAppParam is NULL\n");
    return -1;
  }
  if (readParamFromFile(CONFIG_FILE_PATH, &pAppParam->appSetInfo) == 0) {
    printf("read param file done\n");
    goto success;
  }
  printf("read param file failed, try read backup param file\n");
  if (readParamFromFile(CONFIG_FILE_BACKUP_PATH, &pAppParam->appSetInfo) == 0) {
    printf("read backup param file done, use backup param file\n");
    goto recoverParamFile;
  }
  printf(
      "read backup param file also failed, "
      "try read old param file");
  if (readOldParamFile(OLD_CONFIG_FILE_PATH, &pAppParam->appSetInfo) == 0) {
    printf("read old param file done\n");
    goto recoverBackupParamFile;
  }
  printf(
      "read old param file also failed, "
      "try read old backup param file");
  if (readOldParamFile(OLD_CONFIG_FILE_BACKUP_PATH, &pAppParam->appSetInfo) == 0) {
    printf("read old backup param file done\n");
    goto recoverBackupParamFile;
  }
  printf(
      "read old backup param file also failed, "
      "reset param to default");
  DEV_PARAM_SetDefaultConf(&pAppParam->appSetInfo);
  return -1;
recoverBackupParamFile:
  if (writeParamToFile(CONFIG_FILE_BACKUP_PATH, &pAppParam->appSetInfo, true) == 0) {
    printf("recover param file done\n");
  } else {
    printf("recover param file failed\n");
  }
recoverParamFile:
  if (writeParamToFile(CONFIG_FILE_PATH, &pAppParam->appSetInfo, true) == 0) {
    printf("recover param file done\n");
  } else {
    printf("recover param file failed\n");
  }
success:
  if (access(OLD_CONFIG_FILE_PATH, F_OK) == 0) {
    if (unlink(OLD_CONFIG_FILE_PATH) == 0) {
      printf("unlink OLD_CONFIG_FILE_PATH success\n");
    } else {
      printf("unlink OLD_CONFIG_FILE_PATH failed\n");
    }
  }
  if (access(OLD_CONFIG_FILE_BACKUP_PATH, F_OK) == 0) {
    if (unlink(OLD_CONFIG_FILE_BACKUP_PATH) == 0) {
      printf("unlink OLD_CONFIG_FILE_BACKUP_PATH success\n");
    } else {
      printf("unlink OLD_CONFIG_FILE_BACKUP_PATH failed\n");
    }
  }
  return 0;
}

static void setIotUrl(char *regionId) {
  bool getRegionId = true;
  if (pAppParam == NULL) {
    printf("pAppParam is NULL\n");
    return;
  }
  memset(pAppParam->appSetInfo.data.aws_conf.iot_url, 0, sizeof(pAppParam->appSetInfo.data.aws_conf.iot_url));
  if (regionId != NULL) {
    printf("Get regionId: %s", regionId);
    if (strcmp(regionId, QRCODE_REGION_PROD_US_ID) == 0) {
      strcpy(pAppParam->appSetInfo.data.aws_conf.iot_url, QRCODE_REGION_PROD_US_CERT_URL);
      printf("------------- Environment: US Online -------------\n");
    } else if (strcmp(regionId, QRCODE_REGION_PROD_AP_ID) == 0) {
      strcpy(pAppParam->appSetInfo.data.aws_conf.iot_url, QRCODE_REGION_PROD_AP_CERT_URL);
      printf("------------- Environment: AP Online -------------\n");
    } else if (strcmp(regionId, QRCODE_REGION_PROD_EU_ID) == 0) {
      strcpy(pAppParam->appSetInfo.data.aws_conf.iot_url, QRCODE_REGION_PROD_EU_CERT_URL);
      printf("------------- Environment: EU Online -------------\n");
    } else if (strcmp(regionId, QRCODE_REGION_PROD_CN_ID) == 0) {
      strcpy(pAppParam->appSetInfo.data.aws_conf.iot_url, QRCODE_REGION_PROD_CN_CERT_URL);
      printf("------------- Environment: CN Online -------------\n");
    } else if (strcmp(regionId, QRCODE_REGION_TEST_US_ID) == 0) {
      strcpy(pAppParam->appSetInfo.data.aws_conf.iot_url, QRCODE_REGION_TEST_US_CERT_URL);
      printf("------------- Environment: US Offline -------------\n");
    } else if (strcmp(regionId, QRCODE_REGION_TEST_AP_ID) == 0) {
      strcpy(pAppParam->appSetInfo.data.aws_conf.iot_url, QRCODE_REGION_TEST_AP_CERT_URL);
      printf("------------- Environment: AP Offline -------------\n");
    } else if (strcmp(regionId, QRCODE_REGION_TEST_EU_ID) == 0) {
      strcpy(pAppParam->appSetInfo.data.aws_conf.iot_url, QRCODE_REGION_TEST_EU_CERT_URL);
      printf("------------- Environment: EU Offline -------------\n");
    } else if (strcmp(regionId, QRCODE_REGION_TEST_CN_ID) == 0) {
      strcpy(pAppParam->appSetInfo.data.aws_conf.iot_url, QRCODE_REGION_TEST_CN_CERT_URL);
      printf("------------- Environment: CN Offline -------------\n");
    } else if (regionId && regionId[0]) {
      strcpy(pAppParam->appSetInfo.data.aws_conf.iot_url, regionId);
      printf("------------- DOORBELL CertURL  -------------\n");
    } else {
      getRegionId = false;
      printf("Invalid region id\n");
    }
  } else {
    getRegionId = false;
    printf("Empty region id\n");
  }
  if (!getRegionId) {
    printf("Get regionID error, use default iot_cert_url");
    if ((access("/mnt/sd/parameter/off_line", F_OK)) == 0) {
      strcpy(pAppParam->appSetInfo.data.aws_conf.iot_url, QRCODE_REGION_TEST_US_CERT_URL);
      printf("------------- Environment: US Offline -------------\n");
    } else {
      strcpy(pAppParam->appSetInfo.data.aws_conf.iot_url, QRCODE_REGION_PROD_US_CERT_URL);
      printf("------------- Environment: US Online -------------\n");
    }
  }
  printf("Set iot_cert_url: %s\n", pAppParam->appSetInfo.data.aws_conf.iot_url);
}

/*****************************************************
 * @fn	    	DEV_PARAM_CheckDevInfoPath
 * @brief		检查设备配置目录是否存在，没有则创建
 * @param
 * @return 	N/A
 ******************************************************/

static int DEV_PARAM_CheckDevInfoPath(void) {
  printf("[DEV] CONFIG_DIR:%s\n", CONFIG_DIR);
  printf("[DEV] CERT_STORE_PATH:%s\n", CERT_STORE_PATH);

  if (access(CONFIG_DIR, F_OK)) {
    if (COMM_API_MkdirRecursive(CONFIG_DIR, 0777)) {
      printf("mkdir %s execyiton error(%d)!\n", CONFIG_DIR, errno);
      return -1;
    }
  }

  if (access(WIFI_CONFIG_DIR, F_OK)) {
    if (COMM_API_MkdirRecursive(WIFI_CONFIG_DIR, 0777)) {
      printf("mkdir %s execyiton error(%d)!\n", WIFI_CONFIG_DIR, errno);
      return -1;
    }
  }

  if (access(CERT_STORE_PATH, F_OK)) {
    if (COMM_API_MkdirRecursive(CERT_STORE_PATH, 0777)) {
      printf("mkdir %s execyiton error(%d)!\n", CERT_STORE_PATH, errno);
      return -1;
    }
  }

  if (access(FW_VERSION_DIR, F_OK)) {
    if (COMM_API_MkdirRecursive(FW_VERSION_DIR, 0777)) {
      printf("mkdir %s execyiton error(%d)!\n", FW_VERSION_DIR, errno);
      return -1;
    }
  }

  if (access(ROOT_DIR, F_OK)) {
    if (COMM_API_MkdirRecursive(ROOT_DIR, 0777)) {
      printf("mkdir %s execyiton error(%d)!\n", ROOT_DIR, errno);
      return -1;
    }
  }

  if (access(FW_VERSION_FILE_PATH, F_OK)) {
    char *defversion = "V1.0.0.100";
    FILE *fp = fopen(FW_VERSION_FILE_PATH, "w");
    if (fp) {
      printf("[DEV] open:%s success", FW_VERSION_FILE_PATH);
      fwrite(defversion, 1, strlen(defversion), fp);
      fclose(fp);
    }
  }

  return 0;
}

/*****************************************************
 * @fn	    	DEV_PARAM_GetParam
 * @brief		获取参数
 * @param		type:参数类型      pDataDst：参数值输出存放指针
 * @return 	0：ok -1：fail
 ******************************************************/
int DEV_PARAM_GetParam(APP_PARAM_TYPE_E type, void *pDataDst) {
  int ret = 0;

  if (pAppParam == NULL) {
    printf("pAppParam is NULL!\n");
    return -1;
  }
  if (!pAppParam->isParamReady) {
    printf("Param is not ready!\n");
    return -1;
  }
  switch (type) {
    case PARAM_TYPE_CONTRAST:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.contrast;
      break;
    case PARAM_TYPE_BRIGHTNESS:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.brightness;
      break;
    case PARAM_TYPE_SHARPNESS:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.sharpness;
      break;
    case PARAM_TYPE_SATURATION:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.saturation;
      break;
    case PARAM_TYPE_MD:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.md;
      break;
    case PARAM_TYPE_MD_SENSE:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.mdSense;
      break;
    case PARAM_TYPE_HUMAN_FILTER:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.humanFilter;
      break;
    case PARAM_TYPE_RESOLUTION:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.resolution;
      break;
    case PARAM_TYPE_INDICATOR:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.indicatorSwitch;
      break;
    case PARAM_TYPE_NIGHT_VISION:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.nightVision;
      break;
    case PARAM_TYPE_VOLUME:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.volume;
      break;
    case PARAM_TYPE_FLIP:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.flip;
      break;
    case PARAM_TYPE_PRIVATE_MODE:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.privateMode;
      break;
    case PARAM_TYPE_TIME_WATERMARK:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.timeWatermark;
      break;
    case PARAM_TYPE_TALK_MODE:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.talkMode;
      break;
    case PARAM_TYPE_TIMEZONE:
      memcpy((char *)pDataDst, pAppParam->appSetInfo.data.timezone, strlen(pAppParam->appSetInfo.data.timezone));
      break;
    case PARAM_TYPE_TIMEZONEPOSIX:
      memcpy((char *)pDataDst, pAppParam->appSetInfo.data.timeZonePosix,
             strlen(pAppParam->appSetInfo.data.timeZonePosix));
      break;
    case PARAM_TYPE_AUTO_TRACKING:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.autoTracking;
      break;
    case PARAM_TYPE_EN_ALGOLOG:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.enAlgoLog;
      break;
    case PARAM_TYPE_EN_DETAILLOG:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.enUploadDetailLog;
      break;
    case PARAM_TYPE_EN_KEYLOG:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.enUploadKeyLog;
      break;
    case PARAM_TYPE_SET_LOGLEVEL:
      memcpy((char *)pDataDst, pAppParam->appSetInfo.data.logLevel, strlen(pAppParam->appSetInfo.data.logLevel));
      break;
    case PARAM_TYPE_AUTO_UPGRADE:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.autoUpgrade;
      break;
    case PARAM_TYPE_TOKEN:
      memcpy((char *)pDataDst, pAppParam->appSetInfo.data.token, strlen(pAppParam->appSetInfo.data.token));
      break;
    case PARAM_TYPE_STREAM_NAME:
      memcpy((char *)pDataDst, pAppParam->appSetInfo.data.streamName, strlen(pAppParam->appSetInfo.data.streamName));
      break;
    case PARAM_TYPE_REGION:
      memcpy((char *)pDataDst, pAppParam->appSetInfo.data.region, strlen(pAppParam->appSetInfo.data.region));
      break;
    case PARAM_TYPE_UPGRADE_FLAG:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.upgradeFlag;
      break;
    case PARAM_TYPE_SOUND_ALARM:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.soundAlarm;
      break;
    case PARAM_TYPE_ANTI_FLICKER:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.antiFlicker;
      break;
    case PARAM_TYPE_DET_ALARM:
      memcpy((DETECT_ALARM_PARAM_S *)pDataDst, &(pAppParam->appSetInfo.data.detAlarm), sizeof(DETECT_ALARM_PARAM_S));
      break;
    case PARAM_TYPE_CRUISE:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.cruise;
      break;
    case PARAM_TYPE_PREPOINT:
      memcpy((DEV_POINT_S **)pDataDst, &(pAppParam->appSetInfo.data.prePoint), sizeof(DEV_POINT_S));
      break;
    case PARAM_TYPE_CLOUD_END_TIME:
      *((long long *)pDataDst) = pAppParam->appSetInfo.data.cloudEndTime;
      break;
    case PARAM_TYPE_CLOUD_STAR_TIME:
      *((long long *)pDataDst) = pAppParam->appSetInfo.data.cloudStarTime;
      break;
    case PARAM_TYPE_IOT_URL:
      *(char **)pDataDst = pAppParam->appSetInfo.data.aws_conf.iot_url;
      printf("iot_url: %s", pAppParam->appSetInfo.data.aws_conf.iot_url);
      break;
    case PARAM_TYPE_ENDPOINT:
      *(char **)pDataDst = pAppParam->appSetInfo.data.aws_conf.endpoint;
      printf("endpoint:%s", pAppParam->appSetInfo.data.aws_conf.endpoint);
      break;
    case PARAM_TYPE_ROLEALIAS:
      *(char **)pDataDst = pAppParam->appSetInfo.data.aws_conf.role_alias;
      printf("role_alias:%s", pAppParam->appSetInfo.data.aws_conf.role_alias);
      break;
    case PARAM_TYPE_HW_VERSION:
      *(int *)pDataDst = pAppParam->devBaseInfo.hwVersion;
      break;
    case PARAM_TYPE_DARK_FULL_COLOR:
      *((unsigned char *)pDataDst) = pAppParam->appSetInfo.data.darkFullColor;
      break;
    case PARAM_TYPE_EVENT_SCHEDULE:
      memcpy((EVENT_SCHEDULE_S *)pDataDst, &(pAppParam->appSetInfo.data.eventsch_conf), sizeof(EVENT_SCHEDULE_S));
      break;
    case PARAM_TYPE_RECORD_INFO:
      memcpy((REC_CONF_S *)pDataDst, &(pAppParam->appSetInfo.data.record_conf), sizeof(REC_CONF_S));
      break;
    default:
      printf("invalid  parameter type:%d!\n", type);
      ret = -1;
      break;
  }
  return ret;
}

/*****************************************************
 * @fn	    	DEV_PARAM_SetParam
 * @brief		设置参数
 * @param		type:参数类型      pValue: 参数值输入存放指针
 * @return 	0：ok -1：fail
 ******************************************************/
int DEV_PARAM_SetParam(APP_PARAM_TYPE_E type, void *pValue) {
  int ret = 0, valTmp = 0;
  long long tmpPtsMs = 0;
  bool sync_file = true;

  if (pAppParam == NULL) {
    printf("pAppParam is NULL!\n");
    return -1;
  }
  if (!pAppParam->isParamReady) {
    printf("Param is not ready!\n");
    return -1;
  }
  pthread_mutex_lock(&pAppParam->lock);
  switch (type) {
    case PARAM_TYPE_CONTRAST:
      pAppParam->appSetInfo.data.contrast = *((unsigned char *)pValue);
      break;
    case PARAM_TYPE_BRIGHTNESS:
      pAppParam->appSetInfo.data.brightness = *((unsigned char *)pValue);
      break;
    case PARAM_TYPE_SHARPNESS:
      pAppParam->appSetInfo.data.sharpness = *((unsigned char *)pValue);
      break;
    case PARAM_TYPE_SATURATION:
      pAppParam->appSetInfo.data.saturation = *((unsigned char *)pValue);
      break;
    case PARAM_TYPE_MD:
      pAppParam->appSetInfo.data.md = *((unsigned char *)pValue);
      DEV_PARAM_MakeConfEffect(PARAM_TYPE_MD);
      break;
    case PARAM_TYPE_MD_SENSE:
      pAppParam->appSetInfo.data.mdSense = *((unsigned char *)pValue);
      DEV_PARAM_MakeConfEffect(PARAM_TYPE_MD_SENSE);
      break;
    case PARAM_TYPE_HUMAN_FILTER:
      pAppParam->appSetInfo.data.humanFilter = *((unsigned char *)pValue);
      DEV_PARAM_MakeConfEffect(PARAM_TYPE_HUMAN_FILTER);
      break;
    case PARAM_TYPE_RESOLUTION:
      pResolutionInfo = (DEV_RESOLUTION_INFO_S *)pValue;
      pAppParam->appSetInfo.data.resolution = pResolutionInfo->resolution;
      DEV_PARAM_MakeConfEffect(PARAM_TYPE_RESOLUTION);
      sync_file = false;
      break;
    case PARAM_TYPE_INDICATOR:
      pAppParam->appSetInfo.data.indicatorSwitch = *((unsigned char *)pValue);
      DEV_PARAM_MakeConfEffect(PARAM_TYPE_INDICATOR);
      break;
    case PARAM_TYPE_NIGHT_VISION:
      pAppParam->appSetInfo.data.nightVision = *((unsigned char *)pValue);
      break;
    case PARAM_TYPE_VOLUME:
      pAppParam->appSetInfo.data.volume = *((unsigned char *)pValue);
      DEV_PARAM_MakeConfEffect(PARAM_TYPE_VOLUME);
      break;
    case PARAM_TYPE_FLIP:
      pAppParam->appSetInfo.data.flip = *((unsigned char *)pValue);
      DEV_PARAM_MakeConfEffect(PARAM_TYPE_FLIP);
      break;
    case PARAM_TYPE_PRIVATE_MODE:
      pAppParam->appSetInfo.data.privateMode = *((unsigned char *)pValue);
      DEV_PARAM_MakeConfEffect(PARAM_TYPE_PRIVATE_MODE);
      // MQTT_MESSAGE_SendDevInfo();
      break;
    case PARAM_TYPE_TIME_WATERMARK:
      pAppParam->appSetInfo.data.timeWatermark = *((unsigned char *)pValue);
      DEV_PARAM_MakeConfEffect(PARAM_TYPE_TIME_WATERMARK);
      break;
    case PARAM_TYPE_TALK_MODE:
      pAppParam->appSetInfo.data.talkMode = *((unsigned char *)pValue);
      break;
    case PARAM_TYPE_TIMEZONE:
      memset(pAppParam->appSetInfo.data.timezone, 0, sizeof(pAppParam->appSetInfo.data.timezone));
      memcpy(pAppParam->appSetInfo.data.timezone, (char *)pValue, strlen(pValue));
      break;
    case PARAM_TYPE_TIMEZONEPOSIX:
      memset(pAppParam->appSetInfo.data.timeZonePosix, 0, sizeof(pAppParam->appSetInfo.data.timeZonePosix));
      memcpy(pAppParam->appSetInfo.data.timeZonePosix, (char *)pValue, strlen(pValue));
      break;
    case PARAM_TYPE_SET_LOGLEVEL:
      memset(pAppParam->appSetInfo.data.logLevel, 0, sizeof(pAppParam->appSetInfo.data.logLevel));
      memcpy(pAppParam->appSetInfo.data.logLevel, (char *)pValue, strlen(pValue));
      break;
    case PARAM_TYPE_EN_ALGOLOG:
      pAppParam->appSetInfo.data.enAlgoLog = *((unsigned char *)pValue);
      break;
    case PARAM_TYPE_EN_DETAILLOG:
      pAppParam->appSetInfo.data.enUploadDetailLog = *((unsigned char *)pValue);
      break;
    case PARAM_TYPE_EN_KEYLOG:
      pAppParam->appSetInfo.data.enUploadKeyLog = *((unsigned char *)pValue);
      break;
    case PARAM_TYPE_AUTO_TRACKING:
      pAppParam->appSetInfo.data.autoTracking = *((unsigned char *)pValue);
      DEV_PARAM_MakeConfEffect(PARAM_TYPE_AUTO_TRACKING);
      break;
    case PARAM_TYPE_AUTO_UPGRADE:
      pAppParam->appSetInfo.data.autoUpgrade = *((unsigned char *)pValue);
      break;
    case PARAM_TYPE_TOKEN:
      memset(pAppParam->appSetInfo.data.token, 0, sizeof(pAppParam->appSetInfo.data.token));
      memcpy(pAppParam->appSetInfo.data.token, (char *)pValue, strlen(pValue));
      break;
    case PARAM_TYPE_STREAM_NAME:
      memset(pAppParam->appSetInfo.data.streamName, 0, sizeof(pAppParam->appSetInfo.data.streamName));
      memcpy(pAppParam->appSetInfo.data.streamName, (char *)pValue, strlen(pValue));
      break;
    case PARAM_TYPE_REGION:
      memset(pAppParam->appSetInfo.data.region, 0, sizeof(pAppParam->appSetInfo.data.region));
      memcpy(pAppParam->appSetInfo.data.region, (char *)pValue, strlen(pValue));
      break;
    case PARAM_TYPE_UPGRADE_FLAG:
      pAppParam->appSetInfo.data.upgradeFlag = *((unsigned char *)pValue);
      break;
    case PARAM_TYPE_SOUND_ALARM:
      pAppParam->appSetInfo.data.soundAlarm = *((unsigned char *)pValue);
      break;
    case PARAM_TYPE_ANTI_FLICKER:
      pAppParam->appSetInfo.data.antiFlicker = *((unsigned char *)pValue);
      DEV_PARAM_MakeConfEffect(PARAM_TYPE_ANTI_FLICKER);
      break;
    case PARAM_TYPE_DET_ALARM:
      valTmp = pAppParam->appSetInfo.data.detAlarm.tone;
      memset(&(pAppParam->appSetInfo.data.detAlarm), 0, sizeof(DETECT_ALARM_PARAM_S));
      memcpy(&(pAppParam->appSetInfo.data.detAlarm), (DETECT_ALARM_PARAM_S *)pValue, sizeof(DETECT_ALARM_PARAM_S));
      if ((valTmp != pAppParam->appSetInfo.data.detAlarm.tone) && pAppParam->appSetInfo.data.detAlarm.detSwitch) {
        DEV_PARAM_MakeConfEffect(PARAM_TYPE_DET_ALARM);
      }
      break;
    case PARAM_TYPE_CRUISE:
      pAppParam->appSetInfo.data.cruise = *((unsigned char *)pValue);
      DEV_PARAM_MakeConfEffect(PARAM_TYPE_CRUISE);
      break;
    case PARAM_TYPE_PREPOINT:
      memset(&(pAppParam->appSetInfo.data.prePoint), 0, sizeof(DEV_POINT_S));
      memcpy(&(pAppParam->appSetInfo.data.prePoint), (DEV_POINT_S *)pValue, sizeof(DEV_POINT_S));
      pAppParam->lastMotorPosUpdateTime = time(NULL);
      sync_file = false;
      break;
    case PARAM_TYPE_CLOUD_END_TIME:
      tmpPtsMs = *((long long *)pValue);
      if (pAppParam->appSetInfo.data.cloudEndTime != tmpPtsMs) {
        pAppParam->appSetInfo.data.cloudEndTime = tmpPtsMs;
        DEV_PARAM_MakeConfEffect(PARAM_TYPE_CLOUD_END_TIME);
      } else {
        sync_file = false;
      }
      break;
    case PARAM_TYPE_CLOUD_STAR_TIME:
      tmpPtsMs = *((long long *)pValue);
      if (pAppParam->appSetInfo.data.cloudStarTime != tmpPtsMs) {
        pAppParam->appSetInfo.data.cloudStarTime = tmpPtsMs;
        DEV_PARAM_MakeConfEffect(PARAM_TYPE_CLOUD_STAR_TIME);
      } else {
        sync_file = false;
      }
      break;
    case PARAM_TYPE_IOT_URL:
      setIotUrl((char *)pValue);
      break;
    case PARAM_TYPE_ENDPOINT:
      strncpy(pAppParam->appSetInfo.data.aws_conf.endpoint, pValue, ENDPOINT_SIZE);
      printf("endpoint:%s\n", pAppParam->appSetInfo.data.aws_conf.endpoint);
      break;
    case PARAM_TYPE_ROLEALIAS:
      strncpy(pAppParam->appSetInfo.data.aws_conf.role_alias, pValue, ROLE_ALIAS_SIZE);
      printf("roleAlias:%s\n", pAppParam->appSetInfo.data.aws_conf.role_alias);
      break;
    case PARAM_TYPE_HW_VERSION:
      pAppParam->devBaseInfo.hwVersion = *(unsigned char *)pValue;
      break;
    case PARAM_TYPE_DARK_FULL_COLOR:
      if (*((unsigned char *)pValue) == 0) {
        pAppParam->appSetInfo.data.darkFullColor = 0;
        printf("darkFullColor is off\n");
      } else {
        pAppParam->appSetInfo.data.darkFullColor = 1;
        printf("darkFullColor is on\n");
      }
      break;
    case PARAM_TYPE_EVENT_SCHEDULE:
      memset(&(pAppParam->appSetInfo.data.eventsch_conf), 0, sizeof(EVENT_SCHEDULE_S));
      memcpy(&(pAppParam->appSetInfo.data.eventsch_conf), (EVENT_SCHEDULE_S *)pValue, sizeof(EVENT_SCHEDULE_S));
      break;
    case PARAM_TYPE_RECORD_INFO:
      memset(&(pAppParam->appSetInfo.data.record_conf), 0, sizeof(REC_CONF_S));
      memcpy(&(pAppParam->appSetInfo.data.record_conf), (REC_CONF_S *)pValue, sizeof(REC_CONF_S));
      DEV_PARAM_MakeConfEffect(PARAM_TYPE_RECORD_INFO);
      break;
    default:
      printf("invalid  parameter type!");
      ret = -1;
      break;
  }
  if (ret == 0 && sync_file) {
    if (syncParamToFile() != 0) {
      printf("sync parameter to file fail!\n");
      ret = -1;
    }
  }
  pthread_mutex_unlock(&pAppParam->lock);
  return ret;
}

/*****************************************************
 * @fn	    	DEV_PARAM_MakeConfEffect
 * @brief		使参数生效
 * @param		type:参数类型
 * @return 	0：ok -1：fail
 ******************************************************/
int DEV_PARAM_MakeConfEffect(APP_PARAM_TYPE_E type) {
  int ret = 0;

#if 0
  if (pAppParam) {
    switch (type) {
      case PARAM_TYPE_VOLUME:
        MEDIA_AUDIO_SetTalkVolume(pAppParam->appSetInfo.data.volume);
        break;
      case PARAM_TYPE_FLIP:
        MEDIAO_VIDEO_SetFlip(pAppParam->appSetInfo.data.flip);
        MOTOR_SetFlip(pAppParam->appSetInfo.data.flip);
        break;
      case PARAM_TYPE_PRIVATE_MODE:
        MEDIAO_VIDEO_SetPrivateMode((int)pAppParam->appSetInfo.data.privateMode);
        MEDIA_AUDIO_SetPrivateMode((int)pAppParam->appSetInfo.data.privateMode);
        MOTOR_SetMode((int)pAppParam->appSetInfo.data.privateMode);

        if (pAppParam->appSetInfo.data.privateMode == 1)
          LED_SetLedSwitch(false);
        else
          LED_SetLedSwitch(true);

        break;
      case PARAM_TYPE_TIME_WATERMARK:
        MEDIAO_VIDEO_OsdShowCtrl((pAppParam->appSetInfo.data.timeWatermark == 0) ? false : true);
        break;
      case PARAM_TYPE_HUMAN_FILTER:
        MEDIAO_VIDEO_SetPersonRectShow((pAppParam->appSetInfo.data.humanFilter == 0) ? false : true);
        break;
      case PARAM_TYPE_AUTO_TRACKING:
        MOTOR_SetAutotracking((int)pAppParam->appSetInfo.data.autoTracking);
        break;
      case PARAM_TYPE_INDICATOR:
        LED_SetLedSwitch((bool)pAppParam->appSetInfo.data.indicatorSwitch);
        break;
      case PARAM_TYPE_ANTI_FLICKER:
        MEDIAO_VIDEO_SetAntiFlickerAttr(pAppParam->appSetInfo.data.antiFlicker);
        break;
      case PARAM_TYPE_CRUISE:
        MOTOR_SetCruiseSwitch(pAppParam->appSetInfo.data.cruise);
        break;
      case PARAM_TYPE_MD:
        MEDIAO_VIDEO_SetMDEnable(pAppParam->appSetInfo.data.md);
        break;
      case PARAM_TYPE_MD_SENSE:
        MEDIAO_VIDEO_SetSensLevel(pAppParam->appSetInfo.data.mdSense);
        break;
      case PARAM_TYPE_DET_ALARM:
        if (pAppParam->appSetInfo.data.detAlarm.tone == 1) {
          MEDIA_AUDIO_PlayFileWithTime(SIREN1_FILE, pAppParam->appSetInfo.data.detAlarm.volume, 3);
        } else if (pAppParam->appSetInfo.data.detAlarm.tone == 2) {
          MEDIA_AUDIO_PlayFileWithTime(SIREN2_FILE, pAppParam->appSetInfo.data.detAlarm.volume, 3);
        } else {
          MEDIA_AUDIO_PlayFileWithTime(SIREN3_FILE, pAppParam->appSetInfo.data.detAlarm.volume, 3);
        }
        break;
      case PARAM_TYPE_CLOUD_END_TIME:
        KVS_SetCloudEndTime(pAppParam->appSetInfo.data.cloudEndTime);
        break;
      case PARAM_TYPE_CLOUD_STAR_TIME:
        // KVS_SetCloudEndTime(pAppParam->appSetInfo.data.cloudStarTime);
        break;
      case PARAM_TYPE_RESOLUTION:
        if (pResolutionInfo != NULL) {
          AWS_WEBRTC_SetResolution(pAppParam->appSetInfo.data.resolution,
                                   (strlen(pResolutionInfo->id) == 0) ? NULL : pResolutionInfo->id);
          pResolutionInfo = NULL;
        }
        break;
      case PARAM_TYPE_RECORD_INFO:
        RECORD_SetRecordSwitch(pAppParam->appSetInfo.data.record_conf.enrecord);
        RECORD_SetMode((RECORD_MODE_E)pAppParam->appSetInfo.data.record_conf.recordType);
        RECORD_SetMute(pAppParam->appSetInfo.data.record_conf.muteRecord);
        break;
      

      default:
        printf("invalid  parameter type!\n");
        ret = -1;
        break;
    }
  }
#endif

  return ret;
}

int DEV_PARAM_Init(void) {
  pAppParam = (DEV_INFO_S *)calloc(sizeof(DEV_INFO_S) + 4, 1);
  if (pAppParam == NULL) {
    printf("calloc failed!\n");
    return -1;
  }

  // MEDIA_CAM_CODEC_params_Init();
  DEV_PARAM_CheckDevInfoPath();
  syncParamFromFile();
  // DEV_PARAM_MakeConfEffect(PARAM_TYPE_AUTO_TRACKING);
  // DEV_PARAM_MakeConfEffect(PARAM_TYPE_PRIVATE_MODE);
  // DEV_PARAM_MakeConfEffect(PARAM_TYPE_CRUISE);
  pthread_mutex_init(&pAppParam->lock, NULL);
  pAppParam->isParamReady = true;

  return 0;
}

int DEV_PARAM_SaveToFile(void) {
  if (pAppParam == NULL) {
    printf("pAppParam is NULL!\n");
    return -1;
  }
  if (syncParamToFile() == 0) {
    printf("save param to file done\n");
    return 0;
  } else {
    printf("save param to file failed\n");
    return -1;
  }
}

int DEV_PARAM_SetFactoryMode(void) {
  int ret = 0;
#if 0
  if (pAppParam == NULL) {
    printf("pAppParam is NULL\n");
    return -1;
  }
  printf("Set factory mode starting...\n");
  LOG_KeyInfoUp("Set factory mode, unbind device, ipc will reboot ...");

  MEDIA_AUDIO_SetFactory(1);

  syncParamToFile();
  pAppParam->stopFlag = 1;
  usleep(1000 * 50);

  COMM_FileUnlink(CONFIG_FILE_REGION_PATH);

  //删除配置文件及绑定标记
  COMM_FileUnlink(CONFIG_FILE_PATH);

  COMM_FileUnlink(CONFIG_FILE_BACKUP_PATH);

  COMM_FileUnlink(BIND_STATUS_FILE);

  //删除证书
  COMM_FileUnlink(CERT_MESSAGE_FILE);

  COMM_FileUnlink(CERTIFICATE_FILE);

  COMM_FileUnlink(PRIVATE_KEY_FILE);

  //删除音频参数
  COMM_FileUnlink(CONFIG_AUDIO_CODEC_FILE_PATH);

  //清除WIF配置
  Network_Manager_ClearConfig();

  LED_SetLedStatus(LED_STATUS_WAITING_STARTUP);

  sleep(1);
  MEDIA_AUDIO_PlayCommFile(MUSIC2_FILE);
  sleep(2);
  printf("+++++++++++++++++ Set factory reboot ++++++++++++++++++");
  COMM_API_RebootSystem((char *)__FUNCTION__, __LINE__);
#endif
  return ret;
}

bool DEV_PARAM_CheckPrivateMode(void) {
  return false;
  if (pAppParam == NULL) {
    printf("pAppParam is NULL\n");
    return false;
  }

  if (pAppParam->appSetInfo.data.privateMode) {
    return true;
  }

  return false;
}

/*****************************************************
 * @fn        DEV_PARAM_get_sub_mac
 * @brief     获取所有子设备MAC地址
 * @param     m_sub_mac: 指向sub_mac_conf*数组指针的指针，调用者负责释放
 * @param     count: 输出参数，返回mac数量
 * @return    0：ok -1：fail
 *
 * {
 *   "sub_mac": [
 *     {
 *       "ble_mac": "1222222222222",
 *       "wifi_mac": "345555555555"
 *     },
 *     {
 *       "ble_mac": "3434344444",
 *       "wifi_mac": "565656666666"
 *     }
 *   ]
 * }
 ******************************************************/
int DEV_PARAM_get_sub_mac(sub_mac_conf **m_sub_mac, int *count) {
  if (m_sub_mac == NULL || count == NULL) {
    printf("invalid param\n");
    return -1;
  }
  *m_sub_mac = NULL;
  *count = 0;

  if (access("/parameter/conf/sub_mac", F_OK) != 0) {
    printf("sub_mac file not exist\n");
    return -1;
  }

  cJSON *root = CommToolbox_parse_cjsonObject_from_file("/parameter/conf/sub_mac");
  if (root == NULL) {
    printf("parse sub_mac file failed\n");
    return -1;
  }

  cJSON *sub_mac_array = cJSON_GetObjectItem(root, "sub_mac");
  if (sub_mac_array == NULL || !cJSON_IsArray(sub_mac_array)) {
    printf("sub_mac is not an array\n");
    cJSON_Delete(root);
    return -1;
  }

  int array_size = cJSON_GetArraySize(sub_mac_array);
  if (array_size <= 0) {
    cJSON_Delete(root);
    return 0;
  }

  sub_mac_conf *mac_list = (sub_mac_conf *)calloc(array_size, sizeof(sub_mac_conf));
  if (!mac_list) {
    printf("malloc sub_mac_conf array failed\n");
    cJSON_Delete(root);
    return -1;
  }

  int idx = 0;
  cJSON *item = NULL;
  cJSON_ArrayForEach(item, sub_mac_array) {
    if (cJSON_IsObject(item)) {
      cJSON *ble_mac = cJSON_GetObjectItem(item, "ble_mac");
      cJSON *wifi_mac = cJSON_GetObjectItem(item, "wifi_mac");
      if (ble_mac && cJSON_IsString(ble_mac))
        strncpy(mac_list[idx].bleMac, ble_mac->valuestring, sizeof(mac_list[idx].bleMac) - 1);
      if (wifi_mac && cJSON_IsString(wifi_mac))
        strncpy(mac_list[idx].wifiMac, wifi_mac->valuestring, sizeof(mac_list[idx].wifiMac) - 1);
      idx++;
      if (idx >= array_size) break;
    }
  }

  *m_sub_mac = mac_list;
  *count = array_size;
  cJSON_Delete(root);

  // 打印所有mac
  for (int i = 0; i < *count; ++i) {
    printf("sub_mac[%d] ble_mac: %s, wifi_mac: %s\n", i, mac_list[i].bleMac, mac_list[i].wifiMac);
  }

  return 0;
}

int DEV_PARAM_get_sub_ble_mac(const char *wifiMac, char *bleMacOut, size_t bleMacOutSize) {
  if (!wifiMac || !bleMacOut || bleMacOutSize == 0 || strlen(wifiMac) == 0) {
    printf("invalid mac input\n");
    return -1;
  }

  sub_mac_conf *mac_list = NULL;
  int count = 0;
  if (DEV_PARAM_get_sub_mac(&mac_list, &count) != 0) {
    printf("get sub_mac failed\n");
    return -1;
  }

  int found = 0;
  for (int i = 0; i < count; i++) {
    if (strncmp(mac_list[i].wifiMac, wifiMac, sizeof(mac_list[i].wifiMac)) == 0) {
      strncpy(bleMacOut, mac_list[i].bleMac, bleMacOutSize - 1);
      bleMacOut[bleMacOutSize - 1] = '\0';
      found = 1;
      break;
    }
  }

  free(mac_list);

  if (!found) {
    printf("sub device with wifi mac %s not found\n", wifiMac);
    return -1;
  }
  return 0;
}

int DEV_PARAM_update_sub_mac(char *m_subBleMac, char *m_subWifiMac) {
  if (!m_subBleMac || !m_subWifiMac || strlen(m_subBleMac) == 0 || strlen(m_subWifiMac) == 0) {
    printf("invalid mac input");
    return -1;
  }

  const char *file_path = "/parameter/conf/sub_mac";
  cJSON *root = NULL;
  cJSON *macArray = NULL;
  int found = 0;

  if (access(file_path, F_OK) == 0) {
    // 文件存在，调用DEV_PARAM_get_sub_mac，读取所有mac地址，再进行匹配
    sub_mac_conf *mac_list = NULL;
    int count = 0;
    if (DEV_PARAM_get_sub_mac(&mac_list, &count) != 0) {
      printf("get sub_mac failed\n");
      return -1;
    }
    for (int i = 0; i < count; i++) {
      // 检查是否已经存在相同的mac地址
      if (strcmp(mac_list[i].bleMac, m_subBleMac) == 0 && strcmp(mac_list[i].wifiMac, m_subWifiMac) == 0) {
        found = 1;
      }
    }
    free(mac_list);

    if (!found) {
      root = CommToolbox_parse_cjsonObject_from_file("/parameter/conf/sub_mac");
      if (root == NULL) {
        printf("parse sub_mac file failed\n");
        return -1;
      }

      macArray = cJSON_GetObjectItem(root, "sub_mac");
      if (macArray == NULL || !cJSON_IsArray(macArray)) {
        printf("sub_mac is not an array\n");
        cJSON_Delete(root);
        return -1;
      }

      // 添加新的mac地址
      cJSON *newMac = cJSON_CreateObject();
      cJSON_AddStringToObject(newMac, "ble_mac", m_subBleMac);
      cJSON_AddStringToObject(newMac, "wifi_mac", m_subWifiMac);
      cJSON_AddItemToArray(macArray, newMac);

      char *jsonString = cJSON_Print(root);
      if (jsonString) {
        if (COMM_NewFile(file_path, jsonString) != 0) {
          printf("write sub_mac file failed\n");
          free(jsonString);
          cJSON_Delete(root);
          return -1;
        }
        free(jsonString);
      } else {
        printf("cJSON_Print failed\n");
        cJSON_Delete(root);
        return -1;
      }
      cJSON_Delete(root);
      printf("Added new sub_mac: ble_mac: %s, wifi_mac: %s\n", m_subBleMac, m_subWifiMac);
    }
  } else {
    // 文件不存在，创建新文件
    root = cJSON_CreateObject();
    macArray = cJSON_CreateArray();
    cJSON *newMac = cJSON_CreateObject();
    cJSON_AddStringToObject(newMac, "ble_mac", m_subBleMac);
    cJSON_AddStringToObject(newMac, "wifi_mac", m_subWifiMac);
    cJSON_AddItemToArray(macArray, newMac);
    cJSON_AddItemToObject(root, "sub_mac", macArray);

    char *jsonString = cJSON_Print(root);
    if (jsonString) {
      COMM_NewFile(file_path, jsonString);
      free(jsonString);
    }
    cJSON_Delete(root);
  }

  return 0;
}
