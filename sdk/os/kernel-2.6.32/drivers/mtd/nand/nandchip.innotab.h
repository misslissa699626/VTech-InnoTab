#ifndef __NANDCHIP_INNOTAB_H
#define __NANDCHIP_INNOTAB_H

#include "innotab.nand_int.h"
#include <../gp_nand/vtech_mtd/nand_innotab.h>

#define INNOTAB_INT_FLASH					0x00
#define INNOTAB_FLASH_CART					0x01

// Bit fields
#define MTD_ALLOW_ERASE_BB					0x01		// Allow erase of bad blcok

#define INNOTAB_NUM_OF_CHIP					4		// Maximum number of chips on innotab
#define MAX_NUM_PART						(8+1)	// Maximum number per chip: 6 partition + 1 placeholder
struct innotab_nand_chip_config
{
	int numOfPart;							// Number of partition
	struct mtd_partition part[MAX_NUM_PART];// Partition table for the current chip
	char chipName[MAX_LEN_CHIP_NAME];		// Name of the chip
};

#define CHIP_NO(chip)		(((struct innotab_nand_chip*)(((struct innotab_nand_int*)(chip->priv))->priv))->phyChipNo)

struct innotab_nand_chip
{
	int phyChipNo;								// Chip number to be passed to lower-level drivers
	void *dmaBuf;								// Special buffer for DMA operations.  To be used with physical-level drivers
	struct innotab_nand_chip_param param;		// Parameter from physical layer drivers. Use for operations bypassing MTD
	int in_mtd;									// 0 - not added to MTD, 1 - already added to MTD

	// Valid only when in_mtd = 1
	int numOfPart;							// Number of partition
	struct mtd_partition part[MAX_NUM_PART];// Partition table for the current chip
	char partName[MAX_NUM_PART*MAX_LEN_PART_NAME];// Buffer to hold the partition names
	int allow_erase_bb;							// Allow erasure of bad block?
};

int innotab_nand_chip_init( int phyChipNo, int lock );
int innotab_findchip( int phyChipNo );
int innotab_nand_chip_release( int phyChipNo, int lock );
int innotab_nand_chip_add_mtd( int phyChipNo, struct innotab_nand_chip_config *config, 
							  unsigned int mtd_options, int lock );
int innotab_nand_chip_del_mtd( int phyChipNo, int lock );
int innotab_nand_chip_mark_bad( int phyChipNo, int blkno, int lock );
int innotab_nand_chip_check_if_dlg_2011_ready( int phyChipNo, int lock );
int innotab_nand_chip_mark_if_dlg_2011_ready( int phyChipNo, int lock );
#endif
