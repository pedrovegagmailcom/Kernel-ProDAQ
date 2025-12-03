/*

  PortentaKernel.ino

*/

#include <Arduino.h>
#include <mbed.h>
#include <rtos.h>
using namespace rtos;
#include "stm32h7xx.h"
#include "modelo.h"
#include "AD7175.h"
#include "LS7366.h"
#include "LTC2602.h"
#include "mcp23s08.h"
#include "IO.h"
#include "alarmas.h"
#include "tramos.h"
#include "GestionComandos.h"
#include "utilidades.h"

void SerialEvent();
void enviarDatosSensor(DatosSensor datos);
void SensorUpdateLoop();
int32_t TaraCelula();

LTC2602 LTCdac;
LS7366 Encoder;
IO IOsystem;
Alarmas alarmas(IOsystem);

Thread RecepcionComms;
Thread TransmisionComms;
Thread SensorUpdateThread;
rtos::Mutex sensorDataMutex;

DatosSensor sensorData;
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



  if (AD7175_Setup() != 0) {
    Serial.println("AD7175 Initialization Failed");
  } else {
    ad7175Inicializado = true;
    Serial.println("AD7175 Initialized Successfully");
  }

  //TaraCelula();

  //TransmisionComms.start(mbed::callback(TransmisionLoop));
  //TransmisionComms.set_priority(osPriorityHigh);

  SensorUpdateThread.start(mbed::callback([] { SensorUpdateLoop(); }));
  SensorUpdateThread.set_priority(osPriorityHigh);

  RecepcionComms.start(mbed::callback(SerialEvent));
  RecepcionComms.set_priority(osPriorityHigh);

  
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




constexpr float   VREF_V            = 3.300f; // ya lo tienes definido
constexpr int     PGA_GAIN          = 1;      // el que uses en el SETUP0
constexpr float   F_FULLSCALE_N     = 50.0f;  // tu célula es de 50 N
constexpr float   CELL_SENS_MV_PER_V= 2.0f;   // EJEMPLO: 2 mV/V (cámbialo)
constexpr float   EXCITATION_V      = 10.0f;   // tensión que le das al puente

constexpr float RAW_ZERO          = 0;//8.327;                   // lectura sin carga
constexpr float SCALE_N_PER_COUNT = 0.000007906f;

float g_offsetVolt = 0.0f;


void SensorUpdateLoop() {
  float ultimaFuerzaLeida = 0.0f;
  int32_t raw = 0;

  while (true) {
    // Comprobar alarmas de IO / seguridad / etc.
    alarmas.comprobar();

    // Lectura de la celda de carga a través del AD7175.
    if (ad7175Inicializado && AD7175_WaitForReady(5) == 0) {
      
      if (AD7175_ReadData(&raw) == 0) {

        // raw ya es 24 bits con signo extendido (bipolar)
        // Fuerza en Newtons usando la calibración 0 kg / 1 kg
        float fuerzaN = (raw) * SCALE_N_PER_COUNT;

        ultimaFuerzaLeida = (fuerzaN - RAW_ZERO); // Ajusta este offset según tu tara
      }
    }

    // Lectura del encoder y conversión a mm.
    long  contador    = Encoder.read_counter();
    float extensionMm = convertirContadorAMilimetros(contador);

    
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
    sensorData.extension = raw;
    sensorData.estado    = estadoCombinado;
    sensorData.timestamp = millis();
    sensorDataMutex.unlock();

    // Periodo de muestreo del lazo de sensores (~10 ms).
    osDelay(0);
  }
}




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
