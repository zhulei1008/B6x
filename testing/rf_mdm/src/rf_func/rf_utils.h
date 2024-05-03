// Common Functions
#ifndef _RF_UTILS_H_
#define _RF_UTILS_H_

#include <stdint.h>


/*
 * STRUCTURES
 ****************************************************************************************
 */

struct ev_cs_tag
{   
    //cs 00h
    unsigned char format; unsigned char dnabort; unsigned char rxbsyen; unsigned char txbsyen;  
    //cs 02h
    unsigned char priv_npub; 
#ifdef CRYPTE_EN
    unsigned char rxcrypt_en; unsigned char txcrypt_en; unsigned char cryptmode; unsigned char mic_mode; unsigned char nullrxllidflt; unsigned char sas;
#endif
    unsigned char linklbl;
    
    //cs 06h
    unsigned char txrate; unsigned char rxrate;
#ifdef EXT_ADV_EN
    unsigned char auxrate;
#endif
    unsigned char txthr; unsigned char rxthr;
    
    unsigned long bdaddrlsb;//cs 08~0ah
    unsigned short bdaddrmsb;//cs 0ch
    unsigned long syncword;//cs 0x0e~10h
    unsigned long crcinit; //cs 12~14h
#ifdef RAL_EN
    unsigned char ralen; unsigned char ralmode; unsigned char localrpasel;
#endif
#ifdef EXT_ADV_EN
    unsigned char peradv_filt_en; unsigned char adi_filt_en; unsigned char adi_filt_mode;

    unsigned char filter_policy;//cs 16h
#endif
    unsigned char ch_idx; unsigned char hopint; unsigned char hop_mode; unsigned char fh_en;    //cs 18h
    unsigned char txpwr; /*unsigned char ends_on_sac; unsigned char rxmafserr; unsigned char rxbfmicerr*/
#ifdef NESN_SN_EN
    unsigned char nesn; unsigned char sn;
#endif
    //unsigned char lastempty; unsigned char rxbuff_full; //cs 1ah
    unsigned short rxwinsz; unsigned char rxwide;     //cs 1ch
    unsigned short txdescptr;      //cs 1eh
#ifdef MINEVNT_TIME_EN
    unsigned short minevtime;      //cs 20h
#else
    unsigned short winoffset;    //cs 20h
#endif
    unsigned short maxevtime;      //cs 22h
#ifdef CONN_EN
    unsigned short conninterval;    //cs 24h
#else
    unsigned long llchmaplsb; unsigned char llchmapmsb;  //cs 24~28h; llchmap3(20h; [4:0])
#endif

#ifdef EXT_ADV_EN
    unsigned char advchmap; unsigned char ch_aux;   //cs 28h
#endif
    unsigned char rxmaxbuf;                    //cs 2ah
    unsigned short rxmaxtime;                 //cs 2ch

#if defined(EXT_ADV_EN)
    unsigned long adv_bd_addr0;          //cs 2e~30h
    unsigned long adv_bd_addr1;          //cs 32~34h
    unsigned short auxtxdescptr;        //cs 36h


#if defined(UNCODED_PHY_2M_EN)
    unsigned short winoffset_2m;     //cs 38h
    unsigned short conninterval_2m;    //cs 3Ah
#endif

#if defined(LONG_RATE_EN)
    unsigned short winoffset_lr;     //cs 38h
    unsigned short conninterval_lr;    //cs 3Ah
#endif
    unsigned char prev_adv_pkt_type;
    unsigned char prev_adv_mode;
    unsigned char pre_lam;
    unsigned char pre_pam;
    unsigned char pre_adim;
    unsigned short adi;                 //cs 54h
    unsigned short evtcnt;                //cs 52h
#else
    unsigned long sk10;                  //cs 30h
    unsigned long sk32;                //cs 32h
    unsigned long sk54;                  //cs 30h
    unsigned long sk76;                  //cs 30h
    unsigned long iv10;                //cs
    unsigned long iv32;                //cs
    unsigned short evtcnt;                //cs 52h

#endif
};

struct ev_tx_con_des_tag
{
    unsigned short nextptr;
    unsigned char txllid; unsigned char txnesn; unsigned char txsn; unsigned char txmd; unsigned char txlen;
    unsigned short txdatptr;
};

struct ev_tx_adv_des_tag
{
    unsigned short nextptr;
    unsigned char txtype;  unsigned char txtxadd; unsigned char txrxadd; unsigned char txchsel; unsigned char txadvlen;
    unsigned short txdatptr;
};

struct ev_tx_ext_adv_des_tag
{
    unsigned short nextptr;
    unsigned char txtype; unsigned char txtxadd; unsigned char txrxadd; unsigned char txchsel; unsigned char txadvlen;
    unsigned char txaelength; unsigned char txaemode; unsigned char txadva; unsigned char txtgta; unsigned char txsupp; unsigned char txadi; unsigned char txauxptr; unsigned char txsync; unsigned char txpow; unsigned char txrsvd;
    unsigned char tx_ll_ch; unsigned char txaux_ca; unsigned char txauxoffset_unit; unsigned char txauxoffsetlsb;
    unsigned char txauxoffsetmsb; unsigned char txaux_phy;
    unsigned short txaeheader_dataptr;
};

struct ev_ral_tag
{
    unsigned char entry_valid;
    unsigned char connected;
    unsigned char in_whlist;
    unsigned char in_padvlist;
    unsigned char pef;
    unsigned char local_rpa_valid;
    unsigned char local_rpa_renew;
    unsigned char local_irk_valid;
    unsigned char peer_rpa_valid;
    unsigned char peer_rpa_renew;
    unsigned char peer_irk_valid;
    unsigned char peer_id_type;
    unsigned long peer_irk[4];
    unsigned short peer_rpa[3];
    unsigned short peer_id[3];
    unsigned long local_irk[4];
    unsigned short local_rpa[3];
};

struct ev_adil_tag
{
    unsigned char exclude;
    unsigned char adimsk;
    unsigned char adilentry_valid;
    unsigned short adi_data;
};

struct ev_wl_tag
{
    unsigned char in_peradvl;
    unsigned char rsvd;
    unsigned char wl_idtype;
    unsigned char wlentry_valid;
    unsigned short wlbdaddr[3];
};

struct ev_peradvl_tag
{
    unsigned char peradventry_valid;
    unsigned char peradv_idtype;
    unsigned char in_wl;
    unsigned char rsvd;
    unsigned short padvbdaddr[3];
};

struct ev_et_tag
{
    unsigned char mode;
    unsigned char status;
    unsigned char iso;
    unsigned char rsvd;
    unsigned char ae_nps;
    unsigned char isobufsel;
    unsigned char spa;
    unsigned char sch_prio1;
    unsigned short rawstp0;
    unsigned short rawstp1;
    unsigned short finestp;
    unsigned short csptr;
    unsigned short priobw;
    unsigned char priobw_unit;
    unsigned char sch_prio2;
    unsigned char sch_prio3;
    unsigned char isochan;
    unsigned char pti_prio;
};

typedef struct
{
    uint32_t time;
    uint32_t next_tick;
} curr_ble_time_t;


/*
 * EXTERN FUNCTIONS
 ****************************************************************************************
 */

void ble_cs_prg(unsigned char elt_idx, struct ev_cs_tag *cs);

void ble_txdsc_phce_prg(unsigned char elt_idx, struct ev_tx_con_des_tag *elt_tx_con_des);

void ble_txdsc_phadv_prg(unsigned char elt_idx, struct ev_tx_adv_des_tag *elt_tx_adv_des);

void ble_ext_txdsc_phadv_prg(unsigned char elt_idx, struct ev_tx_ext_adv_des_tag *elt_ext_tx_adv_des);

void ble_ral_prg(unsigned char elt_idx, struct ev_ral_tag *elt_ral_des);

void ble_adil_prg(unsigned char elt_idx, struct ev_adil_tag *elt_adil_des);

void ble_wl_prg(unsigned char elt_idx, struct ev_wl_tag *elt_wl_des);

void ble_peradvl_prg(unsigned char elt_idx, struct ev_peradvl_tag *elt_peradvl_des);

void ble_ex_et_prg(unsigned char elt_idx, struct ev_et_tag *tmp_ev_et_tag);

curr_ble_time_t get_ble_time(void);

#endif //_RF_UTILS_H_
