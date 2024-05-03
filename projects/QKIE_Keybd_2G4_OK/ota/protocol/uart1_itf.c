#include <string.h>
#include <stdbool.h>
#include "hyb5x.h"
#include "reg_uart.h"
#include "uart.h"
#include "ota_itf.h"

#if (CFG_UART1)

#if (ROLE_CHIPSET)
volatile uint32_t UART1_BAUD __attribute__((at(0x080090C0))) = HOST_UART_BAUD; //0x080000C0, 0x080090C0
#else
#define UART1_BAUD  HOST_UART_BAUD
#endif

struct rxd_buffer uart1_rxd;
//uint8_t  uart1_rxd_data[RXD_BUFF_SIZE];
//volatile uint16_t uart1_rxd_head;
//volatile uint16_t uart1_rxd_tail;

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
#include "dma.h"

#define DMAC_IRQ    0 // Not use irq

DMA_CHNL_CTRL_STRUCT_Typedef gDMACtrlBase __attribute__((aligned(0x100))); //__attribute__((at(0x20001500)));

void uart1_send(uint8_t *data, uint16_t len)
{   
    dma_ping_config(DMA_UART1_TX_CHAN, (uint32_t)(data + 0), len, DMA_UART1_TX, DMA_BASIC);
    dma_chan_ctrl(DMA_UART1_TX_CHAN, DMA_CHAN_ENABLE);

    while (!(DMA_DONE_STA_GET(DMA_UART1_TX_CHAN)));
    DMA_DONE_STA_CLR(DMA_UART1_TX_CHAN);
    
    while (!(UART1->SR.TEM)); //Wait Transmitter Empty 211110 --whl   
//    WAIT_UART_IDLE();
}

void dmaPingRestart(uint8_t chIndex, uint16_t buffSize)
{
    uint32_t dmaCfg = 0;
    DMA_CHNL_CTRL_Typedef *CHNx_Pri     = (DMA_CHNL_CTRL_Typedef *)_DMA_CHNx(chIndex);

    dmaCfg =  CHNx_Pri->TRANS_SET_DATA.Word;
    buffSize = buffSize - 1;
    dmaCfg = (dmaCfg | DMA_PING_PONG | buffSize << 4);
    CHNx_Pri->TRANS_SET_DATA.Word = dmaCfg;
     
}

void dmaPongRestart(uint8_t chIndex, uint16_t buffSize)
{
    uint32_t dmaCfg = 0;
    //get channel alt structure memory address
    DMA_CHNL_CTRL_Typedef *CHNx_Alt = (DMA_CHNL_CTRL_Typedef *)_DMA_CHNx(DMA_CHN_MAX + chIndex);

    dmaCfg =  CHNx_Alt->TRANS_SET_DATA.Word;
    buffSize = buffSize - 1;
    dmaCfg = (dmaCfg | DMA_PING_PONG | buffSize << 4);

    CHNx_Alt->TRANS_SET_DATA.Word = dmaCfg;
    
}

#include "iopad.h"

//static bool pong_flag = false;

extern bool down_flag;

void uart1_proc(void)
{
//    gpioDataSet(PIN_START);
//    gpioDataClr(PIN_START);    
    // primary or alternate transfer done
    if (DMA_DONE_STA_GET(DMA_UART1_RX_CHAN))
    { 
            
        if (DMA_CUR_DATA_STRUCT_GET(DMA_UART1_RX_CHAN) )
        {
            // alternate data structure
//            dma_ping_config(DMA_UART1_RX_CHAN, (uint32_t)&uart1_rxd.data[0], RXD_BUFF_SIZE >> 1, DMA_UART1_RX, DMA_PING_PONG);
            
//            gpioDataSet(PIN_ADC_CHIP);
//            gpioDataClr(PIN_ADC_CHIP);
//            if (down_flag) uart1_send((uint8_t *)&uart1_rxd, sizeof(struct rxd_buffer)); 
            
            dmaPingRestart(DMA_UART1_RX_CHAN, RXD_BUFF_SIZE >> 1);
//            uart1_rxd.head = RXD_BUFF_SIZE >> 1;
        }
        else
        {
            // primary data structure
//            dma_pong_config(DMA_UART1_RX_CHAN, (uint32_t)&uart1_rxd.data[RXD_BUFF_SIZE >> 1], RXD_BUFF_SIZE >> 1, DMA_UART1_RX);
//            gpioDataSet(PIN_PWM_CHIP);
//            gpioDataClr(PIN_PWM_CHIP);
//            if (down_flag)
//            {
//                pong_flag++;
////                uart1_send((uint8_t *)&uart1_rxd, sizeof(struct rxd_buffer));
//            }                
            
            dmaPongRestart(DMA_UART1_RX_CHAN, RXD_BUFF_SIZE >> 1);  
            
            
//            uart1_rxd.head = 0;
        }
        DMA_DONE_STA_CLR(DMA_UART1_RX_CHAN);       
    }   
//    else
//    {   
        // Refresh rxd_head
        if (DMA_CUR_DATA_STRUCT_GET(DMA_UART1_RX_CHAN))
        {
//            pong_flag = true;
//            gpioDataSet(PIN_START);           
            uart1_rxd.head = (RXD_BUFF_SIZE - dma_chan_rmain_data_num(DMA_UART1_RX_CHAN, ALT_CFG_GET)) % RXD_BUFF_SIZE;
        }
        else
        {
//            gpioDataClr(PIN_START); 
            uart1_rxd.head = (RXD_BUFF_SIZE >> 1) - dma_chan_rmain_data_num(DMA_UART1_RX_CHAN, PRI_CFG_GET);
        } 
        
//        if (pong_flag && down_flag)
//        {
//            uint16_t ping = dma_chan_rmain_data_num(DMA_UART1_RX_CHAN, PRI_CFG_GET);
//            uint16_t pong = dma_chan_rmain_data_num(DMA_UART1_RX_CHAN, ALT_CFG_GET);
//            
//            uart1_send((uint8_t *)&ping, 2);
//            uart1_send((uint8_t *)&pong, 2);
//        }
        
//    }
}

void uart1_init(void)
{ 
    // DMA SRAM
    dma_init((uint32_t)&gDMACtrlBase);

    // UART Param
    uartIOInit(0, PIN_TX1, PIN_RX1);
    uart_param_t param =
    {
        .baud     = UART1_BAUD,
        .databits = DATA_BITS_8b,
        .stopbits = STOP_BITS_1b,
        .parity   = PARITY_NONE,
    };
    uartConfig(0, &param);
    //IOMCTL->PIOA[PIN_TX1].DST  = 3; // 0 :1/8, 1: 1/4, 2: 1/2, 3: Max

    uart1_rxd.head = 0;
    uart1_rxd.tail = 0;

    // DMA Conf: direct Init
    UART1->MCR.DMAEN = 0;
    UART1->IER.Word = 0;  
    NVIC_DisableIRQ(UART1_IRQn);
    UART1->LCR.RTOEN = 0;
    
    dma_ping_config(DMA_UART1_RX_CHAN, (uint32_t)&uart1_rxd.data[0], RXD_BUFF_SIZE >> 1, DMA_UART1_RX, DMA_STOP);
    DMA_CUR_DATA_STRUCT_CLR(DMA_UART1_RX_CHAN); //211118 --whl
    
//    DMACHCFG->ch03.ch2_sel = 0x7F;
    DMA_DONE_STA_CLR(DMA_UART1_RX_CHAN);
    
    dma_ping_config(DMA_UART1_RX_CHAN, (uint32_t)&uart1_rxd.data[0], RXD_BUFF_SIZE >> 1, DMA_UART1_RX, DMA_PING_PONG);
    dma_pong_config(DMA_UART1_RX_CHAN, (uint32_t)&uart1_rxd.data[RXD_BUFF_SIZE >> 1], RXD_BUFF_SIZE >> 1, DMA_UART1_RX);
    dma_chan_ctrl(DMA_UART1_RX_CHAN, DMA_CHAN_ENABLE);
    UART1->MCR.DMAEN = 1;

#if (DMAC_IRQ)
    DMA_IRQ_CTRL(0x01 << DMA_UART1_RX_CHAN);
    NVIC_EnableIRQ(DMAC_IRQn);
#endif
}

#if (DMAC_IRQ)
void DMAC_IRQHandler(void)
{
    uint32_t irqStats = DMACHCFG->iflag0.Word;
    // judge produce interupt the channel number
    if (irqStats)
    {
        //TODO  close all dma or single dma interupt enable ???
        DMACHCFG->iefr0.Word &= ~irqStats;

        // clear interupt flag
        DMACHCFG->icfr0.Word |= irqStats;

        DMA_DONE_STA_CLR(DMA_UART1_RX_CHAN);

        //uint8_t dma_alt_struct = DMA_CUR_DATA_STRUCT_GET(DMA_UART1_RX_CHAN);

        // alternate data structure
        if ( DMA_CUR_DATA_STRUCT_GET(DMA_UART1_RX_CHAN) )
        {
            dma_ping_config(DMA_UART1_RX_CHAN, (uint32_t)&uart1_rxd.data[0], RXD_BUFF_SIZE >> 1, DMA_UART1_RX, DMA_PING_PONG);
            //uart1_rxd.head = (RXD_BUFF_SIZE >> 1);
        }
        else // primary data structure
        {
            dma_pong_config(DMA_UART1_RX_CHAN, (uint32_t)&uart1_rxd.data[RXD_BUFF_SIZE >> 1], RXD_BUFF_SIZE >> 1, DMA_UART1_RX);
            //uart1_rxd.head = 0;
        }

        // enable dma interupt
        DMACHCFG->iefr0.Word |= irqStats;
    }
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
//#include "reg_iopad.h"
void uart1_init(void)
{
    uartIOInit(0, PIN_TX1, PIN_RX1);
    //    IOMCTL->PIOA[PIN_TX1].PUD = 1;//IO_NOPULL;
    //    IOMCTL->PIOA[PIN_RX1].PUD = 1;//IO_NOPULL;
    uart_param_t param =
    {
        .baud     = UART1_BAUD,
        .databits = DATA_BITS_8b,
        .stopbits = STOP_BITS_1b,
        .parity   = PARITY_NONE,
    };
    uartConfig(0, &param);
    //IOMCTL->PIOA[PIN_TX1].DST  = 3; // 0 :1/8, 1: 1/4, 2: 1/2, 3: Max

#if (RX_FIFO)
    //UART1->LCR.Word |= UART_RTOEN;
    //UART1->LCR.Word = ((/*DATA_BITS_8b*/3 << 0) | (/*STOP_BITS_1b*/0 << 2) | (/*PARITY_NONE*/0 << 3)
    //                    | (/*UART_BRWEN*/1 << 7) | (/*UART_RTOEN*/1 << 8));
    //UART1->LCR.Word = ((UART_NOPARITY << 3) | (UART_STOPBITS1 << 2) |
    //                           (UART_BYTESIZE8 << 0) | UART_BRWEN | UART_RTOEN);
    //UART1->BRR = UART1_BAUD;
    //UART1->LCR.BRWEN = 0;

    UART1->FCR.RXTL = UART_FIFO_TRIG;  // RX trigger
    //UART1->FCR.TXTL = 0;     // TX trigger 0
    //UART1->FCR.FIFOEN = 1;
    //UART1->FCR.RFRST = 1;
    //UART1->FCR.TFRST = 1;

    UART1->RTOR.Word = UART_FIFO_RTOR;   // Timeout 20sym = 2Byte
    //UART1->MCR.RTSCTRL = 0;  // flow on
#endif
    // Clear interrupt
    UART1->ICR.Word = 0x13;
    //UART1->ICR.RXRD = 1;
    //UART1->ICR.RTO  = 1;
    //UART1->ICR.TXS  = 1;

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
    //UART1->IER.RXRD = 1;
    //UART1->IER.RTO  = 1;
    //UART1->IER.TXS  = 1;
    UART1->LCR.RTOEN = 1;
    UART1->LCR.RXEN = 1;     // enable RX
    //UART1->LCR.BC = 1;     // Hold TX now, as we want to fill fifo before transfer start
    NVIC_EnableIRQ(UART1_IRQn);

    uart1_rxd.head = 0;
    uart1_rxd.tail = 0;
    //uart1_send("UART1", 5);
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
        //UART1->IDR.RXRD = 1; // Disable RXRD Interrupt

        for (uint8_t i = 0; i < UART_FIFO_CONT; i++)
        {
            uart1_rxd.data[uart1_rxd.head++] = UART1->RBR;
            uart1_rxd.head %= RXD_BUFF_SIZE;
        }

        UART1->ICR.Word = 0x01; // Clear RXRD Interrupt Flag
        //UART1->IER.RXRD = 1; // Enable RXRD Interrupt
    }

    //RX Timeout trigger interrupt
    if (state & 0x10/*BIT_RTO*/)
    {
        //UART1->IDR.RTO = 1; // Disable RTO Interrupt

        while (UART1->SR.RFNE)
        {
            uart1_rxd.data[uart1_rxd.head++] = UART1->RBR;
            uart1_rxd.head %= RXD_BUFF_SIZE;
        }

        UART1->ICR.Word = 0x10; // Clear RTO Interrupt Flag
        //UART1->IER.RTO = 1; // Enable RTO Interrupt
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
#if (DEBUG_MODE)

#include <stdio.h>
int fputc(int ch, FILE *f)
{

    while (!(UART1->SR.TBEM));
    UART1->TBR = ch;
    return ch;
}
#endif

//const struct uart_itf uart1_itf =
//{
//    .init = uart1_init,
//    .read = uart1_read,
//    .send = uart1_send,
//};

#endif //(CFG_UART1)
