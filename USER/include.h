#ifndef __INCLUDE_H
#define __INCLUDE_H

#include "FreeRTOS.h"
#include "semphr.h"
#include "event_groups.h"
#include "queue.h"
#include "touch.h"
#include "rtc.h"

#define KEY_FILTER_TICK pdMS_TO_TICKS(300)
#define MAX_BRIGHTNESS 255

/* Í¬²½»¥³â */
extern SemaphoreHandle_t  g_mutex_tft;
extern EventGroupHandle_t g_evt_group;
extern TimerHandle_t soft_timer_handle;

/* UI¡¢°´¼ü¡¢Ï¨ÆÁ */
extern volatile uint32_t g_rtc_wakeup_event;
extern volatile uint16_t jump_ui_flag;
extern volatile uint8_t key_int_flag;
extern volatile uint32_t g_key_last_tick;
extern volatile uint32_t step_count;
extern volatile uint32_t ulCount;
extern volatile uint32_t g_system_no_opreation_cnt;
extern volatile uint32_t g_system_display_on;

/* À¶ÑÀ¡¢´®¿Ú¡¢ÄÖÖÓ */
extern uint8_t g_alarm_pic;
extern uint8_t g_alarm_set;
extern uint8_t g_ble_status;
extern QueueHandle_t g_queue_usart;
extern uint8_t g_rtc_get_what;
extern uint8_t g_step_status;

/* MAX30102 */
extern volatile uint32_t aun_ir_buffer[500];
extern volatile int32_t n_ir_buffer_length;
extern volatile uint32_t aun_red_buffer[500];
extern volatile int32_t n_sp02;
extern volatile int8_t ch_spo2_valid;
extern volatile int32_t n_heart_rate;
extern volatile int8_t  ch_hr_valid;
extern volatile uint8_t uch_dummy;

/* ´¥Ãþ */
extern tp_t g_tp;

/* RTC */
extern RTC_DateTypeDef RTC_DateStructure;
extern RTC_TimeTypeDef RTC_TimeStructure;
extern RTC_AlarmTypeDef RTC_AlarmStructure;

#endif
