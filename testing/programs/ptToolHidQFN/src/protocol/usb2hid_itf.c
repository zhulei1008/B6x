#include "usbd.h"
#include "usbd_hid.h"
#include "drvs.h"
#include "uart_itf.h"

#define USBD_BCD                  USB_1_1 // Version
#define USBD_VID                  0x4919  // Vendor ID
#define USBD_PID                  0x0110  // Product ID
#define USBD_MAX_POWER            64      // unit in mA
#define USBD_LANGID_STRING        0x0409  // English(US)


/*
 * Descriptor
 ****************************************************************************
 */

/*!< count of hid interface descriptor */
#define USB_HID_INTF_CNT          1
#define USB_HID_INTF_END          (USB_HID_INTF_CNT - 1)

/*!< config descriptor size (in & out endpoints) */
#define USB_HID_CONFIG_SIZE       (9+(18+7+7)*USB_HID_INTF_CNT)

/*!< custom-raw interface config */
#define RAW_INTF_NUM              (USB_HID_INTF_CNT - 1)
#define RAW_EP_MPS                64
#define RAW_IN_EP                 0x81
#define RAW_IN_EP_INTERVAL        1
#define RAW_OUT_EP                0x01
#define RAW_OUT_EP_INTERVAL       1
#define RAW_RPT_ID                0xB6
#define RAW_REPORT_DESC_SIZE      sizeof(hid_raw_report_desc)

/*!< Declaration of endpoint Handlers  */
void usbd_hid_raw_out_handler(uint8_t ep);

/*!< hid custom-raw report descriptor */
static const uint8_t hid_raw_report_desc[] = {
    0x06, 0x00, 0xFF,  // USAGE_PAGE (Vendor Defined Page 1)
    0x09, 0x01,        // USAGE (Vendor Usage 1)
    0xA1, 0x01,        // COLLECTION (Application)
    0x85, RAW_RPT_ID,  // REPORT_ID
    0x95, RAW_EP_MPS-1,// REPORT_COUNT
    0x75, 0x08,        // REPORT_SIZE (8)
    0x26, 0xFF, 0x00,  // LOGICAL_MAXIMUM (255)
    0x15, 0x00,        // LOGICAL_MINIMUM (0)
    0x09, 0x01,        // USAGE (Vendor Usage 1)
    0x81, 0x02,        // INPUT (Data,Var,Abs)
    0x09, 0x01,        // USAGE (Vendor Usage 1)
    0x91, 0x02,        // OUTPUT (Data,Var,Abs)
    0xC0               // END_COLLECTION
};

/*!< hid device descriptor */
static const uint8_t hid_descriptor[] = {
    /* Descriptor - Device (Size:18) */
    USB_DEVICE_DESCRIPTOR_INIT(USBD_BCD, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0202, 0x01),
    
    /* Descriptor - Configuration (Total Size:9+Intf_Size) */
    USB_CONFIG_DESCRIPTOR_INIT(USB_HID_CONFIG_SIZE, USB_HID_INTF_CNT, 
            0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),

    /* Descriptor - Custom-Raw Interface (Size:18+7*2) */
    HID_INTERFACE_INIT(RAW_INTF_NUM, 2, HID_SUBCLASS_NONE, HID_PROTOCOL_NONE, 0, RAW_REPORT_DESC_SIZE),
    HID_ENDPOINT_DESC(RAW_IN_EP, RAW_EP_MPS, RAW_IN_EP_INTERVAL),
    HID_ENDPOINT_DESC(RAW_OUT_EP, RAW_EP_MPS, RAW_OUT_EP_INTERVAL),
 
    /* Descriptor - String */
    // String0 - Language ID (Size:4)
    USB_LANGID_INIT(USBD_LANGID_STRING),
    
    // String1 - iManufacturer
    0x02,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    
    // String2 - iProduct
    0x16,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    WCHAR('U'),
    WCHAR('S'),
    WCHAR('B'),
    WCHAR(' '),
    WCHAR('R'),
    WCHAR('a'),
    WCHAR('w'),
    WCHAR('H'),
    WCHAR('I'),
    WCHAR('D'),

    // String3 - iSerialNumber
    0x10,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    WCHAR('B'),
    WCHAR('6'),
    WCHAR('x'),
    WCHAR('.'),
    WCHAR('1'),
    WCHAR('.'),
    WCHAR('8'),
    
    /* Descriptor - Device Qualifier (Size:10) */
    #if (USBD_BCD == USB_2_0)
    USB_QUALIFIER_INIT(0x01),
    #endif
    
    /* Descriptor - EOF */
    0x00
};


/*
 * Configuration
 ****************************************************************************
 */

/*!< table of hid interface */
static const hid_intf_t hid_interface[] = {
    HID_INTF_T(RAW_INTF_NUM, RAW_IN_EP, hid_raw_report_desc),
};

/*!< table of endpoints */
static const usbd_ep_t endpoint_tab[] = {
    USBD_EP_T(RAW_IN_EP,  USB_EP_TYPE_INTERRUPT, RAW_EP_MPS,  &usbd_hid_ep_in_handler),
    USBD_EP_T(RAW_OUT_EP, USB_EP_TYPE_INTERRUPT, RAW_EP_MPS, &usbd_hid_raw_out_handler),
};

/*!< table of class */
static const usbd_class_t class_tab[] = {
    USBD_CLASS_T(0, USB_HID_INTF_END, &usbd_hid_class_handler),
};

/*!< USBD Configuration */
static const usbd_config_t hid_configuration[] = {
    USBD_CONFIG_T(1, USB_HID_INTF_CNT, class_tab, endpoint_tab)
};


/*
 * Handlers
 ****************************************************************************
 */
struct rxd_buffer usbd_rxd;

extern void infoReset(void);
extern void enter_boot(void);
extern void resetB(void);

void usbd_hid_raw_out_handler(uint8_t ep)
{
    uint8_t custom_data[RAW_EP_MPS];
    
    /*!< read the data from host send */
    uint8_t length = usbd_ep_read(RAW_OUT_EP, RAW_EP_MPS, custom_data);
//    if (usbd_hid_send_report(RAW_IN_EP, length, custom_data) == USBD_OK)
//    {
//        debug("Reback:(nb=%d)\r\n", length);
//    }

    /*!< you can use the data do some thing you like */
//    debug("Recv:(nb=%d)\r\n", length);
//    debugHex(custom_data, length);
    if (custom_data[0] == RAW_RPT_ID) //0xB6
    {
        if (custom_data[1] & 0x40) //bit6 Control
        {
            #if (USBHIDHOST)
            // SOP USB HID芯片
            if (custom_data[2] == 0xB5) //进入 B5x Boot 模式             
            {
                enter_boot();
            }            
            else if(custom_data[2] == 0xB6) //断开连接复位 B5x           
            {
                resetB();
            } 
            #else
            // QFN HID烧录器
            if (custom_data[2] == RAW_RPT_ID)
            {
                NVIC_SystemReset();
            }    
            #endif            
        }
        else
        {
            uint8_t  len_data = (custom_data[1]& 0x3F);
            
            if (usbd_rxd.head + len_data <= RXD_BUFF_SIZE)
            {
                memcpy(&usbd_rxd.data[usbd_rxd.head], &custom_data[2], len_data);
            }
            else
            {
                uint16_t lenEnd = RXD_BUFF_SIZE - usbd_rxd.head;
                memcpy(&usbd_rxd.data[usbd_rxd.head], &custom_data[2], lenEnd);
                memcpy(&usbd_rxd.data[0], &custom_data[lenEnd + 2], len_data - lenEnd);
            } 
            
            usbd_rxd.head = (usbd_rxd.head + len_data) % RXD_BUFF_SIZE;
        }
    }
    
}

__USBIRQ void usbd_notify_handler(uint8_t event, void *arg)
{
    switch (event) {
        case USBD_EVENT_RESET:
            usbd_hid_reset();
            break;
        case USBD_EVENT_RESUME:
            break;
        case USBD_EVENT_SUSPEND:
            break;

        default:
            break;
    }
}


/*
 * Test Functions
 ****************************************************************************
 */

void usbdInit(void)
{
    usbd_init();
    usbd_register(hid_descriptor, hid_configuration);
    
    for (uint8_t idx = 0; idx < USB_HID_INTF_CNT; idx++) {
        usbd_hid_init(idx, &hid_interface[idx]);
    }
    
    NVIC_EnableIRQ(USB_IRQn);
    __enable_irq();
}

void usbdTest(void)
{
    if (!usbd_is_configured())
        return;

    /*!< delay 100ms the custom test */
}

#define HID_SEND_LENTH_MAX  (RAW_EP_MPS - 2)

void usbd_hid_send(uint8_t *data, uint16_t length)
{
    uint16_t len = length;
    uint8_t buff[RAW_EP_MPS] = {RAW_RPT_ID};
    
    while (len)
    {
        buff[1] = (len < HID_SEND_LENTH_MAX ?  (len%HID_SEND_LENTH_MAX) : HID_SEND_LENTH_MAX);
        memcpy(&buff[2], &data[length - len], buff[1]);
        
//        uart2_send(buff, buff[1]+2);
        
        if (usbd_hid_send_report(RAW_IN_EP, RAW_EP_MPS, buff) == USBD_OK)
        {
            len -= buff[1];
        }    
    }
}

uint16_t usbd_size(void)
{
    return ((usbd_rxd.head + RXD_BUFF_SIZE - usbd_rxd.tail) % RXD_BUFF_SIZE);
}

uint16_t usbd_read(uint8_t *buff, uint16_t max)
{
    uint16_t head = usbd_rxd.head;
    uint16_t tail = usbd_rxd.tail;
    uint16_t tlen, len = (head + RXD_BUFF_SIZE - tail) % RXD_BUFF_SIZE;
    if (len == 0 || max == 0)
    {
        return 0; // empty
    }

    if (len > max)
        len = max;

    if ((head > tail) || (tail + len <= RXD_BUFF_SIZE))
    {
        memcpy(&buff[0], (const void *)&usbd_rxd.data[tail], len);
    }
    else
    {
        tlen = RXD_BUFF_SIZE - tail;

        memcpy(&buff[0], (const void *)&usbd_rxd.data[tail], tlen); // tail_len
        memcpy(&buff[tlen], (const void *)&usbd_rxd.data[0], len - tlen); // head_len
    }
    usbd_rxd.tail = (tail + len) % RXD_BUFF_SIZE;

    return len; // count
}