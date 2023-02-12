#include <cassert>
#include <cmath>
#include <cstdio>

#include <hardware/i2c.h>
#include <hardware/watchdog.h>
#include <pico/binary_info.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>

#include "mpu-6500.hpp"
#include "sample_handler.hpp"
#include "sampler.hpp"
#include "seismometer_config.hpp"
#include "seismometer_utils.hpp"

void i2c_init(i2c_inst_t * i2c, uint sda_pin, uint scl_pin, uint baud)
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
  printf("Delaying for USB connection...\n");
  sleep_ms(TIME_S_TO_MS(5));
}

void boot()
{
  if (watchdog_caused_reboot()) 
  {
    printf("Rebooted by Watchdog!\n");
  } 
  printf("Starting boot.\n");
  printf("Enabling %u ms watchdog.\n", TIME_US_TO_MS(SEISOMETER_WATCHDOG_PERIOD_US));
  watchdog_enable(TIME_US_TO_MS(SEISOMETER_WATCHDOG_PERIOD_US), 1);
  bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
  i2c_init(i2c0, PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, 100*1000);
  watchdog_update();
  mpu_6500_init(i2c0);
  watchdog_update();
//  mpu_6500_calibrate();
  watchdog_update();
  printf("Initializing sample queue\n");
  queue_init(&sample_queue, sizeof(seismometer_sample_s), SEISOMETER_SAMPLE_QUEUE_SIZE);
  watchdog_update();
  printf("Starting sampler thread.\n");
  sampler_thead_args.sample_queue = &sample_queue;
  sampler_thread_pass_args(&sampler_thead_args);
  multicore_launch_core1(sampler_thread_main);
  watchdog_update();
  printf("Boot complete!\n");
}

int main() 
{
  init();
  boot();

  watchdog_update();

  uint i = 0;
  absolute_time_t now = get_absolute_time();
  set_sample_handler_epoch(&now);
  uint32_t loop_start_time = to_ms_since_boot(now);
  while(1)
  {
    watchdog_update();
    
    seismometer_sample_s sample;
    queue_remove_blocking(&sample_queue, &sample);
    now = get_absolute_time();

    sample_handler(&sample);
  }

  return 0;
}