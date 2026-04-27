/* 
 * File:   I2C_Manager.h
 * Author: Ћипатников Ѕорис
 *
 * Created on June 23, 2025, 3:00 PM
 */

#ifndef XC_HEADER_TEMPLATE_H
#define	XC_HEADER_TEMPLATE_H
#include <xc.h> // include processor files - each processor file is guarded.  
#endif	/* XC_HEADER_TEMPLATE_H */

#include <stdbool.h>

#ifndef I2C_MANAGER_H
#define	I2C_MANAGER_H

// конфигурирует микросхемы при запуске
void DD6_Config (void);
void DD7_Config (void);

// прочитать состо€ние кнопок, функци€ вернет байт по всем пинам в котором "1"  
// соответствует нажатой кнопке, активирована инверси€ пол€рности. 
// пин7...пин0 - старший...младший бит
uint8_t DD6_Read_Input (void);

// вкл (true)/ false  светодиод на лицевой панели 
void DD6_LedSet ( uint8_t en);

// установить уровни на выводах, упр разрешени€ми работы токостабилизаторов
// байт: пин7...пин0 - старший...младший бит
void DD7_SetOut (uint8_t);

// чтение загрузочной области , костылем возвращает количество пресетов
void  StartupEEpromChip (void (*callback)(uint8_t));

// считывает дл€ указанного пресета данные и подпись и сохр в структуре пресета
void I2C_ReadPresetEEprom (uint16_t number, uint8_t* value, uint8_t vallen, 
        uint8_t* label, uint8_t lblen );

// загрузка из эпром в структуру  func_dat
uint8_t I2C_LoadDataFunc(uint8_t* buffer, bool halo);


// пока не доделано
uint8_t test (uint8_t * arr4);


#endif	/* I2C_MANAGER_H */




