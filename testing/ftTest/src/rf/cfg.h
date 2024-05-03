/**
 ****************************************************************************************
 *
 * @file cfg.h
 *
 * @brief App Configure MACRO, --preinclude
 *
 * Copyright (C) 2019. HungYi Microelectronics Co.,Ltd
 *
 *
 ****************************************************************************************
 */

#ifndef _APP_CFG_H_
#define _APP_CFG_H_

#define CFG_FPGA           (0)
#define CFG_AGC            (0)

#define CFG_UART2          (0)
#define DBG_MODE           (1)

#if (DBG_MODE)
#define DEBUG_RF           (0)
#define DEBUG_PROC         (1)

#endif

#define EM_ADDR_0x20005000 (1)
#define EM_ADDR_USE_MACRO  (1)

#define EM_BASE_ADDR       0x20005000
// Whitening disabled
#define WHIT_DISABLE       (1)

// UART CMD Port & IO
#define UART_CMD_PORT 0  // UART1
#define UART_IO_TX    14
#define UART_IO_RX    15

#endif  //_APP_CFG_H_
