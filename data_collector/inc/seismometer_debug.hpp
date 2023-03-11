#ifndef __SEISMOMETER_DEBUG_HPP__
#define __SEISMOMETER_DEBUG_HPP__

#include <stdio.h>

typedef enum
{
  SEISMOMETER_LOG_INVALID,
  SEISMOMETER_LOG_ERROR,
  SEISMOMETER_LOG_WARNING,
  SEISMOMETER_LOG_INFO,
  SEISMOMETER_LOG_DEBUG,
  SEISMOMETER_LOG_MAX,

  SEISMOMETER_LOG_NONE=SEISMOMETER_LOG_INVALID,
} seismometer_debug_level_e;

#ifdef SEISMOMETER_DEBUG_BUILD
#define SEISMOMETER_LOG_LEVEL SEISMOMETER_LOG_DEBUG
#else
#define SEISMOMETER_LOG_LEVEL SEISMOMETER_LOG_INFO
#endif

#define SEISMOMETER_PRINTF(level,format,args...) \
  if(level <= SEISMOMETER_LOG_LEVEL) \
  {                                 \
    printf(format, ##args);         \
  }


#endif /*__SEISMOMETER_DEBUG_HPP__ */