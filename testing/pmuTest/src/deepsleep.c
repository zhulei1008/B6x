
#include "deepsleep.h"
#include "drvs.h"
#include "regs.h"


void AON_PMU_IRQHandler(void)
{
    // wait mcu_deepsleep sync
    //bootDelayUs(200);
    
    APBMISC->AON_PMU_CTRL.AON_PMU_INT_CLR = 1;
    APBMISC->AON_PMU_CTRL.WKUP_ST_CLR = 1;
       
}


void ioState(void)
{
	for (uint16_t i = 0; i < 20; i++)
    {
        iom_ctrl(i, IOM_SEL_GPIO | IOM_PULLDOWN);
        //ioInputCtrl(i, 1);
    }
}



void deepsleepTest(void)
{
    RCC->BLE_CLKRST_CTRL.BLE_AHBEN = 1;
    RCC->APBCLK_EN_RUN.AON_CLKEN_RUN = 1;
    RCC->APBCLK_EN_RUN.APBMISC_CLKEN_RUN = 1;
    RCC->AHBCLK_EN_RUN.SYSCFG_CLKEN_RUN = 1;
    RCC->APBCLK_EN_RUN.RF_CLKEN_RUN = 1;
   
	__enable_irq();
    NVIC_EnableIRQ(AON_PMU_IRQn);		
    //NVIC_EnableIRQ(RTC_IRQn);		

    
	// set wkup source 
	#ifdef RTC_WKUP
		//rtcWakeupInit(0, 10, 5, 10);
	#endif
	#ifdef BLE_WKUP
        //oscWakeupSet(0x10000, 0x1000, 0x100);
	#endif		
	#ifdef IO_WKUP
    //for (uint8_t i = 0; i < 8; i++)
		//dp_ioWakeupInit(4, FALLING_EDGE);	
	#endif		
    //debug("wkup st1:0x%x\r\n", AON->PMU_WKUP_ST.Word);
#if 1	
    RF->ANA_EN_CTRL.EN_BG = 0;
    RF->ANA_PWR_CTRL.CLK_EN_PLL = 0;
    
	// sadc ana en disenable
	SADC->SADC_ANA_CTRL.SADC_EN = 0;
	// rf power disable
    ioState();    
    SYSCFG->USB_CTRL.DIG_USB_RXEN = 0;
    //AON->BKHOLD_CTRL.BK_LDO12_TRIM_CFG = 0x10;     
	// adjust core1.2v volrage
	//AON->BKHOLD_CTRL.BK_LDO12_TRIM_CFG = 0;
	// voltage ud
//	APBMISC->LDO_UD_CTRL.LDO_UD_TRIM_CFG = 0x1d;
//	APBMISC->LDO_UD_CTRL.LDO_UD_EN = 1;
#endif       
        
        SCB->SCR |= 1 << 2 ;
        __WFI ();
       
        
    while (1)
    {         
	}

}

