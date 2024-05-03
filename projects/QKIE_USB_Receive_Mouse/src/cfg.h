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

#define CFG_USB                (1)

#define CFG_USE_RPT_ID         (0)

#define DEMO_HID_BOOT          (1)
#define FAST_DONGLE_MODE       (0)
/// System Clock(0=16MHz, 1=32MHz, 2=48MHz, 3=64MHz)
#define SYS_CLK                (2)

/// Debug Mode(0=Disable, 1=via UART, 2=via RTT)
#define DBG_MODE               (0)
#define DBG_UART_TXD           (17)
#define DBG_UART_RXD           (18)

#define PA_UART1_TX            (DBG_UART_TXD)
#define PA_UART1_RX            (DBG_UART_RXD)
//#define UART1_CONF_BAUD        BRR_DIV(921600, 48M)

/// BLE Configure (Single or Multi-Connections)
#define BLE_NB_SLAVE           (0)
#define BLE_NB_MASTER          (1)
#define BLE_ADDR               {0x11, 0x88, 0x3F, 0xA1, 0x01, 0xF3}//{0x2C, 0x28, 0x08, 0x23, 0x20, 0xD2}
#define BLE_DEV_NAME           "usb_receiver_"
#define BLE_DEV_ICON           0x0000
#define BLE_AUTH               (GAP_AUTH_REQ_NO_MITM_NO_BOND)
#define BLE_EN_SMP             (0)
//#define BLE_SYNC_WORD          (0x8E89BED6)//(0x26D5EF45)

#define SCAN_NUM_MAX           (1)
#define CFG_HW_TIMER           (0)
#define BLE_HEAP_BASE          (0x20004E00)

#define BT_MAC_STORE_OFFSET    (0x1200)

/// Profile Configure
#define GATT_CLI               (1)

/// Debug Configure
#if (DBG_MODE)
    #define DBG_APP            (0)
    #define DBG_PROC           (0)
    #define DBG_ACTV           (1)
    #define DBG_GAPM           (1)
    #define DBG_GAPC           (1)
    #define DBG_GATT           (1)
    #define DBG_USB            (0)
#endif

/// USB&BLE CFG
#define GATT_BAS_HDL           (0x1D)
#define GATT_MOUSE_HDL         (0x27)
#define MOUSE_LEN              (4)

#endif  //_APP_CFG_H_
