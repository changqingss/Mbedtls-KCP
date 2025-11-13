#ifndef __SBUF_H__
#define __SBUF_H__

#include <pthread.h>

typedef struct char_buffer_t {
  int init;               // 是否初始化，0：否，1：是
  int max_size;           // 最大长度
  int cur_size;           // 当前长度
  unsigned char *buffer;  // 缓存指针
  pthread_mutex_t mutex;  // 互斥锁

} CharBuffer, *PCharBuffer;

PCharBuffer char_buffer_init(int max_size);
int char_buffer_write(PCharBuffer, unsigned char *, int);
int char_buffer_read(PCharBuffer, unsigned char *, int);
int char_buffer_deinit(PCharBuffer);
int char_buffer_get_avail_write_size(PCharBuffer);
int char_buffer_get_avail_read_size(PCharBuffer);
int char_buffer_clear(PCharBuffer);
int char_buffer_seek(PCharBuffer, unsigned char *, int);

#endif