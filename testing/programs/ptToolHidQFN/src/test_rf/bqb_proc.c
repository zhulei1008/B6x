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
#include "hyreg.h"
#include "uart.h"
#include "uartbuff.h"
#include "uart2_buff.h"
#include "rf_test.h"
#include "poweroff.h"

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
static uint8_t buff2[BLE_MAX_LEN];
static uint16_t buff2_len = 0;
enum uart_cmd
{
    BQB_HEAD     = 0x01,
    
    DF1_DF2_HEAD = 0x30,
};

extern uint16_t g_rx_cnt, g_rx_err;
volatile uint8_t g_tx_rate = 0, g_tx_freq = 0;
bool g_test_end_flag = false;

void uart_proc(void)
{
    static uint8_t null_cnt = 0;
    uint16_t len;
    bool finish = true;
    
    len = uartRxRead(&buff[buff_len], BLE_MAX_LEN - buff_len);
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
        case BQB_HEAD:
        {
            if (buff_len < 4)
                finish = false;
            
            // reset, 01 02 0c 00
            if ((buff[1] == 0x02) && (buff_len == 4))
            {
                uint8_t rsp_data[] = {0x01, 0x02, 0x0C, 0x00};
                
                for (uint8_t i = 0; i < sizeof(rsp_data); i++)
                {
                    uartPutc(0, rsp_data[i]);
                }
                
                poweroffTest();
            }
            
            // reset, 01 03 0c 00
            if ((buff[1] == 0x03) && (buff_len == 4))
            {
                uint8_t rsp_data[7] = {0x04, 0x0E, 0x04, 0x05, 0x03, 0x0C, 0x00};
                
                rf_2_4g_init();
                
                g_rx_cnt = 0;
                g_rx_err = 0;
                g_test_end_flag = false;
                
                for (uint8_t i = 0; i < sizeof(rsp_data); i++)
                {
                    uartPutc(0, rsp_data[i]);
                }
            }
            
            // rx 1M, 01 1D 20 01 chan
            if ((buff[1] == 0x1D) && (buff_len == 5))
            {
                rf_rx_2_4_g(buff[4], RATE_1Mbps, 0xFF);
                
                g_test_end_flag = false;
                
                uint8_t rsp_data[7] = {0x04, 0x0E, 0x04, 0x05, 0x1D, 0x20, 0x00};

                for (uint8_t i = 0; i < sizeof(rsp_data); i++)
                {
                    uartPutc(0, rsp_data[i]);
                }
            }
            
            // rx 2M, 01 33 20 03 chan 02 00
            if ((buff[1] == 0x33) && (buff_len == 7))
            {
                rf_rx_2_4_g(buff[4], RATE_2Mbps, 0xFF);
                
                g_test_end_flag = false;
                
                uint8_t rsp_data[7] = {0x04, 0x0E, 0x04, 0x05, 0x33, 0x20, 0x00};

                for (uint8_t i = 0; i < sizeof(rsp_data); i++)
                {
                    uartPutc(0, rsp_data[i]);
                }
            }
            
            // tx 1M, 01 1E 20 03 chan len tx_payl
            if ((buff[1] == 0x1E) && (buff_len == 7))
            {
                uint8_t rsp_data[7] = {0x04, 0x0E, 0x04, 0x05, 0x1E, 0x20, 0x00};
                
                rf_tx_cmw500(buff[4], RATE_1Mbps, buff[5], buff[6]);
                
                g_test_end_flag = false;
                
                for (uint8_t i = 0; i < sizeof(rsp_data); i++)
                {
                    uartPutc(0, rsp_data[i]);
                }
            }
            
            // tx 2M, 01 34 20 04 chan len tx_payl 02
            if ((buff[1] == 0x34) && (buff_len == 8))
            {
                uint8_t rsp_data[7] = {0x04, 0x0E, 0x04, 0x05, 0x34, 0x20, 0x00};
                
                rf_tx_cmw500(buff[4], RATE_2Mbps, buff[5], buff[6]);
                
                g_test_end_flag = false;
                for (uint8_t i = 0; i < sizeof(rsp_data); i++)
                {
                    uartPutc(0, rsp_data[i]);
                }
            }
            
            // test end, 01 1F 20 00
            if ((buff[1] == 0x1F) && (buff_len == 4))
            {
                g_test_end_flag = true;
                rf_stop_test();
                
                uint16_t correct_cnt = g_rx_cnt - g_rx_err;
                
                uint8_t rsp_data[9] = {0x04, 0x0E, 0x06, 0x05, 0x1F, 0x20, 0x00};
                rsp_data[7] = (correct_cnt &  0xFF);
                rsp_data[8] = ((correct_cnt >> 8) &  0xFF);
                
                for (uint8_t i = 0; i < sizeof(rsp_data); i++)
                {
                    uartPutc(0, rsp_data[i]);
                }
                
                g_rx_cnt = 0;
                g_rx_err = 0;
            }
        } break;
        
        default:
            break;
    }

    if (finish)
    {
        buff_len = 0;
    }
}

void uart2_proc(void)
{
    static uint8_t null_cnt = 0;
    uint16_t len;
    bool finish = true;
    
    len = uart2_RxRead(&buff2[buff2_len], BLE_MAX_LEN - buff2_len);
    if (len > 0)
    {
        buff2_len += len;
        if (buff2_len < BLE_MAX_LEN)
        {
            return; // wait full
        }
    }
    else
    {
        if ((buff2_len > 0) && (null_cnt++ > NULL_CNT))
        {
            //finish = true;
            null_cnt = 0;
        }
        else
        {
            return; // wait again
        }
    }
    
    switch (buff2[0])
    {
        case DF1_DF2_HEAD:
        {
            if (buff2_len < 6)
                finish = false;
            
            RF->ANAMISC_CTRL1.DAC_REFH_ADDJ      = buff2[1];// 调df1, df2
            RF->ANAMISC_CTRL1.DAC_REFH_ADJ       = buff2[2];// 调df1, df2
            RF->ANAMISC_CTRL1.DAC_REFL_ADJ       = buff2[3];// 调df1, df2            
            RF->ANAMISC_CTRL1.DAC_BLE_DELAY_ADJ  = buff2[4];// 调df1, df2
            RF->PLL_GAIN_CTRL.PLL_VTXD_EXT       = buff2[5];// 调df1, df2
//            RF->ANA_TRIM.BG_RES_TRIM             = buff2[4];
//            RF->ANA_TRIM.BG_BIAS_TRIM            = buff2[4];
            
            
        } break;
        
        default:
        {
            for (uint8_t i = 0; i < buff2_len; i++)
            {
                uartPutc(1, buff2[i]);
            }
            buff2_len = 0;
        } break;
    }

    if (finish)
    {
        buff2_len = 0;
    }
}

void user_procedure(void)
{
    uart_proc();
    
    #if (CFG_UART2)
    uart2_proc();    
    #endif

    rf_rx_2_4_g_data_proc();
}
