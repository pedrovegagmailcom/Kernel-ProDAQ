/*

  PortentaKernel.ino

*/

#include <Arduino.h>
#include <mbed.h>
#include <rtos.h>
using namespace rtos;
#include "stm32h7xx.h"
#include "inc/modelo.h"
#include "AD7175.h"
#include "LS7366.h"
#include "LTC2602.h"
#include "mcp23s08.h"
#include "IO.h"
#include "tramos.h"
#include "GestionComandos.h"
#include "utilidades.h"


LTC2602 LTCdac;
//MCP23S08 mcp(PC_15);
//LS7366 Encoder;

//IO IOsystem;

Thread RecepcionComms;
Thread TransmisionComms;

DatosSensor sensorData;
uint32_t estado_maquina;
volatile uint32_t dataRate = 100;
bool transmitirDatos = false;

void setup() {
  
  
  Serial.begin(115200);      // USB serial interrupts already in place 
  delay(1000); 
  SPI.begin();
 
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.println("Booting Kernel...");
  
  LTCdac.begin();
  //mcp.begin();
  //IOsystem.begin();
  //Encoder.begin();
 

  
  
/*
 if (AD7175_Setup() != 0) {
        Serial.println("AD7175 Initialization Failed");
        
    } else {
        Serial.println("AD7175 Initialized Successfully");
    }
*/
/*
  // Rellenamos algunos datos de ejemplo
  sensorData.fuerza = 123.45;
  sensorData.extension = 0;
  sensorData.timestamp = millis();
  sensorData.estado = 0x01;
  sensorData.statusReg = 0x02;
  sensorData.modeReg = 0x03;
  sensorData.ch0Reg = 0x04;
  sensorData.ch0GainReg = 0x05;
  sensorData.maxForce = 500.0;
  sensorData.checksum = 0xFFFF;
*/

  //TransmisionComms.start(mbed::callback(TransmisionLoop));
  //TransmisionComms.set_priority(osPriorityHigh);

  //RecepcionComms.start(mbed::callback(SerialEvent));
  //RecepcionComms.set_priority(osPriorityHigh);

  
}

/*

void loop1() {


    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
    delay(500); 
  
    byte inputs = mcp.readRegister(0x09);
    Serial.println(inputs, BIN);  // Mostrar en binario
    Serial.println("loop()");

    digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
    delay(500);    
    
}

*/
constexpr float   VREF_V = 2.500f; // Voltaje de referencia (V)
constexpr int     PGA_GAIN = 1;    // Ganancia del PGA usada en SETUP0

void loop_analog() {
  if (AD7175_WaitForReady(200)) {
    int32_t raw = 0;
    if (AD7175_ReadData(&raw)) {
      // raw es 24 bits con sign-extend a 32 (bipolar, 2^23 a FS)
      // LSB (V) = Vref / (Gain * 2^23)
      const float lsb_V = VREF_V / (PGA_GAIN * 8388608.0f); // 2^23 = 8,388,608
      float volts = raw * lsb_V;

      Serial.print("RAW= ");
      Serial.print(raw);
      Serial.print("\tV= ");
      Serial.println(volts, 6);
    } else {
      Serial.println("Error de lectura de DATA");
    }
  } else {
    Serial.println("Timeout esperando dato listo");
  }

  delay(500);
}

/*
void loop() {


    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
    delay(500); 
  
    unsigned long  valor = Encoder.read_counter();
    Serial.println(valor);  // Mostrar en binario
    

    digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
    delay(1500);    
    
}





void loop() {

  uint16_t entradas = 0;//IOsystem.readInputs();   // Lee las 12 entradas
  IOsystem.writeOutputs(43690);             // Refleja las entradas en las salidas

  Serial.println(entradas, BIN);  // muestra algo como 101101000111

  delay(200);

}
*/


/*

void loop3() {


    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
    
  
    LTCdac.setOutput(0, 0);
    LTCdac.setOutput(1, 0);

    digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
    delay(2000);   
    
    LTCdac.setOutput(0, 32766);
    LTCdac.setOutput(1, 32766); 

    delay(2000);   
    
    LTCdac.setOutput(0, 65535);
    LTCdac.setOutput(1, 65535); 
    delay(2000);
    
}

*/
// Loop para probar salidas analogicas  

void loop_org() {
  // Ciclo de subida en 1 segundo (0 -> 65535)
  // Usamos 101 pasos: de i=0 a i=100
  for (int i = 0; i <= 100; i++) {
    // Escalamos 'i' a un rango de 0 a ~65500
    uint16_t value = i * 655; // 655 aprox (65535 / 100)
    
    // Ajustamos ambos canales con el mismo valor
    LTCdac.setOutput(0, value);
    LTCdac.setOutput(1, value);
    
    // Retardo de 10 ms para que 101 pasos tomen ~1 segundo
    delay(100);
    Serial.println(value);
 
  }

  // Ciclo de bajada en 1 segundo (65535 -> 0)
  for (int i = 100; i >= 0; i--) {
    uint16_t value = i * 655;
    LTCdac.setOutput(0, value);
    LTCdac.setOutput(1, value);
    delay(100);
    Serial.println(value);
  }
}


void loop() {
digitalWrite(LED_BUILTIN, HIGH);
LTCdac.setOutput(0, 0);
LTCdac.setOutput(1, 0);

delay(100);

digitalWrite(LED_BUILTIN, LOW);
LTCdac.setOutput(0, 65535);
LTCdac.setOutput(1, 65535);

delay(100);

 
}


void TransmisionLoop() {
  uint32_t lastSendTime = osKernelGetTickCount();  
  while (true) {
    if (transmitirDatos && dataRate > 0) {
      uint32_t currentTime = osKernelGetTickCount();
      uint32_t interval_ms = 1000 / dataRate;  

      
      if ((uint32_t)(currentTime - lastSendTime) >= interval_ms) {
        sensorData.extension += 1;
        sensorData.estado = estado_maquina;
        enviarDatosSensor(sensorData);
        lastSendTime += interval_ms; 
      }
    }
    osDelay(1); 
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


void serialEvent() {
    static bool stringComplete = false;
    static uint32_t i = 0;

    Serial.println("debug");
    return;
    while (true) {
        // 1) Acumular caracteres hasta '\n'
        while (Serial.available() && i < sizeof(inputString) - 1) {
            char inChar = (char)Serial.read();
            if (inChar == '\n') {
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
            Serial.println(inputString);
            //handleSerialLine(inputString);

            // Reiniciar para el siguiente comando
            i = 0;
            stringComplete = false;
            memset(inputString, 0, sizeof(inputString));
        }

        osDelay(1);
    }
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
