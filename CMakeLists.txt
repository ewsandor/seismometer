cmake_minimum_required(VERSION 3.25)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(seismometer)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable( seismometer
                seismometer.cpp
                mpu-6500.cpp
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(seismometer pico_stdlib hardware_i2c)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(seismometer)

# STDIO control
pico_enable_stdio_usb (seismometer 1)
pico_enable_stdio_uart(seismometer 0)