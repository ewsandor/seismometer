#ifndef __ADC_MANAGER_HPP__
#define __ADC_MANAGER_HPP__

#include "seismometer_types.hpp"

#define ADC_CH_TO_PIN(channel)  (26+channel)
#define ADC_CH_TO_MASK(channel) (1 << channel)

#define ADC_REFERENCE_VOLTAGE_MV (3300)

typedef enum
{
  ADC_CH_0              = 0,
  ADC_CH_1              = 1,
  ADC_CH_2              = 2,
  ADC_CH_3              = 3,
  ADC_CH_4              = 4,
  /* Max channel value */
  ADC_CH_MAX            = 5,

  ADC_CH_PENDULUM_10X   = ADC_CH_0,
  ADC_CH_PENDULUM_100X  = ADC_CH_1,
  ADC_CH_IC_TEMPERATURE = ADC_CH_4, /* RP2040 Channel 4 is dedicated to the ICs internal temperature */

} adc_channel_assignment_e;
typedef unsigned int adc_channel_t;
typedef unsigned int adc_channel_mask_t;
#define ADC_MANAGER_SAMPLE_INVALID   0xFFFF
#define ADC_MANAGER_SAMPLE_MAX_VALUE ((1<<12)-1) /* RP2040 ADC has 12 bit resolution */
typedef uint16_t adc_sample_t;

/* Initializes the ADC manager and ADC*/
void adc_manager_init(adc_channel_mask_t enabled_channels);
/* Read fresh data from ADC */
void adc_manager_read();
/* Returns current sample for given ADC channel.  Returns ADC_MANAGER_SAMPLE_INVALID if error */
adc_sample_t adc_manager_get_sample(adc_channel_t channel);
/* Returns current sample in microvolts for given ADC channel.  Returns 0 if error */
u_volts_t    adc_manager_get_sample_uv(adc_channel_t channel);


#endif /* __ADC_MANAGER_HPP__ */