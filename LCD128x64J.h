#ifndef XC_HEADER_TEMPLATE_H
#define	XC_HEADER_TEMPLATE_H
#include <xc.h> // include processor files - each processor file is guarded.  
#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */
#ifdef	__cplusplus
}
#endif /* __cplusplus */
#endif	/* XC_HEADER_TEMPLATE_H */

#ifndef _XTAL_FREQ
#include "mcc_generated_files/device_config.h"
#endif

#include <stdbool.h>

#define RESOLUTION_X 128
#define RESOLUTION_Y 64
#define RESOL_PAGE RESOLUTION_Y>>3 //  8 страниц
#define _60S 60000    // ms, длит подсветки

// привязка выводов
#define LCD_A0  RA5
#define LCD_E   RA4
#define LCD_BACKLIGHT RC7
#define CHIP_SELECT RA0
#define LCD_DATA(a)   PORTD = a
//#define LCD_RW    // GND пост подкл
//#define LCD_Res(a)  // +5 пост подкл  

#define LCD_STROB_H   LCD_E = 1, NOP() // E=1
#define LCD_STROB_L   LCD_E = 0, NOP() // E=0
#define LCD_BACKLIGHT_ON  LCD_BACKLIGHT = 1
#define LCD_BACKLIGHT_OFF LCD_BACKLIGHT = 0
#define LCD_CHIP_L  CHIP_SELECT = 1, NOP() //  аппаратная инверсия
#define LCD_CHIP_R  CHIP_SELECT = 0, NOP() // 
//#define LCD_MODE_WRITE    LCD_RW = 0, NOP()  // R/W - 1/0
//#define LCD_MODE_READ     LCD_RW = 1, NOP() //
#define LCD_SET_COMMAND  LCD_A0 = 0, NOP()  // A0 = 0
#define LCD_SET_DATA     LCD_A0 = 1, NOP()    // A0 = 1

//------------------------------------------------------------------------------

// пользовательские функции управления дисплеем
// настройка дисплея при включении
void LCD_Init (void);
// таймер работы подсветки
void LCD_TimerLight (void);
// вкл подсветки
bool LCD_LightON(void);
// (page L and page R)
void LCD_Send_String_Buffer(uint8_t);
void LCD_Send_Full_Buffer(void);
void LCD_Buffer_Clear (void);
// повернуть буфер (экран) на 180, 
void LCD_Rotate_Buffer (bool ) ;
// (txt,y,x,inv)
void WriteString(uint8_t* txt, uint8_t y, uint8_t x, bool inv);

// выравнивание по экрану, значение Х
enum {  
    left=250,
    centr,
    right
};




//------------------------------------------------------------------------------

// отрисовка символов, (код по ascii вин 1251)
uint8_t* Font_8x8 (uint8_t);

void operation_write_byte_buffer (uint16_t a);
char SizeString (char* txt_ptr ); // возвр длину текста в пикс
// принимает число, возвращает строку
char* lcd_IntToStr (uint16_t );

// вкл/откл экран
void LCD_Display_En (bool en);
void LCD_Set_Page (uint8_t);
void LCD_Set_Address (uint8_t);
void LCD_Display_Clear(void);
void LCD_Data_Write (uint8_t);
void lcd_operation_write(uint8_t);

struct StringSet {
    uint8_t number;  // номер строки на странице
    uint8_t length ; // длина строки, 0-по длине текста
                // 255-по всей длине экрана    
    uint8_t leng_text; // длина текста в строке, пикс
    uint8_t alignment; // 1-центр, 0-лево, 2-право
    uint8_t indent_char; // отступ м\у символ, 0== 1пикс
        
};
