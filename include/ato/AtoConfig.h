#ifndef com_github_xhuli_arduino_ato_AtoConfig_H
#define com_github_xhuli_arduino_ato_AtoConfig_H
#pragma once

#include <Arduino.h>
#include <EEPROM.h>
#include <string.h>

namespace xal
{
    namespace ato
    {

        constexpr int CONFIG_EEPROM_ADDRESS = 0;
        constexpr uint16_t CONFIG_MAGIC = 0xA7F0;
        constexpr uint8_t CONFIG_VERSION = 1;

        /**
         * @brief Runtime-tunable ATO timing configuration, persisted to EEPROM.
         * @details Packed to guarantee a fixed, deterministic on-disk layout.
         * magic/version/crc8 are stamped and checked by AtoConfigStore — callers
         * should not need to set them directly.
         *
         * Deliberately scoped to the three values that map onto existing
         * setters (Timer::setDurationMs(), TimedSwitchable::setMaxOnTimeMs()).
         * Button debounce/long-press and sensor push interval would need small
         * setter additions to PushButton.h/LiquidLevelSensor.h first (currently
         * constructor-only) — left for a follow-up.
         */
        struct __attribute__((packed)) AtoConfig
        {
            uint16_t magic;
            uint8_t version;
            uint32_t sleepMaxDurationMs;
            uint32_t idleMaxDurationMs;
            uint32_t pumpMaxOnDurationMs;
            uint8_t crc8;
        };

        /**
         * @class AtoConfigStore
         * @brief Loads/saves AtoConfig to EEPROM with a magic number + version +
         * CRC8 guard, so a corrupt, blank, or version-mismatched EEPROM can
         * never cause the device to run with garbage timing values — it always
         * falls back to (and self-heals with) the compiled-in defaults instead.
         */
        class AtoConfigStore
        {
        public:
            /**
             * @brief Loads config from EEPROM if it passes validation;
             * otherwise writes the given defaults to EEPROM and returns them.
             */
            static AtoConfig loadOrDefault(const AtoConfig &defaults)
            {
                AtoConfig loaded;
                EEPROM.get(CONFIG_EEPROM_ADDRESS, loaded);

                if (isValid(loaded))
                {
                    return loaded;
                }

                AtoConfig safe = defaults;
                save(safe);
                return safe;
            }

            /**
             * @brief Persists config to EEPROM (stamping magic/version/crc8
             * first), skipping the actual write if nothing changed, to limit
             * EEPROM wear from repeated saves of identical values.
             */
            static void save(AtoConfig &config)
            {
                config.magic = CONFIG_MAGIC;
                config.version = CONFIG_VERSION;
                config.crc8 = computeCrc8(config);

                AtoConfig current;
                EEPROM.get(CONFIG_EEPROM_ADDRESS, current);

                if (memcmp(&current, &config, sizeof(AtoConfig)) != 0)
                {
                    EEPROM.put(CONFIG_EEPROM_ADDRESS, config);
                }
            }

        private:
            static bool isValid(const AtoConfig &config)
            {
                if (config.magic != CONFIG_MAGIC)
                {
                    return false;
                }
                if (config.version != CONFIG_VERSION)
                {
                    return false;
                }
                return computeCrc8(config) == config.crc8;
            }

            static uint8_t computeCrc8(const AtoConfig &config)
            {
                return crc8(reinterpret_cast<const uint8_t *>(&config),
                    sizeof(AtoConfig) - sizeof(config.crc8));
            }

            /**
             * @brief Dallas/Maxim CRC8 (poly 0x8C, bit-reversed), computed
             * byte-by-byte with no lookup table — small code size, more than
             * adequate for guarding a ~12-byte config struct.
             */
            static uint8_t crc8(const uint8_t *data, size_t len)
            {
                uint8_t crc = 0x00;
                for (size_t i = 0; i < len; i++)
                {
                    uint8_t inByte = data[i];
                    for (uint8_t bit = 0; bit < 8; bit++)
                    {
                        uint8_t mix = (crc ^ inByte) & 0x01;
                        crc >>= 1;
                        if (mix)
                        {
                            crc ^= 0x8C;
                        }
                        inByte >>= 1;
                    }
                }
                return crc;
            }
        };

    } /* namespace ato */
} /* namespace xal */

#endif
