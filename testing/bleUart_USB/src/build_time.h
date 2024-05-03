#ifndef LIB_BUILD_TIME_H_
#define LIB_BUILD_TIME_H_

#include <stdint.h>

struct build_date
{
    uint8_t date[12];
};

struct build_date ble_build_time(void);
struct build_date drv_build_time(void);

#endif // LIB_BUILD_TIME_H_
