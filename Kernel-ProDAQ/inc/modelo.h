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
    uint32_t modeReg;
    uint32_t ch0Reg;
    uint32_t ch0GainReg;
    float maxForce;
    uint32_t checksum;
} DatosSensor;
#pragma pack(pop)

#endif /* INC_MODELO_H_ */
