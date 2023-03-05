#include <cassert>
#include <cstdio>

#include <hardware/i2c.h>

#include "rtc_ds3231.hpp"
#include "seismometer_utils.hpp"

#define RTC_DS3231_I2C_ADDRESS 0x68

typedef struct 
{
  i2c_inst_t *i2c_inst;
} rtc_ds3231_s;

static rtc_ds3231_s rtc_ds3231_context = 
{
  .i2c_inst = nullptr, 
};


void rtc_ds3231_init(i2c_inst_t *i2c_inst)
{
  printf("Initializing ds3231 RTC.\n");
  error_state_update(ERROR_STATE_RTC_NOT_SET, true);

  //Write buffer index 0 is register address
  //Write buffer index 1 is write data
  uint8_t write_buffer[2];

  rtc_ds3231_context.i2c_inst = i2c_inst;
  //Register 0x0E – Control Register
  write_buffer[0] = 0x0E;
  /* Configure interrupts for Alarm 1*/
  write_buffer[1] = (1<<2) /*INTCN*/ | (1<<0) /*A1IE*/;
  assert(2 == i2c_write_blocking(rtc_ds3231_context.i2c_inst, RTC_DS3231_I2C_ADDRESS, write_buffer, 2, false));

  //Register 0x0F - Status/Control Register
  write_buffer[0] = 0x0F;
  assert(1 == i2c_write_blocking(rtc_ds3231_context.i2c_inst, RTC_DS3231_I2C_ADDRESS, write_buffer, 1, false));
  /* Get current status register */
  assert(1 == i2c_read_blocking(rtc_ds3231_context.i2c_inst, RTC_DS3231_I2C_ADDRESS, &write_buffer[1], 1, false));
  printf("RTC status register 0x%x\n", write_buffer[1]);
  /* Leave current status unmodified, disable clock pin and clear alarm states */
  write_buffer[1] &= ~((1<<3) /* EN32kHz*/ | (1<<1) /* A2F*/ | (1<<0) /* A1F*/);
  assert(2 == i2c_write_blocking(rtc_ds3231_context.i2c_inst, RTC_DS3231_I2C_ADDRESS, write_buffer, 2, false));

}
void rtc_ds3231_read()
{

}
void rtc_ds3231_get_time(seismometer_time_s *time)
{
  assert(time != nullptr);
  *time = {0};
}
