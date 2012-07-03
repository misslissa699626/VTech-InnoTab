//#include "drv_l2_nand_config.h"
#include "drv_l2_nand_manage.h"

//=== This is for code configuration DON'T REMOVE or MODIFY it ===//


/* ----------------  Nand Data Manage Layer Global Varibles -------------- */
UINT8 				*tt_basic_ptr;
UINT8 				*tt_extend_ptr;
UINT32				gWorkBufferNum;
MM					mm[1];
MM					debug_mm[1];
UINT16 				gMaptableChanged[1];

EXCHANGE_A_BLK		gExchangeABlock[ExchangeAMaxNum];
EXCHANGE_A_BLK		gExchangeBBlock[ExchangeBMaxNum];
EXCHANGE_A_BLK		*gpWorkBufferBlock= (void*)NULL;

NF_DATA_HAL_INFO 				gSTNandDataHalInfo;                         
NF_DATA_CONFIG_INFO 		gSTNandDataCfgInfo;

BAD_BLK_ARRAY 		bad_blk_info;

UINT16				gLogic2PhysicAreaStartBlk=0;
UINT32				gBankInfoCurPage;

UINT16    		gCurrentBankNum;
UINT16 				gTotalBankNum;

UINT32 				gLastPage_Write;
UINT32 				gLastPage_Read;
UINT32				gLastPage_Error;

UINT16       	gInitialFlag = 0;
UINT16        gDataPhyBlkNum;
UINT32 				gNandSize;
//UINT32 				gFirstFATStart;
//UINT32 				gFirstFATDataStart;
//UINT32 				gSecondFATStart;
//UINT32 				gSecondFADataStart;
PART_FAT_INFO   part_info[MAX_PARTITION_NUM];
UINT8				 	gCheckOrgBadBlockBCHStatus = 0;
BANK_INFO  	gBankInfo[MAX_BANK_NUM];

//  modify for A/B Area max block size(2MB) alignment sofar,it should be updated if have bigger block later 2011.06.22
const UINT32 FATNandTable[] =	
{
	0x1000,	//0-64M              ----->  2MB
	0x2000, //64-256	        ----->  5MB    ---> 4MB
	0x2000, //256-512	        ----->  5MB    ---> 4MB
	0x2000, //512-1G             ----->  3MB    ---> 4MB
	0x2000	//1G-2G               ----->  5MB    ---> 4MB
};


void GlobalVarInit(void)
{
	SINT32 i;
	SINT32 j;
	// ------------------ Initial all global variable --------------------//

	//for(i = 0; i < gSTNandDataCfgInfo.uiBankNum; i++)
	for(i = 0; i < MAX_BANK_NUM; i++)
	{		
		gBankInfo[i].wLogicBlkNum 		= 0;
		gBankInfo[i].wRecycleBlkNum     = 0;
		gBankInfo[i].wMapTableBlk 		= NAND_ALL_BIT_FULL_W;
		gBankInfo[i].wMapTablePage 		= NAND_ALL_BIT_FULL_W;
		gBankInfo[i].wOrgBlkNum 	    = 0;
		gBankInfo[i].wUserBadBlkNum		= 0;
		gBankInfo[i].wExchangeBlkNum    = 0;
		gBankInfo[i].wStatus			= MAPTAB_INVALID;
	}

	for(i=0;i<ExchangeAMaxNum;i++)
	{
		gExchangeABlock[i].wBank		= NAND_ALL_BIT_FULL_W;
		gExchangeABlock[i].wLogicBlk	= NAND_ALL_BIT_FULL_W;
		gExchangeABlock[i].wPhysicBlk	= NAND_ALL_BIT_FULL_W;
		gExchangeABlock[i].wLogicalPage	= 0;
		gExchangeABlock[i].wCurrentPage	= 0;
		gExchangeABlock[i].wCount		= NAND_ALL_BIT_FULL_B;
		gExchangeABlock[i].wType		= NAND_ALL_BIT_FULL_B;

		//for(j = 0;j < gSTNandDataHalInfo.wBlkPageNum;j++)    
		for(j = 0;j < MAX_PAGE_PER_BLOCK;j++)
			gExchangeABlock[i].wPage2PageTable[j] = NAND_ALL_BIT_FULL_W;
	}
	
	for(i=0;i<ExchangeBMaxNum;i++)
	{
		gExchangeBBlock[i].wBank		= NAND_ALL_BIT_FULL_W;
		gExchangeBBlock[i].wLogicBlk	= NAND_ALL_BIT_FULL_W;
		gExchangeBBlock[i].wPhysicBlk	= NAND_ALL_BIT_FULL_W;
		gExchangeBBlock[i].wLogicalPage	= 0;
		gExchangeBBlock[i].wCurrentPage	= 0;			
		gExchangeBBlock[i].wCount		= NAND_ALL_BIT_FULL_B;
		gExchangeBBlock[i].wType		= NAND_ALL_BIT_FULL_B;

		//for(j = 0;j < gSTNandDataHalInfo.wBlkPageNum;j++)
		for(j = 0;j < MAX_PAGE_PER_BLOCK;j++)
			gExchangeBBlock[i].wPage2PageTable[j] = NAND_ALL_BIT_FULL_W;
	}

	gCurrentBankNum 				= 0;
	gTotalBankNum					= 0;
	gLastPage_Write 				= 0xffffffff; //fix bug for ram
	gLastPage_Read					= 0xffffffff; //fix bug for ram	
	gLastPage_Error					= 0xffffffff;

	//gFirstFATStart     				= 0;
	//gFirstFATDataStart 				= 0;
	//gSecondFATStart    				= 0;
	//gSecondFADataStart 				= 0;

	gMaptableChanged[0] = MAPTAB_WRITE_BACK_NEED;
	
	for(i=0;i<MAX_BAD_BLK;i++)
	{
		bad_blk_info.bad_blk[i]=NAND_ALL_BIT_FULL_W;
	}	
}

