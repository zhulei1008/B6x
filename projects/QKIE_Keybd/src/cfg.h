/**
 ****************************************************************************************
 *
 * @file cfg.h
 *
 * @brief App Configure MACRO, --preinclude
 *
 ****************************************************************************************
 */

#ifndef _APP_CFG_H_
#define _APP_CFG_H_

#define CFG_PIN_ENABLE         (0)

#define BLE_LITELIB            (1)
/// System Clock(0=16MHz, 1=32MHz, 2=48MHz, 3=64MHz)
#define SYS_CLK                (0)

/// BLE Configure (Single or Multi-Connections)
#define BLE_NB_SLAVE           (1)
#define BLE_NB_MASTER          (0)

#define BLE_ADDR_1             {0x04, 0x11, 0x23, 0xA1, 0x08, 0xD1}
#define BLE_ADDR_2             {0x04, 0x11, 0x23, 0xA2, 0x05, 0xD2}
#define BLE_ADDR_3             {0x04, 0x11, 0x23, 0xA3, 0x05, 0xD3}
#define BLE_DEV_NAME           "QKIE-KB"
#define CN_NAME    (0)
#if(CN_NAME)
#define BLE_DEV_NAME_CHN_1     {0xE8 ,0x8A ,0xAF ,0xE4 ,0xB9 ,0x8B ,0xE6 ,0xBA ,0x90 ,0xe6,0x96,0xb9,0xe6,0xa1,0x88}//芯之源方案
#define BLE_DEV_NAME_CHN_2     {0xE8 ,0x8A ,0xAF ,0xE4 ,0xB9 ,0x8B ,0xE6 ,0xBA ,0x90 ,0xe7,0xa7,0x91,0xe6,0x8a,0x80}//芯之源科技
#define BLE_DEV_NAME_CHN_3     {0xE8 ,0x8A ,0xAF ,0xE4 ,0xB9 ,0x8B ,0xE6 ,0xBA ,0x90 ,0xe7,0xa7,0x91,0xe6,0x8a,0x80}//芯之源科技
#define USER_CN_NAME_LEN 15
#else
#define BLE_DEV_NAME_CHN_1      "QKIE-KB1"
#define BLE_DEV_NAME_CHN_2      "QKIE-KB2"
#define BLE_DEV_NAME_CHN_3      "QKIE-KB3"
#endif
#define BLE_NAME_MAX_LEN       (18)
#define BLE_DEV_ICON           0x03C1  // 03C0-Generic HID,03C1-Keyboard,03C2-Mouse,03C4-Gamepad
#define BLE_PHY                (GAP_PHY_LE_1MBPS)

#if (CFG_PIN_ENABLE)
#define BLE_AUTH               (GAP_AUTH_REQ_MITM_BOND)
#define BLE_IOCAP              (GAP_IO_CAP_KB_ONLY)
#else
#define BLE_AUTH               (GAP_AUTH_REQ_NO_MITM_BOND)
#define BLE_IOCAP              (GAP_IO_CAP_NO_INPUT_NO_OUTPUT)
#endif //CFG_PIN_ENABLE

#define CFG_LTK_STORE          (1)

/// Profile Configure
#define PRF_DISS               (1)
#define PRF_BASS               (0)
#define PRF_HIDS               (1)
#define PRF_OTAS               (1)
#define GATT_CLI			   (1)
#if (PRF_OTAS)
    #define ROLE_CHIPSET   (1)
    #define OTA_CHIP       (1)
    #define PT_COMMAND     (1)
    #define PT_FLASH_CMD   (1)
#endif
/// Debug Mode(0=Disable, 1=via UART, 2=via RTT)
#define DBG_MODE               (1)

/// Debug Configure
#if (DBG_MODE)
	#define DBG_MYAPP	       (0)
	#define DBG_BATT	       (1)
	#define DBG_PROC           (0)
	#define DBG_KEYS           (0)
	#define KEY_BOARD          (0)	
	#define DBG_SFT            (0)
	#define DBG_LED            (0)
	#define DBG_APP            (0)
	#define DBG_ACTV           (1)
	#define DBG_GAPC           (1)
	#define DBG_GAPM           (0)
	#define DBG_MSG		       (0)
	#define DBG_GATT           (1)
	#define DBG_DISS	       (0)
	#define DBG_BASS           (0)
	#define DBG_HIDS           (0)
	#define DBG_EMI_TEST       (0)
    #define DBG_CHIPSET        (1)
    #define DBG_OTAS           (0)
#endif

//flash space function definition
#define BT_MAC_USER_OFFSET     (0X1000)
#define LTK_STORE_OFFSET       (0x1100)
#define BT_MAC_STORE_OFFSET    (0x1200)
#define BT_CNANNLE_OFFSET      (0X1300)
#define OS_MODE_FLASH_ADDR     (0X1400)

/// Misc Options
#define LED_PLAY               (1)
#define BT_LED                 (3)
#define POWER_LED              (4)
#define CAP_LED                (1)
#define ALL_LED				   BIT(BT_LED)|BIT(POWER_LED)|BIT(CAP_LED)

#define EMI_COL ((0x1<<15)|(0x1<<17)|(0x1<<16))
#define EMI_ROW ((0x1<<10)|(0x1<<12)|(0x1<<19)|(0x1<<13)|(0x1<<18)|(0x1<<14)|(0x1<<11)|(0x1<<9))

// user define
#define CFG_SLEEP              (1)
#define CFG_POWEROFF           (0)
#define RC32K_CALIB_PERIOD     (15000)

#define PAIR_KEY_FUNC          (1)
#define PAIR_KEY_GPIO          (2)

#define POP_UP_WINDOW   	   (0)
#if (POP_UP_WINDOW)
#define BT_NAME_MAX_LEN        (11)
#else
#define BT_NAME_MAX_LEN        (18)
#endif

#define SYS_IDLE                      (0)
#define SYS_PARING                    (1)
#define SYS_CONNECT_BACK              (2)
#define SYS_CONNECT                   (3)
// time out unit in 15 s
#define SYS_IDLE_TIMEOUT              (2) //30 s
#define SYS_PARING_TIMEOUT            (12)//180s
#define SYS_CONNECT_BACK_TIMEOUT      (2) //30 s
#define SYS_CONNECT_NO_ACTION_TIMEOUT (39)//10 min

#define SFT_TIME_SCAN			      (0)
#define TRIM_LOAD                     (1)

// batt.c
#define ONCHIP_DIODE_VOLTAGEDROP (0) 	//unit mv
#define LOWBATT_THRESHOLD        (3000) //3.0V低电压报警
#define CFG_EMI_MODE       (1)
#endif  //_APP_CFG_H_
