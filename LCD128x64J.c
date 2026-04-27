#include "LCD128x64J.h"


struct {
uint8_t buffer [RESOL_PAGE][RESOLUTION_X];
uint8_t x; // pixel
uint8_t y; // pixel
uint16_t timer_light; // таймер длит свечения
bool reload_buf_flag ; // буфер обновлен, вывести на жк
} lcd;

//------------------------------------------------------------------------------
void LCD_TimerLight (void) {
    if (lcd.timer_light) lcd.timer_light--; else LCD_BACKLIGHT_OFF;
}

bool LCD_LightON(void) {
    bool ret = false;
    if (lcd.timer_light == 0){
        LCD_BACKLIGHT_ON; 
        ret=1;
    }    
    lcd.timer_light = _60S;
    return ret;
}

//------------------------------------------------------------------------------
// пин RES уже должен быть в 1, 
// плавное нарастание напряжения RES на конденсаторе после подачи питания
void LCD_Init (void) {
    LCD_BACKLIGHT_OFF;     
    __delay_us(200); 
    LCD_Display_Clear();
    LCD_CHIP_L;
    LCD_Set_Address(0);
    LCD_CHIP_R;
    LCD_Set_Address(0);
    LCD_Display_En(true);        
}

void LCD_Buffer_Clear (void) {
    for (char p=0; p<RESOL_PAGE; p++) 
        for (char x=0; x<RESOLUTION_X; x++)
            lcd.buffer[p][x]=0;
}

void LCD_Display_Clear(void){    
    LCD_CHIP_L;
    for (char j=0; j<2; j++) {
        for (char b=0; b<RESOL_PAGE; b++) {            
            LCD_Set_Page(b);
            for (char a=0; a<64; a++){
                LCD_Data_Write(0);
            }
        }
        LCD_Set_Page(0);
        LCD_CHIP_R;
    }   
}

void LCD_Send_String_Buffer (uint8_t page) {
    LCD_CHIP_L;
    LCD_Set_Page(page);
    for (char a=0; a<64; a++)
        LCD_Data_Write(lcd.buffer[page][a]);
    LCD_CHIP_R;
    LCD_Set_Page(page);
    for (char a=64; a<RESOLUTION_X; a++)
        LCD_Data_Write(lcd.buffer[page][a]);    
}

void LCD_Send_Full_Buffer(void) {
    for (char p=0; p<RESOL_PAGE; p++)
        LCD_Send_String_Buffer(p);
    lcd.reload_buf_flag = 0;
}

uint8_t operation_rotate (uint8_t a) {    
    uint8_t ret = 0;
    for (uint8_t shift = 0; shift < 8; ++shift) {
        ret <<= 1;
        ret |= (a>>shift)&0b1;        
    }
    return ret;
}

void LCD_Rotate_Buffer (bool a) {
    if (a == false) return;
    uint8_t temp;
    for (uint8_t x = 0; x < 64; ++x) {
        for (uint8_t page = 0; page < (RESOL_PAGE); ++page){
            temp = operation_rotate(lcd.buffer[page][x]);
            lcd.buffer[page][x] = operation_rotate
                    (lcd.buffer[(RESOL_PAGE)-1-page][(RESOLUTION_X)-1-x]);
            lcd.buffer[(RESOL_PAGE)-1-page][(RESOLUTION_X)-1-x] = temp;
        }
    }
}

void LCD_Display_En(bool en) { 
    LCD_SET_COMMAND;
    uint8_t command = 0b111110; //
    if (en) command |= 1 ;
    LCD_CHIP_L;
    lcd_operation_write (command);
    LCD_CHIP_R;
    lcd_operation_write (command);    
}

void LCD_Set_Page (uint8_t page) {
    LCD_SET_COMMAND;    
    uint8_t command = 0b10111000|page; // 0 page
    lcd_operation_write (command);
}

void LCD_Set_Address (uint8_t address) {
    LCD_SET_COMMAND;    
    uint8_t command = 0b1000000|address;
    lcd_operation_write (command);
}

void LCD_Data_Write (uint8_t data) {
    LCD_SET_DATA;
    lcd_operation_write (data);
}

void lcd_operation_write (uint8_t data) {
    //LCD_MODE_WRITE;
    LCD_STROB_H;
    NOP();
    LCD_DATA (data);
    NOP(); NOP();   // 200ns
    LCD_STROB_L;
    __delay_us(7); // удержание + пауза м/у посылками
    LCD_DATA(0);
}

//------------------------------------------------------------------------------
char SizeString (char* txt_ptr ) {
    uint8_t leng_text = 0;
    uint8_t symbol = 0; // символ
    
    while (*txt_ptr != '\0') {
        leng_text += *Font_8x8(*txt_ptr);
        leng_text++ ; // м/у символами пробел
        txt_ptr++ ;
    }
    return leng_text;
}

char* lcd_IntToStr (uint16_t a){
    uint16_t array[6] = {10000,1000,100,10,1,255};
    char count;
    for (char i=0;i<5;i++) {   
        count = 0;
        while (a >= array[i]) {
            a -= array[i];
            count++ ;
        }
        array[i] = count;
    }
    count = 0;
    static char ret[6];    
    while((array[0]==0)&&(count<4)) {
        for (char a=0; a<5; a++) {
            array[a] = array [a+1];
        }
        ++count ;          
    }
    count=0;
    while (array[count]!= 255) {
        ret[count] = (char)array[count] + '0';
        ++count ;
    }
    ret[count] = '\0';
    return ret;
}

void WriteString ( uint8_t* txt, uint8_t y, uint8_t x, bool inv) {
    uint8_t a = 0, page = 0;
    uint8_t* char_ptr = NULL;
    uint16_t dat = 0;
    
    // определение страницы для записи
    while (y>7) {
        page++ ;
        y -= 8;
    }
    
    // выравние по центру экрана
    if (x == centr) x = (RESOLUTION_X - SizeString(txt))>>1;
    if (x == right) x = (RESOLUTION_X - SizeString(txt));
    
    bool start_string = true; // для вставкb пробела перед строкой  
    while ((*txt != '\0')&&(x<RESOLUTION_X)) {                     
        char_ptr = Font_8x8(*txt);
        a = *char_ptr+1; // вставка +1 пробел после символа            

        for (char c=1; c <= a; ++c){   
            if (start_string) {
                start_string = false;
                c = 0; // +1 символ , dat==0  
            }
            else if (c == a) 
                dat = 0;
            else
                dat = *(char_ptr + c)<<1; // выровнять по высоте строки            
            if (inv) dat ^= 0x1FF;
            dat <<= y;
            lcd.buffer [page][x] |= dat ;
            if (page < 8)
                lcd.buffer [page+1][x] |= dat>>8 ;
            ++x ;
        }
        ++txt ;
    }    
}

//------------------------------------------------------------------------------
uint8_t* Font_8x8 (uint8_t win1251) {
   
    uint8_t Char[8]; // [0]-указывает длину символа
    #define _C7(a,b,c,d,e,f,g,h) Char[0]=a, Char[1]=b, Char[2]=c,\
    Char[3]=d, Char[4]=e, Char[5]=f, Char[6]=g, Char[7]=h
            
    switch (win1251) { 
        
        case(0x18):       //  стрелка вверх
            _C7(0x5, 0x04, 0x06, 0x7F, 0x06, 0x04,0,0);
            break;
        case(0x19):       //  стрелка вниз
            _C7(0x5, 0x10, 0x30, 0x7F, 0x30, 0x10,0,0);
            break;  
        case(0x1E):       //  треугольник вниз
            _C7(0x7, 0x10, 0x30, 0x70, 0xF0, 0x70,0x30,0x10);
            break; 
        case(0x1F):       //  треугольник вверх
            _C7(0x7, 0x8, 0xC, 0xE, 0xF, 0xE,0xC,0x8);
            break;     
        case(0x20):       // пробел в 4 пикс
            _C7(0x4, 0x00, 0x00, 0x00, 0x00, 0x00,0,0);
            break;
        case(0x28):    // (
            _C7(0x2, 0x3e, 0x41, 0x00, 0x00, 0x00,0,0);              
            break;
        case(0x29):     //   )
            _C7(0x2, 0x41, 0x3e, 0x00, 0x00, 0x00,0,0);            
            break;
        case(0x2b):     // +
            _C7(0x5, 0x08, 0x08, 0x3e, 0x08, 0x08,0,0);
            break;    
        case(0x2c):    // ,
            _C7(0x2, 0xe0, 0x60, 0x00, 0x00, 0x00,0,0);            
            break;
        case(0x2d):     // -
            _C7(0x5, 0x08, 0x08, 0x08, 0x08, 0x08,0,0);
            break;
        case(0x2e):     // .
            _C7(0x4, 0x0,0x60, 0x60, 0x0,0,0,0);
            break; 
        case(0x2f):     // /
            _C7(0x5, 0x20, 0x10, 0x08, 0x04, 0x02, 0,0);
            break;
        case(0x30):     // 0
            _C7(0x5, 0x3e, 0x41, 0x41, 0x41, 0x3e,0,0); 
            break;            
        case(0x31):     // 1
            _C7(0x3, 0x42, 0x7f, 0x40, 0x00, 0x00,0,0);
             break;
        case(0x32):     // 2
            _C7(0x5, 0x62, 0x51, 0x49, 0x49, 0x46,0,0);
            break;
        case(0x33):     // 3
            _C7(0x5, 0x22, 0x49, 0x49, 0x49, 0x36,0,0);
            break;   
        case(0x34):     // 4
            _C7(0x5, 0x18, 0x14, 0x12, 0x7f, 0x10,0,0);
            break;
        case(0x35):     // 5
            _C7(0x5, 0x2f, 0x49, 0x49, 0x49, 0x31,0,0);
            break;
        case(0x36):     // 6
            _C7(0x5, 0x3c, 0x4a, 0x49, 0x49, 0x30,0,0);
            break;
        case(0x37):     // 7
            _C7(0x5, 0x01, 0x71, 0x09, 0x05, 0x03,0,0);
            break;
        case(0x38):     // 8
            _C7(0x5, 0x36, 0x49, 0x49, 0x49, 0x36,0,0);
            break;
        case(0x39):     // 9
            _C7(0x5, 0x06, 0x49, 0x49, 0x29, 0x1e,0,0);
            break;    
        case(0x3a):    // :
            _C7(0x2, 0x6c, 0x6c, 0x00, 0x00, 0x00,0,0);
            break;    
        case(0x3c):     // <     
            _C7(0x4, 0x08, 0x14, 0x22, 0x41, 0x00,0,0);
            break;
        case(0x3d):     // =
            _C7(0x5, 0x24, 0x24, 0x24, 0x24, 0x24,0,0);
            break;
        case(0x3e):     // >
            _C7(0x4, 0x41, 0x22, 0x14, 0x08, 0x00,0,0);
            break;
        case(0x41):     // A
            _C7(0x5, 0x7e, 0x11, 0x11, 0x11, 0x7e,0,0);
            break;
        case(0x42):     // B
            _C7(0x5, 0x7f, 0x49, 0x49, 0x49, 0x36,0,0);
            break;
        case(0x43):     // C
            _C7(0x5, 0x3e, 0x41, 0x41, 0x41, 0x22,0,0);
            break;    
        case(0x44):     // D
            _C7(0x5, 0x7f, 0x41, 0x41, 0x41, 0x3e,0,0);
            break;   
        case(0x45):     // E
            _C7(0x5, 0x7f, 0x49, 0x49, 0x49, 0x41,0,0);
            break;  
        case(0x46):     // F
            _C7(0x5, 0x7f, 0x09, 0x09, 0x09, 0x01,0,0);
            break;    
        case(0x47):     // G
            _C7(0x5, 0x3e, 0x41, 0x49, 0x49, 0x7a,0,0);
            break;
        case(0x48):     // H
            _C7(0x5, 0x7f, 0x08, 0x08, 0x08, 0x7f,0,0);
            break;    
        case(0x49):     // I
            _C7(0x3, 0x41, 0x7f, 0x41,0,0,0,0);
            break;   
        case(0x4a):     // J
            _C7(0x05, 0x30, 0x40, 0x40, 0x40, 0x3f, 0,0);
            break;
        case(0x4b):     // K
            _C7(0x5, 0x7f, 0x08, 0x14, 0x22, 0x41,0,0);
            break;     
        case(0x4c):     // L
            _C7(0x5, 0x7f, 0x40, 0x40, 0x40, 0x40,0,0);
            break;    
        case(0x4d):     // M
            _C7(0x5,0x7f, 0x02, 0x04, 0x02, 0x7f, 0,0);
            break;
        case(0x4e):     // N
            _C7(0x5, 0x7f, 0x02, 0x04, 0x08, 0x7f,0,0);
            break;   
        case(0x4f):     // O
            _C7(0x5, 0x3e, 0x41, 0x41, 0x41, 0x3e,0,0);
            break;    
        case(0x50):     // P
            _C7(0x5, 0x7f, 0x09, 0x09, 0x09, 0x06,0,0);
            break;    
        case(0x51):     // Q
            _C7(0x5, 0x3e, 0x41, 0x51, 0x21, 0x5e, 0,0);
            break;    
        case(0x52):     // R
            _C7(0x5, 0x7f, 0x09, 0x09, 0x19, 0x66,0,0);
            break;    
        case(0x53):     // S
            _C7(0x5,0x26, 0x49, 0x49, 0x49, 0x32,0,0);
            break;     
        case(0x54):     // T
            _C7(0x5, 0x01, 0x01, 0x7f, 0x01, 0x01,0,0);
            break;    
        case(0x55):     // U
            _C7(0x5, 0x3f, 0x40, 0x40, 0x40, 0x3f,0,0);
            break;  
        case(0x56):     // V
            _C7(0x5, 0x1f, 0x20, 0x40, 0x20, 0x1f, 0,0);
            break;
        case(0x57):     // W
            _C7(0x5, 0x3f, 0x40, 0x3c, 0x40, 0x3f,0,0);
            break;    
        case(0x58):     // X
            _C7(0x5,0x63, 0x14, 0x08, 0x14, 0x63,0,0);
            break;
        case(0x59):     // Y
            _C7(0x5, 0x07, 0x08, 0x70, 0x08, 0x07,0,0);
            break;    
        
        case(0xc0):     // А
            _C7(0x5, 0x7e, 0x11, 0x11, 0x11, 0x7e,0,0);
            break;
        case(0xc1):     // Б
            _C7(0x5, 0x7f, 0x49, 0x49, 0x49, 0x31,0,0);
            break;
        case(0xc2):     // В
            _C7(0x5,0x7f, 0x49, 0x49, 0x49, 0x36, 0,0);
            break;
        case(0xc3):     // Г
            _C7(0x5,0x7f, 0x01, 0x01, 0x01, 0x01, 0,0);
            break;    
        case(0xc4):     // Д
            _C7(0x6,0xc0,0x7e, 0x41, 0x41, 0x7f, 0xc0,0);
            break;
        case(0xc5):     // Е
            _C7(0x5, 0x7f, 0x49, 0x49, 0x49, 0x41,0,0);
            break;
        case(0xc6):     // Ж
            _C7(0x5,0x77, 0x08, 0x7f, 0x08, 0x77, 0,0);
            break;    
        case(0xc7):     // З
            _C7(0x5, 0x22, 0x49, 0x49, 0x49, 0x36,0,0);
            break;    
        case(0xc8):     // И
            _C7(0x5,0x7f, 0x20, 0x10, 0x08, 0x7f, 0,0);
            break;
        case(0xc9):     // Й
            _C7(0x5, 0x7e, 0x21, 0x11, 0x09, 0x7e,0,0);
            break;   
        case(0xca):     // К
            _C7(0x5, 0x7f, 0x08, 0x14, 0x22, 0x41,0,0);
            break;    
        case(0xcb):     // Л
            _C7(0x5, 0x40, 0x7e, 0x01, 0x01, 0x7f,0,0);
            break;    
        case(0xcc):     // М
            _C7(0x5, 0x7f, 0x02, 0x04, 0x02, 0x7f,0,0);
            break;    
        case(0xcd):     // Н
            _C7(0x5, 0x7f, 0x08, 0x08, 0x08, 0x7f,0,0);
            break;   
        case(0xce):     // O
            _C7(0x5, 0x3e, 0x41, 0x41, 0x41, 0x3e,0,0);
            break;    
        case(0xcf):     // П
            _C7(0x5, 0x7f, 0x01, 0x01, 0x01, 0x7f,0,0);
            break;
        case(0xd0):     // P
            _C7(0x5, 0x7f, 0x09, 0x09, 0x09, 0x06,0,0);
            break;    
        case(0xd1):     // С
            _C7(0x5, 0x3e, 0x41, 0x41, 0x41, 0x22,0,0);
            break;
        case(0xd2):     // Т
            _C7(0x5, 0x01, 0x01, 0x7f, 0x01, 0x01,0,0);
            break;
        case(0xd3):     // У
            _C7(0x5, 0x27, 0x48, 0x48, 0x48, 0x3f,0,0);
            break;
        case(0xd4):     // Ф
            _C7(0x5, 0x0e, 0x11, 0x7f, 0x11, 0x0e,0,0);
            break;
        case(0xd6):     // Ц
            _C7(0x5, 0x7f, 0x40, 0x40, 0x7f, 0xc0,0,0);
            break;    
        case(0xd7):     // Ч
            _C7(0x5, 0x07, 0x08, 0x08, 0x08, 0x7f,0,0);
            break;
        case(0xd8):     // Ш
            _C7(0x5, 0x7f, 0x40, 0x7f, 0x40, 0x7f,0,0);
            break;    
        case(0xd9):     // Щ
            _C7(0x5, 0x7f, 0x40, 0x7f, 0x40, 0xff,0,0);
            break;    
        case(0xdb):     // Ы
            _C7(0x5, 0x7f, 0x48, 0x48, 0x30, 0x7f,0,0);
            break;
        case(0xdc):     // Ь
            _C7(0x5, 0x7f, 0x48, 0x48, 0x48, 0x30,0,0);
            break;
        case(0xdd):     // Э
            _C7(0x5, 0x22, 0x41, 0x49, 0x49, 0x3e,0,0);
            break;
        case(0xde):     // Ю
            _C7(0x5,0x7f, 0x08, 0x3e, 0x41, 0x3e,0,0);
            break;    
        case(0xdf):     // Я
            _C7(0x5, 0x66, 0x19, 0x09, 0x09, 0x7f,0,0);
            break;    
        
        case(0xf6):     // ц
            _C7(0x5,0x7C, 0x40, 0x40, 0x7C, 0xC0, 0,0);
            break;    
     /* case():     // 
            _C7(0x, 0,0);
            break;
       */
        default:
        _C7(0x5, 0x2a, 0x1c, 0x36, 0x1c, 0x2a, 0,0);
    }        
    return Char;    
}