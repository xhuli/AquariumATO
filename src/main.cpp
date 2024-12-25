#include "ato/AtoActions.h"
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
        // constexpr uint8_t LowLiquidLevelSensor = PIN_A1; /* 15 */
        constexpr uint8_t HighLiquidLevelSensor = PIN_A2; /* 16 */
        // constexpr uint8_t ReservoirLowLevelSensor = PIN_A3; /* 17 */
    }; /* namespace McuPin */
}; /* namespace ato */

constexpr uint8_t WHEN_ON__PIN_HIGH = HIGH;
constexpr uint8_t WHEN_ON__PIN_LOW = LOW;

constexpr uint8_t INITIAL_READING_HIGH = HIGH;
constexpr uint8_t INITIAL_READING_LOW = LOW;
constexpr uint32_t PERIODIC_PUSH_READING_DISABLED = xal::duration::SECONDS_0;
constexpr uint32_t PERIODIC_PUSH_READING_PERIOD = xal::duration::SECONDS_2;

constexpr uint32_t SLEEP_MAX_DURATION = xal::duration::HOURS_2;
constexpr uint32_t IDLE_MAX_DURATION = xal::duration::HOURS_6;

constexpr uint32_t PUMP_MAX_ON_DURATION = xal::duration::SECONDS_90;

constexpr uint32_t PUSH_BUTTON_PIN_INPUT_MODE = INPUT;        /* INPUT or INPUT_PULLUP */
constexpr uint32_t PUSH_BUTTON_PIN_STATE_WHEN_RELEASED = LOW; /* HIGH or LOW */
constexpr uint32_t PUSH_BUTTON_DEBOUNCE_MS = xal::duration::MILLIS_160;
constexpr uint32_t PUSH_BUTTON_LONG_PRESS_DURATION = xal::duration::SECONDS_3;

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

// xal::LiquidLevelSensor lowLevelSensor(
//     ato::McuPin::LowLiquidLevelSensor,
//     WHEN_ON__PIN_HIGH,
//     PERIODIC_PUSH_READING_PERIOD,
//     INITIAL_READING_HIGH);

// xal::LiquidLevelSensor reservoirLevelSensor(
//     ato::McuPin::ReservoirLowLevelSensor,
//     WHEN_ON__PIN_HIGH,
//     PERIODIC_PUSH_READING_PERIOD,
//     INITIAL_READING_HIGH);

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

    // lowLevelSensor.setIsTriggeredCallback([]() { atoFsm.dispatch(xal::ato::Event::LowLevelSensorIsTriggered); });
    // lowLevelSensor.setNotTriggeredCallback([]() { atoFsm.dispatch(xal::ato::Event::LowLevelSensorNotTriggered); });

    // reservoirLevelSensor.setIsTriggeredCallback([]() { atoFsm.dispatch(xal::ato::Event::ReservoirLevelSensorIsTriggered); });
    // reservoirLevelSensor.setNotTriggeredCallback([]() { atoFsm.dispatch(xal::ato::Event::ReservoirLevelSensorNotTriggered); });
}

void configureTimers() {
    sleepTimer.setDurationMs(SLEEP_MAX_DURATION);
    sleepTimer.setCallback([]() { atoFsm.dispatch(xal::ato::Event::SleepTimeElapsed); });

    idleTimer.setDurationMs(IDLE_MAX_DURATION);
    idleTimer.setCallback([]() { atoFsm.dispatch(xal::ato::Event::MaxIdleTimeElapsed); });
}

void configureWaterPump() {
    waterPump.setMaxOnTimeMs(PUMP_MAX_ON_DURATION);
    waterPump.setOnTimeElapsedCallback([]() { atoFsm.dispatch(xal::ato::Event::DispenserOnTimeElapsed); });
    waterPump.setOff();
}

void configurePushButton() {
    pushButton.setShortPressCallback([]() { atoFsm.dispatch(xal::ato::Event::DispenseButtonIsPushed); });
    pushButton.setLongPressCallback([]() { atoFsm.dispatch(xal::ato::Event::SleepButtonIsPushed); });
}


void setup()
{
    /* Required for logging */
    // Serial.begin(9600);
    //     while (!Serial && !Serial.available()) {
    // }
    // randomSeed(analogRead(0));

    /* Configure Logging */
    // Log.begin(LOG_LEVEL_VERBOSE, &Serial);

    /* Configure the ATO components */
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
