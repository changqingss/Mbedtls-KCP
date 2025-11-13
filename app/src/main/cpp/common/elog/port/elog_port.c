/*
 * This file is part of the EasyLogger Library.
 *
 * Copyright (c) 2015, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for linux.
 * Created on: 2015-04-28
 */

#include <elog.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#ifdef ELOG_FILE_ENABLE
#include <elog_file.h>
#endif
static pthread_mutex_t output_lock;

eLogSaveCb g_logSaveCb;

/**
 * EasyLogger port initialize
 *
 * @return result
 */
ElogErrCode elog_port_init(void) {
  ElogErrCode result = ELOG_NO_ERR;

  pthread_mutex_init(&output_lock, NULL);

#ifdef ELOG_FILE_ENABLE
  elog_file_init();
#endif

  return result;
}

/**
 * EasyLogger port deinitialize
 *
 */
void elog_port_deinit(void) {
#ifdef ELOG_FILE_ENABLE
  elog_file_deinit();
#endif

  pthread_mutex_destroy(&output_lock);
}

/**
 * output log port interface
 *
 * @param log output of log
 * @param size log size
 */
void elog_port_output(const char *log, size_t size) {
  /* output to terminal */
#ifdef ELOG_TERMINAL_ENABLE
  // printf("%.*s", (int)size, log);

  g_logSaveCb((char *)log, size);
#endif

#ifdef ELOG_FILE_ENABLE
  /* write the file */
  elog_file_write(log, size);
#endif
}

int elog_setSaveCb(eLogSaveCb logSaveCb) {
  g_logSaveCb = logSaveCb;
  return 0;
}

/**
 * output lock
 */
void elog_port_output_lock(void) { pthread_mutex_lock(&output_lock); }

/**
 * output unlock
 */
void elog_port_output_unlock(void) { pthread_mutex_unlock(&output_lock); }

/**
 * get current time interface
 *
 * @return current time
 */
const char *elog_port_get_time(void) {
  static char cur_system_time[32] = {0};  // 增大缓冲区大小，避免溢出

  struct timeval nowTime;
  if (gettimeofday(&nowTime, NULL) != 0) {
    // 如果 gettimeofday 失败，返回错误信息
    return "Error getting time";
  }

  struct tm cur_tm;
  localtime_r(&nowTime.tv_sec, &cur_tm);  // 使用 gettimeofday 获取的秒数填充 tm 结构体

  // 格式化当前时间到 cur_system_time (YYYY-MM-DD HH:MM:SS)
  strftime(cur_system_time, sizeof(cur_system_time), "%Y-%m-%d %T", &cur_tm);

  // 获取毫秒部分
  int msec = nowTime.tv_usec / 1000;  // 微秒转毫秒

  // 将毫秒部分格式化为 ".xxx"
  char msecSuffix[8];  // 足够存储 ".xxx" 和 null 终止符
  snprintf(msecSuffix, sizeof(msecSuffix), ".%03d", msec);

  // 将毫秒部分拼接到 cur_system_time
  strncat(cur_system_time, msecSuffix, sizeof(cur_system_time) - strlen(cur_system_time) - 1);

  return cur_system_time;
}

/**
 * get current process name interface
 *
 * @return current process name
 */
const char *elog_port_get_p_info(void) {
  static char cur_process_info[10] = {0};

  snprintf(cur_process_info, 10, "pid:%04d", getpid());

  return cur_process_info;
}

/**
 * get current thread name interface
 *
 * @return current thread name
 */
const char *elog_port_get_t_info(void) {
  static char cur_thread_info[10] = {0};

  snprintf(cur_thread_info, 10, "tid:%04ld", pthread_self());

  return cur_thread_info;
}
