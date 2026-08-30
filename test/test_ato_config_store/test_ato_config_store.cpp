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
 * WHY BYTES ARE CORRUPTED DIRECTLY, NOT VIA AtoConfigStore's PRIVATE crc8()
 * ----------------------------------------------------------------------------
 * AtoConfigStore::crc8()/computeCrc8() are private, so these tests never
 * try to compute a "matching" CRC themselves. Instead: save() a known-good
 * config first (which stamps a correct magic/version/crc for that data),
 * then flip ONE specific byte (via offsetof() to avoid hardcoding struct
 * layout) with XOR 0xFF, which is guaranteed to differ from whatever was
 * there. Flipping the magic or version byte fails validation at that
 * specific check; flipping a payload byte (e.g. idleMaxDurationMs) leaves
 * magic/version looking fine but invalidates the CRC, since the CRC was
 * computed over the original (now-changed) bytes -- exactly simulating
 * real EEPROM bit rot / partial corruption.
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

using xal::ato::AtoConfig;
using xal::ato::AtoConfigStore;

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
} // namespace

/* ============================================================ */
/* Rejection paths: each must fall back to (and self-heal with)   */
/* the given defaults, never run with garbage/corrupt values.     */
/* ============================================================ */

void test_loadOrDefault_self_heals_from_blank_eeprom()
{
    blankEeprom();

    AtoConfig defaults = makeConfig(999001, 999002, 999003);
    AtoConfig result = AtoConfigStore::loadOrDefault(defaults);

    TEST_ASSERT_EQUAL_UINT32(999001, result.sleepMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(999002, result.idleMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(999003, result.pumpMaxOnDurationMs);

    /* Confirm the defaults were actually persisted (self-healed), not
       just returned in memory. */
    AtoConfig onDisk = readRawConfigFromEeprom();
    TEST_ASSERT_EQUAL_UINT16(xal::ato::CONFIG_MAGIC, onDisk.magic);
    TEST_ASSERT_EQUAL_UINT8(xal::ato::CONFIG_VERSION, onDisk.version);
    TEST_ASSERT_EQUAL_UINT32(999001, onDisk.sleepMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(999002, onDisk.idleMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(999003, onDisk.pumpMaxOnDurationMs);
}

void test_loadOrDefault_falls_back_when_magic_is_wrong()
{
    AtoConfig saved = makeConfig(111111, 222222, 333333);
    AtoConfigStore::save(saved);

    corruptByte(offsetof(AtoConfig, magic));

    AtoConfig defaults = makeConfig(999011, 999012, 999013);
    AtoConfig result = AtoConfigStore::loadOrDefault(defaults);

    TEST_ASSERT_EQUAL_UINT32(999011, result.sleepMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(999012, result.idleMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(999013, result.pumpMaxOnDurationMs);

    AtoConfig onDisk = readRawConfigFromEeprom();
    TEST_ASSERT_EQUAL_UINT32(999011, onDisk.sleepMaxDurationMs);
}

void test_loadOrDefault_falls_back_when_version_is_wrong()
{
    AtoConfig saved = makeConfig(111111, 222222, 333333);
    AtoConfigStore::save(saved);

    corruptByte(offsetof(AtoConfig, version));

    AtoConfig defaults = makeConfig(999021, 999022, 999023);
    AtoConfig result = AtoConfigStore::loadOrDefault(defaults);

    TEST_ASSERT_EQUAL_UINT32(999021, result.sleepMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(999022, result.idleMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(999023, result.pumpMaxOnDurationMs);
}

void test_loadOrDefault_falls_back_when_payload_data_is_corrupted()
{
    AtoConfig saved = makeConfig(111111, 222222, 333333);
    AtoConfigStore::save(saved);

    /* Magic and version bytes are untouched here -- only a payload field
       is corrupted, so this specifically exercises the CRC check, not the
       magic/version checks. */
    corruptByte(offsetof(AtoConfig, idleMaxDurationMs));

    AtoConfig defaults = makeConfig(999031, 999032, 999033);
    AtoConfig result = AtoConfigStore::loadOrDefault(defaults);

    TEST_ASSERT_EQUAL_UINT32(999031, result.sleepMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(999032, result.idleMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(999033, result.pumpMaxOnDurationMs);
}

/* ============================================================ */
/* Happy path: a valid saved config is actually loaded, not        */
/* silently replaced by defaults.                                  */
/* ============================================================ */

void test_loadOrDefault_returns_saved_config_when_valid()
{
    AtoConfig saved = makeConfig(444444, 555555, 666666);
    AtoConfigStore::save(saved);

    AtoConfig defaults = makeConfig(999041, 999042, 999043);
    AtoConfig result = AtoConfigStore::loadOrDefault(defaults);

    /* Must match the SAVED values, not the passed-in defaults -- proving
       the load-from-EEPROM path was actually taken, not the fallback. */
    TEST_ASSERT_EQUAL_UINT32(444444, result.sleepMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(555555, result.idleMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(666666, result.pumpMaxOnDurationMs);
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

    UNITY_END();
}

void loop()
{
}
