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
/* P1 serial-console parser hardening                             */
/* ============================================================ */

void test_parse_uint32_boundaries_and_invalid_tokens()
{
    uint32_t value = 123;

    TEST_ASSERT_TRUE(AtoConfigConsole::parseUint32("0", value));
    TEST_ASSERT_EQUAL_UINT32(0, value);
    TEST_ASSERT_TRUE(AtoConfigConsole::parseUint32("4294967295", value));
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, value);

    TEST_ASSERT_FALSE(AtoConfigConsole::parseUint32("4294967296", value));
    TEST_ASSERT_FALSE(AtoConfigConsole::parseUint32("999999999999999999999999999999999", value));
    TEST_ASSERT_FALSE(AtoConfigConsole::parseUint32("-1", value));
    TEST_ASSERT_FALSE(AtoConfigConsole::parseUint32("+1", value));
    TEST_ASSERT_FALSE(AtoConfigConsole::parseUint32("12abc", value));
    TEST_ASSERT_FALSE(AtoConfigConsole::parseUint32("", value));
    TEST_ASSERT_FALSE(AtoConfigConsole::parseUint32(nullptr, value));
}

void test_console_numeric_parse_failure_has_no_side_effect()
{
    AtoConfig active = makeConfig(701, 702, 90000);
    const AtoConfig defaults = active;
    bool traceEnabled = false;
    bool traceVerbose = false;
    consoleApplyCalls = 0;
    AtoConfigConsole console(active, defaults, acceptValidConfig, traceEnabled, traceVerbose);

    char overflow[] = "SET SLEEP_MAX_MS 4294967296";
    TEST_ASSERT_FALSE(console.executeLine(overflow));
    TEST_ASSERT_EQUAL_UINT32(701, active.sleepMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT8(0, consoleApplyCalls);

    char malformed[] = "SET SLEEP_MAX_MS 12abc";
    TEST_ASSERT_FALSE(console.executeLine(malformed));
    TEST_ASSERT_EQUAL_UINT32(701, active.sleepMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT8(0, consoleApplyCalls);
}

void test_console_accepts_full_uint32_for_unbounded_timer_field()
{
    AtoConfig active = makeConfig(711, 712, 90000);
    const AtoConfig defaults = active;
    bool traceEnabled = false;
    bool traceVerbose = false;
    consoleApplyCalls = 0;
    AtoConfigConsole console(active, defaults, acceptValidConfig, traceEnabled, traceVerbose);

    char command[] = "SET SLEEP_MAX_MS 4294967295";
    TEST_ASSERT_TRUE(console.executeLine(command));
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, active.sleepMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT8(1, consoleApplyCalls);
}

void test_console_strict_arity_rejects_extra_arguments_without_side_effects()
{
    AtoConfig active = makeConfig(721, 722, 90000);
    const AtoConfig defaults = makeConfig(731, 732, 100000);
    bool traceEnabled = false;
    bool traceVerbose = false;
    consoleApplyCalls = 0;
    AtoConfigConsole console(active, defaults, acceptValidConfig, traceEnabled, traceVerbose);

    AtoConfig saved = makeConfig(741, 742, 110000);
    TEST_ASSERT_TRUE(AtoConfigStore::save(saved));
    AtoConfig eepromBefore = readRawConfigFromEeprom();

    char getExtra[] = "GET extra";
    TEST_ASSERT_FALSE(console.executeLine(getExtra));

    char saveExtra[] = "SAVE extra";
    TEST_ASSERT_FALSE(console.executeLine(saveExtra));
    AtoConfig eepromAfter = readRawConfigFromEeprom();
    TEST_ASSERT_EQUAL_MEMORY(&eepromBefore, &eepromAfter, sizeof(AtoConfig));

    char resetExtra[] = "RESET extra";
    TEST_ASSERT_FALSE(console.executeLine(resetExtra));
    TEST_ASSERT_EQUAL_UINT32(721, active.sleepMaxDurationMs);

    char traceExtra[] = "TRACE ON extra";
    TEST_ASSERT_FALSE(console.executeLine(traceExtra));
    TEST_ASSERT_FALSE(traceEnabled);
    TEST_ASSERT_FALSE(traceVerbose);

    char setExtra[] = "SET PUMP_MAX_ON_MS 5000 extra";
    TEST_ASSERT_FALSE(console.executeLine(setExtra));
    TEST_ASSERT_EQUAL_UINT32(90000, active.pumpMaxOnDurationMs);
    TEST_ASSERT_EQUAL_UINT8(0, consoleApplyCalls);
}

void test_console_valid_command_forms_still_work()
{
    AtoConfig active = makeConfig(751, 752, 90000);
    const AtoConfig defaults = makeConfig(761, 762, 100000);
    bool traceEnabled = false;
    bool traceVerbose = false;
    consoleApplyCalls = 0;
    AtoConfigConsole console(active, defaults, acceptValidConfig, traceEnabled, traceVerbose);

    char help[] = "HELP";
    TEST_ASSERT_TRUE(console.executeLine(help));

    char get[] = "GET";
    TEST_ASSERT_TRUE(console.executeLine(get));

    char set[] = "SET PUMP_MAX_ON_MS 5000";
    TEST_ASSERT_TRUE(console.executeLine(set));
    TEST_ASSERT_EQUAL_UINT32(5000, active.pumpMaxOnDurationMs);

    char trace[] = "TRACE ON";
    TEST_ASSERT_TRUE(console.executeLine(trace));
    TEST_ASSERT_TRUE(traceEnabled);
    TEST_ASSERT_FALSE(traceVerbose);

    char save[] = "SAVE";
    TEST_ASSERT_TRUE(console.executeLine(save));
    AtoConfig saved = readRawConfigFromEeprom();
    TEST_ASSERT_EQUAL_UINT32(5000, saved.pumpMaxOnDurationMs);

    char reset[] = "RESET";
    TEST_ASSERT_TRUE(console.executeLine(reset));
    TEST_ASSERT_EQUAL_UINT32(761, active.sleepMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(100000, active.pumpMaxOnDurationMs);
}

void test_console_p0_pump_bounds_regression_through_parser()
{
    AtoConfig active = makeConfig(771, 772, 90000);
    const AtoConfig defaults = active;
    bool traceEnabled = false;
    bool traceVerbose = false;
    consoleApplyCalls = 0;
    AtoConfigConsole console(active, defaults, acceptValidConfig, traceEnabled, traceVerbose);

    char zero[] = "SET PUMP_MAX_ON_MS 0";
    TEST_ASSERT_FALSE(console.executeLine(zero));
    char below[] = "SET PUMP_MAX_ON_MS 4999";
    TEST_ASSERT_FALSE(console.executeLine(below));
    char above[] = "SET PUMP_MAX_ON_MS 180001";
    TEST_ASSERT_FALSE(console.executeLine(above));
    TEST_ASSERT_EQUAL_UINT32(90000, active.pumpMaxOnDurationMs);
    TEST_ASSERT_EQUAL_UINT8(0, consoleApplyCalls);

    char minValue[] = "SET PUMP_MAX_ON_MS 5000";
    TEST_ASSERT_TRUE(console.executeLine(minValue));
    char maxValue[] = "SET PUMP_MAX_ON_MS 180000";
    TEST_ASSERT_TRUE(console.executeLine(maxValue));
    TEST_ASSERT_EQUAL_UINT32(180000, active.pumpMaxOnDurationMs);
    TEST_ASSERT_EQUAL_UINT8(2, consoleApplyCalls);
}

void test_console_other_malformed_commands_have_no_side_effects()
{
    AtoConfig active = makeConfig(801, 802, 90000);
    const AtoConfig defaults = makeConfig(811, 812, 100000);
    bool traceEnabled = false;
    bool traceVerbose = false;
    consoleApplyCalls = 0;
    AtoConfigConsole console(active, defaults, acceptValidConfig, traceEnabled, traceVerbose);

    char missingValue[] = "SET PUMP_MAX_ON_MS";
    TEST_ASSERT_FALSE(console.executeLine(missingValue));

    char unknownField[] = "SET UNKNOWN 5000";
    TEST_ASSERT_FALSE(console.executeLine(unknownField));

    char invalidTrace[] = "TRACE MAYBE";
    TEST_ASSERT_FALSE(console.executeLine(invalidTrace));

    char unknownCommand[] = "BOGUS";
    TEST_ASSERT_FALSE(console.executeLine(unknownCommand));

    TEST_ASSERT_EQUAL_UINT32(801, active.sleepMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(802, active.idleMaxDurationMs);
    TEST_ASSERT_EQUAL_UINT32(90000, active.pumpMaxOnDurationMs);
    TEST_ASSERT_FALSE(traceEnabled);
    TEST_ASSERT_FALSE(traceVerbose);
    TEST_ASSERT_EQUAL_UINT8(0, consoleApplyCalls);
}

void test_console_oversized_line_is_discarded_once_and_next_command_works()
{
    AtoConfig active = makeConfig(781, 782, 90000);
    const AtoConfig defaults = active;
    bool traceEnabled = false;
    bool traceVerbose = false;
    consoleApplyCalls = 0;
    AtoConfigConsole console(active, defaults, acceptValidConfig, traceEnabled, traceVerbose);

    uint8_t overflowSignals = 0;
    for (uint8_t i = 0; i < 48; ++i)
    {
        if (console.processInputChar('X') == AtoConfigConsole::INPUT_LINE_TOO_LONG)
        {
            overflowSignals++;
        }
    }

    const char *suffix = "SET PUMP_MAX_ON_MS 5000";
    while (*suffix)
    {
        if (console.processInputChar(*suffix++) == AtoConfigConsole::INPUT_LINE_TOO_LONG)
        {
            overflowSignals++;
        }
    }
    TEST_ASSERT_EQUAL_UINT8(1, overflowSignals);
    TEST_ASSERT_EQUAL_UINT32(90000, active.pumpMaxOnDurationMs);
    TEST_ASSERT_EQUAL_UINT8(0, consoleApplyCalls);

    TEST_ASSERT_EQUAL(AtoConfigConsole::INPUT_NONE, console.processInputChar('\n'));

    const char *valid = "SET PUMP_MAX_ON_MS 5000";
    while (*valid)
    {
        TEST_ASSERT_EQUAL(AtoConfigConsole::INPUT_NONE, console.processInputChar(*valid++));
    }
    TEST_ASSERT_EQUAL(AtoConfigConsole::INPUT_LINE_READY, console.processInputChar('\n'));
    TEST_ASSERT_TRUE(console.executeBufferedLine());
    TEST_ASSERT_EQUAL_UINT32(5000, active.pumpMaxOnDurationMs);
}

void test_console_max_length_crlf_and_backspace_behavior()
{
    AtoConfig active = makeConfig(791, 792, 90000);
    const AtoConfig defaults = active;
    bool traceEnabled = false;
    bool traceVerbose = false;
    AtoConfigConsole console(active, defaults, acceptValidConfig, traceEnabled, traceVerbose);

    const char *help = "HELP";
    while (*help) TEST_ASSERT_EQUAL(AtoConfigConsole::INPUT_NONE, console.processInputChar(*help++));
    for (uint8_t i = 0; i < 43; ++i)
        TEST_ASSERT_EQUAL(AtoConfigConsole::INPUT_NONE, console.processInputChar(' '));
    TEST_ASSERT_EQUAL(AtoConfigConsole::INPUT_LINE_READY, console.processInputChar('\n'));
    TEST_ASSERT_TRUE(console.executeBufferedLine());

    const char *typo = "GETX";
    while (*typo) TEST_ASSERT_EQUAL(AtoConfigConsole::INPUT_NONE, console.processInputChar(*typo++));
    TEST_ASSERT_EQUAL(AtoConfigConsole::INPUT_NONE, console.processInputChar('\b'));
    TEST_ASSERT_EQUAL(AtoConfigConsole::INPUT_LINE_READY, console.processInputChar('\n'));
    TEST_ASSERT_TRUE(console.executeBufferedLine());

    for (uint8_t i = 0; i < 48; ++i)
        console.processInputChar('Y');
    TEST_ASSERT_EQUAL(AtoConfigConsole::INPUT_NONE, console.processInputChar('\b'));
    TEST_ASSERT_EQUAL(AtoConfigConsole::INPUT_NONE, console.processInputChar('\r'));
    TEST_ASSERT_EQUAL(AtoConfigConsole::INPUT_NONE, console.processInputChar('\n'));

    const char *get = "GET";
    while (*get) TEST_ASSERT_EQUAL(AtoConfigConsole::INPUT_NONE, console.processInputChar(*get++));
    TEST_ASSERT_EQUAL(AtoConfigConsole::INPUT_NONE, console.processInputChar('\r'));
    TEST_ASSERT_EQUAL(AtoConfigConsole::INPUT_LINE_READY, console.processInputChar('\n'));
    TEST_ASSERT_TRUE(console.executeBufferedLine());
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
    RUN_TEST(test_parse_uint32_boundaries_and_invalid_tokens);
    RUN_TEST(test_console_numeric_parse_failure_has_no_side_effect);
    RUN_TEST(test_console_accepts_full_uint32_for_unbounded_timer_field);
    RUN_TEST(test_console_strict_arity_rejects_extra_arguments_without_side_effects);
    RUN_TEST(test_console_valid_command_forms_still_work);
    RUN_TEST(test_console_p0_pump_bounds_regression_through_parser);
    RUN_TEST(test_console_other_malformed_commands_have_no_side_effects);
    RUN_TEST(test_console_oversized_line_is_discarded_once_and_next_command_works);
    RUN_TEST(test_console_max_length_crlf_and_backspace_behavior);

    UNITY_END();
}

void loop()
{
}
