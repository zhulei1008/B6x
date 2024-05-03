/**
 ****************************************************************************************
 *
 * @file app.c
 *
 * @brief Application entry point - Example
 *
 * < __weak func as demo, recommend to Override its in 'user porject'/src/myapp.c >
 ****************************************************************************************
 */

#include "app_user.h"
#include "app.h"
#include "bledef.h"
#include "drvs.h"
#include "prf_api.h"

#if (DBG_APP)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif


/*
 * DEFAULT CONFIGURATION
 ****************************************************************************************
 */

#if !defined(BLE_DEV_NAME)
    #define BLE_DEV_NAME        "myBle6"
#endif

#if !defined(BLE_ADDR)
    #define BLE_ADDR            { 0x30, 0x06, 0x23, 0x20, 0x01, 0xD2 }
#endif

#if !defined(BLE_ROLE)
#if (BLE_NB_SLAVE && BLE_NB_MASTER)
    #define BLE_ROLE            (GAP_ROLE_CENTRAL | GAP_ROLE_PERIPHERAL)
#elif (BLE_NB_MASTER)
    #define BLE_ROLE            (GAP_ROLE_CENTRAL)
#else  // Only Slave
    #define BLE_ROLE            (GAP_ROLE_PERIPHERAL)
#endif
#endif

#if !defined(BLE_PHY)
    #define BLE_PHY             (GAP_PHY_LE_1MBPS) // | GAP_PHY_LE_2MBPS)
#endif

#if !defined(BLE_PAIRING)
    #define BLE_PAIRING         (GAPM_PAIRING_LEGACY)
#endif

#if !defined(BLE_AUTH)
    #define BLE_AUTH            (GAP_AUTH_REQ_NO_MITM_NO_BOND)
#endif

#if !defined(BLE_SECREQ)
    #define BLE_SECREQ          (GAP_NO_SEC)
#endif

#if !defined(BLE_SYNC_WORD)
#define BLE_SYNC_WORD          (0x8E89BED6)
#endif
#define SYNC_WORD_L            ((BLE_SYNC_WORD >> 0)  & 0xFFFF)
#define SYNC_WORD_H            ((BLE_SYNC_WORD >> 16) & 0xFFFF)

/*
 * VARIABLES DEFINITIONS
 ****************************************************************************************
 */

/// Application Environment
__VAR_ENV volatile struct app_env_tag app_env;

/// Ble local address (user customize)
bd_addr_t ble_dev_addr = { BLE_ADDR };

/// GAP device configuration
const struct gapm_dev_config ble_dev_config =
{
    // Device Role: Central, Peripheral (@see gap_role)
    .gap_role  = BLE_ROLE,

    // Pairing mode authorized (@see enum gapm_pairing_mode)
    .pairing   = BLE_PAIRING,

    // Preferred LE PHY for data (@see enum gap_phy)
    .pref_phy  = BLE_PHY,

    // Maximal MTU acceptable for device (23~512)
    .max_mtu   = BLE_MTU,
};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @section Profile Interface
 ****************************************************************************************
 */

// Connection interval unit in 1.25ms
#define SLV_PREF_INTV_MIN       (10)
#define SLV_PREF_INTV_MAX       (10)
// Slave latency
#define SLV_PREF_LATENCY        (0)
// Connection supervision timeout multiplier unit in 10ms
#define SLV_PREF_TIME_OUT       (300)

/**
 ****************************************************************************************
 * @brief Retrieve Dev Info to Generic Access Profile, User implement for Callback.
 *
 * @param[in]  conidx  connection index
 * @param[in]  req     iequest of info type @see enum gapc_dev_info
 * @param[in]  maxlen  buffer length, DEV_NAME_MAX_LEN or size of gapc_conn_param
 * @param[out] info    pointer of buffer
 *
 * @return Length of device information, 0 means an error occurs.
 ****************************************************************************************
 */
__weak uint16_t gap_svc_get_dev_info(uint8_t conidx, uint8_t req, uint16_t maxlen, uint8_t *info)
{
    DEBUG("dev_info_req:%d", req);
    if (req == GAPC_DEV_NAME)
    {
        return app_name_get(DEV_NAME_MAX_LEN, info);
    }
    else if (req == GAPC_DEV_APPEARANCE)
    {
        write16(info, app_icon_get());
        return sizeof(uint16_t);
    }
    #if (GAP_ATT_CFG & 0x40/*PCP_EN*/)
    else if (req == GAPC_DEV_SLV_PREF_PARAMS)
    {
        struct gapc_conn_param *slv_pref = (struct gapc_conn_param *)info;
        // Peripheral Preferred Connection Parameters
        slv_pref->intv_min = SLV_PREF_INTV_MIN;
        slv_pref->intv_max = SLV_PREF_INTV_MAX;
        slv_pref->latency  = SLV_PREF_LATENCY;
        slv_pref->time_out = SLV_PREF_TIME_OUT;
        return sizeof(struct gapc_conn_param);
    }
    #endif
    
    return 0;
}

/**
 ****************************************************************************************
 * @brief Create profiles, maybe User Override! (__weak func)
 *        Added in order and judged status in each profile-func.
 ****************************************************************************************
 */
__weak void app_prf_create(void)
{
    // Generic Access Profile(0x1800)
    gap_svc_init(GAP_START_HDL, GAP_ATT_CFG);
}


/**
 ****************************************************************************************
 * @section App Interface
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief API to Init Application, maybe User Override! (__weak func)
 *
 * @param[in] rsn   reset reason @see enum rst_src_bfs
 ****************************************************************************************
 */
__weak void app_init(uint16_t rsn)
{
    // Init BLE and App to startup or Resume BLE to continue if it wakeup from poweroff.
    heap_cfg_t heap;

    // Config Heap, resized with special lib
    heap.base[MEM_ENV] = BLE_HEAP_BASE;
    heap.size[MEM_ENV] = BLE_HEAP_ENV_SIZE;
    heap.base[MEM_MSG] = BLE_HEAP_BASE + BLE_HEAP_ENV_SIZE;
    heap.size[MEM_MSG] = BLE_HEAP_MSG_SIZE;
    ble_heap(&heap);

    // Init BLE and App
    ble_init();
    
    ble_app();

    // Init RF & Modem
    rfmdm_init();

    NVIC_EnableIRQ(BLE_IRQn);
}

/**
 ****************************************************************************************
 * @brief API to Set State of Application, maybe User Override! (__weak func)
 *
 * @param[in] state    new state
 ****************************************************************************************
 */
__weak void app_state_set(uint8_t state)
{
    DEBUG("State(old:%d,new:%d)", app_state_get(), state);

    app_env.state = state;
    
    // Indication, User add more...

}

/**
 ****************************************************************************************
 * @brief API to Get Device Name, maybe User Override! (__weak func)
 *
 * @param[in]  size   Length of name Buffer
 * @param[out] name   Pointer of name buffer
 *
 * @return Length of device name
 ****************************************************************************************
 */
__weak uint8_t app_name_get(uint8_t size, uint8_t *name)
{
    uint8_t len = sizeof(BLE_DEV_NAME) - 1;

    // eg. prefix(BLE_DEV_NAME) + suffix(Addr[0])
    if (size < len + 2)
    {
        // no enough buffer, short copy
        len = size;
        memcpy(name, BLE_DEV_NAME, len);
    }
    else
    {
        // prefix + suffix
        memcpy(name, BLE_DEV_NAME, len);
        name[len++] = co_hex(ble_dev_addr.addr[0] >> 4);
        name[len++] = co_hex(ble_dev_addr.addr[0] & 0x0F);
    }

    return len;
}


/**
 ****************************************************************************************
 * @section FSM Interface
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Finite state machine for Device Configure, maybe User Override! (__weak func)
 *
 * @param[in] evt   configure event @see enum ble_event
 ****************************************************************************************
 */
__weak void app_conf_fsm(uint8_t evt)
{
    if (evt == BLE_RESET)
    {
        memset((uint8_t *)&app_env, 0, sizeof(app_env));

        // Set device config
        gapm_set_dev(&ble_dev_config, &ble_dev_addr, NULL);
    }
    else /*if (evt == BLE_CONFIGURED)*/
    {
        app_state_set(APP_IDLE);

        // Create Profiles
        app_prf_create();

        // Create Activities
        app_actv_create();
        
        ble_2G4_set(SYNC_WORD_L, SYNC_WORD_H);
        memset((uint8_t *)user_scan_addr_list_store, 0x00, SCAN_NUM_MAX * sizeof(struct bd_addr_icon));
        init_timer_start();
    }
}

/**
 ****************************************************************************************
 * @brief Finite state machine for connection event, maybe User Override! (__weak func)
 *
 * @param[in] evt     connection event @see enum ble_event
 * @param[in] conidx  connection index
 * @param[in] param   param of connection event
 ****************************************************************************************
 */
__weak void app_conn_fsm(uint8_t evt, uint8_t conidx, const void* param)
{
    switch (evt)
    {
        case BLE_CONNECTED:
        {
            // Set Connection Bit of conidx
            app_env.conbits |= (1 << conidx);
            
            // Connected state, record Index
            app_env.curidx = conidx;
            app_state_set(APP_CONNECTED);

            DEBUG("  multi(cbit:%02X,curr:%d)", app_env.conbits, app_env.curidx);
            gapc_connect_rsp(conidx, BLE_AUTH);

            // 2个都连接上, 停止扫描
            if (app_env.conbits == 3)
            {
                app_scan_action(ACTV_STOP);

                init_timer_stop();
            }
            
            // 保持和主机指示灯同步
            if ((g_cid_kb == conidx) && (g_kb_led != 0xFF))
            {
                if(g_os_type)
                {
                    g_kb_led|=g_os_type;
                    g_os_type = 0;
                }  
                gatt_write(g_cid_kb, GATT_WRITE_NO_RESPONSE, GATT_WR_HDL, (uint8_t *)&g_kb_led, 1);
                g_kb_led&=0x0f;
                  DEBUG("led");                                
            }
            
        } break;
        
        case BLE_DISCONNECTED:
        {
            DEBUG("DIS 0 ---g_cid_kb:%x, cbits:%x, env:%d, scan_cnt:%d, (conn icon:0x%X, 0x%X)", g_cid_kb, app_env.conbits, app_env.state, scan_cnt, g_conn_icon_type[0], g_conn_icon_type[1]);
            
            
            if (app_env.conbits == 3)
            {
                scan_cnt = 1;

                init_timer_start();

                app_scan_action(ACTV_START);
            }
            
            // Clr Connection Bit of conidx
            app_env.conbits &= ~(1 << conidx);

            if (app_env.conbits == 0)
            {
                g_conn_icon_type[0] = 0;
                g_conn_icon_type[1] = 0;

                scan_cnt = 0;
                g_cid_kb = 0xFF;
                g_os_type = g_os_type_back ;
                // Go READY when all disconnected
                app_state_set(APP_READY);

                app_env.curidx = 0;

                DEBUG("XXX disconnect all XXX");
            }
            else
            {
                // Find new index when curr disconnected
                if (conidx == app_env.curidx)
                {
                    // Least index of connection or * User customize *
                    app_env.curidx = co_ctz(app_env.conbits);
                }

                if (g_conn_icon_type[app_env.conbits & 0x01] == ICON_KB)
                {
                    g_cid_kb = 0xFF;
                    g_os_type = g_os_type_back ;
                }

                g_conn_icon_type[app_env.conbits & 0x01] = 0;
            }
           
            DEBUG("DIS 1 ---g_cid_kb:%x, cbits:%x, env:%d, scan_cnt:%d, (conn icon:0x%X, 0x%X)", g_cid_kb, app_env.conbits, app_env.state, scan_cnt, g_conn_icon_type[0], g_conn_icon_type[1]);
        } break;

        default:
            break;
    }
}
