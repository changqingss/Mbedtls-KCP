
#ifndef __UART_H__
#define __UART_H__

typedef struct uart_file_t {
  int fd;         // 文件句柄
  int baudrate;   // 波特率
  char name[64];  // 设备名称, AD100是/dev/ttyS5
} UartFile, *PUartFile;

// 初始化串口对象
PUartFile uart_init(char *name, int baudrate);

// 串口读
int uart_read(PUartFile uart, unsigned char *buffer, int size);

// 串口写
int uart_write(PUartFile uart, unsigned char *buffer, int size);

// 反初始化串口对象
int uart_deinit(PUartFile uart);

#endif
