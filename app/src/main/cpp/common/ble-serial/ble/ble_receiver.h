#ifndef __BLE_RECRIVER_H__
#define __BLE_RECRIVER_H__

#include <pthread.h>
#include <stdint.h>

#include "common/ble-serial/uart_mgr.h"
#include "common/cbuf/char_buf.h"

/*
 *
+----------------+----------------+----------------+----------------+---------------------------------------+
| Field          | Offset (Bytes) | Length (Bytes) | Symbol         | Description                           |
+----------------+----------------+----------------+----------------+---------------------------------------+
| 数据头          | 0              | 1              | Head           | 0x5A, 标记数据包开头                 |
+----------------+----------------+----------------+----------------+---------------------------------------+
| 类型            | 1              | 1              | Type           | 标记数据通信类型                     |
+----------------+----------------+----------------+----------------+---------------------------------------+
| 总包数          | 2              | 1              | Total Packets  | 数据一共多少包，用于分包时使用       |
+----------------+----------------+----------------+----------------+---------------------------------------+
| 序号            | 3              | 1              | Packet Seq     | 分包数据序号                         |
+----------------+----------------+----------------+----------------+---------------------------------------+
| 长度            | 4              | 1              | Data Length    | 数据内容长度                         |
+----------------+----------------+----------------+----------------+---------------------------------------+
| 数据内容        | 5              | N (1~255)      | Data Content   | 数据内容，LSB格式要求                |
+----------------+----------------+----------------+----------------+---------------------------------------+
| 校验和          | 5 + N          | 1              | Checksum       | 异或校验值                           |
+----------------+----------------+----------------+----------------+---------------------------------------+

*/

#define BLE_HEAD (0x5a)
#define BLE_HEAD_INDEX (0)
#define BLE_TYPE_INDEX (1)
#define BLE_TOTAL_INDEX (2)
#define BLE_SEQ_INDEX (3)
#define BLE_PAYLOAD_LEN_INDEX (4)
#define BLE_PAYLOAD_DATA_INDEX (5)
#define BLE_MIN_PACKET_SIZE (6)  // 5个字节的头 + 1个字节的校验和

#define MAX_BLE_PACKET_NUM (50)   // 组包的最大包数
#define MAX_BLE_PACKET_SIZE (30)  // 单个BLE包的Payload最大长度
#define MAX_BLE_DATA_SIZE (2048)  // BLE组包的Payload最大长度

#define MAX_BLE_HTTPS_SIZE (256)
#define MAX_BLE_TOKEN_SIZE (128)
#define MAX_BLE_SSID_SIZE (256)
#define MAX_BLE_PASSWD_SIZE (256)
#define MAX_BLE_UTC_SIZE (128)

typedef enum BLE_MSG_TYPE {

  BLE_MSG_TYPE_UNKNOWN = 0,
  BLE_MSG_TYPE_GET_UUID = 1,     // 获取设备的UUID
  BLE_MSG_TYPE_SEND_UUID = 2,    // 发送设备的UUID
  BLE_MSG_TYPE_SEND_HTTPS = 3,   // 发送https网址
  BLE_MSG_TYPE_SEND_TOKEN = 4,   // 发送token
  BLE_MSG_TYPE_SEND_SSID = 5,    // 发送SSID
  BLE_MSG_TYPE_SEND_PASSWD = 6,  // 发送Passwd

  BLE_MSG_TYPE_GET_NETWORK_STATUS = 7,  // APP查询设备配网状态
  BLE_MSG_TYPE_SCAN_WIFI = 8,           // 开始扫描wifi列表
  BLE_MSG_TYPE_GET_WIFI_NUM = 9,        // 获取设备扫描WIFI信息(扫描到的wifi个数)
  BLE_MSG_TYPE_GET_WIFI_SSID = 0x0A,    // 获取WIFI的SSID信息

  BLE_MSG_TYPE_SEND_UTC = 0x0B,       // 配网数据包中的设置UTC系统时间
  BLE_MSG_TYPE_NOT_NEED_BIND = 0x0C,  // 不需要绑定，只修改wifi

} BLE_MSG_TYPE;

typedef int (*ble_recv_callback)(BLE_MSG_TYPE, char *, int);

// BLE的数据报文格式
#pragma pack(1)
typedef struct ble_packet_t {
  uint8_t head;
  uint8_t type;
  uint8_t packet_total_num;
  uint8_t packet_seq;
  uint8_t payload_len;
  uint8_t payload[MAX_BLE_PACKET_SIZE];
  uint8_t checksum;
} BlePacket, *PBlePacket;
#pragma pack()

// BLE配网的重要信息，包括SSID、PASSWD、TOKEN、HTTPS
typedef struct ble_connect_info {
  uint8_t https[MAX_BLE_HTTPS_SIZE];
  uint8_t token[MAX_BLE_TOKEN_SIZE];
  uint8_t ssid[MAX_BLE_SSID_SIZE];
  uint8_t passwd[MAX_BLE_PASSWD_SIZE];
  uint8_t utc[MAX_BLE_UTC_SIZE];

} BleConnetInfo, *PBleConnectInfo;

// BLE的数据报文接受状态
typedef enum BLE_PACKET_STATE {
  BLE_PACKET_STATE_IDLE,      // 空闲
  BLE_PACKET_STATE_BUNDLING,  // 组包中
} BLE_PACKET_STATE;

// BLE的数据包控制器
typedef struct ble_mgr_t {
  BLE_PACKET_STATE curBundlingState;  // BLE接受包的状态
  BLE_MSG_TYPE curPacketType;         // 当前BLE包的类型
  int32_t nextPacketIndex;            // 当前index

  ble_recv_callback recvCb;         // BLE接受回调
  int32_t dataBufLen;               // 组包Buffer数据长度
  int32_t dataBufPacketNum;         // 组包的个数
  char dataBuf[MAX_BLE_DATA_SIZE];  // 组包Buffer

  uint8_t sendBuf[MAX_BLE_DATA_SIZE];  // 发送Buffer
  BleConnetInfo connectInfo;           // 配网信息

} BleMgr, *PBleMgr;

int ble_mgr_init(ble_recv_callback recvCb);
int ble_mgr_data_recv(PUartMsg);
int ble_mgr_deinit();

#endif