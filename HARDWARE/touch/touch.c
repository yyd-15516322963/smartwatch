#include "touch.h"

/*创建配置结构体*/
static NVIC_InitTypeDef NVIC_InitStructure;
static EXTI_InitTypeDef  EXTI_InitStructure;
static GPIO_InitTypeDef GPIO_InitStructure;

static uint8_t  g_tp_finger_num=0;

void tp_sda_pin_mode(GPIOMode_TypeDef pin_mode)
{
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;		//第9根引脚
	GPIO_InitStructure.GPIO_Mode = pin_mode;	//设置输出/输入模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;	//开漏模式
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//设置IO的速度为100MHz，频率越高性能越好，频率越低，功耗越低
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//不需要上拉电阻
	GPIO_Init(GPIOD, &GPIO_InitStructure);

}
void tp_i2c_start(void)
{
	//保证SDA引脚为输出模式
	tp_sda_pin_mode(GPIO_Mode_OUT);
	
	TP_SCL_W=1;
	TP_SDA_W=1;
	delay_us(1);
	
	TP_SDA_W=0;
	delay_us(1);

	TP_SCL_W=0;
	delay_us(1);
}


void tp_i2c_stop(void)
{
	//保证SDA引脚为输出模式
	tp_sda_pin_mode(GPIO_Mode_OUT);
	
	TP_SCL_W=1;
	TP_SDA_W=0;
	delay_us(1);
	
	TP_SDA_W=1;
	delay_us(1);
}

void tp_i2c_send_byte(uint8_t byte)
{
	int32_t i;
	
	//保证SDA引脚为输出模式
	tp_sda_pin_mode(GPIO_Mode_OUT);
	
	TP_SCL_W=0;
	TP_SDA_W=0;
	delay_us(1);
	
	for(i=7; i>=0; i--)
	{
		if(byte & (1<<i))
			TP_SDA_W=1;
		else
			TP_SDA_W=0;
	
		delay_us(1);
	
		TP_SCL_W=1;
		delay_us(1);
		
		TP_SCL_W=0;
		delay_us(1);		
	}
}

void tp_i2c_ack(uint8_t ack)
{
	//保证SDA引脚为输出模式
	tp_sda_pin_mode(GPIO_Mode_OUT);
	
	TP_SCL_W=0;
	TP_SDA_W=0;
	delay_us(1);
	
	if(ack)
		TP_SDA_W=1;
	else
		TP_SDA_W=0;

	delay_us(1);

	TP_SCL_W=1;
	delay_us(1);
	
	TP_SCL_W=0;
	delay_us(1);		
}

uint8_t tp_i2c_wait_ack(void)
{
	uint8_t ack;
	//保证SDA引脚为输入模式
	tp_sda_pin_mode(GPIO_Mode_IN);

	//紧接着第九个时钟周期，将SCL拉高
	TP_SCL_W=1;
	delay_us(1);
	
	if(TP_SDA_R)
		ack=1;
	else
		ack=0;
	
	//继续保持占用总线
	TP_SCL_W=0;
	delay_us(1);

	return ack;
}

uint8_t tp_i2c_recv_byte(void)
{

	uint8_t d=0;
	int32_t i;
	
	
	//保证SDA引脚为输入模式
	tp_sda_pin_mode(GPIO_Mode_IN);

	
	for(i=7; i>=0; i--)
	{
		TP_SCL_W=1;
		delay_us(1);
		
		if(TP_SDA_R)
			d|=1<<i;
		
		//继续保持占用总线
		TP_SCL_W=0;
		delay_us(1);	
	}

	return d;
}




void tp_lowlevel_init(void)
{
#if TP_PIN_DEF == TP_PIN_DEF_1	
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);
	
	//使能系统配置的硬件时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_4|GPIO_Pin_14;		//第0 4 14根引脚
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;				//设置输出模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;				//开漏模式
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;			//设置IO的速度为100MHz，频率越高性能越好，频率越低，功耗越低
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;			//不需要上拉电阻
	GPIO_Init(GPIOD, &GPIO_InitStructure);


	//只要是输出模式，有初始电平状态
	TP_SCL_W=1;
	TP_SDA_W=1;	
	
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_100MHz;
	GPIO_Init(GPIOF , &GPIO_InitStructure);
	
	/* 配置外部中断12相关的参数 */
	EXTI_InitStructure.EXTI_Line = EXTI_Line12;	//外部中断12
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;//中断
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  //检测按键的按下，使用下降沿触发
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;//使能
	EXTI_Init(&EXTI_InitStructure);
	
	/* NVIC打开外部中断12的通道，并为它配置优先级 */
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;//中断号
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY;//抢占优先级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;//响应(子)优先级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//打开该通道
	NVIC_Init(&NVIC_InitStructure);	
	//初始化
	TP_RST=1;
	
#endif


#if TP_PIN_DEF == TP_PIN_DEF_2

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	
	//使能系统配置的硬件时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7;		//第6 7根引脚
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;				//设置输出模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;				//开漏模式
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;			//设置IO的速度为100MHz，频率越高性能越好，频率越低，功耗越低
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;			//不需要上拉电阻
	GPIO_Init(GPIOD, &GPIO_InitStructure);


	//只要是输出模式，有初始电平状态
	TP_SCL_W=1;
	TP_SDA_W=1;	

	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_100MHz;
	GPIO_Init(GPIOC , &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_100MHz;
	GPIO_Init(GPIOC , &GPIO_InitStructure);
	
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource8);
	
	/* 配置外部中断8相关的参数 */
	EXTI_InitStructure.EXTI_Line = EXTI_Line8;	//外部中断8
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;//中断
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  //检测按键的按下，使用下降沿触发
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;//使能
	EXTI_Init(&EXTI_InitStructure);
	
	/* NVIC打开外部中断0的通道，并为它配置优先级 */
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;//中断号
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY;//抢占优先级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;//响应(子)优先级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//打开该通道
	NVIC_Init(&NVIC_InitStructure);
	
	//初始化
	TP_RST=1;
	
#endif	
}

void tp_send_byte(uint8_t addr,uint8_t* data)
{
	tp_i2c_start();
	tp_i2c_send_byte(0x2A);tp_i2c_wait_ack();
	tp_i2c_send_byte(addr);tp_i2c_wait_ack();
	tp_i2c_send_byte(*data);tp_i2c_wait_ack();
	tp_i2c_stop();
}

void tp_recv_byte(uint8_t addr,uint8_t* data)
{
	tp_i2c_start();
	tp_i2c_send_byte(0x2A);tp_i2c_wait_ack();
	tp_i2c_send_byte(addr);tp_i2c_wait_ack();
	tp_i2c_start();
	tp_i2c_send_byte(0x2B);tp_i2c_wait_ack();
	*data=tp_i2c_recv_byte();
	tp_i2c_ack(1);
	tp_i2c_stop();
}

void tp_recv(uint8_t addr,uint8_t* data,uint32_t len)
{
	uint8_t *p=data;
	
	tp_i2c_start();
	tp_i2c_send_byte(0x2A);tp_i2c_wait_ack();
	tp_i2c_send_byte(addr);tp_i2c_wait_ack();
	tp_i2c_start();
	tp_i2c_send_byte(0x2B);tp_i2c_wait_ack();
	
	len=len-1;
	
	while(len--)
	{
		*p=tp_i2c_recv_byte();
		tp_i2c_ack(0);
		p++;
	
	}
	
	*p=tp_i2c_recv_byte();
	tp_i2c_ack(1);
	
	tp_i2c_stop();
}

void tp_reset(void)
{
	TP_RST=0;
	delay_ms(10);
	
	TP_RST=1;
	delay_ms(60);
}

void tp_init(void)
{
	uint8_t ChipID=0;
	uint8_t FwVersion=0;
	

	tp_lowlevel_init();
	
	tp_reset();//芯片上电初始化
	tp_recv_byte(0xa7,&ChipID);
	tp_recv_byte(0xa9,&FwVersion);
	
	printf("ChipID:%02X\r\n",ChipID);
	
	/*	
		芯片 ID
		CST716 : 0x20
		CST816S : 0xB4
		CST816T : 0xB5
		CST816D : 0xB6	
	
	*/
	
	printf("FwVersion:%02X\r\n",FwVersion);
}

uint8_t tp_finger_num_get(void)
{
	return g_tp_finger_num;
}

uint8_t tp_read(uint16_t *screen_x,uint16_t *screen_y)
{
	uint8_t buf[7];
	uint16_t x=0,y=0,tmp;
	
	tp_recv(0,buf,7);
	
	x=(uint16_t)((buf[3]&0x0F)<<8)|buf[4];
	y=(uint16_t)((buf[5]&0x0F)<<8)|buf[6];	

	g_tp_finger_num = buf[2];

	if((x<x_len) && (y<y_len))
	{
		
		if(lcd_get_direction()==1)
		{
		   tmp= x;
			x = y;
			y = y_len-tmp;		
		}			
		
		if(lcd_get_direction()==2)
		{
			x = x_len-x;
			y = y_len-y;		
		}	

		if(lcd_get_direction()==3)
		{
		   tmp= y;
			y = x;
			x = x_len-tmp;					
		}			
		
		*screen_x=x;
		*screen_y=y;
		
		/*
			0x00：无手势
			0x01：下滑
			0x02：上滑
			0x03：左滑
			0x04：右滑
			0x05：单击
			0x0B：双击
			0x0C：长按		
		*/

		return buf[1];
	}  	
	
	return 0;
}

#if TP_PIN_DEF == TP_PIN_DEF_1

void EXTI15_10_IRQHandler(void)
{
	uint32_t ulReturn;

	BaseType_t  xHigherPriorityTaskWoken = pdFALSE;	

	/* 进入临界段，临界段可以嵌套 */
	ulReturn = taskENTER_CRITICAL_FROM_ISR();	
	
	//检测标志位
	if(EXTI_GetITStatus(EXTI_Line12) == SET)
	{
		xEventGroupSetBitsFromISR(g_event_group,EVENT_GROUP_TP,&xHigherPriorityTaskWoken);
		
		EXTI_ClearITPendingBit(EXTI_Line12);
	}

	/* 若接收消息队列任务的优先级高于当前运行的任务，则退出中断后立即进行任务切换，执行前者;
	   否则等待下一个时钟节拍才进行任务切换
	*/
	if(xHigherPriorityTaskWoken)
	{
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}	

	/* 退出临界段 */
	taskEXIT_CRITICAL_FROM_ISR( ulReturn );
}
#endif

extern EventGroupHandle_t g_evt_group;

#if TP_PIN_DEF == TP_PIN_DEF_2
void EXTI9_5_IRQHandler(void)
{
	uint32_t ulReturn;

	BaseType_t  xHigherPriorityTaskWoken = pdFALSE;	

	/* 进入临界段，临界段可以嵌套 */
	ulReturn = taskENTER_CRITICAL_FROM_ISR();	
	
	//检测标志位
	if(EXTI_GetITStatus(EXTI_Line8) == SET)
	{
		xEventGroupSetBitsFromISR(g_evt_group,EVENT_GROUP_TP,&xHigherPriorityTaskWoken);
		
		EXTI_ClearITPendingBit(EXTI_Line8);
	}

	/* 若接收消息队列任务的优先级高于当前运行的任务，则退出中断后立即进行任务切换，执行前者;
	   否则等待下一个时钟节拍才进行任务切换
	*/
	if(xHigherPriorityTaskWoken)
	{
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}	

	/* 退出临界段 */
	taskEXIT_CRITICAL_FROM_ISR( ulReturn );
}
#endif
