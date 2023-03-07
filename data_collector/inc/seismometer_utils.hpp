#ifndef __SEISMOMETER_UTILS_HPP__
#define __SEISMOMETER_UTILS_HPP__

#include <pico/platform.h>

#define TIME_S_TO_MS(time_s)   (time_s*1000)
#define TIME_S_TO_US(time_s)   (TIME_S_TO_MS(time_s)*1000)
#define TIME_MS_TO_S(time_ms)  (time_ms/1000)
#define TIME_MS_TO_US(time_ms) (time_ms*1000)
#define TIME_US_TO_MS(time_us) (time_us/1000)
#define TIME_US_TO_S(time_us)  (TIME_US_TO_MS(time_us)/1000)

#define VOLTAGE_MV_TO_UV(mv)   (mv*1000)
#define VOLTAGE_UV_TO_MV(uv)   (uv/1000)

#define SEISMOMETER_MAX(a,b)   MAX(a, b)
#define SEISMOMETER_MIN(a,b)   MIN(a, b)

typedef enum
{
  SMPS_CONTROL_CLIENT_SAMPLER,
  SMPS_CONTROL_CLIENT_MAX,
} smps_control_client_e;

/* Force SMPS into PWM mode to minimize voltage-regulator ripple */
void smps_control_force_pwm(smps_control_client_e client);
/* Allow SMPS to enter power-saving mode which may introduce voltage-regulator ripple */
void smps_control_power_save(smps_control_client_e client);

typedef enum
{
  ERROR_STATE_BOOT,
  ERROR_STATE_RTC_NOT_SET,
  ERROR_STATE_SD_SPI_0_NOT_MOUNTED,
  ERROR_STATE_SD_SPI_0_SAMPLE_FILE_CLOSED,
  ERROR_STATE_MAX,

} error_state_e;
typedef unsigned int error_state_mask_t;
void               error_state_update(const error_state_e state, const bool in_error);
error_state_mask_t error_state_get();
inline bool        error_state_check(const error_state_e state)
{
  assert(state < ERROR_STATE_MAX);
  error_state_mask_t error_state = error_state_get();
  return (0 != (error_state & (1<<state)));
}

#endif /*__SEISMOMETER_UTILS_HPP__*/