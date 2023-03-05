#ifndef __RTC_DS3231_HPP__
#define __RTC_DS3231_HPP__

#include <hardware/i2c.h>

#include "seismometer_types.hpp"

void rtc_ds3231_init(i2c_inst_t *i2c);
void rtc_ds3231_read();
void rtc_ds3231_get_time(seismometer_time_s *time);

#endif //__RTC_DS3231_HPP__