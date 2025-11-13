#include "media_static.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "common/api/comm_api.h"

#define FPS_INTERVAL_TIMEMS (1000)

#define FPS_LOCK(f) (pthread_mutex_lock(&f->mutex))
#define FPS_UNLOCK(f) (pthread_mutex_unlock(&f->mutex))

int32_t media_fps_init(stream_fps_t *fps) {
  memset(fps, 0, sizeof(*fps));
  fps->iFirst = 1;
  pthread_mutex_init(&fps->mutex, NULL);
  return 0;
}

int32_t media_fps_calc(stream_fps_t *fps, int32_t *pfps, int32_t *bitrate) {
  int32_t result = 0;
  int32_t add_enable = 0;
  int32_t calculate_enable = 0;
  int32_t max_interval_ms = FPS_INTERVAL_TIMEMS;
  int64_t real_interval_ms = 0;
  int64_t cur_time_ms = COMM_API_get_tick_ms();

  // FPS_LOCK(fps);

  if (0 == fps->iStartTimeRec) {
    fps->iStartTimeRec = 1;
    fps->start_ms = cur_time_ms;
    fps->iLastFramesCnt = fps->iFramesCnt;
    fps->iLastFrameTotalBytes = fps->iFrameTotalBytes;
  } else if (cur_time_ms - fps->start_ms >= max_interval_ms) {
    calculate_enable = 1;
  }

  if (calculate_enable > 0) {
    real_interval_ms = cur_time_ms - fps->start_ms;
    if (real_interval_ms) {
      fps->iFPS = ((fps->iFramesCnt - fps->iLastFramesCnt) * 1000) / real_interval_ms;
      fps->iBitRate = ((fps->iFrameTotalBytes - fps->iLastFrameTotalBytes) * 1000) / (1024 * real_interval_ms);  // KB
    }

    *pfps = fps->iFPS;
    *bitrate = fps->iBitRate;
    fps->iStartTimeRec = 0;
    result = 1;
  }

  // FPS_UNLOCK(fps);

  return result;
}

int32_t media_fps_add(stream_fps_t *fps, int32_t framesize) {
  // FPS_LOCK(fps);
  fps->iFramesCnt++;
  fps->iFrameTotalBytes += framesize;
  // FPS_UNLOCK(fps);

  return 0;
}

int32_t media_fps_deinit(stream_fps_t *fps) {
  pthread_mutex_destroy(&fps->mutex);
  return 0;
}