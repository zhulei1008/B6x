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
#define DBG_FSH_TEST        (1)
#endif

#define FPGA_TEST           (1)

#define SCT_EM_STEP         (6)

#define SCT_SRAM_STEP       (4)

#define CFG_WDT_DIS         (1)

#endif  //_APP_CFG_H_
