#include "regs.h"
#include "bledef.h"
#include "drvs.h"
#include "app.h"
#include "sftmr.h"
#include "leds.h"

#include "keyboard.h"
#include "user_api.h"
#include "fcc.h"
#include "hidkey.h"
uint8_t QWE_Data = 0;
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
void EMI_Mode(void)
{
		uint8_t mode_data = 0;
		emi_key_t_press_cnt=0;
		QWE_Data = 0;
		bootDelayMs(90);
		GPIO->DIR_SET = EMI_COL;
		GPIO->DAT_CLR = EMI_COL;
		bootDelayMs(10);
		//uint32_t io_data =(gpio_get_all()&EMI_ROW);
		//debug("io_data:%x\r\n",io_data);
		
		if((gpio_get_all()&EMI_ROW)<EMI_ROW)
		{
			//debug("KEY_OCUR\r\n");
			mode_data = 1;
		}
		if(!mode_data){return;}
		for(uint8_t i=0;i<3;i++)
		{
			kb_scan();
			//bootDelayMs(10);
			if( QWE_Data ==7)break;
		}
		//debug("check_qwe\r\n");
		
		if( QWE_Data ==7)
		{
			DEBUG("EMI_ENTER\r\n");
			
		}
		else
		{
			return;
		}
		GPIO_DAT_CLR(BIT(POWER_LED));
		Leds_Play(LED_BT_PAIRING);
		g_is_emi_mode = true;
		fcc_init();
		GLOBAL_INT_START();
        fcc_tx_carr(0);
        emi_key_t_press_cnt=1;
		while(1)
		{
		   // SoftTimer Polling
           sftmr_schedule();
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
        Leds_Play(LED_BT_ON);						
        Leds_Play(LED_BT_CONNECT_BACK);
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
			Leds_Play(LED_BT_PAIRING);
        } break;
        
        default:
        {

        } break;
    }
    
}


