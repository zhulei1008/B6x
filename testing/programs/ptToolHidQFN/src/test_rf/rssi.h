#ifndef RSSI_H_
#define RSSI_H_

#include <stdint.h>

void rssi_sadc_init(void);
uint8_t rssi_get(void);

#endif // RSSI_H_
