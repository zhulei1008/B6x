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

#define DEMO_CDC_UART       (0)
#define DEMO_HID_BOOT       (1)
#define DEMO_HID_CUSTOM     (0)

#if (DEMO_CDC_UART + DEMO_HID_BOOT + DEMO_HID_CUSTOM != 1)
#error "Select only 1 demo to test!"
#endif

/// Debug Mode: 0=Disable, 1=via UART, 2=RTT Viewer
#define DBG_MODE            (1)

#define DBG_UART_TXD        (17) // PA17
#define DBG_UART_RXD        (18) // PA18

#define FPGA_TEST           (1) // 48MHz

/// Run in Flash with full speed
#define XIP_MODE            (1)

#if (DEMO_CDC_UART && (DBG_MODE != 1))
#undef DBG_MODE
#define DBG_MODE            (1) // uartPutC
#endif

#if (DEMO_HID_BOOT)
#define USE_KEYS            (1)
#endif

/// USB Debug Level: 0=Disable, 1=Error, 2=Warning
#if (DBG_MODE)
#define USB_DBG_LEVEL       (1)
#endif

#define USB_FIFO_BUG        (0)

#endif  //_APP_CFG_H_
