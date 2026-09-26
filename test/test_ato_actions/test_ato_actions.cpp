/**
 * Regression tests for xal::ato::AtoActions's pump routing, added alongside
 * the manual-dispensing extended-timeout change (see main.cpp and
 * AtoActions.h): DispensingInAutoModeState must drive waterDispenser,
 * DispensingInManualModeState must drive manualWaterDispenser, and exiting
 * either state must turn both off, regardless of which one was active.
 *
 * AtoActions only ever sees these two pump handles through the generic
 * AbstractSwitchable interface (setOn()/setOff()), so a test-local
 * FakeSwitchable that counts calls is enough to confirm routing without any
 * real pin or TimedSwitchable involved -- the same pattern used by
 * test_timed_switchable.
 *
 * SCOPE
 * -----
 * This suite only covers pump routing. LED/buzzer/timer side effects are
 * intentionally left uncovered here, consistent with test_ato_fsm's existing
 * scope note that AtoActions's non-pump side effects are not comprehensively
 * regression-tested.
 *
 * HOW TO RUN
 * ----------
 *     pio test -e nanoatmega328
 * Small suite (few AtoActions instances), expected to fit the Nano
 * directly; fall back to -e megaatmega2560 if it doesn't.
 */

#include <Arduino.h>
#include <unity.h>

#include "ato/AtoActions.h"
#include <api/AbstractSwitchable.h>

using xal::AbstractSwitchable;
using xal::ato::AtoActions;

namespace {
    /**
     * @brief A minimal AbstractSwitchable that just counts calls, so tests
     * can confirm AtoActions delegates to the correct pump handle -- not
     * just that some internal state changed.
     */
    class FakeSwitchable : public AbstractSwitchable {
    public:
        uint8_t onCount = 0;
        uint8_t offCount = 0;

        void setOn() override {
            AbstractSwitchable::setOn();
            onCount++;
        }

        void setOff() override {
            AbstractSwitchable::setOff();
            offCount++;
        }
    };
} // namespace

void test_auto_entry_turns_on_waterDispenser_only() {
    FakeSwitchable autoPump;
    FakeSwitchable manualPump;
    AtoActions actions;
    actions.setAutoWaterDispenser(&autoPump);
    actions.setManualWaterDispenser(&manualPump);

    actions.onEntryDispensingInAutoModeState();

    TEST_ASSERT_EQUAL_UINT8(1, autoPump.onCount);
    TEST_ASSERT_EQUAL_UINT8(0, manualPump.onCount);
}

void test_manual_entry_turns_on_manualWaterDispenser_only() {
    FakeSwitchable autoPump;
    FakeSwitchable manualPump;
    AtoActions actions;
    actions.setAutoWaterDispenser(&autoPump);
    actions.setManualWaterDispenser(&manualPump);

    actions.onEntryDispensingInManualModeState();

    TEST_ASSERT_EQUAL_UINT8(0, autoPump.onCount);
    TEST_ASSERT_EQUAL_UINT8(1, manualPump.onCount);
}

void test_exit_turns_off_both_dispensers_after_auto_entry() {
    FakeSwitchable autoPump;
    FakeSwitchable manualPump;
    AtoActions actions;
    actions.setAutoWaterDispenser(&autoPump);
    actions.setManualWaterDispenser(&manualPump);

    actions.onEntryDispensingInAutoModeState();
    actions.onExitState();

    TEST_ASSERT_EQUAL_UINT8(1, autoPump.offCount);
    TEST_ASSERT_EQUAL_UINT8(1, manualPump.offCount); /* never was on, but exit is unconditional */
}

void test_exit_turns_off_both_dispensers_after_manual_entry() {
    FakeSwitchable autoPump;
    FakeSwitchable manualPump;
    AtoActions actions;
    actions.setAutoWaterDispenser(&autoPump);
    actions.setManualWaterDispenser(&manualPump);

    actions.onEntryDispensingInManualModeState();
    actions.onExitState();

    TEST_ASSERT_EQUAL_UINT8(1, autoPump.offCount); /* never was on, but exit is unconditional */
    TEST_ASSERT_EQUAL_UINT8(1, manualPump.offCount);
}

void test_entries_are_safe_with_unset_pump_handles() {
    AtoActions actions; /* waterDispenser / manualWaterDispenser left nullptr */

    actions.onEntryDispensingInAutoModeState();
    actions.onEntryDispensingInManualModeState();
    actions.onExitState();
    /* No crash / no assertion: mirrors test_ato_fsm's existing reliance on
       AtoActions's null-checked setOn()/setOff() helpers. */
    TEST_ASSERT_TRUE(true);
}

void setup() {
    delay(2000);

    UNITY_BEGIN();

    RUN_TEST(test_auto_entry_turns_on_waterDispenser_only);
    RUN_TEST(test_manual_entry_turns_on_manualWaterDispenser_only);
    RUN_TEST(test_exit_turns_off_both_dispensers_after_auto_entry);
    RUN_TEST(test_exit_turns_off_both_dispensers_after_manual_entry);
    RUN_TEST(test_entries_are_safe_with_unset_pump_handles);

    UNITY_END();
}

void loop() {
}
