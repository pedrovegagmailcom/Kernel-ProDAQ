/*

  tramos.cpp

*/

#include <Arduino.h>
#include <ArduinoJson.h>
#include "tramos.h"

TramoDTO g_tramos[MAX_TRAMOS];
int g_tramoCount = 0;

void borrarTramosEnMemoria() {
    g_tramoCount = 0;
    // Opcional: limpiar g_tramos con memset(...)
}

bool agregarTramoEnMemoria(const TramoDTO &tramo) {
    if (g_tramoCount >= MAX_TRAMOS) {
        return false; // Lista llena
    }
    g_tramos[g_tramoCount++] = tramo;
    return true;
}

bool parseSingleTramo(const char* jsonString, TramoDTO &tramo) {
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, jsonString);
    if (error) {
        return false;
    }
    // Completar parseo como se requiera, e.g.:
    // tramo.tipoTramo = doc["TipoTramo"] | 0;
    // ...
    return true;
}


bool parseTramosDTO(const char* jsonString, TramoDTO tramos[], int &tramoCount) {
    // Es necesario definir un tamaño para el doc. Por ejemplo:
    JsonDocument doc;
    
    DeserializationError error = deserializeJson(doc, jsonString);

    if (error) {
        Serial.println(F("Error parseando el JSON"));
        return false;
    }

    // Como el JSON es un array en la raíz, se obtiene directamente
    JsonArray arrTramos = doc.as<JsonArray>();
    if (arrTramos.isNull()) {
        Serial.println(F("No se encontró el array de tramos en la raíz"));
        return false;
    }

    tramoCount = 0;
    for (JsonObject tramoObj : arrTramos) {
        if (tramoCount >= MAX_TRAMOS) {
            Serial.println(F("Se superó el número máximo de tramos"));
            break;
        }

        TramoDTO &tramo = tramos[tramoCount++];
        
        tramo.tipoTramo = (TiposTramoEnum)(tramoObj["TipoTramo"].as<int>());
        
        // Parseo de ListaParametros
        JsonArray parametros = tramoObj["ListaParametros"].as<JsonArray>();
        tramo.parametroCount = 0;
        if (!parametros.isNull()) {
            for (JsonObject p : parametros) {
                if (tramo.parametroCount >= MAX_PARAMETROS) break;
                Parametro &param = tramo.listaParametros[tramo.parametroCount++];
                
                param.codigo = p["Codigo"] | 0;

                const char* nombre = p["Nombre"] | "";
                strncpy(param.nombre, nombre, MAX_NAME_LENGTH);
                param.nombre[MAX_NAME_LENGTH - 1] = '\0';

                param.valorDouble = p["ValorDouble"] | 0.0;
                param.valorBool = p["ValorBool"] | false;

                if (!p["LimiteRangoMinimo"].isNull()) {
                    param.limiteRangoMinimo = p["LimiteRangoMinimo"].as<double>();
                    param.hasLimiteRangoMinimo = true;
                } else {
                    param.hasLimiteRangoMinimo = false;
                }

                if (!p["LimiteRangoMaximo"].isNull()) {
                    param.limiteRangoMaximo = p["LimiteRangoMaximo"].as<double>();
                    param.hasLimiteRangoMaximo = true;
                } else {
                    param.hasLimiteRangoMaximo = false;
                }
            }
        }

        // Parseo de ListaLimites
        JsonArray limites = tramoObj["ListaLimites"].as<JsonArray>();
        tramo.limiteCount = 0;
        if (!limites.isNull()) {
            for (JsonObject l : limites) {
                if (tramo.limiteCount >= MAX_LIMITES) break;
                Limite &lim = tramo.listaLimites[tramo.limiteCount++];
                
                const char* nom = l["Nombre"] | "";
                strncpy(lim.nombre, nom, MAX_NAME_LENGTH);
                lim.nombre[MAX_NAME_LENGTH - 1] = '\0';

                lim.valor = l["Valor"] | 0.0;
                lim.tipo = l["Tipo"] | 0;

                const char* unidad = l["Unidad"] | "";
                strncpy(lim.unidad, unidad, MAX_NAME_LENGTH);
                lim.unidad[MAX_NAME_LENGTH - 1] = '\0';

                lim.magnitud = l["Magnitud"] | 0;
                lim.activo = l["Activo"] | false;
            }
        }
    }

    return true;  
}




bool parseTramosDTO__(const char* jsonString, TramoDTO tramos[], int &tramoCount) {
    JsonDocument doc;  
    DeserializationError error = deserializeJson(doc, jsonString);

    if (error) {
        Serial.println(F("Error parseando el JSON"));
        return false;
    }

    JsonArray arrTramos = doc["Tramos"].as<JsonArray>();
    if (arrTramos.isNull()) {
        Serial.println(F("No se encontró el array 'Tramos'"));
        return false;
    }

    tramoCount = 0;
    for (JsonObject tramoObj : arrTramos) {
        if (tramoCount >= MAX_TRAMOS) {
            Serial.println(F("Se superó el número máximo de tramos"));
            break;
        }

        TramoDTO &tramo = tramos[tramoCount++];
        
        tramo.tipoTramo = (TiposTramoEnum)(tramoObj["TipoTramo"].as<int>());
        
        // Parseo de ListaParametros
        JsonArray parametros = tramoObj["ListaParametros"].as<JsonArray>();
        tramo.parametroCount = 0;
        if (!parametros.isNull()) {
            for (JsonObject p : parametros) {
                if (tramo.parametroCount >= MAX_PARAMETROS) break;
                Parametro &param = tramo.listaParametros[tramo.parametroCount++];
                
                param.codigo = p["Codigo"] | 0;

                const char* nombre = p["Nombre"] | "";
                strncpy(param.nombre, nombre, MAX_NAME_LENGTH);
                param.nombre[MAX_NAME_LENGTH - 1] = '\0';

                param.valorDouble = p["ValorDouble"] | 0.0;
                param.valorBool = p["ValorBool"] | false;

                if (!p["LimiteRangoMinimo"].isNull()) {
                    param.limiteRangoMinimo = p["LimiteRangoMinimo"].as<double>();
                    param.hasLimiteRangoMinimo = true;
                } else {
                    param.hasLimiteRangoMinimo = false;
                }

                if (!p["LimiteRangoMaximo"].isNull()) {
                    param.limiteRangoMaximo = p["LimiteRangoMaximo"].as<double>();
                    param.hasLimiteRangoMaximo = true;
                } else {
                    param.hasLimiteRangoMaximo = false;
                }
            }
        }

        // Parseo de ListaLimites
        JsonArray limites = tramoObj["ListaLimites"].as<JsonArray>();
        tramo.limiteCount = 0;
        if (!limites.isNull()) {
            for (JsonObject l : limites) {
                if (tramo.limiteCount >= MAX_LIMITES) break;
                Limite &lim = tramo.listaLimites[tramo.limiteCount++];
                
                const char* nom = l["Nombre"] | "";
                strncpy(lim.nombre, nom, MAX_NAME_LENGTH);
                lim.nombre[MAX_NAME_LENGTH - 1] = '\0';

                lim.valor = l["Valor"] | 0.0;
                lim.tipo = l["Tipo"] | 0;

                const char* unidad = l["Unidad"] | "";
                strncpy(lim.unidad, unidad, MAX_NAME_LENGTH);
                lim.unidad[MAX_NAME_LENGTH - 1] = '\0';

                lim.magnitud = l["Magnitud"] | 0;
                lim.activo = l["Activo"] | false;
            }
        }
    }

    return true;  
}

void DebugTramos(const char* jsonString, TramoDTO tramos[]) {
  
  int tramoCount = 0;
  if (parseTramosDTO(jsonString, tramos, tramoCount)) {
        Serial.print("Se parsearon ");
        Serial.print(tramoCount);
        Serial.println(" tramos.");

        for (int i = 0; i < tramoCount; i++) {
            Serial.print("Tramo ");
            Serial.print(i);
            Serial.print(" TipoTramo: ");
            Serial.println((int)tramos[i].tipoTramo);

            Serial.print("  Parametros encontrados: ");
            Serial.println(tramos[i].parametroCount);
            for (int p = 0; p < tramos[i].parametroCount; p++) {
                Serial.print("    Param ");
                Serial.print(p);
                Serial.print(": Nombre=");
                Serial.print(tramos[i].listaParametros[p].nombre);
                Serial.print(" ValorDouble=");
                Serial.print(tramos[i].listaParametros[p].valorDouble);
                Serial.print(" ValorBool=");
                Serial.println(tramos[i].listaParametros[p].valorBool);
            }

            Serial.print("  Limites encontrados: ");
            Serial.println(tramos[i].limiteCount);
            for (int l = 0; l < tramos[i].limiteCount; l++) {
                Serial.print("    Limite ");
                Serial.print(l);
                Serial.print(": Nombre=");
                Serial.print(tramos[i].listaLimites[l].nombre);
                Serial.print(" Valor=");
                Serial.print(tramos[i].listaLimites[l].valor);
                Serial.print(" Tipo=");
                Serial.print(tramos[i].listaLimites[l].tipo);
                Serial.print(" Unidad=");
                Serial.print(tramos[i].listaLimites[l].unidad);
                Serial.print(" Magnitud=");
                Serial.print(tramos[i].listaLimites[l].magnitud);
                Serial.print(" Activo=");
                Serial.println(tramos[i].listaLimites[l].activo);
            }
        }

    } else {
        Serial.println("Error al parsear tramos");
    }
  }