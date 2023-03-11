#include <cassert>
#include <cstdio>

#include <hardware/adc.h>
#include <pico/binary_info.h>

#include "adc_manager.hpp"
#include "seismometer_debug.hpp"
#include "seismometer_utils.hpp"

static adc_channel_mask_t enabled_channels = 0;
static adc_sample_t current_sample[ADC_CH_MAX] = {0};

void adc_manager_init(adc_channel_mask_t enabled_channels_init)
{
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Initializing ADC manager.\n");

  enabled_channels = enabled_channels_init;

  adc_init();
  bi_decl(bi_1pin_with_name(ADC_CH_TO_PIN(ADC_CH_PENDULUM_10X),  "Pendulum ADC with  10x Hardware Gain"));
  bi_decl(bi_1pin_with_name(ADC_CH_TO_PIN(ADC_CH_PENDULUM_100X), "Pendulum ADC with 100x Hardware Gain"));

  adc_channel_t      adc_channel = 0;
  adc_channel_mask_t enabled_channels_copy = enabled_channels;
  while(enabled_channels_copy)
  {
    SEISMOMETER_ASSERT(adc_channel < ADC_CH_MAX);
    if(enabled_channels_copy & ADC_CH_TO_MASK(adc_channel))
    {
      adc_gpio_init(ADC_CH_TO_PIN(adc_channel));
    }
    enabled_channels_copy &= ~(ADC_CH_TO_MASK(adc_channel));
    adc_channel++;
  }
}

void adc_manager_read()
{
  adc_channel_t      adc_channel = 0;
  adc_channel_mask_t enabled_channels_copy = enabled_channels;
  while(enabled_channels_copy)
  {
    SEISMOMETER_ASSERT(adc_channel < ADC_CH_MAX);
    if(enabled_channels_copy & ADC_CH_TO_MASK(adc_channel))
    {
      adc_select_input(adc_channel);
      current_sample[adc_channel] = adc_read();
    }
    enabled_channels_copy &= ~(ADC_CH_TO_MASK(adc_channel));
    adc_channel++;
  }
}

adc_sample_t adc_manager_get_sample(adc_channel_t channel)
{
  adc_sample_t ret_val = ADC_MANAGER_SAMPLE_INVALID;

  SEISMOMETER_ASSERT(channel < ADC_CH_MAX);

  if(enabled_channels & ADC_CH_TO_MASK(channel))
  {
    ret_val = current_sample[channel];
  }

  return ret_val;
}

m_volts_t adc_manager_get_sample_mv(adc_channel_t channel)
{
  m_volts_t    ret_val = 0;
  adc_sample_t raw_sample = adc_manager_get_sample(channel);
 
  if(raw_sample <= ADC_MANAGER_SAMPLE_MAX_VALUE)
  {
    ret_val = (raw_sample*ADC_REFERENCE_VOLTAGE_MV)/ADC_MANAGER_SAMPLE_MAX_VALUE;
  }

  return ret_val;
}