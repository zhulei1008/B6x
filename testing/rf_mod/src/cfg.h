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

#define USE_UART1          (1)
#define USE_UART2          (0)

#if (USE_UART1)
#define PA_UART1_TX         (6) //PA06
#define PA_UART1_RX         (7) //PA07
#define UART1_CONF_BAUD     (BRR_115200)
#endif

#if (USE_UART2)
#define PA_UART2_TX        (8)
#define PA_UART2_RX        (9)
#define UART2_CONF_BAUD    (BRR_115200)
#endif

#define EM_BASE_ADDR       0x20008000

// Whitening disabled
#define WHIT_DISABLE       (1)

#endif  //_APP_CFG_H_
