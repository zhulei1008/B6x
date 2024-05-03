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

#define DEMO_HID_BOOT          (1)
#if (DEMO_HID_BOOT)
#define USE_KEYS               (1)
#endif

#define SCT_FLASH_STEP         (0)

#define SIM_DEBUG              (0)
#define FPGA_TEST              (1)
#define CFG_FLASH_HPM          (0)

/// System Clock(0=16MHz, 1=32MHz, 2=48MHz, 3=64MHz)
#define SYS_CLK                (0)

/// Debug Mode(0=Disable, 1=via UART, 2=via RTT)
#if (SIM_DEBUG)
#define DBG_MODE               (0)
#else
#define DBG_MODE               (1)
#endif

/// BLE Configure
#define BLE_NB_SLAVE           (1)
#define BLE_NB_MASTER          (0)

#define BLE_ADDR               {0x20, 0x09, 0x22, 0x20, 0x01, 0xD2}
#define BLE_DEV_NAME           "myBle-USB-FPGA"
#define BLE_DEV_ICON           0x0000
#define BLE_PHY                (GAP_PHY_LE_1MBPS)

/// Profile Configure
#define ROM_SRC                (0)
#define PRF_DISS               (0)
#define PRF_BASS               (0)
#define PRF_SESS               (1)
#define PRF_PTSS               (0)
#define PRF_OTAS               (0)
#define PRF_HOGPD              (0)

/// Debug Configure
#if (DBG_MODE)
    #define DBG_APP            (0)
    #define DBG_PRF            (0)
    #define DBG_ACTV           (1)
    #define DBG_GAPM           (1)
    #define DBG_GAPC           (1)

    #define DBG_CORE           (0)
    #define DBG_PROC           (0)
    #define DBG_SESS           (1)
    #define DBG_USB_TEST       (1)
    
    /// USB Debug Level: 0=Disable, 1=Error, 2=Warning
    #define USB_DBG_LEVEL      (1)
    
#endif

/// Misc Options
#define LED_PLAY               (0)
#define CFG_SLEEP              (0)

/// PAD Observe

#define PAD_PWR_FLG            4

#define PAD_OSCEN_FLG          10
#define PAD_BLE_SLEEP_FLG      11
#define PAD_CORE_DEEPSLEEP     12
#define PAD_BB_TX_EN           13
#define PAD_TIM_PIO            14
#define PAD_BB_RX_EN           15
#define PAD_BB_WKUP_END        15
#endif  //_APP_CFG_H_
