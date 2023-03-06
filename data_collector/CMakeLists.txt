cmake_minimum_required(VERSION 3.25)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(seismometer)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Main executible
add_executable( 
                seismometer
                src/adc_manager.cpp
                src/filter_coefficients.cpp
                src/fir_filter.cpp
                src/mpu-6500.cpp
                src/rtc_ds3231.cpp
                src/sample_handler.cpp
                src/sampler.cpp
                src/sd_card_spi.cpp
                src/seismometer.cpp
              )

# Metadata
pico_set_program_name(seismometer "Sandor Laboratories Seismometer")
pico_set_program_description(seismometer "Embedded software for my homemade seismometer.")
pico_set_program_version(seismometer "0.0.1-dev")
pico_set_program_url(seismometer "https://git.sandorlaboratories.com/edward/seismometer/")

# Include directory
include_directories(seismometer inc)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(seismometer pico_multicore pico_stdlib hardware_adc hardware_i2c)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(seismometer)

# STDIO control
pico_enable_stdio_usb (seismometer 0)
pico_enable_stdio_uart(seismometer 1)