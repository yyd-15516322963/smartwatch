#include "ble.h"

static	GPIO_InitTypeDef    GPIO_InitStructure;
static	USART_InitTypeDef  USART_InitStructure;
static	NVIC_InitTypeDef   NVIC_InitStructure;
static	EXTI_InitTypeDef   EXTI_InitStructure;

void ble_init(uint32_t baud)
{

	
	//端口B硬件时钟打开
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB|RCC_AHB1Periph_GPIOE,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	
	//串口3硬件时钟打开
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);
	
	//配置PB10 PB11为AF模式（复用功能）
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10 | GPIO_Pin_11; 	//9 10号引脚
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;//输出模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;//高速，速度越高，响应越快，但是功耗会更高
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;//不使能上下拉电阻
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	
	//由于引脚支持很多功能，需要指定该引脚的功能，当前要制定支持USART3
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource10,GPIO_AF_USART3);
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource11,GPIO_AF_USART3);	
	
	//配置USART3相关参数：波特率、数据位、停止位、校验位
	USART_InitStructure.USART_BaudRate = baud;					//波特率，就是通信的速度
	USART_InitStructure.USART_WordLength = USART_WordLength_8b; //8位数据位
	USART_InitStructure.USART_StopBits = USART_StopBits_1;		//1个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;			//不需要校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//硬件流控制功能不需要
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;//串口允许发送和接收数据
	USART_Init(USART3, &USART_InitStructure);
	
	//配置中断触发方式，接收到一个字节，就通知CPU处理
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
	
	//NVIC配置其的优先级
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;			//中断号
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;	//抢占优先级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 5;			//响应优先级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//打开通道给NVIC管理
	NVIC_Init(&NVIC_InitStructure);
	
	//使能USART3工作
	USART_Cmd(USART3, ENABLE);
}


extern QueueHandle_t g_queue_usart;
#define USART_RX_MAX 128
typedef struct
{
    uint8_t len;
    uint8_t buf[USART_RX_MAX];
} UsartMsg_t;

void USART3_IRQHandler(void)
{
    uint8_t d;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    static UsartMsg_t msg;

    if (USART_GetITStatus(USART3, USART_IT_RXNE))
    {
        d = USART_ReceiveData(USART3);

        if (msg.len < USART_RX_MAX - 1)
        {
            msg.buf[msg.len++] = d;
        }

        if (d == '#' || msg.len >= USART_RX_MAX - 1)
        {
            msg.buf[msg.len] = 0;   // 字符串结束符
            xQueueSendFromISR(g_queue_usart, &msg, &xHigherPriorityTaskWoken);
            msg.len = 0;
            memset(msg.buf, 0, sizeof(msg.buf));
        }

        USART_ClearITPendingBit(USART3, USART_IT_RXNE);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

