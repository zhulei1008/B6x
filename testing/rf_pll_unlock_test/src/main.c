/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
 *
 ****************************************************************************************
 */
#include "regs.h"
#include "drvs.h"
#include "sysdbg.h"
#include "dbg.h"
#include "rf_afe.h"
#include "uart_cmd.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * FUNCTIONS
 ****************************************************************************************
 */
static void sysInit(void)
{
    RCC_APBCLK_EN(APB_APBMISC_BIT | APB_AON_BIT | APB_UART1_BIT | APB_RF_BIT);
    RCC_AHBCLK_EN(AHB_SYSCFG_BIT);
    
    rcc_adc_en();
    rcc_ble_en();
    
    iom_ctrl(19, IOM_PULLUP|IOM_INPUT);
    iospc_rstpin(false);
    
    // QFN48-2, 16000007.96M
    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x19;

    // QFN48-3, 16000008.82M
    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x19;
    
    // QFN48-6, 16000003.96M
//    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x20;

    // QFN48-29
    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x17;
    
    // QFN48-31, 16000008.56M
    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x19;
    
    // QFN48-26, 16000000.00M
//    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x19;
//    trim_load();    

    // QFN32-6, 16000002.91
//    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x17;

    // QFN32-12
//    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x16;

//    // QFN32-18, 15999998.04
//    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x14;

    // QFN32-21, 16000000.00
//    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x15;

    // QFN32-22, 16000000.00
    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x17;
    
    // QFN32-29, 16000000.00
//    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x1A;
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();
    
    iwdt_disable();

//    ioClkOut(CLK_OUT_XO16M);

    dbgInit();
    debug("Start(rsn:0x%X)...\r\n", rsn);

    // AT1 PA19
    iospc_rstpin(true);
    sysdbg_anatst(ATEN_AT0_AT1, 0);
//    while (1)
//    {
//        
//    }

    rf_debug_test_init();
    
//    sys_dbg_init();
//    rf_dbg_sel(RF_DBG_TX);
    
//    GPIO_DAT_CLR(PA02 | PA03);
//    GPIO_DIR_SET(PA02 | PA03);
//    iom_ctrl(2, IOM_SEL_GPIO);
//    iom_ctrl(3, IOM_SEL_GPIO);

//    at0_at1_cbpf_out();
//    rx_2m_if();
//    while (1)
//    {
//        
//    }
//    get_gain_cal_st(17);// 2440M

    // 2468M
//    tx_sw_channel_num_test(31);

    iom_ctrl(PA_AT0, IOM_ANALOG);
}

/******************************************/
// QFN48-3
// SW_PA_GAIN_TARGET = 0x0D
// 3.3V Tx(0.9dBm):9.3mA Rx(-39dBm):14.0mA
// 1.8V Tx(0.1dBm):8.7mA Rx(-39dBm):10.3mA


// SW_PA_GAIN_TARGET = 0x0C
// 3.3V Tx(-2.0dBm):7.8mA Rx(-39dBm):14.0mA
// 1.8V Tx(-3.3dBm):7.2mA Rx(-39dBm):10.3mA
/******************************************/

int main(void)
{
    uint8_t lock_sta = 0, pll_dis = 0, pll_frach = 0;
    
    sysInit();
    devInit();
    
    RF->PLL_TAB_OFFSET.RX_FRAC_OFFSET = 0;
    RF->PLL_ANA_CTRL.PLL_MIX_SEL      = 1; // in Rx --- 0:-2M, 1:+2M
    RF->PLL_ANA_CTRL.PLL_RTX_SEL_REG  = 0; // 选择RX模式
    RF->ANA_EN_CTRL.EN_PA_REG         = 0; // 关闭PA
    RF->PLL_DYM_CTRL.SW_TX_EN         = 0;
    
    RF->ANA_PWR_CTRL.TEST_EN_LDO_VCO     = 0;
    RF->ANA_PWR_CTRL.TEST_EN_LDO_PA      = 0;
    RF->ANA_PWR_CTRL.TEST_EN_LDO_IF      = 0;
    RF->ANA_PWR_CTRL.TEST_EN_LDO_PLL     = 1;
    RF->ANA_PWR_CTRL.TEST_EN_DAC_DIG_PWR = 0;
    RF->ANA_PWR_CTRL.TEST_EN_AGC_PWR     = 0;
    RF->ANA_PWR_CTRL.TEST_EN_LDO_TX      = 0;
    RF->DIG_CTRL.LDO_TEST_EN = 1;
    
    RF->ANAMISC_CTRL1.AT0_SEL = AT0_SEL_VDD_PLL;
//    RF->ANAMISC_CTRL1.AT0_SEL = AT0_SEL_VDD_IF;
    RF->ANAMISC_CTRL1.AT1_SEL = AT1_SEL_VDD_VCO;
    
    GPIO->DAT_CLR = BIT(12);
    GPIO->DIR_SET = BIT(12);
    iom_ctrl(PA12, IOM_HIZ);
    
    while (1)
    {
        for (pll_dis = 4; pll_dis < 9; ++pll_dis)
        {
            for (pll_frach = 0; pll_frach < 18; pll_frach += 2)
            {
//                RF->ANA_PWR_CTRL.TEST_EN_LDO_PLL = 1;
//                btmr_delay(1600, 300);
                
//                lock_sta = pll_rx_unlock_test(6, 0);
                lock_sta = pll_rx_unlock_test(pll_dis, pll_frach);

                if (lock_sta == 0)
                {
                    GPIO->DAT_SET = BIT(12);
                    debug("unlock(dis:%d, frach:%d)(test_en:%d, ldo:%08x)\r\n", pll_dis, pll_frach, 
                    RF->DIG_CTRL.LDO_TEST_EN, RF->ANA_PWR_CTRL.Word);
                    GPIO->DAT_CLR = BIT(12);
                }
                
//                btmr_delay(1600, 20);
//                RF->PLL_DYM_CTRL.SW_RX_EN        = 0;
//                RF->ANA_PWR_CTRL.TEST_EN_LDO_PLL = 0;
//                
//                btmr_delay(1600, 300);
            }
        }
    }
//    pll_tx_1m_lock_test_cmd(23, 8); //2440M
    
    //QFN48-3
//    RF->PLL_DYM_CTRL.SW_PA_GAIN_TARGET   = 0x0C;
//    pll_tx_1m_lock_test_cmd(0x17, 8);
//    pll_rx_1m_lock_test_cmd(5, 0);
//    userUartParse();
}
