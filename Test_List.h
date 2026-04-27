#pragma once

#ifndef XC_HEADER_TEMPLATE_H
#define	XC_HEADER_TEMPLATE_H
#include <xc.h> // include processor files - each processor file is guarded.  
#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */
#ifdef	__cplusplus
}
#endif /* __cplusplus */
#endif	/* XC_HEADER_TEMPLATE_H */

#include <stdbool.h>

#if (__DEBUG)
#define TEST() Test()
#else
#define TEST() 
#endif

#define TEST_INCLUDE

void Test(void);
void TestFuncHalogen (void);

void TestFuncCCT(void);

// стабильность приёма сигнала
void TestReceiveDMX (bool en, uint8_t* dmx , uint8_t size) ;

