#ifndef __COMM_API_H__
#define __COMM_API_H__

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef ABS
#define ABS(a) ((a) > (0) ? (a) : (-a))
#endif

#define PARAMTER_ROOT_DIR "/parameter"
// #define PARAMTER_ROOT_DIR "/root/share/05_tools/06-res/parameter"
// #define PARAMTER_ROOT_DIR   "/usr/data/ad100/res"

#define FW_VERSION_DIR PARAMTER_ROOT_DIR "/vendor/app"
#define FW_VERSION_FILE_PATH PARAMTER_ROOT_DIR "/vendor/app/version.info"
#define FW_VERSION_FLAG "FWVersion="
#define HW_VERSION_FILE_PATH ""
#define HW_VERSION_FLAG ""
#define CURL_TOOL "/usr/bin/curl"
#define QUIET_RESTART_TAG "/parameter/quiet_restart.tag"

typedef enum COMM_API_SYSTIME {
  SYSTIME_M_SECSYSTIME_SEC = 0,
  SYSTIME_SEC,
  SYSTIME_MICRO_SEC,
  SYSTIME_MAX
} COMM_API_SYSTIME_E;

typedef struct {
  size_t VmRSS;
  size_t VmStk;
  size_t VmExe;
  size_t VMLib;
  size_t RssAnon;
  size_t RssFile;
  size_t RssShmem;
  size_t ThreadsNum;
} system_memory_size_t;

long long COMM_API_GetSysTime(COMM_API_SYSTIME_E type);

long long COMM_API_GetUtcTimeMs();
int COMM_API_SystemCommand(char *cmd);
int COMM_API_SystemCommandEx(const char *cmd);
int COMM_API_PopenCmdVerify(const char *cmd, const char *comparestr, int timeout);

long COMM_API_GetFileSize(const char *FileName);

void COMM_API_GetUtcTime(char *utcTime);

int COMM_API_ClearCache(void);

int COMM_API_CheckFileExist(const char *FileName, int mode);
int COMM_API_CheckStr(char *strBuf, int strBufLen, char *checkBuf, int checkBufLen, char *resultBuf, int resultBufLen);
int COMM_API_GetFWVersion(char *pFWVersion);
int COMM_API_GetHWVersion(char *pHWVersion);
int COMM_API_RebootSystem(const char *func, int line);
int COMM_API_GetFWName(char *pFWName);
int COMM_API_GetTimeZoneSec(void);
int COMM_API_TelnetCtrl(int value);
pid_t COMM_API_CheckProcExist(const char *procName);
long COMM_API_GetSystemRunTime(void);
int COMM_GetRandomVal(unsigned short range);
int execute_command(char *command);
int COMM_API_SystemVforkCmd(const char *cmd);
int COMM_Mkdirs(char *dir);
int COMM_CheckTimeIsUpdate(void);
int COMM_GetTimeOfDay(void);
int COMM_FileUnlink(char *filePath);
int COMM_deleteDirectory(char *dirPath);
int COMM_CheckDirFileNum(char *mdir);
int COMM_NewFile(const char *filePath, const char *text);
int COMM_CheckPid(char *pid, int *pidNum);
int COMM_Free(char *func, int line);
int COMM_API_TimedRestart(void);

int COMM_API_GetAllThreadNum(void);
char **COMM_API_GetAllThreadNames(int *thr_num);
void COMM_API_freeGetThreadNames(char **arr, int rows);

int COMM_API_PrintVersionInfo(void);

int32_t COMM_API_open(const char *path, int flags, ...);
int COMM_API_close(int32_t fp);
int32_t COMM_API_write(int32_t fp, unsigned char *buf, int32_t size);
int32_t COMM_API_read(int32_t fp, unsigned char *buf, int32_t size);
void COMM_GetTimeStr(char *time_str);
int COMM_API_GetFWVersion(char *pFWVersion);
uint16_t COMM_API_CalculateCRC16(const uint8_t *data, uint32_t length);
uint8_t COMM_API_Calculate_xor_checksum(const unsigned char *data, size_t length);
int COMM_API_MkdirRecursive(const char *path, mode_t mode);
int COMM_API_ReadFile(char *filename, char **ppContent, int *pSize);
long long COMM_API_get_tick_ms();
int COMM_API_ReadFileContent(char *fileName, char **ppContent, int *pLen);
void COMM_API_PrintFile(const char *filepath);
long long COMM_API_get_tick_us();
uint32_t COMM_API_GenerateConv(uint8_t cam_seq, uint8_t stream_seq);
int COMM_API_MoveFile(const char *src, const char *dest);

/* log api start */
int COMM_API_log_local_copy(const char *srcLog, const char *destLog);
int COMM_API_log_local_save(char *baseDir, char *devId, const char *logpath, const char *logName);
const char *COMM_API_get_basename(const char *path);
int COMM_API_logFileOpen(int *m_LogSaveFd, char *saveDir, char *mpid, char *SaveLogName, int logNameLen, int m_time);
int COMM_API_logFileWrite(int m_LogSaveFd, char *strlog, int logSize);
int COMM_API_logFileClose(int m_LogSaveFd);
void COMM_API_DeleteFilesInDirectory(const char *dir_path);
int COMM_API_CheckProgramExist(const char *keyword);
int COMM_API_SyncTimnZone();
char *COMM_API_PayloadToHexString(const unsigned char *payload, size_t payloadSize);

int COMM_API_check_log_num(char *logdir, char *pid, int maxNum);
void COMM_API_get_readable_rate_str(uint32_t rate, char *str, int size);
void COMM_API_unescape(const char *src, unsigned char *dest);
int COMM_API_is_utf8_hex_string(const char *str);
void COMM_API_unescape(const char *src, unsigned char *dest);
void COMM_API_delete_jpg_files_in_tmp();
bool CONNECT_GetBindStatus(void);
int COMM_API_WriteFileBinary(const char *path, const unsigned char *data, size_t len);
/* log api end */

// 静默重启
int COMM_API_IsQuietRestartTag(void);
void COMM_API_SetQuietRestartTag(void);
void COMM_API_CleanQuietRestartTag(void);
int COMM_API_get_file_lines(const char *file_path);

bool COMM_API_same_subnet(const char *ip1, const char *ip2, const char *interface_name);
int COMM_API_get_subnet_mask(const char *interface_name, char *subnet_mask, size_t size);

// 系统资源接口
int COMM_API_get_fd_count();
size_t COMM_API_get_free_hp(void);
size_t COMM_API_get_buff_cache_size();
int COMM_API_get_memory_sizes(system_memory_size_t *memSizes);

int COMM_API_IsExtender();
void COMM_API_ResetExtenderStatus();
int COMM_API_isIFrame(int isH265, unsigned char *data);

#endif
