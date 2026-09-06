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
        const uint32_t *cycleArray;

    public:
        /* No custom destructor: cycleArray is always a borrowed pointer into
         * someone else's existing array (e.g. AtoActions's pattern arrays) --
         * this class never allocates it, so there is nothing here to free.
         * (A previous version called `delete[] cycleArray;` here, which was
         * undefined behavior: deleting a pointer that was never allocated with
         * `new[]`. It was dormant in practice since production CyclicSwitchable
         * instances are globals that are never destructed, but latent UB
         * nonetheless.) */
        ~AbstractCyclicSwitchable() override = default;

        virtual void setCycleArray(uint8_t cycleArraySize, const uint32_t *cycleArray) = 0;
    };

} /* namespace xal */

#endif
