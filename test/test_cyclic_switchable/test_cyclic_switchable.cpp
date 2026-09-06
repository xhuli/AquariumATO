/**
 * Regression tests for xal::CyclicSwitchable, driven via process(nowMs) and
 * the nowMs-injectable setOn(uint32_t)/setCycleArray(..., uint32_t)
 * overloads extracted from loop()/setOn()/setCycleArray() (see
 * CyclicSwitchable.h for why: those calls read millis() directly).
 *
 * Uses the same FakeSwitchable pattern as test_timed_switchable.cpp, but
 * also overrides setState() specifically, since CyclicSwitchable's
 * interval-cycling logic calls switchable.setState(...) directly rather
 * than setOn()/setOff().
 *
 * HOW TO RUN
 * ----------
 *     pio test -e nanoatmega328
 * Small suite, expected to fit the Nano directly; fall back to
 * -e megaatmega2560 if it doesn't.
 */

#include <Arduino.h>
#include <unity.h>

#include <CyclicSwitchable.h>
#include <api/AbstractSwitchable.h>
#include <enums/SwitchState.h>

using xal::AbstractSwitchable;
using xal::CyclicSwitchable;
using xal::SwitchState;

namespace {
    /**
     * @brief A minimal AbstractSwitchable that counts calls, tracking
     * setOn()/setOff() and setState() separately, since
     * CyclicSwitchable's interval-cycling logic uses setState()
     * specifically, not setOn()/setOff().
     *
     * NOTE: AbstractSwitchable::setOn()/setOff() are themselves
     * implemented in terms of setState() (setOn() calls
     * setState(On), etc.), so calling fake.setOn()/fake.setOff() here
     * ALSO increments setStateCount, via virtual dispatch into this
     * override. Tests that care about interval-driven setState() calls
     * specifically must reset setStateCount to 0 after any setup that
     * calls setOn()/setOff()/setCycleArray(), to isolate it from these
     * incidental calls.
     */
    class FakeSwitchable : public AbstractSwitchable {
    public:
        uint8_t onCount = 0;
        uint8_t offCount = 0;
        uint8_t setStateCount = 0;

        void setOn() override {
            AbstractSwitchable::setOn();
            onCount++;
        }

        void setOff() override {
            AbstractSwitchable::setOff();
            offCount++;
        }

        void setState(SwitchState state) override {
            AbstractSwitchable::setState(state);
            setStateCount++;
        }
    };

    /* Arbitrary but distinct interval durations (ms), matching the
       real project's "alternating on/off durations" pattern convention. */
    constexpr uint32_t testPattern[4] = {100, 200, 300, 400};
} // namespace

void test_setOn_starts_pattern_and_switches_fake_on() {
    FakeSwitchable fake;
    CyclicSwitchable cyclic(fake);

    /* Cyclic starts Off, so setCycleArray() takes the isOff() branch here
       (switchable.setOff() -> fake.offCount == 1) before setOn() flips it. */
    cyclic.setCycleArray(4, testPattern, 0);
    cyclic.setOn(0);

    TEST_ASSERT_TRUE(cyclic.isOn());
    TEST_ASSERT_EQUAL_UINT8(1, fake.onCount);
    TEST_ASSERT_EQUAL_UINT8(1, fake.offCount);
}

void test_advances_through_pattern_at_each_interval() {
    FakeSwitchable fake;
    CyclicSwitchable cyclic(fake);
    cyclic.setCycleArray(4, testPattern, 0);
    cyclic.setOn(0);

    /* setCycleArray()'s off-branch and setOn() each funnel through
       AbstractSwitchable::setOn()/setOff(), which themselves call
       setState() internally -- so setStateCount already includes 2
       incidental calls from setup. Reset here to isolate only the
       interval-driven advances this test actually cares about. */
    fake.setStateCount = 0;

    cyclic.process(50); /* testPattern[0] == 100, not yet elapsed */
    TEST_ASSERT_EQUAL_UINT8(0, fake.setStateCount);

    cyclic.process(100); /* elapsed: advances to index 1 */
    TEST_ASSERT_EQUAL_UINT8(1, fake.setStateCount);

    cyclic.process(250); /* testPattern[1] == 200; only 150ms since last advance */
    TEST_ASSERT_EQUAL_UINT8(1, fake.setStateCount);

    cyclic.process(300); /* 200ms since last advance: elapsed, advances to index 2 */
    TEST_ASSERT_EQUAL_UINT8(2, fake.setStateCount);
}

void test_wraps_around_after_last_interval() {
    FakeSwitchable fake;
    CyclicSwitchable cyclic(fake);
    cyclic.setCycleArray(4, testPattern, 0);
    cyclic.setOn(0);
    fake.setStateCount = 0; /* isolate from setup's incidental setState() calls, as above */

    cyclic.process(100);  /* -> index 1 (interval 0 == 100) */
    cyclic.process(300);  /* -> index 2 (interval 1 == 200) */
    cyclic.process(600);  /* -> index 3 (interval 2 == 300) */
    cyclic.process(1000); /* -> index 0, wraps around (interval 3 == 400) */

    TEST_ASSERT_EQUAL_UINT8(4, fake.setStateCount);
    /* Index 0 is even -> the wrapped-to state should be On. */
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SwitchState::On), static_cast<int>(fake.getState()));
}

void test_does_nothing_while_off() {
    FakeSwitchable fake;
    CyclicSwitchable cyclic(fake);
    cyclic.setCycleArray(4, testPattern, 0); /* cyclic never turned on */
    fake.setStateCount = 0;                  /* isolate from setCycleArray()'s own incidental setState() call */

    cyclic.process(100000);

    TEST_ASSERT_TRUE(cyclic.isOff());
    TEST_ASSERT_EQUAL_UINT8(0, fake.setStateCount);
}

void setup() {
    delay(2000);

    UNITY_BEGIN();

    RUN_TEST(test_setOn_starts_pattern_and_switches_fake_on);
    RUN_TEST(test_advances_through_pattern_at_each_interval);
    RUN_TEST(test_wraps_around_after_last_interval);
    RUN_TEST(test_does_nothing_while_off);

    UNITY_END();
}

void loop() {
}
