# AquariumATO User Guide

## Disclaimer

AquariumATO is a DIY aquarium automation project and should be used at your own risk. The design, firmware, documentation, and recommendations are provided without warranty or guarantee of fitness for a particular aquarium, livestock setup, electrical installation, or operating environment.

The builder and operator are responsible for verifying component ratings, wiring, power-supply suitability, sensor placement, pump behavior, plumbing, siphon prevention, enclosure protection, and safe installation around water.

No automated top-off system should be treated as completely fail-safe. Continue to inspect the aquarium, reservoir, sensors, pump, tubing, and electrical equipment regularly, and use independent safeguards where appropriate.

The project authors and contributors are not responsible for aquarium damage, livestock loss, water damage, electrical damage, equipment failure, or other losses resulting from the construction, modification, installation, configuration, or operation of the device, or from the use or modification of the firmware or schematics.

## 1. Purpose

AquariumATO is an automatic top-off controller that maintains aquarium or sump water level by running a freshwater top-off pump when the normal-level sensor reports that the water level has fallen.

This guide is for day-to-day installation, operation, configuration, and troubleshooting. It assumes the controller hardware is assembled and the firmware has already been installed.

AquariumATO monitors **water level only**. It does not measure salinity, temperature, pH, conductivity, water quality, flow rate, or other aquarium parameters.

> **Important:** Treat the ATO as a supplementary aid, not as a substitute for regular aquarium inspection. Periodically verify the sensors, tubing, reservoir, pump, and water level by hand.

---

## 2. Before You Start

Before putting the controller into unattended service:

1. Verify that the normal-level and high-level sensors are installed at the intended heights.
2. If fitted, verify the low-level and reservoir sensors.
3. Confirm that the pump delivers water in the correct direction.
4. Confirm that tubing is secure and cannot fall out of the aquarium or sump.
5. Test manual dispensing while watching the water level.
6. Test that the high-level sensor stops dispensing and produces a high-water alarm.
7. Confirm that the configured maximum pump runtime is suitable for the installation.

### Prevent siphoning

**Arrange the reservoir and tubing so water cannot continue flowing by siphon after the pump turns off.** A common safe arrangement is to keep the top-off reservoir below the tubing outlet and ensure the outlet is not submerged in a way that can establish a siphon.

The electronic pump cutoff cannot stop water that continues to move through the hose by gravity or siphon.

For sensor placement, wiring, power, and plumbing details, see `HardwareGuide.md`.

---

## 3. Normal Operating Cycle

During normal operation the controller spends most of its time in **Idle**.

A typical automatic top-off cycle is:

1. **Idle** — the green LED blinks slowly.
2. The water level falls below the normal-level sensor.
3. **Automatic dispensing** starts — the green LED stays continuously on and the pump runs.
4. When the normal-level sensor detects water again, the pump stops.
5. The controller returns to **Idle**.

If the high-level sensor is triggered while dispensing, the controller immediately leaves the dispensing state and enters **Water Level High**.

If the pump runs for its configured maximum runtime before the expected normal-level condition is restored, automatic dispensing stops and the controller enters **Reservoir Empty**. This prevents an automatic pump run from continuing indefinitely.

---

## 4. Status LEDs and Buzzer

The LEDs are the quickest way to identify the controller state.

| Indication | State | Meaning | Recommended action |
| --- | --- | --- | --- |
| Green LED slow blink | Idle | Normal monitoring; pump is off | None |
| Green LED solid | Dispensing — automatic | Water level dropped and the controller is topping off | Observe if the run is unusually long |
| Green LED fast blink | Dispensing — manual | Pump was started manually | Watch the water level; short-press to stop |
| Yellow LED slow blink | Sleeping | Automatic operation is temporarily suspended | Short- or long-press to wake, or wait for sleep timeout |
| Red LED fast blink + high-water buzzer pattern | Water Level High | High-level failsafe sensor is active | Inspect water level, sensor position, pump/tubing, and siphon risk |
| Red LED fast blink + low-water buzzer pattern | Water Level Low | Optional low-level sensor reports an abnormally low level | Inspect water level, normal sensor, pump, reservoir, and plumbing |
| Red LED slow blink + reservoir buzzer pattern | Reservoir Empty | Reservoir sensor reports low/empty, or an automatic pump run reached its maximum runtime | Refill/check reservoir; inspect pump and tubing |
| Green + red LEDs slow blink + idle-warning buzzer | Idle For Too Long | No normal top-off event has occurred within the configured idle interval | Check that the normal-level sensor is clean, correctly positioned, and changing state |
| Red LED fast blink + error buzzer pattern | Error | Controller received an inconsistent or unexpected event combination | Inspect sensors and wiring; use TRACE if needed |

### Buzzer patterns

Alarm patterns repeat in a long cycle. The audible sequence is played near the beginning of the cycle, repeated again around 30 seconds later, and then remains quiet until the pattern cycle repeats at approximately 10 minutes.

The patterns are intentionally distinct:

| State | Audible signature |
| --- | --- |
| Reservoir Empty | 3 long buzzes |
| Error | 2 short buzzes, then 1 long buzz |
| Water Level Low | 1 long buzz, then 2 short buzzes |
| Water Level High | 5 long buzzes |
| Idle For Too Long | 5 short buzzes, then 1 longer buzz |

---

## 5. Push Button

The controller recognizes two button actions:

- **Short press** — normal press/release.
- **Long press** — hold for approximately 3 seconds with the default firmware settings.

The result depends on the current state.

### 5.1 Short press

| Current state | Short-press action |
| --- | --- |
| Idle | Start manual dispensing |
| Automatic dispensing | Stop dispensing and return to Idle |
| Manual dispensing | Stop dispensing and return to Idle |
| Water Level Low | Start manual dispensing |
| Water Level High | Acknowledge/leave the alarm state and return to Idle |
| Reservoir Empty | Acknowledge/leave the alarm state and return to Idle |
| Sleeping | Wake and return to Idle |
| Idle For Too Long | Acknowledge/leave the warning and return to Idle |
| Error | Acknowledge/leave Error and return to Idle |

A short press only changes the controller state; it does not repair the condition that caused an alarm. If the underlying condition is still present, the corresponding sensor event can cause the warning state to return.

### 5.2 Long press

From normal, dispensing, warning, or error states, a long press enters **Sleeping**.

While already sleeping, a long press wakes the controller and returns it to **Idle**.

The sleep timer also wakes the controller automatically after the configured maximum sleep duration.

---

## 6. Sleep Mode

Sleep mode temporarily suspends normal automatic top-off operation.

The yellow LED blinks slowly while the controller is sleeping. The pump is off.

Use sleep mode when temporarily working around the sump/aquarium, performing maintenance, or when automatic top-off should be paused without powering down the controller.

With the current compiled defaults, the maximum sleep duration is **2 hours**. The duration can be changed through the serial configuration console.

You can wake the controller at any time with either a short or long button press.

---

## 7. Idle-Too-Long Warning

While in Idle, the controller runs an idle timer. With the current compiled defaults, the warning occurs after **6 hours** without leaving Idle.

This warning does not necessarily mean something has failed. It means the controller has remained idle longer than expected and asks the user to verify that the level sensing system is still operating correctly.

Check:

- whether the normal-level sensor is at the correct height;
- whether the sensor surface is dirty or covered by biofilm;
- whether the water level is actually changing as expected;
- whether the sensor cable or connector is loose;
- whether the top-off system has simply not needed to run during that interval.

A short press acknowledges the warning and returns the controller to Idle.

---

## 8. Reservoir Empty / Pump Runtime Protection

The **Reservoir Empty** state can be entered in two important ways:

- the optional reservoir sensor reports that the top-off reservoir is low/empty; or
- during automatic dispensing, the pump reaches its configured maximum continuous runtime before normal water level is restored.

With the current compiled defaults, `PUMP_MAX_ON_MS` is **90,000 ms (90 seconds)**.

For safety, firmware accepts pump maximum runtimes only in the inclusive range:

- minimum: **5,000 ms (5 seconds)**;
- maximum: **180,000 ms (3 minutes)**.

A configured value of `0` is not permitted because the pump must retain a finite automatic runtime cutoff.

If Reservoir Empty occurs unexpectedly, check the reservoir level, pump inlet, tubing, pump operation, and normal-level sensor before simply clearing the alarm.

### Pump Runtime Limit in Manual Mode

The configured `PUMP_MAX_ON_MS` safety limit also applies when the pump is started manually.

If manual dispensing remains active for the configured maximum pump-on time, the controller automatically stops the pump and returns to `Idle`. Unlike an automatic-dispense timeout, this does **not** indicate a `Reservoir Empty` condition.

This provides a finite runtime limit even if manual dispensing is accidentally left active.
The allowed `PUMP_MAX_ON_MS` range is **5,000–180,000 ms** (5 seconds to 3 minutes), inclusive; a value of `0` is not permitted.

---

## 9. Water-Level Alarms

### 9.1 Water Level High

The high-level sensor is an independent overfill/failsafe input. When it becomes active, dispensing is stopped and the controller enters **Water Level High**.

Check:

- actual sump/aquarium water level;
- whether the high sensor has been moved or fouled;
- whether the normal sensor failed to detect the expected level;
- whether the pump remained physically on;
- whether water is continuing through the tubing by siphon even though the pump is off.

The controller returns to Idle automatically if the high-level sensor clears. A short press can also return it to Idle, but do not use the button as a substitute for investigating a real high-water condition.

### 9.2 Water Level Low

This state is available when the optional low-level sensor is compiled into the firmware and installed.

It indicates that the water level has fallen below the low-level safety threshold. Inspect the normal-level sensor, pump, reservoir, and tubing before relying on manual dispensing.

---

## 10. Error State

**Error** is different from the normal high-water or reservoir alarms. It is used when the finite-state machine receives a combination of sensor/timer events that is inconsistent with the state the controller believes it is in.

Examples include contradictory level indications in certain alarm states or timer events that should not normally occur in the current state.

When Error occurs:

1. Inspect the actual water level first.
2. Inspect sensor positioning and cleanliness.
3. Check sensor connectors and wiring.
4. Check for water or corrosion around connectors/electronics.
5. If the problem is intermittent, use `TRACE ON` or `TRACE ALL` through the serial console to see the events reaching the controller.

A short press returns the controller to Idle. A long press enters Sleep.

If Error repeatedly returns, investigate the hardware or sensor signals rather than repeatedly clearing it.

---

## 11. Serial Configuration Console

The firmware exposes a simple serial console at **9600 baud**.

It can be used to inspect and change runtime timer values and to enable state-machine tracing.

### 11.1 Commands

| Command | Purpose |
| --- | --- |
| `HELP` | Show available commands |
| `GET` | Show the current configuration |
| `SET <NAME> <VALUE>` | Change a setting immediately in RAM; not yet saved |
| `SAVE` | Save the current configuration to EEPROM |
| `RESET` | Restore compiled defaults in RAM; not yet saved |
| `TRACE ON` | Log state transitions |
| `TRACE ALL` | Log transitions plus events that did not match a transition rule |
| `TRACE OFF` | Disable tracing |

Commands are line-oriented. Unexpected arguments and invalid values are rejected with an `ERROR` message.

### 11.2 Configurable values

| Name | Meaning | Current compiled default |
| --- | --- | ---: |
| `SLEEP_MAX_MS` | Maximum time the controller remains asleep before waking automatically | `7200000` (2 h) |
| `IDLE_MAX_MS` | Maximum time continuously in Idle before the Idle-Too-Long warning | `21600000` (6 h) |
| `PUMP_MAX_ON_MS` | Maximum single continuous pump runtime | `90000` (90 s) |

Validation:

- `PUMP_MAX_ON_MS` must be between `5000` and `180000` ms inclusive.
- `SLEEP_MAX_MS` and `IDLE_MAX_MS` must be greater than `0`. A value of `0` makes the timer elapse on every loop, so it is rejected outright.
- `SLEEP_MAX_MS` or `IDLE_MAX_MS` below `60000` ms (1 minute) is accepted but triggers a warning — see [11.5](#115-timer-value-warnings).

### 11.3 View current settings

Enter:

```text
GET
```

The controller prints values similar to:

```text
SLEEP_MAX_MS=7200000
IDLE_MAX_MS=21600000
PUMP_MAX_ON_MS=90000
```

### 11.4 Change a value

For example, to change the maximum pump runtime to 60 seconds:

```text
SET PUMP_MAX_ON_MS 60000
```

The new value is applied immediately, but it is **not persistent yet**.

Check it with:

```text
GET
```

Then persist it:

```text
SAVE
```

### 11.5 Timer value warnings

`SLEEP_MAX_MS` and `IDLE_MAX_MS` accept any value above `0`, but a value below `60000` ms (1 minute) is almost always a mistake: the corresponding timer then elapses within a minute of the controller entering Sleep or Idle. Such a value is applied anyway, with a `WARN` line:

- on `SET`, right after `Applied`;
- on `GET`, after the value list;
- once at runtime, the moment the FSM actually enters `IdleForTooLong` or `Sleeping` with that value in effect (this also catches a short value that was already saved to EEPROM).

Real session (setting a deliberately short idle window, then reverting):

```text
get
SLEEP_MAX_MS=7200000
IDLE_MAX_MS=21600000
PUMP_MAX_ON_MS=90000

set idle_max_ms 300
WARN IDLE_MAX_MS below 60000 ms; its timer elapses within a minute of entering that state
Applied (not yet saved; use SAVE).
WARN entered IdleForTooLong with IDLE_MAX_MS=300 ms (below advisory minimum)

reset
Reset to compiled defaults (not yet saved; use SAVE).
save
Saved to EEPROM.
```

Commands are case-insensitive.

### 11.6 Restore defaults

Enter:

```text
RESET
```

This immediately restores the compiled defaults in RAM, but does not save them to EEPROM. To make the reset persistent across reboot:

```text
SAVE
```

### 11.7 Trace FSM activity

For ordinary troubleshooting:

```text
TRACE ON
```

This logs events that cause real state changes.

For deeper diagnostics:

```text
TRACE ALL
```

This also reports events that reached the FSM but were ignored because no transition rule applied.

Disable tracing with:

```text
TRACE OFF
```

TRACE is a debugging feature and is not persisted as part of the EEPROM configuration.

---

## 12. Sensor Maintenance

Reliable level sensing depends on keeping the sensing surfaces clean and correctly positioned.

### IR sensor quick check

**Tip:** You can perform a quick check of an IR sensor with a phone camera. While the sensor is powered, look at the IR emitter through the camera; on many phones it appears as a faint violet/purple glow even though the infrared light is not visible to the naked eye. Some phone cameras filter infrared more strongly than others, so absence of a visible glow is not by itself proof that the sensor has failed.

### Light, algae, and biofilm

**Tip:** If the IR sensor is exposed to external light, algae or biofilm can build up on the sensing surface and interfere with reliable operation. Reduce exposure from room lighting, windows, refugium lights, or other nearby light sources where possible. If deposits form, gently wipe the sensor crystal with a soft cloth or swab dampened with **5% white vinegar**, then rinse/wipe appropriately before returning it to service.

### Floating debris

**Tip:** Floating dirt or debris can collect around the IR sensor, stick to the sensing surface, and reduce reliability. Keep the area around the sensor reasonably clean and remove deposits from the sensor crystal as needed using a soft wipe dampened with **5% white vinegar**.

After cleaning or repositioning any sensor, manually verify its operation before leaving the ATO unattended.

---

## 13. Pump maintenance

Periodically run the pump **without the tubing attached** in a small container of **5% white vinegar** for a few minutes to help dissolve mineral deposits, then rinse and run it in clean RO/DI water for another few minutes before returning it to service. This is especially useful when the ATO is used with **kalkwasser**, which can leave calcium deposits inside the pump and reduce flow or cause sticking.

Do not run the pump dry during cleaning; keep the pump inlet fully submerged in the cleaning solution and rinse water.

---

## 14. Troubleshooting

| Symptom | Likely checks |
| --- | --- |
| Pump never starts automatically | Normal-level sensor state/position, sensor wiring, Sleep state, controller power |
| Pump starts but stops after maximum runtime | Reservoir empty, blocked tubing, failed/weak pump, normal sensor not detecting restored level |
| High-water alarm | Actual water level, siphon, stuck pump, normal sensor, high sensor contamination/position |
| Reservoir Empty alarm with water still present | Reservoir sensor position/wiring if installed; pump runtime may also have expired |
| Idle-Too-Long warning | Normal sensor cleanliness/position, unusually stable water level, wiring |
| Repeated Error state | Contradictory sensor readings, loose connectors, contamination, electrical noise; use TRACE |
| IR sensor appears unreliable | Clean sensing surface, shield from external/refugium light, remove debris/biofilm |
| Pump runs manually but not automatically | Normal-level sensor and FSM state; check serial TRACE output |
| Controller appears unresponsive | Verify power; if firmware genuinely hangs, the watchdog is designed to reset the MCU |

### When a power cycle is reasonable

A power cycle can be useful after maintenance or to recover from an unusual transient condition, but repeated alarms after reboot usually indicate a real sensor, plumbing, power, or wiring problem that should be investigated.

---

## 15. Optional Sensors

The normal-level and high-level sensors are mandatory in the current firmware architecture.

The low-level and reservoir sensors are optional and are included only when the firmware is compiled with the corresponding build flags:

- `ATO_HAS_LOW_SENSOR`
- `ATO_HAS_RESERVOIR_SENSOR`

If an optional sensor was not included in the firmware build, its associated alarm behavior is not active even if hardware is physically connected to the corresponding PCB connector.

If you are unsure which firmware build is installed, consult whoever built/flashed the controller or use the development documentation.

---

## 16. Safety Checklist

Periodically verify all of the following:

- the top-off reservoir contains enough water;
- the pump inlet is submerged when required by the pump type;
- tubing is secure and free of kinks/blockage;
- the installation cannot create an uncontrolled siphon;
- normal and high sensors respond correctly;
- optional low/reservoir sensors respond correctly if installed;
- sensing surfaces are clean;
- the high-level failsafe stops dispensing;
- the pump stops when commanded;
- electrical connectors remain dry and free of corrosion/salt creep;
- the configured pump maximum runtime remains appropriate for the installation.

Do not rely on a single electronic safeguard to protect an aquarium from overflow. Physical layout, reservoir sizing, siphon prevention, regular inspection, and independent safeguards are all valuable layers of protection.

---

## 17. What AquariumATO Does Not Do

AquariumATO does not currently:

- measure salinity;
- measure temperature;
- measure pH or other water chemistry;
- measure actual pump flow;
- verify that water physically reached the aquarium;
- prevent a gravity siphon after the pump has stopped;
- provide network/cloud monitoring or notifications;
- replace routine aquarium inspection and maintenance.

Its responsibility is deliberately narrow: **monitor configured water-level sensors, control the top-off pump, and report abnormal level/control conditions through LEDs and the buzzer.**
