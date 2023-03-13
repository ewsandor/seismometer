#ifndef __SEISMOMETER_TYPES_HPP__
#define __SEISMOMETER_TYPES_HPP__
#include <cassert>
#include <ctime>

#include <pico/sem.h>

/* Time Types */
typedef struct tm seismometer_time_s;
typedef time_t    seismometer_time_t;
inline seismometer_time_t seismometer_time_s_to_time_t(seismometer_time_s* time)
{
  return mktime(time);
}
inline void seismometer_time_t_to_time_s(const seismometer_time_t *in_time_t, seismometer_time_s* out_time_s)
{
  assert(out_time_s != nullptr);
  seismometer_time_s * result = gmtime_r(in_time_t, out_time_s);
  assert(result == out_time_s);
}

/* Frequency in milli-Hz */
typedef uint m_hz_t;

/* Temperature in milli-Celsius */
typedef int m_celsius_t;

/* Acceleration in millimeters per second^2*/
typedef int mm_ps2_t;

/* Voltage in milli-volts*/
typedef unsigned int m_volts_t;
/* Voltage in micro-volts*/
typedef uint64_t u_volts_t;

typedef struct
{
  mm_ps2_t x;
  mm_ps2_t y;
  mm_ps2_t z;

} acceleration_sample_s;

typedef struct
{
  m_volts_t x10;
  m_volts_t x100;

} pendulum_sample_s;

typedef enum
{
  SEISMOMETER_SAMPLE_TYPE_INVALID,
  SEISMOMETER_SAMPLE_TYPE_ACCELERATION,
  SEISMOMETER_SAMPLE_TYPE_ACCELEROMETER_TEMPERATURE,
  SEISMOMETER_SAMPLE_TYPE_PENDULUM,
  SEISMOMETER_SAMPLE_TYPE_RTC_TICK,
  SEISMOMETER_SAMPLE_TYPE_RTC_ALARM,
  SEISMOMETER_SAMPLE_TYPE_STDIO_CHAR_AVAILABLE,
} seismometer_sample_type_e;

typedef uint sample_index_t;

typedef struct
{
  seismometer_sample_type_e type;
  sample_index_t            index;
  absolute_time_t           time;

  union 
  {
    m_celsius_t            temperature;
    acceleration_sample_s  acceleration;
    pendulum_sample_s      pendulum;
    unsigned int           alarm_index;
    semaphore_t           *semaphore;
  };

} seismometer_sample_s;

typedef enum
{
  SAMPLE_LOG_INVALID           =  0,
  SAMPLE_LOG_ACCEL_X           =  1,
  SAMPLE_LOG_ACCEL_Y           =  2,
  SAMPLE_LOG_ACCEL_Z           =  3,
  SAMPLE_LOG_ACCEL_M           =  4,
  SAMPLE_LOG_ACCEL_X_FILTERED  =  5,
  SAMPLE_LOG_ACCEL_Y_FILTERED  =  6,
  SAMPLE_LOG_ACCEL_Z_FILTERED  =  7,
  SAMPLE_LOG_ACCEL_M_FILTERED  =  8,
  SAMPLE_LOG_ACCEL_TEMP        =  9,
  SAMPLE_LOG_PENDULUM_10X      = 10,
  SAMPLE_LOG_PENDULUM_100X     = 11,
  SAMPLE_LOG_PENDULUM_FILTERED = 12,
  SAMPLE_LOG_MAX_KEY,
} sample_log_key_e;
typedef uint32_t sample_log_key_mask_t;

#endif /*__SEISMOMETER_TYPES_HPP__*/