#ifndef __SAMPLER_HPP__
#define __SAMPLER_HPP__

#include <pico/sem.h>
#include <pico/util/queue.h>

#include "seismometer_config.hpp"

typedef struct
{
  semaphore_t *boot_semaphore;
  queue_t     *sample_queue;
} sample_thread_args_s;

void sampler_thread_pass_args(sample_thread_args_s *args);
void sampler_thread_main();

#endif /*__SAMPLER_HPP__*/