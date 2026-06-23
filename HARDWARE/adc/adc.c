#include "adc.h"

#define ADC_SAMPLE_CNT 100   // 采样总数
#define REMOVE_CNT     10    // 两端各去掉的数量


void adc_init()
{
	GPIO_InitTypeDef			GPIO_InitStructure;
	ADC_InitTypeDef       		ADC_InitStructure;
	ADC_CommonInitTypeDef 		ADC_CommonInitStructure;
	
	// 使能时钟
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3,ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Low_Speed;
	GPIO_Init(GPIOF,&GPIO_InitStructure);
	
	//配置ADC（ADC1~ADC3）通用参数
	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;// 一个ADC工作
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;// 不使用DMA
	ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div8; // ADC硬件时钟频率84/8=10.5MHz
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
	ADC_CommonInit(&ADC_CommonInitStructure);
	
	// 单独配置ADC1
	ADC_InitStructure.ADC_Resolution = ADC_Resolution_8b;	//输出结果为8bit的分辨率
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;//DISABLE,只扫描（转换）一个通道；ENABLE，扫描多个通道
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;//ENABLE，ADC会一直连续工作转化；DISABLE，只转换一次。
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;//决定要不要外部触发（触发沿：上升沿 / 下降沿 / 不触发）
	//ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;//决定用哪个定时器事件来触发
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;//右对齐存储
	ADC_InitStructure.ADC_NbrOfConversion = 1;//当前转换通道数量为1
	ADC_Init(ADC3, &ADC_InitStructure);
	
	//指定ADC1对通道5进行转换，转换排序为1，采样点时间为3个ADC硬件时钟（采样时间越长，数据越准确）
	ADC_RegularChannelConfig(ADC3,ADC_Channel_5,1,ADC_SampleTime_3Cycles);
	
	ADC_Cmd(ADC3,ENABLE);
}

uint16_t adc_median_average_filter(void)
{
	int i,j;
	uint16_t temp;
	uint32_t adc_voltage_all = 0;
	uint16_t data_buf[100] = {0};
	
	for(i=0;i<100;i++)
	{
		//启动ADC1转换
		ADC_SoftwareStartConv(ADC3);
		
		//等待转换完毕
		while(ADC_GetFlagStatus(ADC3,ADC_FLAG_EOC)==RESET);
		data_buf[i] = ADC_GetConversionValue(ADC3);//转换结果值的范围为0~255
	}
	
	// 冒泡排序
	for(i=0;i<ADC_SAMPLE_CNT;i++)
	{
		for(j=0;j<ADC_SAMPLE_CNT-1-i;j++)
		{
			if(data_buf[j]>data_buf[j+1])
			{
				temp = data_buf[j];
                data_buf[j] = data_buf[j + 1];
                data_buf[j + 1] = temp;
			}
		}
	}
	
	// 去掉前后各十个
	for(i=REMOVE_CNT;i<ADC_SAMPLE_CNT-REMOVE_CNT;i++)
	{
		adc_voltage_all += data_buf[i];
	}
	
	return adc_voltage_all / (ADC_SAMPLE_CNT-REMOVE_CNT*2);
}


