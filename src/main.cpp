#include "ato/AtoActions.h"
#include "ato/AtoConfig.h"
#include "ato/AtoConfigConsole.h"
#include "ato/AtoFsm.h"

#include <Arduino.h>
// #include <ArduinoLog.h>
#include <avr/wdt.h>

#include <CyclicSwitchable.h>
#include <Duration.h>
#include <LiquidLevelSensor.h>
#include <PushButton.h>
#include <Runnable.h>
#include <TimedSwitchable.h>
#include <Timer.h>

/* << Constants >> */

namespace ato
{
    namespace McuPin
    {
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

constexpr uint8_t WHEN_ON__PIN_HIGH = HIGH;
constexpr uint8_t WHEN_ON__PIN_LOW = LOW;

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

constexpr uint32_t PUSH_BUTTON_PIN_INPUT_MODE = INPUT;        /* INPUT or INPUT_PULLUP */
constexpr uint32_t PUSH_BUTTON_PIN_STATE_WHEN_RELEASED = LOW; /* HIGH or LOW */
constexpr uint32_t PUSH_BUTTON_DEBOUNCE_MS = xal::duration::MILLIS_160;
constexpr uint32_t PUSH_BUTTON_LONG_PRESS_DURATION = xal::duration::SECONDS_3;

/* Forward declarations: needed before atoConfigConsole's constructor call
 * below can take applyAtoConfig's address. Definitions live in the
 * "Configure helper functions" section, alongside the rest of the
 * configureX() functions, following this file's existing structure. */
void applyAtoConfig(const xal::ato::AtoConfig &config);

/* << Initialization >> */

xal::Switchable switchableRedLed(ato::McuPin::RedLed, WHEN_ON__PIN_HIGH);
xal::Switchable switchableYellowLed(ato::McuPin::YellowLed, WHEN_ON__PIN_HIGH);
xal::Switchable switchableGreenLed(ato::McuPin::GreenLed, WHEN_ON__PIN_HIGH);
xal::Switchable switchableBuzzer(ato::McuPin::Buzzer, WHEN_ON__PIN_HIGH);
xal::Switchable switchableWaterPump(ato::McuPin::WaterPump, WHEN_ON__PIN_HIGH);

xal::CyclicSwitchable redLed(switchableRedLed);
xal::CyclicSwitchable yellowLed(switchableYellowLed);
xal::CyclicSwitchable greenLed(switchableGreenLed);
xal::CyclicSwitchable buzzer(switchableBuzzer);

xal::TimedSwitchable waterPump(switchableWaterPump);

xal::LiquidLevelSensor normalLevelSensor(
    ato::McuPin::NormalLiquidLevelSensor,
    WHEN_ON__PIN_HIGH,
    PERIODIC_PUSH_READING_PERIOD,
    INITIAL_READING_HIGH);

xal::LiquidLevelSensor highLevelSensor(
    ato::McuPin::HighLiquidLevelSensor,
    WHEN_ON__PIN_HIGH,
    PERIODIC_PUSH_READING_PERIOD,
    INITIAL_READING_LOW);

#ifdef ATO_HAS_LOW_SENSOR
xal::LiquidLevelSensor lowLevelSensor(
    ato::McuPin::LowLiquidLevelSensor,
    WHEN_ON__PIN_HIGH,
    PERIODIC_PUSH_READING_PERIOD,
    INITIAL_READING_HIGH);
#endif

#ifdef ATO_HAS_RESERVOIR_SENSOR
xal::LiquidLevelSensor reservoirLevelSensor(
    ato::McuPin::ReservoirLowLevelSensor,
    WHEN_ON__PIN_HIGH,
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

xal::ato::AtoConfigConsole atoConfigConsole(atoConfig, atoConfigDefaults, applyAtoConfig);

/* << Cofigure helper functions >> */

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
void applyAtoConfig(const xal::ato::AtoConfig &config) {
    sleepTimer.setDurationMs(config.sleepMaxDurationMs);
    idleTimer.setDurationMs(config.idleMaxDurationMs);
    waterPump.setMaxOnTimeMs(config.pumpMaxOnDurationMs);
}

void setup()
{
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

    /* Configure runnables */
    xal::Runnable::setupAll();

    delay(xal::duration::MILLIS_100);

    atoActions.onExitState();
    atoActions.onEntryIdleState();

    /* Enable watchdog timer */
    wdt_enable(WDTO_2S); /* !!! do not introduce delays bigger than this !!! */
}

void loop()
{
    /* Loop all runnables */
    xal::Runnable::loopAll();

    /* Reset watchdog timer */
    wdt_reset();
}
