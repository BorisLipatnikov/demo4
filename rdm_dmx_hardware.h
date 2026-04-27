#pragma once

#include <xc.h> // include processor files - each processor file is guarded.  
#include "rdm_protocol.h"
#include <stdbool.h>

// аппаратная привязка, аппаратный дмикс
#define DMX_BUFFER_SIZE     10   // макс кол-во каналов dmx прибора 
#define RDM_BUFFER_SIZE     70  // мин кол-во слотов пакета rdm 25+32+2
#define BUF_RX      U1RXB
//#define BUF_RX9     RC1STAbits.RX9D
#define DMX_ON      U1RXEN = 1 // активация схемы приёмника USART
#define DMX_OFF     U1RXEN = 0 
//#define READ_FERR   U1FERIF_bit // U1ERRIR
#define DMX_Dir     RE2
#define PIN_BREAK_ON   U1BRKOVR = 1
#define PIN_BREAK_OFF  U1BRKOVR = 0 
#define DRV_RECEIVE_MODE    DMX_Dir = 0
#define DRV_TRANSMIT_MODE   DMX_Dir = 1
#define BUF_TX      U1TXB // передающий буфер
//#define BUF_TX9     TX9D
#define TRANSMIT_ON     U1TXEN = 1 // активация схемы передатчика
#define TRANSMIT_OFF    U1TXEN = 0
#define NOREADY_TRANSMIT  U1TXBF //
#define TRANSMIT_END    !(U1TXMTIF) // сдвиговый регистр 0-full , 1-empty
#define UART1_ON    U1CON1bits.ON = 1
#define UART1_OFF   U1CON1bits.ON = 0
 
#ifndef _XTAL_FREQ
#include "mcc_generated_files/device_config.h"
#endif

//#include "1533IR22.h"
#define PeriodLostDMX 1000 // ms

typedef enum {
    reseive_dmx,
    reseive_rdm
} mode ;
void Receive(void); // 
void dmx_Reset(void);
void TimerDMX(bool mode);
//void dmx_address_set (void); // запись адрс при аппаратном дмикс
void Reseive_Mode (mode);


struct {    
    unsigned Break :  1 ;
    unsigned error :  1 ;
    unsigned flagrx : 1 ;   //флаг приема пакета, сброс в BREAK и при чтении        
    unsigned synchro_raduga : 1 ;
    //unsigned synchro_strob : 1 ;  // флаг синхронизации строб   
    
    unsigned char Start ;
    unsigned int start_address;    // 1-512 !
    unsigned int new_address;    
    unsigned int counter;   // счетчик DMX каналов
    unsigned int time;      // счетчик таймаута, 1с
    unsigned char data[DMX_BUFFER_SIZE];   // приемный буфер dmx
    uint8_t personality;   // текущий профиль dmx, от 1!
} dmx;

void DMX_Receive (char) ;
void RDM_Receive (char) ;
void RDM_Transmit (char ); // 0-без break, 1-break

struct {     
    unsigned flag : 1 ; // флаг приёма пакета рдм
    unsigned broadcast : 1 ;
    unsigned personal : 1 ;
    unsigned mute : 1 ;  // 1-немой, 0-отвечает
    unsigned write_memory : 1 ;  // 1-приняты данные для сохранения в память мк
    unsigned overflow : 1; // переполлнение буфера рдм
    
    bool Identif; // флаг активации режима идентификации    
    uint8_t buffer [RDM_BUFFER_SIZE]; // приёмный буффер пакета рдм
    uint8_t slot_count; // счет приёма с 1!!
    uint8_t lenght; // длина принимаемого пакета
    unsigned int PID;   // идентификатор данных
    uint8_t Class;
    uint8_t* PDL; // указатель на размер блока данных
    uint8_t* PD;  // указатель на блок данных
    
} rdm;

/**/






