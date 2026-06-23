#include "mht11.h"

extern void delay_ms(uint32_t t);
extern void delay_us(uint32_t t);

void dht11_set_io(GPIOMode_TypeDef mode)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	//1.开启时钟
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG,ENABLE);
	
	//PG9
	GPIO_InitStructure.GPIO_Mode = mode;//
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;//PG9
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//无上下拉
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;//输出速度 低
	GPIO_Init(GPIOG,&GPIO_InitStructure);
}

u8 DHT11_Check(void)
{
	u32 retry = 0;
	
	// 输入模式发送18ms低电平
	dht11_set_io(GPIO_Mode_OUT);
	DQ_OUT = 0;
	delay_ms(20);
	DQ_OUT = 1;
	
	// 等待20-40ms
	delay_us(30);
	
	// 输入模式
	dht11_set_io(GPIO_Mode_IN);
	// 模块是否拉低响应
	while(DQ_IN && retry < 100)
	{
		retry++;
		delay_us(1);
	}
	
	if(retry >= 100)
		return 1;
	
	// 80ms后拉高
	while(!DQ_IN && retry < 100)
	{
		retry++;
		delay_us(1);
	}
	
	if(retry >= 100)
		return 1;
	
	return 0;
}

u8 dht11_read_bit(void)
{
	u32 retry = 0;
	
	// 等待拉低
	while(DQ_IN && retry < 100)
	{
		retry++;
		delay_us(1);
	}
	if(retry >= 100)
		return 100;
	
	retry = 0;
	
	// 等待拉高
	while(!DQ_IN && retry < 100)
	{
		retry++;
		delay_us(1);
	}
	if(retry >= 100)
		return 100;
	
	delay_us(40);
	return DQ_IN;
}

u8 dht11_read_byte(void)
{
	u8 i,dat = 0;
	
	for(i = 0;i<8;i++)
	{
		dat |=  dht11_read_bit() << (7-i);
	}
	return dat;
}

u8 dht11_read_data(uint32_t *temp,uint32_t *humi)
{
	u8 i,buf[5];
	
	if(DHT11_Check() == 0)
	{
		// 读40bit（5字节）数据
		for(i = 0;i<5;i++)
		{
			buf[i] = dht11_read_byte();
		}
		
		// 判断校验和
		if(((buf[0]+buf[1]+buf[2]+buf[3])&0xff) == buf[4])
		{
			*temp=buf[2];
			*humi=buf[0];
			return 0;
		}
	}
	return 1;
}
