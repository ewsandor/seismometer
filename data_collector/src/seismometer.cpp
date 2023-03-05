#include <cassert>
#include <cmath>
#include <cstdio>

#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <hardware/watchdog.h>
#include <pico/binary_info.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>

#include "mpu-6500.hpp"
#include "adc_manager.hpp"
#include "rtc_ds3231.hpp"
#include "sample_handler.hpp"
#include "sampler.hpp"
#include "seismometer_config.hpp"
#include "seismometer_utils.hpp"

#define SMPS_CONTROL_PIN 23
#define STATUS_LED_PIN   PICO_DEFAULT_LED_PIN

critical_section_t smps_control_critical_section = {0};
static unsigned int smps_control_vote_mask = 0;
void smps_control_force_pwm(smps_control_client_e client)
{
  assert(client < SMPS_CONTROL_CLIENT_MAX);
  critical_section_enter_blocking(&smps_control_critical_section);
  if(0 == smps_control_vote_mask)
  {
    gpio_put(SMPS_CONTROL_PIN, 1);
  }
  smps_control_vote_mask |= (1<<client);
  critical_section_exit(&smps_control_critical_section);
}
void smps_control_power_save(smps_control_client_e client)
{
  assert(client < SMPS_CONTROL_CLIENT_MAX);
  critical_section_enter_blocking(&smps_control_critical_section);
  smps_control_vote_mask &= ~(1<<client);
  if(0 == smps_control_vote_mask)
  {
    gpio_put(SMPS_CONTROL_PIN, 0);
  }
  critical_section_exit(&smps_control_critical_section);
}

critical_section_t error_state_critical_section = {0};
static unsigned int error_state_mask = (1<<ERROR_STATE_BOOT);
static void error_state_init()
{
  critical_section_init(&error_state_critical_section);
  printf("Initialized error state manager.\n");
}
void error_state_update(error_state_e state, bool in_error)
{
  critical_section_enter_blocking(&error_state_critical_section);
  if(in_error == true)
  {
    error_state_mask |= (1<<state);
  }
  else
  {
    error_state_mask &= ~(1<<state);
  }
  critical_section_exit(&error_state_critical_section);
}
error_state_mask_t error_state_get()
{
  critical_section_enter_blocking(&error_state_critical_section);
  error_state_mask_t ret_val = error_state_mask;
  critical_section_exit(&error_state_critical_section);
  return ret_val;
}

static void smps_control_init()
{
  printf("Initializing SMPS power-save control.\n");
  bi_decl(bi_1pin_with_name(SMPS_CONTROL_PIN, "SMPS Power-Saving Control"));
  critical_section_init(&smps_control_critical_section);
  gpio_init(SMPS_CONTROL_PIN);
  gpio_set_dir(SMPS_CONTROL_PIN, GPIO_OUT);
  gpio_pull_down(SMPS_CONTROL_PIN);
  gpio_put(SMPS_CONTROL_PIN, 0);

}
static void status_led_init()
{
  printf("Initializing status LED.\n");
  bi_decl(bi_1pin_with_name(STATUS_LED_PIN, "Status LED"));
  gpio_init(STATUS_LED_PIN);
  gpio_set_dir(STATUS_LED_PIN, GPIO_OUT);
  gpio_pull_down(STATUS_LED_PIN);
  gpio_put(STATUS_LED_PIN, 0);

}
static void status_led_update(bool enabled)
{
  static bool status_led_enabled = false;
  if(enabled != status_led_enabled)
  {
    status_led_enabled = enabled;
    gpio_put(STATUS_LED_PIN, enabled);
  }
}

void static i2c_init(i2c_inst_t * i2c, uint sda_pin, uint scl_pin, uint baud)
{
  printf("Initializing I2C with SDA pin %u and SCL pin %u with %uhz baud.\n", sda_pin, scl_pin, baud);
  i2c_init(i2c, baud);
  gpio_set_function(sda_pin, GPIO_FUNC_I2C);
  gpio_set_function(scl_pin, GPIO_FUNC_I2C);
  gpio_pull_up(sda_pin);
  gpio_pull_up(scl_pin);
}

static sample_thread_args_s sampler_thead_args = {0};
static queue_t              sample_queue       = {0};

void init()
{
  stdio_init_all();
  #ifdef LIB_PICO_STDIO_UART
  uart_set_baudrate(uart_default, 921600);
  #endif
  #ifdef LIB_PICO_STDIO_USB
  printf("Delaying for USB connection...\n");
  sleep_ms(TIME_S_TO_MS(5));
  #endif
  error_state_init();
}

void boot()
{
  if (watchdog_caused_reboot()) 
  {
    printf("\n\n\n"
          "##################################################"
          "##################################################\n"
          "Rebooted by Watchdog!\n");
  } 
  printf("Starting boot.\n");
  watchdog_enable(TIME_US_TO_MS(SEISMOMETER_WATCHDOG_PERIOD_US), 1);
  printf("Enabled %u ms watchdog.\n", TIME_US_TO_MS(SEISMOMETER_WATCHDOG_PERIOD_US));
  bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
  i2c_init(i2c0, PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, 100*1000);
  watchdog_update();
  smps_control_init();
  watchdog_update();
  status_led_init();
  watchdog_update();
  mpu_6500_init(i2c0);
  watchdog_update();
//  mpu_6500_calibrate();
  watchdog_update();
  rtc_ds3231_init(i2c0);
  watchdog_update();
  adc_manager_init(ADC_CH_TO_MASK(ADC_CH_PENDULUM_10X) | ADC_CH_TO_MASK(ADC_CH_PENDULUM_100X));
  watchdog_update();
  printf("Initializing sample queue\n");
  queue_init(&sample_queue, sizeof(seismometer_sample_s), SEISMOMETER_SAMPLE_QUEUE_SIZE);
  watchdog_update();
  printf("Starting sampler thread.\n");
  sampler_thead_args.sample_queue = &sample_queue;
  sampler_thread_pass_args(&sampler_thead_args);
  multicore_launch_core1(sampler_thread_main);
  watchdog_update();
  error_state_update(ERROR_STATE_BOOT, false);
  printf("Boot complete!\n");
}

int main() 
{
  init();
  boot();

  watchdog_update();

  set_sample_handler_epoch(get_absolute_time());

  while(1)
  {
    error_state_mask_t error_state = error_state_get();
    status_led_update(error_state==0);
    if(error_state != 0) 
    {
      printf("ERROR STATE 0x%x\n", error_state);
    }

    seismometer_sample_s sample;
    printf("Queue length %u\n", queue_get_level(&sample_queue));
    watchdog_update();
    queue_remove_blocking(&sample_queue, &sample);
    sample_handler(&sample);
  }

  return 0;
}