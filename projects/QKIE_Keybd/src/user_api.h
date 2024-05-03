#ifndef USER_API_H_
#define USER_API_H_

#include <stdint.h>
#include <stdbool.h>
#include "cfg.h"
/*
 * VARIABLE DECLARATION
 ****************************************************************************************
 */
extern uint8_t batt_lvl;
extern uint8_t conn_intv;
extern uint8_t conn_late;

extern uint8_t led_blink_cnt;
extern uint8_t lowLedBlinkCnt;
extern uint8_t connect_complete;
extern uint8_t os_mode_table[4];
extern uint8_t sys_poweroff_flag;
extern uint8_t updata_conn_status;
extern uint8_t powerOn_Work_Status;
extern uint8_t read_Master_Addr[6];
extern uint8_t masterDongle_Addr[6];
extern uint8_t update_connect_par_flag;
extern uint8_t Request_connection_parameters_cont;
extern uint8_t bt_Addr[20];
extern uint32_t channle_Select;

extern bool btChannleChangeFlag;

extern uint16_t sys_Timeout_Cnt;

extern struct key_env_tag key_env;

extern bool powerOn_PowerLed_ActionDone_Flag;

extern const uint8_t def_MasterDongle_Addr[];

extern const struct gapc_conn_param key_le_conn_pref1;

#if (CFG_PIN_ENABLE)
extern volatile uint8_t g_pin_num;
extern volatile uint32_t g_pin_code;
#endif

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
void BtReset(void);

void kb_scan(void);

void zi_data_clr(void);

void PairKey_Init(void);

void LowBatt_ALarm(void);

void wakeup_report(void);

void BT_Channle_Set(void);

void BtAddr_Add_One(void);

void PairKey_Handle(void);

void user_procedure(void);

void Delete_Pair_Info(void);

void keyboard_poweroff(void);

void Read_Bt_Mac_Ltk_Info(void);

void keyboard_go_poweroff_jage(void);

void flash_erase_write(uint32_t offset, uint32_t *data);

uint16_t batt_vol(void);

uint8_t batt_level(void);

#if(CFG_EMI_MODE)
extern uint8_t QWE_Data;
extern volatile bool g_is_emi_mode;
extern uint8_t emi_key_t_press_cnt ;
void EMI_Mode(void);
void emi_event(uint8_t evt_id);
#endif

#endif // USER_API_H_
