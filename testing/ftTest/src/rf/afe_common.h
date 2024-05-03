#ifndef _AFE_COMMON_H_
#define _AFE_COMMON_H_

#include <stdbool.h>
#include "string.h"

#include "regs.h"

#define THREE_BITS_MAX 0x07
#define FOUR_BITS_MAX  0x0F
#define FIVE_BITS_MAX  0x1F

#define GET_UINT32_BYTE0(x)      (((x) >> 0)  & 0xFF)
#define GET_UINT32_BYTE1(x)      (((x) >> 8)  & 0xFF)
#define GET_UINT32_BYTE2(x)      (((x) >> 16) & 0xFF)
#define GET_UINT32_BYTE3(x)      (((x) >> 24) & 0xFF)

#endif  // _AFE_COMMON_H_
