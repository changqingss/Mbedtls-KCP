/***********************************************************************************
 * 文 件 名   : log_conf.h
 * 负 责 人   : yzf
 * 创建日期   : 2024年7月5日
 * 文件描述   :
 * 版权说明   : Copyright (c) 2015-2024  SWITCHBOT SHENZHEN IOT
 * 字符编码   : UTF-8
 * 修改日志   :
 ***********************************************************************************/

#ifndef __LOG_CONF_H__
#define __LOG_CONF_H__
/*----------------------------------------------*
 * 包含头文件                                           *
 *----------------------------------------------*/
/*----------------------------------------------*
 * 宏定义                              *
 *----------------------------------------------*/
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/*----------------------------------------------*
 * 宏定义                              *
 *----------------------------------------------*/

#include "elog.h"
#define COMMONLOG_A(...) elog_a("COMMON", __VA_ARGS__)
#define COMMONLOG_E(...) elog_e("COMMON", __VA_ARGS__)
#define COMMONLOG_W(...) elog_w("COMMON", __VA_ARGS__)
#define COMMONLOG_I(...) elog_i("COMMON", __VA_ARGS__)
#define COMMONLOG_D(...) elog_d("COMMON", __VA_ARGS__)
#define COMMONLOG_V(...) elog_v("COMMON", __VA_ARGS__)

#define LOG_A(tag, fmt, ...) elog_a(tag, fmt, ##__VA_ARGS__)
#define LOG_E(tag, fmt, ...) elog_e(tag, fmt, ##__VA_ARGS__)
#define LOG_W(tag, fmt, ...) elog_w(tag, fmt, ##__VA_ARGS__)
#define LOG_I(tag, fmt, ...) elog_i(tag, fmt, ##__VA_ARGS__)
#define LOG_D(tag, fmt, ...) elog_d(tag, fmt, ##__VA_ARGS__)
#define LOG_V(tag, fmt, ...) elog_v(tag, fmt, ##__VA_ARGS__)

#ifndef LOG_KeyInfoUp
#define LOG_KeyInfoUp printf
#endif

#ifndef PARAM_CHECK
#define PARAM_CHECK(x, ret)          \
  do {                               \
    if (!(x)) {                      \
      COMMONLOG_E("param is error"); \
      return ret;                    \
    }                                \
  } while (0);
#endif

#ifndef PARAM_CHECK_STRING
#define PARAM_CHECK_STRING(x, ret, fmt, ...) \
  do {                                       \
    if (!(x)) {                              \
      COMMONLOG_E(fmt, ##__VA_ARGS__);       \
      return ret;                            \
    }                                        \
  } while (0);
#endif

#ifndef PARAM_FAILED
#define PARAM_FAILED(x, fmt, ...) \
  do {                            \
    if (!(x)) {                   \
      DLOGE(fmt, ##__VA_ARGS__);  \
      goto FAILED;                \
    }                             \
  } while (0);
#endif

#ifndef PARAM_EXIT
#define PARAM_EXIT(x, fmt, ...)        \
  do {                                 \
    if (!(x)) {                        \
      COMMONLOG_E(fmt, ##__VA_ARGS__); \
      goto exit;                       \
    }                                  \
  } while (0);
#endif

#ifndef PARAM_EXIT1
#define PARAM_EXIT1(x, r, fmt, ...)    \
  do {                                 \
    if (!(x)) {                        \
      ret = r;                         \
      COMMONLOG_E(fmt, ##__VA_ARGS__); \
      goto exit;                       \
    }                                  \
  } while (0);
#endif

#ifndef PARAM_EXIT2
#define PARAM_EXIT2(x) \
  do {                 \
    if (!(x)) {        \
      goto exit;       \
    }                  \
  } while (0);
#endif

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

#endif /* __LOG_CONF_H__ */
