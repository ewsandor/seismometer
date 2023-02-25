#include <cassert>
#include <cstdio>

#include <hardware/adc.h>
#include <pico/binary_info.h>

#include "adc_manager.hpp"

void adc_manager_init(adc_channel_mask_t enabled_channels)
{
  printf("Initializing ADC manager.");

  adc_init();

  adc_channel_mask_t enabled_channels_copy = enabled_channels;
  adc_channel_t      adc_channel = 0;

  bi_decl(bi_1pin_with_name(ADC_CH_TO_PIN(ADC_CH_PENDULUM_10X),  "Pendulum ADC with  10x Hardware Gain"));
  bi_decl(bi_1pin_with_name(ADC_CH_TO_PIN(ADC_CH_PENDULUM_100X), "Pendulum ADC with 100x Hardware Gain"));

  while(enabled_channels_copy)
  {
    assert(adc_channel < ADC_CH_MAX);
    if(enabled_channels_copy & ADC_CH_TO_MASK(adc_channel))
    {
      adc_gpio_init(ADC_CH_TO_PIN(adc_channel));
    }
    enabled_channels_copy &= ~(ADC_CH_TO_MASK(adc_channel));
    adc_channel++;
  }
}