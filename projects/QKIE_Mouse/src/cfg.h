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

#define CFG_HW_SPI          (1)
#define CFG_BTMR_DLY        (1)
#define CFG_BLE_SFT_WKUP    (0)
#define CFG_FLASH_DXIP      (1)

#define CFG_SENSOR          (1)
#define CFG_BT_ADDR_ARR     (1)

/// System Clock(0=16MHz, 1=32MHz, 2=48MHz, 3=64MHz)
#define SYS_CLK             (0)

/// Debug Mode(0=Disable, 1=via UART, 2=via RTT)
#define DBG_MODE            (0)

/// Use ble6_lite.lib
#define BLE_LITELIB         (1)

/// BLE Configure (Single or Multi-Connections)
#define BLE_NB_SLAVE        (1)
#define BLE_NB_MASTER       (0)

#define BLE_ADDR            {0x00, 0x26, 0x10, 0x23, 0x20, 0xD2}
#define BLE_DEV_NAME        "QKIE-Mouse"
#define BLE_DEV_ICON        0x03C2 // 03C0-Generic HID,03C1-Keyboard,03C2-Mouse,03C4-Gamepad
#define BLE_PHY             (GAP_PHY_LE_1MBPS)
#define BLE_AUTH            (GAP_AUTH_REQ_NO_MITM_BOND)
#define CFG_LTK_STORE       (1)

#define BT_MAC_STORE_OFFSET (0x1200)
#define LTK_STORE_OFFSET    (0x1100)
#define CPI_DATA_OFFSET     (0x1300)
#define DONGLE_MAC_OFFSET   (0x1400)

/// Profile Configure
#define PRF_DISS            (1)
#define PRF_BASS            (1)
#define PRF_HIDS            (1)

#if !defined(PA)
#define PA(n) ((0x01UL) << (n))
#endif
/*LED defined*/
#define _BT_LED             (PA15) // PA15
#define _24G_LED            (PA14) // PA14
#define _BATT_LOW_LED       (PA13) // PA13
// work mode define
#define BT_MODE             (0)
#define B24G_MODE           (3)

#define CHANNEL_SLECT_PIN   (9)
#define TIMER_CORR          (0)
// Wheel Scroll             
#define PAD_WHEEL_ZA        (10)
#define PAD_WHEEL_ZB        (11)
#define PAD_WHEEL_MSK       (PA(PAD_WHEEL_ZA) | PA(PAD_WHEEL_ZA))

// Buttons
#define PAD_KSCAN_C0        (5)
#define PAD_KSCAN_R0        (17)
#define PAD_KSCAN_R1        (16)
#define PAD_KSCAN_R2        (18)
                            
#define ROW_CNT             (3)
#define ROW_PAD_MSK         ((1UL << ROW_CNT) - 1)
#define PAD_KSCAN_MSK       (PA(PAD_KSCAN_R0) | PA(PAD_KSCAN_R1) | PA(PAD_KSCAN_R2))

// Sensor
#define PAD_SENSOR_CLK      (3)
#define PAD_SENSOR_DIO      (2)

/// Debug Configure
#if (DBG_MODE)
    #define DBG_APP         (0)
    #define DBG_PROC        (1)
    #define DBG_ACTV        (1)
    #define DBG_GAPM        (0)
    #define DBG_MSG         (0)
    #define DBG_GAPC        (0)
    #define DBG_MOUSE       (1)
    #define DBG_KEYS        (0)
    #define DBG_HIDS        (0)
#endif

/// Misc Options
#define LED_PLAY                      (1)
#define CFG_SLEEP                     (1)
#define CFG_POWEROFF                  (BLE_LITELIB)
#define RC32K_CALIB_PERIOD            (15000)
// user define
#define SYS_IDLE                      (0)
#define SYS_PARING                    (1)
#define SYS_CONNECT_BACK              (2)
#define SYS_CONNECT                   (3)
// time out unit in 15 s
#define SYS_IDLE_TIMEOUT              (1)  // 30 s
#define SYS_PARING_TIMEOUT            (12) // 180s
#define SYS_CONNECT_BACK_TIMEOUT      (2)  // 30 s
#define SYS_CONNECT_NO_ACTION_TIMEOUT (40) // 10 min

#endif //_APP_CFG_H_
