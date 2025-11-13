#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

#include <stdint.h>
// #include "common_include.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define common_attribute_packed
#pragma pack(push)
#pragma pack(1)
#else
#define common_attribute_packed __attribute__((packed))
#endif

#define RB_BUFFERLEN_MAN_AUDIO 32 * 1024
#define RB_BUFFERLEN_MAN_VIDEO 1024 * 1024

// 帧类型 I frame, P frame, B frame …
typedef enum tagVideoFrameType {
  VIDEO_FRAME_MJPEG_FRAME = 0,       /*!< 0 */
  VIDEO_FRAME_IDR_FRAME = 1,         /*!< 1 */
  VIDEO_FRAME_I_FRAME = 2,           /*!< 2 */
  VIDEO_FRAME_P_FRAME = 3,           /*!< 3 */
  VIDEO_FRAME_B_FRAME = 4,           /*!< 4 */
  VIDEO_FRAME_P_FAST_SEEK_FRAME = 5, /*!< 5 */
  VIDEO_FRAME_END_FRAME = 7,         /*!< 7 */
} eVideoFrameType;

typedef enum {
  RBWRITE_MODE_BLOCK = 0,
  RBWRITE_MODE_BLOCK_NON = 1,
} eRbWriteMode;

typedef struct tagRingBufferInfo {
  eRbWriteMode writemode;
  int32_t buflen;
  int32_t msleep;
  uint8_t livestream;
} RingBufferInfo_t;

typedef struct tagRingBufferCtx {
  char username[32];
  RingBufferInfo_t rbinfo;
  void *ringbuf;
} RingBufferCtx_t;

typedef enum tagMsgType {
  MSG_TYPE_VIDEO = 0,
  MSG_TYPE_AUDIO = 1,
  MSG_TYPE_RADAR = 2,
} eMsgType;

typedef struct {
  uint16_t width;       // 图像宽
  uint16_t height;      // 图像宽
  uint8_t frametype;    // 帧类型 I frame, P frame, B frame … eVideoFrameType
  uint8_t encodetype;   // 编码类型，0 H265, 1 H264, 2 MJPEG … eVideoEncodeType
  uint64_t frameIndex;  // 编码类型，0 H265, 1 H264, 2 MJPEG … eVideoEncodeType
  int64_t timestamp;    /**< 时间戳，单位us */
  int64_t wTimestamp;   /**< 时间戳，单位us */
} common_attribute_packed video_head_t;

typedef struct {
  uint8_t audioformat;  // audio_codec_format  0: PCM, 1: AAC, 2: G711A
  uint8_t samplechans;  // eAudioSoundMode
  uint8_t samplebits;   // eAudioBitWidth
  int32_t samplerate;   // 44100, 48000... eAudioSampleRate
  uint64_t frameIndex;  // 编码类型，0 H265, 1 H264, 2 MJPEG … eVideoEncodeType
  int64_t timestamp;    /**< 时间戳，单位us */
  int64_t wTimestamp;   /**< 时间戳，单位us */
} common_attribute_packed audio_head_t;

typedef struct {
  uint8_t meidaType;                   // 帧类型 0 视频 1 音频 2 图片 3 其它
  uint8_t stream_id;                   // 码流类型 0 主码流 1 子码流 g711流
  uint64_t timestamps;                 // 时戳
  long long timeBaseUsec;              // 基准时间 单位是usec
  long long writeRingBufferTimestamp;  // 写入RingBuffer的时间戳 单位是msec
  union {
    video_head_t videoInfo;
    audio_head_t audioInfo;
  } media_info;

  uint32_t framelen;
  uint8_t *framedata;
} common_attribute_packed media_message_t;

int ringbuffer_writer_init(void **ppctx, RingBufferInfo_t *rbinfo, const char *rbname);

int ringbuffer_writer_deinit(void *pctx);

int ringbuffer_writer_data(void *pctx, const char *data, int datalen, int iskey, long long frameTime);

int ringbuffer_writer_data_request(void *pctx, unsigned char **buffer, int bufferlen, int iskey);

int ringbuffer_writer_data_commit(void *pctx, long long frameTime);

int ringbuffer_reader_init(void **ppctx, RingBufferInfo_t *rbinfo, const char *rbname, long long int pretimes);

int ringbuffer_reader_move_to_time(void **ppctx, long long int pretimes);

int ringbuffer_reader_oldest(void **ppctx);

int ringbuffer_reader_latest(void **ppctx);

int ringbuffer_reader_discard_all_data(void **ppctx);

int ringbuffer_reader_deinit(void *pctx);

int ringbuffer_reader_data_request(void *pctx, void **data, int *datalen, uint32_t *iskey);

int ringbuffer_reader_data_commit(void *pctx, void *data, int datalen);

#ifdef __cplusplus
}
#endif

#endif  // __RINGBUFFER_H__