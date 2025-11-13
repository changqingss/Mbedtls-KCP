#include "element_queue.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

queue_context_t *queue_init(int num, int elementSize) {
  queue_context_t *q = NULL;
  queue_node_t *node = NULL;
  int iAllocNum = num + 1;
  int iAllocSize = sizeof(queue_context_t) + sizeof(queue_node_t) * iAllocNum + iAllocNum * elementSize;

  char *mem = NULL;
  char *buf = NULL;
  int i = 0;

  mem = (char *)malloc(iAllocSize);
  if (NULL == mem) {
    printf("malloc mem failed\n");
    return NULL;
  }

  memset(mem, 0, iAllocSize);

  q = (queue_context_t *)mem;
  q->numofNode = num;
  q->freeNum = num;
  q->elementSize = elementSize;
  q->usedNum = 0;
  q->readIndex = 0;
  q->writeIndex = 0;
  // 节点的开始地址
  q->node = (queue_node_t *)(q + 1);
  // 数据开始地址
  buf = (char *)(q->node + iAllocNum);

  for (i = 0; i < iAllocNum; i++) {
    node = q->node + i;
    node->buf = buf + elementSize * i;
    node->max = elementSize;
  }

  q->consumeNode = q->node + num;

  if (pthread_mutex_init(&q->mutex, NULL) < 0) {
    printf("pthread_mutex_init failed\n");
    goto FAILED;
  }

  printf("create queue success\n");
  return q;

FAILED:
  queue_deinit(q);
  return NULL;
}

int queue_put(queue_context_t *q, char *buf, int size) {
  if (NULL == q || NULL == buf) {
    printf("param is invalid\n");
    return -1;
  }

  if (size > q->elementSize) {
    printf("size:%d is big than elementSize:%d\n", size, q->elementSize);
    return -1;
  }

  queue_node_t *node = NULL;
  pthread_mutex_lock(&q->mutex);

  if (q->freeNum > 0) {
    q->freeNum--;
    q->usedNum++;
    node = q->node + q->writeIndex;
    node->size = (node->max > size ? size : node->max);
    memcpy(node->buf, buf, node->size);
    q->writeIndex = (q->writeIndex + 1) % q->numofNode;
  }

  pthread_mutex_unlock(&q->mutex);
  return node ? PUT_QUEUE_SUCCESS : PUT_QUEUE_FAILED;
}

int queue_get(queue_context_t *q) {
  if (NULL == q) {
    printf("param is invalid\n");
    return -1;
  }

  int get = 0;
  queue_node_t *node = NULL;

  pthread_mutex_lock(&q->mutex);

  if (q->usedNum > 0) {
    get = 1;
    q->usedNum--;
    q->freeNum++;
    node = q->node + q->readIndex;
    q->consumeNode->size = node->size;
    memcpy(q->consumeNode->buf, node->buf, node->size);
    q->readIndex = (q->readIndex + 1) % q->numofNode;
  }

  pthread_mutex_unlock(&q->mutex);

  return get;
}

int queue_deinit(queue_context_t *q) {
  if (NULL == q) {
    printf("param is invalid\n");
    return -1;
  }

  pthread_mutex_destroy(&q->mutex);
  free(q);

  return 0;
}