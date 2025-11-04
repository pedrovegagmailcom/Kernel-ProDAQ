// mcp23s08.cpp - Archivo de implementación para controlar MCP23S08

#include "Arduino.h"
#include "mcp23s08.h"
#include <SPI.h>

#define MCP23S08_OPCODE_WRITE 0x40  // Opcode para escritura (A1=A0=0)
#define MCP23S08_OPCODE_READ  0x41  // Opcode para lectura (A1=A0=0)

MCP23S08::MCP23S08(int csPin) {
    _csPin = PC_15;//csPin;
    
}

void MCP23S08::begin() {
    pinMode(PC_15, OUTPUT);
    digitalWrite(PC_15, HIGH);
    SPI.begin();
    
    // Configurar todos los pines como salidas
    //writeRegister(0x00, 0x00);

    // Configurar todos los pines del MCP23S08 como entradas
    writeRegister(0x00, 0xFF);
    // Configurar el registro IOCON para deshabilitar operación secuencial
    writeRegister(0x05, 0x20);
}

void MCP23S08::writeRegister(byte reg, byte value) {
    delay(100);
    digitalWrite(PC_15, LOW);
    SPI.transfer(MCP23S08_OPCODE_WRITE);
    SPI.transfer(reg);
    SPI.transfer(value);
    delay(100);
    digitalWrite(PC_15, HIGH);
}

byte MCP23S08::readRegister(byte reg) {
    delay(100);
    digitalWrite(PC_15, LOW);
    SPI.transfer(MCP23S08_OPCODE_READ);
    SPI.transfer(reg);
    byte value = SPI.transfer(0x00);
    delay(100);
    digitalWrite(PC_15, HIGH);
    return value;
}
