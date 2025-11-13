#include "tcp_client.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "RepeaterApp_log.h"
//#include "common/api/comm_api.h"
#include "common_sock.h"
//#include "common/crypto/aes_key_gen.h"
//#include "repeater_aes.h"
#include "tcp_server.h"
#include "repeater_aes.h"
#include "wo_aes.h"
#include "aes_key_gen.h"


//#define CLIENT_HELLO_LEN strlen(CLIENT_HELLO)
//#define CLIENT_HELLO "client hello"
//#define CLIENT_HELLO_LEN strlen(CLIENT_HELLO)
//#define SERVER_HELLO "server hello"
//#define SERVER_HELLO_LEN strlen(SERVER_HELLO)
//#define SERVER_HELLO_ACK "server hello ack"
//#define SERVER_HELLO_ACK_LEN strlen(SERVER_HELLO_ACK)
//#define AES_KEY_SIZE (16)
//#ifndef AES_BLOCK_SIZE
//#define AES_BLOCK_SIZE (16)
//#endif

#define MAX_RETRY_COUNT (3)
#define MAGIC_HEAD (0xAF55)

#define CLIENT_HELLO_I(fmt, ...) REPEATERLOG_I("Tcp_Clent.c] [" fmt, ##__VA_ARGS__)
#define CLIENT_HELLO_E(fmt, ...) REPEATERLOG_E("Tcp_Clent.c] [" fmt, ##__VA_ARGS__)
#define CLIENT_HELLO_W(fmt, ...) REPEATERLOG_W("Tcp_Clent.c] [" fmt, ##__VA_ARGS__)
// #define CLIENT_HELLO_D(fmt, ...) REPEATERLOG_D("Client Hello] [" fmt, ##__VA_ARGS__)
#define CLIENT_HELLO_D(fmt, ...)

#include <android/log.h>

#define LOG_TAG "REPEATER"
#define LOGD(fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##__VA_ARGS__)

int tcp_client_connect(PTcpClient client) {
  int ret = 0;
  struct sockaddr_in server_addr;

  PARAM_CHECK_STRING(client, -1, "client is null");
  PARAM_CHECK_STRING(client->init, -1, "client not initialized");
  PARAM_CHECK_STRING(client->sock > 0, -1, "client sock is invalid");

  if (client->connected) {
    CLIENT_HELLO_I("client already connected");
    return 0;
  }

  // 设置服务器地址
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(client->server_port);
  ret = inet_pton(AF_INET, client->server_ip, &server_addr.sin_addr);
  PARAM_CHECK_STRING(ret == 1, -1, "invalid server_ip");

  // 连接服务器
  ret = connect(client->sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
//  if (ret != 0) {
//    CLIENT_HELLO_E("connect to server %s:%d failed, error: %s", client->server_ip, client->server_port,
//                   strerror(errno));
//    return -1;
//  }
    if (ret < 0) {
        if (errno == EINPROGRESS) {
            // 等待连接完成
            struct timeval tv;
            fd_set wfds;
            int so_error = 0;
            socklen_t len = sizeof(so_error);

            FD_ZERO(&wfds);
            FD_SET(client->sock, &wfds);
            tv.tv_sec = TCP_CLIENT_TIMEOUT_SEC;
            tv.tv_usec = 0;

            int sel = select(client->sock + 1, NULL, &wfds, NULL, &tv);
            if (sel > 0) {
                getsockopt(client->sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
                if (so_error == 0) {
                    CLIENT_HELLO_I("Connected to server %s:%d successfully",
                                   client->server_ip, client->server_port);
                    client->connected = 1;
                    return 0;
                } else {
                    CLIENT_HELLO_E("connect to server %s:%d failed, socket error: %s",
                                   client->server_ip, client->server_port, strerror(so_error));
                    return -1;
                }
            } else if (sel == 0) {
                CLIENT_HELLO_E("connect to server %s:%d timeout", client->server_ip, client->server_port);
                return -1;
            } else {
                CLIENT_HELLO_E("select error waiting for connect: %s", strerror(errno));
                return -1;
            }
        } else {
            CLIENT_HELLO_E("connect to server %s:%d failed immediately, error: %s",
                           client->server_ip, client->server_port, strerror(errno));
            return -1;
        }
    }

  client->connected = 1;
  CLIENT_HELLO_I("Connected to server %s:%d successfully", client->server_ip, client->server_port);

  return 0;
}

PTcpClient tcp_client_init(const char *server_ip, int server_port) {

   LOGD("tcp_client_init called, server_ip=%s, port=%d", server_ip, server_port);
  int ret = 0, sock = 0;
  struct timeval timeout;
  PTcpClient client = NULL;
  PARAM_CHECK_STRING(server_ip && server_ip[0], NULL, "server_ip is null or empty");
  PARAM_CHECK_STRING(server_port > 0 && server_port <= 65535, NULL, "invalid server_port");
  client = (PTcpClient)calloc(1, sizeof(TcpClient));
  PARAM_CHECK_STRING(client, NULL, "calloc client failed");

  // 1、创建 socket
  sock = socket(AF_INET, SOCK_STREAM, 0);
  PARAM_CHECK_STRING(sock > 0, NULL, "socket error");
    LOGD("sock created successfully, sock=%d", sock);
    // 设置超时时间为2秒
  timeout.tv_sec = TCP_CLIENT_TIMEOUT_SEC;
  timeout.tv_usec = 0;
  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

  timeout.tv_sec = TCP_CLIENT_TIMEOUT_SEC;
  timeout.tv_usec = 0;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  client->sock = sock;
  client->server_ip = strdup(server_ip);
  client->server_port = server_port;
  client->bufferSize = TCP_CLIENT_BUFFER_SIZE;
  client->buffer = calloc(1, client->bufferSize);
  PARAM_EXIT(client->buffer, "calloc client buffer failed");
    LOGD("client buffer allocated successfully, bufferSize=%d", client->bufferSize);

  client->init = 1;
  client->connected = 0;

  ret = tcp_client_connect(client);
  PARAM_EXIT1(ret == 0, ret, "connect to server:%s port:%d failed", server_ip, server_port);
    LOGD("ret after connect=%d", ret);
  return client;

exit:
  tcp_client_deinit(client);
  return NULL;
}

int tcp_client_send_data(PTcpClient client, const char *data, int data_len) {
  int sendSize = 0;

  PARAM_CHECK_STRING(client, -1, "client is null");
  PARAM_CHECK_STRING(client->init, -1, "client not initialized");
  PARAM_CHECK_STRING(client->connected, -1, "client not connected");
  PARAM_CHECK_STRING(data && data_len > 0, -1, "invalid data or data_len");

  Comm_sock_send_data(client->sock, (unsigned char *)data, data_len, &sendSize);
  if (data_len != sendSize) {
    CLIENT_HELLO_E("send data failed, need_send_size:%d real_send_size:%d, error:%s", data_len, sendSize,
                   strerror(errno));
    client->connected = 0;
    return -1;
  }

  CLIENT_HELLO_D("send data success, size: %d", sendSize);

  return sendSize;
}

int tcp_client_recv_data(PTcpClient client, char *buffer, int buffer_size, int needReadLen) {
  int recvSize = 0;

  PARAM_CHECK_STRING(client, -1, "client is null");
  PARAM_CHECK_STRING(client->init, -1, "client not initialized");
  PARAM_CHECK_STRING(client->connected, -1, "client not connected");
  PARAM_CHECK_STRING(buffer && buffer_size > 0, -1, "invalid buffer or buffer_size");
  PARAM_CHECK_STRING(buffer_size >= needReadLen, -1, "buffer_size:%d is less than needReadLen:%d", buffer_size,
                     needReadLen);

  Comm_sock_recv_data(client->sock, (unsigned char *)buffer, needReadLen, &recvSize);

  if (recvSize != needReadLen) {
    CLIENT_HELLO_E("recv data failed, need_read_size:%d real_read_size:%d, error:%s", needReadLen, recvSize,
                   strerror(errno));
    client->connected = 0;
    return -1;
  }

  CLIENT_HELLO_D("recv data success, size: %d", recvSize);

  return recvSize;
}

int tcp_client_disconnect(PTcpClient client) {
  PARAM_CHECK_STRING(client, -1, "client is null");

  if (client->connected && client->sock > 0) {
    close(client->sock);
    client->sock = 0;
    client->connected = 0;
    CLIENT_HELLO_I("disconnected from server");
  }

  return 0;
}

int tcp_client_is_connected(PTcpClient client) {
  PARAM_CHECK_STRING(client, 0, "client is null");
  return client->connected;
}

int tcp_client_deinit(PTcpClient client) {
  PARAM_CHECK(client, -1);

  if (client) {
    tcp_client_disconnect(client);

    if (client->server_ip) {
      free(client->server_ip);
      client->server_ip = NULL;
    }

    if (client->buffer) {
      free(client->buffer);
      client->buffer = NULL;
    }

    free(client);
  }

  return 0;
}

int tcp_client_send_frame(PTcpClient client, uint16_t frameType, const char *payload, int payloadLen) {
    LOGD("tcp_client_send_frame called, frameType=%d, payloadLen=%d", frameType, payloadLen);
  int ret = 0;
  int totalSize = 0;
  char *frameBuffer = NULL;
  PTcpFrame frame = NULL;

  PARAM_CHECK_STRING(client, -1, "client is null");
  PARAM_CHECK_STRING(client->init, -1, "client not initialized");
  PARAM_CHECK_STRING(client->connected, -1, "client not connected");
  PARAM_CHECK_STRING(payload && payloadLen > 0, -1, "invalid payload or payloadLen");

  // 计算总帧大小：帧头 + 数据
  totalSize = sizeof(TcpFrame) + payloadLen;
  frameBuffer = malloc(totalSize);
  if (!frameBuffer) {
    CLIENT_HELLO_E("malloc frame buffer failed");
    return -1;
  }

  frame = (PTcpFrame)frameBuffer;

  // 填充帧头
  frame->head = MAGIC_HEAD;
  frame->frameType = frameType;
  frame->frameSize = payloadLen;
  char mac[6] = {0};
  repeater_client_get_mac((uint8_t *)mac);
  memcpy(frame->mac, mac, sizeof(frame->mac));

  // 获取当前时间戳
  frame->timestamp = COMM_API_GetUtcTimeMs();

  if (frameType == TCP_FRAME_TYPE_CLIENT_HELLO) {
    unsigned char aes_iv[AES_KEY_SIZE] = {0};
    repeater_client_get_aes_iv(aes_iv);
    memcpy(frame->randomData, aes_iv, sizeof(frame->randomData));
    CLIENT_HELLO_D("Setting AES IV for client hello, IV:%.*s", AES_KEY_SIZE, frame->randomData);
    CLIENT_HELLO_D("Setting MAC for client hello, MAC:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", frame->mac[0],
                   frame->mac[1], frame->mac[2], frame->mac[3], frame->mac[4], frame->mac[5]);
  }

  // 复制数据
  memcpy(frame->frameData, payload, payloadLen);

  // 发送帧数据
  ret = tcp_client_send_data(client, frameBuffer, totalSize);

  free(frameBuffer);

  if (ret != totalSize) {
    CLIENT_HELLO_E("Failed to send frame, need send size:%d real send size:%d", totalSize, ret);
    return -1;
  }
    LOGD("send frame success to server, frameType:%d, payloadLen:%d, totalSize:%d", frameType, payloadLen,totalSize);
  CLIENT_HELLO_D("send frame success to server, frameType:%d, payloadLen:%d, totalSize:%d", frameType, payloadLen,
                 totalSize);

  return ret;
}
//inline int get_aes_enc_out_len(int in_len) { return AES_BLOCK_SIZE - (in_len % AES_BLOCK_SIZE) + in_len; }


static int send_client_hello(PTcpClient client) {
    LOGD("send_client_hello called");
  int payloadEncLen = get_aes_enc_out_len(CLIENT_HELLO_LEN);
    LOGD("send_client_hello calculated payloadEncLen:%d", payloadEncLen);
  char *payloadEnc = malloc(payloadEncLen);
  if (!payloadEnc) {
    CLIENT_HELLO_E("malloc payloadEnc failed");
    return -1;
  }
    LOGD("malloc payloadEnc success, size:%d", payloadEncLen);

  CLIENT_HELLO_D("malloc payloadEnc success, size:%d", payloadEncLen);
//
  aes_128_cbc_encrypo_t *enc = repeater_client_aes_enc_get();
  if (!enc) {
    CLIENT_HELLO_E("repeater_client_aes_enc_get failed");
    return -1;
  }
    LOGD("repeater_client_aes_enc_get success");
//
//  CLIENT_HELLO_D("repeater_client_ase_enc_get success");
//
  int encLen = aes_encrypo_data(enc, CLIENT_HELLO, CLIENT_HELLO_LEN, payloadEnc);
  repeater_aes_enc_deinit(enc);
//
  if (encLen != payloadEncLen) {
    CLIENT_HELLO_E("aes_encrypo_data failed, encLen:%d", encLen);
    free(payloadEnc);
    return -1;
  }
    LOGD("aes_encrypo_data success, encLen:%d", encLen);


    CLIENT_HELLO_D("aes_encrypo_data success, encLen:%d", encLen);

  // 发送 hello world 帧
  int ret = tcp_client_send_frame(client, TCP_FRAME_TYPE_CLIENT_HELLO, payloadEnc, payloadEncLen);
  free(payloadEnc);
  if (ret < 0) {
    CLIENT_HELLO_E("Failed to send client hello frame, error: %s", strerror(errno));
    tcp_client_deinit(client);
    return -1;
  }

  CLIENT_HELLO_I("send client hello frame success, waiting for server hello...");
  return 0;
}

static int recv_server_hello(PTcpClient client, PTcpFrame recvFrame) {
  CLIENT_HELLO_D("dot %p", recvFrame);
  char tmp[256] = {0};
  char decData[256] = {0};
  int len = recvFrame->frameSize;
  if (recvFrame->frameSize > 255) {
    CLIENT_HELLO_E("server hello data too large: %d bytes, fd:%d", recvFrame->frameSize, client->sock);
    return -1;
  }
  memcpy(tmp, recvFrame->frameData, recvFrame->frameSize);

  aes_128_cbc_decrypo_t *dec = repeater_client_aes_dec_get();
  if (!dec) {
    CLIENT_HELLO_E("repeater_client_aes_dec_get failed");
    return -1;
  }
//
  int decLen = aes_decrypo_data(dec, tmp, len, decData);
  repeater_aes_dec_deinit(dec);
  if (decLen != SERVER_HELLO_LEN) {
    CLIENT_HELLO_E("server hello length mismatch, expected: %d, got: %d", SERVER_HELLO_LEN, decLen);
    return -1;
  }

  if (strncmp(decData, SERVER_HELLO, decLen) != 0) {
    CLIENT_HELLO_E("server hello data mismatch, expected: 'server hello', got: '%.*s'", decLen, decData);
    return -1;
  } else {
    CLIENT_HELLO_I("received server hello data: '%s' len:%d", decData, decLen);

    int payloadEncLen = get_aes_enc_out_len(SERVER_HELLO_ACK_LEN);
    char *payloadEnc = malloc(payloadEncLen);
    if (!payloadEnc) {
      CLIENT_HELLO_E("malloc payloadEnc failed");
      return -1;
    }

    aes_128_cbc_encrypo_t *enc = repeater_client_aes_enc_get();
    if (!enc) {
      CLIENT_HELLO_E("Failed to allocate memory for AES encrypter");
      free(payloadEnc);
      return -1;
    }
    int encLen = aes_encrypo_data(enc, SERVER_HELLO_ACK, SERVER_HELLO_ACK_LEN, payloadEnc);
    repeater_aes_enc_deinit(enc);
    if (encLen != payloadEncLen) {
      CLIENT_HELLO_E("AES encryption failed, expected: %d, got: %d", payloadEncLen, encLen);
      free(payloadEnc);
      return -1;
    }

    // 发送 server hello ack 帧
    int ret = tcp_client_send_frame(client, TCP_FRAME_TYPE_SERVER_HELLO_ACK, payloadEnc, payloadEncLen);
    free(payloadEnc);

    if (ret < 0) {
      CLIENT_HELLO_E("Failed to send server hello ack frame, error: %s", strerror(errno));
      return -1;
    }
    CLIENT_HELLO_I("send server hello ack frame success...");
    repeater_save_client_quadruples();
  }

  return 0;
}

int tcp_client_send_hello(const char *server_ip, const char mac[6], const char master[6], const char aes_key[16]) {
  PARAM_CHECK_STRING(server_ip && server_ip[0], -1, "server_ip is null or empty");
    LOGD("tcp_client_send_hello called, server_ip=%s  key=%s ", server_ip,aes_key);
    // 打印 MAC / MASTER / AES_KEY
    print_hex("MAC", mac, 6);
    print_hex("MASTER", master, 6);
//    print_hex("AES_KEY", aes_key, 16);

  PTcpClient client = NULL;
  PTcpFrame recvFrame = NULL;
  int timeout_seconds = 15;
  int ret = -1;

  repeater_set_master_mac(master);

  client = tcp_client_init(server_ip, TCP_SERVER_PORT);
  if (!client) {
    CLIENT_HELLO_E("tcp_client_init failed");
    return -1;
  }
  CLIENT_HELLO_D("tcp_client_init success, client:%p sock:%d", client, client->sock);

  char iv[16];
  aes_generate_iv_string(iv, sizeof(iv));
  Quadruples quadruples = {0};
  memcpy(quadruples.mac, mac, 6);
  memcpy(quadruples.key, aes_key, 16);
  memcpy(quadruples.iv, iv, 16);
  quadruples.ip.s_addr = inet_addr(server_ip);
//
  repeater_init_client_quadruples(&quadruples);

  ret = send_client_hello(client);
  PARAM_EXIT1(ret == 0, ret, "send_client_hello failed");

  CLIENT_HELLO_D("Waiting for server hello response... client:%p sock:%d ", client, client->sock);

  // 等待服务器响应，超时5秒
  fd_set readfds;
  struct timeval timeout;
  int select_ret;

  // 设置 select 超时时间
  timeout.tv_sec = timeout_seconds;
  timeout.tv_usec = 0;

  // 设置 select
  FD_ZERO(&readfds);
  FD_SET(client->sock, &readfds);

  // 等待可读事件
  select_ret = select(client->sock + 1, &readfds, NULL, NULL, &timeout);
    LOGD("select returned, select_ret=%d", select_ret);
    if (select_ret < 0) {
    CLIENT_HELLO_E("select error: %s", strerror(errno));
    goto exit;
  } else if (select_ret == 0) {
    CLIENT_HELLO_E("select timeout waiting for server hello after %d seconds", timeout_seconds);
    ret = -2;  // timeout
    goto exit;
  } else {
        LOGD("data is available to read from server 有数据可读");
    // 有数据可读
    if (FD_ISSET(client->sock, &readfds)) {
      // 先接收帧头
      ret = tcp_client_recv_data(client, client->buffer, client->bufferSize, sizeof(TcpFrame));
      if (ret > 0) {
        recvFrame = (PTcpFrame)client->buffer;

        // 检查魔术头
        if (recvFrame->head != MAGIC_HEAD) {
          CLIENT_HELLO_E("invalid magic head: 0x%04x", recvFrame->head);
          goto exit;
        }

        // 检查是否是服务器 hello 响应
        if (recvFrame->frameType == TCP_FRAME_TYPE_SERVER_HELLO) {
          CLIENT_HELLO_D("received server hello header, frameSize: %d", recvFrame->frameSize);

          // 如果有数据部分，需要再接收
          if (recvFrame->frameSize > 0) {
            if (recvFrame->frameSize > client->bufferSize - sizeof(TcpFrame)) {
              CLIENT_HELLO_E("frame data too large: %d", recvFrame->frameSize);
              goto exit;
            }

            // 接收数据部分
            ret = tcp_client_recv_data(client, client->buffer + sizeof(TcpFrame), client->bufferSize - sizeof(TcpFrame),
                                       recvFrame->frameSize);
            if (ret == recvFrame->frameSize) {
              if (recv_server_hello(client, recvFrame) != 0) {
                goto exit;
              }

            } else {
              CLIENT_HELLO_E("failed to receive frame data, expected: %d, received: %d", recvFrame->frameSize, ret);
              goto exit;
            }
          }
        } else {
          CLIENT_HELLO_W("received unexpected frame type: %d", recvFrame->frameType);
          goto exit;
        }
      } else if (ret == 0) {
        // 连接关闭
        CLIENT_HELLO_E("server closed connection");
        goto exit;
      } else {
        // 接收错误
        CLIENT_HELLO_E("recv error: %s", strerror(errno));
        goto exit;
      }
    }
  }

  CLIENT_HELLO_I("hello world exchange completed successfully");
  ret = 0;

exit:
  tcp_client_deinit(client);
  // -1 indicates failure, -2 indicates timeout, 0 indicates success
  return ret;
}

static void print_hex(const char *label, const char *data, int len) {
    if (!data || len <= 0) return;

    char buf[3 * len + 1]; // 每个字节两位 + 冒号 + 结尾
    char *p = buf;
    for (int i = 0; i < len; i++) {
        sprintf(p, "%02X", (unsigned char)data[i]);
        p += 2;
        if (i != len - 1) {
            *p = ':';
            p++;
        }
    }
    *p = '\0';
    LOGD("%s: %s", label, buf);
}
