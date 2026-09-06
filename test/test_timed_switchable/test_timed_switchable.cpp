/**
 * Regression tests for xal::TimedSwitchable, driven via process(nowMs) and
 * the nowMs-injectable setOn(uint32_t)/setOff(uint32_t) overloads extracted
 * from loop()/setOn()/setOff() (see TimedSwitchable.h for why: those calls
 * read millis() directly, so these overloads are the seam that lets tests
 * run deterministically with fabricated timestamps).
 *
 * TimedSwitchable wraps an AbstractSwitchable& (the component it actually
 * turns on/off). It's already designed for substitution via that
 * reference, so no header changes were needed to test it in isolation --
 * a small test-local FakeSwitchable (below) tracks how many times it was
 * told to turn on/off, without touching any real pin.
 *
 * HOW TO RUN
 * ----------
 *     pio test -e nanoatmega328
 * Small suite, expected to fit the Nano directly; fall back to
 * -e megaatmega2560 if it doesn't.
 */

#include <Arduino.h>
#include <unity.h>

#include <TimedSwitchable.h>
#include <api/AbstractSwitchable.h>

using xal::AbstractSwitchable;
using xal::TimedSwitchable;

namespace {
    /**
     * @brief A minimal AbstractSwitchable that just counts calls, so tests
     * can confirm TimedSwitchable actually delegates to its wrapped
     * component -- not just that its own internal state changed.
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

    uint8_t onElapsedCount = 0;
    uint8_t offElapsedCount = 0;

    void onOnElapsed() {
        onElapsedCount++;
    }

    void onOffElapsed() {
        offElapsedCount++;
    }

    void resetCounters() {
        onElapsedCount = 0;
        offElapsedCount = 0;
    }
} // namespace

void test_fires_onTimeElapsed_and_turns_off_after_maxOnTime() {
    resetCounters();
    FakeSwitchable fake;
    TimedSwitchable ts(fake);
    ts.setMaxOnTimeMs(1000);
    ts.setOnTimeElapsedCallback(onOnElapsed);
    ts.setOffTimeElapsedCallback(onOffElapsed);

    ts.setOn(0); /* fake.onCount -> 1 */

    ts.process(500); /* not yet elapsed */
    TEST_ASSERT_TRUE(ts.isOn());
    TEST_ASSERT_EQUAL_UINT8(0, onElapsedCount);

    ts.process(1000); /* elapsed: turns off, fires onTimeElapsedCallback */
    TEST_ASSERT_TRUE(ts.isOff());
    TEST_ASSERT_EQUAL_UINT8(1, onElapsedCount);
    TEST_ASSERT_EQUAL_UINT8(0, offElapsedCount);
    TEST_ASSERT_EQUAL_UINT8(1, fake.offCount); /* confirms real delegation happened */
}

void test_fires_offTimeElapsed_and_turns_on_after_maxOffTime() {
    resetCounters();
    FakeSwitchable fake;
    TimedSwitchable ts(fake);
    ts.setMaxOffTimeMs(1000);
    ts.setOnTimeElapsedCallback(onOnElapsed);
    ts.setOffTimeElapsedCallback(onOffElapsed);

    ts.setOff(0); /* fake.offCount -> 1 */

    ts.process(500);
    TEST_ASSERT_TRUE(ts.isOff());
    TEST_ASSERT_EQUAL_UINT8(0, offElapsedCount);

    ts.process(1000); /* elapsed: turns on, fires offTimeElapsedCallback */
    TEST_ASSERT_TRUE(ts.isOn());
    TEST_ASSERT_EQUAL_UINT8(1, offElapsedCount);
    TEST_ASSERT_EQUAL_UINT8(0, onElapsedCount);
    TEST_ASSERT_EQUAL_UINT8(1, fake.onCount);
}

void test_zero_maxOnTime_never_autoElapses() {
    resetCounters();
    FakeSwitchable fake;
    TimedSwitchable ts(fake);
    /* maxOnTimeMs left at its default of 0 */
    ts.setOnTimeElapsedCallback(onOnElapsed);

    ts.setOn(0);
    ts.process(1000000); /* huge elapsed time */

    TEST_ASSERT_TRUE(ts.isOn());
    TEST_ASSERT_EQUAL_UINT8(0, onElapsedCount);
}

void setup() {
    delay(2000);

    UNITY_BEGIN();

    RUN_TEST(test_fires_onTimeElapsed_and_turns_off_after_maxOnTime);
    RUN_TEST(test_fires_offTimeElapsed_and_turns_on_after_maxOffTime);
    RUN_TEST(test_zero_maxOnTime_never_autoElapses);

    UNITY_END();
}

void loop() {
}
