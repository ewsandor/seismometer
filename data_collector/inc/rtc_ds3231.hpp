#ifndef __RTC_DS3231_HPP__
#define __RTC_DS3231_HPP__

#include <hardware/i2c.h>
#include <pico/time.h>

#include "seismometer_types.hpp"

typedef void (*rtc_ds3231_alarm_cb)(void* user_data_ptr);

void            rtc_ds3231_init(i2c_inst_t *i2c);
/* Reads from the RTC with given reference time */
void            rtc_ds3231_read(absolute_time_t reference);
/* Sets the RTC with given time (ms since unix epoch) */
void            rtc_ds3231_set(seismometer_time_t time);
/* Returns system reference time for current time from RTC filled in *time */
absolute_time_t rtc_ds3231_get_time(seismometer_time_s *time);
/* Configure alarm callbacks */
void            rtc_ds3231_set_alarm1_cb(rtc_ds3231_alarm_cb, void* user_data_ptr);
void            rtc_ds3231_set_alarm2_cb(rtc_ds3231_alarm_cb, void* user_data_ptr);

uint64_t        rtc_ds3231_absolute_time_to_epoch_ms(absolute_time_t t);

#endif //__RTC_DS3231_HPP__