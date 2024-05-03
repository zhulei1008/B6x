/**
 ****************************************************************************************
 *
 * @file usbd_hid.c
 *
 * @brief Function of USB Human Interface Device (HID)
 *
 ****************************************************************************************
 */
#if (CFG_USB)

#include "app_user.h"
#include "app.h"
#include "usbd.h"
#include "usbd_hid.h"
#include "drvs.h"
#include "gatt_api.h"

#if (DBG_USB)
#include "dbg.h"
#define DEBUG(format, ...) debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

/// HID global environment
static usbd_hid_t usbd_hids[HID_INST_CNT];

/// Interface number convert to hid instance
static usbd_hid_t *find_hid_by_intf(uint8_t intf_num)
{
    for (uint8_t i = 0; i < HID_INST_CNT; i++)
    {
        if (usbd_hids[i].hid_intf && (usbd_hids[i].hid_intf->intf_num == intf_num))
        {
            return &usbd_hids[i];
        }
    }

    return NULL;
}

/// Endpoint address convert to hid instance
static usbd_hid_t *find_hid_by_ep(uint8_t ep_addr)
{
    for (uint8_t i = 0; i < HID_INST_CNT; i++)
    {
        if (usbd_hids[i].hid_intf && (usbd_hids[i].hid_intf->ep_in == ep_addr))
        {
            return &usbd_hids[i];
        }
    }

    return NULL;
}

void usbd_hid_init(uint8_t idx, const hid_intf_t *intf)
{
    if (idx < HID_INST_CNT)
    {
        usbd_hids[idx].hid_intf = intf;
        // memset(&usbd_hids[idx].hid_state, 0, 4);
        usbd_hids[idx].hid_info = 0;
    }
}

void usbd_hid_reset(void)
{
    for (uint8_t idx = 0; idx < HID_INST_CNT; idx++)
    {
        // memset(&usbd_hids[idx].hid_state, 0, 4);
        usbd_hids[idx].hid_info  = 0;
//        usbd_hids[idx].hid_state = HID_STATE_IDLE;
    }
}

__WEAK void usbd_hid_leds(uint8_t state)
{
    /*!< here you can write the LED processing from the host */
    if (state & HID_KBD_LED_NUM_LOCK)
    {
        /*!< num lock */
    }
    else
    {
    }

    if (state & HID_KBD_LED_CAPS_LOCK)
    {
        /*!< caps lock */
    }
    else
    {
    }

    if (state & HID_KBD_LED_SCROLL_LOCK)
    {
        /*!< scroll lock */
    }
    else
    {
    }
}

uint8_t usbd_hid_send_report(uint8_t ep, uint8_t len, const uint8_t *data)
{
    uint8_t status = USBD_FAIL;

    if (usbd_is_configured())
    {
        usbd_hid_t *curr_hid = find_hid_by_ep(ep);

//        if (curr_hid) && (curr_hid->hid_state == HID_STATE_IDLE)) {
        if (curr_hid)
        {
            curr_hid->hid_state = HID_STATE_BUSY; // Update before isr occure

            status = usbd_ep_write(ep, len, data, NULL);
            if (status != USBD_OK)
            {
                curr_hid->hid_state = HID_STATE_IDLE; // fail to recover
            }
        }
    }

    return status;
}

uint8_t usbd_mouse_report(void)
{
    uint8_t status = USBD_FAIL;

    if (usbd_is_configured())
    {
        usbd_hid_t *curr_hid = find_hid_by_ep(MOUSE_IN_EP);

        if ((curr_hid) && (curr_hid->hid_state == HID_STATE_IDLE))
        {
            uint8_t *pkt = get_mouse_pkt();

            if (pkt) 
            {
                usbd_wakeup();

                curr_hid->hid_state = HID_STATE_BUSY; // Update before isr occure

                status = usbd_ep_write(MOUSE_IN_EP, USB_RPT_LEN_MOUSE, pkt, NULL);

                if (status != USBD_OK)
                {
                    curr_hid->hid_state = HID_STATE_IDLE; // fail to recover
                }
            }
        }
    }

    return status;
}

uint8_t usbd_kb_report(void)
{
    uint8_t status = USBD_FAIL;

    if (usbd_is_configured())
    {
        usbd_hid_t *curr_hid = find_hid_by_ep(KEYBD_IN_EP);

        if ((curr_hid) && (curr_hid->hid_state == HID_STATE_IDLE))
        {
            uint8_t *pkt = get_kb_pkt();

            if (pkt) 
            {
                usbd_wakeup();

                curr_hid->hid_state = HID_STATE_BUSY; // Update before isr occure

                status = usbd_ep_write(KEYBD_IN_EP, USB_RPT_LEN_KB, pkt, NULL);

                if (status != USBD_OK)
                {
                    curr_hid->hid_state = HID_STATE_IDLE; // fail to recover
                }
            }
        }
    }

    return status;
}

uint8_t usbd_consumer_report(void)
{
    uint8_t status = USBD_FAIL;

    if (usbd_is_configured())
    {
        usbd_hid_t *curr_hid = find_hid_by_ep(CONSUMER_IN_EP);

        if ((curr_hid) && (curr_hid->hid_state == HID_STATE_IDLE))
        {
            uint8_t *pkt = get_consumer_pkt();

            if (pkt) 
            {
                usbd_wakeup();

                DEBUG("pkt:%x, len:%d, id:%d", (uint32_t)pkt, *(pkt+0), *(pkt+1));
                curr_hid->hid_state = HID_STATE_BUSY; // Update before isr occure

                status = usbd_ep_write(CONSUMER_IN_EP, *(pkt+0), (pkt+1), NULL);

                if (status != USBD_OK)
                {
                    curr_hid->hid_state = HID_STATE_IDLE; // fail to recover
                }
            }
        }
    }

    return status;
}

void usbd_hid_ep_in_handler(uint8_t ep)
{
    usbd_hid_t *curr_hid = find_hid_by_ep(ep);

    if (curr_hid && (curr_hid->hid_state == HID_STATE_BUSY))
    {
        /*!< transfer successfully, update the state */
        curr_hid->hid_state = HID_STATE_IDLE;
    }
}

/**
 * @brief Handler called for Class requests.
 *
 * @param setup    Information about the request to execute.
 * @param data     Buffer containing the request result.
 * @param len      Size of the buffer.
 *
 * @return  0 on success, errno code on fail.
 */
uint8_t usbd_hid_class_handler(struct usb_setup_packet *setup, uint8_t **data, uint16_t *len)
{
    uint8_t     intf_num;
    usbd_hid_t *curr_hid;
//    DEBUG("bmRequestType:%02x,bRequest:%02x,wValue:%04x,wIndex:%04x,wLength:%04x", setup->bmRequestType,
//          setup->bRequest, setup->wValue, setup->wIndex, setup->wLength);

    if ((setup->bmRequestType & USB_REQUEST_RECIP_MASK) != USB_REQUEST_RECIP_INTERFACE)
    {
        return USBD_FAIL;
    }

    intf_num = (uint8_t)setup->wIndex;
    curr_hid = find_hid_by_intf(intf_num);

    if (!curr_hid)
    {
        return USBD_FAIL;
    }

    // Standard Request - Get Hid Report Descriptor
    if ((setup->bmRequestType & USB_REQUEST_TYPE_MASK) == USB_REQUEST_STANDARD)
    {
        if (setup->bRequest == USB_REQUEST_GET_DESCRIPTOR)
        {
            uint8_t desc_typ = (uint8_t)(setup->wValue >> 8);

            DEBUG("HID Get_Desc(type:%x,intf:%d)", desc_typ, intf_num);
            if (desc_typ == HID_DESC_TYPE_HID_REPORT)
            {
                if (curr_hid)
                {
                    *len  = curr_hid->hid_intf->desc_size;
                    *data = (uint8_t *)curr_hid->hid_intf->report_desc;
                }
                return USBD_OK;
            }
        }

        return USBD_FAIL;
    }

    // Class Request - HID Get/Set
    switch (setup->bRequest)
    {
        case HID_REQUEST_GET_REPORT:
        {
            DEBUG("HID Get_Report(report_id:%d,report_type:%d)", LO_BYTE(setup->wValue), HI_BYTE(setup->wValue));

            *data = (uint8_t *)&curr_hid->report;
            *len  = 1;
        }
        break;

        case HID_REQUEST_GET_IDLE:
        {
            DEBUG("HID Get_Idle(report_id:%d)", LO_BYTE(setup->wValue));

            *data = (uint8_t *)&curr_hid->idle_state;
            *len  = 1;
        }
        break;

        case HID_REQUEST_GET_PROTOCOL:
        {
            DEBUG("HID Get_Protocol");

            *data = (uint8_t *)&curr_hid->protocol;
            *len  = 1;
        }
        break;

        // host --> device
        case HID_REQUEST_SET_REPORT:
        {
            DEBUG("HID Set_Report(report_id:%d,report_type:%d,report_len:%d,report_data:%02X)", LO_BYTE(setup->wValue),
                  HI_BYTE(setup->wValue), *len, (*data)[0]);
            debugHex(*data, *len);

            #if (0)
            // 带 report id长度为2, 第一个数据为report id
            if (*len == 2)
            {
                curr_hid->report = (*data)[1];
//                usbd_hid_leds((*data)[1]);
                g_kb_led = *(*data + 1);
                //if(g_kb_led!=0xff){g_kb_led|=(g_os_type<<7);}
                DEBUG("g_kb_led:%x\r\n",g_kb_led);
                if ((app_state_get() == APP_CONNECTED) && (g_cid_kb != 0xFF))
                {
                    DEBUG("Write KB(cid:%x, led %x)", g_cid_kb, *(*data + 1));
                    if(g_os_type)
                    {
                        
                        g_kb_led|=g_os_type;
                        g_os_type = 0;
                    }
                    gatt_write(g_cid_kb, GATT_WRITE_NO_RESPONSE, GATT_WR_HDL, (uint8_t *)&g_kb_led/**data + 1*/, 1);
                    g_kb_led&=0x0f;
                }
            }
            #else
            if (*len == 1)
            {
                curr_hid->report = (*data)[0];
//                usbd_hid_leds((*data)[1]);
                g_kb_led = *(*data + 0);
                //if(g_kb_led!=0xff){g_kb_led|=(g_os_type<<7);}
                DEBUG("g_kb_led:%x\r\n",g_kb_led);
                if ((app_state_get() == APP_CONNECTED) && (g_cid_kb != 0xFF))
                {
                    DEBUG("Write KB(cid:%x, led %x)", g_cid_kb, *(*data + 0));
                    if(g_os_type)
                    {
                        g_kb_led|=g_os_type;
                        g_os_type = 0;
                    }
                    gatt_write(g_cid_kb, GATT_WRITE_NO_RESPONSE, GATT_WR_HDL, (uint8_t *)&g_kb_led/**data + 1*/, 1);
                    g_kb_led&=0x0f;
                }
            }
            #endif
        }
        break;

        case HID_REQUEST_SET_IDLE:
        {
            DEBUG("HID Set_Idle(report_id:%d,duration:%d)", LO_BYTE(setup->wValue), HI_BYTE(setup->wIndex));
            g_os_type = 0x40;//windows
            g_os_type_back = g_os_type;
            curr_hid->idle_state = HI_BYTE(setup->wIndex);

        }
        break;

        case HID_REQUEST_SET_PROTOCOL:
        {
            DEBUG("HID Set_Protocol(%d)", LO_BYTE(setup->wValue)); /*protocol*/
            g_os_type = 0x80;//IOS
            g_os_type_back = g_os_type;
            //g_kb_led|=g_os_type;
            curr_hid->protocol = LO_BYTE(setup->wValue);
        }
        break;

        default:
        {
            DEBUG("Unhandled HID Class bRequest 0x%02x", setup->bRequest);
            return USBD_FAIL;
        }
    }

    return USBD_OK;
}
#endif // CFG_USB
