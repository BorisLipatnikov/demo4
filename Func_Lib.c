#include "Func_Lib.h"
#include "Control_PWM_Ext.h"
#include "mcc_generated_files/tmr5.h"
#include "I2C_Device_Manager.h"



//------------------------------------------------------------------------------
// блок функций дл€ работы с пресетами
// структура данных пресета
struct PRESET{
    uint8_t size; // общее кол-во пресетов
    uint8_t current; // номер хран€щегос€ пресета в структуре (ActivPreset)
    uint8_t value[TOTAL_COLOR]; // значение пресета
    uint8_t label[17];  // 0-15 date, 16 = '\0' , подпись пресета
}preset = {0};

void PresetSizeSet (uint8_t value) {
    preset.size = value;
}

void load_preset (uint8_t number_preset ) {
    // проверка на корректность запроса
    if (number_preset == 0) number_preset = 1;
    else if (number_preset > PresetMax()) number_preset = PresetMax();
    // запрос новых ли данных
    if (number_preset == preset.current) return;
    preset.current = number_preset;
    
    // number [1, MaxPreset]
    // 24 байта на 1 пресет. [0,7]-значение, [8,23]-подпись. все незначащие == 0
    I2C_ReadPresetEEprom (number_preset, preset.value, TOTAL_COLOR, preset.label, 16);
    // конец строки, допол подтверждение 
    preset.label[16] = '\0';      
}

uint8_t* PresetLabelRead (uint8_t number_preset) {    
    load_preset(number_preset);
    return &(preset.label);
}

void PresetDataRead (uint8_t number_preset) {   
    // number_preset <1:Max>
    load_preset(number_preset);        
    for (char count=0; count<TOTAL_COLOR; ++count) {
        New_Value[count] = preset.value[count];
    }
}

uint8_t PresetMax (void){
    return preset.size;
}

uint8_t Func_Preset ( uint8_t value ) {  
    value >>= 1;
    if (preset.size < value) value = 0; 
    return value; 
}

//------------------------------------------------------------------------------
// структура данных дл€ галогена и цветовой температуры
struct FuncDat{
    uint8_t size;  // реальный размер из ээпром дл€ используемого массива
    union {
        // указаны максимальные размеры дл€ массивов, т.к. нет поддержки динамических
        struct HalogenData data[49]; //
        struct CT ct[26];
        uint8_t temp[250]; // приЄмный буффер
    };
} func_dat;
    
// выбор данных дл€ записи в структуру
void LoadFuncData(uint8_t type) {   
    func_dat.size = I2C_LoadDataFunc(func_dat.temp, type == halo);     
}

// lvl_end > lvl_begin, снизу вверх
// ret value for lvl , 
uint8_t result_linearization(uint8_t lvl_begin, uint8_t value_begin, 
        uint8_t lvl_end, uint8_t value_end, uint8_t lvl) {
    
    bool rise = true;
    uint16_t delta;
    // линеаризаци€
    if (value_begin > value_end) {
        rise = false;
        delta = value_begin - value_end;        
    }
    else 
        delta = value_end - value_begin;
    
    if (rise) 
        return value_begin + (uint8_t)uDiv(delta*(lvl - lvl_begin), lvl_end - lvl_begin);
    else
        return value_begin - (uint8_t)uDiv(delta*(lvl - lvl_begin), lvl_end - lvl_begin);
}

void Func_Halogen (void) {    
    BufferFill(New_Value, TOTAL_COLOR, 0); //   
    uint8_t  lvl = pwm.Dimmer;
// листаем массив структур , значение диммера от 255 вниз с шагом 5 , 
// кажда€ структура измеренных данных соответствует узлу   
// определ€ем соответствие узлу или расположение между узлами
    for (uint8_t index_struct = 0, lvl_node = 255; 
            index_struct < func_dat.size; ++index_struct, lvl_node -=5) {
        
        if (lvl == lvl_node) {
            uint8_t* ptr_data = &func_dat.data[index_struct].R;
            for (uint8_t i=0; i<5; ++i) 
                New_Value[i] = *ptr_data++ ;
            break;
        }
        if (lvl > lvl_node) {
            uint8_t* ptr_data = &func_dat.data[index_struct].R;
            uint8_t* ptr_prev_data = &func_dat.data[index_struct-1].R;
            for (uint8_t i=0; i<5; ++i) 
                New_Value[i] = result_linearization (lvl_node, *ptr_data++ ,
                        lvl_node+5, *ptr_prev_data++ , lvl);            
            break;
        }
    }      
}

// -----------------------------------------------------------------------------
void Func_Hue(uint8_t H, uint8_t L) {
    
    BufferFill(New_Value, TOTAL_COLOR, 0); 
    
    uint24_t Hue = H;
    Hue = (Hue<<8)+L; // <16b, (H<<8)|L
     
    if(H <= 18) {
        RED = 255;
        AMBER = (uint8_t)*ulDiv(255*Hue, 4863); // (18,255)
    }
    else if (H <= 39){      
        RED = (uint8_t)*ulDiv(255*(9984-Hue), 5375);  // (39,255)-(19,0)
        LIME = (uint8_t)*ulDiv(255*(Hue-4864), 5375);
        AMBER = 255;
    }
    else if (H <= 60) {
        GREEN = (uint8_t)*ulDiv(255*(Hue-10240), 5375);
        LIME = 255;
        AMBER = (uint8_t)*ulDiv(255*(15615-Hue), 5375);
    }
    else if (H <= 81) {
        GREEN = 255;
        LIME = (uint8_t)*ulDiv(255*(20991-Hue), 5375);
    }
    else if (H <= 97) {
        GREEN = 255;
        CYAN = (uint8_t)*ulDiv(255*(Hue-20992), 3839);  
    }
    else if (H <= 123) {
        GREEN = 255;
        BLUE = (uint8_t)*ulDiv(255*(Hue-25088), 6399); 
        CYAN = 255;
    }
    else if (H <= 162) { 
        GREEN = (uint8_t)*ulDiv(255*(41727-Hue), 9983); 
        BLUE = 255;
        CYAN = (uint8_t)*ulDiv(51*(44790-Hue), 2600);  
    }
    else if (H <= 173) {
        BLUE = 255;
        CYAN =  (uint8_t)*ulDiv(60*(44543-Hue), 2815);
    }
    else if (H <= 214) {
        RED = (uint8_t)*ulDiv(255*(Hue-44544), 10239);  
        BLUE = 255;
    }
    else  {
        RED = 255;
        BLUE = (uint8_t)*ulDiv(255*(65535-Hue), 10495);
    }    
}

void Func_Sat(uint8_t Sat, uint8_t CT, uint8_t Tint) {
    uint8_t rgblac[TOTAL_COLOR] = {RED, GREEN, BLUE, LIME, AMBER, CYAN};
    Func_CTT(CT, Tint);
        
    if (Sat < 10)  return; // CCT, полное выбеливание    
    
//  ( (XЦYA)H + (Z+YA)C  )/ D   
    uint16_t A, X, Y, Z, D;
    uint24_t temp, temp2;
    if(Sat < 75) {
        A = 75 - Sat;
        X = 910; Y = 14; Z = 390; D = 1300;
    }
    else if(Sat < 125) {
        A = 125 - Sat;
        X = 850; Y = 3; Z = 150; D = 1000;
    }
    else if(Sat < 250) {
        A = 250 - Sat;
        X = 2500; Y = 3; Z = 0; D = 2500;
    }
// S <250-255> - возврат тона, нет выбеливани€           
    for(char c=0; c<TOTAL_COLOR; ++c) {
        if (Sat >= 250) {
            New_Value[c] = rgblac[c];
        }
        else {
            temp = New_Value[c];
            temp *= (Z + Y*A); 
            temp2 = (X - Y*A);
            temp2 *= rgblac[c];
                                   
            New_Value[c] = (uint8_t)*ulDiv(temp+temp2 , D);
        }
    }    
}

// ----------------------------------------------------------------------
bool FunctionInputControl (uint8_t value){
    bool ret = true;
    static uint8_t prev_value;
// замок на повторную обработку, сброс при новом значении    
    static bool lock = false;
// проверка на мин входное    
    if(value >= 5) {
        if (prev_value == value) {
            if(lock) return ret;
            if (!TMR5_HasOverflowOccured()) {        
                TMR5_StartTimer();   
                return ret;
            }
            else {
                ret = false;
                lock = true;
            }
        }  
        else lock = false;
    }
    prev_value = value;    
    TMR5_StopTimer();
    PIR8bits.TMR5IF = 0; 
    TMR5_Reload();
    
    return ret; // true - 
}

void Function(uint8_t value) {
    if (FunctionInputControl(value)) return;
// функциональный уровень , шаг 4ед   
    value >>= 2;
// 1-22 модул€ци€ частоты шим value[4-91]
    if (value <= 22) Set_PWM_Freq(value);
// управление кривыми    
    else if(value == 23) {pwm.curve = quadr; }
    else if(value == 24) {pwm.curve = lin; }
    else if (value == 25) {pwm.curve = halo;}
    else if (value == 26) {pwm.curve = teatr;} 
    
    LoadFuncData(pwm.curve);
}

// ----------------------------------------------------------------------
void Func_Direct(uint8_t* data){
    for (uint8_t a = 0; a < TOTAL_COLOR; ++a)
        New_Value[a] = data[a+1]; // 1-R, 6-Cyan
}
// ----------------------------------------------------------------------
uint8_t compare_value(uint8_t value1, uint8_t value2){    
    
    if (value1 > value2) return value1;
    else if (value1 <= (value2>>1)) return 0;
    uint16_t temp = value1<<1;   
    return (uint8_t)(temp-value2);
}

uint8_t compare_rgb(uint16_t R, uint16_t G, uint16_t B) {   
    uint24_t temp; 
    if(G > R) {
        temp = 60*B>>8;
        temp += uDiv(195*G, 50);
    }
    else if(B > 5*R) {
        temp = 60*B;
        uint16_t temp2 = 306*R;
        if (temp > temp2) temp -= temp2;
        else temp = 0;
        temp += 1300*G;
        temp >>= 8;
    }
    else {
        temp = uDiv(255*G, 50);
    }
// сохр дл€ возврата из фильта    
    return (uint8_t)temp;
}

uint8_t compare_lime(uint16_t value1, uint16_t value2) {
    value1 >>= 1;
    if (value1 > value2) return (uDiv(value2<<1 , 5));
    return (uDiv(value1<<1 , 5) + (((value2-value1)*204)>>7));
}

void Func_RGB (uint8_t R, uint8_t G, uint8_t B){     
    //if (prev_lock(R, G, B)) return ;    
       
    RED = compare_value(R, G);    
    GREEN = compare_value(G, R);        
    BLUE = compare_value(B, G);  
    
    if(R > G) AMBER = GREEN;
    else AMBER = RED;
    
    if(G > R) LIME = compare_lime(G, R);
    else LIME = compare_lime(R, G);
    
     /*   
    uint16_t _5G = G*5;
    if (B > _5G) CYAN = compare_rgb( R, G, B); //      
    else if (B > G) CYAN = B;
    */
    if (B > (G>>1)) CYAN = G;
    else CYAN = B<<1;   
}

// ----------------------------------------------------------------------
void Func_CCT (uint8_t lvl) {    
// листаем массив структур , значени€ уровн€ lvl от 255 вниз  , 
// кажда€ структура измеренных данных соответствует узлу   
// определ€ем соответствие узлу или расположение между узлами
    for (uint8_t index_struct = 0; index_struct < func_dat.size; ++index_struct){
        uint8_t* ptr_data = &func_dat.ct[index_struct].R;
        if (lvl == func_dat.ct[index_struct].LVL ) {            
            for (uint8_t i=0; i<6; ++i) 
                New_Value[i] = *ptr_data++ ;
            break;
        }
        if (lvl > func_dat.ct[index_struct].LVL) {
            uint8_t* ptr_prev_data = &func_dat.ct[index_struct-1].R;
            for (uint8_t i=0; i<6; ++i) 
                New_Value[i] = result_linearization (func_dat.ct[index_struct].LVL,
                        *ptr_data++ , func_dat.ct[index_struct-1].LVL, 
                        *ptr_prev_data++ , lvl);            
            break;
        }
    }     
}

uint8_t operation_tint (uint16_t value, uint16_t ratio, uint8_t tint) {
    uint24_t temp = value;
    temp *= tint;
    ulDiv(temp, ratio);      
    return (uint8_t)uldiv_ir.quotient;
} 
/*!!*/
void Func_Tint(uint8_t Tint) {
    uint16_t a;
    if (Tint > 128) { // [129, 255]
        Tint = (Tint - 129)>>1;
        a = 126;
        if (RED < 170) RED = RED + operation_tint(170-RED, a, Tint);
        GREEN = GREEN - operation_tint(GREEN, a, Tint);        
        if (LIME > 130) LIME = LIME - operation_tint(LIME-130, a, Tint);
        if(BLUE < 140) BLUE = BLUE + operation_tint(140-BLUE, a, Tint);
    }
    else if (Tint > 0) {  // [1, 128]
        Tint = (128 - Tint)>>1;
        a = 381;
        GREEN = GREEN + operation_tint(3*(255-GREEN), a, Tint);
        BLUE = BLUE - operation_tint (BLUE, a, Tint);
    }
}

void Func_CTT (uint8_t CT, uint8_t Tint) {    
    if (pwm.curve == halo) Func_Halogen();
    else {
        Func_CCT(CT);
        Func_Tint(Tint);
    }
}

// ----------------------------------------------------------------------
void BufferFill ( uint8_t* buffer, uint8_t size, uint8_t value) {
    for (uint8_t count=0; count<(size); ++count) 
        buffer[count] = value;
}

unsigned long* ulDiv (unsigned long divident, unsigned long divisor) {
    unsigned char	counter = 1;    
    uldiv_ir.quotient = 0;
    
    while((divisor & 0x80000000UL) == 0) {
        divisor <<= 1;
        counter++;
    }
    do {
        uldiv_ir.quotient <<= 1;
        if(divisor <= divident) {
            divident -= divisor;
            uldiv_ir.quotient |= 1;
        }
        divisor >>= 1;
    } while(--counter != 0);
    uldiv_ir.rem = divident;
    
    return ((unsigned long*)&uldiv_ir);
}

unsigned int uDiv (unsigned int divident, unsigned int divisor) {
    char counter = 1;
    unsigned int quotient = 0;
    
    while((divisor & 0x8000) == 0) {
        divisor <<= 1;
        counter++;
    }
    do {
        quotient <<= 1;
        if(divisor <= divident) {
            divident -= divisor;
            quotient |= 1;
        }
        divisor >>= 1;
    } while(--counter != 0);
    
    return quotient;
}











