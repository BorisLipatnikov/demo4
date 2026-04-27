
#include <xc.h>
#include "rdm_dmx_hardware.h"
#include "Control_PWM_Ext.h"
#include "rdm_protocol.h"

extern uint8_t DeviceMode; 
extern uint8_t* SID_ConvertString (uint8_t* soft_id_string ) ;
extern void SwitchDeviceMode (uint8_t);

// подписи профилей
const uint8_t* Personal_Description [Total_Personality+1] = {
    "",
    " RGB ",
    " RGB-CT ",
    " HSI-CT ",
    " STUDIO ",
    " DIRECT "        
};

 uint8_t* Personal_Descript (uint8_t namber){
     return Personal_Description[namber];
 }

//------------------------------------------------------------------------------
char Personality_Set (char personality) {
    if ((personality == 0)||(personality > Total_Personality)) return 0;
    dmx.personality = personality;
    //Footprint = Footprint_Personality [Personality];
    DeviceMode = dmx.personality;
    return 1;
}

uint8_t GetFootprintPersonality(uint8_t profil) {
    if (profil > Total_Personality) return 0;
    return Footprint_Personality[profil];
}
//------------------------------------------------------------------------------
void Handler_RDM (void) {
    rdm.flag = 0; 
                       
// проверка целостности    
    if (Checksum_verification()) { return; } // флаг ошибки
// проверка адресата пакета
    if (Adress_verification()) { return; } 
    
    rdm.Class = rdm.buffer [20];
    rdm.PID = (rdm.buffer[21] << 8) + rdm.buffer[22];
    rdm.PDL = &rdm.buffer [23];    
    rdm.PD = &rdm.buffer[24];
    
// выбор функции обработки параметра
    uint8_t type_respons = E120_RESPONSE_TYPE_ACK ;
              
    switch (rdm.PID) {
        case (E120_DISC_UNIQUE_BRANCH):                      
            if ((rdm.mute == false)&&(Discovery_Unique_Branch_Message(rdm.buffer))) 
                    RDM_Transmit(0); 
            return;
        case(E120_DISC_MUTE):
            rdm.mute = 1;
            Discovery_Mute_Message (rdm.PDL);
            break;
        case (E120_DISC_UN_MUTE):
            rdm.mute = 0;
            Discovery_UnMute_Message (rdm.PDL);
            break;            
        case (E120_DEVICE_INFO):
            Device_Info (rdm.PDL);
            break;        
        case (E120_DEVICE_MODEL_DESCRIPTION):
            Label (rdm.PDL, rdm.PD , DEVICE_MODEL_DESCRIPT);
            break;
        case (E120_MANUFACTURER_LABEL):
            Label (rdm.PDL, rdm.PD , MANUFACTUR_LABEL);
            break;           
        case (E120_DEVICE_LABEL):
            if (rdm.Class == E120_GET_COMMAND) Device_Label_Get (rdm.PDL);
            else {
                Device_Label_Set (rdm.PDL);
                rdm.write_memory = 1;
            }
            break;
        case(E120_SOFTWARE_VERSION_LABEL):
            *rdm.PDL = 7;
            SID_ConvertString (rdm.PD);
            break;
        case(E120_IDENTIFY_DEVICE):
            if (rdm.Class == E120_GET_COMMAND) {
                *rdm.PDL = 1;
                *rdm.PD = (uint8_t)rdm.Identif;
            }
            else {   
                *rdm.PDL = 0;
                rdm.Identif = *rdm.PD;
            }
            break;
        case (E120_DMX_START_ADDRESS):
            if (rdm.Class == E120_GET_COMMAND) {
                *rdm.PDL = 2;
                *rdm.PD++ = (dmx.start_address >> 8);
                *rdm.PD = (uint8_t)dmx.start_address;
            }
            else {
                *rdm.PDL = 0;
                dmx.start_address = (rdm.PD[0] << 8)|(rdm.PD[1]);
                rdm.write_memory = 1;
            }
            break;
            
        case(E120_DMX_PERSONALITY):
            if (rdm.Class == E120_GET_COMMAND) {
                *rdm.PDL = 2;
                *rdm.PD++ = dmx.personality;
                *rdm.PD = Total_Personality;
            }
            else { 
                *rdm.PDL = 0;
                dmx.personality = *rdm.PD;
                SwitchDeviceMode(DMX);
                rdm.write_memory = 1;
            }
            break;
            
        case (E120_DMX_PERSONALITY_DESCRIPTION):            
            DMX_Personal_Description (rdm.PD);
            break;
            
        case (E120_SUPPORTED_PARAMETERS):
            // 115 пид за 1 заход
            type_respons = E120_RESPONSE_TYPE_ACK;
            *rdm.PDL = DEVICE_PID << 1 ;
            for (char b=0; b < DEVICE_PID; ++b) {
                *rdm.PD++ = (char)(Support_PID[b] >> 8);
                *rdm.PD++ = (char)Support_PID[b];  
            }
            break;
           
        case (E137_1_CURVE_DESCRIPTION):
            Label (rdm.PDL, rdm.PD+1, Read_Curve_Descript((*rdm.PD)-1));
            *rdm.PDL += 1 ;
            break;
            
        case (E137_1_CURVE):
            if (rdm.Class == E120_GET_COMMAND) {
                *rdm.PDL = 2; 
                *rdm.PD = pwm.curve+1;
                *(rdm.PD+1) = TOTAL_CURVE; 
            } 
            else {
                pwm.curve = (*rdm.PD)-1;
                *rdm.PDL = 0;
                rdm.write_memory = 1;
            }
            break;
            
        case (E137_1_MODULATION_FREQUENCY):
            if (rdm.Class == E120_GET_COMMAND) {
                *(rdm.PDL) = 2; 
                *(rdm.PD) = Freq_HzToMod(pwm.Freq); 
                *(rdm.PD+1) = TOTAL_MODULATION_FREQUENCY; 
            }
            else {
                Set_PWM_Freq (*rdm.PD); 
                *rdm.PDL = 0;
                rdm.write_memory = 1;
            }
            break;
            
        case (E137_1_MODULATION_FREQUENCY_DESCRIPTION):
            if (*rdm.PD > TOTAL_MODULATION_FREQUENCY) 
                type_respons = Error_Range();             
            else 
                *rdm.PDL = Modulation_Frequency_Description (rdm.PD+1, Freq_ModToHz(*rdm.PD));
            break;
            
        case(E120_QUEUED_MESSAGE):
            *rdm.PDL = 0;
            break;
        /*  */
        default:            
            type_respons = Error_PID();            
            break;
    }    
// подготовка ответного пакета, если запрос адрессный
    if (rdm.broadcast) return;
    if (rdm.overflow) type_respons = Error_Overflow();
    
 // заполнение слотов 0-22 включительно, с 23-го(PDL) заполняется в функц рдм.пид  
    Write_buffer (type_respons);
    
    RDM_Transmit (1);         
}
//------------------------------------------------------------------------------
char Error_Overflow (void) {
    *rdm.PDL = 2;
    *rdm.PD = 0;
    *(rdm.PD+1) = (char)E120_NR_PACKET_SIZE_UNSUPPORTED;
    rdm.overflow = 0;
    return E120_RESPONSE_TYPE_NACK_REASON;
}
char Error_PID (void) {
    *rdm.PDL = 2;
    *rdm.PD = 0;
    *(rdm.PD+1) = (char)E120_NR_UNKNOWN_PID;
    return E120_RESPONSE_TYPE_NACK_REASON;
}
char Error_Range(void) {
    *rdm.PDL = 2;
    *rdm.PD = 0;
    *(rdm.PD+1) = (char)E120_NR_DATA_OUT_OF_RANGE;
    return E120_RESPONSE_TYPE_NACK_REASON;
}
//------------------------------------------------------------------------------
void Write_buffer (char type_respons) {
    rdm.buffer[0] = E120_SC_RDM;
    rdm.buffer[1] = E120_SC_SUB_MESSAGE;
    rdm.buffer[2] = 24 + rdm.buffer[23];
    rdm.buffer[3] = rdm.buffer [9]; // Destination UID
    rdm.buffer[4] = rdm.buffer[10];
    rdm.buffer[5] = rdm.buffer[11];
    rdm.buffer[6] = rdm.buffer[12];
    rdm.buffer[7] = rdm.buffer[13];
    rdm.buffer[8] = rdm.buffer[14];
    rdm.buffer [9] = UID[0]; // Source UID
    rdm.buffer[10] = UID[1];
    rdm.buffer[11] = UID[2];
    rdm.buffer[12] = UID[3];
    rdm.buffer[13] = UID[4];
    rdm.buffer[14] = UID[5];
    //rdm.buffer[15] = ; // Transaction Number (TN) , без изменений
    rdm.buffer[16] = type_respons; // RDM Response_TYPE_ACK   default
    rdm.buffer[17] = 0; // Message count
    rdm.buffer[18] = 0; // Sub-Device
    rdm.buffer[19] = 0;
    rdm.buffer[20] += 1; // response
    //rdm.buffer[21] = 0; // PID , без изменений
    //rdm.buffer[22] = 0;
    
// ChSum ,     
    unsigned int cs = 0;
    char b = rdm.buffer[2];
    for (char a=0; a<b; a++) 
        cs += rdm.buffer[a];
    rdm.buffer[b] = (char) (cs >> 8);
    rdm.buffer[b+1] = (char)cs;
}
//------------------------------------------------------------------------------
char Adress_verification (void) {
    unsigned char a = 0;    
    unsigned char c[6];
    
    for (char b=0; b<6; b++) 
        c[b] = rdm.buffer [b+3]; 
        
// проверка на ширововещательное и персональное сообщение    
    rdm.broadcast = 0;
    rdm.personal = 0;
    for (char b=0; b<6; b++)
        if (c[b] != 0xFF) {a = 1; break;}
    
    if (a) {
        for (char b=0; b<6; b++)
            if (UID[b] != c[b]) { a = 0; break;}
        if (a) rdm.personal = 1;
    }
    else rdm.broadcast = 1;
    
    if ((rdm.broadcast)||(rdm.personal)) return 0; // принимаем сообщение
    return 1; // отвергаем сообщение
}
//------------------------------------------------------------------------------
char Checksum_verification (void) {
    unsigned char lenght;
    unsigned int checksum, sum;
    
    lenght = rdm.buffer [2]; // длина пакета без контр суммы
    checksum = rdm.buffer [lenght] << 8;
    checksum |= rdm.buffer [lenght + 1];
    
    sum = 0;
    for (char a=0; a < lenght; a++) 
        sum += rdm.buffer [a];
    
    if (sum != checksum) return 1;    
    return 0;
}





