 /*作    者:粤嵌.温工*/
#ifndef __TFT_H__
#define __TFT_H__

#include "stm32f4xx.h"
#include "sys.h"
#include "FontDotMatrix16.h"
#include "FontDotMatrix32.h"
#include "FontDotMatrix80.h"
#include "delay.h"
#include <string.h>

#define TFT_COLUMN_NUMBER 240
#define TFT_LINE_NUMBER 280
#define TFT_COLUMN_OFFSET 0

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

/* 部分tft屏需要偏移量 */
#define X_OFFSET	0
#define Y_OFFSET	0

#define LCD_SOFT_SPI_ENABLE  0

#define RED 	0XF800   	// 红色
#define GREEN 	0X07E0 		// 绿色
#define BLUE 	0X001F  	// 蓝色
#define WHITE 	0XFFFF 		// 白色
#define BLACK 	0X0000 		// 黑色
#define YELLOW	0xFFE0		// 黄色
#define GREY	0x7BEF		//灰色


#if LCD_SOFT_SPI_ENABLE

#define SPI_CS_0 PGout(6) = 0
#define SPI_CS_1 PGout(6) = 1

#define SPI_SCK_0 PBout(15) = 0
#define SPI_SCK_1 PBout(15) = 1

#define SPI_SDA_0 PDout(10) = 0
#define SPI_SDA_1 PDout(10) = 1

#define LCD_RST_0 PDout(8) = 0
#define LCD_RST_1 PDout(8) = 1

#define LCD_DC_0 PEout(14) = 0
#define LCD_DC_1 PEout(14) = 1

#define LCD_BLK_0 PEout(12) = 0
#define LCD_BLK_1 PEout(12) = 1

#else

// SPI片选  PG6 保持不变
#define SPI_CS_0 PGout(6) = 0
#define SPI_CS_1 PGout(6) = 1

// SPI时钟 SCK 改为硬件SPI1引脚 PB3
#define SPI_SCK_0 PBout(3) = 0
#define SPI_SCK_1 PBout(3) = 1

// SPI数据 MOSI 改为硬件SPI1引脚 PB5
#define SPI_SDA_0 PBout(5) = 0
#define SPI_SDA_1 PBout(5) = 1

// LCD复位 PG8 不变
#define SPI_RST_0 PGout(8) = 0
#define SPI_RST_1 PGout(8) = 1

// LCD命令/数据选择 PG7 不变
#define SPI_DC_0 PGout(7) = 0
#define SPI_DC_1 PGout(7) = 1

// 背光/电源 PB4 不变
#define SPI_BLK_0 PBout(4) = 0
#define SPI_BLK_1 PBout(4) = 1

#endif

extern uint32_t x_len;
extern uint32_t y_len;
extern uint32_t g_lcd_direction;

extern void SYS_init(unsigned char PLL);
extern void IO_init(void);
extern void Delay_us(unsigned int _us_time);
extern void Delay_ms(unsigned int _us_time);
extern void spi1_send_byte(uint8_t byte);
extern void TFT_SEND_CMD(unsigned char o_command);
extern void TFT_SEND_DATA(unsigned char o_data);
extern void TFT_clear(uint32_t color);
extern void TFT_full(unsigned int color);
extern void lcd_fill(uint32_t x_s, uint32_t y_s, uint32_t x_len, uint32_t y_len,uint32_t color);
extern void TFT_init(void);
extern void lcd_addr_set(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2);
extern void lcd_send_data16(uint16_t data);
extern void lcd_show_char(uint32_t x, uint32_t y, uint32_t no, uint32_t fc, uint32_t bc, uint32_t font_size);
extern int FindFontIndex(const char *target);
extern void lcd_show_string(uint32_t x, uint32_t y, const char *str, uint32_t fc, uint32_t bc, uint32_t font_size);
extern void lcd_draw_image(uint32_t x, uint32_t y, uint32_t width, uint32_t height, const uint8_t *image_data);
extern void lcd_show_string_fmt(uint32_t x,uint32_t y,uint16_t fc,uint16_t bc,uint32_t font_size,char *str,...);
extern void lcd_set_direction(uint32_t dir);
extern uint32_t lcd_get_direction(void);
extern void lcd_display_on(uint16_t on);
	
#define USE_HORIZONTAL 0
#endif
