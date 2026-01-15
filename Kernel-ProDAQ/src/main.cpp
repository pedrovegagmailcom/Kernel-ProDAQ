/*

  PortentaKernel.ino

*/

#include <Arduino.h>
#include <mbed.h>
#include <rtos.h>
#include <string.h>
using namespace rtos;
#include "stm32h7xx.h"
#include "modelo.h"
#include "AD7175.h"
#include "LS7366.h"
#include "LTC2602.h"
#include "mcp23s08.h"
#include "IO.h"
#include "alarmas.h"
#include "AlarmEvaluator.h"
#include "RawOverloadEvaluator.h"
#include "tramos.h"
#include "GestionComandos.h"
#include "utilidades.h"
#include "cell_config.h"
#include "MachineMode.h"

void SerialEvent();
void enviarDatosSensor(DatosSensor datos);
void SensorUpdateLoop();
int32_t TaraCelula();

LTC2602 LTCdac;
LS7366 Encoder;
IO IOsystem;
Alarmas alarmas(IOsystem);
AlarmEvaluator alarmEvaluator;
RawOverloadEvaluator rawOverloadEvaluator;

Thread RecepcionComms;
Thread TransmisionComms;
Thread SensorUpdateThread;
rtos::Mutex sensorDataMutex;
rtos::Mutex cellConfigMutex;

DatosSensor sensorData;
CellConfig cellConfigActual = {};
uint32_t estado_maquina;
volatile uint32_t dataRate = 100;
bool transmitirDatos = false;
bool ad7175Inicializado = false;

void setup() {
  
  
  Serial.begin(115200);      // USB serial interrupts already in place 
  delay(1000); 
  SPI.begin();
 
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.println("Booting Kernel...");
  
  LTCdac.begin();
  IOsystem.begin();
  Encoder.begin();
  alarmas.inicializar();
  InicializarConfiguracion();

  if (!readCellConfig(cellConfigActual)) {
    memset(&cellConfigActual, 0, sizeof(cellConfigActual));
  }


  if (AD7175_Setup() != 0) {
    Serial.println("AD7175 Initialization Failed");
  } else {
    ad7175Inicializado = true;
    Serial.println("AD7175 Initialized Successfully");
  }

   Parar();
  //TaraCelula();

  //TransmisionComms.start(mbed::callback(TransmisionLoop));
  //TransmisionComms.set_priority(osPriorityHigh);

  SensorUpdateThread.start(mbed::callback([] { SensorUpdateLoop(); }));
  SensorUpdateThread.set_priority(osPriorityHigh);

  RecepcionComms.start(mbed::callback(SerialEvent));
  RecepcionComms.set_priority(osPriorityHigh);

#ifdef UNIT_TEST
  RunAlarmEvaluatorTests();
#endif
  
  
}




void loop() {

}


void TransmisionLoop() {
  uint32_t lastSendTime = osKernelGetTickCount();  
  while (true) {
    if (transmitirDatos && dataRate > 0) {
      uint32_t currentTime = osKernelGetTickCount();
      uint32_t interval_ms = 1000 / dataRate;  

      
      if ((uint32_t)(currentTime - lastSendTime) >= interval_ms) {
        sensorDataMutex.lock();
        DatosSensor datos = sensorData;
        sensorDataMutex.unlock();

        enviarDatosSensor(datos);
        lastSendTime += interval_ms;
      }
    }
    osDelay(1);
  }
}




constexpr float   VREF_V            = 3.300f; 
constexpr int     PGA_GAIN          = 1;      
constexpr float   F_FULLSCALE_N     = 50.0f;  
constexpr float   CELL_SENS_MV_PER_V= 2.0f;   //  2 mV/V 
constexpr float   EXCITATION_V      = 10.0f;   


float g_offsetVolt = 0.0f;

static inline float evalCubic(float x, float a0, float a1, float a2, float a3) {
  return ((a3 * x + a2) * x + a1) * x + a0;
}

static float applyCellPoly(float Finternal, const CellConfig& c) {
  const float x = fabsf(Finternal);
  if (Finternal >= 0.0f) {
    float y = evalCubic(x, c.x1t, c.x2t, c.x3t, c.x4t);
    return y;
  }

  float y = evalCubic(x, c.x1c, c.x2c, c.x3c, c.x4c);
  return -y;
}

static int32_t filtroRC_counts(int32_t present_reading) {
  static int32_t last_output = 0;
  const int32_t x = 5;

  int32_t var1 = (x * last_output + present_reading);
  last_output = var1 / (x + 1);
  return last_output;
}


void SensorUpdateLoop() {
  float ultimaFuerzaLeida = 0.0f;
  float fuerzaBase = 0.0f;
  int32_t raw = 0;
  int32_t raw_unfiltered = 0;
  uint32_t lastHwCheckMs = 0;
  bool rawEvaluatorInit = false;

  while (true) {
    // Comprobar alarmas cableadas a una cadencia menor.
    uint32_t nowMs = millis();
    if ((uint32_t)(nowMs - lastHwCheckMs) >= 20U) {
      alarmas.comprobarHW();
      lastHwCheckMs = nowMs;
    }

    // Lectura de la celda de carga a través del AD7175.
    if (ad7175Inicializado && AD7175_WaitForReady(5) == 0) {
      
      if (AD7175_ReadData(&raw_unfiltered) == 0) {
        if (!rawEvaluatorInit) {
          rawOverloadEvaluator.setThresholds(0x780000, 0x740000);
          rawOverloadEvaluator.setTiming(2, 5);
          rawEvaluatorInit = true;
        }

        RawOverloadResult rawAlarmas = rawOverloadEvaluator.update(
            raw_unfiltered, g_modoCompresometro);
        alarmas.setSwAlarm(Alarmas::A_TRAC, rawAlarmas.trac);
        alarmas.setSwAlarm(Alarmas::A_COMP, rawAlarmas.comp);

        raw = filtroRC_counts(raw_unfiltered);

        // raw ya es 24 bits con signo extendido (bipolar)
        // Fuerza en Newtons usando la calibración 0 kg / 1 kg
        float fuerzaN = raw;

        fuerzaBase = fuerzaN; // Ajusta este offset según tu tara
      }
    }

    // Lectura del encoder y conversión a mm.
    long  contador    = Encoder.read_counter();
    float extensionMm = convertirContadorAMilimetros(contador);

    // Evaluación de alarmas software (sobrecarga + config).
    CellConfig configSnapshot = {};
    cellConfigMutex.lock();
    configSnapshot = cellConfigActual;
    cellConfigMutex.unlock();

    float Finternal = g_modoCompresometro ? -fabsf(fuerzaBase) : fuerzaBase;

    Finternal = applyCellPoly(Finternal, configSnapshot);
    ultimaFuerzaLeida = Finternal;

    DatosSensor snapshotSensor = {};
    snapshotSensor.fuerza = ultimaFuerzaLeida;

    AlarmEvaluator::Result swAlarmas = alarmEvaluator.update(snapshotSensor, configSnapshot);
    alarmas.setSwAlarm(Alarmas::A_CELULA, swAlarmas.cellConfigFault);

    
    // Empaquetar estado para el canal de comunicación.
    uint32_t estadoCombinado = 0;
    estadoCombinado |= static_cast<uint32_t>(alarmas.getAlarmas());
    estadoCombinado |= static_cast<uint32_t>(alarmas.getStatus()) << 8;
    estadoCombinado |= estado_maquina << 16;

    // Publicar datos de sensor de forma atómica.
    sensorDataMutex.lock();
    sensorData.fuerza    = ultimaFuerzaLeida;
    sensorData.voltaje  = AD7175_Voltage(raw);
    sensorData.extension = extensionMm;
    sensorData.estado    = estadoCombinado;
    sensorData.timestamp = millis();
    sensorDataMutex.unlock();

    // Periodo de muestreo del lazo de sensores (~10 ms).
    osDelay(0);
  }
}

#ifdef UNIT_TEST
void RunAlarmEvaluatorTests() {
  Serial.println("=== AlarmEvaluator tests ===");

  AlarmEvaluator evaluator;
  CellConfig config = {};
  strncpy(config.numeroserie, "CELL1234", sizeof(config.numeroserie));
  config.capacidad = 100;
  config.limite = 80;
  config.resolucion = 1.0f;
  config.overload_trac_count = 50;
  config.overload_comp_count = 40;

  DatosSensor datos = {};
  alarmas.setSwAlarm(Alarmas::A_TRAC, false);
  alarmas.setSwAlarm(Alarmas::A_COMP, false);
  alarmas.setSwAlarm(Alarmas::A_CELULA, false);

  // 1) Fuerza supera por menos de N_ON -> no activa TRAC.
  datos.fuerza = 55.0f;
  AlarmEvaluator::Result r1 = evaluator.update(datos, config);
  delay(10);
  r1 = evaluator.update(datos, config);
  alarmas.setSwAlarm(Alarmas::A_TRAC, r1.trac);
  alarmas.setSwAlarm(Alarmas::A_COMP, r1.comp);
  alarmas.setSwAlarm(Alarmas::A_CELULA, r1.cellConfigFault);
  Serial.print("Test1 Alarmas: ");
  Serial.println(alarmas.getAlarmas(), BIN);

  // 2) Fuerza mantiene por encima N_ON -> activa TRAC.
  delay(15);
  AlarmEvaluator::Result r2 = evaluator.update(datos, config);
  alarmas.setSwAlarm(Alarmas::A_TRAC, r2.trac);
  alarmas.setSwAlarm(Alarmas::A_COMP, r2.comp);
  alarmas.setSwAlarm(Alarmas::A_CELULA, r2.cellConfigFault);
  Serial.print("Test2 Alarmas: ");
  Serial.println(alarmas.getAlarmas(), BIN);

  // 3) Oscila sin cruzar OFF -> se mantiene activa.
  datos.fuerza = 49.5f;
  delay(50);
  AlarmEvaluator::Result r3 = evaluator.update(datos, config);
  alarmas.setSwAlarm(Alarmas::A_TRAC, r3.trac);
  alarmas.setSwAlarm(Alarmas::A_COMP, r3.comp);
  alarmas.setSwAlarm(Alarmas::A_CELULA, r3.cellConfigFault);
  Serial.print("Test3 Alarmas: ");
  Serial.println(alarmas.getAlarmas(), BIN);

  // 4) Baja por debajo de OFF durante N_OFF -> se limpia TRAC.
  datos.fuerza = 45.0f;
  delay(50);
  AlarmEvaluator::Result r4 = evaluator.update(datos, config);
  alarmas.setSwAlarm(Alarmas::A_TRAC, r4.trac);
  alarmas.setSwAlarm(Alarmas::A_COMP, r4.comp);
  alarmas.setSwAlarm(Alarmas::A_CELULA, r4.cellConfigFault);
  Serial.print("Test4 Alarmas: ");
  Serial.println(alarmas.getAlarmas(), BIN);

  // 5) Config inválida -> A_CELULA activo e inhibe TRAC/COMP.
  CellConfig invalidConfig = {};
  datos.fuerza = 60.0f;
  AlarmEvaluator::Result r5 = evaluator.update(datos, invalidConfig);
  alarmas.setSwAlarm(Alarmas::A_TRAC, r5.trac);
  alarmas.setSwAlarm(Alarmas::A_COMP, r5.comp);
  alarmas.setSwAlarm(Alarmas::A_CELULA, r5.cellConfigFault);
  Serial.print("Test5 Alarmas: ");
  Serial.println(alarmas.getAlarmas(), BIN);
}
#endif




#define STX 0x02
#define ETX 0x03
#define DLE 0x10  // Data Link Escape

void enviarDatosSensor(DatosSensor datos) {
  datos.checksum = calcularChecksum(&datos);  // Calcular y actualizar el checksum
  byte* p = (byte*)&datos;

  Serial.write(STX);  // STX
  for (size_t i = 0; i < sizeof(DatosSensor); i++) {
    byte b = p[i];
    // Si el byte es STX, ETX o DLE, lo escapamos
    if (b == STX || b == ETX || b == DLE) {
      Serial.write(DLE);
      Serial.write(b ^ 0xFF);  // Invertimos los bits para diferenciar
    } else {
      Serial.write(b);
    }
  }
  Serial.write(ETX);  // ETX
}

char inputTramos[15000];
char inputString[15000];


void SerialEvent() {
    static bool stringComplete = false;
    static uint32_t i = 0;

    
    
    while (true) {
        // 1) Acumular caracteres hasta '\n'
        while (Serial.available() && i < sizeof(inputString) - 1) {
            char inChar = (char)Serial.read();
            if (inChar == '\r') {
                stringComplete = true;
                break;
            } else {
                inputString[i++] = inChar;
            }
        }
        inputString[i] = '\0';  // Termina la cadena

        // 2) Si se recibió la línea completa, la procesamos
        if (stringComplete) {
            ProcesarMensaje((uint8_t*)inputString, i);
            
            //handleSerialLine(inputString);

            // Reiniciar para el siguiente comando
            i = 0;
            stringComplete = false;
            memset(inputString, 0, sizeof(inputString));
        }

        //osDelay(1);
    }
}


int32_t TaraCelula()
{
    // Asegúrate de que no haya peso en la célula antes de llamar.
    if (!ad7175Inicializado) {
        return -1;
    }

    // Opcional: podrías comprobar aquí que la máquina está parada
    // usando las mismas señales que en SensorUpdateLoop.

    return AD7175_SystemZeroScaleCalibrate();
}


/*

void SerialEvent__() {
  
  
  static bool stringComplete = false;
  uint i = 0;
  while (true) {

    while (Serial.available() && i < sizeof(inputString) - 1) {  // Evitar desbordamiento
      char inChar = (char)Serial.read();

      if (inChar == '\n') {
        stringComplete = true;
        break;  // Salir del bucle si se completa el comando
      } else {
        inputString[i++] = inChar;
      }
    }
    inputString[i] = '\0';  // Terminar la cadena
    int len = i;

    

    if (stringComplete) {

      // Verificar si el string contiene "S2| (Tramos)"
      char* s2Pointer = strstr(inputString, "S2|");
      if (s2Pointer != nullptr) {
        // Extraer el contenido después de "S2|" hasta el próximo delimitador o el final
        s2Pointer += 3;  // Avanzar el puntero para omitir "S2|"
        size_t  j = 0;
        while (*s2Pointer != '\0' && *s2Pointer != '\n' && *s2Pointer != '\r' && *s2Pointer != '|' && j < sizeof(inputTramos) - 1) {
          inputTramos[j++] = *s2Pointer++;
        }
        inputTramos[j] = '\0';  // Terminar el buffer
        
        // DEBUG :
        TramoDTO tramos[MAX_TRAMOS];
        DebugTramos(inputTramos, tramos);
    
      }
    else {
      
        char comando[50];
        float param1, param2;

        if (AnalizarComando(inputString, len, comando, &param1, &param2)) {
          ProcesarComando(comando, param1, param2);
        }
      } 
       
      // Reiniciar variables para el siguiente comando
      i = 0;
      stringComplete = false;
      memset(inputString, 0, sizeof(inputString));
      
    }

    osDelay(1);
  }

}

*/
