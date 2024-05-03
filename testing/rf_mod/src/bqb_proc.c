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
#include "regs.h"
#include "uart.h"
#include "uartRb.h"
#include "rf_test.h"

#if (DEBUG_PROC)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<PROC>" format "\r\n", ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#endif

#define UART_PTR(port)         ((UART_TypeDef *)(UART1_BASE + (port) * 0x1000))

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
bool g_test_end_flag = false;

void uart_send(uint8_t port, uint16_t len, uint8_t *data)
{
    UART_TypeDef* uart = UART_PTR(port);
    while (len)
    {
        while (!(uart->SR.TBEM));
        uart->TBR = *data++;
        len--;
    }
    
    while (!(uart->SR.TBEM)); // wait tx finish
    while (uart->SR.BUSY);    // wait idle state
}

void uart1_proc(void)
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
        case BQB_HEAD:
        {
            if (buff_len < 4)
                finish = false;
            
            g_test_end_flag = false;
            
            // reset, 01 03 0c 00
            if ((buff[1] == 0x03) && (buff_len == 4))
            {
                uint8_t rsp_data[7] = {0x04, 0x0E, 0x04, 0x05, 0x03, 0x0C, 0x00};
                
                rf_2g4_init();
                
                g_rx_cnt = 0;
                g_rx_err = 0;
                
                uart_send(0, sizeof(rsp_data), rsp_data);
            }
            
            // rx 1M, 01 1D 20 01 chan
            if ((buff[1] == 0x1D) && (buff_len == 5))
            {
                rf_2g4_rx(buff[4], RATE_1Mbps, 0xFF);
                
                uint8_t rsp_data[7] = {0x04, 0x0E, 0x04, 0x05, 0x1D, 0x20, 0x00};

                uart_send(0, sizeof(rsp_data), rsp_data);
            }
            
            // rx 2M, 01 33 20 03 chan 02 00
            if ((buff[1] == 0x33) && (buff_len == 7))
            {
                rf_2g4_rx(buff[4], RATE_2Mbps, 0xFF);
                
                uint8_t rsp_data[7] = {0x04, 0x0E, 0x04, 0x05, 0x33, 0x20, 0x00};

                uart_send(0, sizeof(rsp_data), rsp_data);
            }
            
            // tx 1M, 01 1E 20 03 chan len tx_payl
            if ((buff[1] == 0x1E) && (buff_len == 7))
            {
                uint8_t rsp_data[7] = {0x04, 0x0E, 0x04, 0x05, 0x1E, 0x20, 0x00};
                
                rf_tx_cmw500(buff[4], RATE_1Mbps, buff[5], buff[6]);
                
                uart_send(0, sizeof(rsp_data), rsp_data);
            }
            
            // tx 2M, 01 34 20 04 chan len tx_payl 02
            if ((buff[1] == 0x34) && (buff_len == 8))
            {
                uint8_t rsp_data[7] = {0x04, 0x0E, 0x04, 0x05, 0x34, 0x20, 0x00};
                
                rf_tx_cmw500(buff[4], RATE_2Mbps, buff[5], buff[6]);

                uart_send(0, sizeof(rsp_data), rsp_data);
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
                
                uart_send(0, sizeof(rsp_data), rsp_data);
                
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
    
    len = uart1Rb_Read(&buff2[buff2_len], BLE_MAX_LEN - buff2_len);
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
            
            RF->ANAMISC_CTRL1.DAC_REFH_ADDJ         = buff2[1];// 调df1, df2
            RF->ANAMISC_CTRL1.DAC_REFH_ADJ          = buff2[2];// 调df1, df2
            RF->ANAMISC_CTRL1.DAC_REFL_ADJ          = buff2[3];// 调df1, df2            
            RF->ANAMISC_CTRL1.DAC_BLE_DELAY_ADJ_1M  = buff2[4];// 调df1, df2
            RF->PLL_GAIN_CTRL.PLL_VTXD_EXT          = buff2[5];// 调df1, df2
        } break;
        
        default:
        {
            uart_send(1, buff2_len, buff2);
            
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
    #if (USE_UART1)
    uart1_proc();
    #endif
    
    #if (USE_UART2)
    uart2_proc();    
    #endif

    rf_2g4_rx_proc();
}
