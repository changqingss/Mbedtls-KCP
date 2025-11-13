#include "CRingBuf.h"

#include <assert.h>   //assert
#include <errno.h>    //errno
#include <fcntl.h>    //open
#include <pthread.h>  //pthread_mutex_xxx
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>  //mmap
#include <sys/stat.h>
#include <sys/syscall.h>  //gettid
#include <sys/sysinfo.h>  //uptime
#include <sys/types.h>
#include <unistd.h>

#include "debugLog.h"

#define ENABLE_ACCOUNTING

#define MY_NAME (m_pUserInfo[m_userIdx].name)
#define CRB_DEFAULT_ALIGNMENT (512)
#define IS_CONSUMER (m_pUserInfo == m_pRbInfo->consumers)
#define IS_PRODUCTOR (m_pUserInfo == m_pRbInfo->productors)
/*TODO maybe we should support mutiple productors*/
#define PRODUCTOR_INSTALL_SLOT (0)
#define PRODUCTOR_OFFSET (m_pRbInfo->productors[PRODUCTOR_INSTALL_SLOT].offset)
#define PRODUCTOR_INDEX (m_pRbInfo->productors[PRODUCTOR_INSTALL_SLOT].index)
#define CONSUMER_OFFSET(i) (m_pRbInfo->consumers[i].offset)
#define CONSUMER_INDEX(i) (m_pRbInfo->consumers[i].index)
#define VALID_CONSUMER(idx) (idx >= 0 && idx < MAX_CONSUMER_NUM && m_pRbInfo->consumers[idx].name[0] != 0)
#define PRODUCTOR_EXIST (m_pRbInfo->productors[PRODUCTOR_INSTALL_SLOT].name[0] != 0)

#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))
#define ALIGN(x, a) __ALIGN_MASK(x, (typeof(x))(a)-1)

#define BUFFER_LEN(__start, __end) (((m_bufSize) + (__end) - (__start)) % (m_wrapMask))
#define ADVANCE_OFFSET(__offset, __bytes) (((__offset) + (__bytes)) % (m_wrapMask))
#define ADVANCE_OFFSET2(__offset, __bytes, __bytes1) (((__offset) + (__bytes) + (__bytes1)) % (m_wrapMask))

#define BLACKHOLE_MAX_LIFE_SEC (20)
typedef struct _BlackHole {
  uint32_t startOffset;
  uint32_t endOffset;
  time_t createTime;
  char blockerName[64];
} BlackHole;

#define RB_MAGIC "<_R_B_>"
#define RB_NAME_FORMAT "<%20s>"
#define RB_NAME_VALUE m_pRbInfo->name
#define MAX_USER_NAME_LEN (32)
#define USER_FREE (0)
#define USER_HOLDING_FRAME (1)
#define USER_BLOCKED (2)

#define CRB_PERSONALITY_DECRYPTER (1 << 2)
#define CRB_PERSONALITY_DUMPER (1 << 3)
#define CRB_PERSONALITY_MAX (1 << 4)

#define CRB_ERROR_NONE (0)
#define CRB_ERROR_SPACE (0xFFFFFFFF)
#define CRB_ERROR_PERM (0xFFFFFFFE)
#define CRB_ERROR_PARAM (0xFFFFFFFD)
#define CRB_ERROR_NO_IDX (0xFFFFFFFC)
#define CRB_ERROR_EXIST (0xFFFFFFFB)

#define CRB_DUMP_HEADER (0)
#define CRB_DUMP_IDX_10 (1)
#define CRB_DUMP_IDX_VALID (2)
#define CRB_DUMP_IDX_ALL (3)
#define CRB_DUMP_MAX (4)

#define CRB_COMMIT_LAST_REQ_SIZE ((uint32_t)(-1))

typedef struct _RingBufUser {
  char name[MAX_USER_NAME_LEN];
  /*
   * read or write offset
   */
  uint32_t offset;
  uint32_t index;
  uint32_t personality;
  uint16_t refIdxStart;
  uint16_t refIdxEnd;
  uint8_t state;
  uint8_t movedByProductor : 1;
  uint8_t waitIFrame : 1;
  uint8_t reserved00 : 6;
  int32_t requestUptime;
  uint32_t readTimeOut;
#ifdef ENABLE_ACCOUNTING
  uint32_t installCount : 16;
  uint32_t discardCount : 16;
  uint32_t requestCount;
  uint32_t seekCount;
  uint32_t commitCount;
  uint32_t maxReqComGapTime;
  uint32_t commitWithoutRequestCount;
  uint32_t requestWithoutCommitCount;
  uint32_t blockingCount;
  uint64_t bytesCount;
#endif
} RingBufUser;

#define FRAME_NON_CONTINUOUS (0)
#define FRAME_CONTINUOUS (1)
#define FRAME_CONTINUOUS_DONT_CARE (2)
typedef struct _FrameIndex {
  uint32_t offset;
  uint32_t length;
  uint8_t frameType : 7;
  uint8_t continuous : 1;
  int32_t requestTime;
  long long int timeStamp;
} FrameIndex;

#define MMAP_MIN_PAGE_SIZE (4096)
#ifdef RTSV321_PLATFORM_ALLSDK
#define MAX_CONSUMER_NUM (5)
#else
#define MAX_CONSUMER_NUM (10)
#endif
#define MAX_PRODUCTOR_NUM (1)
#define HEAD_INFO_SIZE (6 * MMAP_MIN_PAGE_SIZE)  // 4

#define MAX_FRAME_NUM (1024 - 128)
#define MAX_RB_NAME_LEN (32)
#define ADVANCE_INDEX(__idx, __step) (((__idx) + (__step)) % (MAX_FRAME_NUM))
#define STEPBACK_INDEX(__idx, __step) (((__idx) + MAX_FRAME_NUM - (__step)) % (MAX_FRAME_NUM))

typedef struct _RingBufInfo {
  /*This can be usefull for debuging tools*/
  uint8_t magic[8];
  uint8_t version;
  uint8_t name[MAX_RB_NAME_LEN];
  uint8_t alignFrameType;
  uint32_t holeCreatedNum;
  uint8_t holeNum;
  uint8_t jumpOverHole;
  uint8_t dataWriten;
  bool liveStream;
  bool videoBuffer;
  uint32_t bufSize;
  uint32_t contiBytesWriten;
  int32_t leftedBytes;
  uint16_t alignBytes;
  pthread_mutex_t rbufferLock;
  RingBufUser consumers[MAX_CONSUMER_NUM];
  RingBufUser productors[MAX_PRODUCTOR_NUM];
  FrameIndex frameIdx[MAX_FRAME_NUM];
  BlackHole holes[MAX_CONSUMER_NUM];
} RingBufInfo;

#define ASSERT_SIZEOF_STRUCT(s, n) typedef char assert_sizeof_struct_##s[(sizeof(s) <= (n)) ? 1 : -1]
/*make sure ring buffer house keeping header size is less than 4 pages*/
ASSERT_SIZEOF_STRUCT(RingBufInfo, HEAD_INFO_SIZE);

int32_t CRingBuf::uptime(void) {
  struct sysinfo sInfo;
  sysinfo(&sInfo);
  return sInfo.uptime;
}

void CRingBuf::locknp(const char *name) {
  DEBUG_ERROR(COMMON_LIB, "lock onwer is dead\n");
  pthread_mutex_consistent_np(&m_pRbInfo->rbufferLock);
}

void CRingBuf::lock(const char *name) {
  if (m_pRbInfo == NULL) {
    DEBUG_FLOW_0(RB_NAME_FORMAT "Ring buffer not initialized yet:%d\n", RB_NAME_VALUE, (uint32_t)&m_pRbInfo);
    return;
  }

  int32_t lockRet = pthread_mutex_lock(&m_pRbInfo->rbufferLock);
  if (0 == lockRet) {
    /*locked without error*/
  } else if (EOWNERDEAD == lockRet) {
    locknp(name);
  } else {
    DEBUG_ERROR(COMMON_LIB, "lock return %d\n", lockRet);
  }
}

void CRingBuf::unlock(void) {
  if (m_pRbInfo == NULL) {
    DEBUG_FLOW_0("Ring buffer not initialized yet:%d\n", (uint32_t)&m_pRbInfo);
    return;
  }

  pthread_mutex_unlock(&m_pRbInfo->rbufferLock);
}

int CRingBuf::InstallConsumer(int32_t existingSlot, int32_t firstEmptySlot, RingBufUser *pUsers, const char *name,
                              int personality) {
  m_userIdx = existingSlot;
  printf("InstallConsumer name:%s, pUsers:%p\n", name, pUsers);
  if (existingSlot == -1) {
    m_userIdx = firstEmptySlot;
    assert(strlen(name) > 0);
    assert(pUsers != NULL);
    memset(pUsers[m_userIdx].name, 0x00, sizeof(pUsers[m_userIdx].name));
    strncpy(pUsers[m_userIdx].name, name, sizeof(pUsers[m_userIdx].name));
    pUsers[m_userIdx].personality = personality;
  }
  printf("InstallConsumer m_userIdx:%d,\n", m_userIdx);
  DeleteBlackHoleOwnedBy(name);
  ClearConsumerTimeOut(m_userIdx);
  SetConsumerState(m_userIdx, USER_FREE);
  ClearConsumerMoved(m_userIdx);
  printf("InstallConsumer m_pRbInfo->dataWriten:%d\n", m_pRbInfo->dataWriten);

  if (PRODUCTOR_EXIST && 0 != m_pRbInfo->dataWriten) {
    if (personality & CRB_PERSONALITY_DUMPER) {
      uint32_t idx = FindOldesValidIndex();
      MoveConsumer(m_userIdx, idx, m_pRbInfo->frameIdx[idx].offset, false, "InsDumper");
      printf("Install dumper @ %d\n", idx);
    } else {
      uint32_t idx = IndexFindLatestIFrame();
      if (CRB_ERROR_NO_IDX != idx) {
        MoveConsumer(m_userIdx, idx, m_pRbInfo->frameIdx[idx].offset, false, "InsLIFrame");
        printf("Productor exist and I frame found %d\n", idx);
      } else {
        idx = STEPBACK_INDEX(PRODUCTOR_INDEX, 1);
        MoveConsumer(m_userIdx, PRODUCTOR_INDEX, m_pRbInfo->frameIdx[PRODUCTOR_INDEX].offset, true, "InsPro-1");
        printf("Productor exist but not I frame found\n");
      }
    }
  } else {
    MoveConsumer(m_userIdx, 0, 0, true, "InsNoData");
    printf("InsNoData\n");
  }
#ifdef ENABLE_ACCOUNTING
  pUsers[m_userIdx].installCount++;
#endif
  printf("[%s] installed in slot #%02d (exist=%02d first=%02d) index=%05d offset=%08d\n", pUsers[m_userIdx].name,
         m_userIdx, existingSlot, firstEmptySlot, pUsers[m_userIdx].index, pUsers[m_userIdx].offset);
  return 0;
}

int CRingBuf::InstallProductor(int32_t existingSlot, int32_t firstEmptySlot, RingBufUser *pUsers, const char *name,
                               int personality) {
  m_userIdx = existingSlot;
  DEBUG_FLOW_0("Try to install %s exist=%02d first=%02d\n", name, existingSlot, firstEmptySlot);
  if (existingSlot == -1) {
    assert(strlen(name) > 0);
    assert(pUsers != NULL);
    m_userIdx = firstEmptySlot;
    assert(firstEmptySlot == PRODUCTOR_INSTALL_SLOT);

    PRODUCTOR_INDEX = 0;
    PRODUCTOR_OFFSET = 0;
    m_pRbInfo->dataWriten = 0;
    m_pRbInfo->contiBytesWriten = 0;
    strncpy(pUsers[m_userIdx].name, name, sizeof(pUsers[m_userIdx].name));
    pUsers[m_userIdx].personality = personality;
  }
  assert(m_userIdx == PRODUCTOR_INSTALL_SLOT);
#if 1
  /*
   * If we DONT reset all consumer, the exist data/idx/hole may screw up the productor
   */
  if (IS_PRODUCTOR && existingSlot != -1) {
    ResetAllUserNoLock();
  }
#endif
#ifdef ENABLE_ACCOUNTING
  pUsers[m_userIdx].installCount++;
#endif
  DEBUG_FLOW_0("[%s] installed in slot #%02d (exist=%02d first=%02d) index=%05d offset=%08d\n", pUsers[m_userIdx].name,
               m_userIdx, existingSlot, firstEmptySlot, pUsers[m_userIdx].index, pUsers[m_userIdx].offset);
  return 0;
}

int CRingBuf::InstallUser(RingBufUser *pUsers, const char *name, int personality) {
  int max_count = MAX_CONSUMER_NUM;
  lock(name);
  if (IS_PRODUCTOR) {
    max_count = MAX_PRODUCTOR_NUM;
  }
  int32_t existingSlot = -1;
  int32_t firstEmptySlot = -1;
  for (int i = 0; i < max_count; i++) {
    printf("i = %d, pUsers[i].name[0] = %s, name = %s, pUsers = %p\n", i, &pUsers[i].name[0], name, pUsers);
    if (pUsers[i].name[0] != 0 && 0 == strcmp(&pUsers[i].name[0], name)) {
      existingSlot = i;
      break;
    }
    if (firstEmptySlot == -1 && pUsers[i].name[0] == 0) {
      firstEmptySlot = i;
    }
  }
  printf("%d %d firstEmptySlot:%02d existingSlot:%02d\n", IS_PRODUCTOR, IS_CONSUMER, firstEmptySlot, existingSlot);
  // assert(existingSlot * firstEmptySlot <= 0);

  if (firstEmptySlot == -1 && existingSlot == -1) {
    printf("error firstEmptySlot == -1 && existingSlot == -1");
    return -1;
  }

  if (IS_CONSUMER) {
    InstallConsumer(existingSlot, firstEmptySlot, pUsers, name, personality);
  } else {
    InstallProductor(existingSlot, firstEmptySlot, pUsers, name, personality);
  }
  unlock();
  return 0;
}

CRingBuf::CRingBuf(const char *usrName, const char *ringBufName, int size, int personality, bool liveStream) {
  printf("Install %s On %s\n", usrName, ringBufName);
  if (!usrName || strlen(usrName) <= 0 || !ringBufName || strlen(ringBufName) <= 0 || size <= 0 ||
      ((personality & (CRB_PERSONALITY_WRITER | CRB_PERSONALITY_READER)) ==
       (CRB_PERSONALITY_WRITER | CRB_PERSONALITY_READER)) ||
      ((personality & (CRB_PERSONALITY_WRITER | CRB_PERSONALITY_READER)) == 0)) {
    printf("Invalid param usrName:%s ringBufName:%s size=%d personality=0x%x\n", usrName, ringBufName, size,
           personality);
    return;
  }
  assert(usrName != NULL);
  assert(strlen(usrName) > 0);
  assert(strlen(usrName) < MAX_USER_NAME_LEN);
  assert(ringBufName != NULL);
  assert(strlen(ringBufName) > 0);
  assert(strlen(ringBufName) < MAX_RB_NAME_LEN);
  assert(size > 0);
  assert((personality & (CRB_PERSONALITY_WRITER | CRB_PERSONALITY_READER)) !=
         (CRB_PERSONALITY_WRITER | CRB_PERSONALITY_READER));
  assert((personality & (CRB_PERSONALITY_WRITER | CRB_PERSONALITY_READER)) != 0);

  m_pRbInfo = NULL;
  m_pDataBuffer = NULL;
  m_pUserInfo = NULL;

  /*Data buffer size must be N*4096*/
  m_bufSize = ALIGN(size, MMAP_MIN_PAGE_SIZE);
  m_wrapMask = m_bufSize;

  printf("User need %d(0x%x) bytes, we allocated %d(0x%x) bytes mask=0x%x\n", size, size, m_bufSize, m_bufSize,
         m_wrapMask);

  m_memSize = HEAD_INFO_SIZE + m_bufSize;
  m_lastWriteBytes = 0;
  m_lastAlignmentBytes = 0;
  m_lastReadBytes = 0;
  m_lastWriteFrameType = CRB_FRAME_I_SLICE;
  printf("[%d] m_lastWriteFrameType:%d\n", __LINE__, m_lastWriteFrameType);

  /* 1. 在tmpfs中创建大小为m_memSize的backend文件*/
  char filename[64];
  snprintf(filename, sizeof(filename), "/tmp/%s", ringBufName);
  m_fd = open(filename, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

  if (m_fd < 0) {
    printf("%s Open %s failed:%s\n", ringBufName, filename, strerror(errno));
    return;
  }
  if (-1 == ftruncate(m_fd, m_memSize)) {
    printf("%s ftruncate %s failed:%s\n", ringBufName, filename, strerror(errno));
    return;
  }

  /*let os to choose a proper address*/
  uint8_t *address =
      (uint8_t *)mmap(NULL, m_memSize + m_bufSize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (MAP_FAILED == address) {
    printf("%s mmap %s #0 faild:%s\n", ringBufName, filename, strerror(errno));
    return;
  }
  printf("OS address:0x%08x\n", address);

  /*
   * Map the ringbuffer header region
   * this should be RW for both productor and consumer
   */
  uint8_t *pAddrHeader =
      (uint8_t *)mmap(address, HEAD_INFO_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, m_fd, 0);
  if (MAP_FAILED == pAddrHeader || address != pAddrHeader) {
    printf("%s mmap %s #1 faild: 0x%08x 0x%08x %s\n", ringBufName, filename, (int32_t)address, (int32_t)pAddrHeader,
           strerror(errno));
    return;
  }

  int prot = PROT_READ;
  if (personality & CRB_PERSONALITY_WRITER || personality & CRB_PERSONALITY_DECRYPTER) {
    prot = PROT_WRITE;
  }
  uint8_t *pAddrBuf0 =
      (uint8_t *)mmap(pAddrHeader + HEAD_INFO_SIZE, m_bufSize, prot, MAP_SHARED | MAP_FIXED, m_fd, HEAD_INFO_SIZE);
  if (MAP_FAILED == pAddrBuf0 || pAddrHeader + HEAD_INFO_SIZE != pAddrBuf0) {
    printf("%s mmap %s #2 faild: 0x%08x 0x%08x %s\n", ringBufName, filename, (int32_t)address, (int32_t)pAddrBuf0,
           strerror(errno));
    return;
  }
  /*mirror the data buffer*/
  uint8_t *pAddrBuf1 =
      (uint8_t *)mmap(pAddrBuf0 + m_bufSize, m_bufSize, prot, MAP_SHARED | MAP_FIXED, m_fd, HEAD_INFO_SIZE);
  if (MAP_FAILED == pAddrBuf1 || pAddrBuf1 != pAddrBuf0 + m_bufSize) {
    printf("%s mmap %s #3 faild: 0x%08x 0x%08x %s\n", ringBufName, filename, (int32_t)(pAddrBuf0 + m_bufSize),
           (int32_t)pAddrBuf1, strerror(errno));
    return;
  }
  printf("\nHEADER:0x%08x-0x%08x[%05d]\nBUF0  :0x%08x-0x%08x[%05d]\nBUF1  :0x%08x-0x%08x[%05d]\n",
         (uint32_t)pAddrHeader, (uint32_t)pAddrHeader + HEAD_INFO_SIZE, HEAD_INFO_SIZE, (uint32_t)pAddrBuf0,
         (uint32_t)pAddrBuf0 + m_bufSize, m_bufSize, (uint32_t)pAddrBuf1, (uint32_t)pAddrBuf1 + m_bufSize, m_bufSize);

/*Defend ring buffer backend file against write system call*/
#define INVALID_FILE_OFFSET (0xDeadBeef)
  lseek(m_fd, INVALID_FILE_OFFSET, SEEK_SET);

  m_pRbInfo = (RingBufInfo *)pAddrHeader;

  m_pDataBuffer = (uint8_t *)(((uint8_t *)pAddrBuf0));

  int binit = 0;
  /* 初始化共享缓存上面的各个对象 */
  if (0 == m_pRbInfo->version) {
    m_pRbInfo->version = 1;
    m_pRbInfo->videoBuffer = true;
    binit = 1;
    /* 初始化mutex */
    int ret = 0;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    ret = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    if (ret) {
      printf("%s %s set mutex process shared failed %s\n", ringBufName, usrName, strerror(errno));
      return;
    }
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutexattr_setrobust_np(&attr, PTHREAD_MUTEX_ROBUST);
    pthread_mutex_init(&m_pRbInfo->rbufferLock, &attr);
    printf("Ring buffer lock of %s initialized by %s\n", ringBufName, usrName);
  }

  uint32_t installedUser = GetIntalledUserNum();
  printf("personality:%d installedUser:%d\n", personality, installedUser);

  if (installedUser >= 10) CleanAllConsumer();

  if (personality & CRB_PERSONALITY_WRITER) {
    m_pUserInfo = m_pRbInfo->productors;
    m_pRbInfo->liveStream = liveStream;
    m_pRbInfo->leftedBytes = 0;
    if (0 == binit) {
      CleanProductor();
      CleanAllConsumer();
    }
  } else {
    m_pUserInfo = m_pRbInfo->consumers;
  }
  m_pRbInfo->bufSize = m_bufSize;
  m_pRbInfo->alignBytes = CRB_DEFAULT_ALIGNMENT;
  m_pRbInfo->alignFrameType = CRB_FRAME_I_SLICE;
  memcpy(m_pRbInfo->magic, RB_MAGIC, sizeof(m_pRbInfo->magic));
  strncpy((char *)m_pRbInfo->name, ringBufName, strlen(ringBufName));
  InstallUser(m_pUserInfo, usrName, personality);
  return;
}

uint32_t CRingBuf::GetIntalledUserNum() {
  uint32_t userNum = 0;
  PRODUCTOR_EXIST ? (userNum = 1) : (userNum = 0);  // clear pclint warning
  for (int i = 0; i < MAX_CONSUMER_NUM; i++) {
    if (VALID_CONSUMER(i)) {
      userNum += 1;
    }
  }
  return userNum;
}

void CRingBuf::CleanAllConsumer() {
  printf("CleanAllConsumer begin \n");
  int max_count = MAX_CONSUMER_NUM;
  int i = 0;
  lock(MY_NAME);
  for (i = 0; i < max_count; i++) {
    if (m_pRbInfo->consumers[i].name[0] != 0) {
      memset(&m_pRbInfo->consumers[i], 0x00, sizeof(m_pRbInfo->consumers[i]));
    }
  }
  unlock();
  printf("CleanAllConsumer end\n");
}

void CRingBuf::CleanProductor() {
  printf("CleanProductor begin 1\n");
  int max_count = MAX_PRODUCTOR_NUM;
  int i = 0;
  lock(MY_NAME);
  printf("CleanProductor begin 2\n");
  for (i = 0; i < max_count; i++) {
    memset(&m_pRbInfo->productors[i], 0x00, sizeof(m_pRbInfo->productors[i]));
  }
  printf("CleanProductor begin 3\n");
  unlock();
  printf("CleanProductor begin 4\n");
  printf("CleanProductor end\n");
}

CRingBuf::~CRingBuf() {
  lock(MY_NAME);
  char filename[64];
  snprintf(filename, sizeof(filename), "/tmp/%s", m_pRbInfo->name);
  memset(&m_pUserInfo[m_userIdx], 0x00, sizeof(m_pUserInfo[m_userIdx]));
  printf("m_pRbInfo->liveStream = %d\n", m_pRbInfo->liveStream);
  if (m_pRbInfo->liveStream) {
    unlock();
  } else {
    /*Clear user info saved in tmp file*/
    memset(&m_pUserInfo[m_userIdx], 0x00, sizeof(m_pUserInfo[m_userIdx]));

    uint32_t installedUser = GetIntalledUserNum();
    if (installedUser == 0) {
      /*
       *Since we are the last user and we are going to delete the buffer
       *It's ok not to unlock it
       */
      printf("Deleting %s\n", filename);
      unlink(filename);
    } else {
      printf("useing %s\n", filename);
      unlock();
    }
  }

  /*Relase Process owned resource*/
  uint8_t *address = (uint8_t *)m_pRbInfo;
  if (munmap((address), HEAD_INFO_SIZE + 2 * m_bufSize)) {
    printf("munmap %s 0x%08x failed %s\n", filename, (int32_t *)address, strerror(errno));
  }
  close(m_fd);

  m_pRbInfo = NULL;
  m_pDataBuffer = NULL;
  m_pUserInfo = NULL;
  m_lastWriteBytes = 0;
  m_lastAlignmentBytes = 0;
  m_lastReadBytes = 0;
  m_fd = -1;
}

bool CRingBuf::IsConsumerMoved(uint32_t which) {
  return (0 == m_pRbInfo->consumers[which].movedByProductor) ? false : true;
}

uint8_t CRingBuf::ClearConsumerMoved(uint32_t which) {
  // assert(VALID_CONSUMER(which));
  m_pRbInfo->consumers[which].movedByProductor = 0;
  return 0;
}
uint8_t CRingBuf::SetConsumerMoved(uint32_t which) {
  // assert(VALID_CONSUMER(which));
  m_pRbInfo->consumers[which].movedByProductor = 1;
#ifdef ENABLE_ACCOUNTING
  m_pRbInfo->consumers[which].blockingCount++;
  ;
#endif
  return 0;
}
uint32_t CRingBuf::ClearConsumerTimeOut(uint32_t which) {
  if (VALID_CONSUMER(which)) {
    m_pRbInfo->consumers[which].readTimeOut = 0;
  }
  return 0;
}
uint32_t CRingBuf::UpdateConsumerTimeOut(uint32_t which) {
  if (VALID_CONSUMER(which)) {
    return ++(m_pRbInfo->consumers[which].readTimeOut);
  }
  return 0;
}

uint8_t CRingBuf::GetConsumerState(uint32_t which) {
  // assert(VALID_CONSUMER(which));
  return m_pRbInfo->consumers[which].state;
}

void CRingBuf::SetConsumerState(uint32_t which, uint8_t newState) {
  // assert(VALID_CONSUMER(which));
  m_pRbInfo->consumers[which].state = newState;
  // printf("Set consumer#%02d state=%02d\n", which, newState);
}

void CRingBuf::SetConsumerRefIdx(uint32_t which, uint16_t refIdxStart, uint16_t refIdxEnd) {
  // assert(VALID_CONSUMER(which));
  m_pRbInfo->consumers[which].refIdxStart = refIdxStart;
  m_pRbInfo->consumers[which].refIdxEnd = refIdxEnd;
  DEBUG_FLOW_5("Set consumer#%02d ref %05d-%05d\n", which, refIdxStart, refIdxEnd);
}
void CRingBuf::MoveConsumer(uint32_t which, uint16_t newIndex, uint32_t newOffset, bool waitIFrame,
                            const char *reason) {
#if 0
    uint32_t readOffset = m_pRbInfo->consumers[which].offset;
    uint16_t readIndex = m_pRbInfo->consumers[which].index;
    if (!VALID_CONSUMER(which) || newIndex >= MAX_FRAME_NUM || newOffset >= m_bufSize)
    {
        while(1)
        {
            DEBUG_FLOW_5("ykMoving consumer#%02d(%20s) %05d->%05d offset %05d->%05d (%s)\n", 
                    which, m_pRbInfo->consumers[which].name, readIndex, newIndex, readOffset, newOffset, reason);
            sleep(1);
        }
    }
#endif
#if 0
    uint32_t readOffset = m_pRbInfo->consumers[which].offset;
    uint16_t readIndex = m_pRbInfo->consumers[which].index;
    DEBUG_FLOW_5("Moving consumer#%02d(%20s) %05d->%05d offset %05d->%05d (%s)\n", 
            which, m_pRbInfo->consumers[which].name, readIndex, newIndex, readOffset, newOffset, reason);
#endif
  if (RB_NAME_VALUE[0] != 'I') /*Do not debug IPCAgent recv ring buffer*/
  {
    DEBUG_FLOW_5(RB_NAME_FORMAT "%s Idx %d->%d waitIFrame %d (%s)\n", RB_NAME_VALUE, m_pRbInfo->consumers[which].name,
                 m_pRbInfo->consumers[which].index, newIndex, waitIFrame, reason);
  }
  // assert(VALID_CONSUMER(which));
  // assert(newIndex < MAX_FRAME_NUM);
  // assert(newOffset < m_bufSize);

  m_pRbInfo->consumers[which].index = newIndex;
  m_pRbInfo->consumers[which].offset = newOffset;
  SetConsumerRefIdx(which, newIndex, newIndex);
  if (waitIFrame) {
    m_pRbInfo->consumers[which].waitIFrame = 1;
  } else {
    m_pRbInfo->consumers[which].waitIFrame = 0;
  }
}

#define COMMIT_LAST_REQUESTED_SIZE ((uint32_t)(-1))
int CRingBuf::CommitRead(void) { return CommitRead(COMMIT_LAST_REQUESTED_SIZE); }
int CRingBuf::CommitRead(uint32_t size) {
  assert(IS_CONSUMER);
  /*assert(m_lastReadBytes != 0);*/
  assert(size <= m_lastReadBytes || size == COMMIT_LAST_REQUESTED_SIZE);

  uint32_t commitBytes = size;
  if (size == COMMIT_LAST_REQUESTED_SIZE) {
    commitBytes = m_lastReadBytes;
  }

  lock(MY_NAME);
#ifdef ENABLE_ACCOUNTING
  m_pRbInfo->consumers[m_userIdx].commitCount++;
  if (m_lastReadBytes == 0) {
    m_pRbInfo->consumers[m_userIdx].commitWithoutRequestCount++;
  }
  m_pRbInfo->consumers[m_userIdx].bytesCount += commitBytes;
#endif
  DeleteBlackHoleOwnedBy(m_pRbInfo->consumers[m_userIdx].name);
  ClearConsumerTimeOut(m_userIdx);

  if (IS_PRODUCTOR || 0 == m_lastReadBytes) {
    DEBUG_FLOW_5("IS_PRODUCTOR=%d m_lastReadBytes=%05d\n", IS_PRODUCTOR, m_lastReadBytes);
    unlock();
    return CRB_ERROR_PERM;
  }
  if (IsConsumerMoved(m_userIdx)) {
    /*
     * When consumer blocks, the productor is responsible for
     * moving it to the next I frame.
     * So consumer SHOULD do noting here
     */
    DEBUG_FLOW_5("offset moved consumer#%d\n", m_userIdx);
  } else {
    /*
     * 1. If consumer skips some frame, then it must be done in RequestReadFrame
     *    CommitRead always advance only a single index
     * 2. If consumer is doing continuous read, we will advance to refIdxEnd+1
     */
    MoveConsumer(m_userIdx, ADVANCE_INDEX(m_pRbInfo->consumers[m_userIdx].refIdxEnd, 1),
                 ADVANCE_OFFSET(m_pRbInfo->consumers[m_userIdx].offset, commitBytes), false, "CommitRead");
  }
  m_lastReadBytes = 0;
  SetConsumerState(m_userIdx, USER_FREE);
  ClearConsumerMoved(m_userIdx);
  unlock();
  return CRB_ERROR_NONE;
}

uint8_t *CRingBuf::__RequestReadPtr(uint32_t readOffset, uint32_t writeOffset, int size) {
  /* Not initialized yet */
  if (NULL == m_pDataBuffer || m_userIdx > MAX_CONSUMER_NUM || size == 0 || IS_PRODUCTOR) {
    DEBUG_FLOW_0(RB_NAME_FORMAT "buff=0x%x m_userIdx=%d size=%d IS_PRODUCTOR=%d\n", RB_NAME_VALUE,
                 (int32_t)m_pDataBuffer, m_userIdx, size, IS_PRODUCTOR);
    return NULL;
  }

  if (writeOffset == readOffset) {
    return NULL;
  }

  int32_t readableSize = GetReadableSize(readOffset, writeOffset);
  if (readableSize >= size) {
    m_lastReadBytes = size;
    return m_pDataBuffer + readOffset;
  }

  DEBUG_FLOW_5("Not enough data m_userIdx=%d RO=%d WO=%d ask=%d avaiblable=%d\n", m_userIdx, readOffset, writeOffset,
               size, readableSize);
  return NULL;
}

uint8_t *CRingBuf::RequestReadPtr(int size) {
  assert(size > 0);
  uint8_t *ret = (uint8_t *)CRB_ERROR_PARAM;
  uint32_t writeOffset = PRODUCTOR_OFFSET;
  uint32_t readOffset = m_pRbInfo->consumers[m_userIdx].offset;
  ret = __RequestReadPtr(readOffset, writeOffset, size);
  return ret;
}

/*TODO refactor needed*/
uint32_t CRingBuf::IndexFindNextFrame(uint32_t writeIndex, uint32_t readIndex, FRAME_E type,
                                      int continuous /*Default == FRAME_CONTINUOUS_DONT_CARE*/) {
  DEBUG_FLOW_5(RB_NAME_FORMAT "IndexFindNextFrame indexes rIdx=%05d wIdx=%05d type=%02d continuous=%02d\n",
               RB_NAME_VALUE, readIndex, writeIndex, type, continuous);
  assert(readIndex < MAX_FRAME_NUM);
  assert(writeIndex < MAX_FRAME_NUM);
#if 0
    /*This code can do valid frame index checking*/

    uint32_t oldestIdx = FindOldesValidIndex(); 
    uint32_t oldest = oldestIdx; 
    bool meetConsumer  = false;
    bool meetProductor = false;
    for (int i = 0; i < MAX_FRAME_NUM; i++)
    {
        if (oldest == readIndex)
        {
            meetConsumer    = true;
        }
        if (oldest == writeIndex)
        {
            meetProductor   = true;
        }
        if (meetProductor && !meetConsumer)
        {
            DEBUG_FLOW_0(RB_NAME_FORMAT"IndexFindNextFrame indexes oldest=%05d rIdx=%05d wIdx=%05d type=%02d continuous=%02d\n", 
                    RB_NAME_VALUE, oldest, readIndex, writeIndex, type, continuous);
            assert(0);
        }
        if (meetProductor && meetConsumer)
        {
            break;
        }
        oldest = ADVANCE_INDEX(oldest, 1);
    }
#endif
  uint32_t idx = readIndex;
  uint32_t end = writeIndex;
  FrameIndex *idxes = m_pRbInfo->frameIdx;

  if (readIndex == writeIndex) {
    goto NO_MORE_INDEX;
  }
  while (idx != end) {
    DEBUG_FLOW_5(RB_NAME_FORMAT "IndexFindNextFrame idx=%05d [%c][%c]%05d+%08d\n", RB_NAME_VALUE, idx,
                 (idxes[idx].frameType == CRB_FRAME_I_SLICE) ? 'I' : 'P',
                 (idxes[idx].continuous == FRAME_NON_CONTINUOUS) ? 'N' : 'C', idxes[idx].offset, idxes[idx].length);
    if (continuous == FRAME_CONTINUOUS_DONT_CARE) {
      if (CRB_FRAME_ANY == type || idxes[idx].frameType == type) {
        return idx;
      }
    } else {
      assert(CRB_FRAME_ANY == type);
      assert(FRAME_NON_CONTINUOUS == continuous);
      /*
       * Typically a alignFrameType will be aligned to CRB_DEFAULT_ALIGNMENT
       * Except that when writable memory is already aligned
       *
       * Record module can not handle mutiple I frame right now
       * */
      if (idxes[idx].continuous == continuous || idxes[idx].frameType == m_pRbInfo->alignFrameType) {
        return idx;
      }
    }
    idx = ADVANCE_INDEX(idx, 1);
  }

NO_MORE_INDEX:
  DEBUG_FLOW_5("IndexFindNextFrame NO_IDX indexes rIdx=%05d wIdx=%05d\n", readIndex, writeIndex);
  return CRB_ERROR_NO_IDX;
}

uint32_t CRingBuf::IndexFindLatestFrame(uint32_t writeIndex, uint32_t readIndex, FRAME_E type) {
  DEBUG_FLOW_5("IndexFindLatestFrame indexes rIdx=%05d wIdx=%05d\n", readIndex, writeIndex);
  assert(readIndex < MAX_FRAME_NUM);
  assert(writeIndex < MAX_FRAME_NUM);
  // assert(m_pRbInfo->dataWriten);
  if (!m_pRbInfo->dataWriten) {
    return CRB_ERROR_NO_IDX;
  }
  uint32_t idx = STEPBACK_INDEX(writeIndex, 1);
  uint32_t end = readIndex;
  FrameIndex *idxes = m_pRbInfo->frameIdx;
  if (readIndex == writeIndex) {
    goto NO_MORE_INDEX;
  }
  while (idx != end) {
    DEBUG_FLOW_5("IndexFindLatestFrame idx=%05d [%c][%c]%05d+%08d\n", idx,
                 (idxes[idx].frameType == CRB_FRAME_I_SLICE) ? 'I' : 'P',
                 (idxes[idx].continuous == FRAME_NON_CONTINUOUS) ? 'N' : 'C', idxes[idx].offset, idxes[idx].length);
    if (CRB_FRAME_ANY == type || idxes[idx].frameType == type) {
      return idx;
    }
    idx = STEPBACK_INDEX(idx, 1);
  }
NO_MORE_INDEX:
  DEBUG_FLOW_5("IndexFindLatestFrame NO_IDX indexes rIdx=%05d wIdx=%05d\n", readIndex, writeIndex);
  return CRB_ERROR_NO_IDX;
}

uint32_t CRingBuf::IndexFindSpecifiedTimeFrameInstance(uint32_t writeIndex, uint32_t readIndex, FRAME_E type,
                                                       long long int timeStamp) {
  DEBUG_FLOW_0("IndexFindLatestFrame indexes rIdx=%05d wIdx=%05d, dataWriten:%u\n", readIndex, writeIndex,
               m_pRbInfo->dataWriten);
  assert(readIndex < MAX_FRAME_NUM);
  assert(writeIndex < MAX_FRAME_NUM);

  uint32_t idx = STEPBACK_INDEX(writeIndex, 1);
  uint32_t end = readIndex;
  uint32_t closest_idx = 0;
  long long int timeStamp1 = 0;
  FrameIndex *idxes = m_pRbInfo->frameIdx;

  if (m_pRbInfo->dataWriten <= 0) {
    goto NO_MORE_INDEX;
  }

  if (readIndex == writeIndex) {
    goto NO_MORE_INDEX;
  }
  while (idx != end) {
    DEBUG_FLOW_5("IndexFindLatestFrame idx=%05d [%c][%c]%05d+%08d\n", idx,
                 (idxes[idx].frameType == CRB_FRAME_I_SLICE) ? 'I' : 'P',
                 (idxes[idx].continuous == FRAME_NON_CONTINUOUS) ? 'N' : 'C', idxes[idx].offset, idxes[idx].length);
    timeStamp1 = (long long int)idxes[idx].timeStamp;
    // printf("ringbuffer type:%d timeStamp:%lld %lld %lld\n", idxes[idx].frameType, idxes[idx].timeStamp, timeStamp,
    // timeStamp1);

    if (CRB_FRAME_ANY == type || idxes[idx].frameType == type) {
      closest_idx = idx;
      if (idxes[idx].timeStamp != 0 && timeStamp1 <= timeStamp) return idx;
    }
    idx = STEPBACK_INDEX(idx, 1);
  }
  if (idx == 0) {
    DEBUG_FLOW_5("IndexFindLatestFrame idx=%05d [%c][%c]%05d+%08d\n", idx,
                 (idxes[idx].frameType == CRB_FRAME_I_SLICE) ? 'I' : 'P',
                 (idxes[idx].continuous == FRAME_NON_CONTINUOUS) ? 'N' : 'C', idxes[idx].offset, idxes[idx].length);
    DEBUG_FLOW_5("[%d] timeStamp:%lld\n", __LINE__, idxes[idx].timeStamp);
    timeStamp1 = (long long int)idxes[idx].timeStamp;
    if (CRB_FRAME_ANY == type || idxes[idx].frameType == type) {
      closest_idx = idx;
      if (idxes[idx].timeStamp != 0 && timeStamp1 <= timeStamp) return idx;
    }
  }
  if (closest_idx >= 0) return closest_idx;
NO_MORE_INDEX:
  DEBUG_FLOW_5("IndexFindLatestFrame NO_IDX indexes rIdx=%05d wIdx=%05d\n", readIndex, writeIndex);
  return CRB_ERROR_NO_IDX;
}

uint32_t CRingBuf::FindOldesValidIndex(void) {
  uint32_t from = STEPBACK_INDEX(PRODUCTOR_INDEX, 1);
  uint32_t bufLen = 0;
  uint32_t lastBufLen = 0;
  if (0 == m_pRbInfo->dataWriten) {
    DEBUG_FLOW_0(RB_NAME_FORMAT " Hi no data yet, take %d\n", RB_NAME_VALUE, PRODUCTOR_INDEX);
    return PRODUCTOR_INDEX;
  }
  do {
    bufLen = BUFFER_LEN(m_pRbInfo->frameIdx[from].offset, PRODUCTOR_OFFSET);
    if (bufLen <= lastBufLen) {
      break;
    }
    lastBufLen = bufLen;
    from = STEPBACK_INDEX(from, 1);
  } while (from != PRODUCTOR_INDEX);

  return ADVANCE_INDEX(from, 1);
}

uint32_t CRingBuf::IndexFindLatestIFrame(void) {
  uint32_t oldestIdx = FindOldesValidIndex();
  DEBUG_FLOW_0("IndexFindLatestIFrame %05d -- %05d\n", PRODUCTOR_INDEX, oldestIdx);
  return IndexFindLatestFrame(PRODUCTOR_INDEX, oldestIdx, CRB_FRAME_I_SLICE);
}

uint32_t CRingBuf::IndexMoveToSpecifiedTime(FRAME_E frame, long long int timeStamp, long long int &gettimeStamp) {
  uint32_t oldestIdx = CRB_ERROR_NO_IDX, getIdx = CRB_ERROR_NO_IDX;
  int iret = -1;

  lock(MY_NAME);
  oldestIdx = FindOldesValidIndex();
  DEBUG_FLOW_0("IndexFindLatestIFrame %05d -- %05d\n", PRODUCTOR_INDEX, oldestIdx);
  getIdx = IndexFindSpecifiedTimeFrameInstance(PRODUCTOR_INDEX, oldestIdx, frame, timeStamp);
  if (getIdx != CRB_ERROR_NO_IDX) {
    DeleteBlackHoleOwnedBy(m_pRbInfo->consumers[m_userIdx].name);
    ClearConsumerTimeOut(m_userIdx);
    MoveConsumer(m_userIdx, getIdx, m_pRbInfo->frameIdx[getIdx].offset, true, "Discard");
    gettimeStamp = m_pRbInfo->frameIdx[getIdx].timeStamp;
    iret = 0;
  }
  unlock();

  return iret;
}

uint32_t CRingBuf::IndexMoveToOldest() {
  uint32_t oldestIdx = CRB_ERROR_NO_IDX, getIdx = CRB_ERROR_NO_IDX;
  int iret = -1;

  lock(MY_NAME);
  oldestIdx = FindOldesValidIndex();
  DEBUG_FLOW_0("IndexFindLatestIFrame %05d -- %05d\n", PRODUCTOR_INDEX, oldestIdx);
  getIdx = oldestIdx;
  if (getIdx != CRB_ERROR_NO_IDX) {
    DeleteBlackHoleOwnedBy(m_pRbInfo->consumers[m_userIdx].name);
    ClearConsumerTimeOut(m_userIdx);
    MoveConsumer(m_userIdx, getIdx, m_pRbInfo->frameIdx[getIdx].offset, true, "Discard");
    iret = 0;
  }
  unlock();

  return iret;
}

int CRingBuf::IndexFindBlocker(uint16_t *blockers) {
  uint16_t readIndex = 0;
  int count = 0;
  for (int i = 0; i < MAX_CONSUMER_NUM; i++) {
    if (VALID_CONSUMER(i)) {
      readIndex = m_pRbInfo->consumers[i].index;
      if (ADVANCE_INDEX(PRODUCTOR_INDEX, 1) == readIndex) {
        if (USER_FREE != m_pRbInfo->consumers[i].state) {
          DEBUG_FLOW_1(RB_NAME_FORMAT "Found a idx blocker:reader#%2d index=%05d product index=%05d\n", RB_NAME_VALUE,
                       i, readIndex, PRODUCTOR_INDEX);
        }
        if (blockers) {
          blockers[count] = i;
        }
        count++;
      }
    }
  }
  return count;
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

uint32_t CRingBuf::IndexAdd(uint32_t offset, uint32_t length, uint8_t type, uint8_t continuous, int32_t requestTime,
                            bool bCorrectResolution, long long frameTime) {
  assert(IS_PRODUCTOR);
  /*
   * Productor will make sure there is free index slot
   * by calling IndexFindBlocker in RequestWriteFrame
   * no need to check again
   */
  /*if (0 == strcmp((char *)RB_NAME_VALUE, "CH0-0-V-Stream"))*/
  {
    DEBUG_FLOW_1(RB_NAME_FORMAT "PID#%d IndexAdd #%05d %08d [%c][%c]%05d+%08d (%d+%d %d)\n", RB_NAME_VALUE,
                 syscall(SYS_gettid), PRODUCTOR_INDEX, requestTime, (type == CRB_FRAME_I_SLICE) ? 'I' : 'P',
                 (continuous == FRAME_NON_CONTINUOUS) ? 'N' : 'C', offset, length, PRODUCTOR_OFFSET,
                 m_lastAlignmentBytes, m_lastWriteBytes);
  }
  m_pRbInfo->frameIdx[PRODUCTOR_INDEX].offset = offset;
  m_pRbInfo->frameIdx[PRODUCTOR_INDEX].length = length;
  m_pRbInfo->frameIdx[PRODUCTOR_INDEX].frameType = type;
  m_pRbInfo->frameIdx[PRODUCTOR_INDEX].continuous = continuous;
  m_pRbInfo->frameIdx[PRODUCTOR_INDEX].requestTime = requestTime;
  m_pRbInfo->frameIdx[PRODUCTOR_INDEX].timeStamp = frameTime;
  PRODUCTOR_INDEX = ADVANCE_INDEX(PRODUCTOR_INDEX, 1);
  return CRB_ERROR_NONE;
}

int CRingBuf::WriteLog(const char *pLogBuf, uint32_t size) {
  int ret = CRB_ERROR_PARAM;
  lock(MY_NAME);

  char *pBuffer = (char *)RequestWriteFrame(size, CRB_FRAME_LOG);
  if (!CRB_VALID_ADDRESS(pBuffer)) {
    ret = CRB_ERROR_SPACE;
    goto WRITE_LOG_OUT;
  }
  memcpy(pBuffer, pLogBuf, size);
  CommitWrite(size);
  ret = CRB_ERROR_NONE;

WRITE_LOG_OUT:
  unlock();
  return ret;
}

int CRingBuf::CommitWrite(uint32_t size,       /*DEFAULT: CRB_COMMIT_LAST_REQ_SIZE*/
                          int32_t *pStartTime, /*DEFAULT: NULL*/
                          int32_t *pEndTime,   /*DEFAULT: NULL*/
                          long long frameTime) {
  assert(size == CRB_COMMIT_LAST_REQ_SIZE || size <= m_lastWriteBytes);
  if (IS_CONSUMER || 0 == m_lastWriteBytes) {
    DEBUG_FLOW_0(RB_NAME_FORMAT "Invalid Commit IS_CONSUMER %d m_lastWriteBytes %d\n", RB_NAME_VALUE, IS_CONSUMER,
                 m_lastWriteBytes);
    return CRB_ERROR_PERM;
  }

  /*
   * Size                      meaning
   * -----------------------------------------------------
   * CRB_COMMIT_LAST_REQ_SIZE  Commit last requested bytes
   * 0                         Discard request
   * < m_lastWriteBytes        Commit size bytes
   * > m_lastWriteBytes        Invalid!
   */
  uint32_t commitBytes = m_lastWriteBytes;
  if (size != CRB_COMMIT_LAST_REQ_SIZE) {
    commitBytes = size;
  }
  lock(MY_NAME);
#ifdef ENABLE_ACCOUNTING
  m_pRbInfo->productors[m_userIdx].commitCount++;
  if (m_lastWriteBytes == 0) {
    m_pRbInfo->productors[m_userIdx].commitWithoutRequestCount++;
  }
  m_pRbInfo->productors[m_userIdx].bytesCount += commitBytes;
#endif

  if (0 == commitBytes) {
    goto DISCARD_LAST_WRITE;
  }
  /*RequestWriteContinuousFrames will go here*/
  if (m_lastWriteFrameType == CRB_FRAME_ANY) {
#if 0
        int32_t byteCommited = (int32_t)IndexBuildFromOffset(
                ADVANCE_OFFSET(PRODUCTOR_OFFSET, m_lastAlignmentBytes), 
                commitBytes + m_pRbInfo->leftedBytes, 
                m_pRbInfo->productors[m_userIdx].requestUptime, pStartTime, pEndTime);
        m_pRbInfo->leftedBytes = (commitBytes + m_pRbInfo->leftedBytes) - byteCommited;
#endif
  } else {
    /*
     * We should mark a frame as FRAME_NON_CONTINUOUS in the following case:
     * 1. Aligned
     * 2. Jumped over a hole
     * 3. Productor is writing its first frame
     *    We want to mark this index as FRAME_NON_CONTINUOUS
     *    so that the continuous reader will be able to read it
     */
    IndexAdd(ADVANCE_OFFSET(PRODUCTOR_OFFSET, m_lastAlignmentBytes), commitBytes, m_lastWriteFrameType,
             (0 == m_pRbInfo->dataWriten || m_pRbInfo->jumpOverHole || m_lastAlignmentBytes) ? FRAME_NON_CONTINUOUS
                                                                                             : FRAME_CONTINUOUS,
             m_pRbInfo->productors[m_userIdx].requestUptime, true, frameTime);
    PRODUCTOR_OFFSET = ADVANCE_OFFSET2(PRODUCTOR_OFFSET, commitBytes, m_lastAlignmentBytes);
  }
  m_pRbInfo->dataWriten = 1;
  m_pRbInfo->contiBytesWriten += commitBytes;
  if (m_lastAlignmentBytes > 0) {
    m_pRbInfo->contiBytesWriten = 0;
  }
DISCARD_LAST_WRITE:
  m_lastWriteBytes = 0;
  m_lastAlignmentBytes = 0;
  m_pRbInfo->jumpOverHole = 0;
  unlock();
  return commitBytes;
}

uint32_t CRingBuf::BlackHoleSortMerge(uint32_t writeOffset, BlackHole *sortedHoles) {
  int holeNum = m_pRbInfo->holeNum;
  BlackHole *hole = NULL;
  BlackHole *lastSMhole = NULL;
  int8_t nearestHole = holeNum;
  if (holeNum == 0) {
    return 0;
  }
  for (int i = 0; i < holeNum; i++) {
    hole = &m_pRbInfo->holes[i];
    if (hole->startOffset >= writeOffset) {
      nearestHole = i;
      break;
    }
  }
  uint8_t loop = 1;
  uint8_t from[2] = {0, 0};
  uint8_t to[2] = {0, 0};
  if (nearestHole == 0 || nearestHole == holeNum) {
    from[0] = 0;
    to[0] = holeNum;
  } else {
    from[0] = nearestHole;
    to[0] = holeNum;
    from[1] = 0;
    to[1] = nearestHole;
    loop = 2;
  }

  uint8_t smHoleIdx = 0;
  for (int j = 0; j < loop; j++) {
    for (int i = from[j]; i < to[j]; i++) {
      hole = &m_pRbInfo->holes[i];
      if (0 == smHoleIdx) {
        lastSMhole = &sortedHoles[smHoleIdx];
        memcpy(&sortedHoles[smHoleIdx++], hole, sizeof(BlackHole));
        continue;
      }
      if (j == 0) {
        assert(hole->startOffset >= lastSMhole->startOffset);
      }
      uint32_t lastSMholeLen = BUFFER_LEN(lastSMhole->startOffset, lastSMhole->endOffset);
      uint32_t holeDistance = BUFFER_LEN(lastSMhole->startOffset, hole->startOffset);
      if (holeDistance <= lastSMholeLen) {
        uint32_t newLength = BUFFER_LEN(lastSMhole->startOffset, hole->endOffset);
        if (newLength >= lastSMholeLen) {
          lastSMhole->endOffset = ADVANCE_OFFSET(lastSMhole->startOffset, newLength);
        }
        snprintf(lastSMhole->blockerName + strlen(lastSMhole->blockerName),
                 sizeof(lastSMhole->blockerName) - strlen(lastSMhole->blockerName), "|%s", hole->blockerName);
      } else {
        lastSMhole = &sortedHoles[smHoleIdx];
        memcpy(&sortedHoles[smHoleIdx++], hole, sizeof(BlackHole));
      }
    }
  }
  for (int i = 0; i < smHoleIdx; i++) {
    /*BlackHole *hole = &sortedHoles[i];*/
    DEBUG_FLOW_5("sorted %08d|[%30s:%08d-%08d]\n", writeOffset, hole->blockerName, hole->startOffset, hole->endOffset);
  }
  assert(smHoleIdx <= holeNum);
  return smHoleIdx;
}

uint32_t CRingBuf::GetReadableSize(uint32_t readOffset, uint32_t writeOffset) {
  if (readOffset == writeOffset) {
    return 0;
  }
  return BUFFER_LEN(readOffset, writeOffset);
}

uint32_t CRingBuf::GetWritableSize(uint32_t readOffset, uint32_t writeOffset) {
  if (readOffset == writeOffset) {
    return m_bufSize;
  }
  return BUFFER_LEN(writeOffset, readOffset);
}
uint32_t CRingBuf::GetReadableSize(void) {
  uint32_t readOffset = CONSUMER_OFFSET(m_userIdx);
  uint32_t writeOffset = PRODUCTOR_OFFSET;
  return GetReadableSize(readOffset, writeOffset);
}
uint32_t CRingBuf::GetWritableSize(void) {
  uint32_t minWritableSize = m_bufSize;
  if (NULL == m_pDataBuffer || m_userIdx != 0 || IS_CONSUMER) {
    DEBUG_FLOW_0(RB_NAME_FORMAT "buff=0x%x m_userIdx=%d IS_CONSUMER=%d\n", RB_NAME_VALUE, (int32_t)m_pDataBuffer,
                 m_userIdx, IS_CONSUMER);
    return 0;
  }

  for (int i = 0; i < MAX_CONSUMER_NUM; i++) {
    if (0 == VALID_CONSUMER(i)) {
      continue;
    }

    uint32_t readOffset = CONSUMER_OFFSET(i);
    uint32_t writableSize = GetWritableSize(readOffset, PRODUCTOR_OFFSET);
    if (minWritableSize > writableSize) {
      minWritableSize = writableSize;
    }
  }
  return minWritableSize - 1;
}

uint32_t CRingBuf::GetBufferSize(void) { return m_bufSize; }

uint32_t CRingBuf::BlackholeJump(uint32_t writeOffset, uint32_t writeLength, uint32_t alignment) {
  if (m_pRbInfo->holeNum <= 0) {
    return writeOffset;
  }

  BlackHole sortedHoles[MAX_CONSUMER_NUM];
  uint32_t smHoleNum = BlackHoleSortMerge(writeOffset, sortedHoles);

  uint32_t holeJumpOffset = writeOffset;
  for (uint8_t holeIdx = 0; holeIdx < smHoleNum; holeIdx++) {
    uint32_t writeEnd = ADVANCE_OFFSET2(holeJumpOffset, writeLength, alignment);
    DEBUG_FLOW_5("Test blackhole #%02d [%60s]\nHole:%08d-%08d(%08d)\nWant:%08d-%08d(%08d)\n", holeIdx,
                 sortedHoles[holeIdx].blockerName, sortedHoles[holeIdx].startOffset, sortedHoles[holeIdx].endOffset,
                 BUFFER_LEN(sortedHoles[holeIdx].startOffset, sortedHoles[holeIdx].endOffset), holeJumpOffset, writeEnd,
                 BUFFER_LEN(holeJumpOffset, writeEnd));
    if (holeJumpOffset < writeEnd) {
      if (sortedHoles[holeIdx].startOffset < sortedHoles[holeIdx].endOffset) {
        /*
         * |..........|_hole_|.............|
         * |.|_data_|......................|
         *
         * |..|_h0_|..|_h1_|...............|
         * |...................|_data_|....|
         */
        if (((holeJumpOffset > sortedHoles[holeIdx].endOffset) && (writeEnd > sortedHoles[holeIdx].endOffset)) ||
            ((holeJumpOffset < sortedHoles[holeIdx].startOffset) && (writeEnd <= sortedHoles[holeIdx].startOffset))) {
          break;
        }
      } else {
        /*
         * |_hole_|................|_hole_|
         * |..........|_data_|............|
         */
        if ((holeJumpOffset < sortedHoles[holeIdx].startOffset) && (holeJumpOffset > sortedHoles[holeIdx].endOffset) &&
            (writeEnd <= sortedHoles[holeIdx].startOffset) && (writeEnd > sortedHoles[holeIdx].endOffset)) {
          break;
        }
      }
    } else /*Data wraped*/
    {
      /*
       * |..........|_hole_|.............|
       * |___|................|_data_____|
       */
      if (((sortedHoles[holeIdx].endOffset > sortedHoles[holeIdx].startOffset) &&
           (holeJumpOffset > sortedHoles[holeIdx].endOffset) && (writeEnd <= sortedHoles[holeIdx].startOffset))) {
        break;
      }
    }
    holeJumpOffset = sortedHoles[holeIdx].endOffset;
  }
  return holeJumpOffset;
}

uint8_t *CRingBuf::__RequestWritePtr(uint32_t size, uint16_t alignment, uint16_t *pOutBlockers, /*DEFAULT:NULL*/
                                     uint16_t *pOutBlockerCount /*DEFAULT:NULL*/) {
  /* Not initialized yet */
  if (NULL == m_pDataBuffer || m_userIdx != 0 || size == 0 || IS_CONSUMER || alignment > 4096) {
    DEBUG_FLOW_0(RB_NAME_FORMAT "buff=0x%x m_userIdx=%d size=%d IS_CONSUMER=%d alignment=%d\n", RB_NAME_VALUE,
                 (int32_t)m_pDataBuffer, m_userIdx, size, IS_CONSUMER, alignment);
    return NULL;
  }

  uint32_t writeOffset = PRODUCTOR_OFFSET;
  uint32_t minWritableSize = m_bufSize;
  uint32_t writableSize = 0;
  int32_t consumer_num = 0;

  uint16_t blockers[MAX_CONSUMER_NUM];
  uint16_t blockCount = 0;

  uint16_t alignBlockers[MAX_CONSUMER_NUM];
  int32_t alignBlockerCount = 0;

  uint32_t writableSizes[MAX_CONSUMER_NUM];

  memset(blockers, 0, sizeof(blockers));
  memset(alignBlockers, 0, sizeof(alignBlockers));
  memset(writableSizes, 0, sizeof(writableSizes));
  for (int i = 0; i < MAX_CONSUMER_NUM; i++) {
    if (0 == VALID_CONSUMER(i)) {
      continue;
    }
    consumer_num++;

    uint32_t readOffset = CONSUMER_OFFSET(i);
    writableSize = GetWritableSize(readOffset, writeOffset);
    writableSizes[i] = writableSize;

    if (minWritableSize > writableSize) {
      minWritableSize = writableSize;
    }

    if (writableSize <= size) {
      blockers[blockCount++] = i;
    }
    if (alignment && writableSize <= size + alignment) {
      alignBlockers[alignBlockerCount++] = i;
    }
  }

  /*manual indented, for log format*/
  DEBUG_FLOW_5(
      "offsets:%07d | [%03d] %07d %07d %07d %07d %07d \
%07d %07d %07d %07d %07d \n",
      writeOffset, consumer_num, CONSUMER_OFFSET(0), CONSUMER_OFFSET(1), CONSUMER_OFFSET(2), CONSUMER_OFFSET(3),
      CONSUMER_OFFSET(4), CONSUMER_OFFSET(5), CONSUMER_OFFSET(6), CONSUMER_OFFSET(7), CONSUMER_OFFSET(8),
      CONSUMER_OFFSET(9));
  DEBUG_FLOW_5(
      "avsizes:%07d | [%03d] %07d %07d %07d %07d %07d \
%07d %07d %07d %07d %07d \n",
      writeOffset, consumer_num, writableSizes[0], writableSizes[1], writableSizes[2], writableSizes[3],
      writableSizes[4], writableSizes[5], writableSizes[6], writableSizes[7], writableSizes[8], writableSizes[9]);

  uint32_t holeSkipedBytes = 0;

  /*Prevent productor overwrite existing data, since there is no consumer to BLOCK it*/
  if (pOutBlockers == NULL && /*indicate CRB_WRITE_MODE_BLOCK*/
      consumer_num == 0 &&    /*No consumer installed*/
      m_pRbInfo->dataWriten != 0 /*Some data writen already*/) {
    DEBUG_FLOW_5(RB_NAME_FORMAT "No consumer installed yet in blocking mode\n", RB_NAME_VALUE);
    goto WRITABLE_MEM_NOT_ENOUGH;
  }

  /*Make a quick decision*/
  if (size >= minWritableSize) {
    goto WRITABLE_MEM_NOT_ENOUGH;
  }

  /*
   *We know writable memory size for all consumer
   *now we may need to jump over some black hole
   */
  holeSkipedBytes = BUFFER_LEN(writeOffset, BlackholeJump(writeOffset, size, alignment));
  DEBUG_FLOW_5("holeSkipedBytes=%08d alignment=%4d size=%08d\n", holeSkipedBytes, alignment, size);
  if (holeSkipedBytes + alignment + size >= minWritableSize) {
    DEBUG_FLOW_5("No space due to jump blackhole hole\n");
    /*Update the blockers, and let the caller to move them*/
    blockCount = 0;
    for (int i = 0; i < MAX_CONSUMER_NUM; i++) {
      if (0 == VALID_CONSUMER(i)) {
        continue;
      }
      if (holeSkipedBytes + alignment + size >= writableSizes[i]) {
        blockers[blockCount++] = i;
      }
    }
    goto WRITABLE_MEM_NOT_ENOUGH;
  }

  /*
   * We are able to provide the caller with <size> bytes buffer
   * Need to advance productor offset to the right gap between black hole first
   */
  if (holeSkipedBytes) {
    PRODUCTOR_OFFSET = ADVANCE_OFFSET(PRODUCTOR_OFFSET, holeSkipedBytes);
    DEBUG_FLOW_5("Set productor offset %08d->%08d (%08d)bytes since there are holes\n", writeOffset, PRODUCTOR_OFFSET,
                 holeSkipedBytes);
    writeOffset = PRODUCTOR_OFFSET;
    m_pRbInfo->jumpOverHole = 1;
  }

  if (alignment && 0 == alignBlockerCount) {
    /* Just dont want use m_wrapMask here, so we advance 0 bytes */
    writeOffset = ADVANCE_OFFSET(ALIGN(writeOffset, alignment), 0);
    m_lastAlignmentBytes = BUFFER_LEN(PRODUCTOR_OFFSET, writeOffset);

    DEBUG_FLOW_5("alignment=0x%5x 0x%05x --> 0x%05x real=0x%05x\n", alignment, PRODUCTOR_OFFSET, writeOffset,
                 m_lastAlignmentBytes);

    /*Remember this, will used in CommitWrite*/
    m_lastWriteBytes = size;
    return m_pDataBuffer + writeOffset;
  }
  if (0 == blockCount) {
    /*Tried our best to fullfill alignment but faild see if we can write it sequencely*/
    m_lastWriteBytes = size;
    m_lastAlignmentBytes = 0;
    return m_pDataBuffer + writeOffset;
  }

WRITABLE_MEM_NOT_ENOUGH:
  if (pOutBlockers && pOutBlockerCount) {
    memcpy(pOutBlockers, blockers, sizeof(blockers));
    *pOutBlockerCount = blockCount;
  }
  /*Too many log if there is a consumer just attached to the ring buffer without request frame*/
  DEBUG_FLOW_5(RB_NAME_FORMAT "Not enough space: have %d requested %d holeJumped %d\n", RB_NAME_VALUE, minWritableSize,
               size, holeSkipedBytes);
  return (uint8_t *)CRB_ERROR_SPACE;
}

uint8_t *CRingBuf::RequestWritePtr(int size) {
  assert(size > 0);
  /*If m_lastWriteFrameType is uninitialized,
   * There is a chance CommitWrite will call IndexBuildFromOffset*/
  m_lastWriteFrameType = CRB_FRAME_I_SLICE;
  m_pRbInfo->videoBuffer = false;
  return __RequestWritePtr(size, 0);
}

uint8_t *CRingBuf::RequestReadContinuousFrames(uint32_t *length, FrameIdx *pIdx, uint32_t *pFrameCount) {
  assert(length != NULL);
  if (!pIdx || !pFrameCount) {
    DEBUG_INFO(STORAGE, "Invalid parameter\n");
    return (uint8_t *)CRB_ERROR_PARAM;
  }
  lock(MY_NAME);
  DeleteBlackHoleOwnedBy(m_pRbInfo->consumers[m_userIdx].name);
  ClearConsumerTimeOut(m_userIdx);
#ifdef ENABLE_ACCOUNTING
  m_pRbInfo->consumers[m_userIdx].requestCount++;
  if (m_lastReadBytes > 0) {
    m_pRbInfo->consumers[m_userIdx].requestWithoutCommitCount++;
  }
#endif
  uint8_t *ret = (uint8_t *)CRB_ERROR_PARAM;
  uint32_t readIndex = m_pRbInfo->consumers[m_userIdx].index;
  /*Find the next frame according to type!*/
  uint32_t nextNextAlignedFrame = 0;
  uint32_t endFrame = 0;
  uint32_t maxFrameNumUserWant = *pFrameCount;
  uint32_t nextAlignedFrame = IndexFindNextFrame(
      PRODUCTOR_INDEX, readIndex,
      (m_pRbInfo->liveStream && m_pRbInfo->consumers[m_userIdx].waitIFrame) ? CRB_FRAME_I_SLICE : CRB_FRAME_ANY);
  if (CRB_ERROR_NO_IDX == nextAlignedFrame) {
    if (m_pRbInfo->consumers[m_userIdx].waitIFrame) {
      DEBUG_FLOW_5(RB_NAME_FORMAT "%s need to wait iframe\n", RB_NAME_VALUE, m_pRbInfo->consumers[m_userIdx].name);
    }
    unlock();
    return (uint8_t *)CRB_ERROR_NO_IDX;
  } else {
    nextNextAlignedFrame =
        IndexFindNextFrame(PRODUCTOR_INDEX, ADVANCE_INDEX(nextAlignedFrame, 1), CRB_FRAME_ANY, FRAME_NON_CONTINUOUS);
    if ((uint32_t)CRB_ERROR_NO_IDX == nextNextAlignedFrame) {
      unlock();
      return (uint8_t *)CRB_ERROR_NO_IDX;
    }
  }
  uint32_t totalIdxCount = (nextNextAlignedFrame + MAX_FRAME_NUM - nextAlignedFrame) % MAX_FRAME_NUM;
  if (totalIdxCount >= maxFrameNumUserWant) {
    nextNextAlignedFrame = STEPBACK_INDEX(nextNextAlignedFrame, (totalIdxCount - maxFrameNumUserWant));
    totalIdxCount = maxFrameNumUserWant;
  }
  endFrame = STEPBACK_INDEX(nextNextAlignedFrame, 1);
  DEBUG_FLOW_5("Continuous: %05d-%05d-%05d\n", readIndex, nextAlignedFrame, nextNextAlignedFrame);
  /*Both of two index found*/
  uint32_t readOffset = m_pRbInfo->frameIdx[nextAlignedFrame].offset;
  uint32_t readSize = BUFFER_LEN(m_pRbInfo->frameIdx[nextAlignedFrame].offset, m_pRbInfo->frameIdx[endFrame].offset) +
                      m_pRbInfo->frameIdx[endFrame].length;
  ret = __RequestReadPtr(readOffset, PRODUCTOR_OFFSET, readSize);
  if (CRB_VALID_ADDRESS(ret)) {
    MoveConsumer(m_userIdx, nextAlignedFrame, readOffset, false, "SkipFrame1");
    SetConsumerRefIdx(m_userIdx, nextAlignedFrame, endFrame);
    SetConsumerState(m_userIdx, USER_HOLDING_FRAME);
    DEBUG_FLOW_5("Hold ref to index [%05d, %05d] total 0x%07x bytes\n", nextAlignedFrame, endFrame, readSize);
    *length = readSize;
    uint8_t i = 0;
    if (pIdx) {
      int32_t fIdx = nextAlignedFrame;
      for (i = 0; i < *pFrameCount && i < totalIdxCount; i++) {
        pIdx[i].offset = BUFFER_LEN(m_pRbInfo->frameIdx[nextAlignedFrame].offset, m_pRbInfo->frameIdx[fIdx].offset);
        pIdx[i].length = m_pRbInfo->frameIdx[fIdx].length;
        pIdx[i].isIFrame = (CRB_FRAME_I_SLICE == m_pRbInfo->frameIdx[fIdx].frameType) ? 1 : 0;
        fIdx = ADVANCE_INDEX(fIdx, 1);
      }
    }
    if (pFrameCount) {
      *pFrameCount = i;
    }
    ClearConsumerMoved(m_userIdx);
  } else {
    *length = 0;
    if (pFrameCount) {
      *pFrameCount = 0;
    }
  }
  unlock();
  return ret;
}

int CRingBuf::ResetAllUser(void) {
  lock(MY_NAME);
  int ret = ResetAllUserNoLock();
  unlock();
  return ret;
}

int CRingBuf::ResetAllUserNoLock(void) {
  int ret = CRB_ERROR_PERM;
  if (NULL == m_pDataBuffer || m_userIdx >= MAX_PRODUCTOR_NUM || IS_CONSUMER) {
    DEBUG_FLOW_0(RB_NAME_FORMAT "buff=0x%x m_userIdx=%d IS_PRODUCTOR=%d\n", RB_NAME_VALUE, (int32_t)m_pDataBuffer,
                 m_userIdx, IS_PRODUCTOR);
    ret = CRB_ERROR_PERM;
    goto RESET_OUT;
  }
  for (int i = 0; i < MAX_CONSUMER_NUM; i++) {
    if (VALID_CONSUMER(i)) {
      if (GetConsumerState(i) == USER_HOLDING_FRAME) {
        uint32_t refIdxStart = m_pRbInfo->consumers[i].refIdxStart;
        uint32_t refIdxEnd = m_pRbInfo->consumers[i].refIdxEnd;
        InsertBlackHole(m_pRbInfo->consumers[i].name, m_pRbInfo->frameIdx[refIdxStart].offset,
                        ADVANCE_OFFSET(m_pRbInfo->frameIdx[refIdxEnd].offset, m_pRbInfo->frameIdx[refIdxEnd].length));
        SetConsumerMoved(i);
        SetConsumerState(i, USER_BLOCKED);
      } else if (GetConsumerState(i) == USER_BLOCKED) {
        SetConsumerMoved(i);
      } else if (GetConsumerState(i) == USER_FREE) {
      }
      MoveConsumer(i, 0, 0, true, "Move#PRO_RESET");
    }
  }
  m_pRbInfo->productors[m_userIdx].discardCount++;
  PRODUCTOR_OFFSET = 0;
  PRODUCTOR_INDEX = 0;
  m_pRbInfo->dataWriten = 0;
  m_pRbInfo->contiBytesWriten = 0;
  m_pRbInfo->leftedBytes = 0;
  /*Defend against Request --> Reset --> Commit call sequence*/
  m_lastWriteBytes = 0;
  ret = CRB_ERROR_NONE;
RESET_OUT:
  return ret;
}

void CRingBuf::DiscardAllData(void) {
  if (NULL == m_pDataBuffer || m_userIdx >= MAX_CONSUMER_NUM || IS_PRODUCTOR) {
    DEBUG_FLOW_0(RB_NAME_FORMAT "buff=0x%x m_userIdx=%d IS_PRODUCTOR=%d\n", RB_NAME_VALUE, (int32_t)m_pDataBuffer,
                 m_userIdx, IS_PRODUCTOR);
    return;
  }
  lock(MY_NAME);
  m_pRbInfo->consumers[m_userIdx].discardCount++;
  DeleteBlackHoleOwnedBy(m_pRbInfo->consumers[m_userIdx].name);
  ClearConsumerTimeOut(m_userIdx);
  MoveConsumer(m_userIdx, PRODUCTOR_INDEX, PRODUCTOR_OFFSET, true, "Discard");
  SetConsumerState(m_userIdx, USER_FREE);
  ClearConsumerMoved(m_userIdx);
  /*Defend against Request -> Discard ->Commit API call sequence*/
  m_lastReadBytes = 0;
  unlock();
}

uint8_t *CRingBuf::RequestReadFrame(int32_t *frameLen, uint32_t *isKeyFrame, /*DEFAULT:type = NULL*/
                                    FRAME_E type)                            /*DEFAULT:type = CRB_FRAME_ANY*/
{
  assert(frameLen != NULL);
  assert(type <= CRB_FRAME_ANY);
  DEBUG_FLOW_5("RequestReadFrame frameLen=0x%08x isKeyFrame=0x%08x type=%2d\n", frameLen, isKeyFrame, type);

  lock(MY_NAME);
  DeleteBlackHoleOwnedBy(m_pRbInfo->consumers[m_userIdx].name);
  ClearConsumerTimeOut(m_userIdx);
#ifdef ENABLE_ACCOUNTING
  m_pRbInfo->consumers[m_userIdx].requestCount++;
  if (m_lastReadBytes > 0) {
    m_pRbInfo->consumers[m_userIdx].requestWithoutCommitCount++;
  }
#endif
  uint8_t *ret = (uint8_t *)CRB_ERROR_PARAM;
  uint32_t readIndex = m_pRbInfo->consumers[m_userIdx].index;
  assert(readIndex < MAX_FRAME_NUM);
  /*Find the next frame according to type!*/
  uint32_t nextFrameIdx = IndexFindNextFrame(
      PRODUCTOR_INDEX, readIndex,
      (m_pRbInfo->liveStream && m_pRbInfo->consumers[m_userIdx].waitIFrame) ? CRB_FRAME_I_SLICE : type);
  DEBUG_FLOW_5("Read readIdx=%05d readOffset=%05d writeIdx=%05d writeOffset=%05d, nextFrameIdx=%05d\n", readIndex,
               m_pRbInfo->frameIdx[readIndex].offset, PRODUCTOR_INDEX, PRODUCTOR_OFFSET, nextFrameIdx);
  if (m_pRbInfo->consumers[m_userIdx].offset != m_pRbInfo->frameIdx[readIndex].offset) {
    DEBUG_FLOW_5("BAD consumer offset=%05d frame length=%07d\n", m_pRbInfo->consumers[m_userIdx].offset,
                 m_pRbInfo->frameIdx[readIndex].length);
  }

  if ((uint32_t)CRB_ERROR_NO_IDX != nextFrameIdx) {
    uint32_t readOffset = m_pRbInfo->frameIdx[nextFrameIdx].offset;
    uint32_t readSize = m_pRbInfo->frameIdx[nextFrameIdx].length;
    if (readSize == 0) {
      DEBUG_FLOW_0(RB_NAME_FORMAT
                   "Read readIdx=%05d readOffset=%05d writeIdx=%05d writeOffset=%05d, nextFrameIdx=%05d\n",
                   RB_NAME_VALUE, readIndex, m_pRbInfo->frameIdx[readIndex].offset, PRODUCTOR_INDEX, PRODUCTOR_OFFSET,
                   nextFrameIdx);
    }

    ret = __RequestReadPtr(readOffset, PRODUCTOR_OFFSET, readSize);
    if (CRB_VALID_ADDRESS(ret)) {
      /*
       * Since this consumer skiped (nextFrameIdx-readIndex) frame
       * We advance its index AND read offset inmediately before CommitRead
       * This means that the productor can overwrite these frames/indexes,
       * if there is no other consumer blocking it
       */
      MoveConsumer(m_userIdx, nextFrameIdx, readOffset, false, "SkipFrame");
      SetConsumerState(m_userIdx, USER_HOLDING_FRAME);
      *frameLen = readSize;
      if (isKeyFrame) {
        *isKeyFrame = ((m_pRbInfo->frameIdx[nextFrameIdx].frameType == CRB_FRAME_I_SLICE) ? 1 : 0);
      }
      DEBUG_FLOW_5("ReadFrame #%05d [%c][%c]%08d+%08d\n", nextFrameIdx,
                   (m_pRbInfo->frameIdx[nextFrameIdx].frameType == CRB_FRAME_I_SLICE) ? 'I' : 'P',
                   (m_pRbInfo->frameIdx[nextFrameIdx].continuous == FRAME_NON_CONTINUOUS) ? 'N' : 'C',
                   ret - m_pDataBuffer, readSize);
#if 0
            If we don not clear this flag here
            the consumer will got the same frame for 2 time
            ----------------------------------------------------------
            Move consumer, set flag
            Request #0
            Commit  #0      index/offset not moved due to moved flag
            Request #1      got the same frame with Request #0
            Commit  #1
#endif
      ClearConsumerMoved(m_userIdx);
    } else {
      /*This should not hanppen since we can read out all data*/
      *frameLen = 0;
      if (ret != NULL)
        DEBUG_FLOW_0("Can not read Frame#%05d [%c][%c]%08d+%08d\n", nextFrameIdx,
                     (m_pRbInfo->frameIdx[nextFrameIdx].frameType == CRB_FRAME_I_SLICE) ? 'I' : 'P',
                     (m_pRbInfo->frameIdx[nextFrameIdx].continuous == FRAME_NON_CONTINUOUS) ? 'N' : 'C',
                     ret - m_pDataBuffer, readSize);
    }
  } else {
    if (m_pRbInfo->consumers[m_userIdx].waitIFrame) {
      DEBUG_FLOW_5(RB_NAME_FORMAT "%s need to wait iframe\n", RB_NAME_VALUE, m_pRbInfo->consumers[m_userIdx].name);
    }
  }
  unlock();
  return ret;
}

uint8_t *CRingBuf::RequestReadNextFrameStamp(int32_t *frameLen, long long *pTimeStamp,
                                             uint32_t *isKeyFrame, /*DEFAULT:type = NULL*/
                                             FRAME_E type)         /*DEFAULT:type = CRB_FRAME_ANY*/
{
  assert(frameLen != NULL);
  assert(type <= CRB_FRAME_ANY);
  DEBUG_FLOW_5("RequestReadNextFrameStamp frameLen=0x%08x isKeyFrame=0x%08x type=%2d\n", frameLen, isKeyFrame, type);

  lock(MY_NAME);
  DeleteBlackHoleOwnedBy(m_pRbInfo->consumers[m_userIdx].name);
  ClearConsumerTimeOut(m_userIdx);
#ifdef ENABLE_ACCOUNTING
  m_pRbInfo->consumers[m_userIdx].requestCount++;
  if (m_lastReadBytes > 0) {
    m_pRbInfo->consumers[m_userIdx].requestWithoutCommitCount++;
  }
#endif
  uint8_t *ret = (uint8_t *)CRB_ERROR_PARAM;
  uint32_t readIndex = m_pRbInfo->consumers[m_userIdx].index;
  assert(readIndex <= MAX_FRAME_NUM);
  /*Find the next frame according to type!*/
  uint32_t nextFrameIdx = IndexFindNextFrame(
      PRODUCTOR_INDEX, readIndex,
      (m_pRbInfo->liveStream && m_pRbInfo->consumers[m_userIdx].waitIFrame) ? CRB_FRAME_I_SLICE : type);
  DEBUG_FLOW_5("Read readIdx=%05d readOffset=%05d writeIdx=%05d writeOffset=%05d, nextFrameIdx=%05d\n", readIndex,
               m_pRbInfo->frameIdx[readIndex].offset, PRODUCTOR_INDEX, PRODUCTOR_OFFSET, nextFrameIdx);
  if (m_pRbInfo->consumers[m_userIdx].offset != m_pRbInfo->frameIdx[readIndex].offset) {
    DEBUG_FLOW_5("BAD consumer offset=%05d frame length=%07d\n", m_pRbInfo->consumers[m_userIdx].offset,
                 m_pRbInfo->frameIdx[readIndex].length);
  }

  if ((uint32_t)CRB_ERROR_NO_IDX != nextFrameIdx) {
    uint32_t readOffset = m_pRbInfo->frameIdx[nextFrameIdx].offset;
    uint32_t readSize = m_pRbInfo->frameIdx[nextFrameIdx].length;
    if (readSize == 0) {
      DEBUG_FLOW_0(RB_NAME_FORMAT
                   "Read readIdx=%05d readOffset=%05d writeIdx=%05d writeOffset=%05d, nextFrameIdx=%05d\n",
                   RB_NAME_VALUE, readIndex, m_pRbInfo->frameIdx[readIndex].offset, PRODUCTOR_INDEX, PRODUCTOR_OFFSET,
                   nextFrameIdx);
    }

    ret = __RequestReadPtr(readOffset, PRODUCTOR_OFFSET, readSize);
    if (CRB_VALID_ADDRESS(ret)) {
      /*
       * Since this consumer skiped (nextFrameIdx-readIndex) frame
       * We advance its index AND read offset inmediately before CommitRead
       * This means that the productor can overwrite these frames/indexes,
       * if there is no other consumer blocking it
       */
      MoveConsumer(m_userIdx, nextFrameIdx, readOffset, false, "SkipFrame");
      // SetConsumerState(m_userIdx, USER_HOLDING_FRAME); // do not hold, just get timestamp
      *frameLen = readSize;
      if (isKeyFrame) {
        *isKeyFrame = ((m_pRbInfo->frameIdx[nextFrameIdx].frameType == CRB_FRAME_I_SLICE) ? 1 : 0);
      }
      *pTimeStamp = m_pRbInfo->frameIdx[nextFrameIdx].timeStamp;
      DEBUG_FLOW_5("ReadFrame #%05d [%c][%c]%08d+%08d\n", nextFrameIdx,
                   (m_pRbInfo->frameIdx[nextFrameIdx].frameType == CRB_FRAME_I_SLICE) ? 'I' : 'P',
                   (m_pRbInfo->frameIdx[nextFrameIdx].continuous == FRAME_NON_CONTINUOUS) ? 'N' : 'C',
                   ret - m_pDataBuffer, readSize);
#if 0
            If we don not clear this flag here
            the consumer will got the same frame for 2 time
            ----------------------------------------------------------
            Move consumer, set flag
            Request #0
            Commit  #0      index/offset not moved due to moved flag
            Request #1      got the same frame with Request #0
            Commit  #1
#endif
      ClearConsumerMoved(m_userIdx);
    } else {
      /*This should not hanppen since we can read out all data*/
      *frameLen = -2;
      *pTimeStamp = 0;
      DEBUG_FLOW_0("Can not read Frame#%05d [%c][%c]%08d+%08d\n", nextFrameIdx,
                   (m_pRbInfo->frameIdx[nextFrameIdx].frameType == CRB_FRAME_I_SLICE) ? 'I' : 'P',
                   (m_pRbInfo->frameIdx[nextFrameIdx].continuous == FRAME_NON_CONTINUOUS) ? 'N' : 'C',
                   ret - m_pDataBuffer, readSize);
    }
  } else {
    if (m_pRbInfo->consumers[m_userIdx].waitIFrame) {
      DEBUG_FLOW_5(RB_NAME_FORMAT "%s need to wait iframe\n", RB_NAME_VALUE, m_pRbInfo->consumers[m_userIdx].name);
    }
    *frameLen = -2;
    *pTimeStamp = 0;
  }
  unlock();
  return ret;
}

void CRingBuf::DebugBlackHole(const char *reason) {
  for (int i = 0; i < m_pRbInfo->holeNum; i++) {
    BlackHole *hole = &m_pRbInfo->holes[i];
    printf("%16s Total %02d blackhole #%2d | %08d [%20s:%08d-%08d]\n", reason, m_pRbInfo->holeNum, i, hole->createTime,
           hole->blockerName, hole->startOffset, hole->endOffset);
  }
}

uint32_t CRingBuf::DeleteBlackHoleByIdx(uint8_t holeIdx) {
  assert(holeIdx < m_pRbInfo->holeNum);
  assert(m_pRbInfo->holeNum > 0);
  DebugBlackHole("Before delete");
  BlackHole *hole = &m_pRbInfo->holes[holeIdx];
  /*If not deleting the last one*/
  if (holeIdx < m_pRbInfo->holeNum - 1) {
    printf("delete memmove [%02d], [%02d], %02d\n", holeIdx, holeIdx + 1, m_pRbInfo->holeNum - holeIdx - 1);
    memmove(hole, hole + 1, sizeof(BlackHole) * (m_pRbInfo->holeNum - holeIdx - 1));
  }
  memset(&m_pRbInfo->holes[m_pRbInfo->holeNum - 1], 0, sizeof(BlackHole));
  m_pRbInfo->holeNum--;
  DebugBlackHole("After delete");
  return 0;
}

uint32_t CRingBuf::DeleteBlackHoleOwnedBy(const char *blockerName) {
  assert(NULL != blockerName);
  for (int i = m_pRbInfo->holeNum - 1; i >= 0; --i) {
    BlackHole *hole = &m_pRbInfo->holes[i];
    if ((0 == strcmp(hole->blockerName, blockerName))) {
      DeleteBlackHoleByIdx(i);
    }
  }
  return CRB_ERROR_NONE;
}

uint32_t CRingBuf::DeleteAgedBlackHole(void) {
  BlackHole *hole = NULL;
  int32_t _uptime = uptime();
  uint8_t holeNum = m_pRbInfo->holeNum;
  for (int i = holeNum - 1; i >= 0; --i) {
    hole = &m_pRbInfo->holes[i];
    if (hole->createTime != 0) {
      if (_uptime - hole->createTime > BLACKHOLE_MAX_LIFE_SEC) {
        DEBUG_FLOW_5("Aged blackhole #%2d | %08d [%20s:%08d-%08d]\n", i, hole->createTime, hole->blockerName,
                     hole->startOffset, hole->endOffset);
        DeleteBlackHoleByIdx(i);
      }
    }
  }
  return 0;
}

uint32_t CRingBuf::InsertBlackHole(const char *blockerName, uint32_t startOffset, uint32_t endOffset) {
  assert(m_pRbInfo->holeNum < MAX_CONSUMER_NUM);

  /*Append to the end by default*/
  uint8_t insertIdx = m_pRbInfo->holeNum;
  BlackHole *hole = NULL;
  DebugBlackHole("Before insert");
  DEBUG_FLOW_5("Try to insert blackhole [%20s:%08d-%08d]\n", blockerName, startOffset, endOffset);
  /*For sort insert hole according to offset*/
  for (int i = 0; i < m_pRbInfo->holeNum; i++) {
    hole = &m_pRbInfo->holes[i];
    if (hole->createTime == 0 || hole->startOffset > startOffset ||
        (hole->startOffset == startOffset && hole->endOffset > endOffset)) /*Fix BLACKHOLE_OVERLAP issue*/
    {
      insertIdx = i;
      break;
    } else if (0 == strcmp(hole->blockerName, blockerName)) {
      DEBUG_FLOW_5("ERROR %20s already hold hole #%02d created @%08d [%20s:%08d-%08d]\n", blockerName, i,
                   hole->createTime, hole->blockerName, hole->startOffset, hole->endOffset);
      return CRB_ERROR_EXIST;
    }
  }

  if (insertIdx != m_pRbInfo->holeNum) {
    DEBUG_FLOW_5("insert memmove [%02d], [%02d], %02d\n", insertIdx + 1, insertIdx, m_pRbInfo->holeNum - insertIdx);
    if (hole + 1) memmove(hole + 1, hole, sizeof(BlackHole) * (m_pRbInfo->holeNum - insertIdx));
  } else {
    hole = &m_pRbInfo->holes[m_pRbInfo->holeNum];
  }

  if (hole) {
    if (sizeof(hole->blockerName)) strncpy(hole->blockerName, blockerName, sizeof(hole->blockerName));
    hole->startOffset = startOffset;
    hole->endOffset = endOffset;
    hole->createTime = uptime();
    m_pRbInfo->holeNum++;
    m_pRbInfo->holeCreatedNum++;
    DEBUG_FLOW_5("New blackhole #%2d inserted @%08d [%20s:%08d-%08d]\n", insertIdx, hole->createTime, hole->blockerName,
                 hole->startOffset, hole->endOffset);
  }

  DebugBlackHole("After insert");
  return CRB_ERROR_NONE;
}

uint32_t CRingBuf::MoveAllBlockers(uint16_t *blockers, uint16_t blockerCount) {
  uint32_t nextIframe = 0;
  uint32_t readOffset = 0;
  uint16_t consumerIndex = 0;
  bool iFrameFound = false;
  for (int i = 0; i < blockerCount; i++) {
    consumerIndex = blockers[i];
#if 0
        nextIframe = IndexFindLatestFrame(PRODUCTOR_INDEX,
                m_pRbInfo->consumers[consumerIndex].index, CRB_FRAME_I_SLICE);
#else
    nextIframe = IndexFindNextFrame(PRODUCTOR_INDEX, ADVANCE_INDEX(m_pRbInfo->consumers[consumerIndex].index, 1),
                                    CRB_FRAME_I_SLICE);
#endif
    if (CRB_ERROR_NO_IDX == nextIframe || nextIframe == m_pRbInfo->consumers[consumerIndex].index) {
      nextIframe = STEPBACK_INDEX(PRODUCTOR_INDEX, 1);
      DEBUG_FLOW_0(RB_NAME_FORMAT "Blocker %s moved to product index %d since no new I frame found\n", RB_NAME_VALUE,
                   m_pRbInfo->consumers[consumerIndex].name, PRODUCTOR_INDEX);
    } else {
      iFrameFound = true;
    }
    readOffset = m_pRbInfo->frameIdx[nextIframe].offset;
    DEBUG_FLOW_5("Blocker#%02d consumer#%02d readOffset=%08d state=%02d\n", i, consumerIndex, readOffset,
                 GetConsumerState(consumerIndex));
    if (GetConsumerState(consumerIndex) == USER_FREE) {
      /*
       *This consumer hold no reference to our buffer
       *Just move its index to next I frame
       */
      MoveConsumer(consumerIndex, nextIframe, readOffset, !iFrameFound, "MoveBlocked#FREE");
    } else if (GetConsumerState(consumerIndex) == USER_BLOCKED) {
      MoveConsumer(consumerIndex, nextIframe, readOffset, !iFrameFound, "MoveBlocked#BLOCKED");
      SetConsumerMoved(consumerIndex);
    } else if (GetConsumerState(consumerIndex) == USER_HOLDING_FRAME) {
      /*
       *This consumer still hold reference to our buffer
       *Need to make a blackhole to make sure we wont overwrite the buffer
       */
      uint32_t refIdxStart = m_pRbInfo->consumers[consumerIndex].refIdxStart;
      uint32_t refIdxEnd = m_pRbInfo->consumers[consumerIndex].refIdxEnd;
      InsertBlackHole(m_pRbInfo->consumers[consumerIndex].name, m_pRbInfo->frameIdx[refIdxStart].offset,
                      ADVANCE_OFFSET(m_pRbInfo->frameIdx[refIdxEnd].offset, m_pRbInfo->frameIdx[refIdxEnd].length));
      MoveConsumer(consumerIndex, nextIframe, readOffset, !iFrameFound, "MoveBlocked#HOLD");
      SetConsumerMoved(consumerIndex);
      SetConsumerState(consumerIndex, USER_BLOCKED);
    } else {
      DEBUG_FLOW_0(RB_NAME_FORMAT "Consumer state %05d not handled\n", RB_NAME_VALUE, GetConsumerState(consumerIndex));
      assert(0);
    }
  }
  return 0;
}

int32_t CRingBuf::DiscardLeftedData(void) {
  lock(MY_NAME);
  DEBUG_FLOW_0(RB_NAME_FORMAT "Discard %08d byte lefted data\n", RB_NAME_VALUE, m_pRbInfo->leftedBytes);
  m_pRbInfo->leftedBytes = 0;
  unlock();
  return 0;
}

uint8_t *CRingBuf::RequestWriteFrame(uint32_t size, FRAME_E type,
                                     CRB_WRITE_MODE_E mode /*DEFAULT: CRB_WRITE_MODE_NON_BLOCK*/) {
  /*if (0 == strcmp((char *)RB_NAME_VALUE, "CH0-0-V-Stream"))*/
  { DEBUG_FLOW_1("PID#%d RequestWriteFrame size %d type %d\n", syscall(SYS_gettid), size, type); }
  assert(size > 0);
  assert(size <= m_bufSize);
  assert(type < CRB_FRAME_ANY);
  assert(mode == CRB_WRITE_MODE_BLOCK || mode == CRB_WRITE_MODE_NON_BLOCK);
  /*Mixing RequestWriteFrame/RequestWriteContinuousFrames call?*/
  if (0 != m_pRbInfo->leftedBytes) {
    DEBUG_FLOW_0("Drop %d byte lefted data!!\n", m_pRbInfo->leftedBytes);
    m_pRbInfo->leftedBytes = 0;
  }

#ifdef ENABLE_ACCOUNTING
  lock(MY_NAME);
  m_pRbInfo->productors[m_userIdx].requestCount++;
  if (m_lastWriteBytes > 0) {
    m_pRbInfo->productors[m_userIdx].requestWithoutCommitCount++;
  }
  if (CRB_FRAME_DISCOVERY == type || CRB_FRAME_LOG == type) {
    m_pRbInfo->videoBuffer = false;
  }
  unlock();
#endif

  m_pRbInfo->productors[m_userIdx].requestUptime = uptime();
/*
 *When I frame interval is very small, we may have many I frame in buffer
 *Then size of memory can not be 'allocated' till we move all blockers for many times
 *
 *Assume we have 100 I frame in buffer, we should not fail until move blockers for 100 times.
 */
#define MAX_RETRY_TIME (100)
  int32_t guard = 0;
  uint8_t *ret = (uint8_t *)CRB_ERROR_PARAM;
  do {
    lock(MY_NAME);
    DeleteAgedBlackHole();

    uint16_t blockers[MAX_CONSUMER_NUM];
    uint16_t blockNum = IndexFindBlocker(blockers);
    if (blockNum > 0) {
      if (CRB_WRITE_MODE_BLOCK == mode) {
        unlock();
        return (uint8_t *)CRB_ERROR_NO_IDX;
      } else {
        MoveAllBlockers(blockers, blockNum);
      }
    }

/*Only align selected frame type, like CRB_FRAME_I_SLICE*/
#define MAX_CONTINUOUS_SIZE (m_pRbInfo->bufSize / 5)
    if (m_pRbInfo->alignFrameType == type || m_pRbInfo->contiBytesWriten >= MAX_CONTINUOUS_SIZE) {
      ret = __RequestWritePtr(size, m_pRbInfo->alignBytes, blockers, &blockNum);
    } else {
      ret = __RequestWritePtr(size, 0, blockers, &blockNum);
    }

    if (CRB_VALID_ADDRESS(ret)) {
      m_lastWriteFrameType = type;
      unlock();
      break;
    } else {
      if (CRB_WRITE_MODE_BLOCK == mode) {
        unlock();
        return (uint8_t *)CRB_ERROR_NO_IDX;
      } else {
        MoveAllBlockers(blockers, blockNum);
      }
    }
    unlock();
  } while (++guard < MAX_RETRY_TIME);
  if (guard >= MAX_RETRY_TIME) {
    DEBUG_FLOW_0(RB_NAME_FORMAT "Failed to get write %08d bytes buffer\n", RB_NAME_VALUE, size);
    // assert(0);
  }
  return ret;
}