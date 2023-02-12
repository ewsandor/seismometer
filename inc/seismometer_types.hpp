#ifndef __SEISMOMETER_TYPES_HPP__
#define __SEISMOMETER_TYPES_HPP__

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
  SEISOMETER_SAMPLE_TYPE_INVALID,
  SEISOMETER_SAMPLE_TYPE_ACCELERATION,
  SEISOMETER_SAMPLE_TYPE_ACCELEROMETER_TEMPERATURE,

} seisometer_sample_type_e;

typedef struct
{
  seisometer_sample_type_e type;

  union 
  {
    m_celsius_t           temperature;
    acceleration_sample_s acceleration;
  };

} seisometer_sample_s;


#endif /*__SEISMOMETER_TYPES_HPP__*/