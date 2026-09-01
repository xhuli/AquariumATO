# Toolchain Bootstrap

This guide describes how to take a fresh Ubuntu development machine to a working AquariumATO build environment.

It is intended for developers setting up the project for the first time. Firmware architecture and coding conventions belong in `DevelopmentGuide.md`; test-suite details belong in `TestingGuide.md`.

## 1. Toolchain Overview

AquariumATO is an Arduino AVR project built primarily with PlatformIO.

The repository currently defines two PlatformIO environments:

| Environment | Board | Purpose |
| --- | --- | --- |
| `nanoatmega328` | Arduino Nano / ATmega328P | Production firmware and most tests |
| `megaatmega2560` | Arduino Mega 2560 | Recommended default test environment; all Unity suites can run safely here |

> **Important:** The production target is `nanoatmega328`, **not** `nanoatmega328new`.

The repository also contains a `CMakeLists.txt` used for CLion code navigation, building, and uploading. PlatformIO remains the authoritative build/test environment for the project.

## 2. Install Base Packages on Ubuntu

Install the common build utilities, AVR compiler/toolchain, and `avrdude`:

```shell
sudo apt-get update
sudo apt-get install gcc build-essential
sudo apt-get install gcc-avr binutils-avr avr-libc gdb-avr
sudo apt-get install avrdude
sudo apt-get install libusb-dev
sudo apt-get install curl python3
```

Verify the AVR tools are available:

```shell
avr-gcc --version
avr-g++ --version
avrdude -?
```

## 3. Install PlatformIO Core

The existing project setup expects PlatformIO's default per-user installation under:

```text
~/.platformio
```

One supported installation method on Ubuntu is PlatformIO's installer script:

```shell
mkdir -p "$HOME/Downloads"
cd "$HOME/Downloads"

curl -fsSL -o get-platformio.py \
  https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py

python3 get-platformio.py
```

Create command-line links if the installer did not already expose them on your `PATH`:

```shell
mkdir -p "$HOME/.local/bin"

ln -sf "$HOME/.platformio/penv/bin/platformio" "$HOME/.local/bin/platformio"
ln -sf "$HOME/.platformio/penv/bin/pio" "$HOME/.local/bin/pio"
ln -sf "$HOME/.platformio/penv/bin/piodebuggdb" "$HOME/.local/bin/piodebuggdb"
```

Ensure `~/.local/bin` is on your shell path. For example:

```shell
echo 'export PATH="$PATH:$HOME/.local/bin"' >> "$HOME/.profile"
```

Reload the current shell profile so the updated `PATH` takes effect:

```bash
source "$HOME/.profile"
```

Then verify that PlatformIO is available:

```bash
pio --version
```

Alternatively, log out and back in to start a new session with the updated environment.

## 4. Open the Project

Clone or unpack the repository, then enter its root directory:

```shell
cd AquariumATO-master
```

The project root should contain at least:

```text
platformio.ini
CMakeLists.txt
src/
include/
lib/
test/
doc/
```

PlatformIO commands in this guide assume they are run from this directory.

## 5. Build the Production Firmware

Build the Arduino Nano production firmware with:

```shell
pio run -e nanoatmega328
```

Do not substitute `nanoatmega328new`. The repository intentionally targets the `nanoatmega328` PlatformIO board definition.

PlatformIO will download its managed AVR framework/toolchain packages into `~/.platformio` as required.

### Optional sensor build flags

The Normal and High level sensors are always compiled in.

The Low and Reservoir sensors are optional. They are controlled in `platformio.ini` with:

```ini
-D ATO_HAS_LOW_SENSOR
-D ATO_HAS_RESERVOIR_SENSOR
```

Uncomment only the flags corresponding to sensors physically installed in the unit being built.

After changing build flags, rebuild the firmware before uploading it.

## 6. Connect the Arduino Nano

Connect the Nano over USB and determine the serial device assigned by Linux:

```shell
ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
```

The repository's CMake configuration currently uses:

```text
/dev/ttyUSB0
```

as its default upload port. Your system may assign a different device.

### Linux device permissions

If uploads fail with a permission error, install/configure the PlatformIO udev rules documented at:

<https://docs.platformio.org/en/latest/core/installation/udev-rules.html>

Depending on the Ubuntu setup, serial-port access may also require the user to belong to the appropriate serial-device group. After changing group membership or installing udev rules, log out and back in before testing again.

## 7. Upload with PlatformIO

With the production Nano connected:

```shell
pio run -e nanoatmega328 -t upload
```

If PlatformIO cannot determine the port automatically, specify the correct upload port using the normal PlatformIO configuration/command-line mechanism for your machine.

### Nano upload baud rate

For this project's `nanoatmega328` target, the expected classic Nano upload rate is:

```text
57600 baud
```

This is distinct from the firmware's serial console baud rate.

> **Do not confuse upload baud with serial-monitor baud.** Uploading the Nano uses the board/bootloader upload settings; the AquariumATO runtime console uses **9600 baud**.

## 8. Open the Runtime Serial Console

The firmware initializes `Serial` at:

```text
9600 baud
```

After uploading, open a serial monitor at 9600 baud. For example:

```shell
pio device monitor -b 9600
```

The runtime configuration console supports commands such as:

```text
HELP
GET
SET ...
SAVE
RESET
TRACE ON
TRACE ALL
TRACE OFF
```

The command semantics are documented for end users in `UserGuide.md` and for developers in `DevelopmentGuide.md`.

## 9. VS Code Setup

VS Code is optional; PlatformIO CLI commands work independently of any IDE.

On Ubuntu, install VS Code using your preferred package source or the App Center.

Useful extensions for this repository include:

- C/C++
- PlatformIO IDE
- Markdown linting
- YAML support

The previous project notes also used JetBrains-style icons/themes and duplicate-line helpers, but those are personal workflow choices rather than project requirements.

After installing the PlatformIO IDE extension, open the repository root containing `platformio.ini`.

Use PlatformIO's project tasks for build/upload/test, or run the CLI commands from the integrated terminal.

## 10. CLion Setup

The repository includes a hand-maintained `CMakeLists.txt` so CLion can understand the AVR project and expose build/upload targets.

### 10.1 Install PlatformIO support

In CLion:

1. Open **Settings** (`Ctrl+Alt+S`).
2. Open **Plugins**.
3. Search the Marketplace for **PlatformIO**.
4. Install the plugin and restart CLion.

### 10.2 Create the AVR toolchain

Open: **Settings → Build, Execution, Deployment → Toolchains**

Create a System toolchain named, for example:

```text
Arduino
```

Point the compilers to PlatformIO's managed AVR toolchain:

```text
C Compiler:
~/.platformio/packages/toolchain-atmelavr/bin/avr-gcc

C++ Compiler:
~/.platformio/packages/toolchain-atmelavr/bin/avr-g++
```

Use the expanded absolute path if CLion does not resolve `~` in configuration fields.

### 10.3 Create the Nano CMake profile

Open: **Settings → Build, Execution, Deployment → CMake**

Create a profile such as:

```text
Name: Nano
Toolchain: Arduino
```

The repository's CMake configuration sets the MCU to:

```text
atmega328p
```

and builds against the Arduino AVR core supplied by PlatformIO.

## 11. CMake Machine-Specific Configuration

`CMakeLists.txt` contains a clearly marked **Machine-specific stuff** section.

Review this section whenever the project is opened on a new machine.

### `AVR_DUDE_EXECUTABLE`

Current default:

```cmake
set(AVR_DUDE_EXECUTABLE /usr/bin/avrdude)
```

Change it if `avrdude` is installed elsewhere.

### `AVR_DUDE_PORT`

Current default:

```cmake
set(AVR_DUDE_PORT /dev/ttyUSB0)
```

Change it to the serial device assigned to your Nano.

### `PIO_HOME`

Current default:

```cmake
set(PIO_HOME $ENV{HOME}/.platformio)
```

This must point at PlatformIO's package directory.

### `PIO_FRAMEWORK`

The Arduino AVR framework is expected at:

```cmake
${PIO_HOME}/packages/framework-arduino-avr
```

CMake uses this directory for `Arduino.h`, the Arduino core implementation, EEPROM support, and board variant headers.

### `PIO_BOARD_VARIANT`

The project currently uses:

```cmake
set(PIO_BOARD_VARIANT eightanaloginputs)
```

This matches the Arduino Nano variant used by the production target.

If CMake reports that `pins_arduino.h` cannot be found, inspect the installed variants:

```shell
ls "$HOME/.platformio/packages/framework-arduino-avr/variants/"
```

Then verify that `PIO_BOARD_VARIANT` matches the variant directory installed by the current PlatformIO Arduino AVR package.

## 12. CLion Build and Upload Targets

The CMake project defines the main executable:

```text
AquariumAto
```

Building it also creates an Intel HEX file in the CMake build directory.

The repository also defines an `Upload` custom target using `avrdude`.

Its Nano upload configuration uses:

```text
MCU: atmega328p
Programmer protocol: arduino
Baud: 57600
Port: AVR_DUDE_PORT
```

Before using the CLion `Upload` target, verify `AVR_DUDE_PORT` in `CMakeLists.txt` matches the connected board.

The `UnityTests` CMake target is intentionally a source/navigation target only. It does **not** compile or execute the Unity tests. Run tests with PlatformIO as described in `TestingGuide.md`.

## 13. Basic Test Sanity Check

After setting up the toolchain, run the test suite to verify that PlatformIO can build, upload, and execute the project's tests correctly.

### Recommended: run all tests on Mega

The `megaatmega2560` environment is the recommended default for testing.
All project test suites can run safely on this target, including the complete FSM suite.

With an Arduino Mega 2560 connected:

```shell
pio test -e megaatmega2560
```

A successful full run confirms that the PlatformIO installation, compiler toolchain, upload path, serial test runner, and project test configuration are working.

The Mega environment is intended for development and testing only.
The production firmware target remains `nanoatmega328`.

### If only a Nano is available

The Nano can run the majority of the test suites, but its ATmega328 has significantly less SRAM than the Mega. The complete `test_ato_fsm` suite does not fit reliably on this target.

Run Nano-compatible suites **individually**, for example:

```shell
pio test -e nanoatmega328 -f test_timed_switchable
```

Repeat this for the required suites, excluding `test_ato_fsm`.

Do not use an unfiltered:

```shell
pio test -e nanoatmega328
```

as a full Nano regression command, because PlatformIO will also discover `test_ato_fsm`.

If the FSM suite must be run, use the Mega test target:

```shell
pio test -e megaatmega2560 -f test_ato_fsm
```

If only a Nano is available, skip the FSM suite and run the remaining suites individually.

For the complete list of test suites, suite-specific requirements, EEPROM warnings, and instructions for adding tests, see [`TestingGuide.md`](TestingGuide.md).

## 14. Common Setup Problems

### `pio: command not found`

Check:

```shell
ls "$HOME/.platformio/penv/bin/pio"
```

and verify `~/.local/bin` or PlatformIO's virtual-environment `bin` directory is on `PATH`.

### Upload permission denied

Check the device permissions and install the PlatformIO udev rules. Reconnect the Nano after updating the rules and re-login if group membership changed.

### Wrong serial device

Re-run:

```shell
ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
```

and update the upload port being used by the selected tool.

### Upload fails when using 115200 baud

The production environment targets the classic `nanoatmega328` bootloader configuration. The repository's CMake upload target explicitly uses **57600 baud**.

Do not change it to 115200 merely because that value is common on other Nano bootloader variants.

### Serial console appears as garbage or shows nothing useful

Confirm the terminal is configured for **9600 baud**. The 57600 value used during firmware upload is not the runtime console speed.

### CLion cannot find `Arduino.h`

Verify:

```text
PIO_HOME
PIO_FRAMEWORK
PIO_BOARD_VARIANT
```

in `CMakeLists.txt` against the actual contents of `~/.platformio/packages/`.

### CMake builds but PlatformIO behaves differently

Treat PlatformIO as the authoritative project configuration. `platformio.ini` defines the actual production board environment and optional sensor build flags. The CMake configuration exists primarily to provide the AVR/Arduino project model and convenient CLion targets and therefore must be kept synchronized manually.

## 15. Bootstrap Completion Checklist

A fresh-machine setup is complete when all of the following are true:

- `pio --version` runs successfully.
- `avr-gcc --version` runs successfully.
- `pio run -e nanoatmega328` builds the production firmware.
- The correct Nano serial device is accessible without permission errors.
- Firmware upload succeeds to the production Nano.
- `pio device monitor -b 9600` can communicate with the running firmware.
- At least one Unity test suite can be built/run through PlatformIO.
- If using CLion, the Arduino toolchain and Nano CMake profile load without missing Arduino-core headers.

Once this checklist passes, continue with `TestingGuide.md` for test execution details or `DevelopmentGuide.md` for firmware architecture and contribution conventions.
