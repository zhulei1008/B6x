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

#include "app.h"
#include "gapm_api.h"
#include "user_api.h"

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
 * @section Advertising Activity - Example for User Customize
 *          enable via pre-define @see BLE_EN_ADV
 ****************************************************************************************
 */
#if (BLE_EN_ADV)

#undef DEBUG
#if (DBG_ACTV)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<ADV>" format "\r\n", ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#endif

/*
 * DEFINITIONS
 ****************************************************************************************
 */

/// Advertising duration - 0 mean Always ON (in multiple of 10ms)
#if !defined(APP_ADV_DURATION)
    #define APP_ADV_DURATION  (0)
#endif

/// Advertising channel map - 37, 38, 39
#if !defined(APP_ADV_CHMAP)
    #define APP_ADV_CHMAP     (0x07)
#endif

/// Advertising minimum interval - (n)*0.625ms
#if !defined(APP_ADV_INT_MIN)
    #define APP_ADV_INT_MIN   (32)
#endif

/// Advertising maximum interval - (n)*0.625ms
#if !defined(APP_ADV_INT_MAX)
    #define APP_ADV_INT_MAX   (32)
#endif

#if (APP_ADV_INT_MIN > APP_ADV_INT_MAX)
    #error "ADV_INT_MIN must not exceed ADV_INT_MAX"
#endif

/// Fast advertising interval
#define APP_ADV_FAST_INT      (32)

const uint8_t  def_master_dongle_addr[] = DEF_BLE_ADDR;
/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
static void app_adv_create(void)
{
    struct gapm_adv_create_param adv_param;
    
    // Advertising type (@see enum gapm_adv_type)
    adv_param.type      = GAPM_ADV_TYPE_LEGACY;
    adv_param.prop      = GAPM_ADV_PROP_UNDIR_CONN_MASK;
    adv_param.disc_mode = GAPM_ADV_MODE_NON_DISC;
    
    if (poweron_work_status == SYS_IDLE)
    {
        adv_param.prop = GAPM_ADV_PROP_DIR_CONN_MASK;
        memcpy(&adv_param.peer_addr, def_master_dongle_addr, 7);
    }
    else if (poweron_work_status == SYS_PARING)
    {
        //GAP_TMR_LIM_ADV_TIMEOUT
        adv_param.disc_mode = GAPM_ADV_MODE_LIM_DISC; // GAPM_ADV_MODE_GEN_DISC;
    }

    // Filtering policy (@see enum gapm_adv_filter_policy)
    adv_param.filter_pol            = GAPM_ADV_ALLOW_SCAN_ANY_CON_ANY;
    // Config primary advertising (@see gapm_adv_prim_cfg)
    adv_param.prim_cfg.phy          = GAP_PHY_LE_1MBPS;
    adv_param.prim_cfg.chnl_map     = APP_ADV_CHMAP;
    adv_param.prim_cfg.adv_intv_min = APP_ADV_FAST_INT;
    adv_param.prim_cfg.adv_intv_max = APP_ADV_FAST_INT;

    DEBUG("create(disc:%d,prop:%d)\r\n", adv_param.disc_mode, adv_param.prop);
    
    gapm_create_advertising(GAPM_STATIC_ADDR, &adv_param);
}
 
static void app_adv_set_adv_data(void)
{
    // Reserve 3Bytes for AD_TYPE_FLAGS
    uint8_t adv_data[GAP_ADV_DATA_LEN];
    uint8_t length = 11;
    uint8_t name_len = 0;
    
    // Set flags: 3B
    adv_data[0] = 0x02;
    adv_data[1] = GAP_AD_TYPE_FLAGS;
    adv_data[2] = 0x05;
	
    // Set appearance: 4B
    uint16_t icon = app_icon_get();
    adv_data[3]   = 0x03;
    adv_data[4]   = GAP_AD_TYPE_APPEARANCE; // 0x19
    write16p(&adv_data[5], icon);
	
    // Set list of UUIDs: 4B
    adv_data[7] = 0x03;
    adv_data[8] = GAP_AD_TYPE_COMPLETE_LIST_16_BIT_UUID; // 0x03
    write16p(&adv_data[9], 0x1812);                      // HID Service

    if (poweron_work_status == SYS_CONNECT_BACK)
    {
        adv_data[8]  = GAP_AD_TYPE_MANU_SPECIFIC_DATA;
        if (work_mode == BT_MODE)
        {
            write16p(&adv_data[9], 0x0000);
        }
        else
        {
            adv_data[7]  = 0x05;
            adv_data[9]  = 'Q';
            adv_data[10] = 'K';
            adv_data[11] = 'I';
            adv_data[12] = 'E';
            length = 13;
        }
//        adv_data[8] = GAP_AD_TYPE_MANU_SPECIFIC_DATA;
//        write16p(&adv_data[9], 0x0000);

    }
    else if(poweron_work_status == SYS_PARING)
    {
        adv_data[11] = 0x06;//len
        adv_data[12] = GAP_AD_TYPE_MANU_SPECIFIC_DATA; 
        adv_data[13] = 0X06;//Microsoft vendor id
        adv_data[14] = 0X00;//Microsoft vendor id
        adv_data[15] = 0X03;//Microsoft beacon id
        adv_data[16] = 0X00;//Microsoft beacon id sub scenario
        adv_data[17] = 0X80;//rssi 
        length = 18;
        
    }

    #if (SYNC_WORD_2G4 == SYNC_WORD_BLE)
    // 只在配对或2.4G模式显示名称. 2.4G模式可以不显示(Todo ....)
    if ((work_mode == B24G_MODE) || (poweron_work_status == SYS_PARING))
    #else
    if (poweron_work_status == SYS_PARING)
    #endif
    {
        name_len=app_name_get(DEV_NAME_MAX_LEN,&adv_data[length+2]);
        adv_data[length] = name_len + 1;

        adv_data[length+1] = GAP_AD_TYPE_COMPLETE_NAME; // 0x09

        length += (name_len+2);
    }
    
    gapm_set_adv_data(actv_env.advidx, GAPM_SET_ADV_DATA, length, adv_data);
}

static void app_adv_set_scan_rsp(void)
{
    if (poweron_work_status == SYS_CONNECT_BACK)
    {
        gapm_set_adv_data(actv_env.advidx, GAPM_SET_SCAN_RSP_DATA, 0, 0);
    }
    else
    {    
        uint8_t length;
        uint8_t rsp_data[DEV_NAME_MAX_LEN + 2];

        // Set device name
        length      = app_name_get(DEV_NAME_MAX_LEN, &rsp_data[2]);
        rsp_data[0] = length + 1;
        rsp_data[1] = GAP_AD_TYPE_COMPLETE_NAME; // 0x09
        gapm_set_adv_data(actv_env.advidx, GAPM_SET_SCAN_RSP_DATA, length + 2, rsp_data);
    }
}

/**
 ****************************************************************************************
 * @brief Action/Command of Advertising
 *
 * @param[in] actv_op  Operation of activity
 ****************************************************************************************
 */
void app_adv_action(uint8_t actv_op)
{
    switch (actv_op)
    {
        case ACTV_CREATE:
        {
            if (actv_env.advsta == ACTV_STATE_OFF)
            {
                //DEBUG("Creating");
                app_adv_create();
                actv_env.advsta = ACTV_STATE_CREATE;
            }
        } break;

        case ACTV_START:
        {
            if (actv_env.advsta == ACTV_STATE_READY)
            {
                DEBUG("Starting");
                gapm_start_advertising(actv_env.advidx, APP_ADV_DURATION);
                actv_env.advsta = ACTV_STATE_START;
            }
        } break;
        
        case ACTV_STOP:
        {
            if (actv_env.advsta == ACTV_STATE_START)
            {
                DEBUG("Stopping");
                gapm_stop_activity(actv_env.advidx);
                actv_env.advsta = ACTV_STATE_STOP;
            }
        } break;
        
        case ACTV_DELETE:
        {
            if ((actv_env.advsta != ACTV_STATE_OFF) && (actv_env.advsta != ACTV_STATE_START))
            {
                DEBUG("Deleting");
                gapm_delete_activity(actv_env.advidx);
                actv_env.advsta = ACTV_STATE_OFF;
            }
        } break;
        
        case ACTV_RELOAD:
        {
            if (actv_env.advsta == ACTV_STATE_START)
            {
                DEBUG("Stopping");
                gapm_stop_activity(actv_env.advidx);
                actv_env.advsta = ACTV_STATE_STOP;
            }
            
            if ((actv_env.advsta != ACTV_STATE_OFF) && (actv_env.advsta != ACTV_STATE_START))
            {
                DEBUG("Deleting");
                gapm_delete_activity(actv_env.advidx);
                actv_env.advsta = ACTV_STATE_OFF;
            }
            
            app_adv_create();
            actv_env.advsta = ACTV_STATE_CREATE;
        } break;
        
        default:
            break;
    }
}

/**
 ****************************************************************************************
 * @brief Event Procedure of Advertising 
 *
 * @param[in] gapm_op  Operation of gapm
 * @param[in] status   Status of event
 ****************************************************************************************
 */
void app_adv_event(uint8_t gapm_op, uint8_t status)
{
    DEBUG("Evt(op:0x%X,sta:0x%X)", gapm_op, status);
    
    switch (gapm_op)
    {
        case (GAPM_CREATE_ADV_ACTIVITY):
        {
            app_adv_set_adv_data();
        } break;
        
        case (GAPM_SET_ADV_DATA):
        {
            app_adv_set_scan_rsp();
        } break;
        
        case (GAPM_SET_SCAN_RSP_DATA):
        {
            actv_env.advsta = ACTV_STATE_READY;
            
            app_adv_action(ACTV_START);            
            app_state_set(APP_READY);
        } break;
        
        case (GAPM_STOP_ACTIVITY):
        {
            if ((actv_env.advsta == ACTV_STATE_START) || (actv_env.advsta == ACTV_STATE_STOP))
            {
                actv_env.advsta = ACTV_STATE_READY;
            }
        } break;
        
        default:
            break;
    }
}

#endif //(BLE_EN_ADV)


/**
 ****************************************************************************************
 * @brief Create activities when Initialization complete.
 ****************************************************************************************
 */
void app_actv_create(void)
{
    memset(&actv_env, 0, sizeof(actv_env));
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
