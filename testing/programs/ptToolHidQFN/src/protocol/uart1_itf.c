#include <string.h>
#include <stdbool.h>
#include "b6x.h"
#include "drvs.h"
#include "reg_uart.h"
#include "uart.h"
#include "uart_itf.h"

#if (CFG_UART1)

#if (ROLE_CHIPSET)
volatile uint32_t UART1_BAUD __attribute__((at(0x200036C0))) = BOOT_UART_BAUD;  //0x080000C0, 0x080090C0
#else
#define UART1_BAUD  HOST_UART_BAUD
#endif

struct rxd_buffer uart1_rxd;

uint16_t uart1_size(void)
{
    return ((uart1_rxd.head + RXD_BUFF_SIZE - uart1_rxd.tail) % RXD_BUFF_SIZE);
}

uint16_t uart1_read(uint8_t *buff, uint16_t max)
{
    uint16_t head = uart1_rxd.head;
    uint16_t tail = uart1_rxd.tail;
    uint16_t tlen, len = (head + RXD_BUFF_SIZE - tail) % RXD_BUFF_SIZE;
    if (len == 0 || max == 0)
    {
        return 0; // empty
    }

    if (len > max)
        len = max;

    if ((head > tail) || (tail + len <= RXD_BUFF_SIZE))
    {
        memcpy(&buff[0], (const void *)&uart1_rxd.data[tail], len);
    }
    else
    {
        tlen = RXD_BUFF_SIZE - tail;

        memcpy(&buff[0], (const void *)&uart1_rxd.data[tail], tlen); // tail_len
        memcpy(&buff[tlen], (const void *)&uart1_rxd.data[0], len - tlen); // head_len
    }
    uart1_rxd.tail = (tail + len) % RXD_BUFF_SIZE;

    return len; // count
}

#if (CFG_DMAC)

/// *************** DMA Event Func *************** 
volatile bool rxChnlAlt  = false;
volatile bool txChnlBusy = false;

static void dmaUartRxDone(void)
{
    rxChnlAlt = dma_chnl_reload(DMA_UART1_RX_CHAN);
    if (rxChnlAlt)
    {
        // head to Pong
        uart1_rxd.head = RXD_BUFF_HALF;
    }
    else
    {
        // head to Ping
        uart1_rxd.head = 0;
    }
}

static void dmaUartRxRtor(void)
{
    #if (DMAC_IRQ)    
    uint32_t iflag = UART1->IFM.Word;
    #else
    uint32_t iflag = UART1->RIF.Word;
    #endif
    
    if (iflag & UART_IR_RTO_BIT)
    {
        // clear rto
        UART1->ICR.Word = UART_IR_RTO_BIT;
        
        // update head to middle
        if (rxChnlAlt)
        {
            uart1_rxd.head = RXD_BUFF_HALF + (RXD_BUFF_HALF - dma_chnl_remain(DMA_UART1_RX_CHAN | DMA_CH_ALT));
        }
        else
        {
            uart1_rxd.head = 0 + (RXD_BUFF_HALF - dma_chnl_remain(DMA_UART1_RX_CHAN));
        }
    }
}

bool uart1_init(uint32_t baud)
{
    // DMA SRAM
    dma_init();
    
    // init dma chnl
//    DMA_UARTx_TX_INIT(DMA_UART1_TX_CHAN, 1);
    DMA_UARTx_RX_INIT(DMA_UART1_RX_CHAN, 1);  
    
    // UART Param
    uart_init(UART1_PORT, PIN_TX1, PIN_RX1);
    
    #if (UART1_BAUD_2M)
    uart_conf(UART1_PORT, BRR_BAUD(2000000/*UART1_BAUD*/), LCR_BITS_DFLT);
    #else
    uart_conf(UART1_PORT, BRR_BAUD(UART1_BAUD), LCR_BITS_DFLT);    
    #endif
    
    uart_fctl(UART1_PORT, (FCR_FIFOEN_BIT | FCR_RXTL_1BYTE), 100, UART_IR_RTO_BIT);
    
    uart1_rxd.head = 0;
    uart1_rxd.tail = 0;

    DMA_UARTx_RX_CONF(DMA_UART1_RX_CHAN, 1, (uart1_rxd.data), RXD_BUFF_HALF, CCM_PING_PONG);
    DMA_UARTx_RX_CONF(DMA_UART1_RX_CHAN | DMA_CH_ALT, 1, (&uart1_rxd.data[RXD_BUFF_HALF]), RXD_BUFF_HALF, CCM_PING_PONG);    
    dma_chnl_ctrl(DMA_UART1_RX_CHAN, CHNL_EN);
       
    uart_mctl(UART1_PORT, 1);  // C1 SDK is uart_mctl(1, 1);
    
#if (DMAC_IRQ)
    UART1->IER.RTO = 1;
    DMACHCFG->IEFR0 = (1UL << DMA_UART1_RX_CHAN) /*| (1UL << DMA_UART1_TX_CHAN)*/;
    NVIC_EnableIRQ(DMAC_IRQn);
    NVIC_EnableIRQ(UART1_IRQn);
    __enable_irq();
#endif
}

#if (DMAC_IRQ)
void DMAC_IRQHandler(void)
{
    uint32_t iflag = DMACHCFG->IFLAG0;
    
    // disable intr
    DMACHCFG->IEFR0 &= ~iflag;
    // clear intr flag
    DMACHCFG->ICFR0 = iflag;
    
    if (iflag & (1UL << DMA_UART1_RX_CHAN))
    {
        dmaUartRxDone();
    }
    
//    if (iflag & (1UL << DMA_UART1_TX_CHAN))
//    {
//        txChnlBusy = false;
//    }
    
    // re-enable intr
    DMACHCFG->IEFR0 |= iflag;
}

void UART1_IRQHandler(void)
{
    dmaUartRxRtor();
}

void uart1_send(uint8_t *data, uint16_t len)
{
//    txChnlBusy = true;
//    DMA_UARTx_TX_CONF(DMA_UART1_TX_CHAN, 1, data, len, CCM_BASIC);
//    dma_chnl_ctrl(DMA_UART1_TX_CHAN, CHNL_EN);
//    
//    while (txChnlBusy); // wait done
    //while (!(UART1->SR.TEM)); //Wait Transmitter Empty
    
    while (len)
    {
        while (!(UART1->SR.TBEM));
        UART1->TBR = *data++;
        len--;
    } 
}
#else
void uart1_proc(void)
{
    if (dma_chnl_done(DMA_UART1_RX_CHAN))
    {
        dmaUartRxDone();
    }
    else
    {
        dmaUartRxRtor();
    }
}

void uart1_send(uint8_t *data, uint16_t len)
{
    DMA_UARTx_TX_CONF(DMA_UART1_TX_CHAN, 1, data, len, CCM_BASIC);
    dma_chnl_ctrl(DMA_UART1_TX_CHAN, CHNL_EN);
    
    while (!dma_chnl_done(DMA_UART1_TX_CHAN)); // wait done
    while (!(UART1->SR.TEM)); //Wait Transmitter Empty
}
#endif //(DMAC_IRQ)

#else //UART_IRQ

#define TX_FIFO    0
#define RX_FIFO    1

#if (TX_FIFO)
struct txd_buffer uart1_txd;
static volatile bool uart1_busy;
#endif

void uart1_send(uint8_t *data, uint16_t len)
{
#if (TX_FIFO)
    uint16_t head = uart1_txd.head;

    // copy data
    for (uint16_t i = 0; i < len; i++)
    {
        uart1_txd.data[head++] = *data++;
        head %= TXD_BUFF_SIZE;
    }
    uart1_txd.head = head;

    // start tx
    if (!uart1_busy)//(UART1->SR.TFEM)
    {
        uint8_t data = uart1_txd.data[uart1_txd.tail++];
        uart1_txd.tail %= TXD_BUFF_SIZE;

        UART1->TBR = data;//uart1_txd.data[uart1_txd.tail++];
    }
#else
    while (len)
    {
        while (!(UART1->SR.TBEM));
        UART1->TBR = *data++;
        len--;
    }
#endif
}
#include "reg_aon.h"
void uart1_init(void)
{
    iocsc_uart(0, PIN_TX1, PIN_RX1);

    uart_param_t param =
    {
        .baud     = UART1_BAUD,
        .databits = DATA_BITS_8b,
        .stopbits = STOP_BITS_1b,
        .parity   = PARITY_NONE,
    };

    uartConfig(0, &param);

#if (RX_FIFO)

    uartReceiveFIFOSet(0, UART_FIFO_TRIG);
    uartReceiveTimeOutSet(0, UART_FIFO_RTOR);
    
#endif
    // Clear interrupt
    UART1->ICR.Word = 0x13;

    // Enable interrupt
#if (TX_FIFO)
    uart1_busy = false;
    UART1->IER.Word = 0x13;
#else
#if (RX_FIFO)
    UART1->IER.Word = 0x11;
#else
    UART1->IER.Word = 0x01;
#endif
#endif

    UART1->LCR.RTOEN = 1;
    UART1->LCR.RXEN = 1;     // enable RX

    NVIC_EnableIRQ(UART1_IRQn);

    uart1_rxd.head = 0;
    uart1_rxd.tail = 0;

}
void UART1_IRQHandler(void)
{
    uint32_t state = UART1->IFM.Word; // UART1->RIF.Word;//

#if (TX_FIFO)
    //TX FIFO empty
    if (state & 0x02/*BIT_TXS*/)
    {
        UART1->IDR.TXS = 1;

        uint16_t size = (uart1_txd.head + TXD_BUFF_SIZE - uart1_txd.tail) % TXD_BUFF_SIZE;
        if (size > 0) //Still data to send
        {
            if (size > 16) size = 16;

            for (uint8_t i = 0; i < size; i++)
            {
                UART1->TBR = uart1_txd.data[uart1_txd.tail++];
                uart1_txd.tail %= TXD_BUFF_SIZE;
            }
            uart1_busy = true;
        }
        else //All data sent
        {
            uart1_busy = false;
        }

        UART1->ICR.TXS = 1;
        UART1->IER.TXS = 1;
    }
#endif

#if (RX_FIFO)
    
    //RX fifo trigger interrupt
    if (state & 0x01/*BIT_RXRD*/)
    {
//        UART1->IDR.RXRD = 1; // Disable RXRD Interrupt
        GPIO_DAT_SET(GPIO_PAD9);
        for (uint8_t i = 0; i < UART_FIFO_CONT; i++)  
        {
            uart1_rxd.data[uart1_rxd.head++] = UART1->RBR; // Get 8 Bytes need 12.5us (921600) (S1)
            uart1_rxd.head %= RXD_BUFF_SIZE;               // 8 Times need 200us (921600) RXD_BUFF_SIZE == 0x120 (S1)
                                                           // 8 Times need 18.5us (921600) RXD_BUFF_SIZE == 0x200 (S1)
            
                                                           // 8 Times need 286.2us (921600) RXD_BUFF_SIZE == 0x200 (F1)
                                                           // 8 Times need 210.8us (921600) RXD_BUFF_SIZE == 0x200 (F2)
                                                           // 8 Times need 147.3us (921600) RXD_BUFF_SIZE == 0x200 (F4)
            
                                                           // 8 Times need 47.8us/18.2us (921600) RXD_BUFF_SIZE == 0x200 (FC1)
            
//            if (uart1_rxd.head = RXD_BUFF_SIZE)            // 8 Times need 16.48us (921600) RXD_BUFF_SIZE == 0x120 (S1)
        }
        GPIO_DAT_DAT_CLR(GPIO_PAD9);
        UART1->ICR.Word = 0x01; // Clear RXRD Interrupt Flag
//        UART1->IER.RXRD = 1; // Enable RXRD Interrupt      
    }

    //RX Timeout trigger interrupt
    if (state & 0x10/*BIT_RTO*/)
    {
//        UART1->IDR.RTO = 1; // Disable RTO Interrupt
        while (UART1->SR.RFNE)
        {
            uart1_rxd.data[uart1_rxd.head++] = UART1->RBR;
            uart1_rxd.head %= RXD_BUFF_SIZE;           
        }

        UART1->ICR.Word = 0x10; // Clear RTO Interrupt Flag
//        UART1->IER.RTO = 1; // Enable RTO Interrupt
    }

#else
    UART1->IDR.RXRD = 1; // Disable RXRD Interrupt

    //for (uint8_t i = 0; i < UART_FIFO_CONT; i++)
    {
        uart1_rxd.data[uart1_rxd.head++] = UART1->RBR;
        uart1_rxd.head %= RXD_BUFF_SIZE;
    }

    UART1->ICR.RXRD = 1; // Clear RXRD Interrupt Flag
    UART1->IER.RXRD = 1; // Enable RXRD Interrupt
#endif
    // UART1->ICR.Word = 0xfffff;
}

#endif //(CFG_DMAC)

//@ used by printf(...), replace uart
#if (UART_DBG)

#include <stdio.h>
int fputc(int ch, FILE *f)
{

    while (!(UART1->SR.TBEM));
    UART1->TBR = ch;
    return ch;
}
#endif


#endif //(CFG_UART1)

#include <stdio.h>
int fputc(int ch, FILE *f) {
    // Remap printf(...) to UART
    uart_putc(UART1_PORT, ch);
    return ch;
}
