#ifndef DRV_L2_NAND_HAL_H
#define DRV_L2_NAND_HAL_H

#include <mach/gp_chunkmem.h>
#include <mach/hardware.h>
#include <asm/cacheflush.h>
#include <mach/typedef.h>

typedef enum
{
    BCH1K60_BITS_MODE=0x00,  // phy register define, fixed
    BCH1K40_BITS_MODE=0x01,  // phy register define, fixed
    BCH1K24_BITS_MODE=0x02,  // phy register define, fixed
    BCH1K16_BITS_MODE=0x03,  // phy register define, fixed
    BCH512B8_BITS_MODE=0x04,  // phy register define, fixed
    BCH512B4_BITS_MODE=0x05,  // phy register define, fixed
    BCH_OFF=0xFF
} BCH_MODE_ENUM;

typedef enum {
    NF_PROGRAM_DENY 		=	0x00,  /*Write Protect*/
    NF_PROGRAM_ERR7 		=	0x01,  /*Write Protect*/
    NF_PROGRAM_ERR8 		=	0x02,  /*Write Protect*/
    NF_PROGRAM_ERR9 		=	0x03,  /*Write Protect*/  
    NF_PROGRAM_ERR10		=	0x04,  /*Write Protect*/ 
    NF_PROGRAM_ERR11		=	0x05,  /*Write Protect*/
    NF_PROGRAM_ERR12		=	0x06,  /*Write Protect*/  
    NF_PROGRAM_ERR13		=	0x07,  /*Write Protect*/
    NF_PROGRAM_WELL 		=	0x80,  /*Program Well*/
    NF_PROGRAM_ERR0 		=	0x81,  /*Program Error*/
    NF_PROGRAM_ERR1 		=	0x82,  /*Program Error1*/
    NF_PROGRAM_ERR2 		=	0x83,  /*Program Error2*/
    NF_PROGRAM_ERR3 		=	0x84,  /*Program Error3*/  
    NF_PROGRAM_ERR4 		=	0x85,  /*Program Error4*/
    NF_PROGRAM_ERR5 		=	0x86,  /*Program Error5*/
    NF_PROGRAM_ERR6 		=	0x87,   /*Program Error6*/
    NF_PROGRAM_TIME_OUT =	0xFF
} NFCMD_STATUS;


extern SINT32 Nand_Init(void);
extern SINT32 Nand_UnInit(void);
extern void Nand_Getinfo(UINT16* PageSize,UINT16* BlkPageNum,UINT32* NandBlockNum);
extern SINT32 Nand_ErasePhyBlock(UINT32 wPhysicBlkNum);
extern UINT32 Nand_ReadPhyPage(UINT32 wPhysicPageNum, UINT32 WorkBuffer);
extern UINT32 Nand_WritePhyPage(UINT32 wPhysicPageNum, UINT32 WorkBuffer);
extern NFCMD_STATUS nand_write_status_get(void);
extern void spare_flag_set_L(UINT32 spare_flag_L);
extern void spare_flag_set_H(UINT32 spare_flag_H);
extern UINT32 spare_flag_get_L(void);
extern UINT32 spare_flag_get_H(void);
extern void DrvNand_WP_Disable(void);
extern void DrvNand_WP_Enable(void);
extern void DrvNand_WP_Initial(void);

extern SINT32 NandParseBootHeader(UINT8 *Buffer);
extern UINT16 GetAppStartBlkFromBth(void);
extern UINT16 GetAppSizeOfBlkFromBth(void);
extern UINT16 GetAppPercentFromBth(void);
extern UINT16 GetDataBankLogicSizeFromBth(void);
extern UINT16 GetDataBankRecycleSizeFromBth(void);
extern UINT32 GetNoFSAreaSectorSizeFromBth(void);
extern UINT8 GetPartNumFromBth(void);
extern void GetPartInfoFromBth(UINT8 part,UINT16 *MB_size,UINT8 *attr);

extern UINT16 nand_bch_err_bits_get(void);
extern void bch_mode_set(BCH_MODE_ENUM bch_set);
extern BCH_MODE_ENUM bch_mode_get(void);
extern UINT16 get_bch_mode(void);
extern BCH_MODE_ENUM nand_bch_get(void);

extern SINT16 good_block_check(UINT16 blk_id, UINT32 WorkBuffer);
extern SINT32 Nand_sw_bad_block_set(UINT32 blk_id,UINT32 page_buf_addr,UINT8 sw_bad_flag);

extern void dump_buffer(unsigned char *addr, unsigned long size);
extern UINT8 *nand_get_spare_buf(void);

#endif  
