# Sandor Laboratories Seismometer
This is a project to create a homemade seismometer

## Usage
### Data Collector
### Sample Format
  Samples are output with the C-format string `"S|%02X|%08X|%016llX|%016llX"` which corresponds to `S|<key>|<index>|<timestamp>|<data>`.

  Currently the following log keys are defined:
```
  - SAMPLE_LOG_INVALID           =  0
  - SAMPLE_LOG_ACCEL_X           =  1
  - SAMPLE_LOG_ACCEL_Y           =  2
  - SAMPLE_LOG_ACCEL_Z           =  3
  - SAMPLE_LOG_ACCEL_M           =  4
  - SAMPLE_LOG_ACCEL_X_FILTERED  =  5
  - SAMPLE_LOG_ACCEL_Y_FILTERED  =  6
  - SAMPLE_LOG_ACCEL_Z_FILTERED  =  7
  - SAMPLE_LOG_ACCEL_M_FILTERED  =  8
  - SAMPLE_LOG_ACCEL_TEMP        =  9
  - SAMPLE_LOG_PENDULUM_10X      = 10
  - SAMPLE_LOG_PENDULUM_100X     = 11
  - SAMPLE_LOG_PENDULUM_FILTERED = 12
```
 
#### Commands
  - Reset EEPROM: `RESETEEPROM` 
    - Resets EEPROM to default values
    - Command input must be an exact match (e.g. no trailing space)
  - Set RTC: `T<unix epoch in seconds>` 
    - Example setting RTC via Bash and UART: `echo T$(date +%s) > /dev/ttyACM0`

## Dependencies
### Data Collector 
- SD Card Library: carlk3's no-OS-FatFS-SD-SPI-RPi-Pico 
  - <https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico>
  - Tested with version 1.0.8
  - Library source directory at `data_collector/lib/no-OS-FatFS-SD-SPI-RPi-Pico`
- Optional
  - Compression Library: zlib 
    - <https://zlib.net/>
    - Enable with CMAKE flag `ENABLE_ZLIB_DATA_FILE_COMPRESSION`
    - Tested with version 1.2.13
    - Library source directory at `data_collector/lib/zlib`

### PCB 
- Digikey KiCad LIbrary
  - <https://github.com/Digi-Key/digikey-kicad-library>
- ncarandini's KiCad RP Pico Library
  - <https://github.com/ncarandini/KiCad-RP-Pico>
- MCU-6500 Model
  - <https://www.snapeda.com/parts/MPU-6500/TDK%20InvenSense/view-part/>