// mcp23s08.h - Archivo de cabecera para MCP23S08

#ifndef MCP23S08_H
#define MCP23S08_H

#include <Arduino.h>

class MCP23S08 {
public:
    MCP23S08(int csPin);
    void begin();
    void writeRegister(byte reg, byte value);
    byte readRegister(byte reg);

private:
    int _csPin;
};

#endif
