// hyIC, Analog Pad

#ifndef _DRAGON_ANALOG_PAD_H_
#define _DRAGON_ANALOG_PAD_H_

#include <stdint.h>

#define AT0_PAD       9  // PA09 Fixed, don't modify
#define AT1_PAD       10 // PA10 Fixed, don't modify

#define RC16M_CLK_OUT 4 // PA04 Fixed, don't modify
#define CLK_OUT_PAD   5 // PA05 Fixed, don't modify

enum clk_out_sel
{
    CLK_OUT_HSI  = 1,
    CLK_OUT_HSE,
    CLK_OUT_PLL,    
    CLK_OUT_LSI,
    CLK_OUT_SYS, // 5
    CLK_OUT_AHB ,
    CLK_OUT_APB1,
    CLK_OUT_APB2,
    CLK_OUT_FSHC,
    CLK_OUT_USB, // 10
};

void at0_at1_analog_func_enable(void);
void at1_hiz_set(void);
void at0_hiz_set(void);
void sys_clk_out(uint8_t clk_out);
void rc16m_clk_out(uint8_t freq_cco, uint8_t freq_cl, uint8_t tc, uint8_t fvch);
#endif  // _DRAGON_ANALOG_PAD_H_
