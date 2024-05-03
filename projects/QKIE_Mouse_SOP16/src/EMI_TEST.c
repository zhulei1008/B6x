#include "regs.h"
#include "bledef.h"
#include "drvs.h"
#include "app.h"
#include "sftmr.h"
#include "leds.h"
#include "mouse.h"

#include "user_api.h"
#include "fcc.h"
#include "hidkey.h"
uint8_t emi_key_t_press_cnt = 0;
volatile bool g_is_emi_mode = false;
#if (DBG_EMI_TEST)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<EMI_TEST>" format "\r\n", ##__VA_ARGS__)
#define debugHex(dat, len)
#else
#define DEBUG(format, ...)
#endif

/*
 * DEFINES
 ****************************************************************************************
 */

#define BLE_MAX_LEN  20
#define NULL_CNT     20
#define HOPPING_INV  _MS(200)
extern uint8_t mouse_key_scan(void);
void emi_event(uint8_t evt_id);
void EMI_Mode(void)
{

		emi_key_t_press_cnt=0;
        uint8_t key_data = 0;
        uint8_t key_data_back = 0;
		uint8_t r0r1r2 = mouse_key_scan();
		if( r0r1r2 ==(BTN_L|BTN_R|BTN_M))
		{
			DEBUG("EMI_ENTER\r\n");
			
		}
		else
		{
			return;
		}
       
		while(mouse_key_scan()){;}
        DEBUG("relase");
        g_is_emi_mode = true;
		fcc_init();
		GLOBAL_INT_START();
        fcc_tx_carr(0);
        leds_play(LED_BUSY_BL);
        
		while(1)
		{
		   // SoftTimer Polling
           sftmr_schedule();
           key_data=mouse_key_scan();
           if(key_data_back!=key_data)
           {
               key_data_back = key_data;
               if(key_data_back==BTN_L)
               {
                    emi_key_t_press_cnt++;
                    emi_event(emi_key_t_press_cnt);
               }
           }
           
		}
}


enum emi_cmd
{
    //    CMD_FCC_START= 0,
    //    CMD_FCC_STOP,

    CMD_FCC_TX_CARR1=0,
    CMD_FCC_TX_CARR2,
    CMD_FCC_TX_CARR3,
    //CMD_FCC_RX_CARR,

    CMD_FCC_TX_MOD1,
    CMD_FCC_TX_MOD2,
    CMD_FCC_TX_MOD3,

    CMD_FCC_RX_MOD1,
    CMD_FCC_RX_MOD2,
    CMD_FCC_RX_MOD3,

    CMD_FCC_TX_HOP,

    //    CMD_SET_XOSC16_TR = 0xE0,
    //    CMD_GET_XOSC16_TR,
    EMI_MODE_MAX
};


volatile uint8_t g_hopping_idx, g_hopping_timer_id;

static tmr_tk_t hopping_timer(uint8_t id)
{
    // 0 ~ 39 (2402M ~ 2480M)
    g_hopping_idx = (g_hopping_idx % 40);

    fcc_tx_carr(g_hopping_idx);
    
    ++g_hopping_idx;
    
    return HOPPING_INV;
}

static void hopping_mode(void)
{
    if ((g_hopping_timer_id & 0x80) == 0)
    {
        g_hopping_timer_id = sftmr_start(HOPPING_INV, hopping_timer);
        
        g_hopping_timer_id |= 0x80;
    }
}
/*
 * FUNCTIONS
 ****************************************************************************************
 */
/// Uart Data procedure
void emi_event(uint8_t evt_id)
{
    // Todo Loop-Proc
    evt_id %= EMI_MODE_MAX;
    
    DEBUG("evt_id:%d", evt_id);


    
    if ((evt_id != CMD_FCC_TX_HOP) && (g_hopping_timer_id != 0))
    {
        g_hopping_timer_id &= ~0x80U;
        sftmr_clear(g_hopping_timer_id);
        g_hopping_timer_id = 0;

    }
    if(evt_id != CMD_FCC_TX_HOP)
    {
        leds_play(LED_CONT_ON);						
        leds_play(LED_HINT_BL);
    }
    switch (evt_id)
    {
        case CMD_FCC_TX_CARR1:
        {
            DEBUG("fcc_tx_carr1");  
						
            fcc_tx_carr(0);

        } break;
		case CMD_FCC_TX_CARR2:
        {
            DEBUG("fcc_tx_carr2");            
            fcc_tx_carr(19);

        } break;
		case CMD_FCC_TX_CARR3:
        {
            DEBUG("fcc_tx_carr3");           
            fcc_tx_carr(39);

        } break;
         
        case CMD_FCC_TX_MOD1:
        {
            DEBUG("fcc_tx_mod1");
            fcc_tx_mod(0);

        } break;
        case CMD_FCC_TX_MOD2:
        {
            DEBUG("fcc_tx_mod2");
            fcc_tx_mod(19);

        } break;
		case CMD_FCC_TX_MOD3:
        {
            DEBUG("fcc_tx_mod3");
            fcc_tx_mod(39);

        } break;
				
        case CMD_FCC_RX_MOD1:
        {
            DEBUG("fcc_rx_mod1");
            fcc_rx_mod(0);

        } break;
        case CMD_FCC_RX_MOD2:
        {
            DEBUG("fcc_rx_mod2");
            fcc_rx_mod(19);

        } break;
		case CMD_FCC_RX_MOD3:
        {
            DEBUG("fcc_rx_mod3");
            fcc_rx_mod(39);

        } break;
				
        case CMD_FCC_TX_HOP:
        {
            DEBUG("hopping_mode");
            g_hopping_idx = 0;
            hopping_mode();
			leds_play(LED_FAST_BL);
        } break;
        
        default:
        {

        } break;
    }
    
}


