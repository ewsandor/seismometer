#include <cassert>
#include <cstdint>
#include <cstdio>

#include "mpu-6500.hpp"

#define MPU_6500_I2C_ADDRESS 0x68

enum
{
  ACCELEROMETER_02G = 0b00,
  ACCELEROMETER_04G = 0b01,
  ACCELEROMETER_08G = 0b10,
  ACCELEROMETER_16G = 0b11,
};

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

i2c_inst_t *mpu_6500_i2c_inst = nullptr;

void mpu_6500_init(i2c_inst_t *i2c_inst)
{
  printf("Initializing MPU-6500.\n");

  //Write buffer index 0 is register address
  //Write buffer index 1 is write data
  uint8_t write_buffer[2];

  mpu_6500_i2c_inst = i2c_inst;

  //Register 28 – Accelerometer Configuration
  write_buffer[0] = 28;
  write_buffer[1] = (ACCELEROMETER_02G<<3);
  assert(2 == i2c_write_blocking(mpu_6500_i2c_inst, MPU_6500_I2C_ADDRESS, write_buffer, 2, false));
  //Register 29 – Accelerometer Configuration 2
  write_buffer[0] = 29;
  write_buffer[1] = (A_DLPF_CFG_460<<0);
  assert(2 == i2c_write_blocking(mpu_6500_i2c_inst, MPU_6500_I2C_ADDRESS, write_buffer, 2, false));
  #if 0
  //Register 35 – FIFO Enable
  write_buffer[0] = 35;
  write_buffer[1] = (1<<7) /*TEMP_OUT*/ | (1<<3) /*ACCEL*/;
  assert(2 == i2c_write_blocking(mpu_6500_i2c_inst, MPU_6500_I2C_ADDRESS, write_buffer, 2, false));
  #endif
}

mpu_6500_accelerometer_data_s last_accelerometer_data = {0};

mpu_6500_temperature_t last_temperature;

void mpu_6500_loop()
{
  uint8_t read_buffer;
  uint8_t register_address;

  last_accelerometer_data = {0};

  register_address = ACCEL_XOUT_H;
  assert(1 == i2c_write_blocking(mpu_6500_i2c_inst, MPU_6500_I2C_ADDRESS, &register_address, 1, true));
  assert(1 == i2c_read_blocking (mpu_6500_i2c_inst, MPU_6500_I2C_ADDRESS, &read_buffer,      1, false));
  last_accelerometer_data.x |= (read_buffer << 8);
  register_address = ACCEL_XOUT_L;
  assert(1 == i2c_write_blocking(mpu_6500_i2c_inst, MPU_6500_I2C_ADDRESS, &register_address, 1, true));
  assert(1 == i2c_read_blocking (mpu_6500_i2c_inst, MPU_6500_I2C_ADDRESS, &read_buffer,      1, false));
  last_accelerometer_data.x |= read_buffer;

  register_address = ACCEL_YOUT_H;
  assert(1 == i2c_write_blocking(mpu_6500_i2c_inst, MPU_6500_I2C_ADDRESS, &register_address, 1, true));
  assert(1 == i2c_read_blocking (mpu_6500_i2c_inst, MPU_6500_I2C_ADDRESS, &read_buffer,      1, false));
  last_accelerometer_data.y |= (read_buffer << 8);
  register_address = ACCEL_YOUT_L;
  assert(1 == i2c_write_blocking(mpu_6500_i2c_inst, MPU_6500_I2C_ADDRESS, &register_address, 1, true));
  assert(1 == i2c_read_blocking (mpu_6500_i2c_inst, MPU_6500_I2C_ADDRESS, &read_buffer,      1, false));
  last_accelerometer_data.y |= read_buffer;


  register_address = ACCEL_ZOUT_H;
  assert(1 == i2c_write_blocking(mpu_6500_i2c_inst, MPU_6500_I2C_ADDRESS, &register_address, 1, true));
  assert(1 == i2c_read_blocking (mpu_6500_i2c_inst, MPU_6500_I2C_ADDRESS, &read_buffer,      1, false));
  last_accelerometer_data.z |= (read_buffer << 8);
  register_address = ACCEL_ZOUT_L;
  assert(1 == i2c_write_blocking(mpu_6500_i2c_inst, MPU_6500_I2C_ADDRESS, &register_address, 1, true));
  assert(1 == i2c_read_blocking (mpu_6500_i2c_inst, MPU_6500_I2C_ADDRESS, &read_buffer,      1, false));
  last_accelerometer_data.z |= read_buffer;

  last_temperature = 0;
  register_address = TEMP_OUT_H;
  assert(1 == i2c_write_blocking(mpu_6500_i2c_inst, MPU_6500_I2C_ADDRESS, &register_address, 1, true));
  assert(1 == i2c_read_blocking (mpu_6500_i2c_inst, MPU_6500_I2C_ADDRESS, &read_buffer,      1, false));
  last_temperature |= (read_buffer << 8);
  register_address = TEMP_OUT_L;
  assert(1 == i2c_write_blocking(mpu_6500_i2c_inst, MPU_6500_I2C_ADDRESS, &register_address, 1, true));
  assert(1 == i2c_read_blocking (mpu_6500_i2c_inst, MPU_6500_I2C_ADDRESS, &read_buffer,      1, false));
  last_temperature |= read_buffer;
}

void mpu_6500_accelerometer_data(mpu_6500_accelerometer_data_s *accelerometer_data)
{
  assert(accelerometer_data != nullptr);
  *accelerometer_data = last_accelerometer_data;
}

mpu_6500_temperature_t mpu_6500_temperature()
{
  return last_temperature;
}