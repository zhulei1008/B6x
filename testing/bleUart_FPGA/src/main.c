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
#include "bledef.h"
#include "sftmr.h"
#include "leds.h"
#include "dbg.h"
#include "fpga_spi_rf.h"
#include "sysdbg.h"
#include "build_time.h"


/*
 * DEFINES
 ****************************************************************************************
 */

extern uint32_t g_rx_int_cnt;

/*
 * FUNCTIONS
 ****************************************************************************************
 */

extern void user_procedure(void);

static void nopDly(uint32_t x)
{
    while (x--)
    {
        __NOP();__NOP();
        __NOP();__NOP();
        __NOP();__NOP();
        __NOP();__NOP();
        __NOP();__NOP();
        __NOP();__NOP();
    }
}

static void sysInit(void)
{
    rcc_ble_en();
    
    #if (FPGA_TEST)
    RCC->BLE_CLKRST_CTRL.BLECLK_DIV    = 2; // BB Must 16M
    #else
    RCC->BLE_CLKRST_CTRL.BLECLK_DIV    = 0; // BB Must 16M
    #endif
}

static void iopad_init(void)
{
    iocsc_observe(PAD_OSCEN_FLG,      CSC_OSCEN_FLAG);
    iocsc_observe(PAD_BLE_SLEEP_FLG,  CSC_BLE_SLEEP_FLAG);
    iocsc_observe(PAD_CORE_DEEPSLEEP, CSC_CM0P_DEEPSLEEP);
    iocsc_observe(PAD_BB_TX_EN,       CSC_BB_TX_EN);
//    iocsc_observe(PAD_BB_RX_EN,       CSC_BB_RX_EN);
    iospc_clkout(CLK_OUT_LSI);
    
    GPIO->DAT_CLR = BIT(PAD_TIM_PIO) | BIT(PAD_PWR_FLG) | BIT(PAD_BB_WKUP_END);
    GPIO->DIR_SET = BIT(PAD_TIM_PIO) | BIT(PAD_PWR_FLG) | BIT(PAD_BB_WKUP_END);
}

void projectCodeInfo(void)
{
    extern uint32_t Load$$ER_IROM1$$Base;
    extern uint32_t Image$$ER_IROM1$$Base;
    extern uint32_t Load$$ER_IROM1$$Length;
    
    uint32_t *load_base  = (uint32_t*)&Load$$ER_IROM1$$Base;
    uint32_t *image_base = (uint32_t*)&Image$$ER_IROM1$$Base;
    uint32_t *load_len   = (uint32_t*)&Load$$ER_IROM1$$Length;
    
    // Clear SRAM Code
    if ((SYSCFG->SYS_BACKUP0 & 0x3FUL) != 0)
        SYSCFG->SYS_BACKUP0 &= ~0x3FUL;
    
    // Clear EM Code
    if ((AON->BACKUP0 & 0x1FUL) != 0)
        AON->BACKUP0 &= ~0x1FUL;
    
//    AON->BACKUP0 |= (0x01UL << 8);
    
    debug("LoadBase:%08x\r\n", (uint32_t)load_base);
    debug("ImageBase:%08x\r\n", (uint32_t)image_base);
    debug("LoadLen:%08x\r\n", (uint32_t)load_len);
    
    debug("flash_info 0x0000:%X, %X, %X, %X\r\n", RD_32(0x18000000), RD_32(0x18000004), RD_32(0x18000008), RD_32(0x1800000C));
    debug("flash_info 0x1000:%X, %X, %X, %X\r\n", RD_32(0x18001000), RD_32(0x18001004), RD_32(0x18001008), RD_32(0x1800100C));
    
    debug("BACKUP0:%x, SYS_BACKUP0:%08x\r\n", AON->BACKUP0, SYSCFG->SYS_BACKUP0);
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();
    
    iwdt_disable();
    
    dbgInit();
    debug("Start(rsn:0x%X)...\r\n", rsn);
    
    #if (CFG_FLASH_HPM)
    fshc_quad_mode(PUYA_QUAD);
    puya_enter_hpm();
//    puya_entQuadHpmXipMode();
    #endif
    
    #if (CFG_SLEEP)
//    rtc_conf(true);
//    rc32k_conf(RCLK_DPLL64, 0x0F);
//    rc32k_calib();
    
    core_pmuset(CFG_PMU_DFLT_CNTL);

    #if !(SIM_DEBUG)
//    nopDly(2000000);
    #endif
    #endif

//    for(uint8_t i = 0; i < 64; i++)
//    {
//        *(uint32_t *)(0x20003000 + (i<<2)) = 0;
//    }
//    iopad_init();

    // Start here
//    debug("Start(BLE_DP_TWCORE:%d)...\r\n", APBMISC->BLE_DP_TWCORE);

    struct build_date st_date = ble_build_time();

    debug("B6x Date:%s\r\n", st_date.date);
    debug("Project Date:%s\r\n", __DATE__);
    
    projectCodeInfo();
    
//    APBMISC->BLE_FINECNT_BOUND = 999;
//    iospc_clkout(CLK_OUT_FSHC);
    g_rx_int_cnt = 0;
    
    #if (EM_0x20008400)
    SYSCFG->EM_BASE_CFG = 1;
    #else
    SYSCFG->EM_BASE_CFG = 0;
    #endif
    
    debug("EM_BASE_CFG:%d\r\n", SYSCFG->EM_BASE_CFG);
}

int main(void)
{
    sysInit();
    devInit();

    // Config Heap
    ble_heap(NULL);

    // Init Ble Stack
    ble_init();
    
    // Init App Task
    ble_app();
            
//    ble_dbg_sel(0x83);
    
//    CSC->CSC_PIO[18].Word = 0;
//    GPIO->DAT_CLR = BIT(18);
//    GPIO->DIR_SET = BIT(18);
    
#if !(SIM_DEBUG)
    rf_reg_spi_init();
#endif

    NVIC_EnableIRQ(UART1_IRQn);
    
    // Global Interrupt Enable
    GLOBAL_INT_START();
    
    // main loop
    while (1)
    {
        // Schedule Messages & Events
        ble_schedule();

        // User's Procedure
        user_procedure();
    }
}
