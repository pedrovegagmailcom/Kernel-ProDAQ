#include "cell_config.h"

#include <string.h>

namespace {
Eeprom24LC08 g_eeprom;
bool g_initialized = false;

bool checkBounds(uint16_t baseAddress) {
    return (baseAddress + sizeof(CellConfig)) <= Eeprom24LC08::TOTAL_SIZE;
}

void ensureBegin() {
    if (!g_initialized) {
        g_eeprom.begin();
        g_initialized = true;
    }
}
}  // namespace

bool saveCellConfig(const CellConfig &config) {
    if (!checkBounds(CELL_CONFIG_ADDRESS)) {
        return false;
    }

    ensureBegin();
    g_eeprom.writeStruct(CELL_CONFIG_ADDRESS, config);

    // Verificación básica: leer de nuevo y comparar
    CellConfig verify = {};
    g_eeprom.readStruct(CELL_CONFIG_ADDRESS, verify);
    return memcmp(&verify, &config, sizeof(CellConfig)) == 0;
}

bool readCellConfig(CellConfig &config) {
    if (!checkBounds(CELL_CONFIG_ADDRESS)) {
        return false;
    }

    ensureBegin();
    g_eeprom.readStruct(CELL_CONFIG_ADDRESS, config);
    return true;
}

