#ifndef UART2_BUFF_H_
#define UART2_BUFF_H_

#include <stdint.h>
#include "string.h"

uint16_t uart2_RxCount(void);

uint16_t uart2_RxRead(uint8_t *buff, uint16_t max);

#endif // UART2_BUFF_H_
