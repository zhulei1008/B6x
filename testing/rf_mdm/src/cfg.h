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

#define DBG_MODE           (1)

#if (DBG_MODE)
#define DEBUG_RF           (0)
#define DEBUG_PROC         (0)
#define DEBUG_RF_MDM       (1)
#endif

#define EM_ADDR_0x20005000 (1)
#define EM_ADDR_USE_MACRO  (1)

#define EM_BASE_ADDR       0x20008000
// Whitening disabled
#define WHIT_DISABLE       (1)

#endif  //_APP_CFG_H_
