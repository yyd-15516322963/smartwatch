#ifndef _MHT11_H_
#define _MHT11_H_

#include "sys.h"

#define DQ_IN PGin(9)
#define DQ_OUT PGout(9)

void dht11_set_io(GPIOMode_TypeDef mode);
u8 DHT11_Check(void);
u8 dht11_read_bit(void);
u8 dht11_read_byte(void);
u8 dht11_read_data(uint32_t *temp,uint32_t *humi);

#endif
