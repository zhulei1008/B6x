#ifndef _CHIPSET_H_
#define _CHIPSET_H_

#include <stdint.h>
#include "ptdefs.h"



extern chip_t gChip;

void chipInit(void);
void otaEnd(void);
#endif //_CHIPSET_H_
