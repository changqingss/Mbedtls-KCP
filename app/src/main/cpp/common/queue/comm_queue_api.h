/***********************************************************************************
 * 文 件 名   : comm_queue_api.h
 * 负 责 人   : xwq
 * 创建日期   : 2024年11月19日
 * 文件描述   : comm_queue_api.c 的头文件
 * 版权说明   : Copyright (c) 2015-2029  SWITCHBOT SHENZHEN IOT
 * 字符编码   : UTF-8
 * 修改日志   :
 ***********************************************************************************/

#ifndef __COMM_QUEUE_API_H__
#define __COMM_QUEUE_API_H__
/*----------------------------------------------*
 * 包含头文件                                           *
 *----------------------------------------------*/
#include <pthread.h>

#include "comm_queue.h"
/*----------------------------------------------*
 * 宏定义                              *
 *----------------------------------------------*/
#define QUE_DATA_MAX_SIZE (5 * 1024)
/*----------------------------------------------*
 * 外部变量说明                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 全局变量                        *
 *----------------------------------------------*/

typedef struct COMM_MSG {
  int dataType;
  int datalen;
  char data[0];
} COMM_MSG, *PCOMM_MSG;

typedef struct COMM_QUE_DATA {
  int dataType;
  int datalen;
  char data[QUE_DATA_MAX_SIZE];
  unsigned long long seq;
  long long timestamp;  // 消息的时间戳
} COMM_QUE_DATA_S;

typedef struct COMM_QUE {
  COMM_QUE_DATA_S setdata;
  COMM_QUE_DATA_S getdata;
  pthread_mutex_t queMutex;
  pthread_mutex_t appMutex;
  int current_length;  // 当前队列长度
  Queue* que;
} COMM_QUE_S;

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

extern void COMM_API_queue_free(COMM_QUE_S* g_que);

extern int COMM_API_queue_is_empty(COMM_QUE_S* g_que);

extern int COMM_API_queue_push_tail(COMM_QUE_S* g_que);
extern int COMM_API_queue_push_head(COMM_QUE_S* g_que);

extern int COMM_API_queue_pop_tail(COMM_QUE_S* g_que);
extern int COMM_API_queue_pop_head(COMM_QUE_S* g_que);

extern int COMM_API_queue_init(COMM_QUE_S* g_que);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __COMM_QUEUE_API_H__ */
