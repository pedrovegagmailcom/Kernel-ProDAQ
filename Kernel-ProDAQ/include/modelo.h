/*
 * modelo.h
 *
 *  Created on: May 30, 2024
 *      Author: pedro
 */

#ifndef INC_MODELO_H_
#define INC_MODELO_H_

#pragma pack(push, 1)
typedef struct {
    float fuerza;      // Asumiendo que fuerza es un float (4 bytes)
    float extension;   // Asumiendo que extensión es un float (4 bytes)
    uint32_t timestamp;// Asumiendo un timestamp de 4 bytes
    uint32_t estado;   // 4 bytes para estado
    uint32_t statusReg;
    float voltaje;
    float maxForce;
    uint32_t checksum;
} DatosSensor;
#pragma pack(pop)

struct CellConfig {
  char     numeroserie[10];   // string de 10 caracteres (sin '\0' obligatorio)
  uint16_t capacidad;         // entero sin signo
  uint16_t limite;            // entero sin signo
  float    resolucion;
  float    x1t;
  float    x2t;
  float    x3t;
  float    x4t;
  float    x1c;
  float    x2c;
  float    x3c;
  float    x4c;
  uint16_t overload_t;        // entero sin signo
  uint16_t overload_c;        // entero sin signo
} __attribute__((packed));    // evita padding, opcional pero recomendable



#endif /* INC_MODELO_H_ */
