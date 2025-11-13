#include "ringbuf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define SYSCALL_DEBUG_ENABLE 1

#ifdef SYSCALL_DEBUG_ENABLE
#define LOG_Debug(str, args...) printf(str, ##args)
#define LOG_Error(str, args...) printf(str, ##args)
#define LOG_Info(str, args...) printf(str, ##args)
#define LOG_Warn(str, args...) printf(str, ##args)
#else
#define LOG_Debug(fm, ...)
#define LOG_Error(fm, ...)
#define LOG_Info(fm, ...)
#define LOG_Warn(fm, ...)
#endif

static RingBuf_t g_RingDataBuf[MAX_CHANNEL_NUM];  // 数据环形缓冲(每个通道只有一种码流)
fRING_BUF_ForceIDR gForceIDRfunc = NULL;

int RING_BUF_Init(fRING_BUF_ForceIDR func) {
  int i = 0;
  for (i = 0; i < MAX_CHANNEL_NUM; i++) {
    if (g_RingDataBuf[i].pRingBuf != NULL) {
      free(g_RingDataBuf[i].pRingBuf);
      g_RingDataBuf[i].pRingBuf = NULL;
    }
    memset(&g_RingDataBuf[i], 0, sizeof(RingBuf_t));
    if (i == 0) {
      g_RingDataBuf[i].uiMaxLen = MAX_MAIN_RINGBUF_LEN;
      g_RingDataBuf[i].uiLeftLen = MAX_MAIN_RINGBUF_LEN;
      g_RingDataBuf[i].pRingBuf = malloc(MAX_MAIN_RINGBUF_LEN);
      if (g_RingDataBuf[i].pRingBuf == NULL) {
        LOG_Error("calloc fail");
        return -1;
      }
    } else if (i == 2) {
      g_RingDataBuf[i].uiMaxLen = MAX_SUB_RINGBUF_LEN;
      g_RingDataBuf[i].uiLeftLen = MAX_SUB_RINGBUF_LEN;
      g_RingDataBuf[i].pRingBuf = malloc(MAX_SUB_RINGBUF_LEN);
      if (g_RingDataBuf[i].pRingBuf == NULL) {
        LOG_Error("calloc fail");
        return -1;
      }
    } else {
      g_RingDataBuf[i].uiMaxLen = MAX_SUB_RINGBUF_LEN;
      g_RingDataBuf[i].uiLeftLen = MAX_SUB_RINGBUF_LEN;
      g_RingDataBuf[i].pRingBuf = malloc(MAX_SUB_RINGBUF_LEN);

      if (g_RingDataBuf[i].pRingBuf == NULL) {
        LOG_Error("calloc fail");
        return -1;
      }
    }
    g_RingDataBuf[i].uiReadIndex = 0;
    g_RingDataBuf[i].nIsInit = 1;
    pthread_mutex_init(&g_RingDataBuf[i].lock, NULL);
  }

  gForceIDRfunc = func;

  return 0;
}

int RING_BUF_ForceIDR(int nChannel, char *func __attribute__((unused)), int line __attribute__((unused))) {
  LOG_Info("----- [%s:%d] force IFrame-----!\n", func, line);
  if (gForceIDRfunc) {
    (*gForceIDRfunc)(nChannel);
  }

  return 0;
}

int RING_BUF_ReleaseAll() {
  int i = 0;
  for (i = 0; i < MAX_CHANNEL_NUM; i++) {
    if (g_RingDataBuf[i].pRingBuf != NULL) {
      free(g_RingDataBuf[i].pRingBuf);
    }
    pthread_mutex_destroy(&g_RingDataBuf[i].lock);
    memset(&g_RingDataBuf[i], 0, sizeof(RingBuf_t));
    g_RingDataBuf[i].nIsInit = 0;
  }

  return 0;
}

int RING_BUF_Release(int nChannel) {
  if (nChannel < MAX_CHANNEL_NUM) {
    if (g_RingDataBuf[nChannel].pRingBuf != NULL) {
      free(g_RingDataBuf[nChannel].pRingBuf);
    }
    pthread_mutex_destroy(&g_RingDataBuf[nChannel].lock);
    memset(&g_RingDataBuf[nChannel], 0, sizeof(RingBuf_t));
    g_RingDataBuf[nChannel].nIsInit = 0;
  }

  return 0;
}

int RING_BUF_GetLastIndex(UINT uiIndex) {
  if (uiIndex == 0) {
    uiIndex = MAX_INDEX - 1;
  }
  uiIndex--;

  return uiIndex;
}

int RING_BUF_GetNextIndex(UINT uiIndex) {
  uiIndex++;
  if (uiIndex >= MAX_INDEX) {
    uiIndex = 0;
  }

  return uiIndex;
}

int RING_BUF_GetPreIndex(UINT uiIndex) {
  uiIndex--;
  if (uiIndex == 0) {
    uiIndex = MAX_INDEX;
  }

  return uiIndex;
}

int RING_BUF_FillBuf(int nChannel, int frameType, unsigned long long wTime, unsigned long long pts,
                     unsigned int uiDataLen, char *pData, unsigned int frameIndex) {
  if (!g_RingDataBuf[nChannel].nIsInit) {
    return -1;
  }
  pthread_mutex_lock(&g_RingDataBuf[nChannel].lock);
  UINT uiLeftLen = 0;
  while (1) {
    if (nChannel == 0)
      uiLeftLen = MAX_MAIN_RINGBUF_LEN - g_RingDataBuf[nChannel].uiCurPos;
    else
      uiLeftLen = MAX_SUB_RINGBUF_LEN - g_RingDataBuf[nChannel].uiCurPos;

    if (g_RingDataBuf[nChannel].nIsFull == 1) {
      /*覆盖数据*/
      if (uiLeftLen < uiDataLen) {
        /*剩下的空间不足保存数据，从头开始存数据*/
        g_RingDataBuf[nChannel].uiCurPos = 0;
        g_RingDataBuf[nChannel].uiOldestIndex = g_RingDataBuf[nChannel].uiHeadIndex;
        uiLeftLen = 0;
        while (uiLeftLen < uiDataLen) {
          uiLeftLen += g_RingDataBuf[nChannel].index[g_RingDataBuf[nChannel].uiOldestIndex].len;
          g_RingDataBuf[nChannel].uiOldestIndex = RING_BUF_GetNextIndex(g_RingDataBuf[nChannel].uiOldestIndex);
        }
        g_RingDataBuf[nChannel].uiHeadIndex = g_RingDataBuf[nChannel].uiCurIndex;
      } else {
        // printf("%s,left buffer can save data...\n", __func__);
        uiLeftLen = g_RingDataBuf[nChannel].uiLeftLen;
        while (uiLeftLen < uiDataLen) {
          if (g_RingDataBuf[nChannel].index[g_RingDataBuf[nChannel].uiOldestIndex].offset == 0) {
            /*最旧的buffer是在缓冲头部*/
            uiLeftLen = g_RingDataBuf[nChannel].uiMaxLen - g_RingDataBuf[nChannel].uiCurPos;
            break;
          } else {
            uiLeftLen += g_RingDataBuf[nChannel].index[g_RingDataBuf[nChannel].uiOldestIndex].len;
            g_RingDataBuf[nChannel].uiOldestIndex = RING_BUF_GetNextIndex(g_RingDataBuf[nChannel].uiOldestIndex);
          }
        }
      }
    } else {
      /*BUF是空的情况，直接插入数据*/
      if (uiLeftLen < uiDataLen) {
        g_RingDataBuf[nChannel].nIsFull = 1;
        continue;
      }
    }
    break;
  }
  memcpy(g_RingDataBuf[nChannel].pRingBuf + g_RingDataBuf[nChannel].uiCurPos, pData, uiDataLen);
  g_RingDataBuf[nChannel].index[g_RingDataBuf[nChannel].uiCurIndex].len = uiDataLen;
  g_RingDataBuf[nChannel].index[g_RingDataBuf[nChannel].uiCurIndex].offset = g_RingDataBuf[nChannel].uiCurPos;
  g_RingDataBuf[nChannel].index[g_RingDataBuf[nChannel].uiCurIndex].frameType = frameType;
  g_RingDataBuf[nChannel].index[g_RingDataBuf[nChannel].uiCurIndex].wTime = wTime;
  g_RingDataBuf[nChannel].index[g_RingDataBuf[nChannel].uiCurIndex].pts = pts;
  g_RingDataBuf[nChannel].index[g_RingDataBuf[nChannel].uiCurIndex].frameIndex = frameIndex;

  g_RingDataBuf[nChannel].uiCurPos += uiDataLen;
  if (frameType == RINGBUF_FRAME_TYPE_VIDEO_I) {
    g_RingDataBuf[nChannel].uinewIFrameIndex = g_RingDataBuf[nChannel].uiCurIndex;
  }

  g_RingDataBuf[nChannel].uiCurIndex = RING_BUF_GetNextIndex(g_RingDataBuf[nChannel].uiCurIndex);
  g_RingDataBuf[nChannel].uiLeftLen = uiLeftLen - uiDataLen;

  pthread_mutex_unlock(&g_RingDataBuf[nChannel].lock);
  return 0;
}

int RING_BUF_GetOnePacket(int nChannel, char **pDataBuf, RINGBUF_FRAME_TYPE_E *pFrameType,
                          unsigned long long *TimeStamp, unsigned long long *wTimeStamp, unsigned int *StartReadIndex,
                          unsigned int *frameIndex) {
  int nLen = -1;
  UINT uiReadIndex = *StartReadIndex;
  if (pDataBuf == NULL || uiReadIndex >= MAX_INDEX) {
    if (uiReadIndex >= MAX_INDEX) {
      *StartReadIndex = g_RingDataBuf[nChannel].uinewIFrameIndex;
    }
    return nLen;
  }

  if (!g_RingDataBuf[nChannel].nIsInit) {
    return -1;
  }

  if (g_RingDataBuf[nChannel].uiCurIndex != uiReadIndex) {
    if (g_RingDataBuf[nChannel].index[uiReadIndex].frameType == RINGBUF_FRAME_TYPE_AUDIO_AAC ||
        g_RingDataBuf[nChannel].index[uiReadIndex].frameType == RINGBUF_FRAME_TYPE_AUDIO_G711A ||
        g_RingDataBuf[nChannel].index[uiReadIndex].frameType == RINGBUF_FRAME_TYPE_AUDIO_PCM) {
#if 0
			if( *(g_RingDataBuf[nChannel].pRingBuf + g_RingDataBuf[nChannel].index[uiReadIndex].offset) != 0xff || *(g_RingDataBuf[nChannel].pRingBuf + g_RingDataBuf[nChannel].index[uiReadIndex].offset + 1) != 0xf1)
			{
				
				printf(":::%s:::%d::  Data error   uiCurIndex=%d  uiReadIndex = %d  frameType = %d\n",__func__,__LINE__,g_RingDataBuf[nChannel].uiCurIndex,uiReadIndex,g_RingDataBuf[nChannel].index[uiReadIndex].frameType);
				/*uiReadIndex = RingBuf_GetNextIndex(uiReadIndex);
				if(g_RingDataBuf[nChannel].uiCurIndex >= uiReadIndex)
				{	
					if(g_RingDataBuf[nChannel].uiCurIndex - uiReadIndex > 100)
					{		
						uiReadIndex = g_RingDataBuf[nChannel].uiOldestIndex;
						*StartReadIndex = uiReadIndex;
					}
				}
				else
				{
					if(g_RingDataBuf[nChannel].uiCurIndex + MAX_INDEX - uiReadIndex > 100)
					{		
						uiReadIndex = g_RingDataBuf[nChannel].uiOldestIndex;
						*StartReadIndex = uiReadIndex;
					}
				}*/
				uiReadIndex = g_RingDataBuf[nChannel].uinewIFrameIndex;
				*StartReadIndex = uiReadIndex;
			}
#endif
    } else {
      if (*(g_RingDataBuf[nChannel].pRingBuf + g_RingDataBuf[nChannel].index[uiReadIndex].offset) != 0x00 ||
          *(g_RingDataBuf[nChannel].pRingBuf + g_RingDataBuf[nChannel].index[uiReadIndex].offset + 1) != 0x00 ||
          *(g_RingDataBuf[nChannel].pRingBuf + g_RingDataBuf[nChannel].index[uiReadIndex].offset + 2) != 0x00 ||
          *(g_RingDataBuf[nChannel].pRingBuf + g_RingDataBuf[nChannel].index[uiReadIndex].offset + 3) != 0x01) {
        printf(
            ":::%s:::%d::  Data error   uiCurIndex=%d  uiReadIndex = %d  "
            "frameType = %d\n",
            __func__, __LINE__, g_RingDataBuf[nChannel].uiCurIndex, uiReadIndex,
            g_RingDataBuf[nChannel].index[uiReadIndex].frameType);
        uiReadIndex = g_RingDataBuf[nChannel].uinewIFrameIndex;
        *StartReadIndex = uiReadIndex;
      }
    }
  }

  if (g_RingDataBuf[nChannel].uiCurIndex != uiReadIndex) {
    nLen = g_RingDataBuf[nChannel].index[uiReadIndex].len;
    if (nLen) {
      *pDataBuf = g_RingDataBuf[nChannel].pRingBuf + g_RingDataBuf[nChannel].index[uiReadIndex].offset;
      *pFrameType = g_RingDataBuf[nChannel].index[uiReadIndex].frameType;
      *TimeStamp = g_RingDataBuf[nChannel].index[uiReadIndex].pts;
      *wTimeStamp = g_RingDataBuf[nChannel].index[uiReadIndex].wTime;
      *frameIndex = g_RingDataBuf[nChannel].index[uiReadIndex].frameIndex;
    }
    uiReadIndex = RING_BUF_GetNextIndex(uiReadIndex);
    *StartReadIndex = uiReadIndex;
  }
  return nLen;
}

int RING_BUF_GetPreIFrame(int nChannel, unsigned char PreSec, int frmRate, unsigned long long limitTime,
                          char **pDataBuf, RINGBUF_FRAME_TYPE_E *pFrameType, unsigned long long *TimeStamp,
                          unsigned long long *wTimeStamp, unsigned int *StartReadIndex) {
  int nLen = -1;
  unsigned int ReadIndex = 0;

  if (pDataBuf == NULL) {
    return nLen;
  }

  if (!g_RingDataBuf[nChannel].nIsInit) {
    return -1;
  }

  if (g_RingDataBuf[nChannel].nIsFull != 1) {
    return -1;
  }

  pthread_mutex_lock(&g_RingDataBuf[nChannel].lock);

  if (g_RingDataBuf[nChannel].uiOldestIndex < g_RingDataBuf[nChannel].uiCurIndex) {
    if (g_RingDataBuf[nChannel].uiCurIndex > (UINT)(frmRate * PreSec)) {
      ReadIndex = g_RingDataBuf[nChannel].uiCurIndex - (frmRate * PreSec);
    } else {
      ReadIndex = g_RingDataBuf[nChannel].uiOldestIndex;
    }
  } else {
    if (g_RingDataBuf[nChannel].uiCurIndex > (UINT)(frmRate * PreSec)) {
      ReadIndex = g_RingDataBuf[nChannel].uiCurIndex - (frmRate * PreSec);
    } else {
      if ((MAX_INDEX - g_RingDataBuf[nChannel].uiOldestIndex + g_RingDataBuf[nChannel].uiCurIndex) >
          (UINT)(frmRate * PreSec)) {
        ReadIndex = MAX_INDEX - (frmRate * PreSec - g_RingDataBuf[nChannel].uiCurIndex);
      } else {
        ReadIndex = g_RingDataBuf[nChannel].uiOldestIndex;
      }
    }
  }

  while (g_RingDataBuf[nChannel].uiCurIndex != ReadIndex) {
    if ((g_RingDataBuf[nChannel].index[ReadIndex].frameType == RINGBUF_FRAME_TYPE_VIDEO_I &&
         *(g_RingDataBuf[nChannel].pRingBuf + g_RingDataBuf[nChannel].index[ReadIndex].offset) == 0x00 &&
         *(g_RingDataBuf[nChannel].pRingBuf + g_RingDataBuf[nChannel].index[ReadIndex].offset + 1) == 0x00 &&
         *(g_RingDataBuf[nChannel].pRingBuf + g_RingDataBuf[nChannel].index[ReadIndex].offset + 2) == 0x00 &&
         *(g_RingDataBuf[nChannel].pRingBuf + g_RingDataBuf[nChannel].index[ReadIndex].offset + 3) == 0x01 &&
         (*(g_RingDataBuf[nChannel].pRingBuf + g_RingDataBuf[nChannel].index[ReadIndex].offset + 4) & 0x1F) == 0x07) ||
        *(g_RingDataBuf[nChannel].pRingBuf + g_RingDataBuf[nChannel].index[ReadIndex].offset + 4) == 0x40)

    {
      nLen = g_RingDataBuf[nChannel].index[ReadIndex].len;
      if (nLen) {
        *pDataBuf = g_RingDataBuf[nChannel].pRingBuf + g_RingDataBuf[nChannel].index[ReadIndex].offset;
        *pFrameType = g_RingDataBuf[nChannel].index[ReadIndex].frameType;
        *TimeStamp = g_RingDataBuf[nChannel].index[ReadIndex].pts;
        *wTimeStamp = g_RingDataBuf[nChannel].index[ReadIndex].wTime;
        ReadIndex = RING_BUF_GetNextIndex(ReadIndex);
        *StartReadIndex = ReadIndex;
        LOG_Info("uiOldestIndex = %d  uiCurIndex  =  %d  StartReadIndex  =  %d timeTamp:%llu\n",
                 g_RingDataBuf[nChannel].uiOldestIndex, g_RingDataBuf[nChannel].uiCurIndex, *StartReadIndex,
                 *TimeStamp);
      }
      LOG_Info("+++++++++++ limitTime=%llu, TimeStamp=%llu", limitTime, *TimeStamp);
      if (limitTime > *TimeStamp) {
        continue;
      }
      break;
    }
    ReadIndex = RING_BUF_GetNextIndex(ReadIndex);
    *StartReadIndex = ReadIndex;
  }
  pthread_mutex_unlock(&g_RingDataBuf[nChannel].lock);

  return nLen;
}

int RING_BUF_GetPreOneIFrame(int nChannel, char **pDataBuf, RINGBUF_FRAME_TYPE_E *pFrameType,
                             unsigned long long *TimeStamp, unsigned long long *wTimeStamp, unsigned char PreFlag,
                             unsigned int *StartReadIndex) {
  int nLen = -1;
  unsigned int ReadIndex = 0;
  unsigned int offset = 0;
  char *pRingBuf = NULL;

  if (pDataBuf == NULL) {
    return nLen;
  }

  if (!g_RingDataBuf[nChannel].nIsInit) {
    return -1;
  }

  pthread_mutex_lock(&g_RingDataBuf[nChannel].lock);

  if (PreFlag)  // 预录标志
  {
    ReadIndex = g_RingDataBuf[nChannel].uiOldestIndex;
  } else {
    ReadIndex = g_RingDataBuf[nChannel].uiCurIndex;
  }

  while (g_RingDataBuf[nChannel].uiCurIndex != g_RingDataBuf[nChannel].uiOldestIndex) {
    offset = g_RingDataBuf[nChannel].index[ReadIndex].offset;
    pRingBuf = g_RingDataBuf[nChannel].pRingBuf;
    if (g_RingDataBuf[nChannel].index[ReadIndex].frameType == RINGBUF_FRAME_TYPE_VIDEO_I &&
        ((*(pRingBuf + offset) == 0x00 && *(pRingBuf + offset + 1) == 0x00 && *(pRingBuf + offset + 2) == 0x00 &&
          *(pRingBuf + offset + 3) == 0x01) ||
         (*(pRingBuf + offset) == 0x00 && *(pRingBuf + offset + 1) == 0x00 && *(pRingBuf + offset + 2) == 0x01)))

    {
      nLen = g_RingDataBuf[nChannel].index[ReadIndex].len;
      if (nLen) {
        *pDataBuf = g_RingDataBuf[nChannel].pRingBuf + g_RingDataBuf[nChannel].index[ReadIndex].offset;
        *pFrameType = g_RingDataBuf[nChannel].index[ReadIndex].frameType;
        *TimeStamp = g_RingDataBuf[nChannel].index[ReadIndex].pts;
        *wTimeStamp = g_RingDataBuf[nChannel].index[ReadIndex].wTime;
        ReadIndex = RING_BUF_GetPreIndex(ReadIndex);
        *StartReadIndex = ReadIndex;
        LOG_Info(
            "uiOldestIndex = %d  uiCurIndex  =  %d   StartReadIndex  =  %d   "
            "PreFlag = %d    timeTamp:%llu wTimeStamp:%ld\n",
            g_RingDataBuf[nChannel].uiOldestIndex, g_RingDataBuf[nChannel].uiCurIndex, *StartReadIndex, PreFlag,
            *TimeStamp, *wTimeStamp);
      }
      break;
    }
    ReadIndex = RING_BUF_GetPreIndex(ReadIndex);
    *StartReadIndex = ReadIndex;
  }
  pthread_mutex_unlock(&g_RingDataBuf[nChannel].lock);

  return nLen;
}

int RING_BUF_GetNextIFrameAllRecord(int nChannel, char **pDataBuf, int nBufLen, RINGBUF_FRAME_TYPE_E *pFrameType,
                                    unsigned long long *TimeStamp, unsigned long long *wTimeStamp,
                                    unsigned int *StartReadIndex, unsigned int lastReadIndex) {
  int nLen = -1;
  unsigned int ReadIndex = 0;
  unsigned int offset = 0;
  char *pRingBuf = NULL;

  if (pDataBuf == NULL) {
    return nLen;
  }

  if (!g_RingDataBuf[nChannel].nIsInit) {
    return -1;
  }

  pthread_mutex_lock(&g_RingDataBuf[nChannel].lock);

  if (lastReadIndex == 0) {
    if (g_RingDataBuf[nChannel].uiOldestIndex < g_RingDataBuf[nChannel].uiCurIndex) {
      if (g_RingDataBuf[nChannel].uiCurIndex > READSTART_INDEX) {
        ReadIndex = g_RingDataBuf[nChannel].uiCurIndex - READSTART_INDEX;
      } else {
        ReadIndex = g_RingDataBuf[nChannel].uiOldestIndex;
      }
    } else {
      if (g_RingDataBuf[nChannel].uiCurIndex > READSTART_INDEX) {
        ReadIndex = g_RingDataBuf[nChannel].uiCurIndex - READSTART_INDEX;
      } else {
        if ((MAX_INDEX - g_RingDataBuf[nChannel].uiOldestIndex + g_RingDataBuf[nChannel].uiCurIndex) >
            READSTART_INDEX) {
          ReadIndex = MAX_INDEX - (READSTART_INDEX - g_RingDataBuf[nChannel].uiCurIndex);
        } else {
          ReadIndex = g_RingDataBuf[nChannel].uiOldestIndex;
        }
      }
    }

    if (g_RingDataBuf[nChannel].uiCurIndex > READSTART_INDEX) {
      ReadIndex = g_RingDataBuf[nChannel].uiCurIndex - READSTART_INDEX;
    } else {
      ReadIndex = g_RingDataBuf[nChannel].uiCurIndex + MAX_INDEX - READSTART_INDEX;
    }

    printf("uiCurIndex:%d lastReadIndex:%d\n", g_RingDataBuf[nChannel].uiCurIndex, lastReadIndex);
  } else {
    ReadIndex = lastReadIndex;
    printf("uiCurIndex:%d lastReadIndex:%d\n", g_RingDataBuf[nChannel].uiCurIndex, lastReadIndex);
  }

  while (g_RingDataBuf[nChannel].uiCurIndex != ReadIndex) {
    offset = g_RingDataBuf[nChannel].index[ReadIndex].offset;
    pRingBuf = g_RingDataBuf[nChannel].pRingBuf;
    if (g_RingDataBuf[nChannel].index[ReadIndex].frameType == RINGBUF_FRAME_TYPE_VIDEO_I &&
        ((*(pRingBuf + offset) == 0x00 && *(pRingBuf + offset + 1) == 0x00 && *(pRingBuf + offset + 2) == 0x00 &&
          *(pRingBuf + offset + 3) == 0x01) ||
         (*(pRingBuf + offset) == 0x00 && *(pRingBuf + offset + 1) == 0x00 && *(pRingBuf + offset + 2) == 0x01)))

    {
      nLen = g_RingDataBuf[nChannel].index[ReadIndex].len;
      if (nBufLen >= nLen) {
        *pDataBuf = g_RingDataBuf[nChannel].pRingBuf + g_RingDataBuf[nChannel].index[ReadIndex].offset;
        *pFrameType = g_RingDataBuf[nChannel].index[ReadIndex].frameType;
        *TimeStamp = g_RingDataBuf[nChannel].index[ReadIndex].pts;
        *wTimeStamp = g_RingDataBuf[nChannel].index[ReadIndex].wTime;
        ReadIndex = RING_BUF_GetNextIndex(ReadIndex);
        *StartReadIndex = ReadIndex;
        LOG_Info(
            "uiOldestIndex = %d  uiCurIndex  =  %d   StartReadIndex  =  %d   "
            "timeTamp:%llu wTimeStamp:%ld\n",
            g_RingDataBuf[nChannel].uiOldestIndex, g_RingDataBuf[nChannel].uiCurIndex, *StartReadIndex, *TimeStamp,
            *wTimeStamp);
      } else {
        LOG_Warn("nBufLen < nLen\n");
        nLen = -1;
      }
      break;
    }
    ReadIndex = RING_BUF_GetNextIndex(ReadIndex);
    *StartReadIndex = ReadIndex;
  }
  pthread_mutex_unlock(&g_RingDataBuf[nChannel].lock);

  return nLen;
}

int RING_BUF_GetNextIFrame(int nChannel, char **pDataBuf, RINGBUF_FRAME_TYPE_E *pFrameType,
                           unsigned long long *TimeStamp, unsigned long long *wTimeStamp, unsigned int *StartReadIndex,
                           int preFrmNum, unsigned int *frameIndex) {
  int nLen = -1;
  unsigned int offset = 0;
  unsigned int ReadIndex = 0;
  char *pRingBuf = NULL;

  if (pDataBuf == NULL) {
    return nLen;
  }

  if (!g_RingDataBuf[nChannel].nIsInit) {
    return -1;
  }

  pthread_mutex_lock(&g_RingDataBuf[nChannel].lock);

  if (g_RingDataBuf[nChannel].uiOldestIndex < g_RingDataBuf[nChannel].uiCurIndex) {
    if (g_RingDataBuf[nChannel].uiCurIndex > (UINT)preFrmNum) {
      ReadIndex = g_RingDataBuf[nChannel].uiCurIndex - (UINT)preFrmNum;
    } else {
      ReadIndex = g_RingDataBuf[nChannel].uiOldestIndex;
    }
  } else {
    if (g_RingDataBuf[nChannel].uiCurIndex > (UINT)preFrmNum) {
      ReadIndex = g_RingDataBuf[nChannel].uiCurIndex - (UINT)preFrmNum;
    } else {
      if ((MAX_INDEX - g_RingDataBuf[nChannel].uiOldestIndex + g_RingDataBuf[nChannel].uiCurIndex) > (UINT)preFrmNum) {
        ReadIndex = MAX_INDEX - ((UINT)preFrmNum - g_RingDataBuf[nChannel].uiCurIndex);
      } else {
        ReadIndex = g_RingDataBuf[nChannel].uiOldestIndex;
      }
    }
  }

  if (g_RingDataBuf[nChannel].uiCurIndex > (UINT)preFrmNum) {
    ReadIndex = g_RingDataBuf[nChannel].uiCurIndex - preFrmNum;
  } else {
    ReadIndex = MAX_INDEX - (READSTART_INDEX - g_RingDataBuf[nChannel].uiCurIndex);
  }

  while (g_RingDataBuf[nChannel].uiCurIndex != ReadIndex) {
    offset = g_RingDataBuf[nChannel].index[ReadIndex].offset;
    pRingBuf = g_RingDataBuf[nChannel].pRingBuf;
    if (g_RingDataBuf[nChannel].index[ReadIndex].frameType == RINGBUF_FRAME_TYPE_VIDEO_I &&
        ((*(pRingBuf + offset) == 0x00 && *(pRingBuf + offset + 1) == 0x00 && *(pRingBuf + offset + 2) == 0x00 &&
          *(pRingBuf + offset + 3) == 0x01) ||
         (*(pRingBuf + offset) == 0x00 && *(pRingBuf + offset + 1) == 0x00 && *(pRingBuf + offset + 2) == 0x01))) {
      nLen = g_RingDataBuf[nChannel].index[ReadIndex].len;
      if (nLen) {
        // memcpy( *pDataBuf, g_RingDataBuf[nChannel].pRingBuf +
        // g_RingDataBuf[nChannel].index[ReadIndex].offset, nLen);
        *pDataBuf = g_RingDataBuf[nChannel].pRingBuf + g_RingDataBuf[nChannel].index[ReadIndex].offset;
        *pFrameType = g_RingDataBuf[nChannel].index[ReadIndex].frameType;
        *TimeStamp = g_RingDataBuf[nChannel].index[ReadIndex].pts;
        *wTimeStamp = g_RingDataBuf[nChannel].index[ReadIndex].wTime;
        *frameIndex = g_RingDataBuf[nChannel].index[ReadIndex].frameIndex;
        ReadIndex = RING_BUF_GetNextIndex(ReadIndex);
        *StartReadIndex = ReadIndex;
        /*
        LOG_Info(
            "uiOldestIndex = %d  uiCurIndex  =  %d   StartReadIndex  =  %d   "
            "ModuleName = %s timeTamp:%llu wTimeStamp:%llu\n",
            g_RingDataBuf[nChannel].uiOldestIndex, g_RingDataBuf[nChannel].uiCurIndex, *StartReadIndex, ModuleName,
            *TimeStamp, *wTimeStamp);
        */
      }
      break;
    }
    ReadIndex = RING_BUF_GetNextIndex(ReadIndex);
    *StartReadIndex = ReadIndex;
  }
  pthread_mutex_unlock(&g_RingDataBuf[nChannel].lock);

  return nLen;
}

int RING_BUF_GetOneIndexPacket(int nChannel, unsigned int index, char **pkt, int *frameType) {
  int len = 0;

  pthread_mutex_lock(&g_RingDataBuf[nChannel].lock);
  *pkt = g_RingDataBuf[nChannel].pRingBuf + g_RingDataBuf[nChannel].index[index].offset;
  len = g_RingDataBuf[nChannel].index[index].len;
  *frameType = g_RingDataBuf[nChannel].index[index].frameType;
  pthread_mutex_unlock(&g_RingDataBuf[nChannel].lock);
  return len;
}

int RING_BUF_GetIFrameIndex(int nChannel, int *readIndex) {
  int ret = 0;

  if (!g_RingDataBuf[nChannel].nIsInit) {
    return -1;
  }

  pthread_mutex_lock(&g_RingDataBuf[nChannel].lock);

  *readIndex = g_RingDataBuf[nChannel].uinewIFrameIndex;
  pthread_mutex_unlock(&g_RingDataBuf[nChannel].lock);

  if (*readIndex >= 0 && *readIndex < 400) {
    ret = 1;
  }
  return ret;
}

int RING_BUF_ReflushIDR(fRingBuffLib_ForceIDR ringBuffLibForceIDR, int maxFrame, int nChannel) {
  int ret = 0, iframeIndex = 0, currIndex, allFrame = 0;

  if (!g_RingDataBuf[nChannel].nIsInit) {
    return -1;
  }

  pthread_mutex_lock(&g_RingDataBuf[nChannel].lock);

  currIndex = g_RingDataBuf[nChannel].uiCurIndex;
  iframeIndex = g_RingDataBuf[nChannel].uinewIFrameIndex;

  if (currIndex - iframeIndex >= 0) {
    allFrame = currIndex - iframeIndex;
  } else {
    allFrame = (currIndex + MAX_INDEX) - iframeIndex;
  }
  if (allFrame > maxFrame) {
    ret = ringBuffLibForceIDR(nChannel);
  }
  pthread_mutex_unlock(&g_RingDataBuf[nChannel].lock);
  return ret;
}
