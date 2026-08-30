#ifndef com_github_xhuli_arduino_ato_AtoConfigConsole_H
#define com_github_xhuli_arduino_ato_AtoConfigConsole_H
#pragma once

#include <Arduino.h>
#include <Runnable.h>
#include <ctype.h>
#include <stdlib.h>
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
            typedef void (*ApplyCallback)(const AtoConfig &);

        private:
            static constexpr uint8_t BUFFER_SIZE = 48;

            AtoConfig &config;
            const AtoConfig &defaults;
            ApplyCallback applyCallback;

            char buffer[BUFFER_SIZE];
            uint8_t bufferLen = 0;

        public:
            /**
             * @param config Reference to the live, in-memory config to view/edit.
             * @param defaults Reference to the compiled-in defaults, used by RESET.
             * @param applyCallback Called with the current config after every
             * successful SET/RESET, so changes take effect immediately without
             * requiring a reboot. Typically pushes values into Timer/TimedSwitchable
             * setters.
             */
            AtoConfigConsole(AtoConfig &config, const AtoConfig &defaults, ApplyCallback applyCallback)
                : config(config), defaults(defaults), applyCallback(applyCallback)
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
                    char c = (char)Serial.read();

                    if (c == '\r')
                    {
                        continue;
                    }

                    if (c == '\n')
                    {
                        buffer[bufferLen] = '\0';
                        handleLine(buffer);
                        bufferLen = 0;
                    }
                    else if (c == '\b' || c == 0x7F) /* backspace or DEL */
                    {
                        /* Remove the last buffered character, matching what
                           the user's terminal already did on-screen. Without
                           this, a backspace byte would just get appended
                           literally into the buffer instead of erasing
                           anything, silently corrupting the line. */
                        if (bufferLen > 0)
                        {
                            bufferLen--;
                        }
                    }
                    else if (bufferLen < (BUFFER_SIZE - 1))
                    {
                        buffer[bufferLen++] = c;
                    }
                    else
                    {
                        /* Line too long for the buffer: drop it silently. */
                        bufferLen = 0;
                    }
                }
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

            void handleLine(char *line)
            {
                char *command = strtok(line, " ");
                if (command == nullptr)
                {
                    return;
                }

                if (equalsIgnoreCase(command, "HELP"))
                {
                    printHelp();
                }
                else if (equalsIgnoreCase(command, "GET"))
                {
                    printConfig();
                }
                else if (equalsIgnoreCase(command, "SET"))
                {
                    handleSet();
                }
                else if (equalsIgnoreCase(command, "SAVE"))
                {
                    AtoConfigStore::save(config);
                    Serial.println(F("Saved to EEPROM."));
                }
                else if (equalsIgnoreCase(command, "RESET"))
                {
                    config = defaults;
                    applyCallback(config);
                    Serial.println(F("Reset to compiled defaults (not yet saved; use SAVE)."));
                }
                else
                {
                    Serial.println(F("Unknown command. Type HELP."));
                }
            }

            /**
             * @brief Rejects anything that isn't purely digits, so a value
             * token corrupted by a stray control/escape byte (e.g. an arrow
             * key pressed mid-line, which this console cannot edit around)
             * produces a clear error instead of a silently wrong/partial
             * number being accepted.
             */
            static bool isValidUnsignedNumber(const char *value)
            {
                if (*value == '\0')
                {
                    return false;
                }
                for (const char *p = value; *p != '\0'; p++)
                {
                    if (!isdigit((unsigned char)*p))
                    {
                        return false;
                    }
                }
                return true;
            }

            void handleSet()
            {
                char *name = strtok(nullptr, " ");
                char *value = strtok(nullptr, " ");

                if (name == nullptr || value == nullptr)
                {
                    Serial.println(F("Usage: SET <NAME> <VALUE>. Type HELP."));
                    return;
                }

                if (!isValidUnsignedNumber(value))
                {
                    Serial.println(F("Invalid value: expected digits only (e.g. 5000). "
                                      "Did a stray key (e.g. an arrow key) get typed mid-line? "
                                      "This console has no line editing - retype the whole line."));
                    return;
                }

                uint32_t numericValue = strtoul(value, nullptr, 10);

                if (equalsIgnoreCase(name, "SLEEP_MAX_MS"))
                {
                    config.sleepMaxDurationMs = numericValue;
                }
                else if (equalsIgnoreCase(name, "IDLE_MAX_MS"))
                {
                    config.idleMaxDurationMs = numericValue;
                }
                else if (equalsIgnoreCase(name, "PUMP_MAX_ON_MS"))
                {
                    config.pumpMaxOnDurationMs = numericValue;
                }
                else
                {
                    Serial.println(F("Unknown field. Type HELP."));
                    return;
                }

                applyCallback(config);
                Serial.println(F("Applied (not yet saved; use SAVE)."));
            }

            void printHelp()
            {
                Serial.println(F("Commands:"));
                Serial.println(F("  HELP                 - show this text"));
                Serial.println(F("  GET                  - show current config"));
                Serial.println(F("  SET <NAME> <VALUE>   - change a field (applied now, not saved)"));
                Serial.println(F("  SAVE                 - persist current config to EEPROM"));
                Serial.println(F("  RESET                - revert to compiled defaults (not saved)"));
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
