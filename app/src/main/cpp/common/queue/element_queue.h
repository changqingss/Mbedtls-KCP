
#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <pthread.h>
#include <stdint.h>

#define PUT_QUEUE_SUCCESS (1)
#define PUT_QUEUE_FAILED (0)

typedef struct queue_node_t {
  char* buf;
  int size;
  int max;
} queue_node_t;

typedef struct queue_context_t {
  queue_node_t* node;
  queue_node_t* consumeNode;
  int numofNode;
  int freeNum;
  int usedNum;
  int readIndex;
  int writeIndex;
  int elementSize;
  pthread_mutex_t mutex;
} queue_context_t;

/**
 * 仅仅支持单个消费者，不支持多消费者模型
 **/
queue_context_t* queue_init(int num, int elementSize);
int queue_get(queue_context_t* q);
int queue_put(queue_context_t* q, char* buf, int size);
int queue_deinit(queue_context_t* q);

#endif