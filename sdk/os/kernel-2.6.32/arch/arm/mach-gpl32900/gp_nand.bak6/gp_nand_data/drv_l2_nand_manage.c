#include "drv_l2_nand_manage.h"
#include <mach/diag.h>

#define NF_DATA_DEBUG_PRINT_READ_RETRY
#define POWER_LOST_DEBUG_CHECK

extern void nandAdjustDmaTiming(void);
extern void nandModifyDmaTiming(UINT16 tRP, UINT16 tREH, UINT16 tWP, UINT16 tWH);

//#ifdef PART0_WRITE_MONITOR_DEBUG
#if 1
extern void dump_curren_bank_maptable(void);
extern void dump_bankinfo(void);
extern void dump_exchange_block(void);
extern void dump_debug_bank_maptable(void);
extern SINT16 ReadMapTableFromNand_debug(void);
extern SINT16 ChangeBank_debug(void);
extern SINT32 Check_Part0_logical(UINT32 blk);
extern SINT32 Check_Part0_Physical(UINT32 blk);
extern SINT32 Check_Part0_page(UINT32 page);
extern MM	debug_mm[1];
extern void dump_power_lost_check_info(UINT16 physic,UINT16 exchange,UINT16 error_page,UINT16 who_error,UINT8 *buffer);
#endif

UINT16	start_monitor = 0;

UINT16 	RWFail_Debug(void);

//#define NF_DATA_ERROR(...)	 
//#define NF_DATA_DEBUG(...)  
#define NF_DATA_ERROR		DIAG_INFO
#define NF_DATA_DEBUG		DIAG_INFO

extern UINT32 Nand_Chip;
extern int disable_parse_header;

UINT16 DrvNand_UnIntial(void)
{
	gInitialFlag = 0;
	start_monitor = 0;
	return gInitialFlag;
}

UINT16 DrvNand_initial(void)
{ 	
	UINT16 i;
	UINT16 Step;
	UINT16 TotalStep;
	UINT16 wNeedUpdate;
	UINT32 ret;
	UINT16 wBLkBadFlag;
	UINT8  buffer_num;
	
	NF_DATA_DEBUG("\r\n====Nand data area initial Start!!=====\r\n");
	if(gInitialFlag==0x01)
	{
		return 0;
	}
	
	Nand_OS_Init();
	Nand_OS_LOCK();
		
	ret = GetNandParam();
	if(ret != NF_OK)
	{
		NF_DATA_ERROR("\r\n====Nand data area GetNandParam Fail!<function:DrvNand_initial>=====\r\n");
		Nand_OS_UNLOCK();		
		return ret;
	}
	
	DataGetWorkbuffer(&tt_basic_ptr, &tt_extend_ptr,gSTNandDataHalInfo.wPageSize);
	buffer_num =SetWorkBuffer(tt_basic_ptr, tt_extend_ptr);
	if(buffer_num == 0)
	{
		NF_DATA_ERROR("----- Allocate DATA Workbuffer failed!<function:DrvNand_initial> ------- \n");
		Nand_OS_UNLOCK();		
		return NF_NO_BUFFERPTR;
	}

	GlobalVarInit();

	for(i=gSTNandDataCfgInfo.uiDataStart;i<gLogic2PhysicAreaStartBlk;i++)
	{
		wBLkBadFlag = GetBadFlagFromNand(i);
		if(wBLkBadFlag != NAND_ORG_BAD_TAG)
		{				
			ReadDataFromNand((UINT32)((UINT32)i<<gSTNandDataHalInfo.wBlk2Page),tt_extend_ptr);
			if(GetCArea(NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS,tt_extend_ptr)==NAND_FORMAT_START_TAG)
			{
				NF_DATA_ERROR ("\r\n==== Data area low level format abnormal =====<function:DrvNand_initial>\r\n");
				Nand_OS_UNLOCK();				
				return NF_FORMAT_POWER_LOSE;
			}
		}
	}

	ret = BankInfoAreaInit();
	if(ret==0)
	{	
		BankInfoAreaRead((UINT32)gBankInfo, (gSTNandDataCfgInfo.uiBankNum*sizeof(gBankInfo[0]))<<BYTE_WORD_SHIFT);
	}
	else
	{
		NF_DATA_ERROR ("Warning: No fast initial table page found!<function:DrvNand_initial>\n");
	}
#ifdef PART0_WRITE_MONITOR_DEBUG	
	dump_bankinfo();	
#endif			
	gDataPhyBlkNum = gLogic2PhysicAreaStartBlk;

	TotalStep = (gSTNandDataHalInfo.wNandBlockNum / gSTNandDataCfgInfo.uiBankSize)+1;	// Modify 2010.4.29	
  
	for(Step=0;Step<TotalStep;Step++)
	{
		NF_DATA_DEBUG ("Data area init bank:0x%x\n",Step);
		ret = DrvNand_initial_Update();
		if(ret!=0)
		{
			NF_DATA_ERROR ("Data area init bank:0x%x failed !!<function:DrvNand_initial> \n",Step);
			Nand_OS_UNLOCK();			
			return ret;
		}
	}

	wNeedUpdate = 0x00;
	for(i=0;i<gTotalBankNum;i++)
	{
		if(gBankInfo[i].wStatus != MAPTAB_VALID)
		{			
			gBankInfo[i].wStatus = MAPTAB_VALID;
			wNeedUpdate = 0x01;
		}
	}

	if(wNeedUpdate!=0x00)	// Need Update
	{	
		BankInfoAreaWrite((UINT32)gBankInfo,(gSTNandDataCfgInfo.uiBankNum*sizeof(gBankInfo[0]))<<BYTE_WORD_SHIFT);
	}

	NF_DATA_DEBUG ("Nand Data Area Initial Success!!!\n");
	gInitialFlag = 1;
#ifdef PART0_WRITE_MONITOR_DEBUG
	ChangeBank_debug();
	//dump_bankinfo();
	//dump_exchange_block();
#endif	
	Nand_OS_UNLOCK();

	return 0;
}

UINT32  USBUpDateInitial(void)
{
	UINT32 ret;	
	UINT16 i;
	UINT16 wBLkBadFlag;
	UINT8  buffer_num;
	

	ret = GetNandParam();
	if(ret != NF_OK)
	{
		NF_DATA_ERROR("\r\n====Nand data area GetNandParam Fail !!<function:USBUpDateInitial>=====\r\n");
		Nand_OS_UNLOCK();		
		return ret;
	}
	
	DataGetWorkbuffer(&tt_basic_ptr, &tt_extend_ptr,gSTNandDataHalInfo.wPageSize);
	buffer_num =SetWorkBuffer(tt_basic_ptr, tt_extend_ptr);
	if(buffer_num == 0)
	{
		NF_DATA_ERROR("----- Allocate DATA Workbuffer failed!!! -------<function:USBUpDateInitial> \n");
		Nand_OS_UNLOCK();		
		return NF_NO_BUFFERPTR;
	}
	
	GlobalVarInit();

	for(i=gSTNandDataCfgInfo.uiDataStart;i<gLogic2PhysicAreaStartBlk;i++)
	{
		wBLkBadFlag = GetBadFlagFromNand(i);
		if(wBLkBadFlag != NAND_ORG_BAD_TAG)
		{				
			ReadDataFromNand((UINT32)((UINT32)i<<gSTNandDataHalInfo.wBlk2Page),tt_extend_ptr);
			if(GetCArea(NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS,tt_extend_ptr)==NAND_FORMAT_START_TAG)
			{
				NF_DATA_ERROR ("\r\n==== Data area low level format abnormal=====<function:USBUpDateInitial>\r\n");
				Nand_OS_UNLOCK();				
				return NF_FORMAT_POWER_LOSE;
			}
		}
	}

	ret = BankInfoAreaInit();
	if(ret==0)
	{	
		BankInfoAreaRead((UINT32)gBankInfo, (gSTNandDataCfgInfo.uiBankNum*sizeof(gBankInfo[0]))<<BYTE_WORD_SHIFT);
	}

	gDataPhyBlkNum = gLogic2PhysicAreaStartBlk;
	
	return 0;
}

UINT32 NandInitial_UD(UINT16 Step)
{
	UINT32 ret;
	UINT16 wNeedUpdate;
	
	NF_DATA_DEBUG("Data area init bank:0x%x\n",Step);	
	ret = DrvNand_initial_Update();
	if(ret!=0)
	{
		NF_DATA_ERROR ("Data area init step:0x%x failed !!<function:NandInitial_UD>\n",Step);
		return ret;
	}
	
	wNeedUpdate = 0x00;

	if(gBankInfo[Step].wStatus != MAPTAB_VALID)
	{
		gBankInfo[Step].wStatus = MAPTAB_VALID;
		wNeedUpdate = 0x01;
	}

	if(wNeedUpdate!=0x00)	// Need Update
	{
		BankInfoAreaWrite((UINT32)gBankInfo,(gSTNandDataCfgInfo.uiBankNum*sizeof(gBankInfo[0]))<<BYTE_WORD_SHIFT);
	}
	
	return 0;
}

UINT16 DrvNand_initial_Update(void)
{
	UINT16		i;
	SINT16		ret;
	UINT16		wStartPage;
	UINT16		wIllegalCount;
	UINT16		wGoodBlkCnt;
	UINT16		wPhyBlkNum;			//physical block number 
	UINT16		wLogicBlkInfo;		//logic block number
	UINT16		wUsrMarkBlkCnt;		//user mark bad block
	UINT16		wUsrUnstableBlkCnt;
	UINT16		wMapTblFreeIndex;	//current free locate on map table
	UINT16		wMapTablePhyBlk;	//save map table block
	UINT16		wMovelen;
	UINT16		wRecycleSaveIdx;
	UINT16		wSearchStartIdx;
	UINT16		wTableCount;
	UINT16    	wORGBlkNum;
	IBT			wIllegalBlkTable[NAND_Illegal_BLK_SIZE];
	SINT32		debug_ret;

	if(gDataPhyBlkNum >= gSTNandDataHalInfo.wNandBlockNum)
	{
		return 0;
	}
	
	debug_ret = 0;
	DrvNand_WP_Initial();

	if(gBankInfo[gCurrentBankNum].wStatus == MAPTAB_VALID)
	{
		//gTotalBankNum++;
#ifdef ENABLE_FULL_MONITOR_DEBUG		
		NF_DATA_ERROR("-------------Bank :0x%x Start block:0x%x  \n -----------",gCurrentBankNum,gDataPhyBlkNum);
		NF_DATA_ERROR("-------------Bank :0x%x Org block:0x%x  \n -----------",gCurrentBankNum,gBankInfo[gCurrentBankNum].wOrgBlkNum);
		NF_DATA_ERROR("-------------Bank :0x%x End block:0x%x  \n -----------",gCurrentBankNum,wPhyBlkNum);
#endif		
		wPhyBlkNum	= gDataPhyBlkNum + gSTNandDataCfgInfo.uiBankSize + gSTNandDataCfgInfo.uiBankRecSize + gBankInfo[gCurrentBankNum].wOrgBlkNum;

		if(wPhyBlkNum>=gSTNandDataHalInfo.wNandBlockNum)	// Last Bank
		{
			wPhyBlkNum = gSTNandDataHalInfo.wNandBlockNum;

			if(gBankInfo[gCurrentBankNum].wExchangeBlkNum !=0)
			{
				ret = FixBankExchangeBlk(gCurrentBankNum,gDataPhyBlkNum,wPhyBlkNum);
				if(ret!=0)
				{
					NF_DATA_ERROR(" Fix Bank exchange block failed 1!! Bank:0x%x <function:DrvNand_initial_Update>\n",gCurrentBankNum);
#ifdef ENABLE_FULL_MONITOR_DEBUG
					return NF_FIX_ILLEGAL_BLK_ERROR;
#else
					goto REBUILD_MAPTABLE;
#endif					
				}
			}
			else
			{
				ret = ReadMapTableFromNand(gCurrentBankNum);
				if(ret==NF_READ_ECC_ERR)  //if(ret!=0)
				{
					NF_DATA_ERROR(" Read maptable failed 1!! Bank:0x%x <function:DrvNand_initial_Update> \n",gCurrentBankNum);
					SetBadFlagIntoNand(gBankInfo[gCurrentBankNum].wMapTableBlk, NAND_USER_UNSTABLE_TAG);	//将该blk设置成为坏块
					goto REBUILD_MAPTABLE;
				}
				else if(ret==NF_READ_BIT_ERR_TOO_MUCH)	// Copy to swap block
				{
					wMapTablePhyBlk = GetFreeBlkFromRecycleFifo();
					
					CopyPhysicalBlock(gBankInfo[gCurrentBankNum].wMapTableBlk,wMapTablePhyBlk);
					SetBadFlagIntoNand(gBankInfo[gCurrentBankNum].wMapTableBlk, NAND_USER_UNSTABLE_TAG);	//将该blk设置成为坏块
							
					gBankInfo[gCurrentBankNum].wMapTableBlk = wMapTablePhyBlk;
					gBankInfo[gCurrentBankNum].wMapTablePage = 0;
				}
			}
			gDataPhyBlkNum = wPhyBlkNum;
			gTotalBankNum++;
			DrvNand_get_Size();
			CalculateFATArea();

			return 0;
		}
		else
		{
			if(gBankInfo[gCurrentBankNum].wExchangeBlkNum != 0)
			{
				ret = FixBankExchangeBlk(gCurrentBankNum,gDataPhyBlkNum,wPhyBlkNum);
				if(ret!=0)
				{
					NF_DATA_ERROR(" Fix Bank exchange block failed 2!! Bank:0x%x <function:DrvNand_initial_Update>\n",gCurrentBankNum);
#ifdef ENABLE_FULL_MONITOR_DEBUG					
					return NF_FIX_ILLEGAL_BLK_ERROR;
#else					
					goto REBUILD_MAPTABLE;
#endif					
				}
			}
			else
			{
				ret = ReadMapTableFromNand(gCurrentBankNum);
				if(ret==NF_READ_ECC_ERR)  //if(ret!=0)
				{
					NF_DATA_ERROR(" Read maptable failed 2!! Bank:0x%x <function:DrvNand_initial_Update> \n",gCurrentBankNum);
					SetBadFlagIntoNand(gBankInfo[gCurrentBankNum].wMapTableBlk, NAND_USER_UNSTABLE_TAG);	//将该blk设置成为坏块
					goto REBUILD_MAPTABLE;
				}
				else if(ret==NF_READ_BIT_ERR_TOO_MUCH)	// Copy to swap block
				{
					wMapTablePhyBlk = GetFreeBlkFromRecycleFifo();
					
					CopyPhysicalBlock(gBankInfo[gCurrentBankNum].wMapTableBlk,wMapTablePhyBlk);
					SetBadFlagIntoNand(gBankInfo[gCurrentBankNum].wMapTableBlk, NAND_USER_UNSTABLE_TAG);	//将该blk设置成为坏块
							
					gBankInfo[gCurrentBankNum].wMapTableBlk = wMapTablePhyBlk;
					gBankInfo[gCurrentBankNum].wMapTablePage = 0;
				}
			}

			gDataPhyBlkNum = wPhyBlkNum;
			gTotalBankNum++;
			gCurrentBankNum++;	// change to next bank

			return 0;
		}
	}

REBUILD_MAPTABLE:	
	/*---- Build maptable ----- */
#ifdef ENABLE_FULL_MONITOR_DEBUG
	NF_DATA_ERROR("-------------Bank :0x%x Start block:0x%x  \n -----------",gCurrentBankNum,gDataPhyBlkNum);
	NF_DATA_ERROR("Rebuild bank 0x%x maptable, <function:DrvNand_initial_Update> \n",gCurrentBankNum);
#endif
	wPhyBlkNum     		= gDataPhyBlkNum;
	wLogicBlkInfo	 	= 0;
	wGoodBlkCnt		 	= 0;		
	wUsrMarkBlkCnt	 	= 0;
	wUsrUnstableBlkCnt  = 0;
	wMapTblFreeIndex 	= 0;		
	wIllegalCount	 	= 0;
	wTableCount      	= 0;
	wORGBlkNum       	= 0;
	wMapTablePhyBlk  	= NAND_ALL_BIT_FULL_W;

	//Clear Illegal Block Table in ram.
	for(i=0;i<NAND_Illegal_BLK_SIZE;i++)
	{
		wIllegalBlkTable[i].wPhysicBlkNum = NAND_ALL_BIT_FULL_W;
		wIllegalBlkTable[i].wLogicBlkNum  = NAND_ALL_BIT_FULL_W;
	}
	
	gMaptableChanged[0] = MAPTAB_WRITE_BACK_NEED;
	gBankInfo[gCurrentBankNum].wStatus = MAPTAB_INVALID;
	
	for(i = 0; i < (gSTNandDataCfgInfo.uiBankSize + gSTNandDataCfgInfo.uiBankRecSize); i++)
		mm[0].pMaptable[i] = NAND_MAPTAB_UNUSE;	
	
	do	// Scan all blocks in a bank
	{
		wLogicBlkInfo = ReadLogicNumFromNand(wPhyBlkNum);

		switch(wLogicBlkInfo)
		{
			case NAND_ORG_BAD_BLK:	// Orginal bad block
				wORGBlkNum++;
				break;

			case NAND_USER_BAD_BLK:			
				wUsrMarkBlkCnt++;
				break;
				
			case NAND_USER_UNSTABLE_BLK:
				wGoodBlkCnt++;
				//wUsrUnstableBlkCnt++;
				break;

			case NAND_EMPTY_BLK:
				wGoodBlkCnt++;
				while(mm[0].pMaptable[wMapTblFreeIndex] !=  NAND_MAPTAB_UNUSE)		  	//if this blk is used,go to next blk
					wMapTblFreeIndex++;
					
				mm[0].pMaptable[wMapTblFreeIndex++] = wPhyBlkNum | NAND_FREE_BLK_MARK_BIT;
				break;

			case NAND_MAPTABLE_BLK:
				if(wMapTablePhyBlk!=NAND_ALL_BIT_FULL_W)
				{
					ret = (SINT16)DataEraseNandBlk(wPhyBlkNum);
					if(ret)
					{
						SetBadFlagIntoNand(wPhyBlkNum, NAND_USER_BAD_TAG);
					}
					
					ret = (SINT16)DataEraseNandBlk(wMapTablePhyBlk);
					if(ret)
					{
						SetBadFlagIntoNand(wPhyBlkNum, NAND_USER_BAD_TAG);
						wMapTablePhyBlk = NAND_ALL_BIT_FULL_W;
					}
					continue;
				}
				wGoodBlkCnt++;
				wTableCount++;
				wMapTablePhyBlk = wPhyBlkNum;
				break;

			default:	// have logical# to phsical block#											
				if(wLogicBlkInfo>=(gSTNandDataCfgInfo.uiBankSize + gSTNandDataCfgInfo.uiBankRecSize))
				{
					NF_DATA_ERROR(" Nand Driver found illegal logic block!!! <function:DrvNand_initial_Update> \n");
					NF_DATA_ERROR(" Bank: 0x%x  Logic NO: 0x%x physic block:0x%x <function:DrvNand_initial_Update> \n",gCurrentBankNum,wLogicBlkInfo,wPhyBlkNum);
					//return 1;
					ret = (SINT16)DataEraseNandBlk(wPhyBlkNum);
					if(ret)
					{
						SetBadFlagIntoNand(wPhyBlkNum, NAND_USER_BAD_TAG);
					}					
					continue;
				}
				wGoodBlkCnt++;
#ifdef PART0_WRITE_MONITOR_DEBUG				
				debug_ret = Check_Part0_logical(wLogicBlkInfo);
				if(debug_ret!=0)
				{
					NF_DATA_ERROR(" Logic NO: 0x%x Physical NO:0x%x <function:DrvNand_initial_Update> \n",wLogicBlkInfo,wPhyBlkNum);
				}
#endif				
				if(mm[0].pMaptable[wLogicBlkInfo] !=  NAND_MAPTAB_UNUSE)		// Have logical num yet.
				{
					if(mm[0].pMaptable[wLogicBlkInfo] & NAND_FREE_BLK_MARK_BIT) // update maptable
					{
						while(mm[0].pMaptable[wMapTblFreeIndex] !=  NAND_MAPTAB_UNUSE)
							wMapTblFreeIndex++;

						mm[0].pMaptable[wMapTblFreeIndex] =	mm[0].pMaptable[wLogicBlkInfo];
						mm[0].pMaptable[wLogicBlkInfo]    = wPhyBlkNum;
						wMapTblFreeIndex++;
					}
					else  
					{
						if(wIllegalCount < NAND_Illegal_BLK_SIZE)
						{
							wIllegalBlkTable[wIllegalCount].wPhysicBlkNum = wPhyBlkNum;
							wIllegalBlkTable[wIllegalCount].wLogicBlkNum  = wLogicBlkInfo; 
							wIllegalCount++;
						}
					}
				}
				else
				{
					mm[0].pMaptable[wLogicBlkInfo] = wPhyBlkNum;
				}						
				break;
		}// switch(wLogicBlkInfo)

		wPhyBlkNum++; // next physical block

		if((wGoodBlkCnt + wUsrMarkBlkCnt) == (gSTNandDataCfgInfo.uiBankSize + gSTNandDataCfgInfo.uiBankRecSize))
			break;	// Bank blocks enough

	}while(wPhyBlkNum < gSTNandDataHalInfo.wNandBlockNum);	// Nand block end, the last bank
	
	gBankInfo[gCurrentBankNum].wOrgBlkNum 	 	= wORGBlkNum;	
	gBankInfo[gCurrentBankNum].wUserBadBlkNum 	= wUsrMarkBlkCnt;
	gDataPhyBlkNum  = wPhyBlkNum;	// move here 04.30
	
//	NF_DATA_ERROR("-------------Bank :0x%x End block:0x%x  \n -----------",gCurrentBankNum,gDataPhyBlkNum);
	if(wPhyBlkNum != gSTNandDataHalInfo.wNandBlockNum)		// not the last bank
	{
		if(wUsrMarkBlkCnt>=gSTNandDataCfgInfo.uiBankRecSize)
		{
			NF_DATA_ERROR("Too many bad block!!! <function:DrvNand_initial_Update> \n");
			NF_DATA_ERROR("wUsrMarkBlkCnt:0x%x  Recyle size: 0x%x <function:DrvNand_initial_Update> \n",wUsrMarkBlkCnt,gSTNandDataCfgInfo.uiBankRecSize);
			return NF_TOO_MANY_BADBLK;
		}
		else
		{
			gBankInfo[gCurrentBankNum].wRecycleBlkNum 	=	wMapTblFreeIndex - gSTNandDataCfgInfo.uiBankSize;
			gBankInfo[gCurrentBankNum].wLogicBlkNum 	= 	gSTNandDataCfgInfo.uiBankSize;
		}
	}
	else	/*---  Note:  Maptable is a good block  ---*/
		if((wGoodBlkCnt + wUsrMarkBlkCnt) > (gSTNandDataCfgInfo.uiBankRecSize + 2))
		{  // Last Bank,to guard at least 66 cycles program the same physical block
			if(wUsrMarkBlkCnt>=gSTNandDataCfgInfo.uiBankRecSize-1)		// No recycle block left
			{				
				NF_DATA_ERROR("Too many bad block 1 !!! <function:DrvNand_initial_Update> \n");
				NF_DATA_ERROR("wUsrMarkBlkCnt:0x%x  Recyle size: 0x%x <function:DrvNand_initial_Update> \n",wUsrMarkBlkCnt,gSTNandDataCfgInfo.uiBankRecSize);
				return NF_TOO_MANY_BADBLK;
			}				
			else 
			{ 									                                      
				wMovelen 		= gSTNandDataCfgInfo.uiBankRecSize - wUsrMarkBlkCnt - wTableCount - wIllegalCount;
				wRecycleSaveIdx = gSTNandDataCfgInfo.uiBankSize + wMovelen -1;
				wSearchStartIdx = wMapTblFreeIndex-1;
				
				for(i=0;i<wMovelen;i++)
				{
					mm[0].pMaptable[wRecycleSaveIdx--] = mm[0].pMaptable[wSearchStartIdx--];
					mm[0].pMaptable[wSearchStartIdx+1] = NAND_MAPTAB_UNUSE;
				}

				gBankInfo[gCurrentBankNum].wRecycleBlkNum =  wMovelen; 
				gBankInfo[gCurrentBankNum].wLogicBlkNum   =  wSearchStartIdx + 1;        // consider maptable block
			} 
		}
		else // No enough good blocks,ignore this bank
		{
			if(gCurrentBankNum == 0)
			{
				NF_DATA_ERROR("Too many bad block 2 !!! <function:DrvNand_initial_Update> \n");
				NF_DATA_ERROR("wUsrMarkBlkCnt:0x%x  Recyle size: 0x%x <function:DrvNand_initial_Update> \n",wUsrMarkBlkCnt,gSTNandDataCfgInfo.uiBankRecSize);
				return NF_TOO_MANY_BADBLK;
			}
			else
			{
				if(gSTNandDataCfgInfo.uiBankNum == gSTNandDataCfgInfo.uiMM_Num)	
				{			
					gMaptableChanged[gCurrentBankNum] = MAPTAB_WRITE_BACK_N_NEED;
				}
				else
				{								//fix bug
					ret = ReadMapTableFromNand(0);
					gMaptableChanged[0] = MAPTAB_WRITE_BACK_N_NEED;
					//if(ret==NF_READ_ECC_ERR)  //if(ret!=0)
					if(ret!=0)
					{
						NF_DATA_ERROR(" Read maptable failed 3!! Bank:0x%x <function:DrvNand_initial_Update>\n",gCurrentBankNum);
					//}
					//else if(ret==NF_READ_BIT_ERR_TOO_MUCH)	// Copy to swap block
					//{
						wMapTablePhyBlk = GetFreeBlkFromRecycleFifo();
						
						CopyPhysicalBlock(gBankInfo[gCurrentBankNum].wMapTableBlk,wMapTablePhyBlk);
						SetBadFlagIntoNand(gBankInfo[gCurrentBankNum].wMapTableBlk, NAND_USER_UNSTABLE_TAG);	//将该blk设置成为坏块
								
						gBankInfo[gCurrentBankNum].wMapTableBlk = wMapTablePhyBlk;
						gBankInfo[gCurrentBankNum].wMapTablePage = 0;
					}					
				}	

				gCurrentBankNum = 0;

				DrvNand_get_Size();				//Update total nand size for fat 
				CalculateFATArea();
				return NF_OK;
			}
		}		
		
	//NF_DATA_ERROR("------ Rerang maptabl %d \n",gCurrentBankNum);	
	for (i=0;i<(gSTNandDataCfgInfo.uiBankSize + gSTNandDataCfgInfo.uiBankRecSize);i++)		//clear map table free mark bit
		mm[0].pMaptable[i]=mm[0].pMaptable[i]&(~NAND_FREE_BLK_MARK_BIT);
		

	/* ---------------------    fixup illegal block		-----------------------------   */					
	if(wIllegalCount != 0)
	{
		ret = FixupIllegalBlock((UINT16 *)(&wIllegalBlkTable), wIllegalCount);
		if(ret!=0)
		{
			NF_DATA_ERROR("Fix illegal block failed!! <function:DrvNand_initial_Update> \n");
			return NF_FIX_ILLEGAL_BLK_ERROR;
		}
	}

	/* ---------------------    search maptable page	-----------------------------   */
	wStartPage = NAND_ALL_BIT_FULL_W;
	if(wMapTablePhyBlk != NAND_ALL_BIT_FULL_W)
	{
		wStartPage = FindMapTableCurrentPage(wMapTablePhyBlk);		
	}

	if(wStartPage == NAND_ALL_BIT_FULL_W)	// No page found, need get an new maptable block
	{
		wMapTablePhyBlk = GetFreeBlkFromRecycleFifo();
		if(NF_NO_SWAP_BLOCK == wMapTablePhyBlk)
		{
			//RWFail_Debug();			
			NF_DATA_ERROR("No more swap blocks for maptable block! <function:DrvNand_initial_Update> \n");
			return NF_NO_SWAP_BLOCK;
		}
		wStartPage = 0;
	}
	//--------------------------------
	//write map table to recycle last block , page 0    	
	gBankInfo[gCurrentBankNum].wMapTableBlk = wMapTablePhyBlk;
	gBankInfo[gCurrentBankNum].wMapTablePage = wStartPage;
	ret = WriteMapTableToNand(gCurrentBankNum);
#ifdef ENABLE_FULL_MONITOR_DEBUG	
	if(ret==0x44)
	{
		NF_DATA_ERROR("Write Maptable page failed! <function:DrvNand_initial_Update>(1) \n");
		dump_curren_bank_maptable();
		dump_bankinfo();
		return 0x44;
	}
#endif	
	while(ret)
	{
		if(ret == NF_NAND_PROTECTED)
		{
			NF_DATA_ERROR("Write Maptable page protect! <function:DrvNand_initial_Update> \n");
			return  NF_NAND_PROTECTED;
		}
		
		ret = SetBadFlagIntoNand(gBankInfo[gCurrentBankNum].wMapTableBlk, NAND_USER_BAD_TAG);	//将该blk设置成为坏块
		if(ret!=0)
		{
			NF_DATA_ERROR("Set Maptable blk to bad fail!  <function:DrvNand_initial_Update> \n");
			return 0x44;
		}
		
		gBankInfo[gCurrentBankNum].wMapTableBlk = GetFreeBlkFromRecycleFifo();
		if(NF_NO_SWAP_BLOCK ==gBankInfo[gCurrentBankNum].wMapTableBlk)
		{
			NF_DATA_ERROR("Write maptable failed &no more swap blocks! <function:DrvNand_initial_Update> \n");
			return NF_NO_SWAP_BLOCK;
		}
		gBankInfo[gCurrentBankNum].wMapTablePage =0;

		ret = WriteMapTableToNand(gCurrentBankNum);
#ifdef ENABLE_FULL_MONITOR_DEBUG			
		if(ret==0x44)
		{
			NF_DATA_ERROR("Write Maptable page failed! <function:DrvNand_initial_Update>(2) \n");
			dump_curren_bank_maptable();
			dump_bankinfo();
			return 0x44;
		}
#endif		
	}	
	
	//gDataPhyBlkNum  = wPhyBlkNum;
	gTotalBankNum++;	

	if(gDataPhyBlkNum >= gSTNandDataHalInfo.wNandBlockNum)  //finish last bank scan power save head
	{
		DrvNand_get_Size();	// Update total nand size for fat 
		CalculateFATArea();
	}
	else
	{
		gCurrentBankNum++;	// change to next bank
	}

	return NF_OK;
}


SINT16 FixBankExchangeBlk(UINT16 wTargetBankNum,UINT32 wBankStartBlk,UINT32 wBankEndBlk)
{
	UINT32 i,j;
	UINT16 wCurrentIndex=0;
	UINT32 wTempPhyBlk;
	UINT32 wLogicBlkInfo;
	UINT32 wBlockArray[64];
	IBT	   wExchangeBlk[10];
	UINT16 wIllegBlkNum=0;
	SINT16 ret;
	SINT32 ret1;
	UINT16 wMapTablePhyBlk;	//save map table block
	
	ret1 = 0;
	for(i=0;i<64;i++)
	{
		wBlockArray[i] = 0xffff;
	}
	
	ret = ReadMapTableFromNand(wTargetBankNum);
	if(ret==NF_READ_ECC_ERR)  //if(ret!=0)
	{
		NF_DATA_ERROR("Bank 0x%x read maptable error! <function:FixBankExchangeBlk>! \n",wTargetBankNum);
		SetBadFlagIntoNand(gBankInfo[gCurrentBankNum].wMapTableBlk, NAND_USER_UNSTABLE_TAG);	//将该blk设置成为坏块
		//goto REBUILD_MAPTABLE;
		return -1;
	}
	else if(ret==NF_READ_BIT_ERR_TOO_MUCH)	// Copy to swap block
	{
		wMapTablePhyBlk = GetFreeBlkFromRecycleFifo();
		
		CopyPhysicalBlock(gBankInfo[gCurrentBankNum].wMapTableBlk,wMapTablePhyBlk);
		SetBadFlagIntoNand(gBankInfo[gCurrentBankNum].wMapTableBlk, NAND_USER_UNSTABLE_TAG);	//将该blk设置成为坏块
				
		gBankInfo[gCurrentBankNum].wMapTableBlk = wMapTablePhyBlk;
		gBankInfo[gCurrentBankNum].wMapTablePage = 0;
	}
	
#ifdef ENABLE_FULL_MONITOR_DEBUG	
	dump_curren_bank_maptable();
	if(wTargetBankNum==0)
	{
		start_monitor = 1;
		ReadMapTableFromNand_debug();
	}
	NF_DATA_ERROR("wBankStartBlk:0x%x wBankEndBlk:0x%x \n",wBankStartBlk,wBankEndBlk);
#endif
	for(i=wBankStartBlk;i<wBankEndBlk;i++)
	{
		for(j=0;j<(gSTNandDataCfgInfo.uiBankSize + gSTNandDataCfgInfo.uiBankRecSize);j++)
		{
			wTempPhyBlk = mm[0].pMaptable[j];

			if(i==wTempPhyBlk)
			{
				break;
			}
		}

		if(j>=(gSTNandDataCfgInfo.uiBankSize + gSTNandDataCfgInfo.uiBankRecSize))	// Can`t find physical block i in maptable
		{
			wBlockArray[wCurrentIndex++] = i;
		}		
	}

#ifdef ENABLE_FULL_MONITOR_DEBUG
	for(j=0,i=0;i<wCurrentIndex;i++)
	{
		NF_DATA_ERROR(" Physical block:0x%x   \n",wBlockArray[i]);
	}
#endif	
	
	for(j=0,i=0;i<wCurrentIndex;i++)
	{
		wLogicBlkInfo = ReadLogicNumFromNand(wBlockArray[i]);
		
		switch(wLogicBlkInfo)
		{
			case NAND_ORG_BAD_BLK:	// Orginal bad block			
				break;

			case NAND_USER_BAD_BLK:
			case NAND_USER_UNSTABLE_BLK:
				break;
				
			case NAND_MAPTABLE_BLK:
				break;				
			
			case NAND_EMPTY_BLK:
				PutFreeBlkIntoRecycleFifo(wBlockArray[i]);
				break;
				
			default:
				switch(GetCArea(NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS,tt_extend_ptr))
				{
					case NAND_EXCHANGE_A_TAG:						
					case NAND_EXCHANGE_B_TAG:
#ifdef PART0_WRITE_MONITOR_DEBUG					
						ret1 = Check_Part0_logical(wLogicBlkInfo);
						if(ret1!=0)
						{
							NF_DATA_ERROR("Illegal logical block found!! 234 <function:FixBankExchangeBlk> \n");
							NF_DATA_ERROR("Logical Block:0x%x  Physical Block:0x%x <function:FixBankExchangeBlk> \n",wLogicBlkInfo,wBlockArray[i]);
							dump_curren_bank_maptable();
							dump_bankinfo();
							dump_exchange_block();
							dump_debug_bank_maptable();	
							
							return -1;
						}
#endif						
						if(j>=10)	// More illegal exchange block
						{
							NF_DATA_ERROR("More illegal exchange block on bank:0x%x  <function:FixBankExchangeBlk> \n",wTargetBankNum);
							return -1;
						}
						wExchangeBlk[j].wPhysicBlkNum = wBlockArray[i];
						wExchangeBlk[j].wLogicBlkNum  = wLogicBlkInfo;						
						wIllegBlkNum++;
						j++;
						break;
						
					default :	
					    NF_DATA_ERROR("Unknown illegal block type:0x%x <function:FixBankExchangeBlk> \n",GetCArea(NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS,tt_extend_ptr));
						NF_DATA_ERROR("Logical Block:0x%x  Physical Block:0x%x <function:FixBankExchangeBlk> \n",wLogicBlkInfo,wBlockArray[i]);
#ifdef ENABLE_FULL_MONITOR_DEBUG						
						dump_curren_bank_maptable();
						dump_bankinfo();
						dump_exchange_block();
						dump_debug_bank_maptable();	
#endif						
					    return -1;
					    break;
				}
		}		
	}
	
	if(wIllegBlkNum!=0)
	{
		ret = FixupIllegalBlock((UINT16 *)(&wExchangeBlk), wIllegBlkNum);
		if(ret!=0)
		{
			NF_DATA_ERROR(" FixupIllegalBlock failed !  <function:FixBankExchangeBlk> \n");
			return ret;
		}
	}
	
	return 0;
}

/*--- Initial FAT Area ---*/
UINT32 CalculateFATArea(void)
{
	UINT32 i;
	UINT16 TempVal;
	UINT16 wPartMSize;
	UINT16 wFATAreaLen;	
	
	for(i=0;i<gSTNandConfigInfo.uiPartitionNum;i++)
	{
		part_info[i].FATStart	= 0;
		part_info[i].DATAStart	= 0;
		
		if(gSTNandConfigInfo.Partition[i].size==0)
		{
			continue;
		}			
		wPartMSize = gSTNandConfigInfo.Partition[i].size>>11;  //to MB
		
		TempVal = (wPartMSize >> 8);
		if(TempVal)//256M
		{
			if(TempVal >= 4)
			{				
				TempVal =3;	
			}
			wFATAreaLen = FATNandTable[TempVal+1];
		}	
		else
		{
			TempVal = wPartMSize>>6;
			if(TempVal)
				wFATAreaLen = FATNandTable[1];
			else
				wFATAreaLen = FATNandTable[0];
		}
		
		part_info[i].FATStart 		=  gSTNandConfigInfo.Partition[i].offset;
		part_info[i].DATAStart		=  part_info[i].FATStart + wFATAreaLen;
	}
	
	return NF_OK;
}


/*************************************************************************
* function:  FindMapTableCurrentPage
*
*
*
**************************************************************************/
UINT16 FindMapTableCurrentPage(UINT16 wPhysicBlk)
{
	UINT16 wStartPage;
	UINT16 wEndPage;
	UINT16 wLength;
	UINT32 ret;

	wStartPage = 0;
	wEndPage = gSTNandDataHalInfo.wBlkPageNum;

	do
	{
		wLength = (wStartPage+wEndPage)>>1;	// binary search
		ret = ReadDataFromNand(((UINT32)wPhysicBlk<<gSTNandDataHalInfo.wBlk2Page) + wLength, tt_extend_ptr);
		//if(ret==NF_READ_ECC_ERR)
		if(ret!=0)
		{
			NF_DATA_ERROR("Just Debug Warning1!  <function:FindMapTableCurrentPage> \n");
			ret = SetBadFlagIntoNand(wPhysicBlk, NAND_USER_UNSTABLE_TAG);	//将该blk设置成为坏块
			if(ret!=0)
			{
				NF_DATA_ERROR("Error!set bad block failed!  <function:FindMapTableCurrentPage> \n");				
			}
			return NAND_ALL_BIT_FULL_W;
		}
		if(GetCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS, tt_extend_ptr) != NAND_MAPTABLE_TAG)
			wEndPage = wLength;
		else
			wStartPage = wLength; 
	}while((wEndPage-wStartPage) >4);

	do
	{
		ret = ReadDataFromNand(((UINT32)wPhysicBlk<<gSTNandDataHalInfo.wBlk2Page) + wStartPage, tt_extend_ptr);
		if(ret!=0)//if(ret==NF_READ_ECC_ERR)
		{
			NF_DATA_ERROR("Just Debug Warning2!  <function:FindMapTableCurrentPage> \n");
			ret = SetBadFlagIntoNand(wPhysicBlk, NAND_USER_UNSTABLE_TAG);	//将该blk设置成为坏块
			if(ret!=0)
			{
				NF_DATA_ERROR("Error!set bad block failed2!  <function:FindMapTableCurrentPage> \n");
			}
			return NAND_ALL_BIT_FULL_W;
		}
		if(GetCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS, tt_extend_ptr) != NAND_MAPTABLE_TAG)
			break;
		wStartPage++;
	}while(wEndPage>=wStartPage);

	if(wStartPage == gSTNandDataHalInfo.wBlkPageNum)
	{
		ret = PutFreeBlkIntoRecycleFifo(wPhysicBlk);
		if(ret!=0)
		{
			NF_DATA_ERROR("Error! Put free blk failed!!  <function:FindMapTableCurrentPage> \n");
		}
		wStartPage = NAND_ALL_BIT_FULL_W;
	}

	return 	wStartPage;
}

void GetNandDataCfgInfo(NF_DATA_CONFIG_INFO *cfg_info)
{	
	cfg_info->uiDataStart 	= GetDataAreaStartBlock();
	cfg_info->uiBankSize 		= GetDataAreaBankSize();
	cfg_info->uiBankRecSize = GetDataAreaBankRecycleSize();
	cfg_info->uiBankNum 		= 30;
	cfg_info->uiMM_Num 			= 1;
}

/*************************************************************************
* function:  GetNandParam  
*
*
*
**************************************************************************/
SINT32 GetNandParam(void)
{
	UINT32 wRet;
	UINT16 i;
	UINT16 wTmpVal;
	
	if(disable_parse_header==0)
	{
		if(GetNandConfigInfo()!=0)
		{
			NF_DATA_ERROR("------- DATA Get nand config information failed!! <function:GetNandParam> \n");
			
			wRet = Nand_Init();
			if(wRet!=0)
			{
				NF_DATA_ERROR("------- DATA hal initial failed!!  <function:GetNandParam>  \n");
				return NF_UNKNOW_TYPE;
			}
		}
	}
	else
	{
		wRet = Nand_Init();
		if(wRet!=0)
		{
			NF_DATA_ERROR("------- DATA hal initial failed2!!  <function:GetNandParam>  \n");
			return NF_UNKNOW_TYPE;
		}
		
		GetNandConfigInfo();
	}
	
	Nand_Getinfo((&gSTNandDataHalInfo.wPageSize),(&gSTNandDataHalInfo.wBlkPageNum),(&gSTNandDataHalInfo.wNandBlockNum));
	
	GetNandDataCfgInfo((&gSTNandDataCfgInfo));
	
	#if 1
	nandAdjustDmaTiming();
	#else
	nandModifyDmaTiming(tRP, tREH, tWP, tWH); // dominant, please get from header
	#endif
	NF_DATA_DEBUG("gSTNandDataHalInfo.wPageSize: 0x%x  \n",			gSTNandDataHalInfo.wPageSize);
	NF_DATA_DEBUG("gSTNandDataHalInfo.wBlkPageNum: 0x%x  \n",		gSTNandDataHalInfo.wBlkPageNum);
	NF_DATA_DEBUG("gSTNandDataHalInfo.wNandBlockNum: 0x%x  \n",		gSTNandDataHalInfo.wNandBlockNum);
	
	NF_DATA_DEBUG("gSTNandDataCfgInfo.uiDataStart: 0x%x  \n",		gSTNandDataCfgInfo.uiDataStart);
	NF_DATA_DEBUG("gSTNandDataCfgInfo.uiBankSize: 0x%x  \n",			gSTNandDataCfgInfo.uiBankSize);
	NF_DATA_DEBUG("gSTNandDataCfgInfo.uiBankRecSize: 0x%x  \n",		gSTNandDataCfgInfo.uiBankRecSize);	
	NF_DATA_DEBUG("gSTNandDataCfgInfo.uiBankNum: 0x%x  \n",			gSTNandDataCfgInfo.uiBankNum);
	NF_DATA_DEBUG("gSTNandDataCfgInfo.uiMM_Num: 0x%x  \n",			gSTNandDataCfgInfo.uiMM_Num);
	
	
	gLogic2PhysicAreaStartBlk 			= gSTNandDataCfgInfo.uiDataStart +20;	// Reserve 20 blocks for speed up
		
	gSTNandDataHalInfo.wPageSectorSize = gSTNandDataHalInfo.wPageSize/512;
	gSTNandDataHalInfo.wPageSectorMask = gSTNandDataHalInfo.wPageSectorSize - 1;

	wTmpVal = gSTNandDataHalInfo.wPageSectorSize*gSTNandDataHalInfo.wBlkPageNum;
	for(i = 0;;i++)
	{
		wTmpVal /= 2;
		if(wTmpVal == 0)
			break;
	}
	gSTNandDataHalInfo.wBlk2Sector = i;

	wTmpVal = gSTNandDataHalInfo.wBlkPageNum; 
	for(i = 0;;i++)
	{
		wTmpVal /= 2;
		if(wTmpVal == 0)
			break;
	}
	gSTNandDataHalInfo.wBlk2Page = i;

	wTmpVal = gSTNandDataHalInfo.wPageSectorSize;
	for(i = 0;;i++)
	{
		wTmpVal /= 2;
		if(wTmpVal == 0)
			break;
	}
	gSTNandDataHalInfo.wPage2Sector = i;

	return 0;
}

/*************************************************************************
* function:  DrvNand_get_Size 
*
*
*
**************************************************************************/
UINT32 DrvNand_get_Size(void)
{
	UINT16  i;
	UINT32 wNandSize;
	wNandSize = 0; 	
	
	//for(i=0;i<gSTNandDataCfgInfo.uiBankNum;i++)
	for(i=0;i<gTotalBankNum;i++)		// zurong modify 2010.6.16	
	{
		if(gBankInfo[i].wLogicBlkNum!=0xffff)
		{
			wNandSize += gBankInfo[i].wLogicBlkNum;
		}
	}
	
	wNandSize = wNandSize << gSTNandDataHalInfo.wBlk2Sector;	
	gNandSize = wNandSize;	
	
	return wNandSize;
}

/*************************************************************************
* function:  GetBadFlagFromNand 
*
*
*
**************************************************************************/
UINT16 GetBadFlagFromNand(UINT16 wPhysicBlkNum)
{
	UINT8 flag = 0;

	flag = good_block_check(wPhysicBlkNum,(UINT32)tt_extend_ptr);
	if(flag==NAND_USER_BAD_TAG)
	{
		NF_DATA_ERROR("Warning：User bad block found!Block: 0x%x <function:GetBadFlagFromNand> \n",wPhysicBlkNum);
		return NAND_USER_BAD_TAG;		
	}
	else if(flag==NAND_USER_UNSTABLE_TAG)
	{
		return NAND_USER_UNSTABLE_TAG;
	}
	else if(flag!=NAND_GOOD_TAG)
	{	
		NF_DATA_ERROR("Warning：Org bad block found!Block: 0x%x  <function:GetBadFlagFromNand> \n",wPhysicBlkNum);
		return NAND_ORG_BAD_TAG;
	}
	else
	{
		return NAND_GOOD_TAG;
	}
}

/*************************************************************************
* function:  SetBadFlagIntoNand 
*
*
*
**************************************************************************/
SINT32 SetBadFlagIntoNand(UINT16 wPhysicBlkNum, UINT16 ErrFlag)
{
	SINT32 ret = 0;
	
	if(wPhysicBlkNum<gSTNandDataCfgInfo.uiDataStart)
	{
		NF_DATA_ERROR("##-->Serious Warning: Data beyond range!! Set bad block: 0x%x  <function:SetBadFlagIntoNand> \n",wPhysicBlkNum);
		return -1;
	}
#ifdef PART0_WRITE_MONITOR_DEBUG	
	ret = Check_Part0_Physical(wPhysicBlkNum);
	if(ret!=0)
	{
		NF_DATA_ERROR("Error：Beyond Set bad Block: 0x%x Bad type:0x%x  <function:SetBadFlagIntoNand> \n",wPhysicBlkNum,ErrFlag);
		
		dump_curren_bank_maptable();
		dump_bankinfo();
		dump_exchange_block();
		dump_debug_bank_maptable();
		return 0x44;
	}
	NF_DATA_ERROR("Warning：Set bad Block: 0x%x Bad type:0x%x  <function:SetBadFlagIntoNand> \n",wPhysicBlkNum,ErrFlag);
#endif
	memset(tt_extend_ptr,0xff,gSTNandDataHalInfo.wPageSize);	// zurong add
	ret = Nand_sw_bad_block_set((UINT32)wPhysicBlkNum,(UINT32)tt_extend_ptr,(UINT8)ErrFlag);
	if(ret!=0)
	{
		NF_DATA_ERROR("Error: set bad Block: 0x%x Bad type:0x%x fail.  <function:SetBadFlagIntoNand> \n",wPhysicBlkNum,ErrFlag);
	}
	
	return ret;
}

/*************************************************************************
* function:  ReadLogicNumFromNand
*
*
*
**************************************************************************/
UINT16 ReadLogicNumFromNand(UINT16 wPhysicBlkNum)
{
	UINT32  wBlockFirstPage;
	UINT8 flag = 0;	
	UINT32 ret;
	UINT32 i;
	
	flag = good_block_check(wPhysicBlkNum,(UINT32)tt_extend_ptr);
	if(flag==NAND_USER_BAD_TAG)
	{
		NF_DATA_ERROR("Warning：User bad block found!Block: 0x%x  <function:ReadLogicNumFromNand> \n",wPhysicBlkNum);
		return NAND_USER_BAD_BLK;		
	}
	else if(flag==NAND_USER_UNSTABLE_TAG)
	{
		return NAND_USER_UNSTABLE_BLK;
	}
	else if(flag!=NAND_GOOD_TAG)
	{	
		NF_DATA_ERROR("Warning：Org bad block found!Block: 0x%x  <function:ReadLogicNumFromNand> \n",wPhysicBlkNum);
		return NAND_ORG_BAD_BLK;
	}
	
    wBlockFirstPage	= ((UINT32)wPhysicBlkNum << gSTNandDataHalInfo.wBlk2Page);
    
	for(i=0;i<gSTNandDataHalInfo.wBlkPageNum;i++)
	{
		ret = ReadDataFromNand(wBlockFirstPage+i, tt_extend_ptr);
		if(ret!=NF_READ_ECC_ERR)
		{
			//NF_DATA_ERROR("Just debug warning, read page ecc error! page:0x%x  <function:ReadLogicNumFromNand> \n",wBlockFirstPage);
			break;
		}
	}
	
	return GetCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS, tt_extend_ptr);
}

/*************************************************************************
* function:  ReadMapTableFromNand 
*
*
*
**************************************************************************/
SINT16 ReadMapTableFromNand(UINT16 wBankNum) 
{
	UINT32 wPhysicBlkNum;
	UINT32 wPhysicPage;
	UINT16 wRet;
	wPhysicBlkNum = gBankInfo[wBankNum].wMapTableBlk;
	wPhysicPage   = wPhysicBlkNum << gSTNandDataHalInfo.wBlk2Page;
	wPhysicPage   = wPhysicPage + gBankInfo[wBankNum].wMapTablePage - 1;
	
	wRet = ReadDataFromNand(wPhysicPage, tt_extend_ptr);	
	if(wRet == NF_READ_ECC_ERR)
	{		
		NF_DATA_ERROR("Serious Warning: Read Maptable page failed! <function:ReadMapTableFromNand> \n");
		//return -1;	// very serious error!!!!
		return NF_READ_ECC_ERR;
	}
	
	if(gSTNandDataCfgInfo.uiBankNum == gSTNandDataCfgInfo.uiMM_Num)
	{
	 	DMAmmCopy((UINT32)tt_extend_ptr, (UINT32)mm[wBankNum].pMaptable, (gSTNandDataCfgInfo.uiBankSize+gSTNandDataCfgInfo.uiBankRecSize)*sizeof(UINT16));
	 	gMaptableChanged[wBankNum] 	= MAPTAB_WRITE_BACK_N_NEED;	//
	}
	else	
	{
		DMAmmCopy((UINT32)tt_extend_ptr, (UINT32)mm[0].pMaptable, (gSTNandDataCfgInfo.uiBankSize+gSTNandDataCfgInfo.uiBankRecSize)*sizeof(UINT16));
		gMaptableChanged[0] = MAPTAB_WRITE_BACK_N_NEED;	
	}
		
	return wRet;
}

/*************************************************************************
* function:  WriteMapTableToNand
*
*
*
**************************************************************************/
UINT16 WriteMapTableToNand(UINT16 wBankNum)
{
	UINT32 wPhysicBlkNum;
	UINT32 wPhysicPage;
	UINT16 wRetNand;
	//UINT32 wTmpBlkNum;
	
	wPhysicBlkNum = gBankInfo[wBankNum].wMapTableBlk;
	wPhysicPage = wPhysicBlkNum << gSTNandDataHalInfo.wBlk2Page;
	wPhysicPage = wPhysicPage + gBankInfo[wBankNum].wMapTablePage;
	
	if(gSTNandDataCfgInfo.uiBankNum == gSTNandDataCfgInfo.uiMM_Num)
	{	
		DMAmmCopy((UINT32)mm[wBankNum].pMaptable, (UINT32)tt_extend_ptr, (gSTNandDataCfgInfo.uiBankSize+gSTNandDataCfgInfo.uiBankRecSize)*sizeof(UINT16));
	}
	else
	{	
		DMAmmCopy((UINT32)mm[0].pMaptable, (UINT32)tt_extend_ptr, (gSTNandDataCfgInfo.uiBankSize+gSTNandDataCfgInfo.uiBankRecSize)*sizeof(UINT16));
	}

	//Mark C-AREA FLAG
	PutCArea(NAND_C_AREA_BAD_FLG_OFS_1,		NAND_ALL_BIT_FULL_B);
	PutCArea(NAND_C_AREA_COUNT_OFS,			NAND_ALL_BIT_FULL_B);
	PutCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS,	NAND_MAPTABLE_TAG);
	PutCArea(NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS,NAND_ALL_BIT_FULL_B);
	PutCArea(NAND_C_AREA_BAD_FLG_OFS_6,		NAND_ALL_BIT_FULL_B);
	PutCArea(NAND_C_AREA_PAGE_TO_PAGE_OFS,	NAND_ALL_BIT_FULL_W);	

	wRetNand = WriteDataIntoNand(wPhysicPage, tt_extend_ptr);
	while(wRetNand)	// if write maptable is fail
	{
#ifdef ENABLE_FULL_MONITOR_DEBUG	
		NF_DATA_ERROR("Warning: Write page fail:0x%x  <function:WriteMapTableToNand>\n", (gBankInfo[wBankNum].wMapTableBlk<<gSTNandDataHalInfo.wBlk2Page)+gBankInfo[wBankNum].wMapTablePage );
		if(wRetNand==0x44)
		{
			NF_DATA_ERROR("Error:I`m dead 1st,<function:WriteMapTableToNand>(1)\n");
			NF_DATA_ERROR("Error:I`m dead 1st, Bank: 0x%x <function:WriteMapTableToNand>(1)\n",wBankNum);
			//dump_curren_bank_maptable();
			//dump_bankinfo();
			
			dump_curren_bank_maptable();
			dump_bankinfo();
			dump_exchange_block();
			dump_debug_bank_maptable();	
			return 0x44;
		}
#endif		
		wRetNand = nand_write_status_get();
		if((NFCMD_STATUS)wRetNand<=NF_PROGRAM_ERR13)
		{
			NF_DATA_ERROR("Error,write maptabl fail! Protected! <function:WriteMapTableToNand> \n");
			return NF_NAND_PROTECTED;
		}
		
		wRetNand = SetBadFlagIntoNand(wPhysicBlkNum, NAND_USER_BAD_TAG);	//将该blk设置成为坏块
		if(wRetNand!=0)
		{	
			NF_DATA_ERROR("Error:set bad block failed! <function:WriteMapTableToNand>(2)\n");
#ifdef ENABLE_FULL_MONITOR_DEBUG				
			dump_curren_bank_maptable();
			dump_bankinfo();
			dump_exchange_block();
			dump_debug_bank_maptable();	
#endif			
			return 0x44;
		}
		gBankInfo[wBankNum].wUserBadBlkNum++;		

		wPhysicBlkNum = GetFreeBlkFromRecycleFifo();
		if(NF_NO_SWAP_BLOCK == wPhysicBlkNum)
		{
			NF_DATA_ERROR("Error,No swap block!<function:WriteMapTableToNand> \n");
			return 0xffff;
		}		
		gBankInfo[wBankNum].wMapTableBlk  = wPhysicBlkNum;
		gBankInfo[wBankNum].wMapTablePage = 0;	
		
		if(gSTNandDataCfgInfo.uiBankNum == gSTNandDataCfgInfo.uiMM_Num)
		{	
		    DMAmmCopy((UINT32)mm[wBankNum].pMaptable, (UINT32)tt_extend_ptr, (gSTNandDataCfgInfo.uiBankSize+gSTNandDataCfgInfo.uiBankRecSize)*sizeof(UINT16));
		}
		else
		{	
			DMAmmCopy((UINT32)mm[0].pMaptable, (UINT32)tt_extend_ptr, (gSTNandDataCfgInfo.uiBankSize+gSTNandDataCfgInfo.uiBankRecSize)*sizeof(UINT16));
		}
		
		PutCArea(NAND_C_AREA_BAD_FLG_OFS_1,		NAND_ALL_BIT_FULL_B);
		PutCArea(NAND_C_AREA_COUNT_OFS,			NAND_ALL_BIT_FULL_B);
		PutCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS,	NAND_MAPTABLE_TAG);
		PutCArea(NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS,NAND_ALL_BIT_FULL_B);
		PutCArea(NAND_C_AREA_BAD_FLG_OFS_6,		NAND_ALL_BIT_FULL_B);
		PutCArea(NAND_C_AREA_PAGE_TO_PAGE_OFS,	NAND_ALL_BIT_FULL_W);

		wRetNand = WriteDataIntoNand(wPhysicBlkNum<<gSTNandDataHalInfo.wBlk2Page, tt_extend_ptr);
#ifdef ENABLE_FULL_MONITOR_DEBUG			
		if(wRetNand==0x44)
		{	
			NF_DATA_ERROR("wPhysicBlkNum:0x%x,<function:WriteMapTableToNand>(2)\n",wPhysicBlkNum);
			NF_DATA_ERROR("Error:I`m dead 2st,<function:WriteMapTableToNand>\n(2)");
			NF_DATA_ERROR("Error:I`m dead 2st, Bank: 0x%x <function:WriteMapTableToNand>(2)\n",wBankNum);
			//dump_curren_bank_maptable();
			//dump_bankinfo();
			
			dump_curren_bank_maptable();
			dump_bankinfo();
			dump_exchange_block();
			dump_debug_bank_maptable();	
			
			return 0x44;
		}
#endif		
	}
	
	if(gSTNandDataCfgInfo.uiBankNum == gSTNandDataCfgInfo.uiMM_Num)	
	{
		gMaptableChanged[wBankNum] = MAPTAB_WRITE_BACK_N_NEED;
	}
	else
	{
		gMaptableChanged[0] = MAPTAB_WRITE_BACK_N_NEED;
	}
	
	gBankInfo[wBankNum].wMapTablePage++;
	//gBankInfo[wBankNum].wStatus	= MAPTAB_VALID;
	
	return 0;
}

/*************************************************************************
* function:  ChangeRamMapTable
*
*
*
**************************************************************************/
SINT16 ChangeBank(UINT16 wBankNum) 
{	
	UINT32 ret=0;
	SINT32 ret_debug=0;
	//UINT32 i = 0;
	UINT16 wMapTablePhyBlk;	//save map table block
	
	if(wBankNum != gCurrentBankNum)
	{		
		if(gWorkBufferNum==1)	// Single workbuffer
		{
			gLastPage_Read 	= 0xffffffff;
			gLastPage_Write	= 0xffffffff;
			
			if((gpWorkBufferBlock!=NULL) && (gpWorkBufferBlock->wLogicalPage != NAND_ALL_BIT_FULL_W))
			{
				ret_debug = WriteBackWorkbuffer(gpWorkBufferBlock);
				// zurong,这里还需要完善error handle
#ifdef ENABLE_FULL_MONITOR_DEBUG					
				if(ret_debug==0x44)
				{
					NF_DATA_ERROR("Error:I`m dead 1st, <function:ChangeBank>(1)\n");
					//NF_DATA_ERROR("Error:I`m dead 1st, Bank: 0x%x <function:WriteMapTableToNand>\n",wBankNum);
					dump_bankinfo();
					dump_curren_bank_maptable();
					dump_exchange_block();
					return 0x44;
				}
#endif				
			}			
		}
		else if(gWorkBufferNum==2)
		{
			gLastPage_Read 	= 0xffffffff;
		}	

		if(	gMaptableChanged[0] == MAPTAB_WRITE_BACK_NEED)	// if maptable of bank have changed,save to nand flash		
		{
			//save current bank map table 	
			if(gBankInfo[gCurrentBankNum].wMapTablePage == gSTNandDataHalInfo.wBlkPageNum)	// Need exchange block
			{
				ret_debug = PutFreeBlkIntoRecycleFifo(gBankInfo[gCurrentBankNum].wMapTableBlk);
				if(ret_debug!=0)
				{
					NF_DATA_ERROR("Error: PutFreeBlkIntoRecycleFifo falied!!  <function:ChangeBank> \n");	
#ifdef ENABLE_FULL_MONITOR_DEBUG
					dump_curren_bank_maptable();
					dump_bankinfo();
					dump_exchange_block();
					dump_debug_bank_maptable();	
#endif					
					return 0x44;
				}
				gBankInfo[gCurrentBankNum].wMapTableBlk = GetFreeBlkFromRecycleFifo();
				if(NF_NO_SWAP_BLOCK == gBankInfo[gCurrentBankNum].wMapTableBlk)
				{
					NF_DATA_ERROR("Error: No more swap block! I Will die...   <function:ChangeBank> \n");
					return 0x44;
				}
				gBankInfo[gCurrentBankNum].wMapTablePage = 0;
			}

			ret = WriteMapTableToNand(gCurrentBankNum);		//write CurrentBank into nand
#ifdef ENABLE_FULL_MONITOR_DEBUG			
			if(ret==0x44)
			{
				NF_DATA_ERROR("Write Maptable page failed!   <function:ChangeBank>(2) \n");
				dump_curren_bank_maptable();
				dump_bankinfo();
				dump_exchange_block();
				dump_debug_bank_maptable();	
				return 0x44;
			}
#endif		
			while(ret)
			{
				ret = nand_write_status_get();
				if((NFCMD_STATUS)ret<=NF_PROGRAM_ERR13)
				{
					NF_DATA_ERROR("Error,write maptabl fail! Protected! <function:ChangeBank> \n");
					return NF_NAND_PROTECTED;
				}
				
				ret = SetBadFlagIntoNand(gBankInfo[gCurrentBankNum].wMapTableBlk, NAND_USER_BAD_TAG);	//将该blk设置成为坏块
				if(ret!=0)
				{	
					NF_DATA_ERROR("Error,Set bad block failed! <function:ChangeBank> \n");
					return 0x44;
				}
				
				gBankInfo[gCurrentBankNum].wMapTableBlk = GetFreeBlkFromRecycleFifo();
				if(NF_NO_SWAP_BLOCK == gBankInfo[gCurrentBankNum].wMapTableBlk)
				{
					NF_DATA_ERROR("Error: No more swap block! I Will die...   <function:ChangeBank> \n");
					return 0x44;
				}
				gBankInfo[gCurrentBankNum].wMapTablePage = 0;
				
				ret = WriteMapTableToNand(gCurrentBankNum);		//write CurrentBank into nand
			}
	
			gBankInfo[gCurrentBankNum].wStatus = MAPTAB_VALID;
			BankInfoAreaWrite((UINT32)gBankInfo,gSTNandDataCfgInfo.uiBankNum*sizeof(gBankInfo[0]));
		}

		//restore request bank maptable
		ret = ReadMapTableFromNand(wBankNum);
		gMaptableChanged[0] = MAPTAB_WRITE_BACK_N_NEED;
		if(ret==NF_READ_ECC_ERR)  //if(ret!=0)
		{
			NF_DATA_ERROR("Warning:Read maptabl fail!Bank:0x%x  <function:ChangeBank> \n",gCurrentBankNum);
			wMapTablePhyBlk = GetFreeBlkFromRecycleFifo();
			
			CopyPhysicalBlock(gBankInfo[gCurrentBankNum].wMapTableBlk,wMapTablePhyBlk);
			SetBadFlagIntoNand(gBankInfo[gCurrentBankNum].wMapTableBlk, NAND_USER_UNSTABLE_TAG);	//将该blk设置成为坏块
					
			gBankInfo[gCurrentBankNum].wMapTableBlk = wMapTablePhyBlk;
			gBankInfo[gCurrentBankNum].wMapTablePage = 0;	
			//return -1;
		}
		else if(ret==NF_READ_BIT_ERR_TOO_MUCH)	// Copy to swap block
		{
			wMapTablePhyBlk = GetFreeBlkFromRecycleFifo();
			
			CopyPhysicalBlock(gBankInfo[gCurrentBankNum].wMapTableBlk,wMapTablePhyBlk);
			SetBadFlagIntoNand(gBankInfo[gCurrentBankNum].wMapTableBlk, NAND_USER_UNSTABLE_TAG);	//将该blk设置成为坏块
					
			gBankInfo[gCurrentBankNum].wMapTableBlk = wMapTablePhyBlk;
			gBankInfo[gCurrentBankNum].wMapTablePage = 0;
		}		

		//switch to this bank
		gCurrentBankNum = wBankNum;			
	}
	
	return 0;
}

void FlushCurrentMaptable(void)
{
   UINT16 ret;
   SINT32 debug_ret = 0;
   //UINT32 i = 0;	
	
	if(	gMaptableChanged[0] == MAPTAB_WRITE_BACK_NEED)	// if maptable of bank have changed,save to nand flash		
	{		
		if(gWorkBufferNum==1)	// Single workbuffer
		{
			gLastPage_Read 	= 0xffffffff;
			gLastPage_Write	= 0xffffffff;
			
			if((gpWorkBufferBlock!=NULL) && (gpWorkBufferBlock->wLogicalPage != NAND_ALL_BIT_FULL_W))
			{
				debug_ret = WriteBackWorkbuffer(gpWorkBufferBlock);
#ifdef ENABLE_FULL_MONITOR_DEBUG			
				if(debug_ret==0x44)
				{
					NF_DATA_ERROR("Error:I`m dead 1st,  <function:FlushCurrentMaptable>(1)\n");
					//NF_DATA_ERROR("Error:I`m dead 1st, Bank: 0x%x <function:WriteMapTableToNand>\n",wBankNum);
					dump_bankinfo();
					dump_curren_bank_maptable();
					dump_exchange_block();
					return ;
				}
#endif			
			}			
		}
		else if(gWorkBufferNum==2)
		{
			gLastPage_Read 	= 0xffffffff;
		}
	
		//save current bank map table 	
		if(gBankInfo[gCurrentBankNum].wMapTablePage == gSTNandDataHalInfo.wBlkPageNum)	// Need exchange block
		{
			ret = PutFreeBlkIntoRecycleFifo(gBankInfo[gCurrentBankNum].wMapTableBlk);
			if(ret)
			{
				NF_DATA_ERROR("PutFreeBlkIntoRecycleFifo failed!   <function:FlushCurrentMaptable>(2) \n");
#ifdef ENABLE_FULL_MONITOR_DEBUG				
				dump_curren_bank_maptable();
				dump_bankinfo();
				dump_exchange_block();
#endif				
			//return 0x44;
			}	
			gBankInfo[gCurrentBankNum].wMapTableBlk = GetFreeBlkFromRecycleFifo();
			if(NF_NO_SWAP_BLOCK == gBankInfo[gCurrentBankNum].wMapTableBlk)
			{
				NF_DATA_ERROR("Error,No swap block!  <function:FlushCurrentMaptable> \n");
				return ;
			}
			gBankInfo[gCurrentBankNum].wMapTablePage = 0;
		}

		ret = WriteMapTableToNand(gCurrentBankNum);		//write CurrentBank into nand
#ifdef ENABLE_FULL_MONITOR_DEBUG		
		if(ret==0x44)
		{
			NF_DATA_ERROR("Write Maptable page failed!  <function:FlushCurrentMaptable>(2) \n");
			dump_curren_bank_maptable();
			dump_bankinfo();
			dump_exchange_block();
			//return 0x44;
		}		
#endif
		while(ret)
		{
			ret = nand_write_status_get();
			if((NFCMD_STATUS)ret<=NF_PROGRAM_ERR13)
			{
				NF_DATA_ERROR("Error,write maptabl fail! Protected! <function:FlushCurrentMaptable> \n");
				//return NF_NAND_PROTECTED;
				return;
			}
			
			ret = SetBadFlagIntoNand(gBankInfo[gCurrentBankNum].wMapTableBlk, NAND_USER_BAD_TAG);	//将该blk设置成为坏块
			if(ret!=0)
			{	
				NF_DATA_ERROR("Error,Set bad block failed! <function:FlushCurrentMaptable> \n");
				//return 0x44;
				return;
			}
			
			gBankInfo[gCurrentBankNum].wMapTableBlk = GetFreeBlkFromRecycleFifo();
			if(NF_NO_SWAP_BLOCK == gBankInfo[gCurrentBankNum].wMapTableBlk)
			{
				NF_DATA_ERROR("Error: No more swap block! I Will die...   <function:FlushCurrentMaptable> \n");
				//return 0x44;
				return;
			}
			gBankInfo[gCurrentBankNum].wMapTablePage = 0;
			
			ret = WriteMapTableToNand(gCurrentBankNum);		//write CurrentBank into nand
		}		
		
		gBankInfo[gCurrentBankNum].wStatus = MAPTAB_VALID;
		BankInfoAreaWrite((UINT32)gBankInfo,gSTNandDataCfgInfo.uiBankNum*sizeof(gBankInfo[0]));
		gMaptableChanged[0] = MAPTAB_WRITE_BACK_N_NEED;
	}
}

/*************************************************************************
* function:  UpdateMaptable
*
*
*
**************************************************************************/
void UpdateMaptable(UINT16 wSwapBlkNum, UINT16 wLogicNum)
{
	SINT32 ret_debug;
	
	mm[0].Maptable.Logic2Physic[wLogicNum] = wSwapBlkNum;
	gMaptableChanged[0] = MAPTAB_WRITE_BACK_NEED;	// Changed
	
	if(gBankInfo[gCurrentBankNum].wStatus == MAPTAB_VALID)	// Need write start flag
	{		
		if(gWorkBufferNum==1)	// Single workbuffer
		{
			gLastPage_Read 	= 0xffffffff;
			gLastPage_Write	= 0xffffffff;
			
			if((gpWorkBufferBlock!=NULL) && (gpWorkBufferBlock->wLogicalPage != NAND_ALL_BIT_FULL_W))
			{
				ret_debug = WriteBackWorkbuffer(gpWorkBufferBlock);
				if(ret_debug==0x44)
				{
					NF_DATA_ERROR("Error:I`m dead 1st,  <function:PutFreeBlkIntoRecycleFifo>\n");
#ifdef ENABLE_FULL_MONITOR_DEBUG
					dump_bankinfo();
					dump_curren_bank_maptable();
					dump_exchange_block();
#endif					
					//return 0x44;
					//return;
				}
			}			
		}
		else if(gWorkBufferNum==2)
		{
			gLastPage_Read 	= 0xffffffff;
		}
			
		gBankInfo[gCurrentBankNum].wStatus = MAPTAB_INVALID;
		
		BankInfoAreaWrite((UINT32)gBankInfo,(gSTNandDataCfgInfo.uiBankNum*sizeof(gBankInfo[0]))<<BYTE_WORD_SHIFT);
		//DBG_PRINT("\r \n gCurrent Bank:%d",gCurrentBankNum);
		//DBG_PRINT("\r \n MAPTAB_INVALID -- Put Free Blk");
	}	
}

/*************************************************************************
* function:		GetFreeBlkFromRecycleFifo 
*	
*	
*	
*************************************************************************/
UINT16 GetFreeBlkFromRecycleFifo(void)
{
	UINT16 	i;
	UINT16	wPhysicBlkNum;	
	SINT32	ret;
	UINT32  count = 0;
	
	do
	{
		if(gBankInfo[gCurrentBankNum].wRecycleBlkNum == 0)
		{
			wPhysicBlkNum = mm[0].pMaptable[gSTNandDataCfgInfo.uiBankSize+0];
			if(wPhysicBlkNum>=NAND_MAPTAB_UNUSE)
			{
				count = ReUseUnstableBlock(gCurrentBankNum);
				if(count==0)
				{
					NF_DATA_ERROR("Error,No swap block!Bank:0x%x   <function:GetFreeBlkFromRecycleFifo> \n",gCurrentBankNum);
					return	NF_NO_SWAP_BLOCK;
				}
			}
		}
		
		wPhysicBlkNum = mm[0].pMaptable[gSTNandDataCfgInfo.uiBankSize+0];	
		gBankInfo[gCurrentBankNum].wRecycleBlkNum--;		
	}while(wPhysicBlkNum>=NAND_MAPTAB_UNUSE);
	
	for(i=0;i<gBankInfo[gCurrentBankNum].wRecycleBlkNum;i++)	
		mm[0].pMaptable[gSTNandDataCfgInfo.uiBankSize+i] = mm[0].pMaptable[gSTNandDataCfgInfo.uiBankSize+i+1];				

	mm[0].pMaptable[gSTNandDataCfgInfo.uiBankSize+i] = NAND_MAPTAB_UNUSE;
	gMaptableChanged[0] = MAPTAB_WRITE_BACK_NEED;
		
	if(gBankInfo[gCurrentBankNum].wStatus == MAPTAB_VALID)	// Need write start flag
	{		
		if(gWorkBufferNum==1)	// Single workbuffer
		{
			gLastPage_Read 	= 0xffffffff;
			gLastPage_Write	= 0xffffffff;
			
			if((gpWorkBufferBlock!=NULL) && (gpWorkBufferBlock->wLogicalPage != NAND_ALL_BIT_FULL_W))
			{
				ret = WriteBackWorkbuffer(gpWorkBufferBlock);				
				if(ret==0x44)
				{
					NF_DATA_ERROR("Error:I`m dead 1st,  <function:GetFreeBlkFromRecycleFifo>\n");
#ifdef ENABLE_FULL_MONITOR_DEBUG					
					dump_bankinfo();
					dump_curren_bank_maptable();
					dump_exchange_block();
#endif					
					return 0x44;
				}
			}			
		}
		else if(gWorkBufferNum==2)
		{
			gLastPage_Read 	= 0xffffffff;
		}
			
		gBankInfo[gCurrentBankNum].wStatus = MAPTAB_INVALID;
		
		BankInfoAreaWrite((UINT32)gBankInfo,(gSTNandDataCfgInfo.uiBankNum*sizeof(gBankInfo[0]))<<BYTE_WORD_SHIFT);		
	}	

	return wPhysicBlkNum;
}
	
/*************************************************************************
* function:		PutFreeBlkIntoRecycleFifo 
*	
*	
*	
*************************************************************************/ 
SINT32 PutFreeBlkIntoRecycleFifo(UINT16 wPhysicBlkNum)
{
	UINT16 ret;	
	SINT32	ret_debug;
	
	ret = DataEraseNandBlk(wPhysicBlkNum);
	if(ret)
	{	
		//RWFail_Debug();			
		NF_DATA_ERROR("Warning! Erase block 0x%x failed!! Bank:0x%x   <function:PutFreeBlkIntoRecycleFifo> \n",wPhysicBlkNum,gCurrentBankNum);
		ret_debug = SetBadFlagIntoNand(wPhysicBlkNum, NAND_USER_BAD_TAG);
		if(ret_debug!=0)
		{
			NF_DATA_ERROR("Warning! Set bad block failed!!   <function:PutFreeBlkIntoRecycleFifo> \n");
			return 0x44;
		}
		gBankInfo[gCurrentBankNum].wUserBadBlkNum++;
		
		return -1;
	}	
	else
	{
		if(gSTNandDataCfgInfo.uiBankNum == gSTNandDataCfgInfo.uiMM_Num)	
		{		
			mm[gCurrentBankNum].pMaptable[gSTNandDataCfgInfo.uiBankSize+gBankInfo[gCurrentBankNum].wRecycleBlkNum] = wPhysicBlkNum;	
			gMaptableChanged[gCurrentBankNum] = MAPTAB_WRITE_BACK_NEED;// Changed
		}
		else
		{
			mm[0].pMaptable[gSTNandDataCfgInfo.uiBankSize+gBankInfo[gCurrentBankNum].wRecycleBlkNum] = wPhysicBlkNum;	
			gMaptableChanged[0] = MAPTAB_WRITE_BACK_NEED;	// Changed
		}
		
		gBankInfo[gCurrentBankNum].wRecycleBlkNum++;
		if(gBankInfo[gCurrentBankNum].wRecycleBlkNum > gSTNandDataCfgInfo.uiBankRecSize)
		{		
			//RWFail_Debug();
			//Sys_Exception(NF_BANK_RECYCLE_NUM_ERR);
			NF_DATA_ERROR("Error!Bank:%d RecycleNum:%d   <function:PutFreeBlkIntoRecycleFifo> \n",gBankInfo[gCurrentBankNum].wRecycleBlkNum,gCurrentBankNum);
			return 0x44;
		}		
	}
  	
  	if(gBankInfo[gCurrentBankNum].wStatus == MAPTAB_VALID)	// Need write start flag
	{		
		if(gWorkBufferNum==1)	// Single workbuffer
		{
			gLastPage_Read 	= 0xffffffff;
			gLastPage_Write	= 0xffffffff;
			
			if((gpWorkBufferBlock!=NULL) && (gpWorkBufferBlock->wLogicalPage != NAND_ALL_BIT_FULL_W))
			{
				ret_debug = WriteBackWorkbuffer(gpWorkBufferBlock);
				if(ret_debug==0x44)
				{
					NF_DATA_ERROR("Error:I`m dead 1st,  <function:PutFreeBlkIntoRecycleFifo>\n");
#ifdef ENABLE_FULL_MONITOR_DEBUG
					dump_bankinfo();
					dump_curren_bank_maptable();
					dump_exchange_block();
#endif					
					return 0x44;
				}
			}			
		}
		else if(gWorkBufferNum==2)
		{
			gLastPage_Read 	= 0xffffffff;
		}
			
		gBankInfo[gCurrentBankNum].wStatus = MAPTAB_INVALID;
		
		BankInfoAreaWrite((UINT32)gBankInfo,(gSTNandDataCfgInfo.uiBankNum*sizeof(gBankInfo[0]))<<BYTE_WORD_SHIFT);
		//DBG_PRINT("\r \n gCurrent Bank:%d",gCurrentBankNum);
		//DBG_PRINT("\r \n MAPTAB_INVALID -- Put Free Blk");
	}
	
	return 0;
}

SINT32 NandSetOrgLogicBlock(UINT16 wLogicBlk,UINT16 wType)
{
	UINT16  wPhysicBlk,wSwapBlk;
	UINT32	wPhysicalPage;	
	UINT32	i,ret=0; 
	UINT8   count = 0;

	if(gSTNandDataCfgInfo.uiBankNum == gSTNandDataCfgInfo.uiMM_Num)	
	{		
		wPhysicBlk = mm[gCurrentBankNum].Maptable.Logic2Physic[wLogicBlk];
	}
	else
	{
		wPhysicBlk = mm[0].Maptable.Logic2Physic[wLogicBlk];
	}
	
	wPhysicalPage = wPhysicBlk << gSTNandDataHalInfo.wBlk2Page;	
	
	ret = ReadDataFromNand(wPhysicalPage, tt_extend_ptr);	 // TBD 加强处理
	count = ((GetCArea(NAND_C_AREA_COUNT_OFS,tt_extend_ptr))&0x7f);
	if(ret!=0)
	{		
		do
		{
			wSwapBlk = GetFreeBlkFromRecycleFifo();
			for(i=0;i<gSTNandDataHalInfo.wBlkPageNum;i++)
			{
				ReadDataFromNand(wPhysicalPage+i, tt_extend_ptr);
				PutCArea(NAND_C_AREA_COUNT_OFS,		count);
				PutCArea(NAND_C_AREA_BAD_FLG_OFS_1,			NAND_ALL_BIT_FULL_B);
				PutCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS,		wLogicBlk);
				PutCArea(NAND_C_AREA_BAD_FLG_OFS_6,			NAND_ALL_BIT_FULL_B);
				PutCArea(NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS,	wType);
			
				ret = WriteDataIntoNand((wSwapBlk << gSTNandDataHalInfo.wBlk2Page)+i, tt_extend_ptr);
				if(ret!=0)
				{
					SetBadFlagIntoNand(wSwapBlk,NAND_USER_BAD_TAG);
					break;
				}
			}
		}while(ret);
		SetBadFlagIntoNand(wPhysicBlk,NAND_USER_BAD_TAG);
		UpdateMaptable(wSwapBlk, wLogicBlk);
	}
	else if(GetCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS, tt_extend_ptr)==NAND_EMPTY_BLK)
	{
		PutCArea(NAND_C_AREA_BAD_FLG_OFS_1,			NAND_ALL_BIT_FULL_B);
		PutCArea(NAND_C_AREA_COUNT_OFS,				((count + 1)&0x7f));
		PutCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS,		wLogicBlk);
		PutCArea(NAND_C_AREA_BAD_FLG_OFS_6,			NAND_ALL_BIT_FULL_B);
		PutCArea(NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS,	wType);

#ifdef ENABLE_FULL_MONITOR_DEBUG	
		NF_DATA_ERROR("Debuging! Write Page 0x%x <function:NandSetOrgLogicBlock> \n",wPhysicalPage);
#endif		
		ret = WriteDataIntoNand(wPhysicalPage, tt_extend_ptr);
		if(ret==0x44)
		{
			NF_DATA_ERROR("Error:I`m dead 1st,  <function:NandSetOrgLogicBlock>\n");
			//NF_DATA_ERROR("Error:I`m dead 1st, Bank: 0x%x <function:NandSetOrgLogicBlock>\n",wBankNum);
#ifdef ENABLE_FULL_MONITOR_DEBUG			
			dump_bankinfo();
			dump_curren_bank_maptable();
#endif
			return 0x44;
		}
		//if(ret!=0)
		while(ret)
		{			
			//NF_DATA_ERROR("Warning! Write Page 0x%x fail!<function:NandSetOrgLogicBlock> \n",wPhysicalPage);			
			SetBadFlagIntoNand(wPhysicBlk,NAND_USER_BAD_TAG);
			wPhysicBlk = GetFreeBlkFromRecycleFifo();
			
			PutCArea(NAND_C_AREA_BAD_FLG_OFS_1,			NAND_ALL_BIT_FULL_B);
			PutCArea(NAND_C_AREA_COUNT_OFS,				((count + 1)&0x7f));
			PutCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS,		wLogicBlk);
			PutCArea(NAND_C_AREA_BAD_FLG_OFS_6,			NAND_ALL_BIT_FULL_B);
			PutCArea(NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS,	wType);
			
			ret = WriteDataIntoNand(wPhysicBlk << gSTNandDataHalInfo.wBlk2Page, tt_extend_ptr);
			
			UpdateMaptable(wPhysicBlk, wLogicBlk);
		}
	}
	
	return 0;
}


/*************************************************************************
* function:		NandFlushA
*	
*	
*	
*************************************************************************/ 
SINT32 NandFlushA(UINT16 wBlkIndex, UINT16 TargetBank, SINT32 wMode, UINT16 wType)
{
	UINT16 i;
	UINT16 wPhysicBlk;
	UINT16 wSwapPhysicBlk;
	UINT16 wPageToPageVal;
	UINT16 wRetNand=0;
	UINT16 *pPagetoPage;
	UINT16 wNeedCombinFlag;
	UINT16 wExchangeBlockNum;	
	EXCHANGE_A_BLK *pExchangeBlock;
	UINT16 wErrPhysicFlag 	= 0;
	UINT16 wErrExchangeFlag = 0;
	SINT32 ret;

	if(wType == NF_EXCHANGE_A_TYPE)	// Use exchange A
	{	
		pExchangeBlock 		= &gExchangeABlock[wBlkIndex];
		wExchangeBlockNum	= ExchangeAMaxNum;
	}
	else					// Use exchange B
	{	
		pExchangeBlock 		= &gExchangeBBlock[wBlkIndex];
		wExchangeBlockNum	= ExchangeBMaxNum;
	}
	
	if(wBlkIndex>(wExchangeBlockNum-1))
	{
		//RWFail_Debug();
		NF_DATA_ERROR("Warning! Index 0x%x illegal,type:0x%x   <function:NandFlushA> \n",wBlkIndex,wType);
		return -1;
	}
	
	if(pExchangeBlock->wPhysicBlk!=NAND_ALL_BIT_FULL_W)	// Flush exchange block data to nand 
	{
		if(gWorkBufferNum==1)
		{
			gLastPage_Read = 0xffffffff;
			gLastPage_Write = 0xffffffff;
			//NF_DATA_DEBUG("+++++ Clear gLastPage_Read & gLastPage_Write! +++++  \r \n");
		}
		else if(gWorkBufferNum == 2)
		{
			gLastPage_Read = 0xffffffff;
		}
	
		if((gWorkBufferNum==1)&&(gpWorkBufferBlock!=NULL) && (gpWorkBufferBlock->wLogicalPage!= NAND_ALL_BIT_FULL_W)) // 2009 11 16 modify
		{
			//NF_DBG_PRINT(" ------ Nand flash, Write Back Workbuffer -----   \r \n");
			ret = WriteBackWorkbuffer(gpWorkBufferBlock);
			if(ret==0x44)
			{
				NF_DATA_ERROR("Error:I`m dead 123st,  <function:NandFlushA>\n");
#ifdef ENABLE_FULL_MONITOR_DEBUG
				dump_bankinfo();
				dump_curren_bank_maptable();
				dump_exchange_block();
#endif				
				return 0x44;
			}
		}
		else if(gWorkBufferNum==2)	// zurong fix 2 workbuffer flush bug,	2010.05.11
		{		
			if((gpWorkBufferBlock!=NULL) && (gpWorkBufferBlock->wLogicalPage!= NAND_ALL_BIT_FULL_W)&&(gpWorkBufferBlock->wLogicBlk == pExchangeBlock->wLogicBlk))
			{
				//NF_DBG_PRINT(" ------ Nand flash, Write Back Workbuffer -----   \r \n");
				ret=WriteBackWorkbuffer(gpWorkBufferBlock);	
				if(ret==0x44)
				{
					NF_DATA_ERROR("Error:I`m dead 234st,  <function:NandFlushA>(2)\n");
#ifdef ENABLE_FULL_MONITOR_DEBUG
					dump_bankinfo();
					dump_curren_bank_maptable();
					dump_exchange_block();
#endif					
					return 0x44;
				}		
			}		
		}
		
		if(gCurrentBankNum!=pExchangeBlock->wBank)
		{
			ret = (SINT32)ChangeBank(pExchangeBlock->wBank);	// destroy maptable_ptr
			if(ret==0x44)
			{
				NF_DATA_ERROR("Error:I`m dead 1st,  <function:NandFlushA>(3)\n");
#ifdef ENABLE_FULL_MONITOR_DEBUG
				dump_bankinfo();
#endif				
				return 0x44;
			}
			if(ret!=0)
			{
				NF_DATA_ERROR("Error! ChangeBank 0x%x fail   <function:NandFlushA> \n",pExchangeBlock->wBank);
				return -1;
			}
		}

		if(gSTNandDataCfgInfo.uiBankNum == gSTNandDataCfgInfo.uiMM_Num)	
		{
			wPhysicBlk = mm[gCurrentBankNum].Maptable.Logic2Physic[pExchangeBlock->wLogicBlk];
		}
		else
		{
			wPhysicBlk = mm[0].Maptable.Logic2Physic[pExchangeBlock->wLogicBlk];
		}

		pPagetoPage = (UINT16*)(pExchangeBlock->wPage2PageTable);

		if(((pExchangeBlock->wCount)&0xff) == NAND_ALL_BIT_FULL_B)	//zurong add ,must get count first.
		{
			ReadDataFromNand((UINT32)wPhysicBlk<<gSTNandDataHalInfo.wBlk2Page, tt_extend_ptr);
			pExchangeBlock->wCount = (GetCArea(NAND_C_AREA_COUNT_OFS, tt_extend_ptr)+1)&0x7f;
		}
		
		wNeedCombinFlag = 0;

		for(i=0;i<(pExchangeBlock->wCurrentPage);i++)
		{			
			if((*(pPagetoPage + i))!=i)	// Judge if need get new block
			{
				wNeedCombinFlag = 0x01;	
				break;
			}
		}
		
		if(wNeedCombinFlag!=0)	// Need get new block and combine
		{			
			//NF_DATA_ERROR ("\r\n=====Combin Exchange & Org Block to New Block! ====\r\n");
			
			do	
			{
				wSwapPhysicBlk = GetFreeBlkFromRecycleFifo();
				if(NF_NO_SWAP_BLOCK == wSwapPhysicBlk)
				{
					NF_DATA_ERROR("No swap block!   <function:NandFlushA> \n");
					return -1;
				}

				for(i=0;i<gSTNandDataHalInfo.wBlkPageNum;i++)
				{
					wPageToPageVal = *(pPagetoPage + i);
					
					if(wPageToPageVal == NAND_ALL_BIT_FULL_W)
					{						
						ret = ReadDataFromNand(((UINT32)wPhysicBlk<<gSTNandDataHalInfo.wBlk2Page) + i, tt_extend_ptr);
						if(ret!=0)
						{
							if(ret==NF_READ_ECC_ERR)
							{
								NF_DATA_ERROR("Error! Read page 0x%x fail 1    <function:NandFlushA> \n",((UINT32)wPhysicBlk<<gSTNandDataHalInfo.wBlk2Page) + i);
								//return -1;
#ifdef POWER_LOST_DEBUG_CHECK
							dump_power_lost_check_info(wPhysicBlk,pExchangeBlock->wPhysicBlk,i,0,tt_extend_ptr);
#endif								
							}else
							{
								NF_DATA_ERROR("Warning! Read page 0x%x fail 1   <function:NandFlushA> \n",((UINT32)wPhysicBlk<<gSTNandDataHalInfo.wBlk2Page) + i);								
							}
							wErrPhysicFlag = 1;
						}
					}
					else
					{
						ret = ReadDataFromNand((((UINT32)(pExchangeBlock->wPhysicBlk))<<gSTNandDataHalInfo.wBlk2Page) + wPageToPageVal, tt_extend_ptr);
						if(ret!=0)
						{
							if(ret==NF_READ_ECC_ERR)
							{
								NF_DATA_ERROR("Error! Read page 0x%x fail 2    <function:NandFlushA> \n",(((UINT32)(pExchangeBlock->wPhysicBlk))<<gSTNandDataHalInfo.wBlk2Page) + wPageToPageVal);
								//return -1;
#ifdef POWER_LOST_DEBUG_CHECK
							dump_power_lost_check_info(wPhysicBlk,pExchangeBlock->wPhysicBlk,wPageToPageVal,1,tt_extend_ptr);
#endif									
							}else
							{
								NF_DATA_ERROR("Warning! Read page 0x%x fail 2   <function:NandFlushA> \n",(((UINT32)(pExchangeBlock->wPhysicBlk))<<gSTNandDataHalInfo.wBlk2Page) + wPageToPageVal);								
							}
							wErrExchangeFlag = 1;
						}							
					}
					PutCArea(NAND_C_AREA_BAD_FLG_OFS_1,			NAND_ALL_BIT_FULL_B);
					PutCArea(NAND_C_AREA_COUNT_OFS,				(pExchangeBlock->wCount+1)&0x7f);
					PutCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS,		pExchangeBlock->wLogicBlk);
					PutCArea(NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS,	wType);
					PutCArea(NAND_C_AREA_BAD_FLG_OFS_6,			NAND_ALL_BIT_FULL_B);
					PutCArea(NAND_C_AREA_PAGE_TO_PAGE_OFS,		i);
					//NF_DATA_ERROR("Combin...................... \n");
					wRetNand = 	WriteDataIntoNand(((UINT32)wSwapPhysicBlk << gSTNandDataHalInfo.wBlk2Page) + i, tt_extend_ptr);
					if(wRetNand==0x44)
					{
						NF_DATA_ERROR("Error:I`m dead 1st,  <function:NandFlushA>(4)\n");
#ifdef ENABLE_FULL_MONITOR_DEBUG
						dump_bankinfo();
						dump_curren_bank_maptable();
#endif						
						return 0x44;
					}
					if(wRetNand)
					{
						NF_DATA_ERROR("Warning! Write page 0x%x fail   <function:NandFlushA> \n",((UINT32)wSwapPhysicBlk << gSTNandDataHalInfo.wBlk2Page) + i);
						wRetNand = nand_write_status_get();
						if((NFCMD_STATUS)wRetNand<=NF_PROGRAM_ERR13)
						{
							NF_DATA_ERROR("Error! Write page 0x%x protect! <  function:NandFlushA> \n",((UINT32)wSwapPhysicBlk << gSTNandDataHalInfo.wBlk2Page) + i);
							return NF_NAND_PROTECTED;
						}
						//NF_DBG_PRINT(" !!! Set bad block, wSwapPhysicBlk !!!  \r \n ");
						wRetNand = SetBadFlagIntoNand(wSwapPhysicBlk,NAND_USER_BAD_TAG);
						if(wRetNand!=0)
						{
							NF_DATA_ERROR("Error:Set bad block failed!!,  <function:NandFlushA>(4)\n");
#ifdef ENABLE_FULL_MONITOR_DEBUG
							dump_bankinfo();
							dump_curren_bank_maptable();
#endif							
							return 0x44;
						}
						gBankInfo[gCurrentBankNum].wUserBadBlkNum++;
						break;
					}
				}
			}while(wRetNand);
			
			if(wErrPhysicFlag != 1) //if have read fail,no recycle
			{
				wRetNand = PutFreeBlkIntoRecycleFifo(wPhysicBlk);
				if(wRetNand!=0)
				{
					NF_DATA_ERROR("Error:PutFreeBlkIntoRecycleFifo failed!!,  <function:NandFlushA>(4)\n");
#ifdef ENABLE_FULL_MONITOR_DEBUG
					dump_bankinfo();
					dump_curren_bank_maptable();
#endif					
					return 0x44;
				}
			}
			else
			{
				//NF_DBG_PRINT(" !!! Set bad block,wPhysicBlk !!!  \r \n ");
				wRetNand = SetBadFlagIntoNand(wPhysicBlk,NAND_USER_UNSTABLE_TAG);
				if(wRetNand!=0)
				{
					NF_DATA_ERROR("Error:Set bad block failed!!,  <function:NandFlushA>(2)\n");
#ifdef ENABLE_FULL_MONITOR_DEBUG
					dump_bankinfo();
					dump_curren_bank_maptable();
#endif					
					return 0x44;
				}
				gBankInfo[gCurrentBankNum].wUserBadBlkNum++;
			}
			
			if(wErrExchangeFlag!=1)
			{
				wRetNand = PutFreeBlkIntoRecycleFifo(pExchangeBlock->wPhysicBlk);
				if(wRetNand!=0)
				{
					NF_DATA_ERROR("Error:PutFreeBlkIntoRecycleFifo failed!!,< function:NandFlushA>(111)\n");
#ifdef ENABLE_FULL_MONITOR_DEBUG
					dump_bankinfo();
					dump_curren_bank_maptable();
#endif					
					return 0x44;
				}
			}
			else
			{
				wRetNand = SetBadFlagIntoNand(pExchangeBlock->wPhysicBlk,NAND_USER_UNSTABLE_TAG);
				if(wRetNand!=0)
				{
					NF_DATA_ERROR("Error:Set bad block failed!!, <function:NandFlushA>(10)\n");
#ifdef ENABLE_FULL_MONITOR_DEBUG
					dump_bankinfo();
					dump_curren_bank_maptable();
#endif					
					return 0x44;
				}
				gBankInfo[gCurrentBankNum].wUserBadBlkNum++;
			}			
			
			UpdateMaptable(wSwapPhysicBlk, pExchangeBlock->wLogicBlk);
		}
		else
		{
			if(pExchangeBlock->wCurrentPage != 0)	// Empty Block
			{				
				//NF_DATA_ERROR ("\r\n=====Copy Org Block to Exchange Block! ====\r\n");
				for(i=pExchangeBlock->wCurrentPage;i<gSTNandDataHalInfo.wBlkPageNum;i++)
				{			
					ret = ReadDataFromNand(((UINT32)wPhysicBlk<<gSTNandDataHalInfo.wBlk2Page) + i,tt_extend_ptr);				
					if(ret!=0)
					{
						if(ret==NF_READ_ECC_ERR)
						{
							NF_DATA_ERROR("Error! Read page 0x%x fail 3   <function:NandFlushA> \n",((UINT32)wPhysicBlk<<gSTNandDataHalInfo.wBlk2Page) + i);
#ifdef POWER_LOST_DEBUG_CHECK							
							dump_power_lost_check_info(wPhysicBlk,pExchangeBlock->wPhysicBlk,i,0,tt_extend_ptr);
#endif							
						}else
						{
							NF_DATA_ERROR("Warning! Read page 0x%x fail 3   <function:NandFlushA> \n",((UINT32)wPhysicBlk<<gSTNandDataHalInfo.wBlk2Page) + i);							
						}
						wErrPhysicFlag = 1;
					}					
					
					PutCArea(NAND_C_AREA_BAD_FLG_OFS_1,		NAND_ALL_BIT_FULL_B		);
					PutCArea(NAND_C_AREA_COUNT_OFS,			pExchangeBlock->wCount		);
					PutCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS,	pExchangeBlock->wLogicBlk	);
					PutCArea(NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS,wType					);
					PutCArea(NAND_C_AREA_BAD_FLG_OFS_6,		NAND_ALL_BIT_FULL_B		);
					PutCArea(NAND_C_AREA_PAGE_TO_PAGE_OFS,		i						);
					//NF_DATA_ERROR ("\r\n=====Copy a page ................ ====\r\n");
					wRetNand = 	WriteDataIntoNand(((UINT32)(pExchangeBlock->wPhysicBlk)<< gSTNandDataHalInfo.wBlk2Page) + i,tt_extend_ptr);
					if(wRetNand==0x44)
					{
						NF_DATA_ERROR("Error:I`m dead 1st,  <function:NandFlushA>(5)\n");
#ifdef ENABLE_FULL_MONITOR_DEBUG
						dump_bankinfo();
						dump_curren_bank_maptable();
						dump_exchange_block();
#endif						
						return 0x44;
					}
					if(wRetNand)
					{
						NF_DATA_ERROR("Warning! Write page 0x%x fail 2   <function:NandFlushA> \n",((UINT32)(pExchangeBlock->wPhysicBlk)<< gSTNandDataHalInfo.wBlk2Page) + i);
						wRetNand = nand_write_status_get();
						if((NFCMD_STATUS)wRetNand<=NF_PROGRAM_ERR13)
						{	
							NF_DATA_ERROR("Error! Write page 0x%x protect 2!   <function:NandFlushA> \n",((UINT32)(pExchangeBlock->wPhysicBlk)<< gSTNandDataHalInfo.wBlk2Page) + i);							return NF_NAND_PROTECTED;
						}		
					}
				}				
				
				if(wErrPhysicFlag != 1) //if have read fail,no recycle
				{
					wRetNand = PutFreeBlkIntoRecycleFifo(wPhysicBlk);
					if(wRetNand!=0)
					{
						NF_DATA_ERROR("Error:PutFreeBlkIntoRecycleFifo failed,  <function:NandFlushA>(010)\n");
#ifdef ENABLE_FULL_MONITOR_DEBUG
						dump_bankinfo();
						dump_curren_bank_maptable();
						dump_exchange_block();
#endif						
						return 0x44;
					}
				}
				else
				{
					//NF_DBG_PRINT(" !!! Set bad block,wPhysicBlk !!!  \r \n ");
					wRetNand = SetBadFlagIntoNand(wPhysicBlk,NAND_USER_UNSTABLE_TAG);
					if(wRetNand!=0)
					{
						NF_DATA_ERROR("Error:Set bad block failed!!,  <function:NandFlushA>(11)\n");
#ifdef ENABLE_FULL_MONITOR_DEBUG
						dump_bankinfo();
						dump_curren_bank_maptable();
#endif						
						return 0x44;
					}
					gBankInfo[gCurrentBankNum].wUserBadBlkNum++;
				}

				UpdateMaptable(pExchangeBlock->wPhysicBlk, pExchangeBlock->wLogicBlk);
			}
		}

		for(i=0;i<(gSTNandDataHalInfo.wBlkPageNum);i++)	// Clear page to page value
		{
			*(pPagetoPage + i) = NAND_ALL_BIT_FULL_W;
		}
		
		pExchangeBlock->wBank		= NAND_ALL_BIT_FULL_W;
		pExchangeBlock->wLogicBlk	= NAND_ALL_BIT_FULL_W;
		pExchangeBlock->wPhysicBlk	= NAND_ALL_BIT_FULL_W;
		pExchangeBlock->wCurrentPage= 0;
		pExchangeBlock->wCount		= NAND_ALL_BIT_FULL_B;	// zurong add
		pExchangeBlock->wType		= NAND_ALL_BIT_FULL_B;
		
		if(gBankInfo[gCurrentBankNum].wExchangeBlkNum!=0)
		{
			gBankInfo[gCurrentBankNum].wExchangeBlkNum--;		
		}	
		//DBG_PRINT("\r \n --");
	}	

	if(wMode&NAND_BUF_GET_EXCHANGE)
	{	
		if(gCurrentBankNum!=TargetBank)
		{
			ret = (SINT32)ChangeBank(TargetBank);
			if(ret==0x44)
			{
				NF_DATA_ERROR("Error:I`m dead 1st,  <function:NandFlushA>(6)\n");
				NF_DATA_ERROR("Error:I`m dead 1st, Bank: 0x%x   <function:NandFlushA>(6)\n",gCurrentBankNum);
				dump_bankinfo();
				return 0x44;
			}
			if(ret!=0)
			{
				NF_DATA_ERROR("Error! ChangeBank2 0x%x fail   <function:NandFlushA> \n",pExchangeBlock->wBank);
				return -1;
			}
		}

		//NF_DBG_PRINT("Get an new Block!  \r \n ");
		
		pExchangeBlock->wPhysicBlk 	= GetFreeBlkFromRecycleFifo();
		pExchangeBlock->wBank		= TargetBank;
		pExchangeBlock->wCurrentPage= 0;
		pExchangeBlock->wCount		= NAND_ALL_BIT_FULL_B;
		pExchangeBlock->wLogicalPage= NAND_ALL_BIT_FULL_W;
		
		if(NF_NO_SWAP_BLOCK == pExchangeBlock->wPhysicBlk)
		{
			//RWFail_Debug();
			NF_DATA_ERROR("No swap block 2!   <function:NandFlushA> \n");
			return -1;
		}
		
		pPagetoPage = (UINT16*)(pExchangeBlock->wPage2PageTable);		
		for(i = 0;i < gSTNandDataHalInfo.wBlkPageNum;i++)
		{
			pPagetoPage[i] = NAND_ALL_BIT_FULL_W;
		}
		
		if(gBankInfo[gCurrentBankNum].wExchangeBlkNum<(ExchangeAMaxNum+ExchangeBMaxNum))
		{
			gBankInfo[gCurrentBankNum].wExchangeBlkNum++;
		}
		
		//DBG_PRINT("\r \n ++");		
	}

	return 0;
}


/*************************************************************************
* function:		DrvNand_flush_allblk
*	
*	
*	
*************************************************************************/
UINT16	DrvNand_flush_allblk(void)
{
	SINT32 i;
	
	Nand_OS_LOCK();
	
	NF_DATA_ERROR("Debuging! Flush all blk!!!    <function:DrvNand_flush_allblk> \n");
	
	for(i=0;i<ExchangeAMaxNum;i++)
	{
		NandFlushA(i,0,NAND_BUF_FLUSH_ALL,NF_EXCHANGE_A_TYPE);
	}
	NF_DATA_ERROR("Debuging! Flush all blk!!! 22   <function:DrvNand_flush_allblk> \n");
	for(i=0;i<ExchangeBMaxNum;i++)
	{
		NandFlushA(i,0,NAND_BUF_FLUSH_ALL,NF_EXCHANGE_B_TYPE);
	}

	ChangeReadErrBlk();
	NF_DATA_ERROR("Debuging! Flush all blk!!! 33   <function:DrvNand_flush_allblk> \n");
	FlushCurrentMaptable();
	NF_DATA_ERROR("Debuging! Flush all blk!!! 44   <function:DrvNand_flush_allblk> \n");
	gLastPage_Write = 0xffffffff;
	gLastPage_Read = 0xffffffff;
	
	Nand_OS_UNLOCK();
	
	
	return 0;
}

SINT16 CopyPhysicalBlock(UINT16 wSrcBlk,UINT16 wTarBlk)
{
	UINT16 		i;
	UINT32		wSRCPageNum;
	UINT32		wTARPageNum;
	SINT32		ret;
	//SINT16		copy_status = 0;
	
	wSRCPageNum = (UINT32)wSrcBlk << gSTNandDataHalInfo.wBlk2Page;
	wTARPageNum = (UINT32)wTarBlk << gSTNandDataHalInfo.wBlk2Page; 
	
	for(i=0;i<gSTNandDataHalInfo.wBlkPageNum;i++)
	{
		ret = ReadDataFromNand(wSRCPageNum+i, tt_extend_ptr);		
		ret = WriteDataIntoNand(wTARPageNum+i, tt_extend_ptr);
		if(ret!=0)	// TBD  write fail handle
		{
			//return -1;
		}
	}
	
	return 0;
}
// TBD Copy fail handle
SINT32 CopyMultiPhysicalPage(UINT16 wSRCBlkNum, UINT16 wTARBlkNum, UINT16 wStartPage, UINT16 wEndPage, UINT16 wLogicBlkNum,UINT16 wType)	// zurong add
{
	UINT16 		i;
	UINT32		wSRCPageNum;
	UINT32		wTARPageNum;
	//UINT16   	RetNand;
	UINT16  	wErrFlag = 0;
	SINT32		ret;
	
	wSRCPageNum = (UINT32)wSRCBlkNum << gSTNandDataHalInfo.wBlk2Page;
	wTARPageNum = (UINT32)wTARBlkNum << gSTNandDataHalInfo.wBlk2Page; 

	NF_DATA_ERROR("Debuging! Copy page start 0x%x ,end 0x%x  <function:CopyMultiPhysicalPage> \n",wStartPage,wEndPage);
	for(i = wStartPage;i <= wEndPage;i++)
	{
		ret = ReadDataFromNand(wSRCPageNum+i, tt_extend_ptr);
		if(ret!=0)
		{
			if(ret==NF_READ_ECC_ERR)
			{
				NF_DATA_ERROR("Error! Read page 0x%x fail    <function:CopyMultiPhysicalPage> \n",wSRCPageNum+i);
				//return -1;
			}else
			{
				NF_DATA_ERROR("warning! Read page 0x%x fail    <function:CopyMultiPhysicalPage> \n",wSRCPageNum+i);				
			}
			wErrFlag = 1;
		}
				
		PutCArea(NAND_C_AREA_BAD_FLG_OFS_1,				NAND_ALL_BIT_FULL_B 	);
		PutCArea(NAND_C_AREA_COUNT_OFS,					NAND_ALL_BIT_FULL_B 	);
		PutCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS,			wLogicBlkNum			);
		PutCArea(NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS,		wType 					);
		PutCArea(NAND_C_AREA_BAD_FLG_OFS_6,				NAND_ALL_BIT_FULL_B	);
		PutCArea(NAND_C_AREA_PAGE_TO_PAGE_OFS,			NAND_ALL_BIT_FULL_W 	);
	
		ret = WriteDataIntoNand(wTARPageNum+i, tt_extend_ptr);
		if(ret==0x44)
		{
			NF_DATA_ERROR("Error:I`m dead 1st,  <function:CopyMultiPhysicalPage>\n");
			//NF_DATA_ERROR("Error:I`m dead 1st, Bank: 0x%x <function:WriteMapTableToNand>\n",wBankNum);
			dump_bankinfo();
			dump_curren_bank_maptable();
			return 0x44;
		}
		if(ret)
		{
			NF_DATA_ERROR("Warning! Write page 0x%x fail   <function:CopyMultiPhysicalPage> \n",wTARPageNum+i);
			ret = nand_write_status_get();
			if((NFCMD_STATUS)ret<=NF_PROGRAM_ERR13)
			{				
				return NF_NAND_PROTECTED;
			}
			else
			{
				return NF_PHY_WRITE_FAIL;
			}
		}
	}

	if(wErrFlag == 1)
		return NF_PHY_READ_FAIL;
		
	return 0;
}

/*************************************************************************
* function:		ReadDataFromNand
*	
*	
*	
*************************************************************************/
//return 0是OK；其他的均为有bit err
//SINT32 ReadDataFromNand_Normal(UINT32 wPhysicPageNum)
SINT32 ReadDataFromNand(UINT32 wPhysicPageNum, UINT8 *tt_ptr)
{
	SINT32 RetNand; 
	UINT16 BCH_err_cnt;
	UINT16 retry = 0;
	
	RetNand = Nand_ReadPhyPage(wPhysicPageNum, (UINT32)tt_ptr);
	if(0!=RetNand)
	{
		retry++;
		RetNand = Nand_ReadPhyPage(wPhysicPageNum, (UINT32)tt_ptr);
	}
	
	if(0!=RetNand)	// BCH Error
	{
		RWFail_Debug();
		NF_DATA_ERROR("ReadDataFromNand ECC ERROR!!   <function:ReadDataFromNand> \n");
		return NF_READ_ECC_ERR;		// ECC error, can`t correct
	}
	else	// BCH OK
	{
		BCH_err_cnt = nand_bch_err_bits_get();		
		
		#if defined(NF_DATA_DEBUG_PRINT_READ_RETRY)
		if (retry > 0) {
			NF_DATA_ERROR("Warning, Nand retry read once become ok:0x%x  Error page:0x%x   \n",BCH_err_cnt,wPhysicPageNum);
			NF_DATA_ERROR("Warning <function:ReadDataFromNand> \n");
			#if 1
			//dump_buffer(tt_ptr, nand_page_size_get());
			dump_buffer(tt_ptr, gSTNandDataHalInfo.wPageSize);
			dump_buffer(nand_get_spare_buf(), 1024);
			#endif
		}
		#endif
		
		if(BCH_err_cnt >= GetBitErrCntAsBadBlk())
		{
			RWFail_Debug();
			NF_DATA_ERROR("Warning, Data ECC over threshold:0x%x  Error page:0x%x   <function:ReadDataFromNand> \n",BCH_err_cnt,wPhysicPageNum);
			return NF_READ_BIT_ERR_TOO_MUCH;	// More than 5 bit error,but corrected
		}
	}
	return 0;							// No bit error
}

/*************************************************************************
* function:		WriteDataIntoNand_Normal
*	
*	
*	
*************************************************************************/
SINT32 WriteDataIntoNand(UINT32 wPhysicPageNum,UINT8 *tt_ptr)
{	
	SINT32 RetNand;
	UINT16 wPhysicBlkNum;
	
	wPhysicBlkNum = wPhysicPageNum>>gSTNandDataHalInfo.wBlk2Page;
	if(wPhysicBlkNum<gSTNandDataCfgInfo.uiDataStart)
	{
		NF_DATA_ERROR("##-->Serious error: Data beyond range!! Nand write page: 0x%x  \n",wPhysicPageNum);
		return 2;
	}
#ifdef PART0_WRITE_MONITOR_DEBUG	
	RetNand = Check_Part0_page(wPhysicPageNum);
	if(RetNand!=0)
	{
		NF_DATA_ERROR("Error: Check  Write page 0x%x fail!! <function:WriteDataIntoNand>  \n",wPhysicPageNum);
		dump_curren_bank_maptable();
		dump_bankinfo();
		dump_exchange_block();
		dump_debug_bank_maptable();			
		return 0x44;
	}
#endif	
	RetNand = Nand_WritePhyPage(wPhysicPageNum, (UINT32)tt_ptr);
	if(RetNand!=0)
	{
		if(RetNand!=0x44)
		{
			NF_DATA_ERROR("Warning: Write page 0x%x fail!! <function:WriteDataIntoNand>  \n",wPhysicPageNum);
			return 1;
		}
		else
		{
			NF_DATA_ERROR("Error: I will be dead!! Write page 0x%x fail!! <function:WriteDataIntoNand>  \n",wPhysicPageNum);
			return 0x44;
		}
	}
	return 0;
}

/*************************************************************************
* function:		DataEraseNandBlk
*	
*	
*	
*************************************************************************/
SINT32 DataEraseNandBlk(UINT16 wPhysicBlkNum)
{
	SINT32 wRet;
	
	if(wPhysicBlkNum<gSTNandDataCfgInfo.uiDataStart)
	{
		NF_DATA_ERROR("##-->Serious error: Data beyond range!! Erase block: 0x%x  \n",wPhysicBlkNum);
		return 2;
	}
#ifdef PART0_WRITE_MONITOR_DEBUG	
	wRet = Check_Part0_Physical(wPhysicBlkNum);
	if(wRet!=0)
	{
		NF_DATA_ERROR(" Erase illage block!!! Bank 0x%x BankNum:0x%x <function:DataEraseNandBlk>  \n",gCurrentBankNum);
		dump_curren_bank_maptable();
		dump_bankinfo();
		dump_exchange_block();
		dump_debug_bank_maptable();
		return 0x44;
	}
#endif	
	wRet =  Nand_ErasePhyBlock(wPhysicBlkNum);	
	if(wRet & 0x01)
	{
		NF_DATA_ERROR("Warning: Erase block 0x%x fail!! <function:DataEraseNandBlk>  \n",wPhysicBlkNum);
		return 1;
	}
	else
	{
		return 0;
	}
}

/*************************************************************************
* function:  FixupIllegalBlock  
*
*
*
**************************************************************************/
SINT16 FixupIllegalBlock(UINT16* pIllegalTable , UINT16 wCount)
{
	UINT16 i,j,k,l,m;
	UINT16 wIllegalLogicNum,todo_block;//,not_safe_block;
	IBT wIllegalPhyNum[10];
	IBT wTodoIllegalPhyNum[10];
	//IBT wNotSafePhyNum[10];
	IBT Temp_IBT;
	UINT16 wExchangeBlockNum=0;
	UINT16 wExchangeBlkNumInCurBank;
	EXCHANGE_A_BLK* pBlk=(EXCHANGE_A_BLK*)0;
	SINT32 ret;
	UINT32 block_in_maptable;
	UINT32 pages1,pages2;
	SINT16 safe_block_flg;
	UINT16 swap_block;
	
	wExchangeBlkNumInCurBank = 0;
	
	for(i=0;i<wCount;i++)
	{	
		k = 0;
		todo_block = 0;
		//not_safe_block = 0;
		block_in_maptable = NAND_ALL_BIT_FULL_W;
		pBlk=(EXCHANGE_A_BLK*)0;
		wExchangeBlockNum = 0;
		
		wIllegalLogicNum = ((IBT*)pIllegalTable)[i].wLogicBlkNum;
		if(wIllegalLogicNum == NAND_ALL_BIT_FULL_W)
		{
			continue;
		}
#ifdef PART0_WRITE_MONITOR_DEBUG		
		ret = Check_Part0_logical(wIllegalLogicNum);
		if(ret!=0)
		{
			NF_DATA_ERROR("Illegal logical block found!! <function:FixupIllegalBlock> \n");
			NF_DATA_ERROR("Logical Block:0x%x  Physical block:0x%x <function:FixupIllegalBlock> \n",wIllegalLogicNum, ((IBT*)pIllegalTable)[i].wPhysicBlkNum);
			dump_curren_bank_maptable();
			dump_bankinfo();
			dump_exchange_block();
			dump_debug_bank_maptable();	
			NF_DATA_ERROR("Illegal logical block found!! <function:FixupIllegalBlock> \n");
			return -1;
		}
#endif		
		wIllegalPhyNum[k++] = ((IBT *)pIllegalTable)[i];
		((IBT *)pIllegalTable)[i].wLogicBlkNum = NAND_ALL_BIT_FULL_W;
		for (j= i+1;j<wCount;j++)
		{
			if(wIllegalLogicNum == ((IBT *)pIllegalTable)[j].wLogicBlkNum)
			{
				wIllegalPhyNum[k++] = ((IBT *)pIllegalTable)[j];
				((IBT *)pIllegalTable)[j].wLogicBlkNum = NAND_ALL_BIT_FULL_W;
			}
		}		
		
		if(mm[0].Maptable.Logic2Physic[wIllegalLogicNum] !=  NAND_MAPTAB_UNUSE)
		{				
			wIllegalPhyNum[k].wPhysicBlkNum = mm[0].Maptable.Logic2Physic[wIllegalLogicNum];
			wIllegalPhyNum[k].wLogicBlkNum  = wIllegalLogicNum;
			k++;
			if(gBankInfo[gCurrentBankNum].wStatus==MAPTAB_VALID)
			{
				block_in_maptable = mm[0].Maptable.Logic2Physic[wIllegalLogicNum];
			}
			mm[0].Maptable.Logic2Physic[wIllegalLogicNum] = NAND_MAPTAB_UNUSE;
			
		}
		else if(gBankInfo[gCurrentBankNum].wStatus==MAPTAB_VALID)
		{
			gBankInfo[gCurrentBankNum].wStatus=MAPTAB_INVALID;
			//return -1; // goto rebuild maptable
		}
		
		if(k>3)
		{
			//NF_DBG_WHILE(1);
			NF_DATA_ERROR(" Same logical block 0x%x more than 3 block!<function:FixupIllegalBlock> \n",k);
			
			for(l=0;l<k;l++)
			{
				NF_DATA_ERROR(" Logical:0x%x  Physical:0x%x <function:FixupIllegalBlock> \n",wIllegalPhyNum[l].wLogicBlkNum,wIllegalPhyNum[l].wPhysicBlkNum);
			}
			//return -1;
		}
		else if(k==0)
		{
			continue;
		}
		
		for(l=0;l<k;l++)	// Get all the same logical blocks` counts
		{
			safe_block_flg = 0;
			for(j=0;j<gSTNandDataHalInfo.wBlkPageNum;j++)
			{			
				ret = ReadDataFromNand(((wIllegalPhyNum[l].wPhysicBlkNum)<< gSTNandDataHalInfo.wBlk2Page)+j,tt_extend_ptr);
				if(ret!=NF_READ_ECC_ERR)
				{
					break;
				}
				else
				{
					safe_block_flg = 1;
				}
			}
			
			wIllegalPhyNum[l].wCount = GetCArea(NAND_C_AREA_COUNT_OFS,tt_extend_ptr);
			if((safe_block_flg!=0) &&(wIllegalPhyNum[l].wCount==0xff))	// found empty page
			{
				PutFreeBlkIntoRecycleFifo(wIllegalPhyNum[l].wPhysicBlkNum);
				continue;
			}			
			
			if((GetCArea(NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS,tt_extend_ptr)) == NAND_EXCHANGE_A_TAG)
			{
				wExchangeBlockNum = ExchangeAMaxNum;
				pBlk 			  = gExchangeABlock;
			}
			else if((GetCArea(NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS,tt_extend_ptr)) == NAND_EXCHANGE_B_TAG)
			{
				wExchangeBlockNum = ExchangeBMaxNum;
				pBlk 			  = gExchangeBBlock;
			}
			else
			{				
				if(gBankInfo[gCurrentBankNum].wStatus==MAPTAB_VALID)
				{
					DataEraseNandBlk(wIllegalPhyNum[l].wPhysicBlkNum);
					gBankInfo[gCurrentBankNum].wStatus=MAPTAB_INVALID;
					return -1; // goto rebuild maptable
				}
				else
				{
					PutFreeBlkIntoRecycleFifo(wIllegalPhyNum[l].wPhysicBlkNum);
					continue;
				}
			}

			wTodoIllegalPhyNum[todo_block++] = wIllegalPhyNum[l];
		}		
			
		//for(l=0;l<k;l++)
		for(l=0;l<todo_block;l++)
		{
			for(m=0;m<(todo_block-l-1);m++)
			{
			 	if(wTodoIllegalPhyNum[m].wCount > wTodoIllegalPhyNum[m+1].wCount)
			 	{
			 		Temp_IBT 				= wTodoIllegalPhyNum[m];
			 		wTodoIllegalPhyNum[m] 	= wTodoIllegalPhyNum[m+1];
			 		wTodoIllegalPhyNum[m+1] = Temp_IBT;
			 	}
			}
		}
#if 1		
		if(todo_block>=2)	// check if have the same count block,then handle it
		{		    
			for(l=0;l<(todo_block-1);)
			{	
				j = l;
				
				if(wTodoIllegalPhyNum[l].wCount==wTodoIllegalPhyNum[l+1].wCount)
				{
					pages1= CheckBlockValidPages(wTodoIllegalPhyNum[l].wPhysicBlkNum);
					pages2= CheckBlockValidPages(wTodoIllegalPhyNum[l+1].wPhysicBlkNum);
					if(pages1==pages2)
					{
						//DataEraseNandBlk(wTodoIllegalPhyNum[l+1].wPhysicBlkNum);
						PutFreeBlkIntoRecycleFifo(wTodoIllegalPhyNum[l+1].wPhysicBlkNum);
						j++;
					}
					else if(pages1>pages2)
					{
						PutFreeBlkIntoRecycleFifo(wTodoIllegalPhyNum[l+1].wPhysicBlkNum);
						j++;
					}
					else
					{
						PutFreeBlkIntoRecycleFifo(wTodoIllegalPhyNum[l].wPhysicBlkNum);
					}
					
					for(j=j;j<(todo_block-1);j++)
					{
						wTodoIllegalPhyNum[j]= wTodoIllegalPhyNum[j+1];
					}
					todo_block--;
				}
				else
				{
					l++;
				}
			}
		}
#endif		
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		for(l=0;l<todo_block;l++)	// Need to think more
		{		
			if(((wTodoIllegalPhyNum[todo_block-1].wCount + 1)&0x7f)==(wTodoIllegalPhyNum[0].wCount))
			{
				Temp_IBT = wTodoIllegalPhyNum[todo_block-1];

				for(m=todo_block-1;m>0;m--)
				{
					wTodoIllegalPhyNum[m] = wTodoIllegalPhyNum[m-1];
				}
				wTodoIllegalPhyNum[0] = Temp_IBT;
			}
			else
			{
				break;
			}		
		}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		if(gBankInfo[gCurrentBankNum].wStatus==MAPTAB_VALID)
		{			  
			if(block_in_maptable!=wTodoIllegalPhyNum[0].wPhysicBlkNum)
			{
				gBankInfo[gCurrentBankNum].wStatus=MAPTAB_INVALID;
			}
		}
		
		mm[0].Maptable.Logic2Physic[wIllegalLogicNum] = wTodoIllegalPhyNum[0].wPhysicBlkNum;
				
		if(todo_block>1)
		{			
			for(l=0;l<wExchangeBlockNum;l++)
			{
				if((pBlk[l].wBank==0xffff) || (pBlk[l].wLogicBlk==0xffff) || (pBlk[l].wPhysicBlk==0xffff))
				{
					wExchangeBlkNumInCurBank++;

					pBlk[l].wBank 		= gCurrentBankNum;
					pBlk[l].wLogicBlk 	= wIllegalLogicNum;
					pBlk[l].wPhysicBlk	= wTodoIllegalPhyNum[1].wPhysicBlkNum;
					pBlk[l].wCount		= wTodoIllegalPhyNum[1].wCount;
					pBlk[l].wCurrentPage= FindBlkCurrentPage(&pBlk[l],&safe_block_flg);
					if(safe_block_flg!=0)	// Need copy to good block
					{
						swap_block = GetFreeBlkFromRecycleFifo();
						CopyPhysicalBlock(pBlk[l].wPhysicBlk,swap_block);
						SetBadFlagIntoNand(pBlk[l].wPhysicBlk, NAND_USER_UNSTABLE_TAG);							
						
						pBlk[l].wPhysicBlk	= swap_block;							
						pBlk[l].wCurrentPage= gSTNandDataHalInfo.wBlkPageNum;							
					}

					break;
				}		
			}			
		}
		
		for(l=2;l<todo_block;l++)
		{
			ret = PutFreeBlkIntoRecycleFifo(wTodoIllegalPhyNum[l].wPhysicBlkNum);
			if((ret!=0)&&(gBankInfo[gCurrentBankNum].wStatus==MAPTAB_VALID))
			{
				NF_DATA_ERROR(" PutFreeBlkIntoRecycleFifo failed!!! <function:FixupIllegalBlock>(1) \n");	
#ifdef PART0_WRITE_MONITOR_DEBUG					
				dump_debug_bank_maptable();
				dump_bankinfo();
				dump_curren_bank_maptable();
				dump_exchange_block();
#endif	
				for(i=0;i<ExchangeAMaxNum;i++)
				{
					if(gExchangeABlock[i].wBank == gCurrentBankNum)	// Clear current bank exchange block information for rebuild use
					{
						gExchangeABlock[i].wBank		= NAND_ALL_BIT_FULL_W;
						gExchangeABlock[i].wLogicBlk	= NAND_ALL_BIT_FULL_W;
						gExchangeABlock[i].wPhysicBlk	= NAND_ALL_BIT_FULL_W;
						gExchangeABlock[i].wLogicalPage	= 0;
						gExchangeABlock[i].wCurrentPage	= 0;
						gExchangeABlock[i].wCount		= NAND_ALL_BIT_FULL_B;
						gExchangeABlock[i].wType		= NAND_ALL_BIT_FULL_B;
					}
				}
				
				for(i=0;i<ExchangeBMaxNum;i++)
				{
					if(gExchangeBBlock[i].wBank == gCurrentBankNum)	// Clear current bank exchange block information for rebuild use
					{
						gExchangeBBlock[i].wBank		= NAND_ALL_BIT_FULL_W;
						gExchangeBBlock[i].wLogicBlk	= NAND_ALL_BIT_FULL_W;
						gExchangeBBlock[i].wPhysicBlk	= NAND_ALL_BIT_FULL_W;
						gExchangeBBlock[i].wLogicalPage	= 0;
						gExchangeBBlock[i].wCurrentPage	= 0;
						gExchangeBBlock[i].wCount		= NAND_ALL_BIT_FULL_B;
						gExchangeBBlock[i].wType		= NAND_ALL_BIT_FULL_B;
					}
				}	
				return 0x44;
			}
		}
	}
	
	if((gBankInfo[gCurrentBankNum].wStatus==MAPTAB_VALID)&&(wExchangeBlkNumInCurBank != gBankInfo[gCurrentBankNum].wExchangeBlkNum))
	{
		NF_DATA_ERROR("Current Bank:0x%x Exchange block:0x%x  \n",gCurrentBankNum,gBankInfo[gCurrentBankNum].wExchangeBlkNum);
		NF_DATA_ERROR("Exchange block found:0x%x <function:FixupIllegalBlock>  \n",wExchangeBlkNumInCurBank);
		gBankInfo[gCurrentBankNum].wExchangeBlkNum 	= wExchangeBlkNumInCurBank;
		gBankInfo[gCurrentBankNum].wStatus			= MAPTAB_INVALID;
		
		for(i=0;i<ExchangeAMaxNum;i++)
		{
			if(gExchangeABlock[i].wBank == gCurrentBankNum)	// Clear current bank exchange block information for rebuild use
			{
				gExchangeABlock[i].wBank		= NAND_ALL_BIT_FULL_W;
				gExchangeABlock[i].wLogicBlk	= NAND_ALL_BIT_FULL_W;
				gExchangeABlock[i].wPhysicBlk	= NAND_ALL_BIT_FULL_W;
				gExchangeABlock[i].wLogicalPage	= 0;
				gExchangeABlock[i].wCurrentPage	= 0;
				gExchangeABlock[i].wCount		= NAND_ALL_BIT_FULL_B;
				gExchangeABlock[i].wType		= NAND_ALL_BIT_FULL_B;
			}
		}
		
		for(i=0;i<ExchangeBMaxNum;i++)
		{
			if(gExchangeBBlock[i].wBank == gCurrentBankNum)	// Clear current bank exchange block information for rebuild use
			{
				gExchangeBBlock[i].wBank		= NAND_ALL_BIT_FULL_W;
				gExchangeBBlock[i].wLogicBlk	= NAND_ALL_BIT_FULL_W;
				gExchangeBBlock[i].wPhysicBlk	= NAND_ALL_BIT_FULL_W;
				gExchangeBBlock[i].wLogicalPage	= 0;
				gExchangeBBlock[i].wCurrentPage	= 0;
				gExchangeBBlock[i].wCount		= NAND_ALL_BIT_FULL_B;
				gExchangeBBlock[i].wType		= NAND_ALL_BIT_FULL_B;
			}
		}
		return -1;
	}
	else if(gBankInfo[gCurrentBankNum].wStatus==MAPTAB_INVALID)
	{ 
		gBankInfo[gCurrentBankNum].wExchangeBlkNum 	= wExchangeBlkNumInCurBank;
	}

	return 0;
}

/*************************************************************************
* function:  FindBlkCurrentPage
*
*
*
**************************************************************************/
UINT16 FindBlkCurrentPage(EXCHANGE_A_BLK *Blk,SINT16 *ecc_status)
{
	SINT32 i;
	UINT32 wPhysicBlkPage;
	UINT16 wPhysicCurrentPage;
	UINT16 wCurrPage;
	UINT16 wPageToPageVal;
	UINT16 *pPagetoPage;
	SINT32 ret;

	wPhysicCurrentPage = gSTNandDataHalInfo.wBlkPageNum;// last page

	pPagetoPage = (UINT16 *)Blk->wPage2PageTable;
	for(i = 0;i < (gSTNandDataHalInfo.wBlkPageNum);i++)
	{
		*(pPagetoPage + i) = NAND_ALL_BIT_FULL_W;
	}
		
	wPhysicBlkPage = (UINT32)(Blk->wPhysicBlk)<<gSTNandDataHalInfo.wBlk2Page;
	
	*ecc_status = 0;
	
	for(i=(gSTNandDataHalInfo.wBlkPageNum-1);i>=0;i--)
	{
		ret = ReadDataFromNand(wPhysicBlkPage+i, tt_extend_ptr);
		if(ret!=0)
		{
			*ecc_status = -1;
			if(ret==NF_READ_ECC_ERR)
			{
				NF_DATA_ERROR("Error:Read page 0x%x fail <function:FindBlkCurrentPage>  \n",wPhysicBlkPage+i);
				continue;
			}
			else
			{
				NF_DATA_ERROR("Warning:Read page 0x%x fail <function:FindBlkCurrentPage>  \n",wPhysicBlkPage+i);
			}			
		}
		if(GetCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS, tt_extend_ptr) == NAND_ALL_BIT_FULL_W)
		{
			wPhysicCurrentPage = i;
		}
		else
		{
			#if 1
			if(((Blk->wLogicBlk) != GetCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS, tt_extend_ptr))
				&&((Blk->wLogicBlk) != NAND_ALL_BIT_FULL_W))	
			{
				continue; //break;
			}
			#endif
			wCurrPage = GetCArea(NAND_C_AREA_PAGE_TO_PAGE_OFS, tt_extend_ptr);
				
			//i phiscal page num ,wCurrPage is Logic Page Num 
			wPageToPageVal = *(pPagetoPage + wCurrPage);				
			if(wPageToPageVal != NAND_ALL_BIT_FULL_W)
			  continue;

			*(pPagetoPage + wCurrPage) = i;
		//gExchangeABlock[wBlkID].wLogicBlk = GetCAreaW_Copy(NAND_C_AREA_LOGIC_BLK_NUM_OFS, tt_copy_ptr);			
		}		
	}
	
	return wPhysicCurrentPage;	
}


/*************************************************************************
* function:  NandLowLeverFormat
*
*
*
**************************************************************************/
UINT16 DrvNand_lowlevelformat_UP(UINT16 step)
{
	UINT16  ret;
	UINT16 i;
	UINT16 wBLkBadFlag;
	//UINT16 wNandEraseCount;
	UINT16 wValue = 32;
	
	step += 1;	
	gInitialFlag = 0;
	
	if(gSTNandDataCfgInfo.uiDataStart > (step*wValue))
	{
		return 0;
	}
	if((step*wValue - gSTNandDataCfgInfo.uiDataStart) < wValue)
	{
		//erase all the block except block 0 and app area
		for(i=gSTNandDataCfgInfo.uiDataStart;i<(wValue*step);i++)
		{
			wBLkBadFlag = GetBadFlagFromNand(i);
			if(wBLkBadFlag != NAND_ORG_BAD_TAG)
			{
				if(wBLkBadFlag != NAND_FIRST_SCAN_BAD_TAG)
				{
					ret = DataEraseNandBlk(i);
					if(ret)
					{
						ret = nand_write_status_get();
						if((NFCMD_STATUS)ret<=NF_PROGRAM_ERR13)
						{
							return NF_NAND_PROTECTED;
						}
						SetBadFlagIntoNand(i , NAND_FIRST_SCAN_BAD_TAG);	
					}
				}
			}
		}
	}
	else
	{
		for(i = wValue*(step-1);i < wValue*step;i++)
		{
			wBLkBadFlag = GetBadFlagFromNand(i);
			if(wBLkBadFlag != NAND_ORG_BAD_TAG)
			{
				if(wBLkBadFlag != NAND_FIRST_SCAN_BAD_TAG)
				{
					ret = DataEraseNandBlk(i);
					if(ret)
					{
						ret = nand_write_status_get();
						if((NFCMD_STATUS)ret<=NF_PROGRAM_ERR13)
						{
							return NF_NAND_PROTECTED;
						}
						SetBadFlagIntoNand(i , NAND_FIRST_SCAN_BAD_TAG);
					}	
				}	
			}	
		}
	}
	
	return 0;
}


/*************************************************************************
* function:  NandLowLevelFormat
*
*
*
**************************************************************************/
UINT16 DrvNand_lowlevelformat(void)
{
	SINT32  ret;
	UINT16 i;
	UINT16 wBLkBadFlag;
	UINT16 BadBlockNum;
	UINT16 wPhysicalBlk;
	UINT8  buffer_num;
	
	BadBlockNum = 0;
	wPhysicalBlk = 0xffff;
	
	Nand_OS_Init();
	Nand_OS_LOCK();
	
	//Get nand parameter for global use and init nand flash hardware 
	ret = GetNandParam();
	if(ret != 0)
	{
		Nand_OS_UNLOCK();	
		return NF_UNKNOW_TYPE;	
	}
	
	DataGetWorkbuffer(&tt_basic_ptr, &tt_extend_ptr,gSTNandDataHalInfo.wPageSize);
	buffer_num =SetWorkBuffer(tt_basic_ptr, tt_extend_ptr);
	if(buffer_num == 0)
	{
		NF_DATA_ERROR("----- Allocate DATA Workbuffer failed!!!  ------- \n");
		Nand_OS_UNLOCK();		
		return NF_NO_BUFFERPTR;
	}
	
	DrvNand_WP_Initial();
	
	gInitialFlag = 0;		
	
	//erease all the block except block 0.	
	for(i = gSTNandDataCfgInfo.uiDataStart; i < gSTNandDataHalInfo.wNandBlockNum;i++)
	{
		wBLkBadFlag = GetBadFlagFromNand(i);
		if(wBLkBadFlag != NAND_ORG_BAD_TAG)
		{
			ret = DataEraseNandBlk(i);
			if(ret)
			{
				NF_DATA_ERROR(" Erase failed!! 0x%x  \n",i);
				ret = nand_write_status_get();
				if((NFCMD_STATUS)ret<=NF_PROGRAM_ERR13)
				{
					NF_DATA_ERROR(" Nand protected!! \n");
					Nand_OS_UNLOCK();
					return NF_NAND_PROTECTED;
				}
				SetBadFlagIntoNand(i , NAND_FIRST_SCAN_BAD_TAG);
			}
			
			if(wPhysicalBlk==0xffff)	// 防止format掉电
			{
				wPhysicalBlk = i;			
				PutCArea(NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS,	NAND_FORMAT_START_TAG );
				//NF_DATA_ERROR("Write Format flag \n");
				ret = WriteDataIntoNand((UINT32)((UINT32)wPhysicalBlk<<gSTNandDataHalInfo.wBlk2Page),tt_extend_ptr);
				if(ret==0x44)
				{
					NF_DATA_ERROR(" Formating failed!!!  \n");
				}
			}			
		}	
		else
		{
			NF_DATA_ERROR("Org bad block: %d   \n",i);
			BadBlockNum++;
		}
	}
	
	if(wPhysicalBlk!=0xffff)
	{
		DataEraseNandBlk(wPhysicalBlk);
		//NF_DATA_ERROR("earse block %d  \n",wPhysicalBlk);
	}

	Nand_OS_UNLOCK();

	return NF_OK;
}

SINT32 WriteBackWorkbuffer(EXCHANGE_A_BLK *pBlk)
{
	UINT16  *pPagetoPage;
	SINT32	ret;
	UINT16  wBankNum;

	if((pBlk->wLogicalPage)!=(pBlk->wCurrentPage))
	{
		pBlk->wType = 0x55;
	}

	//NF_DBG_PRINT("Write Back RAM:  Exchange Block -- %d	 Current Page -- %d   Logical Page -- %d  \r \n",pBlk->wPhysicBlk,pBlk->wCurrentPage,pBlk->wLogicalPage);

	//NF_DATA_ERROR("Write workbuffer \n");
	PutCArea(NAND_C_AREA_BAD_FLG_OFS_1,			NAND_ALL_BIT_FULL_B 	);
	PutCArea(NAND_C_AREA_COUNT_OFS,				pBlk->wCount 			);
	PutCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS,		pBlk->wLogicBlk 		);
	PutCArea(NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS,	NF_EXCHANGE_B_TYPE		);
	PutCArea(NAND_C_AREA_BAD_FLG_OFS_6,			NAND_ALL_BIT_FULL_B 	);
	PutCArea(NAND_C_AREA_PAGE_TO_PAGE_OFS,		pBlk->wLogicalPage 		);
	
	ret = WriteDataIntoNand((UINT32)((UINT32)(pBlk->wPhysicBlk)<<gSTNandDataHalInfo.wBlk2Page) + pBlk->wCurrentPage,tt_basic_ptr);
	if(ret==0x44)
	{
		NF_DATA_ERROR("Error:I`m dead 1st,<function:WriteBackWorkbuffer>  \n");
		NF_DATA_ERROR("Error: Target block:0x%x Target page:0x%x ,<function:WriteBackWorkbuffer> \n",pBlk->wPhysicBlk,pBlk->wCurrentPage);
		NF_DATA_ERROR("Block Type:0x%x \n",pBlk->wType);
		//NF_DATA_ERROR("Error:I`m dead 1st, Bank: 0x%x <function:WriteMapTableToNand>\n",wBankNum);
		dump_bankinfo();
		dump_curren_bank_maptable();
		dump_exchange_block();
		return 0x44;
	}
	wBankNum = gCurrentBankNum;	
	//while(ret)//if(ret!=0)
	if(ret)
	{
		NF_DATA_ERROR("Warning:Write page 0x%x fail <function:WriteBackWorkbuffer>  \n",(UINT32)((UINT32)(pBlk->wPhysicBlk)<<gSTNandDataHalInfo.wBlk2Page) + pBlk->wCurrentPage);
		//ChangeBank(pBlk->wBank);	
		
	}
	pPagetoPage =  pBlk->wPage2PageTable;

	*(pPagetoPage +  pBlk->wLogicalPage) = pBlk->wCurrentPage;

	pBlk->wCurrentPage++;
	pBlk->wLogicalPage = NAND_ALL_BIT_FULL_W;
	pBlk = NULL;
	gpWorkBufferBlock=NULL;
	
	return 0;
}

void FlushWorkbuffer_NoSem(void)
{
   SINT32 ret;
	if((gpWorkBufferBlock!=NULL) && (gpWorkBufferBlock->wLogicalPage != NAND_ALL_BIT_FULL_W))
	{
		 ret = WriteBackWorkbuffer(gpWorkBufferBlock);
		 if(ret==0x44)
		{
			NF_DATA_ERROR("Error:I`m dead 1st,<function:FlushWorkbuffer_NoSem> \n");
			//NF_DATA_ERROR("Error:I`m dead 1st, Bank: 0x%x <function:WriteMapTableToNand>\n",wBankNum);
			dump_bankinfo();
			dump_curren_bank_maptable();
			dump_exchange_block();
			return ;
		}
	}
}

void FlushWorkbuffer(void)
{
	Nand_OS_LOCK();
	
	NF_DATA_ERROR("Warning:This should can`t be called,<function:FlushWorkbuffer> \n");
	FlushWorkbuffer_NoSem();
	
	Nand_OS_UNLOCK();
}

UINT16 GetExchangeBlock(UINT16 wType, UINT16 wBank, UINT16 wLogicBlk)
{
	UINT16  wTmpVal;
	UINT16  wBlkIndex;
	UINT16	wExchangeBlockNum;
	static  UINT16 gExchangeACurIdx=0;
	EXCHANGE_A_BLK* pBlk;
	SINT32 ret;
	
	//NF_DATA_ERROR("Get Exchange Block: Type -- %d  Bank -- %d  Logic Blk --%d  \r \n",wType,wBank,wLogicBlk);
	
	if(wType == NF_EXCHANGE_A_TYPE)	// Use exchange A
	{
		pBlk 				= gExchangeABlock;
		wExchangeBlockNum	= ExchangeAMaxNum;
	}
	else					// Use exchange B
	{
		pBlk 				= gExchangeBBlock;
		wExchangeBlockNum	= ExchangeBMaxNum;
	}

	for(wBlkIndex =0; wBlkIndex<wExchangeBlockNum; wBlkIndex++)
	{
		if((wBank == pBlk[wBlkIndex].wBank)&&(pBlk[wBlkIndex].wLogicBlk == wLogicBlk))
		{
			//NF_DBG_PRINT("Hit Exchange Block!  \r \n ");
			break;
		}
	}
	
	if(wBlkIndex>(wExchangeBlockNum-1))   // not match any exchange block,find out an unused one
	{
		for(wBlkIndex =0; wBlkIndex<wExchangeBlockNum; wBlkIndex++)
		{
			if(NAND_ALL_BIT_FULL_W == pBlk[wBlkIndex].wBank)	// This exchange block is not used 
			{
				//NF_DBG_PRINT("Have an empty Block!  \r \n ");
				break;
			}			
		}
	}
	
	if(wBlkIndex>(wExchangeBlockNum-1))   // not match any exchange block, Need flush one
	{	
		if(wType == NF_EXCHANGE_A_TYPE)		// A替换策略与B不同		
		{
			wBlkIndex = gExchangeACurIdx;			
			gExchangeACurIdx++;
			if(gExchangeACurIdx == wExchangeBlockNum)
			{
				gExchangeACurIdx = 0;
			}			
		}
		else
		{	
			wTmpVal = pBlk[0].wCurrentPage;
			for(wBlkIndex = 1; wBlkIndex<wExchangeBlockNum; wBlkIndex++)
			{
				wTmpVal = (wTmpVal > pBlk[wBlkIndex].wCurrentPage) ? wTmpVal:pBlk[wBlkIndex].wCurrentPage;				
			}

			for(wBlkIndex = 0; wBlkIndex<wExchangeBlockNum; wBlkIndex++)
			{
				if(wTmpVal==pBlk[wBlkIndex].wCurrentPage)
					break;
			}
		}
	}	

	if(wBlkIndex>(wExchangeBlockNum-1))
	{
		NF_DATA_ERROR("Warning: wBlkIndex:0x%x <function:GetExchangeBlock> \n",wBlkIndex);
		return 0xffff;
	}

	if((wLogicBlk!=pBlk[wBlkIndex].wLogicBlk)||(wBank!=pBlk[wBlkIndex].wBank))	// Get New exchange block
	{
		//NF_DATA_ERROR("Flush an Exist Block!  \r \n ");
		if(NandFlushA(wBlkIndex,wBank,NAND_BUF_GET_EXCHANGE,wType))
		{
			NF_DATA_ERROR("Error: Nandflush wBlkIndex 0x%x fail <function:GetExchangeBlock> \n",wBlkIndex);
			return 0xffff;
		}
		pBlk[wBlkIndex].wBank 	  		= wBank;
		pBlk[wBlkIndex].wLogicBlk   	= wLogicBlk;
		ret = NandSetOrgLogicBlock(wLogicBlk,wType);
		if(ret==0x44)
		{
			NF_DATA_ERROR("Error:I`m dead 1st,<function:GetExchangeBlock> \n");
			//NF_DATA_ERROR("Error:I`m dead 1st, Bank: 0x%x <function:WriteMapTableToNand>\n",wBankNum);
			dump_bankinfo();
			dump_curren_bank_maptable();
			dump_exchange_block();
			return 0xffff;
		}
	}
	
	return wBlkIndex;
}

/*************************************************************************
* function:		ReadNandAreaA
*	
*	
*	
*************************************************************************/ 
SINT32 ReadNandAreaA(UINT32 wReadLBA, UINT16 wLen, UINT32 DataBufAddr,UINT16 wReadType,UINT8 mode)
{
	UINT16	ret=0;
	UINT16	wTargetBank;
	UINT16	wTargetLogicBlk;
	UINT16 	wTargetPage;
	UINT16  wTargetSector;
	UINT16  wPhysicBlk;
	UINT16  wLenThisBlk;
	UINT16  wDmaLen;
	UINT16  wTargetCurrSector;
	UINT16  wReadExchangeBlkFlag;
	UINT32  dwWorkBufOffset;
	UINT16  wPageToPageVal=0xffff;
	UINT16  *pPagetoPage = (UINT16*)0;
	UINT16	wBlkIndex;
	//UINT16  wRetNand = 0;
	UINT32	wLogicalPage;
	UINT16	wExchangeBlockNum;
	EXCHANGE_A_BLK *pExchangeBlock;
	UINT8 		*p_WorkBuffer;
	SINT32 ret_debug;
	UINT8  wPhysicBlkReadErr;
	UINT8  wExchangeBlkReadErr;
	UINT32 i,tmp_blk;
	UINT32 read_cur_status = 0;
	
	read_cur_status = 0;
	
	wLogicalPage	=  wReadLBA>>gSTNandDataHalInfo.wPage2Sector;
	
	if(wLogicalPage == gLastPage_Write)
	{
		wTargetSector 	= wReadLBA & (gSTNandDataHalInfo.wPageSectorSize-1);
		if((wTargetSector + wLen)>gSTNandDataHalInfo.wPageSectorSize)
		{
			wDmaLen = gSTNandDataHalInfo.wPageSectorSize - wTargetSector;
			wLen 	= wLen - wDmaLen;
		}
		else
		{
			wDmaLen = wLen;
			wLen	= 0;
		}
		dwWorkBufOffset=(UINT32)tt_basic_ptr+(wTargetSector*(512>>BYTE_WORD_SHIFT));
		if(mode==1)
		{
			DMAmmCopyToUser(dwWorkBufOffset, DataBufAddr, wDmaLen*(512>>BYTE_WORD_SHIFT));
		}
		else
		{
			DMAmmCopy(dwWorkBufOffset, DataBufAddr, wDmaLen*(512>>BYTE_WORD_SHIFT));
		}	

		wReadLBA    +=	wDmaLen;
		DataBufAddr += 	wDmaLen*(512>>BYTE_WORD_SHIFT);
		
		//NF_DATA_ERROR(" ------  Hit gLastPage_Write: Type -- %d ------- \r \n",wReadType);
		
		if(wLen==0)
		{
			return NF_OK;
		}
	}
	
	//NF_DATA_ERROR(" ------  111 ------- \r \n");
	if(gWorkBufferNum!=1)
	{
		if(wLogicalPage == gLastPage_Read)
		{
			wTargetSector 	= wReadLBA & (gSTNandDataHalInfo.wPageSectorSize-1);
			if((wTargetSector + wLen)>gSTNandDataHalInfo.wPageSectorSize)
			{
				wDmaLen = gSTNandDataHalInfo.wPageSectorSize - wTargetSector;
				wLen 	= wLen - wDmaLen;
			}
			else
			{
				wDmaLen = wLen;
				wLen	= 0;
			}
			dwWorkBufOffset=(UINT32)tt_extend_ptr+(wTargetSector*(512>>BYTE_WORD_SHIFT));
			if(mode==1)
			{	
				DMAmmCopyToUser(dwWorkBufOffset, DataBufAddr, wDmaLen*(512>>BYTE_WORD_SHIFT));
			}
			else
			{
				DMAmmCopy(dwWorkBufOffset, DataBufAddr, wDmaLen*(512>>BYTE_WORD_SHIFT));
			}			
			
			wReadLBA    +=	wDmaLen;
			DataBufAddr += 	wDmaLen*(512>>BYTE_WORD_SHIFT);
			
			//NF_DATA_ERROR(" ------  Hit gLastPage_Write: Type -- %d ------- \r \n",wReadType);
			
			if(wLen==0)
			{
				return NF_OK;
			}
		}
	}
	//NF_DATA_ERROR(" ------  222 ------- \r \n");	
	wLogicalPage	= wReadLBA >> gSTNandDataHalInfo.wPage2Sector;		// zurong add fix bug.081111
	wTargetLogicBlk = wReadLBA >> gSTNandDataHalInfo.wBlk2Sector;
	wTargetBank 	= wTargetLogicBlk / gSTNandDataCfgInfo.uiBankSize;
	wTargetLogicBlk = wTargetLogicBlk % gSTNandDataCfgInfo.uiBankSize;
	wTargetPage		= (wReadLBA / gSTNandDataHalInfo.wPageSectorSize)%gSTNandDataHalInfo.wBlkPageNum;
	wTargetSector 	= wReadLBA & (gSTNandDataHalInfo.wPageSectorSize-1);
	
	
	if(gWorkBufferNum==1)
	{
		if((gpWorkBufferBlock!=NULL)&&(gpWorkBufferBlock->wLogicalPage != NAND_ALL_BIT_FULL_W))
		{
			//NF_DATA_ERROR("  -------------- Write back Workbuffer ---------------- \r \n");
			ret_debug = WriteBackWorkbuffer(gpWorkBufferBlock);	// Write Back Workbuffer data
			if(ret_debug==0x44)
			{
				NF_DATA_ERROR("Error:I`m dead 1111st,<function:ReadNandAreaA> \n");
				//NF_DATA_ERROR("Error:I`m dead 1st, Bank: 0x%x <function:WriteMapTableToNand>\n",wBankNum);
				dump_bankinfo();
				dump_curren_bank_maptable();
				dump_exchange_block();
				return 0x44;
			}
		}
	}
	//NF_DATA_ERROR(" ------  333 ------- \r \n");	
	if(wReadType == NF_EXCHANGE_A_TYPE)	// Use exchange A
	{
		//NF_DATA_ERROR(" pPagetoPage %d\r\n",pPagetoPage);
		pExchangeBlock 		= (EXCHANGE_A_BLK *)gExchangeABlock;
		wExchangeBlockNum	= ExchangeAMaxNum;
		p_WorkBuffer		= tt_extend_ptr;
		//NF_DATA_ERROR(" pExchangeBlock %x\r\n",pExchangeBlock);
	}
	else								// Use exchange B
	{
		//NF_DATA_ERROR(" pPagetoPage %d\r\n",pPagetoPage);
		pExchangeBlock 		= (EXCHANGE_A_BLK *)gExchangeBBlock;
		wExchangeBlockNum	= ExchangeBMaxNum;
		p_WorkBuffer		= tt_extend_ptr;
		//NF_DATA_ERROR(" pExchangeBlock %x\r\n",pExchangeBlock);
	}

	do
	{
		if(wTargetBank!=gCurrentBankNum)
		{
			ret = ChangeBank(wTargetBank);
			if(ret==0x44)
			{
				NF_DATA_ERROR("Error:I`m dead 1st,<function:ReadNandAreaA>(2) \n");
				NF_DATA_ERROR("Error:I`m dead 1st, Bank: 0x%x <function:ReadNandAreaA>(2)  \n",gCurrentBankNum);
				dump_bankinfo();
				dump_curren_bank_maptable();
				dump_exchange_block();
				return 0x44;
			}
			if(ret!=0)
			{			
				NF_DATA_ERROR("Error: ChangeBank 0x%x fail 1 <function:ReadNandAreaA>  \n",wTargetBank);
				return -1;
			}
		}
		
		//read block is in ExchangeABlock?
		for(wBlkIndex =0; wBlkIndex<wExchangeBlockNum; wBlkIndex++)
		{
			if((wTargetBank == pExchangeBlock[wBlkIndex].wBank)&&(pExchangeBlock[wBlkIndex].wLogicBlk == wTargetLogicBlk))
			{
				break;
			}
		}

		if(wBlkIndex>(wExchangeBlockNum-1))
		{
			wReadExchangeBlkFlag = 0;
		}
		else
		{
			wReadExchangeBlkFlag = 1;
			pPagetoPage =(UINT16*)(pExchangeBlock[wBlkIndex].wPage2PageTable);
		}

		wLenThisBlk=((gSTNandDataHalInfo.wBlkPageNum-wTargetPage)<<gSTNandDataHalInfo.wPage2Sector)-wTargetSector;		

		if(gSTNandDataCfgInfo.uiBankNum == gSTNandDataCfgInfo.uiMM_Num)
		{
			wPhysicBlk = mm[gCurrentBankNum].Maptable.Logic2Physic[wTargetLogicBlk];
		}
		else
		{		
			wPhysicBlk = mm[0].Maptable.Logic2Physic[wTargetLogicBlk];
		}	

		if (wLen>wLenThisBlk)
			wLen = wLen - wLenThisBlk;
		else
		{
			wLenThisBlk=wLen;
			wLen=0;
		}

		wPhysicBlkReadErr = 0;
		wExchangeBlkReadErr = 0;
		
		do
		{
			if((wReadExchangeBlkFlag == 1))
			{	
				wPageToPageVal = *(pPagetoPage + wTargetPage);
			}

			if (wLogicalPage != gLastPage_Write)// can't read data from nand
			{
				if((wReadExchangeBlkFlag == 1)&&(wPageToPageVal != NAND_ALL_BIT_FULL_W))
				{
					ret = ReadDataFromNand(((UINT32)(pExchangeBlock[wBlkIndex].wPhysicBlk)<<gSTNandDataHalInfo.wBlk2Page) + wPageToPageVal, p_WorkBuffer);
				    if(ret!=0)
				    {	
						if(ret==NF_READ_ECC_ERR)
						{
							read_cur_status = 1;
							gLastPage_Error = wLogicalPage;
							NF_DATA_ERROR("Error: Read page 0x%x fail 1 <function:ReadNandAreaA>  \n",((UINT32)(pExchangeBlock[wBlkIndex].wPhysicBlk)<<gSTNandDataHalInfo.wBlk2Page) + wPageToPageVal);
							//return -1;
						}
						else
						{
							NF_DATA_ERROR("Warning: Read page 0x%x fail 1 <function:ReadNandAreaA>  \n",((UINT32)(pExchangeBlock[wBlkIndex].wPhysicBlk)<<gSTNandDataHalInfo.wBlk2Page) + wPageToPageVal);
						}						
						//SaveBadBlkInfo(pExchangeBlock[wBlkIndex].wPhysicBlk);
						wExchangeBlkReadErr = 1;
				    }
				}
				else
				{
					ret = ReadDataFromNand(((UINT32)wPhysicBlk<<gSTNandDataHalInfo.wBlk2Page)+wTargetPage, p_WorkBuffer);					
				    if(ret!=0)
				    {	
						if(ret==NF_READ_ECC_ERR)
						{
							read_cur_status = 1;
							gLastPage_Error = wLogicalPage;
							NF_DATA_ERROR("Error: Read page 0x%x fail 2 <function:ReadNandAreaA>  \n",((UINT32)wPhysicBlk<<gSTNandDataHalInfo.wBlk2Page)+wTargetPage);
							//return -1;
						}
						else
						{
							NF_DATA_ERROR("Warning: Read page 0x%x fail 2 <function:ReadNandAreaA>  \n",((UINT32)wPhysicBlk<<gSTNandDataHalInfo.wBlk2Page)+wTargetPage);
						}					
					    //SaveBadBlkInfo(wPhysicBlk);
						wPhysicBlkReadErr = 1;
				    }					
				}
			}
			else	// Hit RAM
			//copy data from write_workbuffer to read_workbuffer, sync read and write
			{
				//NF_DATA_ERROR(" ------  999 ------- \r \n");
				DMAmmCopy((UINT32)tt_basic_ptr, (UINT32)p_WorkBuffer, (512>>BYTE_WORD_SHIFT)<<gSTNandDataHalInfo.wPage2Sector);
			}
			/* -- TBD--*/		
		/* 	if(ret != 0)				
				return -1; //return 0;	 */
			//NF_DATA_ERROR(" ------  100 ------- \r \n");
			wTargetCurrSector = gSTNandDataHalInfo.wPageSectorSize-wTargetSector;

			if (wTargetCurrSector > wLenThisBlk)
			{
				//NF_DATA_ERROR(" ------  10A ------- \r \n");
				wDmaLen = wLenThisBlk;
				wLenThisBlk=0;
			}
			else
			{
				//NF_DATA_ERROR(" ------  10B ------- \r \n");
				wDmaLen=wTargetCurrSector;
				wLenThisBlk=wLenThisBlk-wTargetCurrSector;
			}
			
			//NF_DATA_ERROR(" ------  10C ------- \r \n");
			dwWorkBufOffset=(UINT32)p_WorkBuffer+(wTargetSector*(512>>BYTE_WORD_SHIFT));

			wTargetSector = (wTargetSector+wDmaLen)& gSTNandDataHalInfo.wPageSectorMask;

			wDmaLen=wDmaLen*(512>>BYTE_WORD_SHIFT);
			
			if(mode==1)
			{	
				DMAmmCopyToUser(dwWorkBufOffset, DataBufAddr, wDmaLen);	
			}
			else
			{
				DMAmmCopy(dwWorkBufOffset, DataBufAddr, wDmaLen);	
			}	

			DataBufAddr=DataBufAddr+wDmaLen;

			wTargetPage++;
			
			if(gWorkBufferNum==1)
			{
				//NF_DATA_ERROR(" ------  10D ------- \r \n");
				gLastPage_Read = gLastPage_Write = wLogicalPage++;
			}
			else
			{
				//NF_DATA_ERROR(" ------  10E ------- \r \n");
				gLastPage_Read = wLogicalPage++;
			}

		}while(wLenThisBlk>0);
		
		if(wExchangeBlkReadErr!=0)
		{		
			if(gWorkBufferNum==1)	// Single workbuffer
			{
				gLastPage_Read 	= 0xffffffff;
				gLastPage_Write	= 0xffffffff;
			}			
			else if(gWorkBufferNum==2)
			{
				gLastPage_Read 	= 0xffffffff;
			}		
		
			do
			{
				ret = 0;
				tmp_blk=GetFreeBlkFromRecycleFifo();
				for(i=0;i<gSTNandDataHalInfo.wBlkPageNum;i++)
				{
					ReadDataFromNand(((UINT32)(pExchangeBlock[wBlkIndex].wPhysicBlk)<<gSTNandDataHalInfo.wBlk2Page) + i, tt_extend_ptr);
					//PutCArea(NAND_C_AREA_COUNT_OFS,	((GetCArea(NAND_C_AREA_COUNT_OFS,tt_extend_ptr) + 1)&0x7f));
					ret = WriteDataIntoNand((tmp_blk<<gSTNandDataHalInfo.wBlk2Page) + i,tt_extend_ptr);
					if(ret!=0)
					{
						SetBadFlagIntoNand(tmp_blk,NAND_USER_BAD_TAG);
						break;
					}
				}
			}while(ret);
			
			SetBadFlagIntoNand(pExchangeBlock[wBlkIndex].wPhysicBlk,NAND_USER_UNSTABLE_TAG);
			pExchangeBlock[wBlkIndex].wPhysicBlk = tmp_blk;
		}
		
		if(wPhysicBlkReadErr!=0)
		{
			if(gWorkBufferNum==1)	// Single workbuffer
			{
				gLastPage_Read 	= 0xffffffff;
				gLastPage_Write	= 0xffffffff;
			}			
			else if(gWorkBufferNum==2)
			{
				gLastPage_Read 	= 0xffffffff;
			}
			
			do
			{
				ret = 0;
				tmp_blk=GetFreeBlkFromRecycleFifo();
				for(i=0;i<gSTNandDataHalInfo.wBlkPageNum;i++)
				{
					ReadDataFromNand((((UINT32)wPhysicBlk)<<gSTNandDataHalInfo.wBlk2Page) + i, tt_extend_ptr);
					//PutCArea(NAND_C_AREA_COUNT_OFS,	((GetCArea(NAND_C_AREA_COUNT_OFS,tt_extend_ptr) + 1)&0x7f));
					ret = WriteDataIntoNand((tmp_blk<<gSTNandDataHalInfo.wBlk2Page) + i,tt_extend_ptr);
					if(ret!=0)
					{
						SetBadFlagIntoNand(tmp_blk,NAND_USER_BAD_TAG);
						break;
					}
				}
			}while(ret);
			
			SetBadFlagIntoNand(wPhysicBlk,NAND_USER_UNSTABLE_TAG);
			wPhysicBlk = tmp_blk;
			UpdateMaptable(tmp_blk,wTargetLogicBlk);			
		}		
		
		if(wLen!=0)	// Next block
		{
			wTargetLogicBlk++;
			wTargetPage = 0;
			wTargetSector =0;

			if(wTargetLogicBlk >= gBankInfo[wTargetBank].wLogicBlkNum)	// Next bank
			{
				wTargetLogicBlk = 0;
				wTargetBank++;
			}
		}

	}while(wLen>0);

	//return NF_OK;	
	return read_cur_status;
}

/*************************************************************************
* function:		WriteNandAreaA
*	
*	
*	
*************************************************************************/ 
SINT32 WriteNandAreaA(UINT32 wWriteLBA, UINT16 wLen, UINT32 DataBufAddr,UINT16 wWriteType,UINT8 mode)
{
	UINT16  ret;
	UINT16  wTargetBank;
	UINT16  wTargetLogicBlk;
	UINT16  wTargetPage;
	UINT16  wTargetSector;
	UINT16  wPhysicBlk;
	UINT16  wLenThisBlk;
	UINT16  wDmaLen;
	UINT16  wCurrentFreeSector;
	UINT32  dwWorkBufOffset;
	UINT16  wPageToPageVal;
	UINT16  wRetNand;
	UINT16  wBlkIndex;
	UINT16	wNeedWriteFlag = 0;
	UINT16  wExchangeAPhysicBlkTemp;
	UINT16  wExchangeASaveBlk;
	UINT16  *pPagetoPage;
	//UINT16 	wTmpVal;
	UINT32	wLogicalPage;
	EXCHANGE_A_BLK *pExchangeBlock;
	UINT8	*p_WorkBuffer;
	SINT32	ret_debug;
	UINT8	wPhysicBlkReadErr,wExchangeBlkReadErr;
	UINT32  tmp_blk,i;	
	
	//NF_DBG_PRINT("Write : Type -- %d  LBA  -- %d   Len -- %d  \r \n",wWriteType,wWriteLBA,wLen);
	
	wLogicalPage	=  wWriteLBA>>gSTNandDataHalInfo.wPage2Sector;	// zurong add for Update gLastPage
    
	if((gpWorkBufferBlock!=NULL)&&(gpWorkBufferBlock->wLogicalPage != NAND_ALL_BIT_FULL_W)) // Workbuffer data valid
	{
		if(wLogicalPage == ((((UINT32)gpWorkBufferBlock->wBank)*(gSTNandDataCfgInfo.uiBankSize) + gpWorkBufferBlock->wLogicBlk)*((UINT32)gSTNandDataHalInfo.wBlkPageNum) + gpWorkBufferBlock->wLogicalPage))
		{
			//if(wWriteType == NF_EXCHANGE_A_TYPE)
			//{
			//	while(1);
			//}
			
			//NF_DBG_PRINT("Write Hit RAM: Bank-- %d  Bank_logic_Blk-- %d  Blk_logic_page-- %d  \r \n",gpWorkBufferBlock->wBank,gpWorkBufferBlock->wLogicBlk,gpWorkBufferBlock->wLogicalPage);
			
			wTargetSector =  wWriteLBA & (gSTNandDataHalInfo.wPageSectorSize-1);
			
			if((wTargetSector + wLen)>=gSTNandDataHalInfo.wPageSectorSize)
			{
				wDmaLen = gSTNandDataHalInfo.wPageSectorSize - wTargetSector;
				wLen 	= wLen - wDmaLen;
			}
			else
			{
				wDmaLen = wLen;
				wLen	= 0;
			}
			
			dwWorkBufOffset=(UINT32)(tt_basic_ptr)+(wTargetSector*(512>>BYTE_WORD_SHIFT));
			if(mode==1)
			{	
				DMAmmCopyFromUser(DataBufAddr, dwWorkBufOffset, wDmaLen*(512>>BYTE_WORD_SHIFT));
			}
			else
			{
				DMAmmCopy(DataBufAddr, dwWorkBufOffset, wDmaLen*(512>>BYTE_WORD_SHIFT));
			}		
			
			wWriteLBA    +=	wDmaLen;
			DataBufAddr += 	wDmaLen*(512>>BYTE_WORD_SHIFT);			
			if((wWriteType==NF_EXCHANGE_B_TYPE) && (1 != gWorkBufferNum))//2010-11-22 modify
			{				
				if(gLastPage_Read==gLastPage_Write)
				{					
					gLastPage_Read = 0xffffffff;
				}
			}					
		}
		
		if(wLen==0)
		{
			return NF_OK;
		}
		
		if((wWriteType==NF_EXCHANGE_B_TYPE) || (gWorkBufferNum==1))
		{	
			//NF_DBG_PRINT("  -------------- Write back Workbuffer ---------------- \r \n");
			ret_debug = WriteBackWorkbuffer(gpWorkBufferBlock);	// Write Back Workbuffer data
			if(ret_debug==0x44)
			{
				NF_DATA_ERROR("Error:I`m dead 11111-1st,<function:WriteNandAreaA>  \n");
				//NF_DATA_ERROR("Error:I`m dead 1st, Bank: 0x%x <function:WriteMapTableToNand>\n",wBankNum);
				dump_bankinfo();
				dump_curren_bank_maptable();
				dump_exchange_block();
				return 0x44;
			}
		}
	}

	wLogicalPage		=  wWriteLBA>>gSTNandDataHalInfo.wPage2Sector;
	wTargetLogicBlk 	= wWriteLBA >> gSTNandDataHalInfo.wBlk2Sector;
	wTargetBank 		= wTargetLogicBlk / gSTNandDataCfgInfo.uiBankSize;
	wTargetLogicBlk		= wTargetLogicBlk % gSTNandDataCfgInfo.uiBankSize;
	wTargetPage 		= (wWriteLBA / gSTNandDataHalInfo.wPageSectorSize) % gSTNandDataHalInfo.wBlkPageNum;
	wTargetSector 		= wWriteLBA & (gSTNandDataHalInfo.wPageSectorSize-1);
	
	//NF_DBG_PRINT("Write: Bank -- %d	Block -- %d	 Page --%d	Sector -- %d  Len -- %d  \r \n",wTargetBank,wTargetLogicBlk,wTargetPage,wTargetSector,wLen);	
	
	if(wWriteType==NF_EXCHANGE_A_TYPE)
	{
		pExchangeBlock = gExchangeABlock;
		p_WorkBuffer  = tt_extend_ptr;
	}
	else
	{
		pExchangeBlock = gExchangeBBlock;
		p_WorkBuffer  = tt_basic_ptr;
	}
	
	do
	{
		wBlkIndex = GetExchangeBlock(wWriteType,wTargetBank,wTargetLogicBlk);
		if(wBlkIndex==0xffff)
		{
			NF_DATA_ERROR("Error: Get exchange block fail <function:WriteNandAreaA>  \n");
			return -1;
		}
		
		if(wTargetBank != gCurrentBankNum)
		{			
			ret = ChangeBank(wTargetBank);
			if(ret==0x44)
			{
				NF_DATA_ERROR("Error:I`m dead 1st,<function:WriteNandAreaA>(2)\n");
				NF_DATA_ERROR("Error:I`m dead 1st, Bank: 0x%x <function:WriteNandAreaA>(2)  \n",gCurrentBankNum);
				dump_bankinfo();
				return 0x44;
			}
			if(ret!=0)
			{
				NF_DATA_ERROR("Error: ChangeBank 0x%x fail <function:WriteNandAreaA>  \n",wTargetBank);
				return -1;
			}
		}
		
		wLenThisBlk = ((gSTNandDataHalInfo.wBlkPageNum - wTargetPage ) << gSTNandDataHalInfo.wPage2Sector) - wTargetSector;	
							
		if (wLen > wLenThisBlk)		//wLen is sector , wFreeLen is free sector in a block
			wLen = wLen - wLenThisBlk;
		else
		{				
			wLenThisBlk = wLen;
			wLen = 0;
		}

		do
		{	
			wPhysicBlkReadErr = 0;
			wExchangeBlkReadErr = 0;
			if((pExchangeBlock[wBlkIndex].wCurrentPage)>=gSTNandDataHalInfo.wBlkPageNum)	// 顺序位置需要考虑是否调整
			{
				//NF_DATA_ERROR(" $$$$$$$$$  Exchange Block Page Full,fulsh it!  $$$$$$$$$$ \r \n");
				ret = NandFlushA(wBlkIndex,wTargetBank,NAND_BUF_GET_EXCHANGE,wWriteType);
				if(ret!=0)
				{
					NF_DATA_ERROR("Error: Nand flush 0x%x fail <function:WriteNandAreaA>  \n",wBlkIndex);
					return -1;
				}
				pExchangeBlock[wBlkIndex].wBank 	  		= wTargetBank;
				pExchangeBlock[wBlkIndex].wLogicBlk   		= wTargetLogicBlk;
				
				ret_debug = NandSetOrgLogicBlock(wTargetLogicBlk,wWriteType);
				if(ret_debug==0x44)
				{
					NF_DATA_ERROR("Error:I`m dead 1st,<function:WriteNandAreaA>(3)  \n");
					//NF_DATA_ERROR("Error:I`m dead 1st, Bank: 0x%x <function:WriteMapTableToNand>\n",wBankNum);
					dump_bankinfo();
					dump_curren_bank_maptable();
					return 0x44;
				}
			}
			
			if(gSTNandDataCfgInfo.uiBankNum == gSTNandDataCfgInfo.uiMM_Num)
			{	
				wPhysicBlk = mm[gCurrentBankNum].Maptable.Logic2Physic[wTargetLogicBlk];// Original block
			}
			else
			{
				wPhysicBlk = mm[0].Maptable.Logic2Physic[wTargetLogicBlk];				// Original block
			}
			
			if((pExchangeBlock[wBlkIndex].wCount & 0xff)== 0xff)						// Read count first
			{
				ReadDataFromNand(((UINT32)wPhysicBlk<<gSTNandDataHalInfo.wBlk2Page),			p_WorkBuffer);
				pExchangeBlock[wBlkIndex].wCount = (GetCArea(NAND_C_AREA_COUNT_OFS,p_WorkBuffer) + 1)&0x7f;
			}

			pPagetoPage = (UINT16*)(pExchangeBlock[wBlkIndex].wPage2PageTable);			// update page to page

			if(wWriteType==NF_EXCHANGE_B_TYPE)
			{
				if((pExchangeBlock[wBlkIndex].wType!=0x55)&&(wTargetPage>pExchangeBlock[wBlkIndex].wCurrentPage))
				{
					UINT16 i;
#ifdef Enable_normal_debug_info					
					NF_DATA_ERROR("Debuging: Copy to physical block:0x%x   \n",((UINT32)(pExchangeBlock[wBlkIndex].wPhysicBlk)));
					NF_DATA_ERROR("Debuging: Copying page, start 0x%x , end 0x%x <function:WriteNandAreaA>  \n",pExchangeBlock[wBlkIndex].wCurrentPage,wTargetPage-1);
#endif		
					
					for(i= pExchangeBlock[wBlkIndex].wCurrentPage;i<wTargetPage;i++)	// 需要Copy 数据了，会先破坏Workbuffer的数据
					{
						ret_debug = ReadDataFromNand((((UINT32)wPhysicBlk)<<gSTNandDataHalInfo.wBlk2Page)+i,p_WorkBuffer);
						if(ret_debug!=0)
						{
							wPhysicBlkReadErr = 1;							
						}
						PutCArea(NAND_C_AREA_BAD_FLG_OFS_1,			NAND_ALL_BIT_FULL_B				);
						PutCArea(NAND_C_AREA_COUNT_OFS,				pExchangeBlock[wBlkIndex].wCount	);
						PutCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS,		wTargetLogicBlk					);
						PutCArea(NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS,	wWriteType							);
						PutCArea(NAND_C_AREA_BAD_FLG_OFS_6,			NAND_ALL_BIT_FULL_B				);
						PutCArea(NAND_C_AREA_PAGE_TO_PAGE_OFS,			i								);
						//NF_DATA_ERROR("Copy Jump data..... \n");
						ret_debug = WriteDataIntoNand(((UINT32)(pExchangeBlock[wBlkIndex].wPhysicBlk)<<gSTNandDataHalInfo.wBlk2Page) + i,p_WorkBuffer);
						if(ret_debug==0x44)
						{
							NF_DATA_ERROR("Error:I`m dead 1st,<function:WriteNandAreaA>(4)  \n");
							NF_DATA_ERROR("Error:I`m dead 1st,<function:WriteNandAreaA>(4)  \n");
							NF_DATA_ERROR("Type: 0x%x,Index:0x%x \n",pExchangeBlock[wBlkIndex].wType,wBlkIndex);
							//NF_DATA_ERROR("Error:I`m dead 1st, Bank: 0x%x <function:WriteMapTableToNand>\n",wBankNum);
							dump_bankinfo();
							dump_curren_bank_maptable();
							dump_exchange_block();
							return 0x44;
						}
						*(pPagetoPage + i) = i;
						pExchangeBlock[wBlkIndex].wCurrentPage++;
					}
				}
				else if(wTargetPage<pExchangeBlock[wBlkIndex].wCurrentPage)
				{
					pExchangeBlock[wBlkIndex].wType = 0x55;		// Change 
				}
			}

			if(wPhysicBlkReadErr!=0)
			{
				if(gWorkBufferNum==1)	// Single workbuffer
				{
					gLastPage_Read 	= 0xffffffff;
					gLastPage_Write	= 0xffffffff;
				}			
				else if(gWorkBufferNum==2)
				{
					gLastPage_Read 	= 0xffffffff;
				}
				
				do
				{
					tmp_blk=GetFreeBlkFromRecycleFifo();
					for(i=0;i<gSTNandDataHalInfo.wBlkPageNum;i++)
					{
						ReadDataFromNand((((UINT32)wPhysicBlk)<<gSTNandDataHalInfo.wBlk2Page) + i, tt_extend_ptr);
						ret = WriteDataIntoNand((tmp_blk<<gSTNandDataHalInfo.wBlk2Page) + i,tt_extend_ptr);
						if(ret!=0)
						{
							SetBadFlagIntoNand(tmp_blk,NAND_USER_BAD_TAG);
							break;
						}
					}
				}while(ret);
				
				SetBadFlagIntoNand(wPhysicBlk,NAND_USER_UNSTABLE_TAG);
				wPhysicBlk = tmp_blk;
				UpdateMaptable(tmp_blk,wTargetLogicBlk);			
			}			
			
			wCurrentFreeSector = gSTNandDataHalInfo.wPageSectorSize - wTargetSector;	// get this page free len
			
			if (wCurrentFreeSector > wLenThisBlk)		//check the page free len > request page len
			{
				wDmaLen = wLenThisBlk;
				wLenThisBlk = 0;
				wNeedWriteFlag = 0x00;
			}
			else
			{
				wDmaLen = wCurrentFreeSector;
				wLenThisBlk = wLenThisBlk-wCurrentFreeSector;
				if(wWriteType == NF_EXCHANGE_B_TYPE)
				{
					wNeedWriteFlag = 0x01;
				}
				else
				{
					wNeedWriteFlag = 0x00;
				}
			}

			wPhysicBlkReadErr = 0;
			wExchangeBlkReadErr = 0;
			
			do
			{			
				if((wTargetSector != 0)||(wLenThisBlk < gSTNandDataHalInfo.wPageSectorSize))
				{
					wPageToPageVal = *(pPagetoPage + wTargetPage);
					if(wPageToPageVal == NAND_ALL_BIT_FULL_W)
					{
						wRetNand = ReadDataFromNand(((UINT32)wPhysicBlk<<gSTNandDataHalInfo.wBlk2Page) + wTargetPage, p_WorkBuffer);
						if(wRetNand != NF_OK)
						{
							if(wRetNand==NF_READ_ECC_ERR)
							{
								NF_DATA_ERROR("Error:Read page fail 0x%x <function:WriteNandAreaA>  \n",((UINT32)wPhysicBlk<<gSTNandDataHalInfo.wBlk2Page) + wTargetPage);
								//return -1;
							}
							else
							{
								NF_DATA_ERROR("Warning:save bad block 0x%x <function:WriteNandAreaA>  \n",wPhysicBlk);
								//SaveBadBlkInfo(wPhysicBlk);
							}
							wPhysicBlkReadErr = 1;			
						}
					}
					else
					{	
						wRetNand = ReadDataFromNand(((UINT32)(pExchangeBlock[wBlkIndex].wPhysicBlk)<<gSTNandDataHalInfo.wBlk2Page) + wPageToPageVal, p_WorkBuffer);
						if(wRetNand != NF_OK)
						{
							if(wRetNand==NF_READ_ECC_ERR)
							{
								NF_DATA_ERROR("Error1:Read page fail 0x%x <function:WriteNandAreaA>  \n",((UINT32)(pExchangeBlock[wBlkIndex].wPhysicBlk)<<gSTNandDataHalInfo.wBlk2Page) + wPageToPageVal);
								//return -1;
							}
							else
							{
								NF_DATA_ERROR("Warning1:save bad block 0x%x <function:WriteNandAreaA>  \n",pExchangeBlock[wBlkIndex].wPhysicBlk);
								//SaveBadBlkInfo(pExchangeBlock[wBlkIndex].wPhysicBlk);
							}						
							wExchangeBlkReadErr = 1;
						}	
					}
				}
				
				if(wExchangeBlkReadErr!=0)
				{		
					if(gWorkBufferNum==1)	// Single workbuffer
					{
						gLastPage_Read 	= 0xffffffff;
						gLastPage_Write	= 0xffffffff;
					}			
					else if(gWorkBufferNum==2)
					{
						gLastPage_Read 	= 0xffffffff;
					}		
				
					do
					{
						tmp_blk=GetFreeBlkFromRecycleFifo();
						for(i=0;i<gSTNandDataHalInfo.wBlkPageNum;i++)
						{
							ReadDataFromNand(((UINT32)(pExchangeBlock[wBlkIndex].wPhysicBlk)<<gSTNandDataHalInfo.wBlk2Page) + i, tt_extend_ptr);
							ret = WriteDataIntoNand((tmp_blk<<gSTNandDataHalInfo.wBlk2Page) + i,tt_extend_ptr);
							if(ret!=0)
							{
								SetBadFlagIntoNand(tmp_blk,NAND_USER_BAD_TAG);
								break;
							}
						}
					}while(ret);
					
					SetBadFlagIntoNand(pExchangeBlock[wBlkIndex].wPhysicBlk,NAND_USER_UNSTABLE_TAG);
					pExchangeBlock[wBlkIndex].wPhysicBlk = tmp_blk;
				}
				
				if(wPhysicBlkReadErr!=0)
				{
					if(gWorkBufferNum==1)	// Single workbuffer
					{
						gLastPage_Read 	= 0xffffffff;
						gLastPage_Write	= 0xffffffff;
					}			
					else if(gWorkBufferNum==2)
					{
						gLastPage_Read 	= 0xffffffff;
					}
					
					do
					{
						tmp_blk=GetFreeBlkFromRecycleFifo();
						for(i=0;i<gSTNandDataHalInfo.wBlkPageNum;i++)
						{
							ReadDataFromNand((((UINT32)wPhysicBlk)<<gSTNandDataHalInfo.wBlk2Page) + i, tt_extend_ptr);
							ret = WriteDataIntoNand((tmp_blk<<gSTNandDataHalInfo.wBlk2Page) + i,tt_extend_ptr);
							if(ret!=0)
							{
								SetBadFlagIntoNand(tmp_blk,NAND_USER_BAD_TAG);
								break;
							}
						}
					}while(ret);
					
					SetBadFlagIntoNand(wPhysicBlk,NAND_USER_UNSTABLE_TAG);
					wPhysicBlk = tmp_blk;
					UpdateMaptable(tmp_blk,wTargetLogicBlk);			
				}		
				
			}while((wPhysicBlkReadErr!=0)||(wExchangeBlkReadErr!=0));

			dwWorkBufOffset=(UINT32)p_WorkBuffer+(wTargetSector*(512>>BYTE_WORD_SHIFT));  //
			wTargetSector = (wTargetSector+wDmaLen)& gSTNandDataHalInfo.wPageSectorMask;		

		#ifdef _DEBUG_ERR
			if (wTargetSector!=0)
			{
				//RWFail_Debug();
				//Sys_Exception(HaveSameERR);
				NF_DATA_ERROR("Debug: wTargetSector:0x%x!=0  <function:WriteNandAreaA>  \n",wTargetSector);
			}
		#endif
			wDmaLen = wDmaLen*(512>>BYTE_WORD_SHIFT);	
			
			if(mode==1)	
			{	
				DMAmmCopyFromUser(DataBufAddr, dwWorkBufOffset, wDmaLen);
			}
			else
			{
				DMAmmCopy(DataBufAddr, dwWorkBufOffset, wDmaLen);
			}		

			PutCArea(NAND_C_AREA_BAD_FLG_OFS_1,			NAND_ALL_BIT_FULL_B 				);
			PutCArea(NAND_C_AREA_COUNT_OFS,				pExchangeBlock[wBlkIndex].wCount 	);
			PutCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS,		wTargetLogicBlk 					);
			PutCArea(NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS,	wWriteType							);
			PutCArea(NAND_C_AREA_BAD_FLG_OFS_6,			NAND_ALL_BIT_FULL_B 				);
			PutCArea(NAND_C_AREA_PAGE_TO_PAGE_OFS,			wTargetPage 					);
			
			if((wNeedWriteFlag==0x01) || (wWriteType==NF_EXCHANGE_A_TYPE))	// Exchange A or Exchange B full page write
			{	
				//NF_DATA_ERROR("Normal write...... \n");
				ret = WriteDataIntoNand(((UINT32)(pExchangeBlock[wBlkIndex].wPhysicBlk)<<gSTNandDataHalInfo.wBlk2Page) + pExchangeBlock[wBlkIndex].wCurrentPage, p_WorkBuffer);			
				if(ret==0x44)
				{
					NF_DATA_ERROR("Error:I`m dead 222st,<function:WriteNandAreaA>(5)  \n");
					NF_DATA_ERROR("Type: 0x%x,Index:0x%x \n",pExchangeBlock[wBlkIndex].wType,wBlkIndex);
					dump_bankinfo();
					dump_curren_bank_maptable();
					dump_exchange_block();
					return 0x44;
				}
				if(ret)
				{
					NF_DATA_ERROR("Warning: Write page fail 0x%x <function:WriteNandAreaA>  \n",((UINT32)(pExchangeBlock[wBlkIndex].wPhysicBlk)<<gSTNandDataHalInfo.wBlk2Page) + pExchangeBlock[wBlkIndex].wCurrentPage);
					
					wRetNand = nand_write_status_get();
					if((NFCMD_STATUS)wRetNand<=NF_PROGRAM_ERR13)
					{
						NF_DATA_ERROR("Error: Write page fail 0x%x <function:WriteNandAreaA>  \n",((UINT32)(pExchangeBlock[wBlkIndex].wPhysicBlk)<<gSTNandDataHalInfo.wBlk2Page) + pExchangeBlock[wBlkIndex].wCurrentPage);
						return NF_NAND_PROTECTED;
					}
					if(wTargetBank != gCurrentBankNum)
					{	
						NF_DATA_ERROR("Warning: change bank 0x%x to bank 0x%x <function:WriteNandAreaA>  \n",gCurrentBankNum,wTargetBank);
						wRetNand = (UINT16)ChangeBank(wTargetBank);
						if(wRetNand==0x44)
						{
							NF_DATA_ERROR("Error:I`m dead 1st,<function:WriteNandAreaA>(6)  \n");
							NF_DATA_ERROR("Error:I`m dead 1st, Bank: 0x%x <function:WriteNandAreaA>  \n",gCurrentBankNum);
							dump_bankinfo();
							dump_curren_bank_maptable();
							dump_exchange_block();
							return 0x44;
						}
						if(wRetNand!=0)
						{
							NF_DATA_ERROR("Error: change bank 0x%x to bank 0x%x <function:WriteNandAreaA>  \n",gCurrentBankNum,wTargetBank);
							return -1;
						}
					}
						
					wExchangeAPhysicBlkTemp 			= pExchangeBlock[wBlkIndex].wPhysicBlk;
					wExchangeASaveBlk					= GetFreeBlkFromRecycleFifo();	// zurong add
					pExchangeBlock[wBlkIndex].wPhysicBlk= GetFreeBlkFromRecycleFifo();
					if(NF_NO_SWAP_BLOCK == pExchangeBlock[wBlkIndex].wPhysicBlk)
					{						
						NF_DATA_ERROR("Error: No swap block <function:WriteNandAreaA>  \n");
						return -1;
					}
					//NF_DATA_ERROR("Normal write failed...... \n");	//如果这个时候掉电就很危险，需要处理好
					wRetNand = WriteDataIntoNand((UINT32)wExchangeASaveBlk<<gSTNandDataHalInfo.wBlk2Page, p_WorkBuffer);	// zurong add
					if(wRetNand==0x44)
					{
						NF_DATA_ERROR("Error:I`m dead 333st,<function:WriteNandAreaA>(7)  \n");
						//NF_DATA_ERROR("Error:I`m dead 1st, Bank: 0x%x <function:WriteMapTableToNand>\n",wBankNum);
						dump_bankinfo();
						dump_curren_bank_maptable();
						dump_exchange_block();
						return 0x44;
					}
					//从0 page开始copy. 
					
					wRetNand = CopyMultiPhysicalPage(wExchangeAPhysicBlkTemp, pExchangeBlock[wBlkIndex].wPhysicBlk, 0, pExchangeBlock[wBlkIndex].wCurrentPage-1, pExchangeBlock[wBlkIndex].wLogicBlk,wWriteType);
					if(wRetNand==0x44)
					{
						NF_DATA_ERROR("Error:I`m dead 4444st,<function:WriteNandAreaA>(8)  \n");
						NF_DATA_ERROR("Type: 0x%x,Index:0x%x \n",pExchangeBlock[wBlkIndex].wType,wBlkIndex);
						//NF_DATA_ERROR("Error:I`m dead 1st, Bank: 0x%x <function:WriteMapTableToNand>\n",wBankNum);
						dump_bankinfo();
						dump_curren_bank_maptable();
						dump_exchange_block();
						return 0x44;
					}
					if(wRetNand==NF_NAND_PROTECTED)
					{
						NF_DATA_ERROR("Error: Copy data fail! <function:WriteNandAreaA>  \n");
						return -1;
					}
					
					////www?? use normal or copy buffer
					ReadDataFromNand((UINT32)wExchangeASaveBlk<<gSTNandDataHalInfo.wBlk2Page, p_WorkBuffer);	// zurong add
					ret = WriteDataIntoNand(((UINT32)(pExchangeBlock[wBlkIndex].wPhysicBlk) <<gSTNandDataHalInfo.wBlk2Page)+pExchangeBlock[wBlkIndex].wCurrentPage, p_WorkBuffer);		
					if(ret==0x44)
					{
						NF_DATA_ERROR("Error:I`m dead 4444st,<function:WriteNandAreaA>(9)  \n");
						NF_DATA_ERROR("Type: 0x%x,Index:0x%x \n",pExchangeBlock[wBlkIndex].wType,wBlkIndex);
						//NF_DATA_ERROR("Error:I`m dead 1st, Bank: 0x%x <function:WriteMapTableToNand>\n",wBankNum);
						dump_bankinfo();
						dump_curren_bank_maptable();
						dump_exchange_block();
						return 0x44;
					}
					
					ret = SetBadFlagIntoNand(wExchangeAPhysicBlkTemp,NAND_USER_BAD_TAG);
					if(ret==0x44)
					{
						NF_DATA_ERROR("Set bad block failed!!! <function:WriteNandAreaA>(1)  \n");
						NF_DATA_ERROR("Type: 0x%x,Index:0x%x \n",pExchangeBlock[wBlkIndex].wType,wBlkIndex);
						//NF_DATA_ERROR("Error:I`m dead 1st, Bank: 0x%x <function:WriteMapTableToNand>\n",wBankNum);
						dump_bankinfo();
						dump_curren_bank_maptable();
						dump_exchange_block();
						return 0x44;
					}
					gBankInfo[gCurrentBankNum].wUserBadBlkNum++;
					ret = PutFreeBlkIntoRecycleFifo(wPhysicBlk);
					if(ret!=0)
					{
						NF_DATA_ERROR(" PutFreeBlkIntoRecycleFifo failed!!! <function:WriteNandAreaA>(1)  \n");						
						dump_bankinfo();
						dump_curren_bank_maptable();
						dump_exchange_block();
						return 0x44;
					}
					
				}

				*(pPagetoPage + wTargetPage) = pExchangeBlock[wBlkIndex].wCurrentPage;		
				
				pExchangeBlock[wBlkIndex].wCurrentPage++;

				if((wWriteType==NF_EXCHANGE_A_TYPE)&&(gWorkBufferNum==1))// 这个时候WorkBuffer的数据还是保留着的，所以，不能被清零
				{
					NF_DATA_ERROR("Debuging: Clear gpWorkBufferBlock! <function:WriteNandAreaA>  \n");
					gpWorkBufferBlock = NULL; // Clear workbuffer record
				}
			}
			else if(wWriteType==NF_EXCHANGE_B_TYPE)	// When record workbuffer 
			{			
				gpWorkBufferBlock = &pExchangeBlock[wBlkIndex];
				gpWorkBufferBlock->wLogicalPage = wTargetPage;
			}

			//go next buffer	
			DataBufAddr = DataBufAddr + wDmaLen;			

			//multi buffer
			if(gWorkBufferNum==1)
			{				
				gLastPage_Write = gLastPage_Read = wLogicalPage++;
			}	
			else if(wWriteType==NF_EXCHANGE_A_TYPE)
			{				
				gLastPage_Read = wLogicalPage++;
			}
			else if(wWriteType==NF_EXCHANGE_B_TYPE)
			{
				gLastPage_Write = wLogicalPage++;
				if(gLastPage_Read==gLastPage_Write)
				{					
					gLastPage_Read = 0xffffffff;
				}
			}

			wTargetPage++;

		}while( wLenThisBlk > 0);

		if(wLen != 0)	//Is need change block ?
		{
			wTargetLogicBlk++;	//go next block

			wTargetPage = 0;	//page number block =0

			wTargetSector = 0;	//sector num =0

			if(wTargetLogicBlk >= gBankInfo[wTargetBank].wLogicBlkNum) //Is exceed current bank ? 
			{
				wTargetLogicBlk = 0;
				wTargetBank++;
			}
		}			

	}while(wLen > 0);
	
	//NF_DBG_PRINT(" gLastPage_Read = %d  gLastPage_Write = %d  \r \n",gLastPage_Read,gLastPage_Write);

	return NF_OK;
}


/*************************************************************************
* function:		ReadNand
*	
*	
*	
*************************************************************************/ 
UINT16 DrvNand_read_sector(UINT32 wReadLBA, UINT16 wLen, UINT32 DataBufAddr,UINT8 mode)
{
	UINT16  ret = 0xffff;

	//NF_DATA_ERROR ("\r\n====DrvNand_read_sector enter !!=====\r\n");
	Nand_OS_LOCK();
	ret = NandXsfer(wReadLBA,wLen,DataBufAddr,NF_READ,mode);
	Nand_OS_UNLOCK();

	return ret;
}

UINT16 DrvNand_read_sector_NoSem(UINT32 wReadLBA, UINT16 wLen, UINT32 DataBufAddr,UINT8 mode)
{
	UINT16  ret = 0xffff;
	
	ret = NandXsfer(wReadLBA,wLen,DataBufAddr,NF_READ,mode);

	return ret;
}


/*************************************************************************
* function:		WriteNand
*	
*	
*	
*************************************************************************/ 
UINT16 DrvNand_write_sector(UINT32 wWriteLBA, UINT16 wLen, UINT32 DataBufAddr,UINT8 mode)
{
	UINT16  ret = 0xffff;	

	Nand_OS_LOCK();
	
	ret = NandXsfer(wWriteLBA,wLen,DataBufAddr,NF_WRITE,mode);

	Nand_OS_UNLOCK();
	
	return ret;
}

UINT16 DrvNand_write_sector_NoSem(UINT32 wWriteLBA, UINT16 wLen, UINT32 DataBufAddr,UINT8 mode)
{
	UINT16  ret = 0xffff;	
	
	ret = NandXsfer(wWriteLBA,wLen,DataBufAddr,NF_WRITE,mode);
	
	return ret;
}

/*************************************************************************
* function:		WriteNand
*	
*	
*	
*************************************************************************/
UINT16 NandXsfer(UINT32 wLBA, UINT16 wLen, UINT32 wAddr,UINT16 XsferMode,UINT8 space_mode)
{
	UINT16   ret = 0xffff;
	UINT32   wFreeLen;
	UINT16   wAreaFlg=0xff;
	UINT32	 i;

#ifndef	 NAND_16BIT
	if (((4 - (((UINT32) wAddr) & 0x00000003)) & 0x00000003) != 0)
	{
		NF_DATA_ERROR("##-->Serious Error: Traget buffer not 4 byte alignment!!   \n");
		return NF_DMA_ALIGN_ADDR_NEED;
	}
#endif

	while(wLen)
	{	
		wAreaFlg=0xff;
		
		for(i=0;i<(gSTNandConfigInfo.uiPartitionNum-1);i++)		
		{
			if(gSTNandConfigInfo.Partition[i].size==0)
			{
				continue;
			}
			if(wLBA<part_info[i].DATAStart)	// FAT AREA
			{
				wFreeLen = part_info[i].DATAStart - wLBA;
				wAreaFlg = 0x00;
				break;
			}
			else if(wLBA<part_info[i+1].FATStart)
			{
				wFreeLen = part_info[i+1].FATStart - wLBA;
				wAreaFlg = 0x01;
				break;
			}			
		}		
	
		if(wAreaFlg==0xff)	// 没有找到区间
		{
			i = gSTNandConfigInfo.uiPartitionNum-1;
			if(wLBA<part_info[i].DATAStart)	// FAT AREA
			{
				wFreeLen = part_info[i].DATAStart - wLBA;
				wAreaFlg = 0x00;
			}
			else if(wLBA < gNandSize)	// DATA AREA
			{
				wFreeLen = gNandSize - wLBA;	
				wAreaFlg = 0x01;
			}
			else 
			{
				//RWFail_Debug();
				NF_DATA_ERROR("data write beyond size !!!<function:NandXsfer>   \n");
				return NF_BEYOND_NAND_SIZE;
			}
		}
		
		if(wFreeLen > wLen)
		{
			wFreeLen = wLen;
		}
		
		if(wAreaFlg)	// DATA Area   //if(1)		
		{
			if(NF_WRITE == XsferMode)
			{	
				//NF_DATA_ERROR ("====Nand data area Write: %d  len: %d \n",wLBA,wFreeLen);
				ret = WriteNandAreaA(wLBA, wFreeLen, wAddr,NF_EXCHANGE_B_TYPE,space_mode);	
			}
			else
			{
				//NF_DATA_ERROR ("====Nand data area Read: %d  len: %d \n",wLBA,wFreeLen);
				ret = ReadNandAreaA(wLBA, wFreeLen, wAddr,NF_EXCHANGE_B_TYPE,space_mode);
			}
		}
		else			// FAT	Area
		{
			if(NF_WRITE == XsferMode)
			{	
				ret = WriteNandAreaA(wLBA, wFreeLen, wAddr,NF_EXCHANGE_A_TYPE,space_mode);	
			}
			else
			{
				ret = ReadNandAreaA(wLBA, wFreeLen, wAddr,NF_EXCHANGE_A_TYPE,space_mode);
			}
		}
		
		if(ret)		// if error, return
			return ret;
		
		wLen  = wLen  - wFreeLen;
		wLBA  = wLBA  + wFreeLen;		
		wAddr = wAddr + ((UINT32)wFreeLen *(512>>BYTE_WORD_SHIFT));
	}	
	return ret;
}


void SaveBadBlkInfo(UINT16 wBlkNum)
{
	UINT16 i;
	UINT16 wTotalBlkNum;
	UINT16 wPhysicBlk;
	UINT16 wNeedSave;

	if((bad_blk_info.wArrayIndex)<MAX_BAD_BLK)
	{
		wTotalBlkNum = bad_blk_info.wArrayIndex;
	}
	else
	{
		wTotalBlkNum = MAX_BAD_BLK;
	}
	
	wNeedSave = 0x01;	// not saved
	
	// Scan block if have saved
	for(i=0;i<wTotalBlkNum;i++)
	{
		wPhysicBlk = bad_blk_info.bad_blk[i];
		if(wBlkNum == wPhysicBlk)
		{
			wNeedSave = 0x00;  // have saved
			break;
		}		
	}
	
	if(0x00!=wNeedSave)	//需要保存
	{	
		if((bad_blk_info.wArrayIndex)<MAX_BAD_BLK)
		{
			NF_DATA_ERROR("Debuging: 0x%x <function:SaveBadBlkInfo>   \n",wBlkNum);
			bad_blk_info.bad_blk[bad_blk_info.wArrayIndex] = wBlkNum;
			bad_blk_info.wArrayIndex ++;
		}		
	}	
}

/*************************************************************************
* function:		ChangeReadErrBlk
*	
*	
*	
*************************************************************************/ 
UINT16 ChangeReadErrBlk(void)	
{
#if 0
	UINT32 	wTmpPhysicBlk;
	UINT16  wphyscialblknum;
	UINT16	wBadBlkCount;
	UINT16  wPhyscialBadBlk[MAX_BAD_BLK];
	UINT16	wLogicBadBlk[MAX_BAD_BLK];
	UINT16	wBadBlkType[MAX_BAD_BLK];
	UINT32	wSRCPageNum;
	UINT32	wTARPageNum;
	UINT16  wBankNum;
	UINT16  wRetNand=0;
	UINT16  i,j,k;
	UINT16  BankRecyclePosition_DecCount;

	if(bad_blk_info.wArrayIndex==0)		// No bad block
	{
		return 0;
	}

	for(wBankNum = 0; wBankNum< gTotalBankNum; wBankNum++)
	{
		if(wBankNum != gCurrentBankNum)
		{
			NF_DATA_ERROR("Debuging1: Chang bank 0x%x to bank 0x%x <function:ChangeReadErrBlk> \n",gCurrentBankNum,wBankNum);
			wRetNand = ChangeBank(wBankNum);
			if(wRetNand!=0)
			{
				NF_DATA_ERROR("Error1: Chang bank 0x%x to bank 0x%x <function:ChangeReadErrBlk> \n",gCurrentBankNum,wBankNum);
				return 0xffff;
			}
		}

		for(i=0; i<(MAX_BAD_BLK); i++)
		{
		    wPhyscialBadBlk[i] 	= NAND_ALL_BIT_FULL_W;
		    wLogicBadBlk[i] 	= NAND_ALL_BIT_FULL_W;
		    wBadBlkType[i] 		= NAND_ALL_BIT_FULL_W;
		}

		wBadBlkCount = 0;
		
		for(i=0;i<MAX_BAD_BLK;i++)
		{
			wphyscialblknum = bad_blk_info.bad_blk[i];
			
			for(j=0; j<(gSTNandDataCfgInfo.uiBankSize + gSTNandDataCfgInfo.uiBankRecSize);j++)
			{
				if((bad_blk_info.wArrayIndex)==0)
				{
					goto have_bank_readerr;
				}

				if(gSTNandDataCfgInfo.uiBankNum == gSTNandDataCfgInfo.uiMM_Num)
				{
					wTmpPhysicBlk = mm[wBankNum].pMaptable[j];
				}
				else
				{
					wTmpPhysicBlk = mm[0].pMaptable[j];
				}
				
				if((wTmpPhysicBlk== wphyscialblknum)&&(wphyscialblknum!=0xffff))//pMaptable have maptable and recycle
				{
					wPhyscialBadBlk[wBadBlkCount] =  wphyscialblknum;
					wLogicBadBlk[wBadBlkCount] 	  = j;
					if(j < gSTNandDataCfgInfo.uiBankSize)
					{
						wBadBlkType[wBadBlkCount] =  0x01;		// Normal logical block				
					}
					else
					{
						wBadBlkType[wBadBlkCount] =  0x10;		// Recycle block
					}
					
					NF_DATA_ERROR("Debuging1: Have Bad block,block:0x%x,location:0x%x,Bank:0x%x <function:ChangeReadErrBlk> \n",wTmpPhysicBlk,wBadBlkType[wBadBlkCount],gCurrentBankNum);
					
					wBadBlkCount++;
					bad_blk_info.wArrayIndex--;
				}

				break;
			}
		}

//处理该bank内的read err block
have_bank_readerr:

		BankRecyclePosition_DecCount = 0; // 先将bad swap blk从cache队列中剔除，以免在后续操作中使用它
		for(i = 0; i< wBadBlkCount;i++)
		{
			if(wBadBlkType[i] == 0x10)    // recycle
			{
				k = wLogicBadBlk[i];
				k -= BankRecyclePosition_DecCount;//由于修改了队列，之前记录的位置须向前偏移
				while(1)
				{
					if( k < (gBankInfo[wBankNum].wRecycleBlkNum + gSTNandDataCfgInfo.uiBankSize - 1) )
					{
						if(gSTNandDataCfgInfo.uiBankNum == gSTNandDataCfgInfo.uiMM_Num)
						{
							mm[wBankNum].pMaptable[k] = mm[wBankNum].pMaptable[k + 1];
						}
						else
						{
							mm[0].pMaptable[k] = mm[0].pMaptable[k + 1];
						}
						k++;
					}
					else
					{
						if(gSTNandDataCfgInfo.uiBankNum == gSTNandDataCfgInfo.uiMM_Num)
						{
							mm[wBankNum].pMaptable[k] = NAND_ALL_BIT_FULL_W;
						}
						else
						{
							mm[0].pMaptable[k] = NAND_ALL_BIT_FULL_W;
						}
						break;
					}
				}
				BankRecyclePosition_DecCount ++;
				gBankInfo[wBankNum].wRecycleBlkNum--;
			}
		}
		//
		for(i = 0; i< wBadBlkCount;i++)
		{
			if(wBadBlkType[i] == 0x10)
			{
				do //此处要判断是否要将改phy blk 打为bad blk，还是作swap blk
				{
					wTmpPhysicBlk = GetFreeBlkFromRecycleFifo();
					if(NF_NO_SWAP_BLOCK == wTmpPhysicBlk)
					{
						//RWFail_Debug();
						//Sys_Exception(NF_NO_SWAP_BLOCK);
						return 0xffff;
					}

					wSRCPageNum = (UINT32)(wPhyscialBadBlk[i] << gSTNandDataHalInfo.wBlk2Page);
					wTARPageNum = (UINT32)(wTmpPhysicBlk << gSTNandDataHalInfo.wBlk2Page);
					for(j = 0;j < gSTNandDataHalInfo.wBlkPageNum;j++)
					{
						wRetNand = ReadDataFromNand(wSRCPageNum+j,tt_extend_ptr);
						PutCArea(NAND_C_AREA_COUNT_OFS,(GetCArea(NAND_C_AREA_COUNT_OFS,tt_extend_ptr)-1)&0x7f);
						NF_DATA_ERROR("Handle read error block...... \n");
						wRetNand = WriteDataIntoNand(wTARPageNum+j,tt_extend_ptr);
						if(wRetNand)
						{
							//RWFail_Debug();
							wRetNand = nand_write_status_get();
							if((NFCMD_STATUS)wRetNand<=NF_PROGRAM_ERR13)
							{
								return NF_NAND_PROTECTED;
							}
							SetBadFlagIntoNand(wTmpPhysicBlk,NAND_USER_BAD_TAG);
							gBankInfo[gCurrentBankNum].wUserBadBlkNum++;
							continue;
						}
					}
				}while(wRetNand);
				
				wRetNand = IsBlk_GoodOrBad(wPhyscialBadBlk[i], wTmpPhysicBlk);
				if(wRetNand != NF_OK)
				{
					SetBadFlagIntoNand(wPhyscialBadBlk[i], NAND_USER_BAD_TAG);
					gBankInfo[gCurrentBankNum].wUserBadBlkNum++;
				}
				else
					PutFreeBlkIntoRecycleFifo(wPhyscialBadBlk[i]);
				
				PutFreeBlkIntoRecycleFifo(wTmpPhysicBlk);
				//
			}
		}
		
		//处理logic 区的bad blk
		for(i = 0; i< wBadBlkCount;i++)
		{
			if(wBadBlkType[i] == 0x01)  //mm
			{
				do
				{
					wTmpPhysicBlk = GetFreeBlkFromRecycleFifo();	//get new gExchangeBPhysicBlk
					if(NF_NO_SWAP_BLOCK == wTmpPhysicBlk)
					{
						//Sys_Exception(NF_NO_SWAP_BLOCK);
					}	
					wSRCPageNum = (UINT32)(wPhyscialBadBlk[i] << gSTNandDataHalInfo.wBlk2Page);
					wTARPageNum = (UINT32)(wTmpPhysicBlk   << gSTNandDataHalInfo.wBlk2Page); 
					for(j = 0;j < gSTNandDataHalInfo.wBlkPageNum;j++)
					{
						wRetNand = ReadDataFromNand(wSRCPageNum+j,tt_extend_ptr);
						PutCArea(NAND_C_AREA_COUNT_OFS,(GetCArea(NAND_C_AREA_COUNT_OFS,tt_extend_ptr)+1)&0x7f);
						NF_DATA_ERROR("Handle error block...... \n");
						wRetNand = WriteDataIntoNand(wTARPageNum+j,tt_extend_ptr);
						if(wRetNand)
						{
							//RWFail_Debug();
							wRetNand = nand_write_status_get();
							if((NFCMD_STATUS)wRetNand<=NF_PROGRAM_ERR13)
							{
								return NF_NAND_PROTECTED;
							}
							SetBadFlagIntoNand(wTmpPhysicBlk,NAND_USER_BAD_TAG);
							gBankInfo[gCurrentBankNum].wUserBadBlkNum++;
							continue;
						}
					}
				}while(wRetNand);
				
				UpdateMaptable(wTmpPhysicBlk,wLogicBadBlk[i]);
				
				wRetNand = IsBlk_GoodOrBad(wPhyscialBadBlk[i], wTmpPhysicBlk); //此处要判断是否要将改phy blk 打为bad blk，还是作swap blk
				if(wRetNand != NF_OK)
				{
					SetBadFlagIntoNand(wPhyscialBadBlk[i], NAND_USER_BAD_TAG);
					gBankInfo[gCurrentBankNum].wUserBadBlkNum++;
				}	
				else
					PutFreeBlkIntoRecycleFifo(wPhyscialBadBlk[i]);
			}
		}
	}
	
	bad_blk_info.wArrayIndex=0;
	for(i=0;i<MAX_BAD_BLK;i++)
	{	
		bad_blk_info.bad_blk[i]=NAND_ALL_BIT_FULL_W;		
	}
	
	for(i = 0; i< gTotalBankNum; i++)
	{
		NF_DATA_ERROR("Debuging2: Chang bank 0x%x to bank 0x%x <function:ChangeReadErrBlk> \n",gCurrentBankNum,wBankNum);
		wRetNand = ChangeBank(i);		
		if(wRetNand!=0)
		{
			NF_DATA_ERROR("Error2: Chang bank 0x%x to bank 0x%x <function:ChangeReadErrBlk> \n",gCurrentBankNum,i);
			return 0xffff;
		}
	}
#endif	
	NF_DATA_ERROR("Debuging: Done! <function:ChangeReadErrBlk>   \n");
	return 0;
}

//将wblktemp中的data写入wblknum，再读出看是否OK，以此来判断wblknum的好坏
///0: good; 1: bad
UINT16 IsBlk_GoodOrBad(UINT16 wblknum, UINT16 wblktemp)
{
	UINT32	wSRCPageNum;
	UINT32	wTARPageNum;
	UINT16	wRetNand;
	UINT16	j;
	
	wRetNand = DataEraseNandBlk(wblknum);
	if(wRetNand)
	{
		//RWFail_Debug();
		wRetNand = nand_write_status_get();
		if((NFCMD_STATUS)wRetNand<=NF_PROGRAM_ERR13)
		{
			return NF_NAND_PROTECTED;
		}
		//Erase fail
		return 0xffff;
	}
	//write data, read back,看是否有问题
	wSRCPageNum = (UINT32)(wblktemp << gSTNandDataHalInfo.wBlk2Page);
	wTARPageNum = (UINT32)(wblknum << gSTNandDataHalInfo.wBlk2Page);
	for(j = 0;j < gSTNandDataHalInfo.wBlkPageNum;j++)
	{
		ReadDataFromNand(wSRCPageNum+j,tt_extend_ptr);
		PutCArea(NAND_C_AREA_COUNT_OFS,(GetCArea(NAND_C_AREA_COUNT_OFS,tt_extend_ptr)-1)&0x7f);
		NF_DATA_ERROR("Handle error block...... \n");
		wRetNand = WriteDataIntoNand(wTARPageNum+j,tt_extend_ptr);
		if(wRetNand)
		{
			//RWFail_Debug();
			wRetNand = nand_write_status_get();
			if((NFCMD_STATUS)wRetNand<=NF_PROGRAM_ERR13)
			{
				return NF_NAND_PROTECTED;
			}
			return 0xffff;
		}
	}
	if(wRetNand == NF_OK)//read back
	{
		for(j = 0;j < gSTNandDataHalInfo.wBlkPageNum;j++)
		{
			wRetNand = ReadDataFromNand(wSRCPageNum+j,tt_extend_ptr);
			if(wRetNand)
			{
				//RWFail_Debug();
				return 0xffff;
			}
		}
	}
	return NF_OK;
}

SINT32 Set_CalculateFATArea (UINT16 partnum, UINT32 DataStartSec)
{
#if 0	
	UINT16  i;
	UINT32	block_sectornum;
	Nand_OS_LOCK();
	////flush all
	for(i=0;i<ExchangeAMaxNum;i++)
	{
		NandFlushA(i,0,NAND_BUF_FLUSH_ALL,NF_EXCHANGE_A_TYPE);
	}
	
	for(i=0;i<ExchangeBMaxNum;i++)
	{
		NandFlushA(i,0,NAND_BUF_FLUSH_ALL,NF_EXCHANGE_B_TYPE);
	}

	ChangeReadErrBlk();

	gLastPage_Write = 0xffffffff;
	gLastPage_Read = 0xffffffff;
	////
	block_sectornum = (1<<gSTNandDataHalInfo.wBlk2Sector);
	if(partnum==0)
	{
		gFirstFATStart =  Nand_Part0_Offset;
		//match block
		gFirstFATDataStart =  Nand_Part0_Offset + ((DataStartSec-1)/block_sectornum+1)*block_sectornum;
	}
	if(partnum==1)
	{
		gSecondFATStart = Nand_Part1_Offset;
		//match block
		gSecondFADataStart =  Nand_Part1_Offset + ((DataStartSec-1)/block_sectornum+1)*block_sectornum;
	}
	Nand_OS_UNLOCK();
#endif	
	return 0;
}

SINT32 Default_CalculateFATArea (void)
{
	UINT16  i;
	Nand_OS_LOCK();
	////flush all
	for(i=0;i<ExchangeAMaxNum;i++)
	{
		NandFlushA(i,0,NAND_BUF_FLUSH_ALL,NF_EXCHANGE_A_TYPE);
	}
	
	for(i=0;i<ExchangeBMaxNum;i++)
	{
		NandFlushA(i,0,NAND_BUF_FLUSH_ALL,NF_EXCHANGE_B_TYPE);
	}

	ChangeReadErrBlk();
	FlushCurrentMaptable();	// zurong add
	CalculateFATArea();
	gLastPage_Write = 0xffffffff;
	gLastPage_Read = 0xffffffff;
	Nand_OS_UNLOCK();
	return 0;
}

UINT16 	RWFail_Debug(void)
{
	//RWFail_RecordCount ++;
	NF_DATA_ERROR ("\r\n====RWFail_Debug!!=====\r\n");
	//return RWFail_RecordCount;
	return 0;
}

//buffer1: write buffer
UINT8 SetWorkBuffer(UINT8 *p_tt_basic, UINT8 *p_tt_extend)
{	
	if((p_tt_basic == NULL)&&(p_tt_extend==NULL))
	{
		gWorkBufferNum 	= 0;
	}	
	else if((p_tt_basic!=NULL)&&(p_tt_extend==NULL))
	{
		tt_basic_ptr		= p_tt_basic;
		tt_extend_ptr		= p_tt_basic;
		gWorkBufferNum 	= 1;
	}
	else if((p_tt_basic==NULL)&&(p_tt_extend!=NULL))
	{
		tt_basic_ptr		= p_tt_extend;
		tt_extend_ptr		= p_tt_extend;
		gWorkBufferNum 	= 1;
	}
	else
	{
		tt_basic_ptr		= p_tt_basic;
		tt_extend_ptr		= p_tt_extend;
		gWorkBufferNum 	= 2;
	}
	return 	gWorkBufferNum;
}

UINT16	GetNextGoodBlock(UINT16 cur_block)
{
	UINT16	i;
	UINT16	next_block;
	UINT16	get_block;
	UINT16 	wBLkBadFlag;

	get_block = 0;
	next_block = cur_block;
					
	if((next_block >= gLogic2PhysicAreaStartBlk) || (next_block <gSTNandDataCfgInfo.uiDataStart))
	{
		next_block = gSTNandDataCfgInfo.uiDataStart;
	}
	
	for(i=0;i<(gLogic2PhysicAreaStartBlk-gSTNandDataCfgInfo.uiDataStart); i++)	// 最多查找一个循环 - 1个
	{
		wBLkBadFlag = GetBadFlagFromNand(next_block);
		if((wBLkBadFlag != NAND_ORG_BAD_TAG) && (wBLkBadFlag != NAND_USER_BAD_TAG))
		{
			get_block = 1;	// 获取到一个可用block
			break;
		}
		else	// 坏块
		{
			next_block++;
			if((next_block >= gLogic2PhysicAreaStartBlk) || (next_block <gSTNandDataCfgInfo.uiDataStart))
			{
				next_block = gSTNandDataCfgInfo.uiDataStart;
			}
		}
	}
	
	if(get_block!=1)
	{
		return 0xffff;
	}
	else
	{
		return next_block;
	}
}

SINT16 BankInfoAreaInit(void)
{	
	UINT32	total_page;
	UINT32	cur_page_num;
	UINT16	cur_blk;
	UINT16	wBLkBadFlag;	
	UINT16	*ptr_buf;
	UINT16	tag_flag;
	UINT16	control_flag;
	SINT32	ret;
	UINT16	wFirstGoodBlk	= 0xffff;
	UINT16	wLastGoodBlk 	= 0xffff;
	UINT16  wFirstBadBlk  	= 0xffff;
	UINT32	first_page		= 0xffffffff; 
	UINT32  first_bad_page	= 0xffffffff;
	UINT32  wCurBadPage 	= 0xffffffff;
	SINT16  i;

	control_flag 			= 0;	
	gBankInfoCurPage 		= 0xffffffff;
	
	if(gWorkBufferNum!=1)
	{
		gLastPage_Read 	= 0xffffffff;
		ptr_buf = (UINT16 *)tt_extend_ptr;
	}
	else
	{
		gLastPage_Read 	= 0xffffffff;
		gLastPage_Write = 0xffffffff;
		ptr_buf = (UINT16 *)tt_basic_ptr;
	}	
	
	total_page 		= (UINT32)gLogic2PhysicAreaStartBlk*(UINT32)gSTNandDataHalInfo.wBlkPageNum;	
	cur_page_num 	= (UINT32)(gSTNandDataCfgInfo.uiDataStart)*(UINT32)gSTNandDataHalInfo.wBlkPageNum;	
	
	for(cur_page_num = cur_page_num;cur_page_num<total_page;cur_page_num++)
	{
// Stage 1: 判断当前block是否是bad block
		if((cur_page_num%(UINT32)gSTNandDataHalInfo.wBlkPageNum)==0)	// first check if the bad block
		{
			cur_blk = cur_page_num/(UINT32)gSTNandDataHalInfo.wBlkPageNum;
			wBLkBadFlag = GetBadFlagFromNand(cur_blk);
			if((wBLkBadFlag == NAND_ORG_BAD_TAG)||(wBLkBadFlag == NAND_USER_BAD_TAG)||(wBLkBadFlag == NAND_USER_UNSTABLE_TAG))
			{				
				cur_page_num += (UINT32)gSTNandDataHalInfo.wBlkPageNum -1;	// Skip this block
				continue;
			}
			else if(wFirstGoodBlk == 0xffff)
			{
				wFirstGoodBlk = cur_blk;
			}	
		}
				
// Stage 2: 读取当前page，确保读取到无误的page才往下走
		ret = ReadDataFromNand(cur_page_num, (UINT8*)ptr_buf);
		tag_flag= GetCArea(NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS,(UINT8*)ptr_buf);
		if(ret!=0)		//Read出错，此笔数据不可靠,宁缺毋滥
		{
			if(first_bad_page==0xffffffff)
			{
				first_bad_page = cur_page_num;		
			}
			
			if(control_flag==1)	//前面已经有speed up block了
			{
				if((cur_page_num/(UINT32)gSTNandDataHalInfo.wBlkPageNum)!=(first_page/gSTNandDataHalInfo.wBlkPageNum))	
				{	// 出现跟speed up block不在同一个block的错误page   // 这里应该是出错出口
					ret = DataEraseNandBlk(cur_page_num/(UINT32)gSTNandDataHalInfo.wBlkPageNum);
					if(ret)
					{
						SetBadFlagIntoNand(cur_page_num/(UINT32)gSTNandDataHalInfo.wBlkPageNum, NAND_USER_BAD_TAG);
						//gBankInfo[gCurrentBankNum].wUserBadBlkNum++;
					}
					if(wFirstBadBlk!=0xffff)
					{						
						ret = DataEraseNandBlk(wFirstBadBlk);
						if(ret)
						{
							SetBadFlagIntoNand(wFirstBadBlk, NAND_USER_BAD_TAG);
							//gBankInfo[gCurrentBankNum].wUserBadBlkNum++;
						}
					}
					ret = DataEraseNandBlk(first_page/gSTNandDataHalInfo.wBlkPageNum);
					if(ret)
					{
						SetBadFlagIntoNand(first_page/gSTNandDataHalInfo.wBlkPageNum, NAND_USER_BAD_TAG);
						//gBankInfo[gCurrentBankNum].wUserBadBlkNum++;
					}
					
					gBankInfoCurPage = 0xffffffff;
					NF_DATA_ERROR("##Serious error: Bank info initial failed!!   \n");
					return -1;
				}
			}
//////  能够出现在这里说明,1.要么不存在speed up block,  2. 要么cur_page_num跟speed up是同一个block; 否则应该从出错出口出去了
			if(wFirstBadBlk!=0xffff)
			{
				if((cur_page_num/(UINT32)gSTNandDataHalInfo.wBlkPageNum)!=wFirstBadBlk)
				{
					ret = DataEraseNandBlk(wFirstBadBlk);
					if(ret)
					{
						SetBadFlagIntoNand(wFirstBadBlk, NAND_USER_BAD_TAG);
						gBankInfo[gCurrentBankNum].wUserBadBlkNum++;
					}
				}
			}
			wFirstBadBlk = cur_page_num/(UINT32)gSTNandDataHalInfo.wBlkPageNum;			
			
			wCurBadPage = cur_page_num;
			continue; 	//继续下一个page
		}

//stage 3:	到这里读取到的数据一定是安全可靠的  =====================================
		if(tag_flag!=NAND_BANK_INFO_TAG)  // no data found
		{			
			if(control_flag!=1) // 空block,跳过当前block，前面发生bad block则erase掉
			{
				if(wFirstBadBlk!=0xffff)// 前面有发生读错的情况,直接擦除
				{
					ret = DataEraseNandBlk(wFirstBadBlk);
					if(ret)
					{
						SetBadFlagIntoNand(wFirstBadBlk, NAND_USER_BAD_TAG);
					}					
					wFirstBadBlk = 0xffff;
				}// Skip this block left pages
				cur_page_num = ((cur_page_num/gSTNandDataHalInfo.wBlkPageNum + 1)*(UINT32)gSTNandDataHalInfo.wBlkPageNum)-1;
				continue;
			}
			
// 说明上一个page就是最后的maptable所在的page，if(control_flag == 1)
			if(wFirstBadBlk!=0xffff)
			{
				if(wFirstBadBlk!=(gBankInfoCurPage/gSTNandDataHalInfo.wBlkPageNum))
				{
					ret = DataEraseNandBlk(wFirstBadBlk);
					if(ret)
					{
						SetBadFlagIntoNand(wFirstBadBlk, NAND_USER_BAD_TAG);
					}
					wFirstBadBlk = 0xffff;
				}								
			}
			
			if((wCurBadPage!=0xffffffff)&&(wCurBadPage>=gBankInfoCurPage))
			{
				if(wFirstBadBlk!=0xffff)	// 跟
				{
					ret = DataEraseNandBlk(wFirstBadBlk);
					if(ret)
					{
						SetBadFlagIntoNand(wFirstBadBlk, NAND_USER_BAD_TAG);
					}
				}
				ret = DataEraseNandBlk(gBankInfoCurPage/gSTNandDataHalInfo.wBlkPageNum);
				if(ret)
				{
					SetBadFlagIntoNand(gBankInfoCurPage/gSTNandDataHalInfo.wBlkPageNum, NAND_USER_BAD_TAG);
				}
				
				gBankInfoCurPage = 0xffffffff;				
				return -1;	
			}

			if((gBankInfoCurPage%gSTNandDataHalInfo.wBlkPageNum)==0)
			{	// 如果是第1个有效的Block的第1个page,需要判断是否写最后一个block掉电
				if((gBankInfoCurPage/gSTNandDataHalInfo.wBlkPageNum)==wFirstGoodBlk) 
				{
					for(i=(gLogic2PhysicAreaStartBlk-1);i>=gSTNandDataCfgInfo.uiDataStart;i--)
					{
						wBLkBadFlag = GetBadFlagFromNand(i);
						if((wBLkBadFlag != NAND_ORG_BAD_TAG)&&(wBLkBadFlag != NAND_USER_BAD_TAG))	// Good block get
						{								
							wLastGoodBlk = i;
							break;
						}
						if(i==0) { break;} //担心i--会变成一个更大的负/正数，强行break						
					}
					
					if((wFirstGoodBlk!=wLastGoodBlk)&&(wLastGoodBlk!=0xffff))
					{
						ret = DataEraseNandBlk(wLastGoodBlk);	//为了安全起见直接将最后一个good block擦除掉
						if(ret)
						{	
							SetBadFlagIntoNand(wLastGoodBlk, NAND_USER_BAD_TAG);
						}
					}//
				}
			}
			
/*-----------至此,毫无风险的speed up page 被找到了，真是不容易啊!!!-------------*/
			return 0;
		}
		else					// found data
		{	
			gBankInfoCurPage = cur_page_num; // record last valid page number					
			if(control_flag == 0)
			{
				first_page	= cur_page_num;
				control_flag = 1;	// find page yet
				continue;
			}
			
			if((gBankInfoCurPage/gSTNandDataHalInfo.wBlkPageNum)!=(first_page/gSTNandDataHalInfo.wBlkPageNum))
			{
				ret = DataEraseNandBlk(first_page/gSTNandDataHalInfo.wBlkPageNum);
				if(ret)
				{	
					SetBadFlagIntoNand(first_page/gSTNandDataHalInfo.wBlkPageNum, NAND_USER_BAD_TAG);
				}				
				first_page = gBankInfoCurPage;
			}
		}
	}// for
	
//////////  能够到这里说明是整个都扫了一圈了////////////////////////	
	if(control_flag!=0)
	{
		if((wCurBadPage!=0xffffffff)&&(wCurBadPage>=gBankInfoCurPage))
		{
			if(wFirstBadBlk!=0xffff)
			{
				ret = DataEraseNandBlk(wFirstBadBlk);
				if(ret)
				{
					SetBadFlagIntoNand(wFirstBadBlk, NAND_USER_BAD_TAG);
				}
			}
			ret = DataEraseNandBlk(gBankInfoCurPage/gSTNandDataHalInfo.wBlkPageNum);
			if(ret)
			{
				SetBadFlagIntoNand(gBankInfoCurPage/gSTNandDataHalInfo.wBlkPageNum, NAND_USER_BAD_TAG);
			}
			gBankInfoCurPage = 0xffffffff;
			return -1;
		}
		
		if(first_bad_page!=0xffffffff)
		{
			if(first_bad_page==(wFirstGoodBlk*(UINT32)gSTNandDataHalInfo.wBlkPageNum))
			{
				if(wFirstBadBlk!=0xffff)	// 跟
				{
					ret = DataEraseNandBlk(wFirstBadBlk);
					if(ret)
					{
						SetBadFlagIntoNand(wFirstBadBlk, NAND_USER_BAD_TAG);
					}
				}
				ret = DataEraseNandBlk(gBankInfoCurPage/gSTNandDataHalInfo.wBlkPageNum);
				if(ret)
				{
					SetBadFlagIntoNand(gBankInfoCurPage/gSTNandDataHalInfo.wBlkPageNum, NAND_USER_BAD_TAG);
				}
				gBankInfoCurPage = 0xffffffff;
				return -1;
			}
		}		
	}
	if(control_flag==0)
	{
		if(wFirstBadBlk!=0xffff)
		{
			ret = DataEraseNandBlk(wFirstBadBlk);
			if(ret)
			{
				SetBadFlagIntoNand(wFirstBadBlk, NAND_USER_BAD_TAG);
			}
		}
		gBankInfoCurPage = 0xffffffff;
		return -1;
	}
	return 0;
}

SINT16	BankInfoAreaRead(UINT32 pBuf, UINT32 bytes)
{
	UINT16 *ptr_buf;	
	UINT32 ret;	
	
	if(gBankInfoCurPage == 0xffffffff)
	{		
		//NF_DATA_ERROR("No bank info page found!! \n");
		return -1;
	}

	if(gWorkBufferNum!=1)
	{
		ptr_buf = (UINT16 *)tt_extend_ptr;
		gLastPage_Read = 0xffffffff;
	}
	else
	{
		ptr_buf = (UINT16 *)tt_basic_ptr;		
		gLastPage_Read = 0xffffffff;
		gLastPage_Write = 0xffffffff;
	}	

	if(bytes>gSTNandDataHalInfo.wPageSize)
	{
		bytes = gSTNandDataHalInfo.wPageSize;
	}
	
	ret = ReadDataFromNand(gBankInfoCurPage, (UINT8*)ptr_buf);	// Don`t check ECC status		
	if(ret!=0)	// 如果有ECC error 需要将此块打为坏块
	{	
		ret = DataEraseNandBlk(gBankInfoCurPage/gSTNandDataHalInfo.wBlkPageNum);
		if(ret)
		{	
			SetBadFlagIntoNand(gBankInfoCurPage/gSTNandDataHalInfo.wBlkPageNum, NAND_USER_BAD_TAG);
		}
		
		gBankInfoCurPage = 0xffffffff;		
		return -1;
	}
	else
	{
		nf_memcpy_w((void*)pBuf,(void *)ptr_buf,bytes>>1);
	}
	
	return 0;
}

SINT16	BankInfoAreaWrite(UINT32 pBuf, UINT32 bytes)
{
	return NandWriteBankInfoAreaExt(pBuf,bytes,0x00);	// user normal write
}

SINT16	NandWriteBankInfoAreaExt(UINT32 pBuf, UINT32 bytes,UINT16 mode)
{
	UINT16 	*ptr_buf;	
	UINT16	next_block;	
	UINT16	target_block;
	UINT16	cur_block_num;
	UINT16	page_ofs;
	UINT32	next_page;
	SINT32	ret,ret1;
//	UINT16 	wBLkBadFlag;
	
	if(gBankInfoCurPage == 0xffffffff)	 // Not found 
	{
		cur_block_num 	= 0xffff;
		target_block	= gSTNandDataCfgInfo.uiDataStart;
		next_page		= (UINT32)target_block * gSTNandDataHalInfo.wBlkPageNum;
		page_ofs  		= 0;
	}
	else
	{
		cur_block_num	= gBankInfoCurPage/gSTNandDataHalInfo.wBlkPageNum;	
		
		next_page 		= gBankInfoCurPage + 1;
		target_block	= next_page/gSTNandDataHalInfo.wBlkPageNum;		// block
		page_ofs 			= next_page%gSTNandDataHalInfo.wBlkPageNum;		// page
	}
	
	if(page_ofs == 0)	// Need write New block
	{
		next_block = GetNextGoodBlock(target_block);
		if(next_block==0xffff)
		{
			if(cur_block_num!=0xffff)
			{
				ret = DataEraseNandBlk(cur_block_num);
				if(ret)
				{
					SetBadFlagIntoNand(cur_block_num , NAND_USER_BAD_TAG);		
				}
			}
			gBankInfoCurPage = 0xffffffff;
			NF_DATA_ERROR("## Serious Warning: No Good block for speed up!!! \n ");
			return -1;
		}		
		next_page = (UINT32)next_block * gSTNandDataHalInfo.wBlkPageNum;
	}
	else if(cur_block_num!=0xffff)
	{
		next_block = cur_block_num;
	}
	else
	{
		next_block = target_block;
	}
	
	if(gWorkBufferNum!=1)	
	{
		ptr_buf = (UINT16 *)tt_extend_ptr;
		gLastPage_Read = 0xffffffff;
	}
	else
	{
		ptr_buf = (UINT16 *)tt_basic_ptr;
		gLastPage_Read = 0xffffffff;
		gLastPage_Write = 0xffffffff;
	}	
	
	if(bytes>gSTNandDataHalInfo.wPageSize)
	{
		bytes = gSTNandDataHalInfo.wPageSize;	
	}	
	
	do
	{
		nf_memset_w(ptr_buf,0xffff,(gSTNandDataHalInfo.wPageSize>>1));		
		nf_memcpy_w((void *)ptr_buf,(void *)pBuf,bytes>>1);	
		
		PutCArea(NAND_C_AREA_BAD_FLG_OFS_1,		NAND_ALL_BIT_FULL_B);
		PutCArea(NAND_C_AREA_COUNT_OFS,			NAND_ALL_BIT_FULL_B);
		PutCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS,	NAND_ALL_BIT_FULL_W);
		PutCArea(NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS,NAND_BANK_INFO_TAG);
		PutCArea(NAND_C_AREA_BAD_FLG_OFS_6,		NAND_ALL_BIT_FULL_B);
		PutCArea(NAND_C_AREA_PAGE_TO_PAGE_OFS,	NAND_ALL_BIT_FULL_W);	
		
		ret = WriteDataIntoNand(next_page, (UINT8*)ptr_buf);
		if(ret==0x44)
		{
			NF_DATA_ERROR("Error:I`m dead 1st,<function:NandWriteBankInfoAreaExt>  \n");
			//NF_DATA_ERROR("Error:I`m dead 1st, Bank: 0x%x <function:WriteMapTableToNand>\n",wBankNum);
			//dump_bankinfo();
			//dump_curren_bank_maptable();
			//dump_exchange_block();
			return 0x44;
		}
		if(ret)
		{
				ret1 = DataEraseNandBlk(next_page/gSTNandDataHalInfo.wBlkPageNum);
				if(ret1)
				{
					SetBadFlagIntoNand(next_page/gSTNandDataHalInfo.wBlkPageNum, NAND_USER_BAD_TAG);
				}
								
				next_block = GetNextGoodBlock((next_page/gSTNandDataHalInfo.wBlkPageNum)+1);
				if(next_block==0xffff)
				{
					if(cur_block_num!=0xffff)
					{
						ret = DataEraseNandBlk(cur_block_num);
						if(ret)
						{
							SetBadFlagIntoNand(cur_block_num , NAND_USER_BAD_TAG);		
						}
					}
					gBankInfoCurPage = 0xffffffff;
					NF_DATA_ERROR("## Serious Warning: No Good block for speed up 1 !!! \n");
					return -1;
				}
				next_page = (UINT32)next_block * gSTNandDataHalInfo.wBlkPageNum;
		}
	}while(ret);	
	
	gBankInfoCurPage = next_page;
	
	if(mode==0x00)	// normal write mode 
	{
		if((cur_block_num!=next_block)&&(cur_block_num!=0xffff))
		{			
				ret = DataEraseNandBlk(cur_block_num);
				if(ret)
				{
					SetBadFlagIntoNand(cur_block_num , NAND_USER_BAD_TAG);		
				}
		}
	}
	
	return 0;
}

UINT32 GetNandBadBlkNumber(UINT16 wBadType)
{
	UINT16 i;
	UINT32 bad_blk_number = 0;	
	
	Nand_OS_LOCK();

	switch(wBadType)
	{
		case 0x00:
			for(i=0;i<gTotalBankNum;i++)
			{			
				bad_blk_number += gBankInfo[i].wOrgBlkNum + gBankInfo[i].wUserBadBlkNum;			
			}
			break;
			
		case 0x01:
			for(i=0;i<gTotalBankNum;i++)
			{			
				bad_blk_number += gBankInfo[i].wOrgBlkNum;
			}
			break;
			
		case 0x02:
			for(i=0;i<gTotalBankNum;i++)
			{			
				bad_blk_number += gBankInfo[i].wUserBadBlkNum;			
			}
			break;
			
		default:
			bad_blk_number = 0;
			break;
	}
	
	Nand_OS_UNLOCK();
	
	return bad_blk_number;
}

void nf_memset_w(void *address, UINT16 value, UINT16 len)
{
	UINT16 i;
	UINT16 *ptr;
	
	ptr = (UINT16 *)address;
	
	for(i=0;i<len;i++)
	{
		ptr[i] = value;
	}	
}

void nf_memcpy_w(void *taraddr, void *srcaddr, UINT16 len)
{
	UINT16 i;
	UINT16 *ptr1;
	UINT16 *ptr2;
	
	ptr1 = (UINT16 *)taraddr;
	ptr2 = (UINT16 *)srcaddr;
	
	for(i=0;i<len;i++)
	{
		ptr1[i] = ptr2[i];
	}	
}

UINT16 GetCArea(UINT16 byte_ofs,UINT8 *WorkBuffer)
{

	UINT16 temp=0;
	UINT8  sparebuf[8];
	UINT16 *hwBuff;
	UINT32 spare_word;
	UINT8  i;
	
	
	spare_word = spare_flag_get_L();	
	for(i = 0;i < 4;i++) {
		sparebuf[i] = (UINT8)((spare_word >> (i*8)) & 0xff);
	}
	
	spare_word = spare_flag_get_H();	
	for(i = 0;i < 4;i++) {
		sparebuf[i+4] = (UINT8)((spare_word >> (i*8)) & 0xff);
	}
	
	hwBuff = (UINT16 *)sparebuf;	
	
	switch(byte_ofs)
	{
		case NAND_C_AREA_BAD_FLG_OFS_1:				// byte
		case NAND_C_AREA_COUNT_OFS:				
		case NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS:	
		case NAND_C_AREA_BAD_FLG_OFS_6:			
			temp = sparebuf[byte_ofs];
			break;
			
		case NAND_C_AREA_LOGIC_BLK_NUM_OFS:			// word
		case NAND_C_AREA_PAGE_TO_PAGE_OFS:
			temp = hwBuff[byte_ofs>>1];
			break;
			
		default:		
			break;
	}
	
	return temp;
	
}

void PutCArea(UINT16 byte_ofs,UINT16 data)
{

	UINT16 buf_ofs;
	UINT16 *hwBuff;
	UINT8  sparebuf[8];
	UINT32 *spare;
	UINT8 i;
	UINT32 spare_word;
	
	/*Get Spare Area*/
	spare_word = spare_flag_get_L();	
	for(i = 0;i < 4;i++) {
		sparebuf[i] = (UINT8)((spare_word >> (i*8)) & 0xff);
	}
	
	spare_word = spare_flag_get_H();	
	for(i = 0;i < 4;i++) {
		sparebuf[i+4] = (UINT8)((spare_word >> (i*8)) & 0xff);
	}
	
	hwBuff = (UINT16 *)sparebuf;
		
	buf_ofs = byte_ofs;
	switch(byte_ofs)
	{
		case NAND_C_AREA_BAD_FLG_OFS_1:				// byte
		case NAND_C_AREA_COUNT_OFS:				
		case NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS:	
		case NAND_C_AREA_BAD_FLG_OFS_6:			
			sparebuf[byte_ofs] = (UINT8)data;
			break;
			
		case NAND_C_AREA_LOGIC_BLK_NUM_OFS:			// word
		case NAND_C_AREA_PAGE_TO_PAGE_OFS:
			hwBuff[buf_ofs>>1] = data;
			break;
			
		default:		
			break;
	}
	
	/*Set Spare Area*/
	spare = (UINT32*)&sparebuf[0];
	spare_flag_set_L((UINT32)*spare);
	
	spare = (UINT32*)&sparebuf[4];
	spare_flag_set_H((UINT32)*spare);
		
}


SINT16 ChangeBank_debug(void) 
{	
	UINT32 ret=0;
	SINT32 ret_debug=0;
	
	start_monitor = 1;
	
	NF_DATA_ERROR("Debug, Change debug bank <function:ChangeBank_debug>(1)\n");
	if(1)
	{		
		if(gWorkBufferNum==1)	// Single workbuffer
		{
			gLastPage_Read 	= 0xffffffff;
			gLastPage_Write	= 0xffffffff;
			
			if((gpWorkBufferBlock!=NULL) && (gpWorkBufferBlock->wLogicalPage != NAND_ALL_BIT_FULL_W))
			{
				NF_DATA_ERROR("Warning , Should no write heare <function:ChangeBank_debug>(1)\n");
				ret_debug = WriteBackWorkbuffer(gpWorkBufferBlock);
				if(ret==0x44)
				{
					NF_DATA_ERROR("Error:I`m dead 1st,  <function:ChangeBank_debug>(1)\n");
					//NF_DATA_ERROR("Error:I`m dead 1st, Bank: 0x%x <function:WriteMapTableToNand>\n",wBankNum);
					dump_bankinfo();
					dump_curren_bank_maptable();
					dump_exchange_block();
					return 0x44;
				}
			}			
		}
		else if(gWorkBufferNum==2)
		{
			gLastPage_Read 	= 0xffffffff;
		}		

		//restore request bank maptable
		ret = ReadMapTableFromNand_debug();
		if(ret!=0)
		{
			NF_DATA_ERROR("Warning:Read maptabl fail!Bank:0x%x  <function:ChangeBank_debug> \n",0);
			NF_DATA_ERROR("Warning: I`m dead!<function:ChangeBank> \n");
			
			return -1;
		}		
	}
	
	dump_debug_bank_maptable();
	
	return 0;
}

SINT16 ReadMapTableFromNand_debug(void) 
{
	UINT32 wPhysicBlkNum;
	UINT32 wPhysicPage;
	UINT16 wRet;
	wPhysicBlkNum = gBankInfo[0].wMapTableBlk;
	wPhysicPage   = wPhysicBlkNum << gSTNandDataHalInfo.wBlk2Page;
	wPhysicPage   = wPhysicPage + gBankInfo[0].wMapTablePage - 1;
	
	wRet = ReadDataFromNand(wPhysicPage, tt_extend_ptr);
	if(wRet == NF_READ_ECC_ERR)
	{
		//RWFail_Debug();
		
		NF_DATA_ERROR("Serious Warning: Read Maptable page failed!  <function:ReadMapTableFromNand_debug> \n");
		return -1;	// very serious error!!!!
	}	
	
	DMAmmCopy((UINT32)tt_extend_ptr, (UINT32)debug_mm[0].pMaptable, (gSTNandDataCfgInfo.uiBankSize+gSTNandDataCfgInfo.uiBankRecSize)*sizeof(UINT16));
	
	return 0;
}

SINT32 Check_Part0_logical(UINT32 blk)
{
#ifdef PART0_WRITE_MONITOR_DEBUG	
	UINT32 part0_start_blk;
	UINT32 part0_end_blk;
	UINT32 block_sector;
	
	if(gCurrentBankNum!=0)
	{
		return 0;
	}
	
	block_sector = (gSTNandDataHalInfo.wBlkPageNum*gSTNandDataHalInfo.wPageSectorSize);
	
	if((gSTNandConfigInfo.Partition[0].offset%block_sector)!=0)
	{
		NF_DATA_ERROR("\n -->Part 0 offset not block alignment   \n");
	}
	
	part0_start_blk = (gSTNandConfigInfo.Partition[0].offset/block_sector);
	
	if(((gSTNandConfigInfo.Partition[0].offset+gSTNandConfigInfo.Partition[0].size)%block_sector)!=0)
	{
		NF_DATA_ERROR("\n -->Part 0 size not block alignment  \n");
	}

	part0_end_blk = (gSTNandConfigInfo.Partition[0].offset+gSTNandConfigInfo.Partition[0].size)/block_sector;
	
	
	if((blk>=part0_start_blk)&&(blk<part0_end_blk))
	{
		return 0x44;
	}
#endif	
	return 0;
}

SINT32 Check_Part0_Physical(UINT32 blk)
{
#ifdef PART0_WRITE_MONITOR_DEBUG	
	UINT32 i,j;
	UINT32 part0_start_blk;
	UINT32 part0_end_blk;
	UINT32 block_sector;
	
	if(start_monitor==0)
	{
		return 0;
	}
	
	block_sector = (gSTNandDataHalInfo.wBlkPageNum*gSTNandDataHalInfo.wPageSectorSize);
	
	if((gSTNandConfigInfo.Partition[0].offset%block_sector)!=0)
	{
		NF_DATA_ERROR("\n -->Part 0 offset not block alignment  \n");
	}

	part0_start_blk = (gSTNandConfigInfo.Partition[0].offset/block_sector);	
	
	if(((gSTNandConfigInfo.Partition[0].offset+gSTNandConfigInfo.Partition[0].size)%block_sector)!=0)
	{
		NF_DATA_ERROR("\n -->Part 0 size not block alignment  \n");
	}
	
	part0_end_blk = (gSTNandConfigInfo.Partition[0].offset+gSTNandConfigInfo.Partition[0].size)/block_sector;
	
	for(i=part0_start_blk;i<part0_end_blk;i++)
	{
		 if(blk==debug_mm[0].Maptable.Logic2Physic[i])
		 {
			for(j=0;i<50;j++)
			{
				NF_DATA_ERROR("\n Error: --> Physical blk 0x%x Logic blk 0x%x can`t be writen  \n",blk,i);
			}
			
			return 0x44;
		 }
	}
#endif	
	return 0;
}

SINT32 Check_Part0_page(UINT32 page)
{
#ifdef PART0_WRITE_MONITOR_DEBUG	
	UINT32 blk;
	SINT32 ret;
	
	if(start_monitor==0)
	{
		return 0;
	}
	
	blk = page/gSTNandDataHalInfo.wBlkPageNum;
	
	 ret = Check_Part0_Physical(blk);
	 if(ret!=0)
	 {
		NF_DATA_ERROR("\n Error: --> Physical page can`t be writen  \n",page);
	 }
	 
	 return ret;
#else
	return 0;
#endif	 
}

void dump_debug_bank_maptable(void)
{
#ifdef PART0_WRITE_MONITOR_DEBUG
	UINT32 i;
	
	NF_DATA_ERROR("\n -->Dump debug bank mm:  \n");
	
	for(i=0;i<(512+64);i++)
	{
		NF_DATA_ERROR("0x%x ",debug_mm[0].pMaptable[i]);
	}	
#endif	
	NF_DATA_ERROR("\n  \n");	
}

void dump_curren_bank_maptable(void)
{
	UINT32 i;
	
	NF_DATA_ERROR("Bank:0x%x   \n",gCurrentBankNum);
	for(i=0;i<(512+64);i++)
	{
		if((i%16)==0)
		{
			NF_DATA_ERROR(" \n");
		}
		NF_DATA_ERROR(" 0x%x ",mm[0].pMaptable[i]);
	}
	NF_DATA_ERROR("\n Bank:0x%x   \n",gCurrentBankNum);
}

void dump_bankinfo(void)
{
   UINT32 ii=0;
   
	for(ii=0;ii<8;ii++)
	{
		NF_DATA_ERROR(" gBankInfo[0x%x].wLogicBlkNum= 0x%x \n",ii,gBankInfo[ii].wLogicBlkNum);
		NF_DATA_ERROR(" gBankInfo[0x%x].wRecycleBlkNum= 0x%x \n",ii,gBankInfo[ii].wRecycleBlkNum);
		NF_DATA_ERROR(" gBankInfo[0x%x].wMapTableBlk= 0x%x \n",ii,gBankInfo[ii].wMapTableBlk);
		NF_DATA_ERROR(" gBankInfo[0x%x].wMapTablePage= 0x%x \n",ii,gBankInfo[ii].wMapTablePage);
		NF_DATA_ERROR(" gBankInfo[0x%x].wOrgBlkNum= 0x%x \n",ii,gBankInfo[ii].wOrgBlkNum);
		NF_DATA_ERROR(" gBankInfo[0x%x].wUserBadBlkNum= 0x%x \n",ii,gBankInfo[ii].wUserBadBlkNum);
		NF_DATA_ERROR(" gBankInfo[0x%x].wExchangeBlkNum= 0x%x \n",ii,gBankInfo[ii].wExchangeBlkNum);
		NF_DATA_ERROR(" gBankInfo[0x%x].wStatus= 0x%x \n",ii,gBankInfo[ii].wStatus);
	}	
}

void dump_exchange_block(void)
{
	UINT32 i,j;
	
	NF_DATA_ERROR("Dump Exchange A Blocks    \n");
	for(i=0;i<ExchangeAMaxNum;i++)
	{
		NF_DATA_ERROR("gExchangeABlock[0x%x].wBank = 0x%x \n",i,gExchangeABlock[i].wBank);
		NF_DATA_ERROR("gExchangeABlock[0x%x].wLogicBlk= 0x%x \n",i,gExchangeABlock[i].wLogicBlk);
		NF_DATA_ERROR("gExchangeABlock[0x%x].wPhysicBlk= 0x%x \n",i,gExchangeABlock[i].wPhysicBlk);
		NF_DATA_ERROR("gExchangeABlock[0x%x].wLogicalPage= 0x%x \n",i,gExchangeABlock[i].wLogicalPage);
		NF_DATA_ERROR("gExchangeABlock[0x%x].wCurrentPage= 0x%x \n",i,gExchangeABlock[i].wCurrentPage);
		NF_DATA_ERROR("gExchangeABlock[0x%x].wCount= 0x%x \n",i,gExchangeABlock[i].wCount);
		NF_DATA_ERROR("gExchangeABlock[0x%x].wType = 0x%x \n",i,gExchangeABlock[i].wType);
		for(j=0;j<gSTNandDataHalInfo.wBlkPageNum;j++)
		{
			NF_DATA_ERROR(" 0x%x ",gExchangeABlock[i].wPage2PageTable[j]);
		}
		NF_DATA_ERROR(" \n");

	}
	NF_DATA_ERROR(" --------Dump Exchange B Blocks -------  \n");
	for(i=0;i<ExchangeBMaxNum;i++)
	{
		NF_DATA_ERROR("gExchangeBBlock[0x%x].wBank = 0x%x \n",i,gExchangeBBlock[i].wBank);
		NF_DATA_ERROR("gExchangeBBlock[0x%x].wLogicBlk= 0x%x \n",i,gExchangeBBlock[i].wLogicBlk);
		NF_DATA_ERROR("gExchangeBBlock[0x%x].wPhysicBlk= 0x%x \n",i,gExchangeBBlock[i].wPhysicBlk);
		NF_DATA_ERROR("gExchangeBBlock[0x%x].wLogicalPage= 0x%x \n",i,gExchangeBBlock[i].wLogicalPage);
		NF_DATA_ERROR("gExchangeBBlock[0x%x].wCurrentPage= 0x%x \n",i,gExchangeBBlock[i].wCurrentPage);
		NF_DATA_ERROR("gExchangeBBlock[0x%x].wCount= 0x%x \n",i,gExchangeBBlock[i].wCount);
		NF_DATA_ERROR("gExchangeBBlock[0x%x].wType= 0x%x \n",i,gExchangeBBlock[i].wType);
		for(j=0;j<gSTNandDataHalInfo.wBlkPageNum;j++)
		{
			NF_DATA_ERROR(" 0x%x ",gExchangeBBlock[i].wPage2PageTable[j]);
		}
		NF_DATA_ERROR(" \n");

	}
	NF_DATA_ERROR(" --------Dump Current Exchange Blocks -------  \n");
	if(gpWorkBufferBlock!=NULL)
	{
		NF_DATA_ERROR("gpWorkBufferBlock->wBank= 0x%x \n",gpWorkBufferBlock->wBank);
		NF_DATA_ERROR("gpWorkBufferBlock->wLogicBlk= 0x%x \n",gpWorkBufferBlock->wLogicBlk);
		NF_DATA_ERROR("gpWorkBufferBlock->wPhysicBlk= 0x%x \n",gpWorkBufferBlock->wPhysicBlk);
		NF_DATA_ERROR("gpWorkBufferBlock->wLogicalPage= 0x%x \n",gpWorkBufferBlock->wLogicalPage);
		NF_DATA_ERROR("gpWorkBufferBlock->wCurrentPage= 0x%x \n",gpWorkBufferBlock->wCurrentPage);
		NF_DATA_ERROR("gpWorkBufferBlock->wCount= 0x%x \n",gpWorkBufferBlock->wCount);
		NF_DATA_ERROR("gpWorkBufferBlock->wType= 0x%x \n",gpWorkBufferBlock->wType);
		for(j=0;j<gSTNandDataHalInfo.wBlkPageNum;j++)
		{
			NF_DATA_ERROR(" 0x%x ",gpWorkBufferBlock->wPage2PageTable[j]);
		}
	}
	NF_DATA_ERROR(" \n");
}

SINT32 ReUseUnstableBlock(UINT16 TargetBank)
{
	UINT32 BankStartBlk,BankEndBlk;
	UINT16 i ;
	UINT32 blk_cnt_found=0;	
	SINT32 ret=0;
	
	ChangeBank(TargetBank);
	
	BankStartBlk = 0;
	BankEndBlk = 0;
	for(i=0;i<TargetBank;i++)
	{
		BankStartBlk += gSTNandDataCfgInfo.uiBankSize + gSTNandDataCfgInfo.uiBankRecSize + gBankInfo[i].wOrgBlkNum;
	}
	
	BankStartBlk += gLogic2PhysicAreaStartBlk ;
	BankEndBlk    = BankStartBlk + gSTNandDataCfgInfo.uiBankSize + gSTNandDataCfgInfo.uiBankRecSize;
	
	if(BankStartBlk>=gSTNandDataHalInfo.wNandBlockNum)
	{
		BankStartBlk = gSTNandDataHalInfo.wNandBlockNum;
	}
	
	if(BankEndBlk>=gSTNandDataHalInfo.wNandBlockNum)
	{
		BankEndBlk = gSTNandDataHalInfo.wNandBlockNum;
	}
	
	for(i=BankStartBlk;i<BankEndBlk;i++)
	{
		if(GetBadFlagFromNand(i)==NAND_USER_UNSTABLE_TAG)
		{
			ret = PutFreeBlkIntoRecycleFifo(i);
			if(ret==0)
			{
				blk_cnt_found++;
			}
		}
	}
	
	return blk_cnt_found;
}

UINT32 CheckBlockValidPages(UINT32 block)
{
	UINT32 i,pages;
	
	pages = 0;
	for(i=0;i<gSTNandDataHalInfo.wBlkPageNum;i++)
	{
		ReadDataFromNand((block<<gSTNandDataHalInfo.wBlk2Page)+i, tt_extend_ptr);
		if(GetCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS, tt_extend_ptr)!=0xffff)
		{
			pages++;
		}
	}
	
	return pages;
}

#ifdef POWER_LOST_DEBUG_CHECK
	void dump_power_lost_check_info(UINT16 physic,UINT16 exchange,UINT16 error_page,UINT16 who_error,UINT8 *buffer)
	{
		UINT32 i,physic_page;
		SINT32 ret;
		
		if(who_error==0)
		{
			NF_DATA_ERROR("Who happen ECC Error: Original block: 0x%x \n",physic);			
		}
		else
		{
			NF_DATA_ERROR("Who happen ECC Error: Exchange block: 0x%x \n",exchange);
		}
		
		if(who_error==0)
		{
			physic_page = (physic<<gSTNandDataHalInfo.wBlk2Page) + error_page;
		}
		else
		{
			physic_page = (exchange<<gSTNandDataHalInfo.wBlk2Page) + error_page;
		}
		
		if(error_page>=(gSTNandDataHalInfo.wBlkPageNum-1))
		{
			NF_DATA_ERROR("Block Last Page error,No next page need dump \n");
		}
		else
		{
			NF_DATA_ERROR("----------  Dump next page   ------------ \n");
			
			ReadDataFromNand(physic_page+1, buffer);
			
			#if 1
			//dump_buffer(tt_ptr, nand_page_size_get());
			dump_buffer(buffer, gSTNandDataHalInfo.wPageSize);
			dump_buffer(nand_get_spare_buf(), 1024);
			#endif			
		}
		
		for(i=0;i<gSTNandDataHalInfo.wBlkPageNum;i++)
		{
			ret = ReadDataFromNand((physic<<gSTNandDataHalInfo.wBlk2Page) + i, buffer);
			if(ret!=NF_READ_ECC_ERR)
			{
				NF_DATA_ERROR(" Original block count:0x%x \n",GetCArea(NAND_C_AREA_COUNT_OFS,buffer));
				break;
			}
			else
			{
				NF_DATA_ERROR("----------  Page 0x%x ECC Error!   ------------ \n",i);
			}
		}		
		
		
		for(i=0;i<gSTNandDataHalInfo.wBlkPageNum;i++)
		{
			ret = ReadDataFromNand((exchange<<gSTNandDataHalInfo.wBlk2Page) + i, buffer);
			if(ret!=NF_READ_ECC_ERR)
			{
				NF_DATA_ERROR(" Exchange block count:0x%x \n",GetCArea(NAND_C_AREA_COUNT_OFS,buffer));
				break;
			}
			else
			{
				NF_DATA_ERROR("----------  Page 0x%x ECC Error!   ------------ \n",i);
			}
		}
		
		if(who_error==0)
		{
			physic_page = (physic<<gSTNandDataHalInfo.wBlk2Page) + error_page;
		}
		else
		{
			physic_page = (exchange<<gSTNandDataHalInfo.wBlk2Page) + error_page;
		}
		
		NF_DATA_ERROR("----------  Restore buffer data   ------------ \n");
		ReadDataFromNand(physic_page,buffer);
	}
#endif