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
    int8_t codigo;
} ComandoMap;


bool AnalizarComando(const char* Buf, uint32_t Len, char* comando, float* param1, float* param2);

bool ProcesarComando(char* comando, float param1, float param2);
void handleSerialLine(const char* receivedLine);
void handleTramoCommand(char* s2Pointer);

#endif /* INC_GESTIONCOMANDOS_H_ */
