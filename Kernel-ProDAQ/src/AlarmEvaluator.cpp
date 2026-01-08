#include "AlarmEvaluator.h"

#include <math.h>

namespace {
constexpr uint32_t N_ON_MS = 20;
constexpr uint32_t N_OFF_MS = 40;

constexpr float HYST_PCT = 0.02f;
constexpr float HYST_MIN = 1.0f;
}  // namespace

AlarmEvaluator::AlarmEvaluator() = default;

void AlarmEvaluator::resetCounters(OverloadState &state) {
    state.onMs = 0;
    state.offMs = 0;
}

void AlarmEvaluator::resetOverloadState() {
    _trac.active = false;
    _comp.active = false;
    resetCounters(_trac);
    resetCounters(_comp);
}

void AlarmEvaluator::reset() {
    _lastUpdateMs = 0;
    resetOverloadState();
}

bool AlarmEvaluator::isValidCellConfig(const CellConfig &config) {
    if (config.capacidad == 0) {
        return false;
    }

    if (config.limite == 0) {
        return false;
    }

    if (config.limite > static_cast<uint32_t>(config.capacidad) * 2U) {
        return false;
    }

    if (config.overload_t == 0 || config.overload_c == 0) {
        return false;
    }

    if (!(config.resolucion > 0.0f)) {
        return false;
    }

    bool hasSerial = false;
    for (char c : config.numeroserie) {
        if (c != '\0' && c != ' ') {
            hasSerial = true;
            break;
        }
    }

    return hasSerial;
}

AlarmEvaluator::Result AlarmEvaluator::update(const DatosSensor &sensor, const CellConfig &config) {
    Result result{false, false, false};

    const uint32_t nowMs = millis();
    uint32_t deltaMs = 0;
    if (_lastUpdateMs != 0) {
        deltaMs = nowMs - _lastUpdateMs;
    }
    _lastUpdateMs = nowMs;
    if (deltaMs == 0) {
        deltaMs = 1;
    }

    if (!isValidCellConfig(config)) {
        result.cellConfigFault = true;
        resetOverloadState();
        return result;
    }

    const float overloadT = static_cast<float>(config.overload_t);
    const float overloadC = static_cast<float>(config.overload_c);
    const float hTrac = fmaxf(HYST_MIN, overloadT * HYST_PCT);
    const float hComp = fmaxf(HYST_MIN, overloadC * HYST_PCT);

    const float fuerza = sensor.fuerza;
    const float tracOffThreshold = overloadT - hTrac;
    const float compOffThreshold = -(overloadC - hComp);

    if (_trac.active) {
        resetCounters(_comp);
        if (fuerza <= tracOffThreshold) {
            _trac.offMs += deltaMs;
            if (_trac.offMs >= N_OFF_MS) {
                _trac.active = false;
                resetCounters(_trac);
            }
        } else {
            _trac.offMs = 0;
        }
    } else if (_comp.active) {
        resetCounters(_trac);
        if (fuerza >= compOffThreshold) {
            _comp.offMs += deltaMs;
            if (_comp.offMs >= N_OFF_MS) {
                _comp.active = false;
                resetCounters(_comp);
            }
        } else {
            _comp.offMs = 0;
        }
    } else {
        if (fuerza >= overloadT) {
            _trac.onMs += deltaMs;
            if (_trac.onMs >= N_ON_MS) {
                _trac.active = true;
                resetCounters(_trac);
            }
        } else {
            _trac.onMs = 0;
        }

        if (!_trac.active) {
            if (fuerza <= -overloadC) {
                _comp.onMs += deltaMs;
                if (_comp.onMs >= N_ON_MS) {
                    _comp.active = true;
                    resetCounters(_comp);
                }
            } else {
                _comp.onMs = 0;
            }
        } else {
            _comp.onMs = 0;
        }
    }

    result.trac = _trac.active;
    result.comp = _comp.active;
    return result;
}
