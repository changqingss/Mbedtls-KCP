#ifndef __MSG_QUEUE_H__
#define __MSG_QUEUE_H__

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MSG_DATA_SIZE (16 * 1024)

// 定义消息数据结构
typedef struct msg_data {
  int32_t type;                  // 数据类型
  int32_t length;                // 数据长度
  long long timestamp_ms;        // 放入队列的时间戳（毫秒）
  char data[MAX_MSG_DATA_SIZE];  // 数据指针
} MsgData;

// 定义队列节点结构体
typedef struct msg_queue_node {
  MsgData msg_data;             // 节点数据
  struct msg_queue_node *next;  // 指向下一个节点
} MsgQueueNode, *PMsgQueueNode;

// 定义消息队列结构体
typedef struct msg_queue {
  int32_t init;            // 初始化标志（0表示未初始化，1表示已初始化）
  int32_t node_count;      // 当前节点数量
  int32_t max_node_count;  // 最大节点数量（0表示无限制）
  MsgQueueNode *head;      // 队列头节点
  MsgQueueNode *tail;      // 队列尾节点
  pthread_mutex_t mutex;   // 互斥锁
} MsgQueue, *PMsgQueue;

// 初始化消息队列
PMsgQueue msg_queue_init(PMsgQueue queue, int initQueueNum);

// 向消息队列添加节点
int32_t msg_queue_put(PMsgQueue queue, PMsgQueueNode node);

// 从消息队列获取节点（非阻塞）
PMsgQueueNode msg_queue_get(PMsgQueue queue);

// 释放消息队列
int32_t msg_queue_deinit(PMsgQueue queue);

int32_t msg_queue_is_empty(PMsgQueue queue);
#endif  // __MSG_QUEUE_H__
