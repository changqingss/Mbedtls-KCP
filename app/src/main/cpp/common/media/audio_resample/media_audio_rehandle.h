/***********************************************************************************
 * 文 件 名   : media_audio_rehandle.h
 * 负 责 人   : xwq
 * 创建日期   : 2023年7月12日
 * 文件描述   : media_audio_rehandle.c 的头文件
 * 版权说明   : Copyright (c) 2015-2023  SWITCHBOT SHENZHEN IOT
 * 字符编码   : UTF-8
 * 修改日志   :
 ***********************************************************************************/

#ifndef __MEDIA_AUDIO_REHANDLE_H__
#define __MEDIA_AUDIO_REHANDLE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

/*----------------------------------------------*
 * 包含头文件                                           *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                              *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 外部变量说明                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 全局变量                        *
 *----------------------------------------------*/

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

extern int MEDIAO_AUDIO_resample_init(void);
// extern int MEDIA_AUDIO_resample_handle(SRC_STATE * src_state, const int16_t
// * srcData, int srcLen, int srcRate, int16_t	* destData, int * destLen, int
// destRate);
extern int MEDIA_AUDIO_resample_payload(int workType, const char* srcData, int srcLen, int srcRate, char* destData,
                                        int* destLen, int destRate);
// extern int resample_16k_to_8k(short      output_short[], short input_short[],
// int srcLen ); extern int resample_8k_to_16k(short	    output_short[],
// short input_short[], int srcLen );
int MEDIAO_AUDIO_resample_Deinit(void);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __MEDIA_AUDIO_REHANDLE_H__ */
