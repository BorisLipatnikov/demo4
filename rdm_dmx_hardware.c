#include <xc.h>
#include "rdm_dmx_hardware.h"
#include "mcc_generated_files/interrupt_manager.h"
#include "mcc_generated_files/uart1.h"

void TimerDMX (bool mode) {
    if (mode) {
        if (dmx.time < PeriodLostDMX) ++dmx.time;
        else dmx.error = 1;
    }  
}

void Receive (void) {         
    // сигнал BREAK
    if (U1RXBKIF) {   
        U1RXBKIF = 0;
        dmx.Break = 1;   
        dmx.Start = 0;
        dmx.counter = 0;
        dmx.time = 0;  
        dmx.flagrx = 0;
        dmx.error = 0;
        dmx.synchro_raduga = 1;   
        rdm.slot_count = 1;
        rdm.lenght = 0;
        rdm.overflow = 0;
    }        
    // сигнал START
    else if (dmx.Break) {
        dmx.Break = 0;
        dmx.Start = BUF_RX ;  // прием значений каналов    
        if (dmx.Start == 0) {
            dmx.Start = 1;
            Reseive_Mode(reseive_dmx);
        }
        if (dmx.Start == 0xCC) Reseive_Mode(reseive_rdm);        
    }
    // выбор функции приЄма данных
    else if (dmx.Start == 1) DMX_Receive (BUF_RX);
    else if (dmx.Start == 0xCC) RDM_Receive (BUF_RX);
    else dmx.counter = BUF_RX; // очистка FIFO
}

// прием значений каналов      
void DMX_Receive (char buffer) {    
    dmx.data[dmx.counter] = buffer; 
    ++dmx.counter;                     
    if (dmx.counter == DMX_BUFFER_SIZE) {
        dmx.flagrx = 1;
        dmx.Start = 0; // отк вызов функции
    }      
}

// приЄм пакета RDM
void RDM_Receive (char buffer) {
    rdm.buffer [rdm.slot_count] = buffer;
// общее кол-во слотов в пакете с контр суммой
    if (rdm.slot_count == 2) rdm.lenght = rdm.buffer [2] + 2 ;      
    ++rdm.slot_count ;
    
// окончание пакета    
    if (rdm.slot_count == RDM_BUFFER_SIZE) rdm.overflow = 1; // переполнение
    if ((rdm.slot_count == rdm.lenght)||(rdm.overflow)) {        
        rdm.buffer [0] = 0xCC;        
        dmx.Start = 0; // отк вызов функции
        rdm.flag = 1;
        rdm.lenght = 0;   
        Reseive_Mode(reseive_dmx);
    }       
}
    
// отправка пакета рдм
void RDM_Transmit (char respond){      
    DMX_OFF;
    TRANSMIT_ON;
    
    //__delay_us(100);  //  176us EOP -> SOP         
    INTERRUPT_GlobalInterruptDisable();    
    rdm.slot_count = 0 ;
    uint8_t lenght = rdm.buffer [2] + 2; // 2 контр сум;
        
    uint8_t tmp = U1CON0;
    if (respond == 0) {    
        UART1_OFF;  
        U1CON0 = (tmp&0b11110000);         
        UART1_ON;
        BUF_TX = 0xFF; // отсеч break послед!!  
        NOP();         
        lenght = 24;                
        while (TRANSMIT_END);
    }
    
    U1P1 = lenght-1; // дл€ апарат счетчика дмикс длина пакета        
    __delay_us(4); // MAB , авт генер
    DRV_TRANSMIT_MODE; // перекл вых драйвера на трансл€цию после отсечки
       
    do {
        while (NOREADY_TRANSMIT);        
        BUF_TX = rdm.buffer [rdm.slot_count];
        rdm.slot_count ++ ;  
    } while ((rdm.slot_count < lenght));  
    while (TRANSMIT_END);
    NOP();
           
    if (respond == 0) U1CON0 = tmp; // возвращение режима dmx    
    TRANSMIT_OFF;
    NOP();
    DMX_ON;
    NOP();
    DRV_RECEIVE_MODE; // перекл вых драйвера на приЄм    
    INTERRUPT_GlobalInterruptEnable();   
 }

void Reseive_Mode (mode a){
    if (a == reseive_rdm) {
        U1P2=0;
        U1P3=RDM_BUFFER_SIZE-1;
    }
    else {  // reseive_dmx
        U1P2 = dmx.start_address-1;
        U1P3 = U1P2 + DMX_BUFFER_SIZE-1; 
    }
}

void dmx_Reset(void) {
  // сброс флага OERR
    DMX_OFF;         	    
	dmx.counter = BUF_RX; 
	dmx.counter = BUF_RX;
	dmx.counter = BUF_RX;
	dmx.counter = 0; 	
	dmx.Break = 0; 
    dmx.Start = 0; 
    dmx.flagrx=0;
    rdm.slot_count = 1;
    rdm.lenght = 0;
    U1RXFOIF = 0;
    DMX_ON;   
    /*  */
}

