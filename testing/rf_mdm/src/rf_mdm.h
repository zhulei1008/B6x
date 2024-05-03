#ifndef RF_MDM_2P4G_H_
#define RF_MDM_2P4G_H_

#include "string.h"
#include <stdint.h>

void rf_mdm_init(void);
void rf_mdm_tx_start(uint8_t rf_chan, uint8_t rf_rate);
void rf_mdm_tx_stop(void);

void rf_mdm_tx(uint8_t rf_chan, uint8_t rf_rate, uint8_t *data, uint8_t data_len);
void rf_mdm_rx(uint8_t rf_chan, uint8_t rf_rate);
#endif // RF_MDM_2P4G_H_
