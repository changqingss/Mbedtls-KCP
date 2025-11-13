#include "task_queue.h"

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <time.h>
#include <unistd.h>

#include "../log/log_conf.h"

static long long get_tick_ms() {
  struct timespec ts;

  if (0 == clock_gettime(CLOCK_MONOTONIC, &ts)) {
    time_t sec = ts.tv_sec;
    unsigned long long msec = ts.tv_nsec / 1000000;
    return (sec * 1000 + msec);
  }

  return 0;
}

static void *Message_Thread(void *arg) {
  MessageQueue *q = (MessageQueue *)arg;
  MessageQueueApiCallBacks *apis = &q->apiCallbacks;
  MessageNode *node = NULL;
  MessageNode *next = NULL;
  MessageNode *prev = NULL;
  MessageNode read_node;
  int writeable = 0;
  int readable = 0;
  int timeout_ns = 1000 * 1000 * 1000;
  int one_time_ns = 1000 * 1000 * 1000;

  struct timeval now;
  struct timespec outtime;

  char func_name[256] = "";
  snprintf(func_name, sizeof(func_name), "%s", q->name);
  prctl(PR_SET_NAME, func_name);

  while (1) {
    writeable = 0;
    readable = 0;
    gettimeofday(&now, NULL);

    outtime.tv_sec = now.tv_sec + 2;
    outtime.tv_nsec = now.tv_usec * 1000;

    if (apis->checkCB) apis->checkCB(q->userdata);
    if (0 == q->iLoopThreadRun) {
      COMMONLOG_I("%s iLoopThreadRun is be 0", q->name);
      break;
    }

    pthread_mutex_lock(&q->mutex);
    node = q->head->next;
    if ((q->iNodeNum > 0) && (node != q->head)) {
      memcpy(&read_node, node, sizeof(MessageNode));

      readable = 1;
      prev = node->prev;
      next = node->next;

      prev->next = next;
      next->prev = prev;
      q->iNodeNum--;
      free(node);
    } else if (q->iEventCnt > 0) {
      q->iEventCnt--;
      writeable = 1;
    } else {
      pthread_cond_timedwait(&q->cond, &q->mutex, &outtime);
    }

    pthread_mutex_unlock(&q->mutex);

    if (readable && apis->readCb) {
      apis->readCb(&read_node);
    }

    if (writeable && apis->writeCB) {
      apis->writeCB(q->userdata);
    }
  }

  if (apis->stopCB) apis->stopCB(q->userdata);

  COMMONLOG_I("%s Message_Thread quit success", q->name);

  q->isQuitThread = 1;

  return NULL;
}

int32_t Message_PutMsg(MessageQueue *q, MessageNode *node) {
  PARAM_CHECK(q && node, -1);
  PARAM_CHECK(q->init, -1);
  PARAM_CHECK_STRING(q->init, -1, "%s is not init", q->name);

  if (0 == q->iLoopThreadRun) return 0;

  MessageNode *tail = NULL;
  MessageNode *newNode = (MessageNode *)malloc(sizeof(MessageNode));
  if (NULL == newNode) {
    COMMONLOG_E("malloc MessageNode failed");
    return -1;
  }

  memset(newNode, 0, sizeof(MessageNode));

  pthread_mutex_lock(&q->mutex);

  memcpy(newNode, node, sizeof(MessageNode));
  tail = q->head->prev;

  newNode->prev = tail;
  newNode->next = q->head;
  tail->next = newNode;
  q->head->prev = newNode;
  q->iNodeNum++;

  pthread_cond_signal(&q->cond);
  pthread_mutex_unlock(&q->mutex);

  return 0;
}

int32_t Message_GetEventCount(MessageQueue *q) {
  PARAM_CHECK(q, -1);
  int32_t event_cnt = 0;
  pthread_mutex_lock(&q->mutex);
  event_cnt = q->iNodeNum;
  pthread_mutex_unlock(&q->mutex);
  return event_cnt;
}

int32_t Message_PutEvent(MessageQueue *q) {
  PARAM_CHECK(q, -1);

  if (0 == q->iLoopThreadRun) return 0;

  pthread_mutex_lock(&q->mutex);
  q->iEventCnt++;
  pthread_cond_signal(&q->cond);
  pthread_mutex_unlock(&q->mutex);

  return 0;
}

int32_t Message_Init(MessageQueue *q, PMessageQueueApiCallBacks pApiCallbacks) {
  PARAM_CHECK(q && pApiCallbacks && pApiCallbacks->name && pApiCallbacks->name[0], -1);

  pthread_attr_t attr;
  struct sched_param param;
  int iAllocSize = sizeof(MessageNode);
  char *mem = (char *)malloc(iAllocSize);
  if (NULL == mem) {
    COMMONLOG_E("malloc MessageQueue failed");
    return -1;
  }

  memset(mem, 0, iAllocSize);

  q->head = (MessageNode *)mem;
  q->head->next = q->head->prev = q->head;

  q->userdata = pApiCallbacks->userData;
  q->iEventCnt = 0;
  q->iNodeNum = 0;
  q->iLoopThreadRun = 1;
  q->isQuitThread = 0;
  q->detach = 0;
  q->init = 1;

  strcpy(q->name, pApiCallbacks->name);
  q->apiCallbacks = *pApiCallbacks;
  prctl(PR_SET_NAME, q->name);

  if (pthread_mutex_init(&q->mutex, NULL)) {
    COMMONLOG_E("pthread_mutex_init failed errno:%d", errno);
    goto exit;
  }

  if (pthread_cond_init(&q->cond, NULL)) {
    COMMONLOG_E("pthread_cond_init failed errno:%d", errno);
    goto exit;
  }

  if (pApiCallbacks->async) {
    // 初始化线程属性
    pthread_attr_init(&attr);

#if USE_THREAD_PRIORTY_HIGH
    // 设置调度策略为SCHED_FIFO
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

    // 获取调度参数并设置优先级
    pthread_attr_getschedparam(&attr, &param);
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);

    // 设置线程属性的调度参数
    pthread_attr_setschedparam(&attr, &param);
#endif

    if (q->detach) {
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    } else {
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    }

    if (pthread_create(&q->lLoopThreadId, &attr, Message_Thread, q)) {
      COMMONLOG_E("pthread_create failed errno:%d", errno);
      pthread_attr_destroy(&attr);
      goto exit;
    }

    pthread_attr_destroy(&attr);
  } else {
    Message_Thread(q);
  }

  COMMONLOG_I("name:%s create MessageQueue Success", q->name);

  return 0;

exit:
  return -1;
}

int32_t Message_StopAsync(MessageQueue *q) {
  PARAM_CHECK(q, -1);

  pthread_mutex_lock(&q->mutex);
  q->iLoopThreadRun = 0;
  pthread_mutex_unlock(&q->mutex);

  return 0;
}

static int32_t Message_Destroy(MessageQueue *q) {
  PARAM_CHECK(q, -1);

  MessageNode *head = q->head;
  MessageNode *node = q->head->next;
  MessageNode *next = NULL;

  while (node != head) {
    next = node->next;
    free(node);
    node = next;
  }

  if (head) free(head);
  pthread_mutex_destroy(&q->mutex);
  pthread_cond_destroy(&q->cond);

  return 0;
}

int32_t Message_Deinit(MessageQueue *q) {
  PARAM_CHECK(q && q->init, -1);

  const int64_t time_out_ms = 10000;
  const int64_t cur_time_ms = get_tick_ms();

  Message_StopAsync(q);
  if (q->lLoopThreadId) {
    if (!q->detach) {
      pthread_join(q->lLoopThreadId, NULL);
    } else {
      while (!q->isQuitThread && get_tick_ms() - cur_time_ms < time_out_ms) {
        usleep(1000 * 100);
      }
    }

    q->lLoopThreadId = 0;
  }

  Message_Destroy(q);
  q->init = 0;
  return 0;
}