/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2002 by Sunplus Technology Co., Ltd.             *
 *                                                                        *
 *  This software is copyrighted by and is the property of Sunplus        *
 *  Technology Co., Ltd. All rights are reserved by Sunplus Technology    *
 *  Co., Ltd. This software may only be used in accordance with the       *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Sunplus Technology Co., Ltd.                       *
 *                                                                        *
 *  Sunplus Technology Co., Ltd. reserves the right to modify this        *
 *  software without notice.                                              *
 *                                                                        *
 *  Sunplus Technology Co., Ltd.                                          *
 *  19, Innovation First Road, Science-Based Industrial Park,             *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 *  Author: GuanYu Peng		                                              *
 *                                                                        *
 **************************************************************************/
#ifndef _NAND_API_H_
#define _NAND_API_H_

#include <mach/common.h>
#include <mach/gp_nand.h>

#include "NFTL/nftl.h"
#include "NFTL/nand.h"


#define SPMP_NF_S330	1

#define SUPPORT_NF_DISK0 1
#define SUPPORT_NF_DISK1 0	//第二片flash支持

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


/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/



/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

#define NAND_PATTERN_START 0
#define NAND_PATTERN_COUNT 2
#define BADBLK_INFO_PATTERN 0x42414442 //BADB

/// added by chenhn 	2008/08/05
typedef struct _devInfo_s {
	UINT8  makerId;
	UINT8  hwDevId;
	UINT8  active;      /* Inserted: 1, pulled-out: 0           */
	UINT8  reserved1;		// added by chenhn
	UINT16 megaByte;    /* Device capacity in unit of MB        */
	UINT16 reserved2;		// added by chenhn
	UINT8* pdevName;    /* ASCII name of this device            */
	UINT32 nrSects;     /* How many sectors of this device      */
	UINT32 nrFreeBytes; /* Free space in byte                   */
	UINT32 fsDepend;    /* For FAT, fsDepend = byte per cluster */
} _devInfo_t;

typedef struct nandInfo_s {
	UINT8 chipNr;
	UINT8 cellType;
	UINT8 pgPageNr;
	UINT8 interPgMulChip;
	UINT8 cachePg;
	UINT8 redunSize;
	UINT8 bus;
	UINT8 accessTime;
	UINT32 pageSize;
	UINT32 blkSize;
	UINT32 redOffset;
	UINT32 addrOption;	//20070322, champ
#if SUPPORT_NAND_MULTI_CS	/* renkai.liu@20070426, for Support Mult Nand CS*/
	UINT32 nrCS;
	UINT32 nrBlksPerCS;
#endif
	UINT8	nrBitsPagePerBlk;
	UINT8	nrBitsSectorPerPage;
	UINT8	reserved[2];
}nandInfo_t;

typedef struct smcDev_s {
	_devInfo_t common ;				// Common information for all cards 
	nandInfo_t*	p_nand_info;
	UINT16	  blkOffset;			// to move some zoom for the reserved data
	UINT16    nrPhyBlks;			// the real used blocks in the file system
	UINT16    rsvBlkOffset ;		// not used now
	UINT16    nrRsvBlks ;			// not used now
	UINT16    nrGoodOnes ;			// number of good blocks in a zone
	UINT16    sectorSize ;			// the sector size for the file system
	UINT32    blkSize ;				// the block size in bytes
	UINT32    nrBlks ;				// the real blocks we used
	UINT8     sectorBit ;			// this value is always 9
	UINT8     nrBitsSectorPerBlk ;	// bits shift for a page address to convert to a block one
	UINT8     addrCycle ;
	UINT8     eccCnt ;
	UINT8     redundBytes ;			//
	UINT8     flashMode ;
	UINT8     idExtraCycle1 ;   /*eric@03252005*/
	UINT8     idExtraCycle2 ;   /*eric@03252005*/

	UINT16*   pallocTable ;
	UINT32    currZone ;			// the zone size is 1024 blocks

	UINT8*    predund ;
	UINT8*	predund_NA;
	//UINT8     firstBlk ;		// modified by chenhn
	UINT32		firstBlk;
	UINT32    badBlks ;
	UINT8*    ppageBuf;/*eric@11282006*/
	UINT8*	ppageBuf_NA;
	UINT8*    pDummySts;/*eric@12222006*/
	UINT8*    ppageRedDummy;/*eric@12222006*/

#if NAND_SMALL_BUFFER	//20070504, champ
	UINT32 bufSize;
	UINT32 bufSects;
#endif //NAND_SMALL_BUFFER

} smcDev_t;
extern smcDev_t nf_Disk0;
#if (SUPPORT_NF_DISK1==1)
extern smcDev_t nf_Disk1;
#endif

/*************************************************************************
 *                       ADD FOR MULTI-PARTITION IOCTL                   *
 *************************************************************************/


/**************************************************************************
 *                  E X T E R N A L    R E F E R E N C E                  *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

#ifndef NO_BLOCK
#define NO_BLOCK (unsigned short)-1
#endif

/***************************** Linux Driver API **********************************/

typedef struct Nand_Func_s {
	UINT32 (*detect) (Chip_Info_t*);
	UINT32 (*read)   (UINT32 , UINT32 , UINT8 *);
	UINT32 (*write)  (UINT32 , UINT32 , UINT8 *);
	UINT32 (*erase)  (UINT32 );
	UINT32 (*CacheFlush)(void);
	void   (*remove) (void);
	void   (*fdisk) (void);

	smcDev_t* (*DevObjectGet)(void);
}nf_disk_func;
typedef struct nf_disk_s {
	smcDev_t* devinfo;
	nf_disk_func *func;
}nf_disk;

extern nf_disk_func* get_nf_disk_op(int disk_num);

#endif

