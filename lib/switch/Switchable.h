#ifndef com_github_xhuli_arduino_lib_switch_Switchable_H
#define com_github_xhuli_arduino_lib_switch_Switchable_H
#pragma once

#include "api/AbstractSwitchable.h"
#include "enums/SwitchState.h"

#include <Arduino.h>
#include <Runnable.h>

namespace xal {

    /**
     * @class Switchable
     * @brief A switchable component that can be switched on or off.
     * @implements AbstractSwitchable, Runnable
     */
    class Switchable : virtual public AbstractSwitchable,
                       virtual public Runnable {
    protected:
        uint8_t pin;
        uint8_t pinValueWhenSwitchStateOn;

        uint8_t getPinValueWhenSwitchStateOn() {
            return pinValueWhenSwitchStateOn;
        }

        uint8_t getPinValueWhenSwitchStateOff() {
            return pinValueWhenSwitchStateOn == HIGH ? LOW : HIGH;
        }

    public:
        Switchable(uint8_t pin, uint8_t pinValueWhenSwitchStateOn)
            : AbstractSwitchable(SwitchState::Off), pin(pin), pinValueWhenSwitchStateOn(pinValueWhenSwitchStateOn) {
        }

        Switchable(SwitchState state, uint8_t pin, uint8_t pinValueWhenSwitchStateOn)
            : AbstractSwitchable(state), pin(pin), pinValueWhenSwitchStateOn(pinValueWhenSwitchStateOn) {
        }

        ~Switchable() override = default;

        void doSwitch(SwitchState state) {
            if (state == SwitchState::On) {
                digitalWrite(pin, getPinValueWhenSwitchStateOn());
            } else {
                digitalWrite(pin, getPinValueWhenSwitchStateOff());
            }
        }

        void setOn() override {
            AbstractSwitchable::setOn();
            doSwitch(SwitchState::On);
        }

        void setOff() override {
            AbstractSwitchable::setOff();
            doSwitch(SwitchState::Off);
        }

        void toggle() override {
            AbstractSwitchable::toggle();
            doSwitch(AbstractSwitchable::getState());
        }

        void setState(SwitchState state) override {
            AbstractSwitchable::setState(state);
            doSwitch(state);
        }

        void setup() override {
            pinMode(pin, OUTPUT);
            doSwitch(state);
        }

        void loop() override {}
    };

} /* namespace xal */

#endif
