#ifndef __SAMPLER_HPP__
#define __SAMPLER_HPP__

#include "seismometer_config.hpp"

typedef struct
{
  int foo;
} sample_thread_args_s;

void sampler_thread_pass_args(sample_thread_args_s *args);
void sampler_thread_main();

#endif /*__SAMPLER_HPP__*/