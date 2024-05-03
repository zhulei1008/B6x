#ifndef _CFG_H_
#define _CFG_H_

/// Role as Chipset|Burner|Batch
#define ROLE_BURNER        1

#define SYS_CLK            2  // USB 48Mhz

/// Configure
#define CFG_UART1          1 // burner <--> host(Batch or PC)
#define CFG_UART2          1 // burner <--> chip

#define CFG_BOOT           1
#define CFG_FIRM           1
#define CFG_READ           1 // Read flash or otp
#define CFG_FLASH          1
#define CFG_OTP            0 // Not support
#define CFG_TEST           0 // Test Cmd

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
#define USB_EN             1
#define PIN_TX1            6
#define PIN_RX1            7
#endif

#if (CFG_UART2)
#define PIN_TX2        10
#define PIN_RX2        11
#define PIN_RST        14
#endif

/// LEDs
#define PIN_FAIL_LED       16
#define PIN_BUSY_LED       17
#define PIN_OK_LED         18

/// BTNs
#define PIN_START          8

/// Chip-ITF
#define PIN_PWM_CHIP       9
#define PIN_ADC_CHIP       12
#define PIN_VSS_CHIP       13
#define PIN_VCC_CHIP       15

/// SPI-Flash
#define PIN_QCS_FLASH      2
#define PIN_QIO1_FLASH     3
#define PIN_QSCK_FLASH     4
#define PIN_QIO0_FLASH     5

#define LED_PLAY           1
#endif
