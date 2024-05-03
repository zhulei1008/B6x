/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
 *
 * Copyright (C) 2019. HungYi Microelectronics Co.,Ltd
 *
 *
 ****************************************************************************************
 */


/*
 * INCLUDES
 ****************************************************************************************
 */
#include "hyreg.h"
#include "uart.h"
#include "uart_cmd.h"
#include "analog_pad.h"
#include "rf_test.h"
#include "dbg.h"
#include "rcc.h"
#include "iopad.h"
#include "bb_mdm_rf_debug.h"
#include "rssi.h"
#include "pwm.h"
/*
 * DEFINES
 ****************************************************************************************
 */
//DMA_CHNL_CTRL_STRUCT_Typedef dma_ctrl_base __attribute__((aligned(0x100)));

/*
 * FUNCTIONS
 ****************************************************************************************
 */
void user_procedure(void);

static void nopDly(uint32_t x)
{
    while (x--)
    {
        __NOP();__NOP();
        __NOP();__NOP();
    }
}

static void sysInit(void)
{
    // ADC clock
    RCC->AHBCLK_EN_RUN.ADC_CLKEN_RUN     = 0;
    RCC->AHBRST_CTRL.ADC_RSTREQ          = 1;
    RCC->AHBRST_CTRL.ADC_RSTREQ          = 0;
    RCC->AHBCLK_EN_RUN.ADC_CLKEN_RUN     = 1;
    
    RCC->APBCLK_EN_RUN.APBMISC_CLKEN_RUN = 1;
    RCC->APBCLK_EN_RUN.AON_CLKEN_RUN = 1;
    RCC->AHBCLK_EN_RUN.SYSCFG_CLKEN_RUN = 1;
    RCC->BLE_CLKRST_CTRL.BLE_AHBEN = 1; // em clk enable
    // em power enable    
    AON->PWOFF_CTRL.EM_PD_EN = 0;
    AON->PWOFF_CTRL.EM_ACCESS_EN = 1;
    // wait em power stable
    while (AON->PWOFF_CTRL.EM_PD_ACK);
    // io latch enable
    AON->PMU_WKUP_CTRL.IO_LATCH_N = 1;
    
    // APB Peripheral Clock Setting
    RCC->APBCLK_EN_RUN.UART1_CLKEN_RUN =1;
    RCC->APBCLK_EN_RUN.MDM_CLKEN_RUN =1;
    RCC->APBCLK_EN_RUN.RF_CLKEN_RUN =1;
//    RCC->BLE_CLKRST_CTRL.BLE_AHBEN = 1;	
//    RCC->BLE_CLKRST_CTRL.BLE_AHB_RSTREQ = 1;	
//    RCC->BLE_CLKRST_CTRL.BLE_AHB_RSTREQ = 0;

//    GPIO->DAT_CLR = (1UL << 4) | (1UL << 14)| (1UL << 15);
//    GPIO->DIR_SET = (1UL << 4) | (1UL << 14)| (1UL << 15);
     
    AON->BKHOLD_CTRL.PIOA13_FUNC_SEL = 0;
 
    AON->XOSC16M_CTRL.XOSC16M_LP       = 0; // bit6

    AON->XOSC16M_CTRL.LDO_XOSC_TR      = 5; // bit[17:14]
    
    AON->XOSC16M_CTRL.XOSC16M_CAP_TR   = 0x0B;  // bit[5:0]
    
    AON->XOSC16M_CTRL.XOSC16M_BIAS_ADJ = 1;  // bit[11:8]
  
    // #01_40SU, -40°C, ppm -1.88
    AON->XOSC16M_CTRL.XOSC16M_CAP_TR   = 0x00;  // bit[5:0]
    
    // #01_40SU, 125°C, ppm 16
//    AON->XOSC16M_CTRL.XOSC16M_CAP_TR   = 0x3F;  // bit[5:0]
    
//    RCC->AHBCLK_EN_RUN.FSHC_CLKEN_RUN = 0;

    // CB_#06_21H, ppm -0.56
    AON->XOSC16M_CTRL.XOSC16M_CAP_TR   = 0x0A;  // bit[5:0]

    // CB_#06_21H, ppm 0.44
//    AON->XOSC16M_CTRL.XOSC16M_CAP_TR   = 0x04;  // bit[5:0]
    
    // CC_#05_21U, ppm 0.63
//    AON->XOSC16M_CTRL.XOSC16M_CAP_TR   = 0x09;  // bit[5:0]
    
    // CC_#04_21U, ppm 0.44
//    AON->XOSC16M_CTRL.XOSC16M_CAP_TR   = 0x0B;  // bit[5:0]
#if (NO_CACHE)
    CACHE->CCR.CACHE_EN = 0;
#endif

#if (FLASH_CODE)
    // flash clk 128M/(FSHCCLK_DIV_NUM+1), FSHCCLK_DIV_NUM > 0
    RCC->CLK_EN_ST.FSHCCLK_DIV_NUM = 2;
#endif

    // #01_40SU, VDD12, 0x10 1.12V, 0x0F 1.37V
//    AON->BKHOLD_CTRL.BK_LDO12_TRIM_CFG = 0x00;
    
#if (DPLL_EN_TEST)
    // Only DPLL1
//    APBMISC->DPLL_CTRL.DPLL2_EN = 0;
//    APBMISC->DPLL_CTRL.DPLL_EN  = 1;
//    APBMISC->DPLL_CTRL.DPLL_SEL = 0;
    
    // DPLL2_POWER
//    #if 1
//    AON->PMU_ANA_CTRL.ANA_RESV |= (0x01UL<<2);
//    #else
//    AON->PMU_ANA_CTRL.ANA_RESV &= ~(0x01UL<<2);
//    #endif
//    
//    // Only DPLL2
//    APBMISC->DPLL_CTRL.DPLL2_EN = 1;
//    APBMISC->DPLL_CTRL.DPLL_SEL = 1;
//    APBMISC->DPLL_CTRL.DPLL_EN  = 0;

    // Select DPLL2
//    APBMISC->DPLL_CTRL.DPLL2_EN = 1;
//    APBMISC->DPLL_CTRL.DPLL_SEL = 1;
//    APBMISC->DPLL_CTRL.DPLL_EN  = 1;
    
    // Select DPLL1
//    APBMISC->DPLL_CTRL.DPLL2_EN = 1;
//    APBMISC->DPLL_CTRL.DPLL_EN  = 1;
//    APBMISC->DPLL_CTRL.DPLL_SEL = 0;
#endif
//    nopDly(100000);
    
//    rccSysClockSwitch(SYS_CLK_48M);

    AON->PMU_ANA_CTRL.LDO12_IBSEL = 0x10;
    
    // CC_#07_21U,CC_#16_21H, ppm 0
//    AON->XOSC16M_CTRL.XOSC16M_CAP_TR   = 0x0C;  // bit[5:0]
    
    // CC_#09_40SU, ppm 0
//    AON->XOSC16M_CTRL.XOSC16M_CAP_TR   = 0x0B;  // bit[5:0]

    // CC_#19_21H, ppm 0.58
//    AON->XOSC16M_CTRL.XOSC16M_CAP_TR   = 0x07;  // bit[5:0]

    //QFN32_#55, ppm 0.125
//    AON->XOSC16M_CTRL.XOSC16M_CAP_TR   = 0x0B;  // bit[5:0]
}

#if (CFG_UART2)
static void uart2_init(void)
{
    RCC->APBCLK_EN_RUN.UART2_CLKEN_RUN = 1;
    
    uart_param_t param = {
        .baud     = DBG_UART_BAUD,
        .databits = DATA_BITS_8b,
        .stopbits = STOP_BITS_1b,
        .parity   = PARITY_NONE,
    };
    
    uartIOInit(1, 6, 7);
    uartConfig(1, &param);
    
    uartReceiveFIFOSet(1, 2/*UART_RX_FIFO_TRIGGER_LVL_8BYTE*/);
    uartReceiveTimeOutSet(1, 20);
    uartITEnable(1, (UART_IT_RXRD | UART_IT_RTO));
    
    NVIC_EnableIRQ(UART2_IRQn);  
}
#endif

void dpll_testa(void)
{
    // Only DPLL2
    APBMISC->DPLL_CTRL.DPLL2_EN = 1;
    APBMISC->DPLL_CTRL.DPLL_SEL = 1;
    APBMISC->DPLL_CTRL.DPLL_EN  = 0;
    
    // Only DPLL1
//    APBMISC->DPLL_CTRL.DPLL2_EN = 0;
//    APBMISC->DPLL_CTRL.DPLL_EN  = 1;
//    APBMISC->DPLL_CTRL.DPLL_SEL = 0;

    /***********************************************************/
    /********************* DPLL外灌电源测试 ********************/

    // PA08, bit10 --- analog enable
    CSC->CSC_PIO[8].Word = 0x400;
    
    // 清bit[1:0]为0
    AON->PMU_ANA_CTRL.ANA_RESV &= ~0x03UL;
    
    // ANA_RESV<0> =1，PA08给DPLL和RF的levelshift同时供电
    AON->PMU_ANA_CTRL.ANA_RESV |= (0x01UL << 0);

    // ANA_RESV<1> =1，DPLL2的供电送到PA08上
    AON->PMU_ANA_CTRL.ANA_RESV |= (0x01UL << 1);

    // 关闭DPLL_LDO使能
    AON->PMU_ANA_CTRL.ANA_RESV &= ~(0x01UL << 2);
    /***********************************************************/
}

static void devInit(void)
{
    dbgInit();
    
//    AON->BKHOLD_CTRL.RC32K_EN_CFG = 1;
//    
//    while (1)
//    {
//        uint8_t cmd = uartGetc(0);
//        debug("cmd:%x\r\n", cmd);
//        
//        if (cmd == 0x88)
//        {
//            AON->BKHOLD_CTRL.RC32K_EN_CFG = 0;
//            APBMISC->RC16M_EN_CFG = 0;
//        }
//        else
//        {
//            AON->BKHOLD_CTRL.RC32K_EN_CFG = 1;
//        }
//    }
//    ioBleTxRx(4, 5);
    #if (CFG_UART2)
    uart2_init();
    #endif

//    dpll_testa();
//    // Only DPLL2
//    APBMISC->DPLL_CTRL.DPLL2_EN = 1;
//    APBMISC->DPLL_CTRL.DPLL_SEL = 1;
//    APBMISC->DPLL_CTRL.DPLL_EN  = 0;
//    sys_clk_out(CLK_OUT_PLL);
//    while (1)
//    {
//        
//    }
    
    rf_2_4g_init();
    
//    rssi_sadc_init();
    
//    // 0x3C, Read Write
//    RF->ANAMISC_CTRL1.DAC_REFH_ADDJ      = 0;// 调df1, df2
//    RF->ANAMISC_CTRL1.DAC_REFH_ADJ       = 0;// 调df1, df2
//    RF->ANAMISC_CTRL1.DAC_REFL_ADJ       = 0x05;// 调df1, df2
//    RF->ANAMISC_CTRL1.DAC_BLE_DELAY_ADJ  = 0;// 调df1, df2
//    
//    // 0x08, Read Write
//    RF->PLL_GAIN_CTRL.PLL_VTXD_EXT      = 28;// 调df1, df2
        
    NVIC_EnableIRQ(UART1_IRQn);
    NVIC_EnableIRQ(BLE_IRQn);
    
//    CSC->CSC_PIO[9].Word     = 0x400;
//    RF->ANAMISC_CTRL1.AT0_SEL= 0;
//    RF->ANA_TRIM.LDO_RX_TRIM = 0;

//    debug_io_init();
//    rf_debug_io(RF_DBG_TX);
//    at0_at1_analog_func_enable();
}

void io_tog(void)
{
//    sys_clk_out(CLK_OUT_AHB);

    iocsc_ctmr_chnl(10, 4);
    // PWM START.
    RCC->APBCLK_EN_RUN.GPTIMB1_CLKEN_RUN = 1;
    PWM_CtrlType pwm_cfg = {
        .mode = PWM_MODE1,
        .polar = HIGH,
    };
    
    pwmInit(CTMR1, 0, 1);
    pwmSet(CTMR1, PWM_CH_A, 1, &pwm_cfg);
//    pwmSet(CTMR1, PWM_CH_B, 1, &pwm_cfg);
//    pwmSet(CTMR1, PWM_CH_C, 1, &pwm_cfg);
//    pwmSet(CTMR1, PWM_CH_D, 1, &pwm_cfg);
    pwmStart(CTMR1);
}

int main(void)
{
//    rccSysClockSwitch(SYS_CLK_64M);

    sysInit();
    
    devInit();
    io_tog();
    // Interrupt Enable
    GLOBAL_INT_START();
    
    #if !(CMW500_BQB)
    rf_rx_2_4_g(1, 0, 15);
    #endif
    
    #if !(FLASH_CODE)
    RCC->AHBCLK_EN_RUN.FSHC_CLKEN_RUN = 0;
    #endif

    while (1)
    {
        user_procedure();
    }
}
