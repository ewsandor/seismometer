#include <cstring>

#include <hardware/watchdog.h>

#include "at24c_eeprom.hpp"
#include "seismometer_debug.hpp"
#include "seismometer_eeprom.hpp"
#include "seismometer_utils.hpp"

at24c_eeprom_c *eeprom;

static const seismometer_eeprom_data_s default_eeprom_data =
{
  .header = 
  {
    .identifier      = EEPROM_IDENTIFIER,
    .version         = SEISMOMETER_EEPROM_VERSION_1,
    .reset_requested = false,
  },
  .sample_log_config = 
  {
    .key_mask_stdio = 0x00,
    .key_mask_sd    = ((1<<SAMPLE_LOG_MAX_KEY)-1),
  },
};

static seismometer_eeprom_data_s eeprom_data = {0};
#define EEPROM_ADDRESS_HEADER            0x0000
#define EEPROM_ADDRESS_SAMPLE_LOG_CONFIG 0x0040

static bool eeprom_write_header(const seismometer_eeprom_header_s *header)
{
  SEISMOMETER_ASSERT(header != nullptr);
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Writing new header to EEPROM.\n");
  bool ret_val = (sizeof(seismometer_eeprom_header_s) == 
                  eeprom->write_data(EEPROM_ADDRESS_HEADER, (uint8_t*) header, sizeof(seismometer_eeprom_header_s)));
  return ret_val;
}
static bool eeprom_write_sample_log_config(const seismometer_eeprom_sample_log_config_s * config)
{
  SEISMOMETER_ASSERT(config != nullptr);
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Writing new sample log config to EEPROM.\n");
  bool ret_val = (sizeof(seismometer_eeprom_sample_log_config_s) == 
                  eeprom->write_data(EEPROM_ADDRESS_SAMPLE_LOG_CONFIG, (uint8_t*) config, sizeof(seismometer_eeprom_sample_log_config_s)));
  return ret_val;
}
static bool eeprom_write_data(const seismometer_eeprom_data_s *new_eeprom_data)
{
  SEISMOMETER_ASSERT(new_eeprom_data != nullptr);
  bool ret_val = true;
  
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Writing new EEPROM data.\n");

  do
  {
    if(!eeprom_write_header(&new_eeprom_data->header))
    {
      ret_val = false;
      break;
    }
    if(!eeprom_write_sample_log_config(&new_eeprom_data->sample_log_config))
    {
      ret_val = false;
      break;
    }
  } while(0);

  return ret_val;
}

void eeprom_init(seismometer_i2c_handle_s *i2c_handle)
{

  eeprom = new at24c_eeprom_c(i2c_handle, AT24C_EEPROM_ADDRESS_7, AT24C_EEPROM_SIZE_32K);
  #ifdef SEISMOMETER_DEBUG_BUILD
  eeprom->dump_eeprom();
  watchdog_update();
  #endif

  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Loading data from EEPROM.\n");
  SEISMOMETER_ASSERT_CALL(sizeof(eeprom_data.header) == 
    eeprom->read_data(EEPROM_ADDRESS_HEADER, (uint8_t*) &eeprom_data.header, sizeof(eeprom_data.header)));

  if((0 !=strncmp((char*)eeprom_data.header.identifier, EEPROM_IDENTIFIER, SEISMOMETER_EEPROM_IDENTIFIER_LENGTH)) ||
     (eeprom_data.header.version <= SEISMOMETER_EEPROM_VERSION_INVALID) ||
     (eeprom_data.header.version >= SEISMOMETER_EEPROM_VERSION_MAX)     ||
     (true == eeprom_data.header.reset_requested))
  {
    watchdog_update();
    if(true == eeprom_data.header.reset_requested)
    {
      SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "EEPROM reset requested.\n");
    }
    else
    {
      SEISMOMETER_PRINTF(SEISMOMETER_LOG_ERROR, "EEPROM data is invalid.\n");
    }
    eeprom->clear_eeprom(0xFF);
    watchdog_update();
    SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Programming default EEPROM.\n");

    SEISMOMETER_ASSERT_CALL(eeprom_write_data(&default_eeprom_data));
    seismometer_force_reboot();
  }

  SEISMOMETER_ASSERT_CALL(sizeof(eeprom_data.sample_log_config) == 
    eeprom->read_data(EEPROM_ADDRESS_SAMPLE_LOG_CONFIG, (uint8_t*) &eeprom_data.sample_log_config, sizeof(eeprom_data.sample_log_config)));
}

bool eeprom_request_reset()
{
  bool ret_val = true;
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Requesting EEPROM reset at reboot.\n");

  seismometer_eeprom_header_s new_header = eeprom_data.header;
  new_header.reset_requested=1;

  if(eeprom_write_header(&new_header))
  {
    seismometer_force_reboot();
  }
  else
  {
    SEISMOMETER_PRINTF(SEISMOMETER_LOG_ERROR, "Failed to place EEPROM reset request.\n");
    ret_val = false;
  }

  return ret_val;
}

const seismometer_eeprom_sample_log_config_s *eeprom_get_sample_log_config()
{
  return &eeprom_data.sample_log_config;
}