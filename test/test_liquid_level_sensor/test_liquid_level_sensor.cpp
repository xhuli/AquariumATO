/**
 * Regression tests for xal::LiquidLevelSensor, driven via the process()
 * method extracted from loop() (see LiquidLevelSensor.h for why: loop()
 * calls digitalRead()/millis() directly, so process(rawReading, nowMs) is
 * the seam that lets these tests run with fabricated inputs and no
 * hardware/wiring, exactly like test_push_button.cpp).
 *
 * WHAT'S BEING VERIFIED
 * ----------------------
 * - Ring-buffer debounce: a single noisy reading or a minority noise burst
 *   must not flip the debounced state (this sensor is the original source
 *   of the RingBuffer-averaging pattern later applied to PushButton).
 * - A sustained new reading correctly flips state and fires the matching
 *   callback (isTriggered vs. notTriggered, per pinStateWhenLiquidIsPresent).
 * - The periodic re-push behavior (pushReadingToCallbackMs): no re-fire
 *   before the interval elapses, a re-fire once it does, and re-push
 *   disabled entirely when the interval is 0.
 *
 * HOW TO RUN
 * ----------
 *     pio test -e nanoatmega328
 * Small suite (no AtoActions instances), expected to fit the Nano directly;
 * fall back to -e megaatmega2560 if it doesn't.
 */

#include <Arduino.h>
#include <unity.h>

#include <LiquidLevelSensor.h>

using xal::LiquidLevelSensor;

namespace
{
    /* Matches main.cpp's actual normalLevelSensor wiring
       (WHEN_ON__PIN_HIGH, PERIODIC_PUSH_READING_PERIOD) so these tests
       exercise realistic values, not arbitrary numbers. */
    constexpr uint8_t PIN_STATE_WHEN_PRESENT = HIGH;
    constexpr uint32_t PUSH_INTERVAL_MS = 2000;

    uint8_t triggeredCount = 0;
    uint8_t notTriggeredCount = 0;

    void onTriggered()
    {
        triggeredCount++;
    }

    void onNotTriggered()
    {
        notTriggeredCount++;
    }

    void resetCounters()
    {
        triggeredCount = 0;
        notTriggeredCount = 0;
    }

    /**
     * @brief Wires callbacks on a sensor constructed directly in its final
     * storage location.
     */
    void wireSensor(LiquidLevelSensor &sensor)
    {
        sensor.setIsTriggeredCallback(onTriggered);
        sensor.setNotTriggeredCallback(onNotTriggered);
    }

    /**
     * @brief Calls process(level, atMs) `times` times in a row, simulating
     * that many consecutive loop() iterations reading the same raw level.
     */
    void feed(LiquidLevelSensor &sensor, uint8_t level, uint8_t times, uint32_t atMs)
    {
        for (uint8_t i = 0; i < times; i++)
        {
            sensor.process(level, atMs);
        }
    }
} // namespace

/* ============================================================ */
/* Ring-buffer debounce smoothing.                                */
/* ============================================================ */

void test_single_noise_spike_does_not_change_debounced_state()
{
    resetCounters();
    LiquidLevelSensor sensor(/* pin */ 2, PIN_STATE_WHEN_PRESENT, PUSH_INTERVAL_MS, LOW);
    wireSensor(sensor);

    /* One glitchy HIGH reading among 15 still-LOW buffered samples: a
       clear minority, must not flip the debounced state. */
    sensor.process(HIGH, 0);

    TEST_ASSERT_EQUAL_UINT8(LOW, sensor.getLastState());
    TEST_ASSERT_EQUAL_UINT8(0, triggeredCount);
    TEST_ASSERT_EQUAL_UINT8(0, notTriggeredCount);
}

void test_minority_noise_burst_does_not_flip_debounced_state()
{
    resetCounters();
    LiquidLevelSensor sensor(/* pin */ 2, PIN_STATE_WHEN_PRESENT, PUSH_INTERVAL_MS, LOW);
    wireSensor(sensor);

    /* 6 HIGH readings against 10 remaining LOW samples in the 16-window:
       still a clear minority (sum=6, round(6/16)=0). */
    feed(sensor, HIGH, 6, 0);

    TEST_ASSERT_EQUAL_UINT8(LOW, sensor.getLastState());
    TEST_ASSERT_EQUAL_UINT8(0, triggeredCount);
    TEST_ASSERT_EQUAL_UINT8(0, notTriggeredCount);
}

void test_sustained_new_reading_fires_correct_callback()
{
    resetCounters();
    LiquidLevelSensor sensor(/* pin */ 2, PIN_STATE_WHEN_PRESENT, PUSH_INTERVAL_MS, LOW);
    wireSensor(sensor);

    /* A full window of consistent HIGH readings is a clear majority and
       must fire isTriggeredCallback, since PIN_STATE_WHEN_PRESENT == HIGH. */
    feed(sensor, HIGH, 16, 0);

    TEST_ASSERT_EQUAL_UINT8(HIGH, sensor.getLastState());
    TEST_ASSERT_EQUAL_UINT8(1, triggeredCount);
    TEST_ASSERT_EQUAL_UINT8(0, notTriggeredCount);
}

/* ============================================================ */
/* Periodic re-push (pushReadingToCallbackMs) timing.             */
/* ============================================================ */

void test_state_unchanged_before_interval_does_not_refire()
{
    resetCounters();
    LiquidLevelSensor sensor(/* pin */ 2, PIN_STATE_WHEN_PRESENT, PUSH_INTERVAL_MS, LOW);
    wireSensor(sensor);

    feed(sensor, HIGH, 16, 0); /* stabilizes at HIGH, fires once at t=0 */
    resetCounters();           /* isolate the check below from the stabilization fire */

    /* Same state, well before the 2000ms interval has elapsed: must not refire. */
    sensor.process(HIGH, 1000);

    TEST_ASSERT_EQUAL_UINT8(0, triggeredCount);
    TEST_ASSERT_EQUAL_UINT8(0, notTriggeredCount);
}

void test_periodic_repush_fires_after_interval_elapses()
{
    resetCounters();
    LiquidLevelSensor sensor(/* pin */ 2, PIN_STATE_WHEN_PRESENT, PUSH_INTERVAL_MS, LOW);
    wireSensor(sensor);

    feed(sensor, HIGH, 16, 0); /* stabilizes at HIGH, fires once at t=0 */
    resetCounters();

    /* Past the 2000ms interval since the last fire: must refire even
       though the debounced state itself hasn't changed. */
    sensor.process(HIGH, 2500);

    TEST_ASSERT_EQUAL_UINT8(1, triggeredCount);
    TEST_ASSERT_EQUAL_UINT8(0, notTriggeredCount);
}

void test_periodic_repush_disabled_when_interval_is_zero()
{
    resetCounters();
    LiquidLevelSensor sensor(/* pin */ 2, PIN_STATE_WHEN_PRESENT, /* pushIntervalMs */ 0, LOW);
    wireSensor(sensor);

    feed(sensor, HIGH, 16, 0); /* stabilizes at HIGH, fires once at t=0 */
    resetCounters();

    /* pushReadingToCallbackMs == 0 disables periodic re-push entirely,
       regardless of how much time has passed. */
    sensor.process(HIGH, 1000000);

    TEST_ASSERT_EQUAL_UINT8(0, triggeredCount);
    TEST_ASSERT_EQUAL_UINT8(0, notTriggeredCount);
}

/* ============================================================ */
/* isTriggered()/isNotTriggered() pure helpers.                   */
/* ============================================================ */

void test_isTriggered_and_isNotTriggered_helpers()
{
    LiquidLevelSensor sensor(/* pin */ 2, PIN_STATE_WHEN_PRESENT, PUSH_INTERVAL_MS, LOW);
    wireSensor(sensor);

    TEST_ASSERT_TRUE(sensor.isTriggered(HIGH));
    TEST_ASSERT_FALSE(sensor.isTriggered(LOW));
    TEST_ASSERT_TRUE(sensor.isNotTriggered(LOW));
    TEST_ASSERT_FALSE(sensor.isNotTriggered(HIGH));
}

/* ============================================================ */
/* Unity runner                                                   */
/* ============================================================ */

void setup()
{
    delay(2000);

    UNITY_BEGIN();

    RUN_TEST(test_single_noise_spike_does_not_change_debounced_state);
    RUN_TEST(test_minority_noise_burst_does_not_flip_debounced_state);
    RUN_TEST(test_sustained_new_reading_fires_correct_callback);
    RUN_TEST(test_state_unchanged_before_interval_does_not_refire);
    RUN_TEST(test_periodic_repush_fires_after_interval_elapses);
    RUN_TEST(test_periodic_repush_disabled_when_interval_is_zero);
    RUN_TEST(test_isTriggered_and_isNotTriggered_helpers);

    UNITY_END();
}

void loop()
{
}
