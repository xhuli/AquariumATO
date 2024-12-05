#ifndef com_github_xhuli_arduino_ato_AtoFsm_H
#define com_github_xhuli_arduino_ato_AtoFsm_H
#pragma once

// #include <ArduinoLog.h>
#include "AtoActions.h"

namespace xal {
namespace ato {

    enum class State {
        Idle,
        DispensingInAutoMode,
        DispensingInManualMode,
        WaterLevelLow,
        WaterLevelHigh,
        ReservoirEmpty,
        Sleeping,
        IdleForTooLong,
        Error
    };

    enum class Event {
        HighLevelSensorIsTriggered,
        HighLevelSensorNotTriggered,

        LowLevelSensorIsTriggered,
        LowLevelSensorNotTriggered,

        ReservoirLevelSensorIsTriggered,
        ReservoirLevelSensorNotTriggered,

        NormalLevelSensorIsTriggered,
        NormalLevelSensorNotTriggered,

        DispenseButtonIsPushed,
        SleepButtonIsPushed,

        DispenserOnTimeElapsed,
        SleepTimeElapsed,
        MaxIdleTimeElapsed,
    };

    /*
    String getState(State state) {
        switch (state)
        {
            default:
            case State::Idle:                   return "Idl";
            case State::IdleForTooLong:         return "Idl42Lng";
            case State::DispensingInAutoMode:   return "DspAuto";
            case State::DispensingInManualMode: return "DspManual";
            case State::WaterLevelLow:          return "WtrLvlLow";
            case State::WaterLevelHigh:         return "WtrLvlHigh";
            case State::ReservoirEmpty:         return "RE";
            case State::Sleeping:               return "Slp";
            case State::Error:                  return "Err";
        }   
    }

    String getEvent(Event event) {
        switch (event) {
            default:
            case Event::HighLevelSensorIsTriggered:       return "HiOn";
            case Event::HighLevelSensorNotTriggered:      return "HiOff";
            case Event::LowLevelSensorIsTriggered:        return "LowOn";
            case Event::LowLevelSensorNotTriggered:       return "LowOff";
            case Event::ReservoirLevelSensorIsTriggered:  return "RsrvOn";
            case Event::ReservoirLevelSensorNotTriggered: return "RsrvOff";
            case Event::NormalLevelSensorIsTriggered:     return "NormOn";
            case Event::NormalLevelSensorNotTriggered:    return "NormOff";
            case Event::DispenseButtonIsPushed:           return "DispBtnOn";
            case Event::SleepButtonIsPushed:              return "SleepBtnOn";
            case Event::DispenserOnTimeElapsed:           return "DispElapsed";
            case Event::SleepTimeElapsed:                 return "SleepElapsed";
            case Event::MaxIdleTimeElapsed:               return "MaxIdleElapsed";
        }
    }
    // */

    class AtoFsm {
    private:
        State state = State::Idle;
        AtoActions& atoActions;

    public:
        explicit AtoFsm(AtoActions& atoActions)
            : atoActions(atoActions)
        {
        }
        ~AtoFsm() = default;

        void dispatch(Event event) {
            // Log.noticeln("AFsm St=%s Ev=%s", getState(state).c_str(), getEvent(event).c_str());

            switch (state) {
                case State::Idle:
                    switch (event) {
                        case Event::HighLevelSensorIsTriggered:
                            transit(State::WaterLevelHigh);
                            break;
                        case Event::LowLevelSensorNotTriggered:
                            transit(State::WaterLevelLow);
                            break;
                        case Event::SleepTimeElapsed:
                        case Event::DispenserOnTimeElapsed:
                            transit(State::Error);
                            break;
                        case Event::NormalLevelSensorNotTriggered:
                            transit(State::DispensingInAutoMode);
                            break;
                        case Event::ReservoirLevelSensorNotTriggered:
                            transit(State::ReservoirEmpty);
                            break;
                        case Event::DispenseButtonIsPushed:
                            transit(State::DispensingInManualMode);
                            break;
                        case Event::MaxIdleTimeElapsed:
                            transit(State::IdleForTooLong);
                            break;
                        case Event::SleepButtonIsPushed:
                            transit(State::Sleeping);
                            break;
                        default:
                            break;
                    }
                    break;

                case State::DispensingInAutoMode:
                    switch (event) {
                        case Event::HighLevelSensorIsTriggered:
                            transit(State::WaterLevelHigh);
                            break;
                        case Event::LowLevelSensorNotTriggered:
                            transit(State::WaterLevelLow);
                            break;
                        case Event::SleepTimeElapsed:
                            transit(State::Error);
                            break;
                        case Event::NormalLevelSensorIsTriggered:
                        case Event::DispenseButtonIsPushed:
                            transit(State::Idle);
                            break;
                        case Event::ReservoirLevelSensorNotTriggered:
                        case Event::DispenserOnTimeElapsed:
                            transit(State::ReservoirEmpty);
                            break;
                        case Event::SleepButtonIsPushed:
                            transit(State::Sleeping);
                            break;
                        default:
                            break;
                    }
                    break;

                case State::DispensingInManualMode:
                    switch (event) {
                        case Event::HighLevelSensorIsTriggered:
                            transit(State::WaterLevelHigh);
                            break;
                        case Event::SleepTimeElapsed:
                            transit(State::Error);
                            break;
                        case Event::NormalLevelSensorIsTriggered:
                        case Event::DispenseButtonIsPushed:
                        case Event::DispenserOnTimeElapsed:
                            transit(State::Idle);
                            break;
                        case Event::ReservoirLevelSensorNotTriggered:
                            transit(State::ReservoirEmpty);
                            break;
                        case Event::SleepButtonIsPushed:
                            transit(State::Sleeping);
                            break;
                        default:
                            break;
                    }
                    break;

                case State::WaterLevelLow:
                    switch (event) {
                        case Event::LowLevelSensorIsTriggered:
                        case Event::NormalLevelSensorIsTriggered:
                            transit(State::Idle);
                            break;
                        case Event::DispenseButtonIsPushed:
                            transit(State::DispensingInManualMode);
                            break;
                        case Event::SleepButtonIsPushed:
                            transit(State::Sleeping);
                            break;
                        default:
                            break;
                    }
                    break;

                case State::WaterLevelHigh:
                    switch (event) {
                        case Event::DispenseButtonIsPushed:
                        case Event::HighLevelSensorNotTriggered:
                            transit(State::Idle);
                            break;
                        case Event::SleepButtonIsPushed:
                            transit(State::Sleeping);
                            break;
                        case Event::NormalLevelSensorNotTriggered:
                        case Event::LowLevelSensorNotTriggered:
                            transit(State::Error);
                            break;
                        default:
                            break;
                    }
                    break;

                case State::ReservoirEmpty:
                    switch (event) {
                        case Event::HighLevelSensorIsTriggered:
                        case Event::LowLevelSensorNotTriggered:
                        case Event::SleepTimeElapsed:
                        case Event::DispenserOnTimeElapsed:
                            transit(State::Error);
                            break;
                        case Event::NormalLevelSensorIsTriggered:
                        case Event::DispenseButtonIsPushed:
                        case Event::ReservoirLevelSensorIsTriggered:
                            transit(State::Idle);
                            break;
                        case Event::SleepButtonIsPushed:
                            transit(State::Sleeping);
                            break;
                        default:
                            break;
                    }
                    break;

                case State::Sleeping:
                    switch (event) {
                        case Event::DispenseButtonIsPushed:
                        case Event::SleepButtonIsPushed:
                        case Event::SleepTimeElapsed:
                            transit(State::Idle);
                            break;
                        default:
                            break;
                    }
                    break;

                case State::IdleForTooLong:
                    switch (event) {
                        case Event::NormalLevelSensorNotTriggered:
                            transit(State::DispensingInAutoMode);
                            break;
                        case Event::DispenseButtonIsPushed:
                            transit(State::Idle);
                            break;
                        case Event::LowLevelSensorNotTriggered:
                            transit(State::WaterLevelLow);
                            break;
                        case Event::HighLevelSensorIsTriggered:
                            transit(State::WaterLevelHigh);
                            break;
                        case Event::ReservoirLevelSensorNotTriggered:
                            transit(State::ReservoirEmpty);
                            break;
                        case Event::SleepButtonIsPushed:
                            transit(State::Sleeping);
                            break;
                        default:
                            break;
                    }

                case State::Error:
                    switch (event) {
                        case Event::DispenseButtonIsPushed:
                            transit(State::Idle);
                            break;
                        case Event::SleepButtonIsPushed:
                            transit(State::Sleeping);
                            break;
                        default:
                            break;
                    }
                    break;
            }
        }

    private:
        void enter(State actOnState)
        {
            // Log.noticeln("AFsm enter=%s", getState(actOnState).c_str());
            switch (actOnState) {
                case State::Idle:
                    atoActions.onEntryIdleState();
                    break;
                case State::DispensingInAutoMode:
                    atoActions.onEntryDispensingInAutoModeState();
                    break;
                case State::DispensingInManualMode:
                    atoActions.onEntryDispensingInManualModeState();
                    break;
                case State::ReservoirEmpty:
                    atoActions.onEntryReservoirEmptyState();
                    break;
                case State::Sleeping:
                    atoActions.onEntrySleepingState();
                    break;
                case State::IdleForTooLong:
                    atoActions.onEntryIdleForTooLongState();
                    break;
                case State::WaterLevelLow:
                    atoActions.onEntryWaterLowState();
                    break;
                case State::WaterLevelHigh:
                    atoActions.onEntryWaterHighState();
                    break;
                case State::Error:
                    atoActions.onEntryErrorState();
                    break;
                default:
                    break;
            }
        }

        void exit(State actOnState)
        {
            atoActions.onExitState();
        }

        void transit(State actOnState)
        {
            if (this->state != actOnState) {
                exit(actOnState);
                this->state = actOnState;
                enter(actOnState);
            }
        };
    };

} /* namespace ato */
} /* namespace xal */

#endif
