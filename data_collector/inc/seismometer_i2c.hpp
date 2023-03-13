#ifndef __SEISMOMETER_I2C_HPP__
#define __SEISMOMETER_I2C_HPP__

#include <hardware/i2c.h>
#include <pico/mutex.h>

typedef struct 
{
  mutex_t mutex;
  i2c_inst_t *i2c_inst;
} seismometer_i2c_handle_s;

bool seismometer_i2c_lock  (seismometer_i2c_handle_s* i2c_handle);
bool seismometer_i2c_unlock(seismometer_i2c_handle_s* i2c_handle);
#endif /* __SEISMOMETER_I2C_HPP__ */