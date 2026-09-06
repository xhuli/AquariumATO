#ifndef com_github_xhuli_arduino_ato_AtoConfig_H
#define com_github_xhuli_arduino_ato_AtoConfig_H
#pragma once

#include <Arduino.h>
#include <EEPROM.h>
#include <string.h>
#include <Duration.h>

namespace xal {
    namespace ato {

        constexpr int CONFIG_EEPROM_ADDRESS = 0;
        constexpr uint16_t CONFIG_MAGIC = 0xA7F0;
        constexpr uint8_t CONFIG_VERSION = 1;

        /**
         * Pump runtime safety envelope. Zero is deliberately excluded because
         * TimedSwitchable interprets 0 as "no automatic timeout". Keeping the
         * configured maximum within 5..180 seconds provides an independent
         * pump-protection failsafe even if expected sensor transitions do not
         * occur.
         */
        static constexpr uint32_t PUMP_MAX_ON_MS_MIN = xal::duration::SECONDS_5;
        static constexpr uint32_t PUMP_MAX_ON_MS_MAX = xal::duration::MINUTES_3;

        /**
         * Advisory (non-rejecting) floor for the sleep/idle timers. A value
         * of 0 is rejected outright (Timer treats 0 as "always elapsed", so
         * the timer would fire every loop); a nonzero value below this
         * threshold is legal but makes the timer elapse within a minute of
         * entering its state, which is almost always a mistake. The console
         * prints a WARN line for values in (0, TIMER_MS_ADVISORY_MIN).
         */
        static constexpr uint32_t TIMER_MS_ADVISORY_MIN = xal::duration::MINUTES_1;

        inline bool isBelowTimerAdvisoryMin(uint32_t ms) {
            return ms > 0 && ms < TIMER_MS_ADVISORY_MIN;
        }

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
        struct __attribute__((packed)) AtoConfig {
            uint16_t magic;
            uint8_t version;
            uint32_t sleepMaxDurationMs;
            uint32_t idleMaxDurationMs;
            uint32_t pumpMaxOnDurationMs;
            uint8_t crc8;
        };

        /**
         * @brief Authoritative semantic validation for runtime ATO config.
         * @details Structural EEPROM metadata (magic/version/CRC) is validated
         * separately by AtoConfigStore. All configuration sources must satisfy
         * these runtime safety invariants before values are applied or saved.
         */
        inline bool isValidAtoConfig(const AtoConfig &config) {
            /* Timer treats 0 as "always elapsed", so a zero sleep/idle max
               would fire its timer every loop - reject it. Small-but-nonzero
               values are allowed (see isBelowTimerAdvisoryMin). */
            return config.sleepMaxDurationMs > 0 &&
                   config.idleMaxDurationMs > 0 &&
                   config.pumpMaxOnDurationMs >= PUMP_MAX_ON_MS_MIN &&
                   config.pumpMaxOnDurationMs <= PUMP_MAX_ON_MS_MAX;
        }

        /**
         * @brief Applies a validated config to the three runtime timing sinks.
         * @return false without invoking any setter when semantic validation
         * fails. This prevents an unsafe pump timeout (especially zero) from
         * reaching TimedSwitchable::setMaxOnTimeMs().
         */
        template <typename SleepTimerT, typename IdleTimerT, typename WaterPumpT>
        bool applyValidatedAtoConfig(
            const AtoConfig &config,
            SleepTimerT &sleepTimer,
            IdleTimerT &idleTimer,
            WaterPumpT &waterPump) {
            if (!isValidAtoConfig(config)) {
                return false;
            }

            sleepTimer.setDurationMs(config.sleepMaxDurationMs);
            idleTimer.setDurationMs(config.idleMaxDurationMs);
            waterPump.setMaxOnTimeMs(config.pumpMaxOnDurationMs);
            return true;
        }

        /**
         * @class AtoConfigStore
         * @brief Loads/saves AtoConfig to EEPROM with a magic number + version +
         * CRC8 guard, so a corrupt, blank, or version-mismatched EEPROM can
         * never cause the device to run with garbage timing values — it always
         * falls back to (and self-heals with) the compiled-in defaults instead.
         */
        class AtoConfigStore {
        public:
            /**
             * @brief Loads config from EEPROM if it passes validation;
             * otherwise writes the given defaults to EEPROM and returns them.
             */
            static AtoConfig loadOrDefault(const AtoConfig &defaults) {
                AtoConfig loaded{};
                EEPROM.get(CONFIG_EEPROM_ADDRESS, loaded);

                if (isStructurallyValid(loaded) && isValidAtoConfig(loaded)) {
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
            static bool save(AtoConfig &config) {
                if (!isValidAtoConfig(config)) {
                    return false;
                }

                config.magic = CONFIG_MAGIC;
                config.version = CONFIG_VERSION;
                config.crc8 = computeCrc8(config);

                AtoConfig current{};
                EEPROM.get(CONFIG_EEPROM_ADDRESS, current);

                if (memcmp(&current, &config, sizeof(AtoConfig)) != 0) {
                    EEPROM.put(CONFIG_EEPROM_ADDRESS, config);
                }
                return true;
            }

        private:
            static bool isStructurallyValid(const AtoConfig &config) {
                if (config.magic != CONFIG_MAGIC) {
                    return false;
                }
                if (config.version != CONFIG_VERSION) {
                    return false;
                }
                return computeCrc8(config) == config.crc8;
            }

            static uint8_t computeCrc8(const AtoConfig &config) {
                return crc8(reinterpret_cast<const uint8_t *>(&config),
                            sizeof(AtoConfig) - sizeof(config.crc8));
            }

            /**
             * @brief Dallas/Maxim CRC8 (poly 0x8C, bit-reversed), computed
             * byte-by-byte with no lookup table — small code size, more than
             * adequate for guarding a ~12-byte config struct.
             */
            static uint8_t crc8(const uint8_t *data, size_t len) {
                uint8_t crc = 0x00;
                for (size_t i = 0; i < len; i++) {
                    uint8_t inByte = data[i];
                    for (uint8_t bit = 0; bit < 8; bit++) {
                        uint8_t mix = (crc ^ inByte) & 0x01;
                        crc >>= 1;
                        if (mix) {
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
