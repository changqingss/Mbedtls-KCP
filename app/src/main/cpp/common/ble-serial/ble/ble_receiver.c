
#include "ble_receiver.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common/api/comm_api.h"
#include "common/log/log_conf.h"
#include "common/wifi/wifi.h"

static BleMgr s_ble_mgr;

#define BLE_DATA_CLEAR(m)                        \
  do {                                           \
    m->dataBufLen = 0;                           \
    m->nextPacketIndex = 0;                      \
    m->dataBufPacketNum = 0;                     \
    m->curPacketType = BLE_MSG_TYPE_UNKNOWN;     \
    m->curBundlingState = BLE_PACKET_STATE_IDLE; \
    memset(m->dataBuf, 0, sizeof(m->dataBuf));   \
  } while (0);

#define BLE_UPDATE_CONNECT_INFO(var, buf, size) \
  do {                                          \
    int csize = MIN(size, sizeof(var) - 1);     \
    strncpy(var, buf, csize);                   \
    var[csize] = '\0';                          \
  } while (0);

const char *ble_msg_type_string(BLE_MSG_TYPE type) {
  switch (type) {
    case BLE_MSG_TYPE_UNKNOWN:
      return "BLE_MSG_TYPE_UNKNOWN";
    case BLE_MSG_TYPE_GET_UUID:
      return "BLE_MSG_TYPE_GET_UUID";
    case BLE_MSG_TYPE_SEND_UUID:
      return "BLE_MSG_TYPE_SEND_UUID";
    case BLE_MSG_TYPE_SEND_HTTPS:
      return "BLE_MSG_TYPE_SEND_HTTPS";
    case BLE_MSG_TYPE_SEND_TOKEN:
      return "BLE_MSG_TYPE_SEND_TOKEN";
    case BLE_MSG_TYPE_SEND_SSID:
      return "BLE_MSG_TYPE_SEND_SSID";
    case BLE_MSG_TYPE_SEND_PASSWD:
      return "BLE_MSG_TYPE_SEND_PASSWD";
    case BLE_MSG_TYPE_GET_NETWORK_STATUS:
      return "BLE_MSG_TYPE_GET_NETWORK_STATUS";
    case BLE_MSG_TYPE_SCAN_WIFI:
      return "BLE_MSG_TYPE_SCAN_WIFI";
    case BLE_MSG_TYPE_GET_WIFI_NUM:
      return "BLE_MSG_TYPE_GET_WIFI_NUM";
    case BLE_MSG_TYPE_GET_WIFI_SSID:
      return "BLE_MSG_TYPE_GET_WIFI_SSID";
    case BLE_MSG_TYPE_SEND_UTC:
      return "BLE_MSG_TYPE_SEND_UTC";
    case BLE_MSG_TYPE_NOT_NEED_BIND:
      return "BLE_MSG_TYPE_NOT_NEED_BIND";
    default:
      return "Unknown BLE_MSG_TYPE";
  }
}

int ble_mgr_init(ble_recv_callback recvCb) {
  PBleMgr m = &s_ble_mgr;
  memset(m, 0, sizeof(BleMgr));
  BLE_DATA_CLEAR(m);
  m->recvCb = recvCb;
  return 0;
}

static int ble_mgr_send_response(PBleMgr m, BLE_MSG_TYPE type, char *buf, int size) {
  int32_t index = 0;
  uint8_t *sbuf = m->sendBuf;
  char macHex[32] = {0};
  WIFI_GetBleMac(macHex);

  switch (type) {
    case BLE_MSG_TYPE_GET_UUID:
      sbuf[index++] = 0x01;      // uuid ack码
      sbuf[index++] = BLE_HEAD;  // Header

      // macHex的包
      for (int i = 0; i < 6; ++i) {
        sscanf(macHex + 2 * i, "%2hhx", &sbuf[index++]);
      }

      break;

    case BLE_MSG_TYPE_SEND_HTTPS:
      break;

    case BLE_MSG_TYPE_SEND_TOKEN:
      break;

    case BLE_MSG_TYPE_SEND_SSID:
      break;

    case BLE_MSG_TYPE_SEND_PASSWD:
      // 收到PASSWD开始配网
      break;

    case BLE_MSG_TYPE_SEND_UTC:
      break;

    default:
      break;
  }

  return 0;
}

static int ble_mgr_parse(PBleMgr m, BLE_MSG_TYPE type, char *buf, int size) {
  PARAM_CHECK(m && buf && size, -1);

  PBleConnectInfo conn = &m->connectInfo;

  COMMONLOG_I("[BLE] get ble msg type:%s", ble_msg_type_string(type));

  switch (type) {
    case BLE_MSG_TYPE_GET_UUID:
      COMMONLOG_I("[BLE] recv Get UUID");
      break;

    case BLE_MSG_TYPE_SEND_HTTPS:
      COMMONLOG_I("[BLE] recv Get HTTPS:%s", buf);
      BLE_UPDATE_CONNECT_INFO(conn->https, buf, size);
      break;

    case BLE_MSG_TYPE_SEND_TOKEN:
      COMMONLOG_I("[BLE] recv Get Token:%s", buf);
      BLE_UPDATE_CONNECT_INFO(conn->token, buf, size);
      break;

    case BLE_MSG_TYPE_SEND_SSID:
      COMMONLOG_I("[BLE] recv Get SSID:%s", buf);
      BLE_UPDATE_CONNECT_INFO(conn->ssid, buf, size);
      break;

    case BLE_MSG_TYPE_SEND_PASSWD:
      COMMONLOG_I("[BLE] recv Get Passwd:%s", buf);
      BLE_UPDATE_CONNECT_INFO(conn->passwd, buf, size);
      // 收到PASSWD开始配网
      break;

    case BLE_MSG_TYPE_SEND_UTC:
      COMMONLOG_I("[BLE] recv Get UTC:%s", buf);
      BLE_UPDATE_CONNECT_INFO(conn->utc, buf, size);
      break;

    default:
      break;
  }

  ble_mgr_send_response(m, type, buf, size);

  return 0;
}

// 组包处理
static int ble_mgr_data_aggregation(PBleMgr m, PBlePacket packet) {
  PARAM_CHECK(m && packet, -1);
  int getPacket = 0;

  // 1、检查组包数据是否过载
  PARAM_EXIT2(m->dataBufLen + packet->payload_len + BLE_MIN_PACKET_SIZE < MAX_BLE_DATA_SIZE);

  // 2、检查状态是否为组包状态
  if (m->curBundlingState == BLE_PACKET_STATE_BUNDLING) {
    // 2.1、检查序列号是否符合递增要求
    PARAM_EXIT2(packet->packet_seq == m->nextPacketIndex);
    // 2.2、检查包类型是否一致
    PARAM_EXIT2(packet->type == m->curPacketType);
  }

  // 3、检查接受状态
  switch (m->curBundlingState) {
    case BLE_PACKET_STATE_IDLE:
    case BLE_PACKET_STATE_BUNDLING: {
      // 3.1、提取BLE Payload到组包缓存
      m->curPacketType = packet->type;
      memcpy(m->dataBuf + m->dataBufLen, packet->payload, packet->payload_len);
      m->dataBufLen += packet->payload_len;
      m->dataBufPacketNum++;

      // 3.2、检查是否组包完成，组包完成通知外界
      if (packet->packet_total_num == m->dataBufPacketNum) {
        m->curBundlingState = BLE_PACKET_STATE_IDLE;
        getPacket = 1;
      } else if (m->dataBufPacketNum < packet->packet_total_num) {
        m->curBundlingState = BLE_PACKET_STATE_BUNDLING;
        m->nextPacketIndex = packet->packet_seq + 1;
      }
    } break;

    default:
      break;
  }

  // 4、解析组完包的数据
  if (getPacket && m->recvCb) {
    ble_mgr_parse(m, m->curPacketType, m->dataBuf, m->dataBufLen);
    m->recvCb(m->curPacketType, m->dataBuf, m->dataBufLen);
    BLE_DATA_CLEAR(m);
  }

  return 0;

exit:
  BLE_DATA_CLEAR(m);
  return 0;
}

// 接受串口报文
int ble_mgr_data_recv_uart(PUartMsg umsg) {
  PARAM_CHECK(umsg, -1);

  PBleMgr m = &s_ble_mgr;
  BlePacket blePacket = {0};
  int index = 0;
  uint8_t checkSum = 0;
  uint8_t *payload = umsg->payload;
  int payloadLen = umsg->payloadLen;

  // 1、检查BLE Header
  PARAM_CHECK_STRING(payload[BLE_HEAD_INDEX] == BLE_HEAD, BLE_ERROR_CODE_HEAD_ERROR, "ble head is error");

  // 2、检查UART包的Payload长度字段，需要大于至少6个字节（5字节BLE头+1字节校验和）
  PARAM_CHECK_STRING(payloadLen >= BLE_MIN_PACKET_SIZE, BLE_ERROR_CODE_PAYLOAD_LEN_ERROR,
                     "ble payload len:%hhu is error", payloadLen);

  blePacket.head = payload[index++];
  blePacket.type = payload[index++];
  blePacket.packet_total_num = payload[index++];
  blePacket.packet_seq = payload[index++];
  blePacket.payload_len = payload[index++];

  // 3、检查UART Payload的长度字段，需要跟BLE包的PayloadLen匹配
  PARAM_CHECK_STRING(payloadLen == blePacket.payload_len + BLE_MIN_PACKET_SIZE, BLE_ERROR_CODE_PAYLOAD_LEN_ERROR,
                     "uart payload len:%d is error", payloadLen);

  PARAM_CHECK_STRING(blePacket.payload_len < MAX_BLE_PACKET_SIZE, BLE_ERROR_CODE_PAYLOAD_LEN_ERROR,
                     "payload len:%hhu is big than %d", blePacket.payload_len, MAX_BLE_PACKET_SIZE);

  memcpy(blePacket.payload, payload + index, blePacket.payload_len);
  index += blePacket.payload_len;

  checkSum = COMM_API_Calculate_xor_checksum(payload, index);

  // 4、校验CheckSum
  blePacket.checksum = payload[index++];

  // 5、APP没做校验和，先屏蔽
  // PARAM_CHECK_STRING(checkSum == blePacket.checksum,BLE_ERROR_CODE_CHECKCODE_ERROR,
  // "calcCheckSum:%hhu is not match blePacket.checksum:%hhu",checkSum,blePacket.checksum);

  ble_mgr_data_aggregation(m, &blePacket);

  return 0;
}

int ble_mgr_deinit() { return 0; }
