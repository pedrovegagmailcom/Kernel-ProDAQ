/*
 * utilidades.c
 *
 *  Created on: Jan 19, 2024
 *      Author: pedro
 */

#include <stdint.h>
#include "modelo.h"

void CambiarBit(uint32_t *var, int bit, int estado) {
    if (estado) {
        // Establecer el bit
        *var |= (1 << bit);
    } else {
        // Borrar el bit
        *var &= ~(1 << bit);
    }
}

uint8_t calcularChecksum(DatosSensor *data) {
    uint8_t *bytes = (uint8_t *)data;
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < sizeof(DatosSensor) - sizeof(uint32_t); i++) {
        checksum += bytes[i];
    }
    return checksum;
}
