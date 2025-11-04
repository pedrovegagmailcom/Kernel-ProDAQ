#ifndef IO_H
#define IO_H

#include <Arduino.h>
#include <SPI.h>

    #define IODIR   0x00
    #define GPPU    0x06
    #define GPIO    0x09
    #define OLAT    0x0A
    #define IOCON   0x05

    // Opcodes base
    #define MCP23S08_WRITE  0x40
    #define MCP23S08_READ   0x41

class IO {
public:
    IO();                                   // Constructor
    void begin(uint32_t spiClock = 500000); // Inicializa SPI y MCP23S08
    uint16_t readInputs();                  // Lee las 12 entradas (E0–E11)
    void writeOutputs(uint16_t values);     // Escribe las 12 salidas (S0–S11)
    void setOutput(uint8_t channel, bool state); // Cambia una salida individual

private:
    

    // Métodos auxiliares
    void initMCP(pin_size_t  csPin, bool asInput);
    void writeRegister(pin_size_t  csPin, uint8_t reg, uint8_t value);
    uint8_t readRegister(pin_size_t  csPin, uint8_t reg);

    // Direcciones de registro
    
};

#endif
