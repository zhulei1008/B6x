#include "hyreg.h"

#include "poweroff.h"
#include "rcc.h"
#include "gpio.h"
#include "csc.h"
#include "rtc.h"
#include "btmr.h"
#include "flash.h"

void pwo_clkAutoSet(uint8_t rc32k_ctrl, uint16_t clk_stb_time)
{
    AON->PMU_CTRL.AUTO_DIS_RC32K = rc32k_ctrl;    
    // 2.unused clk disable
    AON->PMU_CTRL.AUTO_DIS_RC16M = 1;
    AON->PMU_CTRL.AUTO_DIS_XOSC16M = 1;
    // aotu enable rc16m and xo16m when chip exit deepsleep state
    AON->PMU_CTRL.AUTO_EN_RC16M = 1;
    AON->PMU_CTRL.AUTO_EN_XOSC16M = 1;
    // wait 2ms, wait rc16m and xo16m stable
    AON->PMU_CTRL.CLK_STB_TIME = clk_stb_time;    
}

void poweroffTest(void)
{ 
    RCC->APBCLK_EN_RUN.AON_CLKEN_RUN = 1;
    
    uint32_t poweroff_flag = AON->PWOFF_CTRL.POWEROFF_LOG;	

	if (!poweroff_flag)
	{
		pwo_clkAutoSet(0, 0x1f);
		
		// set wkup source 
		#ifdef RTC_WKUP
			//rtcWakeupInit(0, 10, 5, 10);
		#endif
		#ifdef BLE_WKUP
			oscWakeupSet(0x10000, 0x1000, 0x100);
		#endif		
		#ifdef IO_WKUP
			pwo_ioWakeupInit(6, FALLING_EDGE);	
            pwo_ioWakeupInit(7, FALLING_EDGE);	
            pwo_ioWakeupInit(12, FALLING_EDGE);	
            pwo_ioWakeupInit(13, FALLING_EDGE);	
		#endif		
        // em power close
//        AON->PWOFF_CTRL.EM_ACCESS_EN = 0;
//        AON->PWOFF_CTRL.EM_PD_EN = 1;
        
        //rtcDisable();
        AON->PWOFF_CTRL.POWEROFF_LOG_CLR = 1;
        AON->PMU_WKUP_CTRL.IOA06_WKUP_ST_CLR = 1;
        AON->PMU_WKUP_CTRL.IOA07_WKUP_ST_CLR = 1;
        AON->PMU_WKUP_CTRL.IOA12_WKUP_ST_CLR = 1;
        AON->PMU_WKUP_CTRL.IOA13_WKUP_ST_CLR = 1;        
        AON->PMU_WKUP_CTRL.IO_LATCH_N = 0;			
//		flashDeInit();
//		flashPadStatus(IO_PDEN);
		// close dpll power
		AON->PMU_ANA_CTRL.ANA_RESV = 0x04;
        //AON->BKHOLD_CTRL.BK_LDOBK_TRIM = 0x04;
		// close flash power
//		SYSCFG->FSH_PW_EN = 0;
//		// sadc ana en disenable
//		SADC->SADC_ANA_CTRL.SADC_EN = 0;
		// poweroff cmd
		AON->PWOFF_CTRL.POWEROFF_CMD = 1;
        while(1);	
	}
    
    AON->PWOFF_CTRL.POWEROFF_LOG_CLR = 1;
    
	// wkup status judge
	if (AON->PMU_WKUP_ST.RTC_WKUP_ST)
	{
        // clear poweroff wakeup status
        AON->PMU_WKUP_CTRL.RTC_WKUP_ST_CLR = 1;		
		//ioTog(14);
	}
    else if (AON->PMU_WKUP_ST.BLE_WKUP_ST)
    {
        AON->PMU_WKUP_CTRL.BLE_WKUP_ST_CLR = 1;
    }    
    else if (AON->PMU_WKUP_ST.IOA06_WKUP_ST ||  AON->PMU_WKUP_ST.IOA07_WKUP_ST || AON->PMU_WKUP_ST.IOA12_WKUP_ST || AON->PMU_WKUP_ST.IOA13_WKUP_ST)
    {
        AON->PMU_WKUP_CTRL.IOA06_WKUP_ST_CLR = 1;
        AON->PMU_WKUP_CTRL.IOA07_WKUP_ST_CLR = 1;
        AON->PMU_WKUP_CTRL.IOA12_WKUP_ST_CLR = 1;
        AON->PMU_WKUP_CTRL.IOA13_WKUP_ST_CLR = 1;
    }

}
