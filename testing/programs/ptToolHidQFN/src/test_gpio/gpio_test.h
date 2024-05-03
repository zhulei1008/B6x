#ifndef _GPIO_TEST_H_
#define _GPIO_TEST_H_

#include <stdint.h>

#define IO_MAX_CNT     20

enum IO_TEST
{
    IO_TEST_PASS     = 0x00,

    IO_TEST_PULLUP   = (1 << 0),
    IO_TEST_PULLDOWN = (1 << 1),
    IO_TEST_SHORTGND = (1 << 2),
    IO_TEST_SHORTVDD = (1 << 3),
    IO_TEST_NEAR     = (1 << 4),
};

enum SHORT_TYPE
{
    SHORT_VDD = 0,
    SHORT_GND = 1,
};

enum PULL_TYPE
{
    PULL_DOWN = 0,
    PULL_UP   = 1,
};

uint8_t gpio_test(uint32_t *masks, uint8_t modes);

#endif // _GPIO_TEST_H_
