/**
 * Regression tests for xal::PushButton, specifically for the ring-buffer
 * debounce added to process() (the logic extracted from loop() so it can
 * be driven directly here without real hardware).
 *
 * WHY process() INSTEAD OF loop()
 * --------------------------------
 * loop() calls digitalRead(pin) and millis() directly, so it can't be
 * driven with a controlled, repeatable input sequence without either
 * wiring a second pin as a fake signal generator, or extracting the actual
 * logic into a method that takes explicit inputs. process(rawReading, nowMs)
 * is that extraction — these tests call it directly with fabricated
 * readings and timestamps, so no jumper wire or physical setup is needed.
 *
 * WHAT'S BEING VERIFIED
 * ----------------------
 * - Short vs. long press classification (unchanged logic, still correct).
 * - A press held shorter than debounceMs is silently ignored (unchanged).
 * - The NEW behavior this change added: a single noisy/minority reading no
 *   longer flips the debounced state at all, unlike the old raw
 *   digitalRead()-per-loop() implementation, which reacted to any single
 *   raw change immediately.
 *
 * HOW TO RUN
 * ----------
 *     pio test -e nanoatmega328
 * This suite is small (one PushButton object per test, no AtoActions
 * instances), so it's expected to fit the Nano's 2KB SRAM directly. If it
 * doesn't, fall back to -e megaatmega2560 as with the other suites.
 */

#include <Arduino.h>
#include <unity.h>

#include <PushButton.h>

using xal::PushButton;

namespace {
    /* Matches the actual production constants in main.cpp
       (PUSH_BUTTON_PIN_STATE_WHEN_RELEASED, PUSH_BUTTON_DEBOUNCE_MS,
       PUSH_BUTTON_LONG_PRESS_DURATION) so these tests exercise realistic
       timing, not arbitrary numbers. */
    constexpr uint8_t PIN_STATE_WHEN_RELEASED = LOW;
    constexpr uint16_t DEBOUNCE_MS = 160;
    constexpr uint16_t LONG_PRESS_MS = 3000;

    uint8_t shortPressCount = 0;
    uint8_t longPressCount = 0;

    void onShortPress() {
        shortPressCount++;
    }

    void onLongPress() {
        longPressCount++;
    }

    void resetCounters() {
        shortPressCount = 0;
        longPressCount = 0;
    }

    /**
     * @brief Wires callbacks on a button constructed directly in its final
     * storage location.
     */
    void wireButton(PushButton &button) {
        button.setShortPressCallback(onShortPress);
        button.setLongPressCallback(onLongPress);
    }

    /**
     * @brief Calls process(level, atMs) `times` times in a row, simulating
     * that many consecutive loop() iterations reading the same raw level.
     */
    void feed(PushButton &button, uint8_t level, uint8_t times, uint32_t atMs) {
        for (uint8_t i = 0; i < times; i++) {
            button.process(level, atMs);
        }
    }
} // namespace

/* ============================================================ */
/* Debounce smoothing: the actual behavior this change added.    */
/* ============================================================ */

void test_single_noise_spike_does_not_register_as_pressed() {
    resetCounters();
    PushButton button(/* pin */ 2, INPUT, PIN_STATE_WHEN_RELEASED, DEBOUNCE_MS, LONG_PRESS_MS);
    wireButton(button);

    /* One glitchy HIGH reading among 15 still-LOW buffered samples: a
       clear minority, should not flip the debounced state at all. The old
       raw digitalRead()-per-loop() implementation would have reacted to
       this single reading immediately. */
    button.process(HIGH, 0);

    TEST_ASSERT_FALSE(button.isPressed());
    TEST_ASSERT_EQUAL_UINT8(0, shortPressCount);
    TEST_ASSERT_EQUAL_UINT8(0, longPressCount);
}

void test_minority_noise_burst_does_not_flip_debounced_state() {
    resetCounters();
    PushButton button(/* pin */ 2, INPUT, PIN_STATE_WHEN_RELEASED, DEBOUNCE_MS, LONG_PRESS_MS);
    wireButton(button);

    /* 6 HIGH readings against 10 remaining LOW samples in the 16-window:
       still a clear minority (sum=6, round(6/16)=0), so the debounced
       state must stay released. */
    feed(button, HIGH, 6, 0);

    TEST_ASSERT_FALSE(button.isPressed());
    TEST_ASSERT_EQUAL_UINT8(0, shortPressCount);
    TEST_ASSERT_EQUAL_UINT8(0, longPressCount);
}

void test_sustained_signal_does_flip_debounced_state() {
    resetCounters();
    PushButton button(/* pin */ 2, INPUT, PIN_STATE_WHEN_RELEASED, DEBOUNCE_MS, LONG_PRESS_MS);
    wireButton(button);

    /* A full window of consistent HIGH readings is a clear, unambiguous
       majority and must register as pressed (no callback fires on the
       press edge itself -- only on release). */
    feed(button, HIGH, 16, 0);

    TEST_ASSERT_TRUE(button.isPressed());
    TEST_ASSERT_EQUAL_UINT8(0, shortPressCount);
    TEST_ASSERT_EQUAL_UINT8(0, longPressCount);
}

/* ============================================================ */
/* Short / long press classification: unchanged logic, still     */
/* correct after the debounce refactor.                          */
/* ============================================================ */

void test_short_press_fires_short_callback() {
    resetCounters();
    PushButton button(/* pin */ 2, INPUT, PIN_STATE_WHEN_RELEASED, DEBOUNCE_MS, LONG_PRESS_MS);
    wireButton(button);

    feed(button, HIGH, 16, 0);  /* press at t=0 */
    feed(button, LOW, 16, 500); /* release at t=500 (> debounce, < long-press) */

    TEST_ASSERT_FALSE(button.isPressed());
    TEST_ASSERT_EQUAL_UINT8(1, shortPressCount);
    TEST_ASSERT_EQUAL_UINT8(0, longPressCount);
}

void test_long_press_fires_long_callback() {
    resetCounters();
    PushButton button(/* pin */ 2, INPUT, PIN_STATE_WHEN_RELEASED, DEBOUNCE_MS, LONG_PRESS_MS);
    wireButton(button);

    feed(button, HIGH, 16, 0);   /* press at t=0 */
    feed(button, LOW, 16, 4000); /* release at t=4000 (> long-press threshold) */

    TEST_ASSERT_FALSE(button.isPressed());
    TEST_ASSERT_EQUAL_UINT8(0, shortPressCount);
    TEST_ASSERT_EQUAL_UINT8(1, longPressCount);
}

void test_press_shorter_than_debounce_fires_no_callback() {
    resetCounters();
    PushButton button(/* pin */ 2, INPUT, PIN_STATE_WHEN_RELEASED, DEBOUNCE_MS, LONG_PRESS_MS);
    wireButton(button);

    feed(button, HIGH, 16, 0);  /* press at t=0 */
    feed(button, LOW, 16, 100); /* release at t=100 (<= debounce of 160: too short, ignored) */

    /* The debounced state still updates to released even though no
       callback fires -- matches the original behavior of updating
       previousState regardless of whether the duration check passes. */
    TEST_ASSERT_FALSE(button.isPressed());
    TEST_ASSERT_EQUAL_UINT8(0, shortPressCount);
    TEST_ASSERT_EQUAL_UINT8(0, longPressCount);
}

/* ============================================================ */
/* Unity runner                                                   */
/* ============================================================ */

void setup() {
    delay(2000);

    UNITY_BEGIN();

    RUN_TEST(test_single_noise_spike_does_not_register_as_pressed);
    RUN_TEST(test_minority_noise_burst_does_not_flip_debounced_state);
    RUN_TEST(test_sustained_signal_does_flip_debounced_state);
    RUN_TEST(test_short_press_fires_short_callback);
    RUN_TEST(test_long_press_fires_long_callback);
    RUN_TEST(test_press_shorter_than_debounce_fires_no_callback);

    UNITY_END();
}

void loop() {
}
