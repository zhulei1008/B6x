#include "b6x.h"
#include "regs.h"
#include "drvs.h"
#include "cfg.h"
#include "ft_test.h"
#include "ft_bist.h"

#define ROM_BIST_VALUE 0xD8C83A76
#define PAD_BIST_DONE 10
#define PAD_BIST_FAIL 11


void ft_memory_bist_test(void)
{
	SYSCFG->ROM_SIGNATURE_TARGET = ROM_BIST_VALUE; // 6vp replace
    // dpll clk enable
    //APBMISC->DPLL_CTRL.DPLL2_EN = 1;
    APBMISC->DPLL_CTRL.DPLL2_EN = 1;
    RCC->AHBCLK_EN_RUN.ADC_CLKEN_RUN = 1;
    RCC->APBCLK_EN_RUN.MDM_CLKEN_RUN = 1;
    RCC->CLK_EN_ST.BIST_CLK_EN = 1;
    
    // reset csc function
    RCC->AHBRST_CTRL.CSC_RSTREQ = 1;
    RCC->AHBRST_CTRL.CSC_RSTREQ = 0;

    // csc output with pulldowm
    csc_output(PAD_BIST_DONE, CSC_GLB_BIST_DONE); // First! 6vp 1117
    csc_output(PAD_BIST_FAIL, CSC_GLB_BIST_FAIL);
    iom_ctrl(PAD_BIST_DONE, IOM_SEL_CSC | IOM_PULLDOWN);
    iom_ctrl(PAD_BIST_FAIL, IOM_SEL_CSC | IOM_PULLDOWN);
    
    // sysclk switch to dpll64m
    //while(!APBMISC->ANAMISC_ST.DPLL2_LOCK_READY);
    RCC->CFG.SYSCLK_SW = 4;       
    SYSCFG->BIST_MODE = 1;
}



