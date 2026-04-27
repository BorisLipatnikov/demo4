#include "main.h"
#include "Control_PWM_Ext.h"
#include "rdm_dmx_hardware.h"

#include "LCD128x64J.h"
#include "Func_Lib.h"
#include "User_Interface.h"
#include "I2C_Device_Manager.h"

         #include "Test_List.h" // !!!!!!!!!!

void SwitchDeviceModeIdentif(void) {
    static bool prev_stat = 0 ;
    if (prev_stat == rdm.Identif) return;
    prev_stat = rdm.Identif;
    if (rdm.Identif == 1) SwitchDeviceMode(IDENTIF);
    else SwitchDeviceMode(DMX);
}

uint8_t TimerStandaloneCount = 0; 
void TimerStandaloneRun (void) {
    if (TimerStandaloneCount) --TimerStandaloneCount;    
    else 
        dmx.flagrx = true;        
}
void TimerStandaloneReset(void){
    TimerStandaloneCount = 64; // ms, period
}

struct Termo termo = {0, 255};
//------------------------------------------------------------------------------
void main(void)
{
    // Initialize the device
    SYSTEM_Initialize(); 
    __delay_ms(500);
    LCD_Init();     
    ADCC_SetADTIInterruptHandler(TermoControl);
    ADCC_StartConversion (channel_ANE1);
    
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//    INTERRUPT_GlobalInterruptEnable();
//    while(1);
//        
        
    DD6_Config();
    DD7_Config();
    StartupEEpromChip(PresetSizeSet );  // чтение загрузочной области эпромки
    StartupMemoryRead();
    
    menu.focus = 1; // перекинуть в заставку
    DrawMenu(); // для первичной отрисовки меню        // !!!!!!!!!!!!!    
        
    INTERRUPT_GlobalInterruptEnable();
            
           //TEST() ;
    
    while (1) {     
        CLRWDT();
        if (dmx.flagrx) Handler ();
        if (rdm.flag) {DMX_OFF; Handler_RDM (); DMX_ON;}
        ReadKey();
        SwitchDeviceModeIdentif();
        memory_write();
//        TermoControl();
        if(U1RXFOIF) dmx_Reset(); // костыль
    }
}
//------------------------------------------------------------------------------
void interrupt_tmr0 (void) { // 1 ms
    PWM_Set();   
    LCD_TimerLight();
    TimerDMX(DeviceMode!= MANUAL); 
    TimerStandaloneRun();
    
}

//------------------------------------------------------------------------------
void Handler (void) {  
    TimerStandaloneReset();
    dmx.flagrx = 0; 
    uint8_t temp;
          
// распределение в соответсвии с активным профилем    
    switch (DeviceMode) {               
    //  профиль прямого упправления
        case(DIRECT):    
            pwm.Dimmer = dmx.data[0]; //
            Calc_Strobo (dmx.data[7]);    
            Func_Direct(dmx.data);            
            break;       
    // ргб режим простой
        case(RGB):              
            pwm.Dimmer = dmx.data[0]; //
            Calc_Strobo (dmx.data[4]);                       
            Func_RGB(dmx.data[1], dmx.data[2], dmx.data[3]); 
            break;
    // ргб расширенный
        case(RGB_CT):
            Function(dmx.data[8]);
            pwm.Dimmer = dmx.data[0]; //  
            temp = Func_Preset(dmx.data[9]);
            if (temp) PresetDataRead(temp);
            else {
                Func_RGB(dmx.data[1], dmx.data[2], dmx.data[3]);
                Func_Sat(dmx.data[4], dmx.data[5], dmx.data[6]); 
            }
            Calc_Strobo (dmx.data[7]);             
            break;
    // режим ХСИ
        case(HS_CT):
            Function(dmx.data[7]);
            pwm.Dimmer = dmx.data[0];
            temp = Func_Preset(dmx.data[8]);
            if (temp) PresetDataRead(temp);
            else {
                Func_Hue(dmx.data[1], dmx.data[2]);
                Func_Sat(dmx.data[3], dmx.data[4], dmx.data[5]); 
            }
            Calc_Strobo (dmx.data[6]);             
            break;
    // режим Студия
        case(STUDIO):
            Function(dmx.data[4]);
            //if(!new_data) break;
            pwm.Dimmer = dmx.data[0]; 
            Func_CTT (dmx.data[1], dmx.data[2]);
            Calc_Strobo (dmx.data[3]);            
            break;
    // режим Пресет
        case(PRESET):
            if (dmx.error) pwm.Dimmer = 255;
            else pwm.Dimmer = dmx.data[0];
        // load from preset.c  
            PresetDataRead(ActivPreset);  
            Calc_Strobo (0);
            break;
    // идентификация по рдм    
        case(IDENTIF):
            PresetDataRead (PresetMax());
            pwm.Dimmer -= 5;
            Calc_Strobo (0);
            break;  
    // ручной 
        case (MANUAL):
        // load from preset.c       
            PresetDataRead(ActivPreset);  
            pwm.Dimmer = 255;   
            Calc_Strobo (0);
            break;
    }   
     
// ограничение общ яркости по температуре
    if (pwm.Dimmer > termo.protect) pwm.Dimmer = termo.protect;       
    Calculation_PWM();  
}

void SwitchDeviceMode (uint8_t divice_mode) {
    // провести перезагрузку функций
    dmx.flagrx = 1; 
    BufferFill(New_Value, TOTAL_COLOR, 0); // 
    BufferFill(dmx.data , DMX_BUFFER_SIZE, 0);
    
    switch (divice_mode) {                
        case (MANUAL):
            DMX_OFF;
            DeviceMode = MANUAL; 
            break;
        case(IDENTIF):
            pwm.Dimmer = 255;
            DeviceMode = IDENTIF;
            break;
        case(PRESET):
            DeviceMode = PRESET;
            break;
        case(DMX):
            DeviceMode = dmx.personality;            
            dmx.Start = 0;    
            //ReceivNewData(DeviceMode); // сбросить фильт дмикс данных
            DMX_ON;    
    }
}

//------------------------------------------------------------------------------
// период вызова 4с по прерыванию
void TermoControl (void) {
    
    // забыл в конфигураторе добавить сдвиг 
    uint16_t a = ADCC_GetFilterValue()>>1;
                 
    // анализ темп диапазонов
    termo.protect = 0;
    if  (a < _false)   termo.error = 2; // обрыв
    else if (a > _80C) termo.error = 3; // перегрев
    else if (termo.error != 3) {
        if (a > _70C) {              // снижение мощности
            termo.protect = 255 - ((a - _70C)<<1);
            termo.error = 4;
        }
        else {
            termo.protect = 255; 
            termo.error = 0; 
        }
    }
    // регулировка работы вентилятора
    if (a > _60C) FanSpeed(255);
    else if (a > _50C) FanSpeed(248);
    else if (a > _45C) FanSpeed(230);
    else if (a < _40C) FanSpeed(0);       
}

void StartupMemoryRead (void) {                             
    dmx.start_address = ((uint16_t)DATAEE_ReadByte(0x20)<<8)|DATAEE_ReadByte(0x21);// (uint16_t)
    dmx.new_address = dmx.start_address;
    dmx.personality = DATAEE_ReadByte(0x22); 
    pwm.curve = DATAEE_ReadByte(0x23);
    Set_PWM_Freq (DATAEE_ReadByte(0x24));    
    // по режиму диммера выбор загрузки данных либо галоген , либо СТ  
    LoadFuncData(pwm.curve); 
            
    AutoStart = DATAEE_ReadByte(0x26);    
    ActivPreset = DATAEE_ReadByte(0x25);
    // первичная загрузка данных и обозначение активного пресета (ActivPreset)
//    PresetDataRead (DATAEE_ReadByte(0x25)); // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    
// чтение из загрузочного сектора внешней ЭПРОМ!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!    
    // версия файла данных
//    Software_Version_ID [3] = I2C_ReadByteEEprom(); // тип
//    Software_Version_ID [4] = I2C_ReadByteEEprom(); // версия
    test(Software_Version_ID);
    
// если до выключения был активен режим ПРЕСЕТ - активация при вкл прибора      
    if(AutoStart) {
        SwitchDeviceMode(PRESET);
        menu.page = 13;
    }
    else {
        SwitchDeviceMode(DMX);
        menu.page = 1;
        ActivPreset = 1;
    }
    DRV_RECEIVE_MODE;
       
    for (uint8_t count = 0; count < 6; ++count)
        UID[count] = DATAEE_ReadByte(0x5-count);   // обратный порядок
    
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!   
    uint16_t addr_read = 0x8;
    bool read_from_mem =  (DATAEE_ReadByte(addr_read) != 0);
    unsigned char* a = DEVICE_MODEL_DESCRIPT;   // !!!!        
    char length = 0;        
    do {
        ++length ;  
        if (read_from_mem) {
            Device_Label [length] = DATAEE_ReadByte(addr_read);
            ++addr_read;
        }
        else
            Device_Label [length] = *a++;          
    } while (Device_Label[length] != '\0' && length < size_device_label);
    Device_Label[0] = length; // длина записи

    Software_Version_ID [0] = FLASH_ReadByte(0x200000);
    Software_Version_ID [1] = FLASH_ReadByte(0x200002); 
    Software_Version_ID [2] = FLASH_ReadByte(0x200004);
    //Software_Version_ID [3] = ; // версия данных И2С , тип   
    //Software_Version_ID [4] = ; // версия файла данных
 //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!       
}

void memory_write (void) {    
    if (rdm.write_memory) rdm.write_memory = 0;
    else return;
    
    DATAEE_WriteByte(0x20, dmx.start_address>>8);
    DATAEE_WriteByte(0x21, (uint8_t)dmx.start_address);
    DATAEE_WriteByte(0x22, dmx.personality);
    DATAEE_WriteByte(0x23, pwm.curve);
    DATAEE_WriteByte(0x24, Freq_HzToMod(pwm.Freq));
    DATAEE_WriteByte(0x25, ActivPreset);
    DATAEE_WriteByte(0x26, AutoStart);
    
    for (uint8_t count = 0; count != Device_Label[0]; ++count ) {
        DATAEE_WriteByte(0x8+count , Device_Label[count+1]);
    }
}










