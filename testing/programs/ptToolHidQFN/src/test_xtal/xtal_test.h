#ifndef _XTAL_TEST_H_
#define _XTAL_TEST_H_

#include <stdint.h>
// 62.5ms, arr = 59999
// tim_cnt_std = 62.5/(1000/16M) = 1000000
// tmr_arr = tim_cnt_std / (arr+1) = 1000000/60000 = 16 = 0x10
// tmr_cnt = tim_cnt_std - tmr_arr*(arr+1) = 40000 = 0x9C40
#define TIM_ARR_STD 0x10
#define TIM_CNT_STD 0x9C40

#define TIM_VAL_STD 0x109C40
#define TIM_VAL_MAX 0x109C42
#define TIM_VAL_MIN 0x109C3E



struct xtal_val
{
    uint16_t cnt;
    uint8_t  arr;
    uint8_t  trim;
};

void xtal_freq(uint8_t pad);

uint32_t xtal_calc(uint8_t pad);

#endif // _XTAL_TEST_H_
