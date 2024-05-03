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

#include "app_user.h"
#include "app.h"
#include "drvs.h"

#if (DBG_APP)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

#define APP_INIT_PERIOD       30
volatile uint8_t g_cid_kb = 0xFF;
volatile uint8_t g_kb_led = 0xFF;
volatile uint8_t g_os_type = 0x00;
volatile uint8_t g_os_type_back = 0x00;
volatile uint8_t g_conn_icon_type[SCAN_NUM_MAX];
volatile uint8_t connect_cnt = 0;
/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */

/// SubTask Declaration, User add more...
extern APP_SUBTASK_HANDLER(gapm_msg);
extern APP_SUBTASK_HANDLER(gapc_msg);
extern APP_SUBTASK_HANDLER(gatt_msg);

//enum app_custom_msg_id
//{
//    APP_INIT_TIMER = APP_BASE_MSG + 3,
//};
struct bd_addr_icon addr_icon_buff[SCAN_NUM_MAX]={{0},{0}};
void app_init_start(void)
{
    struct bd_addr_icon addr_icon[SCAN_NUM_MAX]={0};
    uint8_t init_enable = 0;
    if (scan_cnt > 0)
    {
        // 连接状态时只需要考虑连接第二个, 故此时只允许再次扫描1个
        if (app_state_get() == APP_CONNECTED)
        {
            // 扫描到的icon等于当前连接的icon, 重新扫描
            if (g_conn_icon_type[app_env.curidx] == user_scan_addr_list[1].icon_type)
            {
                scan_cnt = 1;

                memset((uint8_t *)user_scan_addr_list, 0x00, SCAN_NUM_MAX * sizeof(struct bd_addr_icon));

                DEBUG("RESET SCAN LIST");
            }
            // 扫描到第二个, 并且扫描到的icon不等于已连接的icon(加上比较icon是为了防止多次执行app_start_initiating)
            else if ((scan_cnt == 2) 
                && (g_conn_icon_type[app_env.conbits & 0x01] != user_scan_addr_list[1].icon_type))
            {
                DEBUG("INIT 1---conn, cbits:%x, env:%d, scan_cnt:%d, (conn icon:0x%X, 0x%X)", app_env.conbits, app_env.state, scan_cnt, g_conn_icon_type[0], g_conn_icon_type[1]);
                DEBUG("scan addr");
                debugHex((uint8_t *)user_scan_addr_list, SCAN_NUM_MAX*sizeof(struct bd_addr_icon));
                DEBUG("store addr");
                debugHex((uint8_t *)user_scan_addr_list_store, SCAN_NUM_MAX*sizeof(struct bd_addr_icon));
                if( (memcmp((uint8_t *)user_scan_addr_list_store,(uint8_t *)&user_scan_addr_list[1],8))==0 || 
                    (memcmp((uint8_t *)&user_scan_addr_list_store[1],(uint8_t *)&user_scan_addr_list[1],8))==0)//same
                {
                    init_enable = 1;
                    DEBUG("OLD2");
                }
                else if(memcmp((uint8_t *)&user_scan_addr_list_store[1],(uint8_t *)&addr_icon[0],8)==0)
                {
                    if(user_scan_addr_list[1].icon_type!=user_scan_addr_list_store[0].icon_type)
                    {
                        init_enable = 1;
                        ke_timer_set(APP_STORE_ADD2, TASK_APP, 3500);
                        memcpy((uint8_t *)&addr_icon_buff[1],(uint8_t *)&user_scan_addr_list[1],8);
                        DEBUG("NWE2");
                        connect_cnt = 1;
                    }
                
                }
                else
                {
                    scan_cnt = 1;
                    DEBUG("unknown addr");
                    memset((uint8_t *)user_scan_addr_list, 0x00, SCAN_NUM_MAX * sizeof(struct bd_addr_icon));
                }
                if(init_enable)
                {
                    init_enable = 0;
                    g_conn_icon_type[app_env.conbits & 0x01] = user_scan_addr_list[1].icon_type;
   
                    app_start_initiating((struct gap_bdaddr*)(user_scan_addr_list + 1), user_scan_addr_list[1].icon_type);
                }

            }
        }
        else
        {
            // 一个连接都没有, 连接扫描到的第一个slave
            // 连接上第一个后, 状态即为APP_CONNECTED
            if (g_conn_icon_type[0] != user_scan_addr_list[0].icon_type)
            {
                DEBUG("INIT 0---conn, cbits:%x, env:%d, scan_cnt:%d, (conn icon:0x%X, 0x%X)", app_env.conbits, app_env.state, scan_cnt, g_conn_icon_type[0], g_conn_icon_type[1]);
                DEBUG("scan addr");
                debugHex((uint8_t *)user_scan_addr_list, SCAN_NUM_MAX*sizeof(struct bd_addr_icon));
                DEBUG("store addr");
                debugHex((uint8_t *)user_scan_addr_list_store, SCAN_NUM_MAX*sizeof(struct bd_addr_icon));
//                DEBUG("null addr");
//                debugHex((uint8_t *)addr_icon, SCAN_NUM_MAX*sizeof(struct bd_addr_icon));
                
                if(memcmp((uint8_t *)user_scan_addr_list_store,(uint8_t *)addr_icon,SCAN_NUM_MAX * sizeof(struct bd_addr_icon))==0)//NONE
                {
                    init_enable = 1;
                    memcpy((uint8_t *)&addr_icon_buff[0],(uint8_t *)&user_scan_addr_list[0],8);
                    ke_timer_set(APP_STORE_ADD1, TASK_APP, 3500);
                    DEBUG("NWE1");
                }
                else if( (memcmp((uint8_t *)user_scan_addr_list_store,(uint8_t *)user_scan_addr_list,8))==0 || 
                    (memcmp((uint8_t *)&user_scan_addr_list_store[1],(uint8_t *)user_scan_addr_list,8))==0)//same
                {
                    init_enable = 1;
                    DEBUG("OLD1");
                }
                else if(memcmp((uint8_t *)&user_scan_addr_list_store[1],(uint8_t *)&addr_icon[0],8)==0)
                {
                    if(user_scan_addr_list[0].icon_type!=user_scan_addr_list_store[0].icon_type)
                    {
                        init_enable = 1;
                        ke_timer_set(APP_STORE_ADD2, TASK_APP, 3500);
                        memcpy((uint8_t *)&addr_icon_buff[1],(uint8_t *)&user_scan_addr_list[0],8);
                        DEBUG("NWE2");
                        connect_cnt = 0;
                        
                    }
                    else
                    {
                        DEBUG("icon same");
                        scan_cnt = 0;
                        memset((uint8_t *)user_scan_addr_list, 0x00, SCAN_NUM_MAX * sizeof(struct bd_addr_icon));
                    }
                
                }
                else
                {
                    DEBUG("unknown addr");
                    scan_cnt = 0;
                    memset((uint8_t *)user_scan_addr_list, 0x00, SCAN_NUM_MAX * sizeof(struct bd_addr_icon));
                }
                if(init_enable)
                {
                     init_enable = 0;
                     g_conn_icon_type[0] = user_scan_addr_list[0].icon_type;

                     app_start_initiating((struct gap_bdaddr*)(user_scan_addr_list + 0), user_scan_addr_list[0].icon_type);
                     
                }

            }
        }
    }
}

#if (CFG_HW_TIMER)
#include "sftmr.h"
#define SCAN_INV  _MS(APP_INIT_PERIOD)
uint8_t g_init_tid;
static tmr_tk_t app_init_timer(uint8_t id)
{
    app_init_start();
    
    return SCAN_INV;
}
#endif

void init_timer_start(void)
{   
    #if (CFG_HW_TIMER)
    g_init_tid = sftmr_start(SCAN_INV, app_init_timer);
    #else
    ke_timer_set(APP_INIT_TIMER, TASK_APP, APP_INIT_PERIOD);
    #endif

    DEBUG("init_timer_start");
}

void init_timer_stop(void)
{
    #if (CFG_HW_TIMER)
    sftmr_clear(g_init_tid);
    #else
    ke_timer_clear(APP_INIT_TIMER, TASK_APP);
    #endif
    DEBUG("init_timer_stop");
}

/**
 ****************************************************************************************
 * @brief SubTask Handler of Custom or Unknow Message. (__weak func)
 ****************************************************************************************
 */
__weak APP_SUBTASK_HANDLER(custom)
{
    #if !(CFG_HW_TIMER)
    if (msgid == APP_INIT_TIMER)
    {
        ke_timer_set(APP_INIT_TIMER, TASK_APP, APP_INIT_PERIOD);

        app_init_start();
    }
    else if(msgid == APP_STORE_ADD1)
    {
        if (app_state_get() == APP_CONNECTED)
        {
            memcpy((uint8_t *)&user_scan_addr_list_store[0],(uint8_t *)&addr_icon_buff[0],8);
            DEBUG("STORE ADDR1");
        }
        ke_timer_clear(APP_STORE_ADD1,TASK_APP);
    }
    else if(msgid == APP_STORE_ADD2)
    {

        if (app_state_get() == APP_CONNECTED)
        {
            memcpy((uint8_t *)&user_scan_addr_list_store[1],(uint8_t *)&addr_icon_buff[1],8);
            DEBUG("STORE ADDR2");
        }
        
        ke_timer_clear(APP_STORE_ADD1,TASK_APP);
    }
    else 
    #endif
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
        #if (GATT_CLI)
        case (TID_GATT):
            handler = app_gatt_msg_handler;
            break;
        #endif
        
        case (TID_GAPM):
            handler = app_gapm_msg_handler;
            break;

        case (TID_GAPC):
            handler = app_gapc_msg_handler;
            break;

        default:
        {
            handler = app_custom_handler;
        } break;
    }

    return handler;
}
