/*

Copyright (c) 2005-2008, Simon Howard

Permission to use, copy, modify, and/or distribute this software
for any purpose with or without fee is hereby granted, provided
that the above copyright notice and this permission notice appear
in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

 */

#include "comm_queue.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

/* malloc() / free() testing */

#ifdef ALLOC_TESTING
#include "alloc-testing.h"
#endif

/* A double-ended queue */

typedef struct _QueueEntry QueueEntry;

struct _QueueEntry {
  QueueValue data;
  int dataSize;
  QueueEntry *prev;
  QueueEntry *next;
};

struct _Queue {
  QueueEntry *head;
  QueueEntry *tail;
  pthread_mutex_t queMutex;
};

Queue *COMM_queue_new(void) {
  Queue *queue;

  queue = (Queue *)malloc(sizeof(Queue));

  if (queue == NULL) {
    return NULL;
  }

  queue->head = NULL;
  queue->tail = NULL;

  pthread_mutex_init(&queue->queMutex, NULL);

  return queue;
}

void COMM_queue_free(Queue *queue) {
  QueueValue result = NULL;
  int resultSize = 0;
  /* Empty the queue */
  while (!COMM_queue_is_empty(queue)) {
    COMM_queue_pop_head(queue, &result, &resultSize);
    if (result) {
      free(result);
    }
  }

  /* Free back the queue */
  pthread_mutex_destroy(&queue->queMutex);

  free(queue);
}

int COMM_queue_push_head(Queue *queue, QueueValue data, int dataSize) {
  QueueEntry *new_entry;

  /* Create the new entry and fill in the fields in the structure */

  new_entry = malloc(sizeof(QueueEntry));
  if (new_entry == NULL) {
    return 0;
  }

  new_entry->data = malloc(dataSize);
  if (new_entry->data == NULL) {
    free(new_entry);
    return 0;
  }

  memset(new_entry->data, 0, dataSize);
  memcpy(new_entry->data, data, dataSize);
  new_entry->dataSize = dataSize;

  new_entry->prev = NULL;

  pthread_mutex_lock(&queue->queMutex);
  new_entry->next = queue->head;

  /* Insert into the queue */

  if (queue->head == NULL) {
    /* If the queue was previously empty, both the head and
     * tail must be pointed at the new entry */

    queue->head = new_entry;
    queue->tail = new_entry;

  } else {
    /* First entry in the list must have prev pointed back to this
     * new entry */

    queue->head->prev = new_entry;

    /* Only the head must be pointed at the new entry */

    queue->head = new_entry;
  }

  pthread_mutex_unlock(&queue->queMutex);

  return dataSize;
}

int COMM_queue_pop_head(Queue *queue, QueueValue *result, int *resultSize) {
  pthread_mutex_lock(&queue->queMutex);
  // 在加锁后直接检查队列是否为空
  if (queue->head == NULL) {
    pthread_mutex_unlock(&queue->queMutex);
    return 0;
  }

  QueueEntry *entry = queue->head;
  *result = malloc(entry->dataSize);
  if (*result == NULL) {
    pthread_mutex_unlock(&queue->queMutex);
    return 0;
  }

  memset(*result, 0, entry->dataSize);
  memcpy(*result, entry->data, entry->dataSize);
  *resultSize = entry->dataSize;

  queue->head = entry->next;
  if (queue->head == NULL) {
    queue->tail = NULL;
  } else {
    queue->head->prev = NULL;
  }

  free(entry->data);
  free(entry);

  pthread_mutex_unlock(&queue->queMutex);
  return (*resultSize);
}

int COMM_queue_push_tail(Queue *queue, QueueValue data, int dataSize) {
  QueueEntry *new_entry;

  new_entry = malloc(sizeof(QueueEntry));
  if (new_entry == NULL) {
    return 0;
  }

  new_entry->data = malloc(dataSize);
  if (new_entry->data == NULL) {
    free(new_entry);
    return 0;
  }

  pthread_mutex_lock(&queue->queMutex);
  memset(new_entry->data, 0, dataSize);
  memcpy(new_entry->data, data, dataSize);
  new_entry->dataSize = dataSize;

  new_entry->prev = queue->tail;
  new_entry->next = NULL;

  /* Insert into the queue tail */

  if (queue->tail == NULL) {
    /* If the queue was previously empty, both the head and
     * tail must be pointed at the new entry */

    queue->head = new_entry;
    queue->tail = new_entry;

  } else {
    /* The current entry at the tail must have next pointed to this
     * new entry */

    queue->tail->next = new_entry;

    /* Only the tail must be pointed at the new entry */

    queue->tail = new_entry;
  }

  pthread_mutex_unlock(&queue->queMutex);

  return dataSize;
}

int COMM_queue_pop_tail(Queue *queue, QueueValue *result, int *resultSize) {
  pthread_mutex_lock(&queue->queMutex);

  if (queue->tail == NULL) {
    // 空队列
    pthread_mutex_unlock(&queue->queMutex);
    return 0;
  }

  QueueEntry *entry = queue->tail;

  // 先分配result空间，不修改队列结构
  *result = malloc(entry->dataSize);
  if (*result == NULL) {
    // 分配失败，不修改队列指针，直接解锁返回
    pthread_mutex_unlock(&queue->queMutex);
    return 0;
  }

  memset(*result, 0, entry->dataSize);
  memcpy(*result, entry->data, entry->dataSize);
  *resultSize = entry->dataSize;

  // 分配成功后再修改队列结构
  queue->tail = entry->prev;
  if (queue->tail == NULL) {
    queue->head = NULL;
  } else {
    queue->tail->next = NULL;
  }

  if (entry->data) free(entry->data);
  free(entry);

  pthread_mutex_unlock(&queue->queMutex);
  return (*resultSize);
}

int COMM_queue_is_empty(Queue *queue) {
  pthread_mutex_lock(&queue->queMutex);
  int val = (queue->head == NULL);
  pthread_mutex_unlock(&queue->queMutex);
  return val;
}

/**
 * @brief 计算队列中元素的数量
 * @param queue 队列指针
 * @return 队列中元素的数量，如果队列为空或无效返回0
 */
int COMM_queue_size(Queue *queue) {
  int count = 0;
  QueueEntry *entry;

  if (queue == NULL) {
    return 0;
  }

  pthread_mutex_lock(&queue->queMutex);

  // 从头到尾遍历队列计数
  entry = queue->head;
  while (entry != NULL) {
    count++;
    entry = entry->next;
  }

  pthread_mutex_unlock(&queue->queMutex);

  return count;
}
