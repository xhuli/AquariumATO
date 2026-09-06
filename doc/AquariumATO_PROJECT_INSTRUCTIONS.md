# AquariumATO — Project Instructions

Repository: <https://github.com/xhuli/AquariumATO>

Fast orientation for Claude in this project — what it is, where things live, which guide to search for specifics. Kept deliberately short; detail lives in the five guides under `doc/` and root `README.md`, not duplicated here.

---

## 1. What this project is

AquariumATO is an **Auto Top-Off (ATO) controller** for a freshwater/reef aquarium: watches water level via liquid-level sensors, runs a pump to top off evaporated water, with independent safety layers against overfill, dry-running, and runaway pumping.

- **Firmware:** C++/Arduino, PlatformIO, targeting **Arduino Nano / ATmega328, old bootloader** (`nanoatmega328`, NOT `nanoatmega328new`). A `megaatmega2560` environment exists only as a dev test target for Unity suites needing more SRAM.
- **Hardware:** custom PCB (EasyEDA, "Auto Top Off" rev 1.3) — see `doc/images/` and `doc/HardwareGuide.md`.
- Also buildable via CLion/CMake (`CMakeLists.txt`) for editing/navigation/upload; PlatformIO remains authoritative for build/test.
- Single-maintainer project (xhuli / Gorjan Djundev). Favors simplicity/explicitness over abstraction — data-driven FSM, no dynamic allocation in the hot path, no RTOS.

---

## 2. Where things live

```text
platformio.ini        # nanoatmega328 (production) + megaatmega2560 (test)
CMakeLists.txt         # CLion/CMake build + upload targets
src/main.cpp           # composition root: wiring, setup()/loop()
include/ato/
  AtoFsm.h             # transition-table FSM
  AtoActions.h         # per-state LED/buzzer/pump actions
  AtoConfig.h          # EEPROM config store (magic/version/CRC)
  AtoConfigConsole.h   # serial console (GET/SET/SAVE/RESET/TRACE)
lib/                   # generic, ATO-agnostic: Runnable, RingBuffer,
                        # Switchable family (obsolete code removed),
                        # LiquidLevelSensor, PushButton, Timer
test/                  # one Unity suite per subfolder
doc/                   # the five guides below + images
```

**Rule of thumb:** `lib/` stays generic/reusable, no knowledge of aquariums/FSM. ATO-specific code belongs in `include/ato/` or `src/main.cpp`.

---

## 3. The five guides — search into these for specifics

| Guide | Covers |
|---|---|
| `README.md` | Overview, build/upload commands, feature list, status-LED summary |
| `doc/UserGuide.md` | Operating the device: LEDs, buzzer, button, sleep, config console, troubleshooting |
| `doc/HardwareGuide.md` | Pin map, power architecture (`VCC` vs `PWR`), sensor types, pump driver, BOM, wet bring-up |
| `doc/ToolchainBootstrap.md` | Fresh-machine setup: PlatformIO, VS Code, CLion/CMake, udev rules, upload baud gotchas |
| `doc/TestingGuide.md` | All 9 Unity suites, per-suite caveats, adding tests, EEPROM-destructive warning |
| `doc/DevelopmentGuide.md` | FSM design, `Runnable`, `AtoActions`, debouncing, config architecture, extension workflows |

Search the relevant guide for real detail rather than relying on this summary.

---

## 4. Firmware essentials

**`Runnable` self-registration:** every component self-registers on construction; `Runnable::setupAll()`/`loopAll()` walk the registry. Safe across translation units; instances are non-copyable/non-movable (each is a unique registered object). `main.cpp`'s `loop()` is just `Runnable::loopAll(); wdt_reset();`. New component → new `Runnable` subclass; no manual registration step.

**Callbacks into the FSM:** sensors/timers/button detect conditions and fire callbacks that dispatch `Event`s (wired in `main.cpp`). All behavior lives in `AtoFsm`/`AtoActions`, never in the sensor/button/timer classes.

**FSM is a data-driven transition table, not a nested switch.** `TRANSITION_TABLE` is a flat array of `{fromState, event, toState}` rows — the single source of truth for legal transitions. No match leaves state unchanged (observable via `TRACE ALL`). This replaced an earlier nested `switch/switch` dispatch with a real missing-`break` fallthrough bug; the table design eliminates that bug class structurally. **Add rows for new states/events — don't reintroduce switch-based dispatch.**

**`AtoActions`** implements per-state entry/exit behavior. LED/buzzer patterns are `static constexpr` arrays, each with a compile-time `static_assert` on total duration — added after a real drift was found (one pattern summed ~3.6s over its intended 10-minute total). This makes future arithmetic mistakes build failures, not silent runtime drift.

**Sensor debouncing, two layers:** software (`RingBuffer<uint8_t,16>` majority-vote averaging; empty/clear/average semantics are regression-tested) and hardware (PCB RC filtering on Normal/Low front-ends). Both sensor/button classes expose `process(rawReading, nowMs)` so Unity tests can drive debounce/timing with fabricated inputs — no jumper wiring needed. If tuning responsiveness, check both layers.

**Configuration:** `AtoConfig`/`AtoConfigStore` persist `SLEEP_MAX_MS`/`IDLE_MAX_MS`/`PUMP_MAX_ON_MS` to EEPROM behind a magic/version/CRC guard — corrupt/blank/mismatched EEPROM always falls back to (and self-heals with) compiled defaults. `PUMP_MAX_ON_MS` also has centralized semantic validation (5,000–180,000 ms inclusive; `0` rejected, since `TimedSwitchable` treats `0` as "no timeout" and that would disable the pump cutoff) — unsafe values from either the console or EEPROM are rejected before they can reach the live pump setter. `SLEEP_MAX_MS`/`IDLE_MAX_MS` likewise reject `0` (`Timer` treats `0` as "always elapsed") and emit a non-fatal `WARN` below 60 s, including once at runtime on entering the affected state. `AtoConfigConsole` (non-blocking `Runnable`) exposes `GET`/`SET`/`SAVE`/`RESET`/`TRACE ON|ALL|OFF` over `Serial` at 9600 baud, with a fail-closed parser: exact `uint32_t` decimal parsing with overflow rejection, strict argument counts, oversized lines discarded whole, and standardized `ERROR` responses — no rejected input has a side effect. `TRACE` hooks `AtoFsm::dispatch()` via an optional callback — `AtoFsm` itself stays Serial-agnostic — and prints via flash-resident (`F()`) strings. This replaced a previously-unused `ArduinoLog` dependency, now fully removed.

---

## 5. Build & tooling

- PlatformIO envs: `nanoatmega328` (production), `megaatmega2560` (test-only). Shared settings live in `[env]`; per-env sections only override what differs.
- No external non-registry dependencies (the old `ArduinoLog` GitHub `lib_deps` entry is gone).
- Watchdog enabled at 2s in `setup()`. **Never block anywhere in the loop path.**
- Optional sensors (Low/Reservoir) are `platformio.ini` build flags (`-D ATO_HAS_LOW_SENSOR`, `-D ATO_HAS_RESERVOIR_SENSOR`), not commented-out code. Normal/High always compiled in.

---

## 6. Conventions

1. New hardware → new `Runnable` subclass (self-registration is automatic).
2. Keep `lib/` generic; ATO-specific logic goes in `include/ato/`.
3. New states/events → add `TRANSITION_TABLE` rows + a no-match regression test.
4. New states need both a table entry and an `onEntryXState()` in `AtoActions` (`exit()` is state-agnostic).
5. New buzzer/LED patterns → `static constexpr` + `static_assert` on total duration.
6. No blocking delays in the loop path (2s watchdog).
7. Time-based `lib/` logic exposes `process(nowMs)` rather than reading `millis()` internally — this is what makes every timing class independently Unity-testable with fabricated timestamps.
8. Don't silently "fix" a discovered numeric/behavioral drift as a side effect of unrelated work — surface it explicitly for a decision.
9. Add tests alongside behavior changes, following existing seam/fake-component patterns (`doc/TestingGuide.md` §14).

---

## 7. Current status

No open questions are currently tracked. Add new ones here with enough context to act on later, and remove them once resolved rather than leaving stale entries.
