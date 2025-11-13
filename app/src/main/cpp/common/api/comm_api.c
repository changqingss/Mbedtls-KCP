#include "comm_api.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <net/if.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "common/Common_toolbox.h"
#include "common/log/log_conf.h"
#include "common/param_config/dev_param.h"

#define MAX_PATH_LENGTH 1024
#define SYS_MIN_UTC_SEC (1640966400)  // 2022/1/1 00:00:00

void COMM_GetTimeStr(char *time_str) {
  if (NULL == time_str) {
    return;
  }

  char buf[64] = "";
  struct timeval tv;

  memset(buf, 0, sizeof(buf));
  gettimeofday(&tv, NULL);
  struct tm tm_info;
  localtime_r(&tv.tv_sec, &tm_info);
  strftime(buf, sizeof(buf) - 1, "%Y-%m-%d %H:%M:%S", &tm_info);
#if USE_LOG_BOOT_TICK_TIMEMS
  sprintf(time_str, "%s.%03d [%lld ms]", buf, (int)(tv.tv_usec / 1000), MXLOG_GetTick());
#else
  sprintf(time_str, "%s.%03d", buf, (int)(tv.tv_usec / 1000));
#endif
}

long long COMM_API_GetSysTime(COMM_API_SYSTIME_E type) {
  struct timespec times = {0, 0};
  long long sysTime = 0;

  if (type >= SYSTIME_MAX) {
    printf("COM_SYS_API_GetSysTime parameter error!\n");
    return -1;
  }

  clock_gettime(CLOCK_MONOTONIC, &times);
  switch (type) {
    case SYSTIME_SEC:
      sysTime = times.tv_sec;
      break;
    case SYSTIME_M_SECSYSTIME_SEC:
      sysTime = times.tv_sec * 1000 + times.tv_nsec / 1000000;
      break;
    case SYSTIME_MICRO_SEC:
      sysTime = times.tv_sec;
      sysTime = sysTime * 1000000;
      sysTime = sysTime + times.tv_nsec / 1000;
      break;
    default:
      printf("Unknow system time type %d\n", type);
      break;
  }

  return sysTime;
}

void COMM_API_GetUtcTime(char *utcTime) {
  time_t time_seconds = time(NULL);
  struct tm now_time;
  localtime_r(&time_seconds, &now_time);

  sprintf(utcTime, "%d-%d-%d %d:%d:%d", now_time.tm_year + 1900, now_time.tm_mon + 1, now_time.tm_mday,
          now_time.tm_hour, now_time.tm_min, now_time.tm_sec);
}

int COMM_API_SystemVforkCmd(const char *cmd) {
  pid_t pid;

  if (-1 == (pid = vfork())) {
    return 1;
  }
  if (0 == pid) {
    execl("/bin/sh", "sh", "-c", cmd, (char *)0);
    return 0;
  } else {
    wait(&pid);
  }
  return 0;
}

int system_popenCmd(char *command) {
  // char buffer[1024];

  // FILE *pipe = popen((const char *)command, "r");
  // if (!pipe) {
  //   printf("Failed to execute command, errno:%d\n", errno);
  //   return -1;
  // }
  // while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
  //   printf(">>>>>>>>>>> %s\n", buffer);
  //   printf(">>>>>>>>>>> %s\n", buffer);
  // }
  // pclose(pipe);
  system(command);

  return 0;
}

int COMM_API_GetTimeZoneSec(void) {
  int timezone = 0;
  time_t timep;
  time_t CurrTime;
  struct tm *gttime;
  struct tm *pTm_time;
  struct tm Tm_time;
  time(&timep);
  gttime = gmtime(&timep);
  time(&CurrTime);
  pTm_time = localtime_r(&CurrTime, &Tm_time);
  if (pTm_time->tm_mday == gttime->tm_mday) {
    timezone = (pTm_time->tm_hour - gttime->tm_hour) * 3600;
  } else {
    if (pTm_time->tm_mday > gttime->tm_mday) {
      timezone = (pTm_time->tm_hour + (24 - gttime->tm_hour)) * 3600;
    } else {
      timezone = ((pTm_time->tm_hour - 24) - gttime->tm_hour) * 3600;
    }
  }
  timezone += (pTm_time->tm_min - gttime->tm_min) * 60;
  return timezone;
}

/*****************************************************
 * @fn	    	COMM_API_SystemCommand
 * @brief		执行system，不需要等待返回
 * @param
 * @return 	解析成功返回0，失败返回-1
 *****************************************************/
int COMM_API_SystemCommand(char *cmd) {
  int ret = -1;

  if (cmd == NULL) {
    printf("COM_SYS_API_SystemCommand parameter error!\n");
    return -1;
  }

  ret = system_popenCmd(cmd);
  if (0 != ret) {
    // ret = socket_system_popenExCmd(cmd);
    // if (ret < 0) {
    //   printf("execute cmd [%s] failed! ret = %d\n", cmd, ret);
    //   ret = -1;
    // }
  }
  return ret;
}

/*****************************************************
 * @fn	    	COMM_API_SystemCommandEx
 * @brief		执行popen，等待返回
 * @param
 * @return 	解析成功返回0，失败返回-1
 *****************************************************/
int COMM_API_SystemCommandEx(const char *cmd) {
  int ret = -1;

  if (cmd == NULL) {
    printf("COM_SYS_API_SystemCommand parameter error!\n");
    return -1;
  }

#if 0
  ret = socket_system_popenExCmd((char *)cmd);
  if (ret < 0) {
    printf("socket cmd err,use local api,cmd=%s", cmd);
    ret = system_popenCmd((char *)cmd);
    if (0 != ret) {
      printf("%s %d:execute cmd [%s] failed! ret = %d\n", __func__, __LINE__, cmd, ret);
      ret = -1;
    }
  }
#else
  ret = system_popenCmd((char *)cmd);
  if (0 != ret) {
    printf("%s %d:execute cmd [%s] failed! ret = %d\n", __func__, __LINE__, cmd, ret);
    ret = -1;
  }
#endif
  return ret;
}

/*****************************************************
 * @fn	    	COMM_API_PopenCmdVerify
 * @brief		执行popen，等待返回，校验结果
 * @param
 * @return 	解析成功返回0，失败返回-1
 ******************************************************/
int COMM_API_PopenCmdVerify(const char *cmd, const char *comparestr, int timeout) {
  int ret = -1;
  char buffer[1024];

  if (cmd == NULL || comparestr == NULL) {
    printf("COM_SYS_API_SystemCommand parameter error!\n");
    return -1;
  }
#if 0
  ret = COMM_SyscmdPopenVerify(cmd, comparestr, timeout);
  if (ret >= 0) {
    ret = 0;
  } else {
    printf("socket cmd err,use local api,cmd=%s", cmd);
    FILE *pipe = popen(cmd, "r");
    if (!pipe) {
      printf("Failed to execute command, errno:%d\n", errno);
      return -1;
    }
    ret = -1;

    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
      if (strstr(buffer, comparestr) != NULL) {
        ret = 0;
        printf("buffer=%s\n", buffer);
        break;
      }
    }
    pclose(pipe);
  }
#else
  printf("socket cmd err,use local api,cmd=%s\n", cmd);
  FILE *pipe = popen(cmd, "r");
  if (!pipe) {
    printf("Failed to execute command, errno:%d\n", errno);
    return -1;
  }
  ret = -1;

  while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
    if (strstr(buffer, comparestr) != NULL) {
      ret = 0;
      printf("buffer=%s\n", buffer);
      break;
    }
  }
  pclose(pipe);
#endif
  return ret;
}

int COMM_API_ClearCache(void) {
  int ret = -1, fd = 0;

  fd = open("/proc/sys/vm/drop_caches", O_WRONLY);
  if (fd <= 0) {
    printf("open /proc/sys/vm/drop_caches error!\n");
  } else {
    ret = write(fd, "3", 1);
    if (ret != 1) {
      printf("write fail\n");
      close(fd);
      sync();
      return -1;
    }

    close(fd);
    sync();
  }

  return 0;
}

pid_t COMM_API_CheckProcExist(const char *procName) {
  pid_t pid = 0;
  FILE *fp;
  char buf[128] = {0};
  char cmd[256] = {'\0'};

  sprintf(cmd, "pidof %s", procName);
  if ((fp = popen(cmd, "r")) != NULL) {
    if (fgets(buf, 255, fp) != NULL) {
      pid = atoi(buf);
    }
  } else {
    printf("[%s::%d] procName:%s popen error!\n", __func__, __LINE__, procName);
    return -1;
  }
  pclose(fp);

  return pid;
}

int COMM_API_TelnetCtrl(int value) {
  int nRet = -1;
  int status = 0;

  if (COMM_API_CheckProcExist("telnetd") <= 0) {
    status = 0;
  } else {
    status = 1;
  }

  if (status != value) {
    if (value) {
      nRet = COMM_API_SystemCommand("telnetd &");
      if (nRet == -1) {
        nRet = COMM_API_SystemCommand("telnetd &");
      }
    } else {
      nRet = COMM_API_SystemCommand("killall -9 telnetd");
      if (nRet == -1) {
        nRet = COMM_API_SystemCommand("killall -9 telnetd");
      }
    }
  }
  return 0;
}

long COMM_API_GetFileSize(const char *FileName) {
  struct stat statbuf;

  if (stat(FileName, &statbuf) == 0) {
    return statbuf.st_size;
  }

  return -1;
}

int COMM_API_CheckFileExist(const char *FileName, int mode) {
  int ret = -1;

  if (NULL == FileName) {
    printf("COM_SYS_API_CheckFileExist parameter error!\n");
    return -1;
  }
  ret = access(FileName, mode);
  return ret;
}

int COMM_API_CheckStr(char *strBuf, int strBufLen, char *checkBuf, int checkBufLen, char *resultBuf, int resultBufLen) {
  char *pStart = NULL;
  int i = 0, ret = 0;

  if ((strBuf == NULL) || (checkBuf == NULL) || (resultBuf == NULL) || (checkBufLen > strBufLen) ||
      (resultBufLen < 8)) {
    printf("COMM_CheckStr parameter error!\n");
    return -1;
  } else {
    memset(resultBuf, 0, resultBufLen);
    if ((pStart = strstr(strBuf, checkBuf)) != NULL) {
      i = 0;
      while ((i <= resultBufLen) && (*(pStart + checkBufLen + i) != '\"')) {
        *(resultBuf + i) = *(pStart + checkBufLen + i);
        i++;
      }
      ret = 0;
    } else {
      ret = -1;
    }
  }
  return ret;
}

int COMM_API_CheckValue(char *strBuf, char *checkBuf, int checklen, int *value) {
  char *pStart = NULL;
  char resultBuf[8] = {0};
  int i = 0, ret = 0;

  if ((strBuf == NULL) || (checkBuf == NULL)) {
    printf("COMM_CheckStr parameter error!\n");
    return -1;
  } else {
    if ((pStart = strstr(strBuf, checkBuf)) != NULL) {
      memcpy(resultBuf, pStart + strlen(checkBuf), checklen);
      *value = atoi(resultBuf);
      ret = 0;
    } else {
      ret = -1;
    }
  }
  return ret;
}

static int COMM_API_GetVersion(char *pFilePath, char *pFlagVersion, char *pVersionOut) {
  FILE *fp;
  int ret = 0;
  char Buf[128], *p1 = NULL;

  if ((fp = fopen(pFilePath, "rb")) == NULL) {
    perror("fopen");
    printf("could not open file %s\n", strerror(errno));
    ret = -1;
  } else {
    while (!feof(fp)) {
      memset(Buf, 0, sizeof(Buf));
      fgets(Buf, 128, fp);
      if (strrchr(Buf, '\n')) {
        *strrchr(Buf, '\n') = '\0';
        if (strrchr(Buf, '\r')) *strrchr(Buf, '\r') = '\0';
      }
      if (0 == strncmp(Buf, pFlagVersion, strlen(pFlagVersion))) {
        p1 = (strrchr(Buf, 'V') + 1);
        strncpy(pVersionOut, p1, strlen(p1));
      }
    }
    fclose(fp);
  }
  return ret;
}

int COMM_API_GetFWVersion(char *pFWVersion) {
  int ret = -1;

  if (pFWVersion == NULL) {
    printf("COMM_API_GetFWVersion input parameter error!\n");
    return ret;
  } else {
  }
  return ret;
}

int COMM_API_GetHWVersion(char *pHWVersion) {
  int ret = -1;

  if (pHWVersion == NULL) {
    printf("COMM_API_GetFWVersion input parameter error!\n");
    return ret;
  } else {
    ret = COMM_API_GetVersion(HW_VERSION_FILE_PATH, HW_VERSION_FLAG, pHWVersion);
  }
  return ret;
}

int COMM_API_GetFWName(char *pFWName) {
  FILE *fp;
  int ret = 0;
  char Buf[128];

  if ((fp = fopen(FW_VERSION_FILE_PATH, "rb")) == NULL) {
    perror("fopen");
    printf("could not open file %s\n", strerror(errno));
    ret = -1;
  } else {
    while (!feof(fp)) {
      memset(Buf, 0, sizeof(Buf));
      fgets(Buf, 128, fp);
      if (strrchr(Buf, '\n')) {
        *strrchr(Buf, '\n') = '\0';
        if (strrchr(Buf, '\r')) *strrchr(Buf, '\r') = '\0';
      }
      if (0 == strncmp(Buf, "FWVersion=", strlen("FWVersion="))) {
        strncpy(pFWName, Buf + strlen("FWVersion="), 64);
      }
    }
    fclose(fp);
  }
  return ret;
}

long COMM_API_GetSystemRunTime(void) {
  struct sysinfo info;

  if (sysinfo(&info) != 0) {
    printf("[%s--%d] get system running time fail!\n", __FUNCTION__, __LINE__);
    return -1;
  }
  return info.uptime;
}

void COMM_API_Restart(void) {
  int sysrq_trigger = open("/proc/sysrq-trigger", O_RDWR);
  if (!sysrq_trigger) {
    printf("Can't open %s\n", "/proc/sysrq-trigger");
    return;
  }
  write(sysrq_trigger, "b", 1);
  close(sysrq_trigger);
}

extern int check_sd_mounted(char *mountPath);
extern int CACHE_log_traversal_directory(char *logSavePath, char *tmpLogPath);
int COMM_API_RebootSystem(const char *func, int line) {
  int ret = -1;
  char cmd[128] = {0};

  COMM_API_SetQuietRestartTag();
  sync();
  printf("\n\n[%s--%d]  ---------------------The system will restart----------------------!\n\n", func, line);
  sleep(2);

  system("reboot");
  sleep(10);
  COMM_API_Restart();

  return 0;
}

int COMM_GetRandomVal(unsigned short range) {
  struct timeval tv;

  gettimeofday(&tv, NULL);
  srand((unsigned int)(tv.tv_usec % 0xffffffff));
  return rand() % range;
}

int COMM_Mkdirs(char *dir) {
  int i, len, ret = 0;
  char str[128];
  strcpy(str, dir);  // 缓存文件路径
  len = strlen(str);
  for (i = 0; i < len; i++) {
    if (str[i] == '/') {
      str[i] = '\0';
      if (access(str, 0) != 0) mkdir(str, 777);
      str[i] = '/';
    }
  }
  if (len > 0 && access(str, 0) != 0) {
    return mkdir(str, 777);
  } else {
    return 0;
  }
}

int COMM_CheckTimeIsUpdate(void) {
  int ret = -1;
  struct timeval tv;

  gettimeofday(&tv, NULL);
  if (tv.tv_sec > SYS_MIN_UTC_SEC) {
    ret = 0;
  }
  // REC_DBG_ERR("[%s--%d] sec = %d!\n", __FUNCTION__, __LINE__, tv.tv_sec);
  return ret;
}
int COMM_GetTimeOfDay(void) {
  int ret = -1;
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return tv.tv_sec;
}

int get_pid_by_name(const char *task_name) {
  int pid_value = -1;
  DIR *dir;
  struct dirent *ptr;
  FILE *fp;
  char filepath[512] = {0};
  char cur_task_name[512] = {0};
  char buf[1024] = {0};

  dir = opendir("/proc");
  if (NULL != dir) {
    while ((ptr = readdir(dir)) != NULL) {
      if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0)) continue;

      if (DT_DIR != ptr->d_type) continue;

      sprintf(filepath, "/proc/%s/status", ptr->d_name);

      fp = fopen(filepath, "r");
      if (NULL != fp) {
        if (fgets(buf, 1023, fp) == NULL) {
          fclose(fp);
          continue;
        }
        sscanf(buf, "%*s %s", cur_task_name);
        if (!strcmp(task_name, cur_task_name)) {
          pid_value = atoi(ptr->d_name);
          fclose(fp);
          break;
        }

        fclose(fp);
      }
    }
    closedir(dir);
  } else {
    printf("open proc failed!\n");
  }

  return pid_value;
}

int COMM_CheckPid(char *pid, int *pidNum) {
  *pidNum = get_pid_by_name(pid);
  return *pidNum;
}

int COMM_FileUnlink(char *filePath) {
  int nRet = -1;
  if (access(filePath, F_OK) == 0) {
    if (unlink(filePath) == 0) {
      printf("delete file %s success!\n", filePath);
      nRet = 0;
    } else {
      printf("delete file %s fail!\n", filePath);
      nRet = -1;
    }
  }
  return nRet;
}

int COMM_deleteDirectory(char *dirPath) {
  DIR *dir = opendir(dirPath);
  if (dir == NULL) {
    perror("Failed to open directory");
    return -1;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    char filePath[MAX_PATH_LENGTH];
    snprintf(filePath, sizeof(filePath), "%s/%s", dirPath, entry->d_name);

    struct stat fileInfo;
    if (stat(filePath, &fileInfo) == -1) {
      printf("Failed to get file information\n");
      continue;
    }

    if (S_ISDIR(fileInfo.st_mode)) {
      COMM_deleteDirectory(filePath);
    } else {
      if (remove(filePath) == -1) {
        printf("Failed to delete file\n");
      } else {
        printf("delete %s success\n", filePath);
      }
    }
  }

  closedir(dir);

  if (rmdir(dirPath) == -1) {
    printf("Failed to delete directory\n");
    return -1;
  } else {
    printf("delete %s success\n", dirPath);
  }

  return 0;
}

int COMM_CheckDirFileNum(char *mdir) {
  int filesNum = 0;
  char cmdStr[256] = {0};
  DIR *pdir = NULL;
  struct dirent *entry = NULL;

  if (access(mdir, F_OK) != 0) {
    return -1;
  }

  printf("mdir=%s", mdir);
  if ((pdir = opendir(mdir)) == NULL) {
    printf("directory isn't exit or error \n");
    return -1;
  }

  filesNum = 0;
  while ((entry = readdir(pdir))) {
    if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0) {
      continue;
    }
    filesNum++;
  }
  closedir(pdir);

  return filesNum;
}

int COMM_NewFile(const char *filePath, const char *text) {
  if (filePath == NULL || strlen(filePath) == 0) {
    fprintf(stderr, "Invalid file path\n");
    return -1;
  }

  FILE *file = fopen(filePath, "w");

  if (!file) {
    perror("could not open file");
    printf("could not open %s, %s\n", filePath, strerror(errno));
    return -1;
  }

  if (text != NULL && strlen(text) > 0) {
    if (fprintf(file, "%s", text) < 0) {
      perror("fail to write to file");
      printf("COMM_NewFile fail to write to file %s\n", strerror(errno));
      fclose(file);
      return -1;
    }
    fflush(file);
  }

  if (fclose(file) == EOF) {
    perror("Failed to close file");
    printf("Failed to close file %s\n", strerror(errno));
    return -1;
  }
  return 0;
}

int COMM_Free(char *func, int line) {
  printf("++++++++++++++ %s:%d, system free +++++++++++++++\n", func, line);
  system("free");
  printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

  return 0;
}

int COMM_API_TimedRestart(void) {
  time_t t;
  struct tm *tm_info;
  struct tm timeResult;
  struct sysinfo info;
  unsigned long uptime_days;

  time(&t);
  tm_info = localtime_r(&t, &timeResult);

  // 获取星期的数值（0-6，0表示星期日，1表示星期一，以此类推）
  int weekday = tm_info->tm_wday;

  // 将星期数值转换为对应的字符串
  char *weekdays[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  char *currentDay = weekdays[weekday];

  if (weekday == 4) {
    if (tm_info->tm_hour == 3) {
      if (sysinfo(&info) != 0) {
        printf("Failed to get system information\n");
        return 1;
      }
      uptime_days = info.uptime / (60 * 60 * 24);
      // 开机时间大于24h,才执行重启操作
      if (uptime_days >= 1) {
        printf("Today is %s, system TimedRestart\n", currentDay);
        COMM_API_RebootSystem((char *)__FUNCTION__, __LINE__);
      }
    }
  }
  return 0;
}

int COMM_API_GetAllThreadNum(void) {
  int thrNum = 0;
  char path[256];
  struct dirent *entry;

  pid_t pid = getpid();  // 获取当前进程ID

  sprintf(path, "/proc/%d/task/", pid);
  DIR *dir = opendir(path);
  if (dir == NULL) {
    printf("opendir %s\n", path);
    return -1;
  }

  thrNum = 0;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
      thrNum++;
    }
  }

  closedir(dir);

  return thrNum;
}

char **COMM_API_GetAllThreadNames(int *thr_num) {
  int i = 0;
  int mIndex = 0;
  int ThrNum = 0;
  char path[256];
  char tidName[128][64] = {0};
  char **arr = NULL;
  struct dirent *entry;

  pid_t pid = getpid();  // 获取当前进程ID

  sprintf(path, "/proc/%d/task/", pid);
  DIR *dir = opendir(path);
  if (dir == NULL) {
    printf("opendir %s\n", path);
    return NULL;
  }

  mIndex = 0;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
      char thread_name_path[MAX_PATH_LENGTH] = {0};
      snprintf(thread_name_path, sizeof(thread_name_path) - 1, "%s%s/comm", path, entry->d_name);

      FILE *fp = fopen(thread_name_path, "r");
      if (fp == NULL) {
        perror("fopen");
        printf("could not open %s, %s\n", thread_name_path, strerror(errno));
        continue;
      }

      char thread_name[64] = {0};

      if (fgets(thread_name, sizeof(thread_name), fp) != NULL) {
        if (mIndex >= 128) {
          fclose(fp);
          break;
        }

        // 移除线程名字末尾的换行符
        thread_name[strcspn(thread_name, "\n")] = 0;
        if (strlen(thread_name) + strlen(entry->d_name) + 1 < 64) {
          strcat(thread_name, "_");
          strcat(thread_name, entry->d_name);
        }
        if (mIndex < 128) {
          snprintf(tidName[mIndex], 63, "%s", thread_name);
          mIndex++;
        }
      }
      fclose(fp);
    }
  }

  closedir(dir);

  if (mIndex) {
    arr = malloc(mIndex * sizeof(char *));
    if (arr == NULL) {
      printf("Memory allocation failed\n");
      return NULL;
    }

    for (i = 0; i < mIndex; i++) {
      arr[i] = (i < 128) ? strdup(tidName[i]) : NULL;
    }

    *thr_num = mIndex;
  }

  return arr;
}

void COMM_API_freeGetThreadNames(char **arr, int rows) {
  if (arr) {
    if (rows) {
      for (int i = 0; i < rows; i++) {
        if (arr[i]) free(arr[i]);
      }
    }

    free(arr);
  }
}

int COMM_API_PrintVersionInfo(void) {
  FILE *fp;
  char buf[512] = {0};
  char *verPath = "/vendor/app/version.info";
  printf("APP was compiled on %s at %s.\n", __DATE__, __TIME__);

  fp = fopen(verPath, "r");
  if (NULL != fp) {
    while (fgets(buf, sizeof(buf), fp) != NULL) {
      printf("%s\n", buf);
    }

    // 关闭文件
    fclose(fp);
  } else {
    perror("fopen");
    printf("could not open %s, %s\n", verPath, strerror(errno));
  }

  return 0;
}

int32_t COMM_API_open(const char *path, int flags, ...) {
  int32_t fp = 0;
  unsigned int mode = 0;
  va_list ap;

  va_start(ap, flags);
  if (flags & O_CREAT) {
    mode = va_arg(ap, unsigned int);
    va_end(ap);
  }

  fp = open(path, flags, mode);

  return fp;
}

int COMM_API_close(int32_t fp) {
  int ret = 0;

  if (fp) {
    fsync(fp);
    ret = close(fp);
  }

  return ret;
}

int32_t COMM_API_write(int32_t fp, uint8_t *buf, int32_t size) {
  uint32_t writeSize = 0;

  writeSize = write(fp, buf, size);

  if (writeSize != size) {
    return -1;
  }

  return writeSize;
}

int32_t COMM_API_read(int32_t fp, uint8_t *buf, int32_t size) {
  int32_t n_read = 0;
  int32_t readIindex = 0;
  int tmpsize = 0;
  int readCount = 0;

  tmpsize = size;
  do {
    n_read = read(fp, buf + n_read, tmpsize);
    readIindex += n_read;
    tmpsize -= n_read;
    readCount++;

  } while (readIindex != size && readCount < 100);

  if (readIindex != size) {
    return -1;
  }

  return readIindex;
}

static const uint16_t g_usCrc16tab[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7, 0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad,
    0xe1ce, 0xf1ef, 0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6, 0x9339, 0x8318, 0xb37b, 0xa35a,
    0xd3bd, 0xc39c, 0xf3ff, 0xe3de, 0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485, 0xa56a, 0xb54b,
    0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d, 0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc, 0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861,
    0x2802, 0x3823, 0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b, 0x5af5, 0x4ad4, 0x7ab7, 0x6a96,
    0x1a71, 0x0a50, 0x3a33, 0x2a12, 0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a, 0x6ca6, 0x7c87,
    0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41, 0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70, 0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a,
    0x9f59, 0x8f78, 0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f, 0x1080, 0x00a1, 0x30c2, 0x20e3,
    0x5004, 0x4025, 0x7046, 0x6067, 0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e, 0x02b1, 0x1290,
    0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256, 0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e,
    0xc71d, 0xd73c, 0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634, 0xd94c, 0xc96d, 0xf90e, 0xe92f,
    0x99c8, 0x89e9, 0xb98a, 0xa9ab, 0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3, 0xcb7d, 0xdb5c,
    0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a, 0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83,
    0x1ce0, 0x0cc1, 0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8, 0x6e17, 0x7e36, 0x4e55, 0x5e74,
    0x2e93, 0x3eb2, 0x0ed1, 0x1ef0};

uint16_t COMM_API_CalculateCRC16(const uint8_t *buf, uint32_t len) {
  uint32_t ulCounter;
  uint16_t usCrc = 0;
  for (ulCounter = 0; ulCounter < len; ulCounter++)
    usCrc = (usCrc << 8) ^ g_usCrc16tab[((usCrc >> 8) ^ *buf++) & 0x00FF];
  return usCrc;
}

uint8_t COMM_API_Calculate_xor_checksum(const unsigned char *data, size_t length) {
  uint8_t checksum = 0;
  for (size_t i = 0; i < length; i++) {
    checksum ^= data[i];
  }
  return checksum;
}

int COMM_API_MkdirRecursive(const char *path, mode_t mode) {
  char temp[256];
  char *p = NULL;
  size_t len;

  // 检查指定路径的文件或目录是否存在
  // 如果存在，则返回0，表示不需要创建目录
  if (access(path, F_OK) == 0) {
    return 0;
  }

  // Copy path
  snprintf(temp, sizeof(temp), "%s", path);
  len = strlen(temp);
  if (temp[len - 1] == '/') {
    temp[len - 1] = '\0';
  }

  // Create parent directories
  for (p = temp + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      if (access(temp, F_OK) != 0) {
        if (mkdir(temp, mode) != 0 && errno != EEXIST) {
          return -1;
        }
      }
      *p = '/';
    }
  }

  // Create the final directory
  if (mkdir(temp, mode) != 0 && errno != EEXIST) {
    return -1;
  }

  return 0;
}

long long COMM_API_get_tick_ms() {
  struct timespec ts;

  if (0 == clock_gettime(CLOCK_MONOTONIC, &ts)) {
    long long sec = ts.tv_sec;
    unsigned long long msec = ts.tv_nsec / 1000000;
    return (sec * 1000 + msec);
  }

  return 0;
}

long long COMM_API_get_tick_us() {
  struct timespec ts;

  if (0 == clock_gettime(CLOCK_MONOTONIC, &ts)) {
    long long sec = ts.tv_sec;
    long long usec = ts.tv_nsec / 1000;
    return (sec * 1000 * 1000 + usec);
  }

  return 0;
}

int COMM_API_ReadFileContent(char *fileName, char **ppContent, int *pLen) {
  PARAM_CHECK(fileName && fileName[0] && ppContent && pLen, -1);

  int ret = 0;
  long len = 0;
  FILE *fd = NULL;
  char *pContent = NULL;

  fd = fopen(fileName, "rb");
  PARAM_CHECK_STRING(fd, -1, "fopen:%s failed, errnoinfo:%s", fileName, strerror(errno));

  fseek(fd, 0, SEEK_END);
  len = ftell(fd);
  fseek(fd, 0, SEEK_SET);
  pContent = (char *)calloc(1, len + 4);
  if (pContent == NULL) {
    COMMONLOG_E("calloc error:%s", fileName);
    fclose(fd);
    return -1;
  }

  if (fread(pContent, 1, len, fd) != len) {
    free(pContent);
    fclose(fd);
    COMMONLOG_E("fread file:%s failed!", fileName);
    return -1;
  }

  *ppContent = pContent;
  *pLen = len;

  if (fd) fclose(fd);
  return 0;
}

long long COMM_API_GetUtcTimeMs() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((int64_t)tv.tv_sec * 1000 + (int64_t)tv.tv_usec / 1000);
}

void COMM_API_PrintFile(const char *filepath) {
  FILE *file = fopen(filepath, "r");
  if (file == NULL) {
    COMMONLOG_E("Error opening file");
    return;
  }

  char line[1024];
  while (fgets(line, sizeof(line), file) != NULL) {
    printf("%s\n", line);
  }

  fclose(file);
}

void COMM_API_DeleteFilesInDirectory(const char *dir_path) {
  struct dirent *entry;
  DIR *dp = opendir(dir_path);

  if (dp == NULL) {
    perror("opendir");
    return;
  }

  while ((entry = readdir(dp)) != NULL) {
    char file_path[1024];
    struct stat statbuf;

    // Skip "." and ".."
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, entry->d_name);

    if (stat(file_path, &statbuf) == 0) {
      if (S_ISREG(statbuf.st_mode)) {
        // If it's a regular file, delete it
        if (unlink(file_path) == -1) {
          perror("unlink");
        }
      }
    }
  }

  closedir(dp);
}

static unsigned int generate_seed() {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
    return (unsigned int)(ts.tv_sec ^ ts.tv_nsec);
  else
    return (unsigned int)time(NULL);
}

void juice_random(void *buf, size_t size) {
  static pthread_mutex_t rand_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_lock(&rand_mutex);

  static bool srandom_called = false;
  if (!srandom_called) {
    srand(generate_seed());
    srandom_called = true;
  }

  uint8_t *bytes = buf;
  for (size_t i = 0; i < size; ++i) bytes[i] = (uint8_t)((rand() & 0x7f80) >> 7);

  pthread_mutex_unlock(&rand_mutex);
}

uint32_t juice_rand32(void) {
  uint32_t r = 0;
  juice_random(&r, sizeof(r));
  return r;
}

uint32_t COMM_API_GenerateConv(uint8_t cam_seq, uint8_t stream_seq) {
  // 确保 cam_seq 和 stream_seq 在有效范围内（0-15）
  cam_seq &= 0x0F;     // 取低4位
  stream_seq &= 0x0F;  // 取低4位

  // 组合 cam_seq 和 stream_seq 到第一个字节
  uint8_t first_byte = (stream_seq << 4) | cam_seq;

  // 生成 24 位随机数（3个字节）
  uint32_t random_part = juice_rand32() & 0xFFFFFF;

  // 组合 first_byte 和 random_part 到一个 32 位的 conv 值
  uint32_t conv = (first_byte << 24) | random_part;

  return conv;
}

int COMM_API_MoveFile(const char *src, const char *dest) {
  FILE *srcFile = NULL, *destFile = NULL;
  char buffer[1024];
  size_t bytesRead;

  // 打开源文件
  srcFile = fopen(src, "rb");
  if (srcFile == NULL) {
    perror("Error opening source file");
    return -1;
  }

  // 打开目标文件
  destFile = fopen(dest, "wb");
  if (destFile == NULL) {
    perror("Error opening destination file");
    fclose(srcFile);
    return -1;
  }

  // 读取源文件并写入到目标文件
  while ((bytesRead = fread(buffer, 1, sizeof(buffer), srcFile)) > 0) {
    if (fwrite(buffer, 1, bytesRead, destFile) != bytesRead) {
      perror("Error writing to destination file");
      fclose(srcFile);
      fclose(destFile);
      return -1;
    }
  }

  fclose(srcFile);
  fclose(destFile);
  unlink(src);

  return 0;
}

/* log api start */
int COMM_API_log_local_copy(const char *srcLog, const char *destLog) {
  FILE *sourceFile = fopen(srcLog, "r");
  FILE *destFile = fopen(destLog, "w");

  if (sourceFile && destFile) {
    char str[1024];
    while (fgets(str, sizeof(str), sourceFile) != NULL) {
      fputs(str, destFile);
    }
  }

  if (sourceFile) fclose(sourceFile);
  if (destFile) fclose(destFile);

  return 0;
}

int COMM_API_log_local_save(char *baseDir, char *devId, const char *logpath, const char *logName) {
  char destDir[256] = {0};
  char destLog[256] = {0};

  if (access(baseDir, F_OK) == 0) {
    memset(destDir, 0, sizeof(destDir));
    snprintf(destDir, sizeof(destDir) - 1, "%s/%s", baseDir, devId);
    if (access(destDir, F_OK) != 0) {
      mkdir(destDir, 0755);
    }
    snprintf(destLog, sizeof(destLog) - 1, "%s/%s", destDir, logName);
    COMM_API_log_local_copy(logpath, destLog);
  }

  return 0;
}

const char *COMM_API_get_basename(const char *path) {
  const char *base = strrchr(path, '/');
  return base ? base + 1 : path;
}

int COMM_API_logFileOpen(int *m_LogSaveFd, char *saveDir, char *mpid, char *SaveLogName, int logNameLen, int m_time) {
  int ret = 0;

  if (!(*m_LogSaveFd)) {
    memset(SaveLogName, 0, sizeof(SaveLogName));

    long long utcTime = time(NULL);

    if (m_time) {
      snprintf(SaveLogName, logNameLen - 1, "%s/%s_%d_%lld.log", saveDir, mpid, m_time, utcTime);
    } else {
      snprintf(SaveLogName, logNameLen - 1, "%s/%s_%lld.log", saveDir, mpid, utcTime);
    }

    *m_LogSaveFd = open(SaveLogName, (O_RDWR | O_CREAT | O_TRUNC), 0777);
    if (*m_LogSaveFd > 0) {
      printf("open %s success\n", SaveLogName);
    } else {
      perror("open fail\n");
      ret = -1;
    }
  }

  return ret;
}

int COMM_API_logFileWrite(int m_LogSaveFd, char *strlog, int logSize) {
  int nRet = 0;

  if (m_LogSaveFd) {
    nRet = write(m_LogSaveFd, strlog, logSize);
    if (nRet == -1) {
      perror(__FUNCTION__);
      return -1;
    }
  }

  return nRet;
}

int COMM_API_logFileClose(int m_LogSaveFd) {
  if (m_LogSaveFd) {
    fsync(m_LogSaveFd);
    close(m_LogSaveFd);

    return 0;
  }

  return -1;
}

int COMM_API_CheckProgramExist(const char *keyword) {
  DIR *proc_dir;
  struct dirent *entry;
  char cmdline_path[256];
  char cmdline[1024];
  FILE *fp;

  if (keyword == NULL) {
    fprintf(stderr, "Keyword cannot be NULL.\n");
    return 0;
  }

  proc_dir = opendir("/proc");
  if (proc_dir == NULL) {
    perror("opendir /proc");
    return 0;
  }

  while ((entry = readdir(proc_dir)) != NULL) {
    // 检查目录名是否为纯数字（进程ID）
    if (entry->d_type == DT_DIR && strspn(entry->d_name, "0123456789") == strlen(entry->d_name)) {
      // 构造 /proc/[pid]/cmdline 文件的路径
      snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%s/cmdline", entry->d_name);

      // 打开 cmdline 文件读取进程命令行参数
      fp = fopen(cmdline_path, "r");
      if (fp == NULL) {
        continue;  // 无法打开，跳过
      }

      // 读取命令行内容
      if (fgets(cmdline, sizeof(cmdline), fp) != NULL) {
        // 检查命令行是否包含指定的关键词
        if (strstr(cmdline, keyword) != NULL) {
          fclose(fp);
          closedir(proc_dir);
          return 1;  // 找到包含指定关键词的进程
        }
      }
      fclose(fp);
    }
  }

  // 关闭 /proc 目录
  closedir(proc_dir);
  return 0;
}

int COMM_API_SyncTimnZone() {
  int len = 0;
  char *content = NULL;
  char *timeZoneFileName = "/parameter/config/timezone.txt";
  if (COMM_API_CheckFileExist(timeZoneFileName, F_OK) == 0) {
    COMM_API_ReadFileContent(timeZoneFileName, &content, &len);
    if (content && len > 0) {
      if (content[len - 1] == '\n') content[len - 1] = '\0';
      CommToolbox_SetTimeZonePosix(content);
      free(content);
    } else {
      return -1;
    }
  } else {
    return -1;
  }

  return 0;
}

int COMM_API_check_log_num(char *logdir, char *pid, int maxNum) {
  int filenum = 0;
  char destlog[128] = {0};

  DIR *pdir = NULL;
  struct dirent *entry = NULL;

  if (access(logdir, F_OK) == 0) {
    if ((pdir = opendir(logdir)) == NULL) {
      printf("directory (%s) isn't exit or error \n", logdir);
      return -1;
    }

    while ((entry = readdir(pdir))) {
      if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0) {
        continue;
      }

      if (strstr(entry->d_name, pid) != NULL) {
        filenum++;
        if (filenum >= maxNum) {
          memset(destlog, 0, sizeof(destlog));
          snprintf(destlog, sizeof(destlog) - 1, "%s/%s", logdir, entry->d_name);
          unlink(destlog);
        }
      }
    }

    closedir(pdir);
  }

  return 0;
}

char *COMM_API_PayloadToHexString(const unsigned char *payload, size_t payloadSize) {
  size_t outputSize = payloadSize * 3;               // 2 hex chars + 1 space per byte
  char *hexString = (char *)malloc(outputSize + 1);  // +1 for the null terminator

  if (!hexString) {
    return NULL;  // Memory allocation failed
  }

  for (size_t i = 0; i < payloadSize; i++) {
    sprintf(hexString + i * 3, "%02x ", payload[i]);
  }

  hexString[outputSize - 1] = '\0';  // Replace the last space with a null terminator
  return hexString;
}

void COMM_API_get_readable_rate_str(uint32_t rate, char *str, int size) {
  if (rate >= 1024 * 1024) {
    snprintf(str, size, "%.2f MB/s", (float)rate / (1024 * 1024));
  } else if (rate >= 1024) {
    snprintf(str, size, "%.2f KB/s", (float)rate / 1024);
  } else {
    snprintf(str, size, "%u B/s", rate);
  }
}

// 函数：判断字符串是否为UTF-8的十六进制字符串格式
int COMM_API_is_utf8_hex_string(const char *str) {
  const char *p = str;
  while (*p) {
    if (*p == '\\' && *(p + 1) == 'x') {
      // 检查接下来的两个字符是否为有效的十六进制数字
      if (isxdigit(*(p + 2)) && isxdigit(*(p + 3))) {
        p += 4;  // 跳过"\x"和两个十六进制数字
      } else {
        return 0;  // 格式不正确
      }
    } else {
      // 如果不是以"\x"开头，则格式不符合要求
      return 0;
    }
  }
  return 1;
}

// 函数：将包含\x十六进制表示的字符串转换为实际的字节序列
void COMM_API_unescape(const char *src, unsigned char *dest) {
  while (*src) {
    if (*src == '\\' && *(src + 1) == 'x') {
      // 提取接下来的两个十六进制字符
      char hex[3] = {*(src + 2), *(src + 3), '\0'};
      // 将十六进制字符串转换为实际的字节
      *dest = (unsigned char)strtol(hex, NULL, 16);
      src += 4;  // 跳过\x和两个十六进制字符
    } else {
      // 如果不是\x开头，直接复制字符
      *dest = *src;
      src++;
    }
    dest++;
  }
  *dest = '\0';  // 添加字符串结束符
}

bool CONNECT_GetBindStatus(void) {
  bool statsu = false;
  int isExtender = COMM_API_IsExtender();

  if (isExtender) {
    if (access(REPEATER_BIND_FILE, F_OK) == 0) {
      statsu = true;
    }
  } else {
    if (access(BIND_STATUS_FILE, F_OK) == 0) {
      statsu = true;
    }
  }

  return statsu;
}

void COMM_API_delete_jpg_files_in_tmp() {
  DIR *dir;
  struct dirent *entry;
  char filepath[PATH_MAX];

  dir = opendir("/tmp");
  if (dir == NULL) {
    perror("Failed to open /tmp directory");
    return;
  }

  while ((entry = readdir(dir)) != NULL) {
    // Skip the "." and ".." entries
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    // Check if the file ends with ".jpg"
    size_t len = strlen(entry->d_name);
    if (len > 4 && strcmp(entry->d_name + len - 4, ".jpg") == 0) {
      // Construct the full file path
      snprintf(filepath, sizeof(filepath), "/tmp/%s", entry->d_name);

      // Attempt to delete the file
      if (remove(filepath) == 0) {
        printf("Deleted: %s\n", filepath);
      } else {
        perror("Failed to delete file");
      }
    }
  }

  closedir(dir);
}

int COMM_API_WriteFileBinary(const char *path, const unsigned char *data, size_t len) {
  FILE *file = fopen(path, "wb");
  if (file) {
    fwrite(data, 1, len, file);
    fclose(file);
    sync();
    return 0;
  }
  return -1;
}

/* log api end */

// 静默重启
int COMM_API_IsQuietRestartTag(void) {
  if (access(QUIET_RESTART_TAG, F_OK) == 0) {
    return 1;
  }
  return 0;
}
void COMM_API_SetQuietRestartTag(void) {
  printf("%s %s\n", __FUNCTION__, QUIET_RESTART_TAG);
  COMM_NewFile(QUIET_RESTART_TAG, NULL);
}
void COMM_API_CleanQuietRestartTag(void) { COMM_FileUnlink(QUIET_RESTART_TAG); }

int COMM_API_get_file_lines(const char *file_path) {
  PARAM_CHECK(file_path && file_path[0], 0);

  FILE *file = fopen(file_path, "r");
  if (file == NULL) {
    return 0;
  }

  int line_count = 0;
  char buffer[1024];

  // 逐行读取文件并计数
  while (fgets(buffer, sizeof(buffer), file) != NULL) {
    line_count++;
  }

  fclose(file);
  return line_count;
}

/**
 * 获取指定网络接口的子网掩码
 * @param interface_name 网络接口名称（如 "eth0" 或 "wlan0"）
 * @param subnet_mask 用于存储子网掩码的缓冲区
 * @param size 缓冲区大小
 * @return 0 成功，-1 失败
 */
int COMM_API_get_subnet_mask(const char *interface_name, char *subnet_mask, size_t size) {
  if (!interface_name || !subnet_mask || size == 0) {
    return -1;
  }

  int fd;
  struct ifreq ifr;

  // 打开套接字
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    perror("socket");
    return -1;
  }

  // 设置接口名称
  strncpy(ifr.ifr_name, interface_name, IFNAMSIZ - 1);
  ifr.ifr_name[IFNAMSIZ - 1] = '\0';

  // 获取子网掩码
  if (ioctl(fd, SIOCGIFNETMASK, &ifr) < 0) {
    perror("ioctl");
    close(fd);
    return -1;
  }

  // 转换为点分十进制格式
  struct sockaddr_in *netmask = (struct sockaddr_in *)&ifr.ifr_netmask;
  strncpy(subnet_mask, inet_ntoa(netmask->sin_addr), size - 1);
  subnet_mask[size - 1] = '\0';

  close(fd);
  return 0;
}

/**
 * 判断两个 IP 地址是否在同一子网
 * @param ip1 第一个 IP 地址
 * @param ip2 第二个 IP 地址
 * @param interface_name 网络接口名称，用于获取子网掩码
 * @return true 如果在同一子网，false 否则
 */
bool COMM_API_same_subnet(const char *ip1, const char *ip2, const char *interface_name) {
  if (!ip1 || !ip2 || !interface_name) {
    return false;
  }

  char subnet_mask[INET_ADDRSTRLEN] = {0};
  if (COMM_API_get_subnet_mask(interface_name, subnet_mask, sizeof(subnet_mask)) != 0) {
    return false;
  }

  struct in_addr addr1, addr2, mask;

  // 转换 IP 地址和子网掩码为网络字节序
  if (inet_aton(ip1, &addr1) == 0 || inet_aton(ip2, &addr2) == 0 || inet_aton(subnet_mask, &mask) == 0) {
    return false;
  }

  // 按位与操作，判断是否在同一子网
  return (addr1.s_addr & mask.s_addr) == (addr2.s_addr & mask.s_addr);
}

size_t COMM_API_get_free_hp(void) {
  struct sysinfo si;
  sysinfo(&si);
  return si.freeram * si.mem_unit;
}

size_t COMM_API_get_buff_cache_size() {
  FILE *fp = fopen("/proc/meminfo", "r");
  if (!fp) {
    perror("Error opening /proc/meminfo");
    return 0;
  }

  char line[256];
  size_t buffers = 0, cached = 0, slab = 0;

  while (fgets(line, sizeof(line), fp)) {
    char label[128];
    size_t value;
    if (sscanf(line, "%127s %zu", label, &value) == 2) {
      if (strcmp(label, "Buffers:") == 0) {
        buffers = value;
      } else if (strcmp(label, "Cached:") == 0) {
        cached = value;
      } else if (strcmp(label, "Slab:") == 0) {
        slab = value;
      }
    }
  }

  fclose(fp);

  // 计算总和并转换为字节
  return (buffers + cached) * 1024;
}

int COMM_API_get_fd_count() {
  int fd_count = 0;
  DIR *dir = opendir("/proc/self/fd");

  if (dir == NULL) {
    perror("opendir");
    return -1;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_LNK) {
      fd_count++;
    }
  }

  closedir(dir);

  // Subtract 2 for '.' and '..' directories
  return fd_count - 2;
}

int COMM_API_get_memory_sizes(system_memory_size_t *memSizes) {
  if (memSizes == NULL) return 0;

  FILE *fp = fopen("/proc/self/status", "r");

  if (fp == NULL) {
    perror("fopen");
    return -1;
  }

  char line[256];
  while (fgets(line, sizeof(line), fp)) {
    if (strncmp(line, "VmRSS:", 6) == 0) {
      sscanf(line, "VmRSS: %zu kB", &memSizes->VmRSS);
    } else if (strncmp(line, "VmStk:", 6) == 0) {
      sscanf(line, "VmStk: %zu kB", &memSizes->VmStk);
    } else if (strncmp(line, "VmExe:", 6) == 0) {
      sscanf(line, "VmExe: %zu kB", &memSizes->VmExe);
    } else if (strncmp(line, "VmLib:", 6) == 0) {
      sscanf(line, "VmLib: %zu kB", &memSizes->VMLib);
    } else if (strncmp(line, "RssAnon:", 8) == 0) {
      sscanf(line, "RssAnon: %zu kB", &memSizes->RssAnon);
    } else if (strncmp(line, "RssFile:", 8) == 0) {
      sscanf(line, "RssFile: %zu kB", &memSizes->RssFile);
    } else if (strncmp(line, "RssShmem:", 9) == 0) {
      sscanf(line, "RssShmem: %zu kB", &memSizes->RssShmem);
    } else if (strncmp(line, "Threads:", 8) == 0) {
      memSizes->ThreadsNum = atoi(line + 8);
    }
  }

  fclose(fp);
  return 0;
}

static int s_extender_is_load_file = 0;
static int s_extender_is_extender = 0;
static pthread_mutex_t s_extender_mutex = PTHREAD_MUTEX_INITIALIZER;

// 判断是否是拓展器，0：不是，1：是
int COMM_API_IsExtender() {
  int ret = 0;
  pthread_mutex_lock(&s_extender_mutex);
  if (!s_extender_is_load_file) {
    s_extender_is_load_file = 1;
    s_extender_is_extender = (access(CAMERA_CLIENT_BIND_CONF, F_OK) == 0 ? 0 : 1);
  }
  ret = s_extender_is_extender;
  pthread_mutex_unlock(&s_extender_mutex);

  return ret;
}

// 重置拓展器状态，强制重新读取CAMERA_CLIENT_BIND_CONF文件状态
void COMM_API_ResetExtenderStatus() {
  pthread_mutex_lock(&s_extender_mutex);
  s_extender_is_load_file = 0;  // 重置加载标志，强制重新读取
  s_extender_is_extender = (access(CAMERA_CLIENT_BIND_CONF, F_OK) == 0 ? 0 : 1);
  pthread_mutex_unlock(&s_extender_mutex);
  printf("COMM_API_ResetExtenderStatus, s_extender_is_extender:%d\n", s_extender_is_extender);
}

int COMM_API_isIFrame(int isH265, unsigned char *data) {
  unsigned char nal_type = 0;
  if (isH265) {
    if ((data[0] == 0) && (data[1] == 0) && (data[2] == 0) && (data[3] == 1)) {
      nal_type = (data[4] >> 1);
      if (nal_type == 32 || (nal_type >= 16 && nal_type <= 21)) {
        return 1;
      }
    } else if ((data[0] == 0) && (data[1] == 0) && (data[2] == 1)) {
      nal_type = (data[3] >> 1);
      if (nal_type == 32 || (nal_type >= 16 && nal_type <= 21)) {
        return 1;
      }
    }
  } else {
    if ((data[0] == 0) && (data[1] == 0) && (data[2] == 0) && (data[3] == 1)) {
      nal_type = (data[4] & 0x1f);
      if (nal_type == 7) {
        return 1;
      }
    } else if ((data[0] == 0) && (data[1] == 0) && (data[2] == 1)) {
      nal_type = (data[3] & 0x1f);
      if (nal_type == 7) {
        return 1;
      }
    }
  }

  return 0;
}