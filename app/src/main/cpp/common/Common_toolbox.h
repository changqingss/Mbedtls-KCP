/***********************************************************************************
 * 文 件 名   : Common_toolbox.h
 * 负 责 人   : xwq
 * 创建日期   : 2024年5月11日
 * 文件描述   : Common_toolbox.c 的头文件
 * 版权说明   : Copyright (c) 2015-2029  SWITCHBOT SHENZHEN IOT
 * 字符编码   : UTF-8
 * 修改日志   :
 ***********************************************************************************/

#ifndef __COMMON_TOOLBOX_H__
#define __COMMON_TOOLBOX_H__
/*----------------------------------------------*
 * 包含头文件                                           *
 *----------------------------------------------*/
#include <stdint.h>

/*----------------------------------------------*
 * 宏定义                              *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 外部变量说明                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 全局变量                        *
 *----------------------------------------------*/

typedef struct repeater_header_data {
  char m_module_name[32];
  char m_msgType[32];
  char m_mac[32];
  uint64_t m_seq;
} repeater_header_data_s, *Prepeater_header_data_s;

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

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

extern uint64_t CommToolbox_get_utcmsec(void);
extern int CommToolbox_get_utcsec(void);
extern void CommToolbox_set_system_time(uint32_t utc_time, int32_t timezone_offset);
extern int32_t CommToolbox_get_timezone_offset(void);
extern void CommToolbox_SetTimeZonePosix(char *pTimezone);

extern int CommToolbox_get_ip(const char *interface_name, char *rvip, int ipSize);
extern int CommToolbox_get_mac(const char *iface, char *rvmac, int macSize);

extern long CommToolbox_getFileSize(const char *filename);

extern int CommToolbox_touch_newFile(const char *filePath, const char *text);

extern int CommToolbox_writeFile(const char *path, const char *content);

extern int CommToolbox_creat_newDir(const char *dirPath);

/*
 * CJSON START
 **/
#include <cJSON.h>

extern cJSON *CommToolbox_parse_cjsonObject_from_file(const char *pathstr);

extern char *CommToolbox_cjson_GetStringValue(const cJSON *root, const char *m_key);

int CommToolbox_repeater_msg_header_parse(cJSON *header, Prepeater_header_data_s pHeaderData);
int CommToolbox_repeater_msg_header_build(cJSON *header, Prepeater_header_data_s pHeaderData);
/*
 * CJSON END
 **/

void CommToolbox_replaceLineWithString(const char *filename, const char *search_string, const char *new_content,
                                       const char *ignor_content, const char *ck_cont);
char *CommToolbox_extract_after_last(const char *s, char delimiter);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __COMMON_TOOLBOX_H__ */
