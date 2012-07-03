#ifndef __NF_ROM_CACHE_H__
#define __NF_ROM_CACHE_H__

#include "../hal/nf_s330.h"
#include "gpz.h"


#define	NFRC_CACHE_SECTOR_SIZE 0x200		// 512 bytes for the fat file system
#define	NFRC_SECTOR_SHIFT 0x9

#define NFFS_ROM_BLOCK_SYS_USES_BLOCK_COUNT 1

#define FLG_ROMFS 1
#define FLG_ROMFS1 2
#define FLG_ROMFS_a 3

#define MAX_SEACHBLK 4

#define ROMFS_SUPPORT_COMPRESS 0
#define ROMFS_SUPPORT_ENCRYPTION 0


typedef struct NFRC_BM_Info
{
	unsigned short start;
	unsigned short count;
}NFRC_BM_Info;

typedef struct NFRC_Nand_Info
{
	unsigned short block_count;
	unsigned short page_per_block;
	unsigned short pagesize;
	unsigned short sectors_per_page;
	unsigned short page_addr;
	unsigned short blockshift;
	unsigned short phy_pagesize;
//	unsigned short phy_block_count;		
	
}NFRC_Nand_Info_t;

typedef struct NFRC_Info
{
	NFRC_BM_Info sys;	
	NFRC_BM_Info rom;
	NFRC_BM_Info rom1;
	NFRC_BM_Info rom_a;
	//NFRC_BM_Info bm;
	NFRC_BM_Info user;
	
	//NFRC_BM_Info rom;
	unsigned short block_count;
	unsigned short page_per_block;
	unsigned short pagesize;
	unsigned short sectors_per_page;
	unsigned short page_addr;
	unsigned short blockshift;

	unsigned short phy_pagesize;
//	unsigned short phy_block_count;	

	unsigned char  u8Support_TwoPlan;//u8NFChannel;
	unsigned char  u8Support_Internal_Interleave;
	unsigned char  u8Support_External_Interleave;
	unsigned char  u8Internal_Chip_Number;	
	
	unsigned char* Readbuf;
	unsigned char* Readbuf_ali;
	unsigned char* Readbuf_ali_NC;
	unsigned char* Buf_Cache;
	unsigned char* Buf_Cache_ali;
	//unsigned char* Buf_Cache_ali_NC;	
	unsigned long* ECCBuffer_ali;
	//unsigned long* ECCBuffer_ali;

	unsigned long* ECCBuffer1;
	unsigned long* ECCBuffer1_ali;
	unsigned long* ECCBuffer2;
	unsigned long* ECCBuffer2_ali;		
	
	
}NFRC_Info_t;

#define NFRC_HEARD_PATTERN 0x524F4653
#define NFRC_VERSION 0x1
#define NFRC_SUPPORTROM_A_VERSION 0x3

#define NFRC_VERSION3 0x3
#define NFRC_VERSION4 0x4

//#define MAX_BM_TAB_SIZE 4096

typedef struct NFRC_Block_Info
{
	unsigned int heard;
	unsigned int version;
	unsigned int size;
	unsigned int RomFs_size;
	unsigned int RomFs_Max_BlkCount;
	unsigned int RomFs1_Max_BlkCount;
	unsigned int flg;	
	unsigned int IsSuppurtencryption;
	unsigned int IsSuppurtenCompress;
	
	unsigned short* pBM_Tab;
	
	FileHeaderGPZip  handrGPZ;
	BlockTableGPZip* pGPZ_Tab;
	
}NFRC_Block_Info_t;

typedef struct NFRC_Block_Data_Info
{
	unsigned int heard;
	unsigned int version;
	unsigned int size;
	unsigned int RomFs_size;
	unsigned int RomFs_Max_BlkCount;
	unsigned int RomFs1_Max_BlkCount;
	unsigned int flg;
	unsigned int IsSuppurtencryption;
	unsigned int IsSuppurtenCompress;			
}NFRC_Block_Data_Info_t;


///////////////////////////////////////////////////////////////////////////////////
typedef struct NFRC_Compress_Buf_Info
{
	unsigned int Cachepageidx;
	unsigned char* Readbuf;
	unsigned char* Readbuf_ali;
	//unsigned char* Readbuf_ali_NC;

	unsigned char* Compressbuf;
	unsigned char* Compressbuf_ali;
	//unsigned char* Compressbuf_ali_NC;	
	
}NFRC_Compress_Buf_Info_t;

///////////////////////////////////////////////////////////////////////////////////

#ifndef CYGPKG_REDBOOT
#define SUPPORT_NFRCCACHE 1
#else
#define SUPPORT_NFRCCACHE 0
#endif

#define MAX_CACHECOUNT 32//16
#define MAX_REFERENCECOUNT 20
#define NFRC_NON_MAPPING_ADDR 0xFFFFFFFF
typedef struct NFRC_CachePage
{
	unsigned int	pageAddr;
	unsigned int	flag;	
	unsigned int	referenceCount;	// we don't swap out the page which is reference most frequently.
	unsigned char*	ptrPage;
}NFRC_CachePage_t;

typedef struct NFRC_Cache
{
	unsigned char* pbuf;
	unsigned char* pbuf_ali64;
	unsigned int count;
	unsigned int pagesize;
	NFRC_CachePage_t cachepage[MAX_CACHECOUNT];
	
}NFRC_Cache_t;

unsigned int NFRC_Cache_Init(NFRC_Cache_t *pnfrc_cache, unsigned int pagesize);
///////////////////////////////////////////////////////////////////////////////////
#define NFRC_START_ADDR		0x1000000
#define NFRC_END_ADDR		0x1F00000

unsigned int getNFRCBuff(unsigned int aSize, unsigned int aMsk);
///////////////////////////////////////////////////////////////////////////////////
void NFRC_print_buf(unsigned char* buf, unsigned int len);
//unsigned int NFRC_EraseAll_Block(void);

//unsigned int NFRC_Init(void);
unsigned int NFRC_Init(unsigned int* PageSize);
unsigned int NFRC_close(void);

//unsigned int NFRC_ReadSectorByCache(unsigned int SectorAddr,unsigned char* pReadPyldBuffer);
unsigned int NFRC_ReadPageByCache(unsigned int PageAddr,unsigned char* pReadPyldBuffer, unsigned int flg);
#ifndef CYGPKG_REDBOOT
unsigned int NFRC_IsSupportRomfs1(void);
//unsigned int NFRC_ReadPageByCache_fs1(unsigned int PageAddr,unsigned char* pReadPyldBuffer);
#endif
//unsigned short NFRC_Get_sectors_per_page(void);
unsigned short NFRC_Get_Page_Aling_Mask(void);

unsigned int NFRC_BMS_Seach_blk(void);

unsigned char* NFRC_Cache2NonCacheAddr(unsigned char* addr);

unsigned int NFRC_Get_MaxRomSize(void);
unsigned int NFRC_Get_RomFs_Max_BlkCount(void);
unsigned int NFRC_Get_RomFs1_Max_BlkCount(void);


/////////////////////////////////////////////////
unsigned int NFRC_Init_ex(NFRC_Nand_Info_t* Info);
psysinfo_t* NFRC_ReadId(void);
//unsigned int NFRC_ErasePhyBlock(unsigned short blk);
unsigned int NFRC_ErasePhyBlock(unsigned int u32PhyAddr);
unsigned int NFRC_ReadPhyPage(unsigned int u32PhyAddr,unsigned char* pReadPyldBuffer);
unsigned int NFRC_WritePhyPage(unsigned int u32PhyAddr,unsigned char* pWritePyldBuffer);

//unsigned int NFRC_ReadPage_ex(unsigned int PageAddr,unsigned char* pReadPyldBuffer);
unsigned int NFRC_ReadPage_ex(unsigned int PageAddr,unsigned char* pReadPyldBuffer, unsigned int flg);


unsigned int NFRC_ErasePhyBlock_ex(unsigned int u32PhyAddr);
unsigned int NFRC_ReadPhyPage_ex(unsigned int u32PhyAddr,unsigned char* pReadPyldBuffer);
unsigned int NFRC_WritePhyPage_ex(unsigned int u32PhyAddr,unsigned char* pWritePyldBuffer);

unsigned int ReadUSBConfig(void);
/////////////////////////////////////////////////

NFRC_Block_Info_t* NFRC_Get_nbi(unsigned int flg);

/////////////////////////////////////////////////
unsigned long NFRC_GetIsSuppurtenCompress(char* fileName);


#endif //#ifndef __NF_ROM_CACHE_H__

