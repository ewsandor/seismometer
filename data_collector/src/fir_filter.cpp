#include <cassert>
#include <cstdlib>

#include "fir_filter.hpp"
#include "seismometer_utils.hpp"

#define INCREMENT_CIRCULAR_BUFFER_ITERATOR(iterator, buffer_size) \
  ((((iterator)+1) < (buffer_size))?((iterator)+1):(0)) /* If iterator exceeds buffer size, reset to 0 */
#define CIRCULAR_BUFFER_OFFSET_NEG(index, buffer_size, offset) \
  (((offset) <= (index))?((index)-(offset)):((index)+(buffer_size)-(offset)))
#define CIRCULAR_BUFFER_OFFSET_POS(index, buffer_size, offset) \
  ((((index)+(offset)) < (buffer_size))?((index)+(offset)):((index)+(offset)-(buffer_size)))

const fir_filter_config_s default_fir_filter_config
{
  .moving_average_order = 0,
  .gain_numerator   = 1,
  .gain_denominator = 1,
};

fir_filter_c::fir_filter_c( filter_order_t order_init, const filter_coefficient_t *coefficient_init, const fir_filter_config_s *config_init)
  : order(order_init), coefficient(coefficient_init), config(*config_init), 
    circular_buffer_size(SEISMOMETER_MAX(order_init, config_init->moving_average_order))
{
  assert(config_init != nullptr);
  assert(coefficient != nullptr);
  assert(order > 0);
  assert(config.gain_denominator != 0);
  assert(circular_buffer_size > 0);
  circular_buffer = (filter_sample_t*) calloc(sizeof(filter_sample_t), circular_buffer_size);
  assert(circular_buffer != nullptr);
}
fir_filter_c::~fir_filter_c()
{
  free(circular_buffer);
}

void fir_filter_c::push_sample(filter_sample_t sample)
{
  if(config.moving_average_order > 0)
  {
    moving_average_sum -= circular_buffer[CIRCULAR_BUFFER_OFFSET_NEG(next_write, circular_buffer_size, config.moving_average_order)];
    moving_average_sum += sample;
    if(moving_average_history < config.moving_average_order)
    {
      moving_average_history++;
    }
  }
  circular_buffer[next_write]=sample;
  next_write = INCREMENT_CIRCULAR_BUFFER_ITERATOR(next_write, circular_buffer_size);
 
  filter_order_t i = 0;
  filtered_sample = 0;
  filter_order_t iterator = CIRCULAR_BUFFER_OFFSET_NEG(next_write, circular_buffer_size, order-1);
  for(i = 0; i < order; i++)
  {
    filtered_sample += coefficient[i]*circular_buffer[iterator];
    iterator         = INCREMENT_CIRCULAR_BUFFER_ITERATOR(iterator, circular_buffer_size);
  }
  filtered_sample*=config.gain_numerator;
  filtered_sample/=config.gain_denominator;

  if(moving_average_history > 0)
  {
    moving_average   = (moving_average_sum/((filter_sample_t)moving_average_history));
  }
}