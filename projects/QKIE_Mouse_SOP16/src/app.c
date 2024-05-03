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

#include "app.h"
#include "bledef.h"
#include "drvs.h"
#include "prf_api.h"
#include "user_api.h"
#include "hid_desc.h"
#include "leds.h"
#if (DBG_APP)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat,len)
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

uint32_t g_sync_word;

__DATA_ALIGNED(4) struct gapc_ltk gLTK;

/// Application Environment
__VAR_ENV struct app_env_tag app_env;

/// Ble local address (user customize)
bd_addr_t ble_dev_addr  = { BLE_ADDR };

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
 * @brief Finite state machine for Device Configure, maybe User Override!
 *
 * @param[in] evt   configure event @see enum ble_event
 ****************************************************************************************
 */
void app_conf_fsm(uint8_t evt)
{
    if (evt == BLE_RESET)
    {
        // Init Environment
        memset(&app_env, 0, sizeof(app_env));

        // Set device config
        gapm_set_dev(&ble_dev_config, &ble_dev_addr, NULL);
    }
    else /*if (evt == BLE_CONFIGURED)*/
    {
        #if (CFG_SLEEP)
        // Set Sleep duration if need

        #if (RC32K_CALIB_PERIOD)
        ke_timer_set(APP_TIMER_RC32K_CORR, TASK_APP, RC32K_CALIB_PERIOD);
        #endif //(RC32K_CALIB_PERIOD)
        #endif //(CFG_SLEEP)

        app_state_set(APP_IDLE);

        // Create Profiles
        app_prf_create();

        // Create Activities
        app_actv_create();
        
        ble_2G4_set((g_sync_word & 0xFFFF), ((g_sync_word >> 16) & 0xFFFF));
        
//        ble_txmd_set(1);
    }
}

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
    
    // Generic Attribute Profile(0x1801)
    #if (BLE_NB_SLAVE)
    gatt_svc_init(GATT_START_HDL);
    #endif

    // Standard Profiles
    #if (PRF_DISS)
    diss_svc_init();
    #endif
    
    #if (PRF_BASS)
    bass_svc_init();
    #endif
    
    #if (PRF_HIDS)
    hids_prf_init();
    #endif
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
#include "reg_rf.h"
struct rf_cal_env_tag
{
    uint32_t bpf_ctrl;
    uint32_t dac_tab0;
    uint32_t dac_tab1;
    uint32_t dac_tab2; // bit[11:0] - dac_tab2, bit[15:12] - pa_target, bit[31:16] - cal_valid
    //uint32_t pll_dym_ctrl;
};
extern struct rf_cal_env_tag rf_cal_env;
__weak void app_init(uint16_t rsn)
{
    // Init BLE and App to startup or Resume BLE to continue if it wakeup from poweroff.
    #if (BLE_LITELIB)
    if (RSN_IS_BLE_WKUP(rsn))
    {
        // Resume BLE (Only supported in LiteLib)
        ble_resume();
    }
    else
    #endif //(BLE_LITELIB)
    {
        heap_cfg_t heap;
        
        // Config Heap, resized with special lib
        heap.base[MEM_ENV] = BLE_HEAP_BASE;
        heap.size[MEM_ENV] = BLE_HEAP_ENV_SIZE;
        heap.base[MEM_MSG] = BLE_HEAP_BASE + BLE_HEAP_ENV_SIZE;
        heap.size[MEM_MSG] = BLE_HEAP_MSG_SIZE;
        ble_heap(&heap);

        // poweroff唤醒不重新校准rf.
        uint32_t tmp_val = (rsn & RSN_POR12_CORE_BIT) ? rf_cal_env.dac_tab2 : 0;
        // Init BLE and App
        ble_init();
        ble_app();
        rf_cal_env.dac_tab2 = tmp_val;
        
        // Init RC32K with Calibration
        #if (CFG_SLEEP || RC32K_CALIB_PERIOD)
        //rc32k_init(); - replace to watch calib result
        rc32k_conf(RCLK_DPLL128, 3); // 1.8ms

        uint16_t trim = rc32k_calib();
        DEBUG("RC32K Calib(Msb:%d,Lsb:%d)", trim & 0xF, trim >> 4);
        #endif //(CFG_SLEEP || RC32K_CALIB_PERIOD)
    }

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
void app_state_set(uint8_t state)
{
    DEBUG("State(old:%d,new:%d)", app_state_get(), state);

    app_env.state = state;
    
    // Indication, User add more...
    if (state == (APP_ENCRYPTED - work_mode))
    {
        poweron_work_status = SYS_CONNECT;
        #if (LED_PLAY)
        leds_play(LED_CONT_ON);
        leds_play(LED_CONNECT_BACK);
        #endif
    }
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
        
        if(channle_slect)
        {
            name[len++] = 0x31;
        }
        else
        {
            name[len++] = 0x30;
        }
        //name[len++] = co_hex(ble_dev_addr.addr[0] >> 4);
        //name[len++] = co_hex(ble_dev_addr.addr[0] & 0x0F);
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
 * @brief Finite state machine for connection event, maybe User Override! (__weak func)
 *
 * @param[in] evt     connection event @see enum ble_event
 * @param[in] conidx  connection index
 * @param[in] param   param of connection event
 ****************************************************************************************
 */
extern uint8_t g_poweron_key_sta;
__weak void app_conn_fsm(uint8_t evt, uint8_t conidx, const void* param)
{
    switch (evt)
    {
        case BLE_CONNECTED:
        {
            // Connected state, record Index
            app_env.curidx = conidx;
            app_state_set(APP_CONNECTED);

            gapc_connect_rsp(conidx, BLE_AUTH);
            
            // 从poweroff唤醒时发送唤醒的按键
            if ((work_mode == B24G_MODE) && (g_rst_rsn == (RSN_IO_WKUP_BIT | RSN_POR12_CORE_BIT))&&(g_poweron_key_sta))
            {
                uint8_t rpt_data[4] = {g_poweron_key_sta, 0, 0, 0};
                mouse_report_send(app_env.curidx, rpt_data);
//                memset(rpt_data,0,4);
//                mouse_report_send(app_env.curidx, rpt_data);  
                g_poweron_key_sta = 0;
                ke_timer_set(APP_SEND_RELSASE_LR,TASK_APP,500);
            }
        } break;
        
        case BLE_DISCONNECTED:
        {
            uint8_t dis_rsn = LE_ERR_HCI2HL(*((uint8_t *)param + offsetof(struct gapc_disconnect_ind, reason)));
            
            DEBUG("dis_rsn:%x\r\n", dis_rsn);
            
            if ((dis_rsn == LL_ERR_REMOTE_USER_TERM_CON) || (dis_rsn == LL_ERR_REMOTE_DEV_POWER_OFF))
            {
                mouse_enter_powerdown();
            }
            if (work_mode != B24G_MODE)
            {
                if (poweron_work_status == SYS_CONNECT)
                {
                    NVIC_SystemReset();
                }
            }
            else
            {
                poweron_work_status = SYS_CONNECT_BACK;
                sys_timeout_cnt=0;;
            }
            app_state_set(APP_READY);
            
            #if (BLE_EN_ADV)
            // Slave role, Restart Advertising
            app_adv_action(ACTV_START);
            #endif //(BLE_EN_ADV)
        } break;
        
        case BLE_BONDED:
        {
            // todo, eg. save the generated slave's LTK to flash
            app_state_set(APP_ENCRYPTED);
            if(g_sync_word == SYNC_WORD_BLE)
            {
                update_connect_par_flag =1;
                ke_timer_set(APP_DELAY_UPDATA, TASK_APP, 5000);
            }

        } break;
        
        case BLE_ENCRYPTED:
        {
            // todo
            app_state_set(APP_ENCRYPTED);
        } break;
        
        default:
            break;
    }
}

/**
 ****************************************************************************************
 * @section SMP Interface
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief API to Get Pairing Feature, maybe User Override! (__weak func)
 *
 * @param[out] feat   Pointer of pairing buffer to fill
 ****************************************************************************************
 */
__weak void app_pairing_get(struct gapc_pairing *feat)
{
    // IO capabilities (@see gap_io_cap)
    feat->iocap     = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    // OOB information (@see gap_oob)
    feat->oob       = GAP_OOB_AUTH_DATA_NOT_PRESENT;
    // Authentication (@see gap_auth)
    feat->auth      = BLE_AUTH;
    // Encryption key size (7 to 16)
    feat->key_size  = GAP_KEY_LEN;
    //Initiator key distribution (@see gap_kdist)
    feat->ikey_dist = GAP_KDIST_NONE;
    //Responder key distribution (@see gap_kdist)
    feat->rkey_dist = GAP_KDIST_ENCKEY;
    // Device security requirements (@see gap_sec_req)
    feat->sec_req   = BLE_SECREQ;
}

/**
 ****************************************************************************************
 * @brief API to Generate LTK for bonding, maybe User Override! (__weak func)
 *
 * @param[in]     conidx   connection index
 * @param[in|out] ltk      Pointer of ltk buffer
 ****************************************************************************************
 */
void app_ltk_gen(uint8_t conidx, struct gapc_ltk *ltk)
{
    DEBUG("LTK Gen");
    
#if (CFG_LTK_STORE)
    // Generate all the values
    gLTK.ediv = rand_hword();

    for (int i = 0; i < GAP_RAND_NB_LEN; i++)
    {
        gLTK.ltk.key[i]   = (uint8_t)rand_word();
        gLTK.randnb.nb[i] = (uint8_t)rand_word();
        gLTK.ltk.key[8+i] = (uint8_t)rand_word();
    }
#endif
    
    memcpy(ltk, &gLTK, sizeof(struct gapc_ltk));
    debugHex((uint8_t *)ltk, sizeof(struct gapc_ltk));
}

/**
 ****************************************************************************************
 * @brief API to Save LTK when bonded, maybe User Override! (__weak func)
 *
 * @param[in] conidx   connection index
 * @param[in] ltk      Pointer of LTK data
 ****************************************************************************************
 */
void app_ltk_save(uint8_t conidx, const struct gapc_ltk *ltk)
{
    DEBUG("LTK Saved Start");
    
    #if (CFG_LTK_STORE)
    memcpy(&ltk_data[channle_slect*28], (uint8_t* )ltk, 28);
   
    flash_erase_write(LTK_STORE_OFFSET, (uint32_t *)ltk_data);
    flash_erase_write(BT_MAC_STORE_OFFSET, bt_addr);
    
    #endif
    DEBUG("LTK Saved Done");
}

/**
 ****************************************************************************************
 * @brief API to Find LTK when re-encryption, maybe User Override! (__weak func)
 *
 * @param[in] ediv     EDIV value for matching
 * @param[in] rand_nb  Rand Nb values for matching
 *
 * @return NULL for not matched, else return Pointer of LTK found.
 ****************************************************************************************
 */

const uint8_t *app_ltk_find(uint16_t ediv, const uint8_t *rand_nb)
{
    DEBUG("Read LTK");
#if (CFG_LTK_STORE)
//    fshc_read(LTK_STORE_OFFSET+(channle_slect*28), (uint32_t *)&gLTK, sizeof(struct gapc_ltk) >> 2, FSH_CMD_RD);
    flash_read(LTK_STORE_OFFSET+(channle_slect*28), (uint32_t *)&gLTK, BLEN2WLEN(sizeof(struct gapc_ltk)));
    
    debugHex((uint8_t *)&gLTK, sizeof(struct gapc_ltk));
#endif  

    if ((ediv == gLTK.ediv) && (memcmp(rand_nb, gLTK.randnb.nb, 8) == 0))
        return &gLTK.ltk.key[0];
    else
        return NULL;
}
