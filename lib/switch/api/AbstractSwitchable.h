#ifndef com_github_xhuli_arduino_lib_switch_api_AbstractSwitchable_H
#define com_github_xhuli_arduino_lib_switch_api_AbstractSwitchable_H
#pragma once

#include "enums/SwitchState.h"

namespace xal {

/**
 * @class AbstractSwitchable
 * @brief Represents an abstract switchable component.
 */
class AbstractSwitchable {
protected:
    SwitchState state = SwitchState::Off;

public:
    /**
     * @brief Default constructor.
     */
    AbstractSwitchable() = default;

    /**
     * @brief Constructs an AbstractSwitchable object with the specified initial state.
     * @param state The initial state.
     */
    AbstractSwitchable(SwitchState state)
        : state(state)
    {
    }

    /**
     * @brief Destructor.
     */
    virtual ~AbstractSwitchable() = default;

    /**
     * @brief Sets the component state to "On".
     */
    virtual void setOn()
    {
        setState(SwitchState::On);
    }

    /**
     * @brief Sets the component state to "Off".
     */
    virtual void setOff()
    {
        setState(SwitchState::Off);
    }

    /**
     * @brief Toggles the component state between "On" and "Off".
     */
    virtual void toggle()
    {
        if (isOn()) {
            setOff();
        } else {
            setOn();
        }
    }

    /**
     * @brief Checks if the component is currently on.
     * @return True if the component is on, false otherwise.
     */
    virtual bool isOn()
    {
        return state == SwitchState::On;
    }

    /**
     * @brief Checks if the component is currently off.
     * @return True if the component is off, false otherwise.
     */
    virtual bool isOff()
    {
        return state == SwitchState::Off;
    }

    /**
     * @brief Gets the current state of the component.
     * @return The current state.
     */
    virtual SwitchState getState()
    {
        return state;
    }

    /**
     * @brief Sets the state of the component.
     * @param state The state to set.
     */
    virtual void setState(SwitchState state)
    {
        this->state = state;
    }
};

} /* namespace xal */

#endif
