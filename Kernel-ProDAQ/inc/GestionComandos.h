/*
 * GestionComandos.h
 *
 *  Created on: Dec 14, 2023
 *      Author: pedro
 */

#ifndef INC_GESTIONCOMANDOS_H_
#define INC_GESTIONCOMANDOS_H_
#include <stdbool.h>

#define CMD_LENGTH 2
typedef void (*ComandoFunc)(float, float);

typedef struct {
    const char* nombreComando;
    ComandoFunc funcion;
    uint8_t codigo;
} ComandoMap;


bool AnalizarComando(uint8_t* Buf, uint32_t Len, char* comando, float* param1, float* param2);
bool ProcesarComando(char* comando, float param1, float param2);

#endif /* INC_GESTIONCOMANDOS_H_ */
