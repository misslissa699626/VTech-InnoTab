#ifndef _NF_CFGS_H_
#define _NF_CFGS_H_

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef FAILURE
#define FAILURE -1
#endif

#ifndef RECOVER
#define RECOVER -2
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef NO_BLOCK
#define NO_BLOCK (unsigned short)-1
#endif

#ifndef NO_PAGE
#define NO_PAGE (unsigned short)-1
#endif

#ifndef NFFS_DEBUG
#define NFFS_DEBUG 0
#endif

#define	SPMP_DEBUG_PRINT(fmt, args...) 	 printk("Debug %d[%s]: " fmt, __LINE__, __FUNCTION__, ##args)

#define SUPPORT12BITECC 1
#define SUPPORTCOPYMACHINE 0

#define SUPPORT_NPB 0
#define NPB_SUPPORT_DMA0 1
#define NPB_SUPPORT_TABLESAVE 1
#define ROMFS_SUPPORT_2PLAN_INTERLEAVE 0

#define NF_SUPPORT_4CS 0

#define RETRY_COUNT 10

#define MAX_BM_TAB_SIZE 4096
#define ROMFS_MAX_SIZE (32*1024*1024) //32MB
#define NFFS_BM_BLOCK_SIZE 64
#define NFFS_ROM_BLOCK_SIZE 128
#define NFFS_SYS_BLOCK_SIZE 64

#define NFFS_SYS_SPI_BLOCK_SIZE 6



#define SIZE_1K 1024
#define NF_ALIGNED64 64
////////////////////////////////////////////////////////////
#define NFUS_COUNT 16
#define NFUS_START (NFFS_SYS_BLOCK_SIZE - NFUS_COUNT)
#define NFUS_START_PAGE (unsigned short)-1
#define NFUS_PERPAGE 2 //must 1, 2 , 4, 8........

#define NFUS_START_INFO_BLK 0
#define NFUS_START_INFO_COUNT 2

#define NFUS_START_BLK (NFUS_START_INFO_BLK+NFUS_START_INFO_COUNT) 
////////////////////////////////////////////////////////////

#define ROMFS_TOTAL_SIZE (8*1024)

#define NAND_PGBASE_SAVEBLK 2
#define NAND_PGBASE_USED_PHYBLK 32
#define NAND_PGBASE_USED_LOGBLK 16

#define SUPPORT_DUAL_ROM_IMAGE 1

///////////// move the block data from NFFS.h
typedef struct Block_Info
{
	unsigned short start;
	unsigned short count;
}Block_Info_t;

typedef struct NFFS_Block_info
{
	Block_Info_t sys_block;	
	Block_Info_t rom_block;
	Block_Info_t rom1_block;
	Block_Info_t rom_a_block;	
	//Block_Info_t bm_block;
	Block_Info_t npb_block;
	Block_Info_t user_block;
	unsigned short reserved_blk;
	unsigned short use_blk;
	unsigned short page_per_block;
	unsigned short page_size;
	unsigned short blockshift;// shift block addr to page addr
	unsigned short pageshift;// shift page tp byte	
	unsigned char u8Internal_Chip_Number;
	unsigned char u8MultiChannel;
	unsigned short reserved;
	
}NFFS_Block_info_t;


#define USBCONFIG_PATTERN_S 0x55545353
#define USBCONFIG_PATTERN_E 0x55545345
typedef struct USB_Store_Info
{
	unsigned int heards;
	unsigned int version;
	unsigned int pattern;
	unsigned int hearde;

}USB_Store_Info_t;



///////////////////////////////
#define SUPPORT_MLLTI_BOOT 0

#if SUPPORT_MLLTI_BOOT
#define SUPPORT_SD0_BOOT	1
#define SUPPORT_SD1_BOOT	1
#define SUPPORT_SPI_BOOT	1
#else
#define SUPPORT_SD0_BOOT	0
#define SUPPORT_SD1_BOOT	0
#define SUPPORT_SPI_BOOT	0

#endif

enum ROM_RET_VAL {
	ROM_OK =0,
	ROM_FAIL,
	ROM_MAX
};

#endif

