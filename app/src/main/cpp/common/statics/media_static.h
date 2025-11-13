#ifndef __MEDIA_STATIC_H__
#define __MEDIA_STATIC_H__

#include <pthread.h>
#include <stdint.h>

typedef struct stream_fps_t {
  int32_t iFirst;
  int32_t iStartTimeRec;
  int32_t iFramesCnt;
  int32_t iFrameTotalBytes;
  int32_t iLastFramesCnt;
  int32_t iLastFrameTotalBytes;
  int32_t iFPS;
  int32_t iBitRate;
  int64_t start_ms;
  int64_t end_ms;
  pthread_mutex_t mutex;

} stream_fps_t;

int32_t media_fps_init(stream_fps_t *fps);
int32_t media_fps_calc(stream_fps_t *fps, int32_t *pfps, int32_t *bitrate);
int32_t media_fps_add(stream_fps_t *fps, int32_t framesize);
int32_t media_fps_deinit(stream_fps_t *fps);

#endif