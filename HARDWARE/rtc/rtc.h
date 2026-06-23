#ifndef _RTC_H_
#define _RTC_H_

#include "sys.h"

void rtc_init(void);
void set_time(uint8_t hour,uint8_t minutes,uint8_t second);
void set_date(uint8_t year,uint8_t month,uint8_t date,uint8_t weekday);

#endif
