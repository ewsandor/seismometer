# Sandor Laboratories Seismometer
This is a project to create a homemade seismometer

## Usage
### Data Collector
#### Commands
  - Reset EEPROm: `RESETEEPROM` 
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