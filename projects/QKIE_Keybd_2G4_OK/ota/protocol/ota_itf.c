#include <string.h>
#include <stdbool.h>
#include "drvs.h"
#include "reg_uart.h"
#include "uart.h"
#include "ota_itf.h"
#include "dbg.h"
#if (PRF_OTAS || ROLE_BURNER)
struct rxd_buffer data_rxd;

void data_buffer(const uint8_t *buff, uint16_t len)
{    
    uint16_t head = data_rxd.head;
    
    if(len < (RXD_BUFF_SIZE - head))
    {
        memcpy(&data_rxd.data[head], buff, len);        
    }
    else
    {
        memcpy(&data_rxd.data[head], buff, RXD_BUFF_SIZE - head);
        
        memcpy(&data_rxd.data[0], &buff[RXD_BUFF_SIZE - head], len - (RXD_BUFF_SIZE - head));
    }
    
    data_rxd.head = (head + len) % RXD_BUFF_SIZE;
}

uint16_t ota_read(uint8_t *buff, uint16_t max)
{
    uint16_t head = data_rxd.head;
    uint16_t tail = data_rxd.tail;
    uint16_t tlen, len = (head + RXD_BUFF_SIZE - tail) % RXD_BUFF_SIZE;
    if (len == 0 || max == 0)
    {
        return 0; // empty
    }

    if (len > max)
        len = max;

    if ((head > tail) || (tail + len <= RXD_BUFF_SIZE))
    {
        memcpy(&buff[0], (const void *)&data_rxd.data[tail], len);
    }
    else
    {
        tlen = RXD_BUFF_SIZE - tail;

        memcpy(&buff[0], (const void *)&data_rxd.data[tail], tlen); // tail_len
        memcpy(&buff[tlen], (const void *)&data_rxd.data[0], len - tlen); // head_len
    }
    data_rxd.tail = (tail + len) % RXD_BUFF_SIZE;

    return len; // count
}


#if (ROLE_BURNER)
void sesc_cb_recv(uint8_t conidx, uint16_t len, const uint8_t *data)
{            
    debug("rxd(cid:%d,len:%d)", conidx, len);
    debugHex(data, len);
    
    data_buffer(data, len);
} 
#else
void otas_cb_rxd(uint8_t conidx, uint16_t len, const uint8_t *data)
{    
    //debug("rxd(cid:%d,len:%d)\r\n", conidx, len);
    //debugHex(data, len);
    
    data_buffer(data, len);
}
#endif //(ROLE_BURNER)
#endif //(CFG_UART1)
