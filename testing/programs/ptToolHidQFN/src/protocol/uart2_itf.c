#include <string.h>
#include <stdbool.h>
#include "b6x.h"
#include "drvs.h"
#include "regs.h"
#include "uart_itf.h"

#if (CFG_UART2)

#define DMAC_IRQ           1 // use irq

#if (ROLE_BATCH)
#define UART2_BAUD  HOST_UART_BAUD
#else
#define UART2_BAUD  BOOT_UART_BAUD
#endif

struct rxd_buffer uart2_rxd;
//uint8_t  uart2_rxd_data[RXD_BUFF_SIZE] __attribute__((aligned(4)));
//volatile uint16_t uart2_rxd_head;
//volatile uint16_t uart2_rxd_tail;

uint16_t uart2_size(void)
{
    return ((uart2_rxd.head + RXD_BUFF_SIZE - uart2_rxd.tail) % RXD_BUFF_SIZE);
}

uint16_t uart2_read(uint8_t *buff, uint16_t max)
{
    uint16_t head = uart2_rxd.head;
    uint16_t tail = uart2_rxd.tail;
    uint16_t tlen, len = (head + RXD_BUFF_SIZE - tail) % RXD_BUFF_SIZE;
    if (len == 0 || max == 0)
    {
        return 0; // empty
    }

    if (len > max)
        len = max;

    if ((head > tail) || (tail + len <= RXD_BUFF_SIZE))
    {
        memcpy(&buff[0], (const void *)&uart2_rxd.data[tail], len);
    }
    else
    {
        tlen = RXD_BUFF_SIZE - tail;

        memcpy(&buff[0], (const void *)&uart2_rxd.data[tail], tlen); // tail_len
        memcpy(&buff[tlen], (const void *)&uart2_rxd.data[0], len - tlen); // head_len
    }
    uart2_rxd.tail = (tail + len) % RXD_BUFF_SIZE;

    return len; // count
}

#if (CFG_DMAC)

/// *************** DMA Event Func *************** 
volatile bool rxChnlAlt2  = false;
volatile bool txChnlBusy2 = false;

static void dmaUartRxDone(void)
{
    rxChnlAlt2 = dma_chnl_reload(DMA_UART2_RX_CHAN);
    if (rxChnlAlt2)
    {
        // head to Pong
        uart2_rxd.head = RXD_BUFF_HALF;
    }
    else
    {
        // head to Ping
        uart2_rxd.head = 0;
    }
}

static void dmaUartRxRtor(void)
{
    #if (DMAC_IRQ)    
    uint32_t iflag = UART2->IFM.Word;
    #else
    uint32_t iflag = UART2->RIF.Word;
    #endif
    
    if (iflag & UART_IR_RTO_BIT)
    {
        // clear rto
        UART2->ICR.Word = UART_IR_RTO_BIT;
        
        // update head to middle
        if (rxChnlAlt2)
        {
            uart2_rxd.head = RXD_BUFF_HALF + (RXD_BUFF_HALF - dma_chnl_remain(DMA_UART2_RX_CHAN | DMA_CH_ALT));
        }
        else
        {
            uart2_rxd.head = 0 + (RXD_BUFF_HALF - dma_chnl_remain(DMA_UART2_RX_CHAN));
        }
    }
}

#define BAUD_SYNC 0xB9
#define BAUD_RSP  0x9B

#include "reg_gpio.h"

bool uart2_auto_baud(void)
{
    uint16_t sync_time = 0x1FFF * (SYS_CLK + 1); //48Mhz
    uint8_t sync_cnt = 100;  // 0.5ms * 200
    bool    auto_mod = false;
    uint8_t invalid_sync = 0;

//    xdelay(1000); //0.5ms
      
    while(sync_cnt && sync_time)  // 100ms time out
    {   
        sync_time--;
        
        if (UART2->SR.DR) //UART_FLAG_DR
        {
            sync_time = 0x1FFF;
            sync_cnt--;
            
            uint8_t data = uart_getc(UART2_PORT);
            
            if (data == BAUD_SYNC) // if baud_sync error, auto calib baud enable
            {
                if (auto_mod)
                {
                    // confirm auto calib baud is vaild
                    UART2->LCR.BRWEN  = 1;
                    UART2->LCR.BRWEN  = 0;

                    UART2->MCR.ABREN = 0;
                }

                // first response 
                uart_putc(1, BAUD_RSP);
                
//                sync_cnt = 0; // break loop
                return true;
            }
            else
            {   
                // first response 
                uart_putc(1, data);
                
                if (invalid_sync > 4)
                {
                    auto_mod = true;
                    UART2->MCR.ABRMOD = 0;   // auto baud mode0  
                    UART2->MCR.ABREN = 1;
                    UART2->MCR.ABRRS = 1;                
                }
                
                invalid_sync++;                
            }
        } 
    }

    return false;
//    RCC->APBRST_CTRL.UART2_RSTREQ = 1;
//    RCC->APBRST_CTRL.UART2_RSTREQ = 0;
}

bool uart2_init(uint32_t baud)
{
    // clear intr flag
    rxChnlAlt2  = false;
    memset(uart2_rxd.data, 0, RXD_BUFF_SIZE);
    
    // DMA SRAM
    dma_init();    
    // init dma chnl
//    DMA_UARTx_TX_INIT(DMA_UART2_TX_CHAN, 2);
    DMA_UARTx_RX_INIT(DMA_UART2_RX_CHAN, 2);  

    // UART Param
    uart_init(UART2_PORT, PIN_TX2, PIN_RX2);
    
    #if (USBHIDHOST)
        uart_conf(UART2_PORT, BRR_DIV(baud, 48M), LCR_BITS_DFLT);
    #else
        uart_conf(UART2_PORT, BRR_DIV(115200, 48M), LCR_BITS_DFLT);    
    #endif
    uart_fctl(UART2_PORT, (FCR_FIFOEN_BIT | FCR_RXTL_1BYTE), 100, UART_IR_RTO_BIT);
    
    #if (!USBHIDHOST)
        #if (1)
        if (!uart2_auto_baud())  // UART2->SR.BUSY will still be 1.  --20231130 WHL
            return false;
        #else
        uartPutc(1, BAUD_RSP);
        #endif
    #endif

    uart2_rxd.head = 0;
    uart2_rxd.tail = 0;

    DMA_UARTx_RX_CONF(DMA_UART2_RX_CHAN, 2, (uart2_rxd.data), RXD_BUFF_HALF, CCM_PING_PONG);
    DMA_UARTx_RX_CONF(DMA_UART2_RX_CHAN | DMA_CH_ALT, 2, (&uart2_rxd.data[RXD_BUFF_HALF]), RXD_BUFF_HALF, CCM_PING_PONG);    
    dma_chnl_ctrl(DMA_UART2_RX_CHAN, CHNL_EN);
       
    uart_mctl(UART2_PORT, 1);  // C1 SDK is uart_mctl(1, 1);
    
#if (DMAC_IRQ)
    UART2->IER.RTO = 1;
    DMACHCFG->IEFR0 = (1UL << DMA_UART2_RX_CHAN) /*| (1UL << DMA_UART2_TX_CHAN)*/;
    NVIC_EnableIRQ(DMAC_IRQn);
    NVIC_EnableIRQ(UART2_IRQn);
    __enable_irq();
#endif

    return true;
}

#if (DMAC_IRQ)
void DMAC_IRQHandler(void)
{
    uint32_t iflag = DMACHCFG->IFLAG0;
    
    // disable intr
    DMACHCFG->IEFR0 &= ~iflag;
    // clear intr flag
    DMACHCFG->ICFR0 = iflag;
    
    if (iflag & (1UL << DMA_UART2_RX_CHAN))
    {
        dmaUartRxDone();
    }
    
//    if (iflag & (1UL << DMA_UART2_TX_CHAN))
//    {
//        txChnlBusy2 = false;
//    }
    
    // re-enable intr
    DMACHCFG->IEFR0 |= iflag;
}

void UART2_IRQHandler(void)
{
    dmaUartRxRtor();
}

void uart2_send(uint8_t *data, uint16_t len)
{
    #if (0)
    txChnlBusy2 = true;
    DMA_UARTx_TX_CONF(DMA_UART2_TX_CHAN, 2, data, len, CCM_BASIC);
    dma_chnl_ctrl(DMA_UART2_TX_CHAN, CHNL_EN);
    
    while (txChnlBusy2); // wait done 
    //while (!(UART1->SR.TEM)); //Wait Transmitter Empty
    #else
    while (len)
    {
        while (!(UART2->SR.TBEM));
        UART2->TBR = *data++;
        len--;
    } 
    #endif    
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
struct txd_buffer uart2_txd;
static volatile bool uart2_busy;
#endif

void uart2_send(uint8_t *data, uint16_t len)
{
#if (TX_FIFO)
    uint16_t head = uart2_txd.head;

    // copy data
    for (uint16_t i = 0; i < len; i++)
    {
        uart2_txd.data[head++] = *data++;
        head %= TXD_BUFF_SIZE;
    }
    uart2_txd.head = head;

    // start tx
    if (!uart2_busy)//(UART2->SR.TFEM)
    {
        uint8_t data = uart2_txd.data[uart2_txd.tail++];
        uart2_txd.tail %= TXD_BUFF_SIZE;
        UART2->TBR = data;
    }
#else
    while (len)
    {
        while (!(UART2->SR.TBEM));
        UART2->TBR = *data++;
        len--;
    }
#endif
}

void uart2_init(void)
{
    NVIC_DisableIRQ(UART2_IRQn);

    uartIOInit(1, PIN_TX2, PIN_RX2);
    uart_param_t param =
    {
        .baud     = UART2_BAUD,
        .databits = DATA_BITS_8b,
        .stopbits = STOP_BITS_1b,
        .parity   = PARITY_NONE,
    };
    uartConfig(1, &param);

#if (RX_FIFO)
    //UART2->LCR.Word = ((/*DATA_BITS_8b*/3 << 0) | (/*STOP_BITS_1b*/0 << 2) | (/*PARITY_NONE*/0 << 3)
    //                    | (/*UART_BRWEN*/1 << 7) | (/*UART_RTOEN*/1 << 8));
    //UART2->BRR = UART2_BAUD;
    //UART2->LCR.BRWEN = 0;

    UART2->FCR.RXTL = UART_FIFO_TRIG;  // RX trigger
    //UART2->FCR.TXTL = 0;     // TX trigger 0
    //UART2->FCR.FIFOEN = 1;
    //UART2->FCR.RFRST = 1;
    //UART2->FCR.TFRST = 1;

    UART2->RTOR.Word = UART_FIFO_RTOR;   // Timeout 20sym = 2Byte
    //UART2->MCR.RTSCTRL = 0;  // flow on
#endif
    // Clear interrupt
    UART2->ICR.Word = 0x13;
    //UART2->ICR.RXRD = 1;
    //UART2->ICR.RTO  = 1;
    //UART2->ICR.TXS  = 1;

    // Enable interrupt
#if (TX_FIFO)
    uart2_busy = false;
    UART2->IER.Word = 0x13;
#else
#if (RX_FIFO)
    UART2->IER.Word = 0x11;
#else
    UART2->IER.Word = 0x01;
#endif
#endif
    //UART2->IER.RXRD = 1;
    //UART2->IER.RTO  = 1;
    //UART2->IER.TXS  = 1;
    UART2->LCR.RTOEN = 1;
    UART2->LCR.RXEN = 1;     // enable RX
    //UART2->LCR.BC = 1;     // Hold TX now, as we want to fill fifo before transfer start
    NVIC_EnableIRQ(UART2_IRQn);

    uart2_rxd.head = 0;
    uart2_rxd.tail = 0;
    //uart2_send("UART2", 5);
}

void UART2_IRQHandler(void)
{
    uint32_t state = UART2->IFM.Word; // UART2->RIF.Word;
#if (TX_FIFO)
    //TX FIFO empty
    if (state & 0x02/*BIT_TXS*/)
    {
        UART2->IDR.TXS = 1;

        uint16_t size = (uart2_txd.head + TXD_BUFF_SIZE - uart2_txd.tail) % TXD_BUFF_SIZE;
        if (size > 0) //Still data to send
        {
            if (size > 16) size = 16;

            for (uint8_t i = 0; i < size; i++)
            {
                UART2->TBR = uart2_txd.data[uart2_txd.tail++];
                uart2_txd.tail %= TXD_BUFF_SIZE;
            }
            uart2_busy = true;
        }
        else //All data sent
        {
            uart2_busy = false;
        }

        UART2->ICR.TXS = 1;
        UART2->IER.TXS = 1;
    }
#endif
#if (RX_FIFO)
    //RX fifo trigger interrupt
    if (state & 0x01/*BIT_RXRD*/)
    {
        UART2->IDR.RXRD = 1; // Disable RXRD Interrupt

        for (uint8_t i = 0; i < UART_FIFO_CONT; i++)/*while (UART2->SR.RFNE)*/
        {
            uart2_rxd.data[uart2_rxd.head++] = UART2->RBR;
            uart2_rxd.head %= RXD_BUFF_SIZE;
        }

        UART2->ICR.RXRD = 1; // Clear RXRD Interrupt Flag
        UART2->IER.RXRD = 1; // Enable RXRD Interrupt
    }

    //RX Timeout trigger interrupt
    if (state & 0x10/*BIT_RTO*/)
    {
        UART2->IDR.RTO = 1; // Disable RTO Interrupt

        while (UART2->SR.RFNE)
        {
            uart2_rxd.data[uart2_rxd.head++] = UART2->RBR;
            uart2_rxd.head %= RXD_BUFF_SIZE;
        }

        UART2->ICR.RTO = 1; // Clear RTO Interrupt Flag
        UART2->IER.RTO = 1; // Enable RTO Interrupt
    }
#else
    UART2->IDR.RXRD = 1; // Disable RXRD Interrupt

    //for (uint8_t i = 0; i < UART_FIFO_CONT; i++)
    {
        uart2_rxd.data[uart2_rxd.head++] = UART2->RBR;
        uart2_rxd.head %= RXD_BUFF_SIZE;
    }

    UART2->ICR.RXRD = 1; // Clear RXRD Interrupt Flag
    UART2->IER.RXRD = 1; // Enable RXRD Interrupt
#endif
    //UART2->ICR.Word = 0xfffff;
}
#endif //(CFG_DMAC)

//const struct uart_itf uart2_itf =
//{
//    .init = uart2_init,
//    .read = uart2_read,
//    .send = uart2_send,
//};

#endif //(CFG_UART2)
