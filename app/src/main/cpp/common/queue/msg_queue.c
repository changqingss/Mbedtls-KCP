#include "msg_queue.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 初始化消息队列
PMsgQueue msg_queue_init(PMsgQueue queue, int initQueueNum) {
  if (!queue) return NULL;
  if (queue && queue->init) return queue;
  memset(queue, 0, sizeof(MsgQueue));
  queue->init = 1;
  queue->head = NULL;
  queue->tail = NULL;
  queue->node_count = 0;
  queue->max_node_count = initQueueNum;

  pthread_mutex_init(&queue->mutex, NULL);
  return queue;
}

// 向消息队列添加节点
int32_t msg_queue_put(PMsgQueue queue, PMsgQueueNode node) {
  if (!queue || !node) return -1;
  if (!queue->init) return -1;

  pthread_mutex_lock(&queue->mutex);

  if (queue->max_node_count > 0 && queue->node_count >= queue->max_node_count) {
    pthread_mutex_unlock(&queue->mutex);
    return -1;
  }

  node->next = NULL;
  if (queue->tail) {
    queue->tail->next = node;
    queue->tail = node;
  } else {
    queue->head = queue->tail = node;
  }
  queue->node_count++;
  pthread_mutex_unlock(&queue->mutex);
  return 0;
}

// 从消息队列获取节点（非阻塞）
PMsgQueueNode msg_queue_get(PMsgQueue queue) {
  if (!queue) return NULL;
  if (!queue->init) return NULL;
  if (queue->node_count <= 0) return NULL;

  pthread_mutex_lock(&queue->mutex);

  PMsgQueueNode node = NULL;
  if (queue->node_count > 0) {
    node = queue->head;

    if (queue->head) {
      queue->head = queue->head->next;
      if (!queue->head) {
        queue->tail = NULL;
      }
      queue->node_count--;
    }
  }

  pthread_mutex_unlock(&queue->mutex);

  return node;
}

// 判断队列为空
int32_t msg_queue_is_empty(PMsgQueue queue) {
  if (!queue) return 1;
  if (!queue->init) return 1;
  int nodeCnt = 0;
  pthread_mutex_lock(&queue->mutex);
  nodeCnt = queue->node_count;
  pthread_mutex_unlock(&queue->mutex);
  return nodeCnt == 0 ? 1 : 0;
}

// 释放消息队列
int32_t msg_queue_deinit(PMsgQueue queue) {
  if (!queue) return -1;
  if (!queue->init) return -1;

  pthread_mutex_lock(&queue->mutex);

  // 释放所有节点
  PMsgQueueNode node = queue->head;
  while (node) {
    PMsgQueueNode next = node->next;
    free(node);
    node = next;
  }
  queue->head = queue->tail = NULL;
  queue->node_count = 0;

  pthread_mutex_unlock(&queue->mutex);
  pthread_mutex_destroy(&queue->mutex);

  return 0;
}
