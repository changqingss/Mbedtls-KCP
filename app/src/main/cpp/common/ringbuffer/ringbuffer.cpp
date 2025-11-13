#include "ringbuffer.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "CRingBuf.h"

int ringbuffer_writer_init(void **ppctx, RingBufferInfo_t *rbinfo, const char *rbname) {
  int ret = -1;
  RingBufferCtx_t *ringbufctx = (RingBufferCtx_t *)calloc(1, sizeof(RingBufferCtx_t));
  printf("%s ringbufctx:%p, rbinfo:%p, rbname:%p\n", __FUNCTION__, ringbufctx, rbinfo, rbname);
  if (ringbufctx) {
    snprintf(ringbufctx->username, sizeof(ringbufctx->username), "%p", ringbufctx);
    memcpy(&ringbufctx->rbinfo, rbinfo, sizeof(RingBufferInfo_t));
    bool livestream = true;
    if (0 == rbinfo->livestream) livestream = false;
    CRingBuf *ringbuf = new CRingBuf(ringbufctx->username, rbname, rbinfo->buflen, CRB_PERSONALITY_WRITER);
    ringbufctx->ringbuf = ringbuf;
    printf("%s, username[%s], rbname[%s], buflen[%d], livestream[%d], rintbuf[%p]", __FUNCTION__, ringbufctx->username,
           rbname, rbinfo->buflen, livestream, ringbufctx->ringbuf);
    *ppctx = ringbufctx;
    ret = 0;
  }

  return ret;
}

int ringbuffer_writer_deinit(void *pctx) {
  RingBufferCtx_t *ringbufctx = (RingBufferCtx_t *)pctx;
  if (ringbufctx) {
    if (ringbufctx->ringbuf) {
      CRingBuf *ringbuf = (CRingBuf *)ringbufctx->ringbuf;
      delete ringbuf;
      ringbufctx->ringbuf = NULL;
    }
    free(ringbufctx);
  }
  return 0;
}

int ringbuffer_writer_data(void *pctx, const char *data, int datalen, int iskey, long long frameTime) {
  int ret = -1;
  RingBufferCtx_t *ringbufctx = (RingBufferCtx_t *)pctx;
  if (ringbufctx && ringbufctx->ringbuf) {
    FRAME_E type = iskey > 0 ? CRB_FRAME_I_SLICE : CRB_FRAME_P_SLICE;
    CRB_WRITE_MODE_E accessmode =
        (ringbufctx->rbinfo.writemode == RBWRITE_MODE_BLOCK) ? CRB_WRITE_MODE_BLOCK : CRB_WRITE_MODE_NON_BLOCK;
    unsigned char *p = NULL;
    CRingBuf *ringbuf = (CRingBuf *)ringbufctx->ringbuf;
    p = ringbuf->RequestWriteFrame(datalen, type, accessmode);
    // COMLOG_I("ringbufctx = %p, datalen = %p, ringbuf = %p", ringbufctx, datalen, ringbuf);
    if (CRB_VALID_ADDRESS(p)) {
      memcpy(p, (const char *)data, datalen);
      printf("%s ringbufctx = %p, datalen = %d", __FUNCTION__, ringbufctx, datalen);
      ringbuf->CommitWrite(-1, NULL, NULL, frameTime);
      return 0;
    }
  }
  return ret;
}

int ringbuffer_writer_data_request(void *pctx, unsigned char **buffer, int bufferlen, int iskey) {
  int ret = -1;
  RingBufferCtx_t *ringbufctx = (RingBufferCtx_t *)pctx;
  // BALLOG_I("%s bufferlen[%d] ringbufctx = %p", __FUNCTION__, bufferlen, ringbufctx);
  if (ringbufctx && ringbufctx->ringbuf) {
    FRAME_E type = CRB_FRAME_I_SLICE;  // iskey;// > 0 ? CRB_FRAME_I_SLICE : CRB_FRAME_P_SLICE;
    if (iskey == 0) type = CRB_FRAME_I_SLICE;
    if (iskey == 1) type = CRB_FRAME_P_SLICE;
    if (iskey == 2) type = CRB_FRAME_AUDIO;

    CRB_WRITE_MODE_E accessmode =
        (ringbufctx->rbinfo.writemode == RBWRITE_MODE_BLOCK) ? CRB_WRITE_MODE_BLOCK : CRB_WRITE_MODE_NON_BLOCK;
    // COMLOG_D("%s accessmode[%d]", __FUNCTION__, accessmode);
    unsigned char *p = NULL;
    CRingBuf *ringbuf = (CRingBuf *)ringbufctx->ringbuf;
    p = ringbuf->RequestWriteFrame(bufferlen, type, accessmode);
    if (CRB_VALID_ADDRESS(p)) {
      *buffer = p;
      return 0;
    }
  }
  return ret;
}

int ringbuffer_writer_data_commit(void *pctx, long long frameTime) {
  int ret = -1;
  RingBufferCtx_t *ringbufctx = (RingBufferCtx_t *)pctx;
  if (ringbufctx && ringbufctx->ringbuf) {
    // COMLOG_I("%s, username[%s]", __FUNCTION__, ringbufctx->username);
    CRingBuf *ringbuf = (CRingBuf *)ringbufctx->ringbuf;
    ringbuf->CommitWrite(-1, NULL, NULL, frameTime);
    ret = 0;
  }
  return ret;
}

// static long long getmSecond() {
//   struct timespec ts;
//   unsigned long long msec = 0;
//   clock_gettime(CLOCK_MONOTONIC, &ts);
//   time_t sec = ts.tv_sec;
//   msec = ts.tv_nsec / 1000000;
//   return (sec * 1000 + msec);
// }
static long long getmSecond() {
  long long msec = 0;
  struct timeval tv;
  struct timezone tz;
  gettimeofday(&tv, &tz);

  msec += (long long)tv.tv_sec * 1000;
  msec += (long long)tz.tz_minuteswest * 60 * 1000;
  msec += (long long)tv.tv_usec / 1000;

  /* gaw modify start 2014-6-9 */
  msec += (long long)tz.tz_dsttime * 60 * 1000;
  /* gaw modify end */
  return msec;
}

int ringbuffer_reader_init(void **ppctx, RingBufferInfo_t *rbinfo, const char *rbname, long long int pretimes) {
  int ret = -1;
  RingBufferCtx_t *ringbufctx = (RingBufferCtx_t *)calloc(1, sizeof(RingBufferCtx_t));
  if (ringbufctx) {
    snprintf(ringbufctx->username, sizeof(ringbufctx->username), "%p", ringbufctx);
    memcpy(&ringbufctx->rbinfo, rbinfo, sizeof(RingBufferInfo_t));
    bool livestream = true;
    if (0 == rbinfo->livestream) livestream = false;
    ringbufctx->ringbuf = new CRingBuf(ringbufctx->username, rbname, rbinfo->buflen, CRB_PERSONALITY_READER);
    printf("%s, username[%s], rbname[%s], buflen[%d], livestream[%d], ringbufctx[%p], rintbuf[%p]\n", __FUNCTION__,
           ringbufctx->username, rbname, rbinfo->buflen, livestream, ringbufctx, ringbufctx->ringbuf);

    long long int mtimecur = getmSecond();
    long long int mtimeget = 0;
    CRingBuf *tmpRingbuf = (CRingBuf *)ringbufctx->ringbuf;
    if (tmpRingbuf->IndexMoveToSpecifiedTime(CRB_FRAME_I_SLICE, pretimes, mtimeget) != 0) {
      printf("[%s] IndexMoveToSpecifiedTime error\n", __FUNCTION__);
      // return ret;
    }
    printf("[%d] mtimecur:%lld diff:%lld mtimeget:%lld\n", __LINE__, mtimecur, mtimecur - pretimes, mtimeget);
    printf("[%d] pretimes:%lld mtimeget:%lld\n", __LINE__, pretimes, mtimeget);

    *ppctx = ringbufctx;
    ret = 0;
  }

  return ret;
}

int ringbuffer_reader_move_to_time(void **ppctx, long long int pretimes) {
  int ret = -1;
  long long int mtimeget = 0;
  RingBufferCtx_t *ringbufctx = (RingBufferCtx_t *)*ppctx;
  if (ringbufctx) {
    CRingBuf *tmpRingbuf = (CRingBuf *)ringbufctx->ringbuf;
    if (tmpRingbuf->IndexMoveToSpecifiedTime(CRB_FRAME_I_SLICE, pretimes, mtimeget) != 0) {
      printf("[%s] error\n", __FUNCTION__);
      return ret;
    }
    printf("[%d] pretimes:%lld mtimeget:%lld\n", __LINE__, pretimes, mtimeget);

    *ppctx = ringbufctx;
    ret = 0;
  }

  return ret;
}

int ringbuffer_reader_oldest(void **ppctx) {
  int ret = -1;
  RingBufferCtx_t *ringbufctx = (RingBufferCtx_t *)*ppctx;
  if (ringbufctx) {
    CRingBuf *tmpRingbuf = (CRingBuf *)ringbufctx->ringbuf;
    if (tmpRingbuf->IndexMoveToOldest() != 0) {
      printf("[%s] error\n", __FUNCTION__);
      // return ret;
    }

    *ppctx = ringbufctx;
    ret = 0;
  }

  return ret;
}

int ringbuffer_reader_latest(void **ppctx) {
  int ret = -1;
  RingBufferCtx_t *ringbufctx = (RingBufferCtx_t *)*ppctx;
  if (ringbufctx) {
    CRingBuf *tmpRingbuf = (CRingBuf *)ringbufctx->ringbuf;
    if (tmpRingbuf->IndexFindLatestIFrame() != 0) {
      printf("[%s] IndexFindLatestIFrame error\n", __FUNCTION__);
      return ret;
    }

    *ppctx = ringbufctx;
    ret = 0;
  }

  return ret;
}

int ringbuffer_reader_discard_all_data(void **ppctx) {
  int ret = -1;
  RingBufferCtx_t *ringbufctx = (RingBufferCtx_t *)*ppctx;
  if (ringbufctx) {
    CRingBuf *tmpRingbuf = (CRingBuf *)ringbufctx->ringbuf;
    tmpRingbuf->DiscardAllData();
    // printf("[%s] IndexFindLatestIFrame error\n", __FUNCTION__);
    // return ret;

    *ppctx = ringbufctx;
    ret = 0;
  }

  return ret;
}

int ringbuffer_reader_deinit(void *pctx) {
  RingBufferCtx_t *ringbufctx = (RingBufferCtx_t *)pctx;
  if (ringbufctx) {
    if (ringbufctx->ringbuf) {
      CRingBuf *ringbuf = (CRingBuf *)ringbufctx->ringbuf;
      delete ringbuf;
      ringbufctx->ringbuf = NULL;
    }
    free(ringbufctx);
  }
  return 0;
}

int ringbuffer_reader_data_request(void *pctx, void **data, int *datalen, uint32_t *iskey) {
  int ret = -1;
  RingBufferCtx_t *ringbufctx = (RingBufferCtx_t *)pctx;
  // printf("ringbufctx:%p ,ringbufctx->ringbuf:%p\n", ringbufctx ,ringbufctx->ringbuf);
  if (ringbufctx && ringbufctx->ringbuf) {
    CRingBuf *ringbuf = (CRingBuf *)ringbufctx->ringbuf;
    *data = ringbuf->RequestReadFrame(datalen, iskey);
    // printf("data:%p\n", *data);
    if (CRB_VALID_ADDRESS(*data) && *datalen > 0) {
      ret = 0;
    }
  }

  return ret;
}

int ringbuffer_reader_data_commit(void *pctx, void *data, int datalen) {
  RingBufferCtx_t *ringbufctx = (RingBufferCtx_t *)pctx;
  if (ringbufctx && ringbufctx->ringbuf && (datalen > 0)) {
    CRingBuf *ringbuf = (CRingBuf *)ringbufctx->ringbuf;
    ringbuf->CommitRead();
    return 0;
  }
  return -1;
}