#ifndef __SEISMOMETER_EEPROM_HPP__
#define __SEISMOMETER_EEPROM_HPP__

#include "seismometer_types.hpp"

typedef struct  __attribute__((packed))
{
  sample_log_key_mask_t key_mask_stdio;
  sample_log_key_mask_t key_mask_sd;
} seismometer_eeprom_sample_log_config_s;

enum
{
  SEISMOMETER_EEPROM_VERSION_INVALID,
  SEISMOMETER_EEPROM_VERSION_1,
  SEISMOMETER_EEPROM_VERSION_MAX,
};

#define SEISMOMETER_EEPROM_IDENTIFIER_LENGTH 15
#define EEPROM_IDENTIFIER "SL-SEISMOMETER"
typedef struct  __attribute__((packed))
{
  uint8_t identifier[SEISMOMETER_EEPROM_IDENTIFIER_LENGTH];
  uint8_t version;
  bool    reset_requested:1;
  uint8_t pad:7;
} seismometer_eeprom_header_s;

typedef struct __attribute__((packed))
{
  seismometer_eeprom_header_s            header;
  seismometer_eeprom_sample_log_config_s sample_log_config;
} seismometer_eeprom_data_s;

void eeprom_init();
bool eeprom_request_reset();

const seismometer_eeprom_sample_log_config_s *eeprom_get_sample_log_config();

#endif /* __SEISMOMETER_EEPROM_HPP__ */