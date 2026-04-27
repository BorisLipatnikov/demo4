#include "Test_List.h"
#include "LCD128x64J.h"
#include "Func_Lib.h"
#include "Control_PWM_Ext.h"


// TMR1 - шаг 1мкс


void Test(void) {
    
    TestFuncHalogen () ;  // ok
//    LoadCTData();   // ok
    TestFuncCCT();  // ok
    
    while(1);   // stop
}


void TestFuncHalogen (void) {
//    LoadHalogenData();
    pwm.Dimmer = 255;
    do{
        Func_Halogen();
        --pwm.Dimmer;
    }while(pwm.Dimmer >= 20);    
}

void TestFuncCCT () {
    uint8_t lvl = 255;
    do {
    Func_CCT(lvl);
    --lvl;    
    }while (lvl!=0);
    Func_CCT(0);
}

void TestReceiveDMX (bool en, uint8_t* dmx , uint8_t size) {
    if (en == false ) return ;
    LCD_Buffer_Clear();
    for (char i=0 , y=0; i<size; i++ , y+=10)
        WriteString (lcd_IntToStr(dmx[i]),y,0,0);
    LCD_Send_Full_Buffer();
}

/*   
           
    
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! test - ok
        
 TestReceiveDMX(ReceivNewData(DeviceMode), dmx.data, GetFootprintPersonality(DeviceMode));
          
 //bool ReceivNewData (uint8_t profil) { 
//   
//    bool ret = false;    
//    static uint8_t prev_data[DMX_BUFFER_SIZE] = {1};
//    
//    for (uint8_t channel = 0; channel < GetFootprintPersonality(profil); ++channel) {
//        if(prev_data[channel] != dmx.data[channel]) ret = true;
//        prev_data[channel] = dmx.data[channel];
//    }
//    return ret;
//    
////    return true;
//}
 

*/











