#ifndef com_github_xhuli_arduino_lib_switch_api_AbstractCyclicSwitchable_H
#define com_github_xhuli_arduino_lib_switch_api_AbstractCyclicSwitchable_H
#pragma once

#include "AbstractSwitchable.h"
#include "enums/SwitchState.h"

#include <Arduino.h>

namespace xal {

class AbstractCyclicSwitchable : virtual public AbstractSwitchable {
private:
    uint8_t cycleArraySize;
    uint32_t* cycleArray;

public:
    ~AbstractCyclicSwitchable() {
        delete[] cycleArray;
    };

    virtual void setCycleArray(uint8_t cycleArraySize, uint32_t* cycleArray) = 0;
};

} /* namespace xal */

#endif
