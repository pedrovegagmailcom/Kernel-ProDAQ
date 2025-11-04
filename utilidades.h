/*
 * utilidades.h
 *
 *  Created on: Jan 19, 2024
 *      Author: pedro
 */

#ifndef INC_UTILIDADES_H_
#define INC_UTILIDADES_H_
#include <stdint.h>
#include "inc/modelo.h"

void CambiarBit(uint32_t *var, int bit, int estado);
uint8_t calcularChecksum(DatosSensor *data);

#endif /* INC_UTILIDADES_H_ */
