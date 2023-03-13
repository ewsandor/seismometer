#include <cassert>
#include <cstdint>
#include <cstdio>

#include "mpu-6500.hpp"
#include "seismometer_debug.hpp"

#define MPU_6500_I2C_ADDRESS 0x69

typedef enum
{
  ACCELEROMETER_02G = 0b00,
  ACCELEROMETER_04G = 0b01,
  ACCELEROMETER_08G = 0b10,
  ACCELEROMETER_16G = 0b11,
} mpu_6500_acceleration_range_e;

#define ACCELEROMETER_RAW_1G(acceleration_range) ((1<<14) >> acceleration_range)

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
  seismometer_i2c_handle_s            *i2c_handle;
  const mpu_6500_acceleration_range_e  acceleration_range;
  mpu_6500_accelerometer_data_s        accelerometer_offsets;

  mpu_6500_accelerometer_data_s        last_accelerometer_data;
  mpu_6500_temperature_t               last_temperature;
} mpu_6500_s;

static mpu_6500_s mpu_6500_context = 
{
  .i2c_handle                = nullptr, 
  .acceleration_range      = ACCELEROMETER_02G,
  .accelerometer_offsets   = {0},
  .last_accelerometer_data = {0},
  .last_temperature        = 0
};

void mpu_6500_init(seismometer_i2c_handle_s * i2c_inst)
{
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Initializing MPU-6500.\n");

  //Write buffer index 0 is register address
  //Write buffer index 1 is write data
  uint8_t write_buffer[2];

  mpu_6500_context.i2c_handle = i2c_inst;

  seismometer_i2c_lock(mpu_6500_context.i2c_handle);
  //Register 107 – Power Management 1
  //Reset device to default settings
  write_buffer[0] = 107;
  write_buffer[1] = (1<<7) /*DEVICE_RESET*/;
  SEISMOMETER_ASSERT_CALL(2 == i2c_write_blocking(mpu_6500_context.i2c_handle->i2c_inst, MPU_6500_I2C_ADDRESS, write_buffer, 2, false));
  sleep_ms(10);
  //Register 108 – Power Management 2
  write_buffer[0] = 108;
  write_buffer[1] = (0x7) /*DISABLE_X/Y/ZG*/;
  SEISMOMETER_ASSERT_CALL(2 == i2c_write_blocking(mpu_6500_context.i2c_handle->i2c_inst, MPU_6500_I2C_ADDRESS, write_buffer, 2, false));
  //Register 25 – Sample Rate Divider
  write_buffer[0] = 25;
  write_buffer[1] = 3; //SAMPLE_RATE = INTERNAL_SAMPLE_RATE / (1 + SMPLRT_DIV) where INTERNAL_SAMPLE_RATE = 1kHz
  SEISMOMETER_ASSERT_CALL(2 == i2c_write_blocking(mpu_6500_context.i2c_handle->i2c_inst, MPU_6500_I2C_ADDRESS, write_buffer, 2, false));
  //Register 28 – Accelerometer Configuration
  write_buffer[0] = 28;
  write_buffer[1] = (ACCELEROMETER_02G<<3);
  SEISMOMETER_ASSERT_CALL(2 == i2c_write_blocking(mpu_6500_context.i2c_handle->i2c_inst, MPU_6500_I2C_ADDRESS, write_buffer, 2, false));
  //Register 29 – Accelerometer Configuration 2
  write_buffer[0] = 29;
  write_buffer[1] = (A_DLPF_CFG_092<<0);
  SEISMOMETER_ASSERT_CALL(2 == i2c_write_blocking(mpu_6500_context.i2c_handle->i2c_inst, MPU_6500_I2C_ADDRESS, write_buffer, 2, false));
  //Register 35 – FIFO Enable
  write_buffer[0] = 35;
  write_buffer[1] = (1<<7) /*TEMP_OUT*/ | (1<<3) /*ACCEL*/;
// SEISMOMETER_ASSERT_CALL(2 == i2c_write_blocking(mpu_6500_context.i2c_handle->i2c_inst, MPU_6500_I2C_ADDRESS, write_buffer, 2, false));
  //Register 55 – INT Pin / Bypass Enable Configuration
  write_buffer[0] = 55;
  write_buffer[1] = (1<<4) /*INT_ANYRD_2CLEAR*/;
//  SEISMOMETER_ASSERT_CALL(2 == i2c_write_blocking(mpu_6500_context.i2c_handle->i2c_inst, MPU_6500_I2C_ADDRESS, write_buffer, 2, false));
  //Register 56 – Interrupt Enable
  write_buffer[0] = 56;
  write_buffer[1] = (1<<0) /*RAW_RDY_EN*/;
//  SEISMOMETER_ASSERT_CALL(2 == i2c_write_blocking(mpu_6500_context.i2c_handle->i2c_inst, MPU_6500_I2C_ADDRESS, write_buffer, 2, false));

  seismometer_i2c_unlock(mpu_6500_context.i2c_handle);
}

void mpu_6500_calibrate()
{
  mpu_6500_read();

  mpu_6500_context.accelerometer_offsets.x = -mpu_6500_context.last_accelerometer_data.x;
  mpu_6500_context.accelerometer_offsets.y = -mpu_6500_context.last_accelerometer_data.y;
  mpu_6500_context.accelerometer_offsets.z = ACCELEROMETER_RAW_1G(mpu_6500_context.acceleration_range)-mpu_6500_context.last_accelerometer_data.z;
}

void __time_critical_func(mpu_6500_read())
{
  uint8_t read_buffer[8];
  uint8_t register_address;

  register_address = ACCEL_XOUT_H;
  seismometer_i2c_lock(mpu_6500_context.i2c_handle);
  SEISMOMETER_ASSERT_CALL(1 == i2c_write_blocking(mpu_6500_context.i2c_handle->i2c_inst, MPU_6500_I2C_ADDRESS, &register_address, 1, true));
  SEISMOMETER_ASSERT_CALL(8 == i2c_read_blocking (mpu_6500_context.i2c_handle->i2c_inst, MPU_6500_I2C_ADDRESS, read_buffer,       8, false));
  seismometer_i2c_unlock(mpu_6500_context.i2c_handle);
  mpu_6500_context.last_accelerometer_data.x = (read_buffer[0] << 8) | read_buffer[1];
  mpu_6500_context.last_accelerometer_data.y = (read_buffer[2] << 8) | read_buffer[3];
  mpu_6500_context.last_accelerometer_data.z = (read_buffer[4] << 8) | read_buffer[5];
  mpu_6500_context.last_temperature          = (read_buffer[6] << 8) | read_buffer[7];
}

void mpu_6500_accelerometer_data_raw(mpu_6500_accelerometer_data_s *accelerometer_data)
{
  SEISMOMETER_ASSERT(accelerometer_data != nullptr);
  *accelerometer_data = mpu_6500_context.last_accelerometer_data;
}

void mpu_6500_accelerometer_data(mpu_6500_accelerometer_data_s *accelerometer_data)
{
  SEISMOMETER_ASSERT(accelerometer_data != nullptr);
  mpu_6500_accelerometer_data_raw(accelerometer_data);
  accelerometer_data->x += mpu_6500_context.accelerometer_offsets.x;
  accelerometer_data->y += mpu_6500_context.accelerometer_offsets.y;
  accelerometer_data->z += mpu_6500_context.accelerometer_offsets.z;
}

mpu_6500_temperature_t mpu_6500_temperature()
{
  return mpu_6500_context.last_temperature;
}

//((TEMP_OUT – RoomTemp_Offset)/Temp_Sensitivity) + 21degC
m_celsius_t mpu_6500_temperature_to_m_celsius(mpu_6500_temperature_t temp_out)
{
  const int64_t room_temp_offset = 0;
  const int64_t temp_sensitivity = 333870; //333.87 LSB/°C

  return (((((int64_t)temp_out-room_temp_offset)*1000*1000)/temp_sensitivity)+21000);
}

mm_ps2_t mpu_6500_acceleration_to_mm_ps2(mpu_6500_acceleration_t raw_acceleration)
{
  return ((mm_ps2_t)raw_acceleration*9800)/ACCELEROMETER_RAW_1G(mpu_6500_context.acceleration_range);
}