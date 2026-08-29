/**
 * Regression tests for xal::ato::AtoFsm's transition table.
 *
 * PURPOSE
 * -------
 * These tests lock in the CURRENT behavior of AtoFsm::dispatch() (the
 * hand-rolled nested-switch implementation) before it is refactored into a
 * data-driven transition table. Every explicit transition in dispatch() is
 * transcribed here as a (fromState, event) -> toState assertion, plus one
 * "unhandled event causes no transition" guard per state.
 *
 * After the transition-table refactor lands, re-run this exact suite
 * unchanged against the new AtoFsm implementation. A green run is the
 * evidence that the refactor preserved behavior.
 *
 * SCOPE
 * -----
 * These tests only observe *state transitions* (dispatch(event) -> getState()).
 * They do NOT verify onEntry/onExit hardware side effects (LED/buzzer
 * patterns, timer start/stop) — AtoActions's component pointers are all
 * nullptr when default-constructed, and every setOn()/setOff() helper in
 * AtoActions already null-checks before touching them, so a bare
 * `AtoActions actions;` is safe to dispatch against for this purpose.
 *
 * HOW TO RUN
 * ----------
 * This project doesn't have a native/mocked Arduino test environment (Timer,
 * CyclicSwitchable, etc. all transitively depend on real Arduino symbols),
 * so these tests run on-target via PlatformIO's embedded Unity runner:
 *
 *     pio test -e nanoatmega328
 *
 * ...with the Nano connected over USB. PlatformIO uploads a test firmware
 * image and reads pass/fail results back over serial.
 *
 * PREREQUISITE
 * ------------
 * AtoFsm.h must expose a `State getState() const` accessor (see the
 * accompanying updated AtoFsm.h). That accessor is purely additive — it
 * does not change dispatch()/transit()/enter()/exit() behavior.
 */

#include <Arduino.h>
#include <unity.h>

#include "ato/AtoFsm.h"

using xal::ato::AtoActions;
using xal::ato::AtoFsm;
using xal::ato::Event;
using xal::ato::State;

namespace
{

    /**
     * @brief Starts a fresh FSM (always begins in State::Idle), dispatches
     * testEvent, and returns the resulting state.
     */
    State transitionFromIdle(Event testEvent)
    {
        AtoActions actions;
        AtoFsm fsm(actions);
        fsm.dispatch(testEvent);
        return fsm.getState();
    }

    /**
     * @brief Starts a fresh FSM, dispatches arriveVia to reach the state
     * under test, then dispatches testEvent and returns the resulting state.
     * Every state in this FSM is reachable in exactly one hop from Idle.
     */
    State transitionFrom(Event arriveVia, Event testEvent)
    {
        AtoActions actions;
        AtoFsm fsm(actions);
        fsm.dispatch(arriveVia);
        fsm.dispatch(testEvent);
        return fsm.getState();
    }

    /**
     * @brief Same as transitionFrom, but only dispatches arriveVia and
     * returns the resulting state — used to confirm each state is
     * reachable at all before testing transitions out of it.
     */
    State reach(Event arriveVia)
    {
        AtoActions actions;
        AtoFsm fsm(actions);
        fsm.dispatch(arriveVia);
        return fsm.getState();
    }

} // namespace

/* ============================================================ */
/* Reachability: confirm the one-hop arrival event for each      */
/* non-Idle state actually lands where the helpers above assume. */
/* ============================================================ */

void test_reach_dispensingInAutoMode()
{
    TEST_ASSERT_EQUAL_INT((int)State::DispensingInAutoMode,
        (int)reach(Event::NormalLevelSensorNotTriggered));
}

void test_reach_dispensingInManualMode()
{
    TEST_ASSERT_EQUAL_INT((int)State::DispensingInManualMode,
        (int)reach(Event::DispenseButtonIsPushed));
}

void test_reach_waterLevelLow()
{
    TEST_ASSERT_EQUAL_INT((int)State::WaterLevelLow,
        (int)reach(Event::LowLevelSensorNotTriggered));
}

void test_reach_waterLevelHigh()
{
    TEST_ASSERT_EQUAL_INT((int)State::WaterLevelHigh,
        (int)reach(Event::HighLevelSensorIsTriggered));
}

void test_reach_reservoirEmpty()
{
    TEST_ASSERT_EQUAL_INT((int)State::ReservoirEmpty,
        (int)reach(Event::ReservoirLevelSensorNotTriggered));
}

void test_reach_sleeping()
{
    TEST_ASSERT_EQUAL_INT((int)State::Sleeping,
        (int)reach(Event::SleepButtonIsPushed));
}

void test_reach_idleForTooLong()
{
    TEST_ASSERT_EQUAL_INT((int)State::IdleForTooLong,
        (int)reach(Event::MaxIdleTimeElapsed));
}

void test_reach_error()
{
    TEST_ASSERT_EQUAL_INT((int)State::Error,
        (int)reach(Event::SleepTimeElapsed));
}

/* ============================================================ */
/* State::Idle                                                   */
/* ============================================================ */

void test_idle_highLevelIsTriggered_to_waterLevelHigh()
{
    TEST_ASSERT_EQUAL_INT((int)State::WaterLevelHigh,
        (int)transitionFromIdle(Event::HighLevelSensorIsTriggered));
}

void test_idle_lowLevelNotTriggered_to_waterLevelLow()
{
    TEST_ASSERT_EQUAL_INT((int)State::WaterLevelLow,
        (int)transitionFromIdle(Event::LowLevelSensorNotTriggered));
}

void test_idle_sleepTimeElapsed_to_error()
{
    TEST_ASSERT_EQUAL_INT((int)State::Error,
        (int)transitionFromIdle(Event::SleepTimeElapsed));
}

void test_idle_dispenserOnTimeElapsed_to_error()
{
    TEST_ASSERT_EQUAL_INT((int)State::Error,
        (int)transitionFromIdle(Event::DispenserOnTimeElapsed));
}

void test_idle_normalLevelNotTriggered_to_dispensingInAutoMode()
{
    TEST_ASSERT_EQUAL_INT((int)State::DispensingInAutoMode,
        (int)transitionFromIdle(Event::NormalLevelSensorNotTriggered));
}

void test_idle_reservoirLevelNotTriggered_to_reservoirEmpty()
{
    TEST_ASSERT_EQUAL_INT((int)State::ReservoirEmpty,
        (int)transitionFromIdle(Event::ReservoirLevelSensorNotTriggered));
}

void test_idle_dispenseButtonPushed_to_dispensingInManualMode()
{
    TEST_ASSERT_EQUAL_INT((int)State::DispensingInManualMode,
        (int)transitionFromIdle(Event::DispenseButtonIsPushed));
}

void test_idle_maxIdleTimeElapsed_to_idleForTooLong()
{
    TEST_ASSERT_EQUAL_INT((int)State::IdleForTooLong,
        (int)transitionFromIdle(Event::MaxIdleTimeElapsed));
}

void test_idle_sleepButtonPushed_to_sleeping()
{
    TEST_ASSERT_EQUAL_INT((int)State::Sleeping,
        (int)transitionFromIdle(Event::SleepButtonIsPushed));
}

void test_idle_unhandledEvent_staysIdle()
{
    /* HighLevelSensorNotTriggered has no case in Idle's table. */
    TEST_ASSERT_EQUAL_INT((int)State::Idle,
        (int)transitionFromIdle(Event::HighLevelSensorNotTriggered));
}

/* ============================================================ */
/* State::DispensingInAutoMode                                   */
/* ============================================================ */

void test_dispensingInAutoMode_highLevelIsTriggered_to_waterLevelHigh()
{
    TEST_ASSERT_EQUAL_INT((int)State::WaterLevelHigh,
        (int)transitionFrom(Event::NormalLevelSensorNotTriggered, Event::HighLevelSensorIsTriggered));
}

void test_dispensingInAutoMode_lowLevelNotTriggered_to_waterLevelLow()
{
    TEST_ASSERT_EQUAL_INT((int)State::WaterLevelLow,
        (int)transitionFrom(Event::NormalLevelSensorNotTriggered, Event::LowLevelSensorNotTriggered));
}

void test_dispensingInAutoMode_sleepTimeElapsed_to_error()
{
    TEST_ASSERT_EQUAL_INT((int)State::Error,
        (int)transitionFrom(Event::NormalLevelSensorNotTriggered, Event::SleepTimeElapsed));
}

void test_dispensingInAutoMode_normalLevelIsTriggered_to_idle()
{
    TEST_ASSERT_EQUAL_INT((int)State::Idle,
        (int)transitionFrom(Event::NormalLevelSensorNotTriggered, Event::NormalLevelSensorIsTriggered));
}

void test_dispensingInAutoMode_dispenseButtonPushed_to_idle()
{
    TEST_ASSERT_EQUAL_INT((int)State::Idle,
        (int)transitionFrom(Event::NormalLevelSensorNotTriggered, Event::DispenseButtonIsPushed));
}

void test_dispensingInAutoMode_reservoirLevelNotTriggered_to_reservoirEmpty()
{
    TEST_ASSERT_EQUAL_INT((int)State::ReservoirEmpty,
        (int)transitionFrom(Event::NormalLevelSensorNotTriggered, Event::ReservoirLevelSensorNotTriggered));
}

void test_dispensingInAutoMode_dispenserOnTimeElapsed_to_reservoirEmpty()
{
    TEST_ASSERT_EQUAL_INT((int)State::ReservoirEmpty,
        (int)transitionFrom(Event::NormalLevelSensorNotTriggered, Event::DispenserOnTimeElapsed));
}

void test_dispensingInAutoMode_sleepButtonPushed_to_sleeping()
{
    TEST_ASSERT_EQUAL_INT((int)State::Sleeping,
        (int)transitionFrom(Event::NormalLevelSensorNotTriggered, Event::SleepButtonIsPushed));
}

void test_dispensingInAutoMode_unhandledEvent_staysInState()
{
    /* ReservoirLevelSensorIsTriggered has no case in DispensingInAutoMode's table. */
    TEST_ASSERT_EQUAL_INT((int)State::DispensingInAutoMode,
        (int)transitionFrom(Event::NormalLevelSensorNotTriggered, Event::ReservoirLevelSensorIsTriggered));
}

/* ============================================================ */
/* State::DispensingInManualMode                                 */
/* ============================================================ */

void test_dispensingInManualMode_highLevelIsTriggered_to_waterLevelHigh()
{
    TEST_ASSERT_EQUAL_INT((int)State::WaterLevelHigh,
        (int)transitionFrom(Event::DispenseButtonIsPushed, Event::HighLevelSensorIsTriggered));
}

void test_dispensingInManualMode_sleepTimeElapsed_to_error()
{
    TEST_ASSERT_EQUAL_INT((int)State::Error,
        (int)transitionFrom(Event::DispenseButtonIsPushed, Event::SleepTimeElapsed));
}

void test_dispensingInManualMode_normalLevelIsTriggered_to_idle()
{
    TEST_ASSERT_EQUAL_INT((int)State::Idle,
        (int)transitionFrom(Event::DispenseButtonIsPushed, Event::NormalLevelSensorIsTriggered));
}

void test_dispensingInManualMode_dispenseButtonPushed_to_idle()
{
    TEST_ASSERT_EQUAL_INT((int)State::Idle,
        (int)transitionFrom(Event::DispenseButtonIsPushed, Event::DispenseButtonIsPushed));
}

void test_dispensingInManualMode_dispenserOnTimeElapsed_to_idle()
{
    TEST_ASSERT_EQUAL_INT((int)State::Idle,
        (int)transitionFrom(Event::DispenseButtonIsPushed, Event::DispenserOnTimeElapsed));
}

void test_dispensingInManualMode_reservoirLevelNotTriggered_to_reservoirEmpty()
{
    TEST_ASSERT_EQUAL_INT((int)State::ReservoirEmpty,
        (int)transitionFrom(Event::DispenseButtonIsPushed, Event::ReservoirLevelSensorNotTriggered));
}

void test_dispensingInManualMode_sleepButtonPushed_to_sleeping()
{
    TEST_ASSERT_EQUAL_INT((int)State::Sleeping,
        (int)transitionFrom(Event::DispenseButtonIsPushed, Event::SleepButtonIsPushed));
}

void test_dispensingInManualMode_unhandledEvent_staysInState()
{
    /* LowLevelSensorIsTriggered has no case in DispensingInManualMode's table. */
    TEST_ASSERT_EQUAL_INT((int)State::DispensingInManualMode,
        (int)transitionFrom(Event::DispenseButtonIsPushed, Event::LowLevelSensorIsTriggered));
}

/* ============================================================ */
/* State::WaterLevelLow                                          */
/* ============================================================ */

void test_waterLevelLow_lowLevelIsTriggered_to_idle()
{
    TEST_ASSERT_EQUAL_INT((int)State::Idle,
        (int)transitionFrom(Event::LowLevelSensorNotTriggered, Event::LowLevelSensorIsTriggered));
}

void test_waterLevelLow_normalLevelIsTriggered_to_idle()
{
    TEST_ASSERT_EQUAL_INT((int)State::Idle,
        (int)transitionFrom(Event::LowLevelSensorNotTriggered, Event::NormalLevelSensorIsTriggered));
}

void test_waterLevelLow_dispenseButtonPushed_to_dispensingInManualMode()
{
    TEST_ASSERT_EQUAL_INT((int)State::DispensingInManualMode,
        (int)transitionFrom(Event::LowLevelSensorNotTriggered, Event::DispenseButtonIsPushed));
}

void test_waterLevelLow_sleepButtonPushed_to_sleeping()
{
    TEST_ASSERT_EQUAL_INT((int)State::Sleeping,
        (int)transitionFrom(Event::LowLevelSensorNotTriggered, Event::SleepButtonIsPushed));
}

void test_waterLevelLow_unhandledEvent_staysInState()
{
    /* HighLevelSensorIsTriggered has no case in WaterLevelLow's table. */
    TEST_ASSERT_EQUAL_INT((int)State::WaterLevelLow,
        (int)transitionFrom(Event::LowLevelSensorNotTriggered, Event::HighLevelSensorIsTriggered));
}

/* ============================================================ */
/* State::WaterLevelHigh                                         */
/* ============================================================ */

void test_waterLevelHigh_dispenseButtonPushed_to_idle()
{
    TEST_ASSERT_EQUAL_INT((int)State::Idle,
        (int)transitionFrom(Event::HighLevelSensorIsTriggered, Event::DispenseButtonIsPushed));
}

void test_waterLevelHigh_highLevelNotTriggered_to_idle()
{
    TEST_ASSERT_EQUAL_INT((int)State::Idle,
        (int)transitionFrom(Event::HighLevelSensorIsTriggered, Event::HighLevelSensorNotTriggered));
}

void test_waterLevelHigh_sleepButtonPushed_to_sleeping()
{
    TEST_ASSERT_EQUAL_INT((int)State::Sleeping,
        (int)transitionFrom(Event::HighLevelSensorIsTriggered, Event::SleepButtonIsPushed));
}

void test_waterLevelHigh_normalLevelNotTriggered_to_error()
{
    TEST_ASSERT_EQUAL_INT((int)State::Error,
        (int)transitionFrom(Event::HighLevelSensorIsTriggered, Event::NormalLevelSensorNotTriggered));
}

void test_waterLevelHigh_lowLevelNotTriggered_to_error()
{
    TEST_ASSERT_EQUAL_INT((int)State::Error,
        (int)transitionFrom(Event::HighLevelSensorIsTriggered, Event::LowLevelSensorNotTriggered));
}

void test_waterLevelHigh_unhandledEvent_staysInState()
{
    /* LowLevelSensorIsTriggered has no case in WaterLevelHigh's table. */
    TEST_ASSERT_EQUAL_INT((int)State::WaterLevelHigh,
        (int)transitionFrom(Event::HighLevelSensorIsTriggered, Event::LowLevelSensorIsTriggered));
}

/* ============================================================ */
/* State::ReservoirEmpty                                         */
/* ============================================================ */

void test_reservoirEmpty_highLevelIsTriggered_to_error()
{
    TEST_ASSERT_EQUAL_INT((int)State::Error,
        (int)transitionFrom(Event::ReservoirLevelSensorNotTriggered, Event::HighLevelSensorIsTriggered));
}

void test_reservoirEmpty_lowLevelNotTriggered_to_error()
{
    TEST_ASSERT_EQUAL_INT((int)State::Error,
        (int)transitionFrom(Event::ReservoirLevelSensorNotTriggered, Event::LowLevelSensorNotTriggered));
}

void test_reservoirEmpty_sleepTimeElapsed_to_error()
{
    TEST_ASSERT_EQUAL_INT((int)State::Error,
        (int)transitionFrom(Event::ReservoirLevelSensorNotTriggered, Event::SleepTimeElapsed));
}

void test_reservoirEmpty_dispenserOnTimeElapsed_to_error()
{
    TEST_ASSERT_EQUAL_INT((int)State::Error,
        (int)transitionFrom(Event::ReservoirLevelSensorNotTriggered, Event::DispenserOnTimeElapsed));
}

void test_reservoirEmpty_normalLevelIsTriggered_to_idle()
{
    TEST_ASSERT_EQUAL_INT((int)State::Idle,
        (int)transitionFrom(Event::ReservoirLevelSensorNotTriggered, Event::NormalLevelSensorIsTriggered));
}

void test_reservoirEmpty_dispenseButtonPushed_to_idle()
{
    TEST_ASSERT_EQUAL_INT((int)State::Idle,
        (int)transitionFrom(Event::ReservoirLevelSensorNotTriggered, Event::DispenseButtonIsPushed));
}

void test_reservoirEmpty_reservoirLevelIsTriggered_to_idle()
{
    TEST_ASSERT_EQUAL_INT((int)State::Idle,
        (int)transitionFrom(Event::ReservoirLevelSensorNotTriggered, Event::ReservoirLevelSensorIsTriggered));
}

void test_reservoirEmpty_sleepButtonPushed_to_sleeping()
{
    TEST_ASSERT_EQUAL_INT((int)State::Sleeping,
        (int)transitionFrom(Event::ReservoirLevelSensorNotTriggered, Event::SleepButtonIsPushed));
}

void test_reservoirEmpty_unhandledEvent_staysInState()
{
    /* LowLevelSensorIsTriggered has no case in ReservoirEmpty's table. */
    TEST_ASSERT_EQUAL_INT((int)State::ReservoirEmpty,
        (int)transitionFrom(Event::ReservoirLevelSensorNotTriggered, Event::LowLevelSensorIsTriggered));
}

/* ============================================================ */
/* State::Sleeping                                                */
/* ============================================================ */

void test_sleeping_dispenseButtonPushed_to_idle()
{
    TEST_ASSERT_EQUAL_INT((int)State::Idle,
        (int)transitionFrom(Event::SleepButtonIsPushed, Event::DispenseButtonIsPushed));
}

void test_sleeping_sleepButtonPushed_to_idle()
{
    TEST_ASSERT_EQUAL_INT((int)State::Idle,
        (int)transitionFrom(Event::SleepButtonIsPushed, Event::SleepButtonIsPushed));
}

void test_sleeping_sleepTimeElapsed_to_idle()
{
    TEST_ASSERT_EQUAL_INT((int)State::Idle,
        (int)transitionFrom(Event::SleepButtonIsPushed, Event::SleepTimeElapsed));
}

void test_sleeping_unhandledEvent_staysInState()
{
    /* HighLevelSensorIsTriggered has no case in Sleeping's table. */
    TEST_ASSERT_EQUAL_INT((int)State::Sleeping,
        (int)transitionFrom(Event::SleepButtonIsPushed, Event::HighLevelSensorIsTriggered));
}

/* ============================================================ */
/* State::IdleForTooLong                                         */
/* ============================================================ */

void test_idleForTooLong_normalLevelNotTriggered_to_dispensingInAutoMode()
{
    TEST_ASSERT_EQUAL_INT((int)State::DispensingInAutoMode,
        (int)transitionFrom(Event::MaxIdleTimeElapsed, Event::NormalLevelSensorNotTriggered));
}

void test_idleForTooLong_dispenseButtonPushed_to_idle()
{
    TEST_ASSERT_EQUAL_INT((int)State::Idle,
        (int)transitionFrom(Event::MaxIdleTimeElapsed, Event::DispenseButtonIsPushed));
}

void test_idleForTooLong_lowLevelNotTriggered_to_waterLevelLow()
{
    TEST_ASSERT_EQUAL_INT((int)State::WaterLevelLow,
        (int)transitionFrom(Event::MaxIdleTimeElapsed, Event::LowLevelSensorNotTriggered));
}

void test_idleForTooLong_highLevelIsTriggered_to_waterLevelHigh()
{
    TEST_ASSERT_EQUAL_INT((int)State::WaterLevelHigh,
        (int)transitionFrom(Event::MaxIdleTimeElapsed, Event::HighLevelSensorIsTriggered));
}

void test_idleForTooLong_reservoirLevelNotTriggered_to_reservoirEmpty()
{
    TEST_ASSERT_EQUAL_INT((int)State::ReservoirEmpty,
        (int)transitionFrom(Event::MaxIdleTimeElapsed, Event::ReservoirLevelSensorNotTriggered));
}

void test_idleForTooLong_sleepButtonPushed_to_sleeping()
{
    TEST_ASSERT_EQUAL_INT((int)State::Sleeping,
        (int)transitionFrom(Event::MaxIdleTimeElapsed, Event::SleepButtonIsPushed));
}

void test_idleForTooLong_unhandledEvent_staysInState()
{
    /* NormalLevelSensorIsTriggered has no case in IdleForTooLong's table
     * (only the *NotTriggered variant is handled there). This is also the
     * state involved in the historical missing-`break` fallthrough bug —
     * this guard protects against that class of regression reappearing. */
    TEST_ASSERT_EQUAL_INT((int)State::IdleForTooLong,
        (int)transitionFrom(Event::MaxIdleTimeElapsed, Event::NormalLevelSensorIsTriggered));
}

/* ============================================================ */
/* State::Error                                                  */
/* ============================================================ */

void test_error_dispenseButtonPushed_to_idle()
{
    TEST_ASSERT_EQUAL_INT((int)State::Idle,
        (int)transitionFrom(Event::SleepTimeElapsed, Event::DispenseButtonIsPushed));
}

void test_error_sleepButtonPushed_to_sleeping()
{
    TEST_ASSERT_EQUAL_INT((int)State::Sleeping,
        (int)transitionFrom(Event::SleepTimeElapsed, Event::SleepButtonIsPushed));
}

void test_error_unhandledEvent_staysInState()
{
    /* HighLevelSensorIsTriggered has no case in Error's table. */
    TEST_ASSERT_EQUAL_INT((int)State::Error,
        (int)transitionFrom(Event::SleepTimeElapsed, Event::HighLevelSensorIsTriggered));
}

/* ============================================================ */
/* Unity runner                                                  */
/* ============================================================ */

void setup()
{
    delay(2000); /* allow board/serial to settle before Unity output starts */

    UNITY_BEGIN();

    RUN_TEST(test_reach_dispensingInAutoMode);
    RUN_TEST(test_reach_dispensingInManualMode);
    RUN_TEST(test_reach_waterLevelLow);
    RUN_TEST(test_reach_waterLevelHigh);
    RUN_TEST(test_reach_reservoirEmpty);
    RUN_TEST(test_reach_sleeping);
    RUN_TEST(test_reach_idleForTooLong);
    RUN_TEST(test_reach_error);

    RUN_TEST(test_idle_highLevelIsTriggered_to_waterLevelHigh);
    RUN_TEST(test_idle_lowLevelNotTriggered_to_waterLevelLow);
    RUN_TEST(test_idle_sleepTimeElapsed_to_error);
    RUN_TEST(test_idle_dispenserOnTimeElapsed_to_error);
    RUN_TEST(test_idle_normalLevelNotTriggered_to_dispensingInAutoMode);
    RUN_TEST(test_idle_reservoirLevelNotTriggered_to_reservoirEmpty);
    RUN_TEST(test_idle_dispenseButtonPushed_to_dispensingInManualMode);
    RUN_TEST(test_idle_maxIdleTimeElapsed_to_idleForTooLong);
    RUN_TEST(test_idle_sleepButtonPushed_to_sleeping);
    RUN_TEST(test_idle_unhandledEvent_staysIdle);

    RUN_TEST(test_dispensingInAutoMode_highLevelIsTriggered_to_waterLevelHigh);
    RUN_TEST(test_dispensingInAutoMode_lowLevelNotTriggered_to_waterLevelLow);
    RUN_TEST(test_dispensingInAutoMode_sleepTimeElapsed_to_error);
    RUN_TEST(test_dispensingInAutoMode_normalLevelIsTriggered_to_idle);
    RUN_TEST(test_dispensingInAutoMode_dispenseButtonPushed_to_idle);
    RUN_TEST(test_dispensingInAutoMode_reservoirLevelNotTriggered_to_reservoirEmpty);
    RUN_TEST(test_dispensingInAutoMode_dispenserOnTimeElapsed_to_reservoirEmpty);
    RUN_TEST(test_dispensingInAutoMode_sleepButtonPushed_to_sleeping);
    RUN_TEST(test_dispensingInAutoMode_unhandledEvent_staysInState);

    RUN_TEST(test_dispensingInManualMode_highLevelIsTriggered_to_waterLevelHigh);
    RUN_TEST(test_dispensingInManualMode_sleepTimeElapsed_to_error);
    RUN_TEST(test_dispensingInManualMode_normalLevelIsTriggered_to_idle);
    RUN_TEST(test_dispensingInManualMode_dispenseButtonPushed_to_idle);
    RUN_TEST(test_dispensingInManualMode_dispenserOnTimeElapsed_to_idle);
    RUN_TEST(test_dispensingInManualMode_reservoirLevelNotTriggered_to_reservoirEmpty);
    RUN_TEST(test_dispensingInManualMode_sleepButtonPushed_to_sleeping);
    RUN_TEST(test_dispensingInManualMode_unhandledEvent_staysInState);

    RUN_TEST(test_waterLevelLow_lowLevelIsTriggered_to_idle);
    RUN_TEST(test_waterLevelLow_normalLevelIsTriggered_to_idle);
    RUN_TEST(test_waterLevelLow_dispenseButtonPushed_to_dispensingInManualMode);
    RUN_TEST(test_waterLevelLow_sleepButtonPushed_to_sleeping);
    RUN_TEST(test_waterLevelLow_unhandledEvent_staysInState);

    RUN_TEST(test_waterLevelHigh_dispenseButtonPushed_to_idle);
    RUN_TEST(test_waterLevelHigh_highLevelNotTriggered_to_idle);
    RUN_TEST(test_waterLevelHigh_sleepButtonPushed_to_sleeping);
    RUN_TEST(test_waterLevelHigh_normalLevelNotTriggered_to_error);
    RUN_TEST(test_waterLevelHigh_lowLevelNotTriggered_to_error);
    RUN_TEST(test_waterLevelHigh_unhandledEvent_staysInState);

    RUN_TEST(test_reservoirEmpty_highLevelIsTriggered_to_error);
    RUN_TEST(test_reservoirEmpty_lowLevelNotTriggered_to_error);
    RUN_TEST(test_reservoirEmpty_sleepTimeElapsed_to_error);
    RUN_TEST(test_reservoirEmpty_dispenserOnTimeElapsed_to_error);
    RUN_TEST(test_reservoirEmpty_normalLevelIsTriggered_to_idle);
    RUN_TEST(test_reservoirEmpty_dispenseButtonPushed_to_idle);
    RUN_TEST(test_reservoirEmpty_reservoirLevelIsTriggered_to_idle);
    RUN_TEST(test_reservoirEmpty_sleepButtonPushed_to_sleeping);
    RUN_TEST(test_reservoirEmpty_unhandledEvent_staysInState);

    RUN_TEST(test_sleeping_dispenseButtonPushed_to_idle);
    RUN_TEST(test_sleeping_sleepButtonPushed_to_idle);
    RUN_TEST(test_sleeping_sleepTimeElapsed_to_idle);
    RUN_TEST(test_sleeping_unhandledEvent_staysInState);

    RUN_TEST(test_idleForTooLong_normalLevelNotTriggered_to_dispensingInAutoMode);
    RUN_TEST(test_idleForTooLong_dispenseButtonPushed_to_idle);
    RUN_TEST(test_idleForTooLong_lowLevelNotTriggered_to_waterLevelLow);
    RUN_TEST(test_idleForTooLong_highLevelIsTriggered_to_waterLevelHigh);
    RUN_TEST(test_idleForTooLong_reservoirLevelNotTriggered_to_reservoirEmpty);
    RUN_TEST(test_idleForTooLong_sleepButtonPushed_to_sleeping);
    RUN_TEST(test_idleForTooLong_unhandledEvent_staysInState);

    RUN_TEST(test_error_dispenseButtonPushed_to_idle);
    RUN_TEST(test_error_sleepButtonPushed_to_sleeping);
    RUN_TEST(test_error_unhandledEvent_staysInState);

    UNITY_END();
}

void loop()
{
    /* Tests run once in setup(); nothing to do here. */
}
