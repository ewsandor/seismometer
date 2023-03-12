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
#include "sd_card_spi.hpp"
#include "seismometer_config.hpp"
#include "seismometer_debug.hpp"
#include "seismometer_eeprom.hpp"
#include "seismometer_utils.hpp"

#define SMPS_CONTROL_PIN 23
#define STATUS_LED_PIN   PICO_DEFAULT_LED_PIN

critical_section_t smps_control_critical_section = {0};
static unsigned int smps_control_vote_mask = 0;
void smps_control_force_pwm(smps_control_client_e client)
{
  SEISMOMETER_ASSERT(client < SMPS_CONTROL_CLIENT_MAX);
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
  SEISMOMETER_ASSERT(client < SMPS_CONTROL_CLIENT_MAX);
  critical_section_enter_blocking(&smps_control_critical_section);
  smps_control_vote_mask &= ~(1<<client);
  if(0 == smps_control_vote_mask)
  {
    gpio_put(SMPS_CONTROL_PIN, 0);
  }
  critical_section_exit(&smps_control_critical_section);
}

critical_section_t error_state_critical_section = {0};
static error_state_mask_t error_state_mask = (1<<ERROR_STATE_BOOT) | 
                                             (1<<ERROR_STATE_SD_SPI_0_NOT_MOUNTED) | 
                                             (1<<ERROR_STATE_SD_SPI_0_SAMPLE_FILE_ERROR);
static void error_state_init()
{
  critical_section_init(&error_state_critical_section);
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Initialized error state manager.\n");
}
void error_state_update(const error_state_e state, const bool in_error)
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
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Initializing SMPS power-save control.\n");
  bi_decl(bi_1pin_with_name(SMPS_CONTROL_PIN, "SMPS Power-Saving Control"));
  critical_section_init(&smps_control_critical_section);
  gpio_init(SMPS_CONTROL_PIN);
  gpio_set_dir(SMPS_CONTROL_PIN, GPIO_OUT);
  gpio_pull_down(SMPS_CONTROL_PIN);
  gpio_put(SMPS_CONTROL_PIN, 0);

}
static void status_led_init()
{
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Initializing status LED.\n");
  bi_decl(bi_1pin_with_name(STATUS_LED_PIN, "Status LED"));
  gpio_init(STATUS_LED_PIN);
  gpio_set_dir(STATUS_LED_PIN, GPIO_OUT);
  gpio_pull_down(STATUS_LED_PIN);
  gpio_put(STATUS_LED_PIN, 0);

}
static void status_led_update(bool enabled)
{
  static bool led_state=false;
  bool new_led_state = false;

  if(enabled)
  {
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    /*1024ms heartbeat to avoid division operator */
    new_led_state = ((now_ms & (1024-1)) < 512);
  }

  if(new_led_state != led_state)
  {
    led_state = new_led_state;
    gpio_put(STATUS_LED_PIN, led_state);
  }
}

void static i2c_init(i2c_inst_t * i2c, uint sda_pin, uint scl_pin, uint baud)
{
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Initializing I2C with SDA pin %u and SCL pin %u with %uhz baud.\n", sda_pin, scl_pin, baud);
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
  stdio_set_translate_crlf(&stdio_uart, false);
  #endif
  #ifdef LIB_PICO_STDIO_USB
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Delaying for USB connection...\n");
  sleep_ms(TIME_S_TO_MS(5));
  #endif
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO,  "\n\n\n\n"
          "################################################################################"
          "################################################################################\n" );
  #ifdef SEISMOMETER_DEBUG_BUILD
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "***DEBUG BUILD***\n", SEISMOMETER_WATCHDOG_PERIOD_MS);
  #endif

  if (watchdog_caused_reboot()) 
  {
    SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Reset by Watchdog!\n");
  } 
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "%ums watchdog active.\n", SEISMOMETER_WATCHDOG_PERIOD_MS);
}

void boot()
{
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Starting boot.\n");
  bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
  i2c_init(i2c0, PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, 100*1000);
  watchdog_update();
  eeprom_init();
  watchdog_update();
  error_state_init();
  watchdog_update();
  smps_control_init();
  watchdog_update();
  sd_card_spi_init();
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
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Initializing sample queue\n");
  queue_init(&sample_queue, sizeof(seismometer_sample_s), SEISMOMETER_SAMPLE_QUEUE_SIZE);
  watchdog_update();
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Starting sampler thread.\n");
  semaphore_t boot_semaphore = {0};
  sem_init(&boot_semaphore, 0, 1);
  sampler_thead_args.boot_semaphore = &boot_semaphore;
  sampler_thead_args.sample_queue   = &sample_queue;
  sampler_thread_pass_args(&sampler_thead_args);
  multicore_launch_core1(sampler_thread_main);
  /* Block until inital sampler task setup is complete */
  sem_acquire_blocking(&boot_semaphore);
  watchdog_update();
  /* Unblock sampler task to start sampling */
  sem_release(&boot_semaphore);
  error_state_update(ERROR_STATE_BOOT, false);
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Boot complete!\n");
}

int main() 
{
  #ifdef SEISMOMETER_DEBUG_BUILD
  watchdog_enable(SEISMOMETER_WATCHDOG_PERIOD_MS, true);
  #else
  watchdog_enable(SEISMOMETER_WATCHDOG_PERIOD_MS, false);
  #endif
  init();
  boot();
  watchdog_update();

  set_sample_handler_epoch(get_absolute_time());

  while(1)
  {
    watchdog_update();

    /* Handle Error State */
    error_state_mask_t error_state = error_state_get();
    status_led_update(error_state==0);
    /* Pop from Sample Queue*/
    seismometer_sample_s sample;
    unsigned int queue_length = queue_get_level(&sample_queue);
    if(queue_length >= (3*SEISMOMETER_SAMPLE_QUEUE_SIZE/4))
    {
      SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "%u - Queue length %u\n", to_ms_since_boot(get_absolute_time()), queue_length);
    }
    queue_remove_blocking(&sample_queue, &sample);

    /* Handle Sample */
    sample_handler(&sample);
  }

  return 0;
}