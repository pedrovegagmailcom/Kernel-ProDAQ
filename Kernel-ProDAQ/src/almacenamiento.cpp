#include <Arduino.h>
#include <BlockDevice.h>
#include <QSPIFBlockDevice.h>

#include "almacenamiento.h"

// QSPIFBlockDevice global interno a la librería (Portenta H7, pines por defecto)
static QSPIFBlockDevice root(QSPI_FLASH1_IO0, QSPI_FLASH1_IO1, QSPI_FLASH1_IO2, QSPI_FLASH1_IO3,
                             QSPI_FLASH1_SCK, QSPI_FLASH1_CSN,
                             QSPIF_POLARITY_MODE_0, MBED_CONF_QSPIF_QSPI_FREQ);

static bool rootInitialized = false;

ConfigStorage::ConfigStorage(size_t dataSize,
                                         uint32_t magic,
                                         uint32_t baseAddress)
    : _base(baseAddress),
      _dataSize(dataSize),
      _magic(magic),
      _eraseSize(0),
      _initialized(false),
      _valid(false)
{}

bool ConfigStorage::begin() {
    if (!rootInitialized) {
        int err = root.init();
        if (err != 0) {
            Serial.print("SimpleConfigStorage: error inicializando QSPI (");
            Serial.print(err);
            Serial.println(")");
            return false;
        }
        rootInitialized = true;
    }

    _eraseSize = root.get_erase_size(_base);
    if (_eraseSize == 0) {
        Serial.println("ConfigStorage: eraseSize = 0");
        return false;
    }

    _initialized = true;
    return true;
}

void ConfigStorage::end() {
    if (rootInitialized) {
        root.deinit();
        rootInitialized = false;
    }
    _initialized = false;
    _valid = false;
}

uint16_t ConfigStorage::calcChecksum(const uint8_t *data, size_t len) const {
    uint16_t sum = 0;
    for (size_t i = 0; i < len; ++i) {
        sum += data[i];
    }
    return sum;
}

bool ConfigStorage::save(const void *data) {
    if (!_initialized) {
        Serial.println("SimpleConfigStorage: llama a begin() antes de save()");
        return false;
    }

    const size_t totalSize = sizeof(Header) + _dataSize;
    uint8_t *buffer = new (std::nothrow) uint8_t[totalSize];
    if (!buffer) {
        Serial.println("SimpleConfigStorage: sin memoria para buffer");
        return false;
    }

    // Rellenar cabecera
    Header header;
    header.magic    = _magic;
    header.checksum = calcChecksum(reinterpret_cast<const uint8_t *>(data), _dataSize);
    header.reserved = 0;

    // Copiar cabecera + datos al buffer
    memcpy(buffer, &header, sizeof(Header));
    memcpy(buffer + sizeof(Header), data, _dataSize);

    // Borrar sector
    int err = root.erase(_base, _eraseSize);
    if (err != 0) {
        Serial.print("SimpleConfigStorage: error borrando sector (");
        Serial.print(err);
        Serial.println(")");
        delete[] buffer;
        return false;
    }

    // Escribir
    err = root.program(buffer, _base, totalSize);
    delete[] buffer;

    if (err != 0) {
        Serial.print("SimpleConfigStorage: error escribiendo datos (");
        Serial.print(err);
        Serial.println(")");
        return false;
    }

    _valid = true;
    return true;
}

bool ConfigStorage::load(void *dataOut) {
    if (!_initialized) {
        Serial.println("SimpleConfigStorage: llama a begin() antes de load()");
        return false;
    }

    const size_t totalSize = sizeof(Header) + _dataSize;
    uint8_t *buffer = new (std::nothrow) uint8_t[totalSize];
    if (!buffer) {
        Serial.println("SimpleConfigStorage: sin memoria para buffer");
        return false;
    }

    int err = root.read(buffer, _base, totalSize);
    if (err != 0) {
        Serial.print("SimpleConfigStorage: error leyendo datos (");
        Serial.print(err);
        Serial.println(")");
        delete[] buffer;
        _valid = false;
        return false;
    }

    // Interpretar cabecera
    Header header;
    memcpy(&header, buffer, sizeof(Header));

    if (header.magic != _magic) {
        Serial.println("SimpleConfigStorage: magic incorrecto. Datos no inicializados/corruptos.");
        delete[] buffer;
        _valid = false;
        return false;
    }

    // Copiar datos al output
    memcpy(dataOut, buffer + sizeof(Header), _dataSize);

    // Verificar checksum
    uint16_t calc = calcChecksum(reinterpret_cast<const uint8_t *>(dataOut), _dataSize);
    if (calc != header.checksum) {
        Serial.println("SimpleConfigStorage: checksum incorrecto. Datos corruptos.");
        delete[] buffer;
        _valid = false;
        return false;
    }

    delete[] buffer;
    _valid = true;
    return true;
}
