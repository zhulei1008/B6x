/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
 *
 ****************************************************************************************
 */

#include "drvs.h"
#include "regs.h"
#include "dbg.h"

/*
 * DEFINES
 ****************************************************************************************
 */

void sysInit(void)
{    
    #if (FPGA_TEST)
    RCC->BLE_CLKRST_CTRL.BLECLK_DIV    = 2; // BB Must 16M
    #else
    RCC->BLE_CLKRST_CTRL.BLECLK_DIV    = 0; // BB Must 16M
    #endif
    
    AON->BKHOLD_CTRL.CORELDO_TRIM_RUN = 0x1b;
    AON->BKHOLD_CTRL.AONLDO_TRIM_RUN = 0x0;
}

static void devInit(void)
{    
    uint16_t rsn = rstrsn();
    
    iwdt_disable();
    
    dbgInit();
    debug("Start(rsn:0x%X)...\r\n", rsn);
    
    AON->PMU_WKUP_CTRL.IO_LATCH_N     = 1;
//    AON->PMU_WKUP_CTRL.BLE_LATCH_N    = 1;
    
    // rc16m freq init
    //APBMISC->RC16M_FREQ_TRIM = 0;
    
}

void sadc_io_init(uint32_t io)
{
	GPIO_DIR_CLR(1 << io);
    
	iom_ctrl(io, IOM_ANALOG);	    
}


int main(void)
{
    uint16_t adc_data = 0;
    sysInit();
    
    devInit();

    debug("BKUP0 = %x\r\n", AON->BACKUP0);
    debug("BKUP1 = %x\r\n", AON->BACKUP1);
    debug("rc16m_trim = %d\r\n", APBMISC->RC16M_FREQ_TRIM);
    debug("rc32k_trim_msb = %d\r\n", AON->BKHOLD_CTRL.RC32K_MSB_TRIM_CFG);
    debug("rc32k_trim_lsb = %d\r\n", AON->BKHOLD_CTRL.RC32K_LSB_TRIM_CFG);
    debug("ROM_SIGNATURE_RPT = %x\r\n", SYSCFG->ROM_SIGNATURE_RPT);
    debug("BIST_STATUS = %x\r\n", SYSCFG->BIST_STATUS);
    
    
    //iospc_clkout(CLK_OUT_SYSCLK);
    
    sadc_init();
    sadc_conf(SADC_CR_DFLT);
    adc_data = sadc_read(SADC_CH_VDD12, 10);
    debug("adc_data = %d\r\n", adc_data);
    
    while(1)
    {
        uint8_t cmd = uart_getc(0);
        //debug("cmd:%x, pmu_wkup_ctrl:%x, %d\r\n", cmd, AON->PMU_WKUP_CTRL.Word, AON->PMU_CTRL.POWEROFF_LOG);
        switch (cmd)
        {
            case 0x66:
               
            break;
            
            case 0x11:
              
            break;
 
            case 0x12:
               
            break;
            
            case 0x13:
                
            break;         
            
           case 0x14:
                
            break;        
           
            default:
                
            break;
        }
    }
}
