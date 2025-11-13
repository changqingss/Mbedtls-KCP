/**
 * @addtogroup module_Storage
 * @{
 */

/**
 * @file
 * @brief MP4 解码器功能
 * @details 用于MP4文件解码操作
 * @version 1.0.0
 * @author sky.houfei
 * @date 2022-5-18
 */

//*****************************************************************************
#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//*****************************************************************************
#include "g711codec.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/codec_id.h"
#include "libavformat/avformat.h"
#include "libavutil/log.h"
#include "libavutil/mathematics.h"
#include "libavutil/opt.h"
#include "libswresample/swresample.h"
// #include "libswscale/swscale.h"
#include "common/log/log_conf.h"
#include "mp4_demuxer.h"

//*****************************************************************************
#define G711A_NB_SAMPLE 640     // G711A 8K采样率对应的采样个数640， 16K为 600
#define AUDIO_SAMPLE_RATE 8000  // 音频采样率
#define MAX_AUDIO_FRAME_SIZE 192000

//*****************************************************************************

#define AUDIO_TYPE_AAC_ENABLE (1)

/*
 *@brief 获取视频帧率
 *@details 根据视频帧数，计算视频帧率，如果视频不足10 fps，则返回15 fps
 *@param[in] mp4DemuxerInfo* demuxer 解码器
 *@return Framerate 视频帧率，单位 fps
 */
int MP4_DEMUXER_GetFrameRate(mp4DemuxerInfo *demuxer) {
  int i = 0;
  int videoStream = 0;
  int Framerate = 0;
  AVFormatContext *ic = demuxer->formatContext;

  if (ic == NULL) {
    COMMONLOG_E("%s %d: AVFormatContext *ic == NULL\n", __func__, __LINE__);
    return -1;
  }

  if (ic->nb_streams <= 0) {
    COMMONLOG_E("%s %d: ic->nb_streams <= 0\n", __func__, __LINE__);
    return -1;
  }

  for (i = 0; i < (ic->nb_streams); i++) {
    if (ic->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      videoStream = i;
      break;
    }
  }

  AVStream *stream = ic->streams[videoStream];

  COMMONLOG_I(
      "i = %d, stream->avg_frame_rate.num = %d, "
      "stream->avg_frame_rate.den = %d\n",
      i, stream->avg_frame_rate.num, stream->avg_frame_rate.den);

  if (stream->avg_frame_rate.num > 0 && stream->avg_frame_rate.den > 0) {
    Framerate = stream->avg_frame_rate.num / stream->avg_frame_rate.den;
  }
  if (Framerate < 10) {
    COMMONLOG_I("real framerate = %d, force set 15 !\n", Framerate);
    Framerate = 15;
  } else {
    COMMONLOG_I("framerate = %d\n", Framerate);
  }

  return Framerate;
}

/*
 *@brief 获取视频帧个数
 *@details 无
 *@param[in] mp4DemuxerInfo* demuxer 解码器
 *@return 返回视频帧个数，单位帧
 */
int MP4_DEMUXER_GetFrameNums(mp4DemuxerInfo *demuxer) {
  int i = 0;
  int videoStream = 0;

  AVFormatContext *ic = demuxer->formatContext;

  if (ic == NULL) {
    COMMONLOG_E("AVFormatContext *ic == NULL\n");
    return -1;
  }

  for (i = 0; i < (ic->nb_streams); i++) {
    if (ic->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      videoStream = i;
      break;
    }
  }

  if (ic->nb_streams <= 0) {
    COMMONLOG_E("ic->nb_streams <= 0");
    return -1;
  }

  AVStream *stream = ic->streams[videoStream];
  if (stream == NULL) {
    COMMONLOG_E("stream == NULL");
    return -1;
  }

  COMMONLOG_I("stream->nb_frames = %llu\n", stream->nb_frames);

  return stream->nb_frames;
}

/*
 *@brief 查找视频某个时刻点位置
 *@details 无
 *@param[in] mp4DemuxerInfo* demuxer 解码器
 *@param[in] int second, 查找视频的第几秒位置，单位秒
 *@return 成功返回0，失败返回其他
 */
int MP4_DEMUXER_Seek(mp4DemuxerInfo *demuxer, int second) {
  int i = 0;
  int videoStream = 0;
  int ret = -1;
  AVFormatContext *ic = demuxer->formatContext;

  if (ic == NULL) {
    COMMONLOG_E("AVFormatContext *ic == NULL\n");
    return -1;
  }

  for (i = 0; i < (ic->nb_streams); i++) {
    if (ic->streams[i]->codec->codec_type == 0) {
      videoStream = i;
      break;
    }
  }

  ret = av_seek_frame(ic, -1, second * AV_TIME_BASE, AVSEEK_FLAG_BACKWARD);
  COMMONLOG_I("avformat_seek_file ret %d\n", ret);

  return ret;
}

/*
 *@brief 解码器初始化，打开视频文件，并设置解码器参数
 *@details 支持视频和音频解码，其中视频支持 MP4的
 *H264解码，音频支持PCM_ALAW(G711A)解码
 *@param[in/out] mp4DemuxerInfo* demuxer 解码器
 *@param[in] char* fileName, 需要解码的视频文件名称
 */
#if 0
void* MP4_DEMUXER_Init(mp4DemuxerInfo* demuxer, char* fileName)
{
	AVCodecContext *a_codecctx = demuxer->codecContext;
	AVCodec *a_codec;
	int ret = 0;

	avcodec_register_all();
	demuxer->formatContext = avformat_alloc_context();
	AVFormatContext *ifmt_ctx = demuxer->formatContext;

	//Input
	if ((ret = avformat_open_input(&ifmt_ctx, fileName, 0, 0)) < 0) {
		printf( "Could not open input file: %s\n", fileName);
		return -1;
	}
	
	if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
		printf( "Failed to retrieve input stream information");
		return -1;
	}

	av_dump_format(ifmt_ctx, 0, fileName, 0);	// 打印视频文件基本信息
	a_codecctx = ifmt_ctx->streams[AVMEDIA_TYPE_AUDIO]->codec;
	a_codec = avcodec_find_decoder(a_codecctx->codec_id);
	if (a_codec == NULL)
	{
		printf("find codec fail\n");
	}

	if (avcodec_open2(a_codecctx,a_codec,NULL) < 0)
	{
		printf("error avcodec_open2\n");
		return -1;
	}

	// 音频转码器参数设置
	uint64_t out_channel_layout = AV_CH_LAYOUT_MONO;   			//单通道
	int out_nb_samples = G711A_NB_SAMPLE;						// 采样个数 
	enum AVSampleFormat  out_sample_fmt = AV_SAMPLE_FMT_S16;	//采样格式, 16bit 采样位数
	int out_sample_rate = AUDIO_SAMPLE_RATE; 					//采样率
	int out_channels = av_get_channel_layout_nb_channels(out_channel_layout); 	// 通道数
	int buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1); 	//创建buffer
	demuxer->audioSize = buffer_size;
	int64_t in_channel_layout = av_get_default_channel_layout(a_codecctx->channels);	// 获取输入音频的通道个数

	//打开音频转码器，设置参数
	demuxer->convertContext =  swr_alloc();
	demuxer->convertContext = swr_alloc_set_opts(demuxer->convertContext, 
	out_channel_layout, out_sample_fmt, out_sample_rate,
	in_channel_layout, a_codecctx->sample_fmt, a_codecctx->sample_rate,
	0, NULL);

	//初始音频化转码器
	swr_init(demuxer->convertContext);

	// 设置视频解码器
	demuxer->bitStreamFileContext = av_bitstream_filter_init("h264_mp4toannexb");
	
	return 0;
}
#else

int MP4_DEMUXER_Init(mp4DemuxerInfo *demuxer, char *fileName) {
  AVCodecContext *a_codecctx = demuxer->codecContext;
  AVCodec *a_codec;
  int ret = 0;

  COMMONLOG_I("fileName:%s\n", fileName);
  avcodec_register_all();

  demuxer->formatContext = avformat_alloc_context();

  // Input
  if ((ret = avformat_open_input(&demuxer->formatContext, fileName, NULL, NULL)) < 0) {
    COMMONLOG_E("Could not open input file: %s\n", fileName);
    return -1;
  }

  COMMONLOG_I("\n---------- start time:%u  -----------\n", time(NULL));
  demuxer->formatContext->probesize = 128 * 1024;
  demuxer->formatContext->max_analyze_duration = 2;

  if (AUDIO_TYPE_AAC_ENABLE) {
    // #ifdef PLATFORM_INGENIC_T23
    demuxer->formatContext->video_codec_id = AV_CODEC_ID_H264;
    // #else
    //     demuxer->formatContext->video_codec_id = AV_CODEC_ID_H265;
    // #endif
    demuxer->formatContext->audio_codec_id = AV_CODEC_ID_AAC;
  } else {
    demuxer->formatContext->video_codec_id = AV_CODEC_ID_H264;
    demuxer->formatContext->audio_codec_id = AV_CODEC_ID_PCM_ALAW;
  }

  if ((ret = avformat_find_stream_info(demuxer->formatContext, NULL)) < 0) {
    COMMONLOG_E("Failed to retrieve input stream information");
    return -1;
  }
  COMMONLOG_I("\n---------- end time:%u  -----------\n", time(NULL));
  av_dump_format(demuxer->formatContext, 0, fileName,
                 0);  // 打印视频文件基本信息

  if (AUDIO_TYPE_AAC_ENABLE) {
    // demuxer->bitStreamFileContext =
    // av_bitstream_filter_init("hevc_mp4toannexb");
  } else {
    a_codecctx = demuxer->formatContext->streams[AVMEDIA_TYPE_AUDIO]->codec;
    a_codec = avcodec_find_decoder(a_codecctx->codec_id);
    if (a_codec == NULL) {
      COMMONLOG_E("find codec fail\n");
    }

    if (avcodec_open2(a_codecctx, a_codec, NULL) < 0) {
      COMMONLOG_E("error avcodec_open2\n");
      return -1;
    }
    // 音频转码器参数设置
    uint64_t out_channel_layout = AV_CH_LAYOUT_MONO;                           // 单通道
    int out_nb_samples = G711A_NB_SAMPLE;                                      // 采样个数
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;                    // 采样格式, 16bit 采样位数
    int out_sample_rate = AUDIO_SAMPLE_RATE;                                   // 采样率
    int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);  // 通道数
    int buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);  // 创建buffer
    demuxer->audioSize = buffer_size;
    int64_t in_channel_layout = av_get_default_channel_layout(a_codecctx->channels);  // 获取输入音频的通道个数

    // 打开音频转码器，设置参数
    demuxer->convertContext = swr_alloc();
    demuxer->convertContext =
        swr_alloc_set_opts(demuxer->convertContext, out_channel_layout, out_sample_fmt, out_sample_rate,
                           in_channel_layout, a_codecctx->sample_fmt, a_codecctx->sample_rate, 0, NULL);

    // 初始音频化转码器
    swr_init(demuxer->convertContext);
    // 设置视频解码器
    demuxer->bitStreamFileContext = av_bitstream_filter_init("h264_mp4toannexb");
  }

  return 0;
}

#endif

/*
 *@brief 解码器反初始化，关闭打开的解码器和视频文件
 */
int MP4_DEMUXER_UnInit(mp4DemuxerInfo *demuxer) {
  if (AUDIO_TYPE_AAC_ENABLE) {
    if (demuxer->formatContext) {
      COMMONLOG_I("----------------free demuxer----------------\n");
      avformat_close_input(&(demuxer->formatContext));
    }
  } else {
    COMMONLOG_I("----------------free demuxer----------------\n");
    av_bitstream_filter_close(demuxer->bitStreamFileContext);

    avcodec_close(demuxer->formatContext->streams[AVMEDIA_TYPE_AUDIO]->codec);
    avformat_close_input(&(demuxer->formatContext));
    swr_free(&(demuxer->convertContext));
    avformat_free_context(demuxer->formatContext);
  }
  return 0;
}

/*
 *@brief 获取数据帧
 *@details 使用ffmepge
 *库，获取视频或者音频数据帧，取出来的可能是音频，也可能是视频数据
 *@param[in] mp4DemuxerInfo* demuxer 视频解码器
 *@param[out] int* frameType, 帧类型，代表音频帧或者视频帧
 *@param[out] uint8_t* frameData, 帧数据内容
 *@param[int] int frameMaxLen, 帧数据最大长度，单位字节，代表 frameData
 *最大缓存多少字节
 *@param[out] int* frameSize, 帧数据大小，单位字节
 *@param[out] int64_t* duration, 视频或者音频帧，持续时间，单位微秒 us
 *@param[out] int64_t* pts, 视频或者音频帧，pts时间，单位微秒 us
 *@return 成功返回 DEMUXER_SUCCESS
 * 失败返回其他，数据类型参考 @ref DemuxerResult 定义
 */
#if 0
DemuxerResult MP4_DEMUXER_GetFrame(mp4DemuxerInfo* demuxer, int* frameType, uint8_t* frameData, int frameMaxLen, 

								int* frameSize, int64_t* duration, int64_t* pts)
{
	//AVPacket* pkt = av_packet_alloc();
	AVPacket pkt = { 0 };
	int ret = -1;
	int getAudio = 0;
	//AVFrame *frame = av_frame_alloc();
	AVFrame frame = {0};
	unsigned char *h264_buf = NULL;
	int h264_len;
	uint8_t *pcmBuf = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);	//  192000 * 2

	if (demuxer->formatContext->streams[1]->codec == NULL)
	{
		printf("%s %d codecContext = null\n",  __func__, __LINE__);
	}

	demuxer->codecContext = demuxer->formatContext->streams[AVMEDIA_TYPE_AUDIO]->codec;
	ret = av_read_frame(demuxer->formatContext, &pkt);	// 去取一帧数据
	if (ret >= 0)
	{
		if(pkt.stream_index == AVMEDIA_TYPE_VIDEO)		// 视频帧
		{
			// 特别注意，av_bitstream_filter_filter 的 poutbuf 为双重指针，拷贝需要深拷贝，不能浅拷贝
			av_bitstream_filter_filter(demuxer->bitStreamFileContext, 
				demuxer->formatContext->streams[AVMEDIA_TYPE_VIDEO]->codec, NULL, 
				&h264_buf, &h264_len, 
				pkt.data, pkt.size, 0);
			if (h264_len > frameMaxLen)
			{
				printf("Warnning, frame data is %d > %d (max len), abandon it \n", h264_len, frameMaxLen);
				av_free(h264_buf);
				av_free(pcmBuf);
				//av_packet_free(&pkt);
				av_packet_unref(&pkt);
				//av_frame_free(&frame);
				av_frame_unref(&frame);
				return DEMUXER_FRAME_TOO_BIG;
			}
			*frameSize = h264_len;
			*frameType = pkt.stream_index;
			memcpy(frameData, h264_buf, h264_len);
			// pts 和 duration 单位转换为 us (AV_TIME_BASE 时间基，单位为us)
			*duration = av_rescale_q(pkt.duration, demuxer->formatContext->streams[AVMEDIA_TYPE_VIDEO]->time_base, AV_TIME_BASE_Q);
			*pts = av_rescale_q(pkt.pts, demuxer->formatContext->streams[AVMEDIA_TYPE_VIDEO]->time_base, AV_TIME_BASE_Q);
			av_free(h264_buf);	// 解码后的数据为双重指针，需要释放最底层的数据，否则会有内存泄漏
			
		}
		else if(pkt.stream_index == AVMEDIA_TYPE_AUDIO)	// 音频帧
		{ 
			if (avcodec_decode_audio4(demuxer->codecContext, &frame, &getAudio, &pkt) < 0)
			{
				printf("error  avcodec_decode_audio4\n");
				av_free(pcmBuf);
				//av_packet_free(&pkt);
				av_packet_unref(&pkt);
				//av_frame_free(&frame);
				av_frame_unref(&frame);
				return DEMUXER_AUDIO_FAIL;
			}
			
			if(getAudio > 0)
			{
				// 转码输出PCM音频
				swr_convert(demuxer->convertContext, &pcmBuf, demuxer->audioSize, (const uint8_t **)frame.data, frame.nb_samples); 
				*frameSize = PCM2G711a(pcmBuf, frameData, demuxer->audioSize, 0);  // 转码为 G711A
				*frameType = pkt.stream_index;
				// pts 和 duration 单位转换为 us (AV_TIME_BASE 时间基，单位为us)
				*duration = av_rescale_q(pkt.duration, demuxer->formatContext->streams[AVMEDIA_TYPE_AUDIO]->time_base, AV_TIME_BASE_Q);
				*pts = av_rescale_q(pkt.pts, demuxer->formatContext->streams[AVMEDIA_TYPE_AUDIO]->time_base, AV_TIME_BASE_Q);
			}
		   	getAudio = 0;
		}
		ret = 0;
	}
	else
	{
		//printf("Can not read the file stream, may be is read end, ret = %d\n", ret);
	}

	// 释放内存	
	av_free(pcmBuf);
	//av_packet_free(&pkt);
	av_packet_unref(&pkt);
	//av_frame_free(&frame);
	av_frame_unref(&frame);
	return ret;
}

#else

DemuxerResult MP4_DEMUXER_GetFrame(mp4DemuxerInfo *demuxer, int *frameType, uint8_t **frameData, int *frameMaxLen,

                                   int *frameSize, int64_t *duration, int64_t *pts, int *IFlag) {
  int ret = -1, getAudio = 0, h264_len = 0, streamIndex = 0;
  uint8_t *pcmBuf = NULL;
  AVPacket pkt = {0};
  AVFrame frame = {0};

  const AVBitStreamFilter *bsf = NULL;
  AVBSFContext *ctx = NULL;
  AVCodecParameters *codecpar = NULL;

  if (demuxer->formatContext->streams[1]->codec == NULL) {
    COMMONLOG_E("codecContext = null\n");
  }

  demuxer->codecContext = demuxer->formatContext->streams[AVMEDIA_TYPE_AUDIO]->codec;
  ret = av_read_frame(demuxer->formatContext, &pkt);  // 去取一帧数据
  if (ret >= 0) {
    streamIndex = pkt.stream_index;
    if (AUDIO_TYPE_AAC_ENABLE) {
      if (streamIndex == AVMEDIA_TYPE_VIDEO) {
        // #ifdef PLATFORM_INGENIC_T23
        bsf = av_bsf_get_by_name("h264_mp4toannexb");
        // #else
        //         bsf = av_bsf_get_by_name("hevc_mp4toannexb");
        // #endif
      } else if (streamIndex == AVMEDIA_TYPE_AUDIO) {
        bsf = av_bsf_get_by_name("aac_adtstoasc");
      } else {
        COMMONLOG_E("streamIndex = %d Unkonw!\n", streamIndex);
        return -1;
      }

      if (!bsf) {
        COMMONLOG_E("Warnning, frame data is %d > %d (max len), abandon it \n", h264_len, *frameMaxLen);
        return DEMUXER_FRAME_TOO_BIG;
      }
      ret = av_bsf_alloc(bsf, &ctx);
      codecpar = demuxer->formatContext->streams[streamIndex]->codecpar;
      ret = avcodec_parameters_copy(ctx->par_in, codecpar);
      if (ret < 0) {
        COMMONLOG_E("in avcodec_parameters_copy fail\n");
      }
      ctx->time_base_in = demuxer->formatContext->streams[streamIndex]->time_base;
      ret = av_bsf_init(ctx);
      ret = av_bsf_send_packet(ctx, &pkt);
      if (ret < 0) {
        COMMONLOG_E("error! ret = 0x%x\n", ret);
      } else {
        ret = av_bsf_receive_packet(ctx, &pkt);
        if (ret < 0) {
          COMMONLOG_E("error! ret = 0x%x\n", ret);
        }
      }

      *frameSize = pkt.size;
      *frameType = pkt.stream_index;
      *IFlag = pkt.flags;
      if (pkt.size > *frameMaxLen || *frameMaxLen == 0) {
        if (*frameData) {
          free(*frameData);
          *frameData = NULL;
        }

        *frameData = (uint8_t *)malloc(pkt.size + 1);
        if (*frameData == NULL) {
          COMMONLOG_E("malloc frame data failed\n");
          return -1;
        }
        *frameMaxLen = pkt.size + 1;
        memset(*frameData, 0, pkt.size + 1);
      }
      memcpy(*frameData, pkt.data, pkt.size);

      // pts 和 duration 单位转换为 us (AV_TIME_BASE 时间基，单位为us)
      if (streamIndex == AVMEDIA_TYPE_VIDEO) {
        *duration =
            av_rescale_q(pkt.duration, demuxer->formatContext->streams[AVMEDIA_TYPE_VIDEO]->time_base, AV_TIME_BASE_Q);
        *pts = av_rescale_q(pkt.pts, demuxer->formatContext->streams[AVMEDIA_TYPE_VIDEO]->time_base, AV_TIME_BASE_Q);
      } else if (streamIndex == AVMEDIA_TYPE_AUDIO) {
        *duration =
            av_rescale_q(pkt.duration, demuxer->formatContext->streams[AVMEDIA_TYPE_AUDIO]->time_base, AV_TIME_BASE_Q);
        *pts = av_rescale_q(pkt.pts, demuxer->formatContext->streams[AVMEDIA_TYPE_AUDIO]->time_base, AV_TIME_BASE_Q);
      }

      av_bsf_free(&ctx);

    } else {
      if (pkt.stream_index == AVMEDIA_TYPE_VIDEO)  // 视频帧
      {
        bsf = av_bsf_get_by_name("h264_mp4toannexb");
        // bsf = av_bsf_get_by_name("hevc_mp4toannexb");

        if (!bsf) {
          COMMONLOG_E("Warnning, frame data is %d > %d (max len), abandon it \n", h264_len, *frameMaxLen);
          return DEMUXER_FRAME_TOO_BIG;
        }
        ret = av_bsf_alloc(bsf, &ctx);
        codecpar = demuxer->formatContext->streams[streamIndex]->codecpar;
        ret = avcodec_parameters_copy(ctx->par_in, codecpar);
        if (ret < 0) {
          COMMONLOG_E("in avcodec_parameters_copy fail\n");
        }
        ctx->time_base_in = demuxer->formatContext->streams[streamIndex]->time_base;
        ret = av_bsf_init(ctx);
        ret = av_bsf_send_packet(ctx, &pkt);
        if (ret < 0) {
          COMMONLOG_E("error! ret = 0x%x\n", ret);
        } else {
          ret = av_bsf_receive_packet(ctx, &pkt);
          if (ret < 0) {
            COMMONLOG_E("error! ret = 0x%x\n", ret);
          }
        }
        av_bsf_free(&ctx);

        *frameSize = pkt.size;
        *frameType = pkt.stream_index;
        *IFlag = pkt.flags;
        if (pkt.size > *frameMaxLen || *frameMaxLen == 0) {
          if (*frameData) {
            free(*frameData);
            *frameData = NULL;
          }

          *frameData = (uint8_t *)malloc(pkt.size + 1);
          if (*frameData == NULL) {
            COMMONLOG_E("malloc frame data failed\n");
            return -1;
          }
          *frameMaxLen = pkt.size + 1;
          memset(*frameData, 0, pkt.size + 1);
        }

        memcpy(*frameData, pkt.data, pkt.size);
        // pts 和 duration 单位转换为 us (AV_TIME_BASE 时间基，单位为us)
        *duration =
            av_rescale_q(pkt.duration, demuxer->formatContext->streams[AVMEDIA_TYPE_VIDEO]->time_base, AV_TIME_BASE_Q);
        *pts = av_rescale_q(pkt.pts, demuxer->formatContext->streams[AVMEDIA_TYPE_VIDEO]->time_base, AV_TIME_BASE_Q);

      } else if (pkt.stream_index == AVMEDIA_TYPE_AUDIO)  // 音频帧
      {
        pcmBuf = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);  //  192000 * 2
        // printf("-------------------------------pkt len:%d\n", pkt.size);
        if (avcodec_decode_audio4(demuxer->codecContext, &frame, &getAudio, &pkt) < 0) {
          COMMONLOG_E("error  avcodec_decode_audio4\n");
          av_free(pcmBuf);
          av_packet_unref(&pkt);
          av_frame_unref(&frame);
          return DEMUXER_AUDIO_FAIL;
        }
        // printf("-------------------------------audio frame len:%d\n",
        // frame.pkt_size);
        if (getAudio > 0) {
          // 转码输出PCM音频
          swr_convert(demuxer->convertContext, &pcmBuf, demuxer->audioSize, (const uint8_t **)frame.data,
                      frame.nb_samples);
          *frameSize = PCM2G711a(pcmBuf, *frameData, demuxer->audioSize,
                                 0);  // 转码为 G711A
          *frameType = pkt.stream_index;
          // pts 和 duration 单位转换为 us (AV_TIME_BASE 时间基，单位为us)
          *duration = av_rescale_q(pkt.duration, demuxer->formatContext->streams[AVMEDIA_TYPE_AUDIO]->time_base,
                                   AV_TIME_BASE_Q);
          *pts = av_rescale_q(pkt.pts, demuxer->formatContext->streams[AVMEDIA_TYPE_AUDIO]->time_base, AV_TIME_BASE_Q);
        }
        getAudio = 0;
        av_free(pcmBuf);
        av_frame_unref(&frame);
      }
    }
    ret = 0;
  } else {
    // printf("Can not read the file stream, may be is read end, ret = %d\n",
    // ret);
  }

  // 释放内存
  // av_packet_free(&pkt);
  av_packet_unref(&pkt);
  // av_frame_free(&frame);
  // av_frame_unref(&frame);
  return ret;
}

#endif

int libAVRecordPlayBackGetFrame(void *pPlaybackId, int *pAVType, char *pAVData, int *pAVDataSize) {
  int ret = -1, streamIndex = 0;
  AVFormatContext *ic = (AVFormatContext *)pPlaybackId;
  AVBSFContext *ctx = NULL;
  const AVBitStreamFilter *bsf = NULL;
  AVCodecParameters *codecpar = NULL;
  AVPacket pkt = {0};

  if (ic == NULL) {
    COMMONLOG_E("AVFormatContext *ic == NULL\n");
    goto end0;
  }

  if (pPlaybackId == NULL || pAVType == NULL || pAVData == NULL || pAVDataSize == NULL) {
    COMMONLOG_E(
        "pPlaybackId %p == NULL || pAVType %p == NULL || pAVData %p == "
        "NULL || pAVDataSize %p == NULL\n",
        pPlaybackId, pAVType, pAVData, pAVDataSize);
    goto end0;
  }

  ret = av_read_frame(ic, &pkt);
  if (ret < 0) {
    COMMONLOG_E("read frame error %d, may be end of file\n", ret);
    goto end1;
  }
  streamIndex = pkt.stream_index;
  if (streamIndex == AVMEDIA_TYPE_VIDEO) {
    bsf = av_bsf_get_by_name("h264_mp4toannexb");

  } else if (streamIndex == AVMEDIA_TYPE_AUDIO) {
    bsf = av_bsf_get_by_name("aac_adtstoasc");
  } else {
    COMMONLOG_E("streamIndex = %d Unkonw!\n", streamIndex);
    goto end0;
  }

  if (!bsf) {
    COMMONLOG_E("streamIndex = %d Unkonw bitstream filter!\n", streamIndex);
    goto end0;
  }
  ret = av_bsf_alloc(bsf, &ctx);
  // if(ic->streams[streamIndex]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
  codecpar = ic->streams[streamIndex]->codecpar;
  ret = avcodec_parameters_copy(ctx->par_in, codecpar);
  if (ret < 0) {
    COMMONLOG_E("in avcodec_parameters_copy fail\n");
  }
  ctx->time_base_in = ic->streams[streamIndex]->time_base;
  ret = av_bsf_init(ctx);
  //}
  ret = av_bsf_send_packet(ctx, &pkt);
  if (ret < 0) {
    COMMONLOG_E("error! ret = 0x%x\n", ret);
  } else {
    ret = av_bsf_receive_packet(ctx, &pkt);
    if (ret < 0) {
      COMMONLOG_E("error! ret = 0x%x\n", ret);
    }
  }
  av_bsf_free(&ctx);

  memcpy(pAVData, pkt.data, pkt.size);
  *pAVType = pkt.stream_index;
  *pAVDataSize = pkt.size;

end1:
  av_packet_unref(&pkt);
end0:
  return ret;
}

/** @} */
