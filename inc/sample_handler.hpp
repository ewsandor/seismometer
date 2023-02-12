#ifndef __SAMPLE_HANDLER_HPP__
#define __SAMPLE_HANDLER_HPP__

#include "seismometer_types.hpp"

void set_sample_handler_epoch(absolute_time_t time);
void sample_handler          (const seismometer_sample_s *sample);

#endif /*__SAMPLE_HANDLER_HPP__*/