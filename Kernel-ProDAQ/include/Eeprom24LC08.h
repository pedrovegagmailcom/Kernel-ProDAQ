#pragma once

#include <Arduino.h>
#include <Wire.h>

class Eeprom24LC08 {
public:
    static const uint16_t TOTAL_SIZE = 1024; // 8 Kbit = 1024 bytes
    static const uint8_t  PAGE_SIZE  = 16;   // tamaño de página típico

    // Llama a esto en setup()
    void begin() {
        Wire.begin();
    }

    // Escribe 'length' bytes a partir de eeAddress (gestiona páginas/bloques)
    void writeBytes(uint16_t eeAddress, const uint8_t* data, uint16_t length);

    // Lee 'length' bytes a partir de eeAddress
    void readBytes(uint16_t eeAddress, uint8_t* data, uint16_t length);

    // -------- Helpers genéricos para structs --------
    template <typename T>
    void writeStruct(uint16_t eeAddress, const T& value) {
        writeBytes(eeAddress,
                   reinterpret_cast<const uint8_t*>(&value),
                   sizeof(T));
    }

    template <typename T>
    void readStruct(uint16_t eeAddress, T& value) {
        readBytes(eeAddress,
                  reinterpret_cast<uint8_t*>(&value),
                  sizeof(T));
    }

private:
    // Dirección I2C (0x50..0x53) según la dirección global 0..1023
    uint8_t getDeviceAddr(uint16_t eeAddress) const;

    // Escribe como mucho 'length' bytes sin cruzar página ni bloque.
    // Devuelve cuántos bytes ha escrito realmente.
    uint8_t writeChunk(uint16_t eeAddress, const uint8_t* data, uint8_t length);

    // Lee como mucho 'length' bytes sin cruzar bloque.
    // Devuelve cuántos bytes ha leído realmente.
    uint8_t readChunk(uint16_t eeAddress, uint8_t* data, uint8_t length);
};
