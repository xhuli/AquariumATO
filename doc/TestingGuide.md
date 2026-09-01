# AquariumATO Testing Guide

This guide describes how AquariumATO is tested with PlatformIO and Unity, what each test suite covers, and the conventions to follow when adding or changing tests.

The tests are **on-target embedded tests**. PlatformIO compiles a temporary Unity test firmware image, uploads it to an AVR board, and reads the results over the board's serial connection. The project does not currently provide a native desktop/mock-Arduino test environment.

For fresh-machine setup, board access, and PlatformIO installation, see [`ToolchainBootstrap.md`](ToolchainBootstrap.md).

## 1. Test environments

AquariumATO defines two PlatformIO environments:

| Environment | Board | Role |
| --- | --- | --- |
| `megaatmega2560` | Arduino Mega 2560 | **Recommended default for all testing** |
| `nanoatmega328` | Arduino Nano / ATmega328P, old bootloader | **Production firmware target** and limited fallback test target |

### 1.1 Recommended default: Mega 2560

Use `megaatmega2560` for normal development, targeted testing, and full regression runs.

All current Unity suites can run safely on this environment, including the complete FSM suite. The Mega's larger SRAM also avoids having to remember which individual suites fit on the Nano.

Run the complete test set with:

```bash
pio test -e megaatmega2560
```

Run one suite with:

```bash
pio test -e megaatmega2560 -f <suite-name>
```

For example:

```bash
pio test -e megaatmega2560 -f test_ato_fsm
```

The Mega environment is for **development and testing only**. It is not the production firmware target.

### 1.2 If only a Nano is available

The Nano can run the smaller suites, but the ATmega328P has only 2 KB of SRAM. Because the complete `test_ato_fsm` Unity image exceeds that practical limit, do not use an unfiltered full-suite test command on the Nano.

If testing is limited to `nanoatmega328`:

1. run suites **individually** with `-f`;
2. run only the Nano-compatible suites listed below;
3. **skip `test_ato_fsm`**;
4. use a Mega later if the complete FSM suite must be executed.

Nano-compatible suites:

```text
test_ato_config_store
test_cyclic_switchable
test_liquid_level_sensor
test_push_button
test_ring_buffer
test_runnable
test_timed_switchable
test_timer
```

Example:

```bash
pio test -e nanoatmega328 -f test_ring_buffer
```

Do **not** use an unfiltered Nano test run as a regression command, because PlatformIO will also discover `test_ato_fsm`.

The FSM suite must be run on the Mega:

```bash
pio test -e megaatmega2560 -f test_ato_fsm
```

> **Important:** this is a test-harness SRAM limitation, not a limitation of the production AquariumATO firmware. Production firmware remains targeted at `nanoatmega328`.

### 1.3 Production validation remains Nano-specific

Passing all tests on the Mega does not replace building the production firmware for the Nano.

After code changes, also build the production environment:

```bash
pio run -e nanoatmega328
```

When appropriate, perform the hardware validation described in the Hardware Guide and User Guide on the actual production controller.

## 2. Test directory structure

Each Unity suite lives in its own directory under `test/`:

```text
test/
├── test_ato_config_store/
├── test_ato_fsm/
├── test_cyclic_switchable/
├── test_liquid_level_sensor/
├── test_push_button/
├── test_ring_buffer/
├── test_runnable/
├── test_timed_switchable/
└── test_timer/
```

The directory name is also the suite name used with PlatformIO's `-f` filter.

### 2.1 File-placement rules

Keep all test-only source under `test/`, not under `src/`.

A Unity test normally defines its own Arduino `setup()` and `loop()`. Placing test firmware under `src/` would mix test entry points into the production build and conflict with the production `setup()` and `loop()`.

Keep helper `.cpp` files required by one suite inside that suite's directory. For example, the multi-translation-unit `Runnable` regression intentionally uses:

```text
test/test_runnable/
├── runnable_fixture.h
├── runnable_fixture_a.cpp
├── runnable_fixture_b.cpp
└── test_runnable.cpp
```

This avoids two common mistakes:

1. accidentally compiling test code into production firmware;
2. putting helper sources where PlatformIO does not compile them as part of the intended suite.

## 3. Test-suite summary

| Suite | Main coverage | Notes |
| --- | --- | --- |
| `test_ato_config_store` | EEPROM integrity, semantic validation, console/parser hardening, runtime config application | **Writes real EEPROM** |
| `test_ato_fsm` | FSM reachability, legal transitions, no-match behavior | **Mega required** because the Unity image exceeds Nano SRAM |
| `test_cyclic_switchable` | Cyclic output-pattern progression | Uses deterministic injected timestamps |
| `test_liquid_level_sensor` | Sensor debounce, callbacks, periodic re-push | Uses fabricated readings and timestamps |
| `test_push_button` | Button debounce, short/long press classification | Uses fabricated readings and timestamps |
| `test_ring_buffer` | Ordering, wraparound, average, clear/empty semantics | Generic utility regression |
| `test_runnable` | Self-registration, traversal, multi-translation-unit registry behavior | Uses multiple fixture `.cpp` files intentionally |
| `test_timed_switchable` | Maximum ON/OFF timeout behavior | Confirms `0` disables timeout at the generic utility level |
| `test_timer` | One-shot, auto-restart, and cancellation behavior | Uses injected timestamps |

The following sections describe coverage and special caveats. They intentionally do not repeat board-specific commands; use the execution policy in §1 and the regression workflow in §13.

## 4. `test_ato_config_store`

Location:

```text
test/test_ato_config_store/test_ato_config_store.cpp
```

This is the configuration safety, persistence, and serial-console regression suite.

### What it verifies

The suite covers:

* blank EEPROM falling back to compiled defaults;
* rejection of incorrect magic values;
* rejection of incompatible config versions;
* rejection of CRC-damaged persisted data;
* correct loading of valid persisted configuration;
* self-healing of invalid persisted data by restoring safe defaults;
* semantic validation of `PUMP_MAX_ON_MS`;
* rejection of `PUMP_MAX_ON_MS=0`;
* rejection of pump timeouts below the configured minimum;
* rejection of pump timeouts above the configured maximum;
* rejection of structurally valid EEPROM data containing an unsafe pump timeout;
* prevention of invalid config values from reaching the runtime pump timeout setter;
* propagation of valid config values to runtime setters;
* exact `uint32_t` parsing and overflow rejection;
* malformed console commands producing no unintended side effects;
* strict command arity;
* complete discard of oversized serial input lines;
* continued operation of valid commands after parser hardening;
* preservation of the P0 pump-safety rules through the console path.

### Destructive EEPROM warning

> **CAUTION: this suite writes the test board's real EEPROM.**

The suite intentionally blanks, corrupts, and rewrites the AquariumATO configuration region to exercise recovery and validation behavior. Running it can overwrite settings previously stored through the serial console.

Do not run this suite on a configured production controller unless you are prepared to restore its settings afterward.

Each test establishes its own EEPROM precondition so the suite does not depend on whatever data was left by a previous test or power cycle.

## 5. `test_ato_fsm`

Location:

```text
test/test_ato_fsm/test_ato_fsm.cpp
```

This suite verifies the state-transition behavior of `AtoFsm` and protects the transition-table design.

### What it verifies

The suite covers:

* reachability of the states exercised by the test matrix;
* every explicit `(state, event) -> state` transition represented by the suite;
* representative unhandled events leaving the FSM in the current state;
* alert, dispensing, sleeping, reservoir, high-water, low-water, idle-too-long, manual-dispensing, and error transitions;
* regression protection for the table-driven dispatch behavior.

The suite observes state changes through `getState()`. It does not comprehensively verify `AtoActions` hardware side effects such as LED patterns, buzzer patterns, or timer switching.

### Mega-only test harness

`test_ato_fsm` must run on `megaatmega2560`.

The complete Unity test image requires more SRAM than is reliably available on the Nano's ATmega328P. This is why the Mega environment exists as a development test target.

Do not reduce production safety, change the production board target, or restructure firmware merely to make this test image fit the Nano.

## 6. `test_cyclic_switchable`

Location:

```text
test/test_cyclic_switchable/test_cyclic_switchable.cpp
```

This suite verifies `CyclicSwitchable` behavior using a fake wrapped switchable and injected timestamps.

### What it verifies

* `setOn()` starts the pattern and activates the wrapped output;
* the pattern advances when the active interval expires;
* the sequence wraps after the final interval;
* no pattern progression occurs while the switchable is off.

The implementation advances according to calls to `process()`. It is not intended to replay every historical interval after a delayed loop call.

## 7. `test_liquid_level_sensor`

Location:

```text
test/test_liquid_level_sensor/test_liquid_level_sensor.cpp
```

`LiquidLevelSensor` exposes `process(rawReading, nowMs)` so debounce and callback behavior can be exercised deterministically without physical sensor wiring.

### What it verifies

* a single noisy reading does not change the debounced state;
* a minority noise burst does not flip the debounced state;
* sustained input changes the debounced state and fires the correct callback;
* unchanged state does not re-fire before the periodic callback interval;
* periodic state re-push occurs after the configured interval;
* periodic re-push is disabled when its interval is `0`;
* `isTriggered()` and `isNotTriggered()` respect the configured liquid-present polarity.

These are firmware logic tests. They do not prove electrical behavior, sensor placement, cleanliness, optical performance, or installation quality.

## 8. `test_push_button`

Location:

```text
test/test_push_button/test_push_button.cpp
```

Like the liquid-level sensor, `PushButton` exposes a deterministic processing seam so tests can supply fabricated raw readings and timestamps.

### What it verifies

* isolated noise does not register as a press;
* minority noise does not change the debounced state;
* sustained input changes the debounced state;
* a valid short press fires the short-press callback;
* a valid long press fires the long-press callback;
* a press shorter than the debounce requirement does not fire a callback.

## 9. `test_ring_buffer`

Location:

```text
test/test_ring_buffer/test_ring_buffer.cpp
```

This suite protects the generic `RingBuffer<T, N>` utility used by the button and liquid-level sensor debounce logic.

### What it verifies

* a fresh buffer is logically empty;
* `average()` is safe on an empty buffer and returns `T()`;
* `clear()` leaves the buffer logically empty;
* pushing after `clear()` behaves like a fresh buffer;
* partial-buffer logical ordering is correct;
* full-buffer ordering is correct;
* wrapped buffers expose values from oldest to newest;
* averages remain correct after wraparound;
* repeated wraparound remains correct;
* `fill(value)` fills the logical capacity with that value.

`clear()` may also reset backing storage to a known default value for debugging, but logical validity is determined by the buffer's count/index state.

## 10. `test_runnable`

Location:

```text
test/test_runnable/
```

This suite protects the `Runnable` self-registration mechanism used throughout the firmware.

### What it verifies

* `setupAll()` reaches registered objects;
* `loopAll()` reaches registered objects;
* the existing LIFO registration/traversal order remains stable;
* registration works when derived objects are defined in multiple translation units.

The extra `runnable_fixture_a.cpp` and `runnable_fixture_b.cpp` files are deliberate. They protect against reintroducing a header-defined registry-storage problem that only appears when `Runnable.h` is used from multiple `.cpp` files.

`Runnable` is intentionally non-copyable and non-movable, and registered instances are expected to have static/global lifetime.

## 11. `test_timed_switchable`

Location:

```text
test/test_timed_switchable/test_timed_switchable.cpp
```

This suite verifies the generic time-limited switch wrapper using a fake `AbstractSwitchable`.

### What it verifies

* an ON duration expires, switches the wrapped component off, and fires the timeout callback;
* an OFF duration expires, switches the wrapped component on, and fires the timeout callback;
* a maximum ON duration of `0` means no automatic ON timeout at the generic utility level.

That last behavior is intentional in `TimedSwitchable`. AquariumATO's higher-level configuration validation therefore rejects `PUMP_MAX_ON_MS=0`, because allowing it for the ATO pump would disable the runtime safety cutoff.

## 12. `test_timer`

Location:

```text
test/test_timer/test_timer.cpp
```

This suite verifies the generic timer independently of `millis()` by supplying explicit timestamps.

### What it verifies

* an OFF timer does nothing;
* a one-shot timer fires and switches itself off;
* an auto-restart timer can fire repeatedly;
* manually switching a timer off cancels a pending fire.

## 13. Running a regression pass

### 13.1 Normal development workflow

For ordinary development, use the Mega for both targeted tests and the full regression pass.

First run the suite most closely related to the change:

```bash
pio test -e megaatmega2560 -f <suite-name>
```

Then run all suites:

```bash
pio test -e megaatmega2560
```

Finally, build the production firmware:

```bash
pio run -e nanoatmega328
```

This is the recommended default workflow because it keeps test execution simple while still validating that production firmware builds for the real Nano target.

### 13.2 When testing is limited to a Nano

If no Mega is available, run only the relevant Nano-compatible suites, one at a time with `-f`.

For example:

```bash
pio test -e nanoatmega328 -f test_ring_buffer
pio test -e nanoatmega328 -f test_timer
pio test -e nanoatmega328 -f test_timed_switchable
```

Use the Nano-compatible suite list in §1.2 and skip `test_ato_fsm`.

A Nano-only pass is therefore a **partial regression pass** until the FSM suite has also been run on a Mega.

### 13.3 EEPROM reminder

If the regression includes `test_ato_config_store`, remember that the suite overwrites the test board's stored ATO configuration. Restore the desired settings afterward if that board will be returned to normal service.

## 14. Adding or changing tests

Behavior changes should be accompanied by tests whenever a deterministic seam exists or can be introduced without compromising the production design.

### 14.1 Create a suite under `test/`

Create a directory with a `test_` prefix:

```text
test/test_new_feature/
    test_new_feature.cpp
```

A conventional on-target Unity test contains:

```cpp
#include <Arduino.h>
#include <unity.h>

void test_example()
{
    TEST_ASSERT_TRUE(true);
}

void setup()
{
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_example);
    UNITY_END();
}

void loop()
{
}
```

Follow existing repository patterns rather than copying this skeleton mechanically.

### 14.2 Prefer deterministic behavior seams

The timer, button, liquid-level sensor, and switchable tests avoid fragile real-time or jumper-wire tests by injecting timestamps, raw readings, or fake components.

Continue that approach where practical:

* test observable behavior rather than private implementation details;
* inject timestamps instead of waiting on wall-clock time;
* inject raw input instead of requiring physical GPIO changes;
* use fake switchables/components behind stable interfaces;
* keep electrical validation separate from firmware logic tests;
* avoid blocking test techniques that encourage blocking production code.

For time-based generic `lib/` code, preserve the existing convention of exposing `process(nowMs)` or an equivalent deterministic seam rather than reading `millis()` internally when testability would otherwise be lost.

### 14.3 Keep tests independent

Each test must establish its own starting conditions. Do not rely on Unity execution order to prepare state for the next test.

This is especially important for EEPROM because persistent state survives reset and previous test runs. The configuration suite explicitly establishes its own EEPROM preconditions for this reason.

### 14.4 FSM changes

When adding a new FSM state, event, or transition:

* add or update the relevant `TRANSITION_TABLE` row;
* add tests for the intended transition;
* add at least one no-match regression where appropriate;
* keep the table as the single source of truth for legal transitions;
* do not reintroduce nested switch-based dispatch.

If a new state requires entry behavior, also cover the corresponding `AtoActions` implementation at the most appropriate test level.

### 14.5 Timing and debounce changes

When changing timing or debounce behavior, test boundary conditions explicitly:

* immediately before the threshold;
* exactly at the threshold;
* immediately after the threshold;
* disabled/zero behavior where zero has defined semantics;
* rollover or repeated-cycle behavior where relevant.

For LED or buzzer interval patterns, preserve the compile-time duration checks in `AtoActions` rather than relying only on runtime tests.

## 15. Troubleshooting tests

### Test firmware does not fit

If a development test image exceeds the Nano's SRAM or flash capacity, run it on the Mega test environment.

For `test_ato_fsm`, the Mega is already required.

Do not solve a test-harness memory problem by changing the production board environment.

### PlatformIO cannot find the board

Check the USB device, serial port, and permissions as described in [`ToolchainBootstrap.md`](ToolchainBootstrap.md). On Linux, verify access to the appropriate `/dev/ttyUSB*` or `/dev/ttyACM*` device.

### Suite is not discovered

Check that:

* the suite directory is under `test/`;
* the directory name begins with `test_`;
* the main test source is inside that suite directory;
* any required helper `.cpp` files are also inside that suite directory;
* the name passed to `-f` matches the actual suite directory.

### Duplicate `setup()` / `loop()` or production link conflicts

Make sure Unity test sources were not added under `src/`. Unity test firmware and production firmware are separate builds.

### Missing helper symbols

For suites using multiple translation units, verify that all helper `.cpp` files live inside the suite directory. `test_runnable` is the reference example.

### Configuration tests changed device settings

This is expected. `test_ato_config_store` deliberately writes real EEPROM. Restore the desired settings afterward through the production configuration workflow.

### Serial test runner does not start reliably

Verify the correct board environment and serial port first. If upload succeeds but Unity output is not observed, check the PlatformIO serial connection and board reset behavior before changing test code.

## 16. What the automated tests do not prove

The current suites provide strong coverage of generic timing/debounce utilities, configuration safety, parser behavior, and FSM transitions. They are not a substitute for testing the assembled ATO as a physical system.

The automated suites do not comprehensively prove:

* real pump flow rate;
* dry-run behavior of a specific pump;
* plumbing and siphon prevention;
* real sensor placement and optical performance;
* electrical noise immunity in the installed system;
* sensor contamination or maintenance state;
* real LED visibility or buzzer audibility;
* every `AtoActions` hardware side effect as an integrated system;
* power-supply behavior under all pump/load combinations;
* long-duration endurance of a complete assembled controller;
* aquarium-specific installation safety.

Use [`HardwareGuide.md`](HardwareGuide.md) for hardware bring-up and electrical/plumbing guidance, and [`UserGuide.md`](UserGuide.md) for operating checks and maintenance.

## 17. Testing policy summary

For day-to-day work, remember these rules:

1. Use `megaatmega2560` as the default test environment.
2. Run the full regression suite on Mega with `pio test -e megaatmega2560`.
3. Use Nano for tests only when Mega is unavailable, and then run compatible suites individually with `-f`.
4. Never run `test_ato_fsm` on the Nano; use Mega.
5. Treat `test_ato_config_store` as EEPROM-destructive.
6. After tests, build the real production target with `pio run -e nanoatmega328`.
7. Keep tests deterministic and add them alongside behavior changes.
8. Keep hardware validation separate from firmware logic tests.
