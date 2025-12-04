
/*

  GestionComandos.cpp

*/

#include <Arduino.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string>
#include <math.h>
#include <rtos.h>

#include "LS7366.h"
#include "IO.h"
#include "alarmas.h"
#include "LTC2602.h"
#include "modelo.h"
#include "AD7175.h"

#include "GestionComandos.h"
#include "utilidades.h"
#include "tramos.h"
#include "almacenamiento.h"
#include "Eeprom24LC08.h"

typedef enum {
    PROTOCOLO_NUEVO,
    PROTOCOLO_VIEJO
} ProtocoloMode;

static ProtocoloMode protocoloActual = PROTOCOLO_NUEVO;

extern uint32_t estado_maquina;
extern volatile bool transmitirDatos;
extern volatile uint32_t dataRate;

extern LS7366 Encoder;
extern IO IOsystem;
extern Alarmas alarmas;
extern LTC2602 LTCdac;
extern DatosSensor sensorData;
extern rtos::Mutex sensorDataMutex;

static float encoderStepsPerMillimeter = 1.0f;
static int32_t encoderPolaritySign     = 1;
static constexpr uint16_t CELL_CONFIG_EEADDR = 0;

static Eeprom24LC08 eeprom24lc08;
static CellConfig   cellConfigCache{};
static bool         cellConfigLoaded = false;

namespace {

constexpr float VELOCIDAD_MAX_MM_MIN = 500.0f;
constexpr uint8_t DAC_CANAL_BIPOLAR = 1; // Canal B del LTC2602
constexpr uint16_t DAC_MID_CODE = 32768U;
constexpr float DAC_MAX_VOLTAGE = 10.0f; // Rango bipolar ±10V
constexpr float DAC_OFFSET_LIMIT_VOLTS = 2.0f; // Ajuste máximo permitido por software
constexpr float DAC_COUNTS_PER_VOLT = 65535.0f / (DAC_MAX_VOLTAGE * 2.0f);

float velocidadConsigna = 0.0f;
float dacZeroOffsetVolts = 0.0f;
int32_t dacZeroOffsetCounts = 0;

struct ConfigStruct {
    float encoderGainStepsPerMillimeter;
    float dacOffsetVolts;
    int32_t encoderPolaritySign;
};

enum class ConfigParameter : uint8_t {
    EncoderGain   = 1,
    DacOffset     = 2,
    EncoderPolarity = 3,
};

constexpr uint32_t CONFIG_MAGIC = 0x43464732; // "CFG2"
constexpr uint32_t CONFIG_BASE_ADDR = 0;

ConfigStruct configData{1.0f, 0.0f, 1};
ConfigStorage configStorage(sizeof(ConfigStruct), CONFIG_MAGIC, CONFIG_BASE_ADDR);

float normalizarVelocidad(float valor) {
    if (!isfinite(valor)) {
        return 0.0f;
    }

    if (valor > VELOCIDAD_MAX_MM_MIN) {
        valor = VELOCIDAD_MAX_MM_MIN;
    } else if (valor < -VELOCIDAD_MAX_MM_MIN) {
        valor = -VELOCIDAD_MAX_MM_MIN;
    }

    return valor / VELOCIDAD_MAX_MM_MIN;
}

void actualizarSalidaVelocidad() {
    float velocidad = velocidadConsigna;

    bool stop = (estado_maquina & (1UL << 2)) != 0;
    bool forward = (estado_maquina & (1UL << 0)) != 0;
    bool reverse = (estado_maquina & (1UL << 1)) != 0;

    if (stop || (!forward && !reverse)) {
        velocidad = 0.0f;
    } else if (reverse && !forward && velocidad > 0.0f) {
        velocidad = -velocidad;
    } else if (forward && !reverse && velocidad < 0.0f) {
        velocidad = -velocidad;
    }

    float fraccion = normalizarVelocidad(velocidad);
    int32_t delta = (int32_t)lrintf(fraccion * 32767.0f);

    int32_t codigo = DAC_MID_CODE + delta;

    codigo += dacZeroOffsetCounts;

    if (codigo < 0) {
        codigo = 0;
    } else if (codigo > 65535) {
        codigo = 65535;
    }

    LTCdac.setOutput(DAC_CANAL_BIPOLAR, static_cast<uint16_t>(codigo));
}

void actualizarOffsetDAC(float offsetVolts) {
    if (!isfinite(offsetVolts)) {
        return;
    }

    if (offsetVolts > DAC_OFFSET_LIMIT_VOLTS) {
        offsetVolts = DAC_OFFSET_LIMIT_VOLTS;
    } else if (offsetVolts < -DAC_OFFSET_LIMIT_VOLTS) {
        offsetVolts = -DAC_OFFSET_LIMIT_VOLTS;
    }

    dacZeroOffsetVolts = offsetVolts;
    dacZeroOffsetCounts = static_cast<int32_t>(lrintf(offsetVolts * DAC_COUNTS_PER_VOLT));
}

void actualizarGananciaEncoder(float pasosPorMilimetro) {
    if (!isfinite(pasosPorMilimetro) || pasosPorMilimetro <= 0.0f) {
        return;
    }

    encoderStepsPerMillimeter = pasosPorMilimetro;
}

void actualizarPolaridadEncoder(int32_t polaridad) {
    if (polaridad >= 0) {
        encoderPolaritySign = 1;
    } else {
        encoderPolaritySign = -1;
    }
}

void guardarConfiguracionFlash() {
    configData.dacOffsetVolts              = dacZeroOffsetVolts;
    configData.encoderGainStepsPerMillimeter = encoderStepsPerMillimeter;
    configData.encoderPolaritySign         = encoderPolaritySign;

    if (!configStorage.save(&configData)) {
        Serial.println("No se pudo guardar la configuración en flash");
    }
}

void aplicarConfiguracion(const ConfigStruct& cfg) {
    actualizarGananciaEncoder(cfg.encoderGainStepsPerMillimeter);
    actualizarOffsetDAC(cfg.dacOffsetVolts);
    actualizarPolaridadEncoder(cfg.encoderPolaritySign);
}

void cargarConfiguracionFlash() {
    if (!configStorage.begin()) {
        Serial.println("No se pudo inicializar el almacenamiento de configuración");
        return;
    }

    if (configStorage.load(&configData)) {
        aplicarConfiguracion(configData);
        Serial.println("Configuración cargada desde flash");
    } else {
        // Guardar valores por defecto para disponer de un bloque válido
        guardarConfiguracionFlash();
        Serial.println("Configuración por defecto guardada en flash");
    }
}

bool tryParseConfigParameter(const char* parametros, ConfigParameter& parameter, const char** valueStart) {
    if (parametros == nullptr || strlen(parametros) < 2) {
        return false;
    }

    char codigoStr[3] = {parametros[0], parametros[1], '\0'};
    char* endPtr = nullptr;
    long codigo = strtol(codigoStr, &endPtr, 10);

    if (endPtr != (codigoStr + 2)) {
        return false;
    }

    switch (codigo) {
        case 1:
            parameter = ConfigParameter::EncoderGain;
            break;
        case 2:
            parameter = ConfigParameter::DacOffset;
            break;
        case 3:
            parameter = ConfigParameter::EncoderPolarity;
            break;
        default:
            return false;
    }

    const char* inicioValor = parametros + 2;
    while (*inicioValor == ' ') {
        ++inicioValor;
    }

    if (valueStart != nullptr) {
        *valueStart = inicioValor;
    }

    return true;
}

bool procesarLecturaConfiguracion(ConfigParameter parameter) {
    switch (parameter) {
        case ConfigParameter::EncoderGain:
            Serial.println(encoderStepsPerMillimeter, 4);
            return true;
        case ConfigParameter::DacOffset:
            Serial.println(dacZeroOffsetVolts, 3);
            return true;
        case ConfigParameter::EncoderPolarity:
            Serial.println(encoderPolaritySign);
            return true;
        default:
            return false;
    }
}

bool procesarEscrituraConfiguracion(ConfigParameter parameter, const char* valueStart) {
    if (valueStart == nullptr || *valueStart == '\0') {
        return false;
    }

    char* endPtr = nullptr;

    switch (parameter) {
        case ConfigParameter::EncoderGain: {
            float ganancia = strtof(valueStart, &endPtr);
            if (endPtr == valueStart || !isfinite(ganancia) || ganancia <= 0.0f) {
                return false;
            }
            actualizarGananciaEncoder(ganancia);
            guardarConfiguracionFlash();
            Serial.println(encoderStepsPerMillimeter, 4);
            return true;
        }
        case ConfigParameter::DacOffset: {
            float offset = strtof(valueStart, &endPtr);
            if (endPtr == valueStart || !isfinite(offset)) {
                return false;
            }
            actualizarOffsetDAC(offset);
            actualizarSalidaVelocidad();
            guardarConfiguracionFlash();
            Serial.println(dacZeroOffsetVolts, 3);
            return true;
        }
        case ConfigParameter::EncoderPolarity: {
            int32_t polaridad = static_cast<int32_t>(strtol(valueStart, &endPtr, 10));
            if (endPtr == valueStart || polaridad == 0) {
                return false;
            }
            actualizarPolaridadEncoder(polaridad);
            guardarConfiguracionFlash();
            Serial.println(encoderPolaritySign);
            return true;
        }
        default:
            return false;
    }
}

bool procesarComandoConfiguracion(const char* comando, const char* parametros) {
    ConfigParameter parametro;
    const char* valueStart = nullptr;

    if (!tryParseConfigParameter(parametros, parametro, &valueStart)) {
        return false;
    }

    if (strcmp(comando, "RP") == 0) {
        return procesarLecturaConfiguracion(parametro);
    }

    if (strcmp(comando, "WP") == 0) {
        return procesarEscrituraConfiguracion(parametro, valueStart);
    }

    return false;
}

float convertirParametroVelocidad(float parametro) {
    if (!isfinite(parametro)) {
        return 0.0f;
    }

    float parteEntera = 0.0f;
    float fraccion = modff(parametro, &parteEntera);

    if (fraccion == 0.0f) {
        // Valor entero: puede provenir del protocolo viejo (sin punto decimal)
        if (parametro > VELOCIDAD_MAX_MM_MIN) {
            parametro /= 10.0f;
        }
    }

    if (parametro > VELOCIDAD_MAX_MM_MIN) {
        parametro = VELOCIDAD_MAX_MM_MIN;
    }
    if (parametro < -VELOCIDAD_MAX_MM_MIN) {
        parametro = -VELOCIDAD_MAX_MM_MIN;
    }

    return parametro;
}

} // namespace

float convertirContadorAMilimetros(long valor) {
    if (encoderStepsPerMillimeter <= 0.0f) {
        return 0.0f;
    }

    double pasos = static_cast<double>(valor);
    double milimetros = pasos / static_cast<double>(encoderStepsPerMillimeter);
    milimetros *= static_cast<double>(encoderPolaritySign);

    return static_cast<float>(milimetros);
}

void InicializarConfiguracion() {
    eeprom24lc08.begin();
    cargarConfiguracionFlash();
}

void Parar() {
    velocidadConsigna = 0.0f;

    CambiarBit(&estado_maquina, 0, 0);
    CambiarBit(&estado_maquina, 1, 0);
    CambiarBit(&estado_maquina, 2, 1);

    actualizarSalidaVelocidad();
}

void CommandWF(float param1, float param2) {

        CambiarBit(&estado_maquina, 0, 1);
        CambiarBit(&estado_maquina, 1, 0);
        CambiarBit(&estado_maquina, 2, 0);

        actualizarSalidaVelocidad();
        Serial.write(13);
}

void CommandWR(float param1, float param2) {
        CambiarBit(&estado_maquina, 0, 0);
        CambiarBit(&estado_maquina, 1, 1);
        CambiarBit(&estado_maquina, 2, 0);

        actualizarSalidaVelocidad();
        Serial.write(13);
}

void CommandWS(float param1, float param2) {
        Parar();
        Serial.write(13);
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

void CommandRR(float param1, float param2) {
    Serial.println(dataRate);
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
        velocidadConsigna = convertirParametroVelocidad(param1);
        actualizarSalidaVelocidad();
        Serial.println("");
}

void CommandWO(float param1, float param2) {
        actualizarOffsetDAC(param1);
        actualizarSalidaVelocidad();
        guardarConfiguracionFlash();
        Serial.println(dacZeroOffsetVolts, 3);
}

void CommandWZ(float param1, float param2) {
    int32_t result = AD7175_SystemZeroScaleCalibrate();
    Serial.println("");
}

void CommandROffset(float param1, float param2) {
        Serial.println(dacZeroOffsetVolts, 3);
}

void CommandWE(float param1, float param2) {
        if (!isfinite(param1) || param1 <= 0.0f) {
                Serial.println("ERR");
                return;
        }

        actualizarGananciaEncoder(param1);
        guardarConfiguracionFlash();
        Serial.println(encoderStepsPerMillimeter, 4);
}

void CommandRE(float param1, float param2) {
        Serial.println(encoderStepsPerMillimeter, 4);
}

void CommandWP(float param1, float param2) {
        if (!isfinite(param1)) {
                Serial.println("ERR");
                return;
        }

        int32_t polaridad = static_cast<int32_t>(lrintf(param1));

        if (polaridad == 0) {
                Serial.println("ERR");
                return;
        }

        actualizarPolaridadEncoder(polaridad);
        guardarConfiguracionFlash();
        Serial.println(encoderPolaritySign);
}

void CommandRP(float param1, float param2) {
        Serial.println(encoderPolaritySign);
}

void CommandWI(float param1, float param2) {
        Serial.println("");
}

void CommandR1(float param1, float param2) {
        sensorDataMutex.lock();
        float fuerza = sensorData.fuerza;
        sensorDataMutex.unlock();

        Serial.println(fuerza, 4);
}

void CommandR2(float param1, float param2) {
    if (encoderStepsPerMillimeter <= 0.0f) {
        Serial.println("ERR");
        return;
    }

    sensorDataMutex.lock();
    float extension = sensorData.extension;
    sensorDataMutex.unlock();

    Serial.println(extension, 4);
}

void CommandR3(float param1, float param2) {
    sensorDataMutex.lock();
    float voltaje = sensorData.voltaje;
    sensorDataMutex.unlock();

    Serial.println(voltaje, 4);
}

void CommandRS(float param1, float param2) {
    sensorDataMutex.lock();
    uint8_t alarmasByte = static_cast<uint8_t>(sensorData.estado & 0xFFu);
    uint8_t statusByte  = static_cast<uint8_t>((sensorData.estado >> 8) & 0xFFu);
    sensorDataMutex.unlock();

    // Delphi pide 3 bytes; usa solo el primero (alarmas)
    Serial.write(alarmasByte);   // buf[1] en Delphi
    Serial.write(statusByte);    // buf[2] (por si otra función lo usa)
    Serial.write('\r');          // buf[3] terminador
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

bool cargarCellConfigDesdeEeprom(CellConfig& destino) {
    eeprom24lc08.readStruct(CELL_CONFIG_EEADDR, destino);
    return true;
}

bool guardarCellConfigEnEeprom(const CellConfig& origen) {
    eeprom24lc08.writeStruct(CELL_CONFIG_EEADDR, origen);
    return true;
}

bool parseCellConfigString(const char* payload, CellConfig& destino) {
    if (payload == nullptr) {
        return false;
    }

    char buffer[200];
    strncpy(buffer, payload, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char* token     = strtok(buffer, ",");
    int   fieldIndex = 0;

    while (token != nullptr) {
        switch (fieldIndex) {
            case 0: {
                memset(destino.numeroserie, 0, sizeof(destino.numeroserie));
                strncpy(destino.numeroserie, token, sizeof(destino.numeroserie));
                break;
            }
            case 1:
                destino.capacidad = static_cast<uint16_t>(strtoul(token, nullptr, 10));
                break;
            case 2:
                destino.limite = static_cast<uint16_t>(strtoul(token, nullptr, 10));
                break;
            case 3:
                destino.resolucion = strtof(token, nullptr);
                break;
            case 4:
                destino.x1t = strtof(token, nullptr);
                break;
            case 5:
                destino.x2t = strtof(token, nullptr);
                break;
            case 6:
                destino.x3t = strtof(token, nullptr);
                break;
            case 7:
                destino.x4t = strtof(token, nullptr);
                break;
            case 8:
                destino.x1c = strtof(token, nullptr);
                break;
            case 9:
                destino.x2c = strtof(token, nullptr);
                break;
            case 10:
                destino.x3c = strtof(token, nullptr);
                break;
            case 11:
                destino.x4c = strtof(token, nullptr);
                break;
            case 12:
                destino.overload_t = static_cast<uint16_t>(strtoul(token, nullptr, 10));
                break;
            case 13:
                destino.overload_c = static_cast<uint16_t>(strtoul(token, nullptr, 10));
                break;
            default:
                return false;
        }

        ++fieldIndex;
        token = strtok(nullptr, ",");
    }

    return fieldIndex == 14;
}

void imprimirCellConfig(const CellConfig& cfg) {
    char serialBuffer[11];
    memcpy(serialBuffer, cfg.numeroserie, sizeof(cfg.numeroserie));
    serialBuffer[sizeof(serialBuffer) - 1] = '\0';

    Serial.print(serialBuffer);
    Serial.print(',');
    Serial.print(cfg.capacidad);
    Serial.print(',');
    Serial.print(cfg.limite);
    Serial.print(',');
    Serial.print(cfg.resolucion, 6);
    Serial.print(',');
    Serial.print(cfg.x1t, 6);
    Serial.print(',');
    Serial.print(cfg.x2t, 6);
    Serial.print(',');
    Serial.print(cfg.x3t, 6);
    Serial.print(',');
    Serial.print(cfg.x4t, 6);
    Serial.print(',');
    Serial.print(cfg.x1c, 6);
    Serial.print(',');
    Serial.print(cfg.x2c, 6);
    Serial.print(',');
    Serial.print(cfg.x3c, 6);
    Serial.print(',');
    Serial.print(cfg.x4c, 6);
    Serial.print(',');
    Serial.print(cfg.overload_t);
    Serial.print(',');
    Serial.println(cfg.overload_c);
}

bool extraerPayloadCellConfig(const uint8_t* Buf, uint32_t Len, char* destino, size_t destinoSize) {
    if (!VerificarFormato(reinterpret_cast<const char*>(Buf), Len) || destinoSize == 0) {
        return false;
    }

    const uint32_t payloadStart = 4;             // | + comando (2) + |
    const uint32_t payloadEnd   = Len > 2 ? Len - 2 : 0;  // Índice del primer '|' final

    if (payloadEnd <= payloadStart) {
        destino[0] = '\0';
        return true;
    }

    uint32_t payloadLength = payloadEnd - payloadStart;
    if (payloadLength >= destinoSize) {
        payloadLength = destinoSize - 1;
    }

    memcpy(destino, Buf + payloadStart, payloadLength);
    destino[payloadLength] = '\0';
    return true;
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
        {"RR", CommandRR, 29}, // Leer datarate actual
    {"RI", CommandRI, 10},
        {"RC", CommandRC, 11},
        {"RX", CommandRX, 12}, // Hay extensometro ?
        {"WM", CommandWM, 13}, // Modo remoto
        {"RV", CommandRV, 14}, // Velocdidad maxima ?
        {"WV", CommandWV, 15},
        {"WI", CommandWV, 16},
        {"WO", CommandWO, 17}, // Ajuste offset analógico en volts
        {"RO", CommandROffset, 18},
        {"WE", CommandWE, 19},
        {"RE", CommandRE, 20},
        {"WP", CommandWP, 21},
        {"RP", CommandRP, 22},
        {"R1", CommandR1, 23},
        {"R2", CommandR2, 24},
        {"R3", CommandR3, 30},
        {"RS", CommandRS, 25},
        {"RH", CommandRH, 26}, // Ensayo en curso ?
        {"WB", CommandRS, 27}, // Alarma baja velo
        {"WT", CommandWT, 28},
        {"WZ", CommandWZ, 31}, // Cero de fuerza vía kernel
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
    if (Len >= 4 && Buf[0] == '|' && Buf[1] == 'C') {
        if (Buf[2] == 'R') {
            if (!cellConfigLoaded) {
                cargarCellConfigDesdeEeprom(cellConfigCache);
                cellConfigLoaded = true;
            }
            imprimirCellConfig(cellConfigCache);
            return true;
        } else if (Buf[2] == 'W') {
            char payload[200];
            if (!extraerPayloadCellConfig(Buf, Len, payload, sizeof(payload))) {
                Serial.println("ERR");
                return true;
            }

            CellConfig nueva{};
            if (!parseCellConfigString(payload, nueva)) {
                Serial.println("ERR");
                return true;
            }

            guardarCellConfigEnEeprom(nueva);
            cellConfigCache  = nueva;
            cellConfigLoaded = true;
            Serial.println("OK");
            return true;
        }
    }

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

    const char* parametros = (l > 2) ? mensaje + 2 : "";

    if (strcmp(comando, "WP") == 0 || strcmp(comando, "RP") == 0) {
        if (procesarComandoConfiguracion(comando, parametros)) {
            return true;
        }
        Serial.println("ERR");
        return true;
    }

    float param1 = 0.0f;
    float param2 = 0.0f;
    // Si hay más caracteres, convertirlos a número (desde la posición 2)
    if (l > 2) {
        param1 = atof(parametros);
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
        if (ProcesarComandoNuevo(Buf, Len)) {
            return true;
        }

        // Compatibilidad: si el formato con tuberías falla, intentar el protocolo viejo
        protocoloActual = PROTOCOLO_VIEJO;
        return ProcesarComandoViejo(Buf, Len);
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
