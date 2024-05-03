#ifndef _RF_TEST_H_
#define _RF_TEST_H_

#include <stdint.h>


#define RF_PACKET_LEN    100
#define RX_DATA_ADDR     ((EM_BASE_ADDR) + (EM_BLE_DATARXBUF_OFFSET))
#define TX_DESC_ADDR     ((EM_BASE_ADDR) + (EM_BLE_TX_DESC_OFFSET))

enum rf_rate
{
    RATE_1Mbps     = 0x0,
    RATE_2Mbps     = 0x1,
    RATE_125Kbps   = 0x2,
    RATE_500Kbps   = 0x3,
};

enum rf_fmt
{
    FMT_TX_TEST    = 0x1C,
    FMT_RX_TEST    = 0x1D,
    FMT_TXRX_TEXT  = 0x1E,
};

enum tx_payl
{
    PAYL_PRBS9     = 0x00,
    PAYL_11110000,
    PAYL_10101010,
    PAYL_PRBS15,
    PAYL_ALL_1,
    PAYL_ALL_0,
    PAYL_00001111,
    PAYL_01010101,
};

void emi_init(void);
void rf_2_4g_init(void);
void rf_stop_test(void);
void rf_rx_test(uint8_t chnl, uint8_t rate, uint8_t pkt_len);
void rf_tx_test(uint8_t chnl, uint8_t rate, uint8_t data, uint16_t data_len);
void rf_rx_test_proc(void);
void rf_tx_test_proc(void);
uint8_t rf_test(uint8_t freq, uint8_t pkt_len);
void rf_tx_cmw500(uint8_t chnl, uint8_t rate, uint16_t data_len, uint8_t tx_payl);
#endif
