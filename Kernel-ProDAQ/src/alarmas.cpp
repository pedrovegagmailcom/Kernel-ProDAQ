#include "Alarmas.h"

Alarmas::Alarmas(IO &io)
    : _io(io), _alarmas(0), _status(0) { }

void Alarmas::inicializar() {
    _alarmas = 0;
    _status  = 0;

    // En Rabbit se hacía estado_stop_on al iniciar :contentReference[oaicite:1]{index=1}
    setStop(true);
}

bool Alarmas::getInputBit(int bitIndex) const {
    if (bitIndex < 0 || bitIndex > 15) return false;
    uint16_t inputs = _io.readInputs();
    return (inputs & (1u << bitIndex)) != 0;
}

void Alarmas::setAlarmaBit(uint8_t bit, bool value) {
    if (bit > 7) return;
    if (value) {
        _alarmas |= (1u << bit);
    } else {
        _alarmas &= ~(1u << bit);
    }
}

void Alarmas::setStatusBit(uint8_t bit, bool value) {
    if (bit > 7) return;
    if (value) {
        _status |= (1u << bit);
    } else {
        _status &= ~(1u << bit);
    }
}

void Alarmas::comprobar() {
    // Leer todas las entradas físicas en un único acceso
    uint16_t inputs = _io.readInputs();

    // Emular: if (!(leer_ioad() & 8)) { _alarmas_ = 0; return; }
    // usando INPUT_ENABLE_BIT si existe
    if (INPUT_ENABLE_BIT >= 0) {
        bool enable = (inputs & (1u << INPUT_ENABLE_BIT)) != 0;
        if (!enable) {
            _alarmas = 0;
            return;
        }
    }

    // Mapear las 8 alarmas al mapa fijo de entradas físicas
    // (suponiendo que tienes definidos en Alarmas.h:
    //  INPUT_FCS_BIT, INPUT_FCI_BIT, INPUT_SETA_BIT, INPUT_MOTOR_BIT,
    //  INPUT_COMP_BIT, INPUT_TRAC_BIT, INPUT_CERO_BIT, INPUT_CELULA_BIT)

    setAlarmaBit(A_FCS,    (inputs & (1u << INPUT_FCS_BIT))    == 0);
    setAlarmaBit(A_FCI,    (inputs & (1u << INPUT_FCI_BIT))    == 0);
    setAlarmaBit(A_SETA,   (inputs & (1u << INPUT_SETA_BIT))   == 0);
    setAlarmaBit(A_MOTOR,  (inputs & (1u << INPUT_MOTOR_BIT))  == 0);

    setAlarmaBit(A_COMP,   (inputs & (1u << INPUT_COMP_BIT))   == 0);
    setAlarmaBit(A_TRAC,   (inputs & (1u << INPUT_TRAC_BIT))   == 0);
    setAlarmaBit(A_CERO,   (inputs & (1u << INPUT_CERO_BIT))   == 0);
    setAlarmaBit(A_CELULA, (inputs & (1u << INPUT_CELULA_BIT)) == 0);
}

// ======= Gestión de estado de máquina =======

void Alarmas::setStop(bool on) {
    setStatusBit(S_STOP, on);
}

bool Alarmas::isStop() const {
    return (_status & (1u << S_STOP)) != 0;
}

void Alarmas::setUpDown(bool on) {
    setStatusBit(S_UPDOWN, on);
}

bool Alarmas::isUpDown() const {
    return (_status & (1u << S_UPDOWN)) != 0;
}

void Alarmas::setRemoto(bool on) {
    setStatusBit(S_REMOTO, on);
}

bool Alarmas::isRemoto() const {
    return (_status & (1u << S_REMOTO)) != 0;
}
