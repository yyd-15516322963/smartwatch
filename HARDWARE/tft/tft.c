#include "tft.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

uint32_t x_len =TFT_COLUMN_NUMBER;
uint32_t y_len=TFT_LINE_NUMBER;
uint32_t g_lcd_direction=0;

static GPIO_InitTypeDef			GPIO_InitStructure;
static SPI_InitTypeDef  	        SPI_InitStructure;

void SYS_init(unsigned char PLL)
{
}

void Delay_us(unsigned int _us_time)
{
  delay_us(_us_time);
}
void Delay_ms(unsigned int _ms_time)
{
  delay_ms(_ms_time);
}
/*************SPI配置函数*******************
SCL空闲时低电平，第一个上升沿采样
模拟SPI
******************************************/

/**************************SPI模块发送函数************************************************

 *************************************************************************/
void spi1_send_byte(uint8_t byte)
{
#if LCD_SOFT_SPI_ENABLE
  unsigned char counter;

  for (counter = 0; counter < 8; counter++)
  {
    SPI_SCK_0;
    if ((byte & 0x80) == 0)
    {
      SPI_SDA_0;
    }
    else
      SPI_SDA_1;
    byte = byte << 1;
    SPI_SCK_1;
  }
  SPI_SCK_0;	
	
#else
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET)
		;
	SPI_I2S_SendData(SPI1, byte);

	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET)
		;
	SPI_I2S_ReceiveData(SPI1);	
	
#endif
}

void TFT_SEND_CMD(unsigned char o_command)
{
	SPI_CS_0;    
	SPI_DC_0;    
	spi1_send_byte(o_command);
	SPI_DC_1;    
	SPI_CS_1;    
}
void lcd_send_cmd(uint8_t cmd)
{
	TFT_SEND_CMD(cmd);
}

// 向液晶屏写一个8位数据
void TFT_SEND_DATA(unsigned char o_data)
{
	SPI_CS_0;               
	SPI_DC_1;               
	spi1_send_byte(o_data);
	SPI_CS_1;              
}
void lcd_send_data(uint8_t dat)
{
	TFT_SEND_DATA(dat);
}
void LCD_WR_DATA8(u8 dat)
{
	lcd_send_data(dat);
}

void LCD_WR_DATA(u16 dat)
{
	LCD_WR_DATA8(dat >> 8);
	LCD_WR_DATA8(dat);
}
	
void lcd_display_on(uint16_t on)
{
	if(on)
		lcd_send_cmd(0x29);	 	
	else
		lcd_send_cmd(0x28);	 	
}

void lcd_fill(uint32_t x_s, uint32_t y_s, uint32_t x_len, uint32_t y_len,uint32_t color)
{
	uint32_t x, y;
	
	lcd_addr_set(x_s,y_s,x_s+x_len-1,y_s+y_len - 1);

	for (y = y_s; y < y_s+y_len; y++) 
	{
		for (x = x_s; x < x_s+x_len; x++) 
		{

			lcd_send_data(color >> 8);
			lcd_send_data(color);
		}
	}
}

void TFT_clear(uint32_t color)
{
  lcd_fill(0,0,x_len,y_len,color);
}

void TFT_init(void) ////ST7789V2
{
  #if LCD_SOFT_SPI_ENABLE
	
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOE, ENABLE);

	// ?????????
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;      // ?????
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; // ??????????
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;     // ?????????Push Pull???????PMOS????NMOS??
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;   // ???????????????
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;      // ?????
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; // ??????????
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;     // ?????????Push Pull???????PMOS????NMOS??
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;   // ???????????????
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	// ?????????
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;      // ?????
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; // ??????????
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;     // ?????????Push Pull???????PMOS????NMOS??
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;   // ???????????????
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
#else
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

	// SCK=PB3,  MOSI=PB5
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;	 //???ù?????
	GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed; //????????????????????????????????????
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;	 //?????????????????
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; //???????????????
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;	 //?????
	GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed; //????????????????????????????????????
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;	 //?????????????????
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; //???????????????
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7 | GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;	 //?????
	GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed; //????????????????????????????????????
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;	 //?????????????????
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; //???????????????
	GPIO_Init(GPIOG, &GPIO_InitStructure);

	// PB3 PB5?????SPI1???
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource3, GPIO_AF_SPI1);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_SPI1);

	//???SPI1
	SPI_Cmd(SPI1, DISABLE);

	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;//???????
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;//???????????????????8bit
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;//????????w25q128???о????????0????????3????????????3,CPOL=1
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;//????????w25q128???о????????0????????3????????????3,CPHA=1
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;//????????????????????
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;//????????????????SCLK?????=84MHz/4=21MHz
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;//???????????漰????????????????????/?????Чλ????????????MSB
	SPI_Init(SPI1, &SPI_InitStructure);
	
	//???SPI1?????
	SPI_Cmd(SPI1, ENABLE);	

    // ???ó???????
    SPI_RST_1;   // RES??????????λ????
    SPI_DC_1;    // DC???????????????
    SPI_BLK_1;   // BLK???????????????
	
#endif

	//??1.69???????????????????
	SPI_RST_0; //??λ
	delay_ms(100);
	SPI_RST_1;
	delay_ms(100);
	
	lcd_send_cmd(0x11); // Sleep out
	delay_ms(120);	  // Delay 120ms

	lcd_send_cmd(0xB2);
	lcd_send_data(0x0C);
	lcd_send_data(0x0C);
	lcd_send_data(0x00);
	lcd_send_data(0x33);
	lcd_send_data(0x33);

	lcd_send_cmd(0x35);
	lcd_send_data(0x00);

	lcd_send_cmd(0x36);
	if (USE_HORIZONTAL == 0)
		lcd_send_data(0x00);
	else if (USE_HORIZONTAL == 1)
		lcd_send_data(0xC0);
	else if (USE_HORIZONTAL == 2)
		lcd_send_data(0x70);
	else
		lcd_send_data(0xA0);

	lcd_send_cmd(0x3A);
	lcd_send_data(0x05);

	lcd_send_cmd(0xB7);
	lcd_send_data(0x35);

	lcd_send_cmd(0xBB);
	lcd_send_data(0x2D);

	lcd_send_cmd(0xC0);
	lcd_send_data(0x2C);

	lcd_send_cmd(0xC2);
	lcd_send_data(0x01);

	lcd_send_cmd(0xC3);
	lcd_send_data(0x15);

	lcd_send_cmd(0xC4);
	lcd_send_data(0x20);

	lcd_send_cmd(0xC6);
	lcd_send_data(0x0F);

	lcd_send_cmd(0xD0);
	lcd_send_data(0xA4);
	lcd_send_data(0xA1);

	lcd_send_cmd(0xD6);
	lcd_send_data(0xA1);

	lcd_send_cmd(0xE0);
	lcd_send_data(0x70);
	lcd_send_data(0x05);
	lcd_send_data(0x0A);
	lcd_send_data(0x0B);
	lcd_send_data(0x0A);
	lcd_send_data(0x27);
	lcd_send_data(0x2F);
	lcd_send_data(0x44);
	lcd_send_data(0x47);
	lcd_send_data(0x37);
	lcd_send_data(0x14);
	lcd_send_data(0x14);
	lcd_send_data(0x29);
	lcd_send_data(0x2F);

	lcd_send_cmd(0xE1);
	lcd_send_data(0x70);
	lcd_send_data(0x07);
	lcd_send_data(0x0C);
	lcd_send_data(0x08);
	lcd_send_data(0x08);
	lcd_send_data(0x04);
	lcd_send_data(0x2F);
	lcd_send_data(0x33);
	lcd_send_data(0x46);
	lcd_send_data(0x18);
	lcd_send_data(0x15);
	lcd_send_data(0x15);
	lcd_send_data(0x2B);
	lcd_send_data(0x2D);

	lcd_send_cmd(0x21);

	lcd_send_cmd(0x29);

	lcd_send_cmd(0x2C);
	
}

void lcd_addr_set(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2)
{
	if (USE_HORIZONTAL == 0)
	{
		TFT_SEND_CMD(0x2a); //?е??????
		LCD_WR_DATA(x1 );
		LCD_WR_DATA(x2 );
		TFT_SEND_CMD(0x2b); //?е??????
		LCD_WR_DATA(y1 + 20);
		LCD_WR_DATA(y2 + 20);
		TFT_SEND_CMD(0x2c); //??????д
	}
	else if (USE_HORIZONTAL == 1)
	{
		TFT_SEND_CMD(0x2a); //?е??????
		LCD_WR_DATA(x1);
		LCD_WR_DATA(x2);
		TFT_SEND_CMD(0x2b); //?е??????
		LCD_WR_DATA(y1 + 80);
		LCD_WR_DATA(y2 + 80);
		TFT_SEND_CMD(0x2c); //??????д
	}
	else if (USE_HORIZONTAL == 2)
	{
		TFT_SEND_CMD(0x2a); //?е??????
		LCD_WR_DATA(x1);
		LCD_WR_DATA(x2);
		TFT_SEND_CMD(0x2b); //?е??????
		LCD_WR_DATA(y1);
		LCD_WR_DATA(y2);
		TFT_SEND_CMD(0x2c); //??????д
	}
	else
	{
		TFT_SEND_CMD(0x2a); //?е??????
		LCD_WR_DATA(x1 + 80);
		LCD_WR_DATA(x2 + 80);
		TFT_SEND_CMD(0x2b); //?е??????
		LCD_WR_DATA(y1);
		LCD_WR_DATA(y2);
		TFT_SEND_CMD(0x2c); //??????д
	}
}

void lcd_send_data16(uint16_t data)
{
	TFT_SEND_DATA((data >> 8) & 0xFF);
	TFT_SEND_DATA(data & 0xFF);
}

void lcd_show_char(uint32_t x, uint32_t y, uint32_t no, uint32_t fc, uint32_t bc, uint32_t font_size)
{
  int32_t i, j;
  uint8_t tmp;
  uint32_t x_s = x;
  uint32_t y_s = y;
  uint32_t x_e = x_s + font_size - 1;
  uint32_t y_e = y_s + font_size - 1;

  // 设置字体的显示位置
  lcd_addr_set(x_s, y_s, x_e, y_e);

  // 显示字体点阵
  for (i = 0; i < font_size * font_size / 8; i++)
  {

    // 取出字体点阵中的某个字节
    if (font_size == 16)
      tmp = g_font_dot_matrix_16[no][i];
	
    if (font_size == 32)
      tmp = g_font_dot_matrix_32[no][i];	
	
	if (font_size == 80)
		tmp = g_font_dot_matrix_80[no][i];	

    // 显示点（每次显示8个点）
    for (j = 7; j >=0; j--)
    {

      if (tmp & (1 << j)) // 要显示的点
      {
		  
		  lcd_send_data16(fc);
      }
      else // 不需要显示的点，换另外一种颜色
      {
		  lcd_send_data16(bc);
      }
    }
  }
}
//字符编码格式：GB2312，查找字符对应的点阵数据参考代码如下:
int FindFontIndex_tft(const char *target)
{
    int i;
    // 遍历索引表
    for (i = 0; i < ARRAY_SIZE(g_font_dot_matrix_16_index); i++)
    {
        // 匹配汉字或ASC
        // ASC字宽1个字节，值范围（0x00~0x7F）
        // 汉字字宽为2个字节，第一字节（0xA1~0xF7）,第二字节（0xA1~0xFE）
        if (strncmp(g_font_dot_matrix_16_index[i], target, (target[0] > 0x7F) ? 2 : 1) == 0)
        {
            // 返回索引值 
            return i;
        }
    }
    // 没有找到
    return -1;
}
void lcd_show_string(uint32_t x, uint32_t y, const char *str, uint32_t fc, uint32_t bc, uint32_t font_size)
{
	int32_t i=0;
	int32_t index=-1;
	int32_t x_s=x;
	
	while(str[i])
	{
		//查找字符在索引表出现的位置
		index=FindFontIndex_tft(&str[i]);
		
		if(index < 0)
		{
			i++;
			continue;
		}
		
		//寻找到新的字符则显示
		lcd_show_char(x,y,index,fc,bc,font_size);
		
		//对新的字符x坐标位置进行偏移
		if(str[i]>0x7F)//该字符为中文，宽度为字体大小
		{
			i+=2; //当前识别到的字符为中文，需要两个字节来表示
			x+=font_size;
		}
		else//该字符为ASC，宽度为字体大小的一半
		{
			i++;//当前识别到的字符为ASC，需要一个字节来表示
			x+=font_size/2;
		}
		
		if(str[i]>0x7F)
		{
			if(x+font_size>239)
			{
				x=x_s;
				y+=font_size;
			}		
		}
		else
		{
			if((x+font_size/2)>239)
			{
				x=x_s;
				y+=font_size;
			}				
		}

	}

}

void lcd_draw_image(uint32_t x, uint32_t y, uint32_t width, uint32_t height, const uint8_t *image_data)
{
  uint32_t i = 0;
  uint32_t x_s = x;
  uint32_t x_e = x_s + width - 1;
  uint32_t y_s = y;
  uint32_t y_e = y_s + height - 1;
  const uint8_t *p = image_data;

  lcd_addr_set(x_s, y_s, x_e, y_e);

  for (i = 0; i < width * height * 2; i++)
    TFT_SEND_DATA(*p++);
}

void lcd_show_string_fmt(uint32_t x,uint32_t y,uint16_t fc,uint16_t bc,uint32_t font_size,char *str,...)
{
    // 声明可变参数列表变量args
    va_list args;
    // 声明临时缓冲区temp_buf，用于存储格式化后的字符串（大小256可根据需求调整）
    char temp_buf[256]; 
    // 初始化可变参数列表，指定最后一个固定参数为str
    va_start(args, str);
    // 使用vsprintf将格式化字符串输出到临时缓冲区temp_buf
    vsprintf(temp_buf, str, args);  
    // 结束可变参数列表的使用
    va_end(args);
	
	lcd_show_string(x,y,temp_buf,fc,bc,font_size);
}

void lcd_set_direction(uint32_t dir)
{
	g_lcd_direction = dir;
	
	/* 0??*/
	if(dir==0)
	{
		lcd_send_cmd(0x36);
		lcd_send_data(0x00);
		x_len=TFT_LINE_NUMBER;
		x_len=TFT_COLUMN_NUMBER;
	}
	
	/* 90??*/
	if(dir==1)
	{
		lcd_send_cmd(0x36);
		lcd_send_data((1<<6)|(1<<5)|(1<<4));
		x_len=TFT_COLUMN_NUMBER;
		x_len=TFT_LINE_NUMBER;
	
	}	
	
	/* 180??*/
	if(dir==2)
	{
		lcd_send_cmd(0x36);
		lcd_send_data((1<<7)|(1<<6));
		x_len=TFT_LINE_NUMBER;
		x_len=TFT_COLUMN_NUMBER;
	
	}
	
	/* 270??*/
	if(dir==3)
	{
		lcd_send_cmd(0x36);
		lcd_send_data((1<<7)|(0<<6)|(1<<5)|(1<<4));
		x_len=TFT_COLUMN_NUMBER;
		x_len=TFT_LINE_NUMBER;
	
	}		

}

uint32_t lcd_get_direction(void)
{
	return g_lcd_direction;
}
