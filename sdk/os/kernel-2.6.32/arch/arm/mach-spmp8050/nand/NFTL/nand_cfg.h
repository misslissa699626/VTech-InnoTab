#ifndef _NAND_CFG_H_
#define _NAND_CFG_H_

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

#ifndef NO_NPB_PAGE
#define NO_NPB_PAGE (unsigned short)-1
#endif

#ifndef FRAGMENT_BLOCK
#define FRAGMENT_BLOCK (unsigned short)-2
#endif

#ifndef NFTL_NO_PAGE
#define NFTL_NO_PAGE (unsigned int)-1
#endif

#define MAX_FIFO_RESERVATIONS 3


#ifndef ALIGNED64
#define ALIGNED64 64
#endif

#define MAX_ZONE_COUNT 64
#define MAX_FRAGMENT_PER_ZONE 10
#define MAX_PHY_BLKS_PER_ZONE 1024
#if 1
#define BANK_BLK 960
#else
#define BANK_BLK 900
#endif


#define SECTOR_SIZE 512
#define SECTOR_SIZE_SHIFT 9



/// for compatibility
#define		NFC_SUCCESS								0
#define		NFC_FALSE								0xFF
#define		NFC_BUF_2_CACHE							0x01
#define		NFC_BUF_2_NAND							0x02

#define NFFS_SUCCESS					0
#define NFFS_BAD_BLOCK					-1
#define NFFS_PAGE_NOT_EXIST				-2
#define NFFS_LOGICAL_ADDRESS_ERROR		-3
#define NFFS_WRITE_PAGE_ERROR			-4
#define NFFS_GC_ERROR					-5
#define NFFS_CANT_GC_THIS_BLOCK			-6
#define NFFS_NO_BLK_TO_RECYCLE			-7
#define NFFS_CANT_RET_ANY_BLK			-8
#define NFFS_NO_PAGE					-9
#define NFFS_NAND_FLASH_FULL			-10
#define	NFFS_OVER_BLOCK_SIZE			-11
#define NFFS_READ_PAGE_ERROR			-12
#define NFFS_DATA_WRITE_BACK_ERROR -13
#define NFFS_ERROR						-14

///////////////////////////////////////////////////////////////////////////////////
#if 1
#define CHECK_NAND_PATTERN_IS_VALID 0x48415244//"HARD"
#define CHECK_NAND_PATTERN_OFFSET 60
#else
//#define CHECK_NAND_PATTERN_IS_VALID 0x4E46544C//"NFTL"
//#define CHECK_NAND_PATTERN_OFFSET 40
#endif
#define NAND_PATTERN_START 0
#define NAND_PATTERN_COUNT 2

#define NAND_BADBLKTABSAVE_START (NAND_PATTERN_START + NAND_PATTERN_COUNT)
#define NAND_BADBLKTABSAVE_COUNT 2

#define NANE_RESERVED_COUNT (NAND_PATTERN_COUNT+NAND_BADBLKTABSAVE_COUNT)

#define NAND_CACHE_ON_ 1

///////////////////////////////////////////////////////////////////////////////////
#define	NAND_USE_HI_MEM	0



#ifndef	_WIN32
#define	printf	diag_printf
#endif //#ifndef	_WIN32

/* add by mm.li 01-12,2011 clean warning */
#define SUPPORT_NAND_MULTI_CS		0	/*1: support; 0: Not support*/
#define		NAND_SMALL_BUFFER		0
/* add end */

#endif//#ifndef _FIFO_H_

