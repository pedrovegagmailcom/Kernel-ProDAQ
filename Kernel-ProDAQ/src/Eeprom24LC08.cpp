#include "Eeprom24LC08.h"

uint8_t Eeprom24LC08::getDeviceAddr(uint16_t eeAddress) const {
    uint8_t block = (eeAddress >> 8) & 0x03;  // 4 bloques de 256 bytes
    return 0x50 | block;                      // 0x50, 0x51, 0x52, 0x53
}

uint8_t Eeprom24LC08::writeChunk(uint16_t eeAddress, const uint8_t* data, uint8_t length) {
    if (eeAddress >= TOTAL_SIZE || length == 0) return 0;

    // Ajustar por tamaño total
    if (eeAddress + length > TOTAL_SIZE) {
        length = TOTAL_SIZE - eeAddress;
    }

    uint8_t devAddr  = getDeviceAddr(eeAddress);
    uint8_t wordAddr = eeAddress & 0xFF;  // 0..255 dentro del bloque

    // No cruzar límite de bloque (256 bytes)
    uint8_t blockRemaining = 256 - wordAddr;
    if (length > blockRemaining) {
        length = blockRemaining;
    }

    // No cruzar límite de página (PAGE_SIZE)
    uint8_t pageOffset    = wordAddr % PAGE_SIZE;
    uint8_t pageRemaining = PAGE_SIZE - pageOffset;
    if (length > pageRemaining) {
        length = pageRemaining;
    }

    Wire.beginTransmission(devAddr);
    Wire.write(wordAddr);  // dirección dentro del bloque
    for (uint8_t i = 0; i < length; i++) {
        Wire.write(data[i]);
    }
    Wire.endTransmission();

    return length;
}

uint8_t Eeprom24LC08::readChunk(uint16_t eeAddress, uint8_t* data, uint8_t length) {
    if (eeAddress >= TOTAL_SIZE || length == 0) return 0;

    if (eeAddress + length > TOTAL_SIZE) {
        length = TOTAL_SIZE - eeAddress;
    }

    uint8_t devAddr  = getDeviceAddr(eeAddress);
    uint8_t wordAddr = eeAddress & 0xFF;

    // No cruzar límite de bloque
    uint8_t blockRemaining = 256 - wordAddr;
    if (length > blockRemaining) {
        length = blockRemaining;
    }

    Wire.beginTransmission(devAddr);
    Wire.write(wordAddr);
    Wire.endTransmission();

    Wire.requestFrom((int)devAddr, (int)length);

    uint8_t i = 0;
    while (Wire.available() && i < length) {
        data[i++] = Wire.read();
    }

    return i;
}

void Eeprom24LC08::writeBytes(uint16_t eeAddress, const uint8_t* data, uint16_t length) {
    while (length > 0) {
        uint8_t written = writeChunk(eeAddress, data, (length > 255) ? 255 : (uint8_t)length);
        if (written == 0) break;

        delay(10);  // tiempo de escritura interna de la EEPROM

        eeAddress += written;
        data      += written;
        length    -= written;
    }
}

void Eeprom24LC08::readBytes(uint16_t eeAddress, uint8_t* data, uint16_t length) {
    while (length > 0) {
        uint8_t read = readChunk(eeAddress, data, (length > 255) ? 255 : (uint8_t)length);
        if (read == 0) break;

        eeAddress += read;
        data      += read;
        length    -= read;
    }
}
