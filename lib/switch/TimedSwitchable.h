#ifndef com_github_xhuli_arduino_lib_switch_TimedSwitchable_H
#define com_github_xhuli_arduino_lib_switch_TimedSwitchable_H
#pragma once

#include "Switchable.h"
#include "api/AbstractSwitchable.h"
#include "enums/SwitchState.h"

#include <Arduino.h>
// #include <ArduinoLog.h>
#include <Runnable.h>

namespace xal
{

    /**
     * @class TimedSwitchable
     * @brief A timed switchable component that can be turned on and off based on specified durations.
     * @details This class is a switchable and has a switchable.
     * @implements AbstractSwitchable, Runnable
     */
    class TimedSwitchable : public AbstractSwitchable,
                            public Runnable
    {
        typedef void (*Callback)();

    private:
        AbstractSwitchable &switchable; /**< A switchable component */

        uint32_t maxOnTimeMs = 0;    /**< The maximum duration in milliseconds that the component can stay on. */
        uint32_t maxOffTimeMs = 0;   /**< The maximum duration in milliseconds that the component can stay off. */
        uint32_t lastSwitchedMs = 0; /**< The timestamp of the last state switch. */

        Callback onTimeElapsedCallback;  /* callback function to be called when on duration is reached */
        Callback offTimeElapsedCallback; /* callback function to be called when off duration is reached */

        /**
         * @brief Checks if the specified duration has elapsed since the last state switch.
         * @param durationMs The duration in milliseconds to check.
         * @return True if the duration has elapsed, false otherwise.
         */
        bool hasElapsed(uint32_t durationMs)
        {
            return ((durationMs > 0) && (millis() - lastSwitchedMs >= durationMs));
        }

        /**
         * @brief Gets the maximum duration for the current state.
         * @return The maximum duration in milliseconds.
         */
        uint32_t currentStateMaxDurationMs()
        {
            return this->isOn() ? maxOnTimeMs : maxOffTimeMs;
        }

    public:
        /**
         * @brief Constructs a TimedSwitchable object with the specified pin number and initial state.
         * @param switchable The switchable component which will be wrapped with timed functionality.
         */
        TimedSwitchable(AbstractSwitchable &switchable) : switchable(switchable)
        {
        }

        /**
         * @brief Default destructor.
         */
        virtual ~TimedSwitchable() = default;

        /**
         * @brief Sets the maximum duration that the component can stay on.
         * @param maxOnTimeMs The maximum duration in milliseconds.
         */
        virtual void setMaxOnTimeMs(uint32_t maxOnTimeMs)
        {
            this->maxOnTimeMs = maxOnTimeMs;
        }

        /**
         * @brief Sets the maximum duration that the component can stay off.
         * @param maxOffTimeMs The maximum duration in milliseconds.
         */
        virtual void setMaxOffTimeMs(uint32_t maxOffTimeMs)
        {
            this->maxOffTimeMs = maxOffTimeMs;
        }

        /**
         * @brief Sets the callback function to be called when the on duration is reached.
         * @param callback The callback function to be called when the on duration is reached.
         */
        virtual void setOnTimeElapsedCallback(Callback callback)
        {
            // Log.noticeln("TmdSwtchbl(%d).OnTmElpsd: state=%d (On=%d)", pin, state, SwitchState::On);
            onTimeElapsedCallback = callback;
        }

        /**
         * @brief Sets the callback function to be called when the off duration is reached.
         * @param callback The callback function to be called when the off duration is reached.
         */
        virtual void setOffTimeElapsedCallback(Callback callback)
        {
            // Log.noticeln("TmdSwtchbl(%d).OffTmElpsd: state=%d (On=%d)", pin, state, SwitchState::On);
            offTimeElapsedCallback = callback;
        }

        virtual void setOn()
        {
            lastSwitchedMs = millis();
            AbstractSwitchable::setOn();
            switchable.setOn();
        }

        virtual void setOff()
        {
            lastSwitchedMs = millis();
            AbstractSwitchable::setOff();
            switchable.setOff();
        }

        virtual void toggle()
        {
            lastSwitchedMs = millis();
            AbstractSwitchable::toggle();
            switchable.toggle();
        }

        virtual void setup() override
        {
        }

        /**
         * @brief Executes the component's main logic.
         * @details This function is called repeatedly in the main loop.
         */
        virtual void loop() override
        {
            if (hasElapsed(currentStateMaxDurationMs()))
            {
                if (isOn())
                {
                    setOff();
                    if (onTimeElapsedCallback != nullptr)
                    {
                        onTimeElapsedCallback();
                    }
                }
                else
                {
                    setOn();
                    if (offTimeElapsedCallback != nullptr)
                    {
                        offTimeElapsedCallback();
                    }
                }
            }
        }
    };

} /* namespace xal */

#endif
