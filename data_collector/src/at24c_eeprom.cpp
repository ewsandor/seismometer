#include <cstring>

#include "at24c_eeprom.hpp"
#include "seismometer_debug.hpp"
#include "seismometer_utils.hpp"

#define AT4C_EEPROM_BASE_ADDRESS 0xA0

at24c_eeprom_c::at24c_eeprom_c(i2c_inst_t * i2c_init, at24c_eeprom_address_e address_init, at24c_eeprom_size_e size_init)
  : address(address_init), size(size_init), i2c_addr(AT4C_EEPROM_BASE_ADDRESS | (address_init & 0x7))
{
  SEISMOMETER_ASSERT((address >= 0) && (address < AT24C_EEPROM_ADDRESS_MAX));
  SEISMOMETER_ASSERT((size >= 0)    && (size    < AT24C_EEPROM_SIZE_MAX));
  SEISMOMETER_ASSERT(i2c_init != nullptr);
  i2c_inst = i2c_init;
}


at24c_eeprom_data_size_t at24c_eeprom_c::read_data(at24c_eeprom_data_address_t start_address, uint8_t * buffer, at24c_eeprom_data_size_t bytes_to_read)
{
  at24c_eeprom_data_size_t ret_val = 0;
  SEISMOMETER_ASSERT(buffer != nullptr);

  if((bytes_to_read > 0) && (start_address < get_size_bytes()))
  {
    ret_val = SEISMOMETER_MIN(bytes_to_read, (get_size_bytes()-start_address));
    uint8_t write_buffer[2]; /* 2 byte address */
    write_buffer[0] = (start_address>>8) & ((AT24C_EEPROM_SIZE_64K==size)?0x1F:0x0F);
    write_buffer[1] = (start_address&0xFF);
    SEISMOMETER_ASSERT_CALL(2       == i2c_write_blocking(i2c_inst, i2c_addr, write_buffer, 2, true));
    SEISMOMETER_ASSERT_CALL(ret_val == i2c_read_blocking (i2c_inst, i2c_addr, buffer, ret_val, true));
  }

  return ret_val;
}
at24c_eeprom_data_size_t at24c_eeprom_c::write_data(at24c_eeprom_data_address_t start_address, const uint8_t * buffer, at24c_eeprom_data_size_t bytes_to_write)
{
  at24c_eeprom_data_size_t bytes_written = 0;
  SEISMOMETER_ASSERT(buffer != nullptr);

  if((bytes_to_write > 0) && (start_address < get_size_bytes()))
  {
    at24c_eeprom_data_size_t btw = SEISMOMETER_MIN(bytes_to_write, (get_size_bytes()-start_address));
    uint8_t write_buffer[34]; /* 32-byte page + 2 byte address */
    write_buffer[0] = (start_address>>8) & ((AT24C_EEPROM_SIZE_64K==size)?0x1F:0x0F);
    write_buffer[1] = (start_address&0xFF);

    while(btw > 0)
    {
      at24c_eeprom_data_size_t btw_now = ((btw > 32)?32:btw);
      memcpy(&write_buffer[1], &buffer[bytes_written], btw_now);
      SEISMOMETER_ASSERT_CALL((btw_now+2) == i2c_write_blocking(i2c_inst, i2c_addr, write_buffer, btw_now+2, false));
      bytes_written+=btw_now;
      btw -= btw_now;
    }
  }

  return bytes_written;
}