/**
 ****************************************************************************************
 *
 * @file proc.c
 *
 * @brief user procedure.
 *
 ****************************************************************************************
 */
#include <stdbool.h>
#include "b6x.h"
#include "uart.h"
#include "uartRb.h"
#include "rf_test.h"

#if (DEBUG_PROC)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<PROC>" format "\r\n", ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#endif
#define BLE_MAX_LEN 20
#define NULL_CNT 20
static uint8_t buff[BLE_MAX_LEN];
static uint16_t buff_len = 0;

enum uart_cmd
{
    RF_Tx     = 0x10,
              
    RF_Rx     = 0x20,
              
    RF_STOP   = 0x30,
    
    RF_RESULT = 0x40,
};

extern uint16_t g_rx_cnt, g_rx_err;
volatile uint8_t g_tx_rate = 0, g_tx_freq = 0;

void uart_proc(void)
{
    static uint8_t null_cnt = 0;
    uint16_t len;
    bool finish = true;
    
    len = uart1Rb_Read(&buff[buff_len], BLE_MAX_LEN - buff_len);
    if (len > 0)
    {
        buff_len += len;
        if (buff_len < BLE_MAX_LEN)
        {
            return; // wait full
        }
    }
    else
    {
        if ((buff_len > 0) && (null_cnt++ > NULL_CNT))
        {
            //finish = true;
            null_cnt = 0;
        }
        else
        {
            return; // wait again
        }
    }
    
    switch (buff[0])
    {
        case RF_Rx:
        {
            if (buff_len >= 4)
            {
                DEBUG("Rx(ch:0x%02x, rate:%d)", buff[1], buff[2]);
                rf_2g4_rx(buff[1], buff[2], buff[3]);
            }
        } break;
        
        case RF_Tx:
        {
            if (buff_len >= 4)
            {
//                g_tx_cnt = 1;
//                g_evt_end_flag = false;
//                g_tx_freq = buff[1];
//                g_tx_rate = buff[2];
//                DEBUG("Tx(ch:0x%02x, rate:%d)", buff[1], buff[2]);
//                rf_2g4_tx(buff[1], g_tx_rate, (void*)(0xC0), 16);
                rf_tx_cmw500(buff[1], buff[2], buff[3], buff[4]);
            }
        } break;
        
        case RF_STOP:
        {
            DEBUG("Stop");
            rf_stop_test();
            rf_2g4_init();
        } break;

        case RF_RESULT:
        {
            DEBUG("rx_cnt: %d, rx_err:%d", g_rx_cnt, g_rx_err);
            g_rx_cnt = 0;
            g_rx_err = 0;
        } break;
        
        default:
            break;
    }

    if (finish)
    {
        buff_len = 0;
    }
}

void user_procedure(void)
{
    uart_proc();

    rf_2g4_rx_proc();
}
