#ifndef _UART_ITF_H_
#define _UART_ITF_H_

#include <stdint.h>
#include "reg_uart.h"

#if (!ROLE_CHIPSET)
#include "usbd_cdc.h"
#endif

#ifndef CFG_DMAC
#define CFG_DMAC             (1)
#endif

//TESST
#define TESST_SEND_PAD       PIN_BT_CS

/// Channel of DMA
#define DMA_UART1_RX_CHAN    DMA_CH0 // Ping-Pong mode
#define DMA_UART1_TX_CHAN    DMA_CH1 // Basic mode
#define DMA_UART2_RX_CHAN    2
#define DMA_UART2_TX_CHAN    3

/// Params of UART
#define HOST_UART_BAUD       (921600)
#define BOOT_UART_BAUD       (115200)
#define RXD_BUFF_SIZE        (0x300)   //DMA Half of size bigger than one pkg.Two DMA done question. 211116 --whl
#define TXD_BUFF_SIZE        (0x200)
#define RXD_BUFF_HALF        (RXD_BUFF_SIZE/2)

#define UART_FIFO_TRIG       (2) // 0:1B,1:4B,2:8B,3:14B
#define UART_FIFO_CONT       (8)
#define UART_FIFO_RTOR       (20) // Timeout 20sym = 2Byte

#define UART_NOPARITY        0x0     // Parity diable
#define UART_ODDPARITY       0x1     // Parity Odd
#define UART_EVENPARITY      0x3     // Parity Even

#define UART_BYTESIZE5       0x0     // Byte size 5 bits
#define UART_BYTESIZE6       0x1     // Byte size 6 bits
#define UART_BYTESIZE7       0x2     // Byte size 7 bits
#define UART_BYTESIZE8       0x3     // Byte size 8 bits

#define UART_STOPBITS1       0x0     // Stop 1 bits
#define UART_STOPBITS2       0x1     // Stop 2 bits
#define UART_BRWEN           (1<<7)  // Baud Rate register access bit
#define UART_RTOEN           (1<<8)  // Rx Timeout register access bit

#define WAIT_UART_IDLE(port)         do { while (!(UART##port->SR.TBEM)); while(UART##port->SR.BUSY);} while(0)
#define UART_MODIFY_BAUD(port, baud) do { \
        UART##port->LCR.BRWEN = 1; \
        UART##port->BRR = ((48000000UL + (baud >> 1)) / baud); \
        do \
        {  \
            UART##port->LCR.BRWEN = 0; \
        } while (UART##port->LCR.BRWEN); \
    } while (0)

/// Structures
struct rxd_buffer
{
    volatile uint16_t head;
    volatile uint16_t tail;
    uint8_t  data[RXD_BUFF_SIZE] __attribute__((aligned(4)));
};

struct txd_buffer
{
    volatile uint16_t head;
    volatile uint16_t tail;
    uint8_t  data[TXD_BUFF_SIZE] __attribute__((aligned(4)));
};

typedef struct uart_itf
{
    bool     (*init)(uint32_t baud);
    uint16_t (*read)(uint8_t *buf, uint16_t len);
    void     (*send)(uint8_t *buf, uint16_t len);
} uitf_t;


/// Interface of UARTx
bool uart1_init(uint32_t baud);
void uart1_send(uint8_t *data, uint16_t len);
uint16_t uart1_read(uint8_t *buff, uint16_t max);
//extern const struct uart_itf uart1_itf;

bool uart2_init(uint32_t baud);
void uart2_send(uint8_t *data, uint16_t len);
uint16_t uart2_read(uint8_t *buff, uint16_t max);
//extern const struct uart_itf uart2_itf;


#define CDC0_IN_EP                0x81

void usbdInit(void);
//#define usbd_send(data, len) usbd_cdc_ep_send(CDC0_IN_EP, len, data)
void usbd_hid_send(uint8_t *data, uint16_t length);
#define usbd_send(data, len) usbd_hid_send(data, len);
uint16_t usbd_read(uint8_t *buff, uint16_t max);



#if (CFG_DMAC && !DMAC_IRQ)
void uart1_proc(void);
void uart2_proc(void);
#else
#define uart1_proc()
#define uart2_proc()
#endif //(CFG_DMAC)

#if (UART_DBG)
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#define DEBUG(format, ...)    printf(format, ##__VA_ARGS__)

#define DEBUGHex(dat,len)     do{                                \
        for (int i=0; i<len; i++){     \
            printf("0x%02X ", *((dat)+i)); \
        }                              \
        \
    } while(0);
#else
#define DEBUG(...)
#endif
void dmaPingRestart(uint8_t chIndex, uint16_t buffSize);
void dmaPongRestart(uint8_t chIndex, uint16_t buffSize);
#endif // _UART_ITF_H_
