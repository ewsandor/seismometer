cmake_minimum_required(VERSION 3.25)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(seismometer)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

option(ENABLE_ZLIB_DATA_FILE_COMPRESSION "Option to enable ZLIB datafile compression." OFF)

# Main executible
add_executable( 
                seismometer
                src/adc_manager.cpp
                src/at24c_eeprom.cpp
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

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DSEISMOMETER_DEBUG_BUILD")

# Include directory
target_include_directories(seismometer PRIVATE inc)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(seismometer pico_multicore pico_stdlib hardware_adc hardware_rtc hardware_i2c)

#Add libraries
#FatFs SD SPI 
add_compile_definitions(FatFs_SPI PRIVATE NO_PICO_LED)
add_subdirectory(lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI EXCLUDE_FROM_ALL)
target_link_libraries(seismometer FatFs_SPI)
if(ENABLE_ZLIB_DATA_FILE_COMPRESSION)
add_compile_definitions(seismometer PRIVATE ENABLE_ZLIB_DATA_FILE_COMPRESSION)
#zlib
add_subdirectory(lib/zlib EXCLUDE_FROM_ALL)
target_include_directories(seismometer PRIVATE lib/zlib)
target_include_directories(seismometer PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/lib/zlib)
target_link_libraries(seismometer zlibstatic)
endif()

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(seismometer)

# STDIO control
pico_enable_stdio_usb (seismometer 0)
pico_enable_stdio_uart(seismometer 1)