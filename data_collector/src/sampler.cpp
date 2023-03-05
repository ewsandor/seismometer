#include <cassert>
#include <cstring>
#include <cstdio>

#include <pico/multicore.h>
#include <pico/stdlib.h>

#include "adc_manager.hpp"
#include "rtc_ds3231.hpp"
#include "mpu-6500.hpp"
#include "sampler.hpp"
#include "seismometer_utils.hpp"

static sample_thread_args_s *args_ptr = nullptr;
static repeating_timer_t     sample_timer         = {0};
#define SAMPLE_TRIGGER_QUEUE_SIZE 4
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
static queue_t sample_trigger_queue = {0};

void sampler_thread_pass_args(sample_thread_args_s *args)
{
  assert(nullptr == args_ptr);
  args_ptr = args;
}

static bool sample_timer_callback(repeating_timer_t *rt)
{
  smps_control_force_pwm(SMPS_CONTROL_CLIENT_SAMPLER);
  sample_trigger_s sample_trigger = {.trigger=SAMPLE_TRIGGER_SAMPLE_PERIOD};
  assert(queue_try_add((queue_t*) rt->user_data, &sample_trigger));
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

static void rtc_alarm_cb(void* user_data_ptr)
{
  seismometer_sample_s sample; 
  memset(&sample, 0, sizeof(seismometer_sample_s));
  sample.type        = SEISMOMETER_SAMPLE_TYPE_RTC_ALARM;
  sample.alarm_index = ((unsigned int) user_data_ptr);
  assert(queue_try_add(args_ptr->sample_queue, &sample));
}

#define RTC_INTERRUPT_PIN 22
static void gpio_irq_callback(uint gpio, uint32_t event_mask)
{
  switch(gpio)
  {
    case RTC_INTERRUPT_PIN:
    {
      assert(event_mask == GPIO_IRQ_EDGE_RISE);
      sample_trigger_s sample_trigger = 
      {
        .trigger  =SAMPLE_TRIGGER_RTC_TICK,
        .timestamp=get_absolute_time(),
      };
      assert(queue_try_add(&sample_trigger_queue, &sample_trigger));
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
  printf("Initializing sample trigger queue.\n");
  queue_init(&sample_trigger_queue, sizeof(sample_trigger_s), SAMPLE_TRIGGER_QUEUE_SIZE);

  printf("Starting sample timer.\n");
  assert(add_repeating_timer_us(-SEISMOMETER_SAMPLE_PERIOD_US, sample_timer_callback, &sample_trigger_queue, &sample_timer));

//  rtc_ds3231_set(1677996693);
  rtc_ds3231_read(get_absolute_time());
  rtc_ds3231_set_alarm1_cb(rtc_alarm_cb, (void*)1);

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
        assert(queue_try_add(args_ptr->sample_queue, &sample));
        break;
      }
      default:
      {
        printf("Unexpected sample trigger %u\n", sample_trigger.trigger);
        assert(0);
      }
    }
  }
}