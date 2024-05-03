/**
 ****************************************************************************************
 *
 * @file usbd_drvs.c
 *
 * @brief USB 1.1 FULL-SPEED FUNCTION CONTROLLER
 *
 ****************************************************************************************
 */

#include "reg_usb.h"
#include "usbd_int.h"


/*
 * Global Variables
 ****************************************************************************
 */

struct usbd_hal_tag usbd_hal;

/* Buffer used for storing standard, class and vendor request data */
//static uint8_t usbd_req_data[USB_REQ_DATA_LEN];
#define usbd_req_data       usbd_hal.req_data

#define usbd_dev_addr       usbd_hal.dev_addr
#define usbd_ep0_state      usbd_hal.ep0_state


/*
 * Macro for USB get and set
 ****************************************************************************
 */

// EP active index
#define usb_epidx_get()     ((uint8_t)(USB->EPIDX))
#define usb_epidx_set(idx)  do{ USB->EPIDX = idx; }while(0)

// Data Count
#define usb_ep0_count()     ((uint8_t)(USB->COUNT0 & 0x7F)) // 7bits
#define usb_out_count()     ((uint8_t)(USB->OUTCOUNT1)) // ((uint16_t)((USB->OUTCOUNT1 & 0xFF) | ((USB->OUTCOUNT2 & 0x07) << 8)))
#define usb_num_frame()     ((uint16_t)((USB->FRAME1 & 0xFF) | ((USB->FRAME2 & 0x07) << 8)))


/*
 * FIFO Read and Write
 ****************************************************************************
 */
#if 0
static void usb_fifo_read(uint8_t ep_idx, uint16_t len, void *buff)
{
    uint8_t *ptr_buff  = (uint8_t *)buff;
    
    #if (USB_FIFO_BUG)
    // read 2 times -- Fix Asyn-Read Bug
    volatile uint8_t bug;
    bug = USB->FIFO[ep_idx];
    bug = USB->FIFO[ep_idx];
    #endif
    
    for (uint16_t i = 0; i < len; i++)
    {
        *ptr_buff++ = USB->FIFO[ep_idx];
    }
    
    //USB_LOG_DBG("*FiFo RD(idx:%d,len:%d)", ep_idx, len);
}

static void usb_fifo_write(uint8_t ep_idx, uint16_t len, const void *data)
{
    uint8_t *ptr_data  = (uint8_t *)data;
    
    #if (USB_FIFO_BUG)
    // from data[1] -- Fix FiFo-OnWay Bug
    for (uint16_t i = 1; i < len; i++)
    {
        ptr_data++; 
        USB->FIFO[ep_idx] = *ptr_data;
    }
    
    // fill data[0]
    USB->FIFO[ep_idx] = *((uint8_t *)data);
    #else
    for (uint16_t i = 0; i < len; i++)
    {
        USB->FIFO[ep_idx] = *ptr_data++;
    }
    #endif
    
    //USB_LOG_DBG("*FiFo WR(idx:%d,len:%d)\r\n", ep_idx, len);
}
#endif

/*
 * EndPoint0 Read and Write
 ****************************************************************************
 */

__USBIRQ static uint16_t usbd_ep0_read(uint8_t *buff, uint16_t max_len)
{
    uint16_t count;

    // Bytes of FIFO Received
    count = usb_ep0_count();

    if (count > max_len) {
        count = max_len;
    }

    // Unload FIFO
    //if (count > 0) {
    //    usb_fifo_read(0, count, buff);
    //}
    for (uint16_t i = 0; i < count; i++)
    {
        buff[i] = USB->FIFO[0];
    }
    
    return count;
}

__USBIRQ static uint16_t usbd_ep0_write(const uint8_t *data, uint16_t data_len)
{
    uint16_t count;

    #if (USB_FIFO_BUG)
    usb_epidx_set(0); // or usb_epidx_get() to fix fifo sync, desc size 17->18
    #endif
    // Fill data to FIFO (Not excced maximum packet size)
    count = (data_len < USB_EP0_MPS) ? data_len : USB_EP0_MPS;
    
    //if (count > 0) {
    //    usb_fifo_write(0, count, data);
    //}
    for (uint16_t i = 0; i < count; i++)
    {
        USB->FIFO[0] = data[i];
    }
    
    // Triggle send, include data_len=0
    if (count < USB_EP0_MPS)
        USB->CSR0 = USB_CSR0_INRDY | USB_CSR0_DATAEND;
    else
        USB->CSR0 = USB_CSR0_INRDY;

    return count;
}


/*
 * EndPoint(1~NUM) Read and Write
 ****************************************************************************
 */

uint16_t usbd_ep_read(uint8_t ep, uint16_t max_len, uint8_t *buff)
{
    uint16_t count;
    uint8_t ep_idx = USB_EP_GET_IDX(ep);

    // Bytes of FIFO Received
    count = usb_out_count();
    
    if (count > 0) {
        //USB_LOG_DBG("rd(cnt=%d,csr=%X)\r\n", count,USB->OUTCSR1);
        if (count > max_len) {
            count = max_len;
        }
    
        // Unload FIFO, then Clear OutPktRdy(bit0)
        //usb_fifo_read(ep_idx, count, buff);
        for (uint16_t i = 0; i < count; i++)
        {
            buff[i] = USB->FIFO[ep_idx];
        }
        
        //if (count != usbd_hal.in_ep[ep_idx].ep_mps)
        USB->OUTCSR1 &= ~(USB_OUTCSR1_OUTRDY);
    }

    return count;
}

uint8_t usbd_ep_write(uint8_t ep, uint16_t data_len, const uint8_t *data, uint16_t *wr_bytes)
{
    uint8_t status = USBD_OK;
    uint8_t ep_idx = USB_EP_GET_IDX(ep);
    uint8_t ep_old;

    if ((ep_idx == 0) || (!data && data_len)) {
        return USBD_ERR_INVAILD;
    }

    ep_old = usb_epidx_get();
    usb_epidx_set(ep_idx);

    // 1 - Wait last packet finish
    uint32_t timeout = 0x7ff;
    while (USB->INCSR1 & USB_INCSR1_INRDY) {
        if (USB->INCSR1 & USB_INCSR1_UNDRN) {
            status = USBD_ERR_UNDRN;
            goto _RET;
        }
        if (!(timeout--)) {
            status = USBD_ERR_TIMEOUT;
            USB_LOG_ERR("WR Timeout(ep:%d,csr:%02X)\r\n", usb_epidx_get(), USB->INCSR1);
            goto _RET;
        }
    }

    // 2 - Fill data to FIFO (Not excced maximum packet size)
    uint16_t ep_mps = (USB->INMAXP) << 3; // 8-bytes unit
    uint16_t count = (data_len < ep_mps) ? data_len : ep_mps;

    // Return send bytes if need - before triggle 6vp 1208
    if (wr_bytes) {
        *wr_bytes = count;
    }

    //if (count > 0) {
    //    usb_fifo_write(ep_idx, count, (uint8_t *)data);
    //}
    for (uint16_t i = 0; i < count; i++)
    {
        USB->FIFO[ep_idx] = data[i];
    }
    
    // Triggle send, include data_len=0
    USB->INCSR1 = USB_INCSR1_INRDY;

_RET:
    usb_epidx_set(ep_old);
    return status;
}


/*
 * EndPoint(1~NUM) Control
 ****************************************************************************
 */

__USBIRQ uint8_t usbd_ep_open(uint8_t ep, uint8_t type, uint8_t mps)
{
    uint8_t ep_old, reg_csr2 = 0;
    uint8_t ep_idx = USB_EP_GET_IDX(ep);
    
    if ((ep_idx == 0) || (ep_idx >= USB_EP_NUM) || (mps > USB_EP_MPS)) {
        return USBD_ERR_INVAILD;
    }
    
    ep_old = usb_epidx_get();
    usb_epidx_set(ep_idx);
    
    if (type == 0x01/*USB_EP_TYPE_ISOCHRONOUS*/) {
        reg_csr2 |= 0x40; // D6: ISO
    } else if (type == 0x02/*USB_EP_TYPE_BULK*/) {
        reg_csr2 |= 0x80; // D7: AutoSet|AutoClear
    }
    
    if (USB_EP_DIR_IS_OUT(ep)) {
        USB->INTROUT1E |= USB_ISR_EP(ep_idx);
        USB->OUTMAXP = (mps + 7) >> 3; // 8-bytes unit
        USB->OUTCSR2 = reg_csr2;
        
        // Reset the Data toggle to zero.
        USB->OUTCSR1 = USB_OUTCSR1_CLRDT | USB_OUTCSR1_FLUSH;
    } else {
        if (type == 0x03 /*USB_EP_TYPE_INTERRUPT*/) {
            reg_csr2 |= 0x08; // D3: FrcDataTog
        }
        
        USB->INTRIN1E |= USB_ISR_EP(ep_idx);
        USB->INMAXP = (mps + 7) >> 3; // 8-bytes unit
        USB->INCSR2 = reg_csr2;
        
        // Reset the Data toggle to zero.
        USB->INCSR1 = USB_INCSR1_CLRDT | USB_INCSR1_FLUSH;
    }
    
    usb_epidx_set(ep_old);
    return USBD_OK;
}

__USBIRQ void usbd_ep_close(uint8_t ep)
{

}

#if (1)
__USBIRQ void usbd_ep_stall(uint8_t ep, uint8_t evt)
{
    uint8_t ep_idx = USB_EP_GET_IDX(ep);
    
    if (ep_idx == 0x00) {
        if (evt == USBD_EVENT_SET_ENDPOINT_HALT) {
            usbd_ep0_state = EP0_STATE_STALL;
            USB->CSR0 |= (USB_CSR0_STALL | USB_CSR0_OUTRDYCLR);
        } else {
            USB->CSR0 &= ~USB_CSR0_STALLED;
        }
    } else {
        if (USB_EP_DIR_IS_OUT(ep)) {
            //ep_idx += 8;
            USB->OUTCSR1 = (evt == USBD_EVENT_SET_ENDPOINT_HALT) ? (USB->OUTCSR1 | USB_OUTCSR1_STALL) 
                : ((USB->OUTCSR1 & ~(USB_OUTCSR1_STALL | USB_OUTCSR1_STALLED)) | USB_OUTCSR1_CLRDT);
        } else {
            USB->INCSR1 = (evt == USBD_EVENT_SET_ENDPOINT_HALT) ? (USB->INCSR1 | USB_INCSR1_STALL) 
                : ((USB->INCSR1 & ~(USB_INCSR1_STALL | USB_INCSR1_STALLED)) | USB_INCSR1_CLRDT);
        }
    }
    
    // USBD_EVENT_SET_ENDPOINT_HALT or USBD_EVENT_CLR_ENDPOINT_HALT
    usbd_notify_handler(evt, NULL);
}
#else
__USBIRQ void usbd_ep_set_stall(uint8_t ep)
{
    uint8_t ep_idx = USB_EP_GET_IDX(ep);

    if (ep_idx == 0x00) {
        usbd_ep0_state = EP0_STATE_STALL;
        USB->CSR0 |= (USB_CSR0_STALL | USB_CSR0_OUTRDYCLR);
    } else {
        if (USB_EP_DIR_IS_OUT(ep)) {
            //ep_idx += 8;
            USB->OUTCSR1 |= USB_OUTCSR1_STALL;
        } else {
            USB->INCSR1 |= USB_INCSR1_STALL;
        }
    }
    
    //usbd_hal.ep_stall |= 1U << ep_idx;
}

__USBIRQ void usbd_ep_clear_stall(uint8_t ep)
{
    uint8_t ep_idx = USB_EP_GET_IDX(ep);

    if (ep_idx == 0x00) {
        USB->CSR0 &= ~USB_CSR0_STALLED;
    } else {
        if (USB_EP_DIR_IS_OUT(ep)) {
            //ep_idx += 8;
            // Clear the stall on an OUT endpoint.
            USB->OUTCSR1 &= ~(USB_OUTCSR1_STALL | USB_OUTCSR1_STALLED);
            // Reset the data toggle.
            USB->OUTCSR1 |= USB_OUTCSR1_CLRDT;
        } else {
            // Clear the stall on an IN endpoint.
            USB->INCSR1 &= ~(USB_INCSR1_STALL | USB_INCSR1_STALLED);
            // Reset the data toggle.
            USB->INCSR1 |= USB_INCSR1_CLRDT;
        }
    }
    
    //usbd_hal.ep_stall &= ~(1U << ep_idx);
}
#endif

uint8_t usbd_ep_is_stalled(uint8_t ep, uint8_t *stalled)
{
    return 0;
}


/*
 * USB System Handler
 ****************************************************************************
 */

/**
 * @brief send data or status to host
 *
 * @return true - finish, false - more data
 */
__USBIRQ static bool usbd_send_to_host(uint16_t len)
{
    bool finish = true;

    uint16_t chunk = usbd_ep0_write(usbd_hal.ep0_data_ptr, usbd_hal.ep0_data_res);

    usbd_hal.ep0_data_ptr += chunk;
    usbd_hal.ep0_data_res -= chunk;
    
    if (usbd_hal.ep0_data_res) {
        /* Send more data if available */
        finish = false;
    } else {
        /*
         * Set ZLP flag when host asks for a bigger length and the
         * last chunk is wMaxPacketSize long, to indicate the last
         * packet.
         */
        if ((len > usbd_hal.ep0_data_len) && !(usbd_hal.ep0_data_len % USB_EP0_MPS)) {
            finish = false;
        }
    }
    
    return finish;
}

__USBIRQ static void usbd_reset_handler(void)
{
    USB_LOG_INFO("reset(addr:%d,UE:%X,IE:%X,OE:%X)\r\n", USB->FADDR,USB->INTRUSBE, USB->INTRIN1E,USB->INTROUT1E);

    /* Reset any state machines for each endpoint */
    //USB->FADDR = 0;
    USB->INTRUSBE = USB_ISR_RESET | USB_ISR_RESUME | USB_ISR_SUSPEND;
    USB->INTRIN1E = USB_ISR_EP0;
    USB->INTROUT1E = 0;
    USB->POWER = USB_POWER_ENSUSPEND;

    for (uint8_t ep_idx = 1; ep_idx < USB_EP_NUM; ep_idx++) {
        usb_epidx_set(ep_idx);
        USB->INCSR1  = USB_INCSR1_CLRDT | USB_INCSR1_FLUSH;
        USB->OUTCSR1 = USB_OUTCSR1_CLRDT | USB_OUTCSR1_FLUSH;
    }

    // reset configuration
    //memset(&usbd_hal, 0, sizeof(usbd_hal));
    usbd_hal_reset();
    usbd_env_reset();
    
    usbd_notify_handler(USBD_EVENT_RESET, NULL);
}

__USBIRQ static void usbd_resume_handler(void)
{
    // Add resume routine here
    USB_LOG_DBG("resume(%02X)\r\n", USB->POWER);
    usbd_notify_handler(USBD_EVENT_RESUME, NULL);
}

__USBIRQ static void usbd_suspend_handler(void)
{
    // Add suspend routine here
    USB_LOG_DBG("suspend(%02X)\r\n", USB->POWER);
    usbd_notify_handler(USBD_EVENT_SUSPEND, NULL);
}

/* Interrupt Handler */
__USBIRQ static void usbd_ep0_handler(void)
{
    uint8_t ep0_status = USB->CSR0;

    /* Check for SentStall */
    if (ep0_status & USB_CSR0_STALLED) {
        USB->CSR0 = (ep0_status & ~USB_CSR0_STALLED);
        usbd_ep0_state = EP0_STATE_SETUP;
        return;
    }

    /* Check for SetupEnd */
    if (ep0_status & USB_CSR0_SETUPEND) {
        USB->CSR0 = (ep0_status | USB_CSR0_SETUPENDCLR);
        usbd_ep0_state = EP0_STATE_SETUP;
        return;
    }

    /* Complete SET_ADDRESS command */
    if (usbd_dev_addr > 0) {
        USB->FADDR = usbd_dev_addr;
        usbd_dev_addr = 0;
    }

    /* Call relevant routines for endpoint 0 state */
    switch (usbd_ep0_state) {
        case EP0_STATE_SETUP:
        {
            if ((ep0_status & USB_CSR0_OUTRDY) == 0) {
                break;
            }
            
            /* There is no need to check that OutCount is set to 8 */
            /* as the MUSBFSFC will reject SETUP packets that are not 8 bytes long. */
            //if (usb_ep0_count() != 8) break;

            /* Read the 8-byte command from the FIFO */
            //usb_fifo_read(0, 8, &usbd_hal.setup);
            for (uint16_t i = 0; i < 8; i++)
            {
                usbd_hal.setup_byte[i] = USB->FIFO[0];
            }
            
            usbd_hal.ep0_data_ptr = usbd_req_data;
            usbd_hal.ep0_data_len = usbd_hal.setup.wLength;
            usbd_hal.ep0_data_res = usbd_hal.setup.wLength;
            
            if (usbd_hal.setup.wLength) {
                if ((usbd_hal.setup.bmRequestType & USB_REQUEST_DIR_MASK) == USB_REQUEST_DIR_OUT) {
                    /* RX-Mode: Prepare to Recv OR Size Error*/
                    if (usbd_hal.setup.wLength > USB_REQ_DATA_LEN) {
                        USB_LOG_ERR("Request buffer too small\r\n"); 
                        USB_PRINT_SETUP((&usbd_hal.setup));
                        
                        //usbd_ep_set_stall(USB_CONTROL_IN_EP0);
                        USB->CSR0 = (USB_CSR0_STALL | USB_CSR0_OUTRDYCLR);
                        usbd_ep0_state = EP0_STATE_STALL;
                    } else {
                        USB_LOG_DBG("prepare to out data\r\n");
                        
                        // this maybe set code in class request code
                        USB->CSR0 = USB_CSR0_OUTRDYCLR;
                        usbd_ep0_state = EP0_STATE_OUT_DATA;
                    }
                    break;
                }
            }

            /* Ask installed handler to process request */
            if (usbd_setup_request_handler(&usbd_hal.setup, &usbd_hal.ep0_data_ptr, &usbd_hal.ep0_data_len) != USBD_OK) {
                USB_LOG_ERR("usbd_setup_request_handler failed\r\n");
                
                //usbd_ep_set_stall(USB_CONTROL_IN_EP0);
                USB->CSR0 = (USB_CSR0_STALL | USB_CSR0_OUTRDYCLR);
                usbd_ep0_state = EP0_STATE_STALL;
                break;
            }
            
            /* RX-Mode: Zero Length Packey finish */
            if (usbd_hal.setup.wLength == 0) {
                USB->CSR0 = USB_CSR0_OUTRDYCLR | USB_CSR0_DATAEND;
                usbd_ep0_state = EP0_STATE_IN_STATUS;
                break;
            }

            /* TX-Mode: Send smallest of requested and offered length */
            USB->CSR0 = USB_CSR0_OUTRDYCLR;
            
            usbd_hal.ep0_data_res = MIN(usbd_hal.ep0_data_len, usbd_hal.setup.wLength);

            #ifdef CONFIG_USB_DCACHE_ENABLE
            /* check if the data buf addr uses usbd_req_data */
            if (((uint32_t)usbd_hal.ep0_data_ptr) != ((uint32_t)usbd_req_data)) {
                /*copy data buf from misalign32 addr to align32 addr*/
                memcpy(usbd_req_data, usbd_hal.ep0_data_ptr, usbd_hal.ep0_data_res);
                usbd_hal.ep0_data_ptr = usbd_req_data;
            }
            #endif
            usbd_ep0_state = EP0_STATE_IN_DATA;
            //uint8_t csr = USB->CSR0;
            /* Send data or status to host */
            //if (usbd_send_to_host(usbd_hal.setup.wLength)) {
            //    usbd_ep0_state = EP0_STATE_OUT_STATUS;
            //} else {
            //    usbd_ep0_state = EP0_STATE_IN_DATA;
            //}

            //if ((usbd_hal.setup.wLength != 0xFF) && (usbd_hal.setup.wLength > 64)) {
            //    USB_LOG_RAW("setup0->(csr:%02X->%02X,res:%d,len:%d)\r\n", csr,USB->CSR0,usbd_hal.ep0_data_res, usbd_hal.ep0_data_len);
            //}
        } /*no break;*/

        case EP0_STATE_IN_DATA:
        {
            //uint8_t csr = USB->CSR0;
            if (usbd_send_to_host(usbd_hal.setup.wLength)) {
                /* ep0 tx has completed, and no data to send */
                usbd_ep0_state = EP0_STATE_OUT_STATUS;
            }
            
            //if ((usbd_hal.setup.wLength != 0xFF) && (usbd_hal.setup.wLength > 64)) {
            //    USB_LOG_RAW("in0->(csr:%02X->%02X,res:%d,len:%d)\r\n", csr,USB->CSR0,usbd_hal.ep0_data_res, usbd_hal.ep0_data_len);
            //}
        } break;
        
        case EP0_STATE_IN_STATUS:
        {
            usbd_ep0_state = EP0_STATE_SETUP;
        } break;
        
        case EP0_STATE_OUT_DATA:
        {
            if ((ep0_status & USB_CSR0_OUTRDY) == 0) {
                break;
            }

            if (usbd_hal.ep0_data_res > 0) {
                /* OUT transfer, data packets */
                uint16_t chunk = usbd_ep0_read(usbd_hal.ep0_data_ptr, usbd_hal.ep0_data_res);
                usbd_hal.ep0_data_ptr += chunk;
                usbd_hal.ep0_data_res -= chunk;
                
                if (usbd_hal.ep0_data_res == 0) {
                    /* Received all, send data to handler */
                    usbd_hal.ep0_data_ptr = usbd_req_data;

                    if (usbd_setup_request_handler(&usbd_hal.setup, &usbd_hal.ep0_data_ptr, &usbd_hal.ep0_data_len) != USBD_OK) {
                        USB_LOG_ERR("usbd_setup_request_handler1 failed\r\n");
                        //usbd_ep_set_stall(USB_CONTROL_IN_EP0);
                        USB->CSR0 = (USB_CSR0_STALL | USB_CSR0_OUTRDYCLR);
                        usbd_ep0_state = EP0_STATE_STALL;
                        break;
                    }
                    
                    /*Send status to host*/
                    usbd_send_to_host(usbd_hal.setup.wLength);
                }
            } else {
                /* OUT transfer, status packets */
                /* absorb zero-length status message */
                USB_LOG_DBG("recv status\r\n");
            }
            
            if (usbd_hal.setup.wLength > USB_EP0_MPS) {
                USB->CSR0 = USB_CSR0_OUTRDYCLR;
            } else {
                USB->CSR0 = USB_CSR0_OUTRDYCLR | USB_CSR0_DATAEND;
                usbd_ep0_state = EP0_STATE_IN_STATUS;
            }
        } break;
        
        case EP0_STATE_OUT_STATUS:
        {
            usbd_ep0_state = EP0_STATE_SETUP;
        } break;
        
        default:
            break;
    }
}

#if (DBG_USB_IRQ)
bool usb_enter;
uint32_t usb_sof_pulse;
uint32_t usb_irq_state;
#endif // (DBG_USB_IRQ)

__USBIRQ void USB_IRQHandler(void)
{
    uint8_t ep_old, ep_idx;
    uint32_t isr_usb, isr_in, isr_out;
//    debug("usb irq\r\n");
    ep_old = usb_epidx_get();
    
    /* 0-Read Interrupt registers(EP_NUM < 8, only read INTR1) */
    isr_usb = USB->INTRUSB;
    isr_in  = USB->INTRIN1;
    isr_out = USB->INTROUT1;
    #if (DBG_USB_IRQ)
    usb_enter = true;
    usb_irq_state = (isr_usb << 0) | (isr_in << 8) | (isr_out << 16);
    #endif // (DBG_USB_IRQ)
    
    /* 1-Check for resume from suspend mode */
    if (isr_usb & USB_ISR_RESUME) {
        usbd_resume_handler();
    }
    
    /* 2-Receive a reset signal from the USB bus */
    if (isr_usb & USB_ISR_RESET) {
        usbd_reset_handler();
    }

    /* 3-Handle EP0 interrupt */
    isr_in &= USB->INTRIN1E;
    if (isr_in & USB_ISR_EP0) {
        usb_epidx_set(0);
        usbd_ep0_handler();
    }
    
    /* 4-Handle IN-EP(1~NUM) interrupt */
    if (isr_in > 1) {
        for (ep_idx = 1; ep_idx < USB_EP_NUM; ep_idx++)
        {
            if (isr_in & USB_ISR_EP(ep_idx)) {
                usb_epidx_set(ep_idx);
                //USB_LOG_RAW("EP:%d,IN_CSR1:%X,IN_CSR2:%X\r\n", ep_idx, USB->INCSR1, USB->INCSR2);

                if (USB->INCSR1 & USB_INCSR1_STALLED) {
                    USB->INCSR1 &= ~USB_INCSR1_STALLED;
                    break;
                }
                
                if (USB->INCSR1 & USB_INCSR1_UNDRN) {
                    USB->INCSR1 &= ~USB_INCSR1_UNDRN;
                }
                
                usbd_ep_isr_handler(USB_EP_IN(ep_idx));
            }
        }
    }
    
    /* 5-Handle OUT-EP(1~NUM) interrupt */
    isr_out &= USB->INTROUT1E;
    if (isr_out > 1) {
        for (ep_idx = 1; ep_idx < USB_EP_NUM; ep_idx++)
        {
            if (isr_out & USB_ISR_EP(ep_idx)) {
                usb_epidx_set(ep_idx);
                
                if (USB->OUTCSR1 & USB_OUTCSR1_STALLED) {
                    USB->OUTCSR1 &= ~USB_OUTCSR1_STALLED;
                    break;
                }
                
                //USB_LOG_RAW("EP:%d,OUT_CSR1:%X,OUT_CNT:%X\r\n", ep_idx, USB->OUTCSR1, USB->OUTCOUNT1);
                if ((USB->OUTCSR1 & USB_OUTCSR1_OUTRDY) != 0) { // remove to fix delay-bug
                    usbd_ep_isr_handler(USB_EP_OUT(ep_idx));
                }
            }
        }
    }
    
    /* 6-Check for suspend mode */
    if (isr_usb & USB_ISR_SUSPEND) {
        usbd_suspend_handler();
    }

    // restore epidx to continue
    usb_epidx_set(ep_old);
}


/*
 * USB Device APIs
 ****************************************************************************
 */

void usbd_init(void)
{
    //memset(&usbd_hal, 0, sizeof(usbd_hal));
    usbd_hal_reset();
    
    // usbd_hal_init()
    USB->EPIDX = 0;
    USB->FADDR = 0;
    
    // enable interrupts
    USB->INTRUSBE = USB_ISR_RESET;
    USB->INTRIN1E = USB_ISR_EP0;
    USB->INTROUT1E = 0;
    USB->POWER = USB_POWER_ENSUSPEND;
}

void usbd_deinit(void)
{

}

bool usbd_resume(bool enable)
{
    uint8_t power = USB->POWER;
    
    if (enable && (power & USB_POWER_SUSPEND)) {
        USB->POWER = power | USB_POWER_RESUME;
        return true;
    }
    
    if (!enable && (power & USB_POWER_RESUME)) {
        USB->POWER = power & ~USB_POWER_RESUME;
        return true;
    }
    
    return false;
}
