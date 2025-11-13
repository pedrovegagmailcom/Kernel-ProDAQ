#include "Arduino.h"
#include "IO.h"
#include <SPI.h>

// Definición de pines (ajusta según tu conexión real)
#define CS1  PC_6   // Entradas 0–7
#define CS2  PH_15  // Entradas 8–11
#define CS3  PA_6   // Salidas 0–7
#define CS4  PK_1   // Salidas 8–11

// Constructor
IO::IO() {}

// Inicialización completa: SPI + los 4 MCP23S08
void IO::begin(uint32_t spiClock) {
 
    
    initMCP1(true);   // Entradas 0–7
    initMCP2(true);   // Entradas 8–11
    initMCP3(false);   // Entradas 0–7
    initMCP4(false);   // Entradas 8–11


    // Habilitar pull-ups en las entradas
    writeRegister1(GPPU, 0xFF);
    writeRegister2(GPPU, 0x0F);
}

// ===================== MÉTODOS AUXILIARES =====================


void IO::initMCP1(bool asInput) {

    pinMode(CS1, OUTPUT);

    
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(CS1, LOW);
    
    SPI.transfer(MCP23S08_WRITE);   // OPCODE write
    SPI.transfer(IODIR);            // Dirección inicial 0x00

    SPI.transfer(0xFF);             // IODIR (por defecto son entradas)
    SPI.transfer(0x00);             // IPOL
    SPI.transfer(0x00);             // GPINTEN
    SPI.transfer(0x00);             // DEFVAL
    SPI.transfer(0x00);             // INTCON
    SPI.transfer(0x00);             // IOCON
    SPI.transfer(0x00);             // GPPU
    SPI.transfer(0x00);             // INTF
    SPI.transfer(0x00);             // INTCAP
    SPI.transfer(0x00);             // GPIO
    SPI.transfer(0x00);             // OLAT

    SPI.endTransaction();
    digitalWrite(CS1, HIGH);

    
    
    // Configurar el registro IOCON para deshabilitar operación secuencial
    writeRegister1(IOCON, 0x20);

    // IODIR: 1 = entrada, 0 = salida
    writeRegister1(IODIR, asInput ? 0xFF : 0x00);

    
}

void IO::initMCP2(bool asInput) {
    pinMode(CS2, OUTPUT);
    
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(CS2, LOW);
    SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
    SPI.transfer(MCP23S08_WRITE);   // OPCODE write
    SPI.transfer(IODIR);            // Dirección inicial 0x00

    SPI.transfer(0xFF);             // IODIR (por defecto son entradas)
    SPI.transfer(0x00);             // IPOL
    SPI.transfer(0x00);             // GPINTEN
    SPI.transfer(0x00);             // DEFVAL
    SPI.transfer(0x00);             // INTCON
    SPI.transfer(0x00);             // IOCON
    SPI.transfer(0x00);             // GPPU
    SPI.transfer(0x00);             // INTF
    SPI.transfer(0x00);             // INTCAP
    SPI.transfer(0x00);             // GPIO
    SPI.transfer(0x00);             // OLAT

    SPI.endTransaction();
    digitalWrite(CS2, HIGH);
   
    // Configurar el registro IOCON para deshabilitar operación secuencial
    writeRegister2(IOCON, 0x20);

    // IODIR: 1 = entrada, 0 = salida
    writeRegister2(IODIR, asInput ? 0xFF : 0x00);
 
}

void IO::initMCP3(bool asInput) {
    pinMode(CS3, OUTPUT);
    
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(CS3, LOW);
    SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
    SPI.transfer(MCP23S08_WRITE);   // OPCODE write
    SPI.transfer(IODIR);            // Dirección inicial 0x00

    SPI.transfer(0xFF);             // IODIR (por defecto son entradas)
    SPI.transfer(0x00);             // IPOL
    SPI.transfer(0x00);             // GPINTEN
    SPI.transfer(0x00);             // DEFVAL
    SPI.transfer(0x00);             // INTCON
    SPI.transfer(0x00);             // IOCON
    SPI.transfer(0x00);             // GPPU
    SPI.transfer(0x00);             // INTF
    SPI.transfer(0x00);             // INTCAP
    SPI.transfer(0x00);             // GPIO
    SPI.transfer(0x00);             // OLAT

    SPI.endTransaction();
    digitalWrite(CS3, HIGH);
   
    // Configurar el registro IOCON para deshabilitar operación secuencial
    writeRegister3(IOCON, 0x20);

    // IODIR: 1 = entrada, 0 = salida
    writeRegister3(IODIR, asInput ? 0xFF : 0x00);
 
}

void IO::initMCP4(bool asInput) {
    pinMode(CS4, OUTPUT);
    
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(CS4, LOW);
    SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
    SPI.transfer(MCP23S08_WRITE);   // OPCODE write
    SPI.transfer(IODIR);            // Dirección inicial 0x00

    SPI.transfer(0xFF);             // IODIR (por defecto son entradas)
    SPI.transfer(0x00);             // IPOL
    SPI.transfer(0x00);             // GPINTEN
    SPI.transfer(0x00);             // DEFVAL
    SPI.transfer(0x00);             // INTCON
    SPI.transfer(0x00);             // IOCON
    SPI.transfer(0x00);             // GPPU
    SPI.transfer(0x00);             // INTF
    SPI.transfer(0x00);             // INTCAP
    SPI.transfer(0x00);             // GPIO
    SPI.transfer(0x00);             // OLAT

    SPI.endTransaction();
    digitalWrite(CS4, HIGH);
   
    // Configurar el registro IOCON para deshabilitar operación secuencial
    writeRegister4(IOCON, 0x20);

    // IODIR: 1 = entrada, 0 = salida
    writeRegister4(IODIR, asInput ? 0xFF : 0x00);
 
}



void IO::writeRegister1(uint8_t reg, uint8_t value) {
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

    digitalWrite(CS1, LOW);
    SPI.transfer(MCP23S08_WRITE); 
    SPI.transfer(reg);
    SPI.transfer(value);
    digitalWrite(CS1, HIGH);

    SPI.endTransaction();
}

void IO::writeRegister2(uint8_t reg, uint8_t value) {
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

    digitalWrite(CS2, LOW);
    SPI.transfer(MCP23S08_WRITE); 
    SPI.transfer(reg);
    SPI.transfer(value);
    digitalWrite(CS2, HIGH);

    SPI.endTransaction();
}

void IO::writeRegister3(uint8_t reg, uint8_t value) {
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

    digitalWrite(CS3, LOW);
    SPI.transfer(MCP23S08_WRITE); 
    SPI.transfer(reg);
    SPI.transfer(value);
    digitalWrite(CS3, HIGH);

    SPI.endTransaction();
}

void IO::writeRegister4(uint8_t reg, uint8_t value) {
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

    digitalWrite(CS4, LOW);
    SPI.transfer(MCP23S08_WRITE); 
    SPI.transfer(reg);
    SPI.transfer(value);
    digitalWrite(CS4, HIGH);

    SPI.endTransaction();
}

uint8_t IO::readRegister1(uint8_t reg) {
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));    

    digitalWrite(CS1, LOW);
    SPI.transfer(MCP23S08_READ);
    SPI.transfer(reg);
    uint8_t val = SPI.transfer(0x00);
    digitalWrite(CS1, HIGH);
    
    SPI.endTransaction();
    return val;
}

uint8_t IO::readRegister2(uint8_t reg) {
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));    

    digitalWrite(CS2, LOW);
    SPI.transfer(MCP23S08_READ);
    SPI.transfer(reg);
    uint8_t val = SPI.transfer(0x00);
    digitalWrite(CS2, HIGH);
    
    SPI.endTransaction();
    return val;
}


uint8_t IO::readRegister3(uint8_t reg) {
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));    

    digitalWrite(CS3, LOW);
    SPI.transfer(MCP23S08_READ);
    SPI.transfer(reg);
    uint8_t val = SPI.transfer(0x00);
    digitalWrite(CS3, HIGH);
    
    SPI.endTransaction();
    return val;
}

uint8_t IO::readRegister4(uint8_t reg) {
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));    

    digitalWrite(CS4, LOW);
    SPI.transfer(MCP23S08_READ);
    SPI.transfer(reg);
    uint8_t val = SPI.transfer(0x00);
    digitalWrite(CS4, HIGH);
    
    SPI.endTransaction();
    return val;
}
// ===================== FUNCIONES PÚBLICAS =====================

// Leer 12 entradas (E0–E11)
uint16_t IO::readInputs() {
    uint8_t inLow  = readRegister1(GPIO);
    uint8_t inHigh = readRegister2(GPIO) & 0x0F;
    return ((uint16_t)inHigh << 8) | inLow;
}

// Escribir 12 salidas (S0–S11)
void IO::writeOutputs(uint16_t values) {
    uint8_t outLow  = values & 0xFF;
    uint8_t outHigh = (values >> 8) & 0xFF;
    writeRegister3(OLAT, outLow);
    writeRegister4(OLAT, outHigh);
   
}

// Cambiar una salida individual
void IO::setOutput(uint8_t channel, bool state) {
    if (channel > 11) return;
    uint16_t current = 0;
    uint8_t outLow  = readRegister3(OLAT);
    uint8_t outHigh = readRegister4(OLAT) & 0x0F;
    current = ((uint16_t)outHigh << 8) | outLow;

    if (state)
        current |= (1 << channel);
    else
        current &= ~(1 << channel);

    writeOutputs(current);
}
