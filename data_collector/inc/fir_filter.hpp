#ifndef __FIR_FILTER_HPP__
#define __FIR_FILTER_HPP__

typedef int          filter_coefficient_t;
typedef int          filter_sample_t;
typedef unsigned int filter_order_t;

class fir_filter_c
{
  private:
    const filter_order_t        order;
    const filter_coefficient_t *coefficient;
    filter_sample_t            *circular_buffer = nullptr;
    unsigned int                next_write=0;
    filter_sample_t             filtered_sample=0;

  public:
    fir_filter_c(filter_order_t order, const filter_coefficient_t *coefficient);
    ~fir_filter_c();

    void                   push_sample(filter_sample_t sample);
    inline filter_sample_t get_filtered_sample() {return filtered_sample;};

};

#endif /*__FIR_FILTER_HPP__*/