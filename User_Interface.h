/* 
 * File:   User_Interface.h
 * Author: Липатников Борис
 *
 * Created on June 24, 2025, 2:16 PM
 */

#ifndef USER_INTERFACE_H
#define	USER_INTERFACE_H

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef	__cplusplus
}
#endif
#endif	/* USER_INTERFACE_H */

#ifndef XC_HEADER_TEMPLATE_H
#define	XC_HEADER_TEMPLATE_H
#include <xc.h> // include processor files - each processor file is guarded.  
#endif	/* XC_HEADER_TEMPLATE_H */


//------------------------------------------------------------------------------
// use TMR4
//#define _PeriodReadKEY  50 // ms 
//void TimerReadKey(void);

// периодический вызов опроса клавиатуры
void ReadKey (void);

void HandlerKey (void) ;

