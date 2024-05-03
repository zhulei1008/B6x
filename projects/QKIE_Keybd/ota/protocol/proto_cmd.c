#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "proto.h"
#include "ota_itf.h"
#include "sftmr.h"
#include "app.h"
#include "prf_sesc.h"
#include "gatt.h"


#if (OTA_HOST)
//#define PKT_ALLOC(payl_len)  uint8_t buff[PKT_HDR_SIZE + payl_len]; pkt_t *pkt = (pkt_t *)buff;
//#define PKT_PARAM(p_struct)  p_struct *param = (p_struct *)pkt->payl;
uint8_t  pt_code;
uint32_t pt_time;

static __inline void pt_fill_cmd(pkt_t *pkt, uint8_t cmd, uint16_t len)
{
    pkt->type = PT_TYPE_CMD;
    pkt->code = cmd;
    pkt->len  = len;
}

static __inline void pt_fill_crc(pkt_t *pkt)
{
    uint16_t len = pkt->len - 1;

    pkt->payl[len] = pkt_crc8(pkt->payl, len);
}

void pt_send_cmd(pkt_t *pkt)
{
	
	sesc_rxd_write(app_env.curidx, GATT_WRITE_NO_RESPONSE, (pkt->len + PKT_HDR_SIZE), (uint8_t *)pkt);
	
	//Retransmission
	if (!retran.cnt)
		memcpy(&retran.buff, (uint8_t *)pkt, (pkt->len + PKT_HDR_SIZE));
	
    pt_code = pkt->code;
    pt_time = currTickCnt();
}

void pt_cmd_noargs(uint8_t cmd)
{
    PKT_ALLOC(PLEN_CMD_NOARGS);
    pt_fill_cmd(pkt, cmd, PLEN_CMD_NOARGS);

    pt_send_cmd(pkt);
}

void pt_cmd_simple(uint8_t cmd, uint8_t byte)
{
    PKT_ALLOC(PLEN_CMD_SIMPLE);
    pt_fill_cmd(pkt, cmd, PLEN_CMD_SIMPLE);

    pkt->payl[0] = byte;
    pt_send_cmd(pkt);
}

/* Boot: Load Sram */
void pt_cmd_srd(uint32_t addr)
{
    PKT_ALLOC(PLEN_CMD_SRD);
    pt_fill_cmd(pkt, PT_CMD_SRD, PLEN_CMD_SRD);

    PKT_PARAM(struct pt_cmd_srd);
    param->addr = addr;

    pt_fill_crc(pkt);
    pt_send_cmd(pkt);
}

void pt_cmd_patch(uint32_t addr)
{
    PKT_ALLOC(PLEN_CMD_PATCH);
    pt_fill_cmd(pkt, PT_CMD_PATCH, PLEN_CMD_PATCH);

    PKT_PARAM(struct pt_cmd_patch);
    param->addr = addr;

    pt_fill_crc(pkt);
    pt_send_cmd(pkt);
}

void pt_cmd_jump(uint32_t addr)
{
    PKT_ALLOC(PLEN_CMD_JUMP);
    pt_fill_cmd(pkt, PT_CMD_JUMP, PLEN_CMD_JUMP);

    PKT_PARAM(struct pt_cmd_jump);
    param->addr = addr;

    pt_fill_crc(pkt);
    pt_send_cmd(pkt);
}

void pt_cmd_swr(uint32_t addr, uint32_t value)
{
    PKT_ALLOC(PLEN_CMD_SWR);
    pt_fill_cmd(pkt, PT_CMD_SWR, PLEN_CMD_SWR);

    PKT_PARAM(struct pt_cmd_swr);
    param->addr  = addr;
    param->value = value;

    pt_fill_crc(pkt);
    pt_send_cmd(pkt);
}

void pt_cmd_crd(uint32_t addr, uint16_t len)
{
    PKT_ALLOC(PLEN_CMD_CRD);
    pt_fill_cmd(pkt, PT_CMD_CRD, PLEN_CMD_CRD);

    PKT_PARAM(struct pt_cmd_crd);
    param->addr = addr;
    param->len  = len;

    pt_fill_crc(pkt);
    pt_send_cmd(pkt);
}

void pt_cmd_wr_hd(uint32_t addr, uint16_t len)
{
    PKT_ALLOC(PLEN_CMD_WR_HD);
    pt_fill_cmd(pkt, PT_CMD_WR_HD, PLEN_CMD_WR_HD);

    PKT_PARAM(struct pt_cmd_wr_hd);
    param->addr = addr;
    param->len  = len;

    pt_fill_crc(pkt);
    pt_send_cmd(pkt);
}

void pt_cmd_cwr(uint32_t addr, uint8_t *data, uint16_t len)
{
    PKT_ALLOC(PLEN_CMD_CWR);
    pt_fill_cmd(pkt, PT_CMD_CWR, len + 5);

    PKT_PARAM(struct pt_cmd_cwr);
    param->addr = addr;
    memcpy(param->data, data, len);

    pt_fill_crc(pkt);
    pt_send_cmd(pkt);
}

void pt_cmd_baud(uint32_t baud)
{
    PKT_ALLOC(PLEN_CMD_BAUD);
    pt_fill_cmd(pkt, PT_CMD_BAUD, PLEN_CMD_BAUD);

    PKT_PARAM(struct pt_cmd_baud);
    param->baud = baud;

    pt_fill_crc(pkt);
    pt_send_cmd(pkt);
}

/* Proto: Burn Firm */
void pt_cmd_er_firm(uint8_t mode, uint8_t mcnt, struct er_map *maps)
{
    PKT_ALLOC(PLEN_CMD_ER_FSH);
    pt_fill_cmd(pkt, PT_CMD_ER_FIRM, (mcnt * sizeof(struct er_map) + 3));

    PKT_PARAM(struct pt_cmd_er_fsh);
    param->mode = mode;
    param->mcnt = mcnt;
    if (mcnt > 0)
    {
        memcpy(param->maps, maps, (mcnt * sizeof(struct er_map)));
    }

    pt_fill_crc(pkt);
    pt_send_cmd(pkt);
}

void pt_cmd_wr_firm(uint16_t pgidx, uint16_t len, const uint8_t *data)
{
    PKT_ALLOC(PLEN_CMD_WR_FSH);
    pt_fill_cmd(pkt, PT_CMD_WR_FIRM, len + 3);

    PKT_PARAM(struct pt_cmd_wr_fsh);
    param->pgidx = pgidx;
    memcpy(param->data, data, len);

    pt_fill_crc(pkt);
    pt_send_cmd(pkt);
}

void pt_cmd_mf_firm(uint32_t addr, uint16_t len, const uint8_t *data)
{
    PKT_ALLOC(PLEN_CMD_MF_FSH);
    pt_fill_cmd(pkt, PT_CMD_MF_FIRM, len + 5);

    PKT_PARAM(struct pt_cmd_mf_fsh);
    param->addr = addr;
    memcpy(param->data, data, len);

    pt_fill_crc(pkt);
    pt_send_cmd(pkt);
}

/* Proto: Burn Flash */
void pt_cmd_er_flash(uint8_t mode, uint8_t mcnt, struct er_map *maps)
{
    PKT_ALLOC(PLEN_CMD_ER_FSH);
    pt_fill_cmd(pkt, PT_CMD_ER_FLASH, (mcnt * sizeof(struct er_map) + 3));

    PKT_PARAM(struct pt_cmd_er_fsh);
    param->mode = mode;
    param->mcnt = mcnt;
    if (mcnt > 0)
    {
        memcpy(param->maps, maps, (mcnt * sizeof(struct er_map)));
    }

    pt_fill_crc(pkt);
    pt_send_cmd(pkt);
}

void pt_cmd_rd_flash(uint16_t pgidx, uint16_t len)
{
    PKT_ALLOC(PLEN_CMD_RD_FSH);
    pt_fill_cmd(pkt, PT_CMD_RD_FLASH, PLEN_CMD_RD_FSH);

    PKT_PARAM(struct pt_cmd_rd_fsh);
    param->pgidx  = pgidx;
    param->length = len;

    pt_fill_crc(pkt);
    pt_send_cmd(pkt);
}

void pt_cmd_wr_flash(uint16_t pgidx, uint16_t len, const uint8_t *data)
{
    PKT_ALLOC(PLEN_CMD_WR_FSH);
    pt_fill_cmd(pkt, PT_CMD_WR_FLASH, len + 3);

    PKT_PARAM(struct pt_cmd_wr_fsh);
    param->pgidx = pgidx;
    memcpy(param->data, data, len);

    pt_fill_crc(pkt);
    pt_send_cmd(pkt);
}

void pt_cmd_vf_flash(uint16_t pgidx, uint16_t len, const uint8_t *data)
{
    PKT_ALLOC(PLEN_CMD_VF_FSH);
    pt_fill_cmd(pkt, PT_CMD_VF_FLASH, len + 3);

    PKT_PARAM(struct pt_cmd_vf_fsh);
    param->pgidx = pgidx;
    memcpy(param->data, data, len);

    pt_fill_crc(pkt);
    pt_send_cmd(pkt);
}

void pt_cmd_mf_flash(uint32_t addr, uint16_t len, const uint8_t *data)
{
    PKT_ALLOC(PLEN_CMD_MF_FSH);
    pt_fill_cmd(pkt, PT_CMD_MF_FLASH, len + 5);

    PKT_PARAM(struct pt_cmd_mf_fsh);
    param->addr = addr;
    memcpy(param->data, data, len);

    pt_fill_crc(pkt);
    pt_send_cmd(pkt);
}
/* Proto: Burn OTP */
void pt_cmd_rd_otp(uint16_t offset, uint16_t len)
{
    PKT_ALLOC(PLEN_CMD_RD_OTP);
    pt_fill_cmd(pkt, PT_CMD_RD_OTP, len + 3);

    PKT_PARAM(struct pt_cmd_rd_otp);
    param->offset = offset;
    param->length = len;

    pt_fill_crc(pkt);
    pt_send_cmd(pkt);
}

void pt_cmd_wr_otp(uint16_t offset, uint16_t len, const uint8_t *data)
{
    PKT_ALLOC(PLEN_CMD_WR_OTP);
    pt_fill_cmd(pkt, PT_CMD_WR_OTP, len + 3);

    PKT_PARAM(struct pt_cmd_wr_otp);
    param->offset = offset;
    memcpy(param->data, data, len);

    pt_fill_crc(pkt);
    pt_send_cmd(pkt);
}

void pt_cmd_vf_otp(uint16_t offset, uint16_t len, const uint8_t *data)
{
    PKT_ALLOC(PLEN_CMD_VF_OTP);
    pt_fill_cmd(pkt, PT_CMD_VF_OTP, len + 3);

    PKT_PARAM(struct pt_cmd_vf_otp);
    param->offset = offset;
    memcpy(param->data, data, len);

    pt_fill_crc(pkt);
    pt_send_cmd(pkt);
}

/* Proto: Test */
void pt_cmd_test_gpio(uint32_t masks, uint8_t modes)
{
    PKT_ALLOC(PLEN_CMD_TEST_GPIO);
    pt_fill_cmd(pkt, PT_CMD_TEST_GPIO, PLEN_CMD_TEST_GPIO);

    PKT_PARAM(struct pt_cmd_test_gpio);
    param->masks = masks;
    param->modes = modes;

    pt_send_cmd(pkt);
}

void pt_cmd_test_xtal(uint8_t mode, uint8_t gpio)
{
    PKT_ALLOC(PLEN_CMD_TEST_XTAL);
    pt_fill_cmd(pkt, PT_CMD_TEST_XTAL, PLEN_CMD_TEST_XTAL);

    PKT_PARAM(struct pt_cmd_test_xtal);
    param->mode = mode;
    param->gpio = gpio;

    pt_send_cmd(pkt);
}

void pt_cmd_test_rf(uint8_t freq, uint8_t cont)
{
    PKT_ALLOC(PLEN_CMD_TEST_RF);
    pt_fill_cmd(pkt, PT_CMD_TEST_RF, PLEN_CMD_TEST_RF);

    PKT_PARAM(struct pt_cmd_test_rf);
    param->freq = freq;
    param->cont = cont;

    pt_send_cmd(pkt);
}

/* Proto: Batch */
void pt_cmd_chans(uint8_t chns, uint8_t mode)
{
    PKT_ALLOC(PLEN_CMD_CHANS);
    pt_fill_cmd(pkt, PT_CMD_CHANS, PLEN_CMD_CHANS);

    PKT_PARAM(struct pt_cmd_chans);
    param->chns = chns;
    param->mode = mode;

    //pt_fill_crc(pkt);
    pt_send_cmd(pkt);
}

#endif //(CFG_UART2)
