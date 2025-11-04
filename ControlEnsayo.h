#ifndef ENSAYO_CONTROL_H
#define ENSAYO_CONTROL_H

#include "tramos.h" // Contiene la definición de TramoDTO, etc.

enum State {
    IDLE,
    SETUP,
    RUN_TRAMO,
    FINISHED,
    ABORTED
};

void InitEnsayoControl();

// Configura y prepara un nuevo ensayo, copiando los tramos internos
void IniciarEnsayo(TramoDTO tramos[], int count);

// Abortará el ensayo en curso
void AbortarEnsayo();

// Actualiza el estado del ensayo, se debe llamar periódicamente (en loop o thread)
void ActualizarEnsayo();

// Devuelve el estado actual de la máquina
State getEstadoActual();

#endif
