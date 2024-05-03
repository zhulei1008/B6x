;********************************************************************************
;* File Name          : startup_iar.s
;* Author             : wq.
;* Version            : V1.0.0
;* Date               : 2021.7.15
;* Description        : Dragon Devices vector table for EWARM toolchain.
;*                      This module performs:
;*                      - Set the initial SP
;*                      - Set the initial PC == __iar_program_start,
;*                      - Set the vector table entries with the exceptions ISR
;*                        address.
;*                      After Reset the Cortex-M0 processor is in Thread mode,
;*                      priority is Privileged, and the Stack is set to Main.
;********************************************************************************
;* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
;* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
;* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
;* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
;* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
;* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
;* FOR MORE INFORMATION PLEASE READ CAREFULLY THE LICENSE AGREEMENT FILE
;* LOCATED IN THE ROOT DIRECTORY OF THIS FIRMWARE PACKAGE.
;*******************************************************************************/
;
;
; The modules in this file are included in the libraries, and may be replaced
; by any user-defined modules that define the PUBLIC symbol _program_start or
; a user defined start symbol.
; To override the cstartup defined in the library, simply add your modified
; version to the workbench project.
;
; The vector table is normally located at address 0.
; When debugging in RAM, it can be located in RAM, aligned to at least 2^6.
; The name "__vector_table" has special meaning for C-SPY:
; it is where the SP start value is found, and the NVIC vector
; table register (VTOR) is initialized to this address if != 0.

        MODULE  ?cstartup

        ;; Forward declaration of sections.
        SECTION CSTACK:DATA:NOROOT(3)

        SECTION .intvec:CODE:NOROOT(2)

        EXTERN  __iar_program_start
        PUBLIC  __vector_table
        PUBLIC  __Vectors
        PUBLIC  __Vectors_End
        PUBLIC  __Vectors_Size

        DATA

__vector_table
        DCD     sfe(CSTACK)                    ; 0,  load top of stack
        DCD     Reset_Handler                  ; 1,  Reset Handler
        DCD     NMI_Handler                    ; 2,  NMI Handler
        DCD     HardFault_Handler              ; 3,  Hard Fault Handler
        DCD     0                              ; 4,  Reserved
        DCD     0                              ; 5,  Reserved
        DCD     0                              ; 6,  Reserved
        DCD     0                              ; 7,  Reserved
        DCD     0                              ; 8,  Reserved
        DCD     0                              ; 9,  Reserved
        DCD     0                              ; 10, Reserved
        DCD     SVCall_Handler                 ; 11, SVCall Handler
        DCD     0                              ; 12, Reserved
        DCD     0                              ; 13, Reserved
        DCD     PendSV_Handler                 ; 14, PendSV Handler
        DCD     SysTick_Handler                ; 15, SysTick Handler

        ; External Interrupts
        DCD    EXTI_IRQHandler                 ; 0,  EXTI
        DCD    IWDT_IRQHandler                 ; 1,  IWDT
        DCD    BLE_IRQHandler                  ; 2,  BB
        DCD    DMAC_IRQHandler                 ; 3,  DMAChannel
        DCD    BB_LP_IRQHandler                ; 4,  BB_LowPower
        DCD    BTMR_IRQHandler                 ; 5,  DMAC 
        DCD    CTMR1_IRQHandler                ; 6,  CTMR1
        DCD    ADMR1_IRQHandler                ; 7,  ATMR1
        DCD    RTC_IRQHandler                  ; 8,  RTC
        DCD    I2C1_IRQHandler                 ; 9,  I2C1
        DCD    SPIM_IRQHandler                 ; 10, SPI Master
        DCD    SPIS_IRQHandler                 ; 11, SPI Slave
        DCD    UART1_IRQHandler                ; 12, UART1
        DCD    UART2_IRQHandler                ; 13, UART2
        DCD    AON_PMU_IRQHandler              ; 14, PMU
        DCD    LVD33_IRQHandler                ; 15, LVD
        DCD    BOD12_IRQHandler                ; 16, BOD
        DCD    USB_IRQHandler                  ; 17, USB
        DCD    USB_SOF_IRQHandler              ; 18, USB_SOF
        DCD    FSHC_IRQHandler                 ; 19, FSHC
        DCD    0                               ; 20, Reserved
        DCD    0                               ; 21, Reserved
        DCD    0                               ; 22, Reserved
        DCD    0                               ; 23, Reserved
        DCD    0                               ; 24, Reserved
        DCD    0                               ; 25, Reserved
        DCD    0                               ; 26, Reserved
        DCD    0                               ; 27, Reserved
        DCD    0                               ; 28, Reserved
        DCD    0                               ; 29, Reserved
        DCD    0                               ; 30, Reserved
        DCD    0                               ; 31, Reserved

__Vectors_End
__Vectors      EQU __vector_table
__Vectors_Size EQU __Vectors_End - __Vectors

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Reset_Handler
;;
        THUMB
        
        PUBWEAK Reset_Handler
        SECTION .text:CODE:NOROOT:REORDER(2)
Reset_Handler
        LDR     R0, =__iar_program_start
        BX      R0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Dummy Exception Handlers 
;; (infinite loops here, can be modified)
;;

        PUBWEAK NMI_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
NMI_Handler
        B .


        PUBWEAK HardFault_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
HardFault_Handler
        B .


        PUBWEAK SVCall_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
SVCall_Handler
        B .


        PUBWEAK PendSV_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
PendSV_Handler
        B .


        PUBWEAK SysTick_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
SysTick_Handler
        B .

        PUBWEAK EXTI_IRQHandler
        PUBWEAK IWDT_IRQHandler
        PUBWEAK BLE_IRQHandler
        PUBWEAK DMAC_IRQHandler
        PUBWEAK BB_LP_IRQHandler
        PUBWEAK BTMR_IRQHandler
        PUBWEAK CTMR1_IRQHandler
        PUBWEAK ADMR1_IRQHandler
        PUBWEAK RTC_IRQHandler
        PUBWEAK I2C1_IRQHandler
        PUBWEAK SPIM_IRQHandler
        PUBWEAK SPIS_IRQHandler
        PUBWEAK UART1_IRQHandler
        PUBWEAK UART2_IRQHandler
        PUBWEAK AON_PMU_IRQHandler
        PUBWEAK LVD33_IRQHandler
        PUBWEAK BOD12_IRQHandler
        PUBWEAK USB_IRQHandler
        PUBWEAK USB_SOF_IRQHandler
        PUBWEAK FSHC_IRQHandler

        SECTION .text:CODE:NOROOT:REORDER(1)

EXTI_IRQHandler
IWDT_IRQHandler
BLE_IRQHandler
DMAC_IRQHandler
BB_LP_IRQHandler
BTMR_IRQHandler
CTMR1_IRQHandler
ADMR1_IRQHandler
RTC_IRQHandler
I2C1_IRQHandler
SPIM_IRQHandler
SPIS_IRQHandler
UART1_IRQHandler
UART2_IRQHandler
AON_PMU_IRQHandler
LVD33_IRQHandler
BOD12_IRQHandler
USB_IRQHandler
USB_SOF_IRQHandler
FSHC_IRQHandler

        B .

        END
