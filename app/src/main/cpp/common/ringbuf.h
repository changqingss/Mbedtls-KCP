#ifndef __FINGBUF_H__
#define __FINGBUF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

#ifdef WIN32
#else
typedef unsigned int UINT;
#endif

#if 1  // def PLATFORM_INGENIC_T23
#define MAX_CHANNEL_NUM 3

#define MAX_MAIN_RINGBUF_LEN 1.5 * 1024 * 1024
#define LEN_MAIN_BUF_IFRAME 512 * 1024  // SPS+PPS+I Frame size

#define MAX_SUB_RINGBUF_LEN 256 * 1024
#define LEN_SUB_BUF_IFRAME 128 * 1024  // SPS+PPS+I Frame size
#else
#define MAX_CHANNEL_NUM 3

#define MAX_MAIN_RINGBUF_LEN 3 * 1024 * 1024
#define LEN_MAIN_BUF_IFRAME 512 * 1024  // SPS+PPS+I Frame size

#define MAX_SUB_RINGBUF_LEN 512 * 1024
#define LEN_SUB_BUF_IFRAME 128 * 1024  // SPS+PPS+I Frame size
#endif

#define MAX_INDEX 1000
#define READSTART_INDEX 5

typedef int (*fRingBuffLib_ForceIDR)(int chn);

typedef enum RINGBUF_FRAME_TYPE {
  RINGBUF_FRAME_TYPE_VIDEO_I = 1,
  RINGBUF_FRAME_TYPE_VIDEO_P,
  RINGBUF_FRAME_TYPE_AUDIO_AAC,
  RINGBUF_FRAME_TYPE_AUDIO_G711A,
  RINGBUF_FRAME_TYPE_AUDIO_PCM,
  RINGBUF_FRAME_TYPE_MAX,
} RINGBUF_FRAME_TYPE_E;

typedef struct Index_s {
  int lock;                 /* 共享锁 */
  int frameType;            /* 当前包的类型 */
  unsigned int frameIndex;  /* 当前包的类型 */
  unsigned int len;         /* 当前包的长度 */
  unsigned int offset;      /* 当前包数据的起始地址 */
  unsigned long long pts;   /* 时间戳 */
  unsigned long long wTime; /* 写入时间 */
} Index_t;

typedef struct RingBuf_s {
  int nIsInit;              /*ringbuf是否已经初始化*/
  int nIsFull;              /*BUF是否满*/
  UINT uiReadIndex;         /*实时预览读数据索引*/
  UINT uiMaxLen;            /*BUF的最大容量*/
  UINT uiCurPos;            /*当前位置*/
  UINT uiHeadIndex;         /*指向BUF头的索引*/
  UINT uiCurIndex;          /*当前的Index*/
  UINT uiLeftLen;           /*剩下的BUF长度*/
  UINT uiOldestIndex;       /*最旧的Index*/
  UINT uinewIFrameIndex;    /*最新的I帧Index*/
  Index_t index[MAX_INDEX]; /*保存每一帧的数据大小和开始点*/
  char *pRingBuf;           /*数据存储BUF*/
  pthread_mutex_t lock;     /*锁*/
} RingBuf_t;

enum {
  BUF_UNLOCK = 0,
  BUF_LOCK,
};

typedef int (*fRING_BUF_ForceIDR)(int chn);

int RING_BUF_Init(fRING_BUF_ForceIDR func); /*初始化*/
int RING_BUF_ReleaseAll();                  /*释放*/
int RING_BUF_Release(int nChannel);         /*释放单个通道*/

int RING_BUF_FillBuf(int nChannel, int frameType, unsigned long long wTime, unsigned long long pts,
                     unsigned int uiDataLen, char *pData, unsigned int frameIndex); /*��������*/

int RING_BUF_GetOnePacket(int nChannel, char **pDataBuf, RINGBUF_FRAME_TYPE_E *pFrameType,
                          unsigned long long *TimeStamp, unsigned long long *wTimeStamp, unsigned int *StartReadIndex,
                          unsigned int *frameIndex);
int RING_BUF_GetNextIFrame(int nChannel, char **pDataBuf, RINGBUF_FRAME_TYPE_E *pFrameType,
                           unsigned long long *TimeStamp, unsigned long long *wTimeStamp, unsigned int *StartReadIndex,
                           int preFrmNum, unsigned int *frameIndex);
int RING_BUF_GetPreIFrame(int nChannel, unsigned char PreSec, int frmRate, unsigned long long limitTime,
                          char **pDataBuf, RINGBUF_FRAME_TYPE_E *pFrameType, unsigned long long *TimeStamp,
                          unsigned long long *wTimeStamp, unsigned int *StartReadIndex);
int RING_BUF_GetNextIFrameAllRecord(int nChannel, char **pDataBuf, int nBufLen, RINGBUF_FRAME_TYPE_E *pFrameType,
                                    unsigned long long *TimeStamp, unsigned long long *wTimeStamp,
                                    unsigned int *StartReadIndex, unsigned int lastReadIndex);

int RING_BUF_ReflushIDR(fRingBuffLib_ForceIDR ringBuffLibForceIDR, int maxFrame, int nChannel);
int RING_BUF_GetIFrameIndex(int nChannel, int *readIndex);
int RING_BUF_GetOneIndexPacket(int chn, unsigned int index, char **pkt, int *frameType);
int RING_BUF_ForceIDR(int nChannel, char *func, int line);

#ifdef __cplusplus
}
#endif

#endif
