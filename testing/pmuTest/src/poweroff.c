#include "poweroff.h"
#include "drvs.h"
#include "regs.h"


static void ioState(void)
{
	for (uint16_t i = 0; i < 20; i++)
    {
        iom_ctrl(i, IOM_SEL_GPIO | IOM_PULLUP);
        //CSC->CSC_PIO[i].PUPDCTRL = 0;
        //ioInputCtrl(i, 1);
    }
}
void poweroffTest(void)
{ 
    RCC->APBCLK_EN_RUN.AON_CLKEN_RUN = 1;

    uint32_t poweroff_flag = AON->PMU_CTRL.POWEROFF_LOG;	

    // em power close
//    AON->PMU_CTRL.EM_ACCESS_EN = 0;
//    AON->PMU_CTRL.EM_PD_EN = 1;
////    
//    AON->PMU_CTRL.AUTO_DIS_RC32K = 1;
    
    ioState();
    
    //rtcDisable();
    APBMISC->POWEROFF_LOG_CLR = 1;
    APBMISC->AON_PMU_CTRL.WKUP_ST_CLR = 1;


    AON->PMU_WKUP_CTRL.IO_LATCH_N = 0;			

    // poweroff cmd
    APBMISC->POWEROFF_WORD_CMD = 0x77ED29B4;
    while(1);	
}
