#include <cstdlib>
#include <cstring>

#include "at24c_eeprom.hpp"
#include "seismometer_debug.hpp"
#include "seismometer_utils.hpp"

#define AT4C_EEPROM_BASE_ADDRESS 0x50

at24c_eeprom_c::at24c_eeprom_c(seismometer_i2c_handle_s * i2c_handle_init, at24c_eeprom_address_e address_init, at24c_eeprom_size_e size_init)
  : address(address_init), size(size_init), i2c_addr(AT4C_EEPROM_BASE_ADDRESS | (address_init & 0x7))
{
  SEISMOMETER_ASSERT((address >= 0) && (address < AT24C_EEPROM_ADDRESS_MAX));
  SEISMOMETER_ASSERT((size >= 0)    && (size    < AT24C_EEPROM_SIZE_MAX));
  SEISMOMETER_ASSERT(i2c_init != nullptr);
  i2c_handle = i2c_handle_init;
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
    seismometer_i2c_lock(i2c_handle);
    SEISMOMETER_ASSERT_CALL(2       == i2c_write_blocking(i2c_handle->i2c_inst, i2c_addr, write_buffer, 2, true));
    SEISMOMETER_ASSERT_CALL(ret_val == i2c_read_blocking (i2c_handle->i2c_inst, i2c_addr, buffer, ret_val, false));
    seismometer_i2c_unlock(i2c_handle);
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

    seismometer_i2c_lock(i2c_handle);
    while(btw > 0)
    {
      at24c_eeprom_data_address_t write_address = (start_address + bytes_written);
      write_buffer[0] = (write_address >>   8) & ((AT24C_EEPROM_SIZE_64K==size)?0x1F:0x0F);
      write_buffer[1] = (write_address & 0xFF);

      at24c_eeprom_data_size_t btw_now = ((btw > 32)?32:btw);
      memcpy(&write_buffer[2], &buffer[bytes_written], btw_now);
      at24c_eeprom_data_size_t bytes_written_now = i2c_write_blocking(i2c_handle->i2c_inst, i2c_addr, write_buffer, btw_now+2, false);
      if(bytes_written_now != btw_now+2)
      {
        SEISMOMETER_PRINTF(SEISMOMETER_LOG_ERROR, "Failed to write to AT24C EEPROM %u.\n", address);
        break;
      }
      bytes_written+=((bytes_written_now > 2)?(bytes_written_now-2):0);
      btw -= btw_now;
      /* Wait for eeprom to ack write*/
      while(PICO_ERROR_GENERIC == i2c_write_blocking(i2c_handle->i2c_inst, i2c_addr, write_buffer, 2, false)) {tight_loop_contents();}
    }
    seismometer_i2c_unlock(i2c_handle);
  }

  return bytes_written;
}

bool at24c_eeprom_c::clear_eeprom(uint8_t value)
{
  bool ret_val = true;
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Clearing AT24C EEPROM %u with value '0x%x'.\n", address, value);

  uint8_t *write_buffer = (uint8_t*)malloc(get_size_bytes());

  if(write_buffer != nullptr)
  {
    memset(write_buffer, value, get_size_bytes());
    SEISMOMETER_ASSERT_CALL(get_size_bytes() == write_data(0x0000, write_buffer, get_size_bytes()));
    free(write_buffer);
  }
  else
  {
    SEISMOMETER_PRINTF(SEISMOMETER_LOG_ERROR, "Failed to allocate memory (0x%04X bytes) for AT24C EEPROM write buffer.\n", get_size_bytes());
    ret_val = false;
  }

  return ret_val;
}

#ifdef SEISMOMETER_DEBUG_BUILD
void at24c_eeprom_c::dump_eeprom()
{
  uint8_t *read_buffer = (uint8_t*)malloc(get_size_bytes());

  SEISMOMETER_PRINTF(SEISMOMETER_LOG_DEBUG, "Dumping AT24C EEPROM %u.\n", address);

  if(read_buffer != nullptr)
  {
    SEISMOMETER_ASSERT_CALL(get_size_bytes() == read_data(0x0000, read_buffer, get_size_bytes()));

    at24c_eeprom_data_address_t itr = 0;

    while(itr < get_size_bytes())
    {
      uint32_t data_0_h = (read_buffer[itr]    << 24) | (read_buffer[itr+1]  << 16) | (read_buffer[itr+2]  << 8) | (read_buffer[itr+3] );
      uint32_t data_0_l = (read_buffer[itr+4]  << 24) | (read_buffer[itr+5]  << 16) | (read_buffer[itr+6]  << 8) | (read_buffer[itr+7] );
      uint32_t data_1_h = (read_buffer[itr+8]  << 24) | (read_buffer[itr+9]  << 16) | (read_buffer[itr+10] << 8) | (read_buffer[itr+11]);
      uint32_t data_1_l = (read_buffer[itr+12] << 24) | (read_buffer[itr+13] << 16) | (read_buffer[itr+14] << 8) | (read_buffer[itr+15]);


      SEISMOMETER_PRINTF(SEISMOMETER_LOG_DEBUG, "%04X: %04X %04X %04X %04X %04X %04X %04X %04X\n", itr, 
        (data_0_h >> 16), (data_0_h & 0xFFFF), (data_0_l >> 16), (data_0_l & 0xFFFF),
        (data_1_h >> 16), (data_1_h & 0xFFFF), (data_1_l >> 16), (data_1_l & 0xFFFF));
      itr += 16;
    }

    free(read_buffer);
  }
  else
  {
    SEISMOMETER_PRINTF(SEISMOMETER_LOG_ERROR, "Failed to allocate memory (0x%04X bytes) for AT24C EEPROM dump.\n", get_size_bytes());
  }
}
#endif