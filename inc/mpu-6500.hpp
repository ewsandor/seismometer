#ifndef __MPU_6500_HPP__
#define __MPU_6500_HPP__

#include <hardware/i2c.h>

#include "seismometer_types.hpp"

typedef int16_t mpu_6500_acceleration_t;
typedef struct 
{
  mpu_6500_acceleration_t x;
  mpu_6500_acceleration_t y;
  mpu_6500_acceleration_t z;
} mpu_6500_accelerometer_data_s;

typedef uint16_t mpu_6500_temperature_t;

void mpu_6500_init(i2c_inst_t *i2c);
void mpu_6500_loop();
void mpu_6500_accelerometer_data(mpu_6500_accelerometer_data_s *accelerometer_data);
mpu_6500_temperature_t mpu_6500_temperature();

m_celsius_t mpu_6500_temperature_to_m_celsius(mpu_6500_temperature_t);
mm_ps2_t    mpu_6500_acceleration_to_mm_ps2  (mpu_6500_acceleration_t);

#endif //__MPU_6500_HPP__