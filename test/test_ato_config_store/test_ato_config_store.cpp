/**
 * Regression tests for xal::ato::AtoConfigStore's EEPROM load/save guard.
 *
 * CAUTION -- REAL EEPROM, NOT MOCKED
 * ------------------------------------
 * AtoConfigStore reads/writes actual chip EEPROM (there's no way to fake
 * this on real hardware). Running this suite WILL OVERWRITE whatever ATO
 * config is currently saved on the test board -- including any real
 * SLEEP_MAX_MS/IDLE_MAX_MS/PUMP_MAX_ON_MS tuning done via the Serial
 * console. Treat this as destructive to the board's current EEPROM
 * contents. EEPROM write-cycle wear (AVR is rated ~100k cycles) is not a
 * practical concern for occasional test runs -- this suite does at most a
 * handful of writes per run.
 *
 * WHAT'S BEING VERIFIED
 * ----------------------
 * The whole point of AtoConfigStore is: a blank, corrupt, or
 * version-mismatched EEPROM must never cause the device to run with
 * garbage timing values -- it must always fall back to (and self-heal
 * with) the compiled-in defaults. These tests exercise every rejection
 * path (blank chip, wrong magic, wrong version, corrupted payload data)
 * plus the happy path (a valid saved config is actually loaded, not
 * just defaulted).
 *
 * HOW EACH TEST ESTABLISHES ITS OWN STARTING STATE
 * ---------------------------------------------------
 * EEPROM persists across test runs and power cycles, so each test writes
 * its own known precondition first (via a real save() call, or by
 * deliberately blanking/corrupting bytes), rather than assuming anything
 * about EEPROM's current contents. This makes the tests order-independent
 * and repeatable regardless of prior history on the chip.
 *
 * HOW STRUCTURAL VS SEMANTIC INVALIDITY IS CREATED
 * -------------------------------------------------
 * Corruption-path tests save a known-good config and then flip one EEPROM
 * byte, exercising magic/version/CRC rejection. The semantic-validation
 * test intentionally writes an unsafe pump value with a matching CRC; it
 * duplicates the small CRC8 routine locally so the stored record remains
 * structurally valid and specifically exercises the new semantic guard.
 *
 * HOW TO RUN
 * ----------
 *     pio test -e nanoatmega328
 * (or -e megaatmega2560 if it doesn't fit -- this suite is small, no
 * AtoActions instances, so it's expected to fit the Nano directly.)
 */

#include <Arduino.h>
#include <EEPROM.h>
#include <stddef.h>
#include <unity.h>

#include <ato/AtoConfig.h>
#include <ato/AtoConfigConsole.h>

using xal::ato::AtoConfig;
using xal::ato::AtoConfigStore;
using xal::ato::AtoConfigConsole;
using xal::ato::applyValidatedAtoConfig;
using xal::ato::PUMP_MAX_ON_MS_MAX;
using xal::ato::PUMP_MAX_ON_MS_MIN;
using xal::ato::isValidAtoConfig;

namespace
{
    /**
     * @brief Builds an AtoConfig with the given real field values. magic/
     * version/crc8 are left zeroed -- AtoConfigStore::save()/loadOrDefault()
     * always stamp/validate those, callers never need to set them.
     */
    AtoConfig makeConfig(uint32_t sleepMs, uint32_t idleMs, uint32_t pumpMs)
    {
        AtoConfig config{};
        config.sleepMaxDurationMs = sleepMs;
        config.idleMaxDurationMs = idleMs;
        config.pumpMaxOnDurationMs = pumpMs;
        return config;
    }

    /**
     * @brief Reads whatever is currently on EEPROM at the config address,
     * bypassing AtoConfigStore's validation entirely -- used to confirm a
     * fallback/self-heal actually persisted to disk, not just returned an
     * in-memory value.
     */
    AtoConfig readRawConfigFromEeprom()
    {
        AtoConfig raw;
        EEPROM.get(xal::ato::CONFIG_EEPROM_ADDRESS, raw);
        return raw;
    }

    /**
     * @brief Flips every bit of the byte at the given field offset within
     * the config struct, guaranteeing it differs from whatever was there
     * (regardless of the original value), without needing to know or
     * assume EEPROM's prior contents.
     */
    void corruptByte(size_t fieldOffset)
    {
        int address = xal::ato::CONFIG_EEPROM_ADDRESS + static_cast<int>(fieldOffset);
        uint8_t original = EEPROM.read(address);
        EEPROM.write(address, original ^ 0xFF);
    }

    /**
     * @brief Simulates a chip whose EEPROM has never been written --
     * every byte in the config's address range set to 0xFF (the typical
     * erased-cell state), which fails the magic check immediately.
     */
    void blankEeprom()
    {
        for (size_t i = 0; i < sizeof(AtoConfig); i++)
        {
            EEPROM.write(xal::ato::CONFIG_EEPROM_ADDRESS + static_cast<int>(i), 0xFF);
        }
    }

    uint8_t computeTestCrc8(const AtoConfig &config)
    {
        const uint8_t *data = reinterpret_cast<const uint8_t *>(&config);
        const size_t len = sizeof(AtoConfig) - sizeof(config.crc8);
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

    void writeStructurallyValidConfigDirectly(AtoConfig config)
    {
        config.magic = xal::ato::CONFIG_MAGIC;
        config.version = xal::ato::CONFIG_VERSION;
        config.crc8 = computeTestCrc8(config);
        EEPROM.put(xal::ato::CONFIG_EEPROM_ADDRESS, config);
    }


    uint8_t consoleApplyCalls = 0;

    bool acceptValidConfig(const AtoConfig &config)
    {
        consoleApplyCalls++;
        return isValidAtoConfig(config);
    }

    struct FakeTimer
    {
        uint8_t calls = 0;
        uint32_t value = 0;

        void setDurationMs(uint32_t durationMs)
        {
            calls++;
            value = durationMs;
        }
    };

    struct FakePump
    {
        uint8_t calls = 0;
        uint32_t value = 12345;

        void setMaxOnTimeMs(uint32_t durationMs)
        {
            calls++;
            value = durationMs;
        }
    };
} // namespace

/* ============================================================ */
/* Rejection paths: each must fall back to (and self-heal with)   */
/* the given defaults, never run with garbage/corrupt values.     */
/* ============================================================ */

void test_loadOrDefault_self_heals_from_blank_eeprom()
{
    blankEeprom();

    AtoConfig defaults = makeConfig(999001, 999002, 90003);
    AtoConfig result = AtoConfigStore::loadOrDefault(defaults);

    TEST_ASSERT_EQUAL_UINT32(999001, result.sleepMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(999002, result.idleMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(90003, result.pumpMaxOnDurationMs);

    /* Confirm the defaults were actually persisted (self-healed), not
       just returned in memory. */
    AtoConfig onDisk = readRawConfigFromEeprom();
    TEST_ASSERT_EQUAL_UINT16(xal::ato::CONFIG_MAGIC, onDisk.magic);
    TEST_ASSERT_EQUAL_UINT8(xal::ato::CONFIG_VERSION, onDisk.version);
    TEST_ASSERT_EQUAL_UINT32(999001, onDisk.sleepMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(999002, onDisk.idleMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(90003, onDisk.pumpMaxOnDurationMs);
}

void test_loadOrDefault_falls_back_when_magic_is_wrong()
{
    AtoConfig saved = makeConfig(111111, 222222, 120000);
    AtoConfigStore::save(saved);

    corruptByte(offsetof(AtoConfig, magic));

    AtoConfig defaults = makeConfig(999011, 999012, 90013);
    AtoConfig result = AtoConfigStore::loadOrDefault(defaults);

    TEST_ASSERT_EQUAL_UINT32(999011, result.sleepMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(999012, result.idleMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(90013, result.pumpMaxOnDurationMs);

    AtoConfig onDisk = readRawConfigFromEeprom();
    TEST_ASSERT_EQUAL_UINT32(999011, onDisk.sleepMaxDurationMs);
}

void test_loadOrDefault_falls_back_when_version_is_wrong()
{
    AtoConfig saved = makeConfig(111111, 222222, 120000);
    AtoConfigStore::save(saved);

    corruptByte(offsetof(AtoConfig, version));

    AtoConfig defaults = makeConfig(999021, 999022, 90023);
    AtoConfig result = AtoConfigStore::loadOrDefault(defaults);

    TEST_ASSERT_EQUAL_UINT32(999021, result.sleepMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(999022, result.idleMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(90023, result.pumpMaxOnDurationMs);
}

void test_loadOrDefault_falls_back_when_payload_data_is_corrupted()
{
    AtoConfig saved = makeConfig(111111, 222222, 120000);
    AtoConfigStore::save(saved);

    /* Magic and version bytes are untouched here -- only a payload field
       is corrupted, so this specifically exercises the CRC check, not the
       magic/version checks. */
    corruptByte(offsetof(AtoConfig, idleMaxDurationMs));

    AtoConfig defaults = makeConfig(999031, 999032, 90033);
    AtoConfig result = AtoConfigStore::loadOrDefault(defaults);

    TEST_ASSERT_EQUAL_UINT32(999031, result.sleepMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(999032, result.idleMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(90033, result.pumpMaxOnDurationMs);
}

/* ============================================================ */
/* Happy path: a valid saved config is actually loaded, not        */
/* silently replaced by defaults.                                  */
/* ============================================================ */

void test_loadOrDefault_returns_saved_config_when_valid()
{
    AtoConfig saved = makeConfig(444444, 555555, 100000);
    AtoConfigStore::save(saved);

    AtoConfig defaults = makeConfig(999041, 999042, 90043);
    AtoConfig result = AtoConfigStore::loadOrDefault(defaults);

    /* Must match the SAVED values, not the passed-in defaults -- proving
       the load-from-EEPROM path was actually taken, not the fallback. */
    TEST_ASSERT_EQUAL_UINT32(444444, result.sleepMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(555555, result.idleMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(100000, result.pumpMaxOnDurationMs);
}

/* ============================================================ */
/* Semantic pump safety validation                                */
/* ============================================================ */

void test_pump_max_on_safety_bounds()
{
    TEST_ASSERT_FALSE(isValidAtoConfig(makeConfig(1, 1, 0)));
    TEST_ASSERT_FALSE(isValidAtoConfig(makeConfig(1, 1, PUMP_MAX_ON_MS_MIN - 1)));
    TEST_ASSERT_TRUE(isValidAtoConfig(makeConfig(1, 1, PUMP_MAX_ON_MS_MIN)));
    TEST_ASSERT_TRUE(isValidAtoConfig(makeConfig(1, 1, 90000)));
    TEST_ASSERT_TRUE(isValidAtoConfig(makeConfig(1, 1, PUMP_MAX_ON_MS_MAX)));
    TEST_ASSERT_FALSE(isValidAtoConfig(makeConfig(1, 1, PUMP_MAX_ON_MS_MAX + 1)));
}

void test_save_rejects_invalid_pump_timeout()
{
    AtoConfig valid = makeConfig(101, 102, 90000);
    TEST_ASSERT_TRUE(AtoConfigStore::save(valid));
    AtoConfig before = readRawConfigFromEeprom();

    AtoConfig invalid = makeConfig(201, 202, 0);
    TEST_ASSERT_FALSE(AtoConfigStore::save(invalid));

    AtoConfig after = readRawConfigFromEeprom();
    TEST_ASSERT_EQUAL_MEMORY(&before, &after, sizeof(AtoConfig));
}

void test_loadOrDefault_falls_back_when_crc_valid_but_pump_timeout_is_unsafe()
{
    AtoConfig unsafe = makeConfig(301, 302, 0);
    writeStructurallyValidConfigDirectly(unsafe);

    AtoConfig defaults = makeConfig(401, 402, 90000);
    AtoConfig result = AtoConfigStore::loadOrDefault(defaults);

    TEST_ASSERT_EQUAL_UINT32(401, result.sleepMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(402, result.idleMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(90000, result.pumpMaxOnDurationMs);

    AtoConfig onDisk = readRawConfigFromEeprom();
    TEST_ASSERT_EQUAL_UINT32(90000, onDisk.pumpMaxOnDurationMs);
    TEST_ASSERT_TRUE(isValidAtoConfig(onDisk));
}

void test_console_invalid_pump_set_does_not_modify_active_config()
{
    AtoConfig active = makeConfig(501, 502, 90000);
    const AtoConfig defaults = active;
    bool traceEnabled = false;
    bool traceVerbose = false;
    consoleApplyCalls = 0;
    AtoConfigConsole console(active, defaults, acceptValidConfig, traceEnabled, traceVerbose);

    TEST_ASSERT_FALSE(console.trySetValue("PUMP_MAX_ON_MS", 0));
    TEST_ASSERT_EQUAL_UINT32(90000, active.pumpMaxOnDurationMs);
    TEST_ASSERT_EQUAL_UINT8(0, consoleApplyCalls);
}

void test_console_valid_pump_set_is_accepted()
{
    AtoConfig active = makeConfig(511, 512, 90000);
    const AtoConfig defaults = active;
    bool traceEnabled = false;
    bool traceVerbose = false;
    consoleApplyCalls = 0;
    AtoConfigConsole console(active, defaults, acceptValidConfig, traceEnabled, traceVerbose);

    TEST_ASSERT_TRUE(console.trySetValue("PUMP_MAX_ON_MS", 5000));
    TEST_ASSERT_EQUAL_UINT32(5000, active.pumpMaxOnDurationMs);
    TEST_ASSERT_EQUAL_UINT8(1, consoleApplyCalls);
}

void test_invalid_config_cannot_reach_pump_timeout_setter()
{
    AtoConfig invalid = makeConfig(601, 602, 0);
    FakeTimer sleepTimer;
    FakeTimer idleTimer;
    FakePump pump;

    TEST_ASSERT_FALSE(applyValidatedAtoConfig(invalid, sleepTimer, idleTimer, pump));
    TEST_ASSERT_EQUAL_UINT8(0, sleepTimer.calls);
    TEST_ASSERT_EQUAL_UINT8(0, idleTimer.calls);
    TEST_ASSERT_EQUAL_UINT8(0, pump.calls);
    TEST_ASSERT_NOT_EQUAL(0, pump.value);
}

void test_valid_config_reaches_all_runtime_setters()
{
    AtoConfig valid = makeConfig(611, 612, 180000);
    FakeTimer sleepTimer;
    FakeTimer idleTimer;
    FakePump pump;

    TEST_ASSERT_TRUE(applyValidatedAtoConfig(valid, sleepTimer, idleTimer, pump));
    TEST_ASSERT_EQUAL_UINT32(611, sleepTimer.value);
    TEST_ASSERT_EQUAL_UINT32(612, idleTimer.value);
    TEST_ASSERT_EQUAL_UINT32(180000, pump.value);
}

/* ============================================================ */
/* Unity runner                                                   */
/* ============================================================ */

void setup()
{
    delay(2000);

    UNITY_BEGIN();

    RUN_TEST(test_loadOrDefault_self_heals_from_blank_eeprom);
    RUN_TEST(test_loadOrDefault_falls_back_when_magic_is_wrong);
    RUN_TEST(test_loadOrDefault_falls_back_when_version_is_wrong);
    RUN_TEST(test_loadOrDefault_falls_back_when_payload_data_is_corrupted);
    RUN_TEST(test_loadOrDefault_returns_saved_config_when_valid);
    RUN_TEST(test_pump_max_on_safety_bounds);
    RUN_TEST(test_save_rejects_invalid_pump_timeout);
    RUN_TEST(test_loadOrDefault_falls_back_when_crc_valid_but_pump_timeout_is_unsafe);
    RUN_TEST(test_console_invalid_pump_set_does_not_modify_active_config);
    RUN_TEST(test_console_valid_pump_set_is_accepted);
    RUN_TEST(test_invalid_config_cannot_reach_pump_timeout_setter);
    RUN_TEST(test_valid_config_reaches_all_runtime_setters);

    UNITY_END();
}

void loop()
{
}
