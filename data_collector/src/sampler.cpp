#include <cassert>
#include <cstring>
#include <cstdio>

#include <pico/stdlib.h>
#include <pico/sem.h>

#include "mpu-6500.hpp"
#include "sampler.hpp"

static sample_thread_args_s *args_ptr = nullptr;
static repeating_timer_t     sample_timer     = {0};
static semaphore_t           sample_semaphore = {0}; 

void sampler_thread_pass_args(sample_thread_args_s *args)
{
  assert(nullptr == args_ptr);
  args_ptr = args;
}

static bool sample_timer_callback(repeating_timer_t *rt)
{
  assert(sem_release((semaphore_t*)rt->user_data));
  return true; /*true to continue repeating, false to stop.*/
}

static void sample_mpu_6500(sample_index_t index, const absolute_time_t *time)
{
  /* Sample sensor temperature */
  seismometer_sample_s temperature_sample; 
  memset(&temperature_sample, 0, sizeof(seismometer_sample_s));
  temperature_sample.type  = SEISMOMETER_SAMPLE_TYPE_ACCELEROMETER_TEMPERATURE;
  temperature_sample.index = index;
  temperature_sample.time  = *time;
  temperature_sample.temperature = mpu_6500_temperature_to_m_celsius(mpu_6500_temperature());
  assert(queue_try_add(args_ptr->sample_queue, &temperature_sample));

  /* Sample acceleration */
  mpu_6500_accelerometer_data_s accelerometer_data;
  mpu_6500_accelerometer_data(&accelerometer_data);
  seismometer_sample_s acceleration_sample; 
  memset(&acceleration_sample, 0, sizeof(seismometer_sample_s));
  acceleration_sample.type  = SEISMOMETER_SAMPLE_TYPE_ACCELERATION;
  acceleration_sample.index = index;
  acceleration_sample.time  = *time;
  acceleration_sample.acceleration.x = mpu_6500_acceleration_to_mm_ps2(accelerometer_data.x);
  acceleration_sample.acceleration.y = mpu_6500_acceleration_to_mm_ps2(accelerometer_data.y);
  acceleration_sample.acceleration.z = mpu_6500_acceleration_to_mm_ps2(accelerometer_data.z);
  assert(queue_try_add(args_ptr->sample_queue, &acceleration_sample));
}

void sampler_thread_main()
{
  printf("Initializing sample semaphore.\n");
  sem_init(&sample_semaphore, 0, 1);

  printf("Starting sample timer.\n");
  assert(add_repeating_timer_us(-SEISMOMETER_SAMPLE_PERIOD_US, sample_timer_callback, &sample_semaphore, &sample_timer));

  sample_index_t sample_index = 0;
  while (1)
  {
    sem_acquire_blocking(&sample_semaphore);
    /* Read from sensor */
    absolute_time_t mpu_6500_read_time = get_absolute_time();
    mpu_6500_read();

    sample_mpu_6500(sample_index, &mpu_6500_read_time);

    sample_index++;
  }
}