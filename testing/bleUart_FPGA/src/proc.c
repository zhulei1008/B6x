/**
 ****************************************************************************************
 *
 * @file proc.c
 *
 * @brief user procedure.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "bledef.h"
#include "drvs.h"
#include "regs.h"

#include "app.h"
#include "prf_sess.h"
#include "uartRb.h"


#if (DBG_PROC)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<PROC>" format "\r\n", ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif


#define UART_PORT             0  //UART1 
#define GPIO_TX               5
#define GPIO_RX               6

#define BLE_MAX_LEN           20
#define NULL_CNT              20
static uint8_t buff[BLE_MAX_LEN];
static uint16_t buff_len = 0;

void gapm_set_channel_map_cmd(le_chnl_map_t *chan_map);

/// Override - Callback on received data from peer device
//void sess_cb_rxd(uint8_t conidx, uint16_t len, const uint8_t *data)
//{
//    DEBUG("rxd(cid:%d,len:%d)", conidx, len);
//    debugHex(data, len);
////    APBMISC->BLE_FINECNT_BOUND = read32p(data);
//}

/// Uart Data procedure
static void data_proc(void)
{
    // Todo Loop-Proc
    static uint8_t null_cnt = 0;
    uint16_t len;

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
        case 0xA1:
        {
            le_chnl_map_t set_chan_map = {0x00, 0x0F, 0xFF, 0xFF, 0x00};
            gapm_set_channel_map_cmd(&set_chan_map);
            
            buff_len = 0;
        } break;
        
        default:
        {
            sess_txd_send(0, buff_len, buff);
            buff_len = 0;
        } break;
    }
}

static void sleep_proc(void)
{
    uint8_t lpsta = ble_sleep(BLE_SLP_TWOSC, BLE_SLP_DURMAX);

    if (lpsta == BLE_IN_SLEEP)
    {
        GPIO->DAT_SET = BIT(PAD_TIM_PIO);
        uint32_t lpcyc = RD_32(0x50000034);
        uint32_t lpcnt = RD_32(0x50000038);
        uint32_t sync_time = APBMISC->BLE_SYNC_TIME;
        GPIO->DAT_CLR = BIT(PAD_TIM_PIO);
        uint16_t lpret = core_sleep(CFG_WKUP_BLE_EN);
        
        lpsta = APBMISC->BLE_LP_CTRL.BLE_DP_VALID;
        
        DEBUG("ble sta:%d,%X,%X, wksrc:%X, sync_time:%d", lpsta, lpcyc, lpcnt, lpret, sync_time);
        //DEBUG("aon sta:%X,ctrl:%X", AON->PMU_WKUP_ST.Word, AON->PMU_WKUP_CTRL.Word);
    }
    else
    {
        //DEBUG("ble sta:%d", lpsta);
    }
}

void user_procedure(void)
{
    #if (CFG_SLEEP)
    sleep_proc();
    #endif
    
    data_proc();
}
