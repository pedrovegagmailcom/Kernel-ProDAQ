#ifndef AD7175_H
#define AD7175_H

#include <Arduino.h>
#include <SPI.h>
#include "AD7175_regs.h"

// AD7175 Constants
#define AD7175_CRC_POLYNOMIAL 0x07 // x^8 + x^2 + x +1 (MSB first)

// Function Declarations
int32_t AD7175_ReadRegister(st_reg* pReg);
int32_t AD7175_WriteRegister(st_reg reg);
int32_t AD7175_WaitForReady(uint32_t timeout);
int32_t AD7175_ReadData(int32_t* pData);
uint8_t AD717X_ComputeCRC8(uint8_t* pBuf, uint8_t bufSize);
uint8_t AD717X_ComputeXOR8(uint8_t* pBuf, uint8_t bufSize);
int32_t AD7175_Setup(void);
int32_t AD717X_Reset(void);
int32_t AD7175_GetState(uint8_t* st);
int32_t AD717X_Standby(void);
int32_t AD717X_Resume(ad_mode_type_t mode);
int32_t AD717X_Data_output_freq(int channel, int freq_hz);
int32_t AD717X_Calibration(ad_mode_type_t cal_type);

#endif // AD7175_H
