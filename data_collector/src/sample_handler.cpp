#include <cassert>
#include <cmath>
#include <cstdio>

#include <pico/time.h>

#include "filter_coefficients.hpp"
#include "fir_filter.hpp"
#include "sample_handler.hpp"

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
} sample_log_key_e;

static inline void log_sample(sample_log_key_e key, sample_index_t index, const absolute_time_t *timestamp, int64_t data)
{
  printf("SAMPLE|%02X|%08X|%016llX|%016llX\n", (uint8_t)key, (uint32_t)index, to_us_since_boot(*timestamp), data);
}

static absolute_time_t epoch = {0};
void set_sample_handler_epoch(absolute_time_t time)
{
  epoch = time;
}

 static inline m_hz_t calculate_sample_rate(const absolute_time_t *first_sample_time, const absolute_time_t *last_sample_time, sample_index_t sample_count)
{
  assert(first_sample_time != nullptr);
  assert(last_sample_time  != nullptr);

  const uint32_t first_ms = to_ms_since_boot(*first_sample_time);
  const uint32_t last_ms  = to_ms_since_boot(*last_sample_time);
  const uint32_t delta = (last_ms-first_ms);

  assert(delta > 0);

  return (((uint64_t)sample_count*1000*1000)/delta);
}

static const fir_filter_config_s acceleration_fir_filter_config
{
  .moving_average_order = 512,
  .gain_numerator   = FIR_HAMMING_LPF_100HZ_FS_10HZ_CUTOFF_GAIN_NUM,
  .gain_denominator = FIR_HAMMING_LPF_100HZ_FS_10HZ_CUTOFF_GAIN_DEN,
};

static fir_filter_c acceleration_filter_x(FIR_HAMMING_LPF_100HZ_FS_10HZ_CUTOFF_ORDER, fir_hamming_lpf_100hz_fs_10hz_cutoff, &acceleration_fir_filter_config);
static fir_filter_c acceleration_filter_y(FIR_HAMMING_LPF_100HZ_FS_10HZ_CUTOFF_ORDER, fir_hamming_lpf_100hz_fs_10hz_cutoff, &acceleration_fir_filter_config);
static fir_filter_c acceleration_filter_z(FIR_HAMMING_LPF_100HZ_FS_10HZ_CUTOFF_ORDER, fir_hamming_lpf_100hz_fs_10hz_cutoff, &acceleration_fir_filter_config);
static fir_filter_c acceleration_filter_m(FIR_HAMMING_LPF_100HZ_FS_10HZ_CUTOFF_ORDER, fir_hamming_lpf_100hz_fs_10hz_cutoff, &acceleration_fir_filter_config);

static void acceleration_sample_handler(const seismometer_sample_s *sample)
{
  assert(sample != nullptr);
  assert(SEISMOMETER_SAMPLE_TYPE_ACCELERATION == sample->type);

  static absolute_time_t last_sample_time = {0};
  mm_ps2_t acceleration_magnitude = sqrt( (sample->acceleration.x*sample->acceleration.x) + 
                                          (sample->acceleration.y*sample->acceleration.y) + 
                                          (sample->acceleration.z*sample->acceleration.z) );

  acceleration_filter_x.push_sample(sample->acceleration.x);
  acceleration_filter_y.push_sample(sample->acceleration.y);
  acceleration_filter_z.push_sample(sample->acceleration.z);
  acceleration_filter_m.push_sample(acceleration_magnitude);

  log_sample(SAMPLE_LOG_ACCEL_X,          sample->index, &sample->time, sample->acceleration.x);
  log_sample(SAMPLE_LOG_ACCEL_Y,          sample->index, &sample->time, sample->acceleration.y);
  log_sample(SAMPLE_LOG_ACCEL_Z,          sample->index, &sample->time, sample->acceleration.z);
  log_sample(SAMPLE_LOG_ACCEL_M,          sample->index, &sample->time, acceleration_magnitude);
  log_sample(SAMPLE_LOG_ACCEL_X_FILTERED, sample->index, &sample->time, acceleration_filter_x.get_filtered_sample_dc_offset_removed());
  log_sample(SAMPLE_LOG_ACCEL_Y_FILTERED, sample->index, &sample->time, acceleration_filter_y.get_filtered_sample_dc_offset_removed());
  log_sample(SAMPLE_LOG_ACCEL_Z_FILTERED, sample->index, &sample->time, acceleration_filter_z.get_filtered_sample_dc_offset_removed());
  log_sample(SAMPLE_LOG_ACCEL_M_FILTERED, sample->index, &sample->time, acceleration_filter_m.get_filtered_sample_dc_offset_removed());

#ifdef SEISMOMETER_SAMPLE_DEBUG_PRINT
  printf("i: %6u hz: %7.3f mean hz: %7.3f - X: %7.3f Y: %7.3f Z: %7.3f %M: %7.3f\n", 
    sample->index, 
    ((double)calculate_sample_rate(&last_sample_time, &sample->time, 1            )/1000),
    ((double)calculate_sample_rate(&epoch,            &sample->time, sample->index)/1000),
    ((double)sample->acceleration.x)/1000,
    ((double)sample->acceleration.y)/1000,
    ((double)sample->acceleration.z)/1000,
    ((double)acceleration_magnitude)/1000);
#endif

  last_sample_time = sample->time;
}

static void accelerometer_temperature_sample_handler(const seismometer_sample_s *sample)
{
  assert(sample != nullptr);
  assert(SEISMOMETER_SAMPLE_TYPE_ACCELEROMETER_TEMPERATURE == sample->type);
  static absolute_time_t last_sample_time = {0};

#ifdef SEISMOMETER_SAMPLE_DEBUG_PRINT
  printf("i: %6u hz: %7.3f mean hz: %7.3f - : %6.3fC\n", 
    sample->index, 
    ((double)calculate_sample_rate(&last_sample_time, &sample->time, 1            )/1000),
    ((double)calculate_sample_rate(&epoch,            &sample->time, sample->index)/1000),
    ((double)sample->temperature)/1000);

  log_sample(SAMPLE_LOG_ACCEL_TEMP, sample->index, &sample->time, sample->temperature);
#endif

  last_sample_time = sample->time;
}

static const fir_filter_config_s pendulum_fir_filter_config
{
  .moving_average_order = 512,
  .gain_numerator   = FIR_HAMMING_LPF_100HZ_FS_10HZ_CUTOFF_GAIN_NUM,
  .gain_denominator = FIR_HAMMING_LPF_100HZ_FS_10HZ_CUTOFF_GAIN_DEN,
};
static fir_filter_c pendulum_10x_filter (FIR_HAMMING_LPF_100HZ_FS_10HZ_CUTOFF_ORDER, fir_hamming_lpf_100hz_fs_10hz_cutoff, &pendulum_fir_filter_config);
static fir_filter_c pendulum_100x_filter(FIR_HAMMING_LPF_100HZ_FS_10HZ_CUTOFF_ORDER, fir_hamming_lpf_100hz_fs_10hz_cutoff, &pendulum_fir_filter_config);
static void pendulum_sample_handler(const seismometer_sample_s *sample)
{
  assert(sample != nullptr);
  assert(SEISMOMETER_SAMPLE_TYPE_PENDULUM == sample->type);
  static absolute_time_t last_sample_time = {0};

#ifdef SEISMOMETER_SAMPLE_DEBUG_PRINT
  printf("i: %6u hz: %7.3f mean hz: %7.3f - : 10x %6.3f 100x %6.3fV\n", 
    sample->index, 
    ((double)calculate_sample_rate(&last_sample_time, &sample->time, 1            )/1000),
    ((double)calculate_sample_rate(&epoch,            &sample->time, sample->index)/1000),
    ((double)sample->pendulum.x10 )/1000,
    ((double)sample->pendulum.x100)/1000);
#endif

  log_sample(SAMPLE_LOG_PENDULUM_10X,  sample->index, &sample->time, sample->pendulum.x10 );
  log_sample(SAMPLE_LOG_PENDULUM_100X, sample->index, &sample->time, sample->pendulum.x100);

  pendulum_10x_filter.push_sample (sample->pendulum.x10);
  pendulum_100x_filter.push_sample(sample->pendulum.x100);

  if( (pendulum_100x_filter.get_filtered_sample_dc_offset_removed() >  500) ||
      (pendulum_100x_filter.get_filtered_sample_dc_offset_removed() < -500) )
  {
    log_sample(SAMPLE_LOG_PENDULUM_FILTERED, sample->index, &sample->time, pendulum_10x_filter.get_filtered_sample_dc_offset_removed()*10);
  }
  else
  {
    log_sample(SAMPLE_LOG_PENDULUM_FILTERED, sample->index, &sample->time, pendulum_100x_filter.get_filtered_sample_dc_offset_removed());
  }


  last_sample_time = sample->time;
}


void sample_handler(const seismometer_sample_s *sample)
{
  assert(sample != nullptr);

  switch(sample->type)
  {
    case SEISMOMETER_SAMPLE_TYPE_ACCELERATION:
    {
      acceleration_sample_handler(sample);
      break;
    }
    case SEISMOMETER_SAMPLE_TYPE_ACCELEROMETER_TEMPERATURE:
    {
      accelerometer_temperature_sample_handler(sample);
      break;
    }
    case SEISMOMETER_SAMPLE_TYPE_PENDULUM:
    {
      pendulum_sample_handler(sample);
      break;
    }
    default:
    {
      printf("Unsupported sample type %u!", sample->type);
      assert(0);
      break;
    }
  }
}