#ifndef BB_MDM_RF_DBG_H_
#define BB_MDM_RF_DBG_H_

#include <stdint.h>

#define IO_ANALOG_ENABLE_BIT (0x01UL << 10)

#define AT0_PAD               9  // PA09 Fixed, don't modify
#define AT1_PAD               10 // PA10 Fixed, don't modify

// RF->DIG_CTRL.RF_DBG_SEL
enum rf_debug
{
    RF_DBG_TX  = 1,
    RF_DBG_RX,
    
    RF_DBG_MAX = 0xFF,
};

// MDM->REG0.DEBUG_MODE
enum mdm_debug
{
    MDM_DBG1    = 1,
    MDM_DBG2,
    MDM_DBG3,
    
    MDM_DBG_MAX = 0xFF,
};

// SYSCFG->DBG_CTRL.MDM_BBN_DEBUG_SEL
enum mdm_bb_dbg_sel
{
    MDM_BBN_DEBUG_SEL_RF = 0,
    MDM_BBN_DEBUG_SEL_BB,
    MDM_BBN_DEBUG_SEL_MDM,
    
    MDM_BBN_DEBUG_SEL_MAX = 0xFF,
};

void debug_io_init(void);

void at0_at1_analog_func_enable(void);

// @see enum rf_debug
void rf_debug_io(uint8_t rtx_sel);

// @see enum mdm_debug
void modem_debug_io(uint8_t debug_mode);

void ble_io_init(void);
void mdm_bb_dbg_sel(void);
    
#endif // BB_MDM_RF_DBG_H_
