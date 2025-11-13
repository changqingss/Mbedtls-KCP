#include "comm_queue_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>

#include "comm_queue.h"

#define MAX_QUEUE_MSG_NUM (50)  // 最大消息数量

void COMM_API_queue_free(COMM_QUE_S *g_que) {
  if (g_que == NULL || g_que->que == NULL) {
    return;
  }
  pthread_mutex_lock(&g_que->queMutex);
  COMM_queue_free(g_que->que);
  g_que->que = NULL;
  pthread_mutex_unlock(&g_que->queMutex);
}

int COMM_API_queue_is_empty(COMM_QUE_S *g_que) {
  int isEmpty = 0;
  if (g_que == NULL || g_que->que == NULL) {
    return -1;
  }
  pthread_mutex_lock(&g_que->queMutex);
  isEmpty = COMM_queue_is_empty(g_que->que);
  pthread_mutex_unlock(&g_que->queMutex);

  return isEmpty;
}

int COMM_API_queue_push_tail(COMM_QUE_S *g_que) {
  int ret = 0;

  if (g_que == NULL || g_que->que == NULL) {
    return -1;
  }

  pthread_mutex_lock(&g_que->queMutex);
  if (COMM_queue_push_tail(g_que->que, (QueueValue)&g_que->setdata, sizeof(COMM_QUE_DATA_S))) {
    g_que->current_length++;
    ret = 0;
  } else {
    pthread_mutex_unlock(&g_que->queMutex);
    return -1;
  }

  pthread_mutex_unlock(&g_que->queMutex);

  return ret;
}

int COMM_API_queue_push_head(COMM_QUE_S *g_que) {
  int ret = 0;

  if (g_que == NULL) {
    printf("g_que is null\n");
    return -1;
  }

  if (g_que->que == NULL) {
    printf("g_que->que is null\n");
    return -1;
  }

  pthread_mutex_lock(&g_que->queMutex);

  if (g_que->current_length >= MAX_QUEUE_MSG_NUM) {
    printf("COMM_queue_push_head is full, current_length:%d\n", g_que->current_length);
    pthread_mutex_unlock(&g_que->queMutex);
    return -1;
  }

  if (COMM_queue_push_head(g_que->que, (QueueValue)&g_que->setdata, sizeof(COMM_QUE_DATA_S))) {
    g_que->current_length++;
    ret = 0;
  } else {
    pthread_mutex_unlock(&g_que->queMutex);
    printf("COMM_queue_push_head error\n");
    return -1;
  }

  pthread_mutex_unlock(&g_que->queMutex);

  return ret;
}

int COMM_API_queue_pop_tail(COMM_QUE_S *g_que) {
  int nRet = -1;
  int resultSize = 0;
  char *result = NULL;
  char strcmd[256] = {0};

  if (g_que == NULL || g_que->que == NULL) {
    return -1;
  }

  pthread_mutex_lock(&g_que->queMutex);
  nRet = COMM_queue_pop_tail(g_que->que, (QueueValue *)&result, &resultSize);
  if (nRet) {
    if (result) {
      g_que->current_length--;
      memset(&g_que->getdata, 0, sizeof(COMM_QUE_DATA_S));
      memcpy(&g_que->getdata, result, sizeof(COMM_QUE_DATA_S));
      free(result);
    }
    pthread_mutex_unlock(&g_que->queMutex);
    return 0;
  }

  pthread_mutex_unlock(&g_que->queMutex);

  return -1;
}

int COMM_API_queue_pop_head(COMM_QUE_S *g_que) {
  int nRet = -1;
  int resultSize = 0;
  char *result = NULL;
  char strcmd[256] = {0};

  if (g_que == NULL || g_que->que == NULL) {
    return -1;
  }

  pthread_mutex_lock(&g_que->queMutex);
  nRet = COMM_queue_pop_head(g_que->que, (QueueValue *)&result, &resultSize);
  if (nRet) {
    if (result) {
      g_que->current_length--;
      memset(&g_que->getdata, 0, sizeof(COMM_QUE_DATA_S));
      memcpy(&g_que->getdata, result, sizeof(COMM_QUE_DATA_S));
      free(result);
    }
    pthread_mutex_unlock(&g_que->queMutex);
    return 0;
  }

  pthread_mutex_unlock(&g_que->queMutex);

  return -1;
}

int COMM_API_queue_init(COMM_QUE_S *g_que) {
  g_que->que = COMM_queue_new();
  if (g_que->que == NULL) {
    printf("COMM_queue_new failed\n");
    return -1;
  }

  g_que->current_length = 0;
  pthread_mutex_init(&g_que->queMutex, NULL);
  return 0;
}
