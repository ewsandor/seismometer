#include <cassert>
#include <cstdio>

#include <pico/stdlib.h>
#include <pico/sync.h>

#include "sampler.hpp"

sample_thread_args_s * args_ptr = nullptr;

void sampler_thread_pass_args(sample_thread_args_s *args)
{
  assert(nullptr == args_ptr);
  args_ptr = args;
}

bool sample_timer_callback(repeating_timer_t *rt)
{
  assert(sem_release((semaphore_t*)rt->user_data));
  return true; /*true to continue repeating, false to stop.*/
}

repeating_timer_t sample_timer     = {0};
semaphore_t       sample_semaphore = {0}; 

void sampler_thread_main()
{
  printf("Initializing sample semaphore.\n");
  sem_init(&sample_semaphore, 0, 1);

  printf("Starting sample timer.\n");
  assert(add_repeating_timer_us(-SEISOMETER_SAMPLE_PERIOD_US, sample_timer_callback, &sample_semaphore, &sample_timer));

  while (1)
  {
    sem_acquire_blocking(&sample_semaphore);
    printf("Sample!\n");
  }
}