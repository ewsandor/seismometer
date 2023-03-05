#ifndef __RTC_DS3231_HPP__
#define __RTC_DS3231_HPP__

#include <hardware/i2c.h>

#include "seismometer_types.hpp"

typedef void (*rtc_ds3231_alarm_cb)(void* user_data_ptr);

void rtc_ds3231_init(i2c_inst_t *i2c);
void rtc_ds3231_read();
void rtc_ds3231_set(seismometer_time_t time);
void rtc_ds3231_get_time(seismometer_time_s *time);
void rtc_ds3231_set_alarm1_cb(rtc_ds3231_alarm_cb, void* user_data_ptr);
void rtc_ds3231_set_alarm2_cb(rtc_ds3231_alarm_cb, void* user_data_ptr);

#endif //__RTC_DS3231_HPP__