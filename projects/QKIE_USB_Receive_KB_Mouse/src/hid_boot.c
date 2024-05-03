#if (CFG_USB)

#include "regs.h"
#include "usbd.h"
#include "usbd_hid.h"
#include "drvs.h"

#define USBD_BCD           USB_1_1 // Version
//#define USBD_VID           0xFFFF  // Vendor ID
//#define USBD_PID           0xFF00  // Product ID
#define USBD_VID           0x3839  // Vendor ID
#define USBD_PID           0x1088  // Product ID
#define USBD_MAX_POWER     100     // unit in mA
#define USBD_LANGID_STRING 0x0409  // English(US)

#if (DBG_USB)
#include "dbg.h"
#define DEBUG(format, ...) debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

/*
 * Descriptor
 ****************************************************************************
 */

/*!< count of hid interface descriptor */
#define USB_HID_INTF_CNT (ENB_KEYBD + ENB_MOUSE + ENB_CONSUMER)
#define USB_HID_INTF_END (USB_HID_INTF_CNT - 1)

/*!< config descriptor size (only in endpoint) */
#define USB_HID_CONFIG_SIZE (9 + (18 + 7) * USB_HID_INTF_CNT)

#if (ENB_KEYBD)
/*!< keyboard interface config */
#define KEYBD_INTF_NUM         (0)
#define KEYBD_IN_EP_SIZE       (USB_RPT_LEN_KB)
#define KEYBD_IN_EP_INTERVAL   (4)
#define KEYBD_REPORT_DESC_SIZE sizeof(hid_keybd_report_desc)

/*!< keyboard report descriptor */
static const uint8_t hid_keybd_report_desc[] =
{
    #if (HID_RPT_KB)
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
//    0x85, RPT_ID_KB,   //   Report ID
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0xE0,        //   Usage Minimum (0xE0)
    0x29, 0xE7,        //   Usage Maximum (0xE7)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x06,        //   Report Count (6)
    0x15, 0x00,        //   Logical Minimum (0)
//    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x25, 0xFF,        //   Logical Maximum (255)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x29, 0xFF,        //   Usage Maximum (0xFF)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x05,        //   Report Count (5)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x05,        //   Usage Maximum (Kana)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x75, 0x03,        //   Report Size (3)
    0x95, 0x01,        //   Report Count (1)
    0x91, 0x03,        //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection
    #endif //(HID_RPT_KB)
};
#endif // ENB_KEYBD

#if (ENB_CONSUMER)
#define CONSUMER_INTF_NUM         (0 + ENB_KEYBD)
#define CONSUMER_IN_EP_SIZE       (USB_RPT_LEN_MEDIA)
#define CONSUMER_IN_EP_INTERVAL   (4)
#define CONSUMER_REPORT_DESC_SIZE sizeof(hid_consumer_report_desc)
static const uint8_t hid_consumer_report_desc[] =
{
    #if (HID_RPT_MEDIA)
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, RPT_ID_MEDIA,//   Report ID
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x18,        //   Report Count (24)

    0x09, 0x70,       // LOCAL_USAGE(Display Brightness Decrement)
    0x09, 0x6F,       // LOCAL_USAGE(Display Brightness Increment)
    0x0A, 0x21, 0x02, // LOCAL_USAGE(AC Search)
    0x0A, 0x92, 0x01, // LOCAL_USAGE(AL Calculator)
    0x0A, 0x06, 0x03, // LOCAL_USAGE(Cut)
    0x0A, 0x07, 0x03, // LOCAL_USAGE(Copy)
    0x09, 0xB6,       // LOCAL_USAGE(Scan Previous Track)
    0x09, 0xCD,       // LOCAL_USAGE(Play/Pause)
    0x09, 0xB5,       // LOCAL_USAGE(Scan Next Track)
    0x09, 0xE2,       // LOCAL_USAGE(Mute)
    0x09, 0xEA,       // LOCAL_USAGE(Volume Decrement)
    0x09, 0xE9,       // LOCAL_USAGE(Volume Increment)
    0x0A, 0x24, 0x02, // LOCAL_USAGE(AC Back)
    0x0A, 0x8A, 0x01, // LOCAL_USAGE(AL Email Reader)
    0x0A, 0xAE, 0x01, // LOCAL_USAGE(AL Keyboard Layout)
    0x0A, 0x23, 0x02, // LOCAL_USAGE(AC Home)
    0x0A, 0x96, 0x01, // LOCAL_USAGE(AL Internet Browser)
    0x0A, 0x92, 0x01, // LOCAL_USAGE(CALCAULATOL)
    0x0A, 0x94, 0x01, // LOCAL_USAGE(COMPUTER)
    0x0A, 0xB7, 0x00, // LOCAL_USAGE(STOP)
    0x0A, 0x2A, 0x02, // LOCAL_USAGE(FAVORITES)
    0x0A, 0x07, 0x03, // LOCAL_USAGE(AND_VIRKB)
    0x0A, 0x83, 0x01, // LOCAL_USAGE(AL Consumer ControlConfiguration)
    0x09, 0x30,       // LOCAL_USAGE(Power)

    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
    #endif //(HID_RPT_MEDIA)

    #if (HID_RPT_SYSTEM)
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x80,        // Usage (Sys Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, RPT_ID_SYSTEM,//   Report ID
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x03,        //   Report Count (3)
    0x09, 0x81,        //   Usage (Sys Power Down)
    0x09, 0x82,        //   Usage (Sys Sleep)
    0x09, 0x83,        //   Usage (Sys Wake Up)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x05,        //   Report Count (5)
    0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
    #endif //(HID_RPT_SYSTEM)
};
#endif // ENB_CONSUMER

#if (ENB_MOUSE)
/*!< mouse interface config */
#define MOUSE_INTF_NUM         (0 + ENB_KEYBD + ENB_CONSUMER)
#define MOUSE_IN_EP_SIZE       (USB_RPT_LEN_MOUSE)
#define MOUSE_IN_EP_INTERVAL   (4)
#define MOUSE_REPORT_DESC_SIZE sizeof(hid_mouse_report_desc)

/*!< mouse report descriptor */
static const uint8_t hid_mouse_report_desc[] =
{
    0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
    0x09, 0x02, // Usage (Mouse)
    0xA1, 0x01, // Collection (Application)
    0x09, 0x01, //   Usage (Pointer)
    0xA1, 0x00, //   Collection (Physical)
    0x05, 0x09, //     Usage Page (Button)
    0x19, 0x01, //     Usage Minimum (0x01)
    0x29, 0x08, //     Usage Maximum (0x08)
    0x15, 0x00, //     Logical Minimum (0)
    0x25, 0x01, //     Logical Maximum (1)
    0x75, 0x01, //     Report Size (1)
    0x95, 0x08, //     Report Count (8)
    0x81, 0x02, //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01, //     Usage Page (Generic Desktop Ctrls)
    0x15, 0x81, //     Logical Minimum (-127)
    0x25, 0x7F, //     Logical Maximum(127)
    0x75, 0x08, //     Report Size(8)
    0x95, 0x02, //     Report Size(2)
    0x09, 0x30, //     Usage (X)
    0x09, 0x31, //     Usage (Y)
    0x81, 0x06, //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x15, 0x81, //     Logical Minimum (-127)
    0x25, 0x7F, //     Logical Maximum (127)
    0x75, 0x08, //     Report Size (8)
    0x95, 0x01, //     Report Count (1)
    0x09, 0x38, //     Usage (Wheel)
    0x81, 0x06, //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,       //   End Collection
    0xC0,       // End Collection
};
#endif // ENB_MOUSE

/*!< hid device descriptor */
static const uint8_t hid_descriptor[] = 
{
    /* Descriptor - Device (Size:18) */
    USB_DEVICE_DESCRIPTOR_INIT(USBD_BCD, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0002, 0x01),

    /* Descriptor - Configuration (Total Size:9+Intf_Size) */
    USB_CONFIG_DESCRIPTOR_INIT(USB_HID_CONFIG_SIZE, USB_HID_INTF_CNT, 0x01,
                               USB_CONFIG_BUS_POWERED | USB_CONFIG_REMOTE_WAKEUP, USBD_MAX_POWER),

#if (ENB_KEYBD)
    /* Descriptor - Keyboard Interface (Size:18+7*1) */
    HID_INTERFACE_INIT(KEYBD_INTF_NUM, 1, HID_SUBCLASS_BOOTIF, HID_PROTOCOL_KEYBOARD, 0, KEYBD_REPORT_DESC_SIZE),
    HID_ENDPOINT_DESC(KEYBD_IN_EP, KEYBD_IN_EP_SIZE, KEYBD_IN_EP_INTERVAL),
#endif

#if (ENB_CONSUMER)
    /* Descriptor - Keyboard Interface (Size:18+7*1) */
    HID_INTERFACE_INIT(CONSUMER_INTF_NUM, 1, HID_SUBCLASS_BOOTIF, HID_PROTOCOL_REPORT, 0, CONSUMER_REPORT_DESC_SIZE),
    HID_ENDPOINT_DESC(CONSUMER_IN_EP, CONSUMER_IN_EP_SIZE, CONSUMER_IN_EP_INTERVAL),
#endif
    
#if (ENB_MOUSE)
    /* Descriptor - Mouse Interface (Size:18+7*1) */
    HID_INTERFACE_INIT(MOUSE_INTF_NUM, 1, HID_SUBCLASS_BOOTIF, HID_PROTOCOL_MOUSE, 0, MOUSE_REPORT_DESC_SIZE),
    HID_ENDPOINT_DESC(MOUSE_IN_EP, MOUSE_IN_EP_SIZE, MOUSE_IN_EP_INTERVAL),
#endif // ENB_MOUSE

    /* Descriptor - String */
    // String0 - Language ID (Size:4)
    USB_LANGID_INIT(USBD_LANGID_STRING),

    // String1 - iManufacturer
    0x02,                 /* bLength */
    USB_DESC_TYPE_STRING, /* bDescriptorType */

    // String2 - iProduct
    0x2C,                 /* bLength */
    USB_DESC_TYPE_STRING, /* bDescriptorType */
    WCHAR('Q'),
    WCHAR('K'),
    WCHAR('I'),
    WCHAR('E'),
    WCHAR(' '),
    WCHAR('U'),
    WCHAR('S'),
    WCHAR('B'),
    WCHAR(' '),
    WCHAR('R'),
    WCHAR('e'),
    WCHAR('c'),
    WCHAR('e'),
    WCHAR('i'),
    WCHAR('v'),
    WCHAR('e'),
    WCHAR('r'),
    WCHAR(' '),
    WCHAR('K'),
    WCHAR('&'),
    WCHAR('M'),
    
    // String3 - iSerialNumber
//    0x0C,                 /* bLength */
//    USB_DESC_TYPE_STRING, /* bDescriptorType */
//    WCHAR('1'),
//    WCHAR('1'),
//    WCHAR('.'),
//    WCHAR('1'),
//    WCHAR('7'),

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
static const hid_intf_t hid_interface[] =
{
#if (ENB_KEYBD)
    HID_INTF_T(KEYBD_INTF_NUM, KEYBD_IN_EP, hid_keybd_report_desc),
#endif
    
#if (ENB_CONSUMER)
    HID_INTF_T(CONSUMER_INTF_NUM, CONSUMER_IN_EP, hid_consumer_report_desc),
#endif
    
#if (ENB_MOUSE)
    HID_INTF_T(MOUSE_INTF_NUM, MOUSE_IN_EP, hid_mouse_report_desc),
#endif // ENB_MOUSE
};

/*!< table of endpoints */
static const usbd_ep_t endpoint_tab[] =
{
#if (ENB_KEYBD)
    USBD_EP_T(KEYBD_IN_EP, USB_EP_TYPE_INTERRUPT, KEYBD_IN_EP_SIZE, &usbd_hid_ep_in_handler),
#endif
    
#if (ENB_CONSUMER)
    USBD_EP_T(CONSUMER_IN_EP, USB_EP_TYPE_INTERRUPT, CONSUMER_IN_EP_SIZE, &usbd_hid_ep_in_handler),
#endif
    
#if (ENB_MOUSE)
    USBD_EP_T(MOUSE_IN_EP, USB_EP_TYPE_INTERRUPT, MOUSE_IN_EP_SIZE, &usbd_hid_ep_in_handler),
#endif // ENB_MOUSE
};

/*!< table of class */
static const usbd_class_t class_tab[] =
{
    USBD_CLASS_T(0, USB_HID_INTF_END, &usbd_hid_class_handler),
};

/*!< USBD Configuration */
static const usbd_config_t hid_configuration[] =
{
    USBD_CONFIG_T(1, USB_HID_INTF_CNT, class_tab, endpoint_tab)
};

/*
 * Handlers
 ****************************************************************************
 */
volatile bool g_suspend;
__USBIRQ void usbd_notify_handler(uint8_t event, void *arg)
{
    DEBUG("0 --- evt:%d, cfg:%d", event, usbd_is_configured());
    switch (event)
    {
        case USBD_EVENT_RESET:
        {
            g_suspend = false;
            usbd_hid_reset();
        } break;

        case USBD_EVENT_SUSPEND:
        {
            g_suspend = true;
        } break;

        case USBD_EVENT_RESUME:
        case USBD_EVENT_CLR_REMOTE_WAKEUP:
        {
            g_suspend = false;
        } break;

        default:
            break;
    }
}

void usbd_wakeup(void)
{
    if (g_suspend && usbd_resume(1))
    {
        btmr_delay(48000, 30);
        usbd_resume(0);
    }
}

void usbdInit(void)
{
    g_suspend = false;
    
    // enable USB clk and iopad
    rcc_usb_en();

    // .DIG_USB_PU = 1, 1.87K
    // .DIG_USB_PU = 2, 1.32K
    // .DIG_USB_PU = 3, 1.27K
    SYSCFG->USB_CTRL.Word = 0x14;

    usbd_init();
    usbd_register(hid_descriptor, hid_configuration);

    NVIC_EnableIRQ(USB_IRQn);
    NVIC_SetPriority(BLE_IRQn, 1);

    for (uint8_t idx = 0; idx < USB_HID_INTF_CNT; idx++)
    {
        usbd_hid_init(idx, &hid_interface[idx]);
    }
}
#endif // (CFG_USB)
