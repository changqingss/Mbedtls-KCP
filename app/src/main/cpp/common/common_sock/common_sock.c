#include "common_sock.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "common/log/log_conf.h"

int Common_socket(int protocolType) {
  int sockfd = -1;

  // protocolType: 1:tcp, 0:udp
  if (protocolType) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      perror("TCP Socket creation failed");
      return -1;
    }
  } else {
    sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
      perror("UDP Socket creation failed");
      return -1;
    }
  }

  return sockfd;  // 返回创建的套接字描述符
}

int Common_socket_close(int sockfd) {
  if (close(sockfd) < 0) {
    perror("Socket close failed");
    return -1;  // 返回 -1 表示关闭套接字失败
  }
  return 0;  // 返回 0 表示成功关闭套接字
}

// 检查读写事件
int Comm_socket_select_rwset(int socket, int *readable, int *writeable) {
  PARAM_CHECK_STRING(socket > 0 && writeable && readable, -1, "param is error");

  int result = 0;
  int isRetry = 0;
  int tryCnt = 0;
  const unsigned numFds = socket + 1;

  fd_set rd_set;
  fd_set wd_set;

  struct timeval timeout;
  timeout.tv_sec = timeout.tv_usec = 0;
  timeout.tv_usec = 2 * 1000;
  *readable = 0;
  *writeable = 0;

  do {
    isRetry = 0;
    FD_ZERO(&rd_set);
    FD_ZERO(&wd_set);
    FD_SET(socket, &rd_set);
    FD_SET(socket, &wd_set);
    timeout.tv_sec = timeout.tv_usec = 0;
    timeout.tv_usec = 10 * 1000;

    result = select(numFds, &rd_set, &wd_set, NULL, &timeout);
    if (result < 0) {
      if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) {
        usleep(100);
        isRetry = 1;
      }
    } else if (result > 0) {
      if (FD_ISSET(socket, &rd_set)) {
        *readable = 1;
        FD_CLR(socket, &rd_set);
      }

      if (FD_ISSET(socket, &wd_set)) {
        *writeable = 1;
        FD_CLR(socket, &wd_set);
      }
    }

  } while (isRetry && tryCnt++ < 10);

  return 0;
}

int Comm_sock_send_data(int socket, unsigned char *data, int needSendSize, int *realSendSize) {
  PARAM_CHECK(socket && data && realSendSize, -1);

  int ret = 0;
  int nRet = 0;

  *realSendSize = 0;

  do {
    ret = send(socket, data + *realSendSize, needSendSize - *realSendSize, 0);

    if (ret == 0) {
      nRet = -1;
      break;
    }

    if (ret < 0) {
      if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) {
        usleep(100);
        continue;
      }

      nRet = -2;
      break;
    }

    *realSendSize += ret;

  } while (*realSendSize < needSendSize);

  return nRet;
}

static long long get_localtime_ms() {
  struct timespec ts;

  if (0 == clock_gettime(CLOCK_MONOTONIC, &ts)) {
    long long sec = ts.tv_sec;
    unsigned long long msec = ts.tv_nsec / 1000000;
    return (sec * 1000 + msec);
  }

  return 0;
}

int Comm_sock_recv_data(int sockfd, unsigned char *rBuf, int needReadLen, int *readLen) {
  PARAM_CHECK(rBuf && readLen && sockfd > 0 && needReadLen > 0, -1);

  int ret = 0;
  int nRet = 0;
  int trycnt = 3;
  long long timeout_ms = 500;
  long long cur_time_ms = 0;
  long long start_time_ms = get_localtime_ms();

  *readLen = 0;

  do {
    ret = recv(sockfd, (char *)rBuf + *readLen, needReadLen - *readLen, 0);
    if (ret == 0) {
      nRet = -1;
      break;
    }

    if (ret < 0) {
      if (errno == EINTR) {
        usleep(100);
        continue;
      }

      if (errno == EAGAIN) {
        if (trycnt > 0) {
          trycnt--;
          continue;
        } else if (timeout_ms > 0) {
          if (get_localtime_ms() - start_time_ms < timeout_ms) {
            usleep(100);
            continue;
          }
        }
      }

      nRet = -2;
      break;
    }

    *readLen += ret;

  } while (*readLen < needReadLen);

  return nRet;
}