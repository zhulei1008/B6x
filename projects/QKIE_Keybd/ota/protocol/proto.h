#ifndef _PROTO_H_
#define _PROTO_H_

#include <stdint.h>
#include <stdbool.h>


/***************************************************************************
 *  Proto Macro API
 ***************************************************************************/

/// Protocol Defines
enum PT_CHNL
{
    PT_HOST,
    PT_CHAN,
    PT_CHIP,
};

enum PT_TYPE
{
    PT_TYPE_CMD          = 0x5A,
    PT_TYPE_RSP          = 0xA5,
};

enum PT_ERR_CODE
{
    PT_OK                = 0x00,

    // proto sch error A0~A4
    PT_ERROR             = 0xA0,
    PT_ERR_CRC           = 0xA0,
    PT_ERR_LEN           = 0xA1,
    PT_ERR_CODE          = 0xA2,
    PT_ERR_HEAD          = 0xA3,
    PT_ERR_TIMEOUT       = 0xA4,

    // Resv 0xA5=RSP

    // proto app error A6~AF
    PT_ERR_VERIFY        = 0xA6,
    PT_ERR_STATUS        = 0xA7,
};

/// Packet Defines @see pt_pkt
#define PKT_HDR_POS        0
#define PKT_HDR_SIZE       4
#define HDR_SYNC_POS       (PKT_HDR_POS)
#define HDR_SYNC_SIZE      (0x01)
#define HDR_CMD_POS        (PKT_HDR_POS + HDR_SYNC_SIZE)
#define HDR_CMD_SIZE       (0x01)
#define HDR_LEN_POS        (PKT_HDR_POS + HDR_SYNC_SIZE + HDR_CMD_SIZE)
#define HDR_LEN_SIZE       (0x02)

#define PKT_PAYL_POS       4
#define PAYL_ADR_SIZE      4
#define PAYL_DAT_MAX_SIZE  (0x100)
#define PAYL_CRC_SIZE      1
#define PKT_PAYL_MAX_SIZE  (PAYL_ADR_SIZE + PAYL_DAT_MAX_SIZE + PAYL_CRC_SIZE)

#define PKT_MAX_SIZE       (((PKT_HDR_SIZE + PKT_PAYL_MAX_SIZE)+3)/4)*4

#define PKT_RETRAN_MAX     (5)

enum PKT_FMT
{
    PKT_FIXLEN           = 0,
    PKT_VARLEN           = 1,
    PKT_FIXLEN_CRC       = 2,
    PKT_VARLEN_CRC       = 3,

    PKT_LEN_MSK          = 0x01,
    PKT_CRC_MSK          = 0x02,
};

/// Protocol Packet
typedef struct pt_pkt
{
    /* head(4B): 1-sync; 1-cmd; 2-len */
    uint8_t   type;
    uint8_t   code;
    uint16_t  len;

    /* payl(nB): A set of variable value */
    uint8_t   payl[];
} pkt_t;

typedef void(*parse_fnct)(struct pt_pkt *pkt, uint8_t status);

extern uint8_t pkt_crc8(uint8_t *buff, uint16_t len);

/// PT Interface
void proto_init(uint8_t chnl, parse_fnct parse);

void proto_schedule(void);


/***************************************************************************
 *  Proto Command & Response Macros
 ***************************************************************************/

enum PT_CMD_CODE
{
    /* Boot: Load Sram */
    PT_CMD_SRD           = 0x01, // Single read
    PT_CMD_SWR           = 0x02, // Single write
    PT_CMD_CRD           = 0x04, // Continue read
    PT_CMD_WR_HD         = 0x07, // Continue write header
    PT_CMD_CWR           = 0x08, // Continue write data
    PT_CMD_BAUD          = 0x0B, // Modify baudrate
    PT_CMD_ECHO          = 0x0D, // Test uart handshake
    PT_CMD_RST           = 0x10, // Reset
    PT_CMD_JUMP          = 0x13, // Jump to sram, and execute
    PT_CMD_PATCH         = 0x15, // Patch to sram, and execute
    PT_CMD_RETURN        = 0x19, // Exit Boot, return ROM

    /* Proto: Version */
    PT_CMD_VERSION       = 0x21, // Version Read

    /* Proto: Firmware */
    PT_CMD_ER_FIRM       = 0x22, // Host-Flash erase
    PT_CMD_RD_FIRM       = 0x23, // Host-Flash read
    PT_CMD_WR_FIRM       = 0x24, // Host-Flash write
    PT_CMD_VF_FIRM       = 0x25, // Host-Flash verify
    PT_CMD_MF_FIRM       = 0x26, // Host-Flash modify

    /* Proto: Sync */
    PT_CMD_SYNC          = 0x2A, // Sync code to classify

    /* Proto: Batch */
    PT_CMD_ACTION        = 0x2B, // Action(Run, Stop, Off)
    PT_CMD_CHANS         = 0x2C, // batch-8CH Detect or Run
    PT_CMD_CHSTA         = 0x2D, // batch query burner state
    PT_CMD_CHRET         = 0x2E, // Result

    /* Proto: Config */
    PT_CMD_FWCFG         = 0x31, // Firmware config info

    /* Proto: Burn */
    PT_CMD_ER_FLASH      = 0x32, // Chip-Flash erase
    PT_CMD_RD_FLASH      = 0x33, // Chip-Flash read
    PT_CMD_WR_FLASH      = 0x34, // Chip-Flash write
    PT_CMD_VF_FLASH      = 0x35, // Chip-Flash verify
    PT_CMD_MF_FLASH      = 0x36, // chipSet write ble mac
    PT_CMD_RD_OTP        = 0x37, // Chip-OTP read
    PT_CMD_WR_OTP        = 0x38, // Chip-OTP write
    PT_CMD_VF_OTP        = 0x39, // Chip-OTP verify

    /* Proto: Trim */
    PT_CMD_TRIMVAL       = 0x41, // write xosc16m trim to Flash Security Register

    /* Proto: Test */
    PT_CMD_TEST_GPIO     = 0x42, // Test
    PT_CMD_TEST_XTAL     = 0x43, // Test
    PT_CMD_TEST_RF       = 0x44, // Test
    PT_CMD_TEST_PWR      = 0x45, // Test
};

enum PT_RSP_CODE
{
    /* Proto: Version */
    PT_RSP_VERSION       = 0x21,

    /* Proto: Firmware */
    PT_RSP_ER_FIRM       = 0x22,
    PT_RSP_RD_FIRM       = 0x23,
    PT_RSP_WR_FIRM       = 0x24,
    PT_RSP_VF_FIRM       = 0x25,
    PT_RSP_MF_FIRM       = 0x26,

    /* Proto: Sync */
    PT_RSP_SYNC          = 0x2A,
    PT_RSP_ACTION        = 0x2B,

    /* Proto: Batch */
    PT_RSP_CHANS         = 0x2C,
    PT_RSP_CHSTA         = 0x2D,
    PT_RSP_CHRET         = 0x2E,

    /* Proto: Config */
    PT_RSP_FWCFG         = 0x31,

    /* Proto: Burn */
    PT_RSP_ER_FLASH      = 0x32,
    PT_RSP_RD_FLASH      = 0x33,
    PT_RSP_WR_FLASH      = 0x34,
    PT_RSP_VF_FLASH      = 0x35,
    PT_RSP_MF_FLASH      = 0x36,
    PT_RSP_RD_OTP        = 0x37,
    PT_RSP_WR_OTP        = 0x38,
    PT_RSP_VF_OTP        = 0x39,

    /* Proto: Test */
    PT_RSP_TRIMVAL       = 0x41,
    PT_RSP_TEST_GPIO     = 0x42,
    PT_RSP_TEST_XTAL     = 0x43,
    PT_RSP_TEST_RF       = 0x44,
    PT_RSP_TEST_PWR      = 0x45,

    /* Boot: Load Sram */
    PT_RSP_SRD           = 0x81,
    PT_RSP_SWR           = 0x82,
    PT_RSP_CRD           = 0x84,
    PT_RSP_WR_HD         = 0x87,
    PT_RSP_CWR           = 0x88,
    PT_RSP_BAUDRV        = 0x8B, // receive signal to modify baud
    PT_RSP_BAUDEND       = 0x8C, // modify baud rate end
    PT_RSP_ECHO          = 0x8D,
    PT_RSP_RST           = 0x90,
    PT_RSP_JUMP          = 0x93,
    PT_RSP_PATCH         = 0x95,
    PT_RSP_RETURN        = 0x99,
};

#define SIZE_FIRM_INFO     52  // sizeof(firmInfo_t)
#define SIZE_MAC_INFO      8   // sizeof(macInfo_t)
#define SIZE_TEST_INFO     8   // sizeof(testInfo_t)
#define SIZE_CHX_STATE     4   // sizeof(burner_t)

enum PAYL_LEN
{
    PLEN_CMD_NOARGS      = 0x00,  // 0
    PLEN_CMD_SIMPLE      = 0x01,  // 1byte

    PLEN_RSP_STATUS      = 0x01,  // 1byte
    PLEN_RSP_MULTIS      = 0x100, // 255payl+1crc

    /* Boot: Load Sram */
    PLEN_CMD_SRD         = 0x05,  // 4addr+1crc
    PLEN_RSP_SRD         = 0x05,  // 4data+1crc

    PLEN_CMD_SWR         = 0x09,  // 4addr+4data+1crc
    PLEN_RSP_SWR         = 0x01,  // 1status

    PLEN_CMD_CRD         = 0x07,  // 4addr+2len+1crc
    PLEN_RSP_CRD         = 0x101, // (1~PAYL_DAT_MAX_SIZE)data+1crc

    PLEN_CMD_WR_HD       = 0x07,  // 4addr+2len+1crc
    PLEN_RSP_WR_HD       = 0x01,  // 1status

    PLEN_CMD_CWR         = 0x105, // 4addr+(1~PAYL_DAT_MAX_SIZE)data+1crc
    PLEN_RSP_CWR         = 0x01,  // 1status

    PLEN_CMD_BAUD        = 0x05,  // 4baud+1crc
    PLEN_RSP_BAUDRV      = 0x01,  // 1status
    PLEN_RSP_BAUDEND     = 0x01,  // 1status

    PLEN_CMD_ECHO        = 0x01,  // 1echo
    PLEN_RSP_ECHO        = 0x02,  // 1echo+1crc

    PLEN_CMD_RST         = 0x00,  // none
    PLEN_RSP_RST         = 0x01,  // 1status

    PLEN_CMD_JUMP        = 0x05,  // 4addr+1crc
    PLEN_RSP_JUMP        = 0x01,  // 1status

    PLEN_CMD_PATCH       = 0x05,  // 4addr+1crc
    PLEN_RSP_PATCH       = 0x01,  // 1status

    PLEN_CMD_RETURN      = 0x00,  // none
    PLEN_RSP_RETURN      = 0x01,  // 1status

    /* Proto: Sync */
    PLEN_CMD_VERSION     = 0x00,  //
    PLEN_RSP_VERSION     = 0x02,  // 2version mdf0806-6vip

    PLEN_CMD_SYNC        = 0x00,  //
    PLEN_RSP_SYNC        = 0x01,  // 1code

    PLEN_CMD_ACTION      = 0x01,  // 1mode
    PLEN_RSP_ACTION      = 0x01,

    /* Proto: Batch */
    PLEN_CMD_CHANS       = 0x02,  // 1chan(00:detect, bit0~7:chan0~chan7)+1mode(0:download, 1:update)
    PLEN_RSP_CHANS       = 0x02,  // 1chan(bit0~7: chan0~chan7)+1result

    PLEN_CMD_CHSTA       = 0x01,  // 1chan(0~7: chan0~chan7)
    PLEN_RSP_CHSTA       = 0x06,  // 4state+1chan+1crc(SIZE_CHX_STATE+2)

    PLEN_CMD_CHRET       = 0x01,  //
    PLEN_RSP_CHRET       = 0x12,  // 8macret+8test+1chan+1crc(SIZE_MAC_INFO+SIZE_TEST_INFO+2)

    /* Proto: Config */
    PLEN_CMD_FWCFG       = 0x00,  //
    PLEN_RSP_FWCFG       = 0x3D,  // 52firmInfo+8macInfo+1crc(SIZE_FIRM_INFO+SIZE_MAC_INFO+1)

    /* Proto: Burn */
    PLEN_CMD_ER_FSH      = 0x0B,  // 1mode+1infocnt+(1~2)struct er_map+1crc
    PLEN_RSP_ER_FSH      = 0x01,  //

    PLEN_CMD_RD_FSH      = 0x05,  // 2pidx+2pcnt+1crc
    PLEN_RSP_RD_FSH      = 0x103, // 2pidx+(pcnt)data+1crc

    PLEN_CMD_WR_FSH      = 0x103, // 2pidx+(1~PAYL_DAT_MAX_SIZE)data+1crc
    PLEN_RSP_WR_FSH      = 0x01,  // 1status

    PLEN_CMD_VF_FSH      = 0x103, // 2pidx+(1~PAYL_DAT_MAX_SIZE)data+1crc
    PLEN_RSP_VF_FSH      = 0x01,  // 1status

    PLEN_CMD_MF_FSH      = 0x105, // 4flash_addr+(1~PAYL_DAT_MAX_SIZE)data+1crc
    PLEN_RSP_MF_FSH      = 0x01,  // 1status

    PLEN_CMD_RD_OTP      = 0x05,  // 2offset+2len+1crc
    PLEN_RSP_RD_OTP      = 0x103, // 2offset+(len)data+1crc

    PLEN_CMD_WR_OTP      = 0x103, // 2offset+(1~PAYL_DAT_MAX_SIZE)data+1crc
    PLEN_RSP_WR_OTP      = 0x01,  // 1status

    PLEN_CMD_VF_OTP      = 0x103, // 2offset+(1~PAYL_DAT_MAX_SIZE)data+1crc
    PLEN_RSP_VF_OTP      = 0x01,  // 1status

    /* Proto: Test */
    PLEN_CMD_TRIMVAL     = 0x01,  // 0x80 & xo16m_cap_trim
    PLEN_RSP_TRIMVAL     = 0x01,  // xo16m_cap_trim

    PLEN_CMD_TEST_GPIO   = 0x05,  // 4masks+1mode
    PLEN_RSP_TEST_GPIO   = 0x01,  // 1result @see IO_TEST

    PLEN_CMD_TEST_XTAL   = 0x02,  // 1mode+1gpio    mode1:freq  mode2:calc
    PLEN_RSP_TEST_XTAL   = 0x04,  // 1status or 2cnt+1arr+1trim

    PLEN_CMD_TEST_RF     = 0x02,  // 1freq+1count
    PLEN_RSP_TEST_RF     = 0x01,  // 1result

    PLEN_CMD_TEST_PWR    = 0x01,  // 1mode
    PLEN_RSP_TEST_PWR    = 0x02,  // 1status or 2result
};

/// Macro Defines
enum SYNC_OK_CODE
{
    SYNC_OK_CHIP         = 0x00,
    SYNC_OK_BOOT         = 0xA3,
    SYNC_OK_BURNER       = 0xB0,
    SYNC_OK_BATCH        = 0xBC,
};

enum XTAL_MODE
{
    XTAL_FREQ,
    XTAL_CALC,
};

enum ACT_MODE
{
    ACT_OFF,
    ACT_RUN,
    ACT_STOP,
    
    ACT_ONLINE,
    ACT_MAX,
};


/***************************************************************************
 *  Proto Command & Response Structs
 ***************************************************************************/

/* Boot Cmd */
struct pt_cmd_srd   // single read=(4addr+1crc)
{
    uint32_t addr;
};

struct pt_cmd_swr   // single write=(4addr+4data+1crc)
{
    uint32_t addr;
    uint32_t value;
};

struct pt_cmd_crd   // continue read=(4addr+2bodylen+1crc)
{
    uint32_t addr;
    uint16_t len;
};

struct pt_cmd_wr_hd // continue write header=(4addr+2bodylen+1crc)
{
    uint32_t addr;
    uint16_t len;
};

struct pt_cmd_cwr   // continue write data=(4addr+Ndata+1crc)
{
    uint32_t addr;
    uint8_t data[]; // 1~PAYL_DAT_MAX_SIZE
};

struct pt_cmd_baud  // modify baud rate=(4baud+1crc)
{
    uint32_t baud;
};

struct pt_cmd_jump  // jump to run=(4addr+1crc)
{
    uint32_t addr;
};

struct pt_cmd_patch // patch to run=(4addr+1crc)
{
    uint32_t addr;
};

struct pt_cmd_echo  // *test echo=(1data)
{
    uint8_t echo;
};

/* Boot Rsp */
struct pt_rsp_status
{
    uint8_t status;
};

struct pt_rsp_echo
{
    uint8_t echo;
    uint8_t crc;
};

struct pt_rsp_srd
{
    uint32_t value;
};

struct pt_rsp_crd
{
    uint8_t data[1];
};

/* Sync & Ver */

struct pt_rsp_version
{
    uint16_t vers;
};

struct pt_rsp_sync
{
    uint8_t code;
};

struct pt_cmd_action
{
    uint8_t mode;
};

/* Batch State */
struct pt_cmd_chans
{
    uint8_t chns;
    uint8_t mode; // @see enum chns_mode
};

struct pt_rsp_chans
{
    uint8_t chns;
    uint8_t state; // @see enum chns_state
};

struct pt_cmd_chsta
{
    uint8_t chan;
};

struct pt_rsp_chsta
{
    uint8_t psta[SIZE_CHX_STATE]; // @see burner_t
    uint8_t chan;
};

struct pt_cmd_chret
{
    uint8_t chan;
};

struct pt_rsp_chret
{
    uint8_t pmac[SIZE_MAC_INFO];  // @see macInfo_t
    uint8_t ptst[SIZE_TEST_INFO]; // @see testInfo_t
    uint8_t chan;
};

struct pt_rsp_fwcfg
{
    uint8_t info[SIZE_FIRM_INFO]; // @see firmInfo_t
    uint8_t pmac[SIZE_MAC_INFO];  // @see macInfo_t
};

/* Flash opCode */
struct er_map
{
    uint16_t eridx;
    uint16_t ercnt;
};

struct pt_cmd_er_fsh
{
    uint8_t mode;  //@see FLASH_EraseDef
    uint8_t mcnt;
    struct er_map maps[];
};

struct pt_cmd_rd_fsh
{
    uint16_t pgidx;
    uint16_t length;
};

struct pt_rsp_rd_fsh
{
    uint16_t pgidx;
    uint8_t  data[];
};

struct pt_cmd_wr_fsh
{
    uint16_t pgidx;
    uint8_t  data[];
};

struct pt_cmd_vf_fsh
{
    uint16_t pgidx;
    uint8_t  data[];
};

struct pt_cmd_mf_fsh
{
    uint32_t addr;
    uint8_t  data[];
};

/* OTP opCode */
struct pt_cmd_rd_otp
{
    uint16_t offset;
    uint16_t length;
};

struct pt_rsp_rd_otp
{
    uint16_t offset;
    uint8_t  data[];
};

struct pt_cmd_wr_otp
{
    uint16_t offset;
    uint8_t  data[];
};

struct pt_cmd_vf_otp
{
    uint16_t offset;
    uint8_t  data[];
};

/* Test opCode */
struct pt_cmd_trimval
{
    uint8_t cap;
};

struct pt_rsp_trimval
{
    uint8_t cap;
};

struct pt_cmd_test_rf
{
    uint8_t freq;
    uint8_t cont;
};

struct pt_rsp_test_rf
{
    uint8_t result;
};

struct pt_cmd_test_gpio
{
    uint32_t masks; //PA00~31
    uint8_t modes;
};

struct pt_rsp_test_gpio
{
    //uint32_t masks; //PA00~31
    uint8_t modes;
};

struct pt_cmd_test_xtal
{
    uint8_t mode;
    uint8_t gpio;
};

union pt_rsp_xtal_calc
{
    struct
    {
        uint16_t cnt;
        uint8_t  arr;
        uint8_t  trim;
    };
    uint32_t value;
};

struct pt_cmd_test_pwr
{
    uint8_t mode;
};

struct pt_rsp_test_pwr
{
    uint16_t value;
};

typedef struct 
{
	uint32_t cnt;	
	uint8_t  buff[PKT_MAX_SIZE];
}retran_t;
/***************************************************************************
 *  Proto Command & Response Function
 ***************************************************************************/

#define PKT_ALLOC(payl_len)  uint8_t buff[PKT_HDR_SIZE + payl_len]; pkt_t *pkt = (pkt_t *)buff
#define PKT_PARAM(p_struct)  p_struct *param = (p_struct *)pkt->payl

/// Command Send via UART2
extern uint8_t  pt_code;
extern uint32_t pt_time;
extern retran_t retran;

void pt_send_cmd(pkt_t *pkt);

void pt_cmd_noargs(uint8_t cmd);
void pt_cmd_simple(uint8_t cmd, uint8_t byte);

/* Boot: Load Sram */
void pt_cmd_srd(uint32_t addr);
void pt_cmd_swr(uint32_t addr, uint32_t value);
void pt_cmd_crd(uint32_t addr, uint16_t len);
void pt_cmd_wr_hd(uint32_t addr, uint16_t len);
void pt_cmd_cwr(uint32_t addr, uint8_t *data, uint16_t len);
void pt_cmd_baud(uint32_t baud);
// void pt_cmd_echo(uint8_t echo);
#define pt_cmd_echo(echo)      pt_cmd_simple(PT_CMD_ECHO, echo)
// void pt_cmd_rst(void);
#define pt_cmd_rst()           pt_cmd_noargs(PT_CMD_RST)
void pt_cmd_jump(uint32_t addr);
void pt_cmd_patch(uint32_t addr);
// void pt_cmd_return(void);
#define pt_cmd_return()        pt_cmd_noargs(PT_CMD_RETURN)

/* Proto: Sync */
// void pt_cmd_sync(void);
#define pt_cmd_sync()          pt_cmd_noargs(PT_CMD_SYNC)
// void pt_cmd_sync(void);
#define pt_cmd_version()       pt_cmd_noargs(PT_CMD_VERSION)

/* Proto: Batch */
// void pt_cmd_action(uint8_t mode);
#define pt_cmd_action(mode)    pt_cmd_simple(PT_CMD_ACTION, mode)
void pt_cmd_chans(uint8_t chan, uint8_t mode);
// void pt_cmd_chsta(uint8_t chan);
#define pt_cmd_chsta(chan)     pt_cmd_simple(PT_CMD_CHSTA, chan)
// void pt_cmd_chret(uint8_t chan);
#define pt_cmd_chret(chan)     pt_cmd_simple(PT_CMD_CHRET, chan)

/* Proto: Config */
// void pt_cmd_fwcfg(void);
#define pt_cmd_fwcfg()         pt_cmd_noargs(PT_CMD_FWCFG)

/* Proto: Burn Firm */
void pt_cmd_er_firm(uint8_t mode, uint8_t mcnt, struct er_map *maps);
void pt_cmd_wr_firm(uint16_t pgidx, uint16_t len, const uint8_t *data);
void pt_cmd_mf_firm(uint32_t addr, uint16_t len, const uint8_t *data);

/* Proto: Burn Flash */
void pt_cmd_er_flash(uint8_t mode, uint8_t mcnt, struct er_map *maps);
void pt_cmd_rd_flash(uint16_t pgidx, uint16_t len);
void pt_cmd_wr_flash(uint16_t pgidx, uint16_t len, const uint8_t *data);
void pt_cmd_vf_flash(uint16_t pgidx, uint16_t len, const uint8_t *data);
void pt_cmd_mf_flash(uint32_t addr, uint16_t len, const uint8_t *data);
/* Proto: Burn OTP */
void pt_cmd_rd_otp(uint16_t offset, uint16_t len);
void pt_cmd_wr_otp(uint16_t offset, uint16_t len, const uint8_t *data);
void pt_cmd_vf_otp(uint16_t offset, uint16_t len, const uint8_t *data);

/* Proto: Test */
// void pt_cmd_trimval(uint8_t trim);
#define pt_cmd_trimval(trim)   pt_cmd_simple(PT_CMD_TRIMVAL, trim)
// void pt_cmd_test_pwr(uint8_t mode);
#define pt_cmd_test_pwr(mode)  pt_cmd_simple(PT_CMD_TEST_PWR, mode)
void pt_cmd_test_gpio(uint32_t masks, uint8_t modes);
void pt_cmd_test_xtal(uint8_t mode, uint8_t gpio);
void pt_cmd_test_rf(uint8_t freq, uint8_t cont);


/// Response Send via UART1
void pt_send_rsp(pkt_t *pkt);

void pt_rsp_status(uint8_t rsp, uint8_t status);
void pt_rsp_multis(uint8_t rsp, uint8_t len, const void *payl);

/* Boot: Load Sram */
void pt_rsp_echo(uint8_t echo);
void pt_rsp_srd(uint32_t value);
void pt_rsp_crd(uint8_t *data, uint16_t len);

/* Proto: Sync */
// void pt_rsp_sync(uint8_t code);
#define pt_rsp_sync(code)      pt_rsp_status(PT_RSP_SYNC, code)
void pt_rsp_version(uint16_t version);

/* Proto: Batch */
void pt_rsp_chans(uint8_t chan, uint8_t state);
void pt_rsp_chsta(uint8_t chan, const void *psta);
void pt_rsp_chret(uint8_t chan, const void *pmac, const void *ptst);

/* Proto: Config */
void pt_rsp_fwcfg(const void *info, const void *pmac);

/* Proto: Burn */
void pt_rsp_rd_firm(uint16_t pgidx, uint16_t len, const uint8_t *data);
void pt_rsp_rd_flash(uint16_t pgidx, uint16_t len, const uint8_t *data);
void pt_rsp_rd_otp(uint16_t offset, uint16_t len, const uint8_t *data);
    
/* Proto: Test */
// void pt_rsp_trimval(uint8_t trim);
#define pt_rsp_trimval(trim)   pt_rsp_status(PT_RSP_TRIMVAL, trim)
void pt_rsp_xtal_calc(uint32_t value);
void pt_rsp_pwr_value(uint16_t value);

#endif // _PROTO_H_
