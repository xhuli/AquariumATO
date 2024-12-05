#ifndef com_github_xhuli_arduino_lib_timers_Timer_H
#define com_github_xhuli_arduino_lib_timers_Timer_H
#pragma once

#include <Arduino.h>
// #include <ArduinoLog.h>
#include <Runnable.h>
#include <api/AbstractSwitchable.h>

namespace xal {

/**
 * @class Timer
 * @brief The Timer class is a base class for timers.
 *        It provides functionality for setting the duration, auto-restart,
 *        and callback function of a timer.
 *        It also implements the setup and loop methods from the Runnable interface.
 * @implements Runnable, AbstractSwitchable
 */
class Timer : virtual public AbstractSwitchable,
              virtual public Runnable {
    typedef void (*Callback)(); /**< Type definition for the timer callback function. */

private:
    uint32_t durationMs = 0; /**< The duration of the timer in milliseconds. */
    uint32_t startMs = 0; /**< The start time of the timer in milliseconds. */
    bool doAutoRestart = false; /**< Flag indicating whether the timer should automatically restart after it elapses. */

    Callback callback = nullptr; /**< Pointer to the callback function to be called when the timer elapses. */

    /**
     * @brief Checks if the timer has elapsed based on the given duration.
     * @param durationMs The duration to check against.
     * @return True if the timer has elapsed, false otherwise.
     */
    bool hasElapsed(uint32_t durationMs)
    {
        return (millis() - startMs >= durationMs);
    }

public:
    Timer() = default;

    virtual ~Timer() = default;

    virtual void setDurationMs(uint32_t durationMs)
    {
        this->durationMs = durationMs;
    }

    virtual void setAutoRestart(bool doAutoRestart)
    {
        this->doAutoRestart = doAutoRestart;
    }

    virtual void setCallback(Callback callback)
    {
        this->callback = callback;
    }

    virtual void setOn() override
    {
        AbstractSwitchable::setOn();
        startMs = millis();
    }

    virtual void setOff() override
    {
        AbstractSwitchable::setOff();
        startMs = 0;
    }

    virtual void setup() override
    {
    }

    virtual void loop() override
    {
        if (isOn()) {
            if (hasElapsed(durationMs)) {
                if (callback != nullptr) {
                    callback();
                }

                if (doAutoRestart) {
                    startMs = millis();
                } else {
                    setOff();
                }

                // Log.noticeln("Timer on=%d", isOn());
            }
        }
    }
};

} /* namespace xal */

#endif
