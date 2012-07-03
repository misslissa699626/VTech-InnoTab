
#ifdef	_WIN32
#include <stdio.h>
#include <malloc.h>

#include <stdlib.h>

#include "nand_pgbase.h"
#define NPB_CACHE_ON_ 0

#define		NAND_SIM
#include "../../NandSim/nand_m210.h"
#else
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/string.h>

#define NPB_CACHE_ON_ 0

#include "fifo.h"
#include "nand_pgbase.h"
#include "nftl.h"
#include "../hal/nf_s330.h"
#endif

#include "../hal/bch_s336.h"

//nf_flag_t g_NandInitFlag;
//nf_flag_t g_NandRWFlag;

int g_isFirst = 0;
//unsigned int g_npb_write_count = 0;
//unsigned int g_npb_write_countXXX = 0;
unsigned int NPB_Recovery(void);
void NPB_Erase_block(unsigned short blk);

//=========================================================================
//		FIFO Manage
//=========================================================================
FIFO_Manage_t g_npb_fifo;

//=========================================================================
//		NPB List
//=========================================================================
NPB_list_t g_npbl;


void NPBL_Push(NPB_list_t* p_npbl, unsigned short item)
{
	unsigned short i;
	for(i=0;i<p_npbl->len;i++)
	{
		if(p_npbl->list[i]==item)
		{	
			return; 
		}	
			
	}
	p_npbl->list[i]=item;
	p_npbl->len++;	
}
//=========================================================================
unsigned short NPBL_Pop(NPB_list_t* p_npbl)
{
	if(p_npbl->len > 0 )
	{
		p_npbl->len--;	
		return p_npbl->list[p_npbl->len];
	}
	
	return NO_NPB_PAGE;
}
unsigned short NPBL_Get_Item(NPB_list_t* p_npbl, unsigned short idx)
{
	return p_npbl->list[idx];
}
//=========================================================================
void NPBL_Init(NPB_list_t* p_npbl)
{
	unsigned short i;
	
	for(i=0;i<MAX_NPB_LIST_SIZE;i++)
	{
		p_npbl->list[i]= NO_NPB_PAGE;		
	}
	p_npbl->len=0;
}
//=========================================================================
unsigned short NPBL_Get_len(NPB_list_t* p_npbl)
{
		return p_npbl->len;
}
//=========================================================================




//=========================================================================
//		NPB 
//=========================================================================
NPB_Info_t g_npb_info;
NPB_Phy2Log_t g_npb_p2l;
unsigned int Is_enable_erase_savetable = 1;

unsigned char* NPB_Cache2NonCacheAddr(unsigned char* addr)
{
#if NPB_CACHE_ON_	
	return (unsigned char *)(((unsigned long)(addr)) | 0x10000000);
#else
	return (unsigned char *)((unsigned long)(addr));
#endif		
}

void cache_sync(void);
void NPB_Cachesync(void) 
{ 
#if NPB_CACHE_ON_	
	cache_sync(); 
#endif  	
} 

void prin_npb_info(void)
{
	int i;
	int count=0;;
	printk("################npb_info###################\n");
	for(i=0;i<g_npb_info.phyblk_count;i++)
	{
		//printk("phy_blk_info[%d]:%d\n",i , g_npb_info.phy_blk_info[i].count);
		printk("{[%d]:%d} ",i , g_npb_info.phy_blk_info[i].count);
		count += g_npb_info.phy_blk_info[i].count;
	}
		
	printk("count:%d\n",count);
	printk("############################################\n");
}

void NPB_print_buf(unsigned char*pBuffer, int len)
{
	unsigned int i;
	unsigned char* buf_NC;
	
	buf_NC = NPB_Cache2NonCacheAddr(pBuffer);
	
	printk("buf_NC:0x%8x\n",(unsigned int)buf_NC);

	for(i=0;i<len;i++)
	{
		if(i%16==0)
		{
			printk("\n0x:%08x | ",i);	
		}
		else if(i%16==8)
		{
			printk("- ");
		}
			
		printk("%02x ",buf_NC[i]);		
	}
	printk("\n");	
}


unsigned int NPB_Info_Init(void)
{
	int i, size;

	size = g_npb_info.logblk_count<<g_npb_info.blockshift;

	printk("p_l2ptab --> size:%d\n",size* sizeof(NPB_Log2Phy_t));
	g_npb_info.p_l2ptab = (NPB_Log2Phy_t *)memAlloc(size * sizeof(NPB_Log2Phy_t));
	if(g_npb_info.p_l2ptab==NULL)
	{
		printk("g_npb_info.p_l2ptab malloc is failure!\n");
		return FAILURE;	
	}
	else
	{
		for(i=0;i<size;i++)
		{
			g_npb_info.p_l2ptab[i].phypage = NO_NPB_PAGE;
			g_npb_info.p_l2ptab[i].version = 0;

		}
	}
	
	//p_p2ltab
	size = g_npb_info.phyblk_count<<g_npb_info.blockshift;
	printk("p_p2ltab --> size:%d\n",size* sizeof(NPB_Phy2Log_t));
	g_npb_info.p_p2ltab = (NPB_Phy2Log_t *)memAlloc(size * sizeof(NPB_Phy2Log_t));
	if(g_npb_info.p_p2ltab==NULL)
	{
		printk("g_npb_info.p_p2ltab malloc is failure!\n");
		return FAILURE;	
	}
	else
	{
		for(i=0;i<size;i++)
		{
			g_npb_info.p_p2ltab[i].logpage = NO_NPB_PAGE;
			g_npb_info.p_p2ltab[i].version = 0;
		}
	}	

#if 0
	size = g_npb_info.page_per_block>>3;
	printk("g_npb_info.phy_blk_info --> size:%d\n",size* g_npb_info.phyblk_count);

	for(i=0;i<g_npb_info.phyblk_count;i++)
	{

		g_npb_info.phy_blk_info[i].count = 0;
		g_npb_info.phy_blk_info[i].pInfo = (unsigned char*)memAlloc(size);
		if(g_npb_info.phy_blk_info[i].pInfo==NULL)
		{
			printk("g_npb_info.phy_blk_info[%d].pInfo malloc is failure!\n", i);
			return FAILURE;	
		}
		else
		{
			memset(g_npb_info.phy_blk_info[i].pInfo, 0, size);
		}
		

	}
#else
	memset((unsigned char*)g_npb_info.phy_blk_info, 0, g_npb_info.phyblk_count * sizeof(NPB_Phy_blk_Info_t));
#endif


	g_npb_info.pReadBuf = (char*)memAlloc(g_npb_info.pagesize+ALIGNED64);
	if(g_npb_info.pReadBuf == NULL)
	{
		printk("g_npb_info.pReadBuf malloc is failure!\n");
		return FAILURE;
	}
	else
	{
		g_npb_info.pReadBuf_ali =(unsigned char*)(((unsigned int)g_npb_info.pReadBuf+ALIGNED64-1)&(0xFFFFFFC0));
		g_npb_info.pReadBuf_ali_NC = NPB_Cache2NonCacheAddr(g_npb_info.pReadBuf_ali);
	}

	g_npb_info.pWriteBuf1 = (char*)memAlloc(g_npb_info.pagesize+ALIGNED64);
	//g_npb_info.pWriteBuf2 = (char*)memAlloc(g_npb_info.pagesize+ALIGNED64);
	if(g_npb_info.pWriteBuf1 == NULL)
	{
		printk("g_npb_info.pWriteBuf malloc is failure!\n");
		return FAILURE;
	}
	else
	{
		g_npb_info.pWriteBuf1_ali =(unsigned char*)(((unsigned int)g_npb_info.pWriteBuf1+ALIGNED64-1)&(0xFFFFFFC0));
		//g_npb_info.pWriteBuf1_ali_NC = NPB_Cache2NonCacheAddr(g_npb_info.pWriteBuf1_ali);
		//g_npb_info.pWriteBuf2_ali =(unsigned char*)(((unsigned int)g_npb_info.pWriteBuf2+ALIGNED64-1)&(0xFFFFFFC0));
		//g_npb_info.pWriteBuf2_ali_NC = NPB_Cache2NonCacheAddr(g_npb_info.pWriteBuf2_ali);			
		
		//init
		g_npb_info.pWriteBuf = g_npb_info.pWriteBuf1_ali;
		//g_npb_info.pWriteBuf_ali_NC = g_npb_info.pWriteBuf1_ali_NC;
	}

	g_npb_info.pEccBuf1 = (char*)memAlloc(g_npb_info.pagesize + ALIGNED64);
	//g_npb_info.pEccBuf2 = (char*)memAlloc(g_npb_info.pagesize+ALIGNED64);	
	if(g_npb_info.pEccBuf1 == NULL)
	{
		printk("g_npb_info.pWriteBuf malloc is failure!\n");
		return FAILURE;
	}
	else
	{
		g_npb_info.pEccBuf1_ali =(unsigned char*)(((unsigned int)g_npb_info.pEccBuf1+ALIGNED64-1)&(0xFFFFFFC0));
		//g_npb_info.pEccBuf2_ali =(unsigned char*)(((unsigned int)g_npb_info.pEccBuf2+ALIGNED64-1)&(0xFFFFFFC0));

		//init
		g_npb_info.pEccBuf = g_npb_info.pEccBuf1_ali;
		//g_npb_info.pEccBuf_ali_NC = g_npb_info.pEccBuf1_ali_NC;			
	}

	printk("g_npb_info.pReadBuf_ali : 0x%x\n", (unsigned int)g_npb_info.pReadBuf_ali);
	printk("g_npb_info.pWriteBuf : 0x%x\n", (unsigned int)g_npb_info.pWriteBuf);
	printk("g_npb_info.pEccBuf : 0x%x\n", (unsigned int)g_npb_info.pEccBuf);


	return SUCCESS;

}

//MALLOC_DBGTAB_ENTRY(NPB_Info_Init);

unsigned int NPB_Info_free(void)
{
	if(g_npb_info.p_l2ptab)
	{
		memFree(g_npb_info.p_l2ptab);
		g_npb_info.p_l2ptab = NULL;
	}
	
	if(g_npb_info.p_p2ltab)
	{
		memFree(g_npb_info.p_p2ltab);
		g_npb_info.p_p2ltab = NULL;		
	}

#if 0
	for(i=0;i<g_npb_info.phyblk_count;i++)
	{
		if(g_npb_info.phy_blk_info[i].pInfo)
		{
			memFree(g_npb_info.phy_blk_info[i].pInfo);
			g_npb_info.phy_blk_info[i].pInfo = NULL;
		}
	}
#endif
	
	if(g_npb_info.pReadBuf)
	{
		memFree(g_npb_info.pReadBuf);
		g_npb_info.pReadBuf = NULL;	
	}

	if(g_npb_info.pWriteBuf1)
	{
		memFree(g_npb_info.pWriteBuf1);
		g_npb_info.pWriteBuf1 = NULL;	
	}
#if 0
	if(g_npb_info.pWriteBuf2)
	{
		memFree(g_npb_info.pWriteBuf2);
		g_npb_info.pWriteBuf2 = NULL;	
	}
#endif

	if(g_npb_info.pEccBuf1)
	{
		memFree(g_npb_info.pEccBuf1);
		g_npb_info.pEccBuf1 = NULL;	

	}

#if 0
	if(g_npb_info.pEccBuf2)
	{
		memFree(g_npb_info.pEccBuf2);
		g_npb_info.pEccBuf2 = NULL;	
	}
#endif

	return SUCCESS;
}
unsigned int NPB_nfvalshift(unsigned int x)
{
	int i=0;
	while(x)
	{
		x >>= 1;
		if(x)
			i++;
	}
	return i;
}

void NPB_set_RBMI(unsigned char* pBuffer, NPB_Phy2Log_t sri)
{
	if(getEccMode()==BCH_S336_24_BIT)
	{
#if 1
		Page_Info_24bit_t *ptr;

		ptr = (Page_Info_24bit_t *)NPB_Cache2NonCacheAddr(pBuffer); 
		ptr->log_page = sri.logpage;
		//ptr->Ver.version = sri.version;

		ptr->ver1 = (unsigned short)(sri.version>>16);
		ptr->ver2 = (unsigned short)(sri.version&0xFFFF);
#endif		
	}
	else
	{
		Page_Info_t *ptr;

		ptr = (Page_Info_t *)NPB_Cache2NonCacheAddr(pBuffer); 
		ptr->sector.log_page = sri.logpage;
		ptr->Ver.version = sri.version;
	}
}

//unsigned int NPB_get_PageInfo(unsigned char* pBuffer, unsigned short *log_page, unsigned char *version)
unsigned int NPB_get_PageInfo(unsigned char* pBuffer, unsigned short *log_page, unsigned int *version)
{
	unsigned short maxlogpage;

	if(getEccMode()==BCH_S336_24_BIT)
	{
#if 1
		Page_Info_24bit_t *ptr;
		ptr = (Page_Info_24bit_t *)NPB_Cache2NonCacheAddr(pBuffer);
		*log_page = ptr->log_page;
		*version = (unsigned int)((ptr->ver1<<16) | ptr->ver2);  	
#endif		
	}
	else
	{	
		Page_Info_t *ptr;
		ptr = (Page_Info_t *)NPB_Cache2NonCacheAddr(pBuffer);
		*log_page = ptr->sector.log_page;
		*version = ptr->Ver.version;
	}
	//if(*log_page==NO_PAGE || *version==0)	
	if(*log_page==0xFFFF || *version==0xFFFFFFFF || *version==0)
	{
		//printk("ERROR-->*log_page :%d *version:0x%x\n", *log_page, *version);
		return FAILURE; 
	}	
	
	maxlogpage = g_npb_info.logblk_count<< g_npb_info.blockshift;
	
	if(*log_page >= maxlogpage)
	{
		return FAILURE; 
	}

	return SUCCESS;	
}

unsigned int NPB_IS_Page_Writed(unsigned short blk, unsigned short page)
{
	unsigned short i,j;

	i = (page>>3);
	j = (page&7);

	if(g_npb_info.phy_blk_info[blk].pInfo[i]&(1<<j))
		return SUCCESS;
	else
		return FAILURE;
}

unsigned int NPB_Page_Set_Writed(unsigned short PhyPage)
{
	unsigned short blk, page;
	unsigned short i,j;

	blk = (PhyPage>>g_npb_info.blockshift);
	page = PhyPage - (blk<<g_npb_info.blockshift);

	i = (page>>3);
	j = (page&7);
	g_npb_info.phy_blk_info[blk].count++;
	g_npb_info.phy_blk_info[blk].pInfo[i] |= 1<<j;
	
	return SUCCESS;

}
unsigned int NPB_Page_Clean_Writed(unsigned short PhyPage)
{
	unsigned short blk, page;
	unsigned short i,j;
	
	if(PhyPage==NO_NPB_PAGE)
	{
		return SUCCESS;
	}

	blk = PhyPage>>g_npb_info.blockshift;
	page = PhyPage - (blk<<g_npb_info.blockshift);

	if(blk >=g_npb_info.phyblk_count)
	{
		printk("ERROR1 : NPB_Page_Clean_Writed(%d, %d, %d) is fail.\n", blk, page, PhyPage);
		return FAILURE;
	}

	if(NPB_IS_Page_Writed(blk, page)==FAILURE)
	{
		printk("ERROR : NPB_Page_Clean_Writed(%d, %d, %d) is fail.\n", blk, page, PhyPage);	
		return FAILURE;
	}

	i = (page>>3);
	j = (page&7);

	if(g_npb_info.phy_blk_info[blk].count)
	{
		g_npb_info.phy_blk_info[blk].count--;
		g_npb_info.phy_blk_info[blk].pInfo[i] &= ~(1<<j);
		return SUCCESS;
	}

	printk("NPB_Page_Clean_Writed(%d) is error!.\n", PhyPage);
	return FAILURE;

}
#if 0
void NPB_switch_buf(void)
{
	if(g_npb_info.pEccBuf == g_npb_info.pEccBuf1_ali)
	{
		g_npb_info.pEccBuf = g_npb_info.pEccBuf2_ali;
		g_npb_info.pWriteBuf = g_npb_info.pWriteBuf2_ali;		
	}
	else
	{	
		g_npb_info.pEccBuf = g_npb_info.pEccBuf1_ali;
		g_npb_info.pWriteBuf = g_npb_info.pWriteBuf1_ali;	
	}
}
#endif

unsigned int NPB_write_page(unsigned short blk, unsigned short page,unsigned char* pWritePyldBuffer,  unsigned char* pEccBuf)
{
	unsigned long u32PhyAddr;

	//printk("NPB_write_page(%d, %d)\n", blk, page);



	if(blk >= g_npb_info.phyblk_count)
	{
		printk("ERROR : NPB_write_page-> blk(%d) >= g_npb_info.phyblk_count(%d)\n", blk, g_npb_info.phyblk_count);
		printk("NPB_write_page(%d, %d)\n", blk, page);
		return FAILURE;
	}

	u32PhyAddr = (unsigned long)(g_npb_info.start_blk + blk) << g_npb_info.blockshift;

	u32PhyAddr += page;	
	
	NPB_Page_Set_Writed((blk << g_npb_info.blockshift)+page);

#if NPB_SUPPORT_TABLESAVE
	NPB_table_erase();
#endif
	NPB_set_RBMI(pEccBuf, g_npb_p2l);
	BCHProcess_ex((unsigned long*)pWritePyldBuffer, (unsigned long*)pEccBuf, g_npb_info.pagesize, BCH_ENCODE);
	ReadWritePage(u32PhyAddr, (unsigned long*)pWritePyldBuffer, (unsigned long*)pEccBuf, NF_WRITE);

 	return SUCCESS;
}


unsigned int  NPB_read_page(unsigned short blk, unsigned short page, unsigned char* pReadPyldBuffer,  unsigned char* pEccBuf, unsigned char IS_BCH)
{
	unsigned long u32PhyAddr;

	//printk("NPB_read_page(%d, %d)\n", blk, page);


	if(blk >= g_npb_info.phyblk_count)
	{
		return FAILURE; 
	}

	u32PhyAddr = (unsigned long)(g_npb_info.start_blk + blk) << g_npb_info.blockshift;

	u32PhyAddr += page;	

#ifdef _WIN32
	NAND_ReadPhyPage(u32PhyAddr, (char*) pReadPyldBuffer, pEccBuf);

#else
	ReadWritePage(u32PhyAddr, (unsigned long*)pReadPyldBuffer, (unsigned long*)pEccBuf, NF_READ);
	
	if(IS_BCH)
	{
		if(BCHProcess_ex((unsigned long*)pReadPyldBuffer, (unsigned long*)pEccBuf, g_npb_info.pagesize, BCH_DECODE)!=1)
		{
			int retry;

			retry = RETRY_COUNT;
		
			while(retry--)
			{
				printk("retry :%d\n",retry);
				ReadWritePage(u32PhyAddr, (unsigned long*)pReadPyldBuffer, (unsigned long*)pEccBuf, NF_READ);
				if(BCHProcess_ex((unsigned long*)pReadPyldBuffer, (unsigned long*)pEccBuf, g_npb_info.pagesize, BCH_DECODE)==1)
				{
					return SUCCESS;	
				}			
			}
			printk("ERROR : NPB_read_page(%d, %d) is error\n",blk, page);
			
			DUMP_NF_BUFFER(debug_flag, pReadPyldBuffer, 512, pEccBuf, 64);
			
			return FAILURE; 
		}
		
	}
	#if 0
	else
	{
		unsigned long*	ptr = (unsigned long*)GET_NON_CACHED_ADDR_PTR(pEccBuf);
		int i, count = g_npb_info.pagesize >> 7;
	
		for(i=0; i<count; i++)
		{
			if(ptr[i]!=0xFFFFFFFF)
			{
				printk("ERROR : ptr[%d]:0x%x!=0xFFFFFFFF\n", i, ptr[i]);
				NPB_print_buf(pEccBuf, g_npb_info.pagesize >>5);
				break;	
			}
		} 
		if(i==count)
		{
			return SUCCESS;
		}
			
		return FAILURE; 
	}
	#endif	
#endif//#ifdef _WIN32


		
	return SUCCESS;

 
}

unsigned int NPB_erase_block(unsigned short blk)
{
	unsigned long PhyAddr = g_npb_info.start_blk + blk;

#ifdef _WIN32
	NAND_EraseOneBlock(PhyAddr);
#else
	EraseBlock(PhyAddr);
#endif
	
	return SUCCESS;
}

int NPB_Test_block(unsigned short blk)
{
#if 0
	DTCM_Enable((UINT32)0x2e000000);
	int offset = g_npb_info.pagesize;
	unsigned char* preadbuf = (unsigned char*)0x9d800000;
	unsigned char* pwritebuf = (unsigned char*)(preadbuf + offset);
	unsigned char* peccbuf = (unsigned char*)g_npb_info.pEccBuf;//(unsigned char*)(pwritebuf + offset);
	
	unsigned char* preadbuf_tcm = (unsigned char*)0x2e000000;
	unsigned char* pwritebuf_tcm = (unsigned char*)(preadbuf_tcm + offset);
#else
	unsigned char* preadbuf = (unsigned char*)g_npb_info.pReadBuf_ali;
	unsigned char* pwritebuf = (unsigned char*)g_npb_info.pWriteBuf;
	unsigned char* peccbuf = (unsigned char*)g_npb_info.pEccBuf;//(unsigned char*)(pwritebuf + offset);

	unsigned char* preadbuf_tcm = (unsigned char*)Cache2NonCacheAddr(preadbuf);
	unsigned char* pwritebuf_tcm = (unsigned char*)Cache2NonCacheAddr(pwritebuf);
#endif	

	memset(pwritebuf_tcm, 0x5a, g_npb_info.pagesize);
	NF_TRACE(debug_flag, "blk=%u\n", blk);

	if(NPB_erase_block(blk)==FAILURE)
	{
		printk("[NPB_Test_block] erase error: %d!\n", blk);
		return FAILURE;	
	}	
	else if(NPB_read_page(blk, g_npb_info.page_per_block-1, preadbuf, peccbuf, 0)==FAILURE)
	{
		printk("[NPB_Test_block] read error: %d!\n", blk);
		return FAILURE;				
	}
	else if(NPB_write_page(blk, g_npb_info.page_per_block-1, pwritebuf, peccbuf)==FAILURE)
	{
		printk("[NPB_Test_block] write error: %d!\n", blk);
		return FAILURE;			
	}
	else if(NPB_read_page(blk, g_npb_info.page_per_block-1, preadbuf, peccbuf, 1)==FAILURE)
	{
		printk("[NPB_Test_block] 1read error: %d!\n", blk);
		return FAILURE;				
	}
	else if(NFTL_memcmp(pwritebuf_tcm, preadbuf_tcm, g_npb_info.pagesize) != 0)
	{
		printk("[NPB_Test_block] memcmp error: %d!\n", blk);
		return FAILURE;				
	}
	else if(NPB_erase_block(blk)==FAILURE)
	{
		printk("[NPB_Test_block] 1erase error: %d!\n", blk);
		return FAILURE;	
	}	
	return SUCCESS;

}
void NPB_CleanBlock_Writed(unsigned short blk)
{
	int i;
	
	g_npb_info.phy_blk_info[blk].count = 0;
	g_npb_info.phy_blk_info[blk].In_FIFO = 1;
	
	for(i=0; i<(g_npb_info.page_per_block>>3); i++)
	{
		g_npb_info.phy_blk_info[blk].pInfo[i] = 0;
	}
}
void NPB_Insert2fifo(unsigned short blk)
{
	FIFO_push(&g_npb_fifo, blk);	
	//Clean block & page write flag 
	NPB_CleanBlock_Writed(blk);
}

void NPB_erasetest(unsigned short blk)
{

	if(NPB_Test_block(blk)==SUCCESS)
	{
		NPB_Insert2fifo(blk);
	}
	else
	{
		printk("Warning : The block(%d) is bad!\n\n",blk);		
	}
 
}

void NPB_Erase_block(unsigned short blk)
{
	if(NPB_erase_block(blk)==SUCCESS)
	{
		NPB_Insert2fifo(blk);	
	}
	else
	{
		printk("Warning : The block(%d) is bad!\n\n",blk);		
	}
}
unsigned short NPB_find_version( NPB_list_t* p_npbl, unsigned char version)
{
	unsigned short i, size, page;
	size = NPBL_Get_len(p_npbl);
	for(i=0; i<size; i++)
	{
		page = NPBL_Get_Item(p_npbl, i);
		
		if(g_npb_info.p_p2ltab[page].version == version)	
		{
			return page;
		}
	}	
	return NO_NPB_PAGE;
}

unsigned short NPB_Get_Current_PhyPage(unsigned short logpage)
{
	unsigned short page, size, ver;
	unsigned short Current_PhyPage;
	size = g_npb_info.phyblk_count <<g_npb_info.blockshift;
	
	NPBL_Init(&g_npbl);
	

	for(page=0; page<size; page++)
	{
		if(g_npb_info.p_p2ltab[page].logpage == logpage)
		{
			printk("NPBL_Push(%d)\n",page);
			NPBL_Push(&g_npbl, page);	
		}
	}
	
	size = NPBL_Get_len(&g_npbl);
	Current_PhyPage = NO_NPB_PAGE;
	
	for(ver=1; ver<=size; ver++)
	{
		if((page=NPB_find_version(&g_npbl, ver))==NO_NPB_PAGE)
		{
			return Current_PhyPage;
		}
		Current_PhyPage	= page;
	}
	
	return NO_NPB_PAGE;
}

unsigned int NPB_IsReGetBlk(void)
{
	unsigned short page = g_npb_info.current_page;
	if(g_npb_info.current_page==NO_NPB_PAGE)
	{
		return SUCCESS;
	}
	else if((page&(g_npb_info.page_per_block-1))==0)
	{
		return SUCCESS;
	}
	return FAILURE;	
}
unsigned int NPB_Get_current_page(void)
{
	unsigned int rc; 
	unsigned short blk;
	
	
	if(NPB_IsReGetBlk()==SUCCESS)
	{
		//printk("NPB_IsReGetBlk!\n");
		if((rc=FIFO_pop(&g_npb_fifo, &blk))==FAILURE)
		{
			printk("FIFO_pop is failure!\n");
			return FAILURE;	
		}
//printk("FIFO_pop(%d).\n", blk);
		if(blk!= NO_BLOCK)
		{
			g_npb_info.phy_blk_info[blk].In_FIFO = 0;
			g_npb_info.current_page = (blk<<g_npb_info.blockshift);
			g_npb_info.current_blk = blk;
		}
		if(rc==RECOVER)
		{
			//printk("NPB_Recovery.\n");
			if(NPB_Recovery()==FAILURE)
			{
				printk("Worring : Can't recovery!\n");
				return FAILURE;//add by king
			}
		}

	}

	return SUCCESS;
}
unsigned int NPB_IsSafePage(unsigned char* pBuffer)
{
	unsigned char* ptr;
	unsigned short i;
	unsigned short len = g_npb_info.pagesize>>5;
	ptr = (unsigned char*)NPB_Cache2NonCacheAddr(pBuffer);
	
	for(i=0; i<len; i++)
	{
		if(ptr[i]!=0xff)
		{
			return FAILURE;
		}
	} 
	return SUCCESS;
}

void print_npb_info(NPB_Info_Data_t* pnpb_info)
{
	printk("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");

	printk("table_save_start_blk:%d\n", pnpb_info->table_save_start_blk);		
	printk("table_save_blk_count:%d\n", pnpb_info->table_save_blk_count);	
	printk("start_blk:%d\n", pnpb_info->start_blk);	
	printk("phyblk_count:%d\n", pnpb_info->phyblk_count);	
	printk("logblk_count:%d\n", pnpb_info->logblk_count);	
	printk("pagesize:%d\n", pnpb_info->pagesize);	
	printk("current_blk:%d\n", pnpb_info->current_blk);	
	printk("current_page:%d\n", pnpb_info->current_page);								
	printk("sector_per_page:%d\n", pnpb_info->sector_per_page);
	printk("blockshift:%d\n", pnpb_info->blockshift);
	printk("pageshift:%d\n", pnpb_info->pageshift);
	printk("sectorhift:%d\n", pnpb_info->sectorhift);
	printk("logsector_count:%d\n", pnpb_info->logsector_count);
	printk("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");	
}

unsigned int NPB_Creat_log2phy_Table(void)
{
	unsigned short blk,page;//,i;
	unsigned short logpage,phypage;
	//unsigned char version;
	unsigned int version;
//	unsigned char* ptr;
//	unsigned short len = g_npb_info.pagesize>>5;
	
//	unsigned int size;

#if NPB_SUPPORT_TABLESAVE	
	if(NPB_table_load()==SUCCESS)
	{
		printk("NPB_table_load()==SUCCESS\n");
		//printk("g_npb_info.current_blk:%d g_npb_info.current_page:%d\n", g_npb_info.current_blk, g_npb_info.current_page);		
		print_npb_info((NPB_Info_Data_t*)&g_npb_info);		
		
		return SUCCESS;
	}
#endif
	
	printk("NPB_Creat_log2phy_Table\n");
	for(blk=0; blk<g_npb_info.phyblk_count; blk++)
	{
		//printk("g_npb_info.phyblk_count:%d blk%d\n", g_npb_info.phyblk_count, blk);
		for(page=0;page<g_npb_info.page_per_block;page++)
		{
		/*
			if(blk==0 && page == 0)
				debug_flag = 1;
			else
				debug_flag = 0;
				*/
			NPB_read_page(blk, page, g_npb_info.pReadBuf_ali, g_npb_info.pEccBuf, 0);
			
			if(NPB_get_PageInfo(g_npb_info.pEccBuf, &logpage, &version)==SUCCESS)
			{
				phypage = (blk<<g_npb_info.blockshift) + page;
				if(g_npb_info.p_l2ptab[logpage].version < version)
				{
					NPB_Page_Clean_Writed(g_npb_info.p_l2ptab[logpage].phypage);
					NPB_Page_Set_Writed(phypage);

					g_npb_info.p_l2ptab[logpage].version = version;
					g_npb_info.p_l2ptab[logpage].phypage = phypage;
				}
					
				g_npb_info.p_p2ltab[phypage].logpage = logpage;
				g_npb_info.p_p2ltab[phypage].version = version;
				//NF_TRACE(debug_flag, "logpage, phypage, version:  %x %x %x\n", logpage, phypage, version);
				//DUMP_NF_BUFFER(debug_flag, g_npb_info.pReadBuf_ali, 512, g_npb_info.pEccBuf,64)
			}
			else
			{
				//NF_TRACE(debug_flag, "logpage, phypage, version:  %x %x %x\n", logpage, phypage, version);
				if(page==0)
				{
					unsigned long* pECC1 = (unsigned long*) NPB_Cache2NonCacheAddr(g_npb_info.pEccBuf);
					if(pECC1[0]==0xFFFFFFFF)
					{
						//NPB_Erase_block(blk);
						NPB_erasetest(blk);
					}
					else
					{
						printk("the block(%d) is bad.\n", blk);	
					}					
				}
				else
				{
					if(NPB_IsSafePage(g_npb_info.pEccBuf)==SUCCESS)
					{
						g_npb_info.current_page = (blk<<g_npb_info.blockshift) + page;
						printk("@@@@@@@@@@@@@@(%d, %d)\n", blk, page);
						//NPB_print_buf(g_npb_info.pEccBuf, 32);
					}
				}
				break;
			}

		}
	}


	NPB_FIFO_Get_Info();
	
	return SUCCESS;
}

unsigned int NPB_eraseall(void)
{
	int blk;
	//erase all blk and install to fifo
	
	printk("NPB_eraseall==>%d\n", g_npb_info.phyblk_count);
	for(blk=0; blk<g_npb_info.phyblk_count; blk++)
	{
		//NPB_Erase_block(blk);
		NPB_erasetest(blk);
	}
	
	FIFO_Get_Info(&g_npb_fifo);
	return SUCCESS;
}
unsigned short NPB_get_Recovery_blk(void)
{
	unsigned short i, min_pageuse;
	unsigned short blk = NO_BLOCK;
	
	min_pageuse = g_npb_info.page_per_block;
	for(i=0; i<g_npb_info.phyblk_count; i++)
	{
		if(i == g_npb_info.current_blk)
			continue;
			
		if((g_npb_info.phy_blk_info[i].In_FIFO==0) &&g_npb_info.phy_blk_info[i].count < min_pageuse)
		{
			blk = i;
			min_pageuse = g_npb_info.phy_blk_info[i].count;
		}
	}
	return blk;
}

unsigned int NPB_WritePage_fromPageAtoPageB(unsigned short Page_A,unsigned short Page_B)
{
	unsigned short blk_A, blk_B, page_A,page_B;
	blk_A = (Page_A>>g_npb_info.blockshift); 
	page_A = Page_A - (blk_A<<g_npb_info.blockshift);

	blk_B = (Page_B>>g_npb_info.blockshift); 
	page_B = Page_B - (blk_B<<g_npb_info.blockshift);
	

	NPB_read_page(blk_A, page_A, g_npb_info.pReadBuf_ali,  g_npb_info.pEccBuf, 1);

	NPB_get_PageInfo(g_npb_info.pEccBuf, &(g_npb_p2l.logpage), &(g_npb_p2l.version));

	if(g_npb_info.p_l2ptab[g_npb_p2l.logpage].version!=g_npb_p2l.version)
	{
		NPB_Page_Clean_Writed(Page_A);
		return FAILURE;
	}
	else
	{
		NPB_write_page(blk_B, page_B, g_npb_info.pReadBuf_ali,  g_npb_info.pEccBuf);
		NPB_Page_Clean_Writed(Page_A);
		g_npb_info.p_l2ptab[g_npb_p2l.logpage].phypage = Page_B;
	}
	return SUCCESS;
	
}

unsigned int NPB_Recovery_blk(unsigned short blk)
{
	unsigned short page;
	for(page=0; page < g_npb_info.page_per_block; page++)
	{
		//to do.....	
		if(NPB_IS_Page_Writed(blk, page)==SUCCESS)
		{
			unsigned short Page_A = (blk<<g_npb_info.blockshift) + page;
			unsigned short Page_B = g_npb_info.current_page;

			//printk("<%d, %d> ",Page_A, Page_B);
			NPB_WritePage_fromPageAtoPageB(Page_A, Page_B);
	
			g_npb_info.current_page++;	
			
			if(NPB_Get_current_page()==FAILURE)
				return FAILURE;	
		}
	}
	
	return SUCCESS;
}

unsigned int NPB_Recovery(void)
{
	unsigned short blk;//page
	unsigned int rc;
	blk = NPB_get_Recovery_blk();

	if(blk==NO_BLOCK)
	{
		return FAILURE;
	}

	rc = NPB_Recovery_blk(blk);
	NPB_Erase_block(blk);
	
	return rc;

}

unsigned int NPB_BadBlkMark(unsigned short blk)
{
	unsigned long u32PhyAddr = (unsigned long)(g_npb_info.start_blk + blk) << g_npb_info.blockshift;
	
	NPB_erase_block(blk);

	memset(NandCache2NonCacheAddr(g_npb_info.pWriteBuf), 0, g_npb_info.pagesize);
	memset(NandCache2NonCacheAddr(g_npb_info.pEccBuf), 0xff, 64);

	ReadWritePage(u32PhyAddr, (unsigned long*)g_npb_info.pWriteBuf, (unsigned long*)g_npb_info.pEccBuf, NF_WRITE);		
	return 0;
}
//for test	
//unsigned int gXXXXbak=0xffffffff;
//unsigned int gXXXXbakcountXXXXXX=0;

void NFTL_memset(unsigned char* ptr, unsigned char ch, unsigned int size);
unsigned int NPB_ReadPage(unsigned short logpage, unsigned char* pReadPyldBuffer)
{
	unsigned short blk, page;
	unsigned short phypage;

//printk("KING->NPB_ReadPage(%d)\n", logpage);
#if 0
	if(g_isFirst != 1)
	{
		cyg_flag_wait(&g_NandRWFlag, 0x1, CYG_FLAG_WAITMODE_OR );
	}
#endif
	 phypage = g_npb_info.p_l2ptab[logpage].phypage;

	if(g_npb_info.p_l2ptab[logpage].version==0)
	{
		NFTL_memset(pReadPyldBuffer, 0, g_npb_info.pagesize);
		NF_TRACE(debug_flag, "g_npb_info.p_l2ptab[%d].version==0\n", logpage);
		return SUCCESS;
		//return FAILURE;	
	}
	
	if(phypage > (NAND_PGBASE_USED_PHYBLK<<g_npb_info.blockshift))
	{
		
		printk("ERROR : NPB_read_logpage(%d, %d) is failure!\n", logpage, phypage);
		printk("NAND_PGBASE_USED_PHYBLK:%d\n", NAND_PGBASE_USED_PHYBLK);
		printk("g_npb_info.blockshift:%d\n", g_npb_info.blockshift);
		return FAILURE;			
	}

	blk = (phypage>>g_npb_info.blockshift);

	page = phypage - (blk<<g_npb_info.blockshift);
/* modify by mm.li 01-12,2011 clean warning */
	/*
	if(debug_flag & 0x301 == 0x301)
	*/
	if((debug_flag & 0x301) == 0x301)
/* modify end */	
	{
		SPMP_DEBUG_PRINT("g_npb_info.blockshift=%u, phypage=%x\n", g_npb_info.blockshift, phypage);
	}

	if(NPB_read_page(blk, page, pReadPyldBuffer, g_npb_info.pEccBuf, 1)==FAILURE)
	{
		printk("ERROR : NPB_read_page(%d, %d) is failure!\n", logpage, phypage);
		printk("NPB_Recovery_blk(%d)!\n",blk);
		NPB_Recovery_blk(blk);
		NPB_CleanBlock_Writed(blk);
		NPB_BadBlkMark(blk);	
		return FAILURE;
	}
	
#if 0//for test	
	if(gXXXXbak == 0)
	{
		printk("KING->XXXXXXXXXXXXXXXXXXXXxxx\n");
		
	}
	
	if((gXXXXbakcountXXXXXX++ > 10) &&(gXXXXbak==0xffffffff))
	{
		printk("KING->NPB_read_page(%d, %d) is failure!\n", logpage, phypage);
		printk("KING->NPB_Recovery_blk(%d)!\n",blk);
		NPB_Recovery_blk(blk);
		NPB_CleanBlock_Writed(blk);
		NPB_BadBlkMark(blk);	
		gXXXXbak = blk;	
	}
#endif	
	
	return SUCCESS;
}

void NPB_print_p_l2ptab(void)
{
	unsigned short i;
	
	for(i=0; i<(g_npb_info.logblk_count<<g_npb_info.blockshift); i++)
	{
		printk("g_npb_info.p_l2ptab[%d].phypage:%d\n", i, g_npb_info.p_l2ptab[i].phypage);	
	}	
}

unsigned int NPB_WritePage(unsigned short logpage, unsigned char* pWritePyldBuffer)
{
	unsigned short blk, page;
	unsigned short phypage;

	//printk("NPB_write_logpage->logpage:%d\n",logpage);

#if 0
	if(g_isFirst != 1)
	{
		cyg_flag_wait(&g_NandRWFlag, 0x1, CYG_FLAG_WAITMODE_OR );
	}
#endif

	if(NPB_Get_current_page()==FAILURE)
		return FAILURE;
		
	//phypage = g_npb_info.current_page;	

	if(g_npb_info.p_l2ptab[logpage].phypage!=NO_NPB_PAGE)
	{
		phypage = g_npb_info.p_l2ptab[logpage].phypage;

		if(phypage> (g_npb_info.phyblk_count<<g_npb_info.blockshift))
		{
			printk("ERROR : NPB_write_logpage(%d, %d, %d)\n", logpage, g_npb_info.current_page, phypage);
			NPB_print_p_l2ptab();
			return FAILURE;
		}
		NPB_Page_Clean_Writed(phypage);
	}


	phypage = g_npb_info.current_page;
	if(phypage==NO_NPB_PAGE)
	{
		printk("NPB_write_logpage->phypage:%d\n",phypage);
		if(NPB_Get_current_page()==FAILURE)
			return FAILURE;		
	}
	
	phypage = g_npb_info.current_page;
	
	g_npb_info.p_l2ptab[logpage].phypage = phypage;
	
	g_npb_info.p_l2ptab[logpage].version++;
		
	g_npb_p2l.logpage = logpage;
	g_npb_p2l.version = g_npb_info.p_l2ptab[logpage].version;
	
	blk = (phypage>>g_npb_info.blockshift);
	page = phypage - (blk<<g_npb_info.blockshift);
	
	NPB_write_page(blk, page, pWritePyldBuffer, g_npb_info.pEccBuf);

	g_npb_info.current_page++;

	return SUCCESS;
}

void prin_NPB_Info(void)
{
	printk("###################NPB_Info##########################\n");
	printk("g_npb_info.table_save_start_blk:%d\n",g_npb_info.table_save_start_blk);
	printk("g_npb_info.table_save_blk_count:%d\n",g_npb_info.table_save_blk_count);
	printk("g_npb_info.start_blk:%d\n",g_npb_info.start_blk);
	printk("g_npb_info.phyblk_count:%d\n",g_npb_info.phyblk_count);
	printk("g_npb_info.page_per_block:%d\n",g_npb_info.page_per_block);
	printk("g_npb_info.pagesize:%d\n",g_npb_info.pagesize);	
	printk("g_npb_info.blockshift:%d\n",g_npb_info.blockshift);
	printk("g_npb_info.pageshift:%d\n",g_npb_info.pageshift);
	printk("g_npb_info.sectorhift:%d\n",g_npb_info.sectorhift);
	printk("g_npb_info.sector_per_page:%d\n",g_npb_info.sector_per_page);
	printk("g_npb_info.current_page:%d\n",g_npb_info.current_page);	
	printk("g_npb_info.logblk_count:%d\n",g_npb_info.logblk_count);	
	printk("g_npb_info.logsector_count:%d\n",g_npb_info.logsector_count);	
	printk("####################################################\n");		
}
unsigned short Get_NPB_log_sector_count(void)
{
	return 	g_npb_info.logsector_count;
}
unsigned short Get_NPB_logpage_count(void)
{
	return 	g_npb_info.logblk_count<<g_npb_info.blockshift;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if NPB_SUPPORT_TABLESAVE
unsigned int NPB_table_write_page(unsigned short blk, unsigned short page,unsigned char* pWritePyldBuffer)
{
	unsigned long u32PhyAddr;

	if(blk >= g_npb_info.table_save_blk_count)
	{
		printk("ERROR : NPB_table_save_write_page-> blk(%d) >= g_npb_info.table_save_blk_count(%d)\n", blk, g_npb_info.table_save_blk_count);
		return FAILURE;
	}

	u32PhyAddr = (unsigned long)(g_npb_info.table_save_start_blk + blk) << g_npb_info.blockshift;
	u32PhyAddr += page;	
	
	NPB_Cachesync();
	
	BCHProcess_ex((unsigned long*)pWritePyldBuffer, (unsigned long*)g_npb_info.pEccBuf, g_npb_info.pagesize, BCH_ENCODE);
	ReadWritePage(u32PhyAddr, (unsigned long*)pWritePyldBuffer, (unsigned long*)g_npb_info.pEccBuf, NF_WRITE);

	//NPB_switch_buf();

 	return SUCCESS;
}

unsigned int  NPB_table_read_page(unsigned short blk, unsigned short page, unsigned char* pReadPyldBuffer)
{
	unsigned long u32PhyAddr;

	if(blk >= g_npb_info.table_save_blk_count)
	{
		return FAILURE; 
	}

	u32PhyAddr = (unsigned long)(g_npb_info.table_save_start_blk + blk) << g_npb_info.blockshift;

	u32PhyAddr += page;	

	ReadWritePage(u32PhyAddr, (unsigned long*)pReadPyldBuffer, (unsigned long*)g_npb_info.pEccBuf, NF_READ);
	
	if(BCHProcess_ex((unsigned long*)pReadPyldBuffer, (unsigned long*)g_npb_info.pEccBuf, g_npb_info.pagesize, BCH_DECODE)!=1)
	{
		int retry;
		retry = RETRY_COUNT;
		while(retry--)
		{
			ReadWritePage(u32PhyAddr, (unsigned long*)pReadPyldBuffer, (unsigned long*)g_npb_info.pEccBuf, NF_READ);
			if(BCHProcess_ex((unsigned long*)pReadPyldBuffer, (unsigned long*)g_npb_info.pEccBuf, g_npb_info.pagesize, BCH_DECODE)==1)
			{
				return SUCCESS;	
			}			
		}
		return FAILURE; 
	}
	return SUCCESS;
}

void NPB_table_erase_blk(unsigned short blk)
{
	unsigned long u32Phyblk;

	if(blk >= g_npb_info.table_save_blk_count)
	{
		return; 
	}

	u32Phyblk = g_npb_info.table_save_start_blk + blk;

	EraseBlock(u32Phyblk);

	//return SUCCESS;	
}
void NPB_table_erase(void)
{
	
	if(Is_enable_erase_savetable)
	{
		printk("NPB_table_erase()==SUCCESS\n");
		NPB_table_erase_blk(0);
		Is_enable_erase_savetable = 0;	
	}

}

unsigned short NPB_table_savebody( unsigned short blk, unsigned short page_offset, unsigned char* ptr, unsigned int size)
{
	//
	unsigned int i, count;
	//NPB_print_buf(ptr, 64);
	
	count = (size+g_npb_info.pagesize-1) >>  g_npb_info.pageshift;
	for(i=0;i<count;i++)
	{	
		memcpy(g_npb_info.pWriteBuf,(unsigned char*)(((unsigned long) ptr)+i*g_npb_info.pagesize),g_npb_info.pagesize);
		NPB_table_write_page(blk, page_offset, g_npb_info.pWriteBuf);
		page_offset++;
	}
		
	return page_offset;
}

unsigned int NPB_table_save(void)
{
	NPB_Store_Info_t npb_store_info;
	unsigned int size;	
	unsigned short blk = 0;
	unsigned short page_offset = 0;

	npb_store_info.heard = NPB_HEARD;
	npb_store_info.version = 0;
	npb_store_info.pattern = NPB_PATTERN;
	npb_store_info.reserved = 0;
	
	if(Is_enable_erase_savetable)
		return SUCCESS;
		
		//g_npb_info.current_page
	printk("g_npb_info.current_blk:%d g_npb_info.current_page:%d\n", g_npb_info.current_blk, g_npb_info.current_page);
	
	//p_l2ptab
	size = (g_npb_info.logblk_count<<g_npb_info.blockshift) * sizeof(NPB_Log2Phy_t);
	//printk("p_l2ptab size:%d page_offset:%d\n", size, page_offset);
	page_offset = NPB_table_savebody(blk, page_offset, (unsigned char *)g_npb_info.p_l2ptab, size);
		
	//p_p2ltab
	size = (g_npb_info.phyblk_count<<g_npb_info.blockshift) * sizeof(NPB_Phy2Log_t);
	//printk("p_p2ltab size:%d page_offset:%d\n", size, page_offset);
	page_offset = NPB_table_savebody(blk, page_offset, (unsigned char *)g_npb_info.p_p2ltab, size);
		
	//phy_blk_info
	size = g_npb_info.phyblk_count * sizeof(NPB_Phy_blk_Info_t);
	//printk("phy_blk_info size:%d page_offset:%d\n", size, page_offset);
	page_offset = NPB_table_savebody(blk, page_offset, (unsigned char *)g_npb_info.phy_blk_info, size);
	
	//NPB_FIFO_Manage_t g_npb_fifo;
	size = sizeof(FIFO_Manage_Data_t);
	//printk("g_npb_fifo size:%d page_offset:%d\n", size, page_offset);	
	page_offset = NPB_table_savebody(blk, page_offset, (unsigned char *)(&g_npb_fifo), size);
		
	size = g_npb_fifo.size * sizeof(unsigned short);
	//printk("g_npb_fifo.plist size:%d g_npb_fifo:%d\n", size, page_offset);	
	page_offset = NPB_table_savebody(blk, page_offset, (unsigned char *)g_npb_fifo.plist, size);

	size = sizeof(NPB_Info_Data_t);
	page_offset = NPB_table_savebody(blk, page_offset, (unsigned char *)(&g_npb_info), size);
	//write success flag to last page
	NPB_table_savebody(blk, g_npb_info.page_per_block-1, (unsigned char *)(&npb_store_info), sizeof(NPB_Store_Info_t));
	
	Is_enable_erase_savetable = 1;
	
	printk("NPB_table_save()==SUCCESS\n");
	print_npb_info((NPB_Info_Data_t*)&g_npb_info);

	return SUCCESS;
}

unsigned short NPB_table_loadbody( unsigned short blk, unsigned short page_offset, unsigned char* ptr, unsigned int size)
{
	//
	unsigned int i, count;
	
	count = (size+g_npb_info.pagesize-1) >>  g_npb_info.pageshift;
	for(i=0;i<count;i++)
	{	
		if(NPB_table_read_page(blk, page_offset, g_npb_info.pReadBuf_ali)==FAILURE)
		{
			return FAILURE;	
		}
		//if(i==0)
			//NPB_print_buf(NPB_Cache2NonCacheAddr(g_npb_info.pReadBuf_ali), 64);
		if(size > g_npb_info.pagesize)
		{
			memcpy((unsigned char*)(((unsigned long) ptr)+i*g_npb_info.pagesize), g_npb_info.pReadBuf_ali_NC, g_npb_info.pagesize);	
			size -= g_npb_info.pagesize;	
		}
		else
		{
			memcpy((unsigned char*)(((unsigned long) ptr)+i*g_npb_info.pagesize), g_npb_info.pReadBuf_ali_NC, size);			
		}
		page_offset++;
	}
		
	return page_offset;
}

unsigned int NPB_table_load(void)
{
	NPB_Store_Info_t* p_npb_store_info;
	unsigned short blk = 0;
	int  size;//,i, count;
	unsigned short page_offset = 0;
	NPB_Info_Data_t npb_info_data;

	if(NPB_table_read_page(blk, g_npb_info.page_per_block-1, g_npb_info.pReadBuf_ali) == FAILURE)
	{
		printk("Can't found NPB table.\n");
		return FAILURE;
	}	

	
	p_npb_store_info = (NPB_Store_Info_t*)NPB_Cache2NonCacheAddr(g_npb_info.pReadBuf_ali);
	if((p_npb_store_info->heard!= NPB_HEARD) || (p_npb_store_info->pattern != NPB_PATTERN))
	{
		printk("p_npb_store_info->heard:0x%x != NPB_HEARD:0x%x\n", p_npb_store_info->heard, NPB_HEARD);
		printk("p_npb_store_info->pattern:0x%x != NPB_PATTERN:0x%x\n", p_npb_store_info->pattern, NPB_PATTERN);
		return FAILURE;
	}	

	//p_l2ptab
	size = (g_npb_info.logblk_count<<g_npb_info.blockshift) * sizeof(NPB_Log2Phy_t);
	//printk("pNPB_table_load -> p_l2ptab size:%d page_offset:%d\n", size, page_offset);
	if((page_offset = NPB_table_loadbody(blk, page_offset, (unsigned char *)g_npb_info.p_l2ptab, size))== FAILURE)
	{
		printk("NPB_table_loadbody p_l2ptab is FAILURE\n");
		return FAILURE;
	}

	//p_p2ltab
	size = (g_npb_info.phyblk_count<<g_npb_info.blockshift) * sizeof(NPB_Log2Phy_t);
	//printk("pNPB_table_load -> p_p2ltab size:%d page_offset:%d\n", size, page_offset);
	if((page_offset = NPB_table_loadbody(blk, page_offset, (unsigned char *)g_npb_info.p_p2ltab, size))== FAILURE)
	{
		printk("NPB_table_loadbody p_p2ltab is FAILURE\n");
		return FAILURE;
	}
	
	//g_npb_info.phy_blk_info
	size = g_npb_info.phyblk_count * sizeof(NPB_Phy_blk_Info_t);
	//printk("pNPB_table_load -> phy_blk_info size:%d page_offset:%d\n", size, page_offset);
	if((page_offset = NPB_table_loadbody(blk, page_offset, (unsigned char *)g_npb_info.phy_blk_info, size))== FAILURE)
	{
		printk("NPB_table_loadbody phy_blk_info is FAILURE\n");
		return FAILURE;
	}
		
	
	//FIFO_Manage_Data_t g_npb_fifo;
	size = sizeof(FIFO_Manage_Data_t);
	//printk("pNPB_table_load -> g_npb_fifo size:%d page_offset:%d\n", size, page_offset);
	if((page_offset = NPB_table_loadbody(blk, page_offset, (unsigned char *)(&g_npb_fifo), size))== FAILURE)
	{
		printk("NPB_table_loadbody FIFO_Manage_Data_t is FAILURE\n");
		return FAILURE;
	}
	
	size = g_npb_fifo.size * sizeof(unsigned short);
	//printk("pNPB_table_load -> g_npb_fifo.plist size:%d page_offset:%d\n", size, page_offset);
	if((page_offset = NPB_table_loadbody(blk, page_offset, (unsigned char *)g_npb_fifo.plist, size))== FAILURE)
	{
		printk("NPB_table_loadbody FIFO_Manage_t is FAILURE\n");
		return FAILURE;
	}

#if 0
		size = sizeof(NPB_Info_Data_t);
		if((page_offset = NPB_table_loadbody(blk, page_offset, &g_npb_info, size))== FAILURE)
		{
			printk("NPB_table_loadbody g_npb_info is FAILURE\n");
			return FAILURE;
		}
#else
		
		size = sizeof(NPB_Info_Data_t);
		if((page_offset = NPB_table_loadbody(blk, page_offset, (unsigned char *)(&npb_info_data), size))== FAILURE)
		{
			printk("NPB_table_loadbody g_npb_info is FAILURE\n");
			return FAILURE;
		}
	
		if(npb_info_data.table_save_start_blk!=g_npb_info.table_save_start_blk)
		{
			printk("npb_info_data.table_save_start_blk:%d!= g_npb_info.table_save_start_blk:%d\n",
				npb_info_data.table_save_start_blk, g_npb_info.table_save_start_blk);
			return FAILURE;
		}
		if(npb_info_data.table_save_blk_count!=g_npb_info.table_save_blk_count)
		{
			printk("npb_info_data.table_save_blk_count:%d!= g_npb_info.table_save_blk_count:%d\n",
				npb_info_data.table_save_blk_count, g_npb_info.table_save_blk_count);
			return FAILURE;
		}
		if(npb_info_data.phyblk_count!=g_npb_info.phyblk_count)
		{
			printk("npb_info_data.phyblk_count:%d!= g_npb_info.phyblk_count:%d\n",
				npb_info_data.phyblk_count, g_npb_info.phyblk_count);
			return FAILURE;
		}
		if(npb_info_data.logblk_count!=g_npb_info.logblk_count)
		{
			printk("npb_info_data.logblk_count:%d!= g_npb_info.logblk_count:%d\n",
				npb_info_data.logblk_count, g_npb_info.logblk_count);
			return FAILURE;
		}
	
		
		memcpy((unsigned char*)&g_npb_info, (unsigned char*)&npb_info_data, sizeof(NPB_Info_Data_t)); 

	//printk("print_npb_info(&g_npb_info)\n");
	//printf_npb_info(&g_npb_info);

	printk("printf_npb_info(&npb_info)\n");
	print_npb_info(&npb_info_data);
#endif



	return SUCCESS; 

}
#endif

extern NFFS_Block_info_t* getmfsxx_block_info(void);
unsigned int NPB_Init(unsigned char Is_First)
{
	NFFS_Block_info_t* nfs_block_info = getmfsxx_block_info();

	//g_NandRWFlag.flag= 0;

	g_npb_info.table_save_start_blk = nfs_block_info->npb_block.start;
	g_npb_info.table_save_blk_count = NAND_PGBASE_SAVEBLK;
	
	g_npb_info.start_blk = g_npb_info.table_save_start_blk + g_npb_info.table_save_blk_count;
	g_npb_info.phyblk_count = nfs_block_info->npb_block.count - g_npb_info.table_save_blk_count;
	g_npb_info.page_per_block = nfs_block_info->page_per_block;
	g_npb_info.pagesize = nfs_block_info->page_size;
	
	g_npb_info.blockshift = NPB_nfvalshift(g_npb_info.page_per_block);
	g_npb_info.pageshift = NPB_nfvalshift(g_npb_info.pagesize);
	g_npb_info.sectorhift = NPB_nfvalshift(NPB_SECTOR_SIZE);
	g_npb_info.sector_per_page = (g_npb_info.pagesize>>g_npb_info.sectorhift);
	g_npb_info.current_page = NO_NPB_PAGE;
	
	g_npb_info.logsector_count = NAND_PGBASE_USED_LOGBLK<<(g_npb_info.blockshift+g_npb_info.pageshift-g_npb_info.sectorhift);
	
	g_npb_info.logblk_count = NAND_PGBASE_USED_LOGBLK;
	
	printk("NPB_Init-->Is_First:%d\n",Is_First);
	printk("NAND_PGBASE_USED_LOGSECTOR:%d\n",NAND_PGBASE_USED_LOGSECTOR);
	prin_NPB_Info();

	g_npb_info.p_l2ptab = NULL;

	if(FIFO_Init(&g_npb_fifo, g_npb_info.phyblk_count) == FAILURE)
	{
		printk("FIFO Init is failure!\n");
		//flag_setbits(&(g_NandInitFlag.flag), 0x01);
		//wake_up_interruptible(&(g_NandInitFlag.queue));
		return FAILURE;
	}

	if(NPB_Info_Init() == FAILURE)
	{
		printk("NPB_Info Init is failure!\n");
		
		//flag_setbits(&(g_NandInitFlag.flag), 0x01);
		//wake_up_interruptible(&(g_NandInitFlag.queue));
		return FAILURE;
	}

	if(Is_First)
	{
		NPB_eraseall();
		g_isFirst = 1;
	}
#if 0	
	else
	{
		printk("NPB_Creat_log2phy_Table!\n");
		
		NPB_Creat_log2phy_Table();
		FIFO_Get_Info(&g_npb_fifo);	
	}
#endif	

	//flag_setbits(&(g_NandInitFlag.flag), 0x01);
	
	return SUCCESS;
}
void NPB_FIFO_Get_Info(void)
{
	FIFO_Get_Info(&g_npb_fifo);
}
unsigned int NPB_free(void)
{
	FIFO_free(&g_npb_fifo);
	
	NPB_Info_free();

	return SUCCESS;
}
