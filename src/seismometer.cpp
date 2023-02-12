#include <cassert>
#include <cmath>
#include <cstdio>

#include <hardware/i2c.h>
#include <hardware/watchdog.h>
#include <pico/binary_info.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>

#include "mpu-6500.hpp"
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

sample_thread_args_s sampler_thead_args = {0};

void init()
{
  stdio_init_all();
  printf("Delaying for USB connection...\n");
  sleep_ms(3000);
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
  printf("Starting sampler thread.\n");
  sampler_thread_pass_args(&sampler_thead_args);
  multicore_launch_core1(sampler_thread_main);
  printf("Boot complete!\n");
  watchdog_update();
}

int main() 
{
  init();
  boot();

  watchdog_update();

  uint i = 0;
  absolute_time_t now = get_absolute_time();
  uint32_t loop_start_time = to_ms_since_boot(now);
  while(1)
  {
    watchdog_update();
    now = get_absolute_time();
    mpu_6500_read();
    mpu_6500_accelerometer_data_s accelerometer_data;
    mpu_6500_accelerometer_data(&accelerometer_data);
    uint acceleration_magnitude = sqrt((accelerometer_data.x*accelerometer_data.x) + 
                                       (accelerometer_data.y*accelerometer_data.y) + 
                                       (accelerometer_data.z*accelerometer_data.z));
    m_celsius_t temperature = mpu_6500_temperature_to_m_celsius(mpu_6500_temperature());

    printf("i: % 6u hz: %u - X: % 6d Y: % 6d Z: % 6d %M: % 6u T: % 2d.%03u\n", 
      i, ((i*1000)/(to_ms_since_boot(now)-loop_start_time)),
      accelerometer_data.x, accelerometer_data.y, accelerometer_data.z, 
      mpu_6500_acceleration_to_mm_ps2(acceleration_magnitude), temperature/1000, temperature%1000);
    i++;
  }

  return 0;
}