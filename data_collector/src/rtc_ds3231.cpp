#include <cassert>
#include <cstdio>

#include <hardware/i2c.h>

#include "rtc_ds3231.hpp"
#include "seismometer_utils.hpp"

#define RTC_DS3231_I2C_ADDRESS 0x68

typedef struct
{
  uint8_t seconds;
  uint8_t minutes;
  uint8_t hours;
  uint8_t day;
  uint8_t date;
  uint8_t month;
  uint8_t year;

  bool    century;
  bool    oscillator_stopped;
  bool    busy;
  bool    alarm1_triggered;
  bool    alarm2_triggered;
  int8_t  aging_offset;
  int8_t  temperature;          /* Integer *C */
  uint8_t temperature_fraction; /* 0.25 *C */
} rtc_ds3231_data_s;

typedef struct 
{
  i2c_inst_t        *i2c_inst;
  rtc_ds3231_data_s  data;
} rtc_ds3231_s;

static rtc_ds3231_s context = 
{
  .i2c_inst = nullptr, 
};


void rtc_ds3231_init(i2c_inst_t *i2c_inst)
{
  printf("Initializing ds3231 RTC.\n");
  error_state_update(ERROR_STATE_RTC_NOT_SET, true);

  context.i2c_inst = i2c_inst;

  //Write buffer index 0 is register address
  //Write buffer index 1 is write data
  uint8_t write_buffer[2];

  //Register 0x07 – Alarm 1 Second
  write_buffer[0] = 0x07;
  write_buffer[1] = 0x0 /* match when seconds is 00 */;
  assert(2 == i2c_write_blocking(context.i2c_inst, RTC_DS3231_I2C_ADDRESS, write_buffer, 2, false));
  //Register 0x08 – Alarm 1 Minute
  write_buffer[0] = 0x08;
  write_buffer[1] = (1<<7) /* A1M2 - match any minute */;
  assert(2 == i2c_write_blocking(context.i2c_inst, RTC_DS3231_I2C_ADDRESS, write_buffer, 2, false));
  //Register 0x09 – Alarm 1 Hour
  write_buffer[0] = 0x09;
  write_buffer[1] = (1<<7) /* A1M2 - match any hour */;
  assert(2 == i2c_write_blocking(context.i2c_inst, RTC_DS3231_I2C_ADDRESS, write_buffer, 2, false));
  //Register 0x0a – Alarm 1 Day
  write_buffer[0] = 0x0a;
  write_buffer[1] = (1<<7) /* A1M2 - match any day */;
  assert(2 == i2c_write_blocking(context.i2c_inst, RTC_DS3231_I2C_ADDRESS, write_buffer, 2, false));

  //Register 0x0E – Control Register
  write_buffer[0] = 0x0E;
  /* Configure interrupts for Alarm 1*/
  write_buffer[1] = (1<<2) /*INTCN*/ | (1<<0) /*A1IE*/;
  assert(2 == i2c_write_blocking(context.i2c_inst, RTC_DS3231_I2C_ADDRESS, write_buffer, 2, false));
  //Register 0x0F - Status/Control Register
  write_buffer[0] = 0x0F;
  assert(1 == i2c_write_blocking(context.i2c_inst, RTC_DS3231_I2C_ADDRESS, write_buffer, 1, false));
  /* Get current status register */
  assert(1 == i2c_read_blocking(context.i2c_inst, RTC_DS3231_I2C_ADDRESS, &write_buffer[1], 1, false));
  printf("RTC status register 0x%x\n", write_buffer[1]);
  /* Leave current status unmodified, disable clock pin and clear alarm states */
  write_buffer[1] &= ~((1<<3) /* EN32kHz*/ | (1<<1) /* A2F*/ | (1<<0) /* A1F*/);
  assert(2 == i2c_write_blocking(context.i2c_inst, RTC_DS3231_I2C_ADDRESS, write_buffer, 2, false));

}

void rtc_ds3231_read()
{
  /* Reset register address to 0x00 */
  uint8_t write_buffer=0x00;
  assert(1 == i2c_write_blocking(context.i2c_inst, RTC_DS3231_I2C_ADDRESS, &write_buffer, 1, false));
  /* Read all 19 registers */
  uint8_t read_buffer[19];
  /* Get current status register */
  assert(19 == i2c_read_blocking(context.i2c_inst, RTC_DS3231_I2C_ADDRESS, read_buffer, 19, false));

  context.data.seconds = (read_buffer[0] & 0xF) + (((read_buffer[0]>>4) & 0x7)*10);
  context.data.minutes = (read_buffer[1] & 0xF) + (((read_buffer[1]>>4) & 0x7)*10);
  context.data.hours   = (read_buffer[2] & 0xF);
  if(0 != (read_buffer[2] & (1<<4)))
  {
    /* Handle 10-hour bit*/
    context.data.hours += 10;
  }
  if(0 != (read_buffer[2] & (1<<5)))
  {
    /* Handle 20-Hour/PM bit */
    if(0 == (read_buffer[2] & (1<<6)))
    {
      /* 24-Hour Mode, 20-Hour bit */
      context.data.hours += 20;
    }
    else
    {
      /* 12-Hour Mode, PM bit */
      context.data.hours += 12;
    }
  }
  context.data.day     = (read_buffer[3] & 0x7);
  context.data.date    = (read_buffer[4] & 0xF) + (((read_buffer[4] >> 4) & 0x3)*10);
  context.data.month   = (read_buffer[5] & 0xF) + (((read_buffer[5] >> 4) & 0x1)*10);
  context.data.year    = (read_buffer[6] & 0xF) + (((read_buffer[6] >> 4) & 0xF)*10);
  context.data.century = ((read_buffer[5] & (1<<7)) != 0);

  context.data.oscillator_stopped   = ((read_buffer[0xF] & (1<<7)) != 0);
  context.data.busy                 = ((read_buffer[0xF] & (1<<2)) != 0);
  context.data.alarm1_triggered     = ((read_buffer[0xF] & (1<<1)) != 0);
  context.data.alarm2_triggered     = ((read_buffer[0xF] & (1<<0)) != 0);
  context.data.aging_offset         = (read_buffer[0x10]);
  context.data.temperature          = (read_buffer[0x11]);
  context.data.temperature_fraction = (read_buffer[0x12]>>6);
}

void rtc_ds3231_get_time(seismometer_time_s *time)
{
  assert(time != nullptr);
  *time = {0};
}
