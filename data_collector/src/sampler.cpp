#include <cassert>
#include <cstring>
#include <cstdio>

#include <hardware/rtc.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>

#include "adc_manager.hpp"
#include "rtc_ds3231.hpp"
#include "mpu-6500.hpp"
#include "sampler.hpp"
#include "seismometer_debug.hpp"
#include "seismometer_utils.hpp"

static sample_thread_args_s *args_ptr     = nullptr;
static __scratch_y("sampler_thread_data") repeating_timer_t sample_timer = {0};
#define SAMPLE_TRIGGER_QUEUE_SIZE 2
typedef enum
{
  SAMPLE_TRIGGER_INVALID,
  SAMPLE_TRIGGER_SAMPLE_PERIOD,
  SAMPLE_TRIGGER_RTC_TICK,
  SAMPLE_TRIGGER_MAX
} sample_trigger_e;
typedef struct 
{
  sample_trigger_e trigger;

  union 
  {
    absolute_time_t timestamp;
  };
} sample_trigger_s;
static queue_t __scratch_y("sampler_thread_data") sample_trigger_queue = {0};

void sampler_thread_pass_args(sample_thread_args_s *args)
{
  SEISMOMETER_ASSERT(nullptr == args_ptr);
  args_ptr = args;
}

static bool __isr __time_critical_func(sample_timer_callback)(repeating_timer_t *rt)
{
  smps_control_force_pwm(SMPS_CONTROL_CLIENT_SAMPLER);
  sample_trigger_s sample_trigger = {.trigger=SAMPLE_TRIGGER_SAMPLE_PERIOD};
  SEISMOMETER_ASSERT_CALL(queue_try_add((queue_t*) rt->user_data, &sample_trigger));
  return true; /*true to continue repeating, false to stop.*/
}

static void __time_critical_func(sample_mpu_6500)(sample_index_t index, const absolute_time_t *time)
{
  /* Sample sensor temperature */
  seismometer_sample_s temperature_sample; 
  memset(&temperature_sample, 0, sizeof(seismometer_sample_s));
  temperature_sample.type  = SEISMOMETER_SAMPLE_TYPE_ACCELEROMETER_TEMPERATURE;
  temperature_sample.index = index;
  temperature_sample.time  = *time;
  temperature_sample.temperature = mpu_6500_temperature_to_m_celsius(mpu_6500_temperature());
  SEISMOMETER_ASSERT_CALL(queue_try_add(args_ptr->sample_queue, &temperature_sample));

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
  SEISMOMETER_ASSERT_CALL(queue_try_add(args_ptr->sample_queue, &acceleration_sample));
}

static void __time_critical_func(sample_pendulum)(sample_index_t index, const absolute_time_t *time)
{
  /* Sample Pendulum Voltage */
  seismometer_sample_s sample; 
  memset(&sample, 0, sizeof(seismometer_sample_s));
  sample.type  = SEISMOMETER_SAMPLE_TYPE_PENDULUM;
  sample.index = index;
  sample.time  = *time;

  sample.pendulum.x10  = adc_manager_get_sample_mv(ADC_CH_PENDULUM_10X);
  sample.pendulum.x100 = adc_manager_get_sample_mv(ADC_CH_PENDULUM_100X);
  SEISMOMETER_ASSERT_CALL(queue_try_add(args_ptr->sample_queue, &sample));
}

static void __time_critical_func(rtc_alarm_cb)(void* user_data_ptr)
{
  seismometer_sample_s sample; 
  memset(&sample, 0, sizeof(seismometer_sample_s));
  sample.type        = SEISMOMETER_SAMPLE_TYPE_RTC_ALARM;
  sample.alarm_index = ((unsigned int) user_data_ptr);
  SEISMOMETER_ASSERT_CALL(queue_try_add(args_ptr->sample_queue, &sample));
}

#define RTC_INTERRUPT_PIN 22
static void __isr __time_critical_func(gpio_irq_callback)(uint gpio, uint32_t event_mask)
{
  switch(gpio)
  {
    case RTC_INTERRUPT_PIN:
    {
      SEISMOMETER_ASSERT(event_mask == GPIO_IRQ_EDGE_RISE);
      sample_trigger_s sample_trigger = 
      {
        .trigger  =SAMPLE_TRIGGER_RTC_TICK,
        .timestamp=get_absolute_time(),
      };
      SEISMOMETER_ASSERT_CALL(queue_try_add(&sample_trigger_queue, &sample_trigger));
      break;
    }
    default:
    {
      SEISMOMETER_PRINTF(SEISMOMETER_LOG_ERROR, "Unexpected interrupt for GPIO %u with event mask 0x%x", gpio, event_mask);
      break;
    }
  }
}

void __time_critical_func(sampler_thread_main)()
{
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Initializing sample trigger queue.\n");
  queue_init(&sample_trigger_queue, sizeof(sample_trigger_s), SAMPLE_TRIGGER_QUEUE_SIZE);

  /* Initialize the built in RTC */
  rtc_init();
//  rtc_ds3231_set(1678047615);
  rtc_ds3231_read(get_absolute_time());
  rtc_ds3231_set_alarm1_cb(rtc_alarm_cb, (void*)1);
  rtc_ds3231_set_alarm2_cb(rtc_alarm_cb, (void*)2);

  /* Initialize GPIO interrupts */
  gpio_set_irq_callback(gpio_irq_callback);
  /* RTC Interrupt */
  gpio_init(RTC_INTERRUPT_PIN);
  gpio_set_dir(RTC_INTERRUPT_PIN, false);
  gpio_pull_up(RTC_INTERRUPT_PIN);
  gpio_set_irq_enabled(RTC_INTERRUPT_PIN, GPIO_IRQ_EDGE_RISE, true);

  /* Sampler task initial setup complete, allow logging task to finish setup */
  sem_release(args_ptr->boot_semaphore);
  /* Do not start sampling until unblocked by logging task */
  sem_acquire_blocking(args_ptr->boot_semaphore);
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Starting sample timer.\n");
  SEISMOMETER_ASSERT_CALL(add_repeating_timer_us(-SEISMOMETER_SAMPLE_PERIOD_US, sample_timer_callback, &sample_trigger_queue, &sample_timer));
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Enabling sampler GPIO interrupts.\n");
  irq_set_enabled(IO_IRQ_BANK0, true);
 
  sample_index_t sample_index = 0;
  while (1)
  {
    /* Wait for sample trigger */
    sample_trigger_s sample_trigger;
    queue_remove_blocking(&sample_trigger_queue, &sample_trigger);

    switch(sample_trigger.trigger)
    {
      case SAMPLE_TRIGGER_SAMPLE_PERIOD:
      {
        /* Read from sensors */
        absolute_time_t adc_manager_read_time = get_absolute_time();
        adc_manager_read();
        absolute_time_t mpu_6500_read_time    = get_absolute_time();
        mpu_6500_read();
        smps_control_power_save(SMPS_CONTROL_CLIENT_SAMPLER);

        /* Commit samples */
        sample_mpu_6500(sample_index, &mpu_6500_read_time);
        sample_pendulum(sample_index, &adc_manager_read_time);

        sample_index++;
        break;
      }
      case SAMPLE_TRIGGER_RTC_TICK:
      {
        rtc_ds3231_read(sample_trigger.timestamp);
        seismometer_sample_s sample; 
        memset(&sample, 0, sizeof(seismometer_sample_s));
        sample.type = SEISMOMETER_SAMPLE_TYPE_RTC_TICK;
        sample.time = sample_trigger.timestamp;
        SEISMOMETER_ASSERT_CALL(queue_try_add(args_ptr->sample_queue, &sample));
        break;
      }
      default:
      {
        SEISMOMETER_PRINTF(SEISMOMETER_LOG_ERROR, "Unexpected sample trigger %u\n", sample_trigger.trigger);
        SEISMOMETER_ASSERT(0);
      }
    }
  }
}