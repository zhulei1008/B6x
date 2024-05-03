#include <stdint.h>
#include <string.h>
#include "proto.h"
#include "ota_itf.h"
#include "prf_otas.h"
#include "app.h"

#if (OTA_CHIP || OTA_HOST)

static __inline void pt_fill_rsp(pkt_t *pkt, uint8_t rsp, uint16_t len)
{
    pkt->type = PT_TYPE_RSP;
    pkt->code = rsp;
    pkt->len  = len;
}

static __inline void pt_fill_crc(pkt_t *pkt)
{
    uint16_t len = pkt->len - 1;

    pkt->payl[len] = pkt_crc8(pkt->payl, len);
}

/*static __inline*/ void pt_send_rsp(pkt_t *pkt)
{
#if (ROLE_BURNER)
    uart1_send((uint8_t *)pkt, pkt->len + PKT_HDR_SIZE);
#else    
    otas_txd_send(app_env.curidx, pkt->len + PKT_HDR_SIZE, (uint8_t *)pkt);
#endif    
}

/* Common: Status */
void pt_rsp_status(uint8_t rsp, uint8_t status)
{
    PKT_ALLOC(PLEN_RSP_STATUS);
    pt_fill_rsp(pkt, rsp, PLEN_RSP_STATUS);

    PKT_PARAM(struct pt_rsp_status);
    param->status = status;

    pt_send_rsp(pkt);
}

void pt_rsp_multis(uint8_t rsp, uint8_t len, const void *payl)
{
    PKT_ALLOC(PLEN_RSP_MULTIS);
    pt_fill_rsp(pkt, rsp, len + 1);

    memcpy(pkt->payl, payl, len);

    pt_fill_crc(pkt);
    pt_send_rsp(pkt);
}

/* Boot: Load Sram */
void pt_rsp_echo(uint8_t echo)
{
    PKT_ALLOC(PLEN_RSP_ECHO);
    pt_fill_rsp(pkt, PT_RSP_ECHO, PLEN_RSP_ECHO);

    PKT_PARAM(struct pt_rsp_echo);
    param->echo = echo;

    pt_fill_crc(pkt);
    pt_send_rsp(pkt);
}

void pt_rsp_srd(uint32_t value)
{
    PKT_ALLOC(PLEN_RSP_SRD);
    pt_fill_rsp(pkt, PT_RSP_SRD, PLEN_RSP_SRD);

    PKT_PARAM(struct pt_rsp_srd);
    param->value = value;

    pt_fill_crc(pkt);
    pt_send_rsp(pkt);
}

void pt_rsp_crd(uint8_t *data, uint16_t len)
{
    PKT_ALLOC(PLEN_RSP_CRD);
    pt_fill_rsp(pkt, PT_RSP_CRD, len + 1);

    //PKT_PARAM(struct pt_rsp_crd);
    memcpy(pkt->payl, data, len);

    pt_fill_crc(pkt);
    pt_send_rsp(pkt);
}

/* Proto: Response */
void pt_rsp_version(uint16_t vers)
{
    PKT_ALLOC(PLEN_RSP_VERSION);
    pt_fill_rsp(pkt, PT_RSP_VERSION, PLEN_RSP_VERSION);

    PKT_PARAM(struct pt_rsp_version);
    param->vers = vers;

    pt_send_rsp(pkt);
}

void pt_rsp_rd_firm(uint16_t pgidx, uint16_t len, const uint8_t *data)
{
    PKT_ALLOC(PLEN_RSP_RD_FSH);
    pt_fill_rsp(pkt, PT_RSP_RD_FIRM, len + 3);

    PKT_PARAM(struct pt_rsp_rd_fsh);
    param->pgidx = pgidx;
    memcpy(param->data, data, len);

    pt_fill_crc(pkt);
    pt_send_rsp(pkt);
}

void pt_rsp_rd_flash(uint16_t pgidx, uint16_t len, const uint8_t *data)
{
    PKT_ALLOC(PLEN_RSP_RD_FSH);
    pt_fill_rsp(pkt, PT_RSP_RD_FLASH, len + 3);

    PKT_PARAM(struct pt_rsp_rd_fsh);
    param->pgidx = pgidx;
    memcpy(param->data, data, len);

    pt_fill_crc(pkt);
    pt_send_rsp(pkt);
}

void pt_rsp_rd_otp(uint16_t offset, uint16_t len, const uint8_t *data)
{
    PKT_ALLOC(PLEN_RSP_RD_OTP);
    pt_fill_rsp(pkt, PT_RSP_RD_OTP, len + 3);

    PKT_PARAM(struct pt_rsp_rd_otp);
    param->offset = offset;
    memcpy(param->data, data, len);

    pt_fill_crc(pkt);
    pt_send_rsp(pkt);
}

void pt_rsp_xtal_calc(uint32_t value)
{
    PKT_ALLOC(PLEN_RSP_TEST_XTAL);
    pt_fill_rsp(pkt, PT_RSP_TEST_XTAL, PLEN_RSP_TEST_XTAL);

    PKT_PARAM(union pt_rsp_xtal_calc);
    param->value = value;

    pt_send_rsp(pkt);
}

void pt_rsp_pwr_value(uint16_t value)
{
    PKT_ALLOC(PLEN_RSP_TEST_PWR);
    pt_fill_rsp(pkt, PT_RSP_TEST_PWR, PLEN_RSP_TEST_PWR);

    PKT_PARAM(struct pt_rsp_test_pwr);
    param->value = value;

    pt_send_rsp(pkt);
}

/* Proto: Batch */
void pt_rsp_chans(uint8_t chns, uint8_t state)
{
    PKT_ALLOC(PT_RSP_CHANS);
    pt_fill_rsp(pkt, PT_RSP_CHANS, PLEN_RSP_CHANS);

    PKT_PARAM(struct pt_rsp_chans);
    param->chns = chns;
    param->state = state;

    pt_send_rsp(pkt);
}

void pt_rsp_chsta(uint8_t chan, const void *psta)
{
    PKT_ALLOC(PT_RSP_CHSTA);
    pt_fill_rsp(pkt, PT_RSP_CHSTA, PLEN_RSP_CHSTA);

    PKT_PARAM(struct pt_rsp_chsta);
    param->chan = chan;
    memcpy(param->psta, psta, SIZE_CHX_STATE);

    pt_fill_crc(pkt);
    pt_send_rsp(pkt);
}

void pt_rsp_chret(uint8_t chan, const void *pmac, const void *ptst)
{
    PKT_ALLOC(PLEN_RSP_CHRET);
    pt_fill_rsp(pkt, PT_RSP_CHRET, PLEN_RSP_CHRET);

    PKT_PARAM(struct pt_rsp_chret);
    param->chan = chan;
    memcpy(&param->pmac, pmac, SIZE_MAC_INFO);
    memcpy(&param->ptst, ptst, SIZE_TEST_INFO);

    pt_fill_crc(pkt);
    pt_send_rsp(pkt);
}

void pt_rsp_fwcfg(const void *info, const void *pmac)
{
    PKT_ALLOC(PLEN_RSP_FWCFG);
    pt_fill_rsp(pkt, PT_RSP_FWCFG, PLEN_RSP_FWCFG);

    PKT_PARAM(struct pt_rsp_fwcfg);
    memcpy(param->info, info, SIZE_FIRM_INFO);
    memcpy(param->pmac, pmac, SIZE_MAC_INFO);

    pt_fill_crc(pkt);
    pt_send_rsp(pkt);
}

#endif //(CFG_UART2)
