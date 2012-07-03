#ifdef	_WIN32
//#include "stdafx.h"
#include <stdlib.h>   // For _MAX_PATH definition
#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <errno.h>

#define		NAND_SIM

#include "../../NandSim/nand_m210.h"
#include "../include/nand_cfg.h"
#include "../include/fifo.h"
#include "../include/nand.h"

#else
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "../hal/nf_s330.h"
#include "../hal/hal_base.h"
#include "../champ/nand_FDisk.h"
#include "../champ/NF_cfgs.h"

#include "nand.h"
#include "nand_badblk.h"
#include "nftl.h"
#include "nand_pgbase.h"

#endif//#ifdef	_WIN32

#include "../hal/bch_s336.h"

unsigned int g_nand_IsfirstInit = 0;

Nand_Info_t g_nand_info;

Nand_LogInfo_t g_nli;

extern unsigned int debug_flag ;

////////////////////////////////////////////////////////////////////////////////


void Nandcache_sync(void)
{
#ifndef	_WIN32
	#if NAND_CACHE_ON_
		cache_sync();	
	#endif		
#endif		
}
unsigned char* NandCache2NonCacheAddr(unsigned char* addr)
{
#ifdef	_WIN32
	return addr;
#else	
	return (unsigned char *)GET_NON_CACHED_ADDR_PTR(addr);
#endif		
}



void Nand_print_buf(unsigned char*pBuffer, int len)
{
	unsigned int i;
	unsigned char* buf_NC;
	
	//buf_NC = NandCache2NonCacheAddr(pBuffer);
	
	buf_NC = pBuffer;
	
	printk("buf_NC:0x%8x\n",(unsigned int)buf_NC);

	for(i=0;i<len;i++)
	{
		if(i%16==0)
		{
			printk("\n0x:%8x | ",i);	
		}
		else if(i%16==8)
		{
			printk("- ");
		}
			
		printk("%2x ",buf_NC[i]);		
	}
	printk("\n");	
}

#ifdef	_WIN32
unsigned char g_str[8192];
void PrintLog(unsigned char* str)
{
#if 0
	FILE* fp = fopen("d:\\logxx.log", "a");
	
	fprintk(fp, "%s\n",str);
	
	fclose(fp);	
#endif
}
#endif

void Nand_SetNli(unsigned char* pBuffer, Nand_LogInfo_t nli)
{
	if(getEccMode()==BCH_S336_24_BIT)
	{
		Nand_Sector_24bit_Info_t *ptr;
		
		ptr = (Nand_Sector_24bit_Info_t *)NandCache2NonCacheAddr(pBuffer); 
		
		ptr->logblk = nli.logblk;
		ptr->ver = nli.ver;
		ptr->pattern = NAND_SECTORINFO_PATTERN;

	}
	else
	{
		Nand_Sector_Info_t *ptr;
		
		ptr = (Nand_Sector_Info_t *)NandCache2NonCacheAddr(pBuffer); 
		
		ptr->logblk = nli.logblk;
		ptr->ver = nli.ver;
		ptr->pattern = NAND_SECTORINFO_PATTERN;
	}
	//ptr->sector2.logblk = nli.logblk;
	//ptr->sector2.ver = nli.ver;

	
}
unsigned int Nand_GetNli(unsigned char* pBuffer, Nand_LogInfo_t *pnli)
{
	
	if(getEccMode()==BCH_S336_24_BIT)
	{
		Nand_Sector_24bit_Info_t *ptr;
		ptr = (Nand_Sector_24bit_Info_t *)NandCache2NonCacheAddr(pBuffer); 
		
		//pnli->logblk = NO_BLOCK;
		//pnli->ver = 0;
		
		pnli->logblk = ptr->logblk;
		pnli->ver = ptr->ver;
		pnli->pattern = ptr->pattern;

	}
	else
	{
		Nand_Sector_Info_t *ptr;
		ptr = (Nand_Sector_Info_t *)NandCache2NonCacheAddr(pBuffer); 
		
		//pnli->logblk = NO_BLOCK;
		//pnli->ver = 0;
		
		pnli->logblk = ptr->logblk;
		pnli->ver = ptr->ver;
		pnli->pattern = ptr->pattern;
	}
	//if(pnli->logblk==NO_BLOCK || pnli->ver==0)	
	if((pnli->logblk==NO_BLOCK) || (pnli->pattern !=NAND_SECTORINFO_PATTERN))
	{
		return FAILURE; 
	}		

	return SUCCESS;	
}

void Nand_parselogblk(unsigned short logblk, unsigned char *pzone, unsigned short *pblkoffset)
{
	Nand_Info_t *pnandInfo = &g_nand_info;
	
	*pzone = (unsigned char)(logblk / pnandInfo->nandtab[0].log2phy_count);
	
	*pblkoffset = logblk - (*pzone * pnandInfo->nandtab[0].log2phy_count);
}

void Nand_PrintInfo(Nand_Info_t *pnandInfo)
{
	printk("###############################################################\n");
	printk("pnandInfo->logblock_count:%d\n", pnandInfo->logblock_count);
	printk("pnandInfo->page_per_block:%d\n", pnandInfo->page_per_block);
	printk("pnandInfo->pagesize:%d\n", pnandInfo->pagesize);
	printk("pnandInfo->reduntlen:%d\n", pnandInfo->reduntlen);
	printk("pnandInfo->sector_per_page:%d\n", pnandInfo->sector_per_page);
	printk("pnandInfo->total_blockcount:%d\n", pnandInfo->total_blockcount);
	printk("pnandInfo->diskstartblk:%d\n", pnandInfo->diskstartblk);
	printk("pnandInfo->phyblock_count:%d\n", pnandInfo->phyblock_count);
	printk("pnandInfo->zone_count:%d\n", pnandInfo->zone_count);
	printk("###############################################################\n");
}
unsigned int Nand_InfoGet(Nand_Info_t *pnandInfo, psysinfo_t *psysInfo)
{
	pnandInfo->diskstartblk = get_DiskstartBlk();

	pnandInfo->total_blockcount = psysInfo->u16TotalBlkNo << psysInfo->u8Internal_Chip_Number;

	pnandInfo->phyblock_count = pnandInfo->total_blockcount - pnandInfo->diskstartblk;

	pnandInfo->page_per_block = psysInfo->u16PageNoPerBlk;
	pnandInfo->pagesize = psysInfo->u16PyldLen <<(psysInfo->u8Support_TwoPlan);
	pnandInfo->sector_per_page = (pnandInfo->pagesize >> SECTOR_SIZE_SHIFT); // (pagesize/512)
	
	pnandInfo->reduntlen = pnandInfo->pagesize>>4;//if support 24bit/1024byte

	pnandInfo->page_per_blk_shift = valshift(pnandInfo->page_per_block);

	pnandInfo->logblock_count =(unsigned short)((unsigned int) (pnandInfo->phyblock_count *BANK_BLK)/MAX_PHY_BLKS_PER_ZONE ); 	
	//pnandInfo->logblock_count -= MAX_FRAGMENT_PER_ZONE;

	pnandInfo->zone_count = (unsigned char)((unsigned int) (pnandInfo->phyblock_count + MAX_PHY_BLKS_PER_ZONE -1 )/MAX_PHY_BLKS_PER_ZONE );
	
	if(pnandInfo->zone_count > MAX_ZONE_COUNT)
	{
		printk("ERROE : pnandInfo->zone_count:%u > MAX_ZONE_COUNT(%u)\n", pnandInfo->zone_count, MAX_ZONE_COUNT);
		return FAILURE;
	}
	
	Nand_PrintInfo(pnandInfo);

	return SUCCESS;
}

unsigned int nand_check_is_valid(Nand_Info_t *pnandInfo)
{

	unsigned short idx;
	unsigned short page;
	unsigned int rc = FAILURE;
	unsigned int pattern = CHECK_NAND_PATTERN_IS_VALID;
	
	unsigned int *ptr = (unsigned int*)pnandInfo->pReadBuf_ali;
	unsigned int *p_nc_long_value = 0;
	
	badblkInfo_init((pnandInfo->phyblock_count>>3) +1);
	
	ptr += CHECK_NAND_PATTERN_OFFSET;
	
	p_nc_long_value = (unsigned int*)NandCache2NonCacheAddr((unsigned char*)ptr);

	for(idx=0 ; idx<NAND_PATTERN_COUNT; idx++)
	{
		page = pnandInfo->page_per_block-1;
		//debug_flag =1;
		if(Nand_Read_PhyPage(idx, page, pnandInfo->pReadBuf_ali, pnandInfo->pEccBuf_ali)==SUCCESS)
		{	
			if(*p_nc_long_value == pattern)
			{
				badblkInfo_load(pnandInfo, idx);				
				rc = SUCCESS;
				break;
			}
		}
	}
	
	if(rc==SUCCESS)
	{
		for(idx=0; idx<NAND_BADBLKTABSAVE_COUNT; idx++)
		{
			if(badblkInfo_load(pnandInfo, idx+NAND_BADBLKTABSAVE_START)==SUCCESS)
			{
				break;
			}
		}
	}	
	return rc;
}
unsigned int nand_set_valid(Nand_Info_t *pnandInfo)
{
	unsigned short idx;
	unsigned short page;
	unsigned int* ptr;
		
	for(idx=0 ; idx<NAND_PATTERN_COUNT; idx++)
	{
		
		nand_eraseblk(pnandInfo, idx);
		//save bad blk table to page 0,..... 
		badblkInfo_save(pnandInfo, idx);
		
		//save vaild flag to last page  
		ptr = (unsigned int *)(pnandInfo->pWriteBuf_ali);

		memset(pnandInfo->pWriteBuf_ali, 0, pnandInfo->pagesize);
		memset(pnandInfo->pEccBuf_ali, 0, pnandInfo->reduntlen);

		ptr += CHECK_NAND_PATTERN_OFFSET;
		
		*ptr = CHECK_NAND_PATTERN_IS_VALID;
		
		Nandcache_sync();

		page = pnandInfo->page_per_block-1;
		Nand_Write_PhyPage(idx, page, pnandInfo->pWriteBuf_ali, pnandInfo->pEccBuf_ali);	
	}

	return SUCCESS;
}
SINT32 nand_set_invalid(void)
{
	unsigned short idx;
	unsigned short page;
	unsigned int* ptr;
	Nand_Info_t *pnandInfo = &g_nand_info;
		
	for(idx=0 ; idx<NAND_PATTERN_COUNT; idx++)
	{
		
		nand_eraseblk(pnandInfo, idx);
		//save bad blk table to page 0,..... 
		badblkInfo_save(pnandInfo, idx);
		
		//save vaild flag to last page  
		ptr = (unsigned int *)(pnandInfo->pWriteBuf_ali);

		memset(pnandInfo->pWriteBuf_ali, 0, pnandInfo->pagesize);
		memset(pnandInfo->pEccBuf_ali, 0, pnandInfo->reduntlen);

		ptr += CHECK_NAND_PATTERN_OFFSET;
		
		*ptr = 0;
		
		Nandcache_sync();

		page = pnandInfo->page_per_block-1;
		Nand_Write_PhyPage(idx, page, pnandInfo->pWriteBuf_ali, pnandInfo->pEccBuf_ali);	
	}

	return SUCCESS;
}
unsigned int nand_buf_init(Nand_Info_t *pnandInfo)
{
	int i, j, size;
	unsigned short phyblock_count;

	phyblock_count = pnandInfo->phyblock_count;

	for(i=0; i< pnandInfo->zone_count; i++)
	{
		FIFO_Init(&pnandInfo->nandtab[i].fifo, MAX_PHY_BLKS_PER_ZONE);	

		pnandInfo->nandtab[i].log2phy_count = BANK_BLK;
		pnandInfo->nandtab[i].fragment_tab_count = MAX_FRAGMENT_PER_ZONE;
		
		size = pnandInfo->nandtab[i].log2phy_count*sizeof(Nand_Log2Phy_t);
		pnandInfo->nandtab[i].plog2phy = (Nand_Log2Phy_t*)memAlloc(size);
		printk("pnandInfo->nandtab[%d].plog2phy malloc(%d)\n", i, size); 
		if(pnandInfo->nandtab[i].plog2phy==NULL)
		{
			printk("g_nand_info.plog2phytab[%d].plog2phy malloc(%d) is failure!\n", i, size);
			return FAILURE;			
		}
		
		//memset(pnandInfo->nandtab[i].plog2phy, 0 , size);
		size = pnandInfo->nandtab[i].log2phy_count;
		for(j=0; j<size; j++)
		{
			pnandInfo->nandtab[i].plog2phy[j].phyblk = NO_BLOCK;
			pnandInfo->nandtab[i].plog2phy[j].ver = 0;
		}
		
		size = pnandInfo->nandtab[i].fragment_tab_count*sizeof(Nand_fragment_tab_t);
		pnandInfo->nandtab[i].pfragment_tab = (Nand_fragment_tab_t*)memAlloc(size);
		printk("pnandInfo->nandtab[%d].pfragment_tab malloc(%d)\n", i, size);	
		if(pnandInfo->nandtab[i].pfragment_tab==NULL)
		{
			printk("g_nand_info.plog2phytab[i].pfragment_tab malloc is failure!\n");
			return FAILURE;			
		}			
		//memset(pnandInfo->nandtab[i].pfragment_tab, 0 , size);
		size = pnandInfo->nandtab[i].fragment_tab_count;
		for(j=0; j<size; j++)
		{
			pnandInfo->nandtab[i].pfragment_tab[j].logblk = NO_BLOCK;
			pnandInfo->nandtab[i].pfragment_tab[j].phyblk_old = NO_BLOCK;
			pnandInfo->nandtab[i].pfragment_tab[j].phyblk_new = NO_BLOCK;
			pnandInfo->nandtab[i].pfragment_tab[j].page_idx = 0;
			pnandInfo->nandtab[i].pfragment_tab[j].ver = 0;
		}			
	}
	
	size = pnandInfo->pagesize + ALIGNED64;
	
	pnandInfo->pReadBuf = memAlloc(size);
	printk("pnandInfo->pReadBuf malloc(%d)\n", size);
	if(pnandInfo->pReadBuf==NULL)
	{
		printk("pnandInfo->pReadBuf malloc(%d) is failure!\n", size);
		return FAILURE;			
	}		
	pnandInfo->pReadBuf_ali =(unsigned char*)(((unsigned int)pnandInfo->pReadBuf+ALIGNED64-1)&(0xFFFFFFC0));

	pnandInfo->pWriteBuf = memAlloc(size);
	printk("pnandInfo->pWriteBuf malloc(%d)\n", size);	
	if(pnandInfo->pWriteBuf==NULL)
	{
		printk("pnandInfo->pWriteBuf malloc(%d) is failure!\n", size);
		return FAILURE;			
	}		
	pnandInfo->pWriteBuf_ali =(unsigned char*)(((unsigned int)pnandInfo->pWriteBuf+ALIGNED64-1)&(0xFFFFFFC0));

	size = pnandInfo->reduntlen + ALIGNED64;
	pnandInfo->pEccBuf = memAlloc(size);
	printk("pnandInfo->pEccBuf malloc(%d)\n", size);		
	if(pnandInfo->pEccBuf==NULL)
	{
		printk("pnandInfo->pEccBuf malloc(%d) is failure!\n", size);
		return FAILURE;			
	}		
	pnandInfo->pEccBuf_ali =(unsigned char*)(((unsigned int)pnandInfo->pEccBuf+ALIGNED64-1)&(0xFFFFFFC0));
	
	return SUCCESS;	
}

unsigned int nand_eraseblk(Nand_Info_t *pnandInfo, unsigned short blk)
{
	unsigned short phyblk = pnandInfo->diskstartblk + blk;

#ifdef _WIN32
	NAND_EraseOneBlock(phyblk);
#else
	EraseBlock(phyblk);
#endif
	
	return SUCCESS;	
	
}

unsigned int nand_test_block(Nand_Info_t *pnandInfo, unsigned short blk)
{
//	int i, j, rc=SUCCESS;
#if 0
	DTCM_Enable((UINT32)0x2e000000);
	int offset = pnandInfo->pagesize;
	unsigned char* preadbuf = (unsigned char*)0x9d800000;
	unsigned char* pwritebuf = (unsigned char*)(preadbuf + offset);
	unsigned char* peccbuf = (unsigned char*)pnandInfo->pEccBuf_ali;//(unsigned char*)(pwritebuf + offset);
	
	unsigned char* preadbuf_tcm = (unsigned char*)0x2e000000;
	unsigned char* pwritebuf_tcm = (unsigned char*)(preadbuf_tcm + offset);
#else

	unsigned char* preadbuf = (unsigned char*)pnandInfo->pReadBuf_ali;
	unsigned char* pwritebuf = (unsigned char*)pnandInfo->pWriteBuf_ali;
	unsigned char* peccbuf = (unsigned char*)pnandInfo->pEccBuf_ali;//(unsigned char*)(pwritebuf + offset);

	unsigned char* preadbuf_tcm = (unsigned char*)Cache2NonCacheAddr(preadbuf);
	unsigned char* pwritebuf_tcm = (unsigned char*)Cache2NonCacheAddr(pwritebuf);

#endif

//if(Is_Exclusion_block_no(blk)==SUCCESS)
	//return FAILURE;	


	memset(preadbuf_tcm, 0xa5, pnandInfo->pagesize);	
	memset(pwritebuf_tcm, 0x5a, pnandInfo->pagesize);	
		
	if(nand_eraseblk(pnandInfo, blk)==FAILURE)
	{
		printk("[nand_test_block] erase error: %d!\n", blk);
		return FAILURE;	
	}
	else if(Nand_Read_PhyPage(blk, pnandInfo->page_per_block-1, preadbuf, peccbuf)==FAILURE)
	{
		printk("[nand_test_block] read error: %d!\n", blk);
		return FAILURE;
	}
	else if(Nand_Write_PhyPage(blk, pnandInfo->page_per_block-1, pwritebuf, peccbuf)==FAILURE)
	{
		printk("[nand_test_block] write error: %d!\n", blk);
		return FAILURE;		
	}else if(Nand_Read_PhyPage(blk, pnandInfo->page_per_block-1, preadbuf, peccbuf)==FAILURE)
	{
		printk("[nand_test_block] 1read error: %d!\n", blk);
		printk("pnandInfo->pagesize : %d!\n", pnandInfo->pagesize);
		//Nand_print_buf(preadbuf, pnandInfo->pagesize);

		return FAILURE;		
 	}else if(NFTL_memcmp(pwritebuf_tcm, preadbuf_tcm, pnandInfo->pagesize) != 0)
	{
		printk("[nand_test_block] memcmp error: %d!\n", blk);
		return FAILURE;				
	}
	else if(nand_eraseblk(pnandInfo, blk)==FAILURE)
	{
		printk("[nand_test_block] 1erase error: %d!\n", blk);
		return FAILURE;	
	}

	return SUCCESS;	
}

unsigned int nand_erasetest(Nand_Info_t *pnandInfo, unsigned short blk)
{
	//Insert blk to FIFO	
	if(nand_test_block(pnandInfo, blk)==SUCCESS)
	{		
		unsigned char zone_count = blk / MAX_PHY_BLKS_PER_ZONE;
		FIFO_push(&pnandInfo->nandtab[zone_count].fifo , blk);
		return SUCCESS;
	}

	badblkInfo_set(blk);

	printk("Warning : the block(%d) is bad!\n\n",blk);	
	return 	FAILURE;
}

unsigned int nand_erase_insert2fifo(Nand_Info_t *pnandInfo, unsigned short blk)
{

	if(nand_eraseblk(pnandInfo, blk)==SUCCESS)
	{
		unsigned char zone_count = blk / MAX_PHY_BLKS_PER_ZONE;
		FIFO_push(&pnandInfo->nandtab[zone_count].fifo , blk);
		return SUCCESS;
	}
	
	badblkInfo_set(blk);
	
	printk("Warning : The block(%d) is bad!\n\n",blk);		
	return 	FAILURE;
}

unsigned int nand_eraseall(Nand_Info_t *pnandInfo)
{
	unsigned short blk;
	printk("\n");

	for(blk=NANE_RESERVED_COUNT; blk<pnandInfo->phyblock_count; blk++)
	{
		if((blk&0x3f)==0x3f)	
			printk("@");

		//nand_erase_insert2fifo(pnandInfo, blk);			
		nand_erasetest(pnandInfo, blk);
	}	
	printk("\n");

	return SUCCESS;
}

unsigned int nand_WriteLogBlk(Nand_Info_t *pnandInfo, unsigned short logblk)
{
	unsigned char zone;
	unsigned short phyblk, blkoffset;

	Nand_parselogblk(logblk, &zone, &blkoffset);
	
	if(FIFO_pop(&pnandInfo->nandtab[zone].fifo, &phyblk)==SUCCESS)
	{
		g_nli.ver = 1;	
		g_nli.logblk = logblk;
		
		//printk("nand_WriteLogBlk(%d, %d)\n", logblk, phyblk);

		pnandInfo->nandtab[zone].plog2phy[blkoffset].phyblk = phyblk;
		pnandInfo->nandtab[zone].plog2phy[blkoffset].ver = 1;

		Nand_Write_PhyPage(phyblk, 0, pnandInfo->pWriteBuf_ali, pnandInfo->pEccBuf_ali);
		Nand_Write_PhyPage(phyblk, pnandInfo->page_per_block-1, pnandInfo->pWriteBuf_ali, pnandInfo->pEccBuf_ali);		
		
		return SUCCESS;
	}
	return FAILURE;
}


// write the phyical block with a default logical block value
unsigned int nand_WriteAllLogBlk(Nand_Info_t *pnandInfo)
{
	unsigned short logblk;
	
	printk("nand_WriteAllLogBlk\n");
	
	for(logblk=0; logblk<pnandInfo->logblock_count; logblk++)
	{
		if((logblk&0x3f)==0x3f)	
			printk("@");
		if(nand_WriteLogBlk(pnandInfo, logblk)==FAILURE)
		{
			printk("ERROR : nand_WriteLogBlk(%d) is fail!\n\n",logblk);
			return FAILURE;	
		}
	}	
	return SUCCESS;	
}


unsigned char Nand_get_CorrectVer(unsigned char ver1, unsigned char ver2)
{
	if((ver1+1)==ver2)
	{
		return ver2;
	}
	else if((ver2+1)==ver1)
	{
		return ver1;	
	}

	printk("ERROR : Nand_get_CorrectVer(%d ,%d)\n", ver1, ver2);

	return (ver1>ver2) ? ver1 : ver2;
}



unsigned short Nand_get_page_idx(Nand_Info_t *pnandInfo, unsigned short blk)
{
	unsigned short page;
	Nand_LogInfo_t* pnli;
	
	for(page=0; page<pnandInfo->page_per_block; page++)
	{
		pnli = Nand_Read_PhyPageWithNli(blk, page, pnandInfo->pReadBuf_ali, pnandInfo->pEccBuf_ali);
		
		if(pnli==NULL)
		{
			break;
		}	
		
	}
	return page;
}

void Nand_PrintInfo2File(unsigned char* file)
{
#ifdef	_WIN32
	int i, j;
	Nand_Info_t *pnandInfo = &g_nand_info;
	
	FILE *fp = fopen(file, "w");
	
	for(i=0; i< pnandInfo->zone_count; i++)
	{
		fprintf(fp, "pnandInfo->nandtab[%d].fragment_tab_count:%d\n", i, pnandInfo->nandtab[i].fragment_tab_count);
		for(j=0; j< pnandInfo->nandtab[i].fragment_tab_count; j++)
		{
			fprintf(fp,"pnandInfo->nandtab[%d].pfragment_tab[%d].logblk:%d\n",i ,j ,pnandInfo->nandtab[i].pfragment_tab[j].logblk);
			fprintf(fp,"pnandInfo->nandtab[%d].pfragment_tab[%d].phyblk_old:%d\n",i ,j ,pnandInfo->nandtab[i].pfragment_tab[j].phyblk_old);
			fprintf(fp,"pnandInfo->nandtab[%d].pfragment_tab[%d].phyblk_new:%d\n",i ,j ,pnandInfo->nandtab[i].pfragment_tab[j].phyblk_new);
			fprintf(fp,"pnandInfo->nandtab[%d].pfragment_tab[%d].page_idx:%d\n",i ,j ,pnandInfo->nandtab[i].pfragment_tab[j].page_idx);
			fprintf(fp,"pnandInfo->nandtab[%d].pfragment_tab[%d].ver:%d\n",i ,j ,pnandInfo->nandtab[i].pfragment_tab[j].ver);
		}


		fprintf(fp, "pnandInfo->nandtab[%d].log2phy_count:%d\n", i, pnandInfo->nandtab[i].log2phy_count);
		
		for(j=0; j< pnandInfo->nandtab[i].log2phy_count; j++)
		{
			fprintf(fp,"pnandInfo->nandtab[%d].plog2phy[%d].phyblk:%d\n",i ,j ,pnandInfo->nandtab[i].plog2phy[j].phyblk);			
			fprintf(fp,"pnandInfo->nandtab[%d].plog2phy[%d].ver:%d\n",i ,j ,pnandInfo->nandtab[i].plog2phy[j].ver);
		}		
	}
	
	fclose(fp);
#endif	
}

void Nand_print_fifo(Nand_Info_t *pnandInfo)
{
	int zone;

	for(zone=0; zone< pnandInfo->zone_count; zone++)
	{
		FIFO_Get_Info(&pnandInfo->nandtab[zone].fifo);
	}

}

void Nand_checklogtab(Nand_Info_t *pnandInfo)
{
	unsigned short blk, zone, blkoffset;
	unsigned short phyblk;

	printk("pnandInfo->logblock_count:%d\n", pnandInfo->logblock_count);
	
	for(blk=0; blk<pnandInfo->logblock_count; blk++)
	{

		Nand_parselogblk(blk,(unsigned char *)&zone, &blkoffset);
		phyblk = pnandInfo->nandtab[zone].plog2phy[blkoffset].phyblk;

		printk("pnandInfo->nandtab[%d].plog2phy[%d]:%d\n", zone, blkoffset, phyblk);

		if(phyblk==NO_BLOCK)
		{
			printk("ERROR : pnandInfo->nandtab[%d].plog2phy[%d]:%d\n", zone, blkoffset, phyblk);
			nand_WriteLogBlk(pnandInfo, blk);
		}
		else if(phyblk==FRAGMENT_BLOCK)
		{
			printk("the log blk(%d) is fragment\n", blk);
		}

	}

	printk("pnandInfo->zone_count = %d\n", pnandInfo->zone_count);

	for(zone=0; zone<pnandInfo->zone_count; zone++)
	{
//pnandInfo->nandtab[i].pfragment_tab[j].

		for(blkoffset=0; blkoffset<pnandInfo->nandtab[zone].fragment_tab_count; blkoffset++)
		{
			printk("pnandInfo->nandtab[%d].pfragment_tab[%d].logblk:%d\n", zone, blkoffset, pnandInfo->nandtab[zone].pfragment_tab[blkoffset].logblk);
			printk("pnandInfo->nandtab[%d].pfragment_tab[%d].page_idx:%d\n", zone, blkoffset, pnandInfo->nandtab[zone].pfragment_tab[blkoffset].page_idx);
			printk("pnandInfo->nandtab[%d].pfragment_tab[%d].phyblk_new:%d\n", zone, blkoffset, pnandInfo->nandtab[zone].pfragment_tab[blkoffset].phyblk_new);
			printk("pnandInfo->nandtab[%d].pfragment_tab[%d].phyblk_old:%d\n\n", zone, blkoffset, pnandInfo->nandtab[zone].pfragment_tab[blkoffset].phyblk_old);

		}

	}

	
}

unsigned int nand_Creat_log2phy_Table(Nand_Info_t *pnandInfo)
{
	unsigned char ver, ver_Correct, zone, ver_bak;
	unsigned short blk, blkoffset, logblk;
	Nand_LogInfo_t* pnli;
	unsigned short fragmentidx;
	
	//Nftl_Buf_t* p_nftl_buf = NFTL_GetAndSwitchBuf();

	// create the log2phy table when booting
	// it is run time creating ..  
	printk("nand_Creat_log2phy_Table\n");
		
	for(blk=NANE_RESERVED_COUNT; blk<pnandInfo->phyblock_count; blk++)
	{
		if(badblkInfo_isbad(blk)==SUCCESS)
		{
			printk("badblkInfo_isbad(%d)\n", blk);
			continue;
		}

		pnli = Nand_Read_PhyPageWithNli(blk, 0, pnandInfo->pReadBuf_ali, pnandInfo->pEccBuf_ali);		

		if(pnli)
		{
			Nand_parselogblk(pnli->logblk, &zone, &blkoffset);

			ver_bak = pnli->ver;
			logblk = pnli->logblk;
			
			if(pnandInfo->nandtab[zone].plog2phy[blkoffset].phyblk==NO_BLOCK)
			{
				pnandInfo->nandtab[zone].plog2phy[blkoffset].phyblk = blk;
				pnandInfo->nandtab[zone].plog2phy[blkoffset].ver = ver_bak;//pnli->ver;
			}
			else
			{
				unsigned short page_idx;//, page_old_idx;
				unsigned short phyblk_old, phyblk_new;
				unsigned short phyblk = pnandInfo->nandtab[zone].plog2phy[blkoffset].phyblk;				
				ver = pnandInfo->nandtab[zone].plog2phy[blkoffset].ver;
				
				ver_Correct = Nand_get_CorrectVer(ver, ver_bak);	
				
				pnandInfo->nandtab[zone].plog2phy[blkoffset].phyblk = FRAGMENT_BLOCK;
				
				fragmentidx = Nand_getfragment_element_idx(zone);	
				
				if(ver_Correct==ver)
				{
					phyblk_new = phyblk;
					phyblk_old = blk;
				}
				else
				{
					phyblk_new = blk;
					phyblk_old = phyblk;			
					
				}
	
				
				page_idx = Nand_get_page_idx(pnandInfo, phyblk_new);	

	#if 0			// only for debugging....  
				if(page_idx >= pnandInfo->page_per_block)
				{
					Nand_LogInfo_t* pnli_tmp;
					
					printk("ERROR : pnli->logblk(%d)\n", pnli->logblk);			
					
					pnli_tmp = Nand_Read_PhyPageWithNli(blk, 0, pnandInfo->pReadBuf_ali, pnandInfo->pEccBuf_ali);;
					printk("blk:%d pnli_tmp->logblk(%d) ", blk, pnli_tmp->logblk);
					printk("pnli_tmp->ver(%d)\n", pnli_tmp->ver);

					pnli_tmp = Nand_Read_PhyPageWithNli(phyblk, 0, pnandInfo->pReadBuf_ali, pnandInfo->pEccBuf_ali);;
					printk("phyblk:%d pnli_tmp->logblk(%d) ", phyblk, pnli_tmp->logblk);
					printk("pnli_tmp->ver(%d)\n", pnli_tmp->ver);

					page_old_idx = Nand_get_page_idx(pnandInfo, phyblk_old);
					printk("ERROR : page_idx(%d) ", page_idx);
					printk("page_old_idx(%d)!\n", page_old_idx);
					printk("blk:%d ", blk);
					printk("phyblk:%d ", phyblk);			
					printk("phyblk_new:%d ", phyblk_new);
					printk("phyblk_old:%d ", phyblk_old);			
					printk("ver:%d ", ver);
					printk("ver_bak:%d \n", ver_bak);					
					
					
				}
	#endif			
				pnandInfo->nandtab[zone].plog2phy[blkoffset].ver = ver_Correct;
				pnandInfo->nandtab[zone].plog2phy[blkoffset].phyblk = FRAGMENT_BLOCK;
				pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].logblk = logblk;//blkoffset;	
				pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_old = phyblk_old;
				pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_new = phyblk_new;
				pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].page_idx = page_idx;
				pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].ver = ver_Correct;
				
			}
		}
		else
		{
			//printk("blk:%d \n", blk);
			//Nand_print_buf(NandCache2NonCacheAddr(pnandInfo->pEccBuf_ali), 64);
			
			FIFO_push(&pnandInfo->nandtab[zone].fifo , blk);
		}
	}	
	//Nand_checklogtab(pnandInfo);
	//Nand_print_fifo(pnandInfo);
	Nand_PrintInfo2File("d:\\nand_Creat_log2phy_Table.txt");

	return SUCCESS;
	
}

void Nand_debugprint(void)
{
	Nand_checklogtab(&g_nand_info);
	Nand_print_fifo(&g_nand_info);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned int Nand_Init(Chip_Info_t* pChipInfo, unsigned int *pIsNeedFdisk)
{
	psysinfo_t* psysInfo;
	
	Nand_Info_t *pnandInfo = &g_nand_info;
	
	if(g_nand_IsfirstInit)
	{
		memcpy((unsigned char*)pChipInfo, (unsigned char*)pnandInfo, sizeof(Chip_Info_t));
		return SUCCESS;	
	}	

	printk("Nand init!\n");
	psysInfo = initDriver(0x09, 0xf05d, 0x1f1111);		// read id also
	
	if(Nand_InfoGet(pnandInfo, psysInfo)==FAILURE)
	{
		printk("Nand_InfoGet is fail.\n");
		return FAILURE;		
	}
	nf_DiskInfo_Set(psysInfo);

	memcpy((unsigned char*)pChipInfo, (unsigned char*)pnandInfo, sizeof(Chip_Info_t));

	g_nand_IsfirstInit = 1;
	
	if(nand_buf_init(pnandInfo)==FAILURE)
	{
		printk("nand_buf_init is fail.\n");
		return FAILURE;
	}
	
	if(nand_check_is_valid(pnandInfo)==FAILURE)
	{
		//erase all blk & install to fifo
		//restore the data
		printk("erase all blk & install to fifo.\n");
	
		nand_eraseall(pnandInfo);
		
		nand_WriteAllLogBlk(pnandInfo);		
		
		nand_set_valid(pnandInfo);
		
		*pIsNeedFdisk = 1;
	}else
	{
	
		//load log2phy table from nand	
		if(nand_Creat_log2phy_Table(pnandInfo)==FAILURE)
		{
			printk("Warning : nand_Creat_log2phy_Table is fail.\n");
			//return FAILURE;			
		}
		*pIsNeedFdisk = 0;
	}
	SPMP_DEBUG_PRINT("Init OK\n");
	return SUCCESS;
}


void Nand_free(void)
{
	int i;
	Nand_Info_t *pnandInfo = &g_nand_info;

	Nand_PrintInfo2File("d:\\Nand_free.txt");
	
	g_nand_IsfirstInit = 0;
	
	for(i=0; i< pnandInfo->zone_count; i++)
	{
		FIFO_free(&pnandInfo->nandtab[i].fifo);
		
		if(pnandInfo->nandtab[i].plog2phy)
		{
			memFree(pnandInfo->nandtab[i].plog2phy);
		}
		
		if(pnandInfo->nandtab[i].pfragment_tab)
		{
			memFree(pnandInfo->nandtab[i].pfragment_tab);		
		}			
		memset(&(pnandInfo->nandtab[i]), 0, sizeof(Nand_tab_t));
	}
	
	if(pnandInfo->pReadBuf)
	{
		memFree(pnandInfo->pReadBuf);
		pnandInfo->pReadBuf = NULL;
		pnandInfo->pReadBuf_ali = NULL;
	}
	NPB_Info_free();
	SPMP_DEBUG_PRINT("Remove nf driver\n");
	Remove_NFDriver();

#if 0
	if(pnandInfo->pWriteBuf)
	{
		memFree(pnandInfo->pWriteBuf);
		pnandInfo->pWriteBuf = NULL;
		pnandInfo->pWriteBuf_ali = NULL;
	}

	if(pnandInfo->pEccBuf)
	{
		memFree(pnandInfo->pEccBuf);
		pnandInfo->pEccBuf = NULL;
		pnandInfo->pEccBuf_ali = NULL;
	}
#endif	
}


unsigned short Nand_getfragmentidx(unsigned short logblk, unsigned char zone)
{
	unsigned short idx;
	Nand_Info_t *pnandInfo = &g_nand_info;
	for(idx=0; idx <= pnandInfo->nandtab[zone].fragment_tab_count; idx++)
	{
		if(pnandInfo->nandtab[zone].pfragment_tab[idx].logblk == logblk)
		{
			return idx;
		}	
	}
	return NO_BLOCK;
}



void Nand_CopyBlkA2BlkB_FromPagea_2pageb(unsigned short blk_a, unsigned short blk_b, unsigned short page_a, unsigned short page_b)
{
	unsigned short i;
	Nand_Info_t *pnandInfo = &g_nand_info;

//printk("Nand_CopyBlkA2BlkB_FromPagea_2pageb(%d, %d, %d, %d)\n", blk_a, blk_b, page_a, page_b);

	if(blk_a==NO_BLOCK || blk_b==NO_BLOCK)
	{
		printk("ERROR : Nand_CopyBlkA2BlkB_FromPagea_2pageb(%d, %d, %d, %d)\n", blk_a, blk_b, page_a, page_b);
		return;
	}
	
	for(i=page_a; i<=page_b; i++)
	{
		if(Nand_Read_PhyPage(blk_a, i, pnandInfo->pReadBuf_ali, pnandInfo->pEccBuf_ali)==SUCCESS)
		{
			Nand_Write_PhyPage(blk_b, i, pnandInfo->pReadBuf_ali, pnandInfo->pEccBuf_ali);
		}
		
	}	
}

unsigned short Nand_getfragment_element_idx(unsigned char zone)
{
	unsigned short idx;
	unsigned short fragmentidx, page;
	unsigned short logblk, blk_a, blk_b, page_a, page_b, blkoffset;
		
	Nand_Info_t *pnandInfo = &g_nand_info;

	fragmentidx=0;
	page=0;
	
	for(idx=0; idx < pnandInfo->nandtab[zone].fragment_tab_count; idx++)
	{
		if(pnandInfo->nandtab[zone].pfragment_tab[idx].logblk == NO_BLOCK)
		{
			return idx;		// find the empty fragment record
		}

		// to check which block contains the more pages
		if(pnandInfo->nandtab[zone].pfragment_tab[idx].page_idx>page)	
		{
			//printk("idx:%d\n", idx);
			fragmentidx = idx;
			page = pnandInfo->nandtab[zone].pfragment_tab[idx].page_idx;
		}
	}

	logblk = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].logblk;	
	blk_a = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_old;
	blk_b = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_new;
	page_a = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].page_idx;
	page_b = pnandInfo->page_per_block-1;

	// do fragment block garbage collection			
	//Merger phyblk_old to phyblk_new
	g_nli.logblk = logblk;
	g_nli.ver = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].ver;
	Nand_CopyBlkA2BlkB_FromPagea_2pageb(blk_a, blk_b, page_a, page_b);

	//erase blk_a
	nand_erase_insert2fifo(pnandInfo, blk_a);
		
	Nand_parselogblk(logblk, &zone, &blkoffset);
	
	pnandInfo->nandtab[zone].plog2phy[blkoffset].phyblk = blk_b;
	pnandInfo->nandtab[zone].plog2phy[blkoffset].ver = g_nli.ver;
	
	pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].logblk = NO_BLOCK;
	pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_old = NO_BLOCK;	
	pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_new = NO_BLOCK;
	pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].page_idx =0;	
	
	return fragmentidx;	
}
unsigned short Nand_ReadPage_getphyblk(unsigned short blk, unsigned short page)
{	
	Nand_Info_t *pnandInfo = &g_nand_info;
	unsigned char zone;
	unsigned short phyblk, blkoffset;
	
	//BANK_BLK=1000, 
	//log blk=36 ==> zone=0, blkoffset=36
	//log blk=2033 ==> zone=2, blkoffset=33
	Nand_parselogblk(blk, &zone, &blkoffset);


	phyblk = pnandInfo->nandtab[zone].plog2phy[blkoffset].phyblk;
	
	//printk("Nand_ReadPage_getphyblk(%d, %d)phyblk:%d\n", blk, page, phyblk);
	
	if(phyblk==FRAGMENT_BLOCK)
	{
		unsigned short fragmentidx = Nand_getfragmentidx(blk, zone);
	
		if(fragmentidx != NO_BLOCK)
		{
			if(pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].page_idx > page)
			{
				return pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_new;	
			}
			return pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_old;
		}
	}
	else if(phyblk==NO_BLOCK)
	{
		printk("ERROR:########################3333\n");
		printk("Nand_parselogblk(%d, %d, %d)\n", blk, zone, blkoffset);
		printk("Nand_ReadPage_getphyblk(%d, %d)phyblk:%d\n", blk, page, phyblk);
		printk("###############################3333\n");	
	}

	return phyblk;
}
unsigned short Nand_WritePage_getphyblk(unsigned short blk, unsigned short page, unsigned char *pver)
{
	
	Nand_Info_t *pnandInfo = &g_nand_info;
	unsigned char zone;
	unsigned short phyblk, blkoffset;
	unsigned short blk_a, blk_b, page_a, page_b;
	unsigned short fragmentidx;	
	
	*pver = 0;
	
	//EX : BANK_BLK=1000
	//log blk=36 ==> zone=0, blkoffset=36
	//log blk=2033 ==> zone=2, blkoffset=33
	Nand_parselogblk(blk, &zone, &blkoffset);
	
	phyblk = pnandInfo->nandtab[zone].plog2phy[blkoffset].phyblk;
	
	if(phyblk==FRAGMENT_BLOCK)
	{
		fragmentidx = Nand_getfragmentidx(blk, zone);
		
		if(fragmentidx != NO_BLOCK)
		{
			// page_idx is the next available page
			if(pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].page_idx == page)
			{
				*pver = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].ver;
				pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].page_idx++;
				return pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_new;	
			}
			else if(pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].page_idx < page)
			{		// start fragment mechinism
			
				blk_a = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_old;
				blk_b = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_new;
				page_a = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].page_idx;
				page_b = page - 1;
	
				g_nli.logblk = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].logblk;
				g_nli.ver = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].ver;

				// copy the old valid pages to the fragment block
				Nand_CopyBlkA2BlkB_FromPagea_2pageb(blk_a, blk_b, page_a, page_b);
				*pver = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].ver;
				pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].page_idx = page+1;
				return pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_new;	
			}
			else
			{	// 1. merge the old block to the fragment block
			    // 2. copy the pages below page_idx to a new fragment block

				blk_a = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_old;
				blk_b = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_new;
				page_a = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].page_idx;
				page_b = pnandInfo->page_per_block-1;
	
				g_nli.logblk = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].logblk;
				g_nli.ver = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].ver;
				
				//Merger phyblk_old to phyblk_new
				Nand_CopyBlkA2BlkB_FromPagea_2pageb(blk_a, blk_b, page_a, page_b);
		
				pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_old = blk_b;
				
				//erase blk_a
				nand_erase_insert2fifo(pnandInfo, blk_a);
				
				//get a blk from fifo
				if(FIFO_pop(&pnandInfo->nandtab[zone].fifo, &blk_a)==SUCCESS)
				{
					pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_new = blk_a;
					pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].ver++;
					
					//copy phyblk_old to phyblk_new from page0 to page-1;	
					if(page)	
					{		
						g_nli.logblk = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].logblk;
						g_nli.ver = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].ver;
						Nand_CopyBlkA2BlkB_FromPagea_2pageb(blk_b, blk_a, 0, page-1);
					}
					
					pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].page_idx = page+1;	
					*pver = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].ver;
					return pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_new;
				}

				printk("ERROR1 : Can't found any blk from fifo\n");
				return NO_BLOCK;				
			}
			
		}
		else
		{
			printk("ERROR2 : get fragmentidx is fail.\n");
			return NO_BLOCK;
		}
	}

	/// non-fragment block

	//from fragment_tab get a element idx
	// to ensure to get a valid fragment record here...... 
	fragmentidx = Nand_getfragment_element_idx(zone);	
	if(fragmentidx != NO_BLOCK)
	{	
//		unsigned char ver;
		if(FIFO_pop(&pnandInfo->nandtab[zone].fifo, &blk_a)==SUCCESS)
		{
			//phyblk = pnandInfo->nandtab[zone].plog2phy[blkoffset].phyblk;
			pnandInfo->nandtab[zone].plog2phy[blkoffset].phyblk = FRAGMENT_BLOCK;
			pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].logblk = blk;	
			pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_old = phyblk;
			pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_new = blk_a;
			pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].page_idx = page;
			pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].ver = pnandInfo->nandtab[zone].plog2phy[blkoffset].ver;
			pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].ver++;
			pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].page_idx++;

			if(page)
			{	
				g_nli.logblk = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].logblk;
				g_nli.ver = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].ver;
				Nand_CopyBlkA2BlkB_FromPagea_2pageb(phyblk, blk_a, 0, page-1);
			}	
			
			*pver = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].ver;
			return pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_new;
		}
		printk("ERROR2 : Can't found any blk from fifo\n");
		return NO_BLOCK;
	}	
	
	printk("ERROR2 : get fragmentidx is fail.\n");
	return NO_BLOCK;		

}

//Defragment block

// one zone only allow 10 fragment blocks
unsigned int Nand_Defragment(unsigned short blk)
{
	Nand_Info_t *pnandInfo = &g_nand_info;
	unsigned char zone;
	unsigned short phyblk, blkoffset;
	unsigned short fragmentidx;	
	
	Nand_parselogblk(blk, &zone, &blkoffset);

	fragmentidx = Nand_getfragmentidx(blk, zone);	

	//erase the old block since the block is filled full 
	phyblk = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_old;
	nand_erase_insert2fifo(pnandInfo, phyblk);
	
	pnandInfo->nandtab[zone].plog2phy[blkoffset].ver = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].ver;
	pnandInfo->nandtab[zone].plog2phy[blkoffset].phyblk = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_new;
	pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].logblk = NO_BLOCK;	
	pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_old = NO_BLOCK;
	pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_new = NO_BLOCK;
	pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].page_idx = 0;	
	pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].ver = 0;
	
	return SUCCESS;
}

Nand_LogInfo_t* Nand_Read_PhyPageWithNli(unsigned short blk, unsigned short page, unsigned char *pdata, unsigned char* peccbuf)
{
	Nand_Info_t *pnandInfo = &g_nand_info;
	
	unsigned long u32PhyAddr;

	if(blk > pnandInfo->phyblock_count)
	{
		printk("ERROR : Nand_Read_PhyPageWithNli blk(%d) > pnandInfo->phyblock_count(%d)\n", blk, pnandInfo->phyblock_count);
		return NULL;
	}
	
	u32PhyAddr = (unsigned long)(blk + pnandInfo->diskstartblk) << pnandInfo->page_per_blk_shift;
	u32PhyAddr += page;
	
#ifdef _WIN32
	NAND_ReadPhyPage(u32PhyAddr, (char*)pdata, (char*)peccbuf);
#else
	ReadWritePage(u32PhyAddr, (unsigned long*)pdata, (unsigned long*)peccbuf, NF_READ);
	if(BCHProcess_ex((unsigned long*)pdata, (unsigned long*)peccbuf, pnandInfo->pagesize, BCH_DECODE)!=1)
	{
		//printk("ERROR : Nand_Read_PhyPageWithNli(%d, %d) is error\n",blk, page);
		return NULL;
	}	
#endif	

	if(Nand_GetNli(peccbuf, &g_nli)==SUCCESS)
	{
		return &g_nli;	
	}

	return NULL;
}


unsigned int Nand_Read_PhyPage(unsigned short blk, unsigned short page, unsigned char *pdata, unsigned char* peccbuf)
{
	unsigned long u32PhyAddr;
	Nand_Info_t *pnandInfo = &g_nand_info;
	unsigned long*	ptr = NULL;
	int i, count = 0;

//printk("Nand_Read_PhyPage(%d, %d)\n", blk, page);
	
	if(blk > pnandInfo->phyblock_count)
	{
		//printk("ERROR : Nand_Read_PhyPage blk(%d) > pnandInfo->phyblock_count(%d)\n", blk, pnandInfo->phyblock_count);
		return FAILURE;
	}
	
	u32PhyAddr = (unsigned long)(blk + pnandInfo->diskstartblk) << pnandInfo->page_per_blk_shift;
	u32PhyAddr += page;
	if( (debug_flag&0xff) ==1)
	{
		SPMP_DEBUG_PRINT( "pageAddr: %x(%lu)\n", 	(unsigned int)u32PhyAddr, u32PhyAddr );
	}
#ifdef _WIN32
	NAND_ReadPhyPage(u32PhyAddr, (char*)pdata, (char*)peccbuf);
#else
	ReadWritePage(u32PhyAddr, (unsigned long*)pdata, (unsigned long*)peccbuf, NF_READ);
	
	//skip all 0xff
	ptr = (unsigned long*)GET_NON_CACHED_ADDR_PTR(peccbuf);
	count = pnandInfo->pagesize >> 7;
	
	for(i=0; i<count; i++)
	{
		if(ptr[i]!=0xFFFFFFFF)
			break;	
	} 
	if(i==count)
	{
		return SUCCESS;
	}
	//end
	
	if(BCHProcess_ex((unsigned long*)pdata, (unsigned long*)peccbuf, pnandInfo->pagesize, BCH_DECODE)!=1)
	{
		//printk("ERROR : Nand_Read_PhyPage(%d, %d) is error\n",blk, page);
		//Nand_print_buf(pdata, 64);
		//Nand_print_buf(peccbuf, pnandInfo->pagesize>>5);
		return FAILURE;
	}
	/*
	if( (debug_flag&0xff) ==1)
	{
		SPMP_DEBUG_PRINT("Dead Lock\n");
		while(1);
	}
	*/
#endif	
	
	return SUCCESS;
}

unsigned int Nand_Write_PhyPage(unsigned short blk, unsigned short page, unsigned char *pdata, unsigned char* peccbuf)
{
	unsigned long u32PhyAddr;
	Nand_Info_t *pnandInfo = &g_nand_info;

//printk("Nand_Write_PhyPage(%d, %d)\n", blk, page);

	if(blk > pnandInfo->phyblock_count)
	{
		printk("ERROR : Nand_Write_PhyPage blk(%d) > pnandInfo->phyblock_count(%d)\n", blk, pnandInfo->phyblock_count);
		return FAILURE;
	}
		
	u32PhyAddr = (unsigned long)(blk + pnandInfo->diskstartblk) << pnandInfo->page_per_blk_shift;
	u32PhyAddr += page;
	
	Nand_SetNli(peccbuf, g_nli);
#ifdef _WIN32
	NAND_WritePhyPage(u32PhyAddr, (char*)pdata, (char*)peccbuf);
#else
	BCHProcess_ex((unsigned long*)pdata, (unsigned long*)peccbuf, pnandInfo->pagesize, BCH_ENCODE);
	ReadWritePage(u32PhyAddr, (unsigned long*)pdata, (unsigned long*)peccbuf, NF_WRITE);
#endif	
		
	return SUCCESS;
}

unsigned int Nand_Block_Replacement(unsigned short logblk, unsigned short phyblk)
{
	Nand_Info_t *pnandInfo = &g_nand_info;
	unsigned char zone;
	unsigned short blk, blkoffset;
	unsigned short blk_a, blk_b, page_a, page_b;
	Nand_LogInfo_t* pnli = NULL;

	Nand_parselogblk(logblk, &zone, &blkoffset);
	
	blk_a = phyblk;
	

	if(FIFO_pop(&pnandInfo->nandtab[zone].fifo, &blk_b)==FAILURE)
	{
		printk("FIFO empty!\n");
		return FAILURE;			
	}

	blk = pnandInfo->nandtab[zone].plog2phy[blkoffset].phyblk;	
	if(blk==FRAGMENT_BLOCK)
	{
		unsigned short fragmentidx = Nand_getfragmentidx(logblk, zone);
	
		if(fragmentidx != NO_BLOCK)
		{		
			//	phyblk_new means the process of writing page in the half way
			if(phyblk==pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_new)
			{			// only copy the used pages
				page_a = 0;
				page_b = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].page_idx - 1;	
				
				pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_new = blk_b;				
			}
			else	// means the block is done
			{
				//page_a = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].page_idx;
				page_a = 0;
				page_b = pnandInfo->page_per_block-1;	
				pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].phyblk_old = blk_b;		
			}
			
			g_nli.logblk = logblk;
			g_nli.ver = pnandInfo->nandtab[zone].pfragment_tab[fragmentidx].ver;

			// copy pages what we needed
			Nand_CopyBlkA2BlkB_FromPagea_2pageb(blk_a, blk_b, page_a, page_b);
			return SUCCESS;	
		}

		printk("Can't found any fragment idx\n");
		return FAILURE;				

	}
	
	page_a = 0;		// copy all pages
	page_b = pnandInfo->page_per_block-1;	

	//get g_nli
	pnli = Nand_Read_PhyPageWithNli(blk_a, 0, pnandInfo->pReadBuf_ali, pnandInfo->pEccBuf_ali);
	
	if(pnli)
	{
		g_nli.logblk = pnli->logblk;
		g_nli.ver = pnli->ver;	
		Nand_CopyBlkA2BlkB_FromPagea_2pageb(blk_a, blk_b, page_a, page_b);
		
		pnandInfo->nandtab[zone].plog2phy[blkoffset].phyblk = blk_b;	
		return SUCCESS;
	}
	
	return FAILURE;	
}

// write the data to these 2 block... 
// if the data is losed, the system will regenerate....
// the system will read the header to see if the data is correct
void Nand_bad_block_mark(unsigned short blk)
{
	int i, idx;
	Nand_Info_t *pnandInfo = &g_nand_info;
	
	badblkInfo_set(blk);	
	
	for(i=0; i<NAND_BADBLKTABSAVE_COUNT; i++)
	{
		idx = i+NAND_BADBLKTABSAVE_START;
		nand_eraseblk(pnandInfo, idx);
		badblkInfo_save(pnandInfo, idx);
	}
}

unsigned int Nand_ReadPage(unsigned int logpage, unsigned char *pdata)
{
	unsigned int rc = SUCCESS;
	unsigned short phyblk, logblk, page; 
	Nand_Info_t *pnandInfo = &g_nand_info;

	logblk = logpage >> pnandInfo->page_per_blk_shift;
	page = logpage & (pnandInfo->page_per_block-1);

	// Max block number is 32768 
	phyblk = Nand_ReadPage_getphyblk(logblk, page);		// to get the physical block number
/* modify by mm.li 01-12,2011 clean warning */
	/*
	if(debug_flag & 0x301 == 0x301)
	*/
	if((debug_flag & 0x301) == 0x301)
/* modify end */	
	{
		SPMP_DEBUG_PRINT("Nand_ReadPage(%d, %d) %d\n", logblk, page, phyblk);
	}

	if(phyblk==NO_BLOCK)
	{
		printk(" ERROR : phyblk(%d) Nand_ReadPage_getphyblk(%d, %d)\n", phyblk, logblk, page);		
	}
	else
	{
		rc = Nand_Read_PhyPage(phyblk, page, pdata, pnandInfo->pEccBuf_ali);
		if(rc == FAILURE)
		{
			//Block Replacement	
			Nand_Block_Replacement(logblk, phyblk);
			
			//bad block mark
			Nand_bad_block_mark(phyblk);
	
		}
	}

	return rc;
}

unsigned int Nand_WritePage(unsigned int logpage, unsigned char *pdata)
{
	unsigned int rc = SUCCESS;
	unsigned char ver;
	unsigned short phyblk, logblk, page; 
	Nand_Info_t *pnandInfo = &g_nand_info;


	
	logblk = logpage >> pnandInfo->page_per_blk_shift;
	page = logpage & (pnandInfo->page_per_block-1);
	
	phyblk = Nand_WritePage_getphyblk(logblk, page, &ver);

//printk("Nand_WritePage(%d, %d) %d\n", logblk, page, phyblk);
	
	if(phyblk==NO_BLOCK)
	{
		printk("ERROR : phyblk(%d) Nand_WritePage_getphyblk(%d, %d)\n", phyblk, logblk, page);		
	}

	if(page==(pnandInfo->page_per_block-1))
	{
		Nand_Defragment(logblk);
	}

	g_nli.logblk = logblk;
	g_nli.ver = ver;

	rc = Nand_Write_PhyPage(phyblk, page, pdata, pnandInfo->pEccBuf_ali);
	return rc;
}

