/***********************************************************************************
 * 文 件 名   : CommonCode.h
 * 负 责 人   : xwq
 * 创建日期   : 2024年5月4日
 * 文件描述   : 公共头文件
 * 版权说明   : Copyright (c) 2015-2024  SWITCHBOT SHENZHEN IOT
 * 字符编码   : UTF-8
 * 修改日志   : 
***********************************************************************************/

#ifndef __COMMONCODE_H__
#define __COMMONCODE_H__

typedef enum {
  /* Doorbell内、外机通讯错误码 */
  CAM_SUIT_BASE=0x1000,
  CAM_SUIT_MAX_BIND,
  CAM_SUIT_ALREADY_BIND,

  /* Doorbell内机进程间通讯错误码 */
  HUB_UNIT_BASE=0x2000,

  /* Doorbell外机错误码 */
  BAT_UNIT_BASE=0x3000,
  
}COMMON_CODE_E;


#endif /* __COMMONCODE_H__ */
