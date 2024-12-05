#ifndef com_github_xhuli_arduino_ato_AtoActions_H
#define com_github_xhuli_arduino_ato_AtoActions_H
#pragma once

#include "api/AbstractCyclicSwitchable.h"
#include "api/AbstractSwitchable.h"
// #include <ArduinoLog.h>
#include <Timer.h>

namespace xal {
namespace ato {

    class AtoActions {
    private:
        uint32_t led_always_on[1] = { 1000ul };
        uint32_t led_blink_pattern_slow[2] = { 1520, 380 };
        uint32_t led_blink_pattern_fast[2] = { 380, 380 };

        uint32_t buzzer_reservoir_pattern[12] = {
            1400, 400, 1400, 400, 1400, /* --- --- --- (5s) */
            25000, /* idle until 30 seconds elapse from the pattern start */
            1400, 400, 1400, 400, 1400, /* --- --- --- (5s) */
            565000 /* idle until 10 minutes elapse from the pattern start */
        };

        uint32_t buzzer_error_pattern[12] = {
            700, 400, 700, 400, 1400, /* - - --- (3.6s) */
            26400, /* idle until 30 seconds elapse from the pattern start */
            700, 400, 700, 400, 1400, /* - - --- (3.6s) */
            566400 /* idle until 10 minutes elapse from the pattern start */
        };

        uint32_t buzzer_water_low_pattern[12] = {
            1400, 400, 700, 400, 700, /** --- - - (3.6s) */
            26400, /* idle until 30 seconds elapse from the pattern start */
            1400, 400, 700, 400, 700, /** --- - - (3.6s) */
            566400 /* idle until 10 minutes elapse from the pattern start */

        };

        uint32_t buzzer_water_high_pattern[20] = {
            1400, 400, 1400, 400, 1400, 400, 1400, 400, 1400, /* --- --- --- --- --- (8.6s) */
            21400, /* idle until 30 seconds elapse from the pattern start */
            1400, 400, 1400, 400, 1400, 400, 1400, 400, 1400, /* --- --- --- --- --- (8.6s) */
            561400 /* idle until 10 minutes elapse from the pattern start */
        };

        uint32_t buzzer_idle_for_too_long_pattern[24] = {
            400, 1400, 400, 1400, 400, 1400, 400, 1400, 400, 1400, 1800, /* - - - - - --- (9s) */
            21000, /* idle until 30 seconds elapse from the pattern start */
            400, 1400, 400, 1400, 400, 1400, 400, 1400, 400, 1400, 1800, /* - - - - - --- (9s) */
            561000, /* idle until 10 minutes elapse from the pattern start */
        };

        AbstractCyclicSwitchable* redLed = nullptr;
        AbstractCyclicSwitchable* yellowLed = nullptr;
        AbstractCyclicSwitchable* greenLed = nullptr;
        AbstractCyclicSwitchable* buzzer = nullptr;
        AbstractSwitchable* waterDispenser = nullptr;
        Timer* sleepTimer = nullptr;
        Timer* idleTimer = nullptr;

    public:
        AtoActions() = default;

        ~AtoActions() = default;

        /* State actions */

        void onExitState()
        {
            // Log.noticeln("Act: onExit");
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
            // Log.noticeln("Act: onIdl");
            setOn(greenLed, 2, led_blink_pattern_slow);
            setOn(idleTimer);
        }

        void onEntryIdleForTooLongState()
        {
            // Log.noticeln("Act: onIdl42Lng");
            setOn(greenLed, 2, led_blink_pattern_slow);
            setOn(redLed, 2, led_blink_pattern_slow);
            setOn(buzzer, 24, buzzer_idle_for_too_long_pattern);
        }

        void onEntryDispensingInAutoModeState()
        {
            // Log.noticeln("Act: onDspAuto");
            setOn(greenLed, 1, led_always_on);
            setOn(waterDispenser);
        }

        void onEntryDispensingInManualModeState()
        {
            // Log.noticeln("Act: onDspManual");
            setOn(greenLed, 2, led_blink_pattern_fast);
            setOn(waterDispenser);
        }

        void onEntryWaterLowState()
        {
            // Log.noticeln("Act: WtrLvlLow");
            setOn(redLed, 2, led_blink_pattern_fast);
            setOn(buzzer, 12, buzzer_water_low_pattern);
        }

        void onEntryWaterHighState()
        {
            // Log.noticeln("Act: WtrLvlHigh");
            setOn(redLed, 2, led_blink_pattern_fast);
            setOn(buzzer, 20, buzzer_water_high_pattern);
        }

        void onEntryReservoirEmptyState()
        {
            // Log.noticeln("Act: onRE");
            setOn(redLed, 2, led_blink_pattern_slow);
            setOn(buzzer, 12, buzzer_reservoir_pattern);
        }

        void onEntrySleepingState()
        {
            // Log.noticeln("Act: onSlp");
            setOn(yellowLed, 2, led_blink_pattern_slow);
            setOn(sleepTimer);
        }

        void onEntryErrorState()
        {
            // Log.noticeln("Act: onErr");
            setOn(redLed, 2, led_blink_pattern_fast);
            setOn(buzzer, 12, buzzer_error_pattern);
        }

        /* Setters */

        void setRedLed(AbstractCyclicSwitchable* redLedPtr) { AtoActions::redLed = redLedPtr; }
        void setYellowLed(AbstractCyclicSwitchable* yellowLedPtr) { AtoActions::yellowLed = yellowLedPtr; }
        void setGreenLed(AbstractCyclicSwitchable* greenLedPtr) { AtoActions::greenLed = greenLedPtr; }
        void setBuzzer(AbstractCyclicSwitchable* buzzerPtr) { AtoActions::buzzer = buzzerPtr; }
        void setWaterDispenser(AbstractSwitchable* waterDispenserPtr) { AtoActions::waterDispenser = waterDispenserPtr; }
        void setSleepTimer(Timer* sleepTimer) { AtoActions::sleepTimer = sleepTimer; }
        void setIdleTimer(Timer* idleTimer) { AtoActions::idleTimer = idleTimer; }

    private:
        void setOn(AbstractCyclicSwitchable* cyclicSwitchable, const uint8_t patternSize, uint32_t* pattern)
        {
            if (cyclicSwitchable != nullptr) {
                if ((pattern != nullptr) && (patternSize > 0)) {
                    cyclicSwitchable->setCycleArray(patternSize, pattern);
                }
                cyclicSwitchable->setOn();
            } else {
                // Log.noticeln("Act: setOn > nullptr");
            }
        }

        void setOn(AbstractSwitchable* switchable)
        {
            if (switchable != nullptr) {
                switchable->setOn();
            }
        }

        void setOff(AbstractSwitchable* switchable)
        {
            if (switchable != nullptr) {
                switchable->setOff();
            }
        }
    };

} /* namespace ato */
} /* namespace xal */

#endif
