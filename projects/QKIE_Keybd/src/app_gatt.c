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

#include "app.h"
#include "gatt.h"
#include "gatt_api.h"
#include "gapc_api.h"
#include "hidkey.h"
#include "keyboard.h"
#include "drvs.h"
#include "user_api.h"
/*
 * DEFINES
 ****************************************************************************************
 */
 #if (DBG_GATT)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif
 
 enum os_judge_step
{
    STEP_NONE,
    STEP_INIT,
    STEP_DONE,
    STEP_NEXT,
};

struct os_judge_env
{
    uint8_t os_type;
    uint8_t step;
    uint8_t mtu;
    uint8_t step_tmp;
} os_judge_env;

const uint8_t apple_vendor_uuid[] = {0x66, 0x43, 0xAE, 0x10, 0x79, 0x48, 0xF8, 0xA5, 0x91, 0x45, 0xB4, 0xBB, 0x78, 0x1E, 0x61, 0xD0};


/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */
void os_judge_init(void)
{
    os_judge_env.os_type  = ANDROID;
    os_judge_env.step     = STEP_INIT;
    os_judge_env.step_tmp = STEP_NONE;

    memset(os_mode_table, 0xFF, 4);
}

void os_judge_env_clr(void)
{
    os_judge_env.os_type  = ANDROID;
    os_judge_env.mtu      = 0;
    os_judge_env.step     = STEP_NONE;
    os_judge_env.step_tmp = STEP_NONE;
}

void os_judge_proc(void)
{ 
    if (os_judge_env.step == os_judge_env.step_tmp)
        return;
    
    //DEBUG("1step:%d, os_judeg:%d, os:%d", os_judge_env.step, os_judge_env.os_type, key_env.sys);
    
    os_judge_env.step_tmp  = os_judge_env.step;
        
    switch (os_judge_env.step)
    {
        case STEP_INIT:
        {
            os_judge_env.step = STEP_DONE;
            DEBUG("STEP_INIT");
            if (os_judge_env.mtu)
            {
                os_judge_env.os_type = WINDOWS;
                os_judge_env.step    = STEP_NEXT;
                os_judge_env.mtu     = 0;
                DEBUG("mtu1");
                gatt_disc(app_env.curidx, GATT_DISC_BY_UUID_SVC, 0x0001, 0xFFFF, 0x10, apple_vendor_uuid);
            }
        } break;

        case STEP_DONE:
        {
            os_judge_env.step = STEP_NONE;
            
            //快连连接默认系统
            if (memcmp(read_Master_Addr,masterDongle_Addr, 6)==0)  
            {
                key_env.sys = WINDOWS;
				DEBUG("master");
            }
            else
            {
				uint8_t now_channle = (channle_Select&0xff);
				key_env.sys       	= os_judge_env.os_type;
                fshc_read(OS_MODE_FLASH_ADDR, (uint32_t *)&os_mode_table, 1, FSH_CMD_RD);
     
                if (os_mode_table[now_channle] != key_env.sys)
                {
                    os_mode_table[now_channle] = key_env.sys;
//					GLOBAL_INT_DISABLE();
//                    fshc_erase(OS_MODE_FLASH_ADDR, FSH_CMD_ER_PAGE);
//                    // store to flash
//					fshc_write(OS_MODE_FLASH_ADDR, (uint32_t *)&os_mode_table, 64, FSH_CMD_WR);       
//					GLOBAL_INT_RESTORE();
                    flash_erase_write(OS_MODE_FLASH_ADDR, (uint32_t *)&os_mode_table);
                }
				DEBUG("key_env.sys%d\r\n",key_env.sys);
            }
        } break;
        
        // nothing
        case STEP_NEXT:
        default :
        {
        } break;
    }
    
    //DEBUG("2step:%d, os_judeg:%d, os:%d", os_judge_env.step, os_judge_env.os_type, key_env.sys);
}

uint8_t os_judge_get(void)
{
    return os_judge_env.os_type;;
}

APP_MSG_HANDLER(gatt_cmp_evt)
{
    DEBUG("Cmp_evt(op:%d,sta:0x%02x)", param->operation, param->status);
	
	if ((GATT_DISC_BY_UUID_SVC == param->operation) && (os_judge_env.os_type == WINDOWS))
    {
        os_judge_env.step = STEP_DONE;
    }
}

APP_MSG_HANDLER(gatt_mtu_changed_ind)
{
    DEBUG("mtu_chg:%d,seq:%d", param->mtu, param->seq_num);
	gapc_update_dle(app_env.curidx,LE_MAX_OCTETS,LE_MAX_TIME);
	os_judge_env.mtu = 1;
}

APP_MSG_HANDLER(gatt_disc_svc_ind)
{
    DEBUG("disc_svc(shdl:0x%X,ehdl:0x%X,ulen:%d)", param->start_hdl, param->end_hdl, param->uuid_len);
    debugHex(param->uuid, param->uuid_len);
	
	if (0 == memcmp(apple_vendor_uuid, param->uuid, param->uuid_len))
    {
        os_judge_env.os_type = SYS_IOS;
        
        uint16_t model_uuid = ATT_CHAR_MODEL_NB;
//        gatt_disc(app_env.curidx, GATT_DISC_BY_UUID_CHAR, 0x0001, 0xFFFF, 2, (uint8_t *)&model_uuid);
        gatt_read_by_uuid(app_env.curidx, 2, (uint8_t *)&model_uuid, 0x0001, 0xFFFF);
    }
	//os_judge_env.step = STEP_DONE;
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
	// 0x2A24
    if (ATT_CHAR_MODEL_NB == read16p(param->uuid))
    {
        DEBUG("Find Model NB");
        gatt_read(app_env.curidx, param->pointer_hdl, 4);
    }
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
	if (memcmp(param->value, "iPad", 4) == 0)
    {
        DEBUG("iPad");
    }
    else if(memcmp(param->value, "iPho", 4) == 0)
    {
        DEBUG("iPhone");
    }
    else if(memcmp(param->value, "Mac", 3) == 0)
    {
        os_judge_env.os_type = SYS_MAC;
        DEBUG("Mac");
    }
    
    os_judge_env.step = STEP_DONE;
}

APP_MSG_HANDLER(gatt_event_ind)
{
    DEBUG("Evt_ind(typ:%d,hdl:0x%02x,len:%d)", param->type, param->handle, param->length);
    debugHex(param->value, param->length);
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
