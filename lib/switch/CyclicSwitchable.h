#ifndef com_github_xhuli_arduino_lib_switch_CyclicSwitchable_H
#define com_github_xhuli_arduino_lib_switch_CyclicSwitchable_H
#pragma once

#include "Switchable.h"
#include "api/AbstractCyclicSwitchable.h"
#include "enums/SwitchState.h"

#include <Arduino.h>
// #include <ArduinoLog.h>
#include <Runnable.h>

namespace xal
{

    /**
     * @class CyclicSwitchable
     * @brief A cyclic switchable component.
     * @details A cyclic switchable component can be switched on and off in a cyclic manner.
     * The component is switched on for a specified duration and then switched off for a specified duration.
     * The durations are specified in a cycle array.
     * The cycle array is an array of unsigned 32-bit integers.
     * By default, the component is switched off.
     * This class is a switchable and has a switchable.
     *
     * This class provides functionality to switch a component on and off in a cyclic manner.
     * @implements AbstractCyclicSwitchable, AbstractSwitchable, Runnable
     */
    class CyclicSwitchable : public AbstractCyclicSwitchable,
                             public Runnable
    {
    private:
        AbstractSwitchable &switchable;

        uint8_t cycleArraySize = 0;
        uint32_t *cycleArray = nullptr;

        uint8_t currentIntervalIndex = 0;
        uint32_t lastSwitchedMs;

        /**
         * @brief Checks if the specified duration has elapsed since the last state switch.
         * @param durationMs The duration in milliseconds to check.
         * @return True if the duration has elapsed, false otherwise.
         */
        bool hasElapsed(uint32_t durationMs)
        {
            return (millis() - lastSwitchedMs >= durationMs);
        }

    public:
        /**
         * @brief Constructs a CyclicSwitchable object with the specified switchable object.
         * @param switchable The switchable component which will be wrapped with cyclic functionality.
         */
        CyclicSwitchable(AbstractSwitchable &switchable) : switchable(switchable)
        {
        }

        /**
         * @brief Destroy the Cyclic Switchable object
         */
        virtual ~CyclicSwitchable() = default;

        /**
         * @brief Sets the cycle array.
         * @param cycleArraySize The size of the cycle array.
         * @param cycleArray The cycle array.
         */
        void setCycleArray(const uint8_t cycleArraySize, uint32_t *cycleArray)
        {
            this->cycleArraySize = cycleArraySize;
            this->cycleArray = cycleArray;

            if (isOn())
            {
                currentIntervalIndex = 0;
                lastSwitchedMs = millis();
                switchable.setOn();
            }
            else
            {
                switchable.setOff();
            }
        }

        /**
         * @brief Switches the component on.
         */
        void setOn()
        {
            if (isOff())
            {
                AbstractSwitchable::setOn();
                currentIntervalIndex = 0;
                lastSwitchedMs = millis();
                switchable.setOn();
            }
        }

        /**
         * @brief Switches the component off.
         */
        void setOff()
        {
            if (isOn())
            {
                AbstractSwitchable::setOff();
                switchable.setOff();
            }
        }

        /**
         * @brief Toggles the component.
         */
        void toggle()
        {
            AbstractSwitchable::toggle();
            switchable.toggle();
        }

        void setup() override
        {
        }

        /**
         * @brief Called in the main Arduino loop function.
         * @details This function switches the component on and off in a cyclic manner.
         */
        void loop() override
        {
            if (isOn() && cycleArray != nullptr && cycleArraySize > 0)
            {
                if (hasElapsed(cycleArray[currentIntervalIndex]))
                {
                    currentIntervalIndex = (currentIntervalIndex + 1) % cycleArraySize;
                    switchable.setState(currentIntervalIndex % 2 == 0 ? SwitchState::On : SwitchState::Off);
                    lastSwitchedMs = millis();
                    // Log.noticeln("CycSw(%d) on=%d", pin, (state == SwitchState::On));
                }
            }
        }
    };

} /* namespace xal */

#endif
