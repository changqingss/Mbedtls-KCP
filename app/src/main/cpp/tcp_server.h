#ifndef __MAIN_APP_TCP_SERVER_H__
#define __MAIN_APP_TCP_SERVER_H__

#include <stdint.h>

#define TCP_SERVER_PORT 1600
#define TCP_SERVER_MAX_CLIENTS 32

typedef enum TCP_FRAME_TYPE {
  TCP_FRAME_TYPE_CLIENT_HELLO = 0,
  TCP_FRAME_TYPE_SERVER_HELLO = 1,
  TCP_FRAME_TYPE_SERVER_HELLO_ACK = 2,
} TCP_FRAME_TYPE;

typedef struct tcp_frame_t {
  uint16_t head;           // 魔术头
  uint16_t frameType;      // 帧类型
  uint32_t frameSize;      // 帧大小
  uint64_t timestamp;      // 时间戳
  uint8_t randomData[16];  // 随机数据 client hello 时使用
  uint8_t mac[6];          // 源BLE MAC地址
  uint8_t reserved[10];    // 保留字段
  uint8_t frameData[0];    // 帧数据 加密后是16的倍数
} TcpFrame, *PTcpFrame;    // 64字节

int MainApp_TcpServer_Init();
int MainApp_TcpServer_Deinit();

#endif