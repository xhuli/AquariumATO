#ifndef com_github_xhuli_arduino_lib_sensors_LiquidLevelSensor_H
#define com_github_xhuli_arduino_lib_sensors_LiquidLevelSensor_H
#pragma once

#include <Arduino.h>
// #include <ArduinoLog.h>
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
    typedef void (*Callback)();

private:
    const uint8_t pin; /**< The pin number of the liquid level sensor. */
    const uint8_t pinStateWhenLiquidIsPresent; /**< The state of the liquid level sensor when liquid is present. Either HIGH or LOW. */
    const uint32_t pushReadingToCallbackMs; /**< The duration in milliseconds to periodically push the current state to the callback function. */
    uint32_t lastCallbackPushReadingMs = 0; /**< The last time in milliseconds when the callback function was called. */

    RingBuffer<uint8_t, 16> buffer; /**< The buffer to store the states of the liquid level sensor. */
    uint8_t lastState; /**< The last state of the liquid level sensor. */

    Callback isTriggeredCallback; /**< The callback function to be called when the liquid level sensor is triggered. */
    Callback notTriggeredCallback; /**< The callback function to be called when the liquid level sensor is not triggered. */

    bool hasElapsed(uint32_t durationMs)
    {
        if (pushReadingToCallbackMs > 0) {
            return (millis() - lastCallbackPushReadingMs >= durationMs);
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
        : pin(pin)
        , pinStateWhenLiquidIsPresent(pinStateWhenLiquidIsPresent)
        , pushReadingToCallbackMs(pushReadingToCallbackMs)
        , lastState(initialReading)
    {
        buffer.fill(initialReading);
    }

    /**
     * @brief Default destructor.
     */
    virtual ~LiquidLevelSensor() = default;

    /**
     * @brief Sets the callback function to be called when the liquid level sensor is triggered.
     * @param callback The callback function to be called when the liquid level sensor is triggered.
     * @return A reference to this LiquidLevelSensor object.
     */
    void setIsTriggeredCallback(Callback callback)
    {
        isTriggeredCallback = callback;
    }

    /**
     * @brief Calls the callback function to be called when the liquid level sensor is triggered.
     */
    void callIsTriggeredCallback()
    {
        if (isTriggeredCallback != nullptr) {
            isTriggeredCallback();
            // Log.noticeln("LqSns(%d) TriggeredYes", pin);
        }
        lastCallbackPushReadingMs = millis();
    }

    /**
     * @brief Sets the callback function to be called when the liquid level sensor is not triggered.
     * @param callback The callback function to be called when the liquid level sensor is not triggered.
     * @return A reference to this LiquidLevelSensor object.
     */
    void setNotTriggeredCallback(Callback callback)
    {
        notTriggeredCallback = callback;
    }

    /**
     * @brief Calls the callback function to be called when the liquid level sensor is not triggered.
     */
    void callNotTriggeredCallback()
    {
        if (notTriggeredCallback != nullptr) {
            notTriggeredCallback();
            // Log.noticeln("LqSns(%d) TriggeredNo", pin);
        }
        lastCallbackPushReadingMs = millis();
    }

    /**
     * @brief Gets the current state of the liquid level sensor.
     * @return true if the liquid level sensor is triggered, false otherwise.
     */
    bool isTriggered(uint8_t state) const
    {
        return (state == pinStateWhenLiquidIsPresent);
    }

    /**
     * @brief Gets the current state of the liquid level sensor.
     * @return true if the liquid level sensor is not triggered, false otherwise.
     */
    bool isNotTriggered(uint8_t state) const
    {
        return (state != pinStateWhenLiquidIsPresent);
    }

    /**
     * @brief Sets up the liquid level sensor.
     * @details This function sets the pin mode of the liquid level sensor to INPUT.
     */
    void setup() override
    {
        pinMode(pin, INPUT);
    }

    /**
     * @brief Called in the main Arduino loop function.
     * @details This function calls the isTriggeredCallback if the liquid level sensor is triggered,
     * otherwise it calls the notTriggeredCallback.
     */
    void loop() override
    {
        buffer.push(digitalRead(pin));
        uint8_t state = buffer.average();

        if ((lastState != state) || hasElapsed(pushReadingToCallbackMs)) {
            lastState = state;

            if (isTriggered(state)) {
                callIsTriggeredCallback();
            } else {
                callNotTriggeredCallback();
            }
        }
    }
};

} /* namespace xal */

#endif
