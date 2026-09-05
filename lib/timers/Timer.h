#ifndef com_github_xhuli_arduino_lib_timers_Timer_H
#define com_github_xhuli_arduino_lib_timers_Timer_H
#pragma once

#include <Arduino.h>
#include <Runnable.h>
#include <api/AbstractSwitchable.h>

namespace xal
{

    /**
     * @class Timer
     * @brief The Timer class is a base class for timers.
     *        It provides functionality for setting the duration, auto-restart,
     *        and callback function of a timer.
     *        It also implements the setup and loop methods from the Runnable interface.
     * @implements Runnable, AbstractSwitchable
     */
    class Timer : virtual public AbstractSwitchable,
                  virtual public Runnable
    {
        typedef void (*Callback)(); /**< Type definition for the timer callback function. */

    private:
        uint32_t durationMs = 0;    /**< The duration of the timer in milliseconds. */
        uint32_t startMs = 0;       /**< The start time of the timer in milliseconds. */
        bool doAutoRestart = false; /**< Flag indicating whether the timer should automatically restart after it elapses. */

        Callback callback = nullptr; /**< Pointer to the callback function to be called when the timer elapses. */

        /**
         * @brief Checks if the timer has elapsed based on the given duration,
         * relative to the given current time.
         * @param nowMs The current time in milliseconds.
         * @param durationMs The duration to check against.
         * @return True if the timer has elapsed, false otherwise.
         */
        bool hasElapsed(uint32_t nowMs, uint32_t durationMs) const
        {
            return (nowMs - startMs >= durationMs);
        }

    public:
        Timer() = default;

        ~Timer() override = default;

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

        void setOn() override
        {
            setOn(millis());
        }

        /**
         * @brief Same as setOn(), but starts the elapsed-time clock at the
         * given timestamp instead of reading millis() internally. Exists so
         * timer logic can be started and driven deterministically with
         * fabricated timestamps in tests, with no hardware dependency.
         * @param nowMs The current time in milliseconds.
         */
        void setOn(uint32_t nowMs)
        {
            AbstractSwitchable::setOn();
            startMs = nowMs;
        }

        void setOff() override
        {
            AbstractSwitchable::setOff();
            startMs = 0;
        }

        void setup() override
        {
        }

        /**
         * @brief Checks whether the timer has elapsed as of nowMs and, if so,
         * fires the callback and either restarts (doAutoRestart) or turns
         * off. Extracted from loop() so it can be driven directly with a
         * fabricated timestamp in tests.
         * @param nowMs The current time in milliseconds (normally millis()).
         */
        void process(uint32_t nowMs)
        {
            if (isOn())
            {
                if (hasElapsed(nowMs, durationMs))
                {
                    if (callback != nullptr)
                    {
                        callback();
                    }

                    if (doAutoRestart)
                    {
                        startMs = nowMs;
                    }
                    else
                    {
                        setOff();
                    }
                }
            }
        }

        void loop() override
        {
            process(millis());
        }
    };

} /* namespace xal */

#endif
