/**
 ****************************************************************************************
 *
 * @file app_actv.c
 *
 * @brief Application Activity(Advertising, Scanning and Initiating) - Example
 *
 * < If want to modify it, recommend to copy the file to 'user porject'/src >
 ****************************************************************************************
 */

#include "app_user.h"
#include "app.h"
#include "gapm_api.h"

#if (DBG_ACTV)
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

/// Index & State of activities - User Customize
struct actv_env_tag
{
    #if (BLE_EN_ADV)
    /// Advertising index and state
    uint8_t advidx;
    uint8_t advsta;
    #endif //(BLE_EN_ADV)
    
    #if (BLE_EN_SCAN)
    /// Scanning index and state
    uint8_t scanidx;
    uint8_t scansta;
    #endif //(BLE_EN_SCAN)
    
    #if (BLE_EN_INIT)
    /// Initiating index and state
    uint8_t initidx;
    uint8_t initsta;
    #endif //(BLE_EN_INIT)
};

/// Activities environment
struct actv_env_tag actv_env;

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @section Scanning Activity - Example for User Customize
 *          enable via pre-define @see BLE_EN_SCAN
 ****************************************************************************************
 */
#if (BLE_EN_SCAN)

/*
 * DEFINITIONS
 ****************************************************************************************
 */

volatile uint8_t scan_cnt = 0;
struct bd_addr_icon user_scan_addr_list[SCAN_NUM_MAX];
struct bd_addr_icon user_scan_addr_list_store[SCAN_NUM_MAX];
/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
static void app_start_scanning(void)
{
    struct gapm_scan_param scan_param;
    
    /// Type of scanning to be started (@see enum gapm_scan_type)
    scan_param.type = GAPM_SCAN_TYPE_CONN_DISC;
    /// Properties for the scan procedure (@see enum gapm_scan_prop)
    scan_param.prop = GAPM_SCAN_PROP_PHY_1M_BIT | GAPM_SCAN_PROP_ACTIVE_1M_BIT | GAPM_SCAN_PROP_FILT_TRUNC_BIT;
    /// Duplicate packet filtering policy (@see enum gapm_dup_filter_pol)
    scan_param.dup_filt_pol = GAPM_DUP_FILT_DIS;//GAPM_DUP_FILT_EN;
    /// Scan window opening parameters for LE 1M PHY (in unit of 625us)
    scan_param.scan_param_1m.scan_intv = 4;//GAP_SCAN_FAST_INTV;
    scan_param.scan_param_1m.scan_wd   = 4;//GAP_SCAN_FAST_WIND;
    /// Scan window opening parameters for LE Coded PHY
    //scan_param.scan_param_coded.scan_intv = GAP_SCAN_SLOW_INTV1;
    //scan_param.scan_param_coded.scan_wd   = GAP_SCAN_SLOW_WIND1;
    /// Scan duration (in unit of 10ms). 0 means that the controller will scan continuously until
    /// reception of a stop command from the application
    scan_param.duration = 0;//GAP_TMR_GEN_DISC_SCAN;
    /// Scan period (in unit of 1.28s). Time interval betweem two consequent starts of a scan duration
    /// by the controller. 0 means that the scan procedure is not periodic
    scan_param.period = 0;
    
    gapm_start_activity(actv_env.scanidx, sizeof(struct gapm_scan_param), &scan_param);
}

/**
 ****************************************************************************************
 * @brief Action/Command of Scanning
 *
 * @param[in] actv_op  Operation of activity
 ****************************************************************************************
 */
void app_scan_action(uint8_t actv_op)
{
    switch (actv_op)
    {
        case ACTV_CREATE:
        {
            if (actv_env.scansta == ACTV_STATE_OFF)
            {
                DEBUG("Creating");
                gapm_create_activity(GAPM_ACTV_TYPE_SCAN, GAPM_STATIC_ADDR);
                actv_env.scansta = ACTV_STATE_CREATE;
            }
        } break;

        case ACTV_START:
        {
            if (actv_env.scansta == ACTV_STATE_READY)
            {
                DEBUG("Scan Starting");
                app_start_scanning();
                actv_env.scansta = ACTV_STATE_START;
            }
        } break;

        case ACTV_STOP:
        {
            if (actv_env.scansta == ACTV_STATE_START)
            {
                DEBUG("Stopping");
                gapm_stop_activity(actv_env.scanidx);
                actv_env.scansta = ACTV_STATE_STOP;
            }
        } break;

        case ACTV_DELETE:
        {
            if ((actv_env.scansta != ACTV_STATE_OFF) && (actv_env.scansta != ACTV_STATE_START))
            {
                DEBUG("Deleting");
                gapm_delete_activity(actv_env.scanidx);
                actv_env.scansta = ACTV_STATE_OFF;
            }
        } break;

        case ACTV_RELOAD:
        {
            if (actv_env.scansta == ACTV_STATE_START)
            {
                DEBUG("Stopping");
                gapm_stop_activity(actv_env.scanidx);
                actv_env.scansta = ACTV_STATE_STOP;
            }

            if ((actv_env.scansta != ACTV_STATE_OFF) && (actv_env.scansta != ACTV_STATE_START))
            {
                DEBUG("Deleting");
                gapm_delete_activity(actv_env.scanidx);
                actv_env.scansta = ACTV_STATE_OFF;
            }

            DEBUG("Creating");
            gapm_create_activity(GAPM_ACTV_TYPE_SCAN, GAPM_STATIC_ADDR);
            actv_env.scansta = ACTV_STATE_CREATE;
        } break;

        default:
            break;
    }
}

/**
 ****************************************************************************************
 * @brief Event Procedure of Scanning 
 *
 * @param[in] gapm_op  Operation of gapm
 * @param[in] status   Status of event
 ****************************************************************************************
 */
void app_scan_event(uint8_t gapm_op, uint8_t status)
{
    DEBUG("Evt(op:0x%X,sta:0x%X)", gapm_op, status);
    
    switch (gapm_op)
    {
        case GAPM_CREATE_SCAN_ACTIVITY:
        {
            actv_env.scansta = ACTV_STATE_READY;

            if (status == GAP_ERR_NO_ERROR)
            {
                DEBUG("Scan start...");
                app_scan_action(ACTV_START);
            }
        } break;

        case GAPM_STOP_ACTIVITY:
        {
            if ((actv_env.scansta == ACTV_STATE_START) || (actv_env.scansta == ACTV_STATE_STOP))
            {
                actv_env.scansta = ACTV_STATE_READY;
            }

            DEBUG("-->scan_cnt:%d, app_sta:%d, kid:%d", scan_cnt, app_state_get(), g_cid_kb);

//            if (app_env.conbits < 3)
//            {
//                init_timer_start();
//                app_scan_action(ACTV_START);
//            }

            #if (DBG_MODE)
            for (uint8_t idx = 0; idx < scan_cnt; idx++)
            {
                DEBUG("Scan List[%d]-->", idx);
                debugHex((uint8_t *)(&user_scan_addr_list[idx]), sizeof(struct bd_addr_icon));             
            }
            #endif // DBG_MODE
        } break;

        default:
            break;
    }
}

/**
 ****************************************************************************************
 * @brief Store result of Scanning when filter by app_actv_report_ind
 *
 * @param[in] paddr    gap_bdaddr of peer device
 ****************************************************************************************
 */
void app_scan_result(const struct gap_bdaddr* paddr, uint8_t icon_type)
{
    for (uint8_t i = 0; i < scan_cnt; i++)
    {
        // 排除相同的地址或相同的icon
        if ((!memcmp(&user_scan_addr_list[i], paddr, sizeof(struct gap_bdaddr)))
            || (user_scan_addr_list[i].icon_type == icon_type))
        {
            return;
        }
    }

    //get null array
    if (scan_cnt < SCAN_NUM_MAX) 
    {
        memcpy((uint8_t *)&user_scan_addr_list[scan_cnt], paddr, sizeof(struct gap_bdaddr));
        user_scan_addr_list[scan_cnt].icon_type = icon_type;

        DEBUG("FIND_NEW[%d]----------------", scan_cnt);
        debugHex((uint8_t *)(user_scan_addr_list+scan_cnt), sizeof(struct bd_addr_icon));

        scan_cnt++;

        #if (DBG_MODE)
        if (scan_cnt == SCAN_NUM_MAX)
        {
            debugHex((uint8_t *)user_scan_addr_list, SCAN_NUM_MAX * sizeof(struct bd_addr_icon));
        }
        #endif // DBG_MODE
    }
}

/**
 ****************************************************************************************
 * @brief Handles activity report. (@see GAPM_EXT_ADV_REPORT_IND)
 *
 * @param[in] report  Report of Advertising data be scanned
 ****************************************************************************************
 */
void app_actv_report_ind(struct gapm_ext_adv_report_ind const* report)
{
    uint16_t icon;

    // filter report
    if ((report->info & GAPM_REPORT_INFO_REPORT_TYPE_MASK) == GAPM_REPORT_TYPE_ADV_LEG)
    {
        const uint8_t *p_cursor = report->data;
        const uint8_t *p_end_cusor = report->data + report->length;

        while (p_cursor < p_end_cusor)
        {
            // Extract AD type
            uint8_t ad_type = *(p_cursor + 1);

            if (ad_type == GAP_AD_TYPE_APPEARANCE)
            {
                icon = read16p(p_cursor+2);
            }

            // 过滤广播中带QKEI, 并且icon为键盘或鼠标
            if (((ad_type == GAP_AD_TYPE_MANU_SPECIFIC_DATA) && (*p_cursor > 4) && (0 == memcmp(p_cursor+2, "QKIE", 4)))
                && ((icon == GAP_APPEARANCE_HID_KEYBOARD) || (icon == GAP_APPEARANCE_HID_MOUSE)))
            {
//                DEBUG("---Find:%d, icon_type:%x", report->length, icon & 0xFF);
//                debugHex(report->data, report->length);
                app_scan_result(&report->trans_addr, icon & 0xFF);
                break;
            }

            /* Go to next advertising info */
            p_cursor += (*p_cursor + 1);
        }
    }
}
#endif //(BLE_EN_SCAN)


/**
 ****************************************************************************************
 * @section Initiating Activity - Example for User Customize
 *          enable via pre-define @see BLE_EN_INIT
 ****************************************************************************************
 */
#if (BLE_EN_INIT)

/*
 * DEFINITIONS
 ****************************************************************************************
 */

const struct gapm_conn_param ms_conn_param = 
{
    /// Minimum value for the connection interval (in unit of 1.25ms). Allowed range is 7.5ms to 4s.
    .conn_intv_min = 6,
    /// Maximum value for the connection interval (in unit of 1.25ms). Allowed range is 7.5ms to 4s.
    .conn_intv_max = 6,
    /// Slave latency. Number of events that can be missed by a connected slave device
    .conn_latency = 0,
    /// Link supervision timeout (in unit of 10ms). Allowed range is 100ms to 32s
    .supervision_to = 300,
    /// Recommended minimum duration of connection events (in unit of 625us)
    .ce_len_min = 4,
    /// Recommended maximum duration of connection events (in unit of 625us)
    .ce_len_max = 6,
};

const struct gapm_conn_param kb_conn_param = 
{
    /// Minimum value for the connection interval (in unit of 1.25ms). Allowed range is 7.5ms to 4s.
    .conn_intv_min = 12,
    /// Maximum value for the connection interval (in unit of 1.25ms). Allowed range is 7.5ms to 4s.
    .conn_intv_max = 12,
    /// Slave latency. Number of events that can be missed by a connected slave device
    .conn_latency = 0,
    /// Link supervision timeout (in unit of 10ms). Allowed range is 100ms to 32s
    .supervision_to = 300,
    /// Recommended minimum duration of connection events (in unit of 625us)
    .ce_len_min = 6,
    /// Recommended maximum duration of connection events (in unit of 625us)
    .ce_len_max = 8,
};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Start initiating to peer device
 *
 * @param[in] paddr    gap_bdaddr of peer device
 ****************************************************************************************
 */

void app_start_initiating(const struct gap_bdaddr* paddr, uint8_t icon_type)
{
//    bd_addr_t invalid_addr = {{[0 ... 5] = 0}};
//    
//    if (0 == memcmp((uint8_t *)&invalid_addr, (uint8_t *)paddr, GAP_BD_ADDR_LEN))
//    {
//        DEBUG("Init invalid addr");
//        return;
//    }

    if ((actv_env.initsta == ACTV_STATE_READY) || (actv_env.initsta == ACTV_STATE_STOP))
    {
        struct gapm_init_param init_param;

        init_param.type = GAPM_INIT_TYPE_DIRECT_CONN_EST;
        init_param.prop = GAPM_INIT_PROP_1M_BIT;
        init_param.scan_param_1m.scan_intv = GAP_SCAN_FAST_INTV;
        init_param.scan_param_1m.scan_wd   = GAP_SCAN_FAST_WIND;

        if (icon_type == ICON_MS)
        {
            memcpy(&init_param.conn_param_1m, &ms_conn_param, sizeof(struct gapm_conn_param));
        }
        else
        {
            memcpy(&init_param.conn_param_1m, &kb_conn_param, sizeof(struct gapm_conn_param));
        }

        memcpy(&init_param.peer_addr, paddr, sizeof(struct gap_bdaddr));

        // timeout unit in 10ms, update from v1.3
        init_param.conn_to = 300; 

        gapm_start_activity(actv_env.initidx, sizeof(struct gapm_init_param), &init_param);

        actv_env.initsta = ACTV_STATE_START;
        DEBUG("Init Starting");
        debugHex((uint8_t *)paddr, 6);
    }
}

/**
 ****************************************************************************************
 * @brief Action/Command of Initiating
 *
 * @param[in] actv_op  Operation of activity
 ****************************************************************************************
 */
void app_init_action(uint8_t actv_op)
{  
    switch (actv_op)
    {
        case ACTV_CREATE:
        {
            if (actv_env.initsta == ACTV_STATE_OFF)
            {
                DEBUG("Creating");
                gapm_create_activity(GAPM_ACTV_TYPE_INIT, GAPM_STATIC_ADDR);
                actv_env.initsta = ACTV_STATE_CREATE;
            }
        } break;

        case ACTV_START:
        {
//            app_start_initiating(NULL, NULL);
        } break;

        case ACTV_STOP:
        {
            if (actv_env.initsta == ACTV_STATE_START)
            {
                DEBUG("Stopping");
                gapm_stop_activity(actv_env.initidx);
                actv_env.initsta = ACTV_STATE_STOP;
            }
        } break;

        case ACTV_DELETE:
        {
            if ((actv_env.initsta != ACTV_STATE_OFF) && (actv_env.initsta != ACTV_STATE_START))
            {
                DEBUG("Deleting");
                gapm_delete_activity(actv_env.initidx);
                actv_env.initsta = ACTV_STATE_OFF;
            }
        } break;

        case ACTV_RELOAD:
        {
            if (actv_env.initsta == ACTV_STATE_START)
            {
                DEBUG("Stopping");
                gapm_stop_activity(actv_env.initidx);
                actv_env.initsta = ACTV_STATE_STOP;
            }

            if ((actv_env.initsta != ACTV_STATE_OFF) && (actv_env.initsta != ACTV_STATE_START))
            {
                DEBUG("Deleting");
                gapm_delete_activity(actv_env.initidx);
                actv_env.initsta = ACTV_STATE_OFF;
            }

            DEBUG("Creating");
            gapm_create_activity(GAPM_ACTV_TYPE_INIT, GAPM_STATIC_ADDR);
            actv_env.initsta = ACTV_STATE_CREATE;
        } break;

        default:
            break;
    }
}

/**
 ****************************************************************************************
 * @brief Event Procedure of Initiating 
 *
 * @param[in] gapm_op  Operation of gapm
 * @param[in] status   Status of event
 ****************************************************************************************
 */
void app_init_event(uint8_t gapm_op, uint8_t status)
{
    DEBUG("Evt(op:0x%X,sta:0x%X)", gapm_op, status);

    switch (gapm_op)
    {
        case GAPM_CREATE_INIT_ACTIVITY:
        {
            actv_env.initsta = ACTV_STATE_READY;
        } break;

        case GAPM_STOP_ACTIVITY:
        {
            if ((actv_env.initsta == ACTV_STATE_START) || (actv_env.initsta == ACTV_STATE_STOP))
            {
                actv_env.initsta = ACTV_STATE_READY;
            }

            if (status == GAP_ERR_TIMEOUT)
            {
                scan_cnt = app_env.conbits ? 1 : 0;
                g_conn_icon_type[app_env.conbits & 0x01] = 0;
            }
        } break;
        
        default:
            break;
    }
}

#endif //(BLE_EN_INIT)


/**
 ****************************************************************************************
 * @brief Create activities when Initialization complete.
 ****************************************************************************************
 */
void app_actv_create(void)
{
    scan_cnt = 0;
    
    memset((uint8_t *)&actv_env, 0, sizeof(actv_env));
    
    #if (BLE_EN_ADV)
    app_adv_action(ACTV_CREATE);
    #endif //(BLE_EN_ADV)

    #if (BLE_EN_SCAN)
    app_scan_action(ACTV_CREATE);
    #endif //(BLE_EN_SCAN)
    
    #if (BLE_EN_INIT)
    app_init_action(ACTV_CREATE);
    #endif //(BLE_EN_INIT)
}

/**
 ****************************************************************************************
 * @brief Handles activity command complete event.
 *
 * @param[in] gapm_op  Operation of gapm
 * @param[in] status   Status of event
 ****************************************************************************************
 */
void app_actv_cmp_evt(uint8_t operation, uint8_t status)
{
    switch (operation)
    {
        #if (BLE_EN_ADV)
        case (GAPM_CREATE_ADV_ACTIVITY):
        case (GAPM_SET_ADV_DATA):
        case (GAPM_SET_SCAN_RSP_DATA):     
        {
            app_adv_event(operation, status);
        } break;  
        #endif //(BLE_EN_ADV)

        #if (BLE_EN_SCAN)
        case GAPM_CREATE_SCAN_ACTIVITY:
        {
            app_scan_event(operation, status);
        } break;
        #endif //(BLE_EN_SCAN)

        #if (BLE_EN_INIT)
        case GAPM_CREATE_INIT_ACTIVITY:
        {
            app_init_event(operation, status);
        } break;
        #endif //(BLE_EN_INIT)

        default:
            break;
    }
}

/**
 ****************************************************************************************
 * @brief Handles activity created. (@see GAPM_ACTIVITY_CREATED_IND)
 *
 * @param[in] actv_type  Type of activities(@see enum gapm_actv_type)
 * @param[in] actv_idx   Index of activities created
 ****************************************************************************************
 */
void app_actv_created_ind(uint8_t actv_type, uint8_t actv_idx)
{
    switch (actv_type)
    {
        #if (BLE_EN_ADV)
        case GAPM_ACTV_TYPE_ADV:
        {
            actv_env.advidx = actv_idx;
        } break;
        #endif //(BLE_EN_ADV)

        #if (BLE_EN_SCAN)
        case GAPM_ACTV_TYPE_SCAN:
        {
            actv_env.scanidx = actv_idx;
        } break;
        #endif //(BLE_EN_SCAN)

        #if (BLE_EN_INIT)
        case GAPM_ACTV_TYPE_INIT:
        {
            actv_env.initidx = actv_idx;
        } break;
        #endif //(BLE_EN_INIT)

        default:
            break;
    }
}

/**
 ****************************************************************************************
 * @brief Handles activity stopped. (@see GAPM_ACTIVITY_STOPPED_IND)
 *
 * @param[in] actv_type  Type of activity(@see enum gapm_actv_type)
 * @param[in] reason     Reason of stopped
 ****************************************************************************************
 */
void app_actv_stopped_ind(uint8_t actv_type, uint8_t actv_idx, uint8_t reason)
{
    switch (actv_type)
    {
        #if (BLE_EN_ADV)
        case GAPM_ACTV_TYPE_ADV:
        {
            // Advertising Stopped by slave connection or duration timeout
            app_adv_event(GAPM_STOP_ACTIVITY, reason);

            // Duration timeout, go IDLE - update from v1.3
            if ((reason == GAP_ERR_TIMEOUT) && (app_state_get() == APP_READY))
            {
                app_state_set(APP_IDLE);
            }
        } break;
        #endif //(BLE_EN_ADV)

        #if (BLE_EN_SCAN)
        case GAPM_ACTV_TYPE_SCAN:
        {
            app_scan_event(GAPM_STOP_ACTIVITY, reason);
        } break;
        #endif //(BLE_EN_SCAN)

        #if (BLE_EN_INIT)
        case GAPM_ACTV_TYPE_INIT:
        {
            app_init_event(GAPM_STOP_ACTIVITY, reason);
        } break;
        #endif //(BLE_EN_INIT)

        default:
            break;
    }
}
