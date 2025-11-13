/*

  tramos.h

*/

#ifndef INC_TRAMOS_H_
#define INC_TRAMOS_H_

#include <arduino.h>

#define MAX_NAME_LENGTH 50
#define MAX_PARAMETROS  10
#define MAX_LIMITES     10
#define MAX_TRAMOS      10

// Enums equivalentes
enum TiposTramoEnum {
    TIPO_A = 0,
    TIPO_B = 1
    // Agregar otros tipos según corresponda
};

enum TipoLimite {
    mayor_que = 1,
    menor_que = 0
};

enum OrigenLimite {
    Fuerza = 0,
    Extension = 1,
    Tiempo = 2,
    Aux1 = 3
};

// Estructura Parametro (reemplazando propiedades por campos normales)
// Se asume que el nombre es un string corto.
struct Parametro {
    int codigo;
    char nombre[MAX_NAME_LENGTH]; 
    double valorDouble;
    bool valorBool;
    double limiteRangoMinimo; 
    bool hasLimiteRangoMinimo; // Para indicar si el valor es válido
    double limiteRangoMaximo; 
    bool hasLimiteRangoMaximo; // Para indicar si el valor es válido
};

// Estructura Limite 
struct Limite {
    char nombre[MAX_NAME_LENGTH];
    double valor;
    int tipo;       // podría mapear a TipoLimite directamente si se desea
    char unidad[MAX_NAME_LENGTH];
    int magnitud;
    bool activo;
};

// Estructura TramoDTO
// Ya que en Arduino no se cuenta con List<T>, se usarán arreglos fijos
struct TramoDTO {
    TiposTramoEnum tipoTramo;

    Parametro listaParametros[MAX_PARAMETROS];
    int parametroCount; // cuántos parámetros se parsearon realmente

    Limite listaLimites[MAX_LIMITES];
    int limiteCount; // cuántos límites se parsearon realmente
};

void borrarTramosEnMemoria();
bool parseSingleTramo(const char* json, TramoDTO &tramo);
bool agregarTramoEnMemoria(const TramoDTO &tramo);
bool parseTramosDTO(const char* jsonString, TramoDTO tramos[], int &tramoCount);
void DebugTramos(const char* jsonString, TramoDTO tramos[]);

#endif