/**
 * @file usbd_int.h
 *
 * @brief Internal Header file of USB Device.
 *
 */

#ifndef _USBD_INT_H_
#define _USBD_INT_H_

#include <stdint.h>
#include "usbd.h"


/*
 * CONFIGURE
 ****************************************************************************
 */

#ifndef __INLINE__
#define __INLINE__          static __forceinline
#endif

/* Define maximum packet size for endpoint 0 */
#ifndef USB_EP0_MPS
#define USB_EP0_MPS         (64)
#endif
#ifndef USB_EP_NUM
#define USB_EP_NUM          (5) // 0,1,...,NUM-1
#endif

#ifndef USB_REQ_DATA_LEN
#define USB_REQ_DATA_LEN    (256)
#endif

// Marco for feature enable
#define USBD_OS_DESC        (0)
#define USBD_FEAT_TEST      (0)


/*
 * DEFINES
 ****************************************************************************************
 */

enum ep0_state
{
    EP0_STATE_SETUP,
    EP0_STATE_IN_DATA,
    EP0_STATE_IN_STATUS,
    EP0_STATE_OUT_DATA,
    EP0_STATE_OUT_STATUS,
    EP0_STATE_STALL,
};

struct usbd_hal_tag {
    /** Setup packet */
    union {
        struct usb_setup_packet setup;
        uint8_t setup_byte[8];
    };
    /** Pointer to data buffer */
    uint8_t *ep0_data_ptr;
    /** Remaining bytes in buffer */
    uint16_t ep0_data_res;
    /** Total length of control transfer */
    uint16_t ep0_data_len;

    /** Stalled bits0~7:IN, 8~15:OUT*/
    uint16_t ep_stall;
    
    volatile uint8_t dev_addr;
    volatile uint8_t ep0_state;
    uint8_t req_data[USB_REQ_DATA_LEN];
};

struct usbd_env_tag {
    #if (USBD_OS_DESC)
    const struct usb_bos_descriptor    *bos_desc;
    const struct usb_msosv1_descriptor *msv1_desc;
    const struct usb_msosv2_descriptor *msv2_desc;
    #endif

    /** Pointer to registered descriptors */
    const uint8_t *descriptors;
    
    const usbd_config_t *configurations;
    
    uint8_t conf_cnt;
    
    /** Currently selected configuration */
    uint8_t conf_num;
    
    /** Remote wakeup feature status */
    uint16_t remote_wakeup;
};

/*
 * GLOBAL VARIABLES
 ****************************************************************************
 */

extern struct usbd_hal_tag usbd_hal;

extern struct usbd_env_tag usbd_env;


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

__INLINE__ void usbd_hal_reset(void)
{
    usbd_hal.ep_stall = 0;
    usbd_hal.dev_addr = 0;
    usbd_hal.ep0_state = EP0_STATE_SETUP;
}

__INLINE__ void usbd_env_reset(void)
{
    usbd_env.conf_num = 0;
    usbd_env.remote_wakeup = 0;
}

/**
 * @brief Set USB device address
 *
 * @param[in] addr Device address
 *
 * @return 0 on success, errno code on fail.
 */
__INLINE__ void usbd_set_address(uint8_t addr)
{
    //if (addr == 0) {
    //    USB->FADDR = 0;
    //}

    usbd_hal.dev_addr = addr;
}

/**
 * @brief configure and enable endpoint.
 *
 * This function sets endpoint configuration according to one specified in USB.
 * endpoint descriptor and then enables it for data transfers.
 *
 * @param [in]  ep_desc Endpoint descriptor byte array.
 *
 * @return true if successfully configured and enabled.
 */
uint8_t usbd_ep_open(uint8_t ep, uint8_t type, uint8_t mps);

/**
 * @brief Disable the selected endpoint
 *
 * Function to disable the selected endpoint. Upon success interrupts are
 * disabled for the corresponding endpoint and the endpoint is no longer able
 * for transmitting/receiving data.
 *
 * @param[in] ep Endpoint address corresponding to the one
 *               listed in the device configuration table
 *
 * @return 0 on success, errno code on fail.
 */
void usbd_ep_close(uint8_t ep);

#if (0)
/**
 * @brief Set stall condition for the selected endpoint
 *
 * @param[in] ep Endpoint address corresponding to the one
 *               listed in the device configuration table
 *
 * @return 0 on success, errno code on fail.
 */
void usbd_ep_set_stall(uint8_t ep);

/**
 * @brief Clear stall condition for the selected endpoint
 *
 * @param[in] ep Endpoint address corresponding to the one
 *               listed in the device configuration table
 *
 * @return 0 on success, errno code on fail.
 */
void usbd_ep_clear_stall(uint8_t ep);
#else
void usbd_ep_stall(uint8_t ep, uint8_t en);
#endif

/**
 * @brief Check if the selected endpoint is stalled
 *
 * @param[in]  ep       Endpoint address corresponding to the one
 *                      listed in the device configuration table
 * @param[out] stalled  Endpoint stall status
 *
 * @return 0 on success, errno code on fail.
 */
uint8_t usbd_ep_is_stalled(uint8_t ep, uint8_t *stalled);

/*
 * USB Endpoint(1~NUM) Callback Handler
 ****************************************************************************
 */
__USBIRQ void usbd_ep_isr_handler(uint8_t ep_addr);

/*
 * USB Event Notify Handler
 ****************************************************************************
 */
__USBIRQ void usbd_notify_handler(uint8_t event, void *arg);

/*
 * USB SETUP Request Handler - EP0
 ****************************************************************************
 */
__USBIRQ uint8_t usbd_setup_request_handler(struct usb_setup_packet *setup, uint8_t **data, uint16_t *len);

#endif // _USBD_INT_H_
