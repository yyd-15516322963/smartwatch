#ifndef __BLE_H
#define __BLE_H

#include "sys.h"
#include <stddef.h>  // 提供 NULL
#include <string.h>  // 提供 memset
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"
extern void ble_init(uint32_t baud);


#endif
