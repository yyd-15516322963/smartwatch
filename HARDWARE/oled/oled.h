#ifndef __OLED_H
#define __OLED_H			  	 
#include "sys.h"
#include "stdlib.h"	    	
#define OLED_MODE 0
#define SIZE 8
#define XLevelL		0x00
#define XLevelH		0x10
#define Max_Column	128
#define Max_Row		64
#define	Brightness	0xFF 
#define X_WIDTH 	128
#define Y_WIDTH 	64	    						  
//-----------------OLED IIC----------------  					   

#define OLED_SCLK_Clr() GPIO_ResetBits(GPIOD,GPIO_Pin_6)//SDA IIC
#define OLED_SCLK_Set() GPIO_SetBits(GPIOD,GPIO_Pin_6)

#define OLED_SDIN_Clr() GPIO_ResetBits(GPIOD,GPIO_Pin_7)//SCL IIC
#define OLED_SDIN_Set() GPIO_SetBits(GPIOD,GPIO_Pin_7)

 		     
#define OLED_CMD  0
#define OLED_DATA 1	

extern void OLED_WR_Byte(unsigned dat,unsigned cmd);  
extern void OLED_Display_On(void);
extern void OLED_Display_Off(void);	   							   		    
extern void OLED_Init(void);
extern void OLED_Clear(void);
extern void OLED_Fill(uint8_t x,uint8_t y,uint8_t len,uint8_t dot);
extern void OLED_DrawPoint(u8 x,u8 y,u8 t);
extern void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 Char_Size);
extern void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size);
extern void OLED_ShowString(u8 x,u8 y, u8 *p,u8 Char_Size);	 
extern void OLED_Set_Pos(unsigned char x, unsigned char y);
extern void OLED_ShowCHinese(u8 x,u8 y,u8 no);
extern void OLED_DrawBMP(unsigned char x0, unsigned char y0,unsigned char x1, unsigned char y1,unsigned char BMP[]);
extern void Delay_50ms(unsigned int Del_50ms);
extern void Delay_1ms(unsigned int Del_1ms);
extern void fill_picture(unsigned char fill_Data);
extern void Picture(void);
extern void IIC_Start(void);
extern void IIC_Stop(void);
extern void Write_IIC_Command(unsigned char IIC_Command);
extern void Write_IIC_Data(unsigned char IIC_Data);
extern void Write_IIC_Byte(unsigned char IIC_Byte);
extern void IIC_Wait_Ack(void);
extern void oled_show_string(uint8_t x,uint8_t y,const char *str);
extern void oled_show_string_fmt(uint32_t x,uint32_t y,char *str,...);

/**
 * @brief   在OLED指定位置显示图片
 * @param   x       : 起始横坐标 (0-127)
 * @param   y       : 起始页坐标 (0-7)
 * @param   width   : 图片宽度
 * @param   height  : 图片高度
 * @param   picture : 图片数据指针
 * @retval  None
 */
void oled_draw_picture(unsigned char x, unsigned char y, 
                       unsigned char width, unsigned char height, 
                       const unsigned char *picture);
#endif  
	 



