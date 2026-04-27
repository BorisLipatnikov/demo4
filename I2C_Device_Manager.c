
#include "I2C_Device_Manager.h"
#include "mcc_generated_files/i2c1_master.h"

#define DD6_ADDR  0b0111100     // PCA9554A 
#define DD7_ADDR  0b0111101
#define EEPROM_ADDR  0b1010011  // 24LC64
#define PCA_CONF_REG    3   // R/W
#define PCA_INP_REG     0   // R
#define PCA_OUT_REG     1   // R/W
#define PCA_POL_REG     2   // R/W 

//------------------------------------------------------------------------------
void I2C_ReadNBytes(i2c1_address_t address, uint8_t *data, uint8_t len)
{
    while(!I2C1_Open(address)); // sit here until we get the bus..
    I2C1_SetBuffer(data,len);
    I2C1_MasterRead();
    while(I2C1_BUSY == I2C1_Close()); // sit here until finished.
}

void I2C_WriteNBytes(i2c1_address_t address, uint8_t* data, uint8_t len)
{
    while(!I2C1_Open(address)); // sit here until we get the bus..
    I2C1_SetBuffer(data,len);
//    I2C1_SetAddressNackCallback(I2C1_STOP,NULL); //NACK polling?
    I2C1_MasterWrite();
    while(I2C1_BUSY == I2C1_Close()); // sit here until finished.
}

//------------------------------------------------------------------------------
// работа с I2C регистрами
void DD6_Config (void) {
    // 1-inp, 0-out
    uint8_t pins_config[] = {(uint8_t)PCA_CONF_REG, 0b11111110};
    I2C_WriteNBytes (DD6_ADDR, pins_config , 2 );
    uint8_t pins_polarity[] = {(uint8_t)PCA_POL_REG, 0b00111110};
    I2C_WriteNBytes (DD6_ADDR, pins_polarity, 2);
}

void DD7_Config (void) {
    DD7_SetOut(0);
    uint8_t pins_config[2] = {(uint8_t)PCA_CONF_REG, 0b11000000};
    I2C_WriteNBytes (DD7_ADDR, pins_config , 2 );      
}

// функци€ вернет бит "1" если кнопка нажата 
uint8_t DD6_Read_Input (void) {
    while(!I2C1_Open(DD6_ADDR)); 
    uint8_t temp = (uint8_t)PCA_INP_REG;
    I2C1_SetBuffer(&temp, 1);
    I2C1_MasterWrite();
    I2C1_SetBuffer(&temp, 1);
    I2C1_MasterRead();
    while(I2C1_BUSY == I2C1_Close());
    return temp;
}

void DD6_LedSet (uint8_t en) {
    uint8_t pin_out [] = {(uint8_t)PCA_OUT_REG , en};
    I2C_WriteNBytes (DD6_ADDR, pin_out, 2);
}

void DD7_SetOut (uint8_t value) {
    static uint8_t prev = 0b11000000; // 2 старших не используютс€ по схеме
    if (value == prev) return;
    prev = value; 
    uint8_t pins_out [] = {(uint8_t)PCA_OUT_REG, value};
    I2C_WriteNBytes (DD7_ADDR, pins_out, 2);
}

//------------------------------------------------------------------------------
// работа с I2C ЁЁѕ–ќћ
struct {
    uint8_t number_file;
    uint8_t version_data;
    uint16_t addr_pres; // изначально внизу адресного пространства находитс€
    uint16_t addr_halo;
    uint16_t addr_ct;
}chip_eeprom;

// устанавливает позицию внутреннего счетчика адреса
void EEpromChipAddrSet (uint16_t addr) {
    uint8_t temp [2] = { addr>>8, (uint8_t)addr };
    I2C_WriteNBytes (EEPROM_ADDR, temp, 2);    
}

void StartupEEpromChip (void (*callback)(uint8_t)) {
    EEpromChipAddrSet(0);
    uint8_t buff[16];
    I2C_ReadNBytes (EEPROM_ADDR, buff, sizeof(buff));
    
    chip_eeprom.number_file = buff[0];
    chip_eeprom.version_data = buff[2];
    chip_eeprom.addr_pres = (uint16_t)(buff[8]<<8|buff[9]);  // 8-й всегда 0
    chip_eeprom.addr_halo = (uint16_t)(buff[10]<<8|buff[11]);
    chip_eeprom.addr_ct = (uint16_t)(buff[12]<<8|buff[13]);
    // это выкинуть кол-во пресетов в их структуру
    callback (buff[1]);
}

void I2C_ReadPresetEEprom (uint16_t number, uint8_t* value, uint8_t vallen, 
        uint8_t* label, uint8_t lblen) {
    
    EEpromChipAddrSet( 24*(number-1) + chip_eeprom.addr_pres );
    I2C_ReadNBytes (EEPROM_ADDR , value, vallen);
    I2C_ReadNBytes (EEPROM_ADDR , label, 2); // сброс 2-х не используемых €чеек
    I2C_ReadNBytes (EEPROM_ADDR , label, lblen);
}

uint8_t I2C_LoadDataFunc(uint8_t* buffer, bool halo){
    
    if (halo) {
        // 0x09C0, address = 0x09C1 , size = 48 , 
        // I2C_ReadBlockEEprom (0x09C1 , 240 , func_dat.data);
        EEpromChipAddrSet(chip_eeprom.addr_halo);
    }
    else {
        // address = 0x0AD0 , size = 7*25 , 
        // I2C_ReadBlockEEprom (0x0AD1 , 175 , func_dat.ct);
        EEpromChipAddrSet(chip_eeprom.addr_ct);
    }
    
    uint8_t ret_size; 
    I2C_ReadNBytes (EEPROM_ADDR, &ret_size, 1);  
    I2C_ReadNBytes (EEPROM_ADDR, buffer, ret_size);      

    return ret_size;
}



uint8_t test (uint8_t * arr4){
    arr4[3] = chip_eeprom.number_file;
    arr4[4] = chip_eeprom.version_data;
    return 0;
}



