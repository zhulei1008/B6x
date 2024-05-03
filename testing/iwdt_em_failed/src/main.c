/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "regs.h"
#include "drvs.h"

/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * FUNCTIONS
 ****************************************************************************************
 */

int main(void)
{
    uint16_t rsn = rstrsn();
    
    iospc_clkout(CLK_OUT_LSI);
    
    GPIO_DIR_SET(BIT(6) | BIT(7));
    GPIO_DAT_CLR(BIT(6) | BIT(7));

    rcc_sysclk_set(SYS_CLK_LSI);
    
    APBMISC->DPLL_CTRL.DPLL2_EN = 0;
//    AON->PMU_CTRL.OSC_EN_RUN    = 1; // EM_ACCESS_EN failed
    AON->PMU_CTRL.OSC_EN_RUN    = 0;  // EM_ACCESS_EN ok
    APBMISC->AON_PMU_CTRL.HSI_EN_RUN = 0;
    
    while (1)
    {
        if (AON->PMU_CTRL.EM_ACCESS_EN)
        {
            GPIO_DAT_TOG(BIT(6));
        }
        else
        {
            GPIO_DAT_TOG(BIT(7));
        }
    }
}
