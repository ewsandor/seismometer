# Sandor Laboratories Seismometer
This is a project to create a homemade seismometer

## Data Collector Dependencies
- SD Card Library: no-OS-FatFS-SD-SPI-RPi-Pico 
  - https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico
  - Tested with version 1.0.8
  - Library source directory at `data_collector/lib/no-OS-FatFS-SD-SPI-RPi-Pico`
- Optional
  - Compression Library: zlib 
    - https://zlib.net/
    - Enable with CMAKE flag `ENABLE_ZLIB_DATA_FILE_COMPRESSION`
    - Tested with version 1.2.13
    - Library source directory at `data_collector/lib/zlib`