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
#include "uartRb.h"
#include "flash_test.h"

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

void projectCodeInfo(void)
{
    extern uint32_t Load$$ER_IROM1$$Base;
    extern uint32_t Image$$ER_IROM1$$Base;
    extern uint32_t Load$$ER_IROM1$$Length;
    
    uint32_t *load_base  = (uint32_t*)&Load$$ER_IROM1$$Base;
    uint32_t *image_base = (uint32_t*)&Image$$ER_IROM1$$Base;
    uint32_t *load_len   = (uint32_t*)&Load$$ER_IROM1$$Length;

    debug("flash_info 0x0000:%X, %X, %X, %X\r\n", RD_32(0x18000000), RD_32(0x18000004), RD_32(0x18000008), RD_32(0x1800000C));
    debug("flash_info 0x1000:%X, %X, %X, %X\r\n", RD_32(0x18001000), RD_32(0x18001004), RD_32(0x18001008), RD_32(0x1800100C));

    debug("BACKUP0:%x, SYS_BACKUP0:%08x\r\n", AON->BACKUP0, SYSCFG->SYS_BACKUP0);
    
    if ((AON->BACKUP0 & 0x1FUL) != 0)
        AON->BACKUP0 &= ~0x11FUL;

    if ((SYSCFG->SYS_BACKUP0 & 0x3FUL) != 0)
        SYSCFG->SYS_BACKUP0 &= ~0x3FUL;

    debug("LoadBase: %08x\r\n", (uint32_t)load_base);
    debug("ImageBase:%08x\r\n", (uint32_t)image_base);
    debug("LoadLen:  %x\r\n",   (uint32_t)load_len);
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();
    
    #if (CACHE_ENABLE)
    fshc_cache_conf(0x18008000);
    #endif
    
    iwdt_disable();
    
//    dbgInit();
    uart1Rb_Init();
    debug("Start(rsn:0x%X, flashID:0x%06x)...\r\n", rsn, flashReadId());

    projectCodeInfo();
}

int main(void)
{
    sysInit();
    devInit();
    
    GPIO->DAT_CLR = 0x0C;
    GPIO->DIR_SET = 0x0C;
    
    GLOBAL_INT_START();
    
    while(1)
    {
        GPIO->DAT_TOG = 0x08;
        uart_proc();
    }
}
