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
#define FAST_DONGLE_MODE       (1)
/// System Clock(0=16MHz, 1=32MHz, 2=48MHz, 3=64MHz)
#define SYS_CLK                (2)

/// Debug Mode(0=Disable, 1=via UART, 2=via RTT)
#define DBG_MODE               (0)
#define DBG_UART_TXD           (13)
#define DBG_UART_RXD           (18)

#define PA_UART1_TX            (DBG_UART_TXD)
#define PA_UART1_RX            (DBG_UART_RXD)
#if (SYS_CLK == 2)
#define DBG_UART_BAUD           BRR_DIV(921600, 48M)
#else
#define DBG_UART_BAUD           BRR_DIV(921600, 16M)
#endif

/// BLE Configure (Single or Multi-Connections)
#define BLE_NB_SLAVE           (0)
#define BLE_NB_MASTER          (2)
#define SCAN_NUM_MAX           (2)
#define BLE_EN_SMP             (0)
#define BLE_ADDR               {0x01, 0x20, 0x10, 0x23, 0x20, 0xD2}
#define BLE_DEV_NAME           "usb_receiver2_"
#define BLE_DEV_ICON           0x0000
#define BLE_AUTH               (GAP_AUTH_REQ_NO_MITM_NO_BOND)
#define BLE_SYNC_WORD          (0x26D5EF45)

#define BLE_HEAP_BASE          (0x20004A00)

#define BT_MAC_STORE_OFFSET    (0x1200)

#define CFG_HW_TIMER           (0)
#if (CFG_HW_TIMER)
#define SFTMR_SRC              (TMS_SysTick)
#define TMS_SysTick_ARR        (48 * 10000)
#endif

/// Profile Configure
#define GATT_CLI               (1)

/// Debug Configure
#if (DBG_MODE)
    #define DBG_APP            (0)
    #define DBG_ACTV           (0)
    #define DBG_GAPC           (0)
    #define DBG_GAPM           (0)
    #define DBG_GATT           (0)
    #define DBG_USB            (1)
#endif

/********************************************************/
/// USB & BLE CFG
/********************************************************/

/// HID Report Map, same with Keybd & Mouse
#define HID_RPT_KB          (1)
#define HID_RPT_MEDIA       (1)
#define HID_RPT_SYSTEM      (1)

/// HID Report ID and Length, declared in Report Map (User Customize)
#define RPT_ID_KB           (1)
#define BLE_RPT_LEN_KB      (8) // 1B(ctlkeys) + 1B(resv) + 6B(keycode)
#define USB_RPT_LEN_KB      (BLE_RPT_LEN_KB) // + rptid(1B)

#define RPT_ID_MEDIA        (2)
#define BLE_RPT_LEN_MEDIA   (3) // 24bits
#define USB_RPT_LEN_MEDIA   (BLE_RPT_LEN_MEDIA + 1) // + rptid(1B)

#define RPT_ID_SYSTEM       (3)
#define BLE_RPT_LEN_SYSTEM  (1) // 8bits
#define USB_RPT_LEN_SYSTEM  (BLE_RPT_LEN_SYSTEM + 1) // + rptid(1B)

#define RPT_ID_MOUSE        (4)
#define BLE_RPT_LEN_MOUSE   (4) // 1B(button) + 1B(X) + 1B(Y) + 1B(Wheel)
#define USB_RPT_LEN_MOUSE   (BLE_RPT_LEN_MOUSE) // no rptid

#define PKT_KB_LEN          (USB_RPT_LEN_KB)
// data_len + rpt_id + data
#define PKT_CONSUMER_LEN    (USB_RPT_LEN_MEDIA + 1)
#define PKT_MS_LEN          (USB_RPT_LEN_MOUSE)

// Keybd & Mouse GATT Handler
#define GATT_MOUSE_HDL      (0x23)

#define GATT_KB_HDL         (0x23)
#define GATT_WR_HDL         (0x27)
#define GATT_MEDIA_HDL      (0x2A)
#define GATT_SYS_HDL        (0x2E)

#endif  //_APP_CFG_H_
