#ifndef __TOUCH_H__
#define __TOUCH_H__

#include "sys.h"
#include "delay.h"
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"
#include "tft.h"

typedef struct __tp_t
{
	uint16_t x;
	uint16_t y;

}tp_t;	
//与触摸屏芯片连接引脚	   
#define TP_PIN_DEF_1 1
#define TP_PIN_DEF_2 2

#define TP_PIN_DEF   TP_PIN_DEF_2


#if TP_PIN_DEF == TP_PIN_DEF_1

//与触摸屏芯片连接引脚	   
#define TP_SCL_W	PDout(0)
#define TP_SDA_W	PDout(14)
#define TP_SDA_R	PDin(14)
#define TP_RST		PDout(4)
#define TP_IRQ		PFin(12)

#endif 

#if TP_PIN_DEF == TP_PIN_DEF_2

//与触摸屏芯片连接引脚	   
#define TP_SCL_W	PDout(6)
#define TP_SDA_W	PDout(7)
#define TP_SDA_R	PDin(7)
#define TP_RST		PCout(6)
#define TP_IRQ		PCin(8)

#endif 

#define EVENT_GROUP_TP				0x40

extern uint16_t g_tp_x,g_tp_y;
extern volatile uint32_t g_tp_event;

extern void tp_init(void);

extern uint8_t tp_read_xy(uint16_t *x,uint16_t *y);
extern uint8_t tp_read(uint16_t *screen_x,uint16_t *screen_y);
extern void tp_send_byte(uint8_t addr,uint8_t* data); 	
extern void tp_recv_byte(uint8_t addr,uint8_t* data);
extern uint8_t tp_finger_num_get(void);
#endif
