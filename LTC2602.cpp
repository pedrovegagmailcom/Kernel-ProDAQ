#include "LTC2602.h"
#include <SPI.h>

// Constructor
LTC2602::LTC2602(uint8_t csPin) : _csPin(csPin) {
}

// Inicializar el LTC2602 (SPI, pin CS, etc.)
void LTC2602::begin(uint32_t spiClock) {
    // Iniciar SPI
    SPI.begin();

    // Configurar el pin CS como salida
    //pinMode(_csPin, OUTPUT);
    //digitalWrite(_csPin, HIGH);
    pinMode(PB_8, OUTPUT);
    digitalWrite(PB_8, HIGH);

    // Configurar la transacción SPI
    // Modo 0 (CPOL = 0, CPHA = 0), MSB first
    // Frecuencia por defecto 4MHz, se puede ajustar si se requiere
    //SPISettings spiSettings(8000000, MSBFIRST, SPI_MODE3);
    //SPI.beginTransaction(SPISettings(spiClock, MSBFIRST, SPI_MODE0));
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE3));
}

// Escribir y actualizar un canal (0 = A, 1 = B)
void LTC2602::setOutput(uint8_t channel, uint16_t value) {
    // El comando Write & Update DAC = 0x3 (CMD_WRITE_INPUT_UPDATE_SINGLE)
    // En la hoja de datos, Table 1, la parte baja del nibble selecciona el canal.
    //    - Canal A -> 0
    //    - Canal B -> 1
    // EJ:
    //    Write & Update DAC A: 0x30 (0b0011_0000)
    //    Write & Update DAC B: 0x31 (0b0011_0001)

    uint8_t command = (CMD_WRITE_INPUT_UPDATE_SINGLE << 4) | (channel & 0x01);

    // Dividir el valor de 16 bits en dos bytes
    uint8_t highByte = (value >> 8) & 0xFF;
    uint8_t lowByte = value & 0xFF;

    // Activar CS
    //digitalWrite(_csPin, LOW);
    digitalWrite(PB_8, LOW);

    // Enviar el comando y los dos bytes de datos
    SPI.transfer(command);
    SPI.transfer(highByte);
    SPI.transfer(lowByte);

    // Desactivar CS
    digitalWrite(PB_8, HIGH);
}
