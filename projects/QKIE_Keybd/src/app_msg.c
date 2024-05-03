/**
 ****************************************************************************************
 *
 * @file app_msg.c
 *
 * @brief Application Messages Handler - Example
 *
 * < If want to modify it, recommend to copy the file to 'user porject'/src >
 ****************************************************************************************
 */

#include "app.h"
#include "drvs.h"
#include "leds.h"
#include "prf_bass.h"
#include "gapc_api.h"
#include "user_api.h"
/*
 * DEFINES
 ****************************************************************************************
 */
#if (DBG_MSG)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */

/// SubTask Declaration, User add more...
extern APP_SUBTASK_HANDLER(gapm_msg);
extern APP_SUBTASK_HANDLER(gapc_msg);
extern APP_SUBTASK_HANDLER(gatt_msg);
extern APP_SUBTASK_HANDLER(l2cc_msg);
extern APP_SUBTASK_HANDLER(mesh_msg);
/**
 ****************************************************************************************
 * @brief SubTask Handler of Custom or Unknow Message. (__weak func)
 ****************************************************************************************
 */
__weak APP_SUBTASK_HANDLER(custom)
{
	uint8_t sleep_flag = 0;
	
	#if (RC32K_CALIB_PERIOD)
    if (msgid == APP_TIMER_RC32K_CORR)
    {
        //DEBUG("rc32k_calib");
        uint16_t rc32k=rc32k_calib();
        DEBUG("32K(M:%d,L:%d)", rc32k & 0xF, rc32k >> 4);
        #if(TRIM_LOAD&&(PRF_BASS))
        static uint8_t lvl_back ;
        batt_lvl = batt_level();
        
        if(lvl_back != batt_lvl)
        {
            bass_bat_lvl_update(batt_lvl);
            lvl_back = batt_lvl;
        }
        #endif
        ke_timer_set(APP_TIMER_RC32K_CORR, TASK_APP, RC32K_CALIB_PERIOD);
        
        if (powerOn_Work_Status == SYS_IDLE)
        {
            if (sys_Timeout_Cnt >= SYS_IDLE_TIMEOUT)
            {
                sleep_flag = 1;
            }
        }
        else if(powerOn_Work_Status == SYS_PARING)
        {
            if (sys_Timeout_Cnt >= SYS_PARING_TIMEOUT)
            {
                sleep_flag = 1;
            }
        }
        else if(powerOn_Work_Status == SYS_CONNECT_BACK)
        {
            if (sys_Timeout_Cnt >= SYS_CONNECT_BACK_TIMEOUT)
            {
                sleep_flag = 1;
            }
        }
        else if(powerOn_Work_Status == SYS_CONNECT)
        {
            if (sys_Timeout_Cnt >= SYS_CONNECT_NO_ACTION_TIMEOUT)
            {
                sleep_flag = 1;
            }
        }
        
        sys_Timeout_Cnt++;
        DEBUG("Cnt:%d\r\n",sys_Timeout_Cnt);
        
        if(sleep_flag)keyboard_poweroff();
    }
	else if(msgid==APP_TIME_FIRST_UPADTA_PAR)
	{
		if((conn_intv!=12)&&(connect_complete==1)&&(Request_connection_parameters_cont>0))
		{
			gapc_update_param(app_env.curidx, &key_le_conn_pref1);
			Request_connection_parameters_cont--;
			ke_timer_set(APP_TIME_FIRST_UPADTA_PAR,TASK_APP,5000);
		}
		else
		{
			ke_timer_clear(APP_TIME_FIRST_UPADTA_PAR,TASK_APP);
			update_connect_par_flag = 0;
			DEBUG("TIMEOUT_UPDATA\r\n");
		}			
	}
	else if(msgid==HOGPD_WAKEUP_REPORT)
	{
		wakeup_report();
		DEBUG("HOGPD_WAKEUP_REPORT");
		ke_timer_clear(HOGPD_WAKEUP_REPORT, TASK_APP);
	}
	else if(msgid==APP_DISCONNECT_TIME)
	{
		//gapc_disconnect(app_env.curidx);
		if(sys_poweroff_flag)
		{
			DEBUG("APP_DISCONNECT_TIME0");
			if(app_state_get()>=APP_CONNECTED)
			{
				ke_timer_set(APP_DISCONNECT_TIME,TASK_APP,1000);
				gapc_disconnect(app_env.curidx);
			}
			else
			{
				sys_poweroff_flag = 0;
				ke_timer_clear(APP_DISCONNECT_TIME, TASK_APP);
				keyboard_poweroff();
			}	
		}
		else
		{
			DEBUG("APP_DISCONNECT_TIME1");
			ke_timer_clear(APP_DISCONNECT_TIME, TASK_APP);
			keyboard_poweroff();
		}
	}
	else if(msgid == APP_TIMER_OFF_LED)
	{
		GPIO_DAT_CLR(BIT(POWER_LED));	
		ke_timer_clear(APP_TIMER_OFF_LED,TASK_APP);	
		powerOn_PowerLed_ActionDone_Flag = true;		
	}
	else if(msgid == LED_PLAY_LINK)
	{
		led_blink_cnt++;
		if(led_blink_cnt%2)
		{
			GPIO_DAT_CLR(0x1<<BT_LED);
		}
		else
		{
			GPIO_DAT_SET(0x1<<BT_LED);
		}
		ke_timer_set(LED_PLAY_LINK,TASK_APP,200);	
		if(led_blink_cnt==3)
		{
			ke_timer_clear(LED_PLAY_LINK,TASK_APP);
			led_blink_cnt = 0;
		}
	}
	else if(msgid == APP_TIMER_LOWBAT_ALARM)
	{
		lowLedBlinkCnt++;
		DEBUG("Alarm_Cnt:%d",lowLedBlinkCnt);
		
		if(lowLedBlinkCnt % 2)
		{
			GPIO_DAT_CLR(BIT(POWER_LED));
		}
		else
		{
			GPIO_DAT_SET(BIT(POWER_LED));
		}	

		ke_timer_set(APP_TIMER_LOWBAT_ALARM,TASK_APP,1000);	
		if(lowLedBlinkCnt == 21)
		{
			ke_timer_clear(APP_TIMER_LOWBAT_ALARM,TASK_APP);
			lowLedBlinkCnt = 0;
			keyboard_go_poweroff_jage();
		}		
	}	
    else
    #endif //(RC32K_CALIB_PERIOD)
    {
        uint16_t length = ke_param2msg(param)->param_len;
        DEBUG("Unknow MsgId:0x%X", msgid);
        debugHex((uint8_t *)param, length);
    }
    
    return (MSG_STATUS_FREE);
}

/**
 ****************************************************************************************
 * @brief Dispatch TASK_APP message to sub-handler.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] task_idx  Index of the receiving task instance.
 *
 * @return Handler of the message or NULL.
 ****************************************************************************************
 */
__TASKFN void* app_task_dispatch(msg_id_t msgid, uint8_t task_idx)
{
    msg_func_t handler = NULL;

    switch (MSG_TYPE(msgid))
    {
        case (TID_GAPM):
            handler = app_gapm_msg_handler;
            break;

        case (TID_GAPC):
            handler = app_gapc_msg_handler;
            break;

        #if (GATT_CLI)
        case (TID_GATT):
            handler = app_gatt_msg_handler;
            break;
        #endif
        
        #if (L2CC_LECB)
        case (TID_L2CC):
            handler = app_l2cc_msg_handler;
            break;
        #endif

        #if (PRF_MESH)
        case TID_MESH:
            status = app_mesh_msg_handler;
            break;
        #endif

        default:
        {
            handler = app_custom_handler;
        } break;
    }

    return handler;
}
