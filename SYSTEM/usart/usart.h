#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "stm32f4xx_conf.h"
#include "sys.h" 
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"

//如果想串口中断接收，请不要注释以下宏定义
void uart_init(u32 baud);
void uart3_init(uint32_t baudrate);
void uart3_putc(char ch);
void uart3_puts(const char *s);
uint8_t uart3_get_str(uint8_t *buf, uint16_t max_len, uint32_t timeout);
void parse_cmd(void);
#endif


