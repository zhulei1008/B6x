/**
 ****************************************************************************************
 *
 * @file cfg.h
 *
 * @brief App Configure MACRO, --preinclude
 *
 ****************************************************************************************
 */
 
//SOP16 PIN without vdd12
/*
         ——————————
    RFP |1         16|GND
    PA9 |2         15|XO16M_IN
    PA10|3         14|XO16M_OUT
    PA19|4         13|PA0
    PA17|5         12|PA1
    PA13|6         11|PA2
    VDD |7         10|PA6
    PA8 |8          9|PA7
         ——————————    
*/
//SDIO PA6
//SCLK PA1
//BTLED PA0
//CHANNLE SELECT PA
//C0 PA8
//R0 PA13
//R1 PA17
//ZB PA19
//ZA PA10
//R2 PA9
#ifndef _APP_CFG_H_
#define _APP_CFG_H_

/// IO观测
#define CFG_IO_OBSERVE      (0)

/// SOP16带VDD12的封装
#define CFG_SOP16_VDD12     (0)
#define CFG_SOP16           (1)
#define CFG_QFN32           (0)

/// 鼠标画圆测试, 同时按DPI和中键开始, 按右键停止.
#define CFG_TEST_CIRCLE_DATA (0)

#define CFG_HW_SPI          (1)
#define CFG_BTMR_DLY        (1)
#define CFG_FLASH_DXIP      (1)

#define CFG_SENSOR          (1)

/// System Clock(0=16MHz, 1=32MHz, 2=48MHz, 3=64MHz)
#define SYS_CLK             (0)

/// Debug Mode(0=Disable, 1=via UART, 2=via RTT)
#define DBG_MODE            (1)

/// Use ble6_lite.lib
#define BLE_LITELIB         (1)

/// BLE Configure (Single or Multi-Connections)
#define BLE_NB_SLAVE        (1)
#define BLE_NB_MASTER       (0)

#define BLE_ADDR            {0x00, 0x26, 0x10, 0x23, 0x20, 0xD2}
// 快连Master MAC(6 addr + 1 type)(type = ADDR[5]<0xC0 ? 0 : 1)
#define DEF_BLE_ADDR        {0x11, 0x88, 0x3F, 0xA1, 0x01, 0xF3, 0x01}

#define BLE_DEV_NAME        "QKIE-Mouse"
#define BLE_DEV_ICON        0x03C2 // 03C0-Generic HID,03C1-Keyboard,03C2-Mouse,03C4-Gamepad
#define BLE_PHY             (GAP_PHY_LE_1MBPS)
#define BLE_AUTH            (GAP_AUTH_REQ_NO_MITM_BOND)
#define CFG_LTK_STORE       (1)

#define SYNC_WORD_2G4       (0x26D5EF45)//(0x26D5EF45)
#define SYNC_WORD_BLE       (0x8E89BED6)
// work mode define
#define BT_MODE             (0)
#define B24G_MODE           (3)
#define TIMER_CORR          (0)  

#define BT_MAC_STORE_OFFSET (0x1200)
#define LTK_STORE_OFFSET    (0x1100)
#define CPI_DATA_OFFSET     (0x1300)
#define DONGLE_MAC_OFFSET   (0x1400)
#define POWEROFF_STORE_DONGLE_MAC_OFFSET (0x1000)
// 无单独模式选择键时使用
#define WORK_MODE_OFFSET    (0x1500)

/// Profile Configure
#define PRF_DISS            (1)
#define PRF_BASS            (1)
#define PRF_HIDS            (1)
#define HID_NB_PKT_MAX      (20)

#if !defined(PA)
#define PA(n) ((0x01UL) << (n))
#endif

#if (CFG_SOP16_VDD12)
#define CHANNLE_SELECT_EN   (1)
/*LED defined*/
#if (DBG_MODE) || (CFG_IO_OBSERVE)
#define _BT_LED             (5)
#define _24G_LED            (4)
#else
#define _BT_LED             (0)
#define _24G_LED            (7)
#endif
#define _BATT_LOW_LED       (3) // Todo...

#define CHANNEL_SLECT_PIN   (0) // Todo...

// Wheel Scroll             
#define PAD_WHEEL_ZA        (10)
#define PAD_WHEEL_ZB        (19)

// Buttons
#define PAD_KSCAN_C0        (8)
#define PAD_KSCAN_R0        (13)
#define PAD_KSCAN_R1        (17)
#define PAD_KSCAN_R2        (9)

// Sensor
#define PAD_SENSOR_CLK      (2)
#define PAD_SENSOR_DIO      (6)
#elif(CFG_SOP16)
#define CHANNLE_SELECT_EN   (1)
/*LED defined*/
#if (DBG_MODE)
#define _BT_LED             (1) // PA06
#else
#define _BT_LED             (6) // PA06
#endif
#define _24G_LED            (1) // PA03
#define _BATT_LOW_LED       (1) // Todo...

#define CHANNEL_SLECT_PIN   (0)
// Wheel Scroll             
#define PAD_WHEEL_ZA        (10)
#if(DBG_MODE)
#define PAD_WHEEL_ZB        (19)// (19)
#else
#define PAD_WHEEL_ZB        (7)// (19)
#endif
// Buttons
#define PAD_KSCAN_C0        (8)
#define PAD_KSCAN_R0        (13)
#define PAD_KSCAN_R1        (17)
#define PAD_KSCAN_R2        (9)

// Sensor
#define PAD_SENSOR_CLK      (2)
#define PAD_SENSOR_DIO      (6)
#elif(CFG_QFN32)
#define CHANNLE_SELECT_EN   (0)
/*LED defined*/
#define _BT_LED             (PA14) // PA15
#define _24G_LED            (PA15) // PA14
#define _BATT_LOW_LED       (PA13) // PA13
// work mode define

#define CHANNEL_SLECT_PIN   (9)
#define TIMER_CORR          (0)
// Wheel Scroll             
#define PAD_WHEEL_ZA        (10)
#define PAD_WHEEL_ZB        (11)


// Buttons
#define PAD_KSCAN_C0        (5)
#define PAD_KSCAN_R0        (17)
#define PAD_KSCAN_R1        (16)
#define PAD_KSCAN_R2        (18)
                            
// Sensor
#define PAD_SENSOR_CLK      (3)
#define PAD_SENSOR_DIO      (2)

#endif

#define PAD_WHEEL_MSK       (PA(PAD_WHEEL_ZA) | PA(PAD_WHEEL_ZA))
#define ROW_CNT             (3)
#define ROW_PAD_MSK         ((1UL << ROW_CNT) - 1)
#define PAD_KSCAN_MSK       (PA(PAD_KSCAN_R0) | PA(PAD_KSCAN_R1) | PA(PAD_KSCAN_R2))
#define PAD_POWEROFF_WEKUP_MSK       (PA(PAD_KSCAN_R0) | PA(PAD_KSCAN_R2))
/// Debug Configure
#if (DBG_MODE)
    #define DBG_APP         (0)
    #define DBG_PROC        (0)
    #define DBG_ACTV        (0)
    #define DBG_APP_MSG     (1)
    #define DBG_GAPM        (1)
    #define DBG_MSG         (0)
    #define DBG_GAPC        (1)
    #define DBG_MOUSE       (1)
    #define DBG_KEYS        (0)
    #define DBG_HIDS        (0)
    #define DBG_BATT        (0)
    #define DBG_EMI_TEST    (0)
#endif

/// Misc Options
#define LED_PLAY                      (1)
#define CFG_SLEEP                     (1)
#define CFG_POWEROFF                  (BLE_LITELIB)
#define RC32K_CALIB_PERIOD            (15000)

// user define
#define _24G_PARING_ENABLE            (0)
#define SYS_IDLE                      (0)
#define SYS_PARING                    (1)
#define SYS_CONNECT_BACK              (2)
#define SYS_CONNECT                   (3)
// time out unit in 15 s
#define SYS_IDLE_TIMEOUT              (1)  // 30 s
#define SYS_PARING_TIMEOUT            (1) // 30s
#define SYS_CONNECT_BACK_TIMEOUT      (1)  // 30 s
#define SYS_CONNECT_NO_ACTION_TIMEOUT (40) // 10 min
#define ONLY_24G_MODE                  (0)
// byte len to word len, byte len must 4 times
#define BLEN2WLEN(blen) ((blen) >> 2)

#define CFG_CONN_LATENCY (5)
#define CFG_EMI_MODE       (1)
#endif //_APP_CFG_H_
