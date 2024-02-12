// multimod_BME280.c
// Date Created: 2023-09-15
// Date Updated: 2023-09-15
// Declarations for BME280 functions

/************************************Includes***************************************/

#include "../multimod_BME280.h"

#include <stdint.h>
#include "../multimod_i2c.h"

/************************************Includes***************************************/

/********************************Public Functions***********************************/

void BME280_Init(void){
    I2C_Init(I2C_A_BASE);

    //CTRL_HUM
    BME280_WriteRegister(BME280_CTRL_HUM_ADDR, 0x0);

    //CTRL_MEAS
    BME280_WriteRegister(BME280_CTRL_MEAS_ADDR, 0b10100011);

    //CONFIG
    BME280_WriteRegister(BME280_CONFIG_ADDR, 0b0001000);

    return;
}

void BME280_WriteRegister(uint8_t addr, uint8_t data){
    uint8_t toWrite[2] = {addr, data};
    I2C_WriteMultiple(I2C_A_BASE, BME280_ADDR, toWrite, 2);
}

uint8_t BME280_ReadRegister(uint8_t addr){
    I2C_WriteSingle(I2C_A_BASE, BME280_ADDR, addr);
    uint8_t data = I2C_ReadSingle(I2C_A_BASE, BME280_ADDR);
    return data;
}

void BME280_MultiReadRegister(uint8_t addr, uint8_t* data, uint8_t num_bytes){
    I2C_WriteSingle(I2C_A_BASE, BME280_ADDR, addr);
    I2C_ReadMultiple(I2C_A_BASE, BME280_ADDR, data, num_bytes);
}

int32_t BME280_TempGetResult(){
    while (!(BME280_GetDataStatus() & BME280_STATUS_MEASURING));

    uint8_t bytes[3];

    BME280_MultiReadRegister(BME280_TEMP_MSB_ADDR, bytes, 3);
    uint32_t adc_T = ((uint32_t)bytes[0] << 12 | (uint32_t)bytes[1]  << 4 | bytes[2] >> 4);


    return BME280_compensate_T_int32(adc_T);
}
uint8_t BME280_GetDataStatus(){
    return BME280_ReadRegister(BME280_STATUS_ADDR);
}

int32_t BME280_compensate_T_int32(uint32_t adc_T){
    int32_t t_fine, var1, var2, T;

    var1 = ((((adc_T>>3) - ((int32_t)BME280_DIG_T1 << 1))) * ((int32_t)BME280_DIG_T2)) >> 11;
    var2 = (((((adc_T>>4) - ((int32_t)BME280_DIG_T1)) * ((adc_T>>4) - ((int32_t)BME280_DIG_T1))) >> 12)
            * ((int32_t)BME280_DIG_T3)) >> 14;
    t_fine = var1 + var2;

    T = (t_fine*5 + 128) >> 8;
    return T;
}

