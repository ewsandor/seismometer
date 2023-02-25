#ifndef __ADC_MANAGER_HPP__
#define __ADC_MANAGER_HPP__

#define ADC_CH_TO_PIN(channel)  (26+channel)
#define ADC_CH_TO_MASK(channel) (1 << channel)

typedef enum
{
  ADC_CH_0              = 0,
  ADC_CH_PENDULUM_10X   = 0,
  ADC_CH_1              = 1,
  ADC_CH_PENDULUM_100X  = 1,
  ADC_CH_2              = 2,
  ADC_CH_3              = 3,
  ADC_CH_4              = 4,
  ADC_CH_IC_TEMPERATURE = 4,
  /* Max channel value */
  ADC_CH_MAX            = 5
} adc_channel_assignment_e;
typedef unsigned int adc_channel_t;
typedef unsigned int adc_channel_mask_t;

void adc_manager_init(adc_channel_mask_t enabled_channels);
void adc_manager_init(adc_channel_mask_t enabled_channels);

#endif /* __ADC_MANAGER_HPP__ */