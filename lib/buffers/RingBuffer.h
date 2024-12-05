#ifndef com_github_xhuli_arduino_lib_buffers_RingBuffer_H
#define com_github_xhuli_arduino_lib_buffers_RingBuffer_H
#pragma once

#include <Arduino.h>

namespace xal {

/**
 * @brief A class representing a ring buffer.
 *
 * This class provides functionality to store and retrieve data in a ring buffer.
 * The buffer is implemented as a fixed size array. When the buffer is full, the
 * oldest element is overwritten.
 * @tparam T The type of data to store in the buffer.
 * @tparam N The maximum number of elements that can be stored in the buffer.
 */
template <typename T, uint8_t N>
class RingBuffer {
private:
    T buffer[N]; /**< The buffer array. */
    uint8_t index = 0; /**< The index for the buffer. */
    uint8_t count = 0; /**< The number of elements in the buffer. */

public:
    /**
     * @brief Default constructor.
     */
    RingBuffer() = default;

    /**
     * @brief Default destructor.
     */
    ~RingBuffer() = default;

    /**
     * @brief Checks if the buffer is empty.
     * @return True if the buffer is empty, false otherwise.
     */
    bool isEmpty()
    {
        return count == 0;
    }

    /**
     * @brief Checks if the buffer is full.
     * @return True if the buffer is full, false otherwise.
     */
    bool isFull()
    {
        return count == N;
    }

    /**
     * @brief Gets the number of elements in the buffer.
     * @return The number of elements in the buffer.
     */
    uint8_t size()
    {
        return count;
    }

    /**
     * @brief Gets the maximum number of values that can be stored in the buffer.
     * @return The maximum number of values that can be stored in the buffer.
     */
    uint8_t capacity()
    {
        return N;
    }

    /**
     * @brief Adds an value to the buffer.
     * @param value The value to add.
     * @return True if the value was added, false otherwise.
     */
    bool push(T value)
    {
        buffer[index] = value;
        index = (index + 1) % N;

        if (!isFull()) {
            count++;
        }

        return true;
    }

    /**
     * @brief Gets the value at the specified index.
     * @param index The index of the value to get.
     * @return The value at the specified index.
     */
    T get(uint8_t index) const
    {
        if (index > count) {
            index = (index + 1) % N;
        }
        return buffer[index];
    }

    /**
     * @brief Fills the buffer and sets all values to the specified value.
     * @param value The value to set all elements to.
     */
    void fill(T value)
    {
        for (uint8_t i = 0; i < N; i++) {
            push(value);
        }
    }

    /**
     * @brief Clears the buffer.
     */
    void clear()
    {
        fill(T());
    }

    /**
     * @brief Gets the average of all values in the buffer.
     * @return The average of all values in the buffer.
     */
    T average() const
    {
        double sum = 0;
        for (uint8_t i = 0; i < count; i++) {
            sum += get(i);
        }

        return static_cast<T>(round(sum / count));
    }
};

} /* namespace xal */

#endif
