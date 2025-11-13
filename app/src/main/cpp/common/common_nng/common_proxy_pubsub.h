/***********************************************************************************
 * 文 件 名   : common_proxy_pubsub.h
 * 负 责 人   : xwq
 * 创建日期   : 2024年5月11日
 * 文件描述   : common_proxy_pubsub.c 的头文件
 * 版权说明   : Copyright (c) 2015-2029  SWITCHBOT SHENZHEN IOT
 * 字符编码   : UTF-8
 * 修改日志   :
 ***********************************************************************************/

#ifndef __COMMON_PROXY_PUBSUB_H__
#define __COMMON_PROXY_PUBSUB_H__
/*----------------------------------------------*
 * 包含头文件                                           *
 *----------------------------------------------*/
#include <nng.h>
#include <protocol/pubsub0/pub.h>
#include <protocol/pubsub0/sub.h>

#include "common/CommonBase.h"

/*----------------------------------------------*
 * 宏定义                              *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 外部变量说明                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 全局变量                        *
 *----------------------------------------------*/
typedef enum {
  NNG_SOCKET_PUB = 0,
  NNG_SOCKET_SUB,

  NNG_SOCKET_MAX
} NNG_SOCKET_E;

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

extern void Common_fatal(const char *func, int rv);
const char *Common_date_string(void);

extern int Common_proxy_pub_get_topic_index(void *arg, char *topicHead, char *method);
extern int Common_proxy_pub_add_topic(COMMON_PUB_CONF_S *pub_conf, char *topicHead, char *method,
                                      msgHandleCallback msgCb);
extern int Common_proxy_pub_free_topic(COMMON_PUB_CONF_S *pub_conf);
extern int Common_proxy_pub_msg(nng_socket sid, char *sendMsg, int msgLen);

extern void Common_nng_sub_msgCb(void *arg);
extern int Common_proxy_sub_add_topic(COMMON_SUB_CONF_S *sub_conf, char *topicHead, char *method,
                                      msgHandleCallback msgCb);
extern int Common_proxy_sub_free_topic(COMMON_SUB_CONF_S *sub_conf);
extern int Common_proxy_sub_aio_init(COMMON_SUB_CONF_S *sub_Conf);
extern int Common_proxy_sub_aio_deinit(COMMON_SUB_CONF_S *sub_Conf);

extern int Common_nng_pubsub_dial(void *arg, NNG_SOCKET_E nngSocketType);

extern int Common_nng_socket_open(nng_socket *sid, NNG_SOCKET_E nngSocketType);
extern int Common_nng_socket_close(nng_socket sid);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __COMMON_PROXY_PUBSUB_H__ */
