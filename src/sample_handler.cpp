#include <cassert>
#include <cmath>
#include <cstdio>

#include "sample_handler.hpp"

static absolute_time_t epoch = {0};

void set_sample_handler_epoch(const absolute_time_t *time)
{
  assert(time != nullptr);
  epoch = *time;
}

static void acceleration_sample_handler(const seismometer_sample_s *sample)
{
  assert(sample != nullptr);
  assert(SEISMOMETER_SAMPLE_TYPE_ACCELERATION == sample->type);

  mm_ps2_t acceleration_magnitude = sqrt( (sample->acceleration.x*sample->acceleration.x) + 
                                          (sample->acceleration.y*sample->acceleration.y) + 
                                          (sample->acceleration.z*sample->acceleration.z) );

  printf("i: % 6u hz: %u - X: %f Y: %f Z: %f %M: %f\n", 
    sample->index, ((sample->index*1000)/(to_ms_since_boot(sample->time)-to_ms_since_boot(epoch))),
    ((double)sample->acceleration.x)/1000,
    ((double)sample->acceleration.y)/1000,
    ((double)sample->acceleration.z)/1000,
    ((double)acceleration_magnitude)/1000);
}

static void accelerometer_teperature_sample_handler(const seismometer_sample_s *sample)
{
  assert(sample != nullptr);
  assert(SEISMOMETER_SAMPLE_TYPE_ACCELEROMETER_TEMPERATURE == sample->type);
    printf("i: % 6u hz: %u - : %f\n", 
      sample->index, ((sample->index*1000)/(to_ms_since_boot(sample->time)-to_ms_since_boot(epoch))),
      ((double)sample->temperature)/1000);
}

void sample_handler(const seismometer_sample_s *sample)
{
  assert(sample != nullptr);

  switch(sample->type)
  {
    case SEISMOMETER_SAMPLE_TYPE_ACCELERATION:
    {
      acceleration_sample_handler(sample);
      break;
    }
    case SEISMOMETER_SAMPLE_TYPE_ACCELEROMETER_TEMPERATURE:
    {
      accelerometer_teperature_sample_handler(sample);
      break;
    }
    default:
    {
      printf("Unsupported sample type %u!", sample->type);
      assert(0);
      break;
    }
  }
}