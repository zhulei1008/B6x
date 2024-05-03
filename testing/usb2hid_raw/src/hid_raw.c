#include "usbd.h"
#include "usbd_hid.h"
#include "drvs.h"
#include "dbg.h"

#if (DEMO_HID_CUSTOM)

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

void usbd_hid_raw_out_handler(uint8_t ep)
{
    uint8_t custom_data[RAW_EP_MPS];
    
    /*!< read the data from host send */
    uint16_t length = usbd_ep_read(RAW_OUT_EP, RAW_EP_MPS, custom_data);
    if (usbd_hid_send_report(RAW_IN_EP, length, custom_data) == USBD_OK)
    {
        debug("Reback:(nb=%d)\r\n", length);
    }

    /*!< you can use the data do some thing you like */
    debug("Recv:(nb=%d)\r\n", length);
    debugHex(custom_data, length);
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
}

void usbdTest(void)
{
    // circlaaae polling to send report
//    static uint8_t keynum = 0;
    
    if (!usbd_is_configured())
        return;

    /*!< delay 100ms the custom test */
//    {
//        bootDelayMs(100);
//        /*!< custom test */
//        uint8_t sendbuffer2[RAW_EP_MPS];
//        
//        memset(sendbuffer2, keynum, RAW_EP_MPS);
//        if (usbd_hid_send_report(RAW_IN_EP, RAW_EP_MPS, sendbuffer2) == USBD_OK)
//        {
//            debug("Send:%02X(nb=%d)\r\n", keynum, RAW_EP_MPS);
//            if (++keynum > 94) keynum = 0;
//        }
//    }
    
}

#endif // (DEMO_HID_CUSTOM)
