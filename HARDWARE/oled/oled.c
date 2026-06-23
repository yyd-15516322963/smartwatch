//////////////////////////////////////////////////////////////////////////////////	 
//本程序仅用于学习使用，未经作者许可，不得用于任何商业用途;
//淘晶科技
//店铺地址:http://shop73023976.taobao.com/?spm=2013.1.0.0.M4PqC2
//
//  文件名称   : oled.c
//  版本号     : v2.0
//  作者       : Evk123
//  创建日期   : 2014-0101
//  修改日期   : 
//  功能描述   : 0.69寸OLED 接口显示驱动(STM32F103ZE系列IIC)
//              说明: 
//              ----------------------------------------------------------------
//              GND   电源地
//              VCC   接5V或3.3v电源
//              SCL   接PD6(SCL)
//              SDA   接PD7(SDA)            
//              ----------------------------------------------------------------
//Copyright(C) 淘晶科技 2014/3/16
//All rights reserved
//////////////////////////////////////////////////////////////////////////////////

#include "oled.h"
#include "stdlib.h"
#include "oledfont.h"  	
#include "FontDotMatrix16.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
//#include "delay.h"

#define OLED_SCL_W	PBout(15)
#define OLED_SDA_W	PDout(10)
#define OLED_SDA_R	PDin(10)

extern void delay_ms(uint32_t t);
extern void delay_us(uint32_t t);

//OLED显存
//存放格式如下.
//[0]0 1 2 3 ... 127	
//[1]0 1 2 3 ... 127	
//[2]0 1 2 3 ... 127	
//[3]0 1 2 3 ... 127	
//[4]0 1 2 3 ... 127	
//[5]0 1 2 3 ... 127	
//[6]0 1 2 3 ... 127	
//[7]0 1 2 3 ... 127 			   

/**********************************************
//IIC Start
//功能: 发送IIC起始信号
//时序: SCL=1时，SDA从1变为0
**********************************************/
void IIC_Start(void)
{
	OLED_SCL_W = 1;
	OLED_SDA_W = 1;
	delay_us(1);

	OLED_SDA_W = 0;
	delay_us(1);	
	
	//发送时钟信号也需要延时，保证数据被正确写入到从设备的移位寄存器
	OLED_SCL_W = 0;
	delay_us(1);
}

/**********************************************
//IIC Stop
//功能: 发送IIC停止信号
//时序: SCL=1时，SDA从0变为1
**********************************************/
void IIC_Stop(void)
{
	//发送停止信号
	OLED_SCL_W = 1;
	OLED_SDA_W = 0;
	delay_us(1);

	OLED_SDA_W = 1;
	delay_us(1);
}

/**********************************************
//IIC Wait Acknowledge
//功能: 等待IIC从设备的应答信号
//说明: 读取SDA线上的低电平作为应答信号
**********************************************/
void IIC_Wait_Ack(void)
{
	OLED_SCL_W = 1;		
	delay_us(1);
	
	OLED_SCL_W = 0;		
	delay_us(1);
}

/**********************************************
// IIC Write Byte
//功能: 向IIC总线写入一个字节数据
//参数: IIC_Byte - 要写入的字节数据
//说明: 从高位到低位依次发送
**********************************************/
void Write_IIC_Byte(unsigned char IIC_Byte)
{
	int32_t i;
	
	OLED_SCL_W = 0;
	OLED_SDA_W = 0;
	delay_us(1);	
	
	//从高位到低位发送
	for(i = 7; i >= 0; i--)
	{
		//将IIC_Byte每个bit放到SDA线上的电平
		if(IIC_Byte & (1 << i))
		{
			//设置SDA线为高电平
			OLED_SDA_W = 1;
		}
		else
		{
			//设置SDA线为低电平
			OLED_SDA_W = 0;		
		}
		
		delay_us(1);
		
		OLED_SCL_W = 1;		
		delay_us(1);

		OLED_SCL_W = 0;		
		delay_us(1);		
	}
}

/**********************************************
// IIC Write Command
//功能: 向OLED写入命令字节
//参数: IIC_Command - 命令字节
//说明: 先发送从设备地址(0x78)，再发送命令标志(0x00)，最后发送命令
**********************************************/
void Write_IIC_Command(unsigned char IIC_Command)
{
   IIC_Start();
   Write_IIC_Byte(0x78);            //从设备地址,SA0=0
   IIC_Wait_Ack();	
   Write_IIC_Byte(0x00);			//写入命令
   IIC_Wait_Ack();	
   Write_IIC_Byte(IIC_Command); 
   IIC_Wait_Ack();	
   IIC_Stop();
}

/**********************************************
// IIC Write Data
//功能: 向OLED写入数据字节
//参数: IIC_Data - 数据字节
//说明: 先发送从设备地址(0x78)，再发送数据标志(0x40)，最后发送数据
**********************************************/
void Write_IIC_Data(unsigned char IIC_Data)
{
   IIC_Start();
   Write_IIC_Byte(0x78);			//D/C#=0; R/W#=0
   IIC_Wait_Ack();	
   Write_IIC_Byte(0x40);			//写入数据
   IIC_Wait_Ack();	
   Write_IIC_Byte(IIC_Data);
   IIC_Wait_Ack();	
   IIC_Stop();
}

/**********************************************
// OLED Write Byte
//功能: 向OLED写入字节数据
//参数: dat - 要写入的数据
//      cmd - 命令/数据标志(0=命令,1=数据)
//说明: 根据cmd标志决定写入命令还是数据
**********************************************/
void OLED_WR_Byte(unsigned dat, unsigned cmd)
{
	if(cmd)
	{
		Write_IIC_Data(dat);
	}
	else 
	{
		Write_IIC_Command(dat);
	}
}

/**********************************************
// Fill Picture
//功能: 填充整个OLED屏幕
//参数: fill_Data - 填充数据(0x00=全黑,0xFF=全亮)
//说明: 逐页填充，每页128字节
**********************************************/
void fill_picture(unsigned char fill_Data)
{
	unsigned char m, n;
	for(m = 0; m < 8; m++)
	{
		OLED_WR_Byte(0xb0 + m, 0);		//设置页地址page0-page7
		OLED_WR_Byte(0x00, 0);			//设置低列起始地址
		OLED_WR_Byte(0x10, 0);			//设置高列起始地址
		for(n = 0; n < 128; n++)
		{
			OLED_WR_Byte(fill_Data, 1);
		}
	}
}

/**********************************************
// Delay Functions
//功能: 软件延时函数
//参数: Del_50ms - 50ms延时倍数
//      Del_1ms  - 1ms延时倍数
**********************************************/
void Delay_50ms(unsigned int Del_50ms)
{
	unsigned int m;
	for(; Del_50ms > 0; Del_50ms--)
		for(m = 6245; m > 0; m--);
}

void Delay_1ms(unsigned int Del_1ms)
{
	unsigned char j;
	while(Del_1ms--)
	{	
		for(j = 0; j < 123; j++);
	}
}

//设置位置
/**********************************************
// OLED Set Position
//功能: 设置OLED显示位置
//参数: x - 列地址(0-127)
//      y - 页地址(0-7)
//说明: SSD1306采用页地址模式，每页8行
**********************************************/
void OLED_Set_Pos(unsigned char x, unsigned char y) 
{ 	
	OLED_WR_Byte(0xb0 + y, OLED_CMD);
	OLED_WR_Byte(((x & 0xf0) >> 4) | 0x10, OLED_CMD);
	OLED_WR_Byte(x & 0x0f, OLED_CMD); 
}  

/**********************************************
// OLED Display On
//功能: 开启OLED显示
//说明: 依次发送DCDC配置命令和显示开启命令
**********************************************/
void OLED_Display_On(void)
{
	OLED_WR_Byte(0X8D, OLED_CMD);  //SET DCDC命令
	OLED_WR_Byte(0X14, OLED_CMD);  //DCDC ON
	OLED_WR_Byte(0XAF, OLED_CMD);  //DISPLAY ON
}

/**********************************************
// OLED Display Off
//功能: 关闭OLED显示
//说明: 依次发送DCDC配置命令和显示关闭命令
**********************************************/
void OLED_Display_Off(void)
{
	OLED_WR_Byte(0X8D, OLED_CMD);  //SET DCDC命令
	OLED_WR_Byte(0X10, OLED_CMD);  //DCDC OFF
	OLED_WR_Byte(0XAE, OLED_CMD);  //DISPLAY OFF
}		   			 

/**********************************************
// OLED Clear
//功能: 清屏，使屏幕全黑
//说明: 向所有显存位置写入0x00
**********************************************/
void OLED_Clear(void)  
{  
	u8 i, n;		    
	for(i = 0; i < 8; i++)  
	{  
		OLED_WR_Byte(0xb0 + i, OLED_CMD);    //设置页地址0~7页
		OLED_WR_Byte(0x00, OLED_CMD);        //设置显示位置-列低地址
		OLED_WR_Byte(0x10, OLED_CMD);        //设置显示位置-列高地址   
		for(n = 0; n < 128; n++)
			OLED_WR_Byte(0, OLED_DATA); 
	} 
}

void OLED_Fill(uint8_t x,uint8_t y,uint8_t len,uint8_t dot)
{
	u8 x_s=x;
	u8 x_e=x_s+len;
	
	OLED_Set_Pos(x,y);
	
	for(;x<x_e;x++)
		OLED_WR_Byte(dot,OLED_DATA);
}
/**********************************************
// OLED On
//功能: 点亮整个屏幕
//说明: 向所有显存位置写入0xFF
**********************************************/
void OLED_On(void)  
{  
	u8 i, n;		    
	for(i = 0; i < 8; i++)  
	{  
		OLED_WR_Byte(0xb0 + i, OLED_CMD);    //设置页地址0~7页
		OLED_WR_Byte(0x00, OLED_CMD);        //设置显示位置-列低地址
		OLED_WR_Byte(0x10, OLED_CMD);        //设置显示位置-列高地址   
		for(n = 0; n < 128; n++)
			OLED_WR_Byte(1, OLED_DATA); 
	} 
}

/**********************************************
// OLED Show Character
//功能: 在指定位置显示一个ASCII字符
//参数: x         - 起始列地址(0-127)
//      y         - 起始页地址(0-7)
//      chr       - 要显示的字符
//      Char_Size - 字符大小(12或16)
//说明: 支持8x16和6x8两种字体
**********************************************/
void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 Char_Size)
{      	
	unsigned char c = 0, i = 0;	
	c = chr - ' ';  //得到偏移后的值			
	
	if(x > Max_Column - 1)
	{
		x = 0;
		y = y + 2;
	}
	
	if(Char_Size == 16)
	{
		OLED_Set_Pos(x, y);	
		for(i = 0; i < 8; i++)
			OLED_WR_Byte(F8X16[c * 16 + i], OLED_DATA);
		OLED_Set_Pos(x, y + 1);
		for(i = 0; i < 8; i++)
			OLED_WR_Byte(F8X16[c * 16 + i + 8], OLED_DATA);
	}
	else 
	{	
		OLED_Set_Pos(x, y);
		for(i = 0; i < 6; i++)
			OLED_WR_Byte(F6x8[c][i], OLED_DATA);
	}
}

/**********************************************
// OLED Power
//功能: 计算m的n次方
//参数: m - 底数
//      n - 指数
//返回: m^n的结果
**********************************************/
u32 oled_pow(u8 m, u8 n)
{
	u32 result = 1;	 
	while(n--)
		result *= m;    
	return result;
}				  

/**********************************************
// OLED Show Number
//功能: 在指定位置显示数字
//参数: x     - 起始列地址
//      y     - 起始页地址
//      num   - 要显示的数字(0~4294967295)
//      len   - 数字位数
//      size2 - 字符大小
//说明: 支持前导零抑制
**********************************************/
void OLED_ShowNum(u8 x, u8 y, u32 num, u8 len, u8 size2)
{         	
	u8 t, temp;
	u8 enshow = 0;						   
	
	for(t = 0; t < len; t++)
	{
		temp = (num / oled_pow(10, len - t - 1)) % 10;
		
		if(enshow == 0 && t < (len - 1))
		{
			if(temp == 0)
			{
				OLED_ShowChar(x + (size2 / 2) * t, y, ' ', size2);
				continue;
			}
			else 
				enshow = 1; 
		}
	 	OLED_ShowChar(x + (size2 / 2) * t, y, temp + '0', size2); 
	}
} 

/**********************************************
// OLED Show String
//功能: 在指定位置显示字符串
//参数: x         - 起始列地址
//      y         - 起始页地址
//      chr       - 字符串指针
//      Char_Size - 字符大小
//说明: 支持自动换行
**********************************************/
void OLED_ShowString(u8 x, u8 y, u8 *chr, u8 Char_Size)
{
	unsigned char j = 0;
	
	while (chr[j] != '\0')
	{		
		OLED_ShowChar(x, y, chr[j], Char_Size);
		x += 8;
		
		if(x > 120)
		{
			x = 0;
			y += 2;
		}
		j++;
	}
}

/**********************************************
// OLED Show Chinese
//功能: 在指定位置显示一个16x16汉字
//参数: x  - 起始列地址
//      y  - 起始页地址
//      no - 汉字在字库中的索引
//说明: 使用GB2312编码的16x16点阵字库
**********************************************/
void OLED_ShowCHinese(u8 x, u8 y, u8 no)
{      			    
	u8 t;
	OLED_Set_Pos(x, y);	
    for(t = 0; t < 16; t++)
		OLED_WR_Byte(g_font_dot_matrix_16[no][t], OLED_DATA);

	OLED_Set_Pos(x, y + 1);	
    for(t = 16; t < 32; t++)
		OLED_WR_Byte(g_font_dot_matrix_16[no][t], OLED_DATA);
}

/**********************************************
// Find Font Index
//功能: 在字库索引表中查找字符对应的索引
//参数: target - 要查找的字符指针
//返回: 字符在字库中的索引，未找到返回-1
//说明: 支持ASCII字符(1字节)和GB2312汉字(2字节)
**********************************************/
int FindFontIndex(const char *target)
{
    int i;
    
    // 遍历索引表
    for (i = 0; i < (sizeof(g_font_dot_matrix_16_index) / sizeof(g_font_dot_matrix_16_index[0])); i++)
    {
        // 匹配字符(ASCII或汉字)
        // ASCII字符占1个字节，值范围在0x00~0x7F
        // 汉字字符占2个字节，第一个字节在0xA1~0xF7，第二个字节在0xA1~0xFE
        if (strncmp(g_font_dot_matrix_16_index[i], target, (target[0] > 0x7F) ? 2 : 1) == 0)
        {
            // 返回索引值
            return i;
        }
    }
    
    // 未找到
    return -1;
}

/**********************************************
// OLED Show String (UTF-8/GB2312)
//功能: 在指定位置显示混合字符串(支持中英文)
//参数: x   - 起始列地址
//      y   - 起始页地址
//      str - 字符串指针(UTF-8或GB2312编码)
//说明: 自动识别ASCII字符和汉字，支持自动换行
**********************************************/
void oled_show_string(uint8_t x, uint8_t y, const char *str)
{
    int32_t index = 0;
    uint32_t x_s = x;
    uint32_t i = 0;
    
    const char *p = str;
    
    while(p[i])
    {
        //在字库索引表中查找当前字符的位置
        index = FindFontIndex(&p[i]);
        
        if(index < 0)
        {
            i++;
            continue;
        }
        
        //找到后显示该字符
        OLED_ShowCHinese(x, y, index);
        
        //调整x坐标偏移量
        if(p[i] > 0x7F)  //汉字需要偏移16个像素
        {
            i += 2;
            x += 16;
        }
        else
        {
            i += 1;
            x += 8;
        }
        
        // 判断是否需要换行
        if (x + 16 > 127) 
        {
            x = x_s;  // 回到行首
            y += 2;   // 换行2页
        }		
    }
}

/**********************************************
// OLED Show String Format
//功能: 在指定位置显示格式化字符串
//参数: x   - 起始列地址
//      y   - 起始页地址
//      str - 格式化字符串
//      ... - 可变参数
//说明: 使用vsprintf进行格式化
**********************************************/
void oled_show_string_fmt(uint32_t x, uint32_t y, char *str, ...)
{
    // 创建可变参数列表args
    va_list args;
    // 创建临时缓冲区temp_buf用于存储格式化后的字符串，大小256字节足够一般使用
    char temp_buf[256]; 
    // 初始化可变参数列表，第一个固定参数为str
    va_start(args, str);
    // 使用vsprintf将格式化字符串写入临时缓冲区temp_buf
    vsprintf(temp_buf, str, args);  
    // 结束可变参数列表的使用
    va_end(args);
	
	oled_show_string(x, y, temp_buf);
}

/**********************************************
// OLED Draw BMP
//功能: 在指定区域显示BMP图片
//参数: x0, y0 - 左上角坐标(列,页)
//      x1, y1 - 右下角坐标(列,页)
//      BMP    - 图片数据指针
//说明: 图片数据按页存储，每页8行
**********************************************/
void OLED_DrawBMP(unsigned char x0, unsigned char y0, 
                  unsigned char x1, unsigned char y1, 
                  unsigned char BMP[])
{ 	
    unsigned int j = 0;
    unsigned char x, y;
  
    if(y1 % 8 == 0) 
        y = y1 / 8;      
    else 
        y = y1 / 8 + 1;
    
    for(y = y0; y < y1; y++)
    {
        OLED_Set_Pos(x0, y);
        for(x = x0; x < x1; x++)
        {      
            OLED_WR_Byte(BMP[j++], OLED_DATA);    	
        }
    }
} 

/**********************************************
// OLED Draw Picture (Linux风格)
//功能: 在指定位置显示图片(更简洁的接口)
//参数: x      - 起始列地址(0-127)
//      y      - 起始页地址(0-7)
//      width  - 图片宽度(像素)
//      height - 图片高度(像素)
//      picture - 图片数据指针
//说明: 图片数据按页存储，每页8行，自动计算页数
**********************************************/
void oled_draw_picture(unsigned char x, unsigned char y, 
                       unsigned char width, unsigned char height, 
                       const unsigned char *picture)
{
    unsigned int j = 0;
    unsigned char page, col;
    unsigned char page_count = (height + 7) / 8;
    
    for (page = 0; page < page_count; page++)
    {
        OLED_Set_Pos(x, y + page);
        for (col = 0; col < width; col++)
        {
            OLED_WR_Byte(picture[j++], OLED_DATA);
        }
    }
}

/**********************************************
// OLED Init
//功能: 初始化SSD1306 OLED控制器
//说明: 配置GPIO为输出模式，发送初始化命令序列
//      SCL -> PB15, SDA -> PD10
**********************************************/
void OLED_Init(void)
{ 	
    GPIO_InitTypeDef  GPIO_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOD, ENABLE);
		
    // 配置PB15为SCL
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Low_Speed;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;  //推挽输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);	
	
	// 配置PD10为SDA
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Low_Speed;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;  //推挽输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure);	

	//I2C总线空闲时SCL和SDA都为高电平
	OLED_SCL_W = 1;
	OLED_SDA_W = 1;
	
	delay_ms(200);

	OLED_WR_Byte(0xAE, OLED_CMD);  //--display off
	OLED_WR_Byte(0x00, OLED_CMD);  //---set low column address
	OLED_WR_Byte(0x10, OLED_CMD);  //---set high column address
	OLED_WR_Byte(0x40, OLED_CMD);  //--set start line address  
	OLED_WR_Byte(0xB0, OLED_CMD);  //--set page address
	OLED_WR_Byte(0x81, OLED_CMD);  // contract control
	OLED_WR_Byte(0xFF, OLED_CMD);  //--设置对比度为128   
	OLED_WR_Byte(0xA1, OLED_CMD);  //set segment remap (列地址映射)
	OLED_WR_Byte(0xA6, OLED_CMD);  //--normal / reverse (正常显示)
	OLED_WR_Byte(0xA8, OLED_CMD);  //--set multiplex ratio(1 to 64)
	OLED_WR_Byte(0x3F, OLED_CMD);  //--1/32 duty (扫描行数)
	OLED_WR_Byte(0xC8, OLED_CMD);  //Com scan direction (COM扫描方向)
	OLED_WR_Byte(0xD3, OLED_CMD);  //-set display offset (显示偏移)
	OLED_WR_Byte(0x00, OLED_CMD);  //

	OLED_WR_Byte(0xD5, OLED_CMD);  //set osc division (振荡器分频)
	OLED_WR_Byte(0x80, OLED_CMD);  //

	OLED_WR_Byte(0xD8, OLED_CMD);  //set area color mode off (区域颜色模式)
	OLED_WR_Byte(0x05, OLED_CMD);  //

	OLED_WR_Byte(0xD9, OLED_CMD);  //Set Pre-Charge Period (预充电周期)
	OLED_WR_Byte(0xF1, OLED_CMD);  //

	OLED_WR_Byte(0xDA, OLED_CMD);  //set com pin configuartion (COM引脚配置)
	OLED_WR_Byte(0x12, OLED_CMD);  //

	OLED_WR_Byte(0xDB, OLED_CMD);  //set Vcomh (VCOM电压)
	OLED_WR_Byte(0x30, OLED_CMD);  //

	OLED_WR_Byte(0x8D, OLED_CMD);  //set charge pump enable (电荷泵使能)
	OLED_WR_Byte(0x14, OLED_CMD);  //

	OLED_WR_Byte(0xAF, OLED_CMD);  //--turn on oled panel (开启显示)
}  
