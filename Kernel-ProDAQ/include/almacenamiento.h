#pragma once

#include <stdint.h>
#include <stddef.h>

class ConfigStorage {
public:
    /**
     * @param dataSize    Tamaño en bytes de la estructura a guardar.
     * @param magic       Número mágico para validar los datos.
     * @param baseAddress Dirección base en la QSPI donde se almacenan los datos.
     */
    ConfigStorage(size_t dataSize,
                        uint32_t magic = 0xDEADBEEF,
                        uint32_t baseAddress = 0);

    /** Inicializa el almacenamiento (inicializa internamente el QSPIFBlockDevice). */
    bool begin();

    /** Libera el QSPIFBlockDevice (opcional). */
    void end();

    /** Guarda la estructura apuntada por data. Devuelve true si OK. */
    bool save(const void *data);

    /**
     * Lee la estructura guardada en dataOut.
     * Devuelve true si los datos son válidos (magic + checksum).
     */
    bool load(void *dataOut);

    /** Indica si los últimos datos leídos pasaron la validación. */
    bool isValid() const { return _valid; }

private:
    struct Header {
        uint32_t magic;
        uint16_t checksum;
        uint16_t reserved;  // para futuro uso / alineación
    };

    uint32_t _base;
    size_t   _dataSize;
    uint32_t _magic;
    size_t   _eraseSize;
    bool     _initialized;
    bool     _valid;

    uint16_t calcChecksum(const uint8_t *data, size_t len) const;
};
