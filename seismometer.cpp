#include <cassert>
#include <cmath>
#include <cstdio>

#include "hardware/i2c.h"
#include "hardware/watchdog.h" 
#include "pico/binary_info.h"
#include "pico/stdlib.h"

#include "mpu-6500.hpp"

void i2c_init(i2c_inst_t * i2c, uint sda_pin, uint scl_pin, uint baud)
{
  printf("Initializing I2C with SDA pin %u and SCL pin %u with %uhz baud.\n", sda_pin, scl_pin, baud);
  i2c_init(i2c, baud);
  gpio_set_function(sda_pin, GPIO_FUNC_I2C);
  gpio_set_function(scl_pin, GPIO_FUNC_I2C);
  gpio_pull_up(sda_pin);
  gpio_pull_up(scl_pin);
  // Make the I2C pins available to picotool
  bi_decl(bi_2pins_with_func(sda_pin, scl_pin, GPIO_FUNC_I2C));
}

int main() 
{
  stdio_init_all();

  sleep_ms(5000);
    
  if (watchdog_caused_reboot()) 
  {
    printf("Rebooted by Watchdog!\n");
  } 

  printf("Starting boot.\n");
  watchdog_enable(100, 1);
  i2c_init(i2c_default, PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, 200*1000);
  watchdog_update();
  mpu_6500_init(i2c_default);
  watchdog_update();
  printf("Boot complete!\n");
  
  uint i = 0;
  absolute_time_t now = get_absolute_time();
  uint32_t loop_start_time = to_ms_since_boot(now);
  while(1)
  {
    watchdog_update();
    now = get_absolute_time();
    mpu_6500_loop();
    mpu_6500_accelerometer_data_s accelerometer_data;
    mpu_6500_accelerometer_data(&accelerometer_data);
    uint acceleration_magnitude = sqrt((accelerometer_data.x*accelerometer_data.x) + 
                                       (accelerometer_data.y*accelerometer_data.y) + 
                                       (accelerometer_data.z*accelerometer_data.z));

    printf("i: %06u hz: %u - X: %06d Y: %06d Z: %06d %M: %06u T: %06u\n", 
      i, ((i*1000)/(to_ms_since_boot(now)-loop_start_time)),
      accelerometer_data.x, accelerometer_data.y, accelerometer_data.z, acceleration_magnitude, mpu_6500_temperature());
    i++;
  }

  return 0;
}