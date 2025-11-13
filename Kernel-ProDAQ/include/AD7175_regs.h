#ifndef AD7175_REGS_H
#define AD7175_REGS_H

#include <Arduino.h>
#include "AD717x_common_regs.h"

// AD7175 Registers Structure
typedef struct {
    int32_t addr;
    int32_t value;
    int32_t size;
} st_reg;

// AD7175 Registers Enumeration
enum AD7175_registers {
    Status_Register = 0x00,
    ADC_Mode_Register,
    Interface_Mode_Register,
    Regs_Check,
    Data_Register,
    IOCon_Register,
    ID_st_reg,
    CH_Map_1,
    CH_Map_2,
    CH_Map_3,
    CH_Map_4,
    Setup_Config_1,
    Setup_Config_2,
    Setup_Config_3,
    Setup_Config_4,
    Filter_Config_1,
    Filter_Config_2,
    Filter_Config_3,
    Filter_Config_4,
    Offset_1,
    Offset_2,
    Offset_3,
    Offset_4,
    Gain_1,
    Gain_2,
    Gain_3,
    Gain_4,
    Communications_Register,
    AD7175_REG_NO
};

// Registers Array Initialization
extern st_reg AD7175_regs[AD7175_REG_NO];

// Communication Register bits
#define COMM_REG_WEN    (0 << 7)
#define COMM_REG_WR     (0 << 6)
#define COMM_REG_RD     (1 << 6)

// Status Register bits
#define STATUS_REG_RDY      (1 << 7)
#define STATUS_REG_ADC_ERR  (1 << 6)
#define STATUS_REG_CRC_ERR  (1 << 5)
#define STATUS_REG_REG_ERR  (1 << 4)
#define STATUS_REG_CH(x)    ((x) & 0x03)

// ADC Mode Register bits
#define AD717X_ADCMODE_REG_MODE(x)    (((x) & 0x7) << 4)
#define AD717X_ADCMODE_SING_CYC       (1 << 13)

// Interface Mode Register bits
#define AD717X_IFMODE_REG_XOR_EN      (0x01 << 2)
#define AD717X_IFMODE_REG_DOUT_RESET  (1 << 8)

// GPIO Configuration Register bits
#define AD717X_GPIOCON_REG_ERR_EN(x)  (((x) & 0x3) << 9)

// Filter Configuration Register bits
#define AD717X_FILT_CONF_REG_ODR(x)   (((x) & 0x1F) << 0)

// Calibration Modes
typedef enum {
    ADC_WORK_MODE_CONTINUOUS = 0x0,
    ADC_WORK_MODE_SINGLE     = 0x1,
    ADC_CAL_INTER_OFFSET     = 0x4,
    ADC_CAL_SYS_OFFSET       = 0x6,
    ADC_CAL_SYS_GAIN         = 0x7,
} ad_mode_type_t;

// Frequencies Enumeration
typedef enum {
    AD7172_FREQ_50HZ = 0x10,
    // Add other frequencies as needed
    AD7172_FREQ_MAX,
} ad7172_sin5_sin1_sing_sys_freq_hz_t;

#endif // AD7175_REGS_H
