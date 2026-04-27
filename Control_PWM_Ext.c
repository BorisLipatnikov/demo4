#include "Control_PWM_Ext.h"
#include "Func_Lib.h"
#include "I2C_Device_Manager.h"

const char*  curve_descript [TOTAL_CURVE] = {
    "LINEAR",
    "SQUARE",
    "THEATRE",
    "HALOGEN"
    };  // подпись дл€ кривых, ответа по рдм и дисплей

const char* Read_Curve_Descript (char number) {       
    return (curve_descript[number]);
}

unsigned int Get_PWM_Freq(void) {    
    return (pwm.Freq);
}

//------------------------------------------------------------------------------
struct {
    bool light ; // 1/0 - светит /не светит, вирт заслонк
    bool en ; // активаци€  стробоскопа (1)
    uint16_t period; // в тактах прерывани€, мс
    uint16_t timer;  // таймер стробоскопа  
} strob;

bool Strobo(void);

//------------------------------------------------------------------------------
void PWM_Set (void) {       
    bool* rise = &(pwm.riseR);
    uint16_t* d_pwm = &pwm.dR;
    // функци€ стробоскопа, return bool; 1-светит, 0-затемн€ет
    bool strob = Strobo();     
        
    for (uint8_t i = 0; i < TOTAL_COLOR; ++i) {
        if(pwm.count) {
            --pwm.count;
            (rise[i]) ? (Current_PWM[i] += d_pwm[i]):(Current_PWM[i] -= d_pwm[i]);            
        }  
        if (strob) {
            *(Reg_PWM[i]) = (uint16_t)(Current_PWM[i]>>pwm.speed); 
        }
        else {
            *(Reg_PWM[i]) = 0;
        }        
    }
    PWM_LOAD;  // запись в регистры шим            
}

//------------------------------------------------------------------------------
uint16_t Calc_Dimmer (uint24_t Chan) {
    Chan *= pwm.Dimmer;     
    //if (pwm.curve == 0)  // линейна€
    if(pwm.curve == teatr) { // teatr
        Chan = *ulDiv( Chan*pwm.Dimmer , 383-(pwm.Dimmer>>1) );
    }
    else if (pwm.curve != lin) {  // квадратична€ + галоген 
        Chan *= (pwm.Dimmer + 1);
        Chan >>= 8 ;
    }
    return PWM_Normalization (Chan);  //размер регистра 65к
}

bool NewDataPWM(uint16_t* new) {
    bool ret = false;
    static uint16_t prev_new[TOTAL_COLOR];
    
    for (uint8_t i=0; i<TOTAL_COLOR; ++i) {
        if(prev_new[i] != new[i]) ret = true;
        prev_new[i] = new[i];
    }
    return ret;    
}

void Calculation_PWM (void) {    
    uint32_t New[TOTAL_COLOR]; // новые уровни дл€ шим 16бит   
                    
// функци€ Flash , мгновенно применить новые уровни, 
// если разница более 100 ед дмикс по каналу общего диммера  
    static char prev_Com = 0;
    uint8_t flash ;            
    if (pwm.Dimmer > prev_Com) flash = pwm.Dimmer - prev_Com;
    else flash = prev_Com - pwm.Dimmer;       
    if (flash < 100) flash = 0;
    prev_Com = pwm.Dimmer;
        
    if (pwm.curve == halo) pwm.speed = 9;
    else if (pwm.curve == quadr) pwm.speed = 8;
    else if (pwm.curve == lin) pwm.speed = 8;
    else pwm.speed = 9; 
      
    for (char i=0; i<TOTAL_COLOR; i++) {  
        New[i] = Calc_Dimmer (New_Value[i]); // ret 16 бит ;
    }
    
    if ((!NewDataPWM(New))&&(pwm.count != 0))  return; 
    pwm.count = 0; // остановка изменений в регистрах шим

// рассчет значений шага изменени€ уровней   
    bool* rise = &(pwm.riseR);
    uint16_t* d_pwm = &pwm.dR;
    uint8_t active_channel = 0;
    
    for (char i=0; i<TOTAL_COLOR; ++i) {     
        if (flash) {
            Current_PWM[i] = New[i];
            *d_pwm = 0;  
        } 
        else {
//            Current_PWM[i] = *Reg_PWM[i];
            Current_PWM[i] >>= pwm.speed;
            if (New[i] > Current_PWM[i]) {
                *d_pwm = (uint16_t)(New[i] - Current_PWM[i]); 
                *rise = true;
            }
            else {
                *d_pwm = (uint16_t)(Current_PWM[i] - New[i]); 
                *rise = false;
            }
        }
        active_channel >>= 1 ;
        if (Current_PWM[i] != 0) active_channel |= 0b100000;        
        Current_PWM[i] <<= pwm.speed;
        ++rise; ++d_pwm;
    }    
    
    DD7_SetOut (active_channel);
    if (flash) pwm.count = 1; // 1 заход записать значени€
    else pwm.count = (1 << pwm.speed) ; // -1

} 
//------------------------------------------------------------------------------
uint16_t Freq_ModToHz ( uint8_t a) {
    uint16_t b;
    if ( a == TOTAL_MODULATION_FREQUENCY) 
        b = HIGH_FREQ;
    else 
        b = MIN_FREQ + 10*(a - 1);
    return b; // ret Freq  Hz
}  

uint8_t Freq_HzToMod (uint16_t Freq) {
    uint16_t b;
    if ( Freq == HIGH_FREQ) 
        b = TOTAL_MODULATION_FREQUENCY;
    else 
        b = 1 + uDiv(Freq - MIN_FREQ, 10);
    return (uint8_t)b;
}
//------------------------------------------------------------------------------
void Set_PWM_Freq ( uint16_t modulation) {
// рассчет значени€ регистра периода шим    
// поступила на вход частота в герцах или модул€ци€х
    if (modulation < MIN_FREQ) pwm.Freq = Freq_ModToHz (modulation);
    
// PWM_PR = PWM_Clock / PWM_Freq   , prescale = 1 !!!        
    //_uldiv = ulDiv (_XTAL_FREQ, pwm.Freq); // 1.125us
    //PWM_PR = *_uldiv;
    unsigned long PWM_PR = *(ulDiv (_XTAL_FREQ, pwm.Freq)); 
  
// запись по регистрам, PWM1PR, PWM2PR, PWM3PR
    PWM_FREQ_SET ((uint8_t)(PWM_PR >> 8), (uint8_t)PWM_PR); // 3us
    
// рассчет нормировочного коэффициента pwm.ratio_normaliz
// 16-ти битный формат, соот-вие м/у входным значением и заполнением шим
// New*pwm.ratio_normaliz >>8 = PWM_DC 
// 255^2 * pwm.ratio_normaliz = PWM_PR << 8     
    // находим макс входное значение дл€ выбраной кривой
    unsigned long max_data = 65025;  // (255*255);  
    // делим на макс входное значение, возвр ссылку на структуру
    unsigned long* _uldiv = ulDiv(PWM_PR<<8, max_data);     
    pwm.ratio_normaliz = *_uldiv;       
    ++_uldiv ;    
    _uldiv = ulDiv(*_uldiv * 10, max_data);
    if (*_uldiv > 6) pwm.ratio_normaliz++ ;   
}
//------------------------------------------------------------------------------
uint16_t PWM_Normalization (uint32_t _pwm) {    // (uint16_t)
    return ((_pwm * pwm.ratio_normaliz) >> 8);  
}  
//------------------------------------------------------------------------------
bool Strobo (void) {        
    bool ret = strob.light;// виртуальна€ заслонка, затемнение 
//    static bool prev_ret;
    
// стробирование с рассчетной частотой    
    if (strob.en) {        
        ++strob.timer ;
        if (strob.timer > strob.period) 
            strob.timer = 0;
        if (strob.timer > _25MS) 
            ret = false;   
        else 
            ret = true;
    }    
    else  
        strob.timer = 0;
      
    return ret; // 1-светит, 0-затемн€ет
}

void Calc_Strobo (uint8_t StrobData) {    
    strob.light = (StrobData == 0)||(StrobData > 216);
    if ((StrobData < 16)||(StrobData > 216)) strob.en = false;         
    else {
        strob.en = true;
        uint8_t FStrob = 0; // частота стробировани€
        while (StrobData > 15) {
            StrobData -= 10;	// шаг стробо
            FStrob ++;		// частота в √ц
        }
//strob.period = 1000/ FStrob;
        strob.period = uDiv(1000, FStrob);
    }    
}

//------------------------------------------------------------------------------












