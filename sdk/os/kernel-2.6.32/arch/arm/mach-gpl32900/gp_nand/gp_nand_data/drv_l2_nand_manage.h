#ifndef __drv_l2_NAND_MANAGE_H__
#define __drv_l2_NAND_MANAGE_H__

//#define PART0_WRITE_MONITOR_DEBUG	//Specail debug define for nand test
//#define ENABLE_FULL_MONITOR_DEBUG

#ifndef ENABLE_FULL_MONITOR_DEBUG
	#ifdef PART0_WRITE_MONITOR_DEBUG
		#define ENABLE_FULL_MONITOR_DEBUG
	#endif
#endif

//#include "../gp_nand_hal/hal_nand.h"
#include "../gp_nand_config/drv_l2_nand_config.h"
#include "drv_l2_data_hal.h"

//------------C Area Flag define-----------------------
//--------NAND READ WRITE AREA define---------------------
#define NAND_C_AREA_BAD_FLG_OFS_1							(0x00)	// 1 byte
#define NAND_C_AREA_COUNT_OFS									(0x01)  // 1 byte
#define NAND_C_AREA_LOGIC_BLK_NUM_OFS					(0x02)	// 2 byte
#define NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS			(0x04)	// 1 byte
#define NAND_C_AREA_BAD_FLG_OFS_6							(0x05)	// 1 byte
#define NAND_C_AREA_PAGE_TO_PAGE_OFS					(0x06)	// 2 byte
#define NAND_C_AREA_HAVE_MANAGE_TAG_LOCATION	(0x08)// 2 byte	// 8K Nand
//---------------------------------------------------------
#define NF_READ																0x01
#define NF_WRITE															0x10
//----------------------------------------------------------
#define	NAND_BUF_FLUSH_ALL										(1<<0)		//force to flush A exchange , become all free		
#define	NAND_BUF_GET_EXCHANGE									(1<<1)		//get a block 
#define	NAND_ALL_BUF_FLUSH_ALL								(1<<2)		//get a block 
//--------------------------------------------------------
#define NAND_ALL_BIT_FULL_B										0xff
#define NAND_ALL_BIT_FULL_W										0xffff
#define NAND_MAPTAB_UNUSE											0x7fff
#define NAND_EMPTY_BLK												0xffff
#define NAND_GOOD_TAG													0xff
#define	NAND_ORG_BAD_BLK											0xff00
#define NAND_ORG_BAD_TAG											0x00
#define	NAND_USER_BAD_BLK											0xff44
#define	NAND_USER_BAD_TAG											0x44
#define	NAND_USER_UNSTABLE_BLK								0xff77
#define	NAND_USER_UNSTABLE_TAG								0x77
#define NAND_FIRST_SCAN_BAD_BLK								0xff55
#define NAND_FIRST_SCAN_BAD_TAG								0x55

#define NAND_MAPTABLE_BLK											0xff66		// word
#define NAND_MAPTABLE_TAG											0xff66		// word

#define NAND_EXCHANGE_A_TAG										0xaa
#define NAND_EXCHANGE_B_TAG										0xbb
#define NAND_BANK_INFO_TAG										0xcc
#define NF_EXCHANGE_A_TYPE										NAND_EXCHANGE_A_TAG
#define NF_EXCHANGE_B_TYPE										NAND_EXCHANGE_B_TAG

//---------------------------------------------------------------------

#define NAND_FORMAT_START_TAG							0xdd

#define MAPTAB_WRITE_BACK_N_NEED					0x01
#define MAPTAB_WRITE_BACK_NEED						0x10
#define MAPTAB_N_DIRTY										0x01
#define MAPTAB_DIRTY											0x10
#define MAPTAB_VALID											0x01
#define MAPTAB_INVALID										0x10
#define NAND_FREE_BLK_MARK_BIT						(1<<15)
//#define NF_APP_LOCK                                  0xfffe
#define NF_PHY_WRITE_FAIL                 0x10
#define NF_PHY_READ_FAIL                  0x01

#define MAX_BAD_BLK												0x20
#define NAND_Illegal_BLK_SIZE							8
#define ExchangeAMaxNum										2
#define ExchangeBMaxNum										3

#define NAND_READ_ALLOW	    							0x1
#define NAND_WRITE_ALLOW									0x2

#define BYTE_WORD_SHIFT 									0

#define MAX_PART_NUM										8
#define MAX_BANK_NUM										30
#define MAX_PAGE_PER_BLOCK									256

typedef struct
{
	UINT16	wPhysicBlkNum;
	UINT16	wLogicBlkNum;
	UINT16  wCount;
}IBT;

typedef struct
{	
	UINT16	wLogicBlkNum;
	UINT16	wRecycleBlkNum;
	UINT16	wMapTableBlk;
	UINT16	wMapTablePage;
	UINT16  wOrgBlkNum;
	UINT16	wUserBadBlkNum;
	UINT16	wExchangeBlkNum;
	UINT16  wStatus;
}BANK_INFO;

typedef struct
{
	UINT16	Logic2Physic[512];
	UINT16	RecycleBlk[200];
}IMM;

typedef union 
{
	IMM 		Maptable;	
	UINT16  pMaptable[512+200];
}MM;

typedef struct 
{
	UINT16 	uiDataStart;
	UINT16	uiBankSize;
	UINT16	uiBankRecSize;
	UINT16	uiBankNum;
	UINT16	uiMM_Num;	
}NF_DATA_CONFIG_INFO;


typedef struct
{
	UINT16	wBank;
	UINT16	wLogicBlk;
	UINT16	wPhysicBlk;
	UINT16	wCurrentPage;	
	UINT16	wLogicalPage;
	UINT16	wCount;	
	UINT16	wType;
	UINT16	wPage2PageTable[MAX_PAGE_PER_BLOCK];	// Èç¹ûÓÐ256 Page/Block
}EXCHANGE_A_BLK;

#ifndef _NF_STATUS
#define _NF_STATUS

typedef enum
{
	NF_OK 						= 0x0000,
	NF_UNKNOW_TYPE				= 0xff1f,
	NF_USER_CONFIG_ERR			= 0xff2f,
	NF_TOO_MANY_BADBLK			= 0xff3f,
	NF_READ_MAPTAB_FAIL			= 0xff4f,
	NF_SET_BAD_BLK_FAIL			= 0xff5f,
	NF_NO_SWAP_BLOCK			= 0xff6f,
	NF_BEYOND_NAND_SIZE			= 0xff7f,
	NF_READ_ECC_ERR				= 0xff8f,
	NF_READ_BIT_ERR_TOO_MUCH    = 0xff9f,
	NF_ILLEGAL_BLK_MORE_2		= 0xffaf,
	NF_BANK_RECYCLE_NUM_ERR		= 0xffbf,
	NF_EXCHAGE_BLK_ID_ERR		= 0xffcf,
	NF_DIRECT_READ_UNSUPPORT	= 0xffdf,
	NF_DMA_ALIGN_ADDR_NEED		= 0xfff0,
	NF_APP_LOCKED				= 0xfff1,
	NF_NO_BUFFERPTR				= 0xfff2,
	NF_FIX_ILLEGAL_BLK_ERROR	= 0xfff3,
	NF_FORMAT_POWER_LOSE		= 0xfff4,
	NF_NAND_PROTECTED			= 0xfff5,
	NF_FIRST_ALL_BANK_OK        = 0xffef
}NF_STATUS;
#endif //_NF_STATUS;

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
	//UINT16	  wBadFlgByteOfs;
	//UINT16	  wBadFlgPage1;
	//UINT16	  wBadFlgPage2	
}NF_DATA_HAL_INFO;

typedef struct
{
	UINT16 bad_blk[MAX_BAD_BLK];
	UINT16 wArrayIndex;
}BAD_BLK_ARRAY;

typedef struct
{
	UINT32 FATStart;
	UINT32 DATAStart;
}PART_FAT_INFO;

extern PART_FAT_INFO   part_info[MAX_PARTITION_NUM];

extern UINT32				gWorkBufferNum;

extern  BANK_INFO  	gBankInfo[MAX_BANK_NUM];

extern  MM					mm[1];
extern UINT16 				gMaptableChanged[1];

extern EXCHANGE_A_BLK		gExchangeABlock[ExchangeAMaxNum];
extern EXCHANGE_A_BLK		gExchangeBBlock[ExchangeBMaxNum];
extern EXCHANGE_A_BLK		*gpWorkBufferBlock;


extern NF_DATA_CONFIG_INFO 		gSTNandDataCfgInfo;
extern NF_DATA_HAL_INFO 			gSTNandDataHalInfo; 
extern UINT8 					*tt_basic_ptr;
extern UINT8 					*tt_extend_ptr;

//extern BAD_BLK_OFFSET 		gbadflag_info;
extern BAD_BLK_ARRAY 		bad_blk_info;
extern UINT16				gLogic2PhysicAreaStartBlk;
extern UINT32				gBankInfoCurPage;
extern UINT16    			gCurrentBankNum;
extern UINT16 				gTotalBankNum;
extern UINT32 				gLastPage_Write;
extern UINT32 				gLastPage_Read;
extern UINT32				gLastPage_Error;
extern UINT16       		gInitialFlag;
extern UINT16          		gDataPhyBlkNum;
extern UINT32 				gNandSize;
//extern UINT32 				gFirstFATStart;
//extern UINT32 				gFirstFATDataStart;
//extern UINT32 				gSecondFATStart;
//extern UINT32 				gSecondFADataStart;
extern const UINT32 		FATNandTable[];


//////////////////------  Data Funcitons Decalre  ///////////////////////
extern UINT16 DrvNand_initial_Update(void);
extern SINT16 FixBankExchangeBlk(UINT16 wTargetBankNum,UINT32 wBankStartBlk,UINT32 wBankEndBlk);
extern void GlobalVarInit(void);
extern UINT16 FindMapTableCurrentPage(UINT16 wPhysicBlk);
extern SINT32 GetNandParam(void);
extern UINT32 DrvNand_get_Size(void);
extern UINT16 GetBadFlagFromNand(UINT16 wPhysicBlkNum);
extern SINT32 SetBadFlagIntoNand(UINT16 wPhysicBlkNum, UINT16 ErrFlag);
extern UINT16 ReadLogicNumFromNand(UINT16 wPhysicBlkNum);
extern SINT16 ReadMapTableFromNand(UINT16 wBankNum);
extern UINT16 WriteMapTableToNand(UINT16 wBankNum);
extern SINT16 ChangeBank(UINT16 wBankNum);
extern void FlushCurrentMaptable(void);
extern void UpdateMaptable(UINT16 wSwapBlkNum, UINT16 wLogicNum);
extern UINT16 GetFreeBlkFromRecycleFifo(void);
extern SINT32 PutFreeBlkIntoRecycleFifo(UINT16 wPhysicBlkNum);
extern SINT32 NandFlushA(UINT16 wBlkIndex, UINT16 TargetBank, SINT32 wMode, UINT16 wType);

extern SINT32 CopyMultiPhysicalPage(UINT16 wSRCBlkNum, UINT16 wTARBlkNum, UINT16 wStartPage, UINT16 wEndPage, UINT16 wLogicBlkNum,UINT16 wType);
extern SINT32 ReadDataFromNand(UINT32 wPhysicPageNum, UINT8 *tt_ptr);
extern SINT32 WriteDataIntoNand(UINT32 wPhysicPageNum,UINT8 *tt_ptr);
extern SINT32 EraseNandBlk(UINT16 wPhysicBlkNum);
extern SINT16 FixupIllegalBlock(UINT16* pIllegalTable , UINT16 wCount);
extern UINT16 FindBlkCurrentPage(EXCHANGE_A_BLK *Blk,SINT16 *ecc_status);

extern SINT32 WriteBackWorkbuffer(EXCHANGE_A_BLK *pBlk);
extern UINT16 GetExchangeBlock(UINT16 wType, UINT16 wBank, UINT16 wLogicBlk);
extern SINT32 ReadNandAreaA(UINT32 wReadLBA, UINT16 wLen, UINT32 DataBufAddr,UINT16 wReadType,UINT8 mode);
extern SINT32 WriteNandAreaA(UINT32 wWriteLBA, UINT16 wLen, UINT32 DataBufAddr,UINT16 wWriteType,UINT8 mode);
extern UINT16 DrvNand_read_sector(UINT32 wReadLBA, UINT16 wLen, UINT32 DataBufAddr,UINT8 mode);
extern UINT16 DrvNand_write_sector(UINT32 wWriteLBA, UINT16 wLen, UINT32 DataBufAddr,UINT8 mode);
extern UINT16 NandXsfer(UINT32 wLBA, UINT16 wLen, UINT32 wAddr,UINT16 XsferMode,UINT8 mode);

extern UINT16 GetCArea(UINT16 byte_ofs,UINT8 *WorkBuffer);

extern void PutCArea(UINT16 byte_ofs,UINT16 data);
extern void SaveBadBlkInfo(UINT16 wBlkNum);
extern UINT16 ChangeReadErrBlk(void);
extern UINT16 IsBlk_GoodOrBad(UINT16 wblknum, UINT16 wblktemp);
extern SINT32 Set_CalculateFATArea (UINT16 partnum, UINT32 DataStartSec);
extern SINT32 Default_CalculateFATArea (void);
extern UINT8 SetWorkBuffer(UINT8* p_tt_basic, UINT8* p_tt_extend);
//extern UINT32 combin_reg_data(UINT8 *data,SINT32 len);
extern UINT16	GetNextGoodBlock(UINT16 cur_block);
extern SINT16	BankInfoAreaInit(void);
extern SINT16	BankInfoAreaRead(UINT32 pBuf, UINT32 bytes);
extern SINT16	BankInfoAreaWrite(UINT32 pBuf, UINT32 bytes);

extern SINT32 Nand_OS_Init(void);
extern SINT32 Nand_OS_Exit(void);
extern void Nand_OS_LOCK(void);
extern void Nand_OS_UNLOCK(void);
extern void Sys_Exception(SINT32 errorcode);
//extern void SetNandInfo(UINT16 app_start,UINT16 app_size,UINT16 app_Percent,  UINT16 data_BankSize, UINT16 data_BankRecSize);
extern void Nand_Getinfo(UINT16* PageSize,UINT16* BlkPageNum,UINT32* NandBlockNum);
extern void nf_memset_w(void *address, UINT16 value, UINT16 len);
extern void nf_memcpy_w(void *taraddr, void *srcaddr, UINT16 len);
extern UINT32 CalculateFATArea(void);
extern UINT32 GetNandBadBlkNumber(UINT16 wBadType);
extern UINT32 DrvNand_RecoveryBlk(void);
extern SINT32 NandSetOrgLogicBlock(UINT16 wLogicBlk,UINT16 wType);
extern UINT16 GetBitErrCntAsBadBlk(void);
extern void FlushWorkbuffer_NoSem(void);
extern void FlushWorkbuffer(void);
extern void GetNandDataCfgInfo(NF_DATA_CONFIG_INFO *cfg_info);
extern void DMAmmCopy(UINT32 wSourceAddr, UINT32 wTargeAddr, UINT16 Count);
extern void DMAmmCopyToUser(UINT32 wSourceAddr, UINT32 wTargeAddr, UINT16 Count);
extern void DMAmmCopyFromUser(UINT32 wSourceAddr, UINT32 wTargeAddr, UINT16 Count);
extern SINT16	NandWriteBankInfoAreaExt(UINT32 pBuf, UINT32 bytes,UINT16 mode);
extern UINT16 DrvNand_read_sector_NoSem(UINT32 wReadLBA, UINT16 wLen, UINT32 DataBufAddr,UINT8 mode);
extern UINT16 DrvNand_write_sector_NoSem(UINT32 wWriteLBA, UINT16 wLen, UINT32 DataBufAddr,UINT8 mode);
extern UINT16	DrvNand_flush_allblk(void);
extern UINT16 DrvNand_initial(void);
extern UINT16 DrvNand_UnIntial(void);

extern SINT32 GetNandConfigInfo(void);
extern UINT16 GetDataAreaStartBlock(void);
extern UINT16 GetDataAreaBankSize(void);
extern UINT16 GetDataAreaBankRecycleSize(void);
extern UINT32 GetNoFSAreaSectorSize(void);
extern UINT8 DataGetWorkbuffer(UINT8 **ptr1, UINT8 **ptr2,UINT32 size);
extern SINT32 DataEraseNandBlk(UINT16 wPhysicBlkNum);
extern SINT32 ReUseUnstableBlock(UINT16 TargetBank);
extern UINT32 CheckBlockValidPages(UINT32 block);
extern SINT16 CopyPhysicalBlock(UINT16 wSrcBlk,UINT16 wTarBlk);

#endif		// __drv_l2_NAND_MANAGE_H__
