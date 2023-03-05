#include <cassert>
#include <cstring>
#include <cstdio>

#include <pico/stdlib.h>
#include <pico/sem.h>

#include "adc_manager.hpp"
#include "rtc_ds3231.hpp"
#include "mpu-6500.hpp"
#include "sampler.hpp"
#include "seismometer_utils.hpp"

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
  smps_control_force_pwm(SMPS_CONTROL_CLIENT_SAMPLER);
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

static void sample_pendulum(sample_index_t index, const absolute_time_t *time)
{
  /* Sample Pendulum Voltage */
  seismometer_sample_s sample; 
  memset(&sample, 0, sizeof(seismometer_sample_s));
  sample.type  = SEISMOMETER_SAMPLE_TYPE_PENDULUM;
  sample.index = index;
  sample.time  = *time;

  sample.pendulum.x10  = adc_manager_get_sample_mv(ADC_CH_PENDULUM_10X);
  sample.pendulum.x100 = adc_manager_get_sample_mv(ADC_CH_PENDULUM_100X);
  assert(queue_try_add(args_ptr->sample_queue, &sample));
}

#define RTC_INTERRUPT_PIN 22
void gpio_irq_callback(uint gpio, uint32_t event_mask)
{
  switch(gpio)
  {
    case RTC_INTERRUPT_PIN:
    {
      assert(event_mask == GPIO_IRQ_EDGE_RISE);
      rtc_ds3231_read();
      break;
    }
    default:
    {
      printf("Unexpected interrupt for GPIO %u with event mask 0x%x", gpio, event_mask);
      break;
    }
  }
}

void sampler_thread_main()
{
  printf("Initializing sample semaphore.\n");
  sem_init(&sample_semaphore, 0, 1);

  printf("Starting sample timer.\n");
  assert(add_repeating_timer_us(-SEISMOMETER_SAMPLE_PERIOD_US, sample_timer_callback, &sample_semaphore, &sample_timer));

//  rtc_ds3231_set(1677996693);
  rtc_ds3231_read();
  absolute_time_t last_rtc_read = get_absolute_time();

  /* Initialize GPIO interrupts */
  gpio_set_irq_callback(gpio_irq_callback);
  /* RTC Interrupt */
  gpio_init(RTC_INTERRUPT_PIN);
  gpio_set_dir(RTC_INTERRUPT_PIN, false);
  gpio_pull_up(RTC_INTERRUPT_PIN);
  gpio_set_irq_enabled(RTC_INTERRUPT_PIN, GPIO_IRQ_EDGE_RISE, true);
  /* Enable GPIO Interrupts */
  irq_set_enabled(IO_IRQ_BANK0, true);

  sample_index_t sample_index = 0;
  while (1)
  {
    /* Wait for sample throttler */
    sem_acquire_blocking(&sample_semaphore);

    /* Read from sensors */
    absolute_time_t adc_manager_read_time = get_absolute_time();
    adc_manager_read();
    absolute_time_t mpu_6500_read_time    = get_absolute_time();
    mpu_6500_read();
    smps_control_power_save(SMPS_CONTROL_CLIENT_SAMPLER);

    if(absolute_time_diff_us(last_rtc_read, get_absolute_time()) > TIME_S_TO_US(1))
    {
//      rtc_ds3231_read();
      last_rtc_read = get_absolute_time();
    }

    /* Commit samples */
    sample_mpu_6500(sample_index, &mpu_6500_read_time);
    sample_pendulum(sample_index, &adc_manager_read_time);

    sample_index++;
  }
}