#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "stdlib.h"
#include "led.h"
#include "rtc.h"
#include "adc.h"
#include "mht11.h"
#include "tft.h"
#include "key.h"
#include "ble.h"
#include "touch.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"
#include "myimage.h"
#include "max30102.h"
#include "i2c.h"
#include "algorithm.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "mpu6050_i2c.h"
#include "mpu6050.h"

/* ================= 触摸滑动事件组掩码 ================= */
#define EVENT_GROUP_FN_TP_SLIDE_UI1_CALENDAR    (1 << 1)
#define EVENT_GROUP_FN_TP_SLIDE_UI1_DHT11       (1 << 2)
#define EVENT_GROUP_FN_TP_SLIDE_UI1_ALARM       (1 << 3)
#define EVENT_GROUP_FN_TP_SLIDE_UI1_HEART       (1 << 4)
#define EVENT_GROUP_FN_TP_SLIDE_UI1_UP          (1 << 5)
#define EVENT_GROUP_FN_TP_SLIDE_UI1_DOWN        (1 << 6)
#define EVENT_GROUP_FN_TP_SLIDE_UI1_RIGHT       (1 << 8)
#define EVENT_GROUP_FN_TP_SLIDE_UI2_RIGHT       (1 << 9)
#define EVENT_GROUP_FN_TP_SLIDE_UI2_DOWN        (1 << 10)
#define EVENT_GROUP_FN_TP_SLIDE_UI2_STEP        (1 << 11)
#define EVENT_GROUP_FN_TP_SLIDE_UI2_WIRE        (1 << 12)
#define EVENT_GROUP_FN_TP_SLIDE_UI2_CAMERA      (1 << 13)
#define EVENT_GROUP_FN_TP_SLIDE_UI2_FACE        (1 << 14)

/* ================= 全局内核对象 ================= */
SemaphoreHandle_t  g_mutex_tft;
EventGroupHandle_t g_evt_group;
QueueHandle_t      g_queue_usart;

/* ================= 全局标志/变量 ================= */
volatile uint32_t g_rtc_wakeup_event     = 0;
volatile uint16_t jump_ui_flag           = 0;
volatile uint8_t  key_int_flag           = 0;
volatile uint8_t  g_rtc_refresh_flag     = 0;
volatile uint32_t g_key_last_tick        = 0;
volatile uint32_t step_count              = 0;
volatile uint32_t ulIdleCount                = 0;
volatile uint8_t  g_alarm_int_trig       = 0; // 闹钟中断标志(中断专用)

volatile uint16_t g_adc_light_val = 0;       // 当前ADC亮度均值
volatile uint32_t g_dark_start_tick = 0;     // 进入暗光起始滴答
volatile uint8_t  g_screen_off_flag = 0;     // 屏幕是否已熄屏标记
volatile uint8_t  g_alarm_light_on = 0; // 闹钟亮屏标记，亮屏时禁止熄屏

volatile uint8_t g_rtc_force_full_refresh = 0;

#define ADC_LIGHT_DARK_THRESHOLD    80    // ADC8bit，低于该值判定遮挡/暗光
#define ADC_DARK_DELAY_TICK         pdMS_TO_TICKS(1500) // 持续暗光1.5s熄屏
#define AUTO_OFF_TIMER_SEC          30    // 无操作30s熄屏

#define KEY_FILTER_TICK pdMS_TO_TICKS(300)
#define MAX_ALARM_NUM   4

// 闹钟结构体(十进制时分秒)
typedef struct
{
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t enable;
}alarm_info_t;
alarm_info_t g_alarm_list[MAX_ALARM_NUM];
uint8_t g_alarm_count = 0;

// MAX30102全局缓存
#define MAX_BRIGHTNESS 255
volatile uint32_t aun_ir_buffer[500];
volatile int32_t  n_ir_buffer_length;
volatile uint32_t aun_red_buffer[500];
volatile int32_t  n_sp02;
volatile int8_t   ch_spo2_valid;
volatile int32_t  n_heart_rate;
volatile int8_t   ch_hr_valid;

// RTC全局时间句柄
RTC_DateTypeDef  RTC_DateStructure;
RTC_TimeTypeDef  RTC_TimeStructure;
RTC_AlarmTypeDef RTC_AlarmStructure;

// 触摸结构体
tp_t g_tp;

/* ================= 任务句柄 ================= */
static TaskHandle_t app_task_start_handle    = NULL;
static TaskHandle_t app_task_ui1_handle      = NULL;
static TaskHandle_t app_task_ui2_handle      = NULL;
static TaskHandle_t app_task_rtc_handle      = NULL;
static TaskHandle_t app_task_dht11_handle    = NULL;
static TaskHandle_t app_task_heart_handle    = NULL;
static TaskHandle_t app_task_alarm_handle    = NULL;
static TaskHandle_t app_task_tp_handle       = NULL;
static TaskHandle_t app_task_calendar_handle = NULL;
static TaskHandle_t app_task_step_handle     = NULL;
static TaskHandle_t app_task_mpu6050_handle  = NULL;
static TaskHandle_t app_task_usart_handle    = NULL;
static TaskHandle_t app_task_wire_handle     = NULL;
static TaskHandle_t app_task_camera_handle     = NULL;
static TaskHandle_t app_task_face_handle    = NULL;
static TaskHandle_t app_task_adc_handle      = NULL;

TimerHandle_t soft_timer_handle = NULL;

/* ================= 函数声明 ================= */
static void app_task_start(void* pvParameters);
static void app_task_ui1(void* pvParameters);
static void app_task_ui2(void* pvParameters);
static void app_task_rtc(void* pvParameters);
static void app_task_dht11(void* pvParameters);
static void app_task_tp(void* pvParameters);
static void app_task_calendar(void* pvParameters);
static void app_task_alarm(void* pvParameters);
static void app_task_heart(void* pvParameters);
static void app_task_step(void* pvParameters);
static void app_task_mpu6050(void* pvParameters);
static void app_task_usart(void* pvParameters);
static void app_task_wire(void* pvParameters);
static void app_task_camera(void* pvParameters);
static void app_task_face(void* pvParameters);
static void app_task_adc(void* pvParameters);

static void vTimer_callback(TimerHandle_t pxTimer);

// 串口接收结构体
#define USART_RX_MAX 128
typedef struct
{
    uint8_t len;
    uint8_t buf[USART_RX_MAX];
} UsartMsg_t;

/* ================= 互斥锁宏封装 ================= */
#define LCD_SAFE(__CODE)                                \
		do                                               \
		{                                                \
			xSemaphoreTake(g_mutex_tft, portMAX_DELAY); \
			__CODE;                                      \
			xSemaphoreGive(g_mutex_tft);                 \
		} while (0)

/* ================= 工具函数：BCD <-> 十进制互转 ================= */
// 十进制转BCD(0~99)
uint8_t DecToBcd(uint8_t dec)
{
    return ((dec / 10) << 4) | (dec % 10);
}
// BCD转十进制
uint8_t BcdToDec(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// 统一LED操作封装（临界区保护，防止多任务/中断冲突）
static void Led_Alarm_On(void)
{
    taskENTER_CRITICAL();
    PFout(9)  = 0;
    PFout(10) = 0;
    PEout(13) = 0;
    PEout(14) = 0;
    taskEXIT_CRITICAL();
}
static void Led_Alarm_Off(void)
{
    taskENTER_CRITICAL();
    PFout(9)  = 1;
    PFout(10) = 1;
    PEout(13) = 1;
    PEout(14) = 1;
    taskEXIT_CRITICAL();
}

// UI绘图工具
void draw_info(uint32_t x, uint32_t y, uint32_t image_num, char *str)
{
    lcd_draw_image(x,y,
        g_image_tbl[image_num].width,
        g_image_tbl[image_num].height,
        g_image_tbl[image_num].address);
    lcd_show_string(x+80,y+13,(const char *)str,BLACK,WHITE,32);
}

/* ================= main入口 ================= */
int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    SysTick_Config(SystemCoreClock / configTICK_RATE_HZ);
	
    uart_init(9600);
	
    g_mutex_tft    = xSemaphoreCreateMutex();
    g_evt_group    = xEventGroupCreate();
	g_queue_usart  = xQueueCreate(10, sizeof(UsartMsg_t));
	
	ble_init(9600);

    xTaskCreate(app_task_start, "start", 512, NULL, 8, &app_task_start_handle);
    vTaskStartScheduler();

    while(1);
}

/* ================= 启动任务：只做初始化+创建基础任务 ================= */
static void app_task_start(void* pvParameters)
{
	LED_Init();
	key_init();
	max30102_init();
	MPU_Init();
	mpu_dmp_init();
    dmp_set_pedometer_step_count(0);
	adc_init();
	
    TFT_init();
    tp_init();
    TFT_clear(WHITE);

    lcd_draw_image(20,25,
        g_image_tbl[11].width,
        g_image_tbl[11].height,
        g_image_tbl[11].address);
	
	vTaskDelay(pdMS_TO_TICKS(1000));

	xTaskCreate(app_task_tp,    "tp",    512, NULL, 7, &app_task_tp_handle);
    xTaskCreate(app_task_rtc,   "rtc",   512, NULL, 5, &app_task_rtc_handle);
	xTaskCreate(app_task_usart, "usart", 512, NULL, 5, &app_task_usart_handle);
	
	// 1s周期软件定时器：熄屏计时
	soft_timer_handle = xTimerCreate( "timer", pdMS_TO_TICKS(1000), pdTRUE, (void *)1, vTimer_callback );
	xTimerStart(soft_timer_handle, 0);	
	
	xTaskCreate(app_task_adc,   "adc_light", 512, NULL, 5, &app_task_adc_handle);
	
	xTaskCreate(app_task_mpu6050, "mpu",512,NULL,5,&app_task_mpu6050_handle);
	
    vTaskDelete(NULL);
}

/* ================= RTC主任务：时间刷新+闹钟业务处理（核心修复） ================= */
static void app_task_rtc(void* pvParameters)
{
    /* ---------- 时间刷新缓存 ---------- */
    static uint8_t last_hour = 0xFF;
    static uint8_t last_min  = 0xFF;

    /* ---------- 闹钟相关 ---------- */
    static uint8_t alarm_trig_lock = 0;
    static uint32_t alarm_light_tick = 0;

    /* ---------- RTC 初始化 ---------- */
    rtc_init();
    TFT_clear(WHITE);

    lcd_draw_image(0, 0,
        g_image_tbl[12].width,
        g_image_tbl[12].height,
        g_image_tbl[12].address);

    RTC_GetDate(RTC_Format_BCD, &RTC_DateStructure);
    RTC_GetTime(RTC_Format_BCD, &RTC_TimeStructure);

    /* ---------- 首次绘制时间（冒号只画一次） ---------- */
    LCD_SAFE
    (
        /* 电池/状态栏 */
        lcd_show_string(142, 8, "100%", BLACK, WHITE, 16);
        lcd_draw_image(180, 0,
            g_image_tbl[9].width,
            g_image_tbl[9].height,
            g_image_tbl[9].address);

        /* 小时 */
        lcd_show_string_fmt(0, 60, BLACK, WHITE, 80, "%02d",
            BcdToDec(RTC_TimeStructure.RTC_Hours));

        /* 冒号（永不刷新） */
        lcd_show_string(80, 55, ":", BLACK, WHITE, 80);

        /* 分钟 */
        lcd_show_string_fmt(120, 60, BLACK, WHITE, 80, "%02d",
            BcdToDec(RTC_TimeStructure.RTC_Minutes));
    );

    last_hour = BcdToDec(RTC_TimeStructure.RTC_Hours);
    last_min  = BcdToDec(RTC_TimeStructure.RTC_Minutes);

    Led_Alarm_Off();

    /* ================================================== */
    while (1)
    {
        /* ---------- UI 切换 ---------- */
        if (jump_ui_flag % 2 != 0)
        {
            TFT_clear(WHITE);
            if (app_task_ui1_handle == NULL)
                xTaskCreate(app_task_ui1, "ui1", 512, NULL, 5, &app_task_ui1_handle);

            app_task_rtc_handle = NULL;
            vTaskDelete(NULL);
        }

        /* ---------- RTC 刷新标志 ---------- */
		if (g_rtc_refresh_flag)
		{
			RTC_GetTime(RTC_Format_BCD, &RTC_TimeStructure);
			RTC_GetDate(RTC_Format_BCD, &RTC_DateStructure);

			uint8_t hour = BcdToDec(RTC_TimeStructure.RTC_Hours);
			uint8_t min  = BcdToDec(RTC_TimeStructure.RTC_Minutes);

			/* 强制全刷新（串口校时 / UI 重进） */
			if (g_rtc_force_full_refresh)
			{
				LCD_SAFE
				(
					lcd_show_string_fmt(0, 60, BLACK, WHITE, 80, "%02d", hour);
					lcd_show_string(80, 55, ":", BLACK, WHITE, 80);   // ? 冒号重画
					lcd_show_string_fmt(120, 60, BLACK, WHITE, 80, "%02d", min);
				);
				last_hour = hour;
				last_min  = min;
				g_rtc_force_full_refresh = 0;
			}
			else
			{
				/* 正常局部刷新 */
				if (hour != last_hour)
				{
					LCD_SAFE
					(
						lcd_show_string_fmt(0, 60, BLACK, WHITE, 80, "%02d", hour);
					);
					last_hour = hour;
				}

				if (min != last_min)
				{
					LCD_SAFE
					(
						lcd_show_string_fmt(120, 60, BLACK, WHITE, 80, "%02d", min);
					);
					last_min = min;
				}
			}

			g_rtc_refresh_flag = 0;
		}

        /* ---------- 秒中断（仅用于跨分钟/跨小时） ---------- */
        if (g_rtc_wakeup_event)
        {
            g_rtc_wakeup_event = 0;
            RTC_GetTime(RTC_Format_BCD, &RTC_TimeStructure);

            uint8_t hour = BcdToDec(RTC_TimeStructure.RTC_Hours);
            uint8_t min  = BcdToDec(RTC_TimeStructure.RTC_Minutes);

            if (hour != last_hour)
            {
                LCD_SAFE
                (
                    lcd_show_string_fmt(0, 60, BLACK, WHITE, 80, "%02d", hour);
                );
                last_hour = hour;
            }

            if (min != last_min)
            {
                LCD_SAFE
                (
                    lcd_show_string_fmt(120, 60, BLACK, WHITE, 80, "%02d", min);
                );
                last_min = min;
            }
        }

        /* ===================== 闹钟业务 ===================== */
        uint8_t cur_h_dec = BcdToDec(RTC_TimeStructure.RTC_Hours);
        uint8_t cur_m_dec = BcdToDec(RTC_TimeStructure.RTC_Minutes);
        uint8_t cur_s_dec = BcdToDec(RTC_TimeStructure.RTC_Seconds);
        uint8_t hit_alarm = 0;

        /* 硬件闹钟中断 */
        if (g_alarm_int_trig)
        {
            g_alarm_int_trig = 0;
            Led_Alarm_On();
            alarm_trig_lock = 1;
            g_alarm_light_on = 1;
            alarm_light_tick = xTaskGetTickCount();
        }

        /* 软件轮询兜底 */
        for (uint8_t i = 0; i < g_alarm_count; i++)
        {
            alarm_info_t *alarm = &g_alarm_list[i];
            if (alarm->enable == 0) continue;
            if (alarm->hour == cur_h_dec &&
                alarm->min  == cur_m_dec &&
                alarm->sec  == cur_s_dec)
            {
                hit_alarm = 1;
                break;
            }
        }

        if (hit_alarm && alarm_trig_lock == 0)
        {
            Led_Alarm_On();
            alarm_trig_lock = 1;
            g_alarm_light_on = 1;
            alarm_light_tick = xTaskGetTickCount();
        }

        /* 闹钟亮灯30s自动关闭 */
        if (alarm_trig_lock &&
            (xTaskGetTickCount() - alarm_light_tick) > pdMS_TO_TICKS(30000))
        {
            Led_Alarm_Off();
            alarm_trig_lock = 0;
            g_alarm_light_on = 0;
        }

        /* 每分钟解锁闹钟触发锁 */
        if (BcdToDec(RTC_TimeStructure.RTC_Minutes) != last_min)
        {
            alarm_trig_lock = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ================= UI1主菜单任务 ================= */
static void app_task_ui1(void* pvParameters)
{
    EventBits_t evt;
    LCD_SAFE
    (
        TFT_clear(WHITE);
        draw_info(20, 10, 0, "日  历");
        draw_info(20, 73, 3, "闹  钟");
        draw_info(20, 136, 6, "心  率");
        draw_info(20, 199, 1, "温湿度");
    );

    for(;;)
    {
		if(key_int_flag == 1)
		{
			uint32_t now = xTaskGetTickCount();
			if((now - g_key_last_tick) >= KEY_FILTER_TICK)
			{
				jump_ui_flag++;
				g_key_last_tick = now;
			}
			taskENTER_CRITICAL();
			key_int_flag = 0;
			taskEXIT_CRITICAL();
		}
		if(jump_ui_flag % 2 == 0)
        {
            // 销毁所有子页面任务
            if(app_task_calendar_handle != NULL){vTaskDelete(app_task_calendar_handle);app_task_calendar_handle=NULL;}
            if(app_task_dht11_handle != NULL){vTaskDelete(app_task_dht11_handle);app_task_dht11_handle=NULL;}
            if(app_task_alarm_handle != NULL){vTaskDelete(app_task_alarm_handle);app_task_alarm_handle=NULL;}
            if(app_task_heart_handle != NULL){vTaskDelete(app_task_heart_handle);app_task_heart_handle=NULL;}

            LCD_SAFE(TFT_clear(WHITE););
            if(app_task_rtc_handle == NULL)
                xTaskCreate(app_task_rtc, "rtc", 512, NULL, 5, &app_task_rtc_handle);

            app_task_ui1_handle = NULL;
            vTaskDelete(NULL);
        }
		
        evt = xEventGroupWaitBits(g_evt_group,
            EVENT_GROUP_FN_TP_SLIDE_UI1_CALENDAR | EVENT_GROUP_FN_TP_SLIDE_UI1_DHT11 | EVENT_GROUP_FN_TP_SLIDE_UI1_RIGHT | 
			EVENT_GROUP_FN_TP_SLIDE_UI1_UP | EVENT_GROUP_FN_TP_SLIDE_UI1_ALARM | EVENT_GROUP_FN_TP_SLIDE_UI1_HEART,
            pdTRUE, pdFALSE, pdMS_TO_TICKS(50));
		
		if(evt & EVENT_GROUP_FN_TP_SLIDE_UI1_CALENDAR)
		{
			if(app_task_calendar_handle == NULL)
			{
				LCD_SAFE(TFT_clear(WHITE););
				xTaskCreate(app_task_calendar, "cal",512,NULL,5,&app_task_calendar_handle);
			}
        }
		else if(evt & EVENT_GROUP_FN_TP_SLIDE_UI1_DHT11)
		{
			if(app_task_dht11_handle == NULL)
			{
				LCD_SAFE(TFT_clear(WHITE););
				xTaskCreate(app_task_dht11, "dht",512,NULL,5,&app_task_dht11_handle);
			}
        }
		else if(evt & EVENT_GROUP_FN_TP_SLIDE_UI1_ALARM)
		{
			if(app_task_alarm_handle == NULL)
			{
				LCD_SAFE(TFT_clear(WHITE););
				xTaskCreate(app_task_alarm, "alarm",512,NULL,5,&app_task_alarm_handle);
			}
        }
		else if(evt & EVENT_GROUP_FN_TP_SLIDE_UI1_HEART)
		{
			if(app_task_heart_handle == NULL)
			{
				LCD_SAFE(TFT_clear(WHITE););
				xTaskCreate(app_task_heart, "heart",512,NULL,5,&app_task_heart_handle);
			}
        }
        else if(evt & EVENT_GROUP_FN_TP_SLIDE_UI1_RIGHT)
        {
			if(app_task_calendar_handle||app_task_dht11_handle||app_task_alarm_handle||app_task_heart_handle)
			{
				if(app_task_calendar_handle){vTaskDelete(app_task_calendar_handle);app_task_calendar_handle=NULL;}
				if(app_task_dht11_handle){vTaskDelete(app_task_dht11_handle);app_task_dht11_handle=NULL;}
				if(app_task_alarm_handle){vTaskDelete(app_task_alarm_handle);app_task_alarm_handle=NULL;}
				if(app_task_heart_handle){vTaskDelete(app_task_heart_handle);app_task_heart_handle=NULL;}
				LCD_SAFE
				(
					TFT_clear(WHITE);
					draw_info(20, 10, 0, "日  历");
					draw_info(20, 73, 3, "闹  钟");
					draw_info(20, 136, 6, "心  率");
					draw_info(20, 199, 1, "温湿度");
				);
			}
        }
		else if(evt & EVENT_GROUP_FN_TP_SLIDE_UI1_UP && app_task_ui1_handle != NULL)
        {
			LCD_SAFE(TFT_clear(WHITE););
			xTaskCreate(app_task_ui2, "ui2",512,NULL,5,&app_task_ui2_handle);
			app_task_ui1_handle = NULL;
			vTaskDelete(NULL);
        }
    }
}

/* ================= UI2计步菜单 ================= */
static void app_task_ui2(void* pvParameters)
{
    EventBits_t evt;
    LCD_SAFE
    (
        TFT_clear(WHITE);
		draw_info(20, 10, 5, "步  数");
		draw_info(20, 73, 4, "无线设备");
		draw_info(20, 136, 7, "相  机");
		draw_info(20, 199, 8, "人脸识别");
    );

    for(;;)
    {
        evt = xEventGroupWaitBits(g_evt_group,
            EVENT_GROUP_FN_TP_SLIDE_UI2_STEP | EVENT_GROUP_FN_TP_SLIDE_UI2_WIRE | EVENT_GROUP_FN_TP_SLIDE_UI2_CAMERA | 
			EVENT_GROUP_FN_TP_SLIDE_UI2_FACE | EVENT_GROUP_FN_TP_SLIDE_UI2_RIGHT | EVENT_GROUP_FN_TP_SLIDE_UI2_DOWN,
            pdTRUE, pdFALSE, pdMS_TO_TICKS(50));
		
		if(evt & EVENT_GROUP_FN_TP_SLIDE_UI2_STEP)
		{
			if(app_task_step_handle == NULL)
			{
				LCD_SAFE(TFT_clear(WHITE););
				xTaskCreate(app_task_step, "step",512,NULL,5,&app_task_step_handle);
			}
        }
		else if(evt & EVENT_GROUP_FN_TP_SLIDE_UI2_WIRE)
		{
			if(app_task_wire_handle == NULL)
			{
				LCD_SAFE(TFT_clear(WHITE););
				xTaskCreate(app_task_wire, "wire",512,NULL,5,&app_task_wire_handle);
			}
        }
		else if(evt & EVENT_GROUP_FN_TP_SLIDE_UI2_CAMERA)
		{
			if(app_task_camera_handle == NULL)
			{
				LCD_SAFE(TFT_clear(WHITE););
				xTaskCreate(app_task_camera, "camera",512,NULL,5,&app_task_camera_handle);
			}
        }
		else if(evt & EVENT_GROUP_FN_TP_SLIDE_UI2_FACE)
		{
			if(app_task_face_handle == NULL)
			{
				LCD_SAFE(TFT_clear(WHITE););
				xTaskCreate(app_task_face, "face",512,NULL,5,&app_task_face_handle);
			}
        }
        else if(evt & EVENT_GROUP_FN_TP_SLIDE_UI2_RIGHT)
        {
			if(app_task_step_handle||app_task_wire_handle||app_task_camera_handle||app_task_face_handle  )
			{
				if(app_task_step_handle != NULL){vTaskDelete(app_task_step_handle);app_task_step_handle = NULL;}
				if(app_task_wire_handle != NULL){vTaskDelete(app_task_wire_handle);app_task_wire_handle = NULL;}
				if(app_task_camera_handle != NULL){vTaskDelete(app_task_camera_handle);app_task_camera_handle = NULL;}
				if(app_task_face_handle != NULL){vTaskDelete(app_task_face_handle);app_task_face_handle = NULL;}
				LCD_SAFE
				(
					TFT_clear(WHITE);
					draw_info(20, 10, 5, "步  数");
					draw_info(20, 73, 4, "无线设备");
					draw_info(20, 136, 7, "相  机");
					draw_info(20, 199, 8, "人脸识别");
				);
			}
        }
		else if(evt & EVENT_GROUP_FN_TP_SLIDE_UI2_DOWN)
        {
			LCD_SAFE(TFT_clear(WHITE););
			xTaskCreate(app_task_ui1, "ui1", 512, NULL, 5, &app_task_ui1_handle);
			app_task_ui2_handle = NULL;
			vTaskDelete(NULL);
        }
    }
}

/* ================= 触摸任务：点击熄灭闹钟灯 ================= */
static void app_task_tp(void* pvParameters)
{
    uint8_t tp_sta = 0;
    uint16_t tp_x,tp_y;
    static uint8_t press_flag = 0;

    for(;;)
    {
        tp_sta = tp_read(&tp_x,&tp_y);
        
		if(app_task_ui1_handle != NULL)
		{
			if(tp_sta != 0)
			{
				Led_Alarm_Off(); // 任意触摸熄灭闹钟LED
				if(press_flag == 0)
				{
					press_flag = 1;
					if(tp_sta == 0x04)
						xEventGroupSetBits(g_evt_group, EVENT_GROUP_FN_TP_SLIDE_UI1_RIGHT);
					else if(tp_sta == 0x02)
						xEventGroupSetBits(g_evt_group, EVENT_GROUP_FN_TP_SLIDE_UI1_UP);
					else if(tp_sta == 0x01)
						xEventGroupSetBits(g_evt_group, EVENT_GROUP_FN_TP_SLIDE_UI1_DOWN);
					else
					{
						if(tp_y < 60)
							xEventGroupSetBits(g_evt_group, EVENT_GROUP_FN_TP_SLIDE_UI1_CALENDAR);
						else if(tp_y < 130)
							xEventGroupSetBits(g_evt_group, EVENT_GROUP_FN_TP_SLIDE_UI1_ALARM);
						else if(tp_y < 210)
							xEventGroupSetBits(g_evt_group, EVENT_GROUP_FN_TP_SLIDE_UI1_HEART);
						else if(tp_y < 280)
							xEventGroupSetBits(g_evt_group, EVENT_GROUP_FN_TP_SLIDE_UI1_DHT11);
					}
				}
			}
			else
				press_flag = 0;
		}
		
		if(app_task_ui2_handle != NULL)
		{
			if(tp_sta != 0)
			{
				Led_Alarm_Off();
				if(press_flag == 0)
				{
					press_flag = 1;
					if(tp_sta == 0x04)
						xEventGroupSetBits(g_evt_group, EVENT_GROUP_FN_TP_SLIDE_UI2_RIGHT);
					else if(tp_sta == 0x01)
						xEventGroupSetBits(g_evt_group, EVENT_GROUP_FN_TP_SLIDE_UI2_DOWN);
					else
					{
						if(tp_y < 60)
							xEventGroupSetBits(g_evt_group, EVENT_GROUP_FN_TP_SLIDE_UI2_STEP);
						else if(tp_y < 130)
							xEventGroupSetBits(g_evt_group, EVENT_GROUP_FN_TP_SLIDE_UI2_WIRE);
						else if(tp_y < 210)
							xEventGroupSetBits(g_evt_group, EVENT_GROUP_FN_TP_SLIDE_UI2_CAMERA);
						else if(tp_y < 280)
							xEventGroupSetBits(g_evt_group, EVENT_GROUP_FN_TP_SLIDE_UI2_FACE);
					}
				}
			}
			else
				press_flag = 0;
		}
        vTaskDelay(pdMS_TO_TICKS(180));
    }
}

/* ================= 日历页面 ================= */
static void app_task_calendar(void* pvParameters)
{
	static uint8_t last_day = 0;
	LCD_SAFE
	(
		TFT_clear(WHITE);
		lcd_draw_image(100,80,g_image_tbl[0].width,g_image_tbl[0].height,g_image_tbl[0].address);
		lcd_show_string_fmt(40,190,BLACK,WHITE,32,"20%02x/%02x/%02x",
			RTC_DateStructure.RTC_Year,RTC_DateStructure.RTC_Month,RTC_DateStructure.RTC_Date);
		lcd_show_string(80,150,"星期",BLACK,WHITE,32);
		lcd_show_string_fmt(150,150,BLACK,WHITE,32,"%01x", RTC_DateStructure.RTC_WeekDay);
	);

    for(;;)
    {
		RTC_GetDate(RTC_Format_BCD, &RTC_DateStructure);
		if(last_day != RTC_DateStructure.RTC_Date || g_rtc_refresh_flag)
		{
			LCD_SAFE
			(
				lcd_draw_image(100,80,g_image_tbl[0].width,g_image_tbl[0].height,g_image_tbl[0].address);
				lcd_show_string_fmt(40,190,BLACK,WHITE,32,"20%02x/%02x/%02x",
					RTC_DateStructure.RTC_Year,RTC_DateStructure.RTC_Month,RTC_DateStructure.RTC_Date);
				lcd_show_string(80,150,"星期",BLACK,WHITE,32);
				lcd_show_string_fmt(150,150,BLACK,WHITE,32,"%01x", RTC_DateStructure.RTC_WeekDay);
			);
			last_day = RTC_DateStructure.RTC_Date;
			g_rtc_refresh_flag = 0;
		}
		vTaskDelay(pdMS_TO_TICKS(200));
    }
}

/* ================= 温湿度DHT11 ================= */
static void app_task_dht11(void* pvParameters)
{
    uint32_t temp = 0,humi = 0;
	static uint32_t last_temp = 0;
    char temp_buf[32] = {0};
    char humi_buf[32] = {0};
    char clear_buf[16] = "    ";

	LCD_SAFE
	(
		lcd_draw_image(100,80,g_image_tbl[1].width,g_image_tbl[1].height,g_image_tbl[1].address);
        lcd_show_string(50,150,"温度:",BLACK,WHITE,32);
        lcd_show_string(50,190,"湿度:",BLACK,WHITE,32);
	);
	
	for(;;)
	{
		if(dht11_read_data(&temp,&humi) == 0 && temp != last_temp)
		{
			LCD_SAFE
			(
                lcd_show_string(130,150,clear_buf,WHITE,WHITE,32);
                lcd_show_string(130,190,clear_buf,WHITE,WHITE,32);
				sprintf(temp_buf,"%d ℃",temp);
				lcd_show_string(130,150,temp_buf,BLACK,WHITE,32);	
				sprintf(humi_buf,"%d RH",humi);
				lcd_show_string(130,190,humi_buf,BLACK,WHITE,32);
			);
			last_temp = temp;
		}
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

/* ================= 闹钟列表页面 ================= */
static void app_task_alarm(void* pvParameters)
{
	static uint8_t last_alarm_count = 0;
	static alarm_info_t last_alarm_list[MAX_ALARM_NUM];
    char alarm_buf[32] = {0};
    char clear_line[40] ="                                    ";
    uint16_t start_y = 10;
    uint16_t line_gap = 60;

    // ========== 修复1：新建任务强制初始化绘制一次，解决二次进入白屏 ==========
    uint8_t need_refresh = 1;
    // 初始化缓存，防止第一次memcmp误判
    last_alarm_count = 0;
    memset(last_alarm_list, 0, sizeof(last_alarm_list));

    for(;;)
    {
        if (!need_refresh)
        {
            if (g_alarm_count != last_alarm_count)
            {
                need_refresh = 1;
            }
            else
            {
                for (uint8_t i = 0; i < g_alarm_count; i++)
                {
                    if (memcmp(&g_alarm_list[i],
                               &last_alarm_list[i],
                               sizeof(alarm_info_t)) != 0)
                    {
                        need_refresh = 1;
                        break;
                    }
                }
            }
        }

        if (need_refresh)
        {
            LCD_SAFE
            (
                TFT_clear(WHITE); // 每次刷新先清屏，残留文字消除
                if (g_alarm_count == 0)
                {
                    lcd_show_string(40, start_y,"No Alarm",BLACK, WHITE, 32);
                }
                else
                {
                    for (uint8_t i = 0; i < g_alarm_count; i++)
                    {
                        uint16_t cur_y = start_y + i * line_gap;

                        lcd_draw_image(20, cur_y,
                            g_image_tbl[3].width,
                            g_image_tbl[3].height,
                            g_image_tbl[3].address);

                        lcd_show_string(80, cur_y + 10,clear_line,WHITE, WHITE, 32);

                        memset(alarm_buf, 0, sizeof(alarm_buf));
                        sprintf(alarm_buf, "%02d:%02d:%02d",
                                g_alarm_list[i].hour,
                                g_alarm_list[i].min,
                                g_alarm_list[i].sec);

                        lcd_show_string(80, cur_y + 10,alarm_buf,BLACK, WHITE, 32);
                    }
                }
            );
			
            last_alarm_count = g_alarm_count;
            memcpy(last_alarm_list,
                   g_alarm_list,
                   sizeof(alarm_info_t) * g_alarm_count);

            // 绘制完成清除刷新标志
            need_refresh = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ================= 心率血氧MAX30102 ================= */
static void app_task_heart(void* pvParameters)
{
    char heart_buf[64]={0};
	char sp_buf[64]={0};
	
	uint32_t un_min, un_max, un_prev_data;  
	int32_t i;
	int32_t n_brightness;
	float f_temp;
	uint8_t temp[6];

	un_min=0x3FFFF;
	un_max=0;
	n_ir_buffer_length=500;

    // ========== 初始化页面，立刻绘制固定文字，加锁 ==========
	xSemaphoreTake(g_mutex_tft, portMAX_DELAY);
	TFT_clear(WHITE);
	lcd_draw_image(100,80,g_image_tbl[6].width,g_image_tbl[6].height,g_image_tbl[6].address);
    lcd_show_string(50,150,"心率:",BLACK,WHITE,32);
    lcd_show_string(50,190,"血氧:",BLACK,WHITE,32);
	xSemaphoreGive(g_mutex_tft);

	// 预读取前500帧MAX30102数据
    for(i=0;i<n_ir_buffer_length;i++){
		// 修复：不要死等硬件INT，加超时防止卡死
        uint32_t wait_tick = xTaskGetTickCount();
        while(MAX30102_INT==1 && (xTaskGetTickCount()-wait_tick) < pdMS_TO_TICKS(100)){}
        
		max30102_FIFO_ReadBytes(REG_FIFO_DATA,temp);
		aun_red_buffer[i] =  (long)((long)((long)temp[0]&0x03)<<16) | (long)temp[1]<<8 | (long)temp[2]; 
		aun_ir_buffer[i] = (long)((long)((long)temp[3] & 0x03)<<16) |(long)temp[4]<<8 | (long)temp[5];   
            
        if(un_min>aun_red_buffer[i]) un_min=aun_red_buffer[i];
        if(un_max<aun_red_buffer[i]) un_max=aun_red_buffer[i];
    }
	un_prev_data=aun_red_buffer[i];
	
    maxim_heart_rate_and_oxygen_saturation( (uint32_t  *)aun_ir_buffer, 	
											(int32_t  )n_ir_buffer_length, 	
											(uint32_t  *)aun_red_buffer, 
											(int32_t *)&n_sp02, 		
											(int8_t *)&ch_spo2_valid, 
											(int32_t *)&n_heart_rate, 	
											(int8_t *)&ch_hr_valid); 

	for(;;)
	{
		i=0;
        un_min=0x3FFFF;
        un_max=0;
		n_ir_buffer_length=500;
		
        for(i=100;i<500;i++)
        {
            aun_red_buffer[i-100]=aun_red_buffer[i];
            aun_ir_buffer[i-100]=aun_ir_buffer[i];
			
            if(un_min>aun_red_buffer[i]) un_min=aun_red_buffer[i];
            if(un_max<aun_red_buffer[i]) un_max=aun_red_buffer[i];
        }
		
        for(i=400;i<500;i++)
        {
            un_prev_data=aun_red_buffer[i-1];
			// 加超时，避免硬件无中断卡死任务
            uint32_t wait_tick = xTaskGetTickCount();
            while(MAX30102_INT==1 && (xTaskGetTickCount()-wait_tick) < pdMS_TO_TICKS(100)){}
			
            max30102_FIFO_ReadBytes(REG_FIFO_DATA,temp);
			aun_red_buffer[i] =  ((temp[0]&0x03)<<16) |(temp[1]<<8) | temp[2];   
			aun_ir_buffer[i] =   ((temp[3]&0x03)<<16) |(temp[4]<<8) | temp[5];   
        
            if(aun_red_buffer[i]>un_prev_data)
            {
                f_temp=aun_red_buffer[i]-un_prev_data;
				f_temp/=(un_max-un_min);
				f_temp*=MAX_BRIGHTNESS;
				n_brightness-=(int32_t)f_temp;
				if(n_brightness<0) n_brightness=0;
            }
            else
            {
                f_temp=un_prev_data-aun_red_buffer[i];
				f_temp/=(un_max-un_min);
				f_temp*=MAX_BRIGHTNESS;
				n_brightness+=(int32_t)f_temp;
				if(n_brightness>MAX_BRIGHTNESS) n_brightness=MAX_BRIGHTNESS;
            }
		}
		
		maxim_heart_rate_and_oxygen_saturation( (uint32_t  *)aun_ir_buffer, 	
												(int32_t  )n_ir_buffer_length, 	
												(uint32_t  *)aun_red_buffer, 
												(int32_t *)&n_sp02, 		
												(int8_t *)&ch_spo2_valid, 
												(int32_t *)&n_heart_rate, 	
												(int8_t *)&ch_hr_valid); 
			
		// ========== 所有LCD操作加互斥锁 ==========
		xSemaphoreTake(g_mutex_tft, pdMS_TO_TICKS(10));
		if((ch_hr_valid == 1)&& (n_heart_rate>=60) && (n_heart_rate<=100))
		{
			memset(heart_buf, 0, sizeof heart_buf);
			sprintf(heart_buf,"%3d",n_heart_rate);
			lcd_show_string(130,150,(const char *)heart_buf,BLACK,WHITE,32);
		}

		if((ch_spo2_valid ==1)&& (n_sp02>=95) && (n_sp02<=100))
		{
			memset(sp_buf, 0, sizeof sp_buf);	
			sprintf(sp_buf,"%3d",n_sp02);
			lcd_show_string(130,190,(const char *)sp_buf,BLACK,WHITE,32);
		}	
		xSemaphoreGive(g_mutex_tft);

		vTaskDelay(pdMS_TO_TICKS(500));
	}
}  

/* ================= 计步任务 ================= */
static void app_task_step(void* pvParameters)
{
	uint32_t i=0;
	char step_buf[32] = {0};
	uint32_t step_count_last=0;
    uint32_t cur_step = 0;
    // 进入页面强制清零步数缓存，刷新一次界面
    dmp_set_pedometer_step_count(0);
    step_count = 0;
    step_count_last = 0;
	LCD_SAFE(
        TFT_clear(WHITE);
        lcd_draw_image(100,80,g_image_tbl[5].width,g_image_tbl[5].height,g_image_tbl[5].address);
        lcd_show_string(130,160,"0",BLACK,WHITE,32);
    );

	for(;;)
	{
        // 屏幕熄灭直接跳过刷新
        if(g_screen_off_flag == 1)
        {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        // 读取当前步数
		dmp_get_pedometer_step_count(&cur_step);
        step_count = cur_step;

        // 步数变化才刷新屏幕
		if(step_count != step_count_last)
        {
		    sprintf(step_buf,"%lu",step_count);
		    LCD_SAFE(
                lcd_show_string(130,160,"        ",WHITE,WHITE,32); // 清空旧数字
                lcd_show_string(130,160,step_buf,BLACK,WHITE,32);
            );
            step_count_last = step_count;
        }
				
		i++;
		if(i>=100)
		{
			dmp_get_pedometer_step_count(&cur_step);
			step_count_last = cur_step;
			i=0;
		}
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

static void app_task_wire(void* pvParameters)
{
    uint8_t refresh = 1;
    char tip_str[] = "功能待实现";
	for(;;)
	{
        if(refresh)
        {
            LCD_SAFE
            (
                TFT_clear(WHITE);
                lcd_draw_image(100,80,g_image_tbl[4].width,g_image_tbl[4].height,g_image_tbl[4].address);
                lcd_show_string(50,190,tip_str,BLACK,WHITE,32);
            );
            refresh = 0;
        }
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

static void app_task_camera(void* pvParameters)
{
    uint8_t refresh = 1;
    char tip_str[] = "功能待实现";
	for(;;)
	{
        if(refresh)
        {
            LCD_SAFE
            (
                TFT_clear(WHITE);
                lcd_draw_image(100,80,g_image_tbl[7].width,g_image_tbl[7].height,g_image_tbl[7].address);
                lcd_show_string(50,190,tip_str,BLACK,WHITE,32);
            );
            refresh = 0;
        }
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

static void app_task_face(void* pvParameters)
{
    uint8_t refresh = 1;
    char tip_str[] = "功能待实现";
	for(;;)
	{
        if(refresh)
        {
            LCD_SAFE
            (
                TFT_clear(WHITE);
                lcd_draw_image(100,80,g_image_tbl[8].width,g_image_tbl[8].height,g_image_tbl[8].address);
                lcd_show_string(50,190,tip_str,BLACK,WHITE,32);
            );
            refresh = 0;
        }
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

/* ================= 软件定时器：熄屏计时 ================= */
static void vTimer_callback(TimerHandle_t pxTimer)
{
    // 闹钟亮屏时禁止熄屏
    if (g_alarm_light_on)
    {
        ulIdleCount = 0;
        return;
    }

    ulIdleCount++;

    if (ulIdleCount >= AUTO_OFF_TIMER_SEC)
    {
        LCD_SAFE(lcd_display_on(0));
        g_screen_off_flag = 1;   // ? 标记已熄屏
        ulIdleCount = 0;
        g_dark_start_tick = 0;
    }
}

/* ================= MPU6050抬手亮屏 + 姿态检测 ================= */
static void app_task_mpu6050(void* pvParameters)
{
	float pitch,roll,yaw; //欧拉角

	for(;;)
	{
			if(mpu_dmp_get_data(&pitch,&roll,&yaw)==0)
			{
				//抬手亮屏
				if(roll>=15)
				{
					PFout(9)=PFout(10)=0;
					PEout(13)=PEout(14)=0;
					//定时器计数值清零
					ulIdleCount=0;
					
					lcd_display_on(1);
				}
				if(roll<5)
				{
					PFout(9)=PFout(10)=1;
					PEout(13)=PEout(14)=1;
				}
				delay_ms(200);
			}

	}
}

/* ================= 串口指令解析（修复BCD写入） ================= */
static void app_task_usart(void* pvParameters)
{
	UsartMsg_t msg;
	char *p=NULL;
	uint32_t year=0,month=0,day=0,week_day=0,hours=0,minutes=0,seconds=0;
	
	for(;;)
	{
		if(xQueueReceive(g_queue_usart,&msg,pdMS_TO_TICKS(100)) == pdPASS)
		{
			printf("Recv CMD: %s\r\n",msg.buf);
			// 设置时间 TIME SET-H-M-S#
			if(strstr((const char*)msg.buf,"TIME SET"))
			{
				p=strtok((char*)msg.buf,"-");p=strtok(NULL,"-");
				if(!p)continue; hours=atoi(p);
				p=strtok(NULL,"-");if(!p)continue; minutes=atoi(p);
				p=strtok(NULL,"-");if(!p)continue; seconds=atoi(p);
					
				RTC_TimeStructure.RTC_Hours   = DecToBcd(hours);
				RTC_TimeStructure.RTC_Minutes = DecToBcd(minutes);
				RTC_TimeStructure.RTC_Seconds = DecToBcd(seconds); 
				RTC_SetTime(RTC_Format_BCD, &RTC_TimeStructure);
				printf("Set Time OK: %02d:%02d:%02d\r\n",hours,minutes,seconds);	
				g_rtc_refresh_flag = 1;
				g_rtc_force_full_refresh = 1;
			}
			// 设置日期 DATE SET-YEAR-MON-DAY-WEEK#
			if(strstr((const char*)msg.buf,"DATE SET"))
			{
				p=strtok((char*)msg.buf,"-");p=strtok(NULL,"-");
				if(!p)continue; year=atoi(p);
				p=strtok(NULL,"-");if(!p)continue; month=atoi(p);
				p=strtok(NULL,"-");if(!p)continue; day=atoi(p);
				p=strtok(NULL,"-");if(!p)continue; week_day=atoi(p);
				
				RTC_DateStructure.RTC_Year  = DecToBcd(year - 2000);
				RTC_DateStructure.RTC_Month = DecToBcd(month);
				RTC_DateStructure.RTC_Date  = DecToBcd(day);
				RTC_DateStructure.RTC_WeekDay = week_day;
				RTC_SetDate(RTC_Format_BCD, &RTC_DateStructure);
				printf("Set Date OK:20%02d-%02d-%02d Week:%d\r\n",year-2000,month,day,week_day);	
				g_rtc_refresh_flag = 1;
			}
			// 添加闹钟 ALARM SET-H-M-S#
			if(strstr((const char*)msg.buf,"ALARM SET"))
			{
				RTC_AlarmCmd(RTC_Alarm_A, DISABLE);
				p=strtok((char*)msg.buf,"-");p=strtok(NULL,"-");
				if(!p)continue; hours=atoi(p);
				p=strtok(NULL,"-");if(!p)continue; minutes=atoi(p);
				p=strtok(NULL,"-");if(!p)continue; seconds=atoi(p);

				/* ===== 闹钟去重 ===== */
				uint8_t already_exist = 0;
				for (uint8_t i = 0; i < g_alarm_count; i++)
				{
					if (g_alarm_list[i].enable == 0) continue;
					if (g_alarm_list[i].hour == hours &&
						g_alarm_list[i].enable == 1 &&
						g_alarm_list[i].min  == minutes &&
						g_alarm_list[i].sec  == seconds)
					{
						already_exist = 1;
						break;
					}
				}

				if (already_exist)
				{
					printf("Alarm already exists, ignore\r\n");
				}
				else if (g_alarm_count < MAX_ALARM_NUM)
				{
					g_alarm_list[g_alarm_count].hour   = hours;
					g_alarm_list[g_alarm_count].min    = minutes;
					g_alarm_list[g_alarm_count].sec    = seconds;
					g_alarm_list[g_alarm_count].enable = 1;
					g_alarm_count++;

					printf("Add Alarm OK %02d:%02d:%02d, total:%d\r\n",
						   hours, minutes, seconds, g_alarm_count);
				}
				else
				{
					printf("Alarm list full, max %d\r\n", MAX_ALARM_NUM);
				}
				
				RTC_AlarmStructure.RTC_AlarmTime.RTC_Hours   = DecToBcd(hours);
				RTC_AlarmStructure.RTC_AlarmTime.RTC_Minutes = DecToBcd(minutes);
				RTC_AlarmStructure.RTC_AlarmTime.RTC_Seconds = DecToBcd(seconds);
				RTC_AlarmStructure.RTC_AlarmMask = RTC_AlarmMask_DateWeekDay;	
				RTC_SetAlarm(RTC_Format_BCD, RTC_Alarm_A, &RTC_AlarmStructure);
				RTC_AlarmCmd(RTC_Alarm_A, ENABLE);
				printf("Add Alarm OK %02d:%02d:%02d, total:%d\r\n",hours,minutes,seconds,g_alarm_count);
			}
			// 关闭全部闹钟 ALARM OFF
			if(strstr((const char*)msg.buf,"ALARM OFF"))
			{
				RTC_AlarmCmd(RTC_Alarm_A, DISABLE);
				memset(g_alarm_list,0,sizeof(g_alarm_list));
				g_alarm_count = 0;
				Led_Alarm_Off();
				printf("All Alarm Closed\r\n");
			}
			memset(msg.buf,0,sizeof(msg.buf));
		}
		vTaskDelay(pdMS_TO_TICKS(50));
	} 
} 

/* ================= ADC光敏亮度采集任务：遮挡/暗光自动息屏 ================= */
static void app_task_adc(void* pvParameters)
{
    uint16_t adc_val;
    uint32_t now_tick;

    for(;;)
    {
        if (g_alarm_light_on == 1)
        {
            g_dark_start_tick = 0;
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        adc_val = adc_median_average_filter();
        g_adc_light_val = adc_val;
        now_tick = xTaskGetTickCount();

        // 光线充足
        if (adc_val < ADC_LIGHT_DARK_THRESHOLD)
        {
            g_dark_start_tick = 0;
        }
        // 暗光 / 遮挡
        else
        {
            if (g_dark_start_tick == 0)
            {
                g_dark_start_tick = now_tick;
            }

            // ? 只在屏幕还亮着的时候才允许熄屏
            if ((now_tick - g_dark_start_tick) >= ADC_DARK_DELAY_TICK &&
                g_screen_off_flag == 0)
            {
                LCD_SAFE(lcd_display_on(0));
                g_screen_off_flag = 1;   // ? 只在这里置位
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

/* ================= FreeRTOS 异常钩子 ================= */
void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for( ;; );
}
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    (void)pxTask;(void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    for( ;; );
}
void vApplicationIdleHook(void){}
void vApplicationTickHook(void){}
