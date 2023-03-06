#include <cassert>
#include <cstdio>

#include <hardware/i2c.h>
#include <hardware/rtc.h>
#include <pico/sync.h>

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
  i2c_inst_t          *i2c_inst;
  critical_section_t   critical_section;
  rtc_ds3231_data_s    data;
  absolute_time_t      reference_time;

  rtc_ds3231_alarm_cb  alarm1_cb;
  void                *alarm1_user_data_ptr;
  rtc_ds3231_alarm_cb  alarm2_cb;
  void                *alarm2_user_data_ptr;

} rtc_ds3231_s;

static rtc_ds3231_s context = 
{
  .i2c_inst             = nullptr,
  .critical_section     = {0},
  .data                 = {0},
  .reference_time       = {0},

  .alarm1_cb            = nullptr,
  .alarm1_user_data_ptr = nullptr,
  .alarm2_cb            = nullptr,
  .alarm2_user_data_ptr = nullptr,
};


void rtc_ds3231_init(i2c_inst_t *i2c_inst)
{
  printf("Initializing ds3231 RTC.\n");
  error_state_update(ERROR_STATE_RTC_NOT_SET, true);

  context.i2c_inst = i2c_inst;
  critical_section_init(&context.critical_section);

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
//  write_buffer[1] = (1<<2) /*INTCN*/ | (1<<0) /*A1IE*/;
  /* Configure 1Hz square wave and register for Alarm 1*/
  write_buffer[1] = (1<<0) /*A1IE*/;
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

void rtc_ds3231_read(absolute_time_t reference)
{
  /* Reset register address to 0x00 */
  uint8_t write_buffer[2];
  write_buffer[0]=0x00;
  assert(1 == i2c_write_blocking(context.i2c_inst, RTC_DS3231_I2C_ADDRESS, write_buffer, 1, true));
  /* Read all 19 registers */
  uint8_t read_buffer[19];
  /* Get current status register */
  assert(19 == i2c_read_blocking(context.i2c_inst, RTC_DS3231_I2C_ADDRESS, read_buffer, 19, false));
  /* Reset alarm interrupt flags */
  write_buffer[0] = 0x0F;
  write_buffer[1] = read_buffer[0xF] & ~((1<<1) /* A2F*/ | (1<<0) /* A1F*/);
  assert(2 == i2c_write_blocking(context.i2c_inst, RTC_DS3231_I2C_ADDRESS, write_buffer, 2, false));

  /* Parse new data */
  rtc_ds3231_data_s new_data = {0};
  new_data.seconds = (read_buffer[0] & 0xF) + (((read_buffer[0]>>4) & 0x7)*10);
  new_data.minutes = (read_buffer[1] & 0xF) + (((read_buffer[1]>>4) & 0x7)*10);
  new_data.hours   = (read_buffer[2] & 0xF);
  if(0 != (read_buffer[2] & (1<<4)))   { /* Handle 10-hour bit*/         new_data.hours += 10; }
  if(0 != (read_buffer[2] & (1<<5)))
  {
    /* Handle 20-Hour/PM bit */
    if(0 == (read_buffer[2] & (1<<6))) { /* 24-Hour Mode, 20-Hour bit */ new_data.hours += 20; }
    else                               { /* 12-Hour Mode, PM bit */      new_data.hours += 12; }
  }
  new_data.day     = (read_buffer[3] & 0x7);
  new_data.date    = (read_buffer[4] & 0xF) + (((read_buffer[4] >> 4) & 0x3)*10);
  new_data.month   = (read_buffer[5] & 0xF) + (((read_buffer[5] >> 4) & 0x1)*10);
  new_data.year    = (read_buffer[6] & 0xF) + (((read_buffer[6] >> 4) & 0xF)*10);
  new_data.century = ((read_buffer[5] & (1<<7)) != 0);

  new_data.oscillator_stopped   = ((read_buffer[0xF] & (1<<7)) != 0);
  new_data.busy                 = ((read_buffer[0xF] & (1<<2)) != 0);
  new_data.alarm2_triggered     = ((read_buffer[0xF] & (1<<1)) != 0);
  new_data.alarm1_triggered     = ((read_buffer[0xF] & (1<<0)) != 0);
  new_data.aging_offset         = (read_buffer[0x10]);
  new_data.temperature          = (read_buffer[0x11]);
  new_data.temperature_fraction = (read_buffer[0x12]>>6);
  error_state_update(ERROR_STATE_RTC_NOT_SET, new_data.oscillator_stopped);

  /* Match built in RTC to battery backed RTC */
  datetime_t pico_rtc_time = 
  {
    .year  = (1900+(int16_t)new_data.year)+(new_data.century?100:0), ///< 0..4095
    .month = (new_data.month),              ///< 1..12, 1 is January
    .day   = (new_data.date),               ///< 1..28,29,30,31 depending on month
    .dotw  = (new_data.day-1),              ///< 0..6, 0 is Sunday
    .hour  = (new_data.hours),              ///< 0..23
    .min   = (new_data.minutes),            ///< 0..59
    .sec   = (new_data.seconds),            ///< 0..59
  };
  assert(rtc_set_datetime(&pico_rtc_time));

  /* Commit new data */
  critical_section_enter_blocking(&context.critical_section);
  context.data = new_data;
  context.reference_time = reference;
  critical_section_exit(&context.critical_section);

  /* Handle alarms */
  if(new_data.alarm1_triggered && (context.alarm1_cb != nullptr))
  {
    context.alarm1_cb(context.alarm1_user_data_ptr);
  }
  if(new_data.alarm2_triggered && (context.alarm2_cb != nullptr))
  {
    context.alarm2_cb(context.alarm2_user_data_ptr);
  }
}

void rtc_ds3231_set(const seismometer_time_t time)
{
  seismometer_time_s time_s;
  seismometer_time_t_to_time_s(&time, &time_s);

  uint8_t write_buffer[8] = {0};

  /* Reset oscillator stopped status to indicate data is good */
  write_buffer[0] = 0x0F;
  assert(1 == i2c_write_blocking(context.i2c_inst, RTC_DS3231_I2C_ADDRESS, write_buffer, 1, false));
  /* Get current status register */
  assert(1 == i2c_read_blocking(context.i2c_inst, RTC_DS3231_I2C_ADDRESS, &write_buffer[1], 1, false));
  printf("RTC status register 0x%x\n", write_buffer[1]);
  /* Clear Oscillator Stop Flag */
  write_buffer[1] &= ~((1<<7) /* OSF */);
  assert(2 == i2c_write_blocking(context.i2c_inst, RTC_DS3231_I2C_ADDRESS, write_buffer, 2, false));

  write_buffer[0]=0x00; /* Reset register address to 0x00 */
  write_buffer[1+0x0]=((time_s.tm_sec%10)  & 0xF) | (((time_s.tm_sec/10) & 0x7)<<4);
  write_buffer[1+0x1]=((time_s.tm_min%10)  & 0xF) | (((time_s.tm_min/10) & 0x7)<<4);
  write_buffer[1+0x2]=((time_s.tm_hour%10) & 0xF);
  if      (time_s.tm_hour>=20) { write_buffer[1+0x2] |= (1<<5); }
  else if (time_s.tm_hour>=10) { write_buffer[1+0x2] |= (1<<4); }
  write_buffer[1+0x3]=((time_s.tm_wday+1)  & 0x7);
  write_buffer[1+0x4]=((time_s.tm_mday%10) & 0xF) | (((time_s.tm_mday/10) & 0x3)<<4);
  write_buffer[1+0x5]=(((time_s.tm_mon+1)%10)  & 0xF) | ((((time_s.tm_mon+1)/10)  & 0x1)<<4);
  if(time_s.tm_year>=100){ write_buffer[1+0x5] |= (1<<7); }
  write_buffer[1+0x6]=((time_s.tm_year%10) & 0xF) | ((((time_s.tm_year/10)%10) & 0xF)<<4);
  assert(8 == i2c_write_blocking(context.i2c_inst, RTC_DS3231_I2C_ADDRESS, write_buffer, 8, false));

}

absolute_time_t rtc_ds3231_get_time(seismometer_time_s *time)
{
  assert(time != nullptr);

  critical_section_enter_blocking(&context.critical_section);
  rtc_ds3231_data_s data_copy = context.data;
  absolute_time_t   ret_val   = context.reference_time;
  critical_section_exit(&context.critical_section);

  *time = {0};
  time->tm_sec   = (data_copy.seconds  % 60);  /* 0-59 */
  time->tm_min   = (data_copy.minutes  % 60);  /* 0-59 */
  time->tm_hour  = (data_copy.hours    % 24);  /* 0-24 */
  time->tm_mday  = (data_copy.date     % 32);  /* 1-31 */
  if(time->tm_mday == 0)      { time->tm_mday++; }
  time->tm_mon   = ((data_copy.month-1)% 12);  /* 0-11 */
  time->tm_year  = (data_copy.year     % 100); /* 0-199 */
  if(data_copy.century==true) { time->tm_year += 100; }
  time->tm_wday  = ((data_copy.day-1)  % 7);   /* 0-6 */
  time->tm_isdst = false;                     /* no DST */

  return ret_val;
}

void rtc_ds3231_set_alarm1_cb(rtc_ds3231_alarm_cb cb, void* user_data_ptr)
{
  context.alarm1_cb            = cb;
  context.alarm1_user_data_ptr = user_data_ptr;
}
void rtc_ds3231_set_alarm2_cb(rtc_ds3231_alarm_cb cb, void* user_data_ptr)
{
  context.alarm2_cb            = cb;
  context.alarm2_user_data_ptr = user_data_ptr;
}