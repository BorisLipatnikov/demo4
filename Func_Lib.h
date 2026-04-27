#pragma once

#ifndef XC_HEADER_TEMPLATE_H
#define	XC_HEADER_TEMPLATE_H
#include <xc.h> // include processor files - each processor file is guarded.  
#endif	/* XC_HEADER_TEMPLATE_H */

#ifndef _STDBOOL_H
#include <stdbool.h>
#endif

/* служебные функции */
// заболнить переданный массив , заданным значением
void BufferFill ( uint8_t* buffer, uint8_t size, uint8_t value);
// ret max 32000
unsigned int uDiv (unsigned int, unsigned int);
// (делимое, делитель)
unsigned long* ulDiv (unsigned long, unsigned long);
struct  {
    unsigned long quotient; // частное
    unsigned int rem;   // остаток
} uldiv_ir;

/* функции работы со светом */
// загружает указанные данные из ээпром в структуру FuncDat
void LoadFuncData(uint8_t type);
// массив структур хранения данных по узлам, порядок от 255 вниз с шагом 5
struct HalogenData {
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint8_t L;
    uint8_t A;
};
void Func_Halogen (void);

// массив структур хранения данных по узлам, порядок от 255 вниз к 0
struct CT {
    uint8_t LVL;
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint8_t L;
    uint8_t A;
    uint8_t C;
};
// функция ССТ , TINT
void Func_CTT (uint8_t, uint8_t);
void Func_CCT (uint8_t lvl);

// функция прямого управления каналами цвета
void Func_Direct(uint8_t*);
// функция РГБ режима
void Func_RGB (uint8_t, uint8_t, uint8_t);
// настройка параметров прибора по линии dmx
void Function(uint8_t);
// выбеливание (Sat, CT, Tint). использует текущее значение из New_Value[]
void Func_Sat(uint8_t, uint8_t, uint8_t);
// задает тон в 16-и битном формате (старшее, младшее)
void Func_Hue(uint8_t, uint8_t);

// -- функционал работы с пресетами -- //
// отсчет пресетов - number_preset <1:Max>
uint8_t ActivPreset;    //!!!!!!!!!!!!!!!!
uint8_t AutoStart;  //!!!!!!!!!!!!!!!!!!!!
// записывает количество пресетов в структуру пресетов
// при чтении загрузочной области чипа ээпром, 
void PresetSizeSet (uint8_t);
// переписывает данные указанного пресета в буфер данных шим
void PresetDataRead (uint8_t number);
// возвращает массив Label пресета
uint8_t* PresetLabelRead (uint8_t number);
// функция возвращает номер пресета соответствующего заданному значению дмикс
// контроллирует выход за границы пресетов
// если 0 и 206+ вернет ноль, свет нужно погасить , приоритет отдать   
uint8_t Func_Preset( uint8_t dmx);
// взозращает номер последнего пресета, соответсвует общему количеству, размеру
uint8_t PresetMax (void);

