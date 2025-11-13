#include "common_proxy_pubsub.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

void Common_fatal(const char *func, int rv) { fprintf(stderr, "%s: %s\n", func, nng_strerror(rv)); }

const char *Common_date_string(void) {
  static char cur_system_time[128] = {0};

  time_t cur_t;
  struct tm cur_tm;

  time(&cur_t);
  localtime_r(&cur_t, &cur_tm);

  memset(cur_system_time, 0, sizeof(cur_system_time));
  strftime(cur_system_time, sizeof(cur_system_time), "%Y-%m-%d %T", &cur_tm);

  struct timeval nowTime;
  if (0 != gettimeofday(&nowTime, NULL)) {
    return 0;
  }

  int msec = nowTime.tv_usec / 1000;
  char msecSuffix[100] = {0};

  snprintf(msecSuffix, sizeof(msecSuffix) - 1, ".%03d", msec);
  strcat(cur_system_time, msecSuffix);
  return cur_system_time;
}

int Common_proxy_pub_add_topic(COMMON_PUB_CONF_S *pub_conf, char *topicHead, char *method, msgHandleCallback msgCb) {
  TopicManager *topicManager = pub_conf->topicManager;

  if (topicManager) {
    if (topicManager->count >= topicManager->capacity) {
      // 如果回调函数数量达到了数组容量，需要增加数组大小
      int newCapacity = (topicManager->capacity == 0) ? 1 : topicManager->capacity * 2;
      topicCallbackManager *newtopicCallbacks =
          (topicCallbackManager *)realloc(topicManager->topicCb, newCapacity * sizeof(topicCallbackManager));
      if (newtopicCallbacks == NULL) {
        // 内存分配失败
        fprintf(stderr, "Failed to allocate memory for callbacks\n");
        return -1;
      }
      topicManager->topicCb = newtopicCallbacks;
      topicManager->capacity = newCapacity;
    }
    // 在回调函数指针数组的末尾添加新的回调函数

    topicManager->topicCb[topicManager->count].msgHandleCb = msgCb;

    memset(topicManager->topicCb[topicManager->count].topic.method, 0,
           sizeof(topicManager->topicCb[topicManager->count].topic.method));
    memset(topicManager->topicCb[topicManager->count].topic.str, 0,
           sizeof(topicManager->topicCb[topicManager->count].topic.str));

    snprintf(topicManager->topicCb[topicManager->count].topic.method,
             sizeof(topicManager->topicCb[topicManager->count].topic.method), "%s", method);
    snprintf(topicManager->topicCb[topicManager->count].topic.str,
             sizeof(topicManager->topicCb[topicManager->count].topic.str), "%s/%s", topicHead, method);

    topicManager->count++;
  }

  return 0;
}

int Common_proxy_pub_free_topic(COMMON_PUB_CONF_S *pub_conf) {
  if (pub_conf->topicManager) {
    if (pub_conf->topicManager->topicCb) {
      free(pub_conf->topicManager->topicCb);
      pub_conf->topicManager->topicCb = NULL;
    }

    free(pub_conf->topicManager);
  }

  return 0;
}

int Common_proxy_pub_get_topic_index(void *arg, char *topicHead, char *method) {
  COMMON_PUB_CONF_S *pubConf = (COMMON_PUB_CONF_S *)arg;
  int i = 0;
  int topicIndex = -1;

  for (i = 0; i < pubConf->topicManager->count; i++) {
    // printf("pubConf->topicManager->topicCb[%d].topic.str %s - %s -%s\n", i,
    // pubConf->topicManager->topicCb[i].topic.str, topicHead, method); printf("topic %s - %s\n",
    // pubConf->topicManager->topicCb[i].topic.method, pubConf->topicManager->topicCb[i].topic.str); printf("topicHead
    // %s
    // - %s\n", topicHead, method);

    if (strcmp(method, pubConf->topicManager->topicCb[i].topic.method) == 0 &&
        strstr(pubConf->topicManager->topicCb[i].topic.str, topicHead) != NULL) {
      topicIndex = i;
      break;
    }
  }

  return topicIndex;
}

int Common_proxy_pub_msg(nng_socket sid, char *sendMsg, int msgLen) {
  int rv = 0;

  // printf("++++++ sendMsg =%s", sendMsg);
  if ((rv = nng_send(sid, sendMsg, msgLen, 0)) != 0) {
    Common_fatal(__FUNCTION__, rv);
    rv = -1;
  }

  return rv;
}

void Common_nng_sub_msgCb(void *arg) {
  int rv;

  COMMON_SUB_CONF_S *subConf = (COMMON_SUB_CONF_S *)arg;

  if (subConf->aioCb_exit) {
    return;
  }

  // 获取异步操作结果
  rv = nng_aio_result(subConf->subaio);
  if (rv != 0) {
    if (rv == NNG_ECANCELED) {
      return;
    } else {
      Common_fatal(__FUNCTION__, rv);
      return;
    }
  }

  nng_msg *m = NULL;
  // 获取接收到的数据
  m = nng_aio_get_msg(subConf->subaio);
  if (m == NULL) {
    Common_fatal(__FUNCTION__, rv);
  }

  // 处理接收到的数据
  char *recvMsg = nng_msg_body(m);
  int recvMsgLen = nng_msg_len(m);
  if (recvMsg) subConf->submsgCb(subConf, recvMsg, recvMsgLen);

  // 释放消息缓冲区
  if (m) {
    nng_msg_clear(m);
    nng_msg_free(m);
    m = NULL;
  }

  // 重新启动异步接收操作
  nng_recv_aio(subConf->subsock, subConf->subaio);

  if (subConf->g_subexit) {
    subConf->aioCb_exit = 1;
    // 完成异步操作
    nng_aio_finish(subConf->subaio, 0);
  }
  usleep(1000);
}

int Common_proxy_sub_add_topic(COMMON_SUB_CONF_S *sub_conf, char *topicHead, char *method, msgHandleCallback msgCb) {
  TopicManager *topicManager = sub_conf->topicManager;
  int rv = 0;

  if (topicManager) {
    if (topicManager->count >= topicManager->capacity) {
      // 如果回调函数数量达到了数组容量，需要增加数组大小
      int newCapacity = (topicManager->capacity == 0) ? 1 : topicManager->capacity * 2;
      topicCallbackManager *newtopicCallbacks =
          (topicCallbackManager *)realloc(topicManager->topicCb, newCapacity * sizeof(topicCallbackManager));
      if (newtopicCallbacks == NULL) {
        // 内存分配失败
        fprintf(stderr, "Failed to allocate memory for callbacks\n");
        return -1;
      }
      topicManager->topicCb = newtopicCallbacks;
      topicManager->capacity = newCapacity;
    }
    // 在回调函数指针数组的末尾添加新的回调函数

    topicManager->topicCb[topicManager->count].msgHandleCb = msgCb;

    memset(topicManager->topicCb[topicManager->count].topic.method, 0,
           sizeof(topicManager->topicCb[topicManager->count].topic.method));
    memset(topicManager->topicCb[topicManager->count].topic.str, 0,
           sizeof(topicManager->topicCb[topicManager->count].topic.str));

    snprintf(topicManager->topicCb[topicManager->count].topic.method,
             sizeof(topicManager->topicCb[topicManager->count].topic.method), "%s", method);
    snprintf(topicManager->topicCb[topicManager->count].topic.str,
             sizeof(topicManager->topicCb[topicManager->count].topic.str), "%s/%s", topicHead, method);

    if ((rv = nng_socket_set(sub_conf->subsock, NNG_OPT_SUB_SUBSCRIBE,
                             topicManager->topicCb[topicManager->count].topic.str,
                             strlen(topicManager->topicCb[topicManager->count].topic.str))) != 0) {
      Common_fatal(__FUNCTION__, rv);
    }

    topicManager->count++;
  }

  return 0;
}

int Common_proxy_sub_free_topic(COMMON_SUB_CONF_S *sub_conf) {
  if (sub_conf->topicManager) {
    if (sub_conf->topicManager->topicCb) {
      free(sub_conf->topicManager->topicCb);
      sub_conf->topicManager->topicCb = NULL;
    }

    free(sub_conf->topicManager);
  }

  return 0;
}

int Common_proxy_sub_aio_init(COMMON_SUB_CONF_S *sub_Conf) {
  int rv;

  // 初始化异步操作
  rv = nng_aio_alloc(&sub_Conf->subaio, Common_nng_sub_msgCb, sub_Conf);
  if (rv != 0) {
    Common_fatal(__FUNCTION__, rv);
    return -1;
  }

  // 设置接收超时
  nng_aio_set_timeout(sub_Conf->subaio, NNG_DURATION_INFINITE);

  nng_recv_aio(sub_Conf->subsock, sub_Conf->subaio);

  return 0;
}

int Common_proxy_sub_aio_deinit(COMMON_SUB_CONF_S *sub_Conf) {
  // 释放异步操作资源
  if (sub_Conf->subaio) {
    nng_aio_stop(sub_Conf->subaio);
    nng_aio_wait(sub_Conf->subaio);
    nng_aio_free(sub_Conf->subaio);
    sub_Conf->subaio = NULL;
  }

  return 0;
}

int Common_nng_pubsub_dial(void *arg, NNG_SOCKET_E nngSocketType) {
  int rv = 0;

  if (nngSocketType == NNG_SOCKET_PUB) {
    COMMON_PUB_CONF_S *pub_conf = (COMMON_PUB_CONF_S *)arg;
    printf("+++++++++++++++ puburl=%s\n", pub_conf->puburl);
    if ((rv = nng_dial(pub_conf->pubsock, pub_conf->puburl, NULL, 0)) != 0) {
      Common_fatal(__FUNCTION__, rv);
      rv = -1;
    }
  } else if (nngSocketType == NNG_SOCKET_SUB) {
    COMMON_SUB_CONF_S *sub_conf = (COMMON_SUB_CONF_S *)arg;
    printf("+++++++++++++++ suburl=%s\n", sub_conf->suburl);
    if ((rv = nng_dial(sub_conf->subsock, sub_conf->suburl, NULL, 0)) != 0) {
      Common_fatal(__FUNCTION__, rv);
      rv = -1;
    }
  }

  return rv;
}

int Common_nng_socket_open(nng_socket *sid, NNG_SOCKET_E nngSocketType) {
  int rv = 0;

  if (nngSocketType == NNG_SOCKET_PUB) {
    if ((rv = nng_pub0_open(sid)) != 0) {
      Common_fatal(__FUNCTION__, rv);
      rv = -1;
    }

  } else if (nngSocketType == NNG_SOCKET_SUB) {
    if ((rv = nng_sub0_open(sid)) != 0) {
      Common_fatal(__FUNCTION__, rv);
      rv = -1;
    }
  }

  return rv;
}

int Common_nng_socket_close(nng_socket sid) {
  int rv = 0;
  rv = nng_close(sid);
  if (rv) {
    Common_fatal(__FUNCTION__, rv);
    rv = -1;
  }

  return rv;
}
