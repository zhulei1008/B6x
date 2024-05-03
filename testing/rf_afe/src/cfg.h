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

/// Debug Mode: 0=Disable, 1=via UART, 2=RTT Viewer
#define DBG_MODE            (1)

#define UART_IO_TX          (10)
#define UART_IO_RX          (11)
#define UART_BAUD           BRR_DIV(115200, 48M)

#define DBG_UART_TXD        UART_IO_TX
#define DBG_UART_RXD        UART_IO_RX
#define DBG_UART_BAUD       UART_BAUD
#endif  //_APP_CFG_H_
