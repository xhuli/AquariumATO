#ifndef com_github_xhuli_arduino_lib_sensors_LiquidLevelSensor_H
#define com_github_xhuli_arduino_lib_sensors_LiquidLevelSensor_H
#pragma once

#include <Arduino.h>
#include <RingBuffer.h>
#include <Runnable.h>

namespace xal {

    /**
     * @brief A class representing a liquid level sensor.
     *
     * This class provides functionality to read the state of a liquid level sensor connected to an Arduino board.
     * The state of the liquid level sensor is either HIGH or LOW.
     * The state of the liquid level sensor is pushed to a callback function periodically.
     * If the pushReadingToCallbackMs is set to 0, the callback function is not called periodically.
     * If the state of the liquid level sensor changes, the callback function is called immediately.
     * The pin state is pushed to a buffer and the average of the buffer is used as the current state of the liquid level sensor.
     * @implements Runnable
     */
    class LiquidLevelSensor : public Runnable {
        using Callback = void (*)();

    private:
        const uint8_t pin;                         /**< The pin number of the liquid level sensor. */
        const uint8_t pinStateWhenLiquidIsPresent; /**< The state of the liquid level sensor when liquid is present. Either HIGH or LOW. */
        const uint32_t pushReadingToCallbackMs;    /**< The duration in milliseconds to periodically push the current state to the callback function. */
        uint32_t lastCallbackPushReadingMs = 0;    /**< The last time in milliseconds when the callback function was called. */

        RingBuffer<uint8_t, 16> buffer; /**< The buffer to store the states of the liquid level sensor. */
        uint8_t lastState;              /**< The last debounced state that was pushed to a callback. */

        Callback isTriggeredCallback = nullptr;  /**< The callback function to be called when the liquid level sensor is triggered. */
        Callback notTriggeredCallback = nullptr; /**< The callback function to be called when the liquid level sensor is not triggered. */

        /**
         * @brief Checks whether durationMs has elapsed since lastCallbackPushReadingMs,
         * relative to the given current time. Returns false unconditionally when
         * periodic pushing is disabled (pushReadingToCallbackMs == 0).
         */
        bool hasElapsed(uint32_t nowMs, uint32_t durationMs) const {
            if (pushReadingToCallbackMs > 0) {
                return (nowMs - lastCallbackPushReadingMs >= durationMs);
            }
            return false;
        }

    public:
        /**
         * @brief Constructs a LiquidLevelSensor object.
         * @param pin The pin number of the liquid level sensor.
         * @param pinStateWhenLiquidIsPresent The state of the liquid level sensor when liquid is present. Either HIGH or LOW.
         */
        explicit LiquidLevelSensor(
            uint8_t pin,
            uint8_t pinStateWhenLiquidIsPresent,
            uint32_t pushReadingToCallbackMs = 12000,
            uint8_t initialReading = HIGH)
            : pin(pin),
              pinStateWhenLiquidIsPresent(pinStateWhenLiquidIsPresent),
              pushReadingToCallbackMs(pushReadingToCallbackMs),
              lastState(initialReading) {
            buffer.fill(initialReading);
        }

        /**
         * @brief Default destructor.
         */
        ~LiquidLevelSensor() override = default;

        /**
         * @brief Sets the callback function to be called when the liquid level sensor is triggered.
         * @param callback The callback function to be called when the liquid level sensor is triggered.
         */
        void setIsTriggeredCallback(Callback callback) {
            isTriggeredCallback = callback;
        }

        /**
         * @brief Sets the callback function to be called when the liquid level sensor is not triggered.
         * @param callback The callback function to be called when the liquid level sensor is not triggered.
         */
        void setNotTriggeredCallback(Callback callback) {
            notTriggeredCallback = callback;
        }

        /**
         * @brief Gets whether the given (already-debounced) state counts as triggered.
         * @return true if the liquid level sensor is triggered, false otherwise.
         */
        bool isTriggered(uint8_t state) const {
            return (state == pinStateWhenLiquidIsPresent);
        }

        /**
         * @brief Gets whether the given (already-debounced) state counts as not triggered.
         * @return true if the liquid level sensor is not triggered, false otherwise.
         */
        bool isNotTriggered(uint8_t state) const {
            return (state != pinStateWhenLiquidIsPresent);
        }

        /**
         * @brief Returns the last debounced state that was pushed to a callback
         * (HIGH or LOW). Read-only accessor added for testability.
         */
        uint8_t getLastState() const {
            return lastState;
        }

        /**
         * @brief Sets up the liquid level sensor.
         * @details This function sets the pin mode of the liquid level sensor to INPUT.
         */
        void setup() override {
            pinMode(pin, INPUT);
        }

        /**
         * @brief Debounces rawReading via ring-buffer averaging and fires the
         * appropriate callback if the debounced state changed, or if
         * pushReadingToCallbackMs has elapsed since the last callback fire.
         * Extracted from loop() so it can be driven directly with fabricated
         * readings/timestamps in tests, with no hardware or wiring required.
         * @param rawReading The raw (un-debounced) pin reading to push into
         * the debounce buffer this call.
         * @param nowMs The current time in milliseconds (normally millis()).
         */
        void process(uint8_t rawReading, uint32_t nowMs) {
            buffer.push(rawReading);
            uint8_t state = buffer.average();

            if ((lastState != state) || hasElapsed(nowMs, pushReadingToCallbackMs)) {
                lastState = state;

                if (isTriggered(state)) {
                    if (isTriggeredCallback != nullptr) {
                        isTriggeredCallback();
                    }
                } else {
                    if (notTriggeredCallback != nullptr) {
                        notTriggeredCallback();
                    }
                }

                lastCallbackPushReadingMs = nowMs;
            }
        }

        /**
         * @brief Called in the main Arduino loop function.
         * @details This function calls the isTriggeredCallback if the liquid level sensor is triggered,
         * otherwise it calls the notTriggeredCallback.
         */
        void loop() override {
            process(digitalRead(pin), millis());
        }
    };

} /* namespace xal */

#endif
