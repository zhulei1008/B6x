#include <stdint.h>
#include <stdbool.h>
#include "proto.h"

#if (OTA_CHIP || OTA_HOST)
#if ((PT_COMMAND + PT_RESPONSE) == 0)
#error "None PT_PKT"
#endif

typedef struct pkt_desc
{
    uint8_t  code; // pkt code
    uint8_t  pfmt; // pkt format
    uint16_t plen; // payl length
} pkt_desc_t;

#define PKT_CRC_SEED      (0xFF)
#define PKT_CMD_CNT       (sizeof(pt_cmd_desc)/sizeof(struct pkt_desc))
#define PKT_RSP_CNT       (sizeof(pt_rsp_desc)/sizeof(struct pkt_desc))

#if (PT_COMMAND)
const struct pkt_desc pt_cmd_desc[] =
{
#if (PT_BOOT_CMD)
    { PT_CMD_SRD,         PKT_FIXLEN_CRC,    PLEN_CMD_SRD       },
    //{ PT_CMD_SWR,         PKT_FIXLEN_CRC,    PLEN_CMD_SWR       },
    //{ PT_CMD_CRD,         PKT_FIXLEN_CRC,    PLEN_CMD_CRD       },
    //{ PT_CMD_WR_HD,       PKT_FIXLEN_CRC,    PLEN_CMD_WR_HD     },
    { PT_CMD_CWR,         PKT_VARLEN_CRC,    PLEN_CMD_CWR       },
    { PT_CMD_BAUD,        PKT_FIXLEN_CRC,    PLEN_CMD_BAUD      },
    //{ PT_CMD_ECHO,        PKT_FIXLEN,        PLEN_CMD_ECHO      },
    { PT_CMD_RST,         PKT_FIXLEN,        PLEN_CMD_RST       },
    { PT_CMD_JUMP,        PKT_FIXLEN,        PLEN_CMD_JUMP      },
    //{ PT_CMD_PATCH,       PKT_FIXLEN,        PLEN_CMD_PATCH     },
    //{ PT_CMD_RETURN,      PKT_FIXLEN,        PLEN_CMD_RETURN    },
#endif //(PT_BOOT_CMD)
    { PT_CMD_RST,         PKT_FIXLEN,        PLEN_CMD_RST       },
    /* Proto: Version */
    { PT_CMD_VERSION,     PKT_FIXLEN,        PLEN_CMD_VERSION   },

#if (PT_FIRM_CMD)
    { PT_CMD_ER_FIRM,     PKT_VARLEN_CRC,    PLEN_CMD_ER_FSH    },
    { PT_CMD_RD_FIRM,     PKT_FIXLEN_CRC,    PLEN_CMD_RD_FSH    },
    { PT_CMD_WR_FIRM,     PKT_VARLEN_CRC,    PLEN_CMD_WR_FSH    },
    { PT_CMD_VF_FIRM,     PKT_VARLEN_CRC,    PLEN_CMD_VF_FSH    },
    { PT_CMD_MF_FIRM,     PKT_VARLEN_CRC,    PLEN_CMD_MF_FSH    },
#endif //(PT_FIRM_CMD)

    /* Proto: Sync */
    { PT_CMD_SYNC,        PKT_FIXLEN,        PLEN_CMD_SYNC      },
    
#if (PT_FIRM_CMD)
    /* Proto: Batch */
    { PT_CMD_ACTION,      PKT_FIXLEN,        PLEN_CMD_ACTION    },
    { PT_CMD_CHANS,       PKT_FIXLEN,        PLEN_CMD_CHANS     },
    { PT_CMD_CHSTA,       PKT_FIXLEN,        PLEN_CMD_CHSTA     },
    { PT_CMD_CHRET,       PKT_FIXLEN,        PLEN_CMD_CHRET     },

    /* Proto: Config */
    { PT_CMD_FWCFG,       PKT_FIXLEN,        PLEN_CMD_FWCFG     },
#endif //(PT_FIRM_CMD)

#if (PT_FLASH_CMD)
    { PT_CMD_ER_FLASH,    PKT_VARLEN_CRC,    PLEN_CMD_ER_FSH    },
    { PT_CMD_RD_FLASH,    PKT_FIXLEN_CRC,    PLEN_CMD_RD_FSH    },
    { PT_CMD_WR_FLASH,    PKT_VARLEN_CRC,    PLEN_CMD_WR_FSH    },
    { PT_CMD_VF_FLASH,    PKT_VARLEN_CRC,    PLEN_CMD_VF_FSH    },
    { PT_CMD_MF_FLASH,    PKT_VARLEN_CRC,    PLEN_CMD_MF_FSH    },
#endif //(PT_FLASH_CMD)

#if (PT_OTP_CMD)
    { PT_CMD_RD_OTP,      PKT_FIXLEN_CRC,    PLEN_CMD_RD_OTP    },
    { PT_CMD_WR_OTP,      PKT_VARLEN_CRC,    PLEN_CMD_WR_OTP    },
    { PT_CMD_VF_OTP,      PKT_VARLEN_CRC,    PLEN_CMD_VF_OTP    },
#endif //(PT_OTP_CMD)

//    { PT_CMD_TRIMVAL,     PKT_FIXLEN,        PLEN_CMD_TRIMVAL   },

#if (PT_TEST_CMD)
    { PT_CMD_TEST_GPIO,   PKT_FIXLEN,        PLEN_CMD_TEST_GPIO },
    { PT_CMD_TEST_XTAL,   PKT_FIXLEN,        PLEN_CMD_TEST_XTAL },
    { PT_CMD_TEST_RF,     PKT_FIXLEN,        PLEN_CMD_TEST_RF   },
    { PT_CMD_TEST_PWR,    PKT_FIXLEN,        PLEN_CMD_TEST_PWR  },
#endif //(PT_TEST_CMD)

};
#endif //(PT_COMMAND)

#if (PT_RESPONSE)
const struct pkt_desc pt_rsp_desc[] =
{
    /* Proto: Version */
    { PT_RSP_VERSION,     PKT_FIXLEN,        PLEN_RSP_VERSION   },

#if (PT_FIRM_RSP)
    { PT_RSP_ER_FIRM,     PKT_FIXLEN,        PLEN_RSP_ER_FSH    },
    { PT_RSP_RD_FIRM,     PKT_VARLEN_CRC,    PLEN_RSP_RD_FSH    },
    { PT_RSP_WR_FIRM,     PKT_FIXLEN,        PLEN_RSP_WR_FSH    },
    { PT_RSP_VF_FIRM,     PKT_FIXLEN,        PLEN_RSP_VF_FSH    },
    { PT_RSP_MF_FIRM,     PKT_FIXLEN,        PLEN_RSP_MF_FSH    },
#endif //(PT_FIRM_RSP)

    /* Proto: Sync */
    { PT_RSP_SYNC,        PKT_FIXLEN,        PLEN_RSP_SYNC      },

    /* Proto: Batch */
    { PT_RSP_ACTION,      PKT_FIXLEN,        PLEN_RSP_ACTION    },
    { PT_RSP_CHANS,       PKT_FIXLEN,        PLEN_RSP_CHANS     },
    { PT_RSP_CHSTA,       PKT_FIXLEN_CRC,    PLEN_RSP_CHSTA     },
    { PT_RSP_CHRET,       PKT_FIXLEN_CRC,    PLEN_RSP_CHRET     },

    /* Proto: Config */
    { PT_RSP_FWCFG,       PKT_FIXLEN_CRC,    PLEN_RSP_FWCFG     },

#if (PT_FLASH_RSP)
    { PT_RSP_ER_FLASH,    PKT_FIXLEN,        PLEN_RSP_ER_FSH    },
    { PT_RSP_RD_FLASH,    PKT_VARLEN_CRC,    PLEN_RSP_RD_FSH    },
    { PT_RSP_WR_FLASH,    PKT_FIXLEN,        PLEN_RSP_WR_FSH    },
    { PT_RSP_VF_FLASH,    PKT_FIXLEN,        PLEN_RSP_VF_FSH    },
    { PT_RSP_MF_FLASH,    PKT_FIXLEN,        PLEN_RSP_MF_FSH    },
#endif //(PT_FLASH_RSP)

#if (PT_OTP_RSP)
    { PT_RSP_RD_OTP,      PKT_VARLEN_CRC,    PLEN_RSP_RD_OTP    },
    { PT_RSP_WR_OTP,      PKT_FIXLEN,        PLEN_RSP_WR_OTP    },
    { PT_RSP_VF_OTP,      PKT_FIXLEN,        PLEN_RSP_VF_OTP    },
#endif //(PT_OTP_RSP)

    { PT_RSP_TRIMVAL,     PKT_FIXLEN,        PLEN_RSP_TRIMVAL   },

#if (PT_TEST_RSP)
    { PT_RSP_TEST_GPIO,   PKT_FIXLEN,        PLEN_RSP_TEST_GPIO },
    { PT_RSP_TEST_XTAL,   PKT_VARLEN,        PLEN_RSP_TEST_XTAL },
    { PT_RSP_TEST_RF,     PKT_FIXLEN,        PLEN_RSP_TEST_RF   },
    { PT_RSP_TEST_PWR,    PKT_VARLEN,        PLEN_RSP_TEST_PWR  },
#endif //(PT_TEST_RSP)
    
#if (PT_BOOT_RSP)
    { PT_RSP_SRD,         PKT_FIXLEN_CRC,    PLEN_RSP_SRD       },
    //{ PT_RSP_SWR,         PKT_FIXLEN,        PLEN_RSP_SWR       },
    //{ PT_RSP_CRD,         PKT_VARLEN_CRC,    PLEN_RSP_CRD       },
    //{ PT_RSP_WR_HD,       PKT_FIXLEN,        PLEN_RSP_WR_HD     },
    { PT_RSP_CWR,         PKT_FIXLEN,        PLEN_RSP_CWR       },
    { PT_RSP_BAUDRV,      PKT_FIXLEN,        PLEN_RSP_BAUDRV    },
    { PT_RSP_BAUDEND,     PKT_FIXLEN,        PLEN_RSP_BAUDEND   },
    //{ PT_RSP_ECHO,        PKT_FIXLEN_CRC,    PLEN_RSP_ECHO      },
    { PT_RSP_RST,         PKT_FIXLEN,        PLEN_RSP_RST       },
    { PT_RSP_JUMP,        PKT_FIXLEN,        PLEN_RSP_JUMP      },
    //{ PT_RSP_PATCH,       PKT_FIXLEN,        PLEN_RSP_PATCH     },
    //{ PT_RSP_RETURN,      PKT_FIXLEN,        PLEN_RSP_RETURN    },
#endif //(PT_BOOT_RSP)
};
#endif //(PT_RESPONSE)

static uint8_t pkt_find(struct pt_pkt *hdr, const struct pkt_desc *desc, uint8_t desc_size)
{
    uint8_t idx;
    bool find = false;

    for (idx = 0; idx < desc_size; idx++)
    {
        if (hdr->code > desc[idx].code)
        {
            continue;
        }
        else
        {
            if (hdr->code == desc[idx].code)
            {
                find = true;
            }
            break;
        }
    }

    if (!find)
        return PT_ERR_CODE;

    if ((desc[idx].pfmt & PKT_LEN_MSK) == PKT_VARLEN)
    {
        if (hdr->len > desc[idx].plen)
            return PT_ERR_LEN;
    }
    else // PKT_FIXLEN
    {
        if (hdr->len != desc[idx].plen)
            return PT_ERR_LEN;
    }

    return desc[idx].pfmt; //PT_OK;
}

uint8_t pkt_hdr_valid(struct pt_pkt *p_hdr)
{
    uint8_t status = PT_ERR_HEAD;

#if (PT_COMMAND)
    if (p_hdr->type == PT_TYPE_CMD)
    {
        return pkt_find(p_hdr, pt_cmd_desc, PKT_CMD_CNT);
    }
#endif //(PT_COMMAND)

#if (PT_RESPONSE)
    if (p_hdr->type == PT_TYPE_RSP)
    {
        return pkt_find(p_hdr, pt_rsp_desc, PKT_RSP_CNT);
    }
#endif //(PT_RESPONSE)

    return status;
}

uint8_t pkt_crc8(uint8_t *buff, uint16_t len)
{
    uint16_t i;
    uint8_t crc = PKT_CRC_SEED;

    for (i = 0; i < len; i++)
    {
        crc ^= buff[i];
    }
    return crc;
}
#endif
