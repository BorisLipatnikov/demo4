#include "main.h"
#include "Control_PWM_Ext.h"
#include "rdm_dmx_hardware.h"
#include "Func_Lib.h"

#include "LCD128x64J.h"
#include "User_Interface.h"
#include "I2C_Device_Manager.h"

struct {
    uint8_t press;  // код нажатой клавиши
    uint8_t press_duration; // длительность нажатия
    uint8_t previous;   // предыдущее сост, не всегда исп
    struct {
        unsigned free_flag : 1; // клавиша отпущена
        unsigned lock : 1;  // блокировка обработки кнопок до освобождения, key.free
        unsigned flag : 1; // есть обновлённые данные по кнопкам
    };
}key;

// определение кнопок по чертежу плёнки
enum {      
    ENT = 1, // ввод SB1
    UP,      // вверх SB2
    ESC,     // назад, отмена SB3
    DWN,     // вниз SB4
    PRV      // правка SB5 
};  

// рассчет сосед знач по колесу знач (текущ знач, макс знач)
void calc_next_value (char value, char max);
// (проверяемое знач, макс гран)
uint16_t min_max_value (uint16_t, uint16_t max);
// массив для колеса значений
uint8_t NextValue[4];

//------------------------------------------------------------------------------
void Pravka (void) {  
    LCD_LightON();
    if (!menu.pravka_activ_flag) {
        SwitchDeviceMode(MANUAL);
        ActivPreset = PresetMax();  // последний , МАКС
        menu.pravka_activ_flag=1;  
        menu.page = 9;
        DrawMenu();
    }
    else if (key.press == PRV){
        menu.page = 1;
        ActivPreset = 1; 
        menu.pravka_activ_flag=0;
        SwitchDeviceMode(DMX);// сброс
        DrawMenu();
    }    
}

uint8_t* UID_ConvertStringHex(uint8_t* uidhex) {
    uint8_t temp[2]; 
    for(char a=0; a<6; ++a) {
        temp[1] = UID[a] & 0b1111;
        temp[0] = UID[a] >> 4;
        for(char b=0; b<2; ++b){
            uint8_t count = b+(a<<1);
            if (temp[b] < 10) uidhex[count] = temp[b] + '0x30';
            else uidhex[count] = temp[b] + '0x37';
        }
    }
    uidhex[12] = '\0';
    return uidhex;
}

uint8_t* SID_ConvertString (uint8_t* soft_id_string ) {
      /* Software_Version_ID  */
    soft_id_string[0] = Software_Version_ID[0] + '0'; 
    soft_id_string[1] = '.';
    soft_id_string[2] = Software_Version_ID[1] + '0';
    soft_id_string[3] = Software_Version_ID[2] + '0';
    soft_id_string[4] = '.';
    soft_id_string[5] = Software_Version_ID[3] + '0';
    soft_id_string[6] = Software_Version_ID[4] + '0';
    soft_id_string[7] = '\0';
  
    return soft_id_string;
}

void ControlLedDMX (void) {
    static uint8_t prev_value = 1;
    if ((dmx.error == 1)&&(prev_value < 2)) {
        prev_value = 2;
        DD6_LedSet (true);
    }
    else if ((dmx.error == 0)&&(prev_value > 0)) {
        prev_value = 0;
        DD6_LedSet (false);
    }
}

//------------------------------------------------------------------------------
// период входа 50мс
void ReadKey (void) {
    if (!TMR4_HasOverflowOccured()) return;
    
    ControlLedDMX ();
        
    uint8_t pins = (DD6_Read_Input()>>1);
    key.free_flag = 0; 
    
    if (pins & 0b1) key.press = ENT;
    else if (pins & 0b10) key.press = UP;
    else if (pins & 0b100) key.press = ESC;
    else if (pins & 0b1000) key.press = DWN;
    else if (pins & 0b10000) key.press = PRV;
    else {
        if (key.press != 0) key.free_flag = 1;
        key.press = 0;
        key.press_duration = 0;
    } 
    
        
    if (key.press) {   // обработка при нажатии/отжатии            
        key.press_duration++;              
        if (!menu.transit_flag)
            HandlerKey ();
    } 
    else 
        menu.transit_flag = 0;
}
//------------------------------------------------------------------------------
//const uint8_t TextMenu  = 6;
void DrawMenu(void) {
    LCD_Buffer_Clear();
    //uint8_t count_str;
    
    switch(menu.page){   
        case(0):
            
            break;
        case(1):{  // основн меню          
            //if(menu.focus>6) count_str = 6; else count_str = 4;
            uint8_t tmp[6]={0x78,0x88,0x98,0xA0,0xB0,0xC0};            
            for (uint8_t y=3, count_str=0; y<63; y+=10, ++count_str) {                
                WriteString (EE_String(tmp[count_str]), y, centr, menu.focus==(count_str+1));                
            }
        }
            break;    
        case(9): 
            WriteString (EE_String(0x58),15,centr,0);
            WriteString (EE_String(0x68),25,centr,0);
            WriteString (EE_String(0x6F),40,centr,1);
            break;
        case(11): // установка адреса
            WriteString (EE_String(0xCF),2,16,0);
            WriteString (EE_String(0xE0) ,25,0,0);
            WriteString (lcd_IntToStr(dmx.start_address),25,100,0);
            WriteString (EE_String(0xF0),40,0,0);
            WriteString (lcd_IntToStr(dmx.new_address),40,100,0);
            WriteString (EE_String(0xF8),55,0,0);     
            WriteString (lcd_IntToStr(dmx.new_address+Footprint_Personality[dmx.personality]),55,100,0);
            break;
        case(12):   // список профилей            
            WriteString(EE_String(0x105),0,centr,0);
            WriteString(Personal_Descript(NextValue[1]),17,centr,0);
            WriteString(Personal_Descript(dmx.personality),27,centr,1);
            WriteString(Personal_Descript(NextValue[2]),37,centr,0);
            WriteString(EE_String(0x118),54,0,0);
            WriteString(lcd_IntToStr(Footprint_Personality[dmx.personality]),54,116,0);
            break;
        case(13):  // список пресетов    
            calc_next_value(ActivPreset, PresetMax());  
            WriteString(PresetLabelRead(NextValue[0]),7,centr,0);
            WriteString(PresetLabelRead(NextValue[1]),17,centr,0);
            WriteString(PresetLabelRead(ActivPreset),30,centr,1);
            WriteString(PresetLabelRead(NextValue[2]),43,centr,0);
            WriteString(PresetLabelRead(NextValue[3]),53,centr,0);
            break;
        case(14):
            WriteString(EE_String(0x130),0, centr, 0);
            WriteString(EE_String(0x140),20,centr,pwm.curve==lin);
            WriteString(EE_String(0x14A),30,centr,pwm.curve==quadr);
            WriteString(EE_String(0x158),40,centr,pwm.curve==teatr);
            WriteString(EE_String(0x168),50,centr,pwm.curve==halo);
            break;            
        case(15): {
            uint8_t a[2] = {0x1F,'\0'}; // для отрисовки стрелок
            WriteString(EE_String(0x170),0,2,0);
            WriteString(a ,19 ,centr,0);
            WriteString(lcd_IntToStr(pwm.Freq),30,centr,0);
            WriteString(EE_String(0x188), 30,90,0);
            a[0] = 0x1E;
            WriteString(a ,40 ,centr,0);
            break;
        }
        case(16): {
            uint8_t temp_str[12+1];
            WriteString (EE_String(0x78),3,0,0);
            WriteString (lcd_IntToStr(dmx.start_address),3,right,0);
            WriteString(EE_String(0x88),13,0,0);
            WriteString(Personal_Descript(dmx.personality),13,right,0);
            WriteString(EE_String(0xB0),23,0,0);
            WriteString(lcd_IntToStr(pwm.Freq),23,right,0);            
            WriteString(EE_String(0xA0),33,0,0);
            WriteString(Read_Curve_Descript(pwm.curve),33,right,0);            
            WriteString(EE_String(0x190),43,0,0);            
            WriteString(SID_ConvertString(temp_str), 43, right, 0);
            WriteString(EE_String(0x18C),53,0,0);              
            WriteString(UID_ConvertStringHex(temp_str), 53, right, 0);
            break;
        }
    }     
    LCD_Rotate_Buffer (menu.rotate_flag) ;
    LCD_Send_Full_Buffer();
}

//------------------------------------------------------------------------------
void HandlerKey (void) {       
// блокировка реакции при удержании кнопки
    menu.transit_flag = 1; 
// отдельная обработка функц клавиши НАПРАВКА
// переводит в ручн режим и вкл пресет МАКС        
    if ((key.press==PRV)||(menu.pravka_activ_flag)) { // режим напрвка        
        Pravka();            
        return;
    }
                  
// из любого положения меню первое нажатие вкл подсветку,     
// вкл подсв с игнором действия
    if (LCD_LightON()) return;
        
    if (menu.page == 1) { // основное меню
        if (key.press==UP) menu.focus = (uint8_t)min_max_value(--menu.focus, 6);
        if (key.press==DWN) menu.focus = (uint8_t)min_max_value(++menu.focus, 6);
        if (key.press==ENT) {
            menu.page = 10 + menu.focus;
            key.press = 0;
        }  
        // разворот экрана     
        if (key.press == ESC) menu.rotate_flag = ~menu.rotate_flag;
    }
    
    switch(menu.page) {                
        case(11): // установка адреса           
            if ((key.press==UP)||(key.press==DWN)) menu.transit_flag = 0;
            if ((key.press_duration>8)||(key.press_duration==1)) {
                if (key.press==UP) 
                    dmx.new_address += 1+(key.press_duration>>3);
                if (key.press==DWN) 
                    dmx.new_address -= 1+(key.press_duration>>3);    
                dmx.new_address = min_max_value(dmx.new_address, 512);
            }  
            else return; 
            if (key.press==ENT) {
                dmx.start_address = dmx.new_address; 
                rdm.write_memory = 1;
            }
            if (key.press==ESC) dmx.new_address = dmx.start_address;
            break;     
        case(12):   // выбор профиля
            if (key.press==UP) 
                dmx.personality = (uint8_t)min_max_value(--dmx.personality, Total_Personality);            
            if (key.press==DWN)  
                dmx.personality = (uint8_t)min_max_value(++dmx.personality, Total_Personality);
            calc_next_value(dmx.personality, Total_Personality);
            if (key.press==ENT) {
                SwitchDeviceMode(DMX);
                rdm.write_memory = 1;
            }
            // изменения dmx.personality активируется через пресеты
            break;
        case(13):   // пресеты
            if (DeviceMode != PRESET) {
                SwitchDeviceMode (PRESET);  
            }
            if (key.press==UP) {
                ActivPreset = (uint8_t)min_max_value(--ActivPreset,PresetMax());            
            }
            if (key.press==DWN) {
                ActivPreset = (uint8_t)min_max_value(++ActivPreset, PresetMax());
            }
            if (key.press==ESC)  {
                SwitchDeviceMode(DMX);
                ActivPreset = 1;
                AutoStart = 0;
                rdm.write_memory = 1;
            }
            if (key.press==ENT) {
                AutoStart = 1;
                rdm.write_memory = 1;
                return;
            }                                  
            break;
        case(14):   // функция диммера
            if (key.press==UP)  if (pwm.curve) --pwm.curve;                           
            if (key.press==DWN) if (pwm.curve < halo) ++pwm.curve;                  
            if (key.press==ENT) {
                rdm.write_memory = 1;
                LoadFuncData(pwm.curve);
            }
            break;            
        case(15):   // частота шим
            if (key.press == UP) {               
                if (pwm.Freq < MAX_FREQ) pwm.Freq += 10;   
                else pwm.Freq = HIGH_FREQ;
            }
            if (key.press == DWN) {
                if (pwm.Freq == HIGH_FREQ) pwm.Freq = MAX_FREQ;
                else if (pwm.Freq > MIN_FREQ) pwm.Freq -= 10;                 
            }
            if (key.press == ENT) rdm.write_memory = 1;
            Set_PWM_Freq(pwm.Freq);
            break;            
        case(16):   // сводная информация        
            break;
    }
    
    if ((key.press==ENT)||(key.press==ESC))  menu.page = 1;    
    
    DrawMenu ();
}

//------------------------------------------------------------------------------
uint16_t min_max_value (uint16_t value, uint16_t max) {
    if (value>max) value=1;
    else if (value==0) value=max;
    return value;
}

void calc_next_value (char value, char max) {
    NextValue[1] = (uint8_t)min_max_value(value-1, max);
    NextValue[0] = (uint8_t)min_max_value(NextValue[1]-1, max);
    NextValue[2] = (uint8_t)min_max_value(value+1, max);
    NextValue[3] = (uint8_t)min_max_value(NextValue[2]+1, max);      
}




        






