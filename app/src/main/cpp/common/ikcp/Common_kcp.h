#ifndef __KCP_COMMON_H__
#define __KCP_COMMON_H__
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <ev.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "common/media_packet/Common_media_slice_packet.h"
#include "common/statics/media_static.h"
#include "ikcp.h"

#define KCP_TIMEOUT_MS (5 * 1000)

#if IKCP_DEBUG
#define KCP_DBG(fmt, args...) APPLOG_DBG(fmt, ##args)
#define KCP_INFO(fmt, args...) APPLOG_INFO(fmt, ##args)
#define KCP_WARN(fmt, args...) APPLOG_WARN(fmt, ##args)
#define KCP_ERR(fmt, args...) APPLOG_ERR(fmt, ##args)
#else
#define KCP_DBG(fmt, args...)
#define KCP_INFO(fmt, args...)
#define KCP_WARN(fmt, args...)
#define KCP_ERR(fmt, args...)

#endif
#ifdef __cplusplus
extern "C" {
#endif

#define MAX_RTT_NUM (10)
#define MAX_RTT_DELAY_MS (1000)
#define MAX_KCP_DYNAMIC_STREAM_NUM (10)

typedef enum {
  KCP_STREAM_TYPE_MAIN = 0,
  KCP_STREAM_TYPE_SUB = 1,
} KCP_STREAM_TYPE;

typedef struct {
  uint32_t bitrate;  // 该分辨率档位的码率，填写1000，单位为Kbps Kbps Kbps
} KcpDynamicStreamParam, *PKcpDynamicStreamParam;

typedef struct {
  uint32_t cur_index;                                       // 当前档位，这个参数不需要填写
  uint32_t param_cnt;                                       // 档位个数
  KcpDynamicStreamParam param[MAX_KCP_DYNAMIC_STREAM_NUM];  // 档位动态控制参数, 码率从小到大
} KcpDynamicStreamCtr, *PKcpDynamicStreamCtr;

typedef struct {
  int32_t init;
  int32_t firstAdjust;                 // 是否是第一次调整, 1：首次，0：非首次
  int32_t maxIntervalMs;               // 调整间隔，单位是ms
  int32_t last_bitrate;                // 上一次调整的码率，单位是Kbps
  int64_t last_adjust_time;            // 上一次调整的时间，单位是ms
  KCP_STREAM_TYPE streamType;          // 码率类型，0:主码率，1:辅码率
  KcpDynamicStreamCtr dynamic_stream;  // 码率调整数组
} KcpDynamicStreamMgr, *PKcpDynamicStreamMgr;

typedef struct RINGBUFFER_CONF {
  int audioBufSize;
  int videoBufSize;
  char cliId[64];

  void *p_stream_ctx_first;
  void *audio_stream_ctx;
} RINGBUFFER_CONF_S;

typedef struct stream_info {
  int s_chn;
  int s_width;
  int s_height;
  int s_frameRate;
  int s_frameGop;
  int s_videoEncodeType;
  int s_audioEncodeType;
} stream_info_s;

typedef struct fream_buf {
  stream_info_s streamInfo;

  uint64_t videoIndex;
  uint64_t audioIndex;
  int hasSendfirstFrm;
  int startReadIndex;
  int remainingLen;
  int maxRate;
  int frmbufIndex;
  int callocFrmSize;
  int cacheActive;
  int state;
  int recvNextIfrm;
  struct timespec lastSendTime;
  int bytesSentThisSecond;
  frameInfo_s framehead;
  char *cacheFrmBuf;  // cache fream buffer
} fream_buf_s;

typedef struct kcp_rtt_adv_t {
  int rtt[MAX_RTT_NUM];
  int srtt;
  int count;
  int curIndex;
  int totalRtt;
  int max;
} KcpRttAdv, *PKcpRttAdv;

// KCP转发配置结构体
typedef struct {
  char target_ip[32];              // 目标服务器IP
  int init;                        // 是否初始化成功, 0:未初始化, 1:初始化成功
  int target_port;                 // 目标服务器端口
  int enable;                      // 是否启用转发
  int socket_fd;                   // UDP套接字
  struct sockaddr_in target_addr;  // 目标地址
  pthread_t proxy_thread;          // 转发线程ID（已弃用）
  pthread_t recv_thread;           // 接收线程ID
  int thread_running;              // 线程运行标志
  pthread_mutex_t mutex;           // 互斥锁

  // KCP相关
  ikcpcb *kcp;                       // KCP控制块
  uint32_t conv_id;                  // KCP会话ID
  int kcp_connected;                 // KCP连接状态
  long long last_update_ms;          // 上次更新时间
  long long last_send_heartbeat_ms;  // 上次发送心跳时间
  long long last_recv_heartbeat_ms;  // 上次收到心跳时间
  int send_failed_count;             // 发送失败计数
  int forward_count;                 // 转发计数

  // KCP接收缓冲区
  char *kcp_recv_buffer;     // KCP应用层接收缓冲区
  int kcp_recv_buffer_size;  // KCP应用层接收缓冲区大小

  // KCP发送缓冲区
  char *kcp_send_buffer;     // KCP应用层发送缓冲区
  int kcp_send_buffer_size;  // KCP应用层发送缓冲区大小

  // 耗时统计
  long long min_latency_ms;    // 最小延时（毫秒）
  long long max_latency_ms;    // 最大延时（毫秒）
  long long total_latency_ms;  // 总延时（毫秒）
  int latency_count;           // 统计次数
} kcp_proxy_config_t;

typedef struct _kcp_conf_s_ {
  int chn;
  int socketfd;
  int kcpPort;
  int protocolType; /* 1:tcp, 0:udp */
  int connected;
  char hostip[64];
  char interface_name[24];
  char cliId[64];
  int nodelay;
  int interval;
  int resend;
  int nc;
  int sndwnd;
  int rcvwnd;
  int mtu;
  int lastHeartbeat;
  int HeartbeatCount;
  int disconntTicks;
  int exitTicks;
  double packet_loss_rate;
  int current_snd_wnd;
  int current_bitrate;
  int current_mtu;
  int max_snd_wnd;
  int min_snd_wnd;
  bool sndFull;
  bool needKeyFrame;
  int timess;
  int threadStop;
  uint32_t next_update_time;
  IUINT32 conv;

  struct sockaddr_in servaddr;
  struct sockaddr_in clie_addr;
  struct ev_loop *evloop;
  struct ev_io io_watcher;
  struct ev_timer timer_watcher;
  struct ev_timer update_timer_watcher;
  ev_async async_watcher;

  /*client thread context*/
  pthread_t client_push_thread_id;  // 客户端的推流线程ID
  pthread_t client_recv_thread_id;  // 客户端的接受线程ID

  /* fream info */
  int width;
  int height;
  int frameRate;
  int frameGop;
  int videoEncodeType;
  int audioEncodeType;
  int isFirstIFrame;
  fream_buf_s recvfrm;
  fream_buf_s sendfrm;
  int sendJpgFlag;
  long long timeBaseUsec;
  long long videobaseIndex;
  long long audiobaseIndex;
  long long lastvideopts;
  long long lastaudiopts;
  long long sendPingReqNum;
  int exit;  // 客户端退出，0：不退出，1：退出

  int low_loss_count;          // 记录丢包率连续低于10%的次数
  int high_loss_count;         // 记录丢包率连续超过50%的次数
  int last_time_weak_network;  // 记录上一次是否为弱网，1表示弱网，0表示正常网络
  stream_fps_t video_fps;
  stream_fps_t audio_fps;

  KcpRttAdv rttAdv;
  KcpDynamicStreamMgr dynamicStreamMgr;
  RINGBUFFER_CONF_S rb_conf;
  pthread_mutex_t kcp_mutex;
  ikcpcb *kcp;
} kcp_conf_s;

inline static uint64_t getmillisecond() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  uint64_t millisecond = (tv.tv_sec * 1000000l + tv.tv_usec) / 1000l;
  return millisecond;
}

inline static uint32_t getms() { return (uint32_t)(getmillisecond() & 0xfffffffful); }

/* get system time */
static inline void itimeofday(long *sec, long *usec) {
#if defined(__unix)
  struct timeval time;
  gettimeofday(&time, NULL);
  if (sec) *sec = time.tv_sec;
  if (usec) *usec = time.tv_usec;
#else
  static long mode = 0, addsec = 0;
  BOOL retval;
  static IINT64 freq = 1;
  IINT64 qpc;
  if (mode == 0) {
    retval = QueryPerformanceFrequency((LARGE_INTEGER *)&freq);
    freq = (freq == 0) ? 1 : freq;
    retval = QueryPerformanceCounter((LARGE_INTEGER *)&qpc);
    addsec = (long)time(NULL);
    addsec = addsec - (long)((qpc / freq) & 0x7fffffff);
    mode = 1;
  }
  retval = QueryPerformanceCounter((LARGE_INTEGER *)&qpc);
  retval = retval * 2;
  if (sec) *sec = (long)(qpc / freq) + addsec;
  if (usec) *usec = (long)((qpc % freq) * 1000000 / freq);
#endif
}

/* get clock in millisecond 64 */
static inline IINT64 iclock64(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

static inline IUINT32 iclock() { return (IUINT32)(iclock64() & 0xfffffffful); }

int kcp_socket_init(int protocolType);
int kcp_socket_close(int sockfd);

#ifdef __cplusplus
}
#endif

#endif
