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
}

static void devInit(void)
{    
    uint16_t rsn = rstrsn();
    
    iwdt_disable();
    
    dbgInit();
    debug("Start(rsn:0x%X)...\r\n", rsn);
    
    AON->PMU_WKUP_CTRL.IO_LATCH_N     = 1;
//    AON->PMU_WKUP_CTRL.BLE_LATCH_N    = 1;
    
}

#define WKUP_IO_MASK 0x80
int main(void)
{
    sysInit();
    devInit();

    while(1)
    {
        uint8_t cmd = uart_getc(0);
        debug("cmd:%x, pmu_wkup_ctrl:%x, %d\r\n", cmd, AON->PMU_WKUP_CTRL.Word, AON->PMU_CTRL.POWEROFF_LOG);
        switch (cmd)
        {
            case 0x66:
            {
                pwroff_io_sw(WKUP_IO_MASK, WKUP_IO_MASK);
                core_pwroff(WKUP_PWIO_EN_BIT/* | WKUP_IO_LATCH_N_BIT*/);
            } break;
            
            case 0x88:
            {
                pwroff_io_sw(WKUP_IO_MASK, 0);
                core_pwroff(WKUP_PWIO_EN_BIT/* | WKUP_IO_LATCH_N_BIT*/);
            } break;
            
            default:
            {
            } break;
        };
    }
}
