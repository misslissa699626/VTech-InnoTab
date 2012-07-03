#ifndef _NFTL_H_
#define _NFTL_H_

#ifdef	_WIN32
#include "../include/nand.h"
#else
#include "nand.h"
//#include <cyg/hal/nf_s330.h>//#define WRITE_AND_GO 0
#endif

#define __NFTL_BUFFER_ON__ 1

typedef struct Nftl_Info
{
	unsigned short block_count;
	unsigned short page_per_block;
	unsigned short pagesize;
	unsigned short sector_per_page;
	
	unsigned char page_per_blk_shift;
	unsigned char sector_per_page_shift;
	unsigned short npb_logpage_count;
	
	unsigned int WriteCachePageaddr;//CachePageaddr;
	unsigned int ReadCachePageaddr;

	unsigned int write_count;
	unsigned int write_count_bak;

	unsigned char* pCacheWriteBuf;
	unsigned char* pCacheWriteBuf_ali;

	unsigned char* pReadBuf;
	unsigned char* pReadBuf_ali;
	
	unsigned char* pCacheReadBuf;
	unsigned char* pCacheReadBuf_ali;

	unsigned char* pTmpBuf;
	unsigned char* pTmpBuf_ali;


	//unsigned char* pWriteBuf;
	//unsigned char* pWriteBuf1;
	//unsigned char* pWriteBuf1_ali;	
	//unsigned char* pWriteBuf2;
	//unsigned char* pWriteBuf2_ali;		

	//unsigned char* pEccBuf;
	//unsigned char* pEccBuf1;
	//unsigned char* pEccBuf1_ali;	
	//unsigned char* pEccBuf2;
	//unsigned char* pEccBuf2_ali;	

}Nftl_Info_t;

#if 0
typedef struct Nftl_Buf
{
	unsigned char* pWriteBuf;
	unsigned char* pEccBuf;
}Nftl_Buf_t;
#endif

#ifdef __cplusplus 
extern "C"
{
#endif
unsigned int NFTL_Init(Chip_Info_t* pChipInfo);
void NFTL_Close(void);
unsigned int NFTL_Read(unsigned int StartSector, unsigned int SectorNum, unsigned char  *pdata);
unsigned int NFTL_Write(unsigned int StartSector, unsigned int SectorNum, unsigned char  *pdata);
//Nftl_Buf_t* NFTL_GetAndSwitchBuf(void);

unsigned int NFTL_memcmp(unsigned char* ptr1, unsigned char* ptr2, int len);
unsigned int NFTL_CacheFlush(void);
unsigned int NFTL_CacheFlush_ex(void);
void  NFTL_FDisk(void);
#ifdef __cplusplus 
};
#endif
#endif
