#include <cassert>
#include <cmath>
#include <cstdio>

#include <f_util.h>
#include <ff.h>
#include <hardware/gpio.h>
#include <pico/time.h>

#include "filter_coefficients.hpp"
#include "fir_filter.hpp"
#include "rtc_ds3231.hpp"
#include "sample_handler.hpp"
#include "sd_card_spi.hpp"
#include "seismometer_utils.hpp"

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

/* Length of sample data filename not including null character i.e. 'seismometer_2023-03-06.dat\0' */
#define SAMPLE_DATA_FILENAME_LENGTH 26
FIL sample_data_file;
void sample_file_open()
{
  /*SAMPLE_DATA_FILENAME_LENGTH+1 for NULL character*/
  char filename[SAMPLE_DATA_FILENAME_LENGTH+1]  = {'\0'};

  seismometer_time_s time_s;
  rtc_ds3231_get_time(&time_s);
  assert(SAMPLE_DATA_FILENAME_LENGTH == strftime(filename, sizeof(filename), "seismometer_%F.dat", &time_s));

  error_state_update(ERROR_STATE_SD_SPI_0_SAMPLE_FILE_CLOSED, true);

  FRESULT fr = f_open(&sample_data_file, filename, FA_OPEN_APPEND | FA_WRITE);
  if (FR_OK == fr)
  {
    error_state_update(ERROR_STATE_SD_SPI_0_SAMPLE_FILE_CLOSED, false);
    printf("Opened sample data file '%s'.\n", filename);

    char buffer[64] = {'\0'};
    assert( sizeof(buffer) > strftime(buffer, sizeof(buffer), "I|Opened at %FT%T.", &time_s));
    
    if(f_putc('\n', &sample_data_file) < 0)
    {
      error_state_update(ERROR_STATE_SD_SPI_0_SAMPLE_FILE_CLOSED, true);
    }
    if(!error_state_check(ERROR_STATE_SD_SPI_0_SAMPLE_FILE_CLOSED))
    {
      if(f_puts(buffer, &sample_data_file) < 0)
      {
        error_state_update(ERROR_STATE_SD_SPI_0_SAMPLE_FILE_CLOSED, true);
      }
    }
  }
  else
  {
    printf("Error (%u) opening sample data file '%s' - %s.\n", fr, filename, FRESULT_str(fr));
  }
}
void sample_file_close()
{
  error_state_update(ERROR_STATE_SD_SPI_0_SAMPLE_FILE_CLOSED, true);

  FRESULT fr = f_close(&sample_data_file);
  if (FR_OK == fr)
  {
    puts("Closed sample data file.");
  }
  else
  {
    printf("Error (%u) closing sample data file - %s.\n", fr, FRESULT_str(fr));
  }
}

static inline void log_sample(sample_log_key_e key, sample_index_t index, uint64_t timestamp, int64_t data)
{
  char buffer[48];
  snprintf(buffer, sizeof(buffer), "S|%02X|%08X|%016llX|%016llX", (uint8_t)key, (uint32_t)index, timestamp, data);
  puts(buffer);

  if(!error_state_check(ERROR_STATE_SD_SPI_0_SAMPLE_FILE_CLOSED))
  {
    if(f_putc('\n', &sample_data_file) < 0)
    {
      sample_file_close();
    }
  }
  if(!error_state_check(ERROR_STATE_SD_SPI_0_SAMPLE_FILE_CLOSED))
  {
    if(f_puts(buffer, &sample_data_file) < 0)
    {
      sample_file_close();
    }
  }
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

  uint64_t timestamp = rtc_ds3231_absolute_time_to_epoch_ms(sample->time);
  log_sample(SAMPLE_LOG_ACCEL_X,          sample->index, timestamp, sample->acceleration.x);
  log_sample(SAMPLE_LOG_ACCEL_Y,          sample->index, timestamp, sample->acceleration.y);
  log_sample(SAMPLE_LOG_ACCEL_Z,          sample->index, timestamp, sample->acceleration.z);
  log_sample(SAMPLE_LOG_ACCEL_M,          sample->index, timestamp, acceleration_magnitude);
  log_sample(SAMPLE_LOG_ACCEL_X_FILTERED, sample->index, timestamp, acceleration_filter_x.get_filtered_sample_dc_offset_removed());
  log_sample(SAMPLE_LOG_ACCEL_Y_FILTERED, sample->index, timestamp, acceleration_filter_y.get_filtered_sample_dc_offset_removed());
  log_sample(SAMPLE_LOG_ACCEL_Z_FILTERED, sample->index, timestamp, acceleration_filter_z.get_filtered_sample_dc_offset_removed());
  log_sample(SAMPLE_LOG_ACCEL_M_FILTERED, sample->index, timestamp, acceleration_filter_m.get_filtered_sample_dc_offset_removed());

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

  uint64_t timestamp = rtc_ds3231_absolute_time_to_epoch_ms(sample->time);
  log_sample(SAMPLE_LOG_PENDULUM_10X,  sample->index, timestamp, sample->pendulum.x10 );
  log_sample(SAMPLE_LOG_PENDULUM_100X, sample->index, timestamp, sample->pendulum.x100);

  pendulum_10x_filter.push_sample (sample->pendulum.x10);
  pendulum_100x_filter.push_sample(sample->pendulum.x100);

  if( (pendulum_100x_filter.get_filtered_sample_dc_offset_removed() >  500) ||
      (pendulum_100x_filter.get_filtered_sample_dc_offset_removed() < -500) )
  {
    log_sample(SAMPLE_LOG_PENDULUM_FILTERED, sample->index, timestamp, pendulum_10x_filter.get_filtered_sample_dc_offset_removed()*10);
  }
  else
  {
    log_sample(SAMPLE_LOG_PENDULUM_FILTERED, sample->index, timestamp, pendulum_100x_filter.get_filtered_sample_dc_offset_removed());
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
    case SEISMOMETER_SAMPLE_TYPE_RTC_TICK:
    {
      seismometer_time_s time_s;
      absolute_time_t reference_time = rtc_ds3231_get_time(&time_s);
      assert(absolute_time_diff_us(reference_time, sample->time)==0);
      char time_string[128];
      strftime(time_string, 128, "%FT%T", &time_s);
      printf("%u - RTC %s trigger time %llu.%06llu\n", 
      to_ms_since_boot(get_absolute_time()),
      time_string,
      to_us_since_boot(reference_time)/1000000, 
      to_us_since_boot(reference_time)%1000000);

      if(error_state_check(ERROR_STATE_SD_SPI_0_NOT_MOUNTED))
      {
        sd_card_spi_mount(0);
      }
      if(!error_state_check(ERROR_STATE_SD_SPI_0_NOT_MOUNTED))
      {
        if(error_state_check(ERROR_STATE_SD_SPI_0_SAMPLE_FILE_CLOSED))
        {
          sample_file_open();
        }
        else
        {
          if(FR_OK != f_sync(&sample_data_file))
          {
            sample_file_close();
            sd_card_spi_unmount(0);
          }
        }
      }

      break;
    }
    case SEISMOMETER_SAMPLE_TYPE_RTC_ALARM:
    {
      printf("RTC Alarm %u!\n", sample->alarm_index);
      if(2 == sample->alarm_index)
      {
        sample_file_close();
      }
      break;
    }
    default:
    {
      printf("Unsupported sample type %u!\n", sample->type);
      assert(0);
      break;
    }
  }
}