# AquariumATO Testing Guide

This guide describes the embedded Unity test suites in AquariumATO, what each suite protects, which board to use, and the practical rules for adding and running tests.

The tests are **on-target PlatformIO tests**. They compile into temporary test firmware, are uploaded to an AVR board, and report Unity results over the board's serial connection. The project does not currently provide a native desktop/mock-Arduino test environment.

For fresh-machine setup, install and verify PlatformIO first as described in `ToolchainBootstrap.md`.

## 1. Test environments

The repository defines two PlatformIO environments relevant to testing:

| Environment | Board | Intended use |
|---|---|---|
| `nanoatmega328` | Arduino Nano / ATmega328P | Production firmware and most small test suites |
| `megaatmega2560` | Arduino Mega 2560 / ATmega2560 | Development test target when a Unity image needs more SRAM |

Production firmware remains targeted at `nanoatmega328`. The Mega environment exists to provide extra test-time memory and is not the production target.

### 1.1 Basic commands

Run tests with a board connected over USB:

```bash
pio test -e nanoatmega328
```

For the FSM suite, use the Mega:

```bash
pio test -e megaatmega2560 -f test_ato_fsm
```

To run one small suite on the Nano:

```bash
pio test -e nanoatmega328 -f test_timer
```

PlatformIO builds and uploads a test image, then reads the Unity result over serial. Running `pio test` therefore requires a real compatible board unless a future native test environment is added.

> **Important:** CMake's `UnityTests` target exists only so CLion can display and navigate the files under `test/`. It does **not** compile or run the Unity tests. Use `pio test` for actual test execution.

## 2. Test directory structure

Each PlatformIO Unity suite lives in its own directory under `test/`:

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

Keep test-only code under `test/`, not under `src/`. A Unity test normally defines its own Arduino `setup()` and `loop()`; placing such a file in `src/` would mix it into the production firmware and conflict with the production entry points.

Keep helper `.cpp` files required by one suite inside that suite's directory. For example, the `Runnable` multi-translation-unit regression uses:

```text
test/test_runnable/
├── runnable_fixture.h
├── runnable_fixture_a.cpp
├── runnable_fixture_b.cpp
└── test_runnable.cpp
```

If a helper source is placed elsewhere, PlatformIO may not compile it as part of the intended suite, producing missing symbols or a test that does not actually exercise the intended multi-file condition.

These two rules avoid common test-placement failures:

1. accidentally compiling test entry points into the production firmware;
2. placing suite helper sources where PlatformIO does not include them in that test build.

## 3. Test-suite summary

| Suite | Preferred target | Main coverage | Special warning |
|---|---|---|---|
| `test_ato_config_store` | Nano | EEPROM config integrity, semantic validation, serial parser hardening, pump config application | **Writes real EEPROM** |
| `test_ato_fsm` | **Mega** | FSM reachability and transition table | Nano SRAM is insufficient |
| `test_cyclic_switchable` | Nano | Cyclic output pattern progression | Timing is injected deterministically |
| `test_liquid_level_sensor` | Nano | Sensor debounce, callbacks, periodic re-push | Uses fabricated readings, not physical sensor wiring |
| `test_push_button` | Nano | Button debounce and short/long press classification | Uses fabricated readings/timestamps |
| `test_ring_buffer` | Nano | Logical ordering, wraparound, average, clear/empty semantics | Generic utility regression suite |
| `test_runnable` | Nano | Registration, traversal, multi-translation-unit registry behavior | Includes multiple `.cpp` fixtures intentionally |
| `test_timed_switchable` | Nano | Max-on/max-off timeout behavior | Confirms zero max-on disables timeout at utility level |
| `test_timer` | Nano | One-shot/auto-restart/cancel behavior | Uses injected timestamps |

If a small suite unexpectedly exceeds Nano resources, running that suite on `megaatmega2560` is an acceptable development fallback. Do not interpret that as permission to move the production target away from the Nano.

## 4. `test_ato_config_store`

Location:

```text
test/test_ato_config_store/test_ato_config_store.cpp
```

This is the configuration safety and persistence regression suite. It covers substantially more than EEPROM serialization.

### What it verifies

- blank EEPROM falls back to compiled defaults;
- wrong magic is rejected;
- wrong config version is rejected;
- CRC-damaged persisted data is rejected;
- valid persisted config is loaded correctly;
- rejected data self-heals by writing safe defaults;
- `PUMP_MAX_ON_MS` safety bounds are enforced;
- `0`, below-minimum, and above-maximum pump timeouts are rejected;
- structurally valid EEPROM data with a semantically unsafe pump timeout is rejected;
- invalid config cannot reach the runtime pump timeout setter;
- valid config reaches the runtime setters;
- exact `uint32_t` serial parsing, including overflow rejection;
- malformed console commands have no side effects;
- strict command arity;
- oversized serial lines are discarded completely;
- valid commands still work after parser hardening;
- P0 pump-safety behavior remains intact through the console parser.

### Destructive EEPROM warning

> **CAUTION: this suite uses the board's real EEPROM.**

Running it overwrites the AquariumATO configuration currently stored on the test board, including values previously saved through the serial console. The suite deliberately blanks, corrupts, and rewrites the config region to exercise recovery behavior.

Do not run this test on a configured production controller unless you are prepared to restore its settings afterward.

Each test establishes its own EEPROM precondition so the suite does not depend on whatever data was left by an earlier test or power cycle.

Typical command:

```bash
pio test -e nanoatmega328 -f test_ato_config_store
```

## 5. `test_ato_fsm`

Location:

```text
test/test_ato_fsm/test_ato_fsm.cpp
```

This suite verifies the state-transition behavior of `AtoFsm`.

### What it verifies

- every non-Idle state used by the suite is reachable;
- each explicit `(state, event) -> state` transition behaves as expected;
- representative unhandled events leave the FSM in the current state;
- alert, dispensing, sleeping, reservoir, high-water, low-water, idle-too-long, and error transitions remain stable during refactoring.

The suite observes state transitions through `getState()`. It does not attempt to verify all `AtoActions` hardware side effects such as LED/buzzer patterns or timer switching.

### Why the Mega is required

Use:

```bash
pio test -e megaatmega2560 -f test_ato_fsm
```

Do **not** use the Nano for this suite. The unoptimized Unity test image has been observed to exceed the Nano's 2 KB SRAM. The source notes an observed data size of about 4.1 KB. The Mega's 8 KB SRAM provides sufficient headroom for this development test.

This is a test-harness resource requirement, not a production firmware requirement.

## 6. `test_cyclic_switchable`

Location:

```text
test/test_cyclic_switchable/test_cyclic_switchable.cpp
```

This suite verifies `CyclicSwitchable` pattern behavior using a fake wrapped switchable and injected timestamps.

### What it verifies

- `setOn()` starts the pattern and activates the wrapped output;
- the pattern advances one interval when its current interval expires;
- the sequence wraps after the final interval;
- no pattern progression occurs while the switchable is off.

The current implementation intentionally advances according to calls to `process()`; it is not designed to skip multiple historical intervals after a delayed loop call.

Typical command:

```bash
pio test -e nanoatmega328 -f test_cyclic_switchable
```

## 7. `test_liquid_level_sensor`

Location:

```text
test/test_liquid_level_sensor/test_liquid_level_sensor.cpp
```

The sensor logic exposes a deterministic `process(rawReading, nowMs)` seam so debounce behavior can be tested without wiring a physical sensor.

### What it verifies

- a single noisy reading does not change the debounced state;
- a minority noise burst does not flip the debounced state;
- a sustained new reading changes state and fires the correct callback;
- unchanged state does not re-fire before the periodic callback interval;
- periodic re-push occurs once the interval elapses;
- periodic re-push is disabled when the interval is `0`;
- `isTriggered()` / `isNotTriggered()` reflect the configured liquid-present polarity.

Typical command:

```bash
pio test -e nanoatmega328 -f test_liquid_level_sensor
```

These are logic/debounce tests. They do not prove the electrical behavior, mounting, cleanliness, or optical performance of a real level sensor; those checks belong to hardware bring-up and maintenance.

## 8. `test_push_button`

Location:

```text
test/test_push_button/test_push_button.cpp
```

Like the sensor suite, this test drives a deterministic `process(rawReading, nowMs)` method rather than relying on physical GPIO transitions.

### What it verifies

- isolated noise does not register as a press;
- minority noise does not change the debounced state;
- a sustained signal does change the debounced state;
- a valid short press fires the short-press callback;
- a valid long press fires the long-press callback;
- a press shorter than the debounce interval does not fire a callback.

Typical command:

```bash
pio test -e nanoatmega328 -f test_push_button
```

## 9. `test_ring_buffer`

Location:

```text
test/test_ring_buffer/test_ring_buffer.cpp
```

This is the regression suite for the generic `RingBuffer<T, N>` utility used by the button and liquid-level sensor debounce logic.

### What it verifies

- a fresh buffer is logically empty;
- `average()` is safe on an empty buffer and returns `T()`;
- `clear()` leaves the buffer logically empty;
- pushing after `clear()` behaves like a fresh buffer;
- partial-buffer logical ordering is correct;
- full-buffer ordering is correct;
- wrapped buffers return values oldest-to-newest;
- averages remain correct after wraparound;
- repeated wraparound remains correct;
- `fill(value)` fills the logical capacity with that value.

`clear()` may also reset backing storage to a known default value for debugging, but logical validity is determined by the buffer's count/index state.

Typical command:

```bash
pio test -e nanoatmega328 -f test_ring_buffer
```

## 10. `test_runnable`

Location:

```text
test/test_runnable/
```

This suite protects the `Runnable` self-registration mechanism.

### What it verifies

- `setupAll()` reaches registered objects;
- `loopAll()` reaches registered objects;
- the existing LIFO registration/traversal order remains stable;
- `Runnable` registration works when derived objects are defined in multiple translation units.

The additional `runnable_fixture_a.cpp` and `runnable_fixture_b.cpp` files are intentional. They protect against reintroducing a header-defined registry-storage problem that only appears when `Runnable.h` is used from multiple `.cpp` files.

The class is intentionally non-copyable and non-movable, and registered instances are expected to have static/global lifetime.

Typical command:

```bash
pio test -e nanoatmega328 -f test_runnable
```

## 11. `test_timed_switchable`

Location:

```text
test/test_timed_switchable/test_timed_switchable.cpp
```

This suite verifies the generic time-limited switch wrapper using a fake `AbstractSwitchable`.

### What it verifies

- an ON duration expires, switches the wrapped component off, and fires the callback;
- an OFF duration expires, switches the component on, and fires the callback;
- a maximum ON time of `0` means no automatic ON timeout at the utility-class level.

That final behavior is why AquariumATO's higher-level configuration validation must reject `PUMP_MAX_ON_MS=0`: `TimedSwitchable` itself deliberately treats zero as "timeout disabled."

Typical command:

```bash
pio test -e nanoatmega328 -f test_timed_switchable
```

## 12. `test_timer`

Location:

```text
test/test_timer/test_timer.cpp
```

This suite tests the generic timer independently of `millis()` by supplying explicit timestamps.

### What it verifies

- an OFF timer does nothing;
- a one-shot timer fires and turns itself off;
- an auto-restart timer can fire repeatedly;
- manually switching a timer off cancels a pending fire.

Typical command:

```bash
pio test -e nanoatmega328 -f test_timer
```

## 13. Running a regression pass

For a normal utility/config change, run the affected suite first, then the broader Nano test set.

Example targeted run:

```bash
pio test -e nanoatmega328 -f test_ring_buffer
```

Then run the Nano-compatible suites:

```bash
pio test -e nanoatmega328
```

Because `test_ato_fsm` is too large for the Nano test image, run it separately on the Mega:

```bash
pio test -e megaatmega2560 -f test_ato_fsm
```

When `test_ato_config_store` is included, remember that the board's stored ATO configuration will be overwritten.

After tests, also build the production firmware:

```bash
pio run -e nanoatmega328
```

A green Mega-only FSM suite does not replace the requirement that the real production image still builds for the Nano.

## 14. Adding a new test suite

Create a new directory under `test/` with a `test_` prefix:

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

Follow the conventions already used by the repository rather than copying this skeleton mechanically. In particular, use deterministic seams such as `process(input, nowMs)` when production `loop()` methods otherwise read `millis()` or GPIO directly.

### 14.1 Prefer behavior-level seams

The existing timer, button, sensor, and switchable suites avoid unstable real-time tests by injecting timestamps or raw input values. Continue that pattern when extending the firmware:

- test state transitions and outputs rather than private internals;
- use fake switchables/components at stable interfaces;
- inject time where practical;
- avoid requiring jumper wires just to generate deterministic input;
- keep hardware-specific electrical verification separate from logic tests.

### 14.2 Keep tests independent

Each test should establish its own starting state. This is particularly important when persistent state is involved. The EEPROM suite explicitly writes its own preconditions because EEPROM survives resets and previous test runs.

Do not rely on Unity test execution order to prepare state for the next test.

## 15. Troubleshooting tests

### Test firmware does not fit the Nano

If a development test image exceeds Nano SRAM or flash, try the Mega environment:

```bash
pio test -e megaatmega2560 -f <suite-name>
```

For `test_ato_fsm`, the Mega is already the required target.

Do not "fix" a test-memory problem by changing the production board environment.

### PlatformIO cannot find the board

Check the USB device and permissions as described in `ToolchainBootstrap.md`. On Linux, verify membership in the appropriate serial-access group and that the expected `/dev/ttyUSB*` or `/dev/ttyACM*` device exists.

### Suite is not discovered

Check that:

- the suite directory is under `test/`;
- its directory name begins with `test_`;
- the main test source and required helper `.cpp` files are inside that suite directory;
- you are using the actual directory name with `-f`.

### Duplicate `setup()` / `loop()` or production link conflicts

Make sure the Unity test source was not added under `src/` and is not being compiled into the production executable. Unity test firmware and production firmware are separate builds.

### Missing helper symbols

For a suite using several translation units, verify all helper `.cpp` files live inside the suite directory. `test_runnable` is the reference example.

### Config tests changed the device settings

This is expected. `test_ato_config_store` deliberately writes real EEPROM. Re-enter and `SAVE` the desired runtime settings afterward, or run the production firmware so its normal config/default recovery path can establish the intended configuration.

## 16. What the current tests do not prove

The test suite provides strong coverage of reusable timing/debounce utilities, configuration safety, parser behavior, and FSM transitions, but it is not a substitute for hardware validation.

The current automated suites do not comprehensively prove:

- real pump flow rate or dry-run behavior;
- sensor placement and optical performance;
- electrical noise immunity;
- plumbing and siphon prevention;
- real LED/buzzer visibility/audibility;
- all `AtoActions` entry/exit hardware side effects as an integrated system;
- long-duration endurance behavior on a complete assembled unit.

Use the Hardware Guide and User Guide procedures for physical-system checks, and perform a production Nano build after code changes.
