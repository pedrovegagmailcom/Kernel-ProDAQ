#ifndef ALARM_EVALUATOR_H
#define ALARM_EVALUATOR_H

#include <Arduino.h>
#include "modelo.h"

class AlarmEvaluator {
public:
    struct Result {
        bool trac;
        bool comp;
        bool cellConfigFault;
    };

    AlarmEvaluator();

    Result update(const DatosSensor &sensor, const CellConfig &config);
    static bool isValidCellConfig(const CellConfig &config);
    void reset();

private:
    struct OverloadState {
        bool active = false;
        uint32_t onMs = 0;
        uint32_t offMs = 0;
    };

    OverloadState _trac;
    OverloadState _comp;
    uint32_t _lastUpdateMs = 0;

    void resetOverloadState();
    void resetCounters(OverloadState &state);
};

#endif // ALARM_EVALUATOR_H
