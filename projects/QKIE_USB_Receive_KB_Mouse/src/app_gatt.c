/**
 ****************************************************************************************
 *
 * @file app_gatt.c
 *
 * @brief App SubTask of GATT Message - Example
 *
 * < If want to modify it, recommend to copy the file to 'user porject'/src >
 ****************************************************************************************
 */

#if (GATT_CLI)
#include "app_user.h"
#include "drvs.h"
#include "app.h"
#include "gatt.h"
#include "gatt_api.h"
#include "usbd_hid.h"

#if (DBG_GATT)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

/*
 * DEFINES
 ****************************************************************************************
 */
#if (CFG_USB)
#ifndef NB_PKT_MS_MAX
#define NB_PKT_MS_MAX 32 // 2**n
#endif

#ifndef NB_PKT_KB_MAX
#define NB_PKT_KB_MAX 32 // 2**n
#endif
#ifndef NB_PKT_CONSUMER_MAX
#define NB_PKT_CONSUMER_MAX 32 // 2**n
#endif
uint8_t pkt_kb[NB_PKT_KB_MAX][PKT_KB_LEN];
uint8_t pkt_consumer[NB_PKT_CONSUMER_MAX][PKT_CONSUMER_LEN];
uint8_t pkt_mouse[NB_PKT_MS_MAX][PKT_MS_LEN];

volatile uint16_t pkt_ms_sidx, pkt_ms_eidx;
volatile uint16_t pkt_kb_sidx, pkt_kb_eidx;
volatile uint16_t pkt_consumer_sidx, pkt_consumer_eidx;

uint8_t *get_mouse_pkt(void)
{
    uint8_t *pkt = NULL;
    
    if (pkt_ms_eidx != pkt_ms_sidx)
    {
        pkt = pkt_mouse[pkt_ms_eidx];
//        g_empty[0] = pkt_mouse[pkt_ms_eidx][0];
        pkt_ms_eidx = (pkt_ms_eidx + 1)  % NB_PKT_MS_MAX;
    }

    return pkt;
}

uint8_t *get_kb_pkt(void)
{
    uint8_t *pkt = NULL;
    
    if (pkt_kb_eidx != pkt_kb_sidx)
    {
        pkt = pkt_kb[pkt_kb_eidx];
        pkt_kb_eidx = (pkt_kb_eidx + 1)  % NB_PKT_KB_MAX;
    }

    return pkt;
}

uint8_t *get_consumer_pkt(void)
{
    uint8_t *pkt = NULL;
    
    if (pkt_consumer_eidx != pkt_consumer_sidx)
    {
        pkt = pkt_consumer[pkt_consumer_eidx];
        pkt_consumer_eidx = (pkt_consumer_eidx + 1)  % NB_PKT_CONSUMER_MAX;
    }

    return pkt;
}
#endif

/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */

APP_MSG_HANDLER(gatt_cmp_evt)
{
    DEBUG("Cmp_evt(op:%d,sta:0x%02x)", param->operation, param->status);
}

APP_MSG_HANDLER(gatt_mtu_changed_ind)
{
    DEBUG("mtu_chg:%d,seq:%d", param->mtu, param->seq_num);
}

APP_MSG_HANDLER(gatt_disc_svc_ind)
{
    DEBUG("disc_svc(shdl:0x%X,ehdl:0x%X,ulen:%d)", param->start_hdl, param->end_hdl, param->uuid_len);
    debugHex(param->uuid, param->uuid_len);
}

APP_MSG_HANDLER(gatt_disc_svc_incl_ind)
{
    DEBUG("disc_incl(ahdl:0x%X,shdl:0x%X,ehdl:0x%X,ulen:%d)", param->attr_hdl, param->start_hdl, param->end_hdl, param->uuid_len);
    debugHex(param->uuid, param->uuid_len);
}

APP_MSG_HANDLER(gatt_disc_char_ind)
{
    DEBUG("disc_char(ahdl:0x%X,phdl:0x%X,prop:0x%X,ulen:%d)", param->attr_hdl, param->pointer_hdl, param->prop, param->uuid_len);
    debugHex(param->uuid, param->uuid_len);
}

APP_MSG_HANDLER(gatt_disc_char_desc_ind)
{
    DEBUG("disc_desc(ahdl:0x%X,ulen:%d)", param->attr_hdl, param->uuid_len);
    debugHex(param->uuid, param->uuid_len);
}

APP_MSG_HANDLER(gatt_read_ind)
{
    DEBUG("Read_ind(hdl:0x%02x,oft:%d,len:%d)", param->handle, param->offset, param->length);
    debugHex(param->value, param->length);
}

APP_MSG_HANDLER(gatt_event_ind)
{
    uint8_t conidx = TASK_IDX(src_id);


#if (CFG_USB)
    if(g_cid_kb==conidx)
    {
        DEBUG("Evt_ind(typ:%d,hdl:0x%02x,len:%d)", param->type, param->handle, param->length);
        debugHex(param->value, param->length);
        switch (param->handle)
        {
                    
            case GATT_KB_HDL:
            {
                g_cid_kb = conidx;
                DEBUG("KB(kid:%d, cid:%d)", g_cid_kb, conidx);
                
            memcpy(pkt_kb[pkt_kb_sidx], param->value, USB_RPT_LEN_KB);
            pkt_kb_sidx = (pkt_kb_sidx + 1) % NB_PKT_KB_MAX;

                usbd_kb_report();
            } break;

            case GATT_MEDIA_HDL:
            {
                g_cid_kb = conidx;
                DEBUG("MEDIA(kid:%d, cid:%d)", g_cid_kb, conidx);

            memcpy(&pkt_consumer[pkt_consumer_sidx][2], param->value, BLE_RPT_LEN_MEDIA);
            pkt_consumer[pkt_consumer_sidx][1] = RPT_ID_MEDIA;
            pkt_consumer[pkt_consumer_sidx][0] = USB_RPT_LEN_MEDIA;
            pkt_consumer_sidx = (pkt_consumer_sidx + 1) % NB_PKT_CONSUMER_MAX;

            usbd_consumer_report();
        } break;

            case GATT_SYS_HDL:
            {
                g_cid_kb = conidx;
                DEBUG("SYS(kid:%d, cid:%d)", g_cid_kb, conidx);

            #if (BLE_RPT_LEN_SYSTEM > 1)
            memcpy(&pkt_consumer[pkt_consumer_sidx][2], param->value, BLE_RPT_LEN_SYSTEM);
            #else
            pkt_consumer[pkt_consumer_sidx][2] = param->value[0];
            #endif
            pkt_consumer[pkt_consumer_sidx][1] = RPT_ID_SYSTEM;
            pkt_consumer[pkt_consumer_sidx][0] = USB_RPT_LEN_SYSTEM;
            pkt_consumer_sidx = (pkt_consumer_sidx + 1) % NB_PKT_CONSUMER_MAX;

            usbd_consumer_report();
        } break;

            default:
            {
            } break;
        }
    }
    else 
    {
        if(param->handle==GATT_MOUSE_HDL)
        {
            //DEBUG("MS(kid:%d, cid:%d)", g_cid_kb, conidx);

            memcpy(pkt_mouse[pkt_ms_sidx], param->value, USB_RPT_LEN_MOUSE);
            pkt_ms_sidx = (pkt_ms_sidx + 1) % NB_PKT_MS_MAX;

            usbd_mouse_report();
        }
    }
       

#endif
}

APP_MSG_HANDLER(gatt_event_req_ind)
{
    uint8_t conidx = TASK_IDX(src_id);

    DEBUG("Evt_req_ind(typ:%d,hdl:0x%02x,len:%d)", param->type, param->handle, param->length);
    debugHex(param->value, param->length);

    gatt_evt_cfm(conidx, param->handle);
}

/**
 ****************************************************************************************
 * @brief SubTask Handler of GATT Message.
 ****************************************************************************************
 */
APP_SUBTASK_HANDLER(gatt_msg)
{
    switch (msgid)
    {
        case GATT_CMP_EVT:
        {
            APP_MSG_FUNCTION(gatt_cmp_evt);
        } break;

        case GATT_MTU_CHANGED_IND:
        {
            APP_MSG_FUNCTION(gatt_mtu_changed_ind);
        } break;

        case GATT_DISC_SVC_IND:
        {
            APP_MSG_FUNCTION(gatt_disc_svc_ind);
        } break;

        case GATT_DISC_SVC_INCL_IND:
        {
            APP_MSG_FUNCTION(gatt_disc_svc_incl_ind);
        } break;

        case GATT_DISC_CHAR_IND:
        {
            APP_MSG_FUNCTION(gatt_disc_char_ind);
        } break;

        case GATT_DISC_CHAR_DESC_IND:
        {
            APP_MSG_FUNCTION(gatt_disc_char_desc_ind);
        } break;

        case GATT_READ_IND:
        {
            APP_MSG_FUNCTION(gatt_read_ind);
        } break;

        case GATT_EVENT_IND:
        {
            APP_MSG_FUNCTION(gatt_event_ind);
        } break;

        case GATT_EVENT_REQ_IND:
        {
            APP_MSG_FUNCTION(gatt_event_req_ind);
        } break;

        default:
        {
            DEBUG("Unknow MsgId:0x%X", msgid);
        } break;
    }

    return (MSG_STATUS_FREE);
}

#endif //(GATT_CLI)
