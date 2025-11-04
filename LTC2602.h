#ifndef LTC2602_DRIVER_H
#define LTC2602_DRIVER_H

#include <Arduino.h>

// Definir el pin de Chip Select para el LTC2602
#ifndef LTC2602_CS_PIN
#define LTC2602_CS_PIN PB_8
#endif

// Comandos y configuraciones para LTC2602
// El LTC2602 es un DAC de 16 bits con dos canales: A y B.
// Formato de datos: [4 bits de comando][16 bits de dato][4 bits don't care]

// Comandos (tabla 1 del datasheet):
#define CMD_WRITE_TO_INPUT_REG        0x0  // 0b0000
#define CMD_UPDATE_DAC_REG            0x1  // 0b0001
#define CMD_WRITE_INPUT_UPDATE_ALL    0x2  // 0b0010
#define CMD_WRITE_INPUT_UPDATE_SINGLE 0x3  // 0b0011


class LTC2602 {
public:
    // Constructor
    LTC2602(uint8_t csPin = LTC2602_CS_PIN);

    // Inicializar el LTC2602 (SPI, pin CS, etc.)
    void begin(uint32_t spiClock = 4000000);

    // Escribir y actualizar un canal (0 = A, 1 = B)
    void setOutput(uint8_t channel, uint16_t value);

private:
    uint8_t _csPin;
};

#endif