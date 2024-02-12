// multimod_BME280.h
// Date Created: 2023-09-15
// Date Updated: 2023-09-15
// Declarations for BME280 functions

#ifndef MULTIMOD_BME280_H_
#define MULTIMOD_BME280_H_
/************************************Includes***************************************/
#include <stdint.h>
#include <stdbool.h>

#include <inc/tm4c123gh6pm.h>
#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <inc/hw_i2c.h>
#include <inc/hw_gpio.h>

#include <driverlib/gpio.h>
#include <driverlib/sysctl.h>
#include <driverlib/uart.h>
/************************************Includes***************************************/

/*************************************Defines***************************************/
// BME280 Defines
#define BME280_ADDR                 0x77
#define BME280_DEVICEID             0x60


// BME280 Addresses
#define BME280_ID_ADDR              0xD0
#define BME280_RESET_ADDR           0xE0
#define BME280_CTRL_HUM_ADDR        0xF2
#define BME280_STATUS_ADDR          0xF3
#define BME280_CTRL_MEAS_ADDR       0xF4
#define BME280_CONFIG_ADDR          0XF5
#define BME280_PRESS_MSB_ADDR       0XF7
#define BME280_PRESS_LSB_ADDR       0XF8
#define BME280_PRESS_XLSB_ADDR      0XF9
#define BME280_TEMP_MSB_ADDR        0XFA
#define BME280_TEMP_LSB_ADDR        0XFB
#define BME280_TEMP_XLSB_ADDR       0XFC
#define BME280_HUM_MSB_ADDR         0XFD
#define BME280_HUM_lSB_ADDR         0XFE

// BME280 Status Bits
#define BME280_STATUS_MEASURING     0x08

// BME280 Trimming Constants
#define BME280_DIG_T1               0x6DFB
#define BME280_DIG_T2               0x65B1
#define BME280_DIG_T3               0x0032

/*************************************Defines***************************************/

/******************************Data Type Definitions********************************/
/******************************Data Type Definitions********************************/

/****************************Data Structure Definitions*****************************/
/****************************Data Structure Definitions*****************************/

/***********************************Externs*****************************************/
/***********************************Externs*****************************************/

/********************************Public Variables***********************************/
/********************************Public Variables***********************************/

/********************************Public Functions***********************************/
void BME280_Init(void);
void BME280_WriteRegister(uint8_t addr, uint8_t data);
uint8_t BME280_ReadRegister(uint8_t addr);
void BME280_MultiReadRegister(uint8_t addr, uint8_t* data, uint8_t num_bytes);
int32_t BME280_TempGetResult();
uint8_t BME280_GetDataStatus();
int32_t BME280_compensate_T_int32(uint32_t adc_T);
/********************************Public Functions***********************************/

/*******************************Private Variables***********************************/
/*******************************Private Variables***********************************/

/*******************************Private Functions***********************************/
/*******************************Private Functions***********************************/

#endif /* MULTIMOD_BME280_H_ */
