#ifndef com_github_xhuli_arduino_lib_switch_CyclicSwitchable_H
#define com_github_xhuli_arduino_lib_switch_CyclicSwitchable_H
#pragma once

#include "Switchable.h"
#include "api/AbstractCyclicSwitchable.h"
#include "enums/SwitchState.h"

#include <Arduino.h>
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
        const uint32_t *cycleArray = nullptr;

        uint8_t currentIntervalIndex = 0;
        uint32_t lastSwitchedMs = 0; /**< Was previously uninitialized: harmless for
                                          global instances (auto-zero-initialized by
                                          C++ for static storage duration) but a real
                                          uninitialized-read risk for any stack-allocated
                                          instance, e.g. in tests. */

        /**
         * @brief Checks if the specified duration has elapsed since the last
         * state switch, relative to the given current time.
         * @param nowMs The current time in milliseconds.
         * @param durationMs The duration in milliseconds to check.
         * @return True if the duration has elapsed, false otherwise.
         */
        bool hasElapsed(uint32_t nowMs, uint32_t durationMs)
        {
            return (nowMs - lastSwitchedMs >= durationMs);
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
        void setCycleArray(const uint8_t cycleArraySize, const uint32_t *cycleArray)
        {
            setCycleArray(cycleArraySize, cycleArray, millis());
        }

        /**
         * @brief Same as setCycleArray(), but records the switch timestamp
         * as the given value instead of reading millis() internally.
         * @param nowMs The current time in milliseconds.
         */
        void setCycleArray(const uint8_t cycleArraySize, const uint32_t *cycleArray, uint32_t nowMs)
        {
            this->cycleArraySize = cycleArraySize;
            this->cycleArray = cycleArray;

            if (isOn())
            {
                currentIntervalIndex = 0;
                lastSwitchedMs = nowMs;
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
            setOn(millis());
        }

        /**
         * @brief Same as setOn(), but records the switch timestamp as the
         * given value instead of reading millis() internally. Exists so
         * cycling logic can be started and driven deterministically with
         * fabricated timestamps in tests.
         * @param nowMs The current time in milliseconds.
         */
        void setOn(uint32_t nowMs)
        {
            if (isOff())
            {
                AbstractSwitchable::setOn();
                currentIntervalIndex = 0;
                lastSwitchedMs = nowMs;
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
         * @brief Checks whether the current cycle interval has elapsed as
         * of nowMs and, if so, advances to the next interval and toggles
         * the wrapped switchable accordingly. Extracted from loop() so it
         * can be driven directly with a fabricated timestamp in tests.
         * @param nowMs The current time in milliseconds (normally millis()).
         */
        void process(uint32_t nowMs)
        {
            if (isOn() && cycleArray != nullptr && cycleArraySize > 0)
            {
                if (hasElapsed(nowMs, cycleArray[currentIntervalIndex]))
                {
                    currentIntervalIndex = (currentIntervalIndex + 1) % cycleArraySize;
                    switchable.setState(currentIntervalIndex % 2 == 0 ? SwitchState::On : SwitchState::Off);
                    lastSwitchedMs = nowMs;
                }
            }
        }

        /**
         * @brief Called in the main Arduino loop function.
         * @details This function switches the component on and off in a cyclic manner.
         */
        void loop() override
        {
            process(millis());
        }
    };

} /* namespace xal */

#endif
