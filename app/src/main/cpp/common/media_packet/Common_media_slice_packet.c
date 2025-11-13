/***********************************************************************************
 * 文 件 名   : media_slice_packet.c
 * 负 责 人   : xwq
 * 创建日期   : 2024年1月15日
 * 文件描述   : 音视频媒体数据切片、合并处理
 * 版权说明   : Copyright (c) 2015-2024  SWITCHBOT SHENZHEN IOT
 * 字符编码   : UTF-8
 * 修改日志   :
 ***********************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                           *
 *----------------------------------------------*/
#include "Common_media_slice_packet.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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

int media_data_slice_unpack(frameInfo_s *frameInfo, char *srcdata, int srcLen, char **destData, int *destLen) {
  int i = 0;
  int totalpkNum = 0;
  int destIndex = 0;
  int srcIndex = 0;
  int sliceHeadLen = sizeof(sliceDataHeader_s);
  sliceDataHeader_s headerInfo;
  char *tempDate = NULL;

  if (srcdata == NULL) {
    return -1;
  }

  *destData = (char *)calloc(srcLen, 1);
  if (*destData == NULL) {
    return -1;
  }
  tempDate = *destData;

  memset(&headerInfo, 0, sliceHeadLen);
  memcpy(&headerInfo, srcdata, sliceHeadLen);

  totalpkNum = headerInfo.m_sliceTotalNum;
  srcIndex = 0;
  destIndex = 0;
  for (i = 0; i < totalpkNum; i++) {
    if (i == 0) {
      frameInfo->m_EncodeType = headerInfo.frameInfo.m_EncodeType;
      frameInfo->m_frameType = headerInfo.frameInfo.m_frameType;
      frameInfo->m_frameRate = headerInfo.frameInfo.m_frameRate;
      frameInfo->m_frameGop = headerInfo.frameInfo.m_frameGop;
      frameInfo->m_frameIndex = headerInfo.frameInfo.m_frameIndex;
      frameInfo->m_frmPts = headerInfo.frameInfo.m_frmPts;
      frameInfo->m_utcPts = headerInfo.frameInfo.m_utcPts;

      srcIndex += sliceHeadLen;
      if (headerInfo.m_packetLen <= DATA_MAX_MTU_SIZE - sliceHeadLen) {
        memcpy(tempDate + destIndex, srcdata + srcIndex, headerInfo.m_packetLen);
        srcIndex += headerInfo.m_packetLen;
        destIndex += headerInfo.m_packetLen;
      } else {
        printf("m_packetLen(%d) ERR", headerInfo.m_packetLen);
        return -1;
      }

    } else {
      memset(&headerInfo, 0, sliceHeadLen);
      memcpy(&headerInfo, srcdata + srcIndex, sliceHeadLen);
      srcIndex += sliceHeadLen;

      if (headerInfo.m_packetLen <= DATA_MAX_MTU_SIZE - sliceHeadLen) {
        memcpy(tempDate + destIndex, srcdata + srcIndex, headerInfo.m_packetLen);
        srcIndex += headerInfo.m_packetLen;
        destIndex += headerInfo.m_packetLen;
      } else {
        printf("m_packetLen(%d) ERR", headerInfo.m_packetLen);
        return -1;
      }
    }
  }

  *destLen = destIndex;

  return 0;
}

int media_data_slice_pack(frameInfo_s *frameInfo, char *srcdata, int srcLen, char **destData, int *destLen,
                          int *packetNum) {
  int i = 0;
  int totalpkNum = 0;
  int nRet = 0;
  int mAllocSize = 0;
  int sliceLen = 0;
  int destIndex = 0;
  int srcIndex = 0;
  int sliceHeadLen = sizeof(sliceDataHeader_s);
  sliceDataHeader_s headerInfo;
  char *tempDate = NULL;

  sliceLen = DATA_MAX_MTU_SIZE - sliceHeadLen;
  totalpkNum = (srcLen / sliceLen) + ((srcLen % (sliceLen) != 0) ? 1 : 0);

  mAllocSize = totalpkNum * DATA_MAX_MTU_SIZE;
  *destData = (char *)calloc(mAllocSize, 1);
  if (*destData == NULL) {
    return -1;
  }

  tempDate = *destData;
  destIndex = 0;
  srcIndex = 0;

  for (i = 0; i < totalpkNum; i++) {
    headerInfo.frameInfo.m_EncodeType = frameInfo->m_EncodeType;
    headerInfo.frameInfo.m_frameType = frameInfo->m_frameType;
    headerInfo.frameInfo.m_frameRate = frameInfo->m_frameRate;
    headerInfo.frameInfo.m_frameGop = frameInfo->m_frameGop;
    headerInfo.frameInfo.m_frameIndex = frameInfo->m_frameIndex;
    headerInfo.frameInfo.m_frmPts = frameInfo->m_frmPts;
    headerInfo.frameInfo.m_utcPts = frameInfo->m_utcPts;

    headerInfo.m_sliceTotalNum = totalpkNum;
    headerInfo.m_sliceIndex = i + 1;

    if (i != totalpkNum - 1) {
      headerInfo.m_packetLen = sliceLen;
    } else {
      headerInfo.m_packetLen = srcLen - srcIndex;
    }
    if (headerInfo.m_packetLen) {
      memcpy(tempDate + destIndex, &headerInfo, sliceHeadLen);
      destIndex += sliceHeadLen;
      memcpy(tempDate + destIndex, srcdata + srcIndex, headerInfo.m_packetLen);
      srcIndex += headerInfo.m_packetLen;
      destIndex += headerInfo.m_packetLen;
    }
  }

  *destLen = destIndex;
  *packetNum = totalpkNum;

  //_LOG("++++ srcLen=%d destLen=%d, packetNum=%d", srcLen, destIndex, totalpkNum);

  return nRet;
}

// FREAM_TYPE_E 枚举转字符串函数
const char *kcp_fream_type_to_string(FREAM_TYPE_E type) {
  switch (type) {
    case FREAM_TYPE_VIDEO_I:
      return "FREAM_TYPE_VIDEO_I";
    case FREAM_TYPE_VIDEO_P:
      return "FREAM_TYPE_VIDEO_P";
    case FREAM_TYPE_AUDIO:
      return "FREAM_TYPE_AUDIO";
    case FREAM_TYPE_HEARTBEAT:
      return "FREAM_TYPE_HEARTBEAT";
    case FREAM_TYPE_PING:
      return "FREAM_TYPE_PING";
    case FREAM_TYPE_PONG:
      return "FREAM_TYPE_PONG";
    case FREAM_TYPE_CTRL:
      return "FREAM_TYPE_CTRL";
    case FREAM_TYPE_FILE:
      return "FREAM_TYPE_FILE";
    case FREAM_TYPE_REPEATER:
      return "FREAM_TYPE_REPEATER";
    default:
      return "UNKNOWN_FREAM_TYPE";
  }
}