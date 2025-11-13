/*
 * utilidades.h
 *
 *  Created on: Jan 19, 2024
 *      Author: pedro
 */

#ifndef INC_UTILIDADES_H_
#define INC_UTILIDADES_H_
#include <stdint.h>

void CambiarBit(uint32_t *var, int bit, int estado);
uint32_t calcularCRC32(uint8_t *data, uint32_t length);

#endif /* INC_UTILIDADES_H_ */
