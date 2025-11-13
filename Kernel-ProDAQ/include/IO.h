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
    
    void initMCP1(bool asInput);
    void initMCP2(bool asInput);
    void initMCP3(bool asInput);
    void initMCP4(bool asInput);
    void writeRegister1(uint8_t reg, uint8_t value);
    void writeRegister2(uint8_t reg, uint8_t value);
    void writeRegister3(uint8_t reg, uint8_t value);
    void writeRegister4(uint8_t reg, uint8_t value);
    uint8_t readRegister1(uint8_t reg);
    uint8_t readRegister2(uint8_t reg);
    uint8_t readRegister3(uint8_t reg);
    uint8_t readRegister4(uint8_t reg);
    
    
};

#endif
