/***********************************************************************************
 * 文 件 名   : media_audio_rehandle.c
 * 负 责 人   : xwq
 * 创建日期   : 2023年7月11日
 * 文件描述   : 对音频数据采样之后的优化处理，例如重采样、算法降噪等
 * 版权说明   : Copyright (c) 2015-2023  SWITCHBOT SHENZHEN IOT
 * 字符编码   : UTF-8
 * 修改日志   :
 ***********************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                           *
 *----------------------------------------------*/
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "samplerate.h"
/*----------------------------------------------*
 * 宏定义                              *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 外部变量说明                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 全局变量                        *
 *----------------------------------------------*/

/* 音频重采样 */
static SRC_STATE *live_state = NULL;

static SRC_STATE *talk_state = NULL;

/*----------------------------------------------*
 * 常量定义                                          *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 内部函数原型说明                                *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 外部函数原型说明                             *
 *----------------------------------------------*/

/* 音频重采样相关 */
#define N16K 1280
int resample_16k_to_8k(short output_short[], short input_short[], int srcLen) {
  int nRet = 0;
  SRC_DATA src_data;
  float input[N16K * 2];
  float output[N16K * 2];

  const int input_sample_rate = 16000;
  const int output_sample_rate = 8000;

  src_short_to_float_array(input_short, input, srcLen);

  src_data.data_in = input;
  src_data.data_out = output;
  src_data.input_frames = srcLen;
  src_data.output_frames = lrint((double)srcLen * input_sample_rate);
  src_data.end_of_input = 0;
  src_data.src_ratio = (float)output_sample_rate / input_sample_rate;

  nRet = src_process(live_state, &src_data);
  // LOG_Info("+++++++++ srcLen=%d, resampleLen=%d, nRe=%d", srcLen,
  // src_data.output_frames_gen, nRet);

  // assert(src_data.output_frames_gen <= src_data.output_frames);
  src_float_to_short_array(output, output_short, src_data.output_frames_gen);

  return src_data.output_frames_gen;
}

int resample_8k_to_16k(short output_short[], short input_short[], int srcLen) {
  int nRet = 0;
  SRC_DATA src_data;
  float input[N16K * 2];
  float output[N16K * 2];

  const int input_sample_rate = 8000;
  const int output_sample_rate = 16000;

  src_short_to_float_array(input_short, input, srcLen);

  src_data.data_in = input;
  src_data.data_out = output;
  src_data.input_frames = srcLen;
  src_data.src_ratio = (float)output_sample_rate / input_sample_rate;
  src_data.output_frames = src_data.src_ratio * src_data.input_frames;  // 要求输出的采样点数;
                                                                        ////lrint((double)srcLen*input_sample_rate);
  src_data.end_of_input = 0;

  src_reset(talk_state);
  do {
    int ret = src_process(talk_state, &src_data);
    if (ret != 0) return -1;
  } while (src_data.output_frames_gen < src_data.output_frames);

  src_float_to_short_array(output, output_short, src_data.output_frames_gen);

  return src_data.output_frames_gen;
}

int MEDIA_AUDIO_resample_handle(SRC_STATE *src_state, const char *inputData, int inputLen, int inputRate,
                                char *destData, int *destLen, int destRate) {
  int nRet = 0;
  SRC_DATA src_data;
  float input[N16K * 2];
  float output[N16K * 2];

  src_short_to_float_array((short *)inputData, input, inputLen);

  src_data.data_in = input;
  src_data.data_out = output;
  src_data.input_frames = inputLen;
  src_data.src_ratio = (float)destRate / inputRate;
  src_data.output_frames = src_data.src_ratio * src_data.input_frames;  // 要求输出的采样点数;
                                                                        ////lrint((double)srcLen*input_sample_rate);
  src_data.end_of_input = 0;

  src_reset(src_state);
  do {
    int ret = src_process(src_state, &src_data);
    if (ret != 0) return -1;
  } while (src_data.output_frames_gen < src_data.output_frames);

  src_float_to_short_array(output, (short *)destData, src_data.output_frames_gen);

  *destLen = src_data.output_frames_gen;

  return src_data.output_frames_gen;
}

int MEDIA_AUDIO_resample_payload(int workType, const char *srcData, int srcLen, int srcRate, char *destData,
                                 int *destLen, int destRate) {
  int nRet = -1;
  if (workType == 0) {
    nRet = MEDIA_AUDIO_resample_handle(live_state, srcData, srcLen, srcRate, destData, destLen, destRate);
  } else if (workType == 1) {
    nRet = MEDIA_AUDIO_resample_handle(talk_state, srcData, srcLen, srcRate, destData, destLen, destRate);
  }

  return nRet;
}

int MEDIAO_AUDIO_resample_init(void) {
  int error = -1;

  live_state = src_new(SRC_LINEAR, 1, &error);
  if (live_state == NULL) {
    return -1;
  }

  talk_state = src_new(SRC_LINEAR, 1, &error);
  if (talk_state == NULL) {
    return -1;
  }

  return 0;
}

int MEDIAO_AUDIO_resample_Deinit(void) {
  if (live_state) {
    src_delete(live_state);
  }

  if (live_state) {
    src_delete(talk_state);
  }

  return 0;
}

/* 音频重采样相关 end */
