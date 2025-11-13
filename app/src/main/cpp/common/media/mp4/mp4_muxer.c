/**
 * @addtogroup module_Storage
 * @{
 */

/**
 * @file
 * @brief MP4 多媒体功能
 * @details 用于MP4文件编码和解码操作
 * @version 1.0.0
 * @author sky.houfei
 * @date 2022-5-12
 */

//*****************************************************************************
#include "mp4_muxer.h"

#include <arpa/inet.h>
#include <asm/types.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libavcodec/avcodec.h>
#include <limits.h>
#include <linux/fs.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <netdb.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/vfs.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common/log/log_conf.h"
#include "common/param_config/dev_param.h"
#include "g711codec.h"
//**************************************************************************

#define MP4_TIME_SCALE 9000000  // mp4 视频文件精度

#define DEFAULT_AUDIO_SAMPLE_RATE 16000  // 16K 采样率

MP4_TYPE_E mp4Type = MP4_TYPE_AAC_H264;

//**************************************************************************
int MP4_MUXER_Open(struct AVRecord_t* pRecord, const char* pFileName) {
  int ret;

  av_register_all();
  ret = avformat_alloc_output_context2(&pRecord->m_fmtCtx, NULL, NULL, pFileName);
  if (ret < 0) {
    printf("Could not allocate memory for avformat ctx!\n");
    goto fail;
  }

  // video:
  if (mp4Type == MP4_TYPE_AAC_H265) {
    pRecord->m_fmtCtx->oformat->video_codec = AV_CODEC_ID_H265;
  } else {
    pRecord->m_fmtCtx->oformat->video_codec = AV_CODEC_ID_H264;
  }

  // audio:
  if (mp4Type == MP4_TYPE_AAC_H264 || mp4Type == MP4_TYPE_AAC_H265) {
    pRecord->m_fmtCtx->oformat->audio_codec = AV_CODEC_ID_AAC;
  } else {
    pRecord->m_fmtCtx->oformat->audio_codec = AV_CODEC_ID_PCM_ALAW;
  }

  pRecord->pVStream = avformat_new_stream(pRecord->m_fmtCtx, NULL);
  if (!pRecord->pVStream) {
    printf("Could not allocate pVStream");
    goto fail;
  }

  pRecord->pAStream = avformat_new_stream(pRecord->m_fmtCtx, NULL);
  if (!pRecord->pAStream) {
    printf("Could not allocate pAStream");
    goto fail;
  }

  /* Video Stream info */
  if (mp4Type == MP4_TYPE_AAC_H265) {
    pRecord->pVStream->codecpar->codec_id = AV_CODEC_ID_H265;
  } else {
    pRecord->pVStream->codecpar->codec_id = AV_CODEC_ID_H264;
  }
  pRecord->pVStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
  pRecord->pVStream->codecpar->width = 640;   // 2304; //1920;
  pRecord->pVStream->codecpar->height = 360;  // 1296; //1080;
  pRecord->pVStream->time_base.num = 1;
  pRecord->pVStream->time_base.den = 15;  // MP4_TIME_SCALE;

  /* Audio Stream info  */
  if (mp4Type == MP4_TYPE_AAC_H265) {
    pRecord->pAStream->codecpar->codec_id = AV_CODEC_ID_AAC;
    pRecord->pAStream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    pRecord->pAStream->codecpar->channels = 1;
    pRecord->pAStream->codecpar->sample_rate = DEFAULT_AUDIO_SAMPLE_RATE;
    pRecord->pAStream->time_base.num = 1;
    pRecord->pAStream->time_base.den = 16000;  // MP4_TIME_SCALE;
    pRecord->pAStream->codecpar->frame_size = 768;
  } else if (mp4Type == MP4_TYPE_AAC_H264) {
    pRecord->pAStream->codecpar->codec_id = AV_CODEC_ID_AAC;
    pRecord->pAStream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    pRecord->pAStream->codecpar->channels = 1;
    pRecord->pAStream->codecpar->sample_rate = DEFAULT_AUDIO_SAMPLE_RATE;
    pRecord->pAStream->time_base.num = 1;
    pRecord->pAStream->time_base.den = DEFAULT_AUDIO_SAMPLE_RATE;  // MP4_TIME_SCALE;
    pRecord->pAStream->codecpar->frame_size = 768;
  } else {
    printf(
        "---------------------------------------------------  G711A "
        "------------------------------------------------\n\n\n\n\n");
    pRecord->pAStream->codecpar->codec_id = AV_CODEC_ID_PCM_ALAW;
    pRecord->pAStream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    pRecord->pAStream->codecpar->channels = 1;
    pRecord->pAStream->codecpar->sample_rate = DEFAULT_AUDIO_SAMPLE_RATE;
    pRecord->pAStream->time_base.num = 1;
    pRecord->pAStream->time_base.den = 8000;  // 16000;//MP4_TIME_SCALE;
    pRecord->pAStream->codecpar->frame_size = 384;
  }
  // 录制视频起始时间
  pRecord->startTimestamp = 0;

  printf("==========Output Information=======%d===\n", __LINE__);
  av_dump_format(pRecord->m_fmtCtx, 0, pFileName, 1);

  AVOutputFormat* pFmtOutCtx = pRecord->m_fmtCtx->oformat;
  if (!(pFmtOutCtx->flags & AVFMT_NOFILE)) {
    ret = avio_open(&pRecord->m_fmtCtx->pb, pFileName, AVIO_FLAG_WRITE);
    if (ret < 0) {
      printf("could not open %s\n", pFileName);
      goto fail;
    }
  }

  ret = avformat_write_header(pRecord->m_fmtCtx, NULL);
  if (ret < 0) {
    printf("Error occurred when opening output file \n");
    goto fail;
  }

  return 0;

fail:
  if (pRecord->m_fmtCtx && !(pRecord->m_fmtCtx->flags & AVFMT_NOFILE)) {
    avio_close(pRecord->m_fmtCtx->pb);
  }
  avformat_free_context(pRecord->m_fmtCtx);
  return -1;
}

int MP4_MUXER_Close(struct AVRecord_t* pRecord) {
  if (pRecord->m_fmtCtx != NULL) {
    av_write_trailer(pRecord->m_fmtCtx);
    if (pRecord->m_fmtCtx && !(pRecord->m_fmtCtx->flags & AVFMT_NOFILE)) {
      avio_close(pRecord->m_fmtCtx->pb);
    }
    avformat_free_context(pRecord->m_fmtCtx);
  }
  return 0;
}

int MP4_MUXER_WriteFrame(struct AVRecord_t* pRecord, enum FRAMETYPE_e type, unsigned char* buf, int64_t timestamp,
                         unsigned int size, int muteFlag) {
  AVPacket* pkt = av_packet_alloc();
  int64_t ts = 0;
  int64_t newTimestap = 0;
  int ret = 0;

  if (pRecord->m_fmtCtx == NULL) {
    printf("Video file is not opened\n");
    ret = -1;
    goto end;
  }

  if (type == FRAME_AUDIO) {
    pkt->stream_index = pRecord->pAStream->index;
    if (pRecord->startTimestamp == 0) {
      // 音频第一帧时间，作为起始时间，视频帧需要等到音频帧录制之后才开始计时
      pRecord->startTimestamp = timestamp;
      ts = 0;
    } else {
      ts = timestamp - pRecord->startTimestamp;
    }

    newTimestap = av_rescale_q(ts, AV_TIME_BASE_Q, pRecord->pAStream->time_base);
    pkt->pts = newTimestap;
    pkt->dts = pkt->pts;
  } else {
    pkt->stream_index = pRecord->pVStream->index;
    if (pRecord->startTimestamp == 0) {
      // 统一以音频第一帧作为起始时间，保证音视频同步
      // 当音频帧没有录制之前，视频帧时间持续设置为0，保证音频视频同步
      pRecord->startTimestamp = timestamp;
      ts = 0;
    } else {
      ts = timestamp - pRecord->startTimestamp;
    }

    newTimestap = av_rescale_q(ts, AV_TIME_BASE_Q, pRecord->pVStream->time_base);
    pkt->pts = newTimestap;
    pkt->dts = pkt->pts;

    // I Frame
    if (type == FRAME_VIDEO_I) {
      pkt->flags = AV_PKT_FLAG_KEY;
    }
  }

  if (muteFlag && (type == FRAME_AUDIO)) {
    unsigned char* data_buf = (unsigned char*)malloc(sizeof(unsigned char) * size);
    memcpy(data_buf, buf, size);
    pkt->data = data_buf;
    pkt->size = size;
    memset(data_buf + AAC_AUDIO_FRAME_HEAD_LEN, 0x5a, size - AAC_AUDIO_FRAME_HEAD_LEN);
    ret = av_interleaved_write_frame(pRecord->m_fmtCtx, pkt);
    if (data_buf) {
      free(data_buf);
      data_buf = NULL;
    }
  } else {
    pkt->data = buf;
    pkt->size = size;
    ret = av_interleaved_write_frame(pRecord->m_fmtCtx, pkt);
  }

end:
  av_packet_free(&pkt);

  return ret;
}

struct mp4Muxer_struct bal_mp4Muxer = {
    .name = "bal_mp4Muxer",
    .mp4Muxer_open = MP4_MUXER_Open,
    .mp4Muxer_close = MP4_MUXER_Close,
    .mp4Muxer_write = MP4_MUXER_WriteFrame,
};

void* bal_mp4Muxer_new(void) { return &bal_mp4Muxer; }
/** @} */
