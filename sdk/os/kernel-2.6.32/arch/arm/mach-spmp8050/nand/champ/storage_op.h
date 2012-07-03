#ifndef __STORAGE_OP_H__
#define __STORAGE_OP_H__

#include "../hal/nf_s330.h"

typedef ppsysinfo_t (*predInitDriver_t)(unsigned char ,unsigned short ,unsigned long);
typedef void (*predReInitDriver_t)(void);
typedef void (*predEraseBlk_t)(unsigned short);
typedef int (*predReadWritePage_t)(unsigned long ,unsigned long* ,unsigned long* ,unsigned char );

enum SDev_flg {
	SMALLBLKFLG = 0,
	SUPPORTBCHFLG
};
enum Device_table {
	DEVICE_NAND =0,
	DEVICE_NAND_PBA, //no bch
	DEVICE_NAND_SBLK, //Small block
	DEVICE_NAND_OTP, //OTP Rom (Nand interface)
	DEVICE_SD0, 
	DEVICE_SD1, 
	DEVICE_SPI, //serial flash
	DEVICE_MAX
};

typedef struct SDev_s {
	unsigned char IsSmallblk;
	unsigned char IsSupportBCH;
	unsigned char DeviceID;
	unsigned char reserved;	
			
	predInitDriver_t predInitDriver;
	//predReInitDriver_t predReInitDriver;
	predEraseBlk_t predEraseBlk ;	
	predReadWritePage_t predReadWritePage ;

}SDev_t;


psysinfo_t* initstorage(void);
psysinfo_t* initstorage_ex(void);
void Erase_Block_storage(unsigned short blk);
unsigned long ReadPage_storage(unsigned int pageNo, unsigned long* ptrPyldData, unsigned long* DataBuffer);
unsigned long WritePage_storage(unsigned int pageNo, unsigned long* ptrPyldData, unsigned long* DataBuffer);
unsigned long WritePage_Hiddern(unsigned int pageNo, unsigned long* ptrPyldData, unsigned long* DataBuffer);

unsigned int IsDualRomImage(void);
unsigned int getNFRCBuff(unsigned int aSize, unsigned int aMsk);

unsigned int IsSmallblk_storage(void);
unsigned char getSDevinfo(unsigned char flg);
#endif
