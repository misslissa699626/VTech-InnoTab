#ifndef _NAND_H_
#define _NAND_H_
////////////////////////////////////////////////////////////////////////////////////////
#ifdef	_WIN32
#include "../include/fifo.h"
#else
#include "fifo.h"
#endif

////////////////////////////////////////////////////////////////////////////////////////
typedef struct __Chip_Info__
{
	unsigned short block_count;
	unsigned short page_per_block;
	unsigned short pagesize;
	unsigned short sector_per_page;
} Chip_Info_t;

//////////////////////////////////////////////////////////////////////////
#define NAND_SECTORINFO_PATTERN 0x5a
typedef struct Nand_Sector_Info
{
//	unsigned char reserved;
	unsigned char pattern;
 	unsigned char ver;  //0's byte for version
	unsigned short logblk;
	unsigned char reserved1[28];
}Nand_Sector_Info_t;

typedef struct Nand_Sector24bit_Info
{
//	unsigned char reserved;
	unsigned char pattern;
 	unsigned char ver;  //0's byte for version
	unsigned char reserved[62];
	unsigned short logblk;	
	unsigned char reserved1[62];	
}Nand_Sector_24bit_Info_t;


#if 0
typedef struct Nand_Blk_Info
{
	Nand_Sector_Info_t sector1;
	Nand_Sector_Info_t sector2;
}Nand_Blk_Info_t;
#endif
//////////////////////////////////////////////////////////////////////////
typedef struct Nand_LogInfo
{
	unsigned short logblk;
	unsigned char ver;
	unsigned char pattern;	
}Nand_LogInfo_t;


typedef struct Nand_Log2Phy
{
	unsigned short phyblk;
	unsigned char ver;
	unsigned char reserved;	
}Nand_Log2Phy_t;

typedef struct Nand_fragment_tab
{
	unsigned short logblk;
	unsigned short phyblk_old;
	unsigned short phyblk_new;
	unsigned short page_idx;
	unsigned char ver;
	unsigned char reserved[3];
}Nand_fragment_tab_t;

typedef struct Nand_tab
{
	unsigned short log2phy_count;
	unsigned short fragment_tab_count;
	
	FIFO_Manage_t fifo;
	Nand_Log2Phy_t* plog2phy;
	Nand_fragment_tab_t* pfragment_tab;	
	
}Nand_tab_t;


/// a zone is 1024 block normally,
/// including 114 reserved blocks & 10 fragment blocks
/// 900 blocks are mapping  to sector base
typedef struct Nand_Info
{	
	unsigned short logblock_count;
	unsigned short page_per_block;
	unsigned short pagesize;
	unsigned short sector_per_page;
	
	unsigned short reduntlen;
	unsigned short phyblock_count;
	
	unsigned short total_blockcount;
	unsigned short diskstartblk;
	
	unsigned char  page_per_blk_shift;
	unsigned char zone_count; //1-64
	unsigned short reserved;
		
	unsigned char* pReadBuf;
	unsigned char* pReadBuf_ali;
	unsigned char* pWriteBuf;
	unsigned char* pWriteBuf_ali;
	unsigned char* pEccBuf;
	unsigned char* pEccBuf_ali;	

	Nand_tab_t nandtab[MAX_ZONE_COUNT];

}Nand_Info_t;



//////////////////////////////////////////////////////////////////////////
void Nandcache_sync(void);
unsigned char* NandCache2NonCacheAddr(unsigned char* addr);
unsigned short Nand_getfragment_element_idx(unsigned char zone);
unsigned int nand_eraseblk(Nand_Info_t *pnandInfo, unsigned short blk);
Nand_LogInfo_t* Nand_Read_PhyPageWithNli(unsigned short blk, unsigned short page, unsigned char *pdata, unsigned char* peccbuf);
unsigned int Nand_Read_PhyPage(unsigned short blk, unsigned short page, unsigned char *pdata, unsigned char* peccbuf);
unsigned int Nand_Write_PhyPage(unsigned short blk, unsigned short page, unsigned char *pdata, unsigned char* peccbuf);
//////////////////////////////////////////////////////////////////////////
unsigned int Nand_Init(Chip_Info_t* pChipInfo, unsigned int *pIsNeedFdisk);
void Nand_free(void);
unsigned int Nand_ReadPage(unsigned int logpage, unsigned char *pdata);
unsigned int Nand_WritePage(unsigned int logpage, unsigned char *pdata);

#endif
