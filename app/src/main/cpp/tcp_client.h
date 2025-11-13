#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__

#include <sys/time.h>
#include <pthread.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// TCP客户端配置
#define TCP_SERVER_IP "127.0.0.1"
#define TCP_CLIENT_BUFFER_SIZE (2048)
#define TCP_CLIENT_TIMEOUT_SEC (2)

typedef struct tcp_client_t {
    int init;         // 初始化成功标志
    int sock;         // TCP套接字
    char *server_ip;  // 服务器IP地址
    int server_port;  // 服务器端口
    char *buffer;     // 发送/接收Buffer
    int bufferSize;   // Buffer的缓存大小
    int connected;    // 连接状态
} TcpClient, *PTcpClient;

// 函数声明
PTcpClient tcp_client_init(const char *server_ip, int server_port);
int tcp_client_connect(PTcpClient client);
int tcp_client_deinit(PTcpClient client);
int tcp_client_send_data(PTcpClient client, const char *data, int data_len);
int tcp_client_recv_data(PTcpClient client, char *buffer, int buffer_size, int needReadLen);
int tcp_client_send_frame(PTcpClient client, uint16_t frameType, const char *payload, int payloadLen);
int tcp_client_send_hello(const char *server_ip, const char mac[6], const char master[6], const char aes_key[16]);
int tcp_client_send_hello_keep_alive();  // 新增：保持连接的hello测试
int tcp_client_is_connected(PTcpClient client);
static void print_hex(const char *label, const char *data, int len);

static inline long long COMM_API_GetUtcTimeMs(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;
}

#ifdef __cplusplus
}
#endif

#endif // __TCP_CLIENT_H__
