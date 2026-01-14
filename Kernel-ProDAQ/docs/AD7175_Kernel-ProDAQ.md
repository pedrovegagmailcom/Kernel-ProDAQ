# AD7175 en Kernel-ProDAQ

> **Estado:** Borrador inicial (documentación viva)

## 1. Propósito de este documento
Este documento describe **cómo se utiliza el ADC AD7175 en el proyecto Kernel-ProDAQ**, qué decisiones de configuración se han tomado y **qué asume explícitamente el kernel** sobre su comportamiento.

No sustituye al datasheet oficial de Analog Devices. El datasheet se considera **material de referencia** y se encuentra en `docs/references/ad7175/`.

---

## 2. Rol del AD7175 en el sistema
El AD7175 es el **convertidor analógico–digital principal** para la medida de fuerza procedente de la célula de carga.

Funciones clave en el sistema:
- Conversión diferencial de alta resolución (24 bits)
- Fuente primaria de la magnitud **fuerza**
- Entrada directa a la cadena de:
  - conversión a Newtons
  - calibración por polinomio
  - evaluación de alarmas

El AD7175 es, por tanto, **un componente crítico de seguridad funcional**.

---

## 3. Variante de chip y referencia
- **Chip utilizado:** AD7175 (familia AD717x)
- **Referencia:** según esquemático de la electrónica ProDAQ (ver documentación HW externa)

> Nota: cualquier cambio de variante (-2 / -8) o de referencia debe considerarse **ruptura de contrato** con el kernel y exige revisión de este documento.

---

## 4. Configuración funcional adoptada

### 4.1 Modo de entrada: diferencial
El kernel asume que la señal de la célula llega al AD7175 como **señal diferencial** procedente del acondicionamiento analógico.

No se contempla uso single-ended.

---

### 4.2 Polaridad: modo bipolar

**Decisión de proyecto (cerrada):**
- El AD7175 se usa en **modo bipolar**.

Implicaciones:
- El código de salida representa valores positivos y negativos alrededor de mid-scale.
- El kernel interpreta el dato leído como **offset-binary bipolar**, convertido a entero con signo.

Conversión aplicada en el driver:

```
signed24 = raw_24bit - 0x800000
```

Esta conversión **es un contrato** entre el driver y el resto del sistema.

> ⚠️ Cambiar el modo a unipolar invalida esta conversión y rompe la semántica de signo de fuerza en todo el kernel.

---

### 4.3 Formato de datos
- Resolución: 24 bits efectivos
- Formato esperado por el kernel:
  - **offset-binary bipolar**
  - convertido a `int32_t` con signo

El driver **no escala ni filtra** el dato: entrega counts firmados.

---

## 5. Driver AD7175: responsabilidades y límites

### 5.1 Qué hace el driver
El driver AD7175 **solo** se encarga de:
- Inicializar registros del ADC
- Leer el registro DATA vía SPI
- Convertir el código 24-bit a entero con signo (`int32_t`)

### 5.2 Qué NO hace el driver (por diseño)
El driver **no**:
- Aplica filtrado digital
- Convierte a unidades físicas (N)
- Aplica tara (`RAW_ZERO`)
- Aplica polinomios de calibración
- Evalúa alarmas

> Esta separación es deliberada para:
> - permitir diagnósticos sobre raw real
> - evitar acoplar DSP y lógica de seguridad al hardware

---

## 6. Filtrado digital

El filtrado **NO reside en el driver**.

- El driver entrega `raw_unfiltered`
- El filtrado RC (si se usa) se aplica en la capa de adquisición (`main.cpp`)

Motivos:
- Necesidad de disponer de raw real para diagnósticos
- Evitar latencias ocultas en el driver
- Poder cambiar o desactivar el filtro sin tocar el driver

---

## 7. Cadena de conversión en el kernel

La cadena completa es:

```
AD7175 (raw signed counts)
   ↓
[opcional] Filtro RC (counts)
   ↓
Escalado a Newtons (SCALE_N_PER_COUNT)
   ↓
Tara (RAW_ZERO)
   ↓
Convención interna de signo (modo máquina)
   ↓
Polinomio de calibración
   ↓
Alarmas / protocolos
```

Este documento solo cubre **hasta la entrega de raw counts**.

---

## 8. Integridad de comunicación (CRC / IFMODE)

### Estado actual
- **CRC SPI:** deshabilitado
- **Motivo:** no se consiguió un funcionamiento fiable durante el desarrollo inicial

Esta decisión es **consciente y documentada**. El kernel asume actualmente que la comunicación SPI con el AD7175 es fiable en condiciones normales de operación.

### Implicaciones
- No se detectan errores de transmisión SPI a nivel de ADC
- Un fallo de bus puede manifestarse como lecturas incorrectas de fuerza

### Mitigaciones actuales
- Separación estricta entre *raw counts* y conversiones posteriores
- Posibilidad de implementar diagnósticos de plausibilidad (stuck-at, saturación, incoherencia con movimiento)

### Trabajo futuro
La activación de CRC se considera **mejora futura**, no un requisito actual. Antes de activarlo será necesario:
1) Revisar configuración exacta de IFMODE según datasheet
2) Validar temporización SPI en Portenta H7
3) Asegurar compatibilidad con lectura continua

Cualquier activación futura de CRC requiere:
- actualización de este documento
- revisión del driver AD7175
- validación en máquina real

---

## 9. Referencias
- Datasheet oficial: ver `docs/references/ad7175/AD7175.pdf`
- Código fuente: `Kernel-ProDAQ/src/AD7175.cpp`

---

## 10. Cambios y control de versiones

Cualquier modificación en:
- modo bipolar/unipolar
- formato de datos
- uso de CRC / IFMODE

requiere:
1) actualización de este documento
2) revisión del driver
3) revisión de la cadena de alarmas
