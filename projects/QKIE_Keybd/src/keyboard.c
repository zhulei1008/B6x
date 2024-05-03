#include "regs.h"
#include "drvs.h"
#include "bledef.h"
#include "app.h"
#include "app_actv.h"
#include "gapc_api.h"
#include "gapm_api.h"
#include "sftmr.h"
#include "hidkey.h"
#include "keyboard.h"
#include "dbg.h"
#include "hid_desc.h"
#include "gpio.h"
#include "leds.h"
#include "batt.h"
#include "user_api.h"

/*
 * DECLARATION
 ****************************************************************************************
 */
uint8_t sys_poweroff_flag;
bool btChannleChangeFlag;

/*
 * DEFINES
 ****************************************************************************************
 */

#if (KEY_BOARD)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#endif

const uint8_t Key_Map[KEY_ROW_NB][KEY_COL_NB] =
{
	{	KEY_F5,	  KEY_1,	  KEY_2,		 KEY_3,	 KEY_4,	KEY_7, KEY_8,		 KEY_9,		 KEY_0,			KEY_F12,   KEY_F10,		  0X00,			KEY_RIGHT, 0X00,	 0X00,		 0X00		},
	{	KEY_FN,	  KEY_ACCENT, KEY_F1,		 KEY_F2, KEY_5,	KEY_6, KEY_EQUAL,	 KEY_F8,	 KEY_MINUS,		0X00,	   KEY_F9,		  KEY_DELETE,	KEY_RIGHT, KEY_TAB,  0X00,		 0X00       },
	{	0X00,	  0X00,		  0X00,			 0X00,	 KEY_B,	KEY_N, 0X00,	     0X00,		 KEY_SLASH,		KEY_RGUI,  0X00,		  KEY_DOWN,		KEY_RIGHT, KEY_LEFT, 0X00,		 0X00		},
	{	KEY_RALT, KEY_Z,	  KEY_X,		 KEY_C,	 KEY_V,	KEY_M, KEY_COMMA,	 KEY_PERIOD, 0X00,			0X00,	   KEY_ENTER,	  KEY_F11,		KEY_RIGHT, KEY_S,	 0X00,		 0X00		},
	{	0X00,	  KEY_ESC,	  0X00,			 KEY_F4, KEY_G,	KEY_H, KEY_F6,		 0X00,	     KEY_QUOTE,	    KEY_LGUI,  0X00,		  KEY_SPACEBAR, KEY_RIGHT, KEY_UP,	 0X00,		 0X00		},
	{	0X00,	  KEY_A,	  KEY_S,		 KEY_D,	 KEY_F,	KEY_J, KEY_K,		 KEY_L,	     KEY_SEMICOLON, KEY_LCTRL, KEY_BACKSLASH, 0X00,			KEY_RIGHT, 0X00,	 KEY_RSHIFT, 0X00		},
	{	0X00,	  KEY_TAB,	  KEY_CAPS_LOCK, KEY_F3, KEY_T,	KEY_Y, KEY_RBRACKET, KEY_F7,	 KEY_LBRACKET,	0X00,	   KEY_BACKSPACE, 0X00,			KEY_RIGHT, 0X00,	 KEY_LSHIFT, KEY_LALT	},
	{	0X00,	  KEY_Q,	  KEY_W,		 KEY_E,	 KEY_R,	KEY_U, KEY_I,		 KEY_O,	     KEY_P,			0X00,	   0X00,		  0X00,			KEY_RIGHT, 0X00,	 0X00,		 0X00		}
};

const uint8_t Key_Col[KEY_COL_NB] =
{
    15, 17, 16,   					 // column c0-c2
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF    
};

const uint8_t Key_Row[KEY_ROW_NB] =
{
    10,  12, 19, 13, 18, 14, 11, 9   // row0~7
};

#define ALL_COL ((0x1<<15)|(0x1<<17)|(0x1<<16))
#define ALL_ROW ((0x1<<10)|(0x1<<12)|(0x1<<19)|(0x1<<13)|(0x1<<18)|(0x1<<14)|(0x1<<11)|(0x1<<9))
#define EXTEND_COL_CONTROL_PIN	(1<<5)  //INTÐÅºÅIO
#define EXTEND_POWER_PIN (1<<8)

const uint8_t Row_Idx[KEY_ROW_NB] =
{
    1, 0, 2, 3, 4, 5, 6, 7
};
const uint8_t Col_Idx[KEY_COL_NB]={0,1,2,3,4,5,7,8,6,15,9,13,10,14,12,11};
struct key_env_tag key_env;

static uint8_t updata_parm_data = 0; 
static uint16_t key_status_cont = 0;
static uint16_t update_time_cnt = 0;
uint8_t update_connect_par_flag;
uint8_t updata_conn_status;
static uint8_t  update_count     = 0;
static void ghost_detect(void);

#if (CFG_PIN_ENABLE)
volatile uint8_t g_pin_num;
volatile uint32_t g_pin_code;
#endif

/*
 * FUNCTIONS
 ****************************************************************************************
 */

static void key_code(uint8_t row, uint8_t col)
{
    uint8_t kcode = Key_Map[row][col];
    //debug("kcode:%x\r\n",kcode);
	
    // Modifier Keys
    if ((kcode >= KEY_LCTRL) && (kcode < KEY_FN))
    {
        key_env.report[0] |= (1 << (kcode - KEY_LCTRL));
        key_env.keyTyp = KT_KEYBOARD;
        return;
    }

    // Fn Key
    if (kcode == KEY_FN)
    {
        key_env.fnFlag = true;

        key_env.keyCnt = 0;
        return;
    }

    key_env.mkCode = 0;

    if (kcode == KEY_CAPS_LOCK)
    {
        key_env.capsFlag = true;
    }
		if ((kcode == KEY_Q) || (kcode == KEY_W) || (kcode == KEY_E))
		{
			//QWE_Data+=kcode;
			if(kcode == KEY_Q)QWE_Data|=1;
			else if(kcode == KEY_W)QWE_Data|=2;
			else if(kcode == KEY_E)QWE_Data|=4;
		}
    #if (CFG_PIN_ENABLE)
    if ((powerOn_Work_Status == SYS_PARING) && ((kcode > KEY_Z) && (kcode < KEY_ENTER)))
    {
        key_env.mkCode = kcode;
        key_env.keyTyp = KT_LOCAL;
    }
    // Generic Keys, max=6
    else if (key_env.keyCnt < 6)
    #else
    if (key_env.keyCnt < 6)
    #endif    
    {
        key_env.keyTyp = KT_KEYBOARD;
        key_env.report[2 + key_env.keyCnt] = kcode;
        key_env.keyCnt++;
    }
}

#if (KEY_GHOST)
static void ghost_detect(void)
{
    uint8_t r, c, colSta;
    // check whether 'L' style or not
    for (r = 0; r < KEY_ROW_NB; r++)
    {
        if (key_env.rowCnt[r] >= 2)
        {
            uint8_t rowBit = (1 << r);
            for (c = 0; c < KEY_COL_NB; c++)
            {
                colSta = key_env.colSta[c];

                if ((colSta & (1 << r))&&(colSta>(rowBit)))
                {
//                    if ((rowBit & colSta) != (1 << r))
//                    {
                        key_env.keyTyp = KT_GHOST;
                        memset(key_env.report, 0, 8);
                        //DEBUG("ghost at(c:%d,r:0x%x)", c, colSta);
                        return;
//                    }
//                    else
//                    {
//                        rowBit |= colSta;
//                    }
                }
            }
        }
    }
}
#else
#define ghost_detect()
#endif

static void col_scan(uint8_t col)
{
    bool press;
    // 1. GPIO Output LOW
	
	if(col < 3)
	{
		GPIO->DIR_SET = 0x1<<(Key_Col[col]);
		GPIO->DAT_CLR = 0x1<<(Key_Col[col]);
		btmr_delay(16,10);
	}

    // 2. Detect column by column
    for (uint8_t row = 0; row < KEY_ROW_NB; row++)
    {
		#if (KEY_DEBOUNCE)
        // Shift right to update
        key_env.shake[row][col] <<= 1;

        if (gpioGetDataBit(Key_Col[col]) != 0)
        {
            key_env.shake[row][col] |= 1;
        }

        // Judge real state
        press = ((key_env.shake[row][col] & KEY_DEBOUNCE) == KEY_DEBOUNCE)
                || (((key_env.shake[row][col] & KEY_DEBOUNCE) != 0) &&
                    (key_env.colSta[col] & (1 << row)));
		#else
        press = ((GPIO_PIN_GET()&(1<<Key_Row[row])) == 0);			
		#endif

        if (press && Key_Map[row][col])    
        {
            //debug("(r:%d,c:%d,)\r\n", row, col);
            key_env.state = KS_PRESS;

			#if (KEY_GHOST)
            // Inc count
            key_env.rowCnt[row]++;
			#endif
			
			#if (KEY_GHOST || KEY_DEBOUNCE)
            // Set bit of state
            key_env.colSta[col] |= (1 << row);
			#endif

            // Key code
            key_code(row, col);
        }
		#if (KEY_GHOST || KEY_DEBOUNCE)
        else
        {
            // Clr bit of state
            key_env.colSta[col] &= ~(1 << row);
        }
		#endif
    }
	// 3. GPIO Restore HiZ
	if(col < 3)
	{
		GPIO->DIR_CLR = 0x1 << (Key_Col[col]);
		//iom_ctrl(Key_Col[col],/*IOM_PULLUP|*/IE_UP);
		iom_ctrl(Key_Col[col],IOM_HIZ);
	}
}

static void kb_send_report(uint8_t type, const uint8_t *report)
{
	uint8_t conidx = app_env.curidx;
	
	if ((app_state_get() < APP_ENCRYPTED)||(sys_poweroff_flag>0))	
		return;
	
    switch(type)
    {
        case KT_KEYBOARD:
        {
			keybd_report_send(conidx,report);												
        }
        break;

        case KT_MEDIA:
        {
			media_report_send(conidx,report);			
        }
        break;

        case KT_POWER:
        {
			system_report_send(conidx,report);
        }
        break;

        default:
            break;
    }
}

void wakeup_report(void)
{
    if ((AON->BACKUP1 == 0x03) && (key_env.state != KS_PRESS))
    {
        uint8_t key_tpye  = KT_POWER;
        
        if (key_env.sys >= SYS_IOS)
        {
            key_tpye          = KT_KEYBOARD;
            key_env.report[0] = KEY_BIT_LCTRL;
        }
        else
        {
            key_env.report[0] = SK0_BIT1_WAKEUP;
        }
        
        kb_send_report(key_tpye, key_env.report);
        kb_send_report(key_tpye, NULL);       
        DEBUG("Send wake report:%d, rep:%x", key_env.sys, key_env.report[0]);
    }
	
	AON->BACKUP1    = 0;
}

void main_fun_delay(uint16_t cnt)
{
	uint16_t cont1,cont2;
	for(cont1=0;cont1<cnt;cont1++)
	{
		for(cont2=0;cont2<100;cont2++);
	}
}
void main_fun_delay_H(uint16_t cnt)
{
	uint16_t cont1,cont2;
	for(cont1=0;cont1<cnt;cont1++)
	{
		for(cont2=0;cont2<50;cont2++);
	}
}

const struct gapc_conn_param key_le_conn_pref1 =
{
    /// Connection interval minimum unit in 1.25ms
    .intv_min = 12,
    /// Connection interval maximum unit in 1.25ms
    .intv_max = 12,
    /// Slave latency
    .latency  = 0,
    /// Connection supervision timeout multiplier unit in 10ms
    .time_out = 300,
};
const struct gapc_conn_param key_le_conn_pref2 =
{
    /// Connection interval minimum unit in 1.25ms
    .intv_min = 12,
    /// Connection interval maximum unit in 1.25ms
    .intv_max = 12,
    /// Slave latency
    .latency  = 9,
    /// Connection supervision timeout multiplier unit in 10ms
    .time_out = 300,
};

void keyboard_poweroff(void)
{
	DEBUG("enterpoweroff\r\n");
	for (uint8_t j = 0; j < 3; j++)
	{
		GPIO->DIR_CLR = 0x1 << (Key_Col[j]);
		GPIO->DIR_SET = 0x1 << (Key_Col[j]);
		GPIO->DAT_CLR = 0x1 << (Key_Col[j]);
		iom_ctrl(Key_Col[j],IOM_PULLDOWN);			
	}	
	
	GPIO->DAT_CLR = ALL_LED;

	#if(DBG_MODE)
	GPIO->DIR_CLR = 0x1<<(6); // tx
	iom_ctrl(6, IOM_HIZ);
	GPIO->DIR_CLR =  0x1<<(7);; // rx
	iom_ctrl(7, IOM_HIZ);
	#endif
	
	bootDelayMs(10);
	wakeup_io_sw(ALL_ROW|EXTEND_COL_CONTROL_PIN|BIT(PAIR_KEY_GPIO), ALL_ROW|EXTEND_COL_CONTROL_PIN|BIT(PAIR_KEY_GPIO));	
	core_pwroff(CFG_WKUP_IO_EN | WKUP_IO_LATCH_N_BIT);
}

void keyboard_go_poweroff_jage(void)
{
	if(app_state_get()>=APP_ENCRYPTED)
	{
		keybd_report_send(app_env.curidx ,NULL);
		DEBUG("rerase_keb");
		media_report_send(app_env.curidx ,NULL);
		DEBUG("rerase_mid");
		sys_poweroff_flag = 1;
		ke_timer_set(APP_DISCONNECT_TIME,TASK_APP,1000);		
	}
	else
	{
		keyboard_poweroff();
	}
}

//×éºÏ¼ü
void Combinationkey (void)
{
	uint8_t kcode = key_env.report[2];
	if(key_env.keyTyp == KT_GHOST)return;
    if (key_env.fnFlag)
    {
		#if(DBG_MODE)
        if(kcode==KEY_Z)
		{
			iospc_rstpin(0);
			iospc_swdpin();
			return;
		}
		if(kcode==KEY_X)
		{
			iom_ctrl(PA_SWCLK,IOM_SEL_GPIO);
			iom_ctrl(PA_SWDIO,IOM_SEL_GPIO);
			iospc_rstpin(1);
			return;
		}
        #endif
		
//        // Fn + C: bt connectable
//        if (kcode == KEY_C)
//        {
//            key_env.mkCode = kcode;
//            key_env.keyTyp = KT_LOCAL;
//            return;
//        }
		
		// Fn + N£ºdelete pair		
		if (kcode == KEY_N)
		{
            key_env.mkCode = kcode;
            key_env.keyTyp = KT_LOCAL;
            return;			
		}

		//Fn + 1/2/3:change os
//		if ((kcode >= KEY_1) && (kcode <= KEY_3))
//		{			
//			key_env.mkCode = kcode;
//			key_env.keyTyp = KT_LOCAL;
//			ghost_detect();
//			if(key_env.keyTyp != KT_GHOST)
//			{
//				uint8_t channle = 0;
//				switch(kcode)
//				{
//					case KEY_1:channle = 0;break;
//					case KEY_2:channle = 1;break;
//					case KEY_3:channle = 2;break;
//					default:break;
//				}
//				if(channle != channle_Select)
//				{
//					btChannleChangeFlag = true;
//					channle_Select = channle;
//					AON->BACKUP1    = 3;
//				}
//				
//				Leds_Play(LED_BT_OFF);
//				Leds_Play(LED_BT_CHANGE_CHANNLE);
//				
//				DEBUG("change channle_channle_Select:");
//				debugHex((uint8_t *)&channle_Select, 4);
//			}
//			return;
//		}
		
        // Fn + Q/W/E: change os
        if ((kcode == KEY_Q) || (kcode == KEY_W) || (kcode == KEY_E))
        {
            key_env.mkCode = kcode;
            key_env.keyTyp = KT_LOCAL;
			
			ghost_detect();
			
			if(key_env.keyTyp != KT_GHOST)
			{
				key_env.sys = (kcode == KEY_Q) ? ANDROID : ((kcode == KEY_W) ? WINDOWS : SYS_IOS);
				Leds_Play(LED_BT_OFF);
				Leds_Play(LED_BT_CHANGE_OS);
		
				if (os_mode_table[channle_Select&0xff] != key_env.sys)
				{
					os_mode_table[channle_Select&0xff] = key_env.sys;
					GLOBAL_INT_DISABLE();
					fshc_erase(OS_MODE_FLASH_ADDR, FSH_CMD_ER_PAGE);
					// store to flash
					fshc_write(OS_MODE_FLASH_ADDR, (uint32_t *)&os_mode_table, 64, FSH_CMD_WR);       
					GLOBAL_INT_RESTORE();
				}
	
				DEBUG("change os_key_env.sys:%d",key_env.sys);	
			}
			return;
        }

		// Fn + Up/Down/Left/Right
        if ((kcode >= KEY_RIGHT) && (kcode <= KEY_UP))
        {
            key_env.mkCode = kcode;
            key_env.keyTyp = KT_KEYBOARD;
            if (key_env.sys >= SYS_IOS)
            {
                key_env.report[0] = 0x08;
                key_env.report[1] = 0x00;
                key_env.report[2] = (kcode == KEY_UP) ? KEY_UP :
                                    ((kcode == KEY_DOWN) ? KEY_DOWN :
                                     ((kcode == KEY_LEFT) ? KEY_LEFT : KEY_RIGHT));
            }
            else
            {
                key_env.report[0] = 0x00;
                key_env.report[1] = 0x00;
                key_env.report[2] = (kcode == KEY_UP) ? KEY_PAGE_UP :
                                    ((kcode == KEY_DOWN) ? KEY_PAGE_DOWN :
                                     ((kcode == KEY_LEFT) ? KEY_HOME : KEY_END));
            }
            return;
        }
				
        //Fn+LCtrl ÐéÄâ¼üÅÌ
        if(key_env.report[0] == KEY_BIT_LCTRL)
        {
            key_env.mkCode = kcode;
            key_env.keyTyp = KT_NONE;
			
            if(key_env.sys == WINDOWS)
            {
                key_env.report[0] = KEY_BIT_LCTRL | KEY_BIT_LGUI;
				key_env.report[1] = 0x00;
                key_env.report[2] = KEY_O;
                key_env.keyTyp = KT_KEYBOARD;
				DEBUG("FN+LCTRL...");
            }
            else if(key_env.sys >= SYS_IOS)
            {
				key_env.report[0] = 0x00;
                key_env.report[1] = MK1_BIT6_VIRKB;
                key_env.keyTyp = KT_MEDIA;
            }
            else
            {
				key_env.report[0] = 0x00;
				key_env.report[1] = 0x00;
                key_env.report[2] = MK2_BIT2_AND_VIRKB;
                key_env.keyTyp = KT_MEDIA;
            }
            return;
		}
		
//        //FN+¿Õ¸ñ ÊäÈë·¨ÇÐ»»
//        if(kcode == KEY_SPACEBAR)
//        {
//            key_env.mkCode = kcode;
//			key_env.keyTyp = KT_NONE;
//			
//            if(key_env.sys >= SYS_IOS)
//            {
//                key_env.report[0] = KEY_BIT_LCTRL;
//                key_env.report[2] = KEY_SPACEBAR;
//                key_env.keyTyp = KT_KEYBOARD;
//            }
//            else if(key_env.sys == ANDROID)
//            {
//                key_env.report[0] = KEY_BIT_LSHIFT;
//                key_env.report[2] = KEY_SPACEBAR;
//                key_env.keyTyp = KT_KEYBOARD;
//            }
//            else if(key_env.sys == WINDOWS)
//            {
//                key_env.report[0] = KEY_BIT_LGUI;
//                key_env.report[2] = KEY_SPACEBAR;
//                key_env.keyTyp = KT_KEYBOARD;
//            }
//            return;
//        }		
		if (kcode == KEY_ESC)
		{
			key_env.report[0] = 0x00;
			key_env.report[2] = KEY_ESC;
			key_env.keyTyp = KT_KEYBOARD;
			return;				
		}
		
		if ((kcode >= KEY_F1) && (kcode <= KEY_F12))
		{
			key_env.report[0] = 0x00;
			key_env.report[2] = kcode;
			key_env.keyTyp = KT_KEYBOARD;
			return;				
		}
		
		if (kcode == KEY_DELETE)
		{
			key_env.report[0] = 0x00;
			key_env.report[2] = KEY_DELETE;
			key_env.keyTyp = KT_KEYBOARD;
			return;					
		}

    }//FN+...
	else
	{		
        if ((kcode >= KEY_F1) && (kcode <= KEY_F12))
        {
            key_env.mkCode = kcode;
            key_env.keyTyp = KT_NONE;

            key_env.report[0] = 0x00;
            key_env.report[1] = 0x00;
            key_env.report[2] = 0x00;

            if (kcode == KEY_F1)
            {
                if (key_env.sys >= SYS_IOS)
                {
                    key_env.report[0] = KEY_BIT_LGUI;
                    key_env.report[2] = KEY_SPACEBAR;
					key_env.keyTyp = KT_KEYBOARD;
                }
                else
                {
					key_env.report[0] = MK0_BIT2_WWW_SEARCH;
					key_env.keyTyp = KT_MEDIA;
                }
            }
            else if (kcode == KEY_F2)
            {
                if (key_env.sys >= SYS_IOS)
                {
					key_env.report[0] = KEY_BIT_LGUI;
                    key_env.report[2] = KEY_A;
					key_env.keyTyp = KT_KEYBOARD;
                }
                else
                {
					key_env.report[0] = KEY_BIT_LCTRL;
                    key_env.report[2] = KEY_A;
					key_env.keyTyp = KT_KEYBOARD;
                }
            }
            else if (kcode == KEY_F3)
            {
                if (key_env.sys >= SYS_IOS)
                {
					key_env.report[0] = KEY_BIT_LGUI;
                    key_env.report[2] = KEY_C;
					key_env.keyTyp = KT_KEYBOARD;
                }
                else
                {
					key_env.report[0] = KEY_BIT_LCTRL;
                    key_env.report[2] = KEY_C;
					key_env.keyTyp = KT_KEYBOARD;
                }
            }
            else if (kcode == KEY_F4)
            {
                if (key_env.sys >= SYS_IOS)
                {
					key_env.report[0] = KEY_BIT_LGUI;
                    key_env.report[2] = KEY_V;
					key_env.keyTyp = KT_KEYBOARD;
                }
                else
                {
					key_env.report[0] = KEY_BIT_LCTRL;
                    key_env.report[2] = KEY_V;
					key_env.keyTyp = KT_KEYBOARD;
                }
            }
            else if (kcode == KEY_F5)
            {
                if (key_env.sys >= SYS_IOS)
                {
					key_env.report[0] = KEY_BIT_LGUI;
                    key_env.report[2] = KEY_X;
					key_env.keyTyp = KT_KEYBOARD;
                }
                else
                {
					key_env.report[0] = KEY_BIT_LCTRL;
                    key_env.report[2] = KEY_X;
					key_env.keyTyp = KT_KEYBOARD;
                }
            }
            else if (kcode == KEY_F6)
            {
				key_env.report[0] = MK0_BIT6_PREV_TRK;
				key_env.keyTyp = KT_MEDIA;
            }
            else if (kcode == KEY_F7)
            {
				key_env.report[0] = MK0_BIT7_START_PAUSE;
				key_env.keyTyp = KT_MEDIA;
            }	
            else if (kcode == KEY_F8)
            {
				key_env.report[1] = MK1_BIT0_NEXY_TRK;
				key_env.keyTyp = KT_MEDIA;
            }	
            else if (kcode == KEY_F9)
            {
				key_env.report[1] = MK1_BIT2_VOL_DN;
				key_env.keyTyp = KT_MEDIA;
            }
            else if (kcode == KEY_F10)
            {
				key_env.report[1] = MK1_BIT3_VOL_UP;
				key_env.keyTyp = KT_MEDIA;
            }	
            else if (kcode == KEY_F11)
            {
				key_env.report[0] = MK0_BIT0_LIGHT_DN;
				key_env.keyTyp = KT_MEDIA;
            }
            else if (kcode == KEY_F12)
            {
				key_env.report[0] = MK0_BIT1_LIGHT_UP;
				key_env.keyTyp = KT_MEDIA;
            }				
            return;
        }

		if (kcode == KEY_ESC)
        {
            //key_env.mkCode = kcode;
            key_env.keyTyp = KT_MEDIA;

            key_env.report[0] = 0x00;
            key_env.report[1] = MK1_BIT7_HOME;
            key_env.report[2] = 0x00;
            return;
        }
		
		if (kcode == KEY_DELETE)
		{
			if (key_env.sys >= SYS_IOS)
			{           
				key_env.report[0] = KEY_BIT_LCTRL | KEY_BIT_LGUI;
				key_env.report[2] = KEY_Q;
				key_env.keyTyp = KT_KEYBOARD;
			}
			else if(key_env.sys == WINDOWS)
			{           
				key_env.report[0] = KEY_BIT_LGUI;
				key_env.report[2] = KEY_L;
				key_env.keyTyp = KT_KEYBOARD;
			}
			else
			{
				key_env.report[2] = MK2_BIT7_POWER;
				key_env.keyTyp = KT_MEDIA;				
			}
			return;
		}		
	}
}

void kb_scan(void)
{
	key_env.keyCnt = 0;
	key_env.state = KS_RELEASE;
	key_env.capsFlag = false;
	key_env.fnFlag = false;
	key_env.keyTyp = KT_NONE;
	memset(key_env.report, 0, 8);
    
	// HiZ the Col GPIOs
	for (uint8_t i = 0; i < 3; i++)
	{
		GPIO->DIR_CLR = 0x1 << (Key_Col[i]);
		iom_ctrl(Key_Col[i],IOM_HIZ);
	}

	GPIO->DAT_SET = EXTEND_COL_CONTROL_PIN;
	GPIO->DIR_SET = EXTEND_COL_CONTROL_PIN;
	
	#if(CFG_SLEEP||CFG_POWEROFF)
	GPIO->DAT_CLR = EXTEND_COL_CONTROL_PIN;
	main_fun_delay(2);
	GPIO->DAT_SET = EXTEND_COL_CONTROL_PIN;
	#endif
	
	#if (KEY_GHOST)
	// Init count
	for (uint8_t r = 0; r < 8; r++)
	{
		key_env.rowCnt[r] = 0;
	}	
	for(uint8_t c = 0; c < 16; c++)
	{
		key_env.colSta[c]=0;
	}
	#endif
	
    // Scan row by row KEY_COL_NB
    for (uint8_t i = 0; i < KEY_COL_NB; i++)
    {
		if(i > 2)
		{
			#if(FUN_MODE==MAIN_MODE)
			GPIO->DAT_CLR = EXTEND_COL_CONTROL_PIN;
			main_fun_delay(1);//3
			#else
			GPIO->DAT_SET=EXTEND_COL_CONTROL_PIN;
			int_fun_delay(1);
			GPIO->DAT_CLR=EXTEND_COL_CONTROL_PIN;
			int_fun_delay(1);
			#endif
		}
		
		col_scan(Col_Idx[i]);
			
		#if(FUN_MODE==MAIN_MODE)
		if(i > 2)
		{
			GPIO->DAT_SET = EXTEND_COL_CONTROL_PIN;
			main_fun_delay(1);
		}
		#endif
    }

	GPIO->DIR_SET = ALL_COL;
	GPIO->DAT_CLR = ALL_COL;
		
    // Check ghost key
    ghost_detect();

		Combinationkey();
	
    // Report changed or repeat
    if (key_env.keyTyp != key_env.keyTyp0)
    {
        key_env.repCnt = 0;
        
        if (key_env.keyTyp0 >= KT_KEYBOARD)
        {
            // Release last report
            if (key_env.keyTyp != KT_GHOST)
            {
                kb_send_report(key_env.keyTyp0, NULL);
							  #if (CFG_EMI_MODE)
                if ((g_is_emi_mode == true) && (key_env.report0[2] == KEY_T))
                {
                   
                    DEBUG("EMI_Mode:(evt:%d)", emi_key_t_press_cnt);
                    emi_event(emi_key_t_press_cnt);
                    ++emi_key_t_press_cnt;

                }
                #endif
            }
        }
        else // Ignore diff type
        {
            if (key_env.keyTyp >= KT_KEYBOARD)
            {
                // Press new report
                kb_send_report(key_env.keyTyp, key_env.report);
            }
        }

        if (key_env.keyTyp != KT_GHOST)
        {
            // backup report
            key_env.keyTyp0 = key_env.keyTyp;
            memcpy(key_env.report0, key_env.report, 8);
        }
        
        #if (CFG_PIN_ENABLE)
        if ((key_env.keyTyp == KT_LOCAL) && (powerOn_Work_Status == SYS_PARING))
        {
            // 0 ~ 9
            uint8_t num = (key_env.mkCode - KEY_Z) % 10;
            g_pin_code *= 10;
            g_pin_code += num;
//            gapc_key_press_notif_cmd(TASK_ID(GAPC, app_env.curidx), num);
            g_pin_num++;
            DEBUG("TK_KEY:%06d, g_pin_num:%d, num:%d", g_pin_code, g_pin_num, num);
        }
        #endif
    }
    else if (memcmp(key_env.report, key_env.report0, 8) != 0)
    {
        key_env.repCnt = 0;
        if (key_env.keyTyp >= KT_KEYBOARD)
        {
            // Press new report
            kb_send_report(key_env.keyTyp, key_env.report);
        }

        // backup report
        memcpy(key_env.report0, key_env.report, 8);
    }
    else
    {
		key_env.repCnt++;        
		if (key_env.keyTyp == KT_LOCAL)
		{
			if ((key_env.mkCode == KEY_N) && (key_env.repCnt == 100))  //delete parering
			{
				key_env.repCnt = 0;
				key_env.mkCode = 0;

				Leds_Play(LED_BT_OFF);
				Leds_Play(LED_BT_DELETE_PAIR);
				Delete_Pair_Info();
				DEBUG("FN+N...DeletePair");				
			}
		}

		if ((key_env.mkCode == KEY_C) && (key_env.state == KS_RELEASE) && (powerOn_Work_Status != SYS_PARING))//enter parring
		{
			key_env.mkCode = 0;
			
			BtAddr_Add_One();
			BtReset();
			powerOn_Work_Status = SYS_PARING;
			
			GPIO_DAT_CLR(BIT(POWER_LED));
			Leds_Play(LED_BT_OFF);
			Leds_Play(LED_BT_PAIRING);
			DEBUG("FN+C...EnterPairing");
		}		
    }
    
    #if (CFG_PIN_ENABLE)
    if ((powerOn_Work_Status == SYS_PARING) && (g_pin_num == 6))
    {
        struct gap_sec_key tk;
        
        // Set the TK value
        memset(&tk.key, 0, GAP_KEY_LEN);
        write32p(&tk.key, g_pin_code);

        DEBUG("pin_code:%06d", g_pin_code);
        gapc_smp_pairing_tk_exch(0, true, &tk);
        
        g_pin_num = 0;
        g_pin_code = 0;
    }
    #endif
	
	if (app_state_get() < APP_ENCRYPTED){
		key_status_cont=0;return ;
	}
	
	update_time_cnt++;
	if(key_env.keyTyp==KT_NONE)
	{
		if(++key_status_cont>17000)key_status_cont=17000;
	}
	else
	{
		key_status_cont = 0;
		extern uint16_t sys_Timeout_Cnt ;
		sys_Timeout_Cnt =0;
	}
	//DEBUG("key_status_cont:0x%x\r\n",key_status_cont);
	
	#if(0)
	if(key_status_cont > 16000)
	{
		
		key_status_cont = 0;
		for (uint8_t j = 0; j < 3; j++)
		{
			GPIO->DIR_CLR = 0x1 << (Key_Col[j]);
			GPIO->DIR_SET = 0x1 << (Key_Col[j]);
			GPIO->DAT_CLR = 0x1 << (Key_Col[j]);
			iom_ctrl(Key_Col[j],IOM_PULLDOWN);			
		}
		
		DEBUG("enterpoweroff\r\n");
		wakeup_io_sw(ALL_ROW|EXTEND_COL_CONTROL_PIN, ALL_ROW|EXTEND_COL_CONTROL_PIN);	
		core_pwroff(WKUP_PWIO_EN_BIT | WKUP_IO_LATCH_N_BIT);
	}
	else if(key_status_cont > 400)
	#else
	#if(SFT_TIME_SCAN)
	if(key_status_cont > 330)
	#else
	if(key_status_cont > 660)
	#endif
	#endif
	{		
		if(((updata_parm_data == 0)||(updata_parm_data == 1))&&(updata_conn_status==0))
		{
			gapc_update_param(app_env.curidx, &key_le_conn_pref2);
			DEBUG("update3");
			update_time_cnt = 0;	
			update_count	= 5;		
		}
		else
		{
			if(((conn_intv != 12)||(conn_late != 9))&&(update_time_cnt > 50)&&(updata_conn_status==0)&&(update_count>0))
			{
				gapc_update_param(app_env.curidx, &key_le_conn_pref2);
				DEBUG("update4");
				update_time_cnt = 0;
				update_count--;
			}
		}
		updata_parm_data = 2;
	}
	else if(key_status_cont<1)
	{	 		
		if (update_connect_par_flag==0) // first connect delay some times go updata
		{
			if(((updata_parm_data == 0)||(updata_parm_data == 2))&&(updata_conn_status==0)&&((conn_intv != 12)||(conn_late != 0)))
			{
				gapc_update_param(app_env.curidx, &key_le_conn_pref1);
				DEBUG("update1");
				update_time_cnt = 0;
				update_count	= 5;		
			}
			else
			{
				if(((conn_intv != 12)||(conn_late != 0))&&(update_time_cnt > 50)&&(updata_conn_status==0)&&(update_count>0))
				{
					gapc_update_param(app_env.curidx, &key_le_conn_pref1);
					DEBUG("update2");
					update_time_cnt = 0;
					update_count--;			
				}
			}

			updata_parm_data = 1;
		}			
	}   
}

void key_data_clear(void)
{
	updata_parm_data 		= 0;
	key_status_cont 		= 0;
	update_time_cnt			= 0;
	update_connect_par_flag = 1;
	update_count 			= 0;
	updata_conn_status 		= 0;
	sys_poweroff_flag  		= 0;
	
	wakeup_io_sw(ALL_ROW|BIT(PAIR_KEY_GPIO), ALL_ROW|BIT(PAIR_KEY_GPIO));	
	memset((uint8_t*)&key_env,0,sizeof(key_env));
}

void kb_init(void)
{
	key_data_clear();
	GPIO->DIR_CLR = ALL_ROW;
	conn_late = 0;
	for(uint8_t i = 0; i < KEY_ROW_NB; i++)
	{
		iom_ctrl(Key_Row[i],IOM_PULLUP|IOM_INPUT);
	}	
	for(uint8_t i = 0; i < 3; i++)
	{
		iom_ctrl(Key_Col[i],IOM_SEL_GPIO|IOM_DRV_LVL1);
	}
	
	GPIO->DIR_CLR = ALL_COL;
	GPIO->DAT_SET = EXTEND_COL_CONTROL_PIN;
	GPIO->DIR_SET = EXTEND_COL_CONTROL_PIN;
	
	GPIO->DIR_CLR = EXTEND_POWER_PIN;
	GPIO->DAT_SET = EXTEND_POWER_PIN;
	GPIO->DIR_SET = EXTEND_POWER_PIN;
}

void kb_sleep(void)
{	
    for (uint8_t i = 0; i < 3; i++)
    {
		GPIO->DIR_CLR = 0x1<<(Key_Col[i]);
		iom_ctrl(Key_Col[i],IOM_HIZ);
    }
}

#define KEYBOARD_SCAN_INTEV 1

uint8_t keyboard_scan_time_id = 0;

static tmr_tk_t keyboard_scan_handle(tmr_id_t id)
{
	if(id == keyboard_scan_time_id)
	{
		#if(TRIM_LOAD)
		#if(CFG_EMI_MODE)
		if(g_is_emi_mode)
		{
			kb_scan();
		}
		else
		{
			LowBatt_ALarm();
		}
		#else
		LowBatt_ALarm();
		#endif
		#endif
		#if(SFT_TIME_SCAN)
		kb_scan();
		#endif
	}

	return(KEYBOARD_SCAN_INTEV);
}

void Keyboard_Scan_Init(void)
{ 
	kb_init();
	
	#if(KEYBOARD_SCAN_INTEV)
	
	if (keyboard_scan_time_id != TMR_ID_NONE)
	{
		sftmr_clear(keyboard_scan_time_id);
		keyboard_scan_time_id = TMR_ID_NONE;
	}

	keyboard_scan_time_id = sftmr_start(KEYBOARD_SCAN_INTEV,keyboard_scan_handle);
	#endif
}
