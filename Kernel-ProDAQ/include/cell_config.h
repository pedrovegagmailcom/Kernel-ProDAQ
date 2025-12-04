#pragma once

#include "Eeprom24LC08.h"
#include "modelo.h"

/** Dirección base en la EEPROM donde se almacena la configuración de la célula. */
static const uint16_t CELL_CONFIG_ADDRESS = 0;

/**
 * Guarda una configuración de célula en la EEPROM 24LC08.
 * @param config Estructura con los datos a almacenar.
 * @return true si se escribe y verifica correctamente.
 */
bool saveCellConfig(const CellConfig &config);

/**
 * Lee la configuración de célula desde la EEPROM 24LC08.
 * @param config Estructura donde se copiarán los datos leídos.
 * @return true si la lectura se realiza dentro de los límites de la EEPROM.
 */
bool readCellConfig(CellConfig &config);

