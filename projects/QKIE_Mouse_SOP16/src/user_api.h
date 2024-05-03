#ifndef USER_API_H_
#define USER_API_H_

#include <stdint.h>
#include <stdbool.h>

/*
 * VARIABLE DECLARATION
 ****************************************************************************************
 */

extern uint8_t ltk_data[];

extern uint32_t bt_addr[2];

extern struct gapc_ltk gLTK;

extern uint8_t batt_lvl;
extern uint8_t lvl_back ;
extern uint8_t batt_blink_max;
extern uint8_t conn_intv;
extern uint8_t conn_late;
extern uint8_t channle_slect;
extern uint8_t sys_timeout_cnt;
extern uint8_t updata_conn_status;
extern uint8_t poweron_work_status;
extern uint8_t update_connect_par_flag;

extern uint16_t g_rst_rsn;

extern volatile uint32_t g_io_sta;
extern uint32_t work_mode;
extern uint32_t g_sync_word;

#if (CFG_TEST_CIRCLE_DATA)
extern uint8_t g_key_start, g_xy_idx;
#endif
/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
void timer_init(void);

uint16_t batt_vol(void);

uint8_t batt_level(void);
void user_procedure(void);

void mouse_data_clear(void);

void mouse_enter_powerdown(void);

void read_bt_mac_ltk_info(uint16_t rsn);

void flash_erase_write(uint32_t offset, uint32_t *data);

//void flash_read(uint32_t offset, uint32_t *buff, uint32_t wlen);

uint16_t core_sleep_rc16m(uint16_t wkup_en);

#endif // USER_API_H_
