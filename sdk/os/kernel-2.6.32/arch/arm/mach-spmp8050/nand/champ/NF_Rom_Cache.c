#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/string.h>



#include "NF_cfgs.h"
#include "../hal/nf_s330.h"
#include "NF_Rom_Cache.h"
#include "storage_op.h"


#ifndef CYGPKG_REDBOOT
	#define SUPPORT_MALLOC 1
	#define NFRC_CACHE_ON_ 0
#else
	#define SUPPORT_MALLOC 0
	#define NFRC_CACHE_ON_ 1
#endif


#define NFRC_ALIGNED64 64
#define NFRC_MAXBUFSIZE (4096 + NFRC_ALIGNED64)
#define NFRC_MAXBUFSIZE_ECC (128 + NFRC_ALIGNED64)
#if SUPPORT_MALLOC
#else  
//unsigned char  __attribute__ ((aligned (64))) g_nfrc_Readbuf[NFRC_MAXBUFSIZE];
//unsigned char  __attribute__ ((aligned (64))) g_nfrc_Buf_Cache[NFRC_MAXBUFSIZE];
//unsigned char  __attribute__ ((aligned (64))) g_nfrc_ECCBuffer1[NFRC_MAXBUFSIZE_ECC];
//unsigned char  __attribute__ ((aligned (64))) g_nfrc_ECCBuffer2[NFRC_MAXBUFSIZE_ECC];
//unsigned short  __attribute__ ((aligned (64))) g_nfrc_BM_Tab[MAX_BM_TAB_SIZE];
//unsigned short  __attribute__ ((aligned (64))) g_nfrc_BM_Tab[MAX_BM_TAB_SIZE];
//#if SUPPORT_DUAL_ROM_IMAGE	
//unsigned short  __attribute__ ((aligned (64))) g_nfrc1_BM_Tab[MAX_BM_TAB_SIZE];		
//#endif
#endif

NFRC_Info_t nfrc;
NFRC_Block_Info_t g_nbi;
//#if SUPPORT_DUAL_ROM_IMAGE
NFRC_Block_Info_t g_nbi1;
//#endif
unsigned int g_IsInit_nfrc = 0;

#ifndef CYGPKG_REDBOOT
NFRC_Block_Info_t g_nbi_a;
#endif

#if SUPPORT_NFRCCACHE
NFRC_Cache_t g_nfrc_cache;
NFRC_Cache_t g_nfrc_cache_fs1;
#endif

NFRC_Compress_Buf_Info_t g_nfrc_compress_buf;

//////////////////////////////////////////////////////////////////////////////////



#ifdef	CYGPKG_REDBOOT
#define ENABLE_MUTEX_LOCK_NFRC 0
//#define NFRC_mutex_t cyg_int32
#else
#include "../hal/hal_base.h"//for cyg_mutex_t
#define ENABLE_MUTEX_LOCK_NFRC 1
//#define NFRC_mutex_t cyg_mutex_t
static nf_mutex_t nfrc_mutex_luck;
#endif



int g_Is_NFRC_mutex_init = 0;

void NFRC_mutex_init(void) 
{
#if ENABLE_MUTEX_LOCK_NFRC	
	if(g_Is_NFRC_mutex_init==0)
	{
		g_Is_NFRC_mutex_init =1;
		init_MUTEX(&nfrc_mutex_luck);
	}
#endif
}

void NFRC_mutex_lock(void)
{
#if ENABLE_MUTEX_LOCK_NFRC
	//down_interruptible(&nfrc_mutex_luck);	
#endif	
}
void NFRC_mutex_unlock(void)
{
#if ENABLE_MUTEX_LOCK_NFRC
	//up(&nfrc_mutex_luck);	
#endif	
}
void NFRC_mutex_destroy(void)
{
#if ENABLE_MUTEX_LOCK_NFRC

#endif	
}
//////////////////////////////////////////////////////////////////////////////////
unsigned int currNFRCAddr = NFRC_START_ADDR;
unsigned int getNFRCBuff(unsigned int aSize, unsigned int aMsk)
{
	unsigned int retAddr = currNFRCAddr;
	unsigned int msk = aMsk;
	unsigned int nextAddr = 0;
	if(0 == msk){
		msk = 0x0000003F;
	}

	retAddr += msk;
	retAddr &= (~msk);
	nextAddr = retAddr + aSize;
	
//printk("nextAddr:0x%x ", nextAddr);	
	if(nextAddr > NFRC_END_ADDR){
		retAddr = 0;
	}
	else{
		currNFRCAddr = nextAddr;
	}
	printk("retAddr:0x%x ", retAddr);
	printk("currNFRCAddr:0x%x\n", currNFRCAddr);
	return retAddr;	
	
}

/////////////////////////////////////////////////////////////////////////////////


unsigned char* NFRC_Cache2NonCacheAddr(unsigned char* addr)
{
#if NFRC_CACHE_ON_	
	return (unsigned char *)(((unsigned long)(addr)) | 0x10000000);
#else
	return (unsigned char *)((unsigned long)(addr));
#endif		
}

void NFRC_print_buf(unsigned char* buf, unsigned int len)
{
//#ifndef	CYGPKG_REDBOOT
	int i;
	unsigned char* buf_NC;
	
	//buf_NC = NFRC_Cache2NonCacheAddr(buf);
	buf_NC = buf;

	printk("buf:0x%x len:%d\n", (unsigned int)buf_NC, len);
	for(i=0;i<len;i++)
	{
		if(i%16==0)
		{
			printk("\n");
			printk("0x%04x==> ",i);	
		}
		else if(i%8==0)
		{
			printk("- ");	
		}
		printk("%02x ",buf_NC[i]);
	}		
	printk("\n");
//#endif
}


unsigned short NFRC_Get_Page_Aling_Mask(void)
{
	return (nfrc.sectors_per_page - 1);
}

void Print_NFRC_INFO(void)
{   	
    printk("**********************NFRC INFO*************************\n");
	printk("nfrc.rom.start : %u\n",nfrc.rom.start);
	printk("nfrc.rom.count : %u\n",nfrc.rom.count);
	printk("nfrc.rom1.start : %u\n",nfrc.rom1.start);
	printk("nfrc.rom1.count : %u\n",nfrc.rom1.count);
	printk("nfrc.rom_a.start : %u\n",nfrc.rom_a.start);
	printk("nfrc.rom_a.count : %u\n",nfrc.rom_a.count);	
	//printk("nfrc.start : %d\n",nfrc.block_count);	
	printk("nfrc.block_count : %u\n",nfrc.block_count);	
//	printk("nfrc.phy_block_count : %d\n",nfrc.phy_block_count);					
	printk("nfrc.page_per_block : %u\n",nfrc.page_per_block);	
	printk("nfrc.pagesize : %u\n",nfrc.pagesize);	
	printk("nfrc.phy_pagesize : %u\n",nfrc.phy_pagesize);						
	printk("nfrc.sectors_per_page : %u\n",nfrc.sectors_per_page);	
	printk("nfrc.u8Support_TwoPlan : %u\n",nfrc.u8Support_TwoPlan);
	printk("nfrc.u8Support_Internal_Interleave : %u\n",nfrc.u8Support_Internal_Interleave);
	printk("nfrc.u8Support_External_Interleave : %u\n",nfrc.u8Support_External_Interleave);
	printk("nfrc.u8Internal_Chip_Number : %u\n",nfrc.u8Internal_Chip_Number);	
	printk("********************************************************\n");
}
unsigned int g_IsFirstGetMaxRomSize = 0;
#define MAX_READCOUNT 64
unsigned int NFRC_Get_MaxRomSize(void)
{
	int i, PageAddr;
	NFRC_Block_Info_t* p_nbi;
	int pageshift = 0;
	//unsigned int PageSize;
	
	if(g_IsFirstGetMaxRomSize)
	{
		printk("[ST]g_nbi.RomFs_Max_BlkCount : %u\n",g_nbi.RomFs_Max_BlkCount);
		return g_nbi.RomFs_Max_BlkCount;
	}
	
	g_IsFirstGetMaxRomSize = 1; 

	pageshift = nfvalshift(nfrc.pagesize);
	g_nbi.RomFs_Max_BlkCount = ROMFS_MAX_SIZE>>(nfrc.blockshift+pageshift);// - (nfrc.u8Support_TwoPlan+nfrc.u8Internal_Chip_Number+nfrc.u8Support_Internal_Interleave));

	//i = 0;
	for(i=0; i<MAX_READCOUNT; i++)
	{	
		PageAddr = i <<nfrc.blockshift;
		if(NFRC_ReadPage_ex(PageAddr, nfrc.Readbuf_ali, FLG_ROMFS)==SUCCESS)
		{
			p_nbi = (NFRC_Block_Info_t*)nfrc.Readbuf_ali_NC;
			
			//printk("p_nbi->heard : %d\n",p_nbi->heard);
			//printk("p_nbi->version : %d\n",p_nbi->version);
			//printk("p_nbi->size : %d\n",p_nbi->size);
			//printk("p_nbi->RomFs_Max_BlkCount : %d\n",p_nbi->RomFs_Max_BlkCount);			
			
			if(p_nbi->heard == NFRC_HEARD_PATTERN)  
			{			
				g_nbi.heard = p_nbi->heard;
				g_nbi.version = p_nbi->version;
				g_nbi.size = p_nbi->size;
				
				if(g_nbi.version>NFRC_VERSION)	
				{
					g_nbi.RomFs_Max_BlkCount = p_nbi->RomFs_Max_BlkCount;
					g_nbi.RomFs1_Max_BlkCount = p_nbi->RomFs1_Max_BlkCount;
				}	
				break;
			}
		}
		else
		{
			printk("NFRC_ReadPage_ex(%d)==fail.\n",PageAddr);
		}
	}
	printk("g_nbi.RomFs_Max_BlkCount : %d\n",g_nbi.RomFs_Max_BlkCount);
	
	return g_nbi.RomFs_Max_BlkCount;
}
unsigned int NFRC_Get_RomFs_Max_BlkCount(void)
{
	unsigned int PageSize;
	
	NFRC_Init(&PageSize);
	
	return g_nbi.RomFs_Max_BlkCount;	
}

unsigned int NFRC_Get_RomFs1_Max_BlkCount(void)
{
	return g_nbi.RomFs1_Max_BlkCount;	
}
void NFRC_Set_Block_Info(psysinfo_t* psysinfo)
{
	int pageshift;
	
	//unsigned int MaxRomSize;

	nfrc.u8Support_TwoPlan 					= 	psysinfo->u8Support_TwoPlan;//u8NFChannel; 
	nfrc.u8Support_Internal_Interleave		= 	psysinfo->u8Support_Internal_Interleave;
	nfrc.u8Support_External_Interleave		=	psysinfo->u8Support_External_Interleave;
	nfrc.u8Internal_Chip_Number		      	=	psysinfo->u8Internal_Chip_Number;	


	nfrc.page_per_block = psysinfo->u16PageNoPerBlk;
	
	nfrc.blockshift = nfvalshift(nfrc.page_per_block);

	//only support single CS
	//nfrc.pagesize = psysinfo->u16PyldLen << psysinfo->u8Internal_Chip_Number << psysinfo->u8MultiChannel;//test by king weng
	nfrc.pagesize = psysinfo->u16PyldLen ;

	pageshift = nfvalshift(nfrc.pagesize);

	nfrc.sectors_per_page = nfrc.pagesize >> 9;
	
	nfrc.phy_pagesize = psysinfo->u16PyldLen;
	
	nfrc.sys.start = 0;	
	nfrc.sys.count = NFFS_SYS_BLOCK_SIZE;

	nfrc.rom.start = nfrc.sys.start + nfrc.sys.count;	

}


void NFRC_Set_Block_Info_ex(psysinfo_t* psysinfo)
{
	//int pageshift;
	
	unsigned int MaxRomSize;

	MaxRomSize = NFRC_Get_MaxRomSize();

	nfrc.page_addr = -1;

	nfrc.sys.start = 0;	
	nfrc.sys.count = NFFS_SYS_BLOCK_SIZE;

	nfrc.rom.start = nfrc.sys.start + nfrc.sys.count;	
//	nfrc.rom.count=NFFS_ROM_BLOCK_SIZE;
	//nfrc.rom.count = ROMFS_MAX_SIZE>>(nfrc.blockshift+pageshift);
	nfrc.rom.count = MaxRomSize;//>>(nfrc.blockshift+pageshift);

	nfrc.rom1.start = nfrc.rom.start + nfrc.rom.count;	

//#if SUPPORT_DUAL_ROM_IMAGE
//	nfrc.rom1.count = nfrc.rom.count;
//#else
//	nfrc.rom1.count = 0 ;
//#endif
	//if(getSDevinfo(SMALLBLKFLG)==0)
	if(IsDualRomImage()==SUCCESS)
	{
		nfrc.rom1.count = nfrc.rom.count;
	}
	else
	{
		nfrc.rom1.count = 0;
	}

	//nfrc.bm.start = nfrc.rom1.start + nfrc.rom1.count;	
	//nfrc.bm.count = NFFS_BM_BLOCK_SIZE;
	
	nfrc.rom_a.start = nfrc.rom1.start + nfrc.rom1.count;
	nfrc.rom_a.count = g_nbi.RomFs1_Max_BlkCount;
	
	nfrc.user.start = nfrc.rom_a.start + nfrc.rom_a.count;
	nfrc.user.count =  psysinfo->u16TotalBlkNo - nfrc.user.start;

	nfrc.block_count = nfrc.rom.count;

	Print_NFRC_INFO();
}

#if 0
//unsigned int NFRC_Init(void)
unsigned int NFRC_Init_ex(NFRC_Nand_Info_t* Info)
{
	unsigned int PageSize;
	
	NFRC_Init(&PageSize);
	                                                 
	Info->block_count       = nfrc.block_count;     
	Info->page_per_block    = nfrc.page_per_block;  
	Info->pagesize          = nfrc.pagesize;        
	Info->sectors_per_page  = nfrc.sectors_per_page;
	Info->page_addr         = nfrc.page_addr;       
	Info->blockshift	    = nfrc.blockshift;	

	Info->phy_pagesize		= nfrc.phy_pagesize;       
//	Info->phy_block_count	= nfrc.phy_block_count;	
	
	return SUCCESS;   
	
}
#endif
unsigned int NFRC_Init(unsigned int* PageSize)
{
	unsigned int size;
	psysinfo_t* psysinfo = NULL;
	//unsigned char  u8CSR = 0x09;
	//unsigned short u16InterMask = 0xf05d; //0xf05c;
	//unsigned long u32ACTiming = 0x1f1111;// Set to the fast speed

	if(g_IsInit_nfrc)
	{
		*PageSize = nfrc.pagesize;
		//NFRC_BMS_Seach_blk();
		return SUCCESS;
		
	}
	
	g_IsInit_nfrc = 1;
	
	NFRC_mutex_init();

	//GdDumpMemInfo(1);

	//psysinfo_t* psysinfo = initDriver(u8CSR,u16InterMask,u32ACTiming);
	psysinfo = initstorage();
	
	NFRC_Set_Block_Info(psysinfo);
	
	*PageSize = nfrc.pagesize;
	
//printf("nfrc.pagesize:%d\n",nfrc.pagesize);	
	size = nfrc.pagesize+NFRC_ALIGNED64;	
#if SUPPORT_MALLOC
	nfrc.Readbuf = (unsigned char*)memAlloc(size);
#else
	//nfrc.Readbuf = (unsigned char*)g_nfrc_Readbuf;
	nfrc.Readbuf = (unsigned char*)getNFRCBuff(size, 0);

#endif
	if(nfrc.Readbuf == NULL)
	{
#ifndef CYGPKG_REDBOOT
		printk("ERROR: malloc nfrc.Readbuf FAILURE!\n");	
#endif		
		return FAILURE;		
	}
	else
	{
		nfrc.Readbuf_ali =(unsigned char*)(((unsigned int)nfrc.Readbuf+NFRC_ALIGNED64-1)&(0xFFFFFFC0));
	
#if NFRC_CACHE_ON_	
		nfrc.Readbuf_ali_NC = (unsigned char*)(((unsigned int)(nfrc.Readbuf_ali)) | 0x10000000);
#else
		nfrc.Readbuf_ali_NC = (unsigned char*)((unsigned int)(nfrc.Readbuf_ali));
#endif 	
	}	

#if SUPPORT_MALLOC
	nfrc.Buf_Cache = (unsigned char*)memAlloc(size);
#else
	//nfrc.Buf_Cache = (unsigned char*)g_nfrc_Buf_Cache;
	nfrc.Buf_Cache = (unsigned char*)getNFRCBuff(size, 0);

#endif	
	if(nfrc.Buf_Cache == NULL)
	{
#ifndef CYGPKG_REDBOOT
		printk("ERROR: malloc nfrc.Readbuf FAILURE!\n");	
#endif		
		return FAILURE;		
	}
	else
	{
		nfrc.Buf_Cache_ali =(unsigned char*)(((unsigned int)nfrc.Buf_Cache+NFRC_ALIGNED64-1)&(0xFFFFFFC0));
	}

	
	//size = nfrc.pagesize >>5;
#if SUPPORT_MALLOC
	//size = NFRC_MAXBUFSIZE+NFRC_ALIGNED64;//nfrc.pagesize >>4;
	nfrc.ECCBuffer1 = (unsigned long*)memAlloc(size);
	nfrc.ECCBuffer2 = (unsigned long*)memAlloc(size);
#else
	//nfrc.ECCBuffer1 = (unsigned long*)g_nfrc_ECCBuffer1;
	//nfrc.ECCBuffer2 = (unsigned long*)g_nfrc_ECCBuffer2;

	nfrc.ECCBuffer1 = (unsigned long*)getNFRCBuff(size, 0);
	nfrc.ECCBuffer2 = (unsigned long*)getNFRCBuff(size, 0);
#endif	
	if(nfrc.ECCBuffer1 == NULL)
	{
#ifndef	CYGPKG_REDBOOT
		printk("ERROR: malloc nfrc.Readbuf1 FAILURE!\n");	
#endif		
		return FAILURE;		
	}
	else
	{
		nfrc.ECCBuffer1_ali =(unsigned long*)(((unsigned int)nfrc.ECCBuffer1+NFRC_ALIGNED64-1)&(0xFFFFFFC0));
	}
	
	if(nfrc.ECCBuffer2 == NULL)
	{
#ifndef CYGPKG_REDBOOT
		printk("ERROR: malloc nfrc.Readbuf2 FAILURE!\n");	
#endif		
		return FAILURE;		
	}
	else
	{
		nfrc.ECCBuffer2_ali =(unsigned long*)(((unsigned int)nfrc.ECCBuffer2+NFRC_ALIGNED64-1)&(0xFFFFFFC0));
	}	
	
	nfrc.ECCBuffer_ali = nfrc.ECCBuffer1_ali;
//#endif


	NFRC_Set_Block_Info_ex(psysinfo);
	
	size = g_nbi.size*sizeof(unsigned short);

#if SUPPORT_MALLOC					
		
		printk("g_nbi.size:%d size:%d\n", g_nbi.size, size);
		printk("g_nbi.pBM_Tab : malloc(%d)\n",size);
		g_nbi.pBM_Tab = (unsigned short*)memAlloc(size);
		if(g_nbi.pBM_Tab == NULL)
		{
			//GdDumpMemInfo(1);

			printk("ERROR: malloc g_nbi.pBM_Tab FAILURE!\n");	
			return FAILURE;		
		}
#else
		//g_nbi.pBM_Tab = (unsigned short*)g_nfrc_BM_Tab;
		g_nbi.pBM_Tab = (unsigned short*)getNFRCBuff(size, 0);

#endif	
//#if SUPPORT_DUAL_ROM_IMAGE
//	if(getSDevinfo(SMALLBLKFLG)==0)
	if(IsDualRomImage()==SUCCESS)	
	{
#if SUPPORT_MALLOC					
		//printk("malloc(%d)\n",size);
		printk("g_nbi1.pBM_Tab : malloc(%d)\n",size);
		g_nbi1.pBM_Tab = (unsigned short*)memAlloc(size);
		if(g_nbi1.pBM_Tab == NULL)
		{
			//GdDumpMemInfo(1);
	
			printk("ERROR: malloc g_nbi1.pBM_Tab FAILURE!\n");	
			return FAILURE;		
		}		
#else
		//g_nbi1.pBM_Tab = (unsigned short*)g_nfrc1_BM_Tab;
		g_nbi1.pBM_Tab = (unsigned short*)getNFRCBuff(size, 0);

#endif
	}	
	else
	{
		printk("no support IsDualRomImage!\n"); 

	}
//#endif

#ifndef CYGPKG_REDBOOT
#if SUPPORT_MALLOC					
		g_nbi_a.pBM_Tab = NULL;
		size = g_nbi.RomFs1_Max_BlkCount*sizeof(unsigned short);
		if(size)
		{
			printk("size:%d\n",size);
			g_nbi_a.pBM_Tab = (unsigned short*)memAlloc(size); 
			printk("g_nbi_a.pBM_Tab : malloc(%d)\n",size);
			if(g_nbi_a.pBM_Tab == NULL)
			{
				//GdDumpMemInfo(1);
	
				printk("ERROR: malloc g_nbi_a.pBM_Tab FAILURE!\n");	
				return FAILURE;		
			}		
		}
#else

#endif	
#endif//#ifdef CYGPKG_REDBOOT


#if SUPPORT_NFRCCACHE	
	if(NFRC_Cache_Init(&g_nfrc_cache, nfrc.pagesize)==FAILURE)
	{
		return FAILURE;
	}

	#ifndef CYGPKG_REDBOOT
		if(NFRC_Cache_Init(&g_nfrc_cache_fs1, nfrc.pagesize)==FAILURE)
		{
			return FAILURE;
		}
	#endif
#endif	

	return NFRC_BMS_Seach_blk();
}
#if SUPPORT_NFRCCACHE
void print_NFRC_Cache_Info(NFRC_Cache_t *pnfrc_cache)
{
	int i;
	printk("pnfrc_cache->count:%d\n", pnfrc_cache->count);
	printk("pnfrc_cache->pagesize:%d\n", pnfrc_cache->pagesize);
	printk("pnfrc_cache->pbuf:0x%x\n", (unsigned int)pnfrc_cache->pbuf);
	printk("pnfrc_cache->pbuf_ali64:0x%x\n", (unsigned int)pnfrc_cache->pbuf_ali64);
	
	for(i=0; i<pnfrc_cache->count; i++)
	{
		printk("pnfrc_cache->cachepage[%d].pageAddr:%d\n", i, pnfrc_cache->cachepage[i].pageAddr);
		printk("pnfrc_cache->cachepage[%d].flag:%d\n", i, pnfrc_cache->cachepage[i].flag);
		printk("pnfrc_cache->cachepage[%d].referenceCount:%d\n", i, pnfrc_cache->cachepage[i].referenceCount);
		printk("pnfrc_cache->cachepage[%d].ptrPage:0x%x\n", i, (unsigned int)pnfrc_cache->cachepage[i].ptrPage);	
	}
	printk("\n");
}
unsigned int NFRC_Cache_Init(NFRC_Cache_t *pnfrc_cache, unsigned int pagesize)
{
	int i;
	pnfrc_cache->count = MAX_CACHECOUNT;
	pnfrc_cache->pagesize = pagesize;

#if SUPPORT_MALLOC	
	pnfrc_cache->pbuf = (unsigned char*)memAlloc(pnfrc_cache->count*pnfrc_cache->pagesize + NFRC_ALIGNED64);	
#else
	pnfrc_cache->pbuf = (unsigned char*)getNFRCBuff(pnfrc_cache->count*pnfrc_cache->pagesize, 0);
#endif

	pnfrc_cache->pbuf_ali64 = (unsigned char*)(((unsigned int)pnfrc_cache->pbuf+NFRC_ALIGNED64-1)&(0xFFFFFFC0));


	if(pnfrc_cache->pbuf_ali64==NULL)
	{
		printk("ERROR: malloc pnfrc_cache->pbuf FAILURE!\n");	
		return FAILURE;
	}
	for(i=0; i<pnfrc_cache->count; i++)
	{
		pnfrc_cache->cachepage[i].pageAddr = NFRC_NON_MAPPING_ADDR;
		pnfrc_cache->cachepage[i].flag = 0;
		pnfrc_cache->cachepage[i].referenceCount = 0;
		pnfrc_cache->cachepage[i].ptrPage = (unsigned char*)((unsigned int)pnfrc_cache->pbuf_ali64 + i*pnfrc_cache->pagesize);		
	}
	
	//print_NFRC_Cache_Info(pnfrc_cache);
	return SUCCESS;
}
#endif

unsigned int NFRC_Close(void)
{
#if SUPPORT_MALLOC

	if(nfrc.Readbuf)
	{
		memFree(nfrc.Readbuf);
		nfrc.Readbuf = NULL;	
	}
	
	if(nfrc.Buf_Cache)
	{
		memFree(nfrc.Buf_Cache);
		nfrc.Buf_Cache = NULL;	
	}
	
	if(nfrc.ECCBuffer1)
	{
		memFree(nfrc.ECCBuffer1);
		nfrc.ECCBuffer1 = NULL;	
	}

	if(nfrc.ECCBuffer2)
	{
		memFree(nfrc.ECCBuffer2);
		nfrc.ECCBuffer2 = NULL;	
	}
	
	//g_nbi.pBM_Tab
	if(g_nbi.pBM_Tab)
	{
		memFree(g_nbi.pBM_Tab);
		g_nbi.pBM_Tab = NULL;	
	}

	if(g_nbi1.pBM_Tab)
	{
		memFree(g_nbi1.pBM_Tab);
		g_nbi1.pBM_Tab = NULL;	
	}		
	
#ifdef CYGPKG_REDBOOT	
	if(g_nbi_a.pBM_Tab)
	{
		memFree(g_nbi_a.pBM_Tab);
		g_nbi_a.pBM_Tab = NULL;	
	}
#endif	
	
#endif

	NFRC_mutex_destroy();

	g_IsInit_nfrc = 0;

	
	return SUCCESS;
}
unsigned int NFRC_log2phyAddr(unsigned int logPageAddr)
{
	unsigned int phyaddr;
	unsigned short blk = (logPageAddr>>nfrc.blockshift) - nfrc.rom.start + 1;
	phyaddr = g_nbi.pBM_Tab[blk] + nfrc.rom.start;
	
	//printk("g_nbi.pBM_Tab[%d]:%d phyaddr:%d logPageAddr:%d\n", blk, g_nbi.pBM_Tab[blk], phyaddr, logPageAddr);
	phyaddr <<= nfrc.blockshift;
	phyaddr += (logPageAddr&(nfrc.page_per_block-1));
	return phyaddr;
}
//#if SUPPORT_DUAL_ROM_IMAGE
unsigned int NFRC1_log2phyAddr(unsigned int logPageAddr)
{
//	if(getSDevinfo(SMALLBLKFLG)==0)
	if(IsDualRomImage()==SUCCESS)	
	{
		unsigned int phyaddr;
		unsigned short blk = (logPageAddr>>nfrc.blockshift) - nfrc.rom1.start + 1;
		phyaddr = g_nbi1.pBM_Tab[blk] + nfrc.rom1.start;
	
		//printk("nfrc.rom1.start:%d\n",nfrc.rom1.start);
		//printk("g_nbi1.pBM_Tab[%d]:%d phyaddr:%d logPageAddr:%d\n", blk, g_nbi1.pBM_Tab[blk], phyaddr, logPageAddr);
		phyaddr <<= nfrc.blockshift;
		phyaddr += logPageAddr&(nfrc.page_per_block-1);
		return phyaddr;
	}
	return 0;
}
//#endif

#ifndef CYGPKG_REDBOOT
unsigned int NFRCa_log2phyAddr(unsigned int logPageAddr)
{
	unsigned int phyaddr;
	unsigned short blk = (logPageAddr>>nfrc.blockshift) - nfrc.rom_a.start + 1;
	phyaddr = g_nbi_a.pBM_Tab[blk] + nfrc.rom_a.start;
	phyaddr <<= nfrc.blockshift;
	phyaddr += logPageAddr&(nfrc.page_per_block-1);
	return phyaddr;
}
#endif

unsigned int NFRC_GetPhyPageAddr_ex(unsigned int PageAddr, unsigned int flg)
{
	unsigned int u32PhyAddr;
	switch(flg)	
	{
		case FLG_ROMFS:
			u32PhyAddr = (unsigned long)(nfrc.rom.start) << nfrc.blockshift; 
			u32PhyAddr += PageAddr;
			
			break;
		case FLG_ROMFS1:
			u32PhyAddr = (unsigned long)(nfrc.rom1.start) << nfrc.blockshift; 
			u32PhyAddr += PageAddr;
			break;

		case FLG_ROMFS_a:
			u32PhyAddr = (unsigned long)(nfrc.rom_a.start) << nfrc.blockshift; 
			u32PhyAddr += PageAddr;
			break;
		
		default:
			u32PhyAddr = 0;
			break;			
		
	}
	return u32PhyAddr;
}
extern unsigned int debug_flag;
unsigned int NFRC_ReadPage_ex(unsigned int PageAddr,unsigned char* pReadPyldBuffer, unsigned int flg)
{
	//unsigned int result;
	unsigned int u32PhyAddr;
	
	u32PhyAddr = NFRC_GetPhyPageAddr_ex(PageAddr, flg);
	//if(flg==2)
	//	debug_flag = 1;
	//SPMP_DEBUG_PRINT("u32PhyAddr:%x PageAddr:%x flg:%u\n", u32PhyAddr, PageAddr, flg);
/* modify by mm.li 01-12,2011 clean warning */	
	/*
	return ReadPage_storage(u32PhyAddr, pReadPyldBuffer, nfrc.ECCBuffer_ali);
	*/
	return ReadPage_storage(u32PhyAddr, (unsigned long* )pReadPyldBuffer, nfrc.ECCBuffer_ali);
/* modify end */	
}


unsigned int NFRC_GetPhyPageAddr(unsigned int PageAddr, unsigned int flg)
{
	unsigned int u32logAddr, u32PhyAddr;
	switch(flg)	
	{
		case FLG_ROMFS:
			u32logAddr = (unsigned long)(nfrc.rom.start) << nfrc.blockshift; 
			u32logAddr += PageAddr;
			u32PhyAddr = NFRC_log2phyAddr(u32logAddr);
			break;
		case FLG_ROMFS1:
			u32logAddr = (unsigned long)(nfrc.rom1.start) << nfrc.blockshift; 
			u32logAddr += PageAddr;
			u32PhyAddr = NFRC1_log2phyAddr(u32logAddr);
			break;
#ifndef CYGPKG_REDBOOT
		case FLG_ROMFS_a:
			u32logAddr = (unsigned long)(nfrc.rom_a.start) << nfrc.blockshift; 
			u32logAddr += PageAddr;
			u32PhyAddr = NFRCa_log2phyAddr(u32logAddr);
			break;
#endif			
		default:
			u32PhyAddr = 0;
			break;			
		
	}
	return u32PhyAddr;
}
#if (ROMFS_SUPPORT_COMPRESS==1)
unsigned int NFRC_getSuppurtenCompress(unsigned int flg)
{
#if 0
	unsigned int SuppurtenCompress;
	switch(flg)	
	{
		case FLG_ROMFS:
			SuppurtenCompress = g_nbi.IsSuppurtenCompress;
			break;
		case FLG_ROMFS1:
			SuppurtenCompress = g_nbi1.IsSuppurtenCompress;
			break;
#ifndef CYGPKG_REDBOOT
		case FLG_ROMFS_a:
			SuppurtenCompress = g_nbi_a.IsSuppurtenCompress;
			break;
#endif			
		default:
			SuppurtenCompress = 0;
			break;			
		
	}
	return SuppurtenCompress;
#else
	NFRC_Block_Info_t* pnbi;
	pnbi = NFRC_Get_nbi(flg);
	if(pnbi)
	{
		return 	pnbi->IsSuppurtenCompress;
	}
	return 0;
#endif	
}
#else
unsigned int NFRC_getSuppurtenCompress(unsigned int flg)
{
	return 0;
}
#endif

#if (ROMFS_SUPPORT_ENCRYPTION ==1)
unsigned int NFRC_getencryption(unsigned int flg)
{
#if 0
	unsigned int encryption;

	switch(flg)	
	{
		case FLG_ROMFS:
			encryption = 0;
			break;
		case FLG_ROMFS1:
			encryption = 0;
			break;
#ifndef CYGPKG_REDBOOT
		case FLG_ROMFS_a:
			encryption = g_nbi_a.IsSuppurtencryption;
			break;
#endif			
		default:
			encryption = 0;
			break;			
		
	}
	return encryption;
#else
	NFRC_Block_Info_t* pnbi;
	pnbi = NFRC_Get_nbi(flg);
	if(pnbi)
	{
		return 	pnbi->IsSuppurtencryption;
	}
	return 0;
#endif	
}
#else
unsigned int NFRC_getencryption(unsigned int flg)
{
	return 0;
}
#endif
#if 0
unsigned long NFRC_GetIsSuppurtenCompress(char* fileName)
{
#ifndef CYGPKG_REDBOOT
	unsigned long rc = 0;//not support
	FileHeaderGPZip* ptr;
	unsigned char buf[64];
	FILE *fp=fp = fopen(fileName, "rb");

	if (fread(buf, 1, 32, fp))
	{
		ptr = (FileHeaderGPZip*)buf;

		if(ptr->GPZipFileID==GPZMAGIC)
		{
			rc = 1;
		}
	}
	fclose(fp);
	return rc;
#endif	
}
#endif

#ifndef CYGPKG_REDBOOT
extern void (*mod_get_data)(unsigned char* p_buf, unsigned int  size);
#endif

unsigned int NFRC_IsAllZero(unsigned char* pBuffer, unsigned int size)
{
	int i;
	unsigned char* ptr =  Cache2NonCacheAddr(pBuffer);
	for(i=0; i<size; i++)	
	{
		if(ptr[i])
		{
			return 0; 	
		}			
	}
	return 1;
	
}

unsigned int NFRC_ReadPage_Nomal(unsigned int PageAddr,unsigned char* pReadPyldBuffer, unsigned int flg)
{
	unsigned int u32PhyAddr;
	//unsigned int encryption;
	
	unsigned char* pReadbuf;

//printk("NFRC_ReadPage_Nomal(PageAddr:%d, pReadPyldBuffer:0x%x, flg:%d)\n", PageAddr, pReadPyldBuffer, flg);


	if(flg==FLG_ROMFS1)
	{
		printk("NFRC_ReadPage(PageAddr:%d, pReadPyldBuffer:0x%x, flg:%d)\n", PageAddr, (unsigned int)pReadPyldBuffer, flg);
	}

//printk("NFRC_ReadPage(PageAddr:%d, pReadPyldBuffer:0x%x, flg:%d)\n", PageAddr, pReadPyldBuffer, flg);
	
	u32PhyAddr = NFRC_GetPhyPageAddr(PageAddr, flg);
	//encryption = NFRC_getencryption(flg);
	
	//printk("encryption:%d = NFRC_getencryption(%d)\n", encryption, flg);
	
	NFRC_mutex_lock();
	
#ifdef	CYGPKG_REDBOOT
	pReadbuf = pReadPyldBuffer;
#else
	pReadbuf = nfrc.Readbuf_ali;
#endif

/* modify by mm.li 01-12,2011 clean warning */	
	/*
	if(ReadPage_storage(u32PhyAddr, pReadbuf, nfrc.ECCBuffer_ali)==SUCCESS)
	*/	
	if(ReadPage_storage(u32PhyAddr, (unsigned long* )pReadbuf, nfrc.ECCBuffer_ali)==SUCCESS)
/* modify end */	
	{
		if(flg==FLG_ROMFS1)
		{
			printk("ReadPage_storage(u32PhyAddr:%d, pReadbuf:0x%x, flg:%d)\n", u32PhyAddr, (unsigned int)pReadbuf, flg);
		}

#ifndef	CYGPKG_REDBOOT
		nfrc.page_addr = PageAddr;
		memcpy(pReadPyldBuffer, NFRC_Cache2NonCacheAddr(pReadbuf), nfrc.pagesize);
	#if 0
		if(encryption)
		{
			if (mod_get_data)
			{ 
	    		mod_get_data(pReadPyldBuffer, nfrc.pagesize);	
	    	}
		}
	    	
	#endif
#endif		
		NFRC_mutex_unlock();
		return SUCCESS;	
	}
		
	NFRC_mutex_unlock();
	printk("NFRC_ReadPage(%d, %d) is FAILURE\n", PageAddr, flg);
	
	return FAILURE;
}
unsigned int NFRC_ReadPage_NO_Compress(unsigned int PageAddr,unsigned char* pReadPyldBuffer, unsigned int flg)
{

	if(NFRC_ReadPage_Nomal(PageAddr, pReadPyldBuffer, flg)==FAILURE)
	{
		return FAILURE;	
	}

	return SUCCESS;
}
#if (ROMFS_SUPPORT_COMPRESS ==1)
unsigned int NFRC_ReadPage_Compress(unsigned int PageAddr,unsigned char* pReadPyldBuffer, unsigned int flg)
{
	int i, j;
	int Blkidx, Blkcnt;//, BlkOffset;//, size;
	int BlockSize, BlockDataOffset;
	
	int CompressDataOffset, CompressDatasize;
	
	NFRC_Block_Info_t* pnbi;
	pnbi = NFRC_Get_nbi(flg);

//printk("NFRC_ReadPage_Compress(%d, 0x%x, %d)\n", PageAddr, pReadPyldBuffer, flg);

	if(pnbi)
	{
		unsigned int CompressDataOffset;
		unsigned short	CompressDataSize;
		unsigned short	CompressDataFlags;
		unsigned char* ptr;
		
		BlockSize = pnbi->handrGPZ.BlockSize;
		BlockDataOffset = pnbi->handrGPZ.BlockDataOffset;
		
		Blkidx = (PageAddr*nfrc.pagesize)/BlockSize;
		Blkcnt = nfrc.pagesize/BlockSize;
	
//printk("Blkidx:%d\n", Blkidx);
//printk("Blkcnt:%d\n", Blkcnt);	
		
		for(i=0; i<Blkcnt; i++)
		{
			ptr = (unsigned char*)(((unsigned int)pReadPyldBuffer)+i*BlockSize);
			CompressDataOffset = pnbi->pGPZ_Tab[Blkidx+i].Offset;
			
			CompressDataOffset += BlockDataOffset;
			
			CompressDataSize = pnbi->pGPZ_Tab[Blkidx+i].Size;
			CompressDataFlags = pnbi->pGPZ_Tab[Blkidx+i].Flags;
			
//printk("CompressDataOffset:%d\n", CompressDataOffset);

//printk("CompressDataSize:%d\n", CompressDataSize);	
//printk("CompressDataFlags:0x%x\n", CompressDataFlags);	
			

			{
				int pageidx, pagecnt, pageOffset;//, Psize;
				unsigned char* ptr1;
				unsigned char* ptr2;
				int len, readlen;
	
				len = CompressDataSize;			
							
				if(CompressDataFlags==BLOCKFLG_PACKED)
				{
					ptr2 = g_nfrc_compress_buf.Compressbuf_ali;
				}
				else
				{
					ptr2 = ptr;
				}
				
				
				do
				{
					pageidx = CompressDataOffset/nfrc.pagesize;
					pageOffset = CompressDataOffset%nfrc.pagesize;
					ptr1 = (unsigned char*)(((unsigned int)g_nfrc_compress_buf.Readbuf_ali)+pageOffset);
	
						
	
					if((len+pageOffset)>=nfrc.pagesize)
					{
						readlen = nfrc.pagesize - pageOffset;
					}
					else
					{
						readlen = len;
					}
		
//printk("pageidx:%d\n", pageidx);
//printk("pageOffset:%d\n", pageOffset);
//printk("readlen:%d\n", readlen);
//printk("len:%d\n", len);	
//printk("ptr1:0x%x\n", ptr1);
//printk("ptr2:0x%x\n", ptr2);	
							
					if(g_nfrc_compress_buf.Cachepageidx!=pageidx)
					{
						if(NFRC_ReadPage_Nomal(pageidx, g_nfrc_compress_buf.Readbuf_ali, flg)==FAILURE)
						{
							return FAILURE;		
						}
						g_nfrc_compress_buf.Cachepageidx = pageidx;	
					}
					
					//printk("memcpy(0x%x, 0x%x, %d)\n", ptr2, NFRC_Cache2NonCacheAddr(ptr1), readlen);
					memcpy(ptr2, NFRC_Cache2NonCacheAddr(ptr1), readlen);
					
					CompressDataOffset += readlen;
					len -= readlen;
					ptr1 += readlen;
					ptr2 += readlen;
					//pageidx++;
						
				}while(len);
				
				if(CompressDataFlags==BLOCKFLG_PACKED)
				{
					int rc;
					int destLen = BlockSize;
					//memcpy(g_nfrc_compress_buf.Compressbuf_ali, NFRC_Cache2NonCacheAddr(ptr1), CompressDataSize);
					if((rc=uncompress (ptr, &destLen, g_nfrc_compress_buf.Compressbuf_ali, CompressDataSize))!=SUCCESS)			
					{
						printk("rc : %d\n", rc); 

						NFRC_print_buf(g_nfrc_compress_buf.Compressbuf_ali, CompressDataSize);
						
						NFRC_print_buf(ptr, destLen);
						return FAILURE;		
					}
				}
			}	

		}
		
	}

	return SUCCESS;
}
#else
unsigned int NFRC_ReadPage_Compress(unsigned int PageAddr,unsigned char* pReadPyldBuffer, unsigned int flg)
{
	return SUCCESS;
}
#endif

unsigned int NFRC_ReadPage(unsigned int PageAddr,unsigned char* pReadPyldBuffer, unsigned int flg)
{
	unsigned int rc;
	unsigned int encryption = NFRC_getencryption(flg);	
	
	if(NFRC_getSuppurtenCompress(flg))
	{
		rc = NFRC_ReadPage_Compress(PageAddr, pReadPyldBuffer,flg);
	}
	else
	{
		rc = NFRC_ReadPage_NO_Compress(PageAddr, pReadPyldBuffer,flg);
	}

#ifndef CYGPKG_REDBOOT	
	if(encryption)
	{
#if(ROMFS_SUPPORT_ENCRYPTION ==1)
		if (mod_get_data)
		{ 
    		mod_get_data(pReadPyldBuffer, nfrc.pagesize);	
    	}
#endif
	}	
#endif	
	return rc;
}

unsigned int NFRC_ReadPageFromStorage(unsigned int PageAddr,unsigned char* pReadPyldBuffer, unsigned int flg)
{
	if(flg==FLG_ROMFS_a)
	{
		return NFRC_ReadPage(PageAddr, pReadPyldBuffer, FLG_ROMFS_a);
	}
	if(NFRC_ReadPage(PageAddr, pReadPyldBuffer, FLG_ROMFS)==SUCCESS)
	{
		//printk("NFRC_ReadPage(%d, %d) is FAILURE\n", PageAddr, flg);

		return SUCCESS;
	}
	else
	{
		printk("XXXNFRC_ReadPage(%d, %d) is FAILURE\n", PageAddr, FLG_ROMFS);
	}
	return NFRC_ReadPage(PageAddr, pReadPyldBuffer, FLG_ROMFS1);
}
#if SUPPORT_NFRCCACHE
unsigned int NFRC_FindPageCached(NFRC_Cache_t *pnfrc_cache, unsigned int pageAddr)
{
	int	i = 0;

	for (i=0; i<pnfrc_cache->count; i++)
	{
		if (pnfrc_cache->cachepage[i].pageAddr == pageAddr)	// we got it
		{
			return i;		// this is what we want... 
		}
	}
	return FAILURE;		
}

unsigned int NFRC_FindPageCaching(NFRC_Cache_t *pnfrc_cache)
{
	int	i = 0;
	int reference = 0x10000;
	int targetPage = FAILURE;

	for (i=0; i<pnfrc_cache->count; i++)
	{
		if (pnfrc_cache->cachepage[i].pageAddr == NFRC_NON_MAPPING_ADDR)	// we got it
		{
			return i;		// this is what we want... 
		}

		if (pnfrc_cache->cachepage[i].referenceCount < reference)
		{
			reference = pnfrc_cache->cachepage[i].referenceCount;
			targetPage = i;
		}
	}
	return targetPage;		
}

NFRC_Cache_t* NFRC_get_nfrc_cache(unsigned int flg)
{
	switch(flg)	
	{
		case FLG_ROMFS:
			return &g_nfrc_cache;
		case FLG_ROMFS1:
			return &g_nfrc_cache;
		case FLG_ROMFS_a:
			return &g_nfrc_cache_fs1;			
	}
	return NULL;
}
#endif

unsigned int NFRC_ReadPageByCache(unsigned int PageAddr,unsigned char* pReadPyldBuffer, unsigned int flg)
{
#if SUPPORT_NFRCCACHE
	unsigned int idx;
	NFRC_Cache_t* pnfrc_cache = NFRC_get_nfrc_cache(flg);
	
//printk("NFRC_ReadPageByCache(%d, %d)\n", PageAddr, flg);

	if(pnfrc_cache==NULL)
	{
		printk("ERROR -> pnfrc_cache:0x%x\n", (unsigned int)pnfrc_cache);
		return FAILURE;
	}
	
	idx = NFRC_FindPageCached(pnfrc_cache, PageAddr);
	
	if (idx == FAILURE)
	{

		idx = NFRC_FindPageCaching(pnfrc_cache);
		if(NFRC_ReadPageFromStorage(PageAddr,pnfrc_cache->cachepage[idx].ptrPage, flg)==FAILURE)
		{
			return FAILURE;
		}
		pnfrc_cache->cachepage[idx].pageAddr = PageAddr;
		pnfrc_cache->cachepage[idx].referenceCount = MAX_REFERENCECOUNT;
		memcpy(pReadPyldBuffer, pnfrc_cache->cachepage[idx].ptrPage,  pnfrc_cache->pagesize);
		
	}
	else
	{
		memcpy(pReadPyldBuffer, pnfrc_cache->cachepage[idx].ptrPage, pnfrc_cache->pagesize);
		pnfrc_cache->cachepage[idx].referenceCount = (MAX_REFERENCECOUNT<<1);
	}

	for (idx=0; idx<pnfrc_cache->count; idx++)
	{
		if(pnfrc_cache->cachepage[idx].referenceCount)
		{
			pnfrc_cache->cachepage[idx].referenceCount--;
		}
	}
	return SUCCESS;
#else
	return NFRC_ReadPageFromStorage(PageAddr, pReadPyldBuffer, flg);
#endif	
}

#ifndef CYGPKG_REDBOOT
unsigned int NFRC_IsSupportRomfs1(void)
{
	if(g_nbi_a.RomFs1_Max_BlkCount)
		return SUCCESS;
	return FAILURE;	
}
#endif

NFRC_Block_Info_t* NFRC_Get_nbi(unsigned int flg)
{
	switch(flg)	
	{
		case FLG_ROMFS:
			return &g_nbi;
			break;
		case FLG_ROMFS1:
			return &g_nbi1;
			break;
#ifndef CYGPKG_REDBOOT
		case FLG_ROMFS_a:
			return &g_nbi_a;
			break;
#endif			
		default:
			break;				
	}	
	return NULL;
}

void NFRC_LoadGPZTab(unsigned int flg)
{
	NFRC_Block_Info_t* pnbi;
	int i, len, size, PageAddr;
	unsigned char* ptr;
	unsigned char* ptr1;
	
	FileHeaderGPZip* pfhgpz;
	pnbi = NFRC_Get_nbi(flg);
		
	for(i=1; i<MAX_SEACHBLK; i++)
	{
		PageAddr = i <<nfrc.blockshift;
		if(NFRC_ReadPage_ex(PageAddr, nfrc.Readbuf_ali, flg)==SUCCESS)
		{
			pfhgpz = (FileHeaderGPZip*)nfrc.Readbuf_ali_NC;
			if(pfhgpz->GPZipFileID == GPZMAGIC)  
			{
				
				//ptr = (unsigned char*)(pnbi->phandrGPZ);
				ptr1 = (unsigned char*)nfrc.Readbuf_ali_NC;
				memcpy(&pnbi->handrGPZ, ptr1, sizeof(FileHeaderGPZip));	

				len = pnbi->handrGPZ.BlockNum*sizeof(BlockTableGPZip); 
#if SUPPORT_MALLOC					
/* modify by mm.li 01-12,2011 clean warning */
				/*
				pnbi->pGPZ_Tab = (unsigned char*)memAlloc(len);
				*/
				pnbi->pGPZ_Tab = (BlockTableGPZip*)memAlloc(len);
/* modify end */
#else
/* modify by mm.li 01-12,2011 clean warning */
				/*
				pnbi->pGPZ_Tab = (unsigned char*)getNFRCBuff(len, 0);
				*/
				pnbi->pGPZ_Tab = (BlockTableGPZip*)getNFRCBuff(len, 0);
/* modify end */
#endif				
				ptr1 += sizeof(FileHeaderGPZip);
				ptr = (unsigned char*)pnbi->pGPZ_Tab;
				
				if(len >= (nfrc.pagesize-sizeof(FileHeaderGPZip)))
				{
					size = (nfrc.pagesize-sizeof(FileHeaderGPZip));
				}
				else
				{
					size = len;
				}
				
				memcpy(ptr, ptr1, size);	
				
				ptr += size;
				len -= size; 
				
				while(len)
				{
					if(len >= nfrc.pagesize)
					{
						size = nfrc.pagesize;
					}
					else
					{
						size = len;
					}
					
					PageAddr++;
					if(NFRC_ReadPage_ex(PageAddr, nfrc.Readbuf_ali, flg)==FAILURE)
					{
						//rc = FAILURE;
						break;
					}
					memcpy(ptr, nfrc.Readbuf_ali_NC,size);
					ptr += size;
					len -= size;
				};//while(len);
				
#if 0
				{
					int j;
					for(j=0; j<pnbi->handrGPZ.BlockNum; j++)
					{
						printk("%d-->Offset:%d Size:%d Flags:%d\n", j, pnbi->pGPZ_Tab[j].Offset, pnbi->pGPZ_Tab[j].Size, pnbi->pGPZ_Tab[j].Flags);
					}					
				}
#endif	
				return;				
			}	
		}
	}
	
}

unsigned int NFRC_compress_buf_init(void)
{
	g_nfrc_compress_buf.Cachepageidx = NFRC_NON_MAPPING_ADDR;
#if SUPPORT_MALLOC	
	#if 1
	g_nfrc_compress_buf.Readbuf = (unsigned char*)memAlloc((nfrc.pagesize<<1)+ NFRC_ALIGNED64);
	g_nfrc_compress_buf.Compressbuf = (unsigned char*)memAlloc(nfrc.pagesize+ NFRC_ALIGNED64);
	#else
	DTCM_Enable((unsigned int)0x2e006000);
	g_nfrc_compress_buf.Readbuf = (unsigned char*)memAlloc((nfrc.pagesize<<1)+ NFRC_ALIGNED64);
	g_nfrc_compress_buf.Compressbuf = (unsigned char*)(0x2e006000);	
	#endif
#else
	g_nfrc_compress_buf.Readbuf = (unsigned char*)getNFRCBuff((nfrc.pagesize<<1), 0);
	g_nfrc_compress_buf.Compressbuf = (unsigned char*)getNFRCBuff((nfrc.pagesize), 0);

#endif	
	if((g_nfrc_compress_buf.Readbuf == NULL) ||(g_nfrc_compress_buf.Compressbuf == NULL))
	{
#ifndef CYGPKG_REDBOOT
		printk("ERROR: malloc g_nfrc_compress_buf.Readbuf:0x%x FAILURE!\n", (unsigned int)g_nfrc_compress_buf.Readbuf);
		printk("ERROR: malloc g_nfrc_compress_buf.Compressbuf:0x%x  FAILURE!\n", (unsigned int)g_nfrc_compress_buf.Compressbuf);	
#endif		
		return FAILURE;		
	}
	else
	{
		g_nfrc_compress_buf.Readbuf_ali =(unsigned char*)(((unsigned int)g_nfrc_compress_buf.Readbuf+NFRC_ALIGNED64-1)&(0xFFFFFFC0));
		//g_nfrc_compress_buf.Readbuf_ali_NC = NFRC_Cache2NonCacheAddr(g_nfrc_compress_buf.Readbuf_ali);
		g_nfrc_compress_buf.Compressbuf_ali =(unsigned char*)(((unsigned int)g_nfrc_compress_buf.Compressbuf+NFRC_ALIGNED64-1)&(0xFFFFFFC0));
		//g_nfrc_compress_buf.Compressbuf_ali_NC = NFRC_Cache2NonCacheAddr(g_nfrc_compress_buf.Compressbuf_ali);


	}
	return SUCCESS;
}
unsigned int NFRC_compress_buf_free(void)
{
#if SUPPORT_MALLOC	
	if(g_nfrc_compress_buf.Readbuf)
	{
		memFree(g_nfrc_compress_buf.Readbuf);
		g_nfrc_compress_buf.Readbuf = NULL;
		g_nfrc_compress_buf.Readbuf_ali = NULL;
		//g_nfrc_compress_buf.Readbuf_ali_NC = NULL;
		
	}
	#if 1
	if(g_nfrc_compress_buf.Compressbuf)
	{
		memFree(g_nfrc_compress_buf.Compressbuf);
		g_nfrc_compress_buf.Compressbuf = NULL;
		g_nfrc_compress_buf.Compressbuf_ali = NULL;
		//g_nfrc_compress_buf.Compressbuf_ali_NC = NULL;
	}
		
	#endif
	return 0;
#endif	
}
unsigned int NFRC_Loadnbi(unsigned int flg)
{
	NFRC_Block_Info_t* p_nbi;
	NFRC_Block_Info_t* p_nbi_tmp;

	int i, len, size, PageAddr;
	unsigned char* ptr;	
	unsigned int rc = FAILURE;
		
	p_nbi = NFRC_Get_nbi(flg);
	
	//for(i=0; i<nfrc.rom.count; i++)
	for(i=0; i<MAX_SEACHBLK; i++)
	{	
		PageAddr = i <<nfrc.blockshift;
		if(NFRC_ReadPage_ex(PageAddr, nfrc.Readbuf_ali, flg)==SUCCESS)
		{
			p_nbi_tmp = (NFRC_Block_Info_t*)nfrc.Readbuf_ali_NC;
			//p_nbi = (NFRC_Block_Info_t*)nfrc.Readbuf_ali;
			if(p_nbi_tmp->heard == NFRC_HEARD_PATTERN)  
			{

				memcpy(p_nbi, p_nbi_tmp, sizeof(NFRC_Block_Data_Info_t));
				
				if(p_nbi->version <= NFRC_VERSION3)
				{
					p_nbi->IsSuppurtenCompress = 0;
				}

printk("flg:%d\n", flg);
printk("p_nbi->heard : 0x%x\n",p_nbi->heard);
printk("p_nbi->version : %d\n",p_nbi->version);
printk("p_nbi->size : %d\n",p_nbi->size);
printk("p_nbi->RomFs_Max_BlkCount : %d\n",p_nbi->RomFs_Max_BlkCount);			
printk("p_nbi->RomFs1_Max_BlkCount : %d\n",p_nbi->RomFs1_Max_BlkCount);	
printk("p_nbi->flg : %d\n",p_nbi->flg);	
printk("p_nbi->IsSuppurtencryption : %d\n",p_nbi->IsSuppurtencryption);
printk("p_nbi->IsSuppurtenCompress : %d\n",p_nbi->IsSuppurtenCompress);			
				
				len = p_nbi->size*sizeof(unsigned short); 
				
				ptr = (unsigned char*)p_nbi->pBM_Tab;
				do
				{
					if(len >= nfrc.pagesize)
					{
						size = nfrc.pagesize;
					}
					else
					{
						size = len;
					}
					
					PageAddr++;
					if(NFRC_ReadPage_ex(PageAddr, nfrc.Readbuf_ali, flg)==FAILURE)
					{
						rc = FAILURE;
						break;
					}
					memcpy(ptr, nfrc.Readbuf_ali_NC,size);
					ptr += size;
					len -= size;
				}while(len);
				if(p_nbi->IsSuppurtenCompress)
				{
					NFRC_LoadGPZTab(flg);
					if(FLG_ROMFS==flg)
					{
						NFRC_compress_buf_init();
					}
				}
				//break;	
				return SUCCESS;
			}
			
		}
	}
	return FAILURE;
}

unsigned int NFRC_BMS_Seach_blk(void)
{
	NFRC_Block_Info_t* p_nbi;
#if 1
	if(NFRC_Loadnbi(FLG_ROMFS)==FAILURE)
	{
		return FAILURE;
	}
	
	if(IsDualRomImage()==SUCCESS)
	{
		if(NFRC_Loadnbi(FLG_ROMFS1)==FAILURE)
		{
			return FAILURE;
		}
	}
	
#ifndef CYGPKG_REDBOOT
	p_nbi = NFRC_Get_nbi(FLG_ROMFS);
	if(p_nbi->RomFs1_Max_BlkCount)
	{
		if(NFRC_Loadnbi(FLG_ROMFS_a)==FAILURE)
		{
			return FAILURE;
		}
	}
#endif		
	return SUCCESS;
#else	
	int i, PageAddr;
	NFRC_Block_Info_t* p_nbi;
	
	unsigned int rc = FAILURE;
	int len, size;
	unsigned char* ptr;
	for(i=0; i<nfrc.rom.count; i++)
	{	
		PageAddr = i <<nfrc.blockshift;
		if(NFRC_ReadPage_ex(PageAddr, nfrc.Readbuf_ali, FLG_ROMFS)==SUCCESS)
		{
			p_nbi = (NFRC_Block_Info_t*)nfrc.Readbuf_ali_NC;
			//p_nbi = (NFRC_Block_Info_t*)nfrc.Readbuf_ali;
			if(p_nbi->heard == NFRC_HEARD_PATTERN)  
			{
				memcpy(&g_nbi, p_nbi, sizeof(NFRC_Block_Data_Info_t));
				
				len = g_nbi.size*sizeof(unsigned short); 
				
				ptr = (unsigned char*)g_nbi.pBM_Tab;
				do
				{
					if(len >= nfrc.pagesize)
					{
						size = nfrc.pagesize;
					}
					else
					{
						size = len;
					}
					
					PageAddr++;
					if(NFRC_ReadPage_ex(PageAddr, nfrc.Readbuf_ali, FLG_ROMFS)==FAILURE)
					{
						rc = FAILURE;
						break;
					}
					memcpy(ptr, nfrc.Readbuf_ali_NC,size);
					ptr += size;
					len -= size;
				}while(len);



				
				LoadGPZTab(FLG_ROMFS);
				break;	
			}
			
		}
	}
//#if SUPPORT_DUAL_ROM_IMAGE
//if(getSDevinfo(SMALLBLKFLG)==0)
	if(IsDualRomImage()==SUCCESS)	
	{
		for(i=0; i<nfrc.rom.count; i++)
		{	
			//PageAddr = (i+nfrc.rom.count) <<nfrc.blockshift;
			PageAddr = (i) <<nfrc.blockshift;
			if(NFRC_ReadPage_ex(PageAddr, nfrc.Readbuf_ali, FLG_ROMFS1)==SUCCESS)
			{
				p_nbi = (NFRC_Block_Info_t*)nfrc.Readbuf_ali_NC;
				//p_nbi = (NFRC_Block_Info_t*)nfrc.Readbuf_ali;
				if(p_nbi->heard == NFRC_HEARD_PATTERN)  
				{
	#if 0
				printk("2p_nbi->heard : 0x%x\n",p_nbi->heard);
				printk("2p_nbi->version : %d\n",p_nbi->version);
				printk("2p_nbi->size : %d\n",p_nbi->size);
				printk("2p_nbi->RomFs_Max_BlkCount : %d\n",p_nbi->RomFs_Max_BlkCount);			
				printk("2p_nbi->RomFs1_Max_BlkCount : %d\n",p_nbi->RomFs1_Max_BlkCount);	
				printk("2p_nbi->flg : %d\n",p_nbi->flg);	
	#endif
	
					
					memcpy(&g_nbi1, p_nbi, sizeof(NFRC_Block_Data_Info_t));
					
					len = g_nbi1.size*sizeof(unsigned short); 
					//g_nbi1.pBM_Tab = (unsigned short*)memAlloc(len); 
					ptr = (unsigned char*)g_nbi1.pBM_Tab;
					do
					{
						if(len >= nfrc.pagesize)
						{
							size = nfrc.pagesize;
						}
						else
						{
							size = len;
						}				
						PageAddr++;
						if(NFRC_ReadPage_ex(PageAddr, nfrc.Readbuf_ali, FLG_ROMFS1)==FAILURE)
						{
							rc = FAILURE;
							break;
						}
						memcpy(ptr, nfrc.Readbuf_ali_NC,size);
						ptr += size;
						len -= size;
					}while(len);
					break;					
				}
			}
		}
	}	
//#endif	

	//if((g_nbi.version < NFRC_SUPPORTROM_A_VERSION) || (g_nbi.RomFs1_Max_BlkCount==0))	
		//return rc;
#ifndef	CYGPKG_REDBOOT
	//for(i=0; i<nfrc.rom.count; i++)
	for(i=0; i<2; i++)
	{	
		//PageAddr = (i+nfrc.rom.count+nfrc.rom1.count) <<nfrc.blockshift;
		PageAddr = (i) << nfrc.blockshift;
		
		memset(nfrc.Readbuf_ali_NC, 0x5a, nfrc.phy_pagesize);
		memset(&g_nbi_a, 0, sizeof(NFRC_Block_Data_Info_t));
		///g_nbi_a.RomFs1_Max_BlkCount
		if(NFRC_ReadPage_ex(PageAddr, nfrc.Readbuf_ali, FLG_ROMFS_a)==SUCCESS)
		{
			//printk("NFRC_ReadPage_ex(%d, %d)\n", i, PageAddr);
			p_nbi = (NFRC_Block_Info_t*)nfrc.Readbuf_ali_NC;

			if(p_nbi->heard == NFRC_HEARD_PATTERN)  
			{
#if 0
				printk("3p_nbi->heard : 0x%x\n",p_nbi->heard);
				printk("3p_nbi->version : %d\n",p_nbi->version);
				printk("3p_nbi->size : %d\n",p_nbi->size);
				printk("3p_nbi->RomFs_Max_BlkCount : %d\n",p_nbi->RomFs_Max_BlkCount);			
				printk("3p_nbi->RomFs1_Max_BlkCount : %d\n",p_nbi->RomFs1_Max_BlkCount);
				printk("3p_nbi->IsSuppurtencryption : %d\n",p_nbi->IsSuppurtencryption);
#endif
	
				memcpy(&g_nbi_a, p_nbi, sizeof(NFRC_Block_Data_Info_t));
					
				len = g_nbi_a.RomFs1_Max_BlkCount * sizeof(unsigned short); 
				
				//g_nbi_a.pBM_Tab = (unsigned short*)memAlloc(len); 
				ptr = (unsigned char*)g_nbi_a.pBM_Tab;
					
				printk("len : %d\n",len);
				printk("g_nbi_a.pBM_Tab : 0x%x\n",g_nbi_a.pBM_Tab);
					
				do
				{
					if(len >= nfrc.pagesize)
					{
						size = nfrc.pagesize;
					}
					else
					{
						size = len;
					}				
					PageAddr++;
					if(NFRC_ReadPage_ex(PageAddr, nfrc.Readbuf_ali, FLG_ROMFS_a)==FAILURE)
					{
						rc = FAILURE;
						break;
					}
					memcpy(ptr, nfrc.Readbuf_ali_NC,size);
					ptr += size;
					len -= size;
				}while(len);

				break;					
			}
		}
		
		//printf("g_nbi_a.RomFs_Max_BlkCount:%d\n", g_nbi_a.RomFs_Max_BlkCount);
		//printf("g_nbi_a.RomFs1_Max_BlkCount:%d\n", g_nbi_a.RomFs1_Max_BlkCount);
	}
#endif//#ifndef	CYGPKG_REDBOOT

	return rc;
#endif	
}


////////////////////////////////////////////////////////////////////////////
psysinfo_t* NFRC_ReadId(void)
{
	//return initDriver(0x09, 0xf05d, 0x1f1111);	
	return initstorage();
}


//unsigned int NFRC_ErasePhyBlock(unsigned short blk)
unsigned int NFRC_ErasePhyBlock_ex(unsigned int u32PhyAddr)//page address
{
	unsigned int blk = u32PhyAddr>>nfrc.blockshift;
	EraseBlock_ex(blk);
	return SUCCESS;
}
unsigned int NFRC_ErasePhyBlock(unsigned int u32PhyAddr)//page address
{

	unsigned int blk = u32PhyAddr>>nfrc.blockshift;
	//printk("NFRC_ErasePhyBlock(%d)\n",blk);
	EraseBlock(blk);
	return SUCCESS;
}

unsigned int NFRC_ReadPhyPage_ex(unsigned int u32PhyAddr,unsigned char* pReadPyldBuffer)
{
	printk("NFRC_ReadPhyPage_ex(%d)\n",u32PhyAddr);
/* modify by mm.li 01-12,2011 clean warning */	
	/*
	return ReadPage_storage(u32PhyAddr, pReadPyldBuffer, nfrc.ECCBuffer_ali);
	*/	
	return ReadPage_storage(u32PhyAddr, (unsigned long* )pReadPyldBuffer, nfrc.ECCBuffer_ali);
/* modify end */	
}
unsigned int NFRC_ReadPhyPage(unsigned int u32PhyAddr,unsigned char* pReadPyldBuffer)
{
	//To do..........
	printk("To do(NFRC_ReadPhyPage)...\n");
	return FAILURE;
}

unsigned int NFRC_WritePhyPage_ex(unsigned int u32PhyAddr,unsigned char* pWritePyldBuffer)
{
	//To do..........
	printk("To do(NFRC_WritePhyPage_ex)...\n");
 	return FAILURE;
}

unsigned int NFRC_WritePhyPage(unsigned int u32PhyAddr,unsigned char* pWritePyldBuffer)
{
	//To do..........	
	printk("To do(NFRC_WritePhyPage)...\n");
 	return FAILURE;
}

#if 0
//return 0 is fail to read
unsigned int ReadUSBConfig(void)
{
	unsigned int pageNo;
	USB_Store_Info_t* pInfo;
	unsigned int PageSize;
	
	NFRC_Init(&PageSize);

	if((GetRomcodeVer() & 0xff0000ff) == 0x5A0000A5)
	{
		printk("g_sysInfo_s.bootInfo_s.UsbNo:0x%x\n",GetRomcodeUsbSerialNo());
		return GetRomcodeUsbSerialNo();
	}
	
	pageNo = nfrc.page_per_block+1;

	if(NFRC_ReadPhyPage_ex(pageNo, nfrc.Readbuf_ali)==FAILURE)	
	{
		printk("222NFRC_ReadPhyPage_ex(%d)==FAILURE\n",pageNo);
		return 0;
	}

	pInfo = (USB_Store_Info_t*)nfrc.Readbuf_ali_NC;
	
	printk("pInfo->heards:%x\n",pInfo->heards);
	printk("pInfo->hearde:%x\n",pInfo->hearde);
	printk("pInfo->pattern:%x\n",pInfo->pattern);
	
	if((pInfo->heards == USBCONFIG_PATTERN_S) && (pInfo->hearde == USBCONFIG_PATTERN_E))
		return pInfo->pattern;	

	return 0;
}
#endif

