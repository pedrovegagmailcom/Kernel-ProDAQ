
/*

  GestionComandos.cpp

*/

#include <Arduino.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string>

#include "LS7366.h"

#include "GestionComandos.h"
#include "utilidades.h"
#include "tramos.h"

typedef enum {
    PROTOCOLO_NUEVO,
    PROTOCOLO_VIEJO
} ProtocoloMode;

static ProtocoloMode protocoloActual = PROTOCOLO_NUEVO;

extern uint32_t estado_maquina;
extern volatile bool transmitirDatos;
extern volatile uint32_t dataRate;

extern LS7366 Encoder;

void CommandWF(float param1, float param2) {

	CambiarBit(&estado_maquina, 0, 1);
	CambiarBit(&estado_maquina, 1, 0);
	CambiarBit(&estado_maquina, 2, 0);
}

void CommandWR(float param1, float param2) {
	CambiarBit(&estado_maquina, 0, 0);
	CambiarBit(&estado_maquina, 1, 1);
	CambiarBit(&estado_maquina, 2, 0);
}

void CommandWS(float param1, float param2) {
	CambiarBit(&estado_maquina, 0, 0);
	CambiarBit(&estado_maquina, 1, 0);
	CambiarBit(&estado_maquina, 2, 1);
}


void CommandAD0(float param1, float param2) {

	//InicializarAD();

}

void CommandAD1(float param1, float param2) {

	//calibrar_internaloffset_ad7175();
	

}

void CommandAD2(float param1, float param2) {
	//calibrar_sysoffset_ad7175();
	

}

void CommandAD3(float param1, float param2) {
	//calibrar_sysgain_ad7175();
	

}

void CommandAD4(float param1, float param2) {
	//calibrar_internalgain_ad7175();

}

void CommandS0(float param1, float param2) {

	if ((int)param1 == 0) {
		transmitirDatos = false;
	}
	else {
		transmitirDatos = true;
	}

}

void CommandRate(float param1, float param2) {
    if (param1 > 0) {
        dataRate = (uint32_t)(param1);
    }
}


void CommandRI(float param1, float param2) {
	Serial.println("RABBIT");
}


void CommandRC(float param1, float param2) {
	Serial.println("1000");
}

void CommandRX(float param1, float param2) {
	Serial.println("0");
}


void CommandWM(float param1, float param2) {
	Serial.println("");
}

void CommandRV(float param1, float param2) {
	Serial.println("500");
}


void CommandWV(float param1, float param2) {
	Serial.println("");
}

void CommandWE(float param1, float param2) {
	Serial.println("");
}

void CommandWI(float param1, float param2) {
	Serial.println("");
}

void CommandR1(float param1, float param2) {
	Serial.println("0.0");
}

void CommandR2(float param1, float param2) {
    unsigned long  valor = Encoder.read_counter();
	Serial.println(valor, 4);
}

void CommandRS(float param1, float param2) {
	Serial.println("\0\0\0");
}

void CommandRH(float param1, float param2) {
	Serial.println("0");
}

void CommandWB(float param1, float param2) {
	Serial.println("");
}

void CommandWT(float param1, float param2) {
	Serial.println("");
}


ComandoMap comandoMaps[] = {
    {"WF", CommandWF, 0},
    {"WR", CommandWR, 1},
    {"WS", CommandWS, 2},
	{"A0", CommandAD0, 3}, // Reset ADC
	{"A1", CommandAD1, 4}, // calibrar internaloffset
	{"A2", CommandAD2, 5}, // calibrar sysoffset
	{"A3", CommandAD3, 6}, // calibrar systemgain
	{"A4", CommandAD4, 7}, // calibrar internalgain
	{"S0", CommandS0, 8}, // iniciar envio datos
	{"S1", CommandRate, 9}, // Modificar datarate
    {"RI", CommandRI, 10},
	{"RC", CommandRC, 11},
	{"RX", CommandRX, 12}, // Hay extensometro ?
	{"WM", CommandWM, 13}, // Modo remoto
	{"RV", CommandRV, 14}, // Velocdidad maxima ?
	{"WV", CommandWV, 15},
	{"WI", CommandWV, 16},
	{"R1", CommandR1, 17},
	{"R2", CommandR2, 18},
	{"RS", CommandRS, 19},
	{"RH", CommandRH, 20}, // Ensayo en curso ?
	{"WB", CommandRS, 21}, // Alarma baja velo
	{"WT", CommandWT, 22},
	{"WE", CommandWE, 23},
    {NULL, NULL, -1} // Marca el fin de la lista
};

int32_t GetCodigoComando(char* nombreComando) {
    for (int i = 0; comandoMaps[i].nombreComando != NULL; ++i) {
        if (strcmp(comandoMaps[i].nombreComando, nombreComando) == 0) {
            return i; // Devolver el índice del comando
        }
    }
    return -1;
}

bool ProcesarComando(char* comando, float param1, float param2) {
    int codigo = GetCodigoComando(comando);
    if (codigo >= 0) {
        comandoMaps[codigo].funcion(param1, param2);
        return true;
    }
    return false;
}



// Verificar si el formato del comando es correcto
bool VerificarFormato(const char* Buf, uint32_t Len) {
    // Verificar el caracter inicial y los dos caracteres finales
    return Len >= 4 && Buf[0] == '|' && Buf[Len - 2] == '|' && Buf[Len - 1] == '|';
}

// Función para analizar comandos
bool AnalizarComando(char* Buf, uint32_t Len, char* comando, float* param1, float* param2) {
    // Comprobar el formato del comando
    if (!VerificarFormato(Buf, Len)) {
        return false;
    }

    // Extraer el comando (asumiendo que está justo después del primer '|')
    strncpy(comando, (char*)Buf + 1, CMD_LENGTH);
    comando[CMD_LENGTH] = '\0'; // Asegura el final de la cadena

    // Inicializar los parámetros
    *param1 = 0.0;
    *param2 = 0.0;

    // Preparar para extraer parámetros
    char* rest = (char*)Buf + 1 + CMD_LENGTH; // Saltar el comando
    char* endPtr = (char*)Buf + Len - 2; // Apuntar al final del mensaje antes de los '||'

    // Verificar si hay parámetros
    if (rest < endPtr) {
        // Extraer el primer parámetro si está presente
        char* token = strtok(rest + 1, "|");
        if (token != NULL && token < endPtr) {
            *param1 = atof(token); // Convertir a float el primer parámetro

            // Intentar extraer un segundo parámetro
            token = strtok(NULL, "|");
            if (token != NULL && token < endPtr) {
                *param2 = atof(token); // Convertir a float el segundo parámetro
            }
        }
    }

    return true;
}

bool ProcesarComandoNuevo(uint8_t* Buf, uint32_t Len) {
    char comando[CMD_LENGTH + 1];
    float param1, param2;
    if (AnalizarComando((char*)Buf, Len, comando, &param1, &param2)) {
        return ProcesarComando(comando, param1, param2);
    }
    return false;
}

// Protocolo VIEJO:
// Formato: 2 caracteres a los que puede seguir un número y siempre termina en "\r"
// Ejemplos: "WF\r" o "WV1000\r" (donde WV modifica la velocidad a 1000mm/min)
bool ProcesarComandoViejo(uint8_t* Buf, uint32_t Len) {
    char mensaje[100];

    
    if (Len >= sizeof(mensaje))
        return false;
    memcpy(mensaje, Buf, Len);
    mensaje[Len] = '\0';
    size_t l = strlen(mensaje);
    if (l > 0 && mensaje[l - 1] == '\r') {
        mensaje[l - 1] = '\0';
    }
    // Extraer los dos primeros caracteres como comando
    char comando[3];
    comando[0] = mensaje[0];
    comando[1] = mensaje[1];
    comando[2] = '\0';
    float param1 = 0.0f;
    float param2 = 0.0f;
    // Si hay más caracteres, convertirlos a número (desde la posición 2)
    if (l > 2) {
        param1 = atof(mensaje + 2);
    }
    
    return ProcesarComando(comando, param1, param2);
}

// Función unificada para procesar el mensaje recibido según el protocolo activo
bool ProcesarMensaje(uint8_t* Buf, uint32_t Len) {
	
    if (protocoloActual == PROTOCOLO_NUEVO) {
        
        // Si se recibe el comando "RI\r", cambiar a modo antiguo.
        if (Len == 2 && strncmp((char*)Buf, "RI", 2) == 0) {
            
            protocoloActual = PROTOCOLO_VIEJO;
            return ProcesarComandoViejo(Buf, Len);
        }
        return ProcesarComandoNuevo(Buf, Len);
    } else {
        return ProcesarComandoViejo(Buf, Len);
    }
}






void handleTramoCommand(char* s2Pointer) {
    // Saltamos "S2|"
    s2Pointer += 3; 

    // 1) Comando para Borrar Tramos (DEL)
    if (strncmp(s2Pointer, "DEL|", 4) == 0) {
        borrarTramosEnMemoria();
        Serial.println("OK, se borraron todos los tramos.");

    // 2) Comando para Añadir Tramo (ADD|{JSON}|)
    } else if (strncmp(s2Pointer, "ADD|", 4) == 0) {
        s2Pointer += 4;  // Avanzar para omitir "ADD|"

        // Copiar hasta el siguiente '|' o fin de string
        char jsonTramo[2000];
        size_t  j = 0;
        while (*s2Pointer != '\0' && *s2Pointer != '|' && j < sizeof(jsonTramo) - 1) {
            jsonTramo[j++] = *s2Pointer++;
        }
        jsonTramo[j] = '\0'; // terminar cadena

        TramoDTO tramoSingle;
        if (parseSingleTramo(jsonTramo, tramoSingle)) {
            if (agregarTramoEnMemoria(tramoSingle)) {
                Serial.println("OK, tramo añadido.");
            } else {
                Serial.println("ERROR: no se pudo agregar (lista llena).");
            }
        } else {
            Serial.println("ERROR al parsear el JSON del tramo.");
        }

    // 3) Comando para Finalizar (FIN)
    } else if (strncmp(s2Pointer, "FIN|", 4) == 0) {
        // Aquí podrías iniciar la ejecución de los tramos, etc.
        Serial.println("OK, finalizó la carga de tramos.");

    } else {
        // Comando desconocido dentro de "S2|"
        Serial.println("ERROR, subcomando S2 desconocido.");
    }
}


void handleSerialLine(const char* receivedLine) {
    // Si contiene el patrón "S2|", lo tratamos como un “comando de tramos”
    char* s2Pointer = strstr(receivedLine, "S2|");
    if (s2Pointer != nullptr) {
        // Llamamos a la función encargada de procesar esos subcomandos
        handleTramoCommand(s2Pointer);
    }
    else {
        // Caso contrario, manejamos el resto de comandos 
        char comando[50];
        float param1, param2;
        if (AnalizarComando((char*)receivedLine, strlen(receivedLine), comando, &param1, &param2)) {
            ProcesarComando(comando, param1, param2);
        }
        else {
            Serial.println("Comando no reconocido.");
        }
    }
}
