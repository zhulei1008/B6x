#include "b6x.h"
#include "regs.h"
#include "drvs.h"
#include "cfg.h"
#include "ft_clock.h"
#include "ft_test.h"



#define PAD_CLKOUT 5

void clk_out(uint8_t clk_sel)
{
	RCC->CFG.MCO_SW = clk_sel;
	iom_ctrl(PAD_CLKOUT, IOM_SEL_SPECL | IOM_DRV_LVL1);
}


