#ifndef com_github_xhuli_arduino_lib_switch_TimedSwitchable_H
#define com_github_xhuli_arduino_lib_switch_TimedSwitchable_H
#pragma once

#include "Switchable.h"
#include "api/AbstractSwitchable.h"
#include "enums/SwitchState.h"

#include <Arduino.h>
#include <Runnable.h>

namespace xal {

    /**
     * @class TimedSwitchable
     * @brief A timed switchable component that can be turned on and off based on specified durations.
     * @details This class is a switchable and has a switchable.
     * @implements AbstractSwitchable, Runnable
     */
    class TimedSwitchable : public AbstractSwitchable,
                            public Runnable {
        using Callback = void (*)();

    private:
        AbstractSwitchable &switchable; /**< A switchable component */

        uint32_t maxOnTimeMs = 0;    /**< The maximum duration in milliseconds that the component can stay on. */
        uint32_t maxOffTimeMs = 0;   /**< The maximum duration in milliseconds that the component can stay off. */
        uint32_t lastSwitchedMs = 0; /**< The timestamp of the last state switch. */

        Callback onTimeElapsedCallback = nullptr;  /* callback function to be called when on duration is reached */
        Callback offTimeElapsedCallback = nullptr; /* callback function to be called when off duration is reached */

        /**
         * @brief Checks if the specified duration has elapsed since the last
         * state switch, relative to the given current time.
         * @param nowMs The current time in milliseconds.
         * @param durationMs The duration in milliseconds to check.
         * @return True if the duration has elapsed, false otherwise.
         */
        bool hasElapsed(uint32_t nowMs, uint32_t durationMs) const {
            return ((durationMs > 0) && (nowMs - lastSwitchedMs >= durationMs));
        }

        /**
         * @brief Gets the maximum duration for the current state.
         * @return The maximum duration in milliseconds.
         */
        uint32_t currentStateMaxDurationMs() {
            return this->isOn() ? maxOnTimeMs : maxOffTimeMs;
        }

    public:
        /**
         * @brief Constructs a TimedSwitchable object with the specified pin number and initial state.
         * @param switchable The switchable component which will be wrapped with timed functionality.
         */
        explicit TimedSwitchable(AbstractSwitchable &switchable) : switchable(switchable) {
        }

        /**
         * @brief Default destructor.
         */
        ~TimedSwitchable() override = default;

        /**
         * @brief Sets the maximum duration that the component can stay on.
         * @param maxOnTimeMs The maximum duration in milliseconds.
         */
        virtual void setMaxOnTimeMs(uint32_t maxOnTimeMs) {
            this->maxOnTimeMs = maxOnTimeMs;
        }

        /**
         * @brief Sets the maximum duration that the component can stay off.
         * @param maxOffTimeMs The maximum duration in milliseconds.
         */
        virtual void setMaxOffTimeMs(uint32_t maxOffTimeMs) {
            this->maxOffTimeMs = maxOffTimeMs;
        }

        /**
         * @brief Sets the callback function to be called when the on duration is reached.
         * @param callback The callback function to be called when the on duration is reached.
         */
        virtual void setOnTimeElapsedCallback(Callback callback) {
            onTimeElapsedCallback = callback;
        }

        /**
         * @brief Sets the callback function to be called when the off duration is reached.
         * @param callback The callback function to be called when the off duration is reached.
         */
        virtual void setOffTimeElapsedCallback(Callback callback) {
            offTimeElapsedCallback = callback;
        }

        void setOn() override {
            setOn(millis());
        }

        /**
         * @brief Same as setOn(), but records the switch timestamp as the
         * given value instead of reading millis() internally. Exists so
         * timing logic can be tested deterministically with fabricated
         * timestamps.
         * @param nowMs The current time in milliseconds.
         */
        void setOn(uint32_t nowMs) {
            lastSwitchedMs = nowMs;
            AbstractSwitchable::setOn();
            switchable.setOn();
        }

        void setOff() override {
            setOff(millis());
        }

        /**
         * @brief Same as setOff(), but records the switch timestamp as the
         * given value instead of reading millis() internally.
         * @param nowMs The current time in milliseconds.
         */
        void setOff(uint32_t nowMs) {
            lastSwitchedMs = nowMs;
            AbstractSwitchable::setOff();
            switchable.setOff();
        }

        void toggle() override {
            toggle(millis());
        }

        /**
         * @brief Same as toggle(), but records the switch timestamp as the
         * given value instead of reading millis() internally.
         * @param nowMs The current time in milliseconds.
         */
        void toggle(uint32_t nowMs) {
            lastSwitchedMs = nowMs;
            AbstractSwitchable::toggle();
            switchable.toggle();
        }

        void setup() override {
        }

        /**
         * @brief Checks whether the current state's max duration has
         * elapsed as of nowMs and, if so, flips state and fires the
         * matching callback. Extracted from loop() so it can be driven
         * directly with a fabricated timestamp in tests.
         * @param nowMs The current time in milliseconds (normally millis()).
         */
        void process(uint32_t nowMs) {
            if (hasElapsed(nowMs, currentStateMaxDurationMs())) {
                if (isOn()) {
                    setOff(nowMs);
                    if (onTimeElapsedCallback != nullptr) {
                        onTimeElapsedCallback();
                    }
                } else {
                    setOn(nowMs);
                    if (offTimeElapsedCallback != nullptr) {
                        offTimeElapsedCallback();
                    }
                }
            }
        }

        /**
         * @brief Executes the component's main logic.
         * @details This function is called repeatedly in the main loop.
         */
        void loop() override {
            process(millis());
        }
    };

} /* namespace xal */

#endif
