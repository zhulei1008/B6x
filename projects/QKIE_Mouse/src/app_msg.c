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
#include "prf_bass.h"

#if (DBG_APP)
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
extern uint8_t poweron_work_status;
extern uint8_t sys_timeout_cnt ;
extern void mouse_enter_powerdown(void);
extern uint8_t batt_lvl;
extern uint8_t batt_level(void);
/**
 ****************************************************************************************
 * @brief SubTask Handler of Custom or Unknow Message. (__weak func)
 ****************************************************************************************
 */
__weak APP_SUBTASK_HANDLER(custom)
{
    #if (RC32K_CALIB_PERIOD)
    if (msgid == APP_TIMER_RC32K_CORR)
    {
        uint8_t sleep_flag = 0;
        static uint8_t lvl_back ;
        DEBUG("rc32k_calib");
        rc32k_calib(); batt_lvl = batt_level();
        
//        if(lvl_back!=batt_lvl)
        if (co_abs(lvl_back - batt_lvl) > 3)
        {
            bass_bat_lvl_update(batt_lvl);
            lvl_back= batt_lvl;
        }
        
        ke_timer_set(APP_TIMER_RC32K_CORR, TASK_APP, RC32K_CALIB_PERIOD);
        
        if(poweron_work_status==SYS_IDLE)
        {
            if(sys_timeout_cnt>=SYS_IDLE_TIMEOUT)
            {
                sleep_flag=1;
            }
        }
        else if(poweron_work_status==SYS_PARING)
        {
            if(sys_timeout_cnt>=SYS_PARING_TIMEOUT)
            {
                sleep_flag=1;
            }
        }
        else if(poweron_work_status==SYS_CONNECT_BACK)
        {
            if(sys_timeout_cnt>=SYS_CONNECT_BACK_TIMEOUT)
            {
                sleep_flag = 1;
            }
        }
        else if(poweron_work_status==SYS_CONNECT)
        {
            if(sys_timeout_cnt>=SYS_CONNECT_NO_ACTION_TIMEOUT)
            {
                sleep_flag = 1;
            }
        }
        
        sys_timeout_cnt++;
        DEBUG("sys_timeout_cnt:%d\r\n",sys_timeout_cnt);
        
        if(sleep_flag)mouse_enter_powerdown();
    }
    else
    #endif //(RC32K_CALIB_PERIOD)
    {
        uint16_t length = ke_param2msg(param)->param_len;
        DEBUG("Unknow MsgId:0x%X\r\n", msgid);
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
