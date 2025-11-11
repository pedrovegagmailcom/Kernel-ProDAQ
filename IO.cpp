#include "Arduino.h"
#include "IO.h"
#include <SPI.h>

// Definición de pines (ajusta según tu conexión real)
pin_size_t CS1 = PC_6;   // Entradas 0–7
pin_size_t CS2 = PH_15;  // Entradas 8–11
pin_size_t CS3 = PA_6;   // Salidas 0–7
pin_size_t CS4 = PK_1;   // Salidas 8–11

// Constructor
IO::IO() {}

// Inicialización completa: SPI + los 4 MCP23S08
void IO::begin(uint32_t spiClock) {
 
    

    initMCP(CS1, true);   // Entradas 0–7
    initMCP(CS2, true);   // Entradas 8–11
    initMCP(CS3, false);  // Salidas 0–7
    initMCP(CS4, false);  // Salidas 8–11



    // Habilitar pull-ups en las entradas
    writeRegister(CS1, GPPU, 0xFF);
    writeRegister(CS2, GPPU, 0x0F);
}

// ===================== MÉTODOS AUXILIARES =====================

void IO::initMCP(pin_size_t  csPin, bool asInput) {

    pinMode(csPin, OUTPUT);

    
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(csPin, LOW);
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
    digitalWrite(csPin, HIGH);

    
    
    // Configurar el registro IOCON para deshabilitar operación secuencial
    writeRegister(csPin, IOCON, 0x30);

    // IOCON: habilitar HAEN (bit 3)
    //writeRegister(csPin, hwAddr, IOCON, 0b00001000);

    // IODIR: 1 = entrada, 0 = salida
    writeRegister(csPin, IODIR, asInput ? 0xFF : 0x00);

    if (!asInput) writeRegister(csPin, OLAT, 0x00);
}

void IO::writeRegister(pin_size_t  csPin, uint8_t reg, uint8_t value) {
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

    digitalWrite(csPin, LOW);
    SPI.transfer(MCP23S08_WRITE); 
    SPI.transfer(reg);
    SPI.transfer(value);
    digitalWrite(csPin, HIGH);

    SPI.endTransaction();
}

uint8_t IO::readRegister(pin_size_t  csPin, uint8_t reg) {
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));    

    digitalWrite(csPin, LOW);
    SPI.transfer(MCP23S08_READ);
    SPI.transfer(reg);
    uint8_t val = SPI.transfer(0x00);
    digitalWrite(csPin, HIGH);
    
    SPI.endTransaction();
    return val;
}

// ===================== FUNCIONES PÚBLICAS =====================

// Leer 12 entradas (E0–E11)
uint16_t IO::readInputs() {
    uint8_t inLow  = readRegister(CS1, GPIO);
    uint8_t inHigh = readRegister(CS2, GPIO) & 0x0F;
    return ((uint16_t)inHigh << 8) | inLow;
}

// Escribir 12 salidas (S0–S11)
void IO::writeOutputs(uint16_t values) {
    uint8_t outLow  = values & 0xFF;
    uint8_t outHigh = (values >> 8) & 0xFF;
    writeRegister(CS3, OLAT, outLow);
    writeRegister(CS4, OLAT, outHigh);
   
}

// Cambiar una salida individual
void IO::setOutput(uint8_t channel, bool state) {
    if (channel > 11) return;
    uint16_t current = 0;
    uint8_t outLow  = readRegister(CS3, OLAT);
    uint8_t outHigh = readRegister(CS4, OLAT) & 0x0F;
    current = ((uint16_t)outHigh << 8) | outLow;

    if (state)
        current |= (1 << channel);
    else
        current &= ~(1 << channel);

    writeOutputs(current);
}
