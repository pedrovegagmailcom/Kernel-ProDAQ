#include "ControlEnsayo.h"
#include "utilidades.h" // subir(), bajar(), parar(), LecturaFuerza(), LecturaDesplazamiento(), etc.

#include <string.h>

// Variables internas
static State estado_maquina_internal = IDLE;
static TramoDTO tramosGlobales[MAX_TRAMOS];
static int tramoCountGlobal = 0;
static int currentTramoIndex = 0;
static bool abortRequested = false;

// Esta variable se puede setear desde fuera (por ejemplo, desde el manejador de comandos) cuando se reciba la orden de iniciar.
static bool iniciarEnsayoFlag = false;

// Tiempo de inicio del tramo en ms (para cálculos de tiempos)
static uint32_t inicioTramoMillis = 0;

// Prototipos de funciones internas
static bool TramoCompletado();
static void ejecutarTramoActual();
static double obtenerParametroPorCodigo(const TramoDTO &t, int codigo, double valorDefault);
static bool verificarLimites(const TramoDTO &t);
static void resetEnsayoInterno();

// ------------------ Funciones Públicas ------------------

void InitEnsayoControl() {
    resetEnsayoInterno();
}


void parar() {

}

void subir(float val) {

}

void bajar(float val) {

}


void CeroFuerza() {

}

void CeroDesplazamiento(){

}

float LecturaFuerza() {

  return 0;
}

float LecturaDesplazamiento() {
  
  return 0;
}


void IniciarEnsayo(TramoDTO tramos[], int count) {
    if (estado_maquina_internal == IDLE || estado_maquina_internal == FINISHED || estado_maquina_internal == ABORTED) {
        // Copiar tramos
        tramoCountGlobal = (count <= MAX_TRAMOS) ? count : MAX_TRAMOS;
        for (int i = 0; i < tramoCountGlobal; i++) {
            tramosGlobales[i] = tramos[i];
        }
        // Indicar que el ensayo está listo para iniciar
        iniciarEnsayoFlag = true;
    } else {
        // Si no está en un estado que lo permita, se podría reportar un error o ignorar
    }
}

void AbortarEnsayo() {
    abortRequested = true;
}

State getEstadoActual() {
    return estado_maquina_internal;
}

void ActualizarEnsayo() {
    switch (estado_maquina_internal) {
        case IDLE:
            // Esperar a que el usuario cargue tramos y de la orden de iniciar
            if (iniciarEnsayoFlag && tramoCountGlobal > 0) {
                estado_maquina_internal = SETUP;
                iniciarEnsayoFlag = false; // Consumir el flag
            }
            if (abortRequested) {
                // No tiene efecto abortar en IDLE, lo ignoramos
                abortRequested = false;
            }
            break;

        case SETUP:
            if (abortRequested) {
                estado_maquina_internal = ABORTED;
                parar(); // Por si acaso
                break;
            }
            // Calibraciones previas al inicio
            CeroFuerza();
            CeroDesplazamiento();
            currentTramoIndex = 0;
            inicioTramoMillis = millis();
            estado_maquina_internal = RUN_TRAMO;
            break;

        case RUN_TRAMO:
            if (abortRequested) {
                parar();
                estado_maquina_internal = ABORTED;
                break;
            }

            ejecutarTramoActual();
            if (TramoCompletado()) {
                // Pasar al siguiente tramo
                currentTramoIndex++;
                if (currentTramoIndex >= tramoCountGlobal) {
                    // No quedan más tramos
                    parar();
                    estado_maquina_internal = FINISHED;
                } else {
                    // Preparar el siguiente tramo
                    inicioTramoMillis = millis();
                    // No cambiamos el estado, seguimos en RUN_TRAMO pero con un nuevo tramo
                }
            }
            break;

        case FINISHED:
            if (abortRequested) {
                // Abort en finished no es lógico, pero podemos resetear a IDLE
                abortRequested = false;
            }
            // Esperar nuevo ensayo
            // Se podría resetear si se desea volver a IDLE inmediatamente:
            // resetEnsayoInterno(); // Opcional, solo si se requiere limpiar datos antiguos.
            break;

        case ABORTED:
            // Ensayo abortado, esperar comando para iniciar nuevo ensayo
            // Podríamos resetear internamente al recibir un nuevo "IniciarEnsayo"
            break;
    }
}

// ------------------ Funciones Internas ------------------

static void resetEnsayoInterno() {
    estado_maquina_internal = IDLE;
    tramoCountGlobal = 0;
    currentTramoIndex = 0;
    abortRequested = false;
    iniciarEnsayoFlag = false;
    inicioTramoMillis = 0;
}


static void ejecutarTramoActual() {
    // Obtener el tramo actual
    TramoDTO &t = tramosGlobales[currentTramoIndex];

    // Ejemplo: asumamos que TIPO_A = subir a cierta velocidad hasta una fuerza X
    //          TIPO_B = bajar a cierta velocidad hasta una extensión Y
    // Estos son ejemplos arbitrarios; en la práctica, se implementará según la lógica requerida.
    switch (t.tipoTramo) {
        case TIPO_A: {
            double velo = obtenerParametroPorCodigo(t, /*codigoVelocidad*/ 1001, 10.0);
            // Ejecutar movimiento hacia arriba
            subir(velo);
            break;
        }
        case TIPO_B: {
            double velo = obtenerParametroPorCodigo(t, /*codigoVelocidad*/ 1001, 5.0);
            // Ejecutar movimiento hacia abajo
            bajar(velo);
            break;
        }
        default:
            // Tipo de tramo desconocido, por seguridad parar
            parar();
            break;
    }
}


// Verifica si el tramo actual se completa al cumplir alguna condición
static bool TramoCompletado() {
    // Checar límites definidos en el tramo: fuerza, extensión, tiempo, etc.
    TramoDTO &t = tramosGlobales[currentTramoIndex];

    return verificarLimites(t);
}

// Verifica las condiciones de salida de un tramo
static bool verificarLimites(const TramoDTO &t) {
    double fuerzaAct = LecturaFuerza();
    double extensionAct = LecturaDesplazamiento();
    uint32_t tiempoTranscurrido = millis() - inicioTramoMillis;

    // Recorrer la lista de límites del tramo
    for (int i = 0; i < t.limiteCount; i++) {
        const Limite &lim = t.listaLimites[i];
        if (!lim.activo) continue; 

        double valorReferencia = lim.valor;
        double valorActual = 0.0;

        // Determinar origen del limite (Fuerza, Extension, Tiempo, Aux...)
        // Suponemos que magnitud indica el tipo (ej: 0=Fuerza, 1=Extension, 2=Tiempo)
        switch (lim.magnitud) {
            case (int)OrigenLimite::Fuerza:
                valorActual = fuerzaAct;
                break;
            case (int)OrigenLimite::Extension:
                valorActual = extensionAct;
                break;
            case (int)OrigenLimite::Tiempo:
                valorActual = (double)tiempoTranscurrido;
                break;
            default:
                // Otros tipos no implementados
                continue;
        }

        // Comparar según tipo de limite (menor_que, mayor_que)
        // Suponemos lim.tipo 0=menor_que, 1=mayor_que
        if (lim.tipo == 0) { // menor_que
            if (valorActual < valorReferencia) {
                // Se cumple la condición para finalizar el tramo
                return true;
            }
        } else { // mayor_que
            if (valorActual > valorReferencia) {
                return true;
            }
        }
    }

    return false; // Ningún límite se ha cumplido todavía
}


// Obtiene un parámetro double de un tramo por código, si no se encuentra devuelve valorDefault
static double obtenerParametroPorCodigo(const TramoDTO &t, int codigo, double valorDefault) {
    for (int i = 0; i < t.parametroCount; i++) {
        if (t.listaParametros[i].codigo == codigo) {
            return t.listaParametros[i].valorDouble;
        }
    }
    return valorDefault;
}
