#ifndef __FIR_FILTER_HPP__
#define __FIR_FILTER_HPP__

#include <cassert>
#include <cstddef>

typedef int          filter_coefficient_t;
typedef int          filter_sample_t;
typedef size_t       filter_order_t;

typedef struct
{
  filter_order_t  moving_average_order; /* Order to remove DC offset via moving average.  Order 0 disables this logic */
  filter_sample_t gain_numerator;       /* Constant gain to multiply filter output by */
  filter_sample_t gain_denominator;     /* Constant gain to divide filter output by */
} fir_filter_config_s;
extern const fir_filter_config_s default_fir_filter_config;

class fir_filter_c
{
  private:
    /* Filter Configuration */
    const fir_filter_config_s         config;
    const filter_order_t              order;
    const filter_coefficient_t *const coefficient;

    /* Circular Buffer Data */
    const filter_order_t        circular_buffer_size;
    filter_sample_t            *circular_buffer = nullptr;
    unsigned int                next_write      = 0;

    /* Sampling data */
    filter_order_t              moving_average_history = 0;
    filter_sample_t             moving_average_sum     = 0;
    filter_sample_t             moving_average         = 0;
    filter_sample_t             filtered_sample        = 0;

  public:
    fir_filter_c( filter_order_t order, const filter_coefficient_t *coefficient, const fir_filter_config_s *config = &default_fir_filter_config);
    ~fir_filter_c();

    /* Push incoming raw sample and compute new filtered sample */
    void                   push_sample(filter_sample_t sample);
    /* Returns current filtered sample */
    inline filter_sample_t get_filtered_sample()                   const {return filtered_sample;};
    /* Returns current filtered sample with DC offset removed via moving average */
    inline filter_sample_t get_filtered_sample_dc_offset_removed() const {return (filtered_sample-moving_average);}; 
    /* Returns current moving average or 0 if moving average is not enabled */
    inline filter_sample_t get_moving_average()                    const {return moving_average;}; 

};

#endif /*__FIR_FILTER_HPP__*/