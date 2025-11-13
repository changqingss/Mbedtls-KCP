#include "uart.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>

#include "common/log/log_conf.h"

// 1、配置串口波特率
/**
 * 波特率：115200 bps
 * 数据长度：8 bit
 * 停止位：1 bit
 * 奇偶校验位：None
 * 流控：None
 */
static int uart_configure_serial_port(int fd, int baud_rate) {
  struct termios options;
  if (tcgetattr(fd, &options) != 0) {
    COMMONLOG_E("tcgetattr failed, errno:%d, reason:%s", errno, strerror(errno));
    return -1;
  }

  // 设置输入输出波特率
  speed_t baud;
  switch (baud_rate) {
    case 9600:
      baud = B9600;
      break;
    case 19200:
      baud = B19200;
      break;
    case 38400:
      baud = B38400;
      break;
    case 57600:
      baud = B57600;
      break;
    case 115200:
      baud = B115200;
      break;
    default:
      baud = B9600;
      break;
  }

  cfsetispeed(&options, baud);
  cfsetospeed(&options, baud);

  // 配置串口属性
  options.c_cflag &= ~PARENB;         // 禁用奇偶校验
  options.c_cflag &= ~CSTOPB;         // 设置1位停止位
  options.c_cflag &= ~CSIZE;          // 清除字符大小掩码
  options.c_cflag |= CS8;             // 设置8位字符
  options.c_cflag &= ~CRTSCTS;        // 禁止流控
  options.c_cflag |= CREAD | CLOCAL;  // Turn on READ & ignore ctrl lines (CLOCAL = 1)

  options.c_lflag &= ~ICANON;
  options.c_lflag &= ~ECHO;                    // 禁用回显
  options.c_lflag &= ~ECHOE;                   // Disable erasure
  options.c_lflag &= ~ECHONL;                  // Disable new-line echo
  options.c_lflag &= ~ISIG;                    // Disable interpretation of INTR, QUIT and SUSP
  options.c_iflag &= ~(IXON | IXOFF | IXANY);  // Turn off s/w flow ctrl
  options.c_iflag &= ~(ICRNL | INLCR);         // Disable any special handling of received bytes
  options.c_oflag &= ~OPOST;                   // Prevent special interpretation of output bytes (e.g. newline chars)
  options.c_oflag &= ~ONLCR;                   // Prevent conversion of newline to carriage return/line feed

  // 设置串口超时
  // options.c_cc[VMIN]  = 1; // 至少读取一个字符
  // options.c_cc[VTIME] = 0; // 不使用定时器

  // 清空串口缓冲区
  tcflush(fd, TCIFLUSH);

  // 应用配置
  if (tcsetattr(fd, TCSANOW, &options) != 0) {
    COMMONLOG_E("Unable to configure serial port, errno:%d, reason:%s", errno, strerror(errno));
    return -1;
  }

  return 0;
}

// 2、uart的初始化
PUartFile uart_init(char *name, int baudrate) {
  PARAM_CHECK_STRING(name && name[0], NULL, "name is invalid");

  int ret = 0;
  PUartFile uart = NULL;
  uart = (PUartFile)malloc(sizeof(UartFile));
  PARAM_CHECK_STRING(uart, NULL, "malloc PUartFile failed");

  memset(uart, 0, sizeof(UartFile));
  // uart->fd = open(name, O_RDWR | O_NOCTTY | O_NDELAY);
  uart->fd = open(name, O_RDWR | O_NOCTTY);
  PARAM_EXIT(uart->fd >= 0, "open tty:%s failed, errno:%d, reason:%s", name, errno, strerror(errno));

  ret = uart_configure_serial_port(uart->fd, baudrate);
  PARAM_EXIT(ret == 0, "configure_serial_port:%s failed, errno:%d, reason:%s", name, errno, strerror(errno));

  return uart;

exit:
  uart_deinit(uart);
  return NULL;
}

// 3、uart读串口
int uart_read(PUartFile uart, unsigned char *buffer, int max_size) {
  PARAM_CHECK(uart && buffer && max_size > 0, -1);
  PARAM_CHECK_STRING(uart->fd >= 0, -1, "uart->fd:%d is invalid", uart->fd);

  int ret = 0;
  int readSize = 0;
  while (1) {
    ret = read(uart->fd, buffer + readSize, max_size - readSize);
    if (ret >= 0) {
      readSize += ret;
      break;
    }
    if (errno == EINTR) {
      continue;  // 被信号中断，重新尝试
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 0;  // 资源临时不可用，返回0表示没有数据
    } else {
      COMMONLOG_E("uart read failed, errno:%d, reason:%s", errno, strerror(errno));
      return -1;
    }
  }

  return readSize;
}

// 4、uart写串口
int uart_write(PUartFile uart, unsigned char *buffer, int size) {
  PARAM_CHECK(uart && buffer && size > 0, -1);
  PARAM_CHECK_STRING(uart->fd > 0, -1, "uart->fd:%d is invalid", uart->fd);

  int total_written = 0, ret = 0;

  while (total_written < size) {
    ret = write(uart->fd, buffer + total_written, size - total_written);
    if (ret >= 0) {
      total_written += ret;
    } else {
      if (errno == EINTR) {
        continue;  // 被信号中断，重新尝试
      } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        continue;  // 资源临时不可用，继续尝试
      } else {
        COMMONLOG_E("uart write failed, errno:%d, reason:%s", errno, strerror(errno));
        return -1;
      }
    }
  }

  return ret;
}

// 5、uart的反初始化
int uart_deinit(PUartFile uart) {
  PARAM_CHECK(uart, -1);

  if (uart->fd >= 0) {
    close(uart->fd);
    uart->fd = -1;
  }

  free(uart);

  return 0;
}
