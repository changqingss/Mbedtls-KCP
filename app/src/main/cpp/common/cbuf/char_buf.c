
#include "char_buf.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/log/log_conf.h"

// 1、CBUF的初始化
PCharBuffer char_buffer_init(int max_size) {
  int ret = 0;
  PCharBuffer buf = NULL;
  buf = (PCharBuffer)malloc(sizeof(CharBuffer));
  PARAM_CHECK_STRING(buf, NULL, "malloc CharBuffer failed");

  memset(buf, 0, sizeof(CharBuffer));
  buf->max_size = max_size;
  buf->cur_size = 0;
  buf->buffer = (unsigned char *)malloc(max_size);
  PARAM_EXIT1(buf->buffer, ret, "malloc buffer failed");

  memset(buf->buffer, 0, max_size);
  ret = pthread_mutex_init(&buf->mutex, NULL);
  PARAM_EXIT1(ret == 0, ret, "pthread_mutex_init failed");

  buf->init = 1;
  return buf;

exit:
  char_buffer_deinit(buf);
  return NULL;
}

// 2、CBUF的反初始化
int char_buffer_deinit(PCharBuffer buf) {
  PARAM_CHECK(buf, -1);

  buf->init = 0;
  pthread_mutex_destroy(&buf->mutex);
  if (buf->buffer) {
    free(buf->buffer);
    buf->buffer = NULL;
  }
  free(buf);
  return 0;
}

// 3、CBUF的写入
int char_buffer_write(PCharBuffer buf, unsigned char *buffer, int write_size) {
  PARAM_CHECK(buf && buffer, -1);
  PARAM_CHECK_STRING(buf->init, -1, "char buffer is not init");
  int new_max_size = 0;
  char *new_buffer = NULL;

  pthread_mutex_lock(&buf->mutex);

  // 1、检查是否需要扩内存，不够需要扩容
  if (buf->max_size - buf->cur_size < write_size) {
    new_max_size = buf->cur_size + write_size;
    new_buffer = realloc(buf->buffer, new_max_size);
    if (new_buffer == NULL) {
      COMMONLOG_E("realloc failed");
      write_size = buf->max_size - buf->cur_size;
    } else {
      buf->buffer = new_buffer;
      buf->max_size = new_max_size;
    }
  }

  // 2、写入数据
  memcpy(buf->buffer + buf->cur_size, buffer, write_size);
  buf->cur_size += write_size;

  pthread_mutex_unlock(&buf->mutex);

  return 0;
}

// 4、CBUF的读取
int char_buffer_read(PCharBuffer buf, unsigned char *buffer, int max_read_size) {
  PARAM_CHECK(buf, -1);
  PARAM_CHECK_STRING(buf->init, -1, "char buffer is not init");

  pthread_mutex_lock(&buf->mutex);

  int read_size = buf->cur_size < max_read_size ? buf->cur_size : max_read_size;
  if (buffer) memcpy(buffer, buf->buffer, read_size);
  buf->cur_size -= read_size;
  memmove(buf->buffer, buf->buffer + read_size, buf->cur_size);  // 移动剩余数据到开头

  pthread_mutex_unlock(&buf->mutex);

  return read_size;
}

// 5、获取可写入的剩余空间大小
int char_buffer_get_avail_write_size(PCharBuffer buf) {
  PARAM_CHECK(buf, -1);
  PARAM_CHECK_STRING(buf->init, -1, "char buffer is not init");

  pthread_mutex_lock(&buf->mutex);
  int avail_size = buf->max_size - buf->cur_size;
  pthread_mutex_unlock(&buf->mutex);

  return avail_size;
}

// 6、获取可读取的数据大小
int char_buffer_get_avail_read_size(PCharBuffer buf) {
  PARAM_CHECK(buf, -1);
  PARAM_CHECK_STRING(buf->init, -1, "char buffer is not init");

  pthread_mutex_lock(&buf->mutex);
  int avail_size = buf->cur_size;
  pthread_mutex_unlock(&buf->mutex);

  return avail_size;
}

// 7、CharBuffer的清理
int char_buffer_clear(PCharBuffer buf) {
  PARAM_CHECK(buf, -1);
  PARAM_CHECK_STRING(buf->init, -1, "char buffer is not init");

  memset(buf->buffer, 0, buf->max_size);
  buf->cur_size = 0;
  return 0;
}

// 在缓冲区中搜索目标字符串，找到后跳转到目标字符串之后的位置
// 如果找到目标字符串，移除目标字符串及其之前的所有数据，返回0
// 如果未找到目标字符串，返回-1
int char_buffer_seek(PCharBuffer buf, unsigned char *target, int target_size) {
  PARAM_CHECK(buf, -1);
  PARAM_CHECK_STRING(buf->init, -1, "char buffer is not init");
  PARAM_CHECK(target && target_size > 0, -1);

  pthread_mutex_lock(&buf->mutex);

  // 如果缓冲区数据不足以包含目标字符串，直接返回失败
  if (buf->cur_size < target_size) {
    pthread_mutex_unlock(&buf->mutex);
    return -1;
  }

  // 在缓冲区中搜索目标字符串
  int found_pos = -1;
  for (int i = 0; i <= buf->cur_size - target_size; i++) {
    if (memcmp(buf->buffer + i, target, target_size) == 0) {
      found_pos = i;
      break;
    }
  }

  // 如果没有找到目标字符串
  if (found_pos == -1) {
    pthread_mutex_unlock(&buf->mutex);
    return -1;
  }

  // 找到目标字符串，计算跳转到目标字符串之后需要读取的字节数
  int bytes_to_skip = found_pos + target_size;  // 跳过目标字符串之前的数据 + 目标字符串本身

  // 使用类似char_buffer_read的方式，移除目标字符串及其之前的所有数据
  buf->cur_size -= bytes_to_skip;
  memmove(buf->buffer, buf->buffer + bytes_to_skip, buf->cur_size);

  pthread_mutex_unlock(&buf->mutex);

  // 返回成功，现在缓冲区的开始位置是目标字符串之后的位置
  return 0;
}