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

void projectCodeInfo(void)
{
    extern uint32_t Load$$ER_IROM1$$Base;
    extern uint32_t Image$$ER_IROM1$$Base;
    extern uint32_t Load$$ER_IROM1$$Length;
    
    uint32_t *load_base  = (uint32_t*)&Load$$ER_IROM1$$Base;
    uint32_t *image_base = (uint32_t*)&Image$$ER_IROM1$$Base;
    uint32_t *load_len   = (uint32_t*)&Load$$ER_IROM1$$Length;
    uint32_t reg_val = 0;

    debug("flash_info 0x0000:%X, %X, %X, %X\r\n", RD_32(0x18000000), RD_32(0x18000004), RD_32(0x18000008), RD_32(0x1800000C));
    debug("flash_info 0x1000:%X, %X, %X, %X\r\n", RD_32(0x18001000), RD_32(0x18001004), RD_32(0x18001008), RD_32(0x1800100C));
    #if (EM_CODE)
    // (codeAddr-0x20008000) / 0x200)
    uint8_t em_info = ((uint32_t)image_base - 0x20008000) >> 9;
    reg_val = AON->BACKUP0;
    reg_val &= ~0x1F;
    reg_val |= (0x10UL | em_info);
    AON->BACKUP0 = reg_val;
    debug("em_info:0x%02X\r\n", em_info);
    #else
    if ((AON->BACKUP0 & 0x1FUL) != 0)
        AON->BACKUP0 &= ~0x11FUL;
    #endif
    
    debug("BACKUP0:%x, SYS_BACKUP0:%08x\r\n", AON->BACKUP0, SYSCFG->SYS_BACKUP0);
//    AON->BACKUP0 |= (0x01UL << 8);
    
    #if ((SRAM_CODE) || (MAX_SRAM_CODE))
    // (codeAddr-0x20003000) / 0x400)
    uint8_t sram_info = ((uint32_t)image_base - 0x20003000) >> 10;
    reg_val = SYSCFG->SYS_BACKUP0;
    reg_val &= ~0x3F;
    reg_val |= (0x20 | sram_info);
    SYSCFG->SYS_BACKUP0 = reg_val;
    debug("sram_info:0x%02X\r\n", sram_info);
    #else
    if ((SYSCFG->SYS_BACKUP0 & 0x3FUL) != 0)
        SYSCFG->SYS_BACKUP0 &= ~0x3FUL;
    #endif
    
    if ((0xFFFFFFFF != RD_32(0x1800000C)) || (0xFFFFFFFF != RD_32(0x1800100C)))
    {
//        debug("Chip Erase\r\n");
        #if !((FLASH_XIP_CODE) || (CACHE_CODE))
        fshc_erase(0, FCM_ERCHIP_BIT|FSH_CMD_ER_CHIP);
        #endif

    }

    debug("LoadBase: %08X\r\n", (uint32_t)load_base);
    debug("ImageBase:%08X\r\n", (uint32_t)image_base);
    debug("LoadLen:  %X\r\n",   (uint32_t)load_len);
}

#if (MAX_SRAM_CODE)
#define BUFF_LEN 0x1AF8
const uint32_t max_sram_buff[BUFF_LEN] = {[0 ... (BUFF_LEN - 1)] = 0x37363534};
#endif
static void devInit(void)
{    
    uint16_t rsn = rstrsn();
    
    iwdt_disable();
    
    dbgInit();
    debug("Start(rsn:0x%X)...\r\n", rsn);
    
    debug("UART1->LCR.Word:%x\r\n", UART1->LCR.Word);
    
    projectCodeInfo();
    
    #if (MAX_SRAM_CODE)
    for (uint16_t i = 0; i < BUFF_LEN; ++i)
    {
        debug("data%d:%x\r\n", i, max_sram_buff[i]);
    }
    #endif
}

#define TOG_IO1     BIT(6)
#define TOG_IO2     BIT(7)
#define TOG_IO_MASK (TOG_IO1 | TOG_IO2)
int main(void)
{
    sysInit();
    devInit();
    
    iom_ctrl(6, IOM_DRV_LVL1 | IOM_PULLUP | IOM_SEL_GPIO);
    iom_ctrl(7, IOM_DRV_LVL1 | IOM_PULLUP | IOM_SEL_GPIO);
    
    GPIO->DAT_CLR = TOG_IO1;
    GPIO->DAT_SET = TOG_IO2;
    GPIO->DIR_SET = TOG_IO_MASK;
    
    while(1)
    {
        GPIO->DAT_TOG =  TOG_IO2;//TOG_IO_MASK;
    }
}
