#ifndef com_github_xhuli_arduino_ato_AtoConfigConsole_H
#define com_github_xhuli_arduino_ato_AtoConfigConsole_H
#pragma once

#include <Arduino.h>
#include <Runnable.h>
#include <ctype.h>
#include <string.h>

#include "AtoConfig.h"

namespace xal
{
    namespace ato
    {

        /**
         * @class AtoConfigConsole
         * @brief A minimal, non-blocking line-based Serial console for viewing
         * and changing AtoConfig at runtime, without reflashing.
         *
         * Commands (case-insensitive, one per line, newline-terminated):
         *   HELP                 - list commands and field names
         *   GET                  - print current in-memory config
         *   SET <NAME> <VALUE>   - set a field; applies immediately to the
         *                          live hardware via the ApplyCallback, but
         *                          is NOT persisted until SAVE is sent
         *   SAVE                 - persist current in-memory config to EEPROM
         *   RESET                - revert in-memory config to the compiled
         *                          defaults; applies immediately, not saved
         *
         * Field names: SLEEP_MAX_MS, IDLE_MAX_MS, PUMP_MAX_ON_MS
         *
         * If nothing is connected to Serial, Serial.available() simply
         * returns 0 and loop() is a no-op — safe to leave wired up in a
         * standalone field deployment with no PC attached.
         *
         * @implements Runnable
         */
        class AtoConfigConsole : public Runnable
        {
            typedef bool (*ApplyCallback)(const AtoConfig &);

        private:
            static constexpr uint8_t BUFFER_SIZE = 48;

            AtoConfig &config;
            const AtoConfig &defaults;
            ApplyCallback applyCallback;
            bool &traceEnabled;
            bool &traceVerbose;

            char buffer[BUFFER_SIZE];
            uint8_t bufferLen = 0;
            bool discardUntilNewline = false;

        public:
            /**
             * @param config Reference to the live, in-memory config to view/edit.
             * @param defaults Reference to the compiled-in defaults, used by RESET.
             * @param applyCallback Called with the current config after every
             * successful SET/RESET, so changes take effect immediately without
             * requiring a reboot. Typically pushes values into Timer/TimedSwitchable
             * setters.
             * @param traceEnabled Reference to a flag toggled by TRACE ON/OFF,
             * read by the FSM trace callback registered in main.cpp.
             * @param traceVerbose Reference to a flag toggled by TRACE ALL/ON,
             * distinguishing "log every dispatch() call" from "log only calls
             * that actually produced a transition".
             */
            AtoConfigConsole(
                AtoConfig &config,
                const AtoConfig &defaults,
                ApplyCallback applyCallback,
                bool &traceEnabled,
                bool &traceVerbose)
                : config(config),
                  defaults(defaults),
                  applyCallback(applyCallback),
                  traceEnabled(traceEnabled),
                  traceVerbose(traceVerbose)
            {
            }

            ~AtoConfigConsole() override = default;

            void setup() override
            {
                /* Serial.begin() is expected to be called once in the sketch's
                   own setup(); this class only reads from an already-initialized
                   Serial and never calls Serial.begin() itself. */
            }

            void loop() override
            {
                while (Serial.available() > 0)
                {
                    const InputResult result = processInputChar((char)Serial.read());

                    if (result == INPUT_LINE_TOO_LONG)
                    {
                        Serial.println(F("ERROR line too long"));
                    }
                    else if (result == INPUT_LINE_READY)
                    {
                        executeBufferedLine();
                    }
                }
            }

            enum InputResult
            {
                INPUT_NONE,
                INPUT_LINE_READY,
                INPUT_LINE_TOO_LONG
            };

            /**
             * @brief Incrementally consumes one serial character without blocking.
             * @details Once the line buffer overflows, the complete physical line
             * is invalidated and every following character (including backspace)
             * is discarded until newline. Exactly the first overflow character
             * reports INPUT_LINE_TOO_LONG; the terminating newline only restores
             * normal input processing.
             */
            InputResult processInputChar(char c)
            {
                if (discardUntilNewline)
                {
                    if (c == '\n')
                    {
                        discardUntilNewline = false;
                        bufferLen = 0;
                    }
                    return INPUT_NONE;
                }

                if (c == '\r')
                {
                    return INPUT_NONE;
                }

                if (c == '\n')
                {
                    buffer[bufferLen] = '\0';
                    bufferLen = 0;
                    return INPUT_LINE_READY;
                }

                if (c == '\b' || c == 0x7F)
                {
                    if (bufferLen > 0)
                    {
                        bufferLen--;
                    }
                    return INPUT_NONE;
                }

                if (bufferLen < (BUFFER_SIZE - 1))
                {
                    buffer[bufferLen++] = c;
                    return INPUT_NONE;
                }

                discardUntilNewline = true;
                bufferLen = 0;
                return INPUT_LINE_TOO_LONG;
            }

            /**
             * @brief Parses an exact decimal uint32_t without libc overflow rules.
             */
            static bool parseUint32(const char *text, uint32_t &result)
            {
                if (text == nullptr || *text == '\0')
                {
                    return false;
                }

                uint32_t value = 0;
                while (*text != '\0')
                {
                    if (*text < '0' || *text > '9')
                    {
                        return false;
                    }

                    const uint8_t digit = static_cast<uint8_t>(*text - '0');
                    if (value > (UINT32_MAX - digit) / 10U)
                    {
                        return false;
                    }

                    value = value * 10U + digit;
                    ++text;
                }

                result = value;
                return true;
            }

            /**
             * @brief Executes one mutable newline-stripped command line.
             * @return true only when the command grammar and requested operation
             * are valid. Errors are printed with the ERROR prefix.
             * This wrapper is public for deterministic Unity coverage.
             */
            bool executeLine(char *line)
            {
                return handleLine(line);
            }

            bool executeBufferedLine()
            {
                return handleLine(buffer);
            }


            /**
             * @brief Attempts one already-parsed SET operation.
             * @details The update is staged in a candidate copy and committed
             * only after semantic validation and successful live application,
             * so a rejected value never mutates the active configuration.
             * This is public to allow deterministic Unity coverage without
             * mocking HardwareSerial.
             */
            bool trySetValue(const char *name, uint32_t numericValue)
            {
                AtoConfig candidate = config;

                if (equalsIgnoreCase(name, "SLEEP_MAX_MS"))
                {
                    candidate.sleepMaxDurationMs = numericValue;
                }
                else if (equalsIgnoreCase(name, "IDLE_MAX_MS"))
                {
                    candidate.idleMaxDurationMs = numericValue;
                }
                else if (equalsIgnoreCase(name, "PUMP_MAX_ON_MS"))
                {
                    candidate.pumpMaxOnDurationMs = numericValue;
                }
                else
                {
                    return false;
                }

                if (!isValidAtoConfig(candidate) || !applyCallback(candidate))
                {
                    return false;
                }

                config = candidate;
                return true;
            }

        private:
            static bool equalsIgnoreCase(const char *a, const char *b)
            {
                while (*a && *b)
                {
                    if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
                    {
                        return false;
                    }
                    a++;
                    b++;
                }
                return *a == *b;
            }

            static bool hasExtraArgument()
            {
                return strtok(nullptr, " ") != nullptr;
            }

            bool rejectExtraArguments()
            {
                if (hasExtraArgument())
                {
                    Serial.println(F("ERROR unexpected argument"));
                    return true;
                }
                return false;
            }

            bool handleLine(char *line)
            {
                char *command = strtok(line, " ");
                if (command == nullptr)
                {
                    return true;
                }

                if (equalsIgnoreCase(command, "HELP"))
                {
                    if (rejectExtraArguments()) return false;
                    printHelp();
                    return true;
                }
                if (equalsIgnoreCase(command, "GET"))
                {
                    if (rejectExtraArguments()) return false;
                    printConfig();
                    return true;
                }
                if (equalsIgnoreCase(command, "SET"))
                {
                    return handleSet();
                }
                if (equalsIgnoreCase(command, "SAVE"))
                {
                    if (rejectExtraArguments()) return false;
                    if (AtoConfigStore::save(config))
                    {
                        Serial.println(F("Saved to EEPROM."));
                        return true;
                    }
                    Serial.println(F("ERROR invalid config; not saved."));
                    return false;
                }
                if (equalsIgnoreCase(command, "RESET"))
                {
                    if (rejectExtraArguments()) return false;
                    if (applyCallback(defaults))
                    {
                        config = defaults;
                        Serial.println(F("Reset to compiled defaults (not yet saved; use SAVE)."));
                        return true;
                    }
                    Serial.println(F("ERROR compiled defaults are invalid; reset refused."));
                    return false;
                }
                if (equalsIgnoreCase(command, "TRACE"))
                {
                    return handleTrace();
                }

                Serial.println(F("ERROR unknown command. Type HELP."));
                return false;
            }

            /**
             * @brief TRACE ON|ALL|OFF — toggles live FSM dispatch tracing.
             */
            bool handleTrace()
            {
                char *mode = strtok(nullptr, " ");
                if (mode == nullptr)
                {
                    Serial.println(F("ERROR usage: TRACE ON|ALL|OFF. Type HELP."));
                    return false;
                }
                if (hasExtraArgument())
                {
                    Serial.println(F("ERROR unexpected argument"));
                    return false;
                }

                if (equalsIgnoreCase(mode, "ON"))
                {
                    traceEnabled = true;
                    traceVerbose = false;
                    Serial.println(F("Trace ON (state changes only)."));
                    return true;
                }
                if (equalsIgnoreCase(mode, "ALL"))
                {
                    traceEnabled = true;
                    traceVerbose = true;
                    Serial.println(F("Trace ALL (includes ignored events)."));
                    return true;
                }
                if (equalsIgnoreCase(mode, "OFF"))
                {
                    traceEnabled = false;
                    traceVerbose = false;
                    Serial.println(F("Trace OFF."));
                    return true;
                }

                Serial.println(F("ERROR usage: TRACE ON|ALL|OFF. Type HELP."));
                return false;
            }

            bool handleSet()
            {
                char *name = strtok(nullptr, " ");
                char *value = strtok(nullptr, " ");

                if (name == nullptr || value == nullptr)
                {
                    Serial.println(F("ERROR usage: SET <NAME> <VALUE>. Type HELP."));
                    return false;
                }
                if (hasExtraArgument())
                {
                    Serial.println(F("ERROR unexpected argument"));
                    return false;
                }

                uint32_t numericValue = 0;
                if (!parseUint32(value, numericValue))
                {
                    Serial.println(F("ERROR invalid value"));
                    return false;
                }

                if (equalsIgnoreCase(name, "PUMP_MAX_ON_MS") &&
                    (numericValue < PUMP_MAX_ON_MS_MIN || numericValue > PUMP_MAX_ON_MS_MAX))
                {
                    Serial.println(F("ERROR PUMP_MAX_ON_MS range 5000..180000"));
                    return false;
                }

                if (!equalsIgnoreCase(name, "SLEEP_MAX_MS") &&
                    !equalsIgnoreCase(name, "IDLE_MAX_MS") &&
                    !equalsIgnoreCase(name, "PUMP_MAX_ON_MS"))
                {
                    Serial.println(F("ERROR unknown field. Type HELP."));
                    return false;
                }

                if (!trySetValue(name, numericValue))
                {
                    Serial.println(F("ERROR invalid config; unchanged."));
                    return false;
                }

                Serial.println(F("Applied (not yet saved; use SAVE)."));
                return true;
            }

            void printHelp()
            {
                Serial.println(F("Commands:"));
                Serial.println(F("  HELP                 - show this text"));
                Serial.println(F("  GET                  - show current config"));
                Serial.println(F("  SET <NAME> <VALUE>   - change a field (applied now, not saved)"));
                Serial.println(F("  SAVE                 - persist current config to EEPROM"));
                Serial.println(F("  RESET                - revert to compiled defaults (not saved)"));
                Serial.println(F("  TRACE ON             - log FSM state changes as they happen"));
                Serial.println(F("  TRACE ALL            - also log events that produced no change"));
                Serial.println(F("  TRACE OFF            - stop logging"));
                Serial.println(F("Fields: SLEEP_MAX_MS, IDLE_MAX_MS, PUMP_MAX_ON_MS"));
            }

            void printConfig()
            {
                Serial.print(F("SLEEP_MAX_MS="));
                Serial.println(config.sleepMaxDurationMs);
                Serial.print(F("IDLE_MAX_MS="));
                Serial.println(config.idleMaxDurationMs);
                Serial.print(F("PUMP_MAX_ON_MS="));
                Serial.println(config.pumpMaxOnDurationMs);
            }
        };

    } /* namespace ato */
} /* namespace xal */

#endif
