#ifndef __ringbuffer_h__
#define __ringbuffer_h__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define CRB_PERSONALITY_READER (1 << 0)
#define CRB_PERSONALITY_WRITER (1 << 1)
#define CRB_ERROR_BEGIN (0xFFFFFFF0)

#define CRB_VALID_ADDRESS(addr) ((uint32_t *)(addr) < (uint32_t *)CRB_ERROR_BEGIN && (uint8_t *)(addr) != NULL)
// #define CRB_VALID_ADDRESS(addr) ((uint8_t*)(addr) != NULL)

typedef struct _RingBufInfo RingBufInfo;
typedef struct _RingBufUser RingBufUser;
typedef struct _BlackHole BlackHole;

typedef struct _FrameIdx {
  uint32_t offset;
  uint32_t length : 31;
  uint32_t isIFrame : 1;
} FrameIdx;

typedef enum __FRAME_E {
  CRB_FRAME_I_SLICE,

  CRB_FRAME_P_SLICE,

  CRB_FRAME_AUDIO,
  CRB_FRAME_JPEG,
  CRB_FRAME_LOG,
  CRB_FRAME_DISCOVERY,
  CRB_FRAME_SEEK,
  CRB_FRAME_END,

  CRB_FRAME_ANY,
} FRAME_E;

/*
 * See RequestWriteFrame
 */
typedef enum __CRB_WRITE_MODE_E {
  CRB_WRITE_MODE_BLOCK,
  CRB_WRITE_MODE_NON_BLOCK,
} CRB_WRITE_MODE_E;

#ifdef __cplusplus

class CRingBuf {
 public:
  CRingBuf(const char *usrName, const char *ringBufName, int size, int personality, bool liveStream = false);
  virtual ~CRingBuf();

 public:
  uint8_t *RequestWriteFrame(uint32_t size, FRAME_E type, CRB_WRITE_MODE_E mode = CRB_WRITE_MODE_NON_BLOCK);
  int32_t DiscardLeftedData(void);
  int CommitWrite(uint32_t size = -1, int32_t *pStartTime = NULL, int32_t *pEndTime = NULL, long long frameTime = 0);
  uint8_t *RequestReadFrame(int32_t *frameLen, uint32_t *isKeyFrame = NULL, FRAME_E type = CRB_FRAME_ANY);

  uint8_t *RequestReadNextFrameStamp(int32_t *frameLen, long long *pTimeStamp, uint32_t *isKeyFrame = NULL,
                                     FRAME_E type = CRB_FRAME_ANY);

  uint8_t *RequestReadContinuousFrames(uint32_t *length, FrameIdx *pIdx = NULL, uint32_t *pFrameCount = NULL);

  int CommitRead(void);
  int CommitRead(uint32_t size);

  void DiscardAllData(void);
  int ResetAllUser(void);
  int WriteLog(const char *pLogBuf, uint32_t size);
  uint8_t *RequestReadPtr(int size);
  uint8_t *RequestWritePtr(int size);
  uint32_t GetWritableSize(void);
  uint32_t GetReadableSize(void);
  uint32_t GetBufferSize(void);

  void DebugBlackHole(const char *reason);
  uint32_t IndexMoveToSpecifiedTime(FRAME_E frame, long long int timeStamp, long long int &gettimeStamp);
  uint32_t IndexMoveToOldest();
  uint32_t IndexFindLatestIFrame(void);

 private:
  void CleanAllConsumer();
  void CleanProductor();
  static int32_t uptime(void);
  int ResetAllUserNoLock(void);

  void locknp(const char *name);
  void lock(const char *name);
  void unlock(void);
  int InstallUser(RingBufUser *pUsers, const char *name, int personality);
  int InstallProductor(int32_t existingSlot, int32_t firstEmptySlot, RingBufUser *pUsers, const char *name,
                       int personality);
  int InstallConsumer(int32_t existingSlot, int32_t firstEmptySlot, RingBufUser *pUsers, const char *name,
                      int personality);
  uint8_t *__RequestWritePtr(uint32_t size, uint16_t alignment, uint16_t *pOutBlockers = NULL,
                             uint16_t *pOutBlockerCount = NULL);
  uint8_t *__RequestReadPtr(uint32_t readOffset, uint32_t writeOffset, int size);

  void MoveConsumer(uint32_t which, uint16_t newIndex, uint32_t newOffset, bool waitIFrame, const char *reason);
  uint8_t SetConsumerMoved(uint32_t which);
  uint8_t ClearConsumerMoved(uint32_t which);
  bool IsConsumerMoved(uint32_t which);
  uint32_t ClearConsumerTimeOut(uint32_t which);
  uint32_t UpdateConsumerTimeOut(uint32_t which);
  uint32_t MoveAllBlockers(uint16_t *blockers, uint16_t blockerCount);
  uint8_t GetConsumerState(uint32_t which);
  void SetConsumerRefIdx(uint32_t which, uint16_t refIdxStart, uint16_t refIdxEnd);
  void SetConsumerState(uint32_t which, uint8_t newState);
  uint32_t InsertBlackHole(const char *blockerName, uint32_t startOffset, uint32_t endOffset);
  uint32_t BlackHoleSortMerge(uint32_t writeOffset, BlackHole *sortedHoles);
  uint32_t DeleteAgedBlackHole(void);
  uint32_t GetSortedBlackHoles(uint32_t writeOffset, BlackHole *sortedHoles);
  uint32_t GetWritableSize(uint32_t readOffset, uint32_t writeOffset);
  uint32_t GetReadableSize(uint32_t readOffset, uint32_t writeOffset);
  uint32_t DeleteBlackHoleOwnedBy(const char *blockerName);
  uint32_t DeleteBlackHoleByIdx(uint8_t holeIdx);
  uint32_t BlackholeJump(uint32_t writeOffset, uint32_t writeLength, uint32_t alignment);
  int IndexFindBlocker(uint16_t *blockers);
  uint32_t FindOldesValidIndex(void);

  uint32_t IndexFindLatestFrame(uint32_t writeIndex, uint32_t readIndex, FRAME_E type);
  uint32_t IndexFindSpecifiedTimeFrameInstance(uint32_t writeIndex, uint32_t readIndex, FRAME_E type,
                                               long long int timeStamp);
  uint32_t IndexAdd(uint32_t offset, uint32_t length, uint8_t type, uint8_t continuous, int32_t requestTime,
                    bool bCorrectResolution, long long frameTime);
  uint32_t IndexFindNextFrame(uint32_t writeIndex, uint32_t readIndex, FRAME_E type, int continuous = 2);
  uint32_t IndexBuildFromOffset(uint32_t offset, uint32_t size, int32_t requestTime, int32_t *pStartTime,
                                int32_t *pEndTime);
  uint32_t GetIntalledUserNum();

 private:
  RingBufInfo *m_pRbInfo;
  uint8_t *m_pDataBuffer;

  RingBufUser *m_pUserInfo;
  int m_fd;
  uint32_t m_memSize;
  uint32_t m_bufSize;
  uint32_t m_userIdx;
  uint32_t m_lastWriteBytes;
  uint8_t m_lastWriteFrameType;
  uint16_t m_lastAlignmentBytes;

  uint32_t m_lastReadBytes;
  uint32_t m_wrapMask;
};
#endif
#endif /* __CRINGBUFF_H__ */