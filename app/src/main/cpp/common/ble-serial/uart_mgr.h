#ifndef __UART_MGR_H__
#define __UART_MGR_H__

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "common/ble-serial/uart/uart.h"
#include "common/cbuf/char_buf.h"
#include "msg_common.h"
#include "netconnect_errcode.h"

#if 0
https://ihqz5dyhwa.feishu.cn/docx/GaJ6dMq32oQybFxPTd7ckZELnSh?renamingWikiNode=true
https://ihqz5dyhwa.feishu.cn/docx/KCqGddtyiowGGZxFtyKclENfnOb

aa 55 01 05 08 00 00 13 d4 35 34 35 57 35 00 01 00 ef 94
                        D4 35 34 35 57 35
aa 55 01 05 08 00 0c 3d d4 35 34 35 57 35 00 01 00 1c a4
+----------------+----------------+----------------+----------------+---------------------------+
| Field          | Offset (Bytes) | Length (Bytes) | Symbol         | Description               |
+----------------+----------------+----------------+----------------+---------------------------+
| Packet Header  | 0              | 2              | Head           | 0xAA55, little-endian     |
+----------------+----------------+----------------+----------------+---------------------------+
| Protocol Ver.  | 2              | 1              | ver            | Protocol version          |
+----------------+----------------+----------------+----------------+----------------------------+
| Command Code   | 3              | 1              | cmd            | 0x01 to 0x06, described above |
+----------------+----------------+----------------+----------------+------------------------------+
| Sub-command    | 4              | 1              | sub_cmd        | Valid if cmd is 0x03         |
+----------------+----------------+----------------+----------------+------------------------------+
| Acknowledge    | 5              | 1              | ack            | Valid if cmd is 0x03 or 0x04 |
+----------------+----------------+----------------+----------------+------------------------------+
| Reserved       | 6              | 2              | reserve        | 0x0000, sequence number      |
+----------------+----------------+----------------+----------------+------------------------------+
| Device MAC     | 8              | 6              | mac            | Big-endian                   |
+----------------+----------------+----------------+----------------+------------------------------+
| Payload Length | 14             | 2              | payload len    | Payload length, big-endian    |
+----------------+----------------+----------------+----------------+-------------------------------+
| Payload        | 16             | N              |                | Generally within 20 bytes     |
+----------------+----------------+----------------+----------------+-------------------------------+
| CRC-16         | 16 + N         | 2              |                | CRC-16 of data from header to payload |
+----------------+----------------+----------------+----------------+-------------------------------+
#endif

#define MAX_BLE_HTTPS_SIZE (256)
#define MAX_BLE_TOKEN_SIZE (128)
#define MAX_BLE_SSID_SIZE (256)
#define MAX_BLE_PASSWD_SIZE (256)
#define MAX_BLE_UTC_SIZE (128)
#define MAX_BLE_MAC_SIZE (32)
#define MAX_SSID_COUNT (20)
#define MAX_SCAN_RESULTS (50)  // 最多扫描32个SSID
#define MAX_SECRET_KEY_SIZE (16)

// 硬件版本查询相关定义
#define HW_VERSION_AD100 0x00   // 硬件版本AD100
#define HW_VERSION_AD102P 0x01  // 硬件版本AD102P
#define HW_VERSION_AD102N 0x02  // 硬件版本AD102N

#define DEFAULT_UART_DEVICE_NAME (128)  // 默认UART设备名称大小
#define DEFAULT_MAX_UNPACK_SIZE (512)
#define UART_PACKET_SIZE (18)
#define UART_PAYLOAD_LEN_INDEX (14)
#define UART_MAC_SIZE (6)

#define UART_HEAD1 (0xAA)
#define UART_HEAD2 (0x55)
#define UART_PROTO_VERSION (0x01)
#define UART_CMD_UART2SOC (0x05)
#define UART_CMD_SOC2UART (0x06)

#define UART_SET_NET_START (0x1)
#define UART_SET_NET_STOP (0x2)
#define UART_SET_NET_START_RESP_FAILED (0x0)
#define UART_SET_NET_START_RESP_SUCCESS (0x1)

// Command Code
typedef enum UART_MSG_TYPE {

  UART_MSG_TYPE_SEND_HTTPS = 3,               // 发送https网址
  UART_MSG_TYPE_SEND_TOKEN = 4,               // 发送token
  UART_MSG_TYPE_SEND_SSID = 5,                // 发送SSID
  UART_MSG_TYPE_SEND_PASSWD = 6,              // 发送Passwd
  UART_MSG_TYPE_GET_NETWORK_STATUS = 7,       // APP查询设备配网状态
  UART_MSG_TYPE_SET_NETWORK = 8,              // 设置配网
  UART_MSG_TYPE_QUERY_MAC = 9,                // 查询蓝牙MAC
  UART_MSG_TYPE_KEY_MESSAGE = 0xA,            // 按键消息
  UART_MSG_TYPE_RESET_FACTORY = 0xB,          // 恢复出厂设置
  UART_MSG_TYPE_WAKEUP_CAMERA = 0xC,          // 唤醒外机
  UART_MSG_TYPE_GET_WIFIMAC = 0xD,            // 获取WIFI MAC
  UART_MSG_TYPE_CONTROL_HUBLOCK = 0xE,        // 控内机锁
  UART_MSG_TYPE_REPORT_HUBLOCK_STATUS = 0xF,  // 内机蓝牙上报锁状态
  UART_MSG_TYPE_CONTROL_LIGHT = 0x10,         // 定义灯光控制的消息类型

  UART_MSG_TYPE_GET_HUB_BLE_VERSION = 0x14,     // 获取蓝牙版本信息
  UART_MSG_TYPE_REPORT_HUB_BLE_VERSION = 0x15,  // 蓝牙上报版本信息
  UART_MSG_TYPE_SCAN_WIFI_LIST = 0x16,          // 扫描WIFI列表
  UART_MSG_TYPE_REPORT_WIFI_LIST = 0x17,        // 上报WIFI列表

  UART_MSG_TYPE_UTC_TIME = 0x1C,  // 请求UTC时间

  UART_MSG_TYPE_MAINTENANCE_LIST = 0x18,    // 下发设备的维护列表
  UART_MSG_TYPE_REPORT_DEVICE_INFO = 0x19,  // 上报维护列表中Seq变化的广播内容
  UART_MSG_TYPE_REMOTE_CONTROL = 0x1A,      // 下发控制命令
  UART_MSG_TYPE_REMOTE_CONTROL_ACK = 0x1B,  // 下发控制命令
  UART_MSG_TYPE_REPORT_BIND_INFO = 0x21,    // 内机蓝牙上报绑定的设备信息内容
  UART_MSG_TYPE_REQUEST_LIST = 0x22,        // 请求轻网关或Matter维护列表

  UART_MSG_TYPE_QUERY_HUB_WIFI_INFO = 0x23,   // 查询内机WIFI的SSID、Passwd、IP、通信密钥
  UART_MSG_TYPE_RECV_HUB_WIFI_CONFIG = 0x24,  // 下发给内机WIFI的SSID、Passwd、IP、通信密钥、原内机MAC
  UART_MSG_TYPE_QUERY_HW_VERSION = 0x25,      // 查询内机硬件版本号

  UART_MSG_TYPE_QUERY_SECRET_KEY = 0x41,      // 查询秘钥
  UART_MSG_TYPE_GET_BIND_DEVICE_INFO = 0x42,  // 查询绑定的设备信息
  UART_MSG_TYPE_GET_BROADCAST_INFO = 0x43,    // 查询广播内容
  UART_MSG_TYPE_ENTER_SLEEP = 0x44,           // 进入休眠
  UART_MSG_YPE_DATA_THROUGH = 0x45,           // 数据透传到外机

  UART_MSG_YPE_SYNC_MATTER_DEV_LIST = 0x46,            // 下发Matter设备维护列表
  UART_MSG_YPE_UPDATE_MATTER_BROADCAST_PACKET = 0x47,  // 更新Matter设备维护列表设备的广播包内容

  UART_MSG_TYPE_SIMULATE_DOORBELL_WAKEUP = 0xD1,  // 模拟外机唤醒

} UART_MSG_TYPE;

typedef enum WIFI_CONNECT_STATUS {
  WIFI_STA_IDLE = 0x30,            // 默认，未发起连接
  WIFI_STA_CONNECTING = 0x31,      // 连接中
  WIFI_STA_WRONG_PASSWORD = 0x32,  // 密码错误
  WIFI_STA_NO_AP_FOUND = 0x33,     // 没找到AP
  WIFI_STA_CONNECT_FAIL = 0x34,    // 连接故障
  WIFI_STA_GOT_IP = 0x35,          // 获取IP成功
  WIFI_STA_CANNOT_GOT_IP = 0x41,   // 不能获取到IP
} WIFI_CONNECT_STATUS;

typedef enum IOT_CONNECT_STATUS {
  IOT_CONNECT_FAILED = 0x30,
  IOT_CONNECT_SUCCESS = 0x31,
} IOT_CONNECT_STATUS;

typedef enum UART_MSG_STATE {
  UART_MSG_STATE_HEAD1,
  UART_MSG_STATE_HEAD2,
  UART_MSG_STATE_VERSION,
  UART_MSG_STATE_CMD,
  UART_MSG_STATE_SUB_CMD,
  UART_MSG_STATE_ACK,
  UART_MSG_STATE_RESERVED,
  UART_MSG_STATE_MAC,
  UART_MSG_STATE_PAYLOAD_LENGTH,
  UART_MSG_STATE_PAYLOAD,
  UART_MSG_STATE_CRC16,
  UART_MSG_STATE_COMPLETE,
} UART_MSG_STATE;

// 灯的颜色

// BLE配网的重要信息，包括SSID、PASSWD、TOKEN、HTTPS
typedef struct uart_connect_info {
  uint8_t https[MAX_BLE_HTTPS_SIZE];
  uint8_t token[MAX_BLE_TOKEN_SIZE];
  uint8_t ssid[MAX_BLE_SSID_SIZE];
  uint8_t passwd[MAX_BLE_PASSWD_SIZE];
  uint8_t utc[MAX_BLE_UTC_SIZE];
} UartConnectInfo, *PUartConnectInfo;

// SSID信息结构体
typedef struct {
  uint8_t length;  // SSID长度 (1 ~ 32)
  int8_t rssi;     // RSSI信号强度 (-127 ~ 127)
  char ssid[33];   // SSID名称
} SSIDInfo, *PSSIDInfo;

// SSID扫描结果
typedef struct {
  uint8_t ssidCount;                  // 实际扫描到的SSID数量 (0 ~ 20)
  SSIDInfo ssidList[MAX_SSID_COUNT];  // SSID列表
} WifiScanResult, *PWifiScanResult;

// 启用1字节对齐
#pragma pack(1)
typedef struct uart_msg_t {
  uint16_t head;
  uint8_t protocal;
  uint8_t cmd;
  uint8_t subCmd;
  uint8_t ack;
  uint16_t reserved;
  uint8_t mac[UART_MAC_SIZE];
  uint16_t payloadLen;
  uint8_t payload[DEFAULT_MAX_UNPACK_SIZE];
  uint16_t crc16;
} UartMsg, *PUartMsg;
// 恢复默认对齐
#pragma pack()

typedef struct {
  char ssid[64];            // SSID
  char passwd[256];         // 密码
  char master_mac[32];      // 主机的MAC地址
  char master_mac_hex[16];  // 主机的MAC地址(十六进制)
  char master_ip[32];       // 主机的IP地址
  char master_key[32];      // 主机的密钥
  int get_wifi_config;      // 0:未获取到，1:获取到
} RepeaterWifiConfig, *PRepeaterWifiConfig;

typedef int (*uart_recv_callback)(PUartMsg);

typedef struct uart_mgr_t {
  uint32_t initState;                          // 初始化状态，0：失败，1：成功
  PUartFile uartFile;                          // 串口句柄
  uint32_t baudrate;                           // 串口的波特率
  uint8_t name[DEFAULT_UART_DEVICE_NAME];      // 串口的终端名称
  UART_MSG_STATE uartState;                    // 串口消息的接受状态
  int32_t threadStop;                          // 0:读线程不停止，1：读线程停止
  uart_recv_callback recvCb;                   // 读线程数据接受回调
  pthread_t threadId;                          // 读线程ID
  PCharBuffer recvBuf;                         // 读线程的接受Buffer
  uint8_t unpackBuf[DEFAULT_MAX_UNPACK_SIZE];  // 拆包缓存
  uint32_t unpackSize;                         // 拆包使用大小
  uint32_t payloadLen;                         // 负载大小
  UartMsg msg;                                 // 消息包
  uint16_t sendMsgSeq;                         // uart发送序列号
  UartConnectInfo connectInfo;                 // 配网信息
  uint8_t sendBuf[DEFAULT_MAX_UNPACK_SIZE];    // 回复消息
  uint8_t mac[UART_MAC_SIZE];                  // 蓝牙MAC地址
  uint32_t connectInfoComplete;                // 获取到配网信息 , 0:未获取到，1:获取到
  uint8_t bind_info_received;                  // 是否收到绑定信息响应, 0:未收到，1:收到
  uint8_t secret_key_received;                 // 是否收到秘钥响应, 0:未收到，1:收到
  RepeaterWifiConfig repeater_wifi_config;     // 中继器WIFI配置
} UartMgr, *PUartMgr;

typedef struct HUB_DEV_LIST_CONF {
  char devMac[64];
  char devType[4];
} HUB_DEV_LIST_CONF_S, *PHUB_DEV_LIST_CONF_S;

typedef enum HUB_CONTROL_LOCK_STATE {
  HUB_CONTROL_LOCK_STATE_IDLE = 0,
  HUB_CONTROL_LOCK_STATE_RUNING = 1,
} HUB_CONTROL_LOCK_STATE;

typedef struct {
  HUB_CONTROL_LOCK_STATE state;
  long long control_timestamp;
  pthread_mutex_t mutex;
} HubControlManager, *PHubControlManager;

int uart_mgr_init(char *name, int baudrate, uart_recv_callback recvCb);
PUartConnectInfo uart_mgr_netinfo_get();
int uart_mgr_send(uint8_t cmd, uint8_t sub_cmd, const uint8_t *payload, uint32_t payloadSize);
int uart_mgr_response(uint8_t subCmd);
int uart_mgr_set_network(int start);
int uart_mgr_wakeup_camera(unsigned char *subMac);
int uart_mgr_control_lock(HUB_LOCK_EVENT event);
int uart_mgr_parse_control_ack(char *devMac, uint8_t *payload, int payloadLen);
int uart_mgr_control_lock_by_secret_key(const char *devMac, const char *secretKey, uint8_t keyId, HUB_LOCK_EVENT event);
bool uart_mgr_is_matter_ctrl(void);
int uart_mgr_deinit();

// app
int uart_mgr_wakeup_default_camera();
int uart_mgr_control_light(LightColor color, LightState state, uint16_t duration);

int uart_mgr_send_maintenance_list(HUB_DEV_LIST_CONF_S *m_devlist, int listNum);
int uart_mgr_send_remote_control(uint8_t *payload, int payloadLen);

int uart_mgr_send_ssid_list(WifiScanResult *scanResult);
int uart_mgr_get_ble_version();
int uart_mgr_get_broadcast_info(int isLock);
int uart_mgr_get_bind_info(int isLock);
int uart_mgr_send_sleep_cmd();
int uart_mgr_data_transmission_through(const char *data, int dataLen);
int uart_mgr_clean_connect_into();
int uart_mgr_send_query_secret_key();
int uart_mgr_simulate_doorbell_wakeup();
int uart_mgr_send_utc_time();

// 回复外机硬件版本查询相关函数
int uart_mgr_parse_hw_version_response();

int uart_mgr_can_wakeup();
int uart_mgr_set_control_lock_state(HUB_CONTROL_LOCK_STATE state);
int uart_mgr_control_camera_reboot(unsigned char *devMac);

// 响应蓝牙的repeater配网请求，回给APP
int uart_mgr_response_hub_wifi_info();

// 处理蓝牙的repeater配网请求，设置给新内机
int uart_mgr_recv_hub_wifi_config(const uint8_t *payload, uint32_t payloadSize);

// 获取中继器WIFI配网的配置信息
int uart_mgr_repeater_netinfo_get();

// 清理中继器WIFI配网的配置信息
void uart_mgr_repeater_wifi_config_clear();
#endif
