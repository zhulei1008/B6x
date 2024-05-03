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

#if (DBG_MODE)
#define DBG_CORE            (1)
#endif

#define SCT_EM_STEP         (9)

#define SCT_SRAM_STEP       (5)

#define SCT_FLASH_STEP      (0x3F)

#define CFG_WDT_DIS         (1)


#define XO16M_CLK           (0)
#define RC16M_CLK           (1)

#endif  //_APP_CFG_H_
