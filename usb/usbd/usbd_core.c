/**
 ****************************************************************************************
 *
 * @file usbd_core.c
 *
 * @brief USB Core Descriptors & Interface
 *
 ****************************************************************************************
 */

#include "usbd_int.h"


#if 0//!defined(CONFIG_USB_HS)
#define USB_DESC_TYPE_MAX              (USB_DESC_TYPE_ENDPOINT)
#else
#define USB_DESC_TYPE_MAX              (USB_DESC_TYPE_OTHER_SPEED)
#endif

/* general descriptor field offsets(@see usb_desc_header) */
#define DESC_bLength                   0 /** Length offset */
#define DESC_bDescriptorType           1 /** Descriptor type offset */

/* config descriptor field offsets(@see usb_configuration_descriptor) */
#define CONF_DESC_wTotalLength         2 /** Total length offset */
#define CONF_DESC_bConfigurationValue  5 /** Configuration value offset */
#define CONF_DESC_bmAttributes         7 /** configuration characteristics */

/* interface descriptor field offsets(@see usb_interface_descriptor) */
#define INTF_DESC_bInterfaceNumber     2 /** Interface number offset */
#define INTF_DESC_bAlternateSetting    3 /** Alternate setting offset */

/* endpoint descriptor field offsets(@see usb_endpoint_descriptor)*/
#define EP_DESC_bAddress               2
#define EP_DESC_bmType                 3
#define EP_DESC_wMPS                   4

/* Global Environment */
struct usbd_env_tag usbd_env;

/* Macro for alias invoke */

#define usbd_descriptors        usbd_env.descriptors
#define usbd_configurations     usbd_env.configurations
#define usbd_configurecount     usbd_env.conf_cnt

#define usbd_curr_config        (&usbd_env.configurations[usbd_env.conf_num-1])
#define usbd_intf_cnt           usbd_env.configurations->intf_cnt
#define usbd_class_tab          usbd_env.configurations->class_tab
#define usbd_class_cnt          usbd_env.configurations->class_cnt
#define usbd_ep_tab             usbd_env.configurations->ep_tab
#define usbd_ep_cnt             usbd_env.configurations->ep_cnt

#if (USBD_OS_DESC)
#define usbd_bos_desc           usbd_env.bos_desc
#define usbd_msosv1_desc        usbd_env.msv1_desc
#define usbd_msosv2_desc        usbd_env.msv2_desc
#endif 

#if (USBD_FEAT_TEST)
void usbd_set_feature(uint16_t index, uint16_t value);
void usbd_clear_feature(uint16_t index, uint16_t value);
#endif


/**
 * @brief Check if the device is in Configured state
 *
 * @return true if Configured, false otherwise.
 */
#define is_device_configured() (usbd_env.conf_num != 0)

/**
 * @brief Check if the interface of given number is valid
 *
 * @param [in] interface Number of the addressed interface
 *
 * This function searches through descriptor and checks
 * is the Host has addressed valid interface.
 *
 * @return true if interface exists - valid
 */
__USBIRQ static bool is_interface_valid(uint8_t intf_num)
{
    #if (DESC_SEARCH_MODE)
    if (is_device_configured()) {
        const uint8_t *p = (uint8_t *)usbd_descriptors;
        const struct usb_configuration_descriptor *cfg_descr;

        /* Search through descriptor for matching interface */
        while (p[DESC_bLength] != 0U) {
            if (p[DESC_bDescriptorType] == USB_DESC_TYPE_CONFIGURATION) {
                cfg_descr = (const struct usb_configuration_descriptor *)p;

                if (intf_num < cfg_descr->bNumInterfaces) {
                    return true;
                }
            }

            p += p[DESC_bLength];
        }
    }
    #else
    if (is_device_configured()) {
        if (intf_num < usbd_intf_cnt) {
            return true;
        }
    }
    #endif

    return false;
}

/**
 * @brief Check if the endpoint of given address is valid
 *
 * @param [in] ep Address of the Endpoint
 *
 * This function checks if the Endpoint of given address
 * is valid for the configured device. Valid Endpoint is
 * either Control Endpoint or one used by the device.
 *
 * @return true if endpoint exists - valid
 */
__USBIRQ static bool is_ep_valid(uint8_t ep)
{
    /* Check if its Endpoint 0 */
    if ((ep & 0x7f) == 0) {
        return true;
    }

    /* Check if in usbd_ep_tab */
    for (uint8_t idx = 0; idx < usbd_ep_cnt; idx++) {
        if (ep == usbd_ep_tab[idx].ep_addr) {
            return true;
        }
    }
    
    return false;
}

#if (0)
/**
 * @brief configure and enable endpoint
 *
 * This function sets endpoint configuration according to one specified in USB
 * endpoint descriptor and then enables it for data transfers.
 *
 * @param [in]  ep_desc Endpoint descriptor byte array
 *
 * @return true if successfully configured and enabled
 */
__USBIRQ static __forceinline void usbd_set_endpoint(const struct usb_endpoint_descriptor *ep_desc)
{
    uint8_t ep_addr = ep_desc->bEndpointAddress;
    uint8_t ep_type = ep_desc->bmAttributes & USB_EP_TYPE_MASK;
    uint16_t ep_mps = ep_desc->wMaxPacketSize & 0xFF;//USB_MAXPACKETSIZE_MASK;
    
    USB_LOG_INFO("Open endpoint:0x%x type:%d mps:%d\r\n",ep_addr, ep_type, ep_mps);
    
    usbd_ep_open(ep_addr, ep_type, ep_mps);
}

/**
 * @brief Disable endpoint for transferring data
 *
 * This function cancels transfers that are associated with endpoint and
 * disabled endpoint itself.
 *
 * @param [in]  ep_desc Endpoint descriptor byte array
 *
 * @return true if successfully deconfigured and disabled
 */
__USBIRQ static __forceinline void usbd_reset_endpoint(const struct usb_endpoint_descriptor *ep_desc)
{
    uint8_t ep_addr = ep_desc->bEndpointAddress;
    uint8_t ep_type = ep_desc->bmAttributes & USB_EP_TYPE_MASK;
    uint16_t ep_mps = ep_desc->wMaxPacketSize & 0xFF;//USB_MAXPACKETSIZE_MASK;

    USB_LOG_INFO("Close endpoint:0x%x type:%u\r\n", ep_addr, ep_type);

    usbd_ep_close(ep_addr);
}
#endif

/**
 * @brief get specified USB descriptor
 *
 * This function parses the list of installed USB descriptors and attempts
 * to find the specified USB descriptor.
 *
 * @param [in]  type_index Type and index of the descriptor
 * @param [out] data       Descriptor data
 * @param [out] len        Descriptor length
 *
 * @return true if the descriptor was found, false otherwise
 */
__USBIRQ static bool usbd_get_descriptor(uint16_t type_index, uint8_t **data, uint16_t *len)
{
    bool found = false;
    uint8_t *p = NULL;
    uint8_t curIdx = 0U;
    uint8_t type  = GET_DESC_TYPE(type_index);
    uint8_t index = GET_DESC_INDEX(type_index);

    if ((type == USB_DESC_TYPE_STRING) && (index == USB_OSDESC_STRING_DESC_INDEX)) {
        USB_LOG_INFO("read MS OS 2.0 descriptor string\r\n");

        #if (USBD_OS_DESC)
        if (usbd_msosv1_desc) {
            *data = (uint8_t *)usbd_msosv1_desc->string;
            *len = usbd_msosv1_desc->string_len;
            return true;
        }
        #endif // (USBD_OS_DESC)
        return false;
    } else if (type == USB_DESC_TYPE_BINARY_OBJECT_STORE) {
        USB_LOG_INFO("read BOS descriptor string\r\n");

        #if (USBD_OS_DESC)
        if (usbd_bos_desc) {
            *data = usbd_bos_desc->string;
            *len = usbd_bos_desc->string_len;
            return true;
        }
        #endif // (USBD_OS_DESC)
        return false;
    }
    /*
     * Invalid types of descriptors,
     * see USB Spec. Revision 2.0, 9.4.3 Get Descriptor
     */
    else if ((type == USB_DESC_TYPE_INTERFACE) 
            || (type == USB_DESC_TYPE_ENDPOINT) 
            || (type > USB_DESC_TYPE_MAX)) {
        return false;
    }

    curIdx = 0U;
    p = (uint8_t *)usbd_descriptors;

    while (p[DESC_bLength] != 0U) {
        if (p[DESC_bDescriptorType] == type) {
            if (curIdx == index) {
                found = true;
                break;
            }

            curIdx++;
        }

        /* skip to next descriptor */
        p += p[DESC_bLength];
    }

    if (found) {
        /* set data pointer */
        *data = p;

        /* get length from structure */
        if ((type == USB_DESC_TYPE_CONFIGURATION)
            || (type == USB_DESC_TYPE_OTHER_SPEED)) {
            /* configuration descriptor is an
             * exception, length is at offset
             * 2 and 3
             */
            *len = (p[CONF_DESC_wTotalLength]) |
                   (p[CONF_DESC_wTotalLength + 1] << 8);
        } else {
            /* normally length is at offset 0 */
            *len = p[DESC_bLength];
        }
    } else {
        /* nothing found */
        USB_LOG_ERR("descriptor <type:%x,index:%x> not found!\r\n", type, index);
    }

    return found;
}

/**
 * @brief set USB configuration
 *
 * This function configures the device according to the specified configuration
 * index and alternate setting by parsing the installed USB descriptor list.
 * A configuration index of 0 unconfigures the device.
 *
 * @param [in] config_index Configuration index
 * @param [in] alt_setting  Alternate setting number
 *
 * @return true if successfully configured false if error or unconfigured
 */
__USBIRQ static bool usbd_set_configuration(uint8_t config_index, uint8_t alt_setting)
{
    uint8_t cur_config = 0xFF;
    uint8_t cur_alt_setting = 0xFF;
    bool found = false;
    const uint8_t *p = usbd_descriptors;

    if (config_index == 0U) {
        /* TODO: unconfigure device */
        USB_LOG_ERR("Device not configured - invalid configuration\r\n");
        return false;
    }

    /* configure endpoints for this configuration/altsetting */
    while (p[DESC_bLength] != 0U) {
        switch (p[DESC_bDescriptorType]) {
            case USB_DESC_TYPE_CONFIGURATION:
            {
                /* remember current configuration index */
                cur_config = p[CONF_DESC_bConfigurationValue];

                if (cur_config == config_index) {
                    found = true;
                }
            } break;

            case USB_DESC_TYPE_INTERFACE:
            {
                /* remember current alternate setting */
                cur_alt_setting = p[INTF_DESC_bAlternateSetting];
            } break;

            case USB_DESC_TYPE_ENDPOINT:
            {
                if ((cur_config != config_index) || (cur_alt_setting != alt_setting)) {
                    break;
                }
                
                //usbd_set_endpoint((struct usb_endpoint_descriptor *)p);
                struct usb_endpoint_descriptor *ep_desc = (struct usb_endpoint_descriptor *)p;
                USB_LOG_INFO("iconf:%d,endpoint:0x%x(type:%d mps:%d)\r\n",cur_config, 
                    ep_desc->bEndpointAddress, ep_desc->bmAttributes, ep_desc->wMaxPacketSize);
                
                usbd_ep_open(p[EP_DESC_bAddress], p[EP_DESC_bmType], p[EP_DESC_wMPS]);
                USB_LOG_INFO("Open endpoint\r\n");
            } break;

            default:
                break;
        }

        /* skip to next descriptor */
        p += p[DESC_bLength];
    }

    return found;
}

/**
 * @brief set USB interface
 *
 * @param [in] iface Interface index
 * @param [in] alt_setting  Alternate setting number
 *
 * @return true if successfully configured false if error or unconfigured
 */
__USBIRQ static bool usbd_set_interface(uint8_t iface, uint8_t alt_setting)
{
    uint8_t cur_iface = 0xFF;
    uint8_t cur_alt_setting = 0xFF;
    //bool found = false;
    const uint8_t *if_desc = NULL;
    const uint8_t *p = usbd_descriptors;

    USB_LOG_DBG("iface %u alt_setting %u\r\n", iface, alt_setting);

    while (p[DESC_bLength] != 0U) {
        switch (p[DESC_bDescriptorType]) {
            case USB_DESC_TYPE_INTERFACE:
            {
                /* remember current alternate setting */
                cur_iface = p[INTF_DESC_bInterfaceNumber];
                cur_alt_setting = p[INTF_DESC_bAlternateSetting];

                if ((cur_iface == iface) && (cur_alt_setting == alt_setting)) {
                    //found = true;
                    if_desc = (void *)p;
                }

                USB_LOG_DBG("Current iface %u alt setting %u", cur_iface, cur_alt_setting);
            } break;

            case USB_DESC_TYPE_ENDPOINT:
            {
                if (cur_iface == iface) {
                    struct usb_endpoint_descriptor *ep_desc = (struct usb_endpoint_descriptor *)p;
                    USB_LOG_INFO("iface:%d,endpoint:0x%x(type:%d mps:%d)\r\n",iface, 
                        ep_desc->bEndpointAddress, ep_desc->bmAttributes, ep_desc->wMaxPacketSize);

                    if (cur_alt_setting == alt_setting) {
                        //usbd_set_endpoint(ep_desc);
                        usbd_ep_open(p[EP_DESC_bAddress], p[EP_DESC_bmType], p[EP_DESC_wMPS]);
                        USB_LOG_INFO("Open endpoint\r\n");
                    } else {
                        //usbd_reset_endpoint(ep_desc);
                        usbd_ep_close(p[EP_DESC_bAddress]);
                        USB_LOG_INFO("Close endpoint\r\n");
                    }
                }
            } break;

            default:
                break;
        }

        /* skip to next descriptor */
        p += p[DESC_bLength];
    }

    usbd_notify_handler(USBD_EVENT_SET_INTERFACE, (void *)if_desc);

    return if_desc; //return found;
}

__USBIRQ static void *usbd_get_class_handler(uint8_t intf_num)
{
    if (intf_num < usbd_intf_cnt) {
        for (uint8_t i = 0; i < usbd_class_cnt; i++) {
            const usbd_class_t *devclass = &usbd_class_tab[i];
            
            if ((intf_num >= devclass->intf_start) && (intf_num <= devclass->intf_end)) {
                return devclass->class_handler;
            }
        }
    }
    
    return NULL;
}


/*
 * USB Standard-Request Handler
 ****************************************************************************
 */

/**
 * @brief handle a standard device request
 *
 * @param [in]     setup    The setup packet
 * @param [in,out] data     Data buffer
 * @param [in,out] len      Pointer to data length
 *
 * @return true if the request was handled successfully
 */
__USBIRQ static uint8_t usbd_std_device_req_handler(struct usb_setup_packet *setup, uint8_t **data, uint16_t *len)
{
    uint8_t status = USBD_OK;
    uint16_t value = setup->wValue;
    #if (USBD_FEAT_TEST)
    uint16_t index = setup->wIndex;
    #endif

    switch (setup->bRequest) {
        case USB_REQUEST_GET_STATUS:
            USB_LOG_DBG("REQ_GET_STATUS\r\n");
            /* bit 0: self-powered */
            /* bit 1: remote wakeup */
            *data = (uint8_t *)&usbd_env.remote_wakeup;
            *len = 2;
            break;

        case USB_REQUEST_CLEAR_FEATURE:
            USB_LOG_DBG("REQ_CLEAR_FEATURE\r\n");
            #if (USBD_FEAT_TEST)
            /* process for feature */
            usbd_clear_feature(index, value);
            #endif
            if (value == USB_FEATURE_REMOTE_WAKEUP) {
                usbd_env.remote_wakeup = 0;
                usbd_notify_handler(USBD_EVENT_CLR_REMOTE_WAKEUP, NULL);
            }
            break;

        case USB_REQUEST_SET_FEATURE:
            USB_LOG_DBG("REQ_SET_FEATURE\r\n");
            #if (USBD_FEAT_TEST)
            /* process for feature */
            usbd_set_feature(index, value);
            #endif
            if (value == USB_FEATURE_REMOTE_WAKEUP) {
                usbd_env.remote_wakeup = 1;
                usbd_notify_handler(USBD_EVENT_SET_REMOTE_WAKEUP, NULL);
            }
            break;

        case USB_REQUEST_SET_ADDRESS:
            USB_LOG_DBG("REQ_SET_ADDRESS, addr 0x%x\r\n", value);
            usbd_set_address(value);
            break;

        case USB_REQUEST_GET_DESCRIPTOR:
            if (!usbd_get_descriptor(value, data, len)) {
                status = USBD_FAIL;
            }
            USB_LOG_DBG("REQ_GET_DESCRIPTOR(type:%04X,ret:%d,len:%d)\r\n", value, ret,*len);
            break;

        case USB_REQUEST_SET_DESCRIPTOR:
            USB_LOG_DBG("Device req 0x%02x not implemented\r\n", setup->bRequest);
            status = USBD_FAIL;
            break;

        case USB_REQUEST_GET_CONFIGURATION:
            USB_LOG_DBG("REQ_GET_CONFIGURATION\r\n");
            /* indicate if we are configured */
            *data = (uint8_t *)&usbd_env.conf_num;
            *len = 1;
            break;

        case USB_REQUEST_SET_CONFIGURATION:
            value &= 0xFF;
            USB_LOG_DBG("REQ_SET_CONFIGURATION, conf 0x%x\r\n", value);

            if (!usbd_set_configuration(value, 0)) {
                USB_LOG_DBG("USB Set Configuration failed\r\n");
                status = USBD_FAIL;
            } else {        
                /* configuration successful, update current configuration */
                usbd_env.conf_num = value;
                usbd_notify_handler(USBD_EVENT_CONFIGURED, NULL);
            }
            break;

        //case USB_REQUEST_GET_INTERFACE:
        //    break;

        //case USB_REQUEST_SET_INTERFACE:
        //    break;

        default:
            USB_LOG_ERR("Illegal device req 0x%02x\r\n", setup->bRequest);
            status = USBD_FAIL;
            break;
    }

    return status;
}

/**
 * @brief handle a standard interface request
 *
 * @param [in]     setup    The setup packet
 * @param [in,out] data     Data buffer
 * @param [in,out] len      Pointer to data length
 *
 * @return true if the request was handled successfully
 */
__USBIRQ static uint8_t usbd_std_interface_req_handler(struct usb_setup_packet *setup, uint8_t **data, uint16_t *len)
{
    /** The device must be configured to accept standard interface
     * requests and the addressed Interface must be valid.
     */
    usbd_request_handler req_handler;
    uint8_t intf_num = (uint8_t)setup->wIndex;
    
    if (!is_interface_valid(intf_num)) {
        return USBD_FAIL;
    }

    req_handler = usbd_get_class_handler(intf_num);
    if (req_handler && (req_handler(setup, data, len) == USBD_OK)) {
        return USBD_OK;
    }
        
    switch (setup->bRequest) {
        case USB_REQUEST_GET_STATUS:
            /* no bits specified */
            *data = (uint8_t *)&usbd_env.remote_wakeup;
            *len = 2;
            break;

        case USB_REQUEST_CLEAR_FEATURE:
        case USB_REQUEST_SET_FEATURE:
            /* not defined for interface */
            return USBD_FAIL;

        case USB_REQUEST_GET_INTERFACE:
            /** This handler is called for classes that does not support
             * alternate Interfaces so always return 0. Classes that
             * support alternative interfaces handles GET_INTERFACE
             * in custom_handler.
             */
            (*data)[0] = 0;
            *len = 1;
            break;

        case USB_REQUEST_SET_INTERFACE:
            USB_LOG_DBG("REQ_SET_INTERFACE\r\n");
            usbd_set_interface(setup->wIndex, setup->wValue);
            break;

        default:
            USB_LOG_ERR("Illegal interface req 0x%02x\r\n", setup->bRequest);
            return USBD_FAIL;
    }

    return USBD_OK;
}

/**
 * @brief handle a standard endpoint request
 *
 * @param [in]     setup    The setup packet
 * @param [in,out] data     Data buffer
 * @param [in,out] len      Pointer to data length
 *
 * @return true if the request was handled successfully
 */
__USBIRQ static uint8_t usbd_std_endpoint_req_handler(struct usb_setup_packet *setup, uint8_t **data, uint16_t *len)
{
    uint8_t status = USBD_FAIL;
    uint8_t ep = (uint8_t)setup->wIndex;

    /* Check if request addresses valid Endpoint */
    if (!is_ep_valid(ep)) {
        return status;
    }

    switch (setup->bRequest) {
        case USB_REQUEST_GET_STATUS:
        {
            /** This request is valid for Control Endpoints when
             * the device is not yet configured. For other
             * Endpoints the device must be configured.
             * Firstly check if addressed ep is Control Endpoint.
             * If no then the device must be in Configured state
             * to accept the request.
             */
            if (((ep & 0x7f) == 0) || is_device_configured()) {
                /* bit 0 - Endpoint halted or not */
                usbd_ep_is_stalled(ep, (uint8_t *)&usbd_env.remote_wakeup);
                
                *data = (uint8_t *)&usbd_env.remote_wakeup;
                *len = 2;
                status = USBD_OK;
            }
        } break;

        case USB_REQUEST_CLEAR_FEATURE:
        {
            if (setup->wValue == USB_FEATURE_ENDPOINT_HALT) {
                /** This request is valid for Control Endpoints when
                 * the device is not yet configured. For other
                 * Endpoints the device must be configured.
                 * Firstly check if addressed ep is Control Endpoint.
                 * If no then the device must be in Configured state
                 * to accept the request.
                 */
                if (((ep & 0x7f) == 0) || is_device_configured()) {
                    USB_LOG_ERR("ep:%x clear halt\r\n", ep);
                    USB_PRINT_SETUP(setup);

                    //usbd_ep_clear_stall(ep);
                    //usbd_notify_handler(USBD_EVENT_CLR_ENDPOINT_HALT, NULL);
                    usbd_ep_stall(ep, USBD_EVENT_CLR_ENDPOINT_HALT);
                    status = USBD_OK;
                }
            }
            /* only ENDPOINT_HALT defined for endpoints */
        } break;

        case USB_REQUEST_SET_FEATURE:
        {
            if (setup->wValue == USB_FEATURE_ENDPOINT_HALT) {
                /** This request is valid for Control Endpoints when
                 * the device is not yet configured. For other
                 * Endpoints the device must be configured.
                 * Firstly check if addressed ep is Control Endpoint.
                 * If no then the device must be in Configured state
                 * to accept the request.
                 */
                if (((ep & 0x7f) == 0) || is_device_configured()) {
                    /* set HALT by stalling */
                    USB_LOG_ERR("ep:%x set halt\r\n", ep);

                    //usbd_ep_set_stall(ep);
                    //usbd_notify_handler(USBD_EVENT_SET_ENDPOINT_HALT, NULL);
                    usbd_ep_stall(ep, USBD_EVENT_SET_ENDPOINT_HALT);
                    status = USBD_OK;
                }
            }
            /* only ENDPOINT_HALT defined for endpoints */
        } break;

        case USB_REQUEST_SYNCH_FRAME:
        {
            /* For Synch Frame request the device must be configured */
            if (is_device_configured()) {
                /* Not supported, return false anyway */
                USB_LOG_DBG("ep req 0x%02x not implemented\r\n", setup->bRequest);
            }
        } break;

        default:
        {
            USB_LOG_ERR("Illegal ep req 0x%02x\r\n", setup->bRequest);
        } break;
    }

    return status;
}


/*
 * USB SETUP Request Handler - EP0
 ****************************************************************************
 */

/**
 * @brief handle a request by calling one of the installed request handlers
 *
 * Local function to handle a request by calling one of the installed request
 * handlers. In case of data going from host to device, the data is at *ppbData.
 * In case of data going from device to host, the handler can either choose to
 * write its data at *ppbData or update the data pointer.
 *
 * @param [in]     setup The setup packet
 * @param [in,out] data  Data buffer
 * @param [in,out] len   Pointer to data length
 *
 * @return true if the request was handles successfully
 */
__USBIRQ uint8_t usbd_setup_request_handler(struct usb_setup_packet *setup, uint8_t **data, uint16_t *len)
{
    usbd_request_handler req_handler;
    uint8_t status = USBD_FAIL;
    uint8_t req_type  = setup->bmRequestType & USB_REQUEST_TYPE_MASK;
    uint8_t recipient = setup->bmRequestType & USB_REQUEST_RECIP_MASK;
    
//    if (recipient == USB_REQUEST_RECIP_INTERFACE) {
//        /* 
//         * usbd_custom_request_handler()
//         * handler for special requests, this handler is called first
//         */
//        req_handler = usbd_get_handler(CUSTOM_HANDLER, (setup->wIndex & 0xFF));
//        
//        if (req_handler && (req_handler(setup, data, len) == 0)) {
//            return true;
//        }
//    }

    switch (req_type) {
        case USB_REQUEST_STANDARD:
        {
            /*
             * usbd_standard_request_handler()
             * default handler for standard ('chapter 9') requests
             */
            if (recipient == USB_REQUEST_RECIP_DEVICE) {
                status = usbd_std_device_req_handler(setup, data, len);
            }
            else if (recipient == USB_REQUEST_RECIP_INTERFACE) {
                status = usbd_std_interface_req_handler(setup, data, len);
            }
            else if (recipient == USB_REQUEST_RECIP_ENDPOINT) {
                status = usbd_std_endpoint_req_handler(setup, data, len);
            }
        } break;
        
        case USB_REQUEST_CLASS:
        {
            /*
             * usbd_class_handler()
             * handler for class requests
             */
            uint8_t intf_num = 0xFF;
            
            if (recipient == USB_REQUEST_RECIP_INTERFACE) {
                intf_num = (setup->wIndex & 0xFF);
            } else if (recipient == USB_REQUEST_RECIP_ENDPOINT) {
                intf_num = ((setup->wIndex >> 8) & 0xFF);
            }
            
            req_handler = usbd_get_class_handler(intf_num);
            
            if (req_handler) {
                status = req_handler(setup, data, len);
            }
        } break;
        
        case USB_REQUEST_VENDOR:
        {
            /*
             * usbd_vendor_request_handler()
             * handler for vendor requests
             */
            // if (recipient != USB_REQUEST_RECIP_DEVICE) {
            //     break;
            // }
            
            #if (USBD_OS_DESC)
            if ((usbd_msosv1_desc) && (setup->bRequest == usbd_msosv1_desc->vendor_code)) {
                if (setup->wIndex == 0x04) {
                    USB_LOG_INFO("get Compat ID\r\n");
                    *data = (uint8_t *)usbd_msosv1_desc->compat_id;
                    *len = usbd_msosv1_desc->compat_id_len;
                    status = true;
                } else if (setup->wIndex == 0x05) {
                    USB_LOG_INFO("get Compat id properties\r\n");
                    *data = (uint8_t *)usbd_msosv1_desc->comp_id_property;
                    *len = usbd_msosv1_desc->comp_id_property_len;
                    status = true;
                } else {
                    USB_LOG_ERR("unknown vendor code\r\n");
                }
                break;
            } else if ((usbd_msosv2_desc) && (setup->bRequest == usbd_msosv2_desc->vendor_code)) {
                if (setup->wIndex == WINUSB_REQUEST_GET_DESCRIPTOR_SET) {
                    USB_LOG_INFO("GET MS OS 2.0 Descriptor\r\n");
                    *data = (uint8_t *)usbd_msosv2_desc->compat_id;
                    *len = usbd_msosv2_desc->compat_id_len;
                    status = true;
                } else {
                    USB_LOG_ERR("unknown vendor code\r\n");
                }
                break;
            }
            #endif // (USBD_OS_DESC)
            
            status = usbd_vendor_handler(setup, data, len);
        } break;
        
        default:
            break;
    }
    
    if (status != USBD_OK) {
        USB_LOG_DBG("Req Err(bReq:0x%02x, wIndex:0x%04x, status:%d)\r\n", setup->bRequest, setup->wIndex, status);
        USB_PRINT_SETUP(setup);
    }
    
    return status;
}


/*
 * USB Endpoint(1~NUM) Callback Handler
 ****************************************************************************
 */

__USBIRQ void usbd_ep_isr_handler(uint8_t ep_addr)
{
    const usbd_ep_t *endpoint;
    
    for (uint8_t i = 0; i < usbd_ep_cnt; i++) {
        endpoint = &usbd_ep_tab[i];
        
        if ((endpoint->ep_addr == ep_addr) && (endpoint->ep_cb)) {
            endpoint->ep_cb(ep_addr);
        }
    }
}


/*
 * USB Event Notify / Vendor Request Handler
 ****************************************************************************
 */

__WEAK __USBIRQ void usbd_notify_handler(uint8_t event, void *arg)
{
    USB_LOG_DBG("USB event: %d\r\n", event);
}

__WEAK __USBIRQ uint8_t usbd_vendor_handler(struct usb_setup_packet *setup, uint8_t **data, uint16_t *len)
{
    USB_LOG_DBG("USB vendor: %d\r\n", setup->bRequest);
    return USBD_FAIL;
}


/*
 * USB Device APIs
 ****************************************************************************
 */

uint8_t usbd_is_configured(void)
{
    return usbd_env.conf_num;
}

void usbd_register(const uint8_t *desc, const usbd_config_t *conf)
{
    usbd_descriptors = desc;
    usbd_configurations = conf;
    usbd_configurecount = 1;
}

#if 0
void usbd_desc_register(const uint8_t *desc)
{
    usbd_descriptors = desc;
}

void usbd_conf_register(const usbd_config_t *conf)
{
    usbd_configurations = conf;
    usbd_configurecount = 1;
}

void usbd_multi_register(const uint8_t *desc, const usbd_config_t *conf, uint8_t conf_cnt)
{
    usbd_descriptors = desc;
    usbd_configurations = conf;
    usbd_configurecount = conf_cnt;
}
#endif

#if (USBD_OS_DESC)
/* Register MS OS Descriptors version 1 */
void usbd_msosv1_desc_register(struct usb_msosv1_descriptor *desc)
{
    usbd_msosv1_desc = desc;
}

/* Register MS OS Descriptors version 2 */
void usbd_msosv2_desc_register(struct usb_msosv2_descriptor *desc)
{
    usbd_msosv2_desc = desc;
}

void usbd_bos_desc_register(struct usb_bos_descriptor *desc)
{
    usbd_bos_desc = desc;
}
#endif // (USBD_OS_DESC)

#if (USBD_FEAT_TEST)
__WEAK void usbd_set_feature(uint16_t index, uint16_t value)
{
}

__WEAK void usbd_clear_feature(uint16_t index, uint16_t value)
{
}
#endif
