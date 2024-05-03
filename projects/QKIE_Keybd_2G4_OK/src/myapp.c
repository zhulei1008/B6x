/**
 ****************************************************************************************
 *
 * @file myapp.c
 *
 * @brief User Application - Override func
 *
 ****************************************************************************************
 */
#include "regs.h"
#include "gatt_api.h"
#include "app.h"
#include "app_actv.h"
#include "bledef.h"
#include "drvs.h"
#include "leds.h"
#include "cfg.h"
#include "hid_desc.h"
#include "keyboard.h"
#include "hidkey.h"
#include "gapm_api.h"
#include "user_api.h"
/*
 * DECLARATION
 ****************************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************************
 */
#if (DBG_MYAPP)
#include "dbg.h"
#define DEBUG(format, ...) debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif
/// Ble local address (user customize)
const bd_addr_t ble_Dev_Addr1 = {BLE_ADDR_1};
const bd_addr_t ble_Dev_Addr2 = {BLE_ADDR_2};
const bd_addr_t ble_Dev_Addr3 = {BLE_ADDR_3};

__DATA_ALIGNED(4) uint8_t bt_Addr[20] = {0};

enum bt_addr_idx
{
    BT_ADDR_0 = 0,
    BT_ADDR_1 = 6,
    BT_ADDR_2 = 12,
};

uint16_t sys_Timeout_Cnt;
uint8_t  powerOn_Work_Status;
uint8_t  conn_intv;
uint8_t  conn_late;

uint8_t  read_Master_Addr[6];
uint8_t  masterDongle_Addr[6];
uint32_t channle_Select;

const uint8_t def_MasterDongle_Addr[] = {0x11, 0x88, 0x3F, 0xA1, 0x01, 0xF3};

__DATA_ALIGNED(4) uint8_t ltk_Data[256];
__DATA_ALIGNED(4) uint8_t os_mode_table[4] = {0xFF, 0xFF, 0xFF, 0xFF};

__DATA_ALIGNED(4) struct gapc_ltk gLTK; // = {

//    /// Long Term Key
//    .ltk = {{0x88, 0x0D, 0x00, 0x20, 0xAC, 0x32, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00}},
//    /// Encryption Diversifier
//    .ediv = 0xB951,
//    /// Random Number
//    .randnb = {{0x02, 0x18, 0xAC, 0x32, 0x00, 0x20, 0x00, 0x00}},
//    /// Encryption key size (7 to 16)
//    .key_size = 16,
//    /// Extend Info
//    .ext_info = 0,
//};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

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
#if (LED_PLAY)
    if (state == APP_IDLE)
    {
        // Leds_Play(LED_SLOW_BL);
    }
    else if (state == APP_READY)
    {
        // Leds_Play(LED_FAST_BL);
    }
    else if (state == APP_CONNECTED)
    {
        // Leds_Play(LED_CONT_ON);
    }
#endif //(LED_PLAY)
    #if (CFG_2G4_MODE)
    if (state == (APP_ENCRYPTED - work_mode))
    #else
    if (state == APP_ENCRYPTED)
    #endif // CFG_2G4_MODE
    {
        powerOn_Work_Status = SYS_CONNECT;

#if (LED_PLAY)
// Leds_Play(LED_CONT_ON);
// Leds_Play(LED_CONNECT_BACK);
#endif
    }
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
        gLTK.ltk.key[i]     = (uint8_t)rand_word();
        gLTK.ltk.key[i + 8] = (uint8_t)rand_word();
        gLTK.randnb.nb[i]   = (uint8_t)rand_word();
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
    if (memcmp(read_Master_Addr, masterDongle_Addr, 6) != 0)
    {
        memcpy(&ltk_Data[channle_Select * 28], (uint8_t *)ltk, 28);

        flash_erase_write(LTK_STORE_OFFSET, (uint32_t *)ltk_Data);
        flash_erase_write(BT_MAC_USER_OFFSET, (uint32_t *)&bt_Addr);
    }
#endif

    os_judge_init();
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

    // uint16_t ediv_b;

#if (CFG_LTK_STORE)
    fshc_read(LTK_STORE_OFFSET + (channle_Select * 28), (uint32_t *)&gLTK, sizeof(struct gapc_ltk) >> 2, FSH_CMD_RD);
    debugHex((uint8_t *)&gLTK, sizeof(struct gapc_ltk));
#endif

    if ((ediv == gLTK.ediv) && (memcmp(rand_nb, gLTK.randnb.nb, 8) == 0))
    {
        return &gLTK.ltk.key[0];
    }
    else
    {
        return NULL;
    }
}

void Read_Bt_Mac_Ltk_Info(void)
{
    uint8_t i = 0;

    // bt_Addr的处理
    uint32_t Programmer_Bt_Addr = 0;

    // bt_channle的处理
    fshc_read(BT_CNANNLE_OFFSET, &channle_Select, 1, FSH_CMD_RD); // 读BT通道
    if (channle_Select == 0xFFFFFFFF)
    {
        channle_Select = 0;                                       // 烧录后第一次上电时，默认使用1通道

        fshc_read(BT_MAC_STORE_OFFSET, &Programmer_Bt_Addr, 1, FSH_CMD_RD);

        if (Programmer_Bt_Addr == 0xFFFFFFFF)
        {
            memcpy((uint8_t *)&bt_Addr[BT_ADDR_0], &ble_Dev_Addr1.addr[0], 6); // 1通道地址
            memcpy((uint8_t *)&bt_Addr[BT_ADDR_1], &ble_Dev_Addr2.addr[0], 6); // 2通道地址
            memcpy((uint8_t *)&bt_Addr[BT_ADDR_2], &ble_Dev_Addr3.addr[0], 6); // 3通道地址
        }
        else
        {
            memcpy((uint8_t *)&bt_Addr[BT_ADDR_0 + 2], (uint8_t *)&Programmer_Bt_Addr, 4); // 1通道地址
            memcpy((uint8_t *)&bt_Addr[BT_ADDR_1 + 2], (uint8_t *)&Programmer_Bt_Addr, 4); // 2通道地址
            memcpy((uint8_t *)&bt_Addr[BT_ADDR_2 + 2], (uint8_t *)&Programmer_Bt_Addr, 4); // 3通道地址

            bt_Addr[BT_ADDR_0]      = 0;
            bt_Addr[BT_ADDR_0 + 1]  = 0;
            bt_Addr[BT_ADDR_0 + 5] -= 1;

            bt_Addr[BT_ADDR_1]     = 0;
            bt_Addr[BT_ADDR_1 + 1] = 0;

            bt_Addr[BT_ADDR_2]      = 0;
            bt_Addr[BT_ADDR_2 + 1]  = 0;
            bt_Addr[BT_ADDR_2 + 5] += 1;
        }

        flash_erase_write(BT_MAC_USER_OFFSET, (uint32_t *)&bt_Addr);
        flash_erase_write(BT_CNANNLE_OFFSET, &channle_Select);
    }
    else
    {
        fshc_read(BT_MAC_USER_OFFSET, (uint32_t *)&bt_Addr, 5, FSH_CMD_RD);
        DEBUG("READ-ADDR");
    }
    DEBUG("addr_0:");
    debugHex((uint8_t *)&bt_Addr[BT_ADDR_0], 6);
    DEBUG("addr_1:");
    debugHex((uint8_t *)&bt_Addr[BT_ADDR_1], 6);
    DEBUG("addr_2:");
    debugHex((uint8_t *)&bt_Addr[BT_ADDR_2], 6);

    DEBUG("POWER_ON_CNANNLE");
    debugHex((uint8_t *)&channle_Select, 4);

#if (CFG_2G4_MODE)
    // Test...
    work_mode = B24G_MODE;
    
    if (work_mode == B24G_MODE)
    {
        g_sync_word = SYNC_WORD_2G4;
        powerOn_Work_Status = SYS_CONNECT_BACK;
        return ;
    }
#endif
    
    // LTK的处理
    fshc_read(LTK_STORE_OFFSET, (uint32_t *)&ltk_Data, 21, FSH_CMD_RD); // 读LTK_FLASH 84字节
    debugHex((uint8_t *)ltk_Data, 84);

    memcpy((uint8_t *)&gLTK, &ltk_Data[channle_Select * 28],
           28); // 将flash里读取的当前channle 28个字节的LTK数据给gapc_LTK
    debugHex((uint8_t *)&gLTK, 28);

    for (i = 0; i < 28; i++)
    {
        if (ltk_Data[(channle_Select * 28) + i] != 0xff) // 判定从LTK_FLASH里读出来的，当前通道LTK数据是不是空的
        {
            break;
        }
    }
    powerOn_Work_Status = (i == 28) ? SYS_IDLE : SYS_CONNECT_BACK; // LTK要是空的系统状态就为空闲，否则回连
#if (DBG_MODE)
// powerOn_Work_Status = SYS_PARING;//DEBUG模式下，强制将系统置为PARING状态
#endif
    DEBUG("powerOn_Work_Status:%d", powerOn_Work_Status);

#if (CFG_BTMR_DLY)
    bootDelayUs(3);
#else
// delay_us(3);
#endif

    // OS的处理
    fshc_read(OS_MODE_FLASH_ADDR, (uint32_t *)&os_mode_table, 1, FSH_CMD_RD);
    if (os_mode_table[channle_Select & 0xff] > SYS_MAX) // 当前通道在flash存储的os类型数据为空时，设置为ANDROID类型
    {
        key_env.sys = ANDROID;
    }
    else
    {
        key_env.sys = os_mode_table[channle_Select & 0xff];
    }
    DEBUG("key_env.sys:%d\r\n", key_env.sys);
}

// 蓝牙地址+1
void BtAddr_Add_One(void)
{
    uint32_t addr = 0;
    memcpy((uint8_t *)&addr, &bt_Addr[(channle_Select * 6)], 4);
    addr += 1;
    memcpy(&bt_Addr[(channle_Select * 6)], (uint8_t *)&addr, 4);
    DEBUG("BTaddr_Add_One_ble_dev_add");
    DEBUG("addr_0:");
    debugHex((uint8_t *)&bt_Addr[BT_ADDR_0], 6);
    DEBUG("addr_1:");
    debugHex((uint8_t *)&bt_Addr[BT_ADDR_1], 6);
    DEBUG("addr_2:");
    debugHex((uint8_t *)&bt_Addr[BT_ADDR_2], 6);
}

// 蓝牙复位
void BtReset(void)
{
    os_judge_env_clr();
    sys_Timeout_Cnt = 0;
    gapm_stop_activity(actv_env.advidx);
    memset(&actv_env, 0, sizeof(actv_env));
    gapm_reset();
}

// 删除配对信息
void Delete_Pair_Info(void)
{
    os_judge_env_clr();
    memset(&ltk_Data, 0xff, 256);

    // 清空LTK的FLASH空间
    flash_erase_write(LTK_STORE_OFFSET, NULL);

    keyboard_go_poweroff_jage();
}

// 蓝牙通道设置
void BT_Channle_Set(void)
{
    if (btChannleChangeFlag)
    {
        // 把改变后的通道写入FLASH
        flash_erase_write(BT_CNANNLE_OFFSET, &channle_Select);

        debugHex((uint8_t *)&channle_Select, 4);

        // 蓝牙链路断开、广播停止
        #if (CFG_2G4_MODE)
        if (app_env.state == (APP_ENCRYPTED - work_mode))
        #else
        if (app_env.state == APP_ENCRYPTED)
        #endif
        {
            gapc_disconnect(app_env.curidx);
        }
        if (actv_env.advsta != ACTV_STATE_READY)
        {
            app_adv_action(ACTV_STOP);
        }

        // 清空按键相关
        memset((uint8_t *)&key_env, 0, sizeof(key_env));

        // 设置新通道的蓝牙地址、LTK信息
        Read_Bt_Mac_Ltk_Info();
        BtReset();
        btChannleChangeFlag = false;
    }
}

__SRAMFN void flash_erase_write(uint32_t offset, uint32_t *data)
{
    GLOBAL_INT_DISABLE();

    while (SYSCFG->ACC_CCR_BUSY);

    fshc_erase(offset, FSH_CMD_ER_PAGE);
    
    if (data != NULL)
    {
        fshc_write(offset, data, 64, FSH_CMD_WR);
    }
    
    GLOBAL_INT_RESTORE();
}

/// Program Size: Code=68268 RO-data=1656 RW-data=24 ZI-data=8848
void zi_data_clr(void)
{
#if (CFG_PIN_ENABLE)
    g_pin_num  = 0;
    g_pin_code = 0;
#endif
    
#if (CFG_2G4_MODE)
    g_sync_word = SYNC_WORD_BLE;
    work_mode = BT_MODE;
#endif
    
    conn_intv = 0;
    conn_late = 0;

    sys_Timeout_Cnt                    = 0;
    led_blink_cnt                      = 0;
    lowLedBlinkCnt                     = 0;
    channle_Select                     = 0;
    sys_poweroff_flag                  = 0;
    updata_conn_status                 = 0;
    connect_complete                   = 0;
    update_connect_par_flag            = 0;
    powerOn_Work_Status                = SYS_IDLE;
    btChannleChangeFlag                = false;
    powerOn_PowerLed_ActionDone_Flag   = false;
    Request_connection_parameters_cont = 0;
#if (CFG_EMI_MODE)
    g_is_emi_mode                      = false;
#endif
//    memset((uint8_t *)bt_Addr, 0, sizeof(bt_Addr));
//    memset((uint8_t *)&key_env, 0, sizeof(key_env));
    memset(&app_env, 0, sizeof(app_env));
}
