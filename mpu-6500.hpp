#ifndef __MPU_6500_HPP__
#define __MPU_6500_HPP__

#include "hardware/i2c.h"

typedef struct 
{
  int16_t x;
  int16_t y;
  int16_t z;
} mpu_6500_accelerometer_data_s;

typedef uint16_t mpu_6500_temperature_t;


void mpu_6500_init(i2c_inst_t *i2c);

void mpu_6500_loop();

void mpu_6500_accelerometer_data(mpu_6500_accelerometer_data_s *accelerometer_data);

mpu_6500_temperature_t mpu_6500_temperature();

#endif //__MPU_6500_HPP__