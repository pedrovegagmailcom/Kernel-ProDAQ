#ifndef ALARMAS_H
#define ALARMAS_H

#include <Arduino.h>
#include "IO.h"

class Alarmas {
public:
    // Bits del BYTE de alarmas (formato compatible Rabbit)
    enum AlarmaBit : uint8_t {
        A_MOTOR  = 0,
        A_COMP   = 1,
        A_FCI    = 2,
        A_TRAC   = 3,
        A_FCS    = 4,
        A_SETA   = 5,
        A_CERO   = 6,
        A_CELULA = 7
    };

    // Bits del BYTE de estado
    enum StatusBit : uint8_t {
        S_UPDOWN = 0,
        S_STOP   = 1,
        S_REMOTO = 2
    };

    explicit Alarmas(IO &io);

    void inicializar();   // equivalente a inicializar_alarmas()
    void comprobar();     // equivalente a comprobar_alarmas()

    bool    hayAlarmas() const { return _alarmas != 0; }
    uint8_t getAlarmas() const { return _alarmas; }  // get_alarmas()
    uint8_t getStatus()  const { return _status; }   // get_status()

    // Gestión de estado de máquina (por si lo necesitas desde fuera)
    void setStop(bool on);
    bool isStop() const;

    void setUpDown(bool on);
    bool isUpDown() const;

    void setRemoto(bool on);
    bool isRemoto() const;

private:
    IO &_io;
    uint8_t _alarmas;
    uint8_t _status;

    // ====== AQUÍ FIJAS PARA SIEMPRE EL MAPEO A LAS ENTRADAS FÍSICAS ======
    // Pon aquí el número de bit de readInputs() que corresponde a cada señal.
    // Ejemplo (igual que Rabbit): FCS->E0, FCI->E1, SETA->E2, MOTOR->E3.
    static constexpr int INPUT_FCS_BIT    = 0;
    static constexpr int INPUT_FCI_BIT    = 1;
    static constexpr int INPUT_SETA_BIT   = 2;
    static constexpr int INPUT_MOTOR_BIT  = 3;
    static constexpr int INPUT_COMP_BIT   = 4;
    static constexpr int INPUT_TRAC_BIT   = 5;
    static constexpr int INPUT_CERO_BIT   = 6;
    static constexpr int INPUT_CELULA_BIT = 7;

    // Si tienes una señal de "enable alarmas" tipo (leer_ioad() & 8),
    // pon aquí su bit. Si no existe, déjalo en -1 y se ignorará.
    static constexpr int INPUT_ENABLE_BIT = -1;

    // =====================================================================

    bool getInputBit(int bitIndex) const;

    void setAlarmaBit(uint8_t bit, bool value);
    void setStatusBit(uint8_t bit, bool value);
};

#endif // ALARMAS_H
