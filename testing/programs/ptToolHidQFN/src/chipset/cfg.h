#ifndef _CFG_H_
#define _CFG_H_

/// Role as Chipset|Burner|Batch
#define ROLE_CHIPSET       1
#define CHIP_MODE          1 // 0-BootMode, 1-FullMode

/// Configure
#define CFG_UART1          1 // chip <--> burner
#define CFG_UART2          0 // none
#define CFG_DMAC           (CHIP_MODE)

#define UART1_BAUD_2M     (0)

#if (CFG_DMAC)
#define DMAC_IRQ           1 // use irq
#endif

#define CFG_BOOT           0 // Boot  - CWR/RST/BAUD/JUMP
#define CFG_FIRM           0 // Firm  - RD/ER/WR/VF
#define CFG_READ           1 // Read  - Flash or OTP
#define CFG_FLASH          1 // Flash - RD/ER/WR/VF
#define CFG_OTP            0 // OTP   - RD/WR/VF
#define CFG_TEST           (CHIP_MODE) // Test  - GPIO/XTAL/RF/PWR

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
#define PT_TEST_RSP        0
#endif

#define PT_COMMAND         (CFG_UART1)
#define PT_RESPONSE        (CFG_UART2)

/// PINs of Mapping
#if (CFG_UART1)
#define PIN_TX1        6
#define PIN_RX1        7
#endif

#if (CFG_UART2)
#define PIN_TX2        17
#define PIN_RX2        18
#define PIN_RST        19
#endif
#include "rf_cfg.h"
#define EM_ADDR_0x20005000 (1)
#define EM_ADDR_USE_MACRO  (1)

#define EM_BASE_ADDR       0x20008000
// Whitening disabled
#define WHIT_DISABLE       (1)
#endif
