/**
 ****************************************************************************************
 *
 * @file mouse.h
 *
 * @brief Header file - Mouse Scanning and Report
 *
 ****************************************************************************************
 */

#ifndef _MOUSE_H_
#define _MOUSE_H_

#include <stdint.h>
#include <stdbool.h>


/// Reports Description @see hid_report_map
enum report_desc
{
    // report id
    REP_NONE         = 0,
    REP_KB           = 1,
    REP_MOUSE        = 2,
    
    // report index and length
    REP_IDX_KB       = 0, // IDX_KB_INPUT
    REP_LEN_KB       = 8, // 8-bytes
    
    REP_IDX_MOUSE    = 2, // IDX_MOUSE_INPUT
    REP_LEN_MOUSE    = 4, // 4-bytes
};

enum mouse_report_offset
{
    MOUSE_BUTTON_OFFSET = 0,
    MOUSE_X_OFFSET,
    MOUSE_Y_OFFSET,
    MOUSE_WHEEL_OFFSET,
};

enum mouse_sta
{
    MOUSE_STA_NONE   = 0x00,
    MOUSE_STA_BUTTON = 0x01,
    MOUSE_STA_XY     = 0x02,
    MOUSE_STA_WHEEL  = 0x04,
};
enum mouse_type
{
    MOUSE_NONE,
    MOUSE_LOCAL,
    
    MOUSE_BUTTON,
    MOUSE_XY,
    MOUSE_WHEEL,
};

enum bt_addr_idx
{
    BT_ADDR_0,
    BT_ADDR_1,
};

///Mouse report (data packet)
struct mouse_report
{
    uint8_t sta;
    
    // current report
    uint8_t mouseTyp;
    uint8_t report[REP_LEN_MOUSE];
    
    // backup report
    uint8_t mouseTyp0;
    uint8_t report0[REP_LEN_MOUSE];
};

void mouse_init(void);
void mouse_io_init(void);
//void mouse_scan(void);
void mouse_sensor_init(void);
bool mouse_proc(void);
void mose_scan_init(void);
void mouse_io_hiz(void);
void mouse_data_handle(void);
void mouse_enter_powerdown(void);


void BTaddr_Add_One(void);
#endif  //_MOUSE_H_

