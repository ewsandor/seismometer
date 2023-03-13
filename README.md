# Sandor Laboratories Seismometer
This is a project to create a homemade seismometer

## Usage
### Data Collector
### Sample Format
  Samples are output with the C-format string `S|%02X|%08X|%016llX|%016llX` which corresponds to `S|<key>|<index>|<timestamp>|<data>`.  Samples may be easily filtered via `grep 'S|<key>'` and separated by the `|` deliminator.

  Currently the following log keys are defined:
```
  - INVALID                  =  0
  - Accelerometer X          =  1
  - Accelerometer Y          =  2
  - Accelerometer Z          =  3
  - Accelerometer M          =  4
  - Accelerometer X Filtered =  5
  - Accelerometer Y Filtered =  6
  - Accelerometer Z Filtered =  7
  - Accelerometer M Filtered =  8
  - Accelerometer TEMP       =  9
  - Pendulum 10X             = 10
  - Pendulum 100X            = 11
  - Pendulum Filtered        = 12
```
 
#### Commands
  - Force a soft-reboot: `REBOOT`
    - Reboot is triggered via a watchdog timer timeout so soft-reboot cannot be triggered if stalled or if the watchdog timer is disabled.
  - Reset EEPROM: `RESETEEPROM` 
    - Resets EEPROM to default values
    - Command input must be an exact match (e.g. no trailing space)
  - Set Sample Key Mask: `SAMPLEKEYMASKSD<key mask>`, `SAMPLEKEYMASKSTDOUT<key mask>`
    - Configures which sample channels are actively logged to the SD card and STDOUT respectively
    - Key masks must be passed as hexadecimal mask with each bit corresponding to a key to be logged (LSB is key '0')
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