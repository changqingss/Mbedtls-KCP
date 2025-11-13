#include "media_service_rb_writer.h"

#include <stdio.h>
#include <string.h>

#include "ringbuffer.h"

int media_service_rb_writer_register(char *devicename, int deviceid, int mediatype, void *arg, void **ppctx) {
  int ret = -1;

  char serviceid[64] = {0};
  snprintf(serviceid, sizeof(serviceid), "%s_%d_%d", devicename, deviceid, mediatype);

  ret = ringbuffer_writer_init(ppctx, arg, serviceid);
  if (ret == -1) {
    printf("[%s] ringbuffer_writer_init error\n", __FUNCTION__);
  }

  printf("%s  mediatype[%d], deviceid[%d], serviceid[%s], pctx[%p]", __FUNCTION__, mediatype, deviceid, serviceid,
         ppctx);

  return ret;
}

int media_service_rb_writer_unregister(int deviceid, int mediatype, void *pctx) {
  int ret = -1;

  ret = ringbuffer_writer_deinit(pctx);

  ret = 0;

  return ret;
}

int media_service_rb_writer_trigger(void *pctx, void *data, int datalen) {
  int ret = -1;

  if (pctx && data) {
    media_message_t *media = (media_message_t *)data;
    int headlen = sizeof(media_message_t);
    int bufferlen = headlen + media->framelen;
    unsigned char *buffer = NULL;
    int iskey = -1;

    if (media->meidaType == MSG_TYPE_VIDEO) {
      if (VIDEO_FRAME_I_FRAME == media->media_info.videoInfo.frametype)
        iskey = 0;
      else
        iskey = 1;
    }

    if (media->meidaType == MSG_TYPE_AUDIO) {
      iskey = 2;
    }

    ret = ringbuffer_writer_data_request(pctx, &buffer, bufferlen, iskey);
    if ((0 == ret) && buffer) {
      memcpy(buffer, (unsigned char *)media, headlen);
      memcpy(buffer + headlen, (const char *)media->framedata, media->framelen);
      if (media->meidaType == MSG_TYPE_VIDEO) {
        ret = ringbuffer_writer_data_commit(pctx, media->media_info.videoInfo.wTimestamp);
      }
      if (media->meidaType == MSG_TYPE_AUDIO) {
        ret = ringbuffer_writer_data_commit(pctx, 0);
      }
    }
  }

  return ret;
}