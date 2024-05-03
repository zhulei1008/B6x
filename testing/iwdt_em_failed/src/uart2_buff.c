#if (CFG_UART2)
#include "regs.h"
#include "uart2_buff.h"

#define FIFO_RXTL 8
#define BUFF_LEN  256

static uint8_t  uartBuff[BUFF_LEN];
static volatile uint16_t buff_head = 0;
static volatile uint16_t buff_tail = 0;

void UART2_IRQHandler(void)
{
    uint32_t state = UART2->IFM.Word; // UART2->RIF.Word;
    
    if (state & 0x01) //(BIT_RXRD)
    {
        UART2->IDR.RXRD = 1; // Disable RXRD Interrupt
        
        for (uint8_t i = 0; i < FIFO_RXTL; i++)
        {
            uartBuff[buff_head++] = UART2->RBR;
            buff_head %= BUFF_LEN;
        }
        
        UART2->ICR.RXRD = 1; // Clear RXRD Interrupt Flag
        UART2->IER.RXRD = 1; // Enable RXRD Interrupt
    }
    
    if (state & 0x10) //(BIT_RTO)
    {
        UART2->IDR.RTO = 1; // Disable RTO Interrupt
        
        while (UART2->SR.RFNE)
        {
            uartBuff[buff_head++] = UART2->RBR;
            buff_head %= BUFF_LEN;
        }
        
        UART2->ICR.RTO = 1; // Clear RTO Interrupt Flag
        UART2->IER.RTO = 1; // Enable RTO Interrupt
    }
}

uint16_t uart2_RxCount(void)
{
    return ((buff_head + BUFF_LEN - buff_tail) % BUFF_LEN);
}

uint16_t uart2_RxRead(uint8_t *buff, uint16_t max)
{
    uint16_t head = buff_head;
    uint16_t tail = buff_tail;
    uint16_t tlen, len = (head + BUFF_LEN - tail) % BUFF_LEN;
    if (len == 0 || max == 0)
    {
        return 0; // empty
    }
    
    if (len > max) 
        len = max;
    
    if ((head > tail) || (tail + len <= BUFF_LEN))
    {
        memcpy(&buff[0], &uartBuff[tail], len);
    } 
    else
    {
        tlen = BUFF_LEN - tail;
        
        memcpy(&buff[0], &uartBuff[tail], tlen); // tail_len
        memcpy(&buff[tlen], &uartBuff[0], len - tlen); // head_len
    }
    buff_tail = (tail + len) % BUFF_LEN;
                  
    return len; // count
}
#endif
