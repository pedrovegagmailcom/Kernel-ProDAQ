#include "Alarmas.h"

Alarmas::Alarmas(IO &io)
    : _io(io), _alarmas_hw(0), _alarmas_sw(0), _status(0) { }

void Alarmas::inicializar() {
    _alarmas_hw = 0;
    _alarmas_sw = 0;
    _status  = 0;

    // En Rabbit se hacía estado_stop_on al iniciar :contentReference[oaicite:1]{index=1}
    setStop(true);
}

bool Alarmas::getInputBit(int bitIndex) const {
    if (bitIndex < 0 || bitIndex > 15) return false;
    uint16_t inputs = _io.readInputs();
    return (inputs & (1u << bitIndex)) != 0;
}

void Alarmas::setAlarmaBit(uint8_t &dest, uint8_t bit, bool value) {
    if (bit > 7) return;
    if (value) {
        dest |= (1u << bit);
    } else {
        dest &= ~(1u << bit);
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
    comprobarHW();
}

void Alarmas::comprobarHW() {
    // Leer todas las entradas físicas en un único acceso
    uint16_t inputs = _io.readInputs();

    // Emular: if (!(leer_ioad() & 8)) { _alarmas_ = 0; return; }
    // usando INPUT_ENABLE_BIT si existe
    if (INPUT_ENABLE_BIT >= 0) {
        bool enable = (inputs & (1u << INPUT_ENABLE_BIT)) != 0;
        if (!enable) {
            _alarmas_hw = 0;
            return;
        }
    }

    _alarmas_hw = 0;

    // Mapear solo las alarmas cableadas
    // (suponiendo que tienes definidos en Alarmas.h:
    //  INPUT_FCS_BIT, INPUT_FCI_BIT, INPUT_SETA_BIT, INPUT_MOTOR_BIT)
    // A_TRAC/A_COMP/A_CELULA/A_CERO se reservan a alarmas software.

    setAlarmaBit(_alarmas_hw, A_FCS,   (inputs & (1u << INPUT_FCS_BIT))   != 0);
    setAlarmaBit(_alarmas_hw, A_FCI,   (inputs & (1u << INPUT_FCI_BIT))   != 0);
    setAlarmaBit(_alarmas_hw, A_SETA,  (inputs & (1u << INPUT_SETA_BIT))  != 0);
    setAlarmaBit(_alarmas_hw, A_MOTOR, (inputs & (1u << INPUT_MOTOR_BIT)) != 0);
}

void Alarmas::setSwAlarm(AlarmaBit bit, bool on) {
    setAlarmaBit(_alarmas_sw, static_cast<uint8_t>(bit), on);
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
