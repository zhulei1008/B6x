#ifndef _CFG_H_
#define _CFG_H_

/// Role as Chipset|Burner|Batch
#define ROLE_BATCH         1

/// Configure
#define CFG_UART1          1 // batch <--> host(PC-App)
#define CFG_UART2          1 // batch <--> chan(Burner)

#define CFG_BOOT           1
#define CFG_FIRM           1
#define CFG_READ           0 // Read flash or otp
#define CFG_FLASH          1
#define CFG_OTP            0 // Not support
#define CFG_TEST           0 // Done via chan

/// PT Support Features
#if (CFG_BOOT)
#define PT_BOOT_CMD        1
#define PT_BOOT_RSP        1
#endif

#if (CFG_FIRM)
#define PT_FIRM_CMD        1
#define PT_FIRM_RSP        1
#endif

#if (CFG_FLASH)
#define PT_FLASH_CMD       1
#define PT_FLASH_RSP       1
#endif

#if (CFG_OTP)
#define PT_OTP_CMD         1
#define PT_OTP_RSP         1
#endif

#if (CFG_TEST)
#define PT_TEST_CMD        1
#define PT_TEST_RSP        1
#endif

#define PT_COMMAND         (CFG_UART1)
#define PT_RESPONSE        (CFG_UART2)

/// PINs of Mapping
#if (CFG_UART1)
#define PIN_TX1            5
#define PIN_RX1            6
#endif

#if (CFG_UART2)
#define PIN_TX2            18 //PIN_RX
#define PIN_RX2            17 //PIN_TX
#define PIN_RST            31 //PIN_RST
#endif

// Chans-Switch(74HC4051D)
#define PIN_BT_S0          10
#define PIN_BT_S1          12
#define PIN_BT_S2          16
#define PIN_BT_CS          30

// LEDs
#define PIN_OK_LED         19 // blue led
#define PIN_FAIL_LED       23 // red led

// BTNs
#define PIN_START          25 // start key
#define PIN_OLED_PG        27 // oled page up/down key

// Beep
#define PIN_BEEP           7

// I2C-OLED
#define PIN_SCL_I2C        28
#define PIN_SDA_I2C        29

// SPI-Flash
#define PIN_QSCK_FLASH     0
#define PIN_QIO0_FLASH     2
#define PIN_QIO1_FLASH     3
#define PIN_QCS_FLASH      4

#endif
