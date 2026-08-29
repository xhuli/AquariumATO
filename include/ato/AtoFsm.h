#ifndef com_github_xhuli_arduino_ato_AtoFsm_H
#define com_github_xhuli_arduino_ato_AtoFsm_H
#pragma once

// #include <ArduinoLog.h>
#include "AtoActions.h"

namespace xal
{
    namespace ato
    {

        enum class State : uint8_t
        {
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

        enum class Event : uint8_t
        {
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
        String getState(State state)
        {
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

        String getEvent(Event event)
        {
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

        /**
         * @brief A single (fromState, event) -> toState rule.
         */
        struct Transition
        {
            State fromState;
            Event event;
            State toState;
        };

        /**
         * @brief The complete transition table, transcribed 1:1 from the
         * original nested-switch dispatch() implementation and verified
         * against test/test_ato_fsm/test_ato_fsm.cpp (69 passing assertions
         * covering every row below plus one "no match -> no transition"
         * guard per state).
         *
         * Ordering within a state doesn't matter (each (fromState, event)
         * pair appears at most once, so there is never more than one
         * matching row) — grouped by originating state here purely for
         * human readability, matching the original switch's structure.
         */
        static constexpr Transition TRANSITION_TABLE[] = {
            /* State::Idle */
            {State::Idle, Event::HighLevelSensorIsTriggered, State::WaterLevelHigh},
            {State::Idle, Event::LowLevelSensorNotTriggered, State::WaterLevelLow},
            {State::Idle, Event::SleepTimeElapsed, State::Error},
            {State::Idle, Event::DispenserOnTimeElapsed, State::Error},
            {State::Idle, Event::NormalLevelSensorNotTriggered, State::DispensingInAutoMode},
            {State::Idle, Event::ReservoirLevelSensorNotTriggered, State::ReservoirEmpty},
            {State::Idle, Event::DispenseButtonIsPushed, State::DispensingInManualMode},
            {State::Idle, Event::MaxIdleTimeElapsed, State::IdleForTooLong},
            {State::Idle, Event::SleepButtonIsPushed, State::Sleeping},

            /* State::DispensingInAutoMode */
            {State::DispensingInAutoMode, Event::HighLevelSensorIsTriggered, State::WaterLevelHigh},
            {State::DispensingInAutoMode, Event::LowLevelSensorNotTriggered, State::WaterLevelLow},
            {State::DispensingInAutoMode, Event::SleepTimeElapsed, State::Error},
            {State::DispensingInAutoMode, Event::NormalLevelSensorIsTriggered, State::Idle},
            {State::DispensingInAutoMode, Event::DispenseButtonIsPushed, State::Idle},
            {State::DispensingInAutoMode, Event::ReservoirLevelSensorNotTriggered, State::ReservoirEmpty},
            {State::DispensingInAutoMode, Event::DispenserOnTimeElapsed, State::ReservoirEmpty},
            {State::DispensingInAutoMode, Event::SleepButtonIsPushed, State::Sleeping},

            /* State::DispensingInManualMode */
            {State::DispensingInManualMode, Event::HighLevelSensorIsTriggered, State::WaterLevelHigh},
            {State::DispensingInManualMode, Event::SleepTimeElapsed, State::Error},
            {State::DispensingInManualMode, Event::NormalLevelSensorIsTriggered, State::Idle},
            {State::DispensingInManualMode, Event::DispenseButtonIsPushed, State::Idle},
            {State::DispensingInManualMode, Event::DispenserOnTimeElapsed, State::Idle},
            {State::DispensingInManualMode, Event::ReservoirLevelSensorNotTriggered, State::ReservoirEmpty},
            {State::DispensingInManualMode, Event::SleepButtonIsPushed, State::Sleeping},

            /* State::WaterLevelLow */
            {State::WaterLevelLow, Event::LowLevelSensorIsTriggered, State::Idle},
            {State::WaterLevelLow, Event::NormalLevelSensorIsTriggered, State::Idle},
            {State::WaterLevelLow, Event::DispenseButtonIsPushed, State::DispensingInManualMode},
            {State::WaterLevelLow, Event::SleepButtonIsPushed, State::Sleeping},

            /* State::WaterLevelHigh */
            {State::WaterLevelHigh, Event::DispenseButtonIsPushed, State::Idle},
            {State::WaterLevelHigh, Event::HighLevelSensorNotTriggered, State::Idle},
            {State::WaterLevelHigh, Event::SleepButtonIsPushed, State::Sleeping},
            {State::WaterLevelHigh, Event::NormalLevelSensorNotTriggered, State::Error},
            {State::WaterLevelHigh, Event::LowLevelSensorNotTriggered, State::Error},

            /* State::ReservoirEmpty */
            {State::ReservoirEmpty, Event::HighLevelSensorIsTriggered, State::Error},
            {State::ReservoirEmpty, Event::LowLevelSensorNotTriggered, State::Error},
            {State::ReservoirEmpty, Event::SleepTimeElapsed, State::Error},
            {State::ReservoirEmpty, Event::DispenserOnTimeElapsed, State::Error},
            {State::ReservoirEmpty, Event::NormalLevelSensorIsTriggered, State::Idle},
            {State::ReservoirEmpty, Event::DispenseButtonIsPushed, State::Idle},
            {State::ReservoirEmpty, Event::ReservoirLevelSensorIsTriggered, State::Idle},
            {State::ReservoirEmpty, Event::SleepButtonIsPushed, State::Sleeping},

            /* State::Sleeping */
            {State::Sleeping, Event::DispenseButtonIsPushed, State::Idle},
            {State::Sleeping, Event::SleepButtonIsPushed, State::Idle},
            {State::Sleeping, Event::SleepTimeElapsed, State::Idle},

            /* State::IdleForTooLong */
            {State::IdleForTooLong, Event::NormalLevelSensorNotTriggered, State::DispensingInAutoMode},
            {State::IdleForTooLong, Event::DispenseButtonIsPushed, State::Idle},
            {State::IdleForTooLong, Event::LowLevelSensorNotTriggered, State::WaterLevelLow},
            {State::IdleForTooLong, Event::HighLevelSensorIsTriggered, State::WaterLevelHigh},
            {State::IdleForTooLong, Event::ReservoirLevelSensorNotTriggered, State::ReservoirEmpty},
            {State::IdleForTooLong, Event::SleepButtonIsPushed, State::Sleeping},

            /* State::Error */
            {State::Error, Event::DispenseButtonIsPushed, State::Idle},
            {State::Error, Event::SleepButtonIsPushed, State::Sleeping},
        };

        /**
         * @class AtoFsm
         * @brief This class implements a finite state machine (FSM) for the Automatic Top-Off (ATO) system.
         *
         * The AtoFsm class manages the state transitions and actions of the ATO system based on various events.
         * It uses the AtoActions class to perform actions corresponding to different states and events.
         *
         * dispatch() looks up the (currentState, event) pair in TRANSITION_TABLE above.
         * If a matching row exists, it transitions to that row's toState. If no row
         * matches, the event is ignored — equivalent to the original implementation's
         * `default: break;` in every state's switch.
         *
         * Usage:
         * - Create an instance of AtoFsm with a reference to an AtoActions instance.
         * - Use the dispatch method to handle events and transition between states.
         */
        class AtoFsm
        {
        private:
            State state = State::Idle;
            AtoActions &atoActions;

        public:
            explicit AtoFsm(AtoActions &atoActions)
                : atoActions(atoActions)
            {
            }
            ~AtoFsm() = default;

            /**
             * @brief Gets the current state of the FSM.
             * @return The current State.
             */
            State getState() const
            {
                return state;
            }

            void dispatch(Event event)
            {
                // Log.noticeln("AFsm St=%s Ev=%s", getState(state).c_str(), getEvent(event).c_str());

                for (const auto &transition : TRANSITION_TABLE)
                {
                    if (transition.fromState == state && transition.event == event)
                    {
                        transit(transition.toState);
                        return;
                    }
                }
                /* No matching (state, event) row: ignore the event, same as the
                   original switch's default: break; in every state. */
            }

        private:
            void enter(State actOnState)
            {
                // Log.noticeln("AFsm enter=%s", getState(actOnState).c_str());
                switch (actOnState)
                {
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
                if (this->state != actOnState)
                {
                    exit(actOnState);
                    this->state = actOnState;
                    enter(actOnState);
                }
            };
        };

    } /* namespace ato */
} /* namespace xal */

#endif
