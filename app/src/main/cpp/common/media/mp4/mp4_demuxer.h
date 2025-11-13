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
#ifndef MP4_DEMUXER_H_
#define MP4_DEMUXER_H_

//*****************************************************************************
#ifdef __cplusplus
extern "C" {
#endif

//*****************************************************************************
#include <ctype.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libavcodec/avcodec.h"
#include "libavcodec/codec_id.h"
#include "libavformat/avformat.h"
#include "libavutil/log.h"
#include "libavutil/mathematics.h"
#include "libavutil/opt.h"
#include "libswresample/swresample.h"
// #include "libswscale/swscale.h"

//*****************************************************************************
typedef struct mp4DemuxerInfoSturct {
  AVFormatContext* formatContext;                  ///< 格式化文本
  AVBitStreamFilterContext* bitStreamFileContext;  ///< 视频解码器
  AVBSFContext* bsf_ctx;                           ///< bit流过滤器上下文
  struct SwrContext* convertContext;               ///< 音频转码器
  AVCodecContext* codecContext;                    ///< 音频编码译码器
  int audioSize;                                   ///< 音频数据大小，单位字节
} mp4DemuxerInfo;

typedef enum {
  DEMUXER_SUCCESS = 0,        ///< 解码成功
  DEMUXER_END = 1,            ///< 解码到文件结束
  DEMUXER_FRAME_TOO_BIG = 2,  ///< 解码的数据帧太大
  DEMUXER_AUDIO_FAIL = 3,     ///< 音频解码失败
} DemuxerResult;

//*****************************************************************************
/*
 *@brief 解码器初始化，打开视频文件，并设置解码器参数
 *@details 支持视频和音频解码，其中视频支持 MP4的
 *H264解码，音频支持PCM_ALAW(G711A)解码
 *@param[in/out] mp4DemuxerInfo* demuxer 解码器
 *@param[in] char* fileName, 需要解码的视频文件名称
 */
int MP4_DEMUXER_Init(mp4DemuxerInfo* demuxer, char* fileName);

/*
 *@brief 解码器反初始化，关闭打开的解码器和视频文件
 */
int MP4_DEMUXER_UnInit(mp4DemuxerInfo* demuxer);

/*
 *@brief 获取视频帧率
 *@details 根据视频帧数，计算视频帧率，如果视频不足10 fps，则返回15 fps
 *@param[in] mp4DemuxerInfo* demuxer 解码器
 *@return Framerate 视频帧率，单位 fps
 */
int MP4_DEMUXER_GetFrameRate(mp4DemuxerInfo* demuxer);

/*
 *@brief 获取视频帧个数
 *@details 无
 *@param[in] mp4DemuxerInfo* demuxer 解码器
 *@return 返回视频帧个数，单位帧
 */
int MP4_DEMUXER_GetFrameNums(mp4DemuxerInfo* demuxer);

/*
 *@brief 查找视频某个时刻点位置
 *@details 无
 *@param[in] mp4DemuxerInfo* demuxer 解码器
 *@param[in] int second, 查找视频的第几秒位置，单位秒
 *@return 成功返回0，失败返回其他
 */
int MP4_DEMUXER_Seek(mp4DemuxerInfo* demuxer, int second);

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
DemuxerResult MP4_DEMUXER_GetFrame(mp4DemuxerInfo* demuxer, int* frameType, uint8_t** frameData, int* frameMaxLen,
                                   int* frameSize, int64_t* duration, int64_t* pts, int* IFlag);

//*****************************************************************************
#ifdef __cplusplus
}
#endif

//*****************************************************************************
#endif /* MP4_DEMUXER_H_ */

/** @} */
