#include <cassert>
#include <cstdint>
#include <cstdio>

#include "mpu-6500.hpp"

#define MPU_6500_I2C_ADDRESS 0x68

typedef enum
{
  ACCELEROMETER_02G = 0b00,
  ACCELEROMETER_04G = 0b01,
  ACCELEROMETER_08G = 0b10,
  ACCELEROMETER_16G = 0b11,
} mpu_6500_acceleration_range_e;

#define BITS_PER_G(acceleration_range) ((1<<14) >> acceleration_range)

enum
{
  A_DLPF_CFG_460 = 0,
  A_DLPF_CFG_184 = 1,
  A_DLPF_CFG_092 = 2,
  A_DLPF_CFG_041 = 3,
  A_DLPF_CFG_020 = 4,
  A_DLPF_CFG_010 = 5,
  A_DLPF_CFG_005 = 6,
};

enum
{
  ACCEL_XOUT_H = 59,
  ACCEL_XOUT_L = 60,
  ACCEL_YOUT_H = 61,
  ACCEL_YOUT_L = 62,
  ACCEL_ZOUT_H = 63,
  ACCEL_ZOUT_L = 64,
  TEMP_OUT_H   = 65,
  TEMP_OUT_L   = 66,
};

typedef struct 
{
  i2c_inst_t                          *i2c_inst;
  const mpu_6500_acceleration_range_e  acceleration_range;

  mpu_6500_accelerometer_data_s        last_accelerometer_data;
  mpu_6500_temperature_t               last_temperature;
} mpu_6500_s;

mpu_6500_s mpu_6500_context = 
{
  .i2c_inst                = nullptr, 
  .acceleration_range      = ACCELEROMETER_02G,
  .last_accelerometer_data = {0},
  .last_temperature        = 0
};

void mpu_6500_init(i2c_inst_t *i2c_inst)
{
  printf("Initializing MPU-6500.\n");

  //Write buffer index 0 is register address
  //Write buffer index 1 is write data
  uint8_t write_buffer[2];

  mpu_6500_context.i2c_inst = i2c_inst;

  //Register 28 – Accelerometer Configuration
  write_buffer[0] = 28;
  write_buffer[1] = (ACCELEROMETER_02G<<3);
  assert(2 == i2c_write_blocking(mpu_6500_context.i2c_inst, MPU_6500_I2C_ADDRESS, write_buffer, 2, false));
  //Register 29 – Accelerometer Configuration 2
  write_buffer[0] = 29;
  write_buffer[1] = (A_DLPF_CFG_460<<0);
  assert(2 == i2c_write_blocking(mpu_6500_context.i2c_inst, MPU_6500_I2C_ADDRESS, write_buffer, 2, false));
  //Register 35 – FIFO Enable
  write_buffer[0] = 35;
  write_buffer[1] = (1<<7) /*TEMP_OUT*/ | (1<<3) /*ACCEL*/;
  //assert(2 == i2c_write_blocking(mpu_6500_context.i2c_inst, MPU_6500_I2C_ADDRESS, write_buffer, 2, false));
  //Register 56 – Interrupt Enable
  write_buffer[0] = 56;
  write_buffer[1] = (1<<0) /*RAW_RDY_EN*/;
  //assert(2 == i2c_write_blocking(mpu_6500_context.i2c_inst, MPU_6500_I2C_ADDRESS, write_buffer, 2, false));
}

void mpu_6500_loop()
{
  uint8_t read_buffer[8];
  uint8_t register_address;

  register_address = ACCEL_XOUT_H;
  assert(1 == i2c_write_blocking(mpu_6500_context.i2c_inst, MPU_6500_I2C_ADDRESS, &register_address, 1, true));
  assert(8 == i2c_read_blocking (mpu_6500_context.i2c_inst, MPU_6500_I2C_ADDRESS, read_buffer,       8, false));
  mpu_6500_context.last_accelerometer_data.x = (read_buffer[0] << 8) | read_buffer[1];
  mpu_6500_context.last_accelerometer_data.y = (read_buffer[2] << 8) | read_buffer[3];
  mpu_6500_context.last_accelerometer_data.z = (read_buffer[4] << 8) | read_buffer[5];
  mpu_6500_context.last_temperature          = (read_buffer[6] << 8) | read_buffer[7];
}

void mpu_6500_accelerometer_data(mpu_6500_accelerometer_data_s *accelerometer_data)
{
  assert(accelerometer_data != nullptr);
  *accelerometer_data = mpu_6500_context.last_accelerometer_data;
}

mpu_6500_temperature_t mpu_6500_temperature()
{
  return mpu_6500_context.last_temperature;
}

//((TEMP_OUT – RoomTemp_Offset)/Temp_Sensitivity) + 21degC
m_celsius_t mpu_6500_temperature_to_m_celsius(mpu_6500_temperature_t temp_out)
{
  const int64_t roomtemp_offset = 0;
  const int64_t temp_sensitivity = 333870; //333.87 LSB/°C

  return (((((int64_t)temp_out-roomtemp_offset)*1000*1000)/temp_sensitivity)+21000);
}

mm_ps2_t mpu_6500_acceleration_to_mm_ps2(mpu_6500_acceleration_t raw_acceleration)
{
  return ((mm_ps2_t)raw_acceleration*9800)/BITS_PER_G(mpu_6500_context.acceleration_range);
}