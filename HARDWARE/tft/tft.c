#include "tft.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

// 屏幕宽、高全局变量
uint32_t x_len = TFT_COLUMN_NUMBER;
uint32_t y_len = TFT_LINE_NUMBER;
// 屏幕显示方向 0:0° 1:90° 2:180° 3:270°
uint32_t g_lcd_direction = 0;

// GPIO初始化结构体
static GPIO_InitTypeDef GPIO_InitStructure;
// SPI初始化结构体
static SPI_InitTypeDef SPI_InitStructure;

/**
 * @brief 系统时钟初始化(空函数，外部实现delay_us/delay_ms)
 * @param PLL PLL倍频参数
 */
void SYS_init(unsigned char PLL)
{
}

/**
 * @brief 微秒级延时封装
 * @param _us_time 延时微秒数
 */
void Delay_us(unsigned int _us_time)
{
  delay_us(_us_time);
}

/**
 * @brief 毫秒级延时封装
 * @param _ms_time 延时毫秒数
 */
void Delay_ms(unsigned int _ms_time)
{
  delay_ms(_ms_time);
}

/**
 * @brief SPI单字节发送函数
 * @param byte 需要发送的8位数据
 * @note 两种模式：软件模拟SPI / 硬件SPI1
 * 软件SPI时序：SCL空闲低，上升沿采样，高位先行MSB
 */
void spi1_send_byte(uint8_t byte)
{
#if LCD_SOFT_SPI_ENABLE
  unsigned char counter;
  // 循环发送8bit，高位先发
  for (counter = 0; counter < 8; counter++)
  {
    SPI_SCK_0;                // SCK拉低准备输出
    if ((byte & 0x80) == 0)   // 取出当前最高位
    {
      SPI_SDA_0;
    }
    else
      SPI_SDA_1;
    byte = byte << 1;         // 左移准备下一位
    SPI_SCK_1;                // SCK拉高，锁存数据
  }
  SPI_SCK_0;                  // 发送完成，SCK恢复低电平
	
#else
  // 硬件SPI1：等待发送缓冲区为空
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET)
		;
	SPI_I2S_SendData(SPI1, byte);
  // 等待接收完成并读取空数据(硬件SPI收发同步，必须读走接收寄存器)
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET)
		;
	SPI_I2S_ReceiveData(SPI1);	
	
#endif
}

/**
 * @brief TFT发送命令
 * @param o_command ST7789寄存器命令码
 * 时序：CS拉低 -> DC置0(命令) -> 发字节 -> DC置1 -> CS拉高
 */
void TFT_SEND_CMD(unsigned char o_command)
{
	SPI_CS_0;    
	SPI_DC_0;    
	spi1_send_byte(o_command);
	SPI_DC_1;    
	SPI_CS_1;    
}

/**
 * @brief 兼容层：发送LCD命令
 * @param cmd 命令码
 */
void lcd_send_cmd(uint8_t cmd)
{
	TFT_SEND_CMD(cmd);
}

/**
 * @brief TFT发送8位显示数据
 * @param o_data 像素/配置参数数据
 * 时序：CS拉低 -> DC置1(数据) -> 发字节 -> CS拉高
 */
void TFT_SEND_DATA(unsigned char o_data)
{
	SPI_CS_0;               
	SPI_DC_1;               
	spi1_send_byte(o_data);
	SPI_CS_1;              
}

/**
 * @brief 兼容层：发送单字节数据
 * @param dat 单字节数据
 */
void lcd_send_data(uint8_t dat)
{
	TFT_SEND_DATA(dat);
}

/**
 * @brief 底层写8位数据接口
 * @param dat 8位数据
 */
void LCD_WR_DATA8(u8 dat)
{
	lcd_send_data(dat);
}

/**
 * @brief 写16位RGB565像素数据(高字节先发)
 * @param dat 16位RGB颜色值
 */
void LCD_WR_DATA(u16 dat)
{
	LCD_WR_DATA8(dat >> 8);   // 先高8位
	LCD_WR_DATA8(dat);        // 后低8位
}

/**
 * @brief 屏幕显示开关
 * @param on 1开启显示(0x29)，0关闭显示(0x28)
 */
void lcd_display_on(uint16_t on)
{
	if(on)
		lcd_send_cmd(0x29);	 	
	else
		lcd_send_cmd(0x28);	 	
}

/**
 * @brief 矩形区域填充纯色
 * @param x_s 起始X坐标
 * @param y_s 起始Y坐标
 * @param x_len 填充宽度
 * @param y_len 填充高度
 * @param color 16位RGB565填充颜色
 */
void lcd_fill(uint32_t x_s, uint32_t y_s, uint32_t x_len, uint32_t y_len,uint32_t color)
{
	uint32_t x, y;
	// 设置填充窗口坐标
	lcd_addr_set(x_s,y_s,x_s+x_len-1,y_s+y_len - 1);
	// 逐行逐点写入颜色
	for (y = y_s; y < y_s+y_len; y++) 
	{
		for (x = x_s; x < x_s+x_len; x++) 
		{
			lcd_send_data(color >> 8);
			lcd_send_data(color);
		}
	}
}

/**
 * @brief 全屏清屏
 * @param color 清屏颜色RGB565
 */
void TFT_clear(uint32_t color)
{
  lcd_fill(0,0,x_len,y_len,color);
}

/**
 * @brief ST7789V2屏幕初始化
 * 包含GPIO/SPI硬件初始化 + ST7789寄存器上电配置
 */
void TFT_init(void)
{
#if LCD_SOFT_SPI_ENABLE
	// 软件SPI模式：开启GPIOB/GPIOD/GPIOE时钟
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOE, ENABLE);

	// PD8 PD10：SPI_CS、SPI_DC推挽输出
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;      // 通用输出模式
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; // IO速度100M
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;     // 推挽输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;   // 无上下拉
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	// PE12 PE14：SPI_SCK、SPI_SDA推挽输出
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	// PB15：复位脚RST推挽输出
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
#else
	// 硬件SPI1模式
	// 开启GPIO时钟：PB(SPI)、GPIOG(控制脚)、SPI1外设时钟
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

	// PB3 SCK / PB5 MOSI 复用功能SPI1
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;	     // 复用功能模式
	GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;    // 高速IO
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;	     // 推挽输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;    // 无上下拉
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	// PB4 NSS软件片选推挽输出
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	// PG6 PG7 PG8：DC、RST、BLK背光控制脚
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7 | GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOG, &GPIO_InitStructure);

	// PB3 PB5映射为SPI1复用功能
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource3, GPIO_AF_SPI1);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_SPI1);

	// 先关闭SPI1再配置参数
	SPI_Cmd(SPI1, DISABLE);

	// SPI1主模式配置
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex; // 双线全双工
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;                      // 主机模式
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;                  // 8bit数据宽度
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;                        // 空闲时钟高电平
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;                       // 第二个边沿采样
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;                          // 软件管理片选
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4; // 分频4，APB2=84M→SCLK=21M
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;                 // 高位先传输
	SPI_Init(SPI1, &SPI_InitStructure);
	
	// 使能SPI1外设
	SPI_Cmd(SPI1, ENABLE);	

    // 初始化控制脚默认电平
    SPI_RST_1;   // 复位释放
    SPI_DC_1;    // 默认数据模式
    SPI_BLK_1;   // 背光打开
	
#endif

	// ST7789硬件复位时序
	SPI_RST_0; // 拉低复位
	Delay_ms(100);
	SPI_RST_1; // 释放复位
	Delay_ms(100);
	
	lcd_send_cmd(0x11); // 退出睡眠模式
	Delay_ms(120);	  // 等待内部电源稳定

	// 帧速率控制
	lcd_send_cmd(0xB2);
	lcd_send_data(0x0C);
	lcd_send_data(0x0C);
	lcd_send_data(0x00);
	lcd_send_data(0x33);
	lcd_send_data(0x33);

	lcd_send_cmd(0x35); // 开启TE信号
	lcd_send_data(0x00);

	lcd_send_cmd(0x36); // 内存访问控制(旋转/镜像配置)
	if (USE_HORIZONTAL == 0)
		lcd_send_data(0x00);
	else if (USE_HORIZONTAL == 1)
		lcd_send_data(0xC0);
	else if (USE_HORIZONTAL == 2)
		lcd_send_data(0x70);
	else
		lcd_send_data(0xA0);

	lcd_send_cmd(0x3A); // 像素格式设置
	lcd_send_data(0x05); // 16bit RGB565

	lcd_send_cmd(0xB7); // 门驱动电压
	lcd_send_data(0x35);

	lcd_send_cmd(0xBB); // VCOM电压设置
	lcd_send_data(0x2D);

	lcd_send_cmd(0xC0); // LCM控制
	lcd_send_data(0x2C);

	lcd_send_cmd(0xC2); // VDV、VRH控制
	lcd_send_data(0x01);

	lcd_send_cmd(0xC3); // VGH电压
	lcd_send_data(0x15);

	lcd_send_cmd(0xC4); // VGL电压
	lcd_send_data(0x20);

	lcd_send_cmd(0xC6); // 帧频控制
	lcd_send_data(0x0F);

	lcd_send_cmd(0xD0); // 电源设置A
	lcd_send_data(0xA4);
	lcd_send_data(0xA1);

	lcd_send_cmd(0xD6); // 电源设置B
	lcd_send_data(0xA1);

	// Gamma正极性校正
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

	// Gamma负极性校正
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

	lcd_send_cmd(0x21); // 反转显示关闭
	lcd_send_cmd(0x29); // 开启显示
	lcd_send_cmd(0x2C); // 准备写入RAM数据
	
}

/**
 * @brief 设置屏幕显示窗口坐标
 * @param x1 窗口起始X
 * @param y1 窗口起始Y
 * @param x2 窗口结束X
 * @param y2 窗口结束Y
 * @note 不同旋转模式存在坐标偏移补偿
 */
void lcd_addr_set(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2)
{
	if (USE_HORIZONTAL == 0)
	{
		TFT_SEND_CMD(0x2a); // 设置X轴窗口
		LCD_WR_DATA(x1 );
		LCD_WR_DATA(x2 );
		TFT_SEND_CMD(0x2b); // 设置Y轴窗口
		LCD_WR_DATA(y1 + 20);
		LCD_WR_DATA(y2 + 20);
		TFT_SEND_CMD(0x2c); // 启动显存写入
	}
	else if (USE_HORIZONTAL == 1)
	{
		TFT_SEND_CMD(0x2a);
		LCD_WR_DATA(x1);
		LCD_WR_DATA(x2);
		TFT_SEND_CMD(0x2b);
		LCD_WR_DATA(y1 + 80);
		LCD_WR_DATA(y2 + 80);
		TFT_SEND_CMD(0x2c);
	}
	else if (USE_HORIZONTAL == 2)
	{
		TFT_SEND_CMD(0x2a);
		LCD_WR_DATA(x1);
		LCD_WR_DATA(x2);
		TFT_SEND_CMD(0x2b);
		LCD_WR_DATA(y1);
		LCD_WR_DATA(y2);
		TFT_SEND_CMD(0x2c);
	}
	else
	{
		TFT_SEND_CMD(0x2a);
		LCD_WR_DATA(x1 + 80);
		LCD_WR_DATA(x2 + 80);
		TFT_SEND_CMD(0x2b);
		LCD_WR_DATA(y1);
		LCD_WR_DATA(y2);
		TFT_SEND_CMD(0x2c);
	}
}

/**
 * @brief 直接发送16位RGB565像素
 * @param data 16位颜色值
 */
void lcd_send_data16(uint16_t data)
{
	TFT_SEND_DATA((data >> 8) & 0xFF);
	TFT_SEND_DATA(data & 0xFF);
}

/**
 * @brief 显示单个字符(含汉字/ASCII)
 * @param x 字符左上角X坐标
 * @param y 字符左上角Y坐标
 * @param no 字符点阵索引
 * @param fc 前景字体颜色
 * @param bc 背景填充颜色
 * @param font_size 字号 16/32/80
 * @note 点阵为8bit一行，bit=1显示前景色，bit=0显示背景色
 */
void lcd_show_char(uint32_t x, uint32_t y, uint32_t no, uint32_t fc, uint32_t bc, uint32_t font_size)
{
  int32_t i, j;
  uint8_t tmp;
  uint32_t x_s = x;
  uint32_t y_s = y;
  uint32_t x_e = x_s + font_size - 1;
  uint32_t y_e = y_s + font_size - 1;

  // 设置字符显示窗口
  lcd_addr_set(x_s, y_s, x_e, y_e);

  // 遍历全部点阵字节
  for (i = 0; i < font_size * font_size / 8; i++)
  {
    // 根据字号选择对应点阵库
    if (font_size == 16)
      tmp = g_font_dot_matrix_16[no][i];
	
    if (font_size == 32)
      tmp = g_font_dot_matrix_32[no][i];	
	
	if (font_size == 80)
		tmp = g_font_dot_matrix_80[no][i];	

    // 一个字节8个点，从高位到低位判断
    for (j = 7; j >=0; j--)
    {
      if (tmp & (1 << j))
      {
		  lcd_send_data16(fc);
      }
      else
      {
		  lcd_send_data16(bc);
      }
    }
  }
}

/**
 * @brief 根据字符查找点阵索引(GB2312编码)
 * @param target 待匹配字符指针
 * @return 点阵索引，-1表示未找到
 * @note ASCII单字节(0x00~0x7F)，汉字双字节(首字节>0x7F)
 */
int FindFontIndex_tft(const char *target)
{
    int i;
    // 遍历16号字索引表
    for (i = 0; i < ARRAY_SIZE(g_font_dot_matrix_16_index); i++)
    {
        // 判断单字节/双字节匹配
        if (strncmp(g_font_dot_matrix_16_index[i], target, (target[0] > 0x7F) ? 2 : 1) == 0)
        {
            return i;
        }
    }
    // 无匹配字符返回-1
    return -1;
}

/**
 * @brief 显示字符串(支持中文+ASCII自动换行)
 * @param x 起始X坐标
 * @param y 起始Y坐标
 * @param str 待显示字符串(GB2312)
 * @param fc 字体前景色
 * @param bc 背景色
 * @param font_size 字号大小
 */
void lcd_show_string(uint32_t x, uint32_t y, const char *str, uint32_t fc, uint32_t bc, uint32_t font_size)
{
	int32_t i=0;
	int32_t index=-1;
	int32_t x_s=x;
	
	while(str[i])
	{
		// 查找当前字符点阵索引
		index=FindFontIndex_tft(&str[i]);
		
		if(index < 0)
		{
			i++;
			continue;
		}
		
		// 输出单个字符
		lcd_show_char(x,y,index,fc,bc,font_size);
		
		// 中文占2字节，宽度等于字号；ASCII占1字节，宽度字号一半
		if(str[i]>0x7F)
		{
			i+=2;
			x+=font_size;
		}
		else
		{
			i++;
			x+=font_size/2;
		}
		
		// 行尾自动换行判断
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

/**
 * @brief 显示RGB565位图
 * @param x 图片左上角X
 * @param y 图片左上角Y
 * @param width 图片宽度
 * @param height 图片高度
 * @param image_data 位图数据缓冲区(2字节/像素RGB565)
 */
void lcd_draw_image(uint32_t x, uint32_t y, uint32_t width, uint32_t height, const uint8_t *image_data)
{
  uint32_t i = 0;
  uint32_t x_s = x;
  uint32_t x_e = x_s + width - 1;
  uint32_t y_s = y;
  uint32_t y_e = y_s + height - 1;
  const uint8_t *p = image_data;

  // 设置图片显示窗口
  lcd_addr_set(x_s, y_s, x_e, y_e);
  // 连续输出全部像素字节
  for (i = 0; i < width * height * 2; i++)
    TFT_SEND_DATA(*p++);
}

/**
 * @brief 格式化打印字符串(类似printf输出到屏幕)
 * @param x 起始X
 * @param y 起始Y
 * @param fc 字体颜色
 * @param bc 背景色
 * @param font_size 字号
 * @param str 格式化模板字符串
 * @param ... 可变参数
 */
void lcd_show_string_fmt(uint32_t x,uint32_t y,uint16_t fc,uint16_t bc,uint32_t font_size,char *str,...)
{
    va_list args;
    char temp_buf[256]; // 格式化缓存，最大256字符
    va_start(args, str);
    vsprintf(temp_buf, str, args);  // 格式化输出到缓存
    va_end(args);
	
	lcd_show_string(x,y,temp_buf,fc,bc,font_size);
}

/**
 * @brief 设置屏幕旋转方向 0/1/2/3对应0°/90°/180°/270°
 * @param dir 旋转编号
 * @note 同步更新全局屏幕宽高x_len/y_len
 */
void lcd_set_direction(uint32_t dir)
{
	g_lcd_direction = dir;
	
	/* 0度 竖屏 */
	if(dir==0)
	{
		lcd_send_cmd(0x36);
		lcd_send_data(0x00);
		x_len=TFT_COLUMN_NUMBER;
		y_len=TFT_LINE_NUMBER;
	}
	
	/* 90度 横屏 */
	if(dir==1)
	{
		lcd_send_cmd(0x36);
		lcd_send_data((1<<6)|(1<<5)|(1<<4));
		x_len=TFT_LINE_NUMBER;
		y_len=TFT_COLUMN_NUMBER;
	}	
	
	/* 180度倒置竖屏 */
	if(dir==2)
	{
		lcd_send_cmd(0x36);
		lcd_send_data((1<<7)|(1<<6));
		x_len=TFT_COLUMN_NUMBER;
		y_len=TFT_LINE_NUMBER;
	}
	
	/* 270度反向横屏 */
	if(dir==3)
	{
		lcd_send_cmd(0x36);
		lcd_send_data((1<<7)|(0<<6)|(1<<5)|(1<<4));
		x_len=TFT_LINE_NUMBER;
		y_len=TFT_COLUMN_NUMBER;
	}		
}

/**
 * @brief 获取当前屏幕旋转方向
 * @return g_lcd_direction 0~3
 */
uint32_t lcd_get_direction(void)
{
	return g_lcd_direction;
}
