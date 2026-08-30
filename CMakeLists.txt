cmake_minimum_required(VERSION 3.15)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
project(AquariumAto C CXX ASM)

###################################
#  Machine-specific stuff
set(AVR_DUDE_EXECUTABLE /usr/bin/avrdude)
set(AVR_DUDE_PORT /dev/ttyUSB0)

# PlatformIO's own managed package cache -- this is where Arduino.h, the
# rest of the Arduino AVR core, board variant pin maps, and PlatformIO's
# bundled libraries (EEPROM, etc.) actually live. None of this is part of
# this repo; CMake needs to be told where to find it directly.
#
# Default install location for PlatformIO Core. If yours differs, this is
# the only line that should need changing.
set(PIO_HOME $ENV{HOME}/.platformio)
set(PIO_FRAMEWORK ${PIO_HOME}/packages/framework-arduino-avr)

# Board variant pin map folder name -- "eightanaloginputs" is correct for
# a real Arduino Nano (8 analog inputs, A0-A7), matching the
# `board = nanoatmega328` PlatformIO target this project actually ships
# for. If Arduino.h/pins_arduino.h still can't be found after this file is
# in place, run:
#     ls ${PIO_FRAMEWORK}/variants/
# and update this to whatever folder name is actually present.
set(PIO_BOARD_VARIANT eightanaloginputs)
#  End of machine-specific stuff
###################################

set(CMAKE_CXX_STANDARD 11)
include(CMakeVerifyCompiler.cmake)

set(MCU atmega328p)

include_directories(lib/arduino)
include_directories(lib/buffers)
include_directories(lib/buttons)
include_directories(lib/duration)
include_directories(lib/runnable)
include_directories(lib/sensors)
include_directories(lib/switch)
include_directories(lib/timers)
include_directories(include)

# PlatformIO Arduino AVR framework (Arduino.h, HardwareSerial.h, etc.),
# this board's pin map (pins_arduino.h), and the bundled EEPROM library
# (used by include/ato/AtoConfig.h).
include_directories(${PIO_FRAMEWORK}/cores/arduino)
include_directories(${PIO_FRAMEWORK}/variants/${PIO_BOARD_VARIANT})
include_directories(${PIO_FRAMEWORK}/libraries/EEPROM/src)

file(GLOB_RECURSE SOURCES "src/*.*"  "lib/*.*"  "include/*.*")

# The Arduino core's own .c/.cpp/.S implementation files (not just headers)
# -- these provide the actual bodies of millis(), digitalRead(), Serial,
# Print::println(), operator new/delete (new.cpp), C++ ABI support stubs
# like __cxa_pure_virtual (abi.cpp), and crucially the real C main() that
# calls setup()/loop() (main.cpp -- the framework's own, distinct from
# src/main.cpp, which only defines setup()/loop() Arduino-sketch-style).
# A flat (non-recursive) glob, matching how PlatformIO's own build compiles
# every file in this directory regardless of whether this particular
# sketch uses all of it -- unused symbols get stripped at link time by
# --gc-sections below.
file(GLOB PIO_FRAMEWORK_SOURCES
        "${PIO_FRAMEWORK}/cores/arduino/*.c"
        "${PIO_FRAMEWORK}/cores/arduino/*.cpp"
        "${PIO_FRAMEWORK}/cores/arduino/*.S"
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-threadsafe-statics -fpermissive" )
add_compile_options(-c  -Os -Wall -ffunction-sections -fdata-sections -flto -mmcu=${MCU})

# ARDUINO_AVR_NANO (not the previous ARDUINO_AVR_UNO): this hardware is a
# real Nano per the schematic and README, and PlatformIO's own build
# (platformio.ini: board = nanoatmega328) targets Nano specifically.
add_compile_definitions(ARDUINO_AVR_NANO;F_CPU=16000000L;ARDUINO_ARCH_AVR;ARDUINO=10808)
add_link_options(-Os -flto -fuse-linker-plugin -mmcu=${MCU} -Wl,--gc-sections,--print-memory-usage,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map -lm)

add_executable(AquariumAto ${SOURCES} ${PIO_FRAMEWORK_SOURCES})

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}> ${HEX_FILE}
        COMMENT "Building ${HEX_FILE}")

add_custom_target(Upload
        ${AVR_DUDE_EXECUTABLE} -p ${MCU} -c arduino -b57600 -P ${AVR_DUDE_PORT} -v -v -F -D -U flash:w:${PROJECT_BINARY_DIR}/${PROJECT_NAME}:e
        DEPENDS ${PROJECT_NAME}
        COMMENT "Flash to ${TARGET_CPU}")

###################################
#  Unity test suites (test/*.*)
#
#  These are PlatformIO Unity tests, run via `pio test -e <env>`, NOT built
#  as part of the AquariumAto executable target above: each test file
#  defines its own setup()/loop(), which would conflict with src/main.cpp's
#  if compiled into the same binary, and they depend on Arduino-core/Unity
#  headers not all of which are wired into this CMake project's include
#  paths (Unity itself, specifically, lives under PlatformIO's per-project
#  .pio/libdeps/<env>/Unity, not the shared package cache above).
#
#  This custom target exists purely so CLion's project tree shows and
#  navigates these files -- it is a no-op target, nothing here is actually
#  compiled or linked by it.
###################################
file(GLOB_RECURSE TEST_SOURCES "test/*.*")
add_custom_target(UnityTests SOURCES ${TEST_SOURCES})
