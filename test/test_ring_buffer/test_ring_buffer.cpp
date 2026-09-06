/**
 * Regression tests for xal::RingBuffer<T, N>, specifically for the get()
 * logical-index -> physical-index mapping bug fix.
 *
 * BACKGROUND
 * ----------
 * The original get(uint8_t index) had two bugs: (1) its parameter shadowed
 * the class's write-cursor member of the same name, so the "adjustment"
 * logic never actually consulted real buffer state; (2) even ignoring the
 * shadowing, the adjustment didn't correctly map a logical position to a
 * physical one once the buffer had wrapped around at least once.
 *
 * average() never surfaced this because summing get(0)..get(count-1) is
 * order-independent, and those calls happened to enumerate every valid
 * physical slot exactly once (whether full or partially filled) — just not
 * necessarily in chronological (oldest-to-newest) order. These tests check
 * get() directly, in both the not-yet-full and wrapped-around cases, plus
 * confirm average() remains correct after the fix.
 *
 * HOW TO RUN
 * ----------
 *     pio test -e nanoatmega328
 * (or -e megaatmega2560, if the test binary doesn't fit the Nano's 2KB SRAM
 * alongside other test suites in the same run — this suite alone is small
 * and likely fits the Nano fine on its own).
 */

#include <Arduino.h>
#include <unity.h>

#include <RingBuffer.h>

using xal::RingBuffer;

/* ============================================================ */
/* Empty-state and clear() semantics                            */
/* ============================================================ */

void test_fresh_buffer_is_empty_and_average_is_default_value() {
    RingBuffer<uint8_t, 4> rb;

    TEST_ASSERT_TRUE(rb.isEmpty());
    TEST_ASSERT_FALSE(rb.isFull());
    TEST_ASSERT_EQUAL_UINT8(0, rb.size());
    TEST_ASSERT_EQUAL_UINT8(0, rb.average());
}

void test_clear_makes_buffer_logically_empty() {
    RingBuffer<uint8_t, 4> rb;
    rb.push(10);
    rb.push(20);
    rb.push(30);

    rb.clear();

    TEST_ASSERT_TRUE(rb.isEmpty());
    TEST_ASSERT_FALSE(rb.isFull());
    TEST_ASSERT_EQUAL_UINT8(0, rb.size());
    TEST_ASSERT_EQUAL_UINT8(0, rb.average());
}

void test_push_after_clear_behaves_like_fresh_buffer() {
    RingBuffer<uint8_t, 4> rb;
    rb.fill(99);
    rb.clear();

    rb.push(7);

    TEST_ASSERT_FALSE(rb.isEmpty());
    TEST_ASSERT_EQUAL_UINT8(1, rb.size());
    TEST_ASSERT_EQUAL_UINT8(7, rb.get(0));
    TEST_ASSERT_EQUAL_UINT8(7, rb.average());

    rb.push(9);
    TEST_ASSERT_EQUAL_UINT8(2, rb.size());
    TEST_ASSERT_EQUAL_UINT8(7, rb.get(0));
    TEST_ASSERT_EQUAL_UINT8(9, rb.get(1));
    TEST_ASSERT_EQUAL_UINT8(8, rb.average());
}

/* ============================================================ */
/* Not-yet-full buffer: physical position == logical position   */
/* since no wraparound has occurred yet.                        */
/* ============================================================ */

void test_partial_fill_get_returns_values_in_push_order() {
    RingBuffer<uint8_t, 4> rb;
    rb.push(1);
    rb.push(2);

    TEST_ASSERT_EQUAL_UINT8(2, rb.size());
    TEST_ASSERT_EQUAL_UINT8(1, rb.get(0)); /* oldest */
    TEST_ASSERT_EQUAL_UINT8(2, rb.get(1)); /* newest */
}

void test_partial_fill_average_is_correct() {
    RingBuffer<uint8_t, 4> rb;
    rb.push(1);
    rb.push(2);

    /* (1 + 2) / 2 = 1.5, rounds to 2. */
    TEST_ASSERT_EQUAL_UINT8(2, rb.average());
}

/* ============================================================ */
/* Full buffer, no wraparound yet: still starts at physical 0.   */
/* ============================================================ */

void test_exactly_full_get_returns_values_in_push_order() {
    RingBuffer<uint8_t, 4> rb;
    rb.push(10);
    rb.push(20);
    rb.push(30);
    rb.push(40);

    TEST_ASSERT_TRUE(rb.isFull());
    TEST_ASSERT_EQUAL_UINT8(10, rb.get(0));
    TEST_ASSERT_EQUAL_UINT8(20, rb.get(1));
    TEST_ASSERT_EQUAL_UINT8(30, rb.get(2));
    TEST_ASSERT_EQUAL_UINT8(40, rb.get(3));
}

/* ============================================================ */
/* Wrapped around: this is the case the original bug broke.     */
/* After more than N pushes, get(0) must still be the OLDEST    */
/* surviving value and get(size()-1) the newest — not whatever  */
/* happens to sit at raw physical index 0..N-1.                 */
/* ============================================================ */

void test_wrapped_get_returns_oldest_to_newest_order() {
    RingBuffer<uint8_t, 4> rb;
    /* First 4 pushes fill the buffer; these get evicted below. */
    rb.push(1);
    rb.push(2);
    rb.push(3);
    rb.push(4);
    /* These 4 pushes wrap around, evicting the original 4. */
    rb.push(10);
    rb.push(20);
    rb.push(30);
    rb.push(40);

    TEST_ASSERT_TRUE(rb.isFull());
    TEST_ASSERT_EQUAL_UINT8(4, rb.size());
    TEST_ASSERT_EQUAL_UINT8(10, rb.get(0)); /* oldest surviving value */
    TEST_ASSERT_EQUAL_UINT8(20, rb.get(1));
    TEST_ASSERT_EQUAL_UINT8(30, rb.get(2));
    TEST_ASSERT_EQUAL_UINT8(40, rb.get(3)); /* newest value */
}

void test_wrapped_average_is_correct() {
    RingBuffer<uint8_t, 4> rb;
    rb.push(1);
    rb.push(2);
    rb.push(3);
    rb.push(4);
    rb.push(10);
    rb.push(20);
    rb.push(30);
    rb.push(40);

    /* (10 + 20 + 30 + 40) / 4 = 25 exactly. */
    TEST_ASSERT_EQUAL_UINT8(25, rb.average());
}

void test_wrapped_multiple_times_still_correct() {
    RingBuffer<uint8_t, 4> rb;
    /* Push 10 values (more than 2x capacity) through a size-4 buffer;
       only the last 4 should remain: 70, 80, 90, 100. */
    for (uint8_t i = 1; i <= 10; i++) {
        rb.push(i * 10);
    }

    TEST_ASSERT_EQUAL_UINT8(70, rb.get(0));
    TEST_ASSERT_EQUAL_UINT8(80, rb.get(1));
    TEST_ASSERT_EQUAL_UINT8(90, rb.get(2));
    TEST_ASSERT_EQUAL_UINT8(100, rb.get(3));
    TEST_ASSERT_EQUAL_UINT8(85, rb.average()); /* (70+80+90+100)/4 = 85 */
}

/* ============================================================ */
/* fill()/clear() sanity checks, since fill() is push()-based    */
/* and should interact correctly with the fixed get().           */
/* ============================================================ */

void test_fill_sets_every_slot_to_same_value() {
    RingBuffer<uint8_t, 4> rb;
    rb.fill(7);

    TEST_ASSERT_TRUE(rb.isFull());
    TEST_ASSERT_EQUAL_UINT8(7, rb.get(0));
    TEST_ASSERT_EQUAL_UINT8(7, rb.get(1));
    TEST_ASSERT_EQUAL_UINT8(7, rb.get(2));
    TEST_ASSERT_EQUAL_UINT8(7, rb.get(3));
    TEST_ASSERT_EQUAL_UINT8(7, rb.average());
}

/* ============================================================ */
/* Unity runner                                                  */
/* ============================================================ */

void setup() {
    delay(2000);

    UNITY_BEGIN();

    RUN_TEST(test_fresh_buffer_is_empty_and_average_is_default_value);
    RUN_TEST(test_clear_makes_buffer_logically_empty);
    RUN_TEST(test_push_after_clear_behaves_like_fresh_buffer);
    RUN_TEST(test_partial_fill_get_returns_values_in_push_order);
    RUN_TEST(test_partial_fill_average_is_correct);
    RUN_TEST(test_exactly_full_get_returns_values_in_push_order);
    RUN_TEST(test_wrapped_get_returns_oldest_to_newest_order);
    RUN_TEST(test_wrapped_average_is_correct);
    RUN_TEST(test_wrapped_multiple_times_still_correct);
    RUN_TEST(test_fill_sets_every_slot_to_same_value);

    UNITY_END();
}

void loop() {
}
