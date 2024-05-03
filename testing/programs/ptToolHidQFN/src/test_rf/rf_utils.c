#include <stdint.h>
#include "reg_blecore.h"
#include "reg_em_ble_cs.h"
#include "reg_em_ble_tx_desc.h"
#include "reg_em_ble_rx_desc.h"
//#include "reg_em_ble_wl.h"
#include "reg_em_et.h"
#include "reg_em_ble_ral.h"

#include "rf_utils.h"


void ble_cs_prg(unsigned char elt_idx, struct ev_cs_tag *cs)
{
    em_ble_cntl_pack(elt_idx, cs->txbsyen, cs->rxbsyen, cs->dnabort, cs->format);
#ifdef CRYPTE_EN
    em_ble_linkcntl_pack(elt_idx, cs->linklbl, cs->sas, cs->nullrxllidflt, cs->mic_mode, cs->cryptmode, cs->txcrypt_en, cs->rxcrypt_en, cs->priv_npub);
#else
    em_ble_link_linklbl_setf(elt_idx, cs->linklbl);
    em_ble_link_priv_npub_setf(elt_idx, cs->priv_npub);
#endif
    em_ble_thrcntl_ratecntl_pack(elt_idx, cs->rxthr, cs->txthr, 0, cs->rxrate, cs->txrate);
#ifdef EXT_ADV_EN
    em_ble_thrcntl_ratecntl_aux_rate_setf(elt_idx, cs->auxrate);
#endif
    em_ble_lebdaddr_bdaddr_setf(elt_idx, 0, (cs->bdaddrlsb & 0xffff));
    em_ble_lebdaddr_bdaddr_setf(elt_idx, 1, ((cs->bdaddrlsb >> 16) & 0xffff));
    em_ble_lebdaddr_bdaddr_setf(elt_idx, 2, cs->bdaddrmsb);
    em_ble_syncwl_syncwordl_setf(elt_idx, (cs->syncword & 0xffff));
    em_ble_syncwh_syncwordh_setf(elt_idx, (cs->syncword >> 16) & 0xffff);
    em_ble_crcinit0_setf(elt_idx, (cs->crcinit & 0xffff));//cs 0ah
    em_ble_crcinit1_crcinit1_setf(elt_idx, ((cs->crcinit >> 16) & 0xff));//cs 0ch
#ifdef RAL_EN
    em_ble_filtpol_ralcntl_ral_en_setf(elt_idx, cs->ralen);
    em_ble_filtpol_ralcntl_ral_mode_setf(elt_idx, cs->ralmode);
    em_ble_filtpol_ralcntl_local_rpa_sel_setf(elt_idx, cs->localrpasel);
#endif

#ifdef EXT_ADV_EN
    em_ble_filtpol_ralcntl_filter_policy_setf(elt_idx, cs->filter_policy);
    em_ble_filtpol_ralcntl_adi_filt_mode_setf(elt_idx, cs->adi_filt_mode);
    em_ble_filtpol_ralcntl_adi_filt_en_setf(elt_idx, cs->adi_filt_en);
    em_ble_filtpol_ralcntl_peradv_filt_en_setf(elt_idx, cs->peradv_filt_en);
#endif
    em_ble_hopcntl_pack(elt_idx, cs->fh_en, cs->hop_mode, cs->hopint, cs->ch_idx);//cs 10h
#ifdef NESN_SN_EN
    em_ble_txrxcntl_sn_setf(elt_idx, cs->sn);
    em_ble_txrxcntl_nesn_setf(elt_idx, cs->nesn);
#endif
    em_ble_txrxcntl_txpwr_setf(elt_idx, cs->txpwr);
    em_ble_rxwincntl_pack(elt_idx, cs->rxwide, cs->rxwinsz);//cs 14h
    em_ble_acltxdescptr_set(elt_idx, cs->txdescptr);

#ifdef MINEVNT_TIME_EN
    em_ble_minevtime_setf(elt_idx, cs->minevtime);
#else
    em_ble_winoffset_setf(elt_idx, cs->winoffset);
#endif
    em_ble_maxevtime_setf(elt_idx, cs->maxevtime);
#ifdef CONN_EN
    ble_conninterval_setf(elt_idx, cs->conninterval);
#else
    em_ble_chmap0_llchmap0_setf(elt_idx, (cs->llchmaplsb & 0xffff));
    em_ble_chmap1_llchmap1_setf(elt_idx, ((cs->llchmaplsb >> 16) & 0xffff));
    em_ble_chmap2_llchmap2_setf(elt_idx, cs->llchmapmsb);//cs 20h
#endif
#ifdef EXT_ADV_EN
    em_ble_chmap2_advchmap_setf(elt_idx, cs->advchmap);
    em_ble_chmap2_ch_aux_setf(elt_idx, cs->ch_aux);
#endif
    em_ble_rxmaxbuf_aclrxmaxbuf_setf(elt_idx, cs->rxmaxbuf);                //cs 22h
    em_ble_rxmaxtime_setf(elt_idx, cs->rxmaxtime);             //cs 24h
#if defined(EXT_ADV_EN)
    em_ble_adv_bd_addr_setf(elt_idx, 0, (cs->adv_bd_addr0 & 0xffff));
    em_ble_adv_bd_addr_setf(elt_idx, 1, ((cs->adv_bd_addr0 >> 16) & 0xffff));
    em_ble_adv_bd_addr_setf(elt_idx, 3, (cs->adv_bd_addr1 & 0xffff));
    em_ble_adv_bd_addr_setf(elt_idx, 4, ((cs->adv_bd_addr1 >> 16) & 0xffff));
    em_ble_auxtxdescptr_setf(elt_idx, cs->auxtxdescptr);
#if defined(UNCODED_PHY_2M_EN)
    em_ble_winoffset_2m_setf(elt_idx, cs->winoffset_2m);     //cs 38h
    em_ble_conninterval_2m_setf(elt_idx, cs->conninterval_2m);    //cs 3Ah
#endif
#if defined(LONG_RATE_EN)
    em_ble_winoffset_lr_setf(elt_idx, cs->winoffset_lr);     //cs 38h
    em_ble_conninterval_lr_setf(elt_idx, cs->conninterval_lr);    //cs 3Ah
#endif
    em_ble_extadvstat_prev_adv_pkt_type_setf(elt_idx, cs->prev_adv_pkt_type);
    em_ble_extadvstat_prev_adv_mode_setf(elt_idx, cs->prev_adv_mode);
    em_ble_extadvstat_prev_lam_setf(elt_idx, cs->pre_lam);
    em_ble_extadvstat_prev_pam_setf(elt_idx, cs->pre_pam);
    em_ble_extadvstat_prev_adim_setf(elt_idx, cs->pre_adim);
    em_ble_adi_setf(elt_idx, cs->adi);
    em_ble_evtcnt_setf(elt_idx, cs->evtcnt);            //cs 4ah
#else
    em_ble_sk_setf(elt_idx, 0, (unsigned short)(cs->sk10 & 0xffff));
    em_ble_sk_setf(elt_idx, 1, (unsigned short)((cs->sk10 >> 16) & 0xffff));
    em_ble_sk_setf(elt_idx, 2, (unsigned short)(cs->sk32 & 0xffff));
    em_ble_sk_setf(elt_idx, 3, (unsigned short)((cs->sk32 >> 16) & 0xffff));
    em_ble_sk_setf(elt_idx, 4, (unsigned short)(cs->sk54 & 0xffff));
    em_ble_sk_setf(elt_idx, 5, (unsigned short)((cs->sk54 >> 16) & 0xffff));
    em_ble_sk_setf(elt_idx, 6, (unsigned short)(cs->sk76 & 0xffff));
    em_ble_sk_setf(elt_idx, 7, (unsigned short)((cs->sk76 >> 16) & 0xffff));
    em_ble_iv_setf(elt_idx, 0, (unsigned short)(cs->iv10 & 0xffff));
    em_ble_iv_setf(elt_idx, 1, (unsigned short)((cs->iv10 >> 16) & 0xffff));
    em_ble_iv_setf(elt_idx, 2, (unsigned short)(cs->iv32 & 0xffff));
    em_ble_iv_setf(elt_idx, 3, (unsigned short)((cs->iv32 >> 16) & 0xffff));
    em_ble_evtcnt_setf(elt_idx, cs->evtcnt);            //cs 4ah
#endif

}

void ble_txdsc_phce_prg(unsigned char elt_idx, struct ev_tx_con_des_tag *elt_tx_con_des)
{
//    em_ble_txcntl_txdone_setf(elt_idx, 0);
//    em_ble_txcntl_nextptr_setf(elt_idx, elt_tx_con_des->nextptr);
//    em_ble_txphce_pack(elt_idx, elt_tx_con_des->txlen, 0, elt_tx_con_des->txmd, elt_tx_con_des->txsn, elt_tx_con_des->txnesn, elt_tx_con_des->txllid);
//    em_ble_txdataptr_setf(elt_idx, elt_tx_con_des->txdatptr);
}

void ble_txdsc_phadv_prg(unsigned char elt_idx, struct ev_tx_adv_des_tag *elt_tx_adv_des)
{
    em_ble_txcntl_txdone_setf(elt_idx, 0);
    em_ble_txcntl_next_ptr_setf(elt_idx, elt_tx_adv_des->nextptr);
    em_ble_txphadv_txchsel2_setf(elt_idx, elt_tx_adv_des->txchsel);
    em_ble_txphadv_txadvlen_setf(elt_idx, elt_tx_adv_des->txadvlen);
    em_ble_txphadv_txadvrfu_setf(elt_idx, 0);
    em_ble_txphadv_txrxadd_setf(elt_idx, elt_tx_adv_des->txrxadd);
    em_ble_txphadv_txtxadd_setf(elt_idx, elt_tx_adv_des->txtxadd);
    em_ble_txphadv_txtype_setf(elt_idx, elt_tx_adv_des->txtype);
    em_ble_txdataptr_setf(elt_idx, elt_tx_adv_des->txdatptr);
}

void ble_ext_txdsc_phadv_prg(unsigned char elt_idx, struct ev_tx_ext_adv_des_tag *elt_ext_tx_adv_des)
{
//    em_ble_txcntl_txdone_setf(elt_idx, 0);
//    em_ble_txcntl_nextptr_setf(elt_idx, elt_ext_tx_adv_des->nextptr);
//    em_ble_txphadv_txadvlen_setf(elt_idx, elt_ext_tx_adv_des->txadvlen);
//    em_ble_txphadv_txrxadd_setf(elt_idx, elt_ext_tx_adv_des->txrxadd);
//    em_ble_txphadv_txtxadd_setf(elt_idx, elt_ext_tx_adv_des->txtxadd);
//    em_ble_txphadv_txtype_setf(elt_idx, elt_ext_tx_adv_des->txtype);
//    em_ble_txaeheader_pack(elt_idx, elt_ext_tx_adv_des->txrsvd, elt_ext_tx_adv_des->txpow, elt_ext_tx_adv_des->txsync, elt_ext_tx_adv_des->txauxptr,
//                           elt_ext_tx_adv_des->txadi, elt_ext_tx_adv_des->txsupp, elt_ext_tx_adv_des->txtgta, elt_ext_tx_adv_des->txadva, elt_ext_tx_adv_des->txaemode,
//                           elt_ext_tx_adv_des->txaelength);
//    em_ble_txauxptr0_pack(elt_idx, elt_ext_tx_adv_des->txauxoffsetlsb, elt_ext_tx_adv_des->txauxoffset_unit, elt_ext_tx_adv_des->txaux_ca,
//                          elt_ext_tx_adv_des->tx_ll_ch);
//    em_ble_txauxptr1_pack(elt_idx, elt_ext_tx_adv_des->txaux_phy, elt_ext_tx_adv_des->txauxoffsetmsb);
//    em_ble_txaedataptr_setf(elt_idx, elt_ext_tx_adv_des->txaeheader_dataptr);
}


//ble_ral_prg is used to program RAL struct with txphadv
void ble_ral_prg(unsigned char elt_idx, struct ev_ral_tag *elt_ral_des)
{
    em_ble_ral_info_pack(elt_idx, elt_ral_des->entry_valid, elt_ral_des->connected, elt_ral_des->in_whlist, elt_ral_des->in_padvlist,  elt_ral_des->pef,
                         elt_ral_des->local_rpa_valid,  elt_ral_des->local_rpa_renew,  elt_ral_des->local_irk_valid,  elt_ral_des->peer_rpa_valid, elt_ral_des->peer_rpa_renew,
                         elt_ral_des->peer_irk_valid,   elt_ral_des->peer_id_type);

    em_ble_ral_peer_irk_peer_irk_setf(elt_idx, 0, (unsigned short)(elt_ral_des->peer_irk[0] & 0xffff));
    em_ble_ral_peer_irk_peer_irk_setf(elt_idx, 1, (unsigned short)((elt_ral_des->peer_irk[0] >> 16) & 0xffff));
    em_ble_ral_peer_irk_peer_irk_setf(elt_idx, 2, (unsigned short)(elt_ral_des->peer_irk[1] & 0xffff));
    em_ble_ral_peer_irk_peer_irk_setf(elt_idx, 3, (unsigned short)((elt_ral_des->peer_irk[1] >> 16) & 0xffff));
    em_ble_ral_peer_irk_peer_irk_setf(elt_idx, 4, (unsigned short)(elt_ral_des->peer_irk[2] & 0xffff));
    em_ble_ral_peer_irk_peer_irk_setf(elt_idx, 5, (unsigned short)((elt_ral_des->peer_irk[2] >> 16) & 0xffff));
    em_ble_ral_peer_irk_peer_irk_setf(elt_idx, 6, (unsigned short)(elt_ral_des->peer_irk[3] & 0xffff));
    em_ble_ral_peer_irk_peer_irk_setf(elt_idx, 7, (unsigned short)((elt_ral_des->peer_irk[3] >> 16) & 0xffff));

    em_ble_ral_peer_rpa_peer_rpa_setf(elt_idx, 0, elt_ral_des->peer_rpa[0]);
    em_ble_ral_peer_rpa_peer_rpa_setf(elt_idx, 1, elt_ral_des->peer_rpa[1]);
    em_ble_ral_peer_rpa_peer_rpa_setf(elt_idx, 2, elt_ral_des->peer_rpa[2]);

    em_ble_ral_peer_id_peer_id_setf(elt_idx, 0, elt_ral_des->peer_id[0]);
    em_ble_ral_peer_id_peer_id_setf(elt_idx, 1, elt_ral_des->peer_id[1]);
    em_ble_ral_peer_id_peer_id_setf(elt_idx, 2, elt_ral_des->peer_id[2]);

    em_ble_ral_local_irk_local_irk_setf(elt_idx, 0, (unsigned short)(elt_ral_des->local_irk[0] & 0xffff));
    em_ble_ral_local_irk_local_irk_setf(elt_idx, 1, (unsigned short)((elt_ral_des->local_irk[0] >> 16) & 0xffff));
    em_ble_ral_local_irk_local_irk_setf(elt_idx, 2, (unsigned short)(elt_ral_des->local_irk[1] & 0xffff));
    em_ble_ral_local_irk_local_irk_setf(elt_idx, 3, (unsigned short)((elt_ral_des->local_irk[1] >> 16) & 0xffff));
    em_ble_ral_local_irk_local_irk_setf(elt_idx, 4, (unsigned short)(elt_ral_des->local_irk[2] & 0xffff));
    em_ble_ral_local_irk_local_irk_setf(elt_idx, 5, (unsigned short)((elt_ral_des->local_irk[2] >> 16) & 0xffff));
    em_ble_ral_local_irk_local_irk_setf(elt_idx, 6, (unsigned short)(elt_ral_des->local_irk[3] & 0xffff));
    em_ble_ral_local_irk_local_irk_setf(elt_idx, 7, (unsigned short)((elt_ral_des->local_irk[3] >> 16) & 0xffff));

    em_ble_ral_local_rpa_local_rpa_setf(elt_idx, 0, elt_ral_des->local_rpa[0]);
    em_ble_ral_local_rpa_local_rpa_setf(elt_idx, 1, elt_ral_des->local_rpa[1]);
    em_ble_ral_local_rpa_local_rpa_setf(elt_idx, 2, elt_ral_des->local_rpa[2]);
}

//ble_adil_prg is used to program ADIL struct with tx/rx
void ble_adil_prg(unsigned char elt_idx, struct ev_adil_tag *elt_adil_des)
{

}

//ble_wl_prg is used to program WL struct with tx/rx
void ble_wl_prg(unsigned char elt_idx, struct ev_wl_tag *elt_wl_des)
{
//    em_ble_wlist_info_pack(elt_idx, elt_wl_des->wlentry_valid, elt_wl_des->wl_idtype, elt_wl_des->rsvd, elt_wl_des->in_peradvl);
//    em_ble_wl_bdaddr_wlbdaddr_setf(elt_idx, 0, elt_wl_des->wlbdaddr[0]);
//    em_ble_wl_bdaddr_wlbdaddr_setf(elt_idx, 1, elt_wl_des->wlbdaddr[1]);
//    em_ble_wl_bdaddr_wlbdaddr_setf(elt_idx, 2, elt_wl_des->wlbdaddr[2]);
}

//ble_peradvl_prg is used to program peradvl struct with txphadv
void ble_peradvl_prg(unsigned char elt_idx, struct ev_peradvl_tag *elt_peradvl_des)
{

}

//ble_et_prg is used to program et struct with tx/rx
void ble_ex_et_prg(unsigned char elt_idx, struct ev_et_tag *tmp_ev_et_tag)
{
    em_le_extab_pack(elt_idx, tmp_ev_et_tag->sch_prio1, tmp_ev_et_tag->spa, tmp_ev_et_tag->ae_nps, tmp_ev_et_tag->rsvd,
                   tmp_ev_et_tag->iso, tmp_ev_et_tag->status, tmp_ev_et_tag->mode);
    em_rawstp0_setf(elt_idx, tmp_ev_et_tag->rawstp0);
    em_rawstp1_setf(elt_idx, tmp_ev_et_tag->rawstp1);
    em_finestp_setf(elt_idx, tmp_ev_et_tag->finestp);
    em_csptr_setf(elt_idx, (tmp_ev_et_tag->csptr >> 2));
    em_priobw_pack(elt_idx, tmp_ev_et_tag->priobw_unit, tmp_ev_et_tag->priobw);
    em_priolvl_pack(elt_idx, tmp_ev_et_tag->sch_prio3, tmp_ev_et_tag->sch_prio2);
    em_pti_vxchan_pti_prio_setf(elt_idx, tmp_ev_et_tag->pti_prio);

}

curr_ble_time_t get_ble_time(void)
{
    curr_ble_time_t res;
    ble_slotclk_samp_setf(1);
    while (ble_slotclk_samp_getf());
    res.time = ble_slotclk_sclk_getf();
    res.next_tick = ble_finetimecnt_finecnt_getf();

    return res;
}
