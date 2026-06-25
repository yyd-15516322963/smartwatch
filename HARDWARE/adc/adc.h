#ifndef _ADC_H_
#define _ADC_H_

#include "sys.h"

void adc_init(void);
uint16_t adc_median_average_filter(void);

#endif
