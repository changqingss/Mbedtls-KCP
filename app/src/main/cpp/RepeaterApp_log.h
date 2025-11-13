#ifndef __REPEATER_APP_LOG_H__
#define __REPEATER_APP_LOG_H__

#include <android/log.h>

#ifdef __cplusplus
extern "C" {
#endif

// ------------------------------
// Android log tag
// ------------------------------
#ifndef REPEATER_LOG_TAG
#define REPEATER_LOG_TAG "REPEATER"
#endif

// ------------------------------
// 日志等级封装
// ------------------------------
#define REPEATERLOG_A(...) __android_log_print(ANDROID_LOG_FATAL,   REPEATER_LOG_TAG, __VA_ARGS__)
#define REPEATERLOG_E(...) __android_log_print(ANDROID_LOG_ERROR,   REPEATER_LOG_TAG, __VA_ARGS__)
#define REPEATERLOG_W(...) __android_log_print(ANDROID_LOG_WARN,    REPEATER_LOG_TAG, __VA_ARGS__)
#define REPEATERLOG_I(...) __android_log_print(ANDROID_LOG_INFO,    REPEATER_LOG_TAG, __VA_ARGS__)
#define REPEATERLOG_D(...) __android_log_print(ANDROID_LOG_DEBUG,   REPEATER_LOG_TAG, __VA_ARGS__)
#define REPEATERLOG_V(...) __android_log_print(ANDROID_LOG_VERBOSE, REPEATER_LOG_TAG, __VA_ARGS__)

// ------------------------------
// 参数检查宏
// ------------------------------
#ifndef PARAM_CHECK
#define PARAM_CHECK(x, ret)             \
  do {                                  \
    if (!(x)) {                         \
      REPEATERLOG_E("param is error");  \
      return ret;                       \
    }                                   \
  } while (0)
#endif

#ifndef PARAM_CHECK_STRING
#define PARAM_CHECK_STRING(x, ret, fmt, ...) \
  do {                                       \
    if (!(x)) {                              \
      REPEATERLOG_E(fmt, ##__VA_ARGS__);     \
      return ret;                            \
    }                                        \
  } while (0)
#endif

#ifndef PARAM_FAILED
#define PARAM_FAILED(x, fmt, ...)       \
  do {                                  \
    if (!(x)) {                         \
      REPEATERLOG_E(fmt, ##__VA_ARGS__);\
      goto FAILED;                      \
    }                                   \
  } while (0)
#endif

#ifndef PARAM_EXIT
#define PARAM_EXIT(x, fmt, ...)         \
  do {                                  \
    if (!(x)) {                         \
      REPEATERLOG_E(fmt, ##__VA_ARGS__);\
      goto exit;                        \
    }                                   \
  } while (0)
#endif

#ifndef PARAM_EXIT1
#define PARAM_EXIT1(x, r, fmt, ...)     \
  do {                                  \
    if (!(x)) {                         \
      ret = r;                          \
      REPEATERLOG_E(fmt, ##__VA_ARGS__);\
      goto exit;                        \
    }                                   \
  } while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif // __REPEATER_APP_LOG_H__
