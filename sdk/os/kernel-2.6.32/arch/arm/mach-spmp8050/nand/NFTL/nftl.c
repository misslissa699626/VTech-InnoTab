
#ifdef	_WIN32
#include <stdlib.h>   // For _MAX_PATH definition
#include <stdio.h>
#include <malloc.h>
#include <memory.h>

#include "../include/nand_cfg.h"
#include "../include/fifo.h"
#include "../include/nand.h"
#include "../include/nftl.h"
#else
#include <linux/blkdev.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/genhd.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/blkpg.h>
#include <linux/ata.h>
#include <linux/hdreg.h>

#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/clk.h>

#include <linux/spinlock.h>

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/spmp_gpio.h>
#include <mach/regs-scu.h>
#include <mach/general.h>
#include <mach/common.h>
#include <mach/gp_gpio.h>

#include "../hal/hal_base.h"
#include "../champ/NF_cfgs.h"
#include "../champ/storage_op.h"
#include "../champ/nand_FDisk.h"
#include "nand.h"
#include "nftl.h"
#include "nand_pgbase.h"


#endif//#ifdef	_WIN32

#define NFTL_SUPPORT_DMA0 0

unsigned int g_nftl_IsfirstInit = 0;
Nftl_Info_t gnftl;

int	g_write_flag_control;/*0:disable,1:enable*/
int	g_wirte_storage_flag;/*0x00:no write,0x01:write nand,0x10:write sd,luosx add*/

//=========================================================================
//		nftl mutex lock
//=========================================================================

static nf_mutex_t nftl_mutex;

#define CYG_PRIORITY_NFTL_MUTEX_CEILING 1
#define ENABLE_MUTEX_LOCK_NFTL 1

int g_Is_nftl_mutex_init = 0;

void nftl_mutex_init(void) 
{
#if ENABLE_MUTEX_LOCK_NFTL	
	if(g_Is_nftl_mutex_init==0)
	{
		g_Is_nftl_mutex_init =1;
		init_MUTEX(&nftl_mutex);
	}
#endif
}

void nftl_mutex_lock(void)
{
#if ENABLE_MUTEX_LOCK_NFTL
	//down_interruptible(&nftl_mutex);
#endif	
}
void nftl_mutex_unlock(void)
{
#if ENABLE_MUTEX_LOCK_NFTL
	//up(&nftl_mutex);
#endif	
}
void nftl_mutex_destroy(void)
{
#if ENABLE_MUTEX_LOCK_NFTL
	
#endif	
}




//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void NFTL_memset(unsigned char* ptr, unsigned char ch, unsigned int size)
{
	unsigned char* ptr_Nc = NandCache2NonCacheAddr(ptr);
	memset(ptr_Nc, ch, size);
}

void NFTL_memcpy(unsigned char* dest, unsigned char* src, unsigned int size)
{
	unsigned char* src_Nc = NandCache2NonCacheAddr(src);
	unsigned char* dest_Nc = NandCache2NonCacheAddr(dest);
#ifdef	_WIN32
	//printk("KING->memcpy(0x%x, 0x%x, %d)\n", dest, src, size);
	memcpy(dest, src, size);
#else
	#if	NFTL_SUPPORT_DMA0				// hnwhy
		//printk("KING->NFTL_memcpy-->dma0_memcpy_pollingExt(0x%x, 0x%x, %d)\n", dest, src, size);
		//if((src_Nc&0x3) || (dest_Nc&0x3))
			//printk("ERROR : NFTL_memcpy-->memcpy(0x%x, 0x%x, %d)\n", dest_Nc, src_Nc, size);	
	
		dma0_memcpy_pollingExt(dest, src, size , 0x100 /* burst 16 */);
	#else
		//printk("KING->NFTL_memcpy-->memcpy(0x%x, 0x%x, %d)\n", dest_Nc, src_Nc, size);
		memcpy(dest_Nc, src_Nc, size);
	#endif
#endif
	
}
unsigned int NFTL_memcmp(unsigned char* ptr1, unsigned char* ptr2, int len)
{
	int i;
	int count = (len+3)>>2;
	for(i=0; i<count; i++)
	{
		if(ptr1[i]!=ptr2[i])	
			return FAILURE;
	}
	return SUCCESS;
}
void NFTL_print_buf(unsigned char* buf,int len)
{
	int i;
	unsigned char* buf_NC = NandCache2NonCacheAddr(buf);
	//printk("buf_NC:0x%8x\n",buf_NC);
	printk("buf_NC:%x len:%d\n", (unsigned int)buf_NC, len);
	for(i=0;i<len;i++)
	{
	
		if( (i%16) == 8)
			printk("- ");
		if( (i%16) == 0)	
			printk("\n");
			
		printk("%02x ",buf_NC[i]);	
	}
	printk("\n");	
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////



unsigned int NFTL_buf_init(void)
{
	gnftl.WriteCachePageaddr = NFTL_NO_PAGE;
	gnftl.ReadCachePageaddr = NFTL_NO_PAGE;
	
	gnftl.pReadBuf = memAlloc(gnftl.pagesize+ALIGNED64);
	gnftl.pCacheReadBuf = memAlloc(gnftl.pagesize+ALIGNED64);
	gnftl.pCacheWriteBuf = memAlloc(gnftl.pagesize+ALIGNED64);

	gnftl.pTmpBuf = memAlloc(gnftl.pagesize+ALIGNED64);

	
	printk("gnftl.pReadBuf malloc(%u):0x%x\n", gnftl.pagesize+ALIGNED64, (unsigned int)(gnftl.pReadBuf) );
	printk("gnftl.pCacheReadBuf malloc(%u):0x%x\n", gnftl.pagesize+ALIGNED64, (unsigned int)(gnftl.pCacheReadBuf));
	printk("gnftl.pCacheWriteBuf malloc(%u):0x%x\n", gnftl.pagesize+ALIGNED64, (unsigned int)(gnftl.pCacheWriteBuf));
	printk("gnftl.pTmpBuf malloc(%u):0x%x\n", gnftl.pagesize+ALIGNED64, (unsigned int)(gnftl.pTmpBuf));

	
	if(gnftl.pCacheWriteBuf && gnftl.pCacheReadBuf && gnftl.pReadBuf && gnftl.pTmpBuf)
	{
#ifdef	_WIN32
		gnftl.pCacheReadBuf_ali	= gnftl.pCacheReadBuf;
		gnftl.pCacheWriteBuf_ali	= gnftl.pCacheWriteBuf;
		gnftl.pReadBuf_ali	= gnftl.pReadBuf;
#else
		gnftl.pCacheReadBuf_ali	= (unsigned char*)(((unsigned int)gnftl.pCacheReadBuf+ALIGNED64-1)&(0xFFFFFFC0));
		gnftl.pCacheWriteBuf_ali	= (unsigned char*)(((unsigned int)gnftl.pCacheWriteBuf+ALIGNED64-1)&(0xFFFFFFC0));	
		gnftl.pReadBuf_ali	= (unsigned char*)(((unsigned int)gnftl.pReadBuf+ALIGNED64-1)&(0xFFFFFFC0));
		gnftl.pTmpBuf_ali	= (unsigned char*)(((unsigned int)gnftl.pTmpBuf+ALIGNED64-1)&(0xFFFFFFC0));
#endif
	}
	else
	{
		printk("Error : gnftl.pCacheWriteBuf(0x%x) malloc is failure!\n", (unsigned int)gnftl.pCacheWriteBuf);
		printk("Error : gnftl.pCacheReadBuf(0x%x) malloc is failure!\n", (unsigned int)gnftl.pCacheReadBuf);
		printk("Error : gnftl.pReadBuf(0x%x) malloc is failure!\n", (unsigned int)gnftl.pReadBuf);
		printk("Error : gnftl.pTmpBuf(0x%x) malloc is failure!\n", (unsigned int)gnftl.pReadBuf);

		return FAILURE;	
	}
	cache_invalidate();
	return 	SUCCESS;
}

void NFTL_printInfo(Chip_Info_t* pChipInfo)
{
	printk("pChipInfo->block_count:%d\n", pChipInfo->block_count);
	printk("pChipInfo->page_per_block:%d\n", pChipInfo->page_per_block);
	printk("pChipInfo->pagesize:%d\n", pChipInfo->pagesize);
	printk("pChipInfo->sector_per_page:%d\n", pChipInfo->sector_per_page);			
}

void  NFTL_FDisk(void)
{
	PartitionInfo_S astPartitionBaseInfo[4]={
#if 1
			{0, 0, PARTITION_TYPE_FAT32, 128},
			{0, 0, PARTITION_TYPE_FAT32, 16},
			{0, 0, PARTITION_TYPE_FAT32, 0xFFFFFFFF},
#else
			{0, 0, PARTITION_TYPE_FAT32, 0xFFFFFFFF},
			{0, 0, PARTITION_TYPE_EMPTY, 0},

			{0, 0, PARTITION_TYPE_EMPTY, 0},
#endif			
			{0, 0, PARTITION_TYPE_EMPTY, 0}
	};

	SPMP_DEBUG_PRINT( "begin Fdisk....\n");
	Nand_FDisk(astPartitionBaseInfo);
	SPMP_DEBUG_PRINT( "End of Fdisk.\n");
}

nf_flag_t g_nftl_ctrl_flag;

//g_nftl_Isfirst_ctrl_flag = 0;
unsigned int NFTL_Init(Chip_Info_t* pChipInfo)
{
	unsigned int IsNeedFdisk = 0;


	//if(getSDevinfo(SMALLBLKFLG))
	if(IsSmallblk_storage())
	{
		return FAILURE;
	}
	
	printk("NFTL_Init g_nftl_IsfirstInit:%d\n", g_nftl_IsfirstInit);
	if(g_nftl_IsfirstInit)
	{
		//if(g_nftl_Isfirst_ctrl_flag==1)
		{
			printk("XXXX\n");
			wait_event_interruptible_timeout(g_nftl_ctrl_flag.queue, g_nftl_ctrl_flag.flag == 1, 1 * 1000);
			printk("VVVV\n");
		}
		memcpy((unsigned char*)pChipInfo, (unsigned char*)&gnftl, sizeof(Chip_Info_t));
		NFTL_printInfo(pChipInfo);

		//cyg_flag_setbits(&g_nftl_ctrl_flag, 1);	
		return 	SUCCESS;
	}

	//init success
	g_nftl_ctrl_flag.flag = 0;
	g_nftl_IsfirstInit=1;

	nftl_mutex_init();

	//printk("11111\n");
	nftl_mutex_lock();

	if(Nand_Init(pChipInfo, &IsNeedFdisk)==FAILURE)
	{ 
		printk("Error : Nand_Init is fail.\n");
		nftl_mutex_unlock();
		return FAILURE;
	}


	gnftl.write_count = 0;
	gnftl.write_count_bak = 0;

#if SUPPORT_NPB
	pChipInfo->block_count += NAND_PGBASE_USED_LOGBLK;
#endif	
	
	NFTL_printInfo(pChipInfo);
	
	memcpy((unsigned char*)&gnftl, (unsigned char*)pChipInfo, sizeof(Chip_Info_t));

	flag_setbits(&(g_nftl_ctrl_flag.flag), 1); 
	printk("\nXXXXXXXXXXXXXXXXXXXXXcyg_flag_setbits(&g_nftl_ctrl_flag, 1).\n");

	//printk("KING->pChipInfo->block_count:%d\n", pChipInfo->block_count);
	
	gnftl.page_per_blk_shift = valshift(gnftl.page_per_block);
	gnftl.sector_per_page_shift = valshift(gnftl.sector_per_page);
	
	if(NFTL_buf_init()==FAILURE)
	{
		printk("Error : NFTL_buf_init is fail.\n");
		nftl_mutex_unlock();
		return FAILURE;		
	}



#if SUPPORT_NPB
	NPB_Init(IsNeedFdisk);//add by king weng

	gnftl.npb_logpage_count = NAND_PGBASE_USED_LOGPAGECOUNT;
	
	printk("gnftl.npb_logpage_count:%d\n", gnftl.npb_logpage_count);
#else
	gnftl.npb_logpage_count = 0;
#endif		


	nftl_mutex_unlock();	

#ifndef	_WIN32
	if(IsNeedFdisk)
	{
		NFTL_FDisk();
	}
#endif

#if !SUPPORTCOPYMACHINE
	//dsp_init();		// should init dsp after the rom file system initialized		      
#endif
#if SUPPORT_NPB
	NPB_Creat_log2phy_Table();
#endif

	SPMP_DEBUG_PRINT("Init OK!\n");
	return 	SUCCESS;
}

void NFTL_Close(void)
{
	nftl_mutex_destroy();

	Nand_free();
	
	printk("NFTL_Close\n");
	
	if(gnftl.pCacheReadBuf)
	{
		memFree(gnftl.pCacheReadBuf);	
	}	
	if(gnftl.pCacheWriteBuf)
	{
		memFree(gnftl.pCacheWriteBuf);	
	}
	if(gnftl.pReadBuf)
	{
		memFree(gnftl.pReadBuf);
	}
	memset(&gnftl, 0, sizeof(Nftl_Info_t));	
	
	g_nftl_IsfirstInit = 0;
}


unsigned int NFTL_ReadPage(unsigned int logpage, unsigned char *pdata)
{
	unsigned int rc;
	unsigned char *ptr = pdata;

//printk("KING->NFTL_ReadPage(%d, 0x%x).\n", logpage, pdata);	

	// if the data pointer is not align 64 byte, we use the temporary buffer to read data
	if(((unsigned int)pdata)&(ALIGNED64-1))
	{
		ptr = gnftl.pTmpBuf_ali;
		//NFTL_memcpy(ptr, pdata, gnftl.pagesize);
		printk("NFTL_ReadPage(%d, 0x%x)\n", logpage, (unsigned int)pdata);
	}	
	
	
#if SUPPORT_NPB	
	if(logpage < gnftl.npb_logpage_count)
	{	
		//rc = NPB_ReadPage(logpage, pdata);	
		rc = NPB_ReadPage(logpage, ptr);
		NF_TRACE(debug_flag, "NPB_ReadPage(%d, 0x%x)\n", logpage, (unsigned int)ptr);
	}
	else
	{
		logpage -= gnftl.npb_logpage_count;
#endif	
	
		//rc = Nand_ReadPage(logpage, pdata);
		rc = Nand_ReadPage(logpage, ptr);
		NF_TRACE(debug_flag, "Nand_ReadPage(%d, 0x%x)\n", logpage, (unsigned int)ptr);		
#if SUPPORT_NPB	
	}
#endif	

	/// if we use the temporary buffer to read data, should copy back to the des buffer
	if(ptr!=pdata)
	{
		NFTL_memcpy(pdata, ptr, gnftl.pagesize);
	}

	return rc;
}

unsigned int NFTL_WritePage(unsigned int logpage, unsigned char *pdata)
{
	unsigned char *ptr = pdata;

//printk("KING->NFTL_WritePage(%d, 0x%x).\n", logpage, pdata);

	if(((unsigned int)pdata)&(ALIGNED64-1))
	{
		ptr = gnftl.pTmpBuf_ali;
		NFTL_memcpy(ptr, pdata, gnftl.pagesize);
	}

#if SUPPORT_NPB
	if(logpage < gnftl.npb_logpage_count)
	{	
		//printk("NPB_WritePage(%d, 0x%x)\n", logpage, ptr);		
		return NPB_WritePage(logpage, ptr);
	}
	logpage -= gnftl.npb_logpage_count;
#endif
	//printk("Nand_WritePage(%d, 0x%x)\n", logpage, ptr);	
	return Nand_WritePage(logpage, ptr);
}

/////////////////////////////////

/// 1. first, check the write buffer to see if the data is cached
/// 2. yes, just copy the data to the des buffer
/// 3. not, read page from nand

////////////////////////////////
unsigned int NFTL_ReadPageByCache(unsigned int logpage, unsigned char *pdata)
{
	unsigned int rc = SUCCESS;
	//nftl_mutex_lock();
#if __NFTL_BUFFER_ON__
	if(logpage==gnftl.WriteCachePageaddr)
	{
		NF_TRACE(debug_flag, "KING->logpage:%d==gnftl.CachePageaddr:%d\n", logpage, gnftl.WriteCachePageaddr);
		NFTL_memcpy(pdata, gnftl.pCacheWriteBuf_ali, gnftl.pagesize);
		//nftl_mutex_unlock();
		return SUCCESS;
	}
#endif
//#if __NFTL_BUFFER_ON__
#if 0
	if(logpage!=gnftl.ReadCachePageaddr)
	{
		NFTL_ReadPage(logpage, gnftl.pCacheReadBuf_ali);
		gnftl.ReadCachePageaddr = logpage;
	}

	NFTL_memcpy(pdata, gnftl.pCacheReadBuf_ali, gnftl.pagesize);
#else
	rc = NFTL_ReadPage(logpage, pdata);
#endif	
	//nftl_mutex_unlock();
	return rc;//SUCCESS;
}

unsigned int NFTL_WritePageByCache(unsigned int logpage, unsigned char *pdata)
{
	unsigned int rc = SUCCESS;
	//nftl_mutex_lock();
#if __NFTL_BUFFER_ON__	
	gnftl.write_count=1;
	
	if(logpage==gnftl.WriteCachePageaddr)
	{
		NFTL_memcpy(gnftl.pCacheWriteBuf_ali, pdata, gnftl.pagesize);
		//nftl_mutex_unlock();
		return SUCCESS;
	}
	if(gnftl.WriteCachePageaddr!=NFTL_NO_PAGE)
	{
		rc = NFTL_WritePage(gnftl.WriteCachePageaddr, gnftl.pCacheWriteBuf_ali);
	}

	NFTL_memcpy(gnftl.pCacheWriteBuf_ali, pdata, gnftl.pagesize);
	gnftl.WriteCachePageaddr = logpage;
#else
	NFTL_WritePage(logpage, pdata);
	//NFTL_SwitchBuf();
#endif
	
	//nftl_mutex_unlock();
	
	return rc;//SUCCESS;
}

unsigned int NFTL_CacheFlush(void)
{
	unsigned int rc = SUCCESS;
	//printk("NFTL_CacheFlush\n");
#if __NFTL_BUFFER_ON__
	nftl_mutex_lock();
	if(gnftl.write_count)		// it is the flush mechnism to sync the data
	{
		//printk("gnftl.write_count:%d   gnftl.write_count_bak=%d\n", gnftl.write_count, gnftl.write_count_bak);
		if(gnftl.write_count == gnftl.write_count_bak)
		{			
			if(gnftl.WriteCachePageaddr!=NFTL_NO_PAGE)
			{
				printk("NFTL_CacheFlush(%d)\n", gnftl.WriteCachePageaddr);
				rc = NFTL_WritePage(gnftl.WriteCachePageaddr, gnftl.pCacheWriteBuf_ali);		
				gnftl.WriteCachePageaddr = NFTL_NO_PAGE;	
			}
			gnftl.write_count = 0;
			gnftl.write_count_bak = 0;
		}
		else
		{
			gnftl.write_count_bak = gnftl.write_count;	
		}

	}
	nftl_mutex_unlock();
#endif
	return rc;//SUCCESS;
}

unsigned int NFTL_CacheFlush_ex(void)
{
	unsigned int rc = SUCCESS;
	//printk("NFTL_CacheFlush\n");
#if __NFTL_BUFFER_ON__
	nftl_mutex_lock();
	if(gnftl.write_count)
	{
		if(gnftl.WriteCachePageaddr!=NFTL_NO_PAGE)
		{
			//printk("NFTL_CacheFlush_ex(%d)\n", gnftl.WriteCachePageaddr);
			rc = NFTL_WritePage(gnftl.WriteCachePageaddr, gnftl.pCacheWriteBuf_ali);		
			gnftl.WriteCachePageaddr = NFTL_NO_PAGE; 
			gnftl.write_count = 0;
		}
	}
	nftl_mutex_unlock();
#endif
	return rc;//SUCCESS;
}


// nand driver read entry point
unsigned int NFTL_Read(unsigned int StartSector, unsigned int SectorNum, unsigned char  *pdata)
{
	//unsigned int rc = SUCCESS;
	unsigned int sector_per_page = gnftl.sector_per_page;
	unsigned int sect_not_align = StartSector & (sector_per_page - 1);
	unsigned int startAddr = StartSector >> (gnftl.sector_per_page_shift);
	unsigned int readSize = 0;

//printk("KING->NFTL_Read(%d, %d, 0x%x).\n", StartSector, SectorNum, pdata);
	//printk("33333\n");

	nftl_mutex_lock();
	if (sect_not_align)
	{
		readSize = (sector_per_page - sect_not_align);

		if (SectorNum < readSize)
		{
			readSize = SectorNum;
		}
		
		//NFTL_ReadPageByCache(startAddr, gnftl.pReadBuf_ali);
		if(NFTL_ReadPageByCache(startAddr, gnftl.pReadBuf_ali)==FAILURE)
		{
			printk("KING->NFTL_Read(%d, %d, 0x%x).\n", StartSector, SectorNum, (unsigned int)pdata);
			printk("1NFTL_ReadPageByCache(%d )\n", startAddr);
		}
		NFTL_memcpy(pdata, &gnftl.pReadBuf_ali[sect_not_align<<SECTOR_SIZE_SHIFT], readSize<<SECTOR_SIZE_SHIFT);
		
		StartSector += readSize;
		SectorNum -= readSize;
		pdata +=  (readSize<< SECTOR_SIZE_SHIFT);
		startAddr++ ;
	}

	while (SectorNum >= sector_per_page)
	{
		//NFTL_ReadPageByCache(startAddr, pdata);
		if(NFTL_ReadPageByCache(startAddr, pdata)==FAILURE)
		{
			printk("KING->NFTL_Read(%d, %d, 0x%x).\n", StartSector, SectorNum, (unsigned int)pdata);
			//printk("2NFTL_Read(%d, %d, 0x%x)\n", StartSector, SectorNum, pdata);
			printk("2NFTL_ReadPageByCache(%d )\n", startAddr);
		}

		startAddr ++;
		SectorNum -= sector_per_page;
		pdata += gnftl.pagesize;
	}

	if (SectorNum)	
	{	
		//ret = NFTL_ReadPageByCache(startAddr, gnftl.pReadBuf_ali);
		if(NFTL_ReadPageByCache(startAddr, gnftl.pReadBuf_ali)==FAILURE)
		{
			printk("KING->NFTL_Read(%d, %d, 0x%x).\n", StartSector, SectorNum, (unsigned int)pdata);
			//printk("3NFTL_Read(%d, %d, 0x%x)\n", StartSector, SectorNum, pdata);
			printk("3NFTL_ReadPageByCache(%d )\n", startAddr);
		}	
	
		NFTL_memcpy(pdata, gnftl.pReadBuf_ali, SectorNum<<SECTOR_SIZE_SHIFT);
	}

	nftl_mutex_unlock();
	return SUCCESS;
}


// nand driver wirte entry point
unsigned int NFTL_Write(unsigned int StartSector, unsigned int SectorNum, unsigned char  *pdata)
{
	unsigned int sector_per_page = gnftl.sector_per_page;
	unsigned int sect_not_align = StartSector & (sector_per_page - 1);
	unsigned int startAddr = StartSector >> (gnftl.sector_per_page_shift);
	unsigned int writeSize = 0;

	nftl_mutex_lock();
	if (sect_not_align)		// not align, read first, fill data and write back
	{
		unsigned int IsWrite2Cache = 0;
		if(NFTL_ReadPageByCache(startAddr, gnftl.pReadBuf_ali)==FAILURE)
		{
			printk("1NFTL_Write(%d, %d, 0x%x) is fail.\n", StartSector, SectorNum, (unsigned int)pdata);
			printk("1NFTL_ReadPageByCache(%d) is fail.\n", startAddr);
		}

		writeSize = (sector_per_page - sect_not_align);
		if (SectorNum < writeSize)
		{
			writeSize = SectorNum;
			IsWrite2Cache = 1;				
		}
		NFTL_memcpy(&gnftl.pReadBuf_ali[sect_not_align<<SECTOR_SIZE_SHIFT], pdata, writeSize<<SECTOR_SIZE_SHIFT);


		if(NFTL_WritePageByCache(startAddr, gnftl.pReadBuf_ali)==FAILURE)
		{
			printk("2NFTL_Write(%d, %d, 0x%x) is fail.\n", StartSector, SectorNum, (unsigned int)pdata);
			printk("2NFTL_ReadPageByCache(%d) is fail.\n", startAddr);
		}

		SectorNum -= writeSize;
		pdata +=  (writeSize<< SECTOR_SIZE_SHIFT);
		startAddr++;
	}
	
	while (SectorNum >= sector_per_page)	// keep writing pages
	{
		if(NFTL_WritePageByCache(startAddr, pdata)==FAILURE)
		{
			printk("3NFTL_Write(%d, %d, 0x%x) is fail.\n", StartSector, SectorNum, (unsigned int)pdata);
			printk("3NFTL_WritePage(%d) is fail.\n", startAddr);
		}

		startAddr++;
		SectorNum -= sector_per_page;
		pdata += gnftl.pagesize;	
	}

	if (SectorNum)	
	{	
		//ret = NFTL_ReadPageByCache(startAddr, gnftl.pReadBuf_ali);
		if(NFTL_ReadPageByCache(startAddr, gnftl.pReadBuf_ali)==FAILURE)
		{
			printk("4NFTL_Write(%d, %d, 0x%x) is fail.\n", StartSector, SectorNum, (unsigned int)pdata);
			printk("4NFTL_ReadPageByCache(%d) is fail.\n", startAddr);
		}

		NFTL_memcpy(gnftl.pReadBuf_ali, pdata, SectorNum<<SECTOR_SIZE_SHIFT);
		
		//ret = NFTL_WritePageByCache(startAddr, gnftl.pReadBuf_ali);
		if(NFTL_WritePageByCache(startAddr, gnftl.pReadBuf_ali)==FAILURE)
		{
			printk("5NFTL_Write(%d, %d, 0x%x) is fail.\n", StartSector, SectorNum, (unsigned int)pdata);
			printk("5NFTL_WritePageByCache(%d) is fail.\n", startAddr);
		}
		//printk("<<<NFTL_WritePageByCache(%d, %d, 0x%x)-->%d.\n", StartSector, SectorNum, pdata, startAddr);

	}
	nftl_mutex_unlock();
	return SUCCESS;
}

