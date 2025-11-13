#include "media_service_rb_reader.h"

#include <stdio.h>
#include <string.h>

#include "ringbuffer.h"

int media_service_rb_reader_register(char *devicename, int deviceid, int mediatype, void *arg, void **ppctx,
                                     long long int pretimes) {
  int ret = -1;

  printf("%s begin\n", __FUNCTION__);
  char serviceid[64] = {0};
  snprintf(serviceid, sizeof(serviceid), "%s_%d_%d", devicename, deviceid, mediatype);

  printf("%s pretimes:%lld\n", __FUNCTION__, pretimes);
  ret = ringbuffer_reader_init(ppctx, arg, serviceid, pretimes);
  if (ret == -1) {
    printf("[%s] ringbuffer_reader_init error\n", __FUNCTION__);
  }

  printf("%s  mediatype[%d], deviceid[%d], serviceid[%s], pctx[%p]\n", __FUNCTION__, mediatype, deviceid, serviceid,
         ppctx);

  return ret;
}

int media_service_rb_reader_move_to_time(void **ppctx, long long int pretimes) {
  int ret = -1;

  printf("%s begin\n", __FUNCTION__);

  ret = ringbuffer_reader_move_to_time(ppctx, pretimes);
  if (ret == -1) {
    printf("[%s] error\n", __FUNCTION__);
  }

  printf("%s pctx[%p]\n", __FUNCTION__, ppctx);

  return ret;
}

int media_service_rb_reader_oldest(void **ppctx) {
  int ret = -1;

  printf("%s begin\n", __FUNCTION__);

  ret = ringbuffer_reader_oldest(ppctx);
  if (ret == -1) {
    printf("[%s] error\n", __FUNCTION__);
  }

  printf("%s pctx[%p]\n", __FUNCTION__, ppctx);

  return ret;
}

int media_service_rb_reader_latest(void **ppctx) {
  int ret = -1;

  printf("%s begin\n", __FUNCTION__);

  ret = ringbuffer_reader_latest(ppctx);
  if (ret == -1) {
    printf("[%s] error\n", __FUNCTION__);
  }

  printf("%s pctx[%p]\n", __FUNCTION__, ppctx);

  return ret;
}

int media_service_rb_reader_discard_all_data(void **ppctx) {
  int ret = -1;

  printf("%s begin\n", __FUNCTION__);

  ret = ringbuffer_reader_discard_all_data(ppctx);
  if (ret == -1) {
    printf("[%s] ringbuffer_reader_discard_all_data error\n", __FUNCTION__);
  }

  printf("%s pctx[%p]\n", __FUNCTION__, ppctx);

  return ret;
}

int media_service_rb_reader_proc(void *pctx, int msgtype, void **data, int *datalen) {
  int ret = -1;

  uint32_t iskey = 0;

  ret = ringbuffer_reader_data_request(pctx, data, datalen, &iskey);
  if (ret == -1) {
    // printf("[%s] ringbuffer_reader_data_request error ret:%d\n", __FUNCTION__, ret);
  }

  if (data && (0 == ret)) {
    ringbuffer_reader_data_commit(pctx, *data, *datalen);
  } else {
    // BALLOG_E("%s data[%p], ret[%d]", __FUNCTION__, data, ret);
  }

  return ret;
}

int media_service_rb_reader_unregister(int deviceid, int mediatype, void *pctx) {
  int ret = -1;

  if (pctx) {
    ret = ringbuffer_reader_deinit(pctx);
  }

  return ret;
}