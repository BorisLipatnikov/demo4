#pragma once
// идентификатор версии программы по 4 бита
#pragma config IDLOC0 = 1 // тип
#pragma config IDLOC1 = 0 // номер сборки(десятки) 0-9
#pragma config IDLOC2 = 1 // номер сборки(единицы) 1-9-0
#pragma config IDLOC3 = 0 // версия файла данных из эпром

#ifndef XC_HEADER_TEMPLATE_H
#define	XC_HEADER_TEMPLATE_H
#include <xc.h> // include processor files - each processor file is guarded.  
#endif	/* XC_HEADER_TEMPLATE_H */

#include "mcc_generated_files/mcc.h"

void interrupt_tmr0 (void);
void StartupMemoryRead (void); // чтение MCU_EEPROM
void memory_write (void);

void Handler (void);  // обработчик данных
uint8_t DeviceMode ; // режим работы прибора
void SwitchDeviceMode (uint8_t); // переключение реж работы

void DrawMenu (void);
struct Menu {
    uint8_t page; // отобр стр меню
    uint8_t focus; // фокусировка навигации на странице
    uint8_t temp_value; // временное хранение изменяемых в меню значений 
    unsigned transit_flag : 1; // флаг перехода по пунктам мемю
    unsigned pravka_activ_flag : 1; // активация(1) режима направки
    unsigned rotate_flag : 1; // поворот меню
} menu;

// термоконтроль //
#define FanSpeed(a) PWM1_LoadDutyValue((255-a)<<2) // 0-x-255
// для опорного ацп в 4096мВ
#define _false 10    //  обрыв термодатчика
#define _40C    108  //      // 1.74V   
#define _45C    124  //  101 // 1.99V
#define _50C    140  //  114 // 2.24V
#define _60C    171  //  139 // 2.74V
#define _70C    200  //  163 // 3.20V
#define _80C    225  //  183 // 3.60V, аварийное откл
// вызывается в обработчике прерывания вычислителя АЦП
void TermoControl (void);
struct Termo {
    uint8_t error ;  // код ошибки 
    uint8_t protect ; // ограничения общ яркости при перегреве
};


