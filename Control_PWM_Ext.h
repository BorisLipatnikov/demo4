// ver 1.0 
// библиотека многофункционального цветного диммера.
// диммирование по выбранной кривой с заданной скоростью 
// диммирование индивидуальное дл€ каждого канала или общее на все сразу
// реализаци€ функций стробировани€ на заданной частоте и виртуальной заслонки
// регулировка частоты работы шим с масштабированием рабочего цикла
// 

#ifndef XC_HEADER_TEMPLATE_H
#define	XC_HEADER_TEMPLATE_H
#include <xc.h> // include processor files - each processor file is guarded.  
#endif	/* XC_HEADER_TEMPLATE_H */

#include "mcc_generated_files/device_config.h"  // _XTAL_FREQ
#include "mcc_generated_files/pwm2_16bit.h"
#include "mcc_generated_files/pwm3_16bit.h"
#include "mcc_generated_files/pwm1_16bit.h"

#ifndef CONTROL_PWM_TEMPLATE_H
#define CONTROL_PWM_TEMPLATE_H

// кол-во цветов, каналов управлени€
#define TOTAL_COLOR   6

 uint16_t* Reg_PWM[TOTAL_COLOR] = {
    &PWM1S1P1,  // красн
    &PWM1S1P2,  // зел
    &PWM2S1P1,  // син
    &PWM3S1P2,  // лайм
    &PWM3S1P1,  // амбер
    &PWM2S1P2   // циан
};
#define PWM_LOAD PWM1_16BIT_LoadBufferRegisters(), \
PWM2_16BIT_LoadBufferRegisters(), PWM3_16BIT_LoadBufferRegisters()
 
#define PWM_FREQ_SET(a,b) PWM1PRH = a, PWM1PRL = b, \
PWM2PRH = a, PWM2PRL = b, PWM3PRH = a, PWM3PRL = b 

// управлени€ шим
// период пересчета шим в ручном режиме
#define _32MS 32 // ms
uint32_t Current_PWM[TOTAL_COLOR]; 
uint8_t New_Value [TOTAL_COLOR];
#define  RED     New_Value[0]
#define  GREEN   New_Value[1]
#define  BLUE    New_Value[2]
#define  LIME    New_Value[3]
#define  AMBER   New_Value[4]
#define  CYAN    New_Value[5]

struct PWM {          
    bool riseR, riseG, riseB, riseL, riseA, riseC;         
    uint16_t dR, dG, dB, dL, dA, dC;  // абсолютна€ разница уровней   
    uint8_t speed; // скорость диммера в битах: 7, 8, 9, 10
    uint16_t count; // 32 ms
    // текуща€ частота шим в √ц, 1000-1200-1300, +25к
    uint16_t Freq; 
    // частоты шим в количестве модул€ций, def 11 , 1-22
    uint16_t ratio_normaliz ; // нормировочный коэф
    uint8_t curve; // режим диммера: 0-лин, 1-квадр
    uint8_t Dimmer; // канал общего диммера
} pwm;

 enum Curve {
    lin, quadr, teatr, halo, TOTAL_CURVE         //#define  4 // rdm_protocol.h
}curve;

const char* Read_Curve_Descript (char);

void PWM_Set (void); // запись шим
// ( [R, G, B, L, A, C], Com )
void Calculation_PWM (void);

// регулировка частоты 1000-1200-1300, +25к
#define TOTAL_MODULATION_FREQUENCY  22  //  нумераци€ с 1
#define HIGH_FREQ  25000   // Hz
#define MIN_FREQ    1100    // Hz
#define MAX_FREQ    1300    // Hz
// уст частоты шим, вычисление pwm.ratio_normaliz(принимает и модул€ци€х, и в √ц )
void Set_PWM_Freq (uint16_t);
// преобраз текущей частоты шим в число модул€ций и обратно
uint16_t Freq_ModToHz (uint8_t);
uint8_t Freq_HzToMod (uint16_t);

unsigned int Get_PWM_Freq(void); // хз!!
// нормировка под период шим
uint16_t PWM_Normalization (uint32_t ) ;

// функции стробировани€
#define _25MS   25  // фиксированна€ длительность вспышки, мс
// задаЄт знчени€ дл€ функции стробировани€
void Calc_Strobo (uint8_t); //  struct strob

#endif  // CONTROL_PWM_TEMPLATE_H 