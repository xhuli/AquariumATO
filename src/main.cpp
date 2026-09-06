#include "ato/AtoActions.h"
#include "ato/AtoConfig.h"
#include "ato/AtoConfigConsole.h"
#include "ato/AtoFsm.h"

#include <Arduino.h>
#include <avr/wdt.h>

#include <CyclicSwitchable.h>
#include <Duration.h>
#include <LiquidLevelSensor.h>
#include <PushButton.h>
#include <Runnable.h>
#include <TimedSwitchable.h>
#include <Timer.h>

/* << Constants >> */

namespace ato {
    namespace McuPin {
        constexpr uint8_t RedLed = 12;
        constexpr uint8_t YellowLed = 11;
        constexpr uint8_t GreenLed = 10;
        constexpr uint8_t WaterPump = 4;
        constexpr uint8_t PushButton = 3;
        constexpr uint8_t Buzzer = 2;

        constexpr uint8_t NormalLiquidLevelSensor = PIN_A0; /* 14 */

#ifdef ATO_HAS_LOW_SENSOR
        constexpr uint8_t LowLiquidLevelSensor = PIN_A1; /* 15 */
#endif

        constexpr uint8_t HighLiquidLevelSensor = PIN_A2; /* 16 */

#ifdef ATO_HAS_RESERVOIR_SENSOR
        constexpr uint8_t ReservoirLowLevelSensor = PIN_A3; /* 17 */
#endif

    }; /* namespace McuPin */
}; /* namespace ato */

constexpr uint8_t WHEN_ON_PIN_HIGH = HIGH;
constexpr uint8_t WHEN_ON_PIN_LOW = LOW;

constexpr uint8_t INITIAL_READING_HIGH = HIGH;
constexpr uint8_t INITIAL_READING_LOW = LOW;
constexpr uint32_t PERIODIC_PUSH_READING_DISABLED = xal::duration::SECONDS_0;
constexpr uint32_t PERIODIC_PUSH_READING_PERIOD = xal::duration::SECONDS_2;

/* Compiled-in defaults, used to seed EEPROM the first time (or whenever
 * EEPROM is blank/corrupt/version-mismatched). Once saved, the live values
 * come from AtoConfig (see configureAtoConfig()) rather than these directly. */
constexpr uint32_t SLEEP_MAX_DURATION = xal::duration::HOURS_2;
constexpr uint32_t IDLE_MAX_DURATION = xal::duration::HOURS_6;
constexpr uint32_t PUMP_MAX_ON_DURATION = xal::duration::SECONDS_90;

static_assert(
    PUMP_MAX_ON_DURATION >= xal::ato::PUMP_MAX_ON_MS_MIN &&
        PUMP_MAX_ON_DURATION <= xal::ato::PUMP_MAX_ON_MS_MAX,
    "Default pump timeout must remain within configured safety bounds");

static_assert(
    SLEEP_MAX_DURATION > 0 && IDLE_MAX_DURATION > 0,
    "Default sleep/idle timers must be non-zero (0 makes Timer elapse every loop)");

constexpr uint32_t PUSH_BUTTON_PIN_INPUT_MODE = INPUT;        /* INPUT or INPUT_PULLUP */
constexpr uint32_t PUSH_BUTTON_PIN_STATE_WHEN_RELEASED = LOW; /* HIGH or LOW */
constexpr uint32_t PUSH_BUTTON_DEBOUNCE_MS = xal::duration::MILLIS_160;
constexpr uint32_t PUSH_BUTTON_LONG_PRESS_DURATION = xal::duration::SECONDS_3;

/* Forward declarations: needed before atoConfigConsole's constructor call
 * below can take applyAtoConfig's address. Definitions live in the
 * "Configure helper functions" section, alongside the rest of the
 * configureX() functions, following this file's existing structure. */
bool applyAtoConfig(const xal::ato::AtoConfig &config);

/* << Initialization >> */

xal::Switchable switchableRedLed(ato::McuPin::RedLed, WHEN_ON_PIN_HIGH);
xal::Switchable switchableYellowLed(ato::McuPin::YellowLed, WHEN_ON_PIN_HIGH);
xal::Switchable switchableGreenLed(ato::McuPin::GreenLed, WHEN_ON_PIN_HIGH);
xal::Switchable switchableBuzzer(ato::McuPin::Buzzer, WHEN_ON_PIN_HIGH);
xal::Switchable switchableWaterPump(ato::McuPin::WaterPump, WHEN_ON_PIN_HIGH);

xal::CyclicSwitchable redLed(switchableRedLed);
xal::CyclicSwitchable yellowLed(switchableYellowLed);
xal::CyclicSwitchable greenLed(switchableGreenLed);
xal::CyclicSwitchable buzzer(switchableBuzzer);

xal::TimedSwitchable waterPump(switchableWaterPump);

xal::LiquidLevelSensor normalLevelSensor(
    ato::McuPin::NormalLiquidLevelSensor,
    WHEN_ON_PIN_HIGH,
    PERIODIC_PUSH_READING_PERIOD,
    INITIAL_READING_HIGH);

xal::LiquidLevelSensor highLevelSensor(
    ato::McuPin::HighLiquidLevelSensor,
    WHEN_ON_PIN_HIGH,
    PERIODIC_PUSH_READING_PERIOD,
    INITIAL_READING_LOW);

#ifdef ATO_HAS_LOW_SENSOR
xal::LiquidLevelSensor lowLevelSensor(
    ato::McuPin::LowLiquidLevelSensor,
    WHEN_ON_PIN_HIGH,
    PERIODIC_PUSH_READING_PERIOD,
    INITIAL_READING_HIGH);
#endif

#ifdef ATO_HAS_RESERVOIR_SENSOR
xal::LiquidLevelSensor reservoirLevelSensor(
    ato::McuPin::ReservoirLowLevelSensor,
    WHEN_ON_PIN_HIGH,
    PERIODIC_PUSH_READING_PERIOD,
    INITIAL_READING_HIGH);
#endif

xal::PushButton pushButton(
    ato::McuPin::PushButton,
    PUSH_BUTTON_PIN_INPUT_MODE,
    PUSH_BUTTON_PIN_STATE_WHEN_RELEASED,
    PUSH_BUTTON_DEBOUNCE_MS,
    PUSH_BUTTON_LONG_PRESS_DURATION);

xal::Timer sleepTimer;
xal::Timer idleTimer;

xal::ato::AtoActions atoActions;
xal::ato::AtoFsm atoFsm(atoActions);

/* Compiled-in defaults: used to seed/self-heal EEPROM. Field order must
 * match AtoConfig's declared order exactly (magic, version, sleepMaxDurationMs,
 * idleMaxDurationMs, pumpMaxOnDurationMs, crc8) — magic/version/crc8 here are
 * placeholders (0), since AtoConfigStore::save()/loadOrDefault() always stamps
 * the real values before any EEPROM write or validity check. */
xal::ato::AtoConfig atoConfigDefaults = {
    /* magic */ 0,
    /* version */ 0,
    /* sleepMaxDurationMs */ SLEEP_MAX_DURATION,
    /* idleMaxDurationMs */ IDLE_MAX_DURATION,
    /* pumpMaxOnDurationMs */ PUMP_MAX_ON_DURATION,
    /* crc8 */ 0};

xal::ato::AtoConfig atoConfig; /* populated from EEPROM (or defaults) in configureAtoConfig() */

/* FSM trace toggles, flipped by AtoConfigConsole's TRACE ON/ALL/OFF command
 * and read by printFsmTrace() below. Kept as plain globals (not part of
 * AtoConfig) since they're a debugging aid, not a persisted setting. */
bool traceEnabled = false;
bool traceVerbose = false;

xal::ato::AtoConfigConsole atoConfigConsole(atoConfig, atoConfigDefaults, applyAtoConfig, traceEnabled, traceVerbose);

/* << Configure helper functions >> */

void configureAtoActions() {
    atoActions.setRedLed(&redLed);
    atoActions.setYellowLed(&yellowLed);
    atoActions.setGreenLed(&greenLed);
    atoActions.setBuzzer(&buzzer);
    atoActions.setWaterDispenser(&waterPump);
    atoActions.setSleepTimer(&sleepTimer);
    atoActions.setIdleTimer(&idleTimer);
}

void configureSensors() {
    normalLevelSensor.setIsTriggeredCallback([]() { atoFsm.dispatch(xal::ato::Event::NormalLevelSensorIsTriggered); });
    normalLevelSensor.setNotTriggeredCallback([]() { atoFsm.dispatch(xal::ato::Event::NormalLevelSensorNotTriggered); });

    highLevelSensor.setIsTriggeredCallback([]() { atoFsm.dispatch(xal::ato::Event::HighLevelSensorIsTriggered); });
    highLevelSensor.setNotTriggeredCallback([]() { atoFsm.dispatch(xal::ato::Event::HighLevelSensorNotTriggered); });

#ifdef ATO_HAS_LOW_SENSOR
    lowLevelSensor.setIsTriggeredCallback([]() { atoFsm.dispatch(xal::ato::Event::LowLevelSensorIsTriggered); });
    lowLevelSensor.setNotTriggeredCallback([]() { atoFsm.dispatch(xal::ato::Event::LowLevelSensorNotTriggered); });
#endif

#ifdef ATO_HAS_RESERVOIR_SENSOR
    reservoirLevelSensor.setIsTriggeredCallback([]() { atoFsm.dispatch(xal::ato::Event::ReservoirLevelSensorIsTriggered); });
    reservoirLevelSensor.setNotTriggeredCallback([]() { atoFsm.dispatch(xal::ato::Event::ReservoirLevelSensorNotTriggered); });
#endif
}

void configureTimers() {
    sleepTimer.setCallback([]() { atoFsm.dispatch(xal::ato::Event::SleepTimeElapsed); });
    idleTimer.setCallback([]() { atoFsm.dispatch(xal::ato::Event::MaxIdleTimeElapsed); });
}

void configureWaterPump() {
    waterPump.setOnTimeElapsedCallback([]() { atoFsm.dispatch(xal::ato::Event::DispenserOnTimeElapsed); });
    waterPump.setOff();
}

void configurePushButton() {
    pushButton.setShortPressCallback([]() { atoFsm.dispatch(xal::ato::Event::DispenseButtonIsPushed); });
    pushButton.setLongPressCallback([]() { atoFsm.dispatch(xal::ato::Event::SleepButtonIsPushed); });
}

/**
 * @brief Loads AtoConfig from EEPROM (or seeds it with atoConfigDefaults if
 * missing/corrupt/version-mismatched), then applies it to live hardware.
 */
void configureAtoConfig() {
    atoConfig = xal::ato::AtoConfigStore::loadOrDefault(atoConfigDefaults);
    applyAtoConfig(atoConfig);
}

/**
 * @brief Pushes the given config's duration values into the live
 * sleepTimer/idleTimer/waterPump objects. Called once at boot (via
 * configureAtoConfig()) and again by AtoConfigConsole after every
 * runtime SET/RESET, so changes take effect immediately without a reboot.
 */
bool applyAtoConfig(const xal::ato::AtoConfig &config) {
    return xal::ato::applyValidatedAtoConfig(config, sleepTimer, idleTimer, waterPump);
}

/**
 * @brief Unconditional (trace-independent) advisory: if the FSM has just
 * entered a state whose governing timer is configured below the advisory
 * minimum, say so on Serial. Mirrors the console's SET/GET WARN lines but
 * catches the case where a short value was already saved to EEPROM and only
 * bites at runtime. IdleForTooLong / Sleeping are each entered at most once
 * per episode, so this cannot spam. Reads the live global atoConfig, which
 * AtoConfigConsole keeps in sync on every SET/RESET.
 */
void warnIfEnteringShortTimerState(xal::ato::State fromState, xal::ato::State toState, bool matched) {
    if (!matched || toState == fromState) {
        return;
    }
    if (toState == xal::ato::State::IdleForTooLong &&
        xal::ato::isBelowTimerAdvisoryMin(atoConfig.idleMaxDurationMs)) {
        Serial.print(F("WARN entered IdleForTooLong with IDLE_MAX_MS="));
        Serial.print(atoConfig.idleMaxDurationMs);
        Serial.println(F(" ms (below advisory minimum)"));
    } else if (toState == xal::ato::State::Sleeping &&
               xal::ato::isBelowTimerAdvisoryMin(atoConfig.sleepMaxDurationMs)) {
        Serial.print(F("WARN entered Sleeping with SLEEP_MAX_MS="));
        Serial.print(atoConfig.sleepMaxDurationMs);
        Serial.println(F(" ms (below advisory minimum); will wake almost immediately"));
    }
}

/**
 * @brief FSM dispatch observer, registered via atoFsm.setTraceCallback() in
 * setup(). Always emits the short-timer advisory above; the rest of the
 * line is printed only when tracing is enabled (TRACE ON/ALL from the config
 * console). With TRACE ON, only calls that produced a real transition are
 * shown, since dispatch() is only ever called on genuine sensor/button/timer
 * events (never from the hot loop), so this stays sparse by construction.
 * TRACE ALL additionally shows events that arrived but matched no rule —
 * useful for confirming whether an expected event even reached the FSM.
 */
void printFsmTrace(xal::ato::State fromState, xal::ato::Event event, xal::ato::State toState, bool matched) {
    warnIfEnteringShortTimerState(fromState, toState, matched);

    if (!traceEnabled) {
        return;
    }
    if (!matched && !traceVerbose) {
        return;
    }

    Serial.print(millis());
    Serial.print(F("  "));
    Serial.print(xal::ato::stateName(fromState));
    Serial.print(F(" + "));
    Serial.print(xal::ato::eventName(event));
    Serial.print(F(" -> "));
    Serial.print(xal::ato::stateName(toState));

    if (!matched) {
        Serial.print(F("  [ignored: no matching rule]"));
    }

    Serial.println();
}

void setup() {
    /* Required for the runtime config console (GET/SET/SAVE/RESET). Safe to
       leave enabled with nothing connected: Serial.available() just returns
       0 and AtoConfigConsole::loop() becomes a no-op. */
    Serial.begin(9600);

    /* Configure the ATO components */
    configureAtoConfig();
    configureAtoActions();
    configureSensors();
    configureTimers();
    configureWaterPump();
    configurePushButton();

    /* Off by default (TRACE ON/ALL/OFF via the config console toggles the
       flags this callback checks); registering it unconditionally costs
       one function pointer and one branch per dispatch() call. */
    atoFsm.setTraceCallback(printFsmTrace);

    /* Configure runnables */
    xal::Runnable::setupAll();

    delay(xal::duration::MILLIS_100);

    atoActions.onExitState();
    atoActions.onEntryIdleState();

    /* Enable watchdog timer */
    wdt_enable(WDTO_2S); /* !!! do not introduce delays bigger than this !!! */
}

void loop() {
    /* Loop all runnables */
    xal::Runnable::loopAll();

    /* Reset watchdog timer */
    wdt_reset();
}
