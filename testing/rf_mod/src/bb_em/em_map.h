/**
 ****************************************************************************************
 *
 * @file em_map.h
 *
 * @brief Mapping of the exchange memory
 *
 ****************************************************************************************
 */

#ifndef EM_MAP_H_
#define EM_MAP_H_

/**
 ****************************************************************************************
 * @addtogroup EM EM
 * @ingroup IP
 * @brief Mapping of the different common area in the exchange memory
 *
 * @{
 ****************************************************************************************
 */
/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "ble_config.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/// Exchange memory base address
#if !defined(EM_BASE_ADDR)
#define EM_BASE_ADDR                     0x20008000
#endif

/// Align value on the multiple of 4 equal or nearest higher ALIGN4_HI.
/// @param[in] val Value to align.
#define EM_ALIGN(_val)                   (((_val) + 3) & ~3)

/// Null pointer in EM offset space
#define EM_PTR_NULL                      (0x0000)

/// Retrieve Exchange memory address to set into HW interface
#define REG_EM_ADDR_GET(elem, idx)       (REG_EM_##elem##_ADDR_GET(idx) >> 2)
/// Retrieve Exchange index from address load from  HW interface
#define REG_EM_IDX_GET(elem, addr)       ((((addr)<<2) - (EM_##elem##_OFFSET))/(REG_EM_##elem##_SIZE))


/*
 ****************************************************************************************
 **********************              Common EM part                **********************
 ****************************************************************************************
 */

/// Start of the common EM part
#define EM_COMMON_OFFSET                 (0)

/*
 * EXCHANGE TABLE - 16x16
 ****************************************************************************************
 */

/// Exchange table area definition
#define EM_ET_OFFSET                     (EM_COMMON_OFFSET)
#define EM_EXCH_TABLE_LEN                16
#define REG_EM_ET_SIZE                   16
#define EM_ET_END                        (EM_ET_OFFSET + EM_EXCH_TABLE_LEN * REG_EM_ET_SIZE)
#define EM_ET_BASE_ADDR                  (EM_BASE_ADDR + EM_ET_OFFSET)

/**
 * MODEn[3:0]
 *
 *  0x0: No mode selected, nothing to be performed
 *  0x1: BR/EDR Mode
 *  0x2: BLE Mode
 *  0x3-0xF: Reserved for future use           -
 */
#define EM_ET_MODE_NONE                  0x00
#define EM_ET_MODE_BREDR                 0x01
#define EM_ET_MODE_BLE                   0x02

/// exchange table entry status
enum em_et_status
{
    /// 000: Exchange Table entry associated event is ready for processing.
    EM_ET_STATUS_READY                 = 0x00,//!< EM_ET_STATUS_READY
    /// 001: Exchange Table entry is waiting for start (already read by Event Scheduler)
    EM_ET_STATUS_UNDER_PROCESS         = 0x01,//!< EM_ET_STATUS_UNDER_PROCESS
    /// 010: Exchange Table entry associated event is started
    EM_ET_STATUS_STARTED               = 0x02,//!< EM_ET_STATUS_STARTED
    /// 011: Exchange Table entry associated event is terminated (normal termination)
    EM_ET_STATUS_TERM_NORMAL           = 0x03,//!< EM_ET_STATUS_TERM_NORMAL
    /// 100: Exchange Table entry associated event is terminated (abort termination under prio bandwidth)
    EM_ET_STATUS_TERM_ABORT_IN_PRIO_BW = 0x04,//!< EM_ET_STATUS_TERM_ABORT_IN_PRIO_BW
    /// 101: Exchange Table entry associated event is terminated (abort termination after prio bandwidth)
    EM_ET_STATUS_TERM_ABORT            = 0x05,//!< EM_ET_STATUS_TERM_ABORT
    /// 110: Exchange Table entry associated event is skipped
    EM_ET_STATUS_SKIPPED               = 0x06,//!< EM_ET_STATUS_SKIPPED
    /// 111: Reserved for future use
};

/**
 * ISOBUFSELn
 * Used to select Isochronous channel Buffer Pointer
 * 0x0: Uses ISO<0/1/2><TX/RX>PTR0
 * 0x1: Uses ISO<0/1/2><TX/RX>PTR1
 */
#define EM_ET_ISO_PTR0                   0x00
#define EM_ET_ISO_PTR1                   0x01

/**
 * ISOn
 * Indicates a Isochronous connection event is programmed
 * 0x0: Not an Isochronous Channel event
 * 0x1: Isochronous Channel event
 */
#define EM_ET_ISO_NOTPROGRAMMED          0x00
#define EM_ET_ISO_PROGRAMMED             0x01

/**
 * ISOCHANn[1:0]
 * Meaningful if ISOn = 1
 * 00: Select Isochronous channel 0.
 * 01: Select Isochronous channel 1.
 * 10: Select Isochronous channel 2.
 * 11: Trash received isochronous packet / Sent null length packet if ET-ISO = 1 / No Tx if ET-ISO = 0
 */
#define EM_ET_ISO_CHANNEL_0              0x00
#define EM_ET_ISO_CHANNEL_1              0x01
#define EM_ET_ISO_CHANNEL_2              0x02
#define EM_ET_ISO_NOCHANNEL              0x03

/**
 * RSVDn
 * Indicates a reserved event
 * 0x0: Not a reserved event (ISO connection re-Tx)
 * 0x1: Reserved event (ISO connection primary event)
 */
#define EM_ET_ISO_NOT_RSVD               0x00
#define EM_ET_ISO_RSVD                   0x01


/*
 * FREQUENCY TABLE - 40x1
 ****************************************************************************************
 */

/// Frequency table area definition
#define EM_FT_OFFSET                     (EM_ET_END)

/// number of frequencies / Depends on RF target
/// Ripple/ExtRC requires 40 x 8-bit words for Frequency table / No VCO sub-band table
#define EM_RF_FREQ_TABLE_LEN             40
#define EM_RF_VCO_TABLE_LEN              0

#define EM_FT_END                        (EM_FT_OFFSET + (EM_RF_VCO_TABLE_LEN + EM_RF_FREQ_TABLE_LEN) * sizeof(uint8_t))


/*
 * RF SW SPI - 1x8
 ****************************************************************************************
 */

/// RF SW-Driven SPI transfers area definition 
#define EM_RF_SW_SPI_OFFSET              (EM_FT_END)
#define EM_RF_SW_SPI_SIZE_MAX            8
#define EM_RF_SW_SPI_END                 (EM_RF_SW_SPI_OFFSET + EM_RF_SW_SPI_SIZE_MAX)


/*
 * RF HW SPI - 1x0
 ****************************************************************************************
 */

/// RF HW-Driven SPI transfers area definition 
#define EM_RF_HW_SPI_OFFSET              (EM_RF_SW_SPI_END)
#define EM_RF_HW_SPI_SIZE_MAX            0
#define EM_RF_HW_SPI_END                 (EM_RF_HW_SPI_OFFSET + EM_RF_HW_SPI_SIZE_MAX)


/// End of the common EM part
#define EM_COMMON_END                    (EM_RF_HW_SPI_END)


/*
 * ENCRYPTION - 16+16
 ****************************************************************************************
 */

/// Encryption area definition
#define EM_ENC_OFFSET                    EM_ALIGN(EM_COMMON_END)
#define EM_ENC_IN_OFFSET                 (EM_ENC_OFFSET)
#define EM_ENC_IN_SIZE                   (16)
#define EM_ENC_OUT_OFFSET                (EM_ENC_IN_OFFSET + EM_ENC_IN_SIZE)
#define EM_ENC_OUT_SIZE                  (16)
#define EM_ENC_END                       (EM_ENC_OFFSET + EM_ENC_IN_SIZE + EM_ENC_OUT_SIZE)


/*
 ****************************************************************************************
 **********************                BLE EM part                 **********************
 ****************************************************************************************
 */

#define EM_BLE_OFFSET                    EM_ALIGN(EM_ENC_END)

//#include "em_map_ble.h"

/*
 * CONTROL STRUCTURES - CS_NB*112
 ****************************************************************************************
 */

/**
 * Control Structures area definition
 *
 * - LINK   (N)
 * - SCAN/INIT
 */
#define EM_BLE_CS_OFFSET                 (EM_BLE_OFFSET)
#define EM_BLE_CS_NB                     (BLE_ACTIVITY_MAX + BLE_OBSERVER + BLE_CENTRAL)
#define REG_EM_BLE_CS_SIZE               112 // "CS Pointers must be 32 bits aligned"
#define EM_BLE_CS_END                    (EM_BLE_CS_OFFSET + EM_BLE_CS_NB * REG_EM_BLE_CS_SIZE)
#define EM_BLE_CS_BASE_ADDR              (EM_BASE_ADDR + EM_BLE_CS_OFFSET)

/**
 * Control Structures indexes definition
 */
#define EM_BLE_CS_ACT_ID_TO_INDEX(act_id)  (act_id)
#define EM_BLE_CS_IDX_TO_ACT_ID(cs_idx)    (cs_idx)

#if (BLE_OBSERVER)
#define EM_BLE_CS_EXT_SCAN_IDX2          (EM_BLE_CS_NB - 1)
#endif // (BLE_OBSERVER)
#if (BLE_CENTRAL)
#define EM_BLE_CS_EXT_INIT_IDX2          (EM_BLE_CS_NB - 2)
#endif // (BLE_CENTRAL)

#define EM_BLE_CS_INDEX_MAX              (EM_BLE_CS_NB)

/**
 * Frame Format
 * Bluetooth state and device activity used for frequency selection and TDD frame format. See section 3.5 for further details
 * Note this field is automatically updated by the RW-BT Core when changing from Page to Master Page Response mode.
 *
 *  - 0000x: Do not use
 *  - 00010: Master Connect
 *  - 00011: Slave Connect
 *  - 00100: Low Duty Cycle Advertiser
 *  - 00101: High Duty Cycle Advertiser
 *  - 00110: Extended Advertiser
 *  - 00111: Do not use
 *  - 01000: Passive Scanner
 *  - 01001: Active Scanner
 *  - 01010: Extended Passive Scanner
 *  - 01011: Extended Active Scanner
 *  - 0110x: Do not use
 *  - 01110: Initiator
 *  - 01111: Extended Initiator
 *  - 10xxx: Do not use
 *  - 110xx: Do not use
 *  - 11100: Tx Test Mode
 *  - 11101: Rx Test Mode
 *  - 11110: Tx / Rx Test Mode
 *  - 11111: Do not use
 */
enum em_ble_cd_fmt
{
    EM_BLE_CS_FMT_MST_CONNECT          = (0x02),
    EM_BLE_CS_FMT_SLV_CONNECT          = (0x03),
    EM_BLE_CS_FMT_LDC_ADV              = (0x04),
    EM_BLE_CS_FMT_HDC_ADV              = (0x05),
    EM_BLE_CS_FMT_EXT_ADV              = (0x06),
    EM_BLE_CS_FMT_PASSIVE_SCAN         = (0x08),
    EM_BLE_CS_FMT_ACTIVE_SCAN          = (0x09),
    EM_BLE_CS_FMT_EXT_PASSIVE_SCAN     = (0x0A),
    EM_BLE_CS_FMT_EXT_ACTIVE_SCAN      = (0x0B),
    EM_BLE_CS_FMT_CHAN_SCAN            = (0x0C),
    EM_BLE_CS_FMT_INITIATOR            = (0x0E),
    EM_BLE_CS_FMT_EXT_INITIATOR        = (0x0F),
    EM_BLE_CS_FMT_TX_TEST              = (0x1C),
    EM_BLE_CS_FMT_RX_TEST              = (0x1D),
    EM_BLE_CS_FMT_TXRX_TEXT            = (0x1E),
};

/// Maximum value of RXWINSZ field (16383)
#define EM_BLE_CS_RXWINSZ_MAX            ((1 << 14)-1)

/// Maximum value of MAXFRMTIME field (65535)
#define EM_BLE_CS_MAXEVTIME_MAX          ((1 << 16)-1)

 /// MIC Mode
enum ENC_MIC
{
    /// AES-CCM with MIC
    ENC_MIC_PRESENT = 0,
    /// AES-CCM MIC-less
    ENC_MIC_LESS    = 1,
};

/// Encryption Mode
enum ENC_MODE
{
    /// AES-CCM nonce use Packet/Payload counter
    ENC_MODE_PKT_PLD_CNT = 0,
    /// AES-CCM nonce use Event counter
    ENC_MODE_EVT_CNT     = 1,
};


/*
 * WHITE LIST - WL_NB*12
 ****************************************************************************************
 */

/// White list area definition
#define EM_BLE_WPAL_OFFSET               EM_ALIGN(EM_BLE_CS_END)
#define REG_EM_BLE_WPAL_SIZE             12 // "WPAL Pointers must be 32 bits aligned"
#define EM_BLE_WPAL_SIZE                 (BLE_WHITELIST_MAX * REG_EM_BLE_WPAL_SIZE)
#define EM_BLE_WPAL_END                  (EM_BLE_WPAL_OFFSET + EM_BLE_WPAL_SIZE)
#define EM_BLE_WPAL_BASE_ADDR            (EM_BASE_ADDR + EM_BLE_WPAL_OFFSET)


/*
 * RESOLVING LIST - RL_NB*56
 ****************************************************************************************
 */

/// Resolving list area definition
#define EM_BLE_RAL_OFFSET                EM_ALIGN(EM_BLE_WPAL_END)
#define REG_EM_BLE_RAL_SIZE              56 // "RAL Pointers must be 32 bits aligned"
#define EM_BLE_RAL_SIZE                  (BLE_RESOL_ADDR_LIST_MAX * REG_EM_BLE_RAL_SIZE)
#define EM_BLE_RAL_END                   (EM_BLE_RAL_OFFSET + EM_BLE_RAL_SIZE)
#define EM_BLE_RAL_BASE_ADDR             (EM_BASE_ADDR + EM_BLE_RAL_OFFSET)


/*
 * RX DESCRIPTORS - RX_NB*28
 ****************************************************************************************
 */

/// RX Descriptors area definition
#define EM_BLE_RX_DESC_OFFSET            EM_ALIGN(EM_BLE_RAL_END)
#define EM_BLE_RX_DESC_NB                (BLE_RX_DESC_NB)
#define REG_EM_BLE_RX_DESC_SIZE          28 // "RX Descriptors must be 32 bits aligned"
#define EM_BLE_RX_DESC_END               (EM_BLE_RX_DESC_OFFSET + EM_BLE_RX_DESC_NB * REG_EM_BLE_RX_DESC_SIZE)
#define EM_BLE_RX_DESC_BASE_ADDR         (EM_BASE_ADDR + EM_BLE_RX_DESC_OFFSET)


/*
 * TX DESCRIPTORS - TX_NB*16
 ****************************************************************************************
 */

/**
 * TX Descriptors area definition
 *
 * - N per connection
 * - 1 per advertising data buffer
 */
#define EM_BLE_TX_DESC_OFFSET            EM_ALIGN(EM_BLE_RX_DESC_END)
#define EM_BLE_TX_DESC_NB                (BLE_TX_DESC_NB)
#define REG_EM_BLE_TX_DESC_SIZE          16 // "TX Descriptors must be 32 bits aligned"
#define EM_BLE_TX_DESC_END               (EM_BLE_TX_DESC_OFFSET + EM_BLE_TX_DESC_NB * REG_EM_BLE_TX_DESC_SIZE)
#define EM_BLE_TX_DESC_BASE_ADDR         (EM_BASE_ADDR + EM_BLE_TX_DESC_OFFSET)

#define EM_BLE_TXDESC_INDEX(act_id, idx)   (BLE_NB_TX_DESC_PER_ACT * act_id + idx)


/*
 * LLCP TX BUFFERS - 2*ACT*PDU_LEN(35->24)
 ****************************************************************************************
 */

//#include "co_llcp.h"
/// LLCP TX buffers area definition
#define EM_BLE_LLCPTXBUF_OFFSET          EM_ALIGN(EM_BLE_TX_DESC_END)
#define EM_BLE_LLCPTXBUF_NB              (2*BLE_ACTIVITY_MAX)
#define EM_BLE_LLCPTXBUF_SIZE            35 //(LL_PDU_LENGTH_MAX)
#define EM_BLE_LLCPTXBUF_END             (EM_BLE_LLCPTXBUF_OFFSET + EM_BLE_LLCPTXBUF_NB * EM_BLE_LLCPTXBUF_SIZE)


/*
 * ADV EXTENDED HEADERS TX BUFFER - ACT*(39+6*(AF_NB-1))
 ****************************************************************************************
 */

/// Advertising TX buffer area definition
#define EM_BLE_ADVEXTHDRTXBUF_OFFSET     EM_ALIGN(EM_BLE_LLCPTXBUF_END)
#define EM_BLE_ADVEXTHDRTXBUF_NB         (BLE_ACTIVITY_MAX)
#if (BLE_EXT_ADV)
#define EM_BLE_ADVEXTHDRTXBUF_SIZE       (39 + 6*(BLE_ADV_FRAG_NB_TX - 1)) // See BLE LLD FS, section 6.2.1.1.3
#else
#define EM_BLE_ADVEXTHDRTXBUF_SIZE       (6) // em_cfg 6vp 20220801
#endif
#define EM_BLE_ADVEXTHDRTXBUF_END        (EM_BLE_ADVEXTHDRTXBUF_OFFSET + EM_BLE_ADVEXTHDRTXBUF_NB * EM_BLE_ADVEXTHDRTXBUF_SIZE)

#define EM_BLE_ADVEXTHDRTXBUF_OFF(act_id)  (EM_BLE_ADVEXTHDRTXBUF_OFFSET + act_id * EM_BLE_ADVEXTHDRTXBUF_SIZE)


/*
  * ADVERTISING DATA TX BUFFERS - ADV_BUF_NB*ADV_FRAG_NB*254
  ****************************************************************************************
  */

 /// Advertising data TX buffers area definition
 #define EM_BLE_ADVDATATXBUF_OFFSET      EM_ALIGN(EM_BLE_ADVEXTHDRTXBUF_END)
 #define EM_BLE_ADVDATATXBUF_NB          (BLE_ADV_BUF_NB_TX)
 #define EM_BLE_ADVDATATXBUF_SIZE        (BLE_ADV_FRAG_NB_TX*BLE_ADV_FRAG_SIZE_TX)
 #define EM_BLE_ADVDATATXBUF_END         (EM_BLE_ADVDATATXBUF_OFFSET + EM_BLE_ADVDATATXBUF_NB * EM_BLE_ADVDATATXBUF_SIZE)


/*
 * AUX_CONNECT_REQ TX BUFFER - 1x3x34
 ****************************************************************************************
 */

/// Aux Connect Req TX buffer area definition
#define EM_BLE_AUXCONNECTREQTXBUF_OFFSET  EM_ALIGN(EM_BLE_ADVDATATXBUF_END)
#define EM_BLE_AUXCONNECTREQTXBUF_NB      (1)
#define EM_BLE_AUXCONNECTREQTXBUF_SIZE    (0x66) // 3 * PDU_CON_REQ_LEN 
#define EM_BLE_AUXCONNECTREQTXBUF_END     (EM_BLE_AUXCONNECTREQTXBUF_OFFSET + EM_BLE_AUXCONNECTREQTXBUF_NB * EM_BLE_AUXCONNECTREQTXBUF_SIZE)


/*
 * ACL RX BUFFERS - BUF_NB_RX*256
 ****************************************************************************************
 */

/// Data RX buffers area definition
#define EM_BLE_DATARXBUF_OFFSET          EM_ALIGN(EM_BLE_AUXCONNECTREQTXBUF_END)
#define EM_BLE_DATARXBUF_NB              (BLE_DATA_BUF_NB_RX)
#define EM_BLE_DATARXBUF_SIZE            (128) //(256) // em_cfg 6vp 20220801
#define EM_BLE_DATARXBUF_END             (EM_BLE_DATARXBUF_OFFSET + EM_BLE_DATARXBUF_NB * EM_BLE_DATARXBUF_SIZE)


/*
 * ACL TX BUFFERS - BUF_NB_TX*256
 ****************************************************************************************
 */

/// ACL TX buffers area definition
#define EM_BLE_ACLTXBUF_OFFSET           EM_ALIGN(EM_BLE_DATARXBUF_END)
#define EM_BLE_ACLTXBUF_NB               (BLE_ACL_BUF_NB_TX)
#define EM_BLE_ACLTXBUF_SIZE             (128) //(256) // em_cfg 6vp 20220801
#define EM_BLE_ACLTXBUF_END              (EM_BLE_ACLTXBUF_OFFSET + EM_BLE_ACLTXBUF_NB * EM_BLE_ACLTXBUF_SIZE)


/*
 * ISO TX/RX Descriptors
 ****************************************************************************************
 */
 
#if (BLE_ISO_PRESENT)
#define REG_EM_BLE_TX_ISO_DESC_SIZE      12
#define REG_EM_BLE_RX_ISO_DESC_SIZE      12
#define REG_EM_BLE_TX_ISO_BUF_SIZE       68
#define REG_EM_BLE_RX_ISO_BUF_SIZE       68

/**
 * ISO Descriptors area definition
 * Pool to be used for TX and RX ISO descriptors. It is assumed that both kind of descriptors
 * have exactly the same length.
 * For each ICO channel or ISO Mode 0 connection, 2 RX ISO descriptors and 2 TX ISO descriptors will be taken from this pool
 * For each ICL channel, either 4 RX ISO descriptors or 4 TX ISO descriptors will be taken from this pool.
 */
#define EM_BLE_ISO_DESC_OFFSET           EM_ALIGN(EM_BLE_ACLTXBUF_END)
#define EM_BLE_ISO_DESC_NB               (BLE_ISO_DESC_NB)
#define EM_BLE_ISO_DESC_SIZE             (REG_EM_BLE_TX_ISO_DESC_SIZE)
#define EM_BLE_ISO_DESC_END              (EM_BLE_ISO_DESC_OFFSET + EM_BLE_ISO_DESC_NB * EM_BLE_ISO_DESC_SIZE)

/// !!!! REMINDER: TX and RX ISO descriptors have the same size !!!! Can use REG_EM_BLE_TX_ISO_DESC_SIZE and REG_EM_BLE_RX_ISO_DESC_SIZE
#define EM_BLE_TX_ISO_DESC_OFFSET        (EM_BLE_ISO_DESC_OFFSET)
#define EM_BLE_RX_ISO_DESC_OFFSET        (EM_BLE_ISO_DESC_OFFSET)
#if (REG_EM_BLE_TX_ISO_DESC_SIZE != REG_EM_BLE_RX_ISO_DESC_SIZE)
#error "TX and RX ISO descriptors shall have the same size"
#endif // (REG_EM_BLE_TX_ISO_DESC_SIZE != REG_EM_BLE_RX_ISO_DESC_SIZE)
#if ((REG_EM_BLE_TX_ISO_DESC_SIZE % 4) != 0)
#error "TX/RX ISO Descriptor must be 32 bits aligned"
#endif // ((REG_EM_BLE_TX_ISO_DESC_SIZE % 4) != 0)

/*
 * ISO TX/RX DATA BUFFERS
 ****************************************************************************************
 */
/// RX/TX Isochronous buffer area definition
#define EM_BLE_ISO_BUF_OFFSET            EM_ALIGN(EM_BLE_ISO_DESC_END)
#define EM_BLE_ISO_BUFFER_NB             (BLE_ISO_BUF_NB)
#define EM_BLE_ISO_BUFFER_SIZE           (REG_EM_BLE_TX_ISO_BUF_SIZE)
#define EM_BLE_ISO_BUFFER_END            (EM_BLE_ISO_BUF_OFFSET + EM_BLE_ISO_BUFFER_NB * EM_BLE_ISO_BUFFER_SIZE)

/// !!!! REMINDER: TX and RX ISO buffer have the same size !!!! Can use REG_EM_BLE_TX_ISO_BUF_SIZE and REG_EM_BLE_RX_ISO_BUF_SIZE
#define EM_BLE_TX_ISO_BUF_OFFSET         (EM_BLE_ISO_BUF_OFFSET)
#define EM_BLE_RX_ISO_BUF_OFFSET         (EM_BLE_ISO_BUF_OFFSET)
#if (REG_EM_BLE_TX_ISO_BUF_SIZE != REG_EM_BLE_RX_ISO_BUF_SIZE)
#error "TX and RX ISO buffer shall have the same size"
#endif // (REG_EM_BLE_TX_ISO_DESC_SIZE != REG_EM_BLE_RX_ISO_DESC_SIZE)
#if ((REG_EM_BLE_TX_ISO_BUF_SIZE % 4) != 0)
#error "TX/RX ISO Buffers must be 32 bits aligned"
#endif // ((REG_EM_BLE_TX_ISO_BUF_SIZE % 4) != 0)

#else
#define EM_BLE_ISO_BUFFER_END            (EM_BLE_ACLTXBUF_END)
#endif // (BLE_ISO_PRESENT)


/*
 * RX DESCRIPTORS FOR DIRECTION FINDING
 ****************************************************************************************
 */

/// RX CTE Descriptors area definition
#define EM_BLE_RX_CTE_DESC_OFFSET        EM_ALIGN(EM_BLE_ISO_BUFFER_END)
#define REG_EM_BLE_RX_CTE_DESC_SIZE      168
#if (BLE_CON_CTE_REQ | BLE_CONLESS_CTE_RX)
#define EM_BLE_RX_CTE_DESC_NB            (BLE_RX_CTE_DESC_NB)
#define EM_BLE_RX_CTE_DESC_END           (EM_BLE_RX_CTE_DESC_OFFSET + EM_BLE_RX_CTE_DESC_NB * REG_EM_BLE_RX_CTE_DESC_SIZE)
#else // (BLE_CON_CTE_REQ | BLE_CONLESS_CTE_RX)
#define EM_BLE_RX_CTE_DESC_END           (EM_BLE_RX_CTE_DESC_OFFSET)
#endif // (BLE_CON_CTE_REQ | BLE_CONLESS_CTE_RX)
#define EM_BLE_RX_CTE_DESC_BASE_ADDR     (EM_BASE_ADDR + EM_BLE_RX_CTE_DESC_OFFSET)


/*
 * ANTENNA IDs FOR DIRECTION FINDING
 ****************************************************************************************
 */

/// Area definition for Tx antenna IDs
#define EM_BLE_TX_ANTENNA_ID_OFFSET      EM_ALIGN(EM_BLE_RX_CTE_DESC_END)
#if (BLE_AOA | BLE_AOD)
#define EM_BLE_TX_ANTENNA_ID_NB          (BLE_MAX_SW_PAT_LEN)
#define EM_BLE_TX_ANTENNA_ID_END         (EM_BLE_TX_ANTENNA_ID_OFFSET + EM_BLE_TX_ANTENNA_ID_NB)
#else // (BLE_CON_CTE_REQ | BLE_CONLESS_CTE_RX)
#define EM_BLE_TX_ANTENNA_ID_END         (EM_BLE_TX_ANTENNA_ID_OFFSET)
#endif // (BLE_CON_CTE_REQ | BLE_CONLESS_CTE_RX)

/// Area definition for Rx antenna IDs
#define EM_BLE_RX_ANTENNA_ID_OFFSET      EM_ALIGN(EM_BLE_TX_ANTENNA_ID_END)
#if (BLE_AOA | BLE_AOD)
#define EM_BLE_RX_ANTENNA_ID_NB          (BLE_MAX_SW_PAT_LEN)
#define EM_BLE_RX_ANTENNA_ID_END         (EM_BLE_RX_ANTENNA_ID_OFFSET + EM_BLE_RX_ANTENNA_ID_NB)
#else // (BLE_CON_CTE_REQ | BLE_CONLESS_CTE_RX)
#define EM_BLE_RX_ANTENNA_ID_END         (EM_BLE_RX_ANTENNA_ID_OFFSET)
#endif // (BLE_CON_CTE_REQ | BLE_CONLESS_CTE_RX)


/*
 * END
 ****************************************************************************************
 */
#define EM_BLE_END                       (EM_BLE_RX_ANTENNA_ID_END)


/// @} IPDEXMEM

#endif // EM_MAP_H_
