#include <linux/kernel.h>
#include <linux/string.h>

#include "NF_cfgs.h"
#include "../hal/nf_s330.h"
#include "storage_op.h"

SDev_t gsdev;

SDev_t* getSDev(void)
{
	return &gsdev;
}

unsigned char getSDevinfo(unsigned char flg)
{
	unsigned char ret = 0;
	SDev_t* pSDev = getSDev();
	switch(flg) 
	{
		case SMALLBLKFLG:
			ret= pSDev->IsSmallblk;
			break;
		case SUPPORTBCHFLG:
			ret = pSDev->IsSupportBCH;
			break;
		default://UNKNOWN
			ret = 0;
			break;	
	}
	SPMP_DEBUG_PRINT("ret = %d\n", ret);
	return ret;
}

unsigned int IsSmallblk_storage(void)
{
	return getSDevinfo(SMALLBLKFLG);
}


//DUAL_ROM_IMAGE
unsigned int IsDualRomImage(void)
{
	SDev_t* pSDev = getSDev();

	switch(pSDev->DeviceID) 
	{
		case DEVICE_NAND:
			return SUCCESS;
		default://UNKNOWN
			break;	
	}
	return FAILURE;

}

void initNandFunptr(void)
{
	SDev_t* pSDev = getSDev();

	pSDev->IsSmallblk = 0;
	pSDev->IsSupportBCH = 1;
	pSDev->DeviceID = DEVICE_NAND;

	pSDev->predInitDriver    = (predInitDriver_t)initDriver;
/* modify by mm.li 01-12,2011 clean warning */
	/*
	pSDev->predEraseBlk = (predReadWritePage_t)EraseBlock_ex;
	*/
	pSDev->predEraseBlk = (predEraseBlk_t)EraseBlock_ex;
/* modify end */	
	pSDev->predReadWritePage = (predReadWritePage_t)ReadWritePage_ex;
	
}


void initNandPBAFunptr(void)
{
	SDev_t* pSDev = getSDev();

	pSDev->IsSmallblk = 0;
	pSDev->IsSupportBCH = 0;
	pSDev->DeviceID = DEVICE_NAND_PBA;

	pSDev->predInitDriver    = (predInitDriver_t)initDriver;
	pSDev->predEraseBlk      = (predEraseBlk_t)EraseBlock;
	pSDev->predReadWritePage = (predReadWritePage_t)ReadWritePage_ex;
}

void initNandSBLKFunptr(void)
{
	SDev_t* pSDev = getSDev();

	pSDev->IsSmallblk = 1;
	pSDev->IsSupportBCH = 1;
	pSDev->DeviceID = DEVICE_NAND_SBLK;

	pSDev->predInitDriver    = (predInitDriver_t)initDriver;
	pSDev->predEraseBlk      = (predEraseBlk_t)EraseBlock_SB;
	pSDev->predReadWritePage = (predReadWritePage_t)ReadWritePage_SB;
}

void initNandOTPFunptr(void)
{
	SDev_t* pSDev = getSDev();

	pSDev->IsSmallblk = 1;
	pSDev->IsSupportBCH = 0;
	pSDev->DeviceID = DEVICE_NAND_OTP;

	pSDev->predInitDriver    = (predInitDriver_t)initDriver;
	pSDev->predEraseBlk      = (predEraseBlk_t)EraseBlock_SB;
	pSDev->predReadWritePage = (predReadWritePage_t)ReadWritePage_SB;
}


#if SUPPORT_SD0_BOOT
void initSD0Funptr(void)
{
	SDev_t* pSDev = getSDev();

	pSDev->IsSmallblk = 0;
	pSDev->IsSupportBCH = 0;
	pSDev->DeviceID = DEVICE_SD0;
	pSDev->predInitDriver    = (predInitDriver_t)initdriver_SD0;
	//pSDev->predReInitDriver  = (predReInitDriver_t)reinitDriver_SD0;
	pSDev->predEraseBlk      = (predEraseBlk_t)EraseBlock_SD0;
	pSDev->predReadWritePage = (predReadWritePage_t)ReadWritePage_SD0;
}
#endif

#if SUPPORT_SD1_BOOT
void initSD1Funptr(void)
{
	SDev_t* pSDev = getSDev();

	pSDev->IsSmallblk = 0;
	pSDev->IsSupportBCH = 0;
	pSDev->DeviceID = DEVICE_SD1;

	pSDev->predInitDriver    = (predInitDriver_t)initdriver_SD1;
	//pSDev->predReInitDriver  = (predReInitDriver_t)reinitDriver_SD1;
	pSDev->predEraseBlk      = (predEraseBlk_t)EraseBlock_SD1;
	pSDev->predReadWritePage = (predReadWritePage_t)ReadWritePage_SD1;
}
#endif

#if SUPPORT_SPI_BOOT
void initSerialFlashFunptr(void)
{
	SDev_t* pSDev = getSDev();

	pSDev->IsSmallblk = 0;
	pSDev->IsSupportBCH = 0;
	pSDev->DeviceID = DEVICE_SPI;

	pSDev->predInitDriver    = (predInitDriver_t)initDriver_SPI;
	//pSDev->predReInitDriver  = (predReInitDriver_t)reinitDriver_SPI;
	//pSDev->predEraseBlk      = (predEraseBlk_t)EraseBlock_SPI;
	pSDev->predReadWritePage = (predReadWritePage_t)ReadWritePage_SPI;
}

#endif

void initfunptr(unsigned int aDevId)
{
	SPMP_DEBUG_PRINT("dev sel :%d\n", aDevId);
	//gSelDevId = aDevId;
	switch(aDevId) 
	{
		case DEVICE_NAND:
			initNandFunptr();
			break;

		case DEVICE_NAND_PBA:
			initNandPBAFunptr();
			break;
		case DEVICE_NAND_SBLK:
			initNandSBLKFunptr();
			break;					
		case DEVICE_NAND_OTP:
			initNandOTPFunptr();
			break;		


#if SUPPORT_SD0_BOOT
		case DEVICE_SD0:
			initSD0Funptr();
			break;
#endif

#if SUPPORT_SD1_BOOT
		case DEVICE_SD1:
			initSD1Funptr();
			break;									
#endif
			
#if SUPPORT_SPI_BOOT							
		case DEVICE_SPI:
			initSerialFlashFunptr();
			break;
#endif						
		default://UNKNOWN
			break;
	}
}

psysinfo_t*	g_pSysInfo_storagop = NULL;
psysinfo_t* initstorage(void)
{
	SDev_t* pSDev = getSDev();

	if(g_pSysInfo_storagop)
		return g_pSysInfo_storagop;

	//initfunptr(GetBootDevID());
	initfunptr(DEVICE_NAND);

	return (g_pSysInfo_storagop = pSDev->predInitDriver(0x09, 0xf05d, 0x1fffff));
	
}
psysinfo_t* initstorage_ex(void)
{
	SDev_t* pSDev = getSDev();
	
	//initfunptr(GetBootDevID());
	initfunptr(DEVICE_NAND);
	g_pSysInfo_storagop = pSDev->predInitDriver(0x09, 0xf05d, 0x1fffff);
	
	return g_pSysInfo_storagop;
}

unsigned long ReadPage_storage(unsigned int pageNo, unsigned long* ptrPyldData, unsigned long* DataBuffer)
{
	unsigned int retry = 0;//RETRY_COUNT;//PengJian test
		
	SDev_t* pSDev = getSDev();
	
	initstorage();
	
	do
	{
		pSDev->predReadWritePage(pageNo, (unsigned long*) ptrPyldData, (unsigned long*) DataBuffer, NF_READ);
		if(pSDev->IsSupportBCH)
		{
			if(BCHProcess_ex((unsigned long*)ptrPyldData, (unsigned long*)DataBuffer, g_pSysInfo_storagop->u16PyldLen, BCH_DECODE) != ret_BCH_FAIL)
			{
				DUMP_NF_BUFFER(debug_flag, ptrPyldData,512, DataBuffer, 64);
				return SUCCESS;
			}
		}
		else
		{
			DUMP_NF_BUFFER(debug_flag, ptrPyldData,512, DataBuffer, 64);
			return SUCCESS;
		}
	}while(retry--);
	
	DUMP_NF_BUFFER(debug_flag, ptrPyldData,512, DataBuffer, 64);
	return FAILURE;		// there is something wrong
}

#define		NAND_ENABLE

#ifndef NAND_ENABLE
unsigned long WritePage_Hiddern(unsigned int pageNo, unsigned long* ptrPyldData, unsigned long* DataBuffer)
{

	SDev_t* pSDev = getSDev();
	
	initstorage();

	pSDev->predReadWritePage(pageNo, (unsigned long*) ptrPyldData, (unsigned long*) DataBuffer, NF_WRITE_HIDDEN);
	

	return SUCCESS;	

}
#endif
unsigned long WritePage_storage(unsigned int pageNo, unsigned long* ptrPyldData, unsigned long* DataBuffer)
{
	SDev_t* pSDev = getSDev();
	
	initstorage();

	if(pSDev->IsSupportBCH)
	{
		BCHProcess_ex((unsigned long*)ptrPyldData, (unsigned long*)DataBuffer, g_pSysInfo_storagop->u16PyldLen, BCH_ENCODE);
	}
	pSDev->predReadWritePage(pageNo, (unsigned long*) ptrPyldData, (unsigned long*) DataBuffer, NF_WRITE);
	

	return SUCCESS;	

}
void Erase_Block_storage(unsigned short blk)
{
	SDev_t* pSDev = getSDev();
	initstorage();
	
	pSDev->predEraseBlk(blk);
	
}

