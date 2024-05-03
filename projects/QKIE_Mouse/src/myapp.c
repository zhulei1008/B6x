/**
 ****************************************************************************************
 *
 * @file myapp.c
 *
 * @brief User Application - Override func
 *
 ****************************************************************************************
 */

#include "app.h"
#include "bledef.h"
#include "drvs.h"
#include "leds.h"

#if (DBG_APP)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat,len)
#endif


/*
 * DEFINES
 ****************************************************************************************
 */
uint8_t read_master_addr[6];
extern uint8_t poweron_work_status;

extern uint8_t master_dongle_addr[6];  

#if (CFG_BT_ADDR_ARR)
extern uint32_t bt_addr[2];
#else
extern uint32_t bt_addr1;
extern uint32_t bt_addr0;
#endif

extern uint8_t channle_slect;
extern uint8_t ltk_data[];
extern uint8_t work_mode;

__attribute__((aligned (4))) struct gapc_ltk gLTK;

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

        #if (CFG_LTK_STORE)
        fshc_read(LTK_STORE_OFFSET, (uint32_t *)&gLTK, sizeof(struct gapc_ltk) >> 2, FSH_CMD_RD);
        debugHex((uint8_t *)&gLTK, sizeof(struct gapc_ltk));
        #else
        memcpy(&gLTK, &DFLT_LTK, sizeof(struct gapc_ltk));
        #endif

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
    }
}

#if (LED_PLAY)
/**
 ****************************************************************************************
 * @brief API to Set State of Application, add leds Indication
 *
 * @param[in] state    new state
 ****************************************************************************************
 */
void app_state_set(uint8_t state)
{
    DEBUG("State(old:%d,new:%d)", app_state_get(), state);

    app_env.state = state;
    
    // Indication, User add more...
    if(state==(APP_ENCRYPTED - work_mode))
    {
        poweron_work_status = SYS_CONNECT;
        #if (LED_PLAY)
        leds_play(LED_CONT_ON);
        leds_play(LED_CONNECT_BACK);
        #endif
    }
}
#endif //(LED_PLAY)

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
    if (memcmp(read_master_addr,master_dongle_addr,6) != 0)
    {
        memcpy(&ltk_data[channle_slect*28],(uint8_t* )ltk,28);
        #if (CFG_BT_ADDR_ARR)
        GLOBAL_INT_DISABLE();
        fshc_erase(LTK_STORE_OFFSET, FSH_CMD_ER_PAGE);
        fshc_write(LTK_STORE_OFFSET/*+(channle_slect*28)*/, (uint32_t *)ltk_data, 64/*sizeof(struct gapc_ltk) >> 2*/, FSH_CMD_WR);
        
        fshc_erase(BT_MAC_STORE_OFFSET,FSH_CMD_ER_PAGE);
        fshc_write(BT_MAC_STORE_OFFSET, bt_addr, 64, FSH_CMD_WR);
        GLOBAL_INT_RESTORE();
        #else
        GLOBAL_INT_DISABLE();
        fshc_erase(LTK_STORE_OFFSET, FSH_CMD_ER_PAGE);
        fshc_write(LTK_STORE_OFFSET/*+(channle_slect*28)*/, (uint32_t *)ltk_data, 14/*sizeof(struct gapc_ltk) >> 2*/, FSH_CMD_WR);
        fshc_erase(BT_MAC_STORE_OFFSET,FSH_CMD_ER_PAGE);
        //if(channle_slect)
        fshc_write(BT_MAC_STORE_OFFSET+4, &bt_addr1, 1, FSH_CMD_WR);
        //else 
        fshc_write(BT_MAC_STORE_OFFSET, &bt_addr0, 1, FSH_CMD_WR);
        GLOBAL_INT_RESTORE();
        #endif
    }
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
    fshc_read(LTK_STORE_OFFSET+(channle_slect*28), (uint32_t *)&gLTK, sizeof(struct gapc_ltk) >> 2, FSH_CMD_RD);
    debugHex((uint8_t *)&gLTK, sizeof(struct gapc_ltk));
#endif  

    if ((ediv == gLTK.ediv) && (memcmp(rand_nb, gLTK.randnb.nb, 8) == 0))
        return &gLTK.ltk.key[0];
    else
        return NULL;
}
