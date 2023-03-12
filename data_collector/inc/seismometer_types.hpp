#ifndef __SEISMOMETER_TYPES_HPP__
#define __SEISMOMETER_TYPES_HPP__
#include <cassert>
#include <ctime>

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
} seismometer_sample_type_e;

typedef uint sample_index_t;

typedef struct
{
  seismometer_sample_type_e type;
  sample_index_t            index;
  absolute_time_t           time;

  union 
  {
    m_celsius_t             temperature;
    acceleration_sample_s   acceleration;
    pendulum_sample_s       pendulum;
    unsigned int            alarm_index;
  };

} seismometer_sample_s;

typedef enum
{
  SAMPLE_LOG_INVALID,
  SAMPLE_LOG_ACCEL_X,
  SAMPLE_LOG_ACCEL_Y,
  SAMPLE_LOG_ACCEL_Z,
  SAMPLE_LOG_ACCEL_M,
  SAMPLE_LOG_ACCEL_X_FILTERED,
  SAMPLE_LOG_ACCEL_Y_FILTERED,
  SAMPLE_LOG_ACCEL_Z_FILTERED,
  SAMPLE_LOG_ACCEL_M_FILTERED,
  SAMPLE_LOG_ACCEL_TEMP,
  SAMPLE_LOG_PENDULUM_10X,
  SAMPLE_LOG_PENDULUM_100X,
  SAMPLE_LOG_PENDULUM_FILTERED,
  SAMPLE_LOG_MAX_KEY,
} sample_log_key_e;
typedef uint32_t sample_log_key_mask_t;



typedef struct  __attribute__((packed)) /* 128-bytes */
{
  sample_log_key_mask_t key_mask_stdio;
  sample_log_key_mask_t key_mask_sd;
  uint8_t reserved[120]; /* For future use. */
} seismometer_eeprom_sample_log_config_s;

enum
{
  SEISMOMETER_EEPROM_VERSION_INVALID,
  SEISMOMETER_EEPROM_VERSION_1,
  SEISMOMETER_EEPROM_VERSION_MAX,
};

#define SEISMOMETER_EEPROM_IDENTIFIER_LENGTH 15
#define EEPROM_IDENTIFIER "SL-SEISMOMETER"
typedef struct  __attribute__((packed)) /* 64-bytes */
{
  uint8_t identifier[SEISMOMETER_EEPROM_IDENTIFIER_LENGTH];
  uint8_t version;
  bool    reset_requested:1;
  uint8_t pad:7;
  uint8_t reserved[47]; /* For future use. */
} seismometer_eeprom_header_s;

typedef struct __attribute__((packed))
{
  seismometer_eeprom_header_s            header;
  seismometer_eeprom_sample_log_config_s sample_log_config;
} seismometer_eeprom_data_s;



#endif /*__SEISMOMETER_TYPES_HPP__*/