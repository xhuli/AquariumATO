#ifndef com_github_xhuli_arduino_lib_buttons_PushButton_H
#define com_github_xhuli_arduino_lib_buttons_PushButton_H
#pragma once

#include <Arduino.h>
// #include <ArduinoLog.h>
#include <Runnable.h>

namespace xal {

using Callback = void (*)();

/**
 * @class PushButton
 * @brief The base class for push button functionality.
 */
class PushButton : public Runnable {
private:
    uint8_t pin; /**< The pin number of the push button. */
    uint8_t pinModeState = INPUT; /**< The pin mode of the push button. Can be INPUT or INPUT_PULLUP. */
    uint8_t pinStateWhenReleased = LOW; /**< The pin state when the push button is released. */

    uint8_t previousState = pinStateWhenReleased; /**< The previous state of the push button. */

    uint16_t debounceMs = 0; /**< The debounce time in milliseconds. */
    uint16_t longPressMs = 0; /**< The minimum duration of a long press in milliseconds. */
    uint32_t lastPressTime = 0; /**< The time (in milliseconds) when the push button was last pressed. */

    Callback shortPressCallback = nullptr; /**< The callback for a short press. */
    Callback longPressCallback = nullptr; /**< The callback for a long press. */

public:
    /**
     * @brief Constructs an PushButton object.
     *
     * This implementation assumes the MCU pin is connected to ground via a pull-down resistor.
     *
     * @param pin The pin number of the push button.
     * @param debounceMs The debounce time in milliseconds.
     * @param longPushMs The long press time in milliseconds.
     * @param buttonListener The button listener object to handle button events.
     */
    explicit PushButton(
        uint8_t pin,
        uint8_t pinModeState,
        uint8_t pinStateWhenReleased,
        uint16_t debounceMs,
        uint16_t longPressMs)
        : pin(pin)
        , pinModeState(pinModeState)
        , pinStateWhenReleased(pinStateWhenReleased)
        , previousState(pinStateWhenReleased)
        , debounceMs(debounceMs)
        , longPressMs(longPressMs)
    {
    }

    /**
     * @brief Default destructor.
     */
    ~PushButton() = default;

    /**
     * @brief Sets the callback for a short press.
     * @param callback The callback for a short press.
     * @return The PushButton object.
     */
    void setShortPressCallback(Callback callback)
    {
        shortPressCallback = callback;
    }

    /**
     * @brief Sets the callback for a long press.
     * @param callback The callback for a long press.
     * @return The PushButton object.
     */
    void setLongPressCallback(Callback callback)
    {
        longPressCallback = callback;
    }

    /**
     * @brief Sets up the push button.
     *
     * This implementation assumes the button MCU pin is connected to ground via a pull-down resistor.
     * This function sets the pin mode of the button to INPUT.
     *
     * @note This function must be called in the setup() function of the sketch using the following code:
     * @example Runnable.setupAll();
     */
    virtual void setup() override
    {
        pinMode(pin, pinModeState);
    }

    /**
     * @brief Runs the push button logic in the loop.
     *
     * @note This function must be called in the loop() function of the sketch using the following code:
     * @example Runnable.loopAll();
     */
    virtual void loop() override
    {
        uint8_t state = digitalRead(pin);
        if (state != previousState) {
            if (state == pinStateWhenReleased) {
                /* button is released */
                if (millis() - lastPressTime > debounceMs) {
                    /* button is released after debounce time */
                    if ((millis() - lastPressTime < longPressMs) && (shortPressCallback != nullptr)) {
                        shortPressCallback();
                    } else if (longPressCallback != nullptr) {
                        longPressCallback();
                    }
                }
            } else {
                /* button is pressed */
                lastPressTime = millis();
            }

            previousState = state;
            // Log.noticeln("PshBttn(%d) pushed=%d", pin, (state != pinStateWhenReleased));
        }
    };
};

} /* namespace xal */

#endif
