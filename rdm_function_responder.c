#include <xc.h>
#include "rdm_dmx_hardware.h"

//------------------------------------------------------------------------------
// функции поиска 
void Discovery_UnMute_Message (char* Slot23) {
    
    *(Slot23 ) = 0x02; //PDL 2
    *(Slot23+1) = 0;    // 0, 0
    *(Slot23+2) = 0;    
}

void Discovery_Mute_Message (char* Slot23) {
    *(Slot23 ) = 0x02; //PDL 2
    *(Slot23+1) = 0;    // 0, 0
    *(Slot23+2) = 0;
}

char Discovery_Unique_Branch_Message (char* buffer) {
    
     char local = 0;  
// Lower Bound UID - Slot 24-29 , Upper Bound UID -  Slot 30-35
// проверка на соответствие заявленному диапазону   
     // по нижней границе
     for(char a=0; a<6; a++) {
        local = *(buffer+24+a);
        if (local > UID[a]) return 0;   // ниже нижней границы 
        if (local < UID[a]) break;  // выше нижней границы
     }  // входит по нижней границе
     
     // по верхней границе
     for(char a=0; a<6; a++) {
         local = *(buffer+30+a);
        if (local < UID[a]) return 0;   // выше верхней границы  
        if (local > UID[a]) break;  // ниже верхней границы
     }  // входит по верхней границе
    
// если UID входит в диапазон формируем ответный пакет    
      
    *(buffer)     = 0xFE;
    *(buffer + 1) = 0xFE;     
    *(buffer + 2) = 0xFE;
    *(buffer + 3) = 0xFE;
    *(buffer + 4) = 0xFE;
    *(buffer + 5) = 0xFE;
    *(buffer + 6) = 0xFE;
    *(buffer + 7) = 0xAA;
    *(buffer + 8) = UID[0] | 0xAA; // MID
    *(buffer + 9) = UID[0] | 0x55;
    *(buffer +10) = UID[1] | 0xAA;
    *(buffer +11) = UID[1] | 0x55;
    *(buffer +12) = UID[2] | 0xAA; // DID
    *(buffer +13) = UID[2] | 0x55;
    *(buffer +14) = UID[3] | 0xAA;
    *(buffer +15) = UID[3] | 0x55;
    *(buffer +16) = UID[4] | 0xAA;
    *(buffer +17) = UID[4] | 0x55;
    *(buffer +18) = UID[5] | 0xAA;
    *(buffer +19) = UID[5] | 0x55;
    
    unsigned int b = 0; 
    for (char c=8; c<20; c++)
        b = b + *(buffer + c); 
    char d = 0;
    d = (char)(b >> 8); 
    *(buffer + 20) = d | 0xAA; // ChSum
    *(buffer + 21) = d | 0x55;
    d = (char) b;
    *(buffer + 22) = d | 0xAA;
    *(buffer + 23) = d | 0x55;
        
    return 1; // 1-UID входит в диапазон , 0- за пределами диапазона
}

//------------------------------------------------------------------------------
void Device_Label_Get(char* pdl) {
    *pdl = Device_Label[0];
    for (char a=1; a<=Device_Label[0]; a++)
        *(pdl+a) = Device_Label[a] ;
}
void Device_Label_Set(char* pdl) {
    Device_Label[0] = *pdl;
    if (Device_Label[0] > size_device_label) Device_Label[0] = size_device_label;
    for (char a=1; a<=Device_Label [0]; a++)
        Device_Label [a] = *(pdl+a);
    *pdl = 0;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void DMX_Personal_Description ( char* pd) {
    char* pdl = pd-1; 
    char data = *pd;
    //*pd = *pd;  // запрошеный профиль
    *(++pd) = 0;    // Footprint
    *(++pd) = Footprint_Personality [dmx.personality];
    char count = 0;
    Label (&count, (pd+1), Personal_Descript(data));
    *pdl = 3 + count;
}
//------------------------------------------------------------------------------
void Label ( char* PDL, char* PD , char* Label) {
    uint8_t count = 0;
    do {
        *PD++ = *Label++;
        ++count;
    } while ((*Label) != '\0');  
    *PDL = count;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void Device_Info (uint8_t* PDL) {
    //char * PD = PDL + 1;
    
    *PDL++ = 0x13; //19
    *PDL++ = 0x01;    // pdm vers 1.20
    *PDL++ = 0;
    *PDL++ = DEVICE_MODEL_ID >> 8; // Device_Model_ID
    *PDL++ = (uint8_t)DEVICE_MODEL_ID;
    *PDL++ = Product_Category >> 8; // Product Category
    *PDL++ = (uint8_t)Product_Category;
    *PDL++ = Software_Version_ID[0]; // Soft vers, H
    *PDL++ = Software_Version_ID[1];
    *PDL++ = Software_Version_ID[2];
    *PDL++ = Software_Version_ID[3]; //L
    //*PDL++ = Software_Version_ID[4]; //L
    *PDL++ = 0; // dmx footprint , занимаемые каналы, 255 макс
    *PDL++ = Footprint_Personality [dmx.personality];
    *PDL++ = dmx.personality; // dmx personality , профиль dmx
    *PDL++ = Total_Personality; 
    *PDL++ = dmx.start_address >> 8; // dmx start adress
    *PDL++ = (uint8_t)dmx.start_address;
    *PDL++ = 0;   // Sub-Device Count, кол-во используемых подустройст
    *PDL++ = SubDevice_Count;
    *PDL = Sensor_Count; //sensor count
}
//------------------------------------------------------------------------------
char Modulation_Frequency_Description (char* pd, uint16_t freq)  {
    // *(pd-1) -requested modulation
    *(pd) = 0xFF; *(++pd) = 0xFF; 
    *(++pd) = 0xFF; *(++pd) = 0xFF; 
    //*(++pd) = freq>>8; *(++pd) = (char)freq;
    
    uint16_t array[5]={10000,1000,100,10,1};
    char count;
    for (char i=0;i<5;i++) {   
        count = 0;
        while (freq >= array[i]) {
            freq -= array[i];
            count++ ;
        }
        array[i] = count;
    }
    /**/
    count = 5;
    for (char i=0; i<5; i++) 
        if (array[i] == 0) count--;  else break;
    
    for (char i=5-count; i<5; i++) 
        *(++pd) = (char)(array [i] + '0');        
    
    *(++pd) = 'H'; *(++pd) = 'z';    //!!!
    return (5+count+2);
}
    
    
    





