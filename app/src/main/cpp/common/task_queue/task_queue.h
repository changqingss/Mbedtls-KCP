

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <pthread.h>
#include <stdint.h>
#include <unistd.h>

typedef void *(*MessageQueueCallBack)(void *);

typedef struct message_queue_api_callbacks {
  char *name;  // 队列名称
  void *userData;
  int async;
  MessageQueueCallBack readCb;
  MessageQueueCallBack writeCB;
  MessageQueueCallBack checkCB;
  MessageQueueCallBack stopCB;
} MessageQueueApiCallBacks, *PMessageQueueApiCallBacks;

typedef struct message_node {
  int64_t command;            // 命令字
  int64_t minor;              // 辅助命令字
  void *userdata;             // 私有数据
  void *rtc_session;          // rtc session
  void *sctpData;             // sctp数据
  int length;                 // sctp数据长度
  void *queue;                // 队列
  struct message_node *next;  // next域
  struct message_node *prev;  // prev域
  char peer_id[256];
} MessageNode, *PMessageNode;

typedef struct message_queue {
  int32_t init;
  int32_t detach;
  int32_t isQuitThread;
  int32_t iNodeNum;
  int32_t iLoopThreadRun;
  MessageNode *head;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  pthread_t lLoopThreadId;
  MessageQueueApiCallBacks apiCallbacks;
  int32_t iEventCnt;
  void *userdata;
  char name[256];
} MessageQueue, *PMessageQueue;

#define MESSAGE_HEAD_SIZE (sizeof(MessageNode))

int32_t Message_Init(MessageQueue *q, PMessageQueueApiCallBacks pApiCallbacks);
int32_t Message_PutMsg(MessageQueue *q, MessageNode *node);
int32_t Message_PutEvent(MessageQueue *q);
int32_t Message_StopAsync(MessageQueue *q);
int32_t Message_GetEventCount(MessageQueue *q);
int32_t Message_Deinit(MessageQueue *q);

#endif
