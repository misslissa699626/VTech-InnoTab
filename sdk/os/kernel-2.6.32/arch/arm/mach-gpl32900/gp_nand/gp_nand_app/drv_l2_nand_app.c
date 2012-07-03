#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/cdev.h>

#include "drv_l2_nand_app.h"
#include <mach/diag.h>

extern void nandAdjustDmaTiming(void);
extern void nandModifyDmaTiming(UINT16 tRP, UINT16 tREH, UINT16 tWP, UINT16 tWH);

#define NF_APP_DEBUG	DIAG_INFO
#define NF_APP_ERROR	DIAG_INFO

UINT8   *APP_tt=0;
static int gp_app_drv_init_flag = 0;

NF_APP_CONFIG_INFO		gSTNandAppCfgInfo;
NF_APP_HAL_INFO 		gSTNandAppHalInfo;

L2P   		gL2PTable[0x400];

/* ----------------  Nand APP Manage Layer Global Varibles -------------- */
UINT16  gAPPCurrPage;
UINT16  gAPPCurrLogicBlock;
UINT16  gAPPCurrPhysicBlock;
UINT16  gAPPCurrSector;
//UINT16  gAPPBadBlkNum;
UINT32  gAPPLastpage;
UINT16  gAPPformatFlag = 0;
//UINT16  gAPPSpeedUp_Flag = 0;  
UINT16 	gSysCodeWriFlag;
UINT16  gAppPhyBlkNum;
UINT32  gBadBlkNum = 0;
//UINT8   dwNFConfig_flag=0;
UINT32	gTargetSectors = 0;
UINT32	gXsferedSectors = 0;

SINT32 NandAppBuildL2P(UINT32 format_type)
{
	UINT32 	i;
	UINT32  ret;
	UINT16  wBadBlkNum;	
	UINT16  wGoodBlkNum;	
	UINT16 	wPhysicBlk;
	UINT16 	wBlkBadFlag;
	UINT16  wTmpVal; 

	for(i = 0;i < gSTNandAppCfgInfo.uiAppSpareSize;i++)	
	{
		gL2PTable[i].wLogicBlk	= 0xffff;
		gL2PTable[i].wPhysicBlk	= 0xffff;
	}

	/* --- Scan Boot Area To Build Maptable Logical Number --- */
	gL2PTable[0].wLogicBlk = 0xfffe;// reserve 0 for maptable use
	wBadBlkNum = 1;
	wPhysicBlk = gSTNandAppCfgInfo.uiAppStart;

	for(i = 0;i < gSTNandAppCfgInfo.uiAppSize;i++)
	{
		wBlkBadFlag = AppCheckOrgBadBlk(wPhysicBlk);
		switch(format_type)
		{
			case 0x01:
				if((wBlkBadFlag != NAND_ORG_BAD_BLK)&&(wBlkBadFlag != NAND_USER_BAD_BLK))
				{
					ret = AppErasePhysicBlock(wPhysicBlk);
					if(ret)	// erase fail, bad block,TBD，需要打成user bad block?
					{
						gL2PTable[wBadBlkNum].wLogicBlk =i;
						wBadBlkNum++;
					}
				}
				else
				{
					gL2PTable[wBadBlkNum].wLogicBlk =i;
					wBadBlkNum++;
				}
				break;

			case 0x10:
				if(wBlkBadFlag != NAND_ORG_BAD_BLK)
				{
					ret = AppErasePhysicBlock(wPhysicBlk);
					if(ret)
					{
						gL2PTable[wBadBlkNum].wLogicBlk =i;
						wBadBlkNum++;
					}
				}
				else
				{
					gL2PTable[wBadBlkNum].wLogicBlk =i;
					wBadBlkNum++;
				}
				break;

			default:
				NF_APP_ERROR("Invalid format type!!! \n");
				return 0xfff1;  //format type error			
		}

		wPhysicBlk++;
		if(wBadBlkNum >= gSTNandAppCfgInfo.uiAppSpareSize)	// Too many bad blocks no include maptable.
		{
			NF_APP_ERROR("Bad blocks more than spare blocks!!! \n");
			return NF_TOO_MANY_BADBLK;
		}
	}

	/* --- Scan Boot Area To Build Maptable --- */
	wGoodBlkNum	= 0;
	wPhysicBlk 	= gSTNandAppCfgInfo.uiAppSpareStart;
	for(i = 0;i < gSTNandAppCfgInfo.uiAppSpareSize;i++)
	{
		wBlkBadFlag = AppCheckOrgBadBlk(wPhysicBlk);

		switch(format_type)
		{
			case 0x01:
				if((wBlkBadFlag != NAND_ORG_BAD_BLK)&&(wBlkBadFlag != NAND_USER_BAD_BLK))
				{
					ret = AppErasePhysicBlock(wPhysicBlk);
					if(!ret)	// Good block
					{
						gL2PTable[wGoodBlkNum].wPhysicBlk = 	wPhysicBlk;
						wGoodBlkNum++;
					}
				}
				break;

			case 0x10:
				if(wBlkBadFlag != NAND_ORG_BAD_BLK)
				{
					ret = AppErasePhysicBlock(wPhysicBlk);
					if(!ret)
					{
						gL2PTable[wGoodBlkNum].wPhysicBlk = 	wPhysicBlk;
						wGoodBlkNum++;
					}
				}
				break;

			default:
				NF_APP_ERROR("Invalid format type!!! \n");
				return -1;
		}

		wPhysicBlk++;
	}

	if((gL2PTable[wGoodBlkNum].wLogicBlk != 0xffff)&&(wGoodBlkNum < gSTNandAppCfgInfo.uiAppSpareSize))
	{
		NF_APP_ERROR("Need more spare good blocks!!! \n");
		return NF_NO_SWAP_BLOCK;		// No enough swap block
	}
	wTmpVal = sizeof(L2P);
	/* --- Write L2P Table to Maptable Block --- */
	// 因为wTmpVal用的是sizeof，所以16 platform & 32 platform byte word会自动调整
	DMAmmCopy((UINT32)gL2PTable, (UINT32)APP_tt, (UINT16)gSTNandAppCfgInfo.uiAppSpareSize*wTmpVal);
	APPPutCArea(NAND_C_AREA_BAD_FLG_OFS_1,		NAND_ALL_BIT_FULL_B);
	APPPutCArea(NAND_C_AREA_BAD_FLG_OFS_6,		NAND_ALL_BIT_FULL_B);
	APPPutCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS,NAND_MAPTABLE_TAG);

	ret = AppWritePhysicPage((UINT32)(((UINT32)gL2PTable[0].wPhysicBlk)<<gSTNandAppHalInfo.wBlk2Page),(UINT32)APP_tt);
	while(ret)			// Write maptable fail
	{
		wPhysicBlk = GetSwapPhysicBlock();
		if(wPhysicBlk == 0xffff)
		{
			NF_APP_ERROR("No more spare good blocks usable!!! \n");
			return NF_NO_SWAP_BLOCK;		// No Swap block
		}

		AppSetBadFlag(gL2PTable[0].wPhysicBlk,NAND_USER_BAD_TAG);

		gL2PTable[0].wPhysicBlk = wPhysicBlk;

		// 因为wTmpVal用的是sizeof，所以16 platform & 32 platform byte word会自动调整
		DMAmmCopy((UINT32)gL2PTable, (UINT32)APP_tt, (UINT16)(gSTNandAppCfgInfo.uiAppSpareSize*sizeof(L2P)));
		APPPutCArea(NAND_C_AREA_BAD_FLG_OFS_1,		NAND_ALL_BIT_FULL_B);
		APPPutCArea(NAND_C_AREA_BAD_FLG_OFS_6,		NAND_ALL_BIT_FULL_B);
		APPPutCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS,	NAND_MAPTABLE_TAG);

		ret = AppWritePhysicPage((UINT32)(((UINT32)gL2PTable[0].wPhysicBlk)<<gSTNandAppHalInfo.wBlk2Page),(UINT32)APP_tt);	  //wwj modify, 2009/5/11
	}
	
	return NF_OK;
}

SINT32 NandAppBuildL2P_Step(UINT32 format_type,UINT16 format_step)
{	
	UINT32 	i;
	UINT32  ret;
	UINT16  wBadBlkNum=0;	
	UINT16 	wPhysicBlk;
	UINT16 	wBlkBadFlag;
	UINT16  wTmpVal; 
			
	if(format_step == 0)  //first format step init value
	{
		wPhysicBlk = gSTNandAppCfgInfo.uiAppStart;
		gAppPhyBlkNum = wPhysicBlk;
		
	}
	
	if(gAppPhyBlkNum > gSTNandAppCfgInfo.uiAppSpareEnd)
	{
		return 0;
	}
	
	if(gAppPhyBlkNum < gSTNandAppCfgInfo.uiAppSpareStart)   //
	{		
		if(format_step==0)
		{
			for(i = 0;i < gSTNandAppCfgInfo.uiAppSpareSize;i++)	
			{
				gL2PTable[i].wLogicBlk	= 0xffff;
				gL2PTable[i].wPhysicBlk	= 0xffff;
			}	

			/* --- Scan Boot Area To Build Maptable Logical Number --- */
			gL2PTable[0].wLogicBlk = 0xfffe;	// reserve 0 for maptable use
			wBadBlkNum = 1;
			wPhysicBlk = gSTNandAppCfgInfo.uiAppStart;
			gAppPhyBlkNum = wPhysicBlk;
		}
		else
		{
			wPhysicBlk = gAppPhyBlkNum;
			wBadBlkNum = gBadBlkNum;
		}
		if((gSTNandAppCfgInfo.uiAppSpareStart - gAppPhyBlkNum) >= 32)
			wTmpVal = 32;
		else
			wTmpVal = gSTNandAppCfgInfo.uiAppSpareStart - gAppPhyBlkNum;
			
		for(i = 0;i < wTmpVal;i++)
		{
			wBlkBadFlag = AppCheckOrgBadBlk(wPhysicBlk);			
	
			switch(format_type)
			{
				case 0x01:
					if((wBlkBadFlag != NAND_ORG_BAD_BLK)&&(wBlkBadFlag != NAND_USER_BAD_BLK))
					{
						ret = AppErasePhysicBlock(wPhysicBlk);
						if(ret)
						{
							gL2PTable[wBadBlkNum].wLogicBlk = wPhysicBlk-gSTNandAppCfgInfo.uiAppStart;//i;	
							wBadBlkNum++;
						}
					}
					else
					{
						gL2PTable[wBadBlkNum].wLogicBlk = wPhysicBlk-gSTNandAppCfgInfo.uiAppStart;//i;
						wBadBlkNum++;
					}
					break;

				case 0x10:
					if(wBlkBadFlag != NAND_ORG_BAD_BLK)
					{
						ret = AppErasePhysicBlock(wPhysicBlk);
						if(ret)
						{
							gL2PTable[wBadBlkNum].wLogicBlk = wPhysicBlk-gSTNandAppCfgInfo.uiAppStart;//i;
							wBadBlkNum++;
						}
					}
					else
					{
						gL2PTable[wBadBlkNum].wLogicBlk = wPhysicBlk-gSTNandAppCfgInfo.uiAppStart;//i;
						wBadBlkNum++;
					}
					break;

				default:
					return 0xfff1;  //format type error			
			}

			wPhysicBlk++;
			if(wBadBlkNum >= gSTNandAppCfgInfo.uiAppSpareSize)	// Too many bad blocks no include maptable.
			{
				return NF_TOO_MANY_BADBLK;	
			}			
		}
		gAppPhyBlkNum = wPhysicBlk;
		gBadBlkNum = wBadBlkNum;
		return 0;  
	}
	/* --- Scan Boot Area To Build Maptable --- */
	if(gAppPhyBlkNum < gSTNandAppCfgInfo.uiAppSpareEnd)
	{
	
		if(gAPPformatFlag == 0)
		{
			wBadBlkNum	= 0;
			wPhysicBlk 	= gSTNandAppCfgInfo.uiAppSpareStart;
			gAppPhyBlkNum  = gSTNandAppCfgInfo.uiAppSpareStart;
			gBadBlkNum = wBadBlkNum;
			gAPPformatFlag = 1;
		}
		else
		{
			wPhysicBlk = gAppPhyBlkNum;
			wBadBlkNum = gBadBlkNum;
		}

		if((gSTNandAppCfgInfo.uiAppSpareEnd - gAppPhyBlkNum) >= 32)
			wTmpVal = 32;
		else
			wTmpVal = gSTNandAppCfgInfo.uiAppSpareEnd - gAppPhyBlkNum;
	
		for(i = 0;i <wTmpVal;i++)
		{
			wBlkBadFlag = AppCheckOrgBadBlk(wPhysicBlk);
					
			switch(format_type)
			{
				case 0x01:
					if((wBlkBadFlag != NAND_ORG_BAD_BLK)&&(wBlkBadFlag != NAND_USER_BAD_BLK))
					{
						ret = AppErasePhysicBlock(wPhysicBlk);
						if(!ret)	// Good block
						{
							gL2PTable[wBadBlkNum].wPhysicBlk = 	wPhysicBlk;
							wBadBlkNum++;
						}
					}
					break;

				case 0x10:
					if(wBlkBadFlag != NAND_ORG_BAD_BLK)
					{
						ret = AppErasePhysicBlock(wPhysicBlk);
						if(!ret)
						{
							gL2PTable[wBadBlkNum].wPhysicBlk = 	wPhysicBlk;
							wBadBlkNum++;
						}
					}
					break;

				default:
					NF_APP_ERROR("Invalid format type!!! \n");
					return -1;
			}

			wPhysicBlk++;
		}
		gAppPhyBlkNum = wPhysicBlk;
		gBadBlkNum = wBadBlkNum;
		return 0;
	}
	if((gL2PTable[wBadBlkNum].wLogicBlk != 0xffff)&&(wBadBlkNum < gSTNandAppCfgInfo.uiAppSpareSize))
	{
		NF_APP_ERROR("Need more spare good blocks!!! \n");
		return NF_NO_SWAP_BLOCK;		// No enough swap block
	}
	wTmpVal = sizeof(L2P);
	/* --- Write L2P Table to Maptable Block --- */
	// 因为wTmpVal用的是sizeof，所以16 platform & 32 platform byte word会自动调整
	DMAmmCopy((UINT32)gL2PTable, (UINT32)APP_tt, (UINT16)gSTNandAppCfgInfo.uiAppSpareSize*wTmpVal);
	APPPutCArea(NAND_C_AREA_BAD_FLG_OFS_1,		NAND_ALL_BIT_FULL_B);
	APPPutCArea(NAND_C_AREA_BAD_FLG_OFS_6,		NAND_ALL_BIT_FULL_B);
	APPPutCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS,	NAND_MAPTABLE_TAG);

	NF_APP_ERROR ("==APP format info save page %d !!==\n",gL2PTable[0].wPhysicBlk <<gSTNandAppHalInfo.wBlk2Page);
	ret = AppWritePhysicPage((UINT32)(((UINT32)gL2PTable[0].wPhysicBlk)<<gSTNandAppHalInfo.wBlk2Page),(UINT32)APP_tt);
	while(ret)			// Write maptable fail
	{
		wPhysicBlk = GetSwapPhysicBlock();
		if(wPhysicBlk == 0xffff)
		{
			NF_APP_ERROR("No more spare good blocks usable!!! \n");
			return NF_NO_SWAP_BLOCK;		// No Swap block
		}

		AppSetBadFlag(gL2PTable[0].wPhysicBlk,NAND_USER_BAD_TAG);

		gL2PTable[0].wPhysicBlk = wPhysicBlk;

		// 因为wTmpVal用的是sizeof，所以16 platform & 32 platform byte word会自动调整
		DMAmmCopy((UINT32)gL2PTable, (UINT32)APP_tt, (UINT16)(gSTNandAppCfgInfo.uiAppSpareSize*sizeof(L2P)));
		APPPutCArea(NAND_C_AREA_BAD_FLG_OFS_1,		NAND_ALL_BIT_FULL_B);
		APPPutCArea(NAND_C_AREA_BAD_FLG_OFS_6,		NAND_ALL_BIT_FULL_B);
		APPPutCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS,	NAND_MAPTABLE_TAG);

		ret = AppWritePhysicPage((UINT32)(((UINT32)gL2PTable[0].wPhysicBlk)<<gSTNandAppHalInfo.wBlk2Page),(UINT32)APP_tt);	  //wwj modify, 2009/5/11
	}
	
	gAppPhyBlkNum++;
	return NF_OK;
}

UINT32 NandAppFormat(UINT16 format_type)
{
	UINT32 ret;
	
	Nand_OS_Init();
	Nand_OS_LOCK();	
	
	if(gSysCodeWriFlag != 0xa55a)
	{
		NF_APP_ERROR("------- APP write disabled!! -------- \n");
		Nand_OS_UNLOCK();		
		return NF_APP_LOCKED;
	}
	
	DrvNand_WP_Initial();
	
	ret = GetNFInfo();
	if(ret != 0)
	{
		NF_APP_ERROR("------- APP Get hal information failed!! --------- \n");
		Nand_OS_UNLOCK();
		return NF_UNKNOW_TYPE;	
	}
	
	if(gSTNandAppCfgInfo.uiAppSpareSize*sizeof(L2P) > sizeof(gL2PTable))
	{
		NF_APP_ERROR("------- APP Spare Size larger then L2P table!! ---------\n");
		Nand_OS_UNLOCK();
		return 0xfff2;
	}
	
	if(gSTNandAppCfgInfo.uiAppSpareSize*sizeof(L2P) > gSTNandAppHalInfo.wPageSize) 
	{
      NF_APP_ERROR("------- L2P table larger then nand page!! ---------\n");
      Nand_OS_UNLOCK();
      return 0xfff3;//App maptable > nand page size
	}

	if(APP_tt==0)
	{
		APP_tt = AppGetWorkbuffer(&APP_tt,gSTNandAppHalInfo.wPageSize);
		if(APP_tt==0)
		{			
			NF_APP_ERROR("----- Allocate APP Workbuffer failed!!! ------- \n");
			Nand_OS_UNLOCK();
			return 0xfff4;
		}
	}
	
	ret = NandAppBuildL2P(format_type);
	
	Nand_OS_UNLOCK();
	return ret;
}

UINT32 NandAppFormat_Step(UINT16 format_type, UINT16 format_step)	
{
	UINT32 ret;
	
	if(gSysCodeWriFlag != 0xa55a)
	{
        return NF_APP_LOCKED;
	}	
	
	if(format_step==0)
	{
		DrvNand_WP_Initial();
		
		ret = GetNFInfo();
		if(ret != 0)
		{
			return NF_UNKNOW_TYPE;	
		}
		
		if(gSTNandAppCfgInfo.uiAppSpareSize*sizeof(L2P) > sizeof(gL2PTable))
		{
			NF_APP_ERROR("------- APP Spare Size larger then L2P table!! ---------\n");
			Nand_OS_UNLOCK();
			return 0xfff2;
		}
		
		if(gSTNandAppCfgInfo.uiAppSpareSize*sizeof(L2P) > gSTNandAppHalInfo.wPageSize) 
		{
	      NF_APP_ERROR("------- L2P table larger then nand page!! ---------\n");
	      Nand_OS_UNLOCK();
	      return 0xfff3;//App maptable > nand page size
		}

		if(APP_tt==0)
		{
			APP_tt = AppGetWorkbuffer(&APP_tt,gSTNandAppHalInfo.wPageSize);
			if(APP_tt==0)
			{			
				NF_APP_ERROR("----- Allocate APP Workbuffer failed!!! ------- \n");
				Nand_OS_UNLOCK();
				return 0xfff4;
			}
		}
	}	
	
	ret = NandAppBuildL2P_Step(format_type,format_step);
	
	return ret;
}

SINT32 NandAppUninit(void)
{
	gp_app_drv_init_flag = 0;
	return 0;
}

SINT32 NandAppInit(void)
{
	UINT32 ret,i;
	UINT16 wPhysicBlk;	
	UINT16 init_flg;

	Nand_OS_Init();
	Nand_OS_LOCK();
	
	if (gp_app_drv_init_flag != 0) {
		Nand_OS_UNLOCK();
		return 0;
	}

	gp_app_drv_init_flag = 1;

	DrvNand_WP_Initial();

	ret = GetNFInfo();
	if(ret != 0)
	{
		NF_APP_ERROR("------- APP Get hal information failed!! --------- \n");
		Nand_OS_UNLOCK();
		return NF_UNKNOW_TYPE;
	}
	
	if(gSTNandAppCfgInfo.uiAppSpareSize*sizeof(L2P) > sizeof(gL2PTable))
	{
		NF_APP_ERROR("-------APP Spare Size larger then L2P table!!---------\n");
		Nand_OS_UNLOCK();
		return 0xfff2;
	}
	
	if(gSTNandAppCfgInfo.uiAppSpareSize*sizeof(L2P) > gSTNandAppHalInfo.wPageSize) 
	{
      NF_APP_ERROR("-------L2P table larger then nand page!!---------\n");
      Nand_OS_UNLOCK();
      return 0xfff3;//App maptable > nand page size
	}

	if(APP_tt==0)
	{
		APP_tt = AppGetWorkbuffer(&APP_tt,gSTNandAppHalInfo.wPageSize);
		if(APP_tt==0)
		{			
			NF_APP_ERROR("----- Allocate APP Workbuffer failed!!! ------- \n");
			Nand_OS_UNLOCK();
			return 0xfff4;
		}
	}

	gAPPCurrPage 		= 0xffff;
	gAPPCurrLogicBlock 	= 0xffff;
	gAPPCurrPhysicBlock = 0xffff;
	gAPPCurrSector 		= 0;	
	
	for(i = 0;i < gSTNandAppCfgInfo.uiAppSpareSize;i++)
	{
		gL2PTable[i].wLogicBlk	= 0xffff;
		gL2PTable[i].wPhysicBlk	= 0xffff;
	}
	/*----	 Scan and Get Maptable Info from nand	---- */
	init_flg = 0;
	for(wPhysicBlk = gSTNandAppCfgInfo.uiAppSpareStart; wPhysicBlk < gSTNandAppCfgInfo.uiAppSpareEnd; wPhysicBlk++)
	{
		ret = GetMapTblInfo(wPhysicBlk);
		if(!ret)
		{
			DMAmmCopy((UINT32)APP_tt, (UINT32)gL2PTable, (UINT16)(gSTNandAppCfgInfo.uiAppSpareSize*sizeof(L2P)));
			if(gL2PTable[0].wPhysicBlk == wPhysicBlk)	// Double Check maptable physic block
			{
				init_flg = 1;
				break;
			} 
			else
			{
				NF_APP_ERROR("------- Found invalid maptable!! --------- \n");
				continue;	//继续找
			}
		}
	}

	if(init_flg==1)
	{
		NF_APP_ERROR("------- APP initial succeed!! --------- \n");
		Nand_OS_UNLOCK();
		return 0;
	}
	else
	{
	  NF_APP_ERROR("------- APP initial failed!!!! --------- \n");
	  Nand_OS_UNLOCK();
		return -1;
	}
}

UINT16 GetMapTblInfo(UINT16 wPhysicBlk)
{
	UINT32 ret;	
    UINT32 maptag;

	ret = AppReadPhysicPage((UINT32)((UINT32)wPhysicBlk<<gSTNandAppHalInfo.wBlk2Page),(UINT32)APP_tt); 
	if(ret!=0)	// Read Error
	{
		return 0xffff;
	}
	else
	{			
	    maptag = APPGetCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS);
		if(maptag == NAND_MAPTABLE_TAG)	// Get the maptable block
		{
			return 0;
		}
		else
		{		   
			return 0xffff;
		}
	}
}

UINT32 GetNFInfo(void)
{
	UINT32 wRet;
	UINT16 i;
	UINT16 wTmpVal;
		
	if(GetNandConfigInfo()!=0)
	{
		NF_APP_ERROR("------- APP Get nand config information failed!! --------  \n");
		//return 1;
		wRet = Nand_Init();
		if(wRet != 0)
		{
			NF_APP_ERROR("------- APP nand hal initial failed!! --------  \n");
			return NF_UNKNOW_TYPE;
		}
	}
	
	Nand_Getinfo(&gSTNandAppHalInfo.wPageSize,&gSTNandAppHalInfo.wBlkPageNum,&gSTNandAppHalInfo.wNandBlockNum);
	
	GetNandAppCfgInfo(&gSTNandAppCfgInfo);
	
	#if 1
	nandAdjustDmaTiming();
	#else
	nandModifyDmaTiming(tRP, tREH, tWP, tWH); // dominant, please get from header
	#endif
	
	NF_APP_DEBUG("gSTNandAppHalInfo.wPageSize: 0x%x  \n",					gSTNandAppHalInfo.wPageSize);
	NF_APP_DEBUG("gSTNandAppHalInfo.wBlkPageNum: 0x%x  \n",				gSTNandAppHalInfo.wBlkPageNum);
	NF_APP_DEBUG("gSTNandAppHalInfo.wNandBlockNum: 0x%x  \n",			gSTNandAppHalInfo.wNandBlockNum);
	
	NF_APP_DEBUG("gSTNandAppCfgInfo.uiAppStart: 0x%x  \n",				gSTNandAppCfgInfo.uiAppStart);
	NF_APP_DEBUG("gSTNandAppCfgInfo.uiAppSize: 0x%x  \n",					gSTNandAppCfgInfo.uiAppSize);
	NF_APP_DEBUG("gSTNandAppCfgInfo.uiAppSpareStart: 0x%x  \n",		gSTNandAppCfgInfo.uiAppSpareStart);
	NF_APP_DEBUG("gSTNandAppCfgInfo.uiAppSparePercent: 0x%x  \n",	gSTNandAppCfgInfo.uiAppSparePercent);
	NF_APP_DEBUG("gSTNandAppCfgInfo.uiAppSpareSize: 0x%x  \n",		gSTNandAppCfgInfo.uiAppSpareSize);
	NF_APP_DEBUG("gSTNandAppCfgInfo.uiAppSpareEnd: 0x%x  \n",			gSTNandAppCfgInfo.uiAppSpareEnd);
		
	gSTNandAppHalInfo.wPageSectorSize = gSTNandAppHalInfo.wPageSize/512;
	gSTNandAppHalInfo.wPageSectorMask = gSTNandAppHalInfo.wPageSectorSize - 1;

	wTmpVal = gSTNandAppHalInfo.wPageSectorSize*gSTNandAppHalInfo.wBlkPageNum;
	for(i = 0;;i++)
	{
		wTmpVal /= 2;
		if(wTmpVal == 0)
			break;
	}
	gSTNandAppHalInfo.wBlk2Sector = i;

	wTmpVal = gSTNandAppHalInfo.wBlkPageNum; 
	for(i = 0;;i++)
	{
		wTmpVal /= 2;
		if(wTmpVal == 0)
			break;
	}
	gSTNandAppHalInfo.wBlk2Page = i;

	wTmpVal = gSTNandAppHalInfo.wPageSectorSize;
	for(i = 0;;i++)
	{
		wTmpVal /= 2;
		if(wTmpVal == 0)
			break;
	}
	gSTNandAppHalInfo.wPage2Sector = i;

	return NF_OK;
}

void GetNandAppCfgInfo(NF_APP_CONFIG_INFO *cfg_info)
{	
	cfg_info->uiAppStart 			= GetAPPAreaStartBlock();
	cfg_info->uiAppSize 			= GetAPPAreaSizeBlock();
	cfg_info->uiAppSpareStart		= cfg_info->uiAppStart + cfg_info->uiAppSize;
	cfg_info->uiAppSparePercent		= GetAPPAreaSparePercent();
	cfg_info->uiAppSpareSize		= cfg_info->uiAppSize/cfg_info->uiAppSparePercent;
	cfg_info->uiAppSpareEnd			= cfg_info->uiAppSpareStart + cfg_info->uiAppSpareSize;	
}

UINT32 AppErasePhysicBlock(UINT32 wPhysicBlkNum)
{
	SINT32 ret;
	
	if((wPhysicBlkNum<gSTNandAppCfgInfo.uiAppStart) ||(wPhysicBlkNum>=gSTNandAppCfgInfo.uiAppSpareEnd))
	{
		NF_APP_ERROR("##-->Serious error: APP beyond range!! Erase block: 0x%x <function:AppErasePhysicBlock> \n",wPhysicBlkNum);
		return 2;
	}
	
	ret = Nand_ErasePhyBlock(wPhysicBlkNum);
	if(ret&0x01)
	{
		NF_APP_ERROR("Error: Erase block failed! Block:0x%x <function:AppErasePhysicBlock> \n",wPhysicBlkNum);
		return 1;
	}
	else
	{
//		NF_APP_ERROR("Debug: Erase block OK! Block:0x%x <function:AppErasePhysicBlock> \n",wPhysicBlkNum);
		return 0;
	}
}

UINT32 AppWritePhysicPage(UINT32 wPhysicPageNum,UINT32 DataAddr)
{
	UINT32 ret;
	UINT16 wPhysicBlkNum;
	
	wPhysicBlkNum = wPhysicPageNum>>gSTNandAppHalInfo.wBlk2Page;
	if((wPhysicBlkNum<gSTNandAppCfgInfo.uiAppStart) ||(wPhysicBlkNum>=gSTNandAppCfgInfo.uiAppSpareEnd))
	{
		NF_APP_ERROR("##-->Serious error: APP beyond range!! Write Page: 0x%x \n",wPhysicPageNum);
		return 2;
	}
	
	ret = Nand_WritePhyPage(wPhysicPageNum, DataAddr);	
	if(ret&0x01)
	{
		NF_APP_ERROR("Error: Write Page failed! Page:0x%x <function:AppWritePhysicPage> \n",wPhysicPageNum);
		return 1;
	}
	else
	{
//		NF_APP_ERROR("Debug: Write Page OK! Page:0x%x <function:AppWritePhysicPage> \n",wPhysicPageNum);
		return 0;
	}	
}

UINT32 AppReadPhysicPage(UINT32 wPhysicPageNum,UINT32 DataAddr)
{
	UINT32 RetNand;
	UINT16 BCH_err_cnt;

	RetNand = Nand_ReadPhyPage(wPhysicPageNum, DataAddr);
	if(RetNand!=0) 
	{
		RetNand = Nand_ReadPhyPage(wPhysicPageNum, DataAddr);
	}

	if(RetNand!=0)
	{
		NF_APP_ERROR("APP Read ECC Error, page:0x%x \n",wPhysicPageNum);
		return NF_READ_ECC_ERR;				// ECC error, can`t correct
	}
	else
	{
		#if 1
		BCH_err_cnt = nand_bch_err_bits_get();		
		if(BCH_err_cnt>GetBitErrCntAsBadBlk())
		{
			NF_APP_ERROR("APP ECC over threshold:0x%x, Error page:0x%x \n",BCH_err_cnt,wPhysicPageNum);
			return NF_READ_BIT_ERR_TOO_MUCH;	// More than 5 bit error,but corrected
		}
		#endif
	}

	return NF_OK;							// No bit error
}


void NandAppDisableWrite(void)
{	
  //DrvNand_WP_Enable();  
	gSysCodeWriFlag = 0x0000;
}

void NandAppEnableWrite(void)
{
  //DrvNand_WP_Disable(); 
	gSysCodeWriFlag = 0xa55a;	
}

void AppSetBadFlag(UINT16 wPhysicBlkNum, UINT16 ErrFlag)
{
	if((wPhysicBlkNum<gSTNandAppCfgInfo.uiAppStart) ||(wPhysicBlkNum>=gSTNandAppCfgInfo.uiAppSpareEnd))
	{
		NF_APP_ERROR("##-->Serious error: APP beyond range!! Set bad block: 0x%x \n",wPhysicBlkNum);
		return ;
	}
	NF_APP_ERROR("Debug: App set bad block:0x%x <AppSetBadFlag> \n",wPhysicBlkNum);
	Nand_sw_bad_block_set((UINT32)wPhysicBlkNum,(UINT32)APP_tt,(UINT8)ErrFlag);
}

UINT16 AppCheckOrgBadBlk(UINT16 wPhysicBlkNum)
{
	UINT8 flag = 0;

	flag = good_block_check(wPhysicBlkNum,(UINT32)APP_tt);
	if(flag==NAND_USER_BAD_TAG)
	{
		return NAND_USER_BAD_BLK;		
	}
	else if(flag!=NAND_GOOD_TAG)
	{		
		return NAND_ORG_BAD_BLK;
	}
	else
	{
		return NAND_GOOD_BLK;
	}
}

SINT16 AppCopyPageToPage(UINT16 wSrcBlock,UINT16 wTargetBlock,UINT16 wPage)
{
	UINT16 i;
	UINT16 ret;

	NF_APP_ERROR("Just Debug: APP Copy Block to Block, Src Block:0x%x Tar Block:0x%x \n",wSrcBlock,wTargetBlock);
	for(i=0;i<wPage;i++)
	{
		ret = AppReadPhysicPage((UINT32)((UINT32)wSrcBlock<<gSTNandAppHalInfo.wBlk2Page)+i,(UINT32)APP_tt); //wwj modify, 2009/5/12
		if(ret!=0)
		{
			if(ret==NF_READ_ECC_ERR)
			{
				NF_APP_ERROR("##-->Serious Error: APP Copy page to page read error, page:0x%x \n",(UINT32)((UINT32)wSrcBlock<<gSTNandAppHalInfo.wBlk2Page)+i);
			}
			else
			{
				NF_APP_ERROR("##-->Serious Warning: APP Copy page to page read error, page:0x%x \n",(UINT32)((UINT32)wSrcBlock<<gSTNandAppHalInfo.wBlk2Page)+i);
			}
		}
		
		ret = AppWritePhysicPage((UINT32)((UINT32)wTargetBlock<<gSTNandAppHalInfo.wBlk2Page)+i,(UINT32)APP_tt); //wwj modify, 2009/5/12
		if(ret)
		{
			NF_APP_ERROR("##-->Serious Error: APP Copy page to page write error, page:0x%x \n",(UINT32)((UINT32)wTargetBlock<<gSTNandAppHalInfo.wBlk2Page)+i);
			return 1;
		}
	}

	return 0;
}

UINT16 GetPhysicBlockNum(UINT16 wLogicNum)
{
	UINT16 i;

	for(i=0;i<gSTNandAppCfgInfo.uiAppSpareSize;i++)
	{
		if(wLogicNum==gL2PTable[i].wLogicBlk)
		{
			return 	(gL2PTable[i].wPhysicBlk);
		}
		else 
			if(0xffff==gL2PTable[i].wLogicBlk)	// The last L2P
		 {
			break;
		 }
	}

	return	(wLogicNum + gSTNandAppCfgInfo.uiAppStart);
}

UINT32 GetSwapPhysicBlock(void)
{
	UINT16 i;
	UINT16 wLastPhysic = 0xffff;

	/*---	Get an new usable physic block	---*/
	for(i=0;i<gSTNandAppCfgInfo.uiAppSpareSize;i++)
	{
		if((0xffff == gL2PTable[i].wLogicBlk)&&(gL2PTable[i].wPhysicBlk!=0xffff))
		{
			wLastPhysic = gL2PTable[i].wPhysicBlk;
			break;
		}
	}

	if(i>=gSTNandAppCfgInfo.uiAppSpareSize)
	{
		return 0xffff;
	}

	/*---	Update L2P table for next use	---*/
	for(;i<gSTNandAppCfgInfo.uiAppSpareSize-1;i++)
	{
		gL2PTable[i].wPhysicBlk = gL2PTable[i+1].wPhysicBlk;
	}

	gL2PTable[gSTNandAppCfgInfo.uiAppSpareSize-1].wPhysicBlk = 0xffff;

	return wLastPhysic;
}

SINT32 PutSwapPhysicBlock(UINT16 wPhysicBlock)
{
	UINT16 i;

	for(i=0;i<gSTNandAppCfgInfo.uiAppSpareSize;i++)
	{
		if(gL2PTable[i].wPhysicBlk == 0xffff)
		{
			gL2PTable[i].wPhysicBlk = wPhysicBlock;
			break;
		}
	}

	if(i<gSTNandAppCfgInfo.uiAppSpareSize)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

SINT32 UpdateL2PTbl(UINT16 wLogicNum,UINT16 wPhysicNum)
{
	UINT16  i;
	UINT16  ret;
	UINT16  CurrPosition;
	UINT16 wSwapBlock;
	UINT16  wSwapDone = 0;

	for(i=0;i<gSTNandAppCfgInfo.uiAppSpareSize;i++)
	{
		if(wLogicNum == gL2PTable[i].wLogicBlk)
		{
			gL2PTable[i].wPhysicBlk =wPhysicNum;
			wSwapDone =1;
			break;
		}
		else if(gL2PTable[i].wLogicBlk ==0xffff)
		{
			wSwapDone =0;
			break;
		}
	}

	if(i>=gSTNandAppCfgInfo.uiAppSpareSize)
	{
		NF_APP_ERROR("##-->Serious Error: APP UpdateL2PTbl,bad logical block full! \n");
		return -1;
	}

	if(wSwapDone==0)	// Adjust L2P table
	{
		CurrPosition =i;
		for(i = gSTNandAppCfgInfo.uiAppSpareSize-1; i>CurrPosition; i--)
		{
			gL2PTable[i].wPhysicBlk = gL2PTable[i-1].wPhysicBlk ;
		}
		gL2PTable[CurrPosition].wPhysicBlk 	= wPhysicNum;
		gL2PTable[CurrPosition].wLogicBlk		= wLogicNum;
	}
	
	NF_APP_ERROR("Debug: APP UpdateL2PTbl! \n");
	
	/* --- Write Maptable to Maptable Block --- */
	DMAmmCopy((UINT32)(gL2PTable), (UINT32)APP_tt, (UINT16)(gSTNandAppCfgInfo.uiAppSpareSize*sizeof(L2P)));
	APPPutCArea(NAND_C_AREA_BAD_FLG_OFS_1,NAND_ALL_BIT_FULL_B);
	APPPutCArea(NAND_C_AREA_BAD_FLG_OFS_6,NAND_ALL_BIT_FULL_B);
	APPPutCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS,NAND_MAPTABLE_TAG);

	ret = AppErasePhysicBlock(gL2PTable[0].wPhysicBlk);
	ret = (ret | AppWritePhysicPage((UINT32)(((UINT32)gL2PTable[0].wPhysicBlk)<<gSTNandAppHalInfo.wBlk2Page),(UINT32)APP_tt));
	while(ret)	// Need swap a maptable block
	{
		wSwapBlock = GetSwapPhysicBlock();
		if(wSwapBlock==0xffff)
		{
			NF_APP_ERROR("##-->Serious Error: APP UpdateL2PTbl,No Swap block 1! \n");
			return NF_NO_SWAP_BLOCK;
		}

		AppSetBadFlag(gL2PTable[0].wPhysicBlk,NAND_USER_BAD_TAG);

		gL2PTable[0].wPhysicBlk = wSwapBlock;

		DMAmmCopy((UINT32)(gL2PTable), (UINT32)APP_tt, (UINT16)(gSTNandAppCfgInfo.uiAppSpareSize*sizeof(L2P)));
		APPPutCArea(NAND_C_AREA_BAD_FLG_OFS_1,NAND_ALL_BIT_FULL_B);
		APPPutCArea(NAND_C_AREA_BAD_FLG_OFS_6,NAND_ALL_BIT_FULL_B);
		APPPutCArea(NAND_C_AREA_LOGIC_BLK_NUM_OFS,NAND_MAPTABLE_TAG);
		ret = AppWritePhysicPage((UINT32)((UINT32)wSwapBlock<<gSTNandAppHalInfo.wBlk2Page),(UINT32)APP_tt);  //wwj modify, 2009/5/12		
	}
	
	return 0;
}

SINT16 AppEraseLogicBlk(UINT16 wLogicBlk)
{
	UINT16 ret;
	UINT16 wTargetPhysicBlk;
	
	wTargetPhysicBlk = GetPhysicBlockNum(wLogicBlk);
//	NF_APP_ERROR("Debug: APP Erase block:0x%x <function:AppEraseLogicBlk> \n",wTargetPhysicBlk);
	ret = AppErasePhysicBlock(wTargetPhysicBlk);
	if(ret)
	{
		NF_APP_ERROR("Error: APP Erase block:0x%x failed <function:AppEraseLogicBlk> \n",wTargetPhysicBlk);
		return 1;
	}

	return 0;
}

SINT32 NandAppFlush(void)
{
	SINT32 	ret;
	UINT16  wNeedUpdate=0;
	UINT16  wSwapPhysicBlk=0xffff;
	UINT16  wTempPhysicBlk=0xffff;

	Nand_OS_LOCK();	
	if(gAPPCurrSector!=0) //flush workbuffer
	{
		NF_APP_ERROR("Debug: APP Flush page:0x%x <function:NandAppFlush> \n",(UINT32)((UINT32)gAPPCurrPhysicBlock<<gSTNandAppHalInfo.wBlk2Page)+gAPPCurrPage);
		ret = AppWritePhysicPage((UINT32)((UINT32)gAPPCurrPhysicBlock<<gSTNandAppHalInfo.wBlk2Page)+gAPPCurrPage,(UINT32)APP_tt);
		while(ret)//写出错
		{
			NF_APP_ERROR("##-->Normal Warning: APP Flush,Write page failed 1! \n");
			wSwapPhysicBlk = GetSwapPhysicBlock();	// Get a new block
			if(wSwapPhysicBlk==0xffff)
			{
				NF_APP_ERROR("##-->Serious Error: APP Flush,No Swap block 1! \n");
				Nand_OS_UNLOCK();
				return NF_NO_SWAP_BLOCK;
			}

			while(ret)  //先一定要将当前workbuffer的数据找一个nand page写进去，以免后面搬移数据破坏workbuffer
			{
				wTempPhysicBlk = GetSwapPhysicBlock();
				if(wTempPhysicBlk == 0xffff)
				{
					NF_APP_ERROR("##-->Serious Error: APP Flush,No Swap block 2! \n");
					Nand_OS_UNLOCK();
					return NF_NO_SWAP_BLOCK;
				}
				ret = AppWritePhysicPage((UINT32)((UINT32)wTempPhysicBlk<<gSTNandAppHalInfo.wBlk2Page),(UINT32)APP_tt);
				if(ret)	//写失败，打坏块
				{
					NF_APP_ERROR("##-->Normal Warning: APP Flush,Write page failed 2! \n");
					AppSetBadFlag(wTempPhysicBlk,NAND_USER_BAD_TAG);
				}
			}
			
			ret = AppCopyPageToPage(gAPPCurrPhysicBlock,wSwapPhysicBlk,gAPPCurrPage);
			while(ret) //write fail ,have new swap blk
			{
				//set bad blk
				AppSetBadFlag(wSwapPhysicBlk,NAND_USER_BAD_TAG);

				wSwapPhysicBlk = GetSwapPhysicBlock();	// Get an new block
				if(wSwapPhysicBlk == 0xffff)
				{
					NF_APP_ERROR("##-->Serious Error: APP Flush,No Swap block 3! \n");
					Nand_OS_UNLOCK();
					return NF_NO_SWAP_BLOCK;
				}
				ret = AppCopyPageToPage(gAPPCurrPhysicBlock,wSwapPhysicBlk,gAPPCurrPage);
			}

			ret = AppReadPhysicPage((UINT32)((UINT32)wTempPhysicBlk<<gSTNandAppHalInfo.wBlk2Page),(UINT32)APP_tt); 
			if(ret)
			{
				//return -1;
			}
			ret = AppErasePhysicBlock(wTempPhysicBlk);
			if(!ret)
			{
				PutSwapPhysicBlock(wTempPhysicBlk);
			}
			else
			{
				AppSetBadFlag(wTempPhysicBlk,NAND_USER_BAD_TAG);
			}

			ret = AppWritePhysicPage((UINT32)((UINT32)wSwapPhysicBlk<<gSTNandAppHalInfo.wBlk2Page)+gAPPCurrPage,(UINT32)APP_tt);

			AppSetBadFlag(gAPPCurrPhysicBlock,NAND_USER_BAD_TAG);			
			
			gAPPCurrPhysicBlock = wSwapPhysicBlk;
			
			wNeedUpdate  = 1;
		}	
		
		if(wNeedUpdate!=0)
		{
			if(UpdateL2PTbl(gAPPCurrLogicBlock,wSwapPhysicBlk))			
			{
				Nand_OS_UNLOCK();
				return -1;
			}	
		}
		
		gAPPCurrSector = 0;
	}
	Nand_OS_UNLOCK();
	return 0;
}

SINT32 NandAppReadSector(UINT32 wReadLBA, UINT16 wLen, UINT32 DataBufAddr,UINT8 mode)
{
	UINT16  wTargetLogicBlk;
	UINT16  wTargetPhysicBlk;
	UINT16  wTargetPage;
	UINT16  wTargetSector;
	UINT16  wLenThisBlk;
	UINT16  wLenThisPage;
	UINT32  ret;
	UINT16  retry_count=2;
	UINT16	wSwapPhysicBlk;
	UINT16	i;
	UINT32	target_addr;
	UINT8	direct_read;
	
#ifndef	 NAND_16BIT	
	if (((4 - (((UINT32) DataBufAddr) & 0x00000003)) & 0x00000003) != 0)
	{
		NF_APP_ERROR("##-->Normal Warning: APP read sector buffer address not alignment!! \n");
		return NF_DMA_ALIGN_ADDR_NEED;
	}
#endif	

	if(wReadLBA >= (gSTNandAppCfgInfo.uiAppSize<<gSTNandAppHalInfo.wBlk2Sector))
	{
		NF_APP_ERROR("##-->Normal Warning: APP read sector address beyond app size!! \n");
		return NF_BEYOND_NAND_SIZE;
	}
	
	Nand_OS_LOCK();
	wTargetLogicBlk	= wReadLBA>>gSTNandAppHalInfo.wBlk2Sector;
	wTargetPage 	= (wReadLBA / gSTNandAppHalInfo.wPageSectorSize)%gSTNandAppHalInfo.wBlkPageNum;
	wTargetSector 	= wReadLBA & gSTNandAppHalInfo.wPageSectorMask;	
	
	gTargetSectors 	= wLen;
	gXsferedSectors = 0;

	do	//传一个block
	{
		wTargetPhysicBlk =  GetPhysicBlockNum(wTargetLogicBlk);
		wLenThisBlk=((gSTNandAppHalInfo.wBlkPageNum-wTargetPage)<<gSTNandAppHalInfo.wPage2Sector)  - wTargetSector;

		if(wLen>wLenThisBlk)
		{
			wLen = wLen - wLenThisBlk;
		}
		else
		{
			wLenThisBlk = wLen;
			wLen = 0;
		}

		do//一个page一个page的写
		{
			wLenThisPage = gSTNandAppHalInfo.wPageSectorSize-wTargetSector;
			if(wLenThisPage>wLenThisBlk)
				wLenThisPage = wLenThisBlk;

			if((gAPPCurrPage==wTargetPage)&&(gAPPCurrLogicBlock==wTargetLogicBlk))// read from workbuffer directly
			{
				if(mode==1) // Kernel mode to user mode
				{
					DMAmmCopyToUser((UINT32)((UINT32)APP_tt+wTargetSector*(512>>BYTE_WORD_SHIFT)), (UINT32)DataBufAddr,(UINT16)(wLenThisPage*(512>>BYTE_WORD_SHIFT)));
				}
				else				// Kernel mode to Kernel mode
				{
					DMAmmCopy((UINT32)((UINT32)APP_tt+wTargetSector*(512>>BYTE_WORD_SHIFT)), (UINT32)DataBufAddr,(UINT16)(wLenThisPage*(512>>BYTE_WORD_SHIFT)));
				}	
			}
			else 
			{
				if((wLenThisPage==gSTNandAppHalInfo.wPageSectorSize)&&(mode==0))	// full page, direct read to user buffer
				{
					target_addr = DataBufAddr;	// User buffer
					direct_read = 1;
				}
				else
				{
					target_addr = (UINT32)APP_tt;		// Work buffer
					direct_read = 0;
				}
			
				ret = AppReadPhysicPage((UINT32)((UINT32)wTargetPhysicBlk<<gSTNandAppHalInfo.wBlk2Page)+wTargetPage,(UINT32)target_addr);
				while(ret && retry_count)
				{
					ret = AppReadPhysicPage((UINT32)((UINT32)wTargetPhysicBlk<<gSTNandAppHalInfo.wBlk2Page)+wTargetPage,(UINT32)target_addr);
					retry_count--;
				}
				
				if(direct_read==0)	// Copy data from workbuffer to user buffer
				{
					if(mode==1) // Kernel mode to user mode
					{
						DMAmmCopyToUser((UINT32)(((UINT32)APP_tt+wTargetSector*(512>>BYTE_WORD_SHIFT))), (UINT32)DataBufAddr,(UINT32)(wLenThisPage*(512>>BYTE_WORD_SHIFT)));
					}
					else				// Kernel mode to Kernel mode
					{
						DMAmmCopy((UINT32)(((UINT32)APP_tt+wTargetSector*(512>>BYTE_WORD_SHIFT))), (UINT32)DataBufAddr,(UINT32)(wLenThisPage*(512>>BYTE_WORD_SHIFT)));
					}
				}
				
				if((retry_count == 0)&& ret)
				{
					NF_APP_ERROR("##-->Normal Warning: APP read error!! Need copy block \n");
					wSwapPhysicBlk = GetSwapPhysicBlock();
					if(wSwapPhysicBlk != 0xffff)
					{	
						NF_APP_ERROR("##-->Normal Warning: APP  copy block 0x%x to 0x%x \n",wTargetPhysicBlk,wSwapPhysicBlk);
						for(i=0;i<gSTNandAppHalInfo.wBlkPageNum;i++)
						{
							AppReadPhysicPage((UINT32)((UINT32)wTargetPhysicBlk<<gSTNandAppHalInfo.wBlk2Page)+i,(UINT32)APP_tt);
							AppWritePhysicPage((UINT32)((UINT32)wSwapPhysicBlk<<gSTNandAppHalInfo.wBlk2Page)+i,(UINT32)APP_tt);
						}

						UpdateL2PTbl(wTargetLogicBlk,wSwapPhysicBlk);
						AppSetBadFlag(wTargetPhysicBlk,NAND_USER_BAD_TAG);
						wTargetPhysicBlk = wSwapPhysicBlk;
					}

					retry_count = 2;

					gAPPCurrLogicBlock 	=0xffff;	// 当前workbuffer里面的数据已经被破坏了，需要clear这些全局变量
					gAPPCurrPhysicBlock	=0xffff;
					gAPPCurrPage 		=0xffff;
					gAPPCurrSector		=0;
				}
				else 
				{
					retry_count = 2;
					
					if(direct_read==0)	// Work buffer updated
					{
						gAPPCurrLogicBlock 	= wTargetLogicBlk;
						gAPPCurrPhysicBlock = wTargetPhysicBlk ;
						gAPPCurrPage 		= wTargetPage;
					}	
				}
			}

			wLenThisBlk = wLenThisBlk - wLenThisPage;
			DataBufAddr = DataBufAddr + wLenThisPage*(512>>BYTE_WORD_SHIFT);

			wTargetPage ++;
			wTargetSector =0;
			
			gXsferedSectors  = gXsferedSectors +wLenThisPage;

		}while(wLenThisBlk>0);

		wTargetLogicBlk++;
		wTargetPage	=0;

	}while(wLen>0);
	
	Nand_OS_UNLOCK();
	return 0;
}

SINT32 NandAppWriteSector(UINT32 wWriteLBA, UINT16 wLen, UINT32 DataBufAddr,UINT8 mode)
{
	SINT16   ret=0;
	//SINT16   ret1;
	//UINT16  wTempBlk;
	UINT16  wTargetLogicBlk;
	UINT16  wTargetPhysicBlk;
	UINT16  wSwapPhysicBlk=0xffff;
	UINT16  wTempPhysicBlk=0xffff;
	UINT16  wTargetPage;
	UINT16  wTargetSector;
	UINT16  wLenThisBlk;
	UINT16  wLenThisPage;
	UINT16  wNeedUpdate =0;


#ifndef	 NAND_16BIT	
	if (((4 - (((UINT32) DataBufAddr) & 0x00000003)) & 0x00000003) != 0)
	{
		NF_APP_ERROR("##-->Normal Warning: APP write sector buffer address not alignment!! \n");
		return NF_DMA_ALIGN_ADDR_NEED;
	}
#endif		

	if(wWriteLBA > ((UINT32)gSTNandAppCfgInfo.uiAppSize*gSTNandAppHalInfo.wBlkPageNum*gSTNandAppHalInfo.wPageSectorSize -1))
	{
		NF_APP_ERROR("##-->Normal Warning: APP write sector address beyond app size!! \n");
		return NF_BEYOND_NAND_SIZE;
	}
	
	Nand_OS_LOCK();
	
	wTargetLogicBlk	= wWriteLBA>>gSTNandAppHalInfo.wBlk2Sector;
	wTargetPage 	= (wWriteLBA / gSTNandAppHalInfo.wPageSectorSize)%gSTNandAppHalInfo.wBlkPageNum;	
	wTargetSector 	= wWriteLBA & gSTNandAppHalInfo.wPageSectorMask;
	
	gTargetSectors = wLen;
	gXsferedSectors = 0;

	do
	{		
		if((wTargetPage==0)&&(wTargetSector==0))	// erase当前block
		{
//			NF_APP_ERROR("Just debug,APP write block start sector & erase this block !! \n");
			ret = AppEraseLogicBlk(wTargetLogicBlk);
			if(ret!=0)
			{
				NF_APP_ERROR("Erase block failed! <function:NandAppWriteSector> \n");
				wSwapPhysicBlk = GetSwapPhysicBlock();
				if(wSwapPhysicBlk==0xffff)
				{
					NF_APP_ERROR("##-->Serious Error: APP write sector get swap block failed 1 !! \n");
					Nand_OS_UNLOCK();
					return NF_NO_SWAP_BLOCK;
				}				
				
				while(ret) //UpdateL2PTbl会破坏workbuffer，先保存workbuffer至nand page				
				{					
					wTempPhysicBlk = GetSwapPhysicBlock();
					if(wTempPhysicBlk==0xffff)
					{
						NF_APP_ERROR("##-->Serious Error: APP write sector get swap block failed 2 !! \n");
						Nand_OS_UNLOCK();
						return NF_NO_SWAP_BLOCK;
					}
					
					ret = AppWritePhysicPage((UINT32)((UINT32)wTempPhysicBlk<<gSTNandAppHalInfo.wBlk2Page),(UINT32)APP_tt);
					if(ret)
					{
						AppErasePhysicBlock(wTempPhysicBlk);
					}
				}
					
				if(UpdateL2PTbl(wTargetLogicBlk,wSwapPhysicBlk))
				{
					NF_APP_ERROR("##-->Serious Error: APP write sector update L2P table failed!! \n");
					Nand_OS_UNLOCK();
					return -1;
				}
				
				ret = AppReadPhysicPage((UINT32)((UINT32)wTempPhysicBlk<<gSTNandAppHalInfo.wBlk2Page),(UINT32)APP_tt);
				if(ret)
				{
					//return -1;	// 就算读错了，也没有办法了，将错就错总比直接宣布失败好 ^_^
				}
				ret = AppErasePhysicBlock(wTempPhysicBlk);
				if(!ret)
				{
					PutSwapPhysicBlock(wTempPhysicBlk);
				}
			}
		}
		
		wTargetPhysicBlk = GetPhysicBlockNum(wTargetLogicBlk);		
		wLenThisBlk 	 = ((gSTNandAppHalInfo.wBlkPageNum - wTargetPage) << gSTNandAppHalInfo.wPage2Sector)  - wTargetSector;

		if(wLen>wLenThisBlk)
		{
			wLen = wLen - wLenThisBlk;
		}
		else
		{
			wLenThisBlk = wLen;
			wLen = 0;
		}

		do
		{
			if((gSTNandAppHalInfo.wPageSectorSize-wTargetSector)<wLenThisBlk)
			{
				wLenThisPage = gSTNandAppHalInfo.wPageSectorSize-wTargetSector;
			}
			else
			{
				wLenThisPage = 	wLenThisBlk;
			}

			if((gAPPCurrPage==wTargetPage)&&(gAPPCurrLogicBlock==wTargetLogicBlk))
			{
				if(mode==1)	// Kernel mode to user mode
				{
					DMAmmCopyFromUser((UINT32)DataBufAddr,(UINT32)((UINT32)APP_tt+wTargetSector*(512>>BYTE_WORD_SHIFT)),(UINT16)(wLenThisPage*(512>>BYTE_WORD_SHIFT)));
				}
				else			 // Kernel mode to Kernel mode
				{
					DMAmmCopy((UINT32)DataBufAddr,(UINT32)((UINT32)APP_tt+wTargetSector*(512>>BYTE_WORD_SHIFT)),(UINT16)(wLenThisPage*(512>>BYTE_WORD_SHIFT)));
				}		
				gAPPCurrSector = wTargetSector+wLenThisPage;
			}
			else
			{
				if(mode==1)
				{	
					DMAmmCopyFromUser((UINT32)DataBufAddr,(UINT32)((UINT32)APP_tt+wTargetSector*(512>>BYTE_WORD_SHIFT)),(UINT16)(wLenThisPage*(512>>BYTE_WORD_SHIFT)));
				}
				else
				{
					DMAmmCopy((UINT32)DataBufAddr,(UINT32)((UINT32)APP_tt+wTargetSector*(512>>BYTE_WORD_SHIFT)),(UINT16)(wLenThisPage*(512>>BYTE_WORD_SHIFT)));
				}

				gAPPCurrSector 		= wTargetSector + wLenThisPage;
				gAPPCurrLogicBlock 	= wTargetLogicBlk;
				gAPPCurrPhysicBlock = wTargetPhysicBlk;
				gAPPCurrPage 		= wTargetPage;
			}			

			if(gAPPCurrSector == gSTNandAppHalInfo.wPageSectorSize)		// Need write data into nand
			{
//				NF_APP_ERROR(" Write data to page:0x%x !! \n",(UINT32)((UINT32)gAPPCurrPhysicBlock<<gSTNandAppHalInfo.wBlk2Page)+gAPPCurrPage);
				ret = AppWritePhysicPage((UINT32)((UINT32)gAPPCurrPhysicBlock<<gSTNandAppHalInfo.wBlk2Page)+gAPPCurrPage,(UINT32)APP_tt);
				while(ret)
				{
					NF_APP_ERROR("Write data to page failed!! \n");
					wSwapPhysicBlk = GetSwapPhysicBlock();	// Get an new block
					if(wSwapPhysicBlk == 0xffff)
					{
						NF_APP_ERROR("##-->Serious Error: APP write sector get swap block failed 3 !! \n");
						Nand_OS_UNLOCK();
						return NF_NO_SWAP_BLOCK;
					}

					while(ret)  //这里也许还需要处理，但是出现错误的机率非常低
					{
						wTempPhysicBlk = GetSwapPhysicBlock();
						if(wTempPhysicBlk == 0xffff)
						{
							NF_APP_ERROR("##-->Serious Error: APP write sector get swap block failed 4 !! \n");
							Nand_OS_UNLOCK();
							return NF_NO_SWAP_BLOCK;
						}
						//save tmp data to nand flash
						ret = AppWritePhysicPage((UINT32)((UINT32)wTempPhysicBlk<<gSTNandAppHalInfo.wBlk2Page),(UINT32)APP_tt);
						if(ret)
						{
							AppErasePhysicBlock(wTempPhysicBlk);
						}
					}
					ret = AppCopyPageToPage(gAPPCurrPhysicBlock,wSwapPhysicBlk,gAPPCurrPage);
					/*if(ret == -1)	// Read source block error
					{
						return -1;
					}
					else*/ 
					if(ret!=0) //write fail ,have new swap blk
					{
						//set bad blk
						AppSetBadFlag(wSwapPhysicBlk,NAND_USER_BAD_TAG);

						wSwapPhysicBlk = GetSwapPhysicBlock();	// Get an new block
						if(wSwapPhysicBlk == 0xffff)
						{
							NF_APP_ERROR("##-->Serious Error: APP write sector get swap block failed 5 !! \n");
							Nand_OS_UNLOCK();
							return NF_NO_SWAP_BLOCK;
						}
						ret = AppCopyPageToPage(gAPPCurrPhysicBlock,wSwapPhysicBlk,gAPPCurrPage);
					}

					ret = AppReadPhysicPage((UINT32)((UINT32)wTempPhysicBlk<<gSTNandAppHalInfo.wBlk2Page),(UINT32)APP_tt);  //wwj modify,2009/5/12
					if(ret)
					{
						//return -1;
					}

					ret = AppErasePhysicBlock(wTempPhysicBlk);
					if(!ret)
					{
						PutSwapPhysicBlock(wTempPhysicBlk);
					}

					ret = AppWritePhysicPage((UINT32)((UINT32)wSwapPhysicBlk<<gSTNandAppHalInfo.wBlk2Page)+gAPPCurrPage,(UINT32)APP_tt);
					AppSetBadFlag(gAPPCurrPhysicBlock,NAND_USER_BAD_TAG);
					gAPPCurrPhysicBlock = wSwapPhysicBlk;
					wNeedUpdate = 1;
				}

				if(wNeedUpdate)
				{
					ret = UpdateL2PTbl(gAPPCurrLogicBlock,wSwapPhysicBlk);
					if(ret)
					{
						Nand_OS_UNLOCK();
						return -1;
					}					
					wNeedUpdate = 0;
					
					//gAPPCurrLogicBlock 	=0xffff;	// 当前workbuffer里面的数据已经被破坏了，需要clear这些全局变量
					//gAPPCurrPhysicBlock	=0xffff;
					gAPPCurrPage 		=0xffff;
				}
				gAPPCurrSector			=0;	
			}

			wLenThisBlk = wLenThisBlk - wLenThisPage;
			DataBufAddr = DataBufAddr + wLenThisPage*(512>>BYTE_WORD_SHIFT);

			wTargetPage ++;
			wTargetSector =0;
			
			gXsferedSectors = gXsferedSectors +wLenThisPage;
		}while(wLenThisBlk>0);

		wTargetLogicBlk++;
		wTargetPage	=0;

	}while(wLen>0);

	Nand_OS_UNLOCK();
	return 0;
}

UINT16 APPGetCArea(UINT16 byte_ofs)
{
	UINT32 spareword[2];
	UINT16 temp=0;
	UINT16 *hwBuff;
	UINT8  *sparebuf;   
	spareword[0] = spare_flag_get_L();
	spareword[1] = spare_flag_get_H();
	hwBuff = (UINT16 *)&spareword[0];
	sparebuf=(UINT8 *)&spareword[0];
    	
	switch(byte_ofs)
	{
		case NAND_C_AREA_BAD_FLG_OFS_1:				// byte
		case NAND_C_AREA_COUNT_OFS:				
		case NAND_C_AREA_EXCHANGE_BLK_TYPE_OFS:	
		case NAND_C_AREA_BAD_FLG_OFS_6:			
			temp = sparebuf[byte_ofs]&0xFF;
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


void APPPutCArea(UINT16 byte_ofs,UINT16 data)
{
    UINT32 spareword[2];
    UINT16 buf_ofs;
    UINT16 *hwBuff;
    UINT8  *sparebuf;
    
    spareword[0] = spare_flag_get_L();
    spareword[1] = spare_flag_get_H();
    hwBuff = (UINT16 *)&spareword[0];
    sparebuf=(UINT8 *)&spareword[0];    

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

	spare_flag_set_L(spareword[0]);
	spare_flag_set_H(spareword[1]);

}

void NandAPPGetNandInfo(UINT32 *blk_size,UINT32 *page_size)
{
	*blk_size 	= gSTNandAppHalInfo.wBlkPageNum;
	*page_size	= gSTNandAppHalInfo.wPageSize;
}


void NandAPPGetProcess(UINT32 *target_sec,UINT32 *xsfered_sec)
{
	*target_sec 	= gTargetSectors;
	*xsfered_sec	= gXsferedSectors;
}
EXPORT_SYMBOL(NandAppWriteSector);
EXPORT_SYMBOL(NandAppReadSector);
EXPORT_SYMBOL(NandAppFormat);
EXPORT_SYMBOL(NandAppInit);
EXPORT_SYMBOL(NandAppFlush);
EXPORT_SYMBOL(NandAPPGetNandInfo);
