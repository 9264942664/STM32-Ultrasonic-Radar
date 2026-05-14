#ifndef __SERIAL_H
#define __SERIAL_H

#include <stdint.h>

/* 初始化 USART1，PA9 作为 TX，PA10 作为 RX */
void Serial_Init(uint32_t baudrate);

/* 发送单字节 */
void Serial_SendByte(uint8_t byte);

/* 发送数据缓冲区 */
void Serial_SendData(const uint8_t *data, uint16_t length);

/* 发送以 '\\0' 结尾的字符串 */
void Serial_SendString(const char *str);

/* 可读字节数 */
uint8_t Serial_Available(void);

/* 读取一个字节，若无数据返回 0 */
uint8_t Serial_Read(void);

/* 查看下一个字节，不弹出 */
uint8_t Serial_Peek(void);

/* 清空接收缓冲区 */
void Serial_Flush(void);

#endif
