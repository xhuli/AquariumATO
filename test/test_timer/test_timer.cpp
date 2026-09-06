/**
 * Regression tests for xal::Timer, driven via process(nowMs) and the
 * nowMs-injectable setOn(uint32_t) overload extracted from loop()/setOn()
 * (see Timer.h for why: loop() and setOn() call millis() directly, so
 * these overloads are the seam that lets tests run deterministically with
 * fabricated timestamps and no hardware dependency).
 *
 * HOW TO RUN
 * ----------
 *     pio test -e nanoatmega328
 * Small suite, expected to fit the Nano directly; fall back to
 * -e megaatmega2560 if it doesn't.
 */

#include <Arduino.h>
#include <unity.h>

#include <Timer.h>

using xal::Timer;

namespace {
    uint8_t fireCount = 0;

    void onFire() {
        fireCount++;
    }

    void resetCounters() {
        fireCount = 0;
    }

    void configureTimer(Timer &timer, uint32_t durationMs, bool autoRestart) {
        timer.setDurationMs(durationMs);
        timer.setAutoRestart(autoRestart);
        timer.setCallback(onFire);
    }
} // namespace

void test_timer_does_nothing_while_off() {
    resetCounters();
    Timer timer;
    configureTimer(timer, 1000, false);

    /* Never turned on -- process() must be a no-op regardless of elapsed time. */
    timer.process(5000);

    TEST_ASSERT_EQUAL_UINT8(0, fireCount);
    TEST_ASSERT_TRUE(timer.isOff());
}

void test_timer_fires_and_turns_off_when_elapsed_without_autoRestart() {
    resetCounters();
    Timer timer;
    configureTimer(timer, 1000, false);

    timer.setOn(0);

    timer.process(500); /* not yet elapsed */
    TEST_ASSERT_EQUAL_UINT8(0, fireCount);
    TEST_ASSERT_TRUE(timer.isOn());

    timer.process(1000); /* elapsed */
    TEST_ASSERT_EQUAL_UINT8(1, fireCount);
    TEST_ASSERT_TRUE(timer.isOff());
}

void test_timer_autoRestarts_and_fires_again_each_interval() {
    resetCounters();
    Timer timer;
    configureTimer(timer, 1000, true);

    timer.setOn(0);

    timer.process(1000); /* first elapse: fires, restarts (startMs -> 1000) */
    TEST_ASSERT_EQUAL_UINT8(1, fireCount);
    TEST_ASSERT_TRUE(timer.isOn());

    timer.process(1500); /* only 500ms since restart: not yet elapsed */
    TEST_ASSERT_EQUAL_UINT8(1, fireCount);

    timer.process(2000); /* 1000ms since restart: elapsed again */
    TEST_ASSERT_EQUAL_UINT8(2, fireCount);
    TEST_ASSERT_TRUE(timer.isOn());
}

void test_timer_manual_setOff_cancels_pending_fire() {
    resetCounters();
    Timer timer;
    configureTimer(timer, 500, false);

    timer.setOn(0);
    timer.setOff(); /* manually stopped before the duration elapses */

    timer.process(10000); /* huge elapsed time, but timer is off */

    TEST_ASSERT_EQUAL_UINT8(0, fireCount);
    TEST_ASSERT_TRUE(timer.isOff());
}

void setup() {
    delay(2000);

    UNITY_BEGIN();

    RUN_TEST(test_timer_does_nothing_while_off);
    RUN_TEST(test_timer_fires_and_turns_off_when_elapsed_without_autoRestart);
    RUN_TEST(test_timer_autoRestarts_and_fires_again_each_interval);
    RUN_TEST(test_timer_manual_setOff_cancels_pending_fire);

    UNITY_END();
}

void loop() {
}
