#ifndef _NAND_BADBLK_H_
#define _NAND_BADBLK_H_
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "nand.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////

#define BADBLK_INFO_PATTERN 0x42414442 //BADB
typedef struct __BadBlk_Info_Data
{
	unsigned int pattern;
	unsigned short size;
	unsigned short reserved;
}BadBlk_Info_Data_t;
typedef struct __BadBlk_Info__
{
	unsigned int pattern;
	unsigned short size;
	unsigned short reserved;
	unsigned char* ptr; 
}BadBlk_Info_t;

/////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned int badblkInfo_init(unsigned short size);
void badblkInfo_free(unsigned short size);
unsigned int badblkInfo_set(unsigned short blk);
unsigned int badblkInfo_isbad(unsigned short blk);
unsigned int badblkInfo_save(Nand_Info_t *pnandInfo, unsigned short blk);
unsigned int badblkInfo_load(Nand_Info_t *pnandInfo, unsigned short blk);

#endif