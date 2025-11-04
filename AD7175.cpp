//#include "PinNames.h"
#include "AD7175.h"


st_reg AD7175_regs[] = 
{
    /* Dirección  , Valor    , Tamaño en bytes */
    /*0x00*/ {AD717X_STATUS_REG     , 0x00    , 1}, // Status_Register
    /*0x01*/ {AD717X_ADCMODE_REG    , 0x802C  , 2}, // ADC_Mode_Register
    /*0x02*/ {AD717X_IFMODE_REG     , 0x0104  , 2}, // Interface_Mode_Reg
    /*0x03*/ {AD717X_REGCHECK_REG   , 0x000000, 3}, // REGCHECK
    /*0x04*/ {AD717X_DATA_REG       , 0x000000, 3}, // Data_Register
    /*0x06*/ {AD717X_GPIOCON_REG    , 0x0400  , 2}, // IOCon_Register
    /*0x07*/ {AD717X_ID_REG         , 0x0000  , 2}, // ID_st_reg
    /*0x10*/ {AD717X_CHMAP0_REG     , 0x8001  , 2}, // CH_Map_1
    /*0x11*/ {AD717X_CHMAP1_REG     , 0x0000  , 2}, // CH_Map_2
    /*0x12*/ {AD717X_CHMAP2_REG     , 0x0000  , 2}, // CH_Map_3
    /*0x13*/ {AD717X_CHMAP3_REG     , 0x0000  , 2}, // CH_Map_4
    /*0x20*/ {AD717X_SETUPCON0_REG  , 0x1320  , 2}, // Setup_Config_1
    /*0x21*/ {AD717X_SETUPCON1_REG  , 0x0000  , 2}, // Setup_Config_2
    /*0x22*/ {AD717X_SETUPCON2_REG  , 0x0000  , 2}, // Setup_Config_3
    /*0x23*/ {AD717X_SETUPCON3_REG  , 0x0000  , 2}, // Setup_Config_4
    /*0x28*/ {AD717X_FILTCON0_REG   , 0x0007  , 2}, // Filter_Config_1
    /*0x29*/ {AD717X_FILTCON1_REG   , 0x0007  , 2}, // Filter_Config_2
    /*0x2A*/ {AD717X_FILTCON2_REG   , 0x0007  , 2}, // Filter_Config_3
    /*0x2B*/ {AD717X_FILTCON3_REG   , 0x0007  , 2}, // Filter_Config_4
    /*0x30*/ {AD717X_OFFSET0_REG    , 0x800000, 3}, // Offset_1
    /*0x31*/ {AD717X_OFFSET1_REG    , 0x800000, 3}, // Offset_2
    /*0x32*/ {AD717X_OFFSET2_REG    , 0x800000, 3}, // Offset_3
    /*0x33*/ {AD717X_OFFSET3_REG    , 0x800000, 3}, // Offset_4
    /*0x38*/ {AD717X_GAIN0_REG      , 0x555555, 3}, // Gain_1
    /*0x39*/ {AD717X_GAIN1_REG      , 0x555555, 3}, // Gain_2
    /*0x3A*/ {AD717X_GAIN2_REG      , 0x555555, 3}, // Gain_3
    /*0x3B*/ {AD717X_GAIN3_REG      , 0x555555, 3}, // Gain_4
    /*0xFF*/ {0xFF                  , 0x00    , 1}, // Communications_Register (Verificar dirección correcta)
};


// AD7175 State Structure
struct AD7175_state {
    uint8_t useCRC;
} AD7175_st;

// Chip Select Pin
//#define AD7175_CS_PIN 10 // Adjust according to your wiring
#define AD7175_CS_PIN PG_10

// Initialize SPI Settings
SPISettings spiSettings(8000000, MSBFIRST, SPI_MODE3);

int32_t AD7175_ReadRegister(st_reg* pReg) {
    uint8_t buffer[8] = {0};
    uint8_t crc = 0;

    // Build the Command word
    buffer[0] = COMM_REG_WEN | COMM_REG_RD | pReg->addr;

    // Begin SPI Transaction
    SPI.beginTransaction(spiSettings);
    digitalWrite(AD7175_CS_PIN, LOW);

    // Send Command and read data
    SPI.transfer(buffer[0]);
    for (uint8_t i = 1; i <= pReg->size; i++) {
        buffer[i] = SPI.transfer(0x00);
    }

    // Read CRC if enabled
    if (AD7175_st.useCRC) {
        buffer[pReg->size + 1] = SPI.transfer(0x00);
        crc = AD717X_ComputeCRC8(buffer, pReg->size + 2);
        if (crc != 0) {
            SPI.endTransaction();
            digitalWrite(AD7175_CS_PIN, HIGH);
            return -1; // CRC Error
        }
    }

    // End SPI Transaction
    digitalWrite(AD7175_CS_PIN, HIGH);
    SPI.endTransaction();

    // Build the result
    pReg->value = 0;
    for (uint8_t i = 1; i <= pReg->size; i++) {
        pReg->value <<= 8;
        pReg->value |= buffer[i];
    }

    return 0;
}

int32_t AD7175_WriteRegister(st_reg reg) {
    uint8_t wrBuf[8] = {0};
    uint8_t crc = 0;

    // Build the Command word
    wrBuf[0] = COMM_REG_WEN | COMM_REG_WR | reg.addr;

    // Fill the write buffer
    int32_t regValue = reg.value;
    for (uint8_t i = 0; i < reg.size; i++) {
        wrBuf[reg.size - i] = regValue & 0xFF;
        regValue >>= 8;
    }

    // Compute the CRC if enabled
    if (AD7175_st.useCRC) {
        crc = AD717X_ComputeCRC8(wrBuf, reg.size + 1);
        wrBuf[reg.size + 1] = crc;
    }

    // Begin SPI Transaction
    SPI.beginTransaction(spiSettings);
    digitalWrite(AD7175_CS_PIN, LOW);

    // Send Command and data
    for (uint8_t i = 0; i < (AD7175_st.useCRC ? reg.size + 2 : reg.size + 1); i++) {
        SPI.transfer(wrBuf[i]);
    }

    // End SPI Transaction
    digitalWrite(AD7175_CS_PIN, HIGH);
    SPI.endTransaction();

    return 0;
}

int32_t AD7175_WaitForReady(uint32_t timeout) {
    uint8_t ready = 0;
    while (!ready && timeout--) {
        if (AD7175_ReadRegister(&AD7175_regs[Status_Register]) != 0)
            return -1;
        ready = !(AD7175_regs[Status_Register].value & STATUS_REG_RDY);
        delay(1); // Delay 1ms
    }
    return ready ? 0 : -1;
}

int32_t AD7175_GetState(uint8_t* st) {
    if (AD7175_ReadRegister(&AD7175_regs[Status_Register]) != 0)
        return -1;
    *st = AD7175_regs[Status_Register].value;
    return 0;
}

int32_t AD7175_ReadData(int32_t* pData) {
    if (AD7175_ReadRegister(&AD7175_regs[Data_Register]) != 0)
        return -1;
    int32_t v = AD7175_regs[Data_Register].value & 0xFFFFFF;
    if (v & 0x800000) 
        v |= 0xFF000000;  // sign-extend
    *pData = v;
    return 0;
}

uint8_t AD717X_ComputeCRC8(uint8_t* pBuf, uint8_t bufSize) {
    uint8_t crc = 0;
    while (bufSize) {
        for (uint8_t i = 0x80; i != 0; i >>= 1) {
            if (((crc & 0x80) != 0) != ((*pBuf & i) != 0)) {
                crc <<= 1;
                crc ^= AD7175_CRC_POLYNOMIAL;
            } else {
                crc <<= 1;
            }
        }
        pBuf++;
        bufSize--;
    }
    return crc;
}

uint8_t AD717X_ComputeXOR8(uint8_t* pBuf, uint8_t bufSize) {
    uint8_t xorVal = 0;
    while (bufSize) {
        xorVal ^= *pBuf;
        pBuf++;
        bufSize--;
    }
    return xorVal;
}

int32_t AD717X_Reset(void) {
    uint8_t wrBuf[8] = {0xFF};

    // Begin SPI Transaction
    SPI.beginTransaction(spiSettings);
    digitalWrite(AD7175_CS_PIN, LOW);

    // Send 64 ones to reset
    for (uint8_t i = 0; i < 8; i++) {
        SPI.transfer(wrBuf[0]);
    }

    // End SPI Transaction
    digitalWrite(AD7175_CS_PIN, HIGH);
    SPI.endTransaction();

    delay(10); // Wait for reset to complete

    return 0;
}

int32_t AD717X_Standby(void) {
    AD7175_regs[ADC_Mode_Register].value &= ~AD717X_ADCMODE_REG_MODE(0x7);
    AD7175_regs[ADC_Mode_Register].value |= AD717X_ADCMODE_REG_MODE(0x2);
    return AD7175_WriteRegister(AD7175_regs[ADC_Mode_Register]);
}

int32_t AD717X_Resume(ad_mode_type_t work_mode) {
    if (work_mode > ADC_WORK_MODE_SINGLE) {
        return -1;
    }
    AD7175_regs[ADC_Mode_Register].value &= ~AD717X_ADCMODE_REG_MODE(0x7);
    AD7175_regs[ADC_Mode_Register].value |= AD717X_ADCMODE_REG_MODE(work_mode);
    return AD7175_WriteRegister(AD7175_regs[ADC_Mode_Register]);
}

int32_t AD717X_Data_output_freq(int channel, int freq_hz) {
    if (freq_hz >= AD7172_FREQ_MAX) {
        return -1;
    }
    st_reg* filterReg = nullptr;
    switch (channel) {
        case 0: filterReg = &AD7175_regs[Filter_Config_1]; break;
        case 1: filterReg = &AD7175_regs[Filter_Config_2]; break;
        case 2: filterReg = &AD7175_regs[Filter_Config_3]; break;
        case 3: filterReg = &AD7175_regs[Filter_Config_4]; break;
        default: return -1;
    }
    filterReg->value &= ~AD717X_FILT_CONF_REG_ODR(0x1F);
    filterReg->value |= AD717X_FILT_CONF_REG_ODR(freq_hz);
    return AD7175_WriteRegister(*filterReg);
}

int32_t AD717X_Calibration(ad_mode_type_t cal_type) {
    if (cal_type < ADC_CAL_INTER_OFFSET || cal_type > ADC_CAL_SYS_GAIN) {
        return -1;
    }
    AD7175_regs[ADC_Mode_Register].value &= ~AD717X_ADCMODE_REG_MODE(0x7);
    AD7175_regs[ADC_Mode_Register].value |= AD717X_ADCMODE_REG_MODE(cal_type);
    return AD7175_WriteRegister(AD7175_regs[ADC_Mode_Register]);
}

int32_t AD7175_Setup(void) {
    // Initialize SPI and CS Pin
    pinMode(AD7175_CS_PIN, OUTPUT);
    digitalWrite(AD7175_CS_PIN, HIGH);
    SPI.begin();
Serial.print("AD7175 0");
    // Reset the device
    
    AD717X_Reset();

    // Read ID register
    if (AD7175_ReadRegister(&AD7175_regs[ID_st_reg]) != 0)
        return -1;



Serial.print("AD7175 ID : ");
Serial.println(AD7175_regs[ID_st_reg].value);

Serial.println("AD7175 1");

    // Configure ADC Mode Register
    AD7175_regs[ADC_Mode_Register].value |= AD717X_ADCMODE_SING_CYC;
    if (AD7175_WriteRegister(AD7175_regs[ADC_Mode_Register]) != 0)
        return -1;

Serial.println("AD7175 2");

    // Configure Interface Mode Register
    AD7175_regs[Interface_Mode_Register].value &= ~(/*CRC_EN|*/ AD717X_IFMODE_REG_XOR_EN);
    AD7175_WriteRegister(AD7175_regs[Interface_Mode_Register]);
    AD7175_st.useCRC = 0; // deshabilitar comprobación

Serial.println("AD7175 3");

    // Configure GPIO
    AD7175_regs[IOCon_Register].value |= AD717X_GPIOCON_REG_ERR_EN(2);
    if (AD7175_WriteRegister(AD7175_regs[IOCon_Register]) != 0)
        return -1;

Serial.println("AD7175 4");

    // Initialize Setup Configuration Registers
    if (AD7175_WriteRegister(AD7175_regs[Setup_Config_1]) != 0)
        return -1;

Serial.println("AD7175 5");

    // Initialize Filter Configuration Registers
    if (AD7175_WriteRegister(AD7175_regs[Filter_Config_1]) != 0)
        return -1;

Serial.println("AD7175 6");

    // Initialize Channel Map Registers
    if (AD7175_WriteRegister(AD7175_regs[CH_Map_1]) != 0)
        return -1;

Serial.println("AD7175 7");

    // Perform Calibration
    uint64_t offset_sum = 0;
    for (int i = 0; i < 16; i++) {
        AD717X_Calibration(ADC_CAL_INTER_OFFSET);
        delay(10);
        if (AD7175_ReadRegister(&AD7175_regs[Offset_1]) != 0)
            return -1;
        offset_sum += AD7175_regs[Offset_1].value;
    }
    AD7175_regs[Offset_1].value = static_cast<int32_t>(offset_sum / 16);
    if (AD7175_WriteRegister(AD7175_regs[Offset_1]) != 0)
        return -1;

    // Resume Continuous Conversion Mode
    AD717X_Resume(ADC_WORK_MODE_CONTINUOUS);

    return 0;
}
