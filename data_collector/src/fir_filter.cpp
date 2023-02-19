#include <cassert>
#include <cstdlib>

#include "fir_filter.hpp"

fir_filter_c::fir_filter_c( filter_order_t order_init, const filter_coefficient_t *coefficient_init, 
                            filter_sample_t gain_numerator_init, filter_sample_t gain_denominator_init )
  : order(order_init), coefficient(coefficient_init), gain_numerator(gain_numerator_init), gain_denominator(gain_denominator_init)
{
  assert(coefficient != nullptr);
  assert(order > 0);
  assert(gain_denominator != 0);
  circular_buffer = (filter_sample_t*) calloc(sizeof(filter_sample_t), order);
  assert(circular_buffer != nullptr);
}
fir_filter_c::~fir_filter_c()
{
  free(circular_buffer);
}

void fir_filter_c::push_sample(filter_sample_t sample)
{
  circular_buffer[next_write]=sample;
  next_write = (next_write+1)%order;

  filter_order_t i = 0;
  filtered_sample = 0;
  for(i = 0; i < order; i++)
  {
    filtered_sample+=coefficient[i]*circular_buffer[(next_write+i)%order];
  }
  filtered_sample*=gain_numerator;
  filtered_sample/=gain_denominator;
}