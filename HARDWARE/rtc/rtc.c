#include "rtc.h"
#include "sys.h"

void rtc_init(void)
{
	RTC_InitTypeDef   			RTC_InitStructure;
	RTC_DateTypeDef 			RTC_DateStructure;
	RTC_TimeTypeDef 			RTC_TimeStructure;	
	EXTI_InitTypeDef   			EXTI_InitStructure;
	NVIC_InitTypeDef   		 	NVIC_InitStructure;
	
	/* Enable the PWR clock ,使能电源时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

	/* Allow access to RTC ，允许访问RTC*/
	PWR_BackupAccessCmd(ENABLE);

	/* 使能LSI*/
	RCC_LSICmd(ENABLE);
	
	/* 检查该LSI是否有效*/  
	while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);

	/* 选择LSI作为RTC的硬件时钟源*/
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
	
	/* Enable the RTC Clock ，使能RTC时钟*/
	RCC_RTCCLKCmd(ENABLE);

	/* Wait for RTC APB registers synchronisation ，等待RTC相关寄存器就绪*/
	RTC_WaitForSynchro();	
	
	//CK_SPRE=32KHz/(127+1)/(249+1)=1Hz
	RTC_InitStructure.RTC_AsynchPrediv = 0x7F;				//异步分频系数
	RTC_InitStructure.RTC_SynchPrediv = 0xF9;				//同步分频系数
	RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;	//24小时格式
	RTC_Init(&RTC_InitStructure);	
	
	//读取备份寄存器0的值，若读到的值不等于0x1234，则需要重置RTC的日期与时间
	if(RTC_ReadBackupRegister(RTC_BKP_DR0)!=0x123456)
	{
		RTC_DateStructure.RTC_Year = 26;
		RTC_DateStructure.RTC_Month = 6;
		RTC_DateStructure.RTC_Date = 3;
		RTC_DateStructure.RTC_WeekDay = 3;
		RTC_SetDate(RTC_Format_BIN, &RTC_DateStructure);	
		
		
		/* Set the time to 23h 59mn 55s PM  */
		RTC_TimeStructure.RTC_H12     = RTC_H12_PM;
		RTC_TimeStructure.RTC_Hours   = 20;
		RTC_TimeStructure.RTC_Minutes = 00;
		RTC_TimeStructure.RTC_Seconds = 00; 
		RTC_SetTime(RTC_Format_BIN, &RTC_TimeStructure); 	
		
		//往备份寄存器0写入0x1234（该值自定义即可，无固定）
		RTC_WriteBackupRegister(RTC_BKP_DR0,0x123456);	
	}

	
	//关闭唤醒功能
	RTC_WakeUpCmd(DISABLE);
	
	//为唤醒功能选择RTC配置好的时钟源
	RTC_WakeUpClockConfig(RTC_WakeUpClock_CK_SPRE_16bits);
	
	//设置唤醒计数值为自动重载，写入值默认是0
	RTC_SetWakeUpCounter(0);
	
	//清除RTC唤醒中断标志
	RTC_ClearITPendingBit(RTC_IT_WUT);
	
	//使能RTC唤醒中断
	RTC_ITConfig(RTC_IT_WUT, ENABLE);

	//使能唤醒功能
	RTC_WakeUpCmd(ENABLE);

	/* Configure EXTI Line22，配置外部中断控制线22 */
	EXTI_InitStructure.EXTI_Line = EXTI_Line22;			//当前使用外部中断控制线22
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;		//中断模式
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;		//上升沿触发中断 
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;			//使能外部中断控制线22
	EXTI_Init(&EXTI_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = RTC_WKUP_IRQn;		//允许RTC唤醒中断触发
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x03;	//抢占优先级为0x3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x03;		//响应优先级为0x3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//使能
	NVIC_Init(&NVIC_InitStructure);	
	
	// RTC闹钟A中断配置
	RTC_ITConfig(RTC_IT_ALRA, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = RTC_Alarm_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	// ========== 新增：RTC闹钟A EXTI17中断配置 ==========
	EXTI_InitTypeDef EXTI_Alarm_Init;
	EXTI_Alarm_Init.EXTI_Line = EXTI_Line17;
	EXTI_Alarm_Init.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_Alarm_Init.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_Alarm_Init.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_Alarm_Init);

}

//void set_time(uint8_t hour,uint8_t minutes,uint8_t second)
//{
//	RTC_TimeTypeDef RTC_TimeStructure;
//	
//	PWR_BackupAccessCmd(ENABLE);
//	RCC_RTCCLKCmd(ENABLE);
//	
//	//设置时间和日期
//	RTC_TimeStructure.RTC_H12 = RTC_H12_AM;
//	RTC_TimeStructure.RTC_Hours = hour;
//	RTC_TimeStructure.RTC_Minutes = minutes;
//	RTC_TimeStructure.RTC_Seconds = second;
//	RTC_SetTime(RTC_Format_BIN,&RTC_TimeStructure);
//}

//void set_date(uint8_t year,uint8_t month,uint8_t date,uint8_t weekday)
//{
//	RTC_DateTypeDef RTC_DateStructure;
//	
//	PWR_BackupAccessCmd(ENABLE);
//	RCC_RTCCLKCmd(ENABLE);
//		
//	RTC_DateStructure.RTC_Year = year;//年
//	RTC_DateStructure.RTC_Month = month;//月
//	RTC_DateStructure.RTC_Date = date;//日
//	RTC_DateStructure.RTC_WeekDay = weekday;//星期
//	RTC_SetDate(RTC_Format_BIN,&RTC_DateStructure);
//}

extern volatile uint32_t g_rtc_wakeup_event;

void RTC_WKUP_IRQHandler(void)
{
	if(RTC_GetITStatus(RTC_IT_WUT) != RESET)
	{
		g_rtc_wakeup_event=1;
		RTC_ClearITPendingBit(RTC_IT_WUT);
		EXTI_ClearITPendingBit(EXTI_Line22);
	} 
}

void RTC_Alarm_IRQHandler(void)
{
    if(RTC_GetITStatus(RTC_IT_ALRA) != RESET)
    {
        // 闹钟到达，点亮LED
        PFout(9) = 0;
        PFout(10) = 0;
        PEout(13) = 0;
        PEout(14) = 0;

        // 1. 清除RTC闹钟中断标志
        RTC_ClearITPendingBit(RTC_IT_ALRA);
        // 2. 必须清除EXTI17挂起标志（缺失会导致中断卡死）
        EXTI_ClearITPendingBit(EXTI_Line17);

        // 3. 重新开启闹钟，支持每日重复触发
        RTC_AlarmCmd(RTC_Alarm_A, DISABLE);
        RTC_AlarmCmd(RTC_Alarm_A, ENABLE);
    }
}
