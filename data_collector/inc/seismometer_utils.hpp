#ifndef __SEISMOMETER_UTILS_HPP__
#define __SEISMOMETER_UTILS_HPP__

#include <pico/platform.h>

#define TIME_S_TO_MS(time_s)   (time_s*1000)
#define TIME_S_TO_US(time_s)   (TIME_S_TO_MS(time_s)*1000)
#define TIME_MS_TO_S(time_ms)  (time_ms/1000)
#define TIME_MS_TO_US(time_ms) (time_ms*1000)
#define TIME_US_TO_MS(time_us) (time_us/1000)
#define TIME_US_TO_S(time_us)  (TIME_US_TO_MS(time_us)/1000)

#define SEISMOMETER_MAX(a,b)   MAX(a, b)
#define SEISMOMETER_MIN(a,b)   MIN(a, b)

#endif /*__SEISMOMETER_UTILS_HPP__*/