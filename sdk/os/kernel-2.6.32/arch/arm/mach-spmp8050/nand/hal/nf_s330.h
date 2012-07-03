#ifndef __NF_S330_H__
#define __NF_S330_H__

#include "hal_base.h"

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef FAILURE
#define FAILURE -1
#endif


#ifndef TRUE
#define TRUE 1
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef ENABLE
#define ENABLE 1
#endif

#ifndef DISENABLE
#define DISENABLE 0
#endif

#define IDTYPE0 0
#define IDTYPE1 1
#define IDTYPE2 2
#define IDTYPE3 3



#define	_CACHE_ON_ 0			// it is always cache on in eCos 

//#define SECTOR_SIZE 12//4096
#if _CACHE_ON_
#define NON_CACHE_REGION				0x10000000
#else
#define NON_CACHE_REGION				0x00000000
#endif
#define GET_NON_CACHED_ADDR(__ptr__) 	(((unsigned long) __ptr__) | NON_CACHE_REGION)
#define GET_CACHED_ADDR(__ptr__) 		(((unsigned long) __ptr__) & (~NON_CACHE_REGION))
#define GET_NON_CACHED_ADDR_PTR(x) ((void *)GET_NON_CACHED_ADDR(x))


#if 0
#define		ID_2_PAGE_SIZE(__xx__)			(0x400 << __xx__)
#define		ID_2_PAYLOAD_SIZE(__xx__)		(0x400 << __xx__)
#define		ID_2_PAGE_SHIFT(__xx__)			(10 + __xx__)
#define		ID_2_BLOCK_SIZE(__xx__)			(0x10000 << __xx__)
#define		ID_2_BLOCK_SHIFT(__xx__)		(6 + __xx__)
#define		ID_2_TOTAL_BLOCK(__xx__)		(0x2000 << __xx__)

#else

#define		ID_0_PAGE_SIZE(__xx__)			(0x400 << __xx__)
#define		ID_0_PAGE_SHIFT(__xx__)			(10 + __xx__)
#define		ID_0_BLOCK_SIZE(__xx__)			(0x10000 << __xx__)
#define		ID_0_BLOCK_SHIFT(__xx__)		(6 + __xx__)

#define		ID_1_PAGE_SIZE(__xx__)			(0x800 << __xx__)
#define		ID_1_PAGE_SHIFT(__xx__)			(11 + __xx__)
#define		ID_1_BLOCK_SIZE(__xx__)			(0x20000 << __xx__)
#define		ID_1_BLOCK_SHIFT(__xx__)		(7 + __xx__)

#endif



#define NF_ERASE		  	1
#define NF_READ         	2
#define NF_WRITE        	3
#define NF_READID       	4
#define NF_READSTATUS   	5
#define NF_WRITE_HIDDEN		6

////////////////////////////////////////////////////
#define BCH_ENCODE				0
#define BCH_DECODE				1

/*BCH register setting*/
#define BCH_CFG							(BCH_BASE_ADDRESS + 0x00)
#define BCH_DATA_START_ADDR				(BCH_BASE_ADDRESS + 0x04)
#define BCH_PARITY_START_ADDR	    	(BCH_BASE_ADDRESS + 0x08)
#define BCH_INT					    	(BCH_BASE_ADDRESS + 0x0C)
#define BCH_SOFT_RESET			    	(BCH_BASE_ADDRESS + 0x10)
#define BCH_INT_MASK			    	(BCH_BASE_ADDRESS + 0x14)
#define BCH_STATUS				    	(BCH_BASE_ADDRESS + 0x18)
#define BCH_ERR_BLK_REPORT		    	(BCH_BASE_ADDRESS + 0x1C)

#define rBCH_CFG						(*(volatile unsigned *)BCH_CFG)
#define rBCH_DATA_START_ADDR			(*(volatile unsigned *)BCH_DATA_START_ADDR)
#define rBCH_PARITY_START_ADDR			(*(volatile unsigned *)BCH_PARITY_START_ADDR)
#define rBCH_INT						(*(volatile unsigned *)BCH_INT)
#define rBCH_SOFT_RESET					(*(volatile unsigned *)BCH_SOFT_RESET)
#define rBCH_INT_MASK					(*(volatile unsigned *)BCH_INT_MASK)
#define rBCH_STATUS						(*(volatile unsigned *)BCH_STATUS)
#define rBCH_ERR_BLK_REPORT				(*(volatile unsigned *)BCH_ERR_BLK_REPORT)


/* NAND register */
//Test Channe register only for test
#define FM_CSR         		(NAND_S330_BASE + 0x00)																	
#define FM_DESC_BA     		(NAND_S330_BASE + 0x04)																	
#define FM_AC_TIMING   		(NAND_S330_BASE + 0x08)																	
#define FM_RDYBSY_EN   		(NAND_S330_BASE + 0x0c)																	
#define FM_PIO_CTRL1   		(NAND_S330_BASE + 0x10)																	
#define FM_PIO_CTRL2   		(NAND_S330_BASE + 0x14)																	
#define FM_PIO_CTRL3   		(NAND_S330_BASE + 0x18)																	
#define FM_PIO_CTRL4   		(NAND_S330_BASE + 0x1c)																	
#define FM_PIO_CTRL5   		(NAND_S330_BASE + 0x20)																	
#define FM_PIO_CTRL6   		(NAND_S330_BASE + 0x24)																	
#define FM_PIO_CTRL7   		(NAND_S330_BASE + 0x28)																	
#define FM_PIO_CTRL8   		(NAND_S330_BASE + 0x2c)																	
#define FM_INTRMSK     		(NAND_S330_BASE + 0x40)																	
#define FM_INTR_STS    		(NAND_S330_BASE + 0x44)																	
#define FM_RDYBSY_DLY_INT	(NAND_S330_BASE + 0x48)																	

#define rFM_CSR         	(*(volatile unsigned *)FM_CSR)
#define rFM_DESC_BA     	(*(volatile unsigned *)FM_DESC_BA)
#define rFM_AC_TIMING   	(*(volatile unsigned *)FM_AC_TIMING)
#define rFM_RDYBSY_EN   	(*(volatile unsigned *)FM_RDYBSY_EN)
#define rFM_PIO_CTRL1   	(*(volatile unsigned *)FM_PIO_CTRL1)
#define rFM_PIO_CTRL2   	(*(volatile unsigned *)FM_PIO_CTRL2)
#define rFM_PIO_CTRL3   	(*(volatile unsigned *)FM_PIO_CTRL3)
#define rFM_PIO_CTRL4   	(*(volatile unsigned *)FM_PIO_CTRL4)
#define rFM_PIO_CTRL5   	(*(volatile unsigned *)FM_PIO_CTRL5)
#define rFM_PIO_CTRL6   	(*(volatile unsigned *)FM_PIO_CTRL6)
#define rFM_PIO_CTRL7   	(*(volatile unsigned *)FM_PIO_CTRL7)
#define rFM_PIO_CTRL8   	(*(volatile unsigned *)FM_PIO_CTRL8)
#define rFM_INTRMSK     	(*(volatile unsigned *)FM_INTRMSK)
#define rFM_INTR_STS    	(*(volatile unsigned *)FM_INTR_STS)
#define rFM_RDYBSY_DLY_INT 	(*(volatile unsigned *)FM_RDYBSY_DLY_INT)
//////////////////////////////////////////////////////////////
//Channel register
#define FM1_CSR          (NAND_S330_BASE1 + 0x00)
#define FM1_DESC_BA      (NAND_S330_BASE1 + 0x04)
#define FM1_AC_TIMING    (NAND_S330_BASE1 + 0x08)
#define FM1_PIO_CTRL1    (NAND_S330_BASE1 + 0x10)
#define FM1_PIO_CTRL2    (NAND_S330_BASE1 + 0x14)
#define FM1_PIO_CTRL3    (NAND_S330_BASE1 + 0x18)
#define FM1_PIO_CTRL4    (NAND_S330_BASE1 + 0x1c)
#define FM1_PIO_CTRL5    (NAND_S330_BASE1 + 0x20)
#define FM1_PIO_CTRL6    (NAND_S330_BASE1 + 0x24)
#define FM1_PIO_CTRL7    (NAND_S330_BASE1 + 0x28)
#define FM1_PIO_CTRL8    (NAND_S330_BASE1 + 0x2c)
#define FM1_INTRMSK      (NAND_S330_BASE1 + 0x40)
#define FM1_INTR_STS     (NAND_S330_BASE1 + 0x44)

#define rFM1_CSR         (*(volatile unsigned *)FM1_CSR)
#define rFM1_DESC_BA     (*(volatile unsigned *)FM1_DESC_BA)
#define rFM1_AC_TIMING   (*(volatile unsigned *)FM1_AC_TIMING)
#define rFM1_PIO_CTRL1   (*(volatile unsigned *)FM1_PIO_CTRL1)
#define rFM1_PIO_CTRL2   (*(volatile unsigned *)FM1_PIO_CTRL2)
#define rFM1_PIO_CTRL3   (*(volatile unsigned *)FM1_PIO_CTRL3)
#define rFM1_PIO_CTRL4   (*(volatile unsigned *)FM1_PIO_CTRL4)
#define rFM1_PIO_CTRL5   (*(volatile unsigned *)FM1_PIO_CTRL5)
#define rFM1_PIO_CTRL6   (*(volatile unsigned *)FM1_PIO_CTRL6)
#define rFM1_PIO_CTRL7   (*(volatile unsigned *)FM1_PIO_CTRL7)
#define rFM1_PIO_CTRL8   (*(volatile unsigned *)FM1_PIO_CTRL8)
#define rFM1_INTRMSK     (*(volatile unsigned *)FM1_INTRMSK)
#define rFM1_INTR_STS    (*(volatile unsigned *)FM1_INTR_STS)

//Channel register
#define FM2_CSR          (NAND_S330_BASE2 + 0x00)
#define FM2_DESC_BA      (NAND_S330_BASE2 + 0x04)
#define FM2_AC_TIMING    (NAND_S330_BASE2 + 0x08)
#define FM2_PIO_CTRL1    (NAND_S330_BASE2 + 0x10)
#define FM2_PIO_CTRL2    (NAND_S330_BASE2 + 0x14)
#define FM2_PIO_CTRL3    (NAND_S330_BASE2 + 0x18)
#define FM2_PIO_CTRL4    (NAND_S330_BASE2 + 0x1c)
#define FM2_PIO_CTRL5    (NAND_S330_BASE2 + 0x20)
#define FM2_PIO_CTRL6    (NAND_S330_BASE2 + 0x24)
#define FM2_PIO_CTRL7    (NAND_S330_BASE2 + 0x28)
#define FM2_PIO_CTRL8    (NAND_S330_BASE2 + 0x2c)
#define FM2_INTRMSK      (NAND_S330_BASE2 + 0x40)
#define FM2_INTR_STS     (NAND_S330_BASE2 + 0x44)

#define rFM2_CSR         (*(volatile unsigned *)FM2_CSR)
#define rFM2_DESC_BA     (*(volatile unsigned *)FM2_DESC_BA)
#define rFM2_AC_TIMING   (*(volatile unsigned *)FM2_AC_TIMING)
#define rFM2_PIO_CTRL1   (*(volatile unsigned *)FM2_PIO_CTRL1)
#define rFM2_PIO_CTRL2   (*(volatile unsigned *)FM2_PIO_CTRL2)
#define rFM2_PIO_CTRL3   (*(volatile unsigned *)FM2_PIO_CTRL3)
#define rFM2_PIO_CTRL4   (*(volatile unsigned *)FM2_PIO_CTRL4)
#define rFM2_PIO_CTRL5   (*(volatile unsigned *)FM2_PIO_CTRL5)
#define rFM2_PIO_CTRL6   (*(volatile unsigned *)FM2_PIO_CTRL6)
#define rFM2_PIO_CTRL7   (*(volatile unsigned *)FM2_PIO_CTRL7)
#define rFM2_PIO_CTRL8   (*(volatile unsigned *)FM2_PIO_CTRL8)
#define rFM2_INTRMSK     (*(volatile unsigned *)FM2_INTRMSK)
#define rFM2_INTR_STS    (*(volatile unsigned *)FM2_INTR_STS)




///  defined by chenhn 	2007/10/17
#define SCU_A_PERI_CLKEN		(SCU_A_BASE + 0x04)
#define rSCU_A_PERI_CLKEN		(*(volatile unsigned *)SCU_A_PERI_CLKEN)

#define	SCU_A_CLKEN_NAND0		0x800
#define	SCU_A_CLKEN_NAND1		0x1000
#define	SCU_A_CLKEN_BCH			0x2000
#define SCU_A_CLKEN_AAHB_M212 (1<<16)     //add by alexchang 10/31/2007
#define SCU_A_CLKEN_NAND_ABT   (1<<20)    //add by alexchang 10/31/2007

//// defined by chenhn 		2007/09/22
#define	ND_INTR_DESC_DONE					0x01
#define	ND_INTR_DESC_END					0x02
#define	ND_INTR_DESC_ERROR				0x04
#define	ND_INTR_DESC_INVALID			0x08
#define ND_INTR_DESC_RB1 0x1000
#define ND_INTR_DESC_RB2 0x2000
#define ND_INTR_DESC_RB (ND_INTR_DESC_RB1|ND_INTR_DESC_RB2)
#define ND_INTR_DESC_NFC_BUSY (1<<7)
//////////////////////////////////////////////////////////////////////////////////////////////
#define NAND_UNKNOWN			0
#define NAND_SAMSUNG      		1
#define NAND_HYNIX				2
#define NAND_ST					3
#define NAND_TOSHIBA			4
#define NAND_MICRON			5
#define NAND_INTEL			6

#define MAUNCMD_RESET	0xFF00
#define MAUNCMD_READID	0x9000
#define MAUNCMD_BLOCK_ERASE	0x6000
#define MAUNCMD_CONF_BLOCK_ERASE	0xD000
#define MAUNCMD_WRITE	0x8000
#define MAUNCMD_CONF_WRITE	0x1000
#define MAUNCMD_WRITE_TWO_PLAN	0x8100
#define MAUNCMD_CONF_WRITE_TWO_PLAN	0x1100

#define MAUNCMD_READ				0x0000
#define MAUNCMD_CONF_READ			0x3000

#define AUTOCMD_BLOCK_ERASE					0x60D0
//#define AUTOCMD_BLOCK_ERASE_TWOPLAN_BEGIN	0x6060
//#define AUTOCMD_BLOCK_ERASE_TWOPLAN_END		0xD000

#define AUTOCMD_READ 						0x0030
#define AUTOCMD_WRITE 						0x8010
#define AUTOCMD_WRITE_TWOPLAN_BEGIN 		0x8011
#define AUTOCMD_WRITE_TWOPLAN_END 			0x8110

#define NF_BEGIN 0
#define NF_END 1


//MCD_TYPE
#define CMD_TYPE_READ 0x1
#define CMD_TYPE_WRITE 0x2
#define CMD_TYPE_ERASE 0x3
#define CMD_TYPE_READ_STATUS 0x4
#define CMD_TYPE_HALFAUTO_ERASE 0x5 //HalfAutoErase
#define CMD_TYPE_HALFAUTO_WRITE 0x6//HalfAutoWrite
#define CMD_TYPE_MANUAL_MODE_CMD 0x7
#define CMD_TYPE_MANUAL_MODE_ADDR 0x8
#define CMD_TYPE_MANUAL_MODE_PYLOAD_WRITE 0x9
#define CMD_TYPE_MANUAL_MODE_PYLOAD_READ 0xa
#define CMD_TYPE_MANUAL_MODE_REDUNT_WRITE 0xb
#define CMD_TYPE_MANUAL_MODE_REDUNT_READ 0xc
#define CMD_TYPE_MANUAL_MODE_SPARE_WRITE 0xd
#define CMD_TYPE_MANUAL_MODE_SPARE_READ 0xe
#define CMD_TYPE_MANUAL_MODE_STS_RD 0xf


#define MANUAL_MODE_OWNER 0x80
#define MANUAL_MODE_END_DESC 0x40
#define MANUAL_MODE_LAST_SECTOR 0x20
#define MANUAL_MODE_REDUNT_ENABLE 0x10
#define MANUAL_MODE_ADDRNUMBER_MASK 0x7


#define DESC_OFFSET 8

#define DESC_CMD 0
#define DESC_LENGTH 1
#define DESC_INTERRUPT 2
#define DESC_ADDR1 3
#define DESC_ADDR2 4
#define DESC_PAYLOAD 5
#define DESC_REDUNT 6
#define DESC_REDUNT_INFO 7

//////////////////////////////////////////////////////////////////////////////////////////////
// 1: use write and go, 0: not use write and go
#define		NAND_NEED_DOUBLE_BUFFER				0	

#define MAX_ID_LEN 32//5
typedef struct SmallBlkInfo
{
	unsigned short PhyPageLen;
	unsigned short PhyPageNoPerBlk;

	unsigned short PhyTotalBlkNo;
	unsigned short LogPageLen;

	unsigned short LogPageNoPerBlk;
	unsigned short LogTotalBlkNo;
	
	unsigned short PhyBlkPerLogBlk;	
	unsigned short PhyPagePerLogBlk;

	unsigned char PhyPagePerLogPage;
	unsigned char PhyPageLenShift;
	unsigned char PhyPageNoPerBlkShift;
	unsigned char PhyPagePerLogPageShift;

	unsigned char PhyBlkPerLogBlkShift;
	unsigned char PhyPagePerLogBlkShift;
	unsigned short PhyReduntLenLog;	

	unsigned int reserved[2];

}SmallBlkInfo_t;

typedef struct SysInfo
{
	unsigned short u16PageNoPerBlk;
	unsigned short u16PageSize;
	
	unsigned short u16PyldLen;
	unsigned short u16ReduntLen;
	//unsigned char u8ReduntLen;
	//unsigned char u8SpareLen;	
	
	unsigned short u16Redunt_Sector_Addr_Offset;
	unsigned short u16Redunt_Sector_Len;
	
	unsigned short u16TotalBlkNo;
	unsigned char u8TotalBlkNoShift;
	unsigned char ecc_mode;
	
	unsigned char  u8MultiChannel;//u8NFChannel;
	unsigned char  u8Support_Internal_Interleave;
	unsigned char  u8Support_External_Interleave;
	unsigned char  u8Internal_Chip_Number;
	
	unsigned short u16InterruptMask;
	unsigned char u8PagePerBlkShift;
	unsigned char u8Support_TwoPlan;
	
	unsigned short u16ReduntLenLog;
	//unsigned short reserved;
	unsigned char vendor_no;
	unsigned char reserved;
	
	unsigned int Id_len;
	
	unsigned char IdBuf[MAX_ID_LEN];
} psysinfo_t,*ppsysinfo_t;


typedef struct DescArg
{
	unsigned short u16InterruptMask;	
	unsigned short reserved;
	//unsigned char reserved;
}DescArg_t;

typedef struct DescInfo
{
	//unsigned char CMD1;
	//unsigned char CMD0;
	unsigned short CMD;
	unsigned char Read_Status_Data;
	unsigned char Multi_Func:4;
	unsigned char cmd_type:4;

	unsigned char Spare_length; 
	unsigned char Redunt_length;
	unsigned short Payload_length:13;
	unsigned short NA:3;

	unsigned short Interrupt_Status;	
	unsigned short Interrupt_Mask;	

	
	unsigned char ADDR2;
	unsigned char ADDR1;
	unsigned char ADDR0;
	unsigned char ADDR_number:4;//only use 3 bits
	unsigned char Redunt_Enable:1;
	unsigned char Last_Sector:1;
	unsigned char End_Desc:1;
	unsigned char Owner:1;
	
	unsigned short Next_Desc_BP;
	unsigned char ADDR4;
	unsigned char ADDR3;
	
	unsigned int Payload_BP;
	unsigned int Redunt_BP;
	unsigned int Spare_BP;
		
}DescInfo_t;

#define NF_DEVICE_NAME "nand"

#if 0
typedef struct BlockX_Info
{
	unsigned short start;
	unsigned short count;
}BlockX_Info_t;

typedef struct NFS_Block_info
{
	BlockX_Info_t sys_block;	
	BlockX_Info_t rom_block;
	BlockX_Info_t rom1_block;
	BlockX_Info_t rom_a_block;	
	BlockX_Info_t npb_block;
	BlockX_Info_t disk_block;
	
	//unsigned short reserved_blk;
	//unsigned short use_blk;
	unsigned short page_per_block;
	unsigned short page_size;
	unsigned short blockshift;// shift block addr to page addr
	unsigned short pageshift;// shift page tp byte	
	unsigned char u8Internal_Chip_Number;
	unsigned char u8MultiChannel;
	unsigned short reserved;
}NFSXX_Block_info_t;
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
unsigned short get_DiskstartBlk(void);
//NFFS_Block_info_t* getmfsxx_block_info(void);

void ChipEnable(unsigned char cs);
ppsysinfo_t initDriver_ex(unsigned int begin_cs_idx, unsigned int cs_count);

ppsysinfo_t initDriver(unsigned char u8CSR ,unsigned short u16InterMask,unsigned long u32ACTiming);
int ReadWritePage(unsigned long u32PhyAddr,unsigned long* PyldBuffer,unsigned long* DataBuffer,unsigned char u8RWMode);
void EraseBlock(unsigned long u32BlkNo);
int ReadWriteMutiPage(unsigned long u32PhyAddr,unsigned long* PyldBuffer,unsigned long* DataBuffer,unsigned char pagecount ,unsigned char u8RWMode);

//only for support single CS
int ReadWritePage_ex(unsigned long u32PhyAddr,unsigned long* PyldBuffer,unsigned long* DataBuffer,unsigned char u8RWMode);
void EraseBlock_ex(unsigned long u32BlkNo);

void EraseBlock_SB(unsigned long u32BlkNo);
int ReadWritePage_SB(unsigned long u32PhyAddr,unsigned long* PyldBuffer,unsigned long* DataBuffer,unsigned char u8RWMode);


int enterPollingState(void);

unsigned char getEccMode(void);
void setEccMode_ex(void);


unsigned int ReadStatus(void);




/////////////////////////////////////////////////
enum bch_ret_value {
	ret_BCH_OK   = 1,
	ret_BCH_FAIL = -1
};
/////////////////////////////////////////////////

int BCHProcess(unsigned char* PyldBuffer, unsigned char* ReduntBuffer, unsigned int len, int op, int eccmode);
int BCHProcess_ex(unsigned long* PyldBuffer, unsigned long* ReduntBuffer, unsigned int len, int op);
//int BCHProcess_Sertor(unsigned long* PyldBuffer,unsigned long* ReduntBuffer,int op);
unsigned int nfvalshift(unsigned int x);

void setgpio_dbg(int idx);
unsigned int valshift(unsigned int x);
/////////////////////////////////////////////////
void pio2Erase(unsigned long u32BlkNo);
void pio2ReadWritePage(unsigned long u32PhyAddr, unsigned long* PyldBuffer, unsigned long* DataBuffer, unsigned char u8RWMode);

void print_Desc(void);//for test
int Remove_NFDriver(void);
unsigned char* Cache2NonCacheAddr(unsigned char* addr);
unsigned int valshift(unsigned int x);
#define nfvalshift valshift
#endif	//__NF_S330_H__

