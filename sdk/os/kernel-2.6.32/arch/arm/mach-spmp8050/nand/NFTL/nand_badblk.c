#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "nand_badblk.h"
#include "../hal/hal_base.h"
#include "../hal/nf_s330.h"

BadBlk_Info_t g_badblkInfo;

unsigned int badblkInfo_init(unsigned short size)
{
	g_badblkInfo.pattern = BADBLK_INFO_PATTERN;
	g_badblkInfo.size = size;

	printk(" malloc g_badblkInfo.ptr(%d)\n",size);
	g_badblkInfo.ptr = (unsigned char*)memAlloc(size);
	
	if(g_badblkInfo.ptr == NULL)
	{
		printk("g_badblkInfo.ptr malloc is failure!\n");
		return FAILURE;	
	}
	memset(g_badblkInfo.ptr, 0, size);
	return SUCCESS;
}

void badblkInfo_free(unsigned short size)
{
	if(g_badblkInfo.ptr)
	{
		memFree(g_badblkInfo.ptr);
		g_badblkInfo.ptr = NULL;	
	}
}
unsigned int badblkInfo_set(unsigned short blk)
{
	unsigned short i,j;

	i = (blk>>3);
	j = (blk&7);

	g_badblkInfo.ptr[i] |= 1<<j;
	return SUCCESS;
}
unsigned int badblkInfo_isbad(unsigned short blk)
{
	unsigned short i,j;

	i = (blk>>3);
	j = (blk&7);

	if(g_badblkInfo.ptr[i]&(1<<j))
		return SUCCESS;
	else
		return FAILURE;	
}

unsigned int badblkInfo_save(Nand_Info_t *pnandInfo, unsigned short blk)
{
	unsigned char* ptr = g_badblkInfo.ptr;
	unsigned int page = 0;//(unsigned int)(blk << pnandInfo->page_per_blk_shift);
	//unsigned int page_count = (size + pnand->p_nand_info->pageSize - 1)/pnand->p_nand_info->pageSize;
	unsigned int size = g_badblkInfo.size;
	
	memcpy(pnandInfo->pWriteBuf_ali, (unsigned char*)&g_badblkInfo, sizeof(BadBlk_Info_Data_t));
	cache_sync();
	//nand_write_page(page, pnand->ppageBuf, pnand->predund);	
	Nand_Write_PhyPage(blk, page, pnandInfo->pWriteBuf_ali, pnandInfo->pEccBuf_ali);
	page++;	
	
	while(size)
	{		
		if(size > pnandInfo->pagesize)
		{
			memcpy(pnandInfo->pWriteBuf_ali, ptr, pnandInfo->pagesize);
			size -= pnandInfo->pagesize;
		}
		else
		{
			memcpy(pnandInfo->pWriteBuf_ali, ptr, size);
			size = 0;			
		}
		cache_sync();
		//nand_write_page(page, pnand->ppageBuf, pnand->predund);			
		Nand_Write_PhyPage(blk, page, pnandInfo->pWriteBuf_ali, pnandInfo->pEccBuf_ali);
		ptr += pnandInfo->pagesize;
		page++;
	}		

	return SUCCESS;
}

unsigned int badblkInfo_load(Nand_Info_t *pnandInfo, unsigned short blk)
{
	unsigned char* ptr = g_badblkInfo.ptr;
	unsigned int page = 0;
	unsigned int size;
	
	//nand_read_page(page, pnandInfo->pWriteBuf_ali, pnand->predund);	
	Nand_Read_PhyPage(blk, page, pnandInfo->pReadBuf_ali, pnandInfo->pEccBuf_ali);
	memcpy(&g_badblkInfo, Cache2NonCacheAddr(pnandInfo->pReadBuf_ali), sizeof(BadBlk_Info_Data_t));	
	page++;
	size = g_badblkInfo.size;
	if(g_badblkInfo.pattern==BADBLK_INFO_PATTERN)
	{
		while(size)
		{		

			//nand_read_page(page, pnand->ppageBuf, pnand->predund);
			Nand_Read_PhyPage(blk, page, pnandInfo->pReadBuf_ali, pnandInfo->pEccBuf_ali);
			if(size>pnandInfo->pagesize)	
			{
				memcpy(ptr, Cache2NonCacheAddr(pnandInfo->pReadBuf_ali), pnandInfo->pagesize);	
				size -= pnandInfo->pagesize;		
			}
			else
			{
				memcpy(ptr, Cache2NonCacheAddr(pnandInfo->pReadBuf_ali), size);	
				size = 0;
			}
			ptr += pnandInfo->pagesize;
			page++;		
		}
		return SUCCESS;
	}

	return FAILURE;
}
