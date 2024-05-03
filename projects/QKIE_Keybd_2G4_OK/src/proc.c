/**
 ****************************************************************************************
 *
 * @file proc.c
 *
 * @brief user procedure.
 *
 ****************************************************************************************
 */

#include "regs.h"
#include "drvs.h"
#include "leds.h"
#include "batt.h"
#include "app.h"
#include "gatt_api.h"
#include "bledef.h"
#include "user_api.h"
#include "sftmr.h"

#if (DBG_PROC)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif


/*
 * DEFINES
 ****************************************************************************************
 */

#if(PAIR_KEY_FUNC)
void PairKey_Init(void)
{
    iom_ctrl(PAIR_KEY_GPIO,IOM_PULLUP|IOM_INPUT);
}

void PairKey_Handle(void)
{
    if (powerOn_Work_Status == SYS_PARING)
        return;
    
    if (gpio_get(PAIR_KEY_GPIO) == 0)
    {
        powerOn_Work_Status = SYS_PARING;
        AON->BACKUP1    = 0;
        BtAddr_Add_One();
        BtReset();
        
        GPIO_DAT_CLR(BIT(POWER_LED));
        Leds_Play(LED_BT_OFF);
        Leds_Play(LED_BT_PAIRING);
        DEBUG("PairKey_Press...EnterParing");
    }
}
#endif

void ota_error_repair(void)
{
    powerOn_Work_Status = SYS_PARING;
    BtReset();        
    Leds_Play(LED_BT_OFF);
    Leds_Play(LED_BT_PAIRING);
}

#if (CFG_SLEEP)
static void sleep_proc(void)
{
    #if(!SFT_TIME_SCAN)
    #if (CFG_2G4_MODE)
    if((app_state_get() == (APP_ENCRYPTED - work_mode))&&(!update_connect_par_flag))
    #else
    if((app_state_get() == APP_ENCRYPTED)&&(!update_connect_par_flag))
    #endif // CFG_2G4_MODE
    {
        uint16_t time = (conn_intv*125)*(conn_late+1);
        //DEBUG("time:%x\r\n",time);
        
        if(time<1375)// if time Less than 13.75ms 
        {
            bootDelayMs(10);
            
            kb_scan();
            
            return;
        }
    }
    
    uint32_t slpdur = ble_slpdur_get();
    
    if ((slpdur > BLE_SLP_MS(10)/*320*/) && (ble_sleep(BLE_SLP_TWOSC, BLE_SLP_DURMAX) == BLE_IN_SLEEP))
    {
        uint16_t core_wkup_en = (slpdur < BLE_SLP_MS(50)/*1600*/) ? (CFG_WKUP_BLE_EN) : (CFG_WKUP_BLE_EN | CFG_WKUP_IO_EN);
        
        uint16_t lpret = core_sleep(core_wkup_en);

        if (lpret & WKUP_ST_IO_BIT)
        {
            ble_wakeup();
        }
        
        kb_scan();
    }

    #else
    uint32_t slpdur = ble_slpdur_get();
    if(slpdur>3200)//100ms
    {
        if(ble_sleep(BLE_SLP_TWOSC, BLE_SLP_DURMAX)==BLE_IN_SLEEP)
        {
            uint16_t lpret = core_sleep(CFG_WKUP_BLE_EN|WKUP_PWIO_EN_BIT);
            if(lpret&WKUP_ST_IO_BIT)
            {
                ble_wakeup();
                kb_scan();
                bootDelayMs(10);
                //DEBUG("lpret:%x\r\n",lpret);
            }
        }
    }
    else
    {
        if(ble_sleep(BLE_SLP_TWOSC, BLE_SLP_DURMAX)==BLE_IN_SLEEP)
        {
             core_sleep(CFG_WKUP_BLE_EN);
        }
    }
    #endif
}
#endif 

void user_procedure(void)
{
    #if (UART_CMD)
    uart_proc();
    #endif
    
    sleep_proc();    

    os_judge_proc();
    
    BT_Channle_Set();

    #if(PAIR_KEY_FUNC)    
    PairKey_Handle();
    #endif
  
}
