#ifndef com_github_xhuli_arduino_ato_AtoFsm_H
#define com_github_xhuli_arduino_ato_AtoFsm_H
#pragma once

#include <Arduino.h>
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

        /**
         * @brief Human-readable, flash-resident (PROGMEM via F()) name for a
         * State, for use in trace/debug output. Costs no RAM — Serial.print()
         * reads directly from flash via the returned __FlashStringHelper*.
         */
        inline const __FlashStringHelper *stateName(State state)
        {
            switch (state)
            {
                default:
                case State::Idle:                   return F("Idle");
                case State::IdleForTooLong:         return F("IdleForTooLong");
                case State::DispensingInAutoMode:   return F("DispensingInAutoMode");
                case State::DispensingInManualMode: return F("DispensingInManualMode");
                case State::WaterLevelLow:          return F("WaterLevelLow");
                case State::WaterLevelHigh:         return F("WaterLevelHigh");
                case State::ReservoirEmpty:         return F("ReservoirEmpty");
                case State::Sleeping:               return F("Sleeping");
                case State::Error:                  return F("Error");
            }
        }

        /**
         * @brief Human-readable, flash-resident name for an Event. See stateName().
         */
        inline const __FlashStringHelper *eventName(Event event)
        {
            switch (event)
            {
                default:
                case Event::HighLevelSensorIsTriggered:       return F("HighLevelSensorIsTriggered");
                case Event::HighLevelSensorNotTriggered:      return F("HighLevelSensorNotTriggered");
                case Event::LowLevelSensorIsTriggered:        return F("LowLevelSensorIsTriggered");
                case Event::LowLevelSensorNotTriggered:       return F("LowLevelSensorNotTriggered");
                case Event::ReservoirLevelSensorIsTriggered:  return F("ReservoirLevelSensorIsTriggered");
                case Event::ReservoirLevelSensorNotTriggered: return F("ReservoirLevelSensorNotTriggered");
                case Event::NormalLevelSensorIsTriggered:     return F("NormalLevelSensorIsTriggered");
                case Event::NormalLevelSensorNotTriggered:    return F("NormalLevelSensorNotTriggered");
                case Event::DispenseButtonIsPushed:           return F("DispenseButtonIsPushed");
                case Event::SleepButtonIsPushed:              return F("SleepButtonIsPushed");
                case Event::DispenserOnTimeElapsed:           return F("DispenserOnTimeElapsed");
                case Event::SleepTimeElapsed:                 return F("SleepTimeElapsed");
                case Event::MaxIdleTimeElapsed:               return F("MaxIdleTimeElapsed");
            }
        }

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
         * An optional trace callback (see setTraceCallback()) can be registered to
         * observe every dispatch() call (fromState, event, resulting state, whether
         * a rule matched) for debugging, without AtoFsm knowing anything about
         * Serial or any other output mechanism — matching this project's existing
         * "components fire callbacks, callers decide what to do with them" pattern.
         *
         * Usage:
         * - Create an instance of AtoFsm with a reference to an AtoActions instance.
         * - Use the dispatch method to handle events and transition between states.
         */
        class AtoFsm
        {
        public:
            /**
             * @brief Called by dispatch() on every event, whether or not it
             * produced a transition. fromState/toState are the same value
             * when matched is false (no rule fired) or when a rule mapped a
             * state to itself (not currently possible in TRANSITION_TABLE,
             * but the callback signature doesn't assume otherwise).
             */
            typedef void (*TraceCallback)(State fromState, Event event, State toState, bool matched);

        private:
            State state = State::Idle;
            AtoActions &atoActions;
            TraceCallback traceCallback = nullptr;

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

            /**
             * @brief Registers a callback invoked on every dispatch() call.
             * Pass nullptr to disable tracing. See TraceCallback for details.
             */
            void setTraceCallback(TraceCallback callback)
            {
                traceCallback = callback;
            }

            void dispatch(Event event)
            {
                State fromState = state;
                bool matched = false;

                for (const auto &transition : TRANSITION_TABLE)
                {
                    if (transition.fromState == state && transition.event == event)
                    {
                        transit(transition.toState);
                        matched = true;
                        break;
                    }
                }

                if (traceCallback != nullptr)
                {
                    traceCallback(fromState, event, state, matched);
                }
            }

        private:
            void enter(State actOnState)
            {
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

            void exit()
            {
                atoActions.onExitState();
            }

            void transit(State actOnState)
            {
                if (this->state != actOnState)
                {
                    exit();
                    this->state = actOnState;
                    enter(actOnState);
                }
            };
        };

    } /* namespace ato */
} /* namespace xal */

#endif
