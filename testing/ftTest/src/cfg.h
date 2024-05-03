/**
 ****************************************************************************************
 *
 * @file cfg.h
 *
 * @brief App Configure MACRO, --preinclude
 *
 ****************************************************************************************
 */

#ifndef _APP_CFG_H_
#define _APP_CFG_H_

#include "rf_cfg.h"

//1: download to auxiliary test chip
//0: download to primary test chip
#define AUX_TEST_CHIP      0  // Auxiliary test chip


#define CHIP_QFN32               //down laod to 32 pin chip 
//#define CHIP_QFN20               //down laod to 20 pin chip 
//#define CHIP_SOP16               //down laod to 16 pin chip 
//#define CHIP_SOP8               //down laod to 32 pin chip 



#define AONLDO_TEST
#define CORELDO_TEST
#define RC16M_TEST
#define TRIM_DOWNLOAD_TEST

#define GPIO_IN_OHEL_TEST  
#define GPIO_IN_OLEH_TEST  
#define GPIO_OUT_OHEL_TEST 
#define GPIO_OUT_OLEH_TEST 
#define GPIO_PULL_OPED_TEST
#define GPIO_PULL_ODEP_TEST

#define RF_TX_TEST
#define RF_RX_TEST

#define ADC_TEST
#define MIC_TEST
#define FLASH_TEST
#define RC32K_TEST
#define DPLL_TEST
//#define USB_TEST
#define DEEPSLEEP_CURRENT_TEST
//#define POWEROFF_CURRENT_TEST

#define MEM_BIST_TEST


#define UART_PORT 0
#define PIN_UART_TX 6
#define PIN_UART_RX 7

#define EM_ADDR_0x20005000 (1)
#define EM_ADDR_USE_MACRO  (1)
#define EM_BASE_ADDR       0x20008000
// Whitening disabled
#define WHIT_DISABLE       (1)

#endif  //_APP_CFG_H_

