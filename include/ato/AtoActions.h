#ifndef com_github_xhuli_arduino_ato_AtoActions_H
#define com_github_xhuli_arduino_ato_AtoActions_H
#pragma once

#include "api/AbstractCyclicSwitchable.h"
#include "api/AbstractSwitchable.h"
#include <Timer.h>

namespace xal
{
    namespace ato
    {

        /**
         * @brief Recursive constexpr sum, valid under strict C++11 constexpr
         * rules (single return statement, no loops/mutable locals) so this
         * compiles regardless of whether the toolchain defaults to
         * -std=gnu++11 or a more permissive later standard.
         */
        template <size_t N>
        constexpr uint32_t sumPatternFrom(const uint32_t (&pattern)[N], size_t startIndex)
        {
            return (startIndex >= N) ? 0 : (pattern[startIndex] + sumPatternFrom(pattern, startIndex + 1));
        }

        template <size_t N>
        constexpr uint32_t sumPattern(const uint32_t (&pattern)[N])
        {
            return sumPatternFrom(pattern, 0);
        }

        /* File-scope (not class-member) so all AtoActions instances share one
         * copy, and so static_assert can check each pattern's total at compile
         * time below. Moving these out of the class doesn't change any call
         * site — onEntryXState() methods reference them by name exactly as
         * before, via ordinary namespace lookup. */

        static constexpr uint32_t led_always_on[1] = {1000ul};
        static constexpr uint32_t led_blink_pattern_slow[2] = {1520, 380};
        static constexpr uint32_t led_blink_pattern_fast[2] = {380, 380};

        static constexpr uint32_t buzzer_reservoir_pattern[12] = {
            1400, 400, 1400, 400, 1400, /* --- --- --- (5s) */
            25000,                      /* idle until 30 seconds elapse from the pattern start */
            1400, 400, 1400, 400, 1400, /* --- --- --- (5s) */
            565000                      /* idle until 10 minutes elapse from the pattern start */
        };
        static_assert(sumPattern(buzzer_reservoir_pattern) == 600000UL,
                      "buzzer_reservoir_pattern must total exactly 10 minutes (600000ms)");

        static constexpr uint32_t buzzer_error_pattern[12] = {
            700, 400, 700, 400, 1400, /* - - --- (3.6s) */
            26400,                    /* idle until 30 seconds elapse from the pattern start */
            700, 400, 700, 400, 1400, /* - - --- (3.6s) */
            566400                    /* idle until 10 minutes elapse from the pattern start */
        };
        static_assert(sumPattern(buzzer_error_pattern) == 600000UL,
                      "buzzer_error_pattern must total exactly 10 minutes (600000ms)");

        static constexpr uint32_t buzzer_water_low_pattern[12] = {
            1400, 400, 700, 400, 700, /** --- - - (3.6s) */
            26400,                    /* idle until 30 seconds elapse from the pattern start */
            1400, 400, 700, 400, 700, /** --- - - (3.6s) */
            566400                    /* idle until 10 minutes elapse from the pattern start */

        };
        static_assert(sumPattern(buzzer_water_low_pattern) == 600000UL,
                      "buzzer_water_low_pattern must total exactly 10 minutes (600000ms)");

        static constexpr uint32_t buzzer_water_high_pattern[20] = {
            1400, 400, 1400, 400, 1400, 400, 1400, 400, 1400, /* --- --- --- --- --- (8.6s) */
            21400,                                            /* idle until 30 seconds elapse from the pattern start */
            1400, 400, 1400, 400, 1400, 400, 1400, 400, 1400, /* --- --- --- --- --- (8.6s) */
            561400                                            /* idle until 10 minutes elapse from the pattern start */
        };
        static_assert(sumPattern(buzzer_water_high_pattern) == 600000UL,
                      "buzzer_water_high_pattern must total exactly 10 minutes (600000ms)");

        static constexpr uint32_t buzzer_idle_for_too_long_pattern[24] = {
            400,
            1400,
            400,
            1400,
            400,
            1400,
            400,
            1400,
            400,
            1400,
            1800,  /* - - - - - --- (10.8s) */
            19200, /* idle until 30 seconds elapse from the pattern start */
            400,
            1400,
            400,
            1400,
            400,
            1400,
            400,
            1400,
            400,
            1400,
            1800,   /* - - - - - --- (10.8s) */
            559200, /* idle until 10 minutes elapse from the pattern start */
        };
        static_assert(sumPattern(buzzer_idle_for_too_long_pattern) == 600000UL,
                      "buzzer_idle_for_too_long_pattern must total exactly 10 minutes (600000ms)");

        /**
         * @class AtoActions
         * @brief This class encapsulates the actions that can be performed by the Automatic Top-Off (ATO) system.
         *
         * The AtoActions class is responsible for managing the state and behavior of various components in the ATO system,
         * including LEDs, buzzers, and timers. It defines various patterns for LED blinking and buzzer sounds to indicate
         * different states and events such as normal operation, errors, and water level alerts.
         *
         * The class provides methods to set and control these components, allowing the ATO system to respond to sensor
         * inputs and other events by activating the appropriate actions.
         *
         * Patterns:
         * - LED always on
         * - LED blinking (slow and fast)
         * - Buzzer patterns for reservoir alerts, errors, low water, high water, and idle for too long
         *
         * Components:
         * - LEDs (Red, Yellow, Green)
         * - Buzzer
         * - Water Dispenser (Pump)
         * - Timers (Sleep and Idle)
         *
         * Usage:
         * - Configure the AtoActions instance with the appropriate components.
         * - Use the provided methods to control the components based on the system's state and events.
         */
        class AtoActions
        {
        private:
            AbstractCyclicSwitchable *redLed = nullptr;
            AbstractCyclicSwitchable *yellowLed = nullptr;
            AbstractCyclicSwitchable *greenLed = nullptr;
            AbstractCyclicSwitchable *buzzer = nullptr;
            AbstractSwitchable *waterDispenser = nullptr;
            Timer *sleepTimer = nullptr;
            Timer *idleTimer = nullptr;

        public:
            AtoActions() = default;

            ~AtoActions() = default;

            /* State actions */

            void onExitState()
            {
                setOff(redLed);
                setOff(yellowLed);
                setOff(greenLed);
                setOff(buzzer);
                setOff(waterDispenser);
                setOff(sleepTimer);
                setOff(idleTimer);
            }

            void onEntryIdleState()
            {
                setOn(greenLed, 2, led_blink_pattern_slow);
                setOn(idleTimer);
            }

            void onEntryIdleForTooLongState()
            {
                setOn(greenLed, 2, led_blink_pattern_slow);
                setOn(redLed, 2, led_blink_pattern_slow);
                setOn(buzzer, 24, buzzer_idle_for_too_long_pattern);
            }

            void onEntryDispensingInAutoModeState()
            {
                setOn(greenLed, 1, led_always_on);
                setOn(waterDispenser);
            }

            void onEntryDispensingInManualModeState()
            {
                setOn(greenLed, 2, led_blink_pattern_fast);
                setOn(waterDispenser);
            }

            void onEntryWaterLowState()
            {
                setOn(redLed, 2, led_blink_pattern_fast);
                setOn(buzzer, 12, buzzer_water_low_pattern);
            }

            void onEntryWaterHighState()
            {
                setOn(redLed, 2, led_blink_pattern_fast);
                setOn(buzzer, 20, buzzer_water_high_pattern);
            }

            void onEntryReservoirEmptyState()
            {
                setOn(redLed, 2, led_blink_pattern_slow);
                setOn(buzzer, 12, buzzer_reservoir_pattern);
            }

            void onEntrySleepingState()
            {
                setOn(yellowLed, 2, led_blink_pattern_slow);
                setOn(sleepTimer);
            }

            void onEntryErrorState()
            {
                setOn(redLed, 2, led_blink_pattern_fast);
                setOn(buzzer, 12, buzzer_error_pattern);
            }

            /* Setters */

            void setRedLed(AbstractCyclicSwitchable *redLedPtr) { AtoActions::redLed = redLedPtr; }
            void setYellowLed(AbstractCyclicSwitchable *yellowLedPtr) { AtoActions::yellowLed = yellowLedPtr; }
            void setGreenLed(AbstractCyclicSwitchable *greenLedPtr) { AtoActions::greenLed = greenLedPtr; }
            void setBuzzer(AbstractCyclicSwitchable *buzzerPtr) { AtoActions::buzzer = buzzerPtr; }
            void setWaterDispenser(AbstractSwitchable *waterDispenserPtr) { AtoActions::waterDispenser = waterDispenserPtr; }
            void setSleepTimer(Timer *sleepTimer) { AtoActions::sleepTimer = sleepTimer; }
            void setIdleTimer(Timer *idleTimer) { AtoActions::idleTimer = idleTimer; }

        private:
            void setOn(AbstractCyclicSwitchable *cyclicSwitchable, const uint8_t patternSize, const uint32_t *pattern)
            {
                if (cyclicSwitchable != nullptr)
                {
                    if ((pattern != nullptr) && (patternSize > 0))
                    {
                        cyclicSwitchable->setCycleArray(patternSize, pattern);
                    }
                    cyclicSwitchable->setOn();
                }
            }

            void setOn(AbstractSwitchable *switchable)
            {
                if (switchable != nullptr)
                {
                    switchable->setOn();
                }
            }

            void setOff(AbstractSwitchable *switchable)
            {
                if (switchable != nullptr)
                {
                    switchable->setOff();
                }
            }
        };

    } /* namespace ato */
} /* namespace xal */

#endif
