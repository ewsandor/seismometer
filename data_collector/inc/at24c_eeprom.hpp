#ifndef __AT24C_EEPROM_HPP__
#define __AT24C_EEPROM_HPP__

#include "seismometer_i2c.hpp"

typedef enum
{
  AT24C_EEPROM_ADDRESS_0,
  AT24C_EEPROM_ADDRESS_1,
  AT24C_EEPROM_ADDRESS_2,
  AT24C_EEPROM_ADDRESS_3,
  AT24C_EEPROM_ADDRESS_4,
  AT24C_EEPROM_ADDRESS_5,
  AT24C_EEPROM_ADDRESS_6,
  AT24C_EEPROM_ADDRESS_7,
  AT24C_EEPROM_ADDRESS_MAX,
} at24c_eeprom_address_e;

typedef enum
{
  AT24C_EEPROM_SIZE_32K,
  AT24C_EEPROM_SIZE_64K,
  AT24C_EEPROM_SIZE_MAX,
} at24c_eeprom_size_e;

typedef unsigned int at24c_eeprom_data_size_t;
typedef unsigned int at24c_eeprom_data_address_t;

class at24c_eeprom_c
{
  private:
    seismometer_i2c_handle_s     *i2c_handle;
    const at24c_eeprom_address_e  address;
    const uint8_t                 i2c_addr;
    const at24c_eeprom_size_e     size;
  public:
    at24c_eeprom_c(seismometer_i2c_handle_s *, at24c_eeprom_address_e, at24c_eeprom_size_e);

    inline at24c_eeprom_size_e      get_size()       const { return size; };
    inline at24c_eeprom_data_size_t get_size_bytes() const { return ((AT24C_EEPROM_SIZE_64K==size)?8192:4096); };

    /* Reads 'bytes_to_read' bytes from the eeprom into 'buffer' starting at the given 'start_address'. 
        'buffer' must be large enough to hold 'bytes_to_read' bytes.
        Returns number of bytes read */
    at24c_eeprom_data_size_t read_data(at24c_eeprom_data_address_t start_address, uint8_t * buffer, at24c_eeprom_data_size_t bytes_to_read);
    /* Writes 'bytes_to_write' bytes to the eeprom from 'buffer' starting at the given 'start_address'. 
        'buffer' must hold at least 'bytes_to_read' bytes.
        Returns number of bytes written */
    at24c_eeprom_data_size_t write_data(at24c_eeprom_data_address_t start_address, const uint8_t * buffer, at24c_eeprom_data_size_t bytes_to_write);

    /* Wipes the eeprom and sets every byte to the given value */
    bool clear_eeprom(uint8_t value);

#ifdef SEISMOMETER_DEBUG_BUILD
    /* Dumps eeprom contents to stdout */
    void dump_eeprom();
#endif
};

#endif /* __AT24C_EEPROM_HPP__ */