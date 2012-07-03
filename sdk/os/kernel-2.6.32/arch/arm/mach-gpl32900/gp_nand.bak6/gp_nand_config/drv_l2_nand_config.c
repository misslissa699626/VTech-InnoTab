#include "drv_l2_nand_config.h"
#include <linux/slab.h>
#include <mach/gp_apbdma0.h>
#include <mach/gp_board.h>
#include <linux/string.h>
#include <linux/uaccess.h>



/*######################	Warning		################################
#																																		#
#		Don`t modify the content below,otherwise it will								#
# 	be cause driver can`t work normally or can`t work optimumly!		#	
#																																		#
###################################################################*/

//static struct semaphore	nand_sem;

NF_CONFIG_INFO 		gSTNandConfigInfo;
UINT16				gBitErrCntAsBadBlk=6;
extern UINT32 		Nand_Chip;
extern int disable_parse_header;

UINT32 Nand_Chip_Get(void)
{
	return Nand_Chip;
}

SINT32 Nand_OS_Init(void)
{
#if 0
	static UINT8 semaphore_flag = 0;
	
		if(semaphore_flag==0)
		{
			init_MUTEX(&nand_sem);
			semaphore_flag=1;
		}
#endif
    return 0;
}

SINT32 Nand_OS_Exit(void)
{
		return 0;
}

void Nand_OS_LOCK(void)
{
	#if 1
		nand_sema_lock();
		Nand_Chip_Switch(Nand_Chip_Get());
	#else
		down(&nand_sem);
	#endif
}

void Nand_OS_UNLOCK(void)
{
	#if 1
		nand_sema_unlock();
	#else
		up(&nand_sem);
	#endif
}

/////////////////////////////////
#if 1
void Sys_Exception(SINT32 errorcode)
{
	switch(errorcode)
	{
		default:
			errorcode++;
		break;
	}
	return;				
}
#endif

UINT8 *AppGetWorkbuffer(UINT8 **ptr,UINT32 size)
{
	if(*ptr==0)
	{		
		*ptr = (UINT8*)kmalloc(size, GFP_DMA);	// 按照实际的page allocate RAM	
	}
	
	return (*ptr);
}


UINT8 DataGetWorkbuffer(UINT8 **ptr1, UINT8 **ptr2,UINT32 size)
{	
	if(*ptr1==0)
	{
		*ptr1 = (UINT8*)kmalloc(size, GFP_DMA);	// 按照实际的page allocate RAM	
	}
	
	if(*ptr2==0)
	{
		*ptr2 = (UINT8*)kmalloc(size, GFP_DMA);	// 按照实际的page allocate RAM
	}
	
	return 0;
}


SINT32 GetNandConfigInfo(void)
{	
	SINT32 ret=-1;
	UINT8  *pTmpbuffer=(UINT8*)0;	
	UINT8	 i;	
	UINT16 bch_mode;
	
	if(disable_parse_header==0)
	{
		pTmpbuffer = (UINT8*)kmalloc(8*1024, GFP_DMA);	//parse header时，page size最大为1024 // william, since read by page so should use max page size. now is 8192, future need to update when greater than this
		if(pTmpbuffer==0)
		{
			printk("Get Nand Config information: Kmalloc temp work buffer fail!\n");
			printk("Kmalloc Size:0x%x \n",8*1024);
				
			ret = -1;
		}
		else
		{		
			ret = NandParseBootHeader(pTmpbuffer);
		}

		if(pTmpbuffer!=0)
		{
			kfree(pTmpbuffer);
		}
	}
	else
	{
		ret = -1;
	}
	
	if(ret != 0)
	{		
		printk("<<<<<<<------=== Default Config Information! ====----->>>>>> \n");
		
		//return -1;
		gSTNandConfigInfo.uiAppStart		 	= APP_AREA_START;
		gSTNandConfigInfo.uiAppSize		 		= APP_AREA_SIZE;
		gSTNandConfigInfo.uiAppSpareStart		= APP_AREA_SPARE_START;
		gSTNandConfigInfo.uiAppSparePercent		= APP_AREA_SPARE_PERCENT;
		gSTNandConfigInfo.uiAppSpareSize	 	= APP_AREA_SPARE_SIZE;
		gSTNandConfigInfo.uiDataStart		 	= DATA_AREA_START;
		gSTNandConfigInfo.uiNoFSAreaSize		= 0;
		gSTNandConfigInfo.uiBankSize		 	= GetDataBankLogicSizeFromBth();		//200;
		gSTNandConfigInfo.uiBankRecSize	 		= GetDataBankRecycleSizeFromBth();			//20;
		gSTNandConfigInfo.uiPartitionNum		= 0;
	}
	else
	{
		printk("<<<<<<-----=== BootHeader Config Information! ====----->>>>> \n");		
		
		gSTNandConfigInfo.uiAppStart			= GetAppStartBlkFromBth();
		gSTNandConfigInfo.uiAppSize				= GetAppSizeOfBlkFromBth();
		gSTNandConfigInfo.uiAppSpareStart		= gSTNandConfigInfo.uiAppStart	+ gSTNandConfigInfo.uiAppSize;
		gSTNandConfigInfo.uiAppSparePercent		= GetAppPercentFromBth();
		gSTNandConfigInfo.uiAppSpareSize		= gSTNandConfigInfo.uiAppSize/gSTNandConfigInfo.uiAppSparePercent;
		gSTNandConfigInfo.uiDataStart			= gSTNandConfigInfo.uiAppSpareStart + gSTNandConfigInfo.uiAppSpareSize;
		gSTNandConfigInfo.uiNoFSAreaSize		= GetNoFSAreaSectorSizeFromBth();
		gSTNandConfigInfo.uiBankSize			= GetDataBankLogicSizeFromBth();
		gSTNandConfigInfo.uiBankRecSize			= GetDataBankRecycleSizeFromBth();
		
		gSTNandConfigInfo.uiPartitionNum 		= GetPartNumFromBth();
		
		for(i=0;i<gSTNandConfigInfo.uiPartitionNum;i++)
		{			
			 GetPartInfoFromBth(i, (UINT16 *)(&gSTNandConfigInfo.Partition[i].size),&(gSTNandConfigInfo.Partition[i].attr));
			 gSTNandConfigInfo.Partition[i].size = (gSTNandConfigInfo.Partition[i].size)*1024*2;// MB to sector
		}
		if(gSTNandConfigInfo.uiPartitionNum!=0)
		{
				gSTNandConfigInfo.Partition[0].offset = gSTNandConfigInfo.uiNoFSAreaSize;
				for(i=1;i<gSTNandConfigInfo.uiPartitionNum;i++)
				{
					gSTNandConfigInfo.Partition[i].offset = gSTNandConfigInfo.Partition[i-1].offset + gSTNandConfigInfo.Partition[i-1].size;
				}				
		}		
	}	
	
	bch_mode = get_bch_mode();
	switch(bch_mode)
	{
		case 0x00:  //BCH1K60_BITS_MODE=0x00
			SetBitErrCntAsBadBlk(48);
			break;
		case 0x01:  //BCH1K40_BITS_MODE=0x01
			SetBitErrCntAsBadBlk(32);
			break;
		case 0x02:  //BCH1K24_BITS_MODE=0x02
			SetBitErrCntAsBadBlk(20);
			break;
		case 0x03:  //BCH1K16_BITS_MODE=0x03
			SetBitErrCntAsBadBlk(13);
			break;
		case 0x04:  //BCH512B8_BITS_MODE=0x04
			SetBitErrCntAsBadBlk(6);
			break;
		case 0x05:  //BCH512B4_BITS_MODE=0x05
			//SetBitErrCntAsBadBlk(4);
			SetBitErrCntAsBadBlk(3);
			break;
		case 0xff:
			SetBitErrCntAsBadBlk(1);
			printk("BCH OFF \n");
			break;
		default:	
			SetBitErrCntAsBadBlk(1);
			printk("Unknow BCH Mode \n");
			break;	
	}
	
	printk("ECC threshold bit numer: 0x%x \n",				GetBitErrCntAsBadBlk());
	
	printk("gSTNandConfigInfo.uiAppStart: 0x%x \n",			gSTNandConfigInfo.uiAppStart);
	printk("gSTNandConfigInfo.uiAppSize: 0x%x \n",			gSTNandConfigInfo.uiAppSize	);
	printk("gSTNandConfigInfo.uiAppSparePercent: 0x%x \n",	gSTNandConfigInfo.uiAppSparePercent);
	printk("gSTNandConfigInfo.uiDataStart: 0x%x \n",			gSTNandConfigInfo.uiDataStart);
	printk("gSTNandConfigInfo.uiNoFSAreaSize: 0x%x \n",		gSTNandConfigInfo.uiNoFSAreaSize);
	printk("gSTNandConfigInfo.uiBankSize: 0x%x \n",			gSTNandConfigInfo.uiBankSize);
	printk("gSTNandConfigInfo.uiBankRecSize: 0x%x \n",		gSTNandConfigInfo.uiBankRecSize);
	
	printk("gSTNandConfigInfo.uiPartitionNum: 0x%x \n",		gSTNandConfigInfo.uiPartitionNum);
	
	for(i=0;i<gSTNandConfigInfo.uiPartitionNum;i++)
	{
		printk("Partition %d offset:0x%x size: 0x%x  attr:0x%x  \n",i, gSTNandConfigInfo.Partition[i].offset,gSTNandConfigInfo.Partition[i].size,gSTNandConfigInfo.Partition[i].attr);
	}
	
	printk("<<<<<<-----=== Get Config Information End! ====----->>>>> \n");
	return ret;
}

void SetBitErrCntAsBadBlk(UINT16 bit_cnt)
{
	gBitErrCntAsBadBlk = bit_cnt;
}

UINT16 GetBitErrCntAsBadBlk(void)
{
	return gBitErrCntAsBadBlk;
}


UINT16 GetAPPAreaStartBlock(void)
{
	return (gSTNandConfigInfo.uiAppStart);
}

UINT16 GetAPPAreaSizeBlock(void)
{
	return (gSTNandConfigInfo.uiAppSize);
}

UINT16 GetAPPAreaSparePercent(void)
{
	return (gSTNandConfigInfo.uiAppSparePercent);
}

UINT16 GetDataAreaStartBlock(void)
{
	return (gSTNandConfigInfo.uiDataStart);
}

UINT32 GetNoFSAreaSectorSize(void)
{
	return (gSTNandConfigInfo.uiNoFSAreaSize);
}

UINT16 GetDataAreaBankSize(void)
{
	return (gSTNandConfigInfo.uiBankSize);
}

UINT16 GetDataAreaBankRecycleSize(void)
{
	return (gSTNandConfigInfo.uiBankRecSize);
}


SINT32 gp_memcpy(SINT8 *dest, SINT8 *src, UINT32 Len)
{
	UINT32 i = 0;

	for ( i = 0; i < Len; i++ ) {
		dest[i] = src[i];
	}

	return(Len);
}

void DMAmmCopy(UINT32 wSourceAddr, UINT32 wTargeAddr, UINT16 Count)
{
	//gp_memcpy((SINT8 *)wTargeAddr,(SINT8 *)wSourceAddr,(UINT32)Count);
	memcpy((SINT8 *)wTargeAddr,(SINT8 *)wSourceAddr,(UINT32)Count);
}

void DMAmmCopyToUser(UINT32 wSourceAddr, UINT32 wTargeAddr, UINT16 Count)
{
	copy_to_user((void *)wTargeAddr,(const void*)wSourceAddr,Count);
}

void DMAmmCopyFromUser(UINT32 wSourceAddr, UINT32 wTargeAddr, UINT16 Count)
{
	copy_from_user((void *)wTargeAddr,(const void*)wSourceAddr,Count);
}

