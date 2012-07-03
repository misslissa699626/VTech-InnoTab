#ifndef DRV_L2_NAND_APP_H
#define DRV_L2_NAND_APP_H

//#include "../gp_nand_hal/hal_nand.h"

#include "drv_l2_app_hal.h"

//--------------------------------------------------------
#define NAND_GOOD_BLK							0xffff
#define NAND_ALL_BIT_FULL					0xffff
#define NAND_ALL_BIT_FULL_B				0xff
#define NAND_ALL_BIT_FULL_W				0xffff

#define NAND_GOOD_TAG							0xff
#define	NAND_ORG_BAD_BLK					0xff00
#define NAND_ORG_BAD_TAG					0x00
#define	NAND_USER_BAD_BLK					0xff44
#define	NAND_USER_BAD_TAG					0x44

#define NAND_MAPTABLE_BLK					0xff66		// word
#define NAND_MAPTABLE_TAG					0xff66		// word


//------------C Area Flag define-----------------------
#define NAND_C_AREA_BAD_FLG_OFS_1					(0x00)	// 1 byte
#define NAND_C_AREA_COUNT_OFS							(0x01)  // 1 byte
#define NAND_C_AREA_LOGIC_BLK_NUM_OFS			(0x02)	// 2 byte
#define NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS	(0x04)	// 1 byte
#define NAND_C_AREA_BAD_FLG_OFS_6					(0x05)	// 1 byte
#define NAND_C_AREA_PAGE_TO_PAGE_OFS			(0x06)	// 2 byte

typedef struct 	
{
	UINT16	wLogicBlk;
	UINT16	wPhysicBlk;
}L2P;

#ifndef _NF_STATUS
#define _NF_STATUS
typedef enum
{
	NF_OK 										= 0x0000,
	NF_UNKNOW_TYPE						= 0xff1f,
	NF_USER_CONFIG_ERR				= 0xff2f,
	NF_TOO_MANY_BADBLK				= 0xff3f,
	NF_READ_MAPTAB_FAIL				= 0xff4f,
	NF_SET_BAD_BLK_FAIL				= 0xff5f,
	NF_NO_SWAP_BLOCK					= 0xff6f,
	NF_BEYOND_NAND_SIZE				= 0xff7f,
	NF_READ_ECC_ERR						= 0xff8f,
	NF_READ_BIT_ERR_TOO_MUCH	= 0xff9f,
	NF_ILLEGAL_BLK_MORE_2			= 0xffaf,
	NF_BANK_RECYCLE_NUM_ERR		= 0xffbf,
	NF_EXCHAGE_BLK_ID_ERR			= 0xffcf,
	NF_DIRECT_READ_UNSUPPORT	= 0xffdf,
	NF_DMA_ALIGN_ADDR_NEED		= 0xfff0,
	NF_APP_LOCKED							= 0xfff1,
	NF_NO_BUFFERPTR						= 0xfff2,
	NF_FIX_ILLEGAL_BLK_ERROR	= 0xfff3,
	NF_FORMAT_POWER_LOSE			= 0xfff4,
	NF_NAND_PROTECTED					= 0xfff5,
	NF_FIRST_ALL_BANK_OK			= 0xffef
}NF_STATUS;
#endif

typedef struct
{
	UINT16    wPageSectorMask;
	UINT16    wPageSectorSize;
	UINT16    wBlk2Sector;
	UINT16    wBlk2Page;
	UINT16    wPage2Sector;
	UINT16    wSector2Word;
	UINT32    wNandBlockNum;
	UINT16    wPageSize;
	UINT16    wBlkPageNum;
//	UINT16	  wBadFlgByteOfs;
//	UINT16	  wBadFlgPage1;
//	UINT16	  wBadFlgPage2;	
}NF_APP_HAL_INFO;

typedef struct
{	
	UINT16	uiAppStart;
	UINT16 	uiAppSize;
	UINT16 	uiAppSpareStart;
	UINT16  uiAppSparePercent;
	UINT16 	uiAppSpareSize;
	UINT16 	uiAppSpareEnd;
}NF_APP_CONFIG_INFO;

#define BYTE_WORD_SHIFT 0

extern UINT32 NandAppFormat_Step(UINT16 format_type, UINT16 format_step);
extern SINT32 NandAppWriteSector(UINT32 wWriteLBA, UINT16 wLen, UINT32 DataBufAddr,UINT8 mode);
extern SINT32 NandAppReadSector(UINT32 wWriteLBA, UINT16 wLen, UINT32 DataBufAddr,UINT8 mode);
extern UINT32 NandAppFormat(UINT16 format_type);
extern SINT32 NandAppInit(void);
extern SINT32 NandAppUninit(void);
extern void NandAppDisableWrite(void);
extern void NandAppEnableWrite(void);
extern SINT32 NandAppFlush(void);
extern void NandAPPGetNandInfo(UINT32 *blk_size,UINT32 *page_size);
extern void NandAPPGetProcess(UINT32 *target_sec,UINT32 *xsfered_sec);

extern void GetNandAppCfgInfo(NF_APP_CONFIG_INFO *cfg_info);
extern UINT16 GetAPPAreaStartBlock(void);
extern UINT16 GetAPPAreaSizeBlock(void);
extern UINT16 GetAPPAreaSparePercent(void);
extern SINT32 Nand_OS_Init(void);
extern void Nand_OS_LOCK(void);
extern void Nand_OS_UNLOCK(void);
extern SINT32 GetNandConfigInfo(void);

void DMAmmCopy(UINT32 wSourceAddr, UINT32 wTargeAddr, UINT16 Count);
void DMAmmCopyToUser(UINT32 wSourceAddr, UINT32 wTargeAddr, UINT16 Count);
void DMAmmCopyFromUser(UINT32 wSourceAddr, UINT32 wTargeAddr, UINT16 Count);
void APPPutCArea(UINT16 byte_ofs,UINT16 data);
void AppSetBadFlag(UINT16 wPhysicBlkNum, UINT16 ErrFlag);

UINT16 GetMapTblInfo(UINT16 wPhysicBlk);
UINT32 AppReadPhysicPage(UINT32 wPhysicPageNum,UINT32 DataAddr);
UINT16 APPGetCArea(UINT16 byte_ofs);
UINT32 GetNFInfo(void);
UINT16 AppCheckOrgBadBlk(UINT16 wPhysicBlkNum);
UINT32 GetSwapPhysicBlock(void);
SINT32 PutSwapPhysicBlock(UINT16 wPhysicBlock);
SINT32 UpdateL2PTbl(UINT16 wLogicNum,UINT16 wPhysicNum);
UINT8 *AppGetWorkbuffer(UINT8 **ptr,UINT32 size);
UINT32 AppErasePhysicBlock(UINT32 wPhysicBlkNum);
UINT32 AppWritePhysicPage(UINT32 wPhysicPageNum,UINT32 DataAddr);
UINT32 AppReadPhysicPage(UINT32 wPhysicPageNum,UINT32 DataAddr);
SINT16 AppCopyPageToPage(UINT16 wSrcBlock,UINT16 wTargetBlock,UINT16 wPage);
UINT16 GetPhysicBlockNum(UINT16 wLogicNum);
SINT16 AppEraseLogicBlk(UINT16 wLogicBlk);
UINT16 GetBitErrCntAsBadBlk(void);

#endif  //#ifndef DRV_L2_NAND_APP_H
