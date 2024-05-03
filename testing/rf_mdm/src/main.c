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
#include "rf_test.h"
#include "dbg.h"
#include "sysdbg.h"
#include "rf_mdm.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define RF_MDM_TX_DATA_LEN          255


/*
 * FUNCTIONS
 ****************************************************************************************
 */
void user_procedure(void);

static void sysInit(void)
{   
    RCC_APBCLK_EN(APB_APBMISC_BIT | APB_AON_BIT | APB_UART1_BIT | APB_RF_BIT | APB_MDM_BIT);
    RCC_AHBCLK_EN(AHB_SYSCFG_BIT);

    rcc_adc_en();
    rcc_ble_en();
    
    iospc_rstpin(false);
    
    // QFN32-6, 16000002.91
    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x17;
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();
    
    iwdt_disable();

    dbgInit();
    debug("Start(rst:%X)...\r\n", rsn);
}

static void mdmTest(void)
{
    uint8_t chan = 0;
    sys_dbg_init();
    mdm_dbg_sel(MDM_DBG_EXT_RX);
    
    rf_2g4_init();

    rf_mdm_init();

//    rf_mdm_tx_start(0, RATE_1Mbps);
//    rf_mdm_tx_stop();
    
    uint8_t tx_data[RF_MDM_TX_DATA_LEN];
    
    for (uint8_t i = 0; i < RF_MDM_TX_DATA_LEN; ++i)
    {
        tx_data[i] = i + 1;
    }
    
    while (1)
    {
        rf_mdm_tx(chan, RATE_1Mbps, tx_data, 20);
        chan = (chan + 1) % 40;
    }
}

int main(void)
{
    sysInit();
    devInit();

    mdmTest();
    
//    // Interrupt Enable
//    GLOBAL_INT_START();

//    while (1)
//    {
//        // empty
//    }
}
