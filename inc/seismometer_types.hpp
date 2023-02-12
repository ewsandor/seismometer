#ifndef __SEISMOMETER_TYPES_HPP__
#define __SEISMOMETER_TYPES_HPP__

#include <pico/time.h>

/* Frequency in milli-Hz */
typedef uint m_hz_t;

/* Temperature in milli-Celsius */
typedef int m_celsius_t;

/* Acceleration in millimeters per second^2*/
typedef int mm_ps2_t;

typedef struct
{
  mm_ps2_t x;
  mm_ps2_t y;
  mm_ps2_t z;

} acceleration_sample_s;

typedef enum
{
  SEISMOMETER_SAMPLE_TYPE_INVALID,
  SEISMOMETER_SAMPLE_TYPE_ACCELERATION,
  SEISMOMETER_SAMPLE_TYPE_ACCELEROMETER_TEMPERATURE,
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
  };

} seismometer_sample_s;


#endif /*__SEISMOMETER_TYPES_HPP__*/