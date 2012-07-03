/*
 * (C) 2003 David Woodhouse <dwmw2@infradead.org>
 *
 * Simple read-only (writable only for RAM) mtdblock driver
 */

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/blktrans.h>
#include <linux/crc32.h>

#include "support.InnoTab.h"

struct innotab_blockro_map
{
	int numOfGoodBlock;					// -1 means uninitialized, 0 means using on chip BBT
	int *logicalToPhysical;				// One entry for each erase block
};
struct innotab_blockro
{
	struct innotab_blockro_map map;
	struct mtd_blktrans_dev dev;
	struct semaphore open_lock;
	int open_cnt;
	char *bbt_buf;
	int logical_offset;
};
int innotab_nand_start_addr_to_phys( struct mtd_info *mtd, struct innotab_blockro_map *map, loff_t start, size_t len, loff_t *retAddr );
int fast_rescan_logic_to_phys_table( struct innotab_blockro *blockro );

#define PRINT_BAD_BLOCK_WARNING_RESCAN
//#define PRINT_GOOD_BLOCK_RESCAN

int rescan_logic_to_phys_table( struct mtd_info *mtd, struct innotab_blockro_map *map )
{
	int i, numblocks, accNumOfGoodBlock, startblock;
	loff_t addr;
	int accNumOfBadBlock;		// For debugging only

	map->numOfGoodBlock = -1;	// Initialize to the "invalid" signature first.

	accNumOfBadBlock = 0;
	accNumOfGoodBlock = 0;

	numblocks = ((unsigned int)mtd->size) / mtd->erasesize;
	startblock = 0;

	if ( map->logicalToPhysical == NULL )
	{
		printk ( "%s() - %d: logicToPhysical map table not allocated!\n", __PRETTY_FUNCTION__, __LINE__ );
		return -1;
	}

	for (i = startblock, addr = startblock * mtd->erasesize; i < (numblocks + startblock); addr += mtd->erasesize, i ++ ) {
		int ret;

		ret = mtd->block_isbad( mtd, addr );
		if ( ret )
		{
			accNumOfBadBlock ++;			// For debug only, should not use this value in core-logic
#ifdef PRINT_BAD_BLOCK_WARNING_RESCAN
			printk (KERN_WARNING "Bad eraseblock %d at 0x%08x\n", i, (unsigned int) addr);
#endif
		}
		else
		{
			map->logicalToPhysical[accNumOfGoodBlock] = i - startblock;
			accNumOfGoodBlock ++;
		}
	}

	map->numOfGoodBlock = accNumOfGoodBlock;
#ifdef PRINT_GOOD_BLOCK_RESCAN
	printk( "%s()-%d: Total number of good block = %d, bad block = %d\n", __PRETTY_FUNCTION__, __LINE__, map->numOfGoodBlock, ((unsigned int)mtd->size) / mtd->erasesize - map->numOfGoodBlock );
	printk( "%s()-%d: startblock = %d, numblocks = %d\n", __PRETTY_FUNCTION__, __LINE__, startblock, numblocks );
	for (i = startblock; i < (numblocks + startblock); i ++ )
	{
		printk( "%s()-%d: Logical Block [%d] to Physical Block [%d]\n", __PRETTY_FUNCTION__, __LINE__, i, map->logicalToPhysical[i] );
	}
#endif

	return 0;
}

// Read signature at the beginning of the block
#define MTDBLOCK_CRC_INIT							0xFFFFFFFFU
#define MTD_BBT_BLOCK_SIZE							512
#define MTD_SIG_LEN									16
int read_block_sig( struct mtd_info *mtd, int blkaddr, char *pageBuf, int pageBufSize, char *sig )
{
	int crc_from_flash;
	size_t retlen;

	if ( MTD_BBT_BLOCK_SIZE < MTD_SIG_LEN + 4 )			// Integrity check: always have room for signature + crc
	{
		printk ( "%s() - %d: MTD_BBT_BLOCK_SIZE too small to accomodate signature + crc\n", __PRETTY_FUNCTION__, __LINE__ );
		return -1;
	}

	if ( sizeof(int) != 4 )
	{
		printk ( "%s() - %d: Current code is developed based on the assumption of 32-bit integer.\n", __PRETTY_FUNCTION__, __LINE__ );
		return -1;
	}

	if ( pageBufSize < MTD_BBT_BLOCK_SIZE )
	{
		printk ( "%s() - %d: pageBufSize too small.  Should be program bug.\n", __PRETTY_FUNCTION__, __LINE__ );
		return -1;
	}

	if ( mtd->read(mtd, blkaddr, MTD_BBT_BLOCK_SIZE, &retlen, pageBuf) || retlen != MTD_BBT_BLOCK_SIZE )
	{
		return -1;
	}
		
	if ( memcmp( sig, pageBuf, MTD_SIG_LEN ) )
	{
		return -1;
	}

	crc_from_flash = *((int *)(&pageBuf[ MTD_BBT_BLOCK_SIZE - 4 ]));
	if ( crc_from_flash != crc32( MTDBLOCK_CRC_INIT, pageBuf, MTD_BBT_BLOCK_SIZE - 4 ) )
	{
		return -1;
	}

	return 0;
}

#define MAX_FIRST_GOOD_BLK_ADDR( device_size )		((device_size)/8)			//1st good block must be within the first 1/8 of the device
#define BBT_REGION_SIZE( device_size )				((device_size)/8)			//Bad block table must be within the last 1/8 of the device
static char gInnoTabBBTSig[MTD_SIG_LEN]	= { 'I', 'n', 'n', 'o', 'T', 'a', 'b', ' ', 'B', 'B', 'T', 0, 0, 0, 0, 0 };			// Signature for the BBT (bad block table)

int header_for_fast_scan_exist( struct innotab_blockro *blockro )
{
	struct mtd_info *mtd = blockro->dev.mtd;
	struct innotab_blockro_map *map = &blockro->map;
	loff_t blkaddr;

	if ( map->logicalToPhysical == NULL )
	{
		printk ( "%s() - %d: logicToPhysical map table not allocated!\n", __PRETTY_FUNCTION__, __LINE__ );
		return 0;
	}

	for ( blkaddr = 0; blkaddr <= MAX_FIRST_GOOD_BLK_ADDR((unsigned int)mtd->size); blkaddr += mtd->erasesize )
	{
		int ret;

		ret = mtd->block_isbad( mtd, blkaddr );
		if ( ret )
		{
#ifdef PRINT_BAD_BLOCK_WARNING_RESCAN
			printk (KERN_WARNING "Bad eraseblock at 0x%08x during BBT scan\n", (unsigned int) blkaddr);
#endif
			continue;
		}

		if ( read_block_sig( mtd, blkaddr, blockro->bbt_buf, MTD_BBT_BLOCK_SIZE, gInnoTabBBTSig ) )		// The valid signature must be at the 1st good block.
		{
			return 0;
		}
		else
		{
			break;		// Signature found
		}
	}
	if ( blkaddr > MAX_FIRST_GOOD_BLK_ADDR((unsigned int)mtd->size) )
	{
		return 0;
	}

	return 1;
}

int fast_rescan_logic_to_phys_table( struct innotab_blockro *blockro )
{
	struct mtd_info *mtd = blockro->dev.mtd;
	struct innotab_blockro_map *map = &blockro->map;
	int numblocks, firstBBTBlkAddr, lastBBTBlkAddr, numblock_mapped = 0;
	loff_t blkaddr;

#ifndef ENABLE_FAST_SCAN
	// The "fast scan" routine below is tested and should work as is.  But the author is never satisfied with the quality of the design.  Hence it's disabled by marco.
	// Improvement before delployment:
	//
	// 1.  Make the logical image as independent of the NAND parameters like erase block size as possible.
	// 1.1.  Have only one BBT marker at the end of the last erase block.  Arrange the rest of the structure like logical-to-phyiscal-address mapping table in strictly 
	//		 descending ordering in address.
	// 1.2.  Mapping should be done in terms of logical erase block rather than physical erase block.  Logical erase block size should be multiple of 512 and a common 
	//		 factor of all phyiscal erase block sizes supported.
	//
	// 2.  Tool support for reliable update:
	// 2.1.  Write the BBT strictly in ascending order of address, so the BBT marker at the end of the last erase block can serves as the "writing-complete" marker.
	//		 Note: This results in more complicated update policy (e.g. retry writing the whole BBT if errors occur in the middle).
	// 2.2.  Erase the last good erase block on the device first, and never writes to it except when updating BBT (i.e. don't write to this area when writing the 
	//		 "normal" part of the file system.)
	// 2.3.  If the last erase block turns bad in the update process, erase the good erase block immediately before it.  Only mark the last erase block bad on SUCCESSFUL 
	//		 erasure.
	//
	INNOTAB_HANG
#endif

	if ( map->logicalToPhysical == NULL )
	{
		printk ( "%s() - %d: logicToPhysical map table not allocated!\n", __PRETTY_FUNCTION__, __LINE__ );
		return -1;
	}

	map->numOfGoodBlock = -1;					// Initialize the "invalid" signature first.

	numblocks = ((unsigned int)mtd->size) / mtd->erasesize;
	lastBBTBlkAddr = ((unsigned int)mtd->size) - mtd->erasesize;
	firstBBTBlkAddr = ((unsigned int)mtd->size)
						- ( ( BBT_REGION_SIZE((unsigned int)mtd->size) + mtd->erasesize - 1 ) / mtd->erasesize ) * mtd->erasesize;

	for ( blkaddr = lastBBTBlkAddr; blkaddr >= firstBBTBlkAddr && numblock_mapped < numblocks; blkaddr -= mtd->erasesize ) {
		int ret;

		loff_t pageaddr;

		ret = mtd->block_isbad( mtd, blkaddr );
		if ( ret )
		{
#ifdef PRINT_BAD_BLOCK_WARNING_RESCAN
			printk (KERN_WARNING "Bad eraseblock at 0x%08x during BBT scan\n", (unsigned int) blkaddr);
#endif
			continue;
		}

		if ( read_block_sig( mtd, blkaddr, blockro->bbt_buf, MTD_BBT_BLOCK_SIZE, gInnoTabBBTSig ) )
		{
			return -1;
		}

		// Rest of the erase block is the bad block table
		for ( pageaddr = blkaddr + MTD_BBT_BLOCK_SIZE; pageaddr + MTD_BBT_BLOCK_SIZE <= blkaddr + mtd->erasesize && numblock_mapped < numblocks; pageaddr += MTD_BBT_BLOCK_SIZE )
		{
			size_t retlen;
			int numblock_in_current_page;

			ret = mtd->read(mtd, pageaddr, MTD_BBT_BLOCK_SIZE, &retlen, blockro->bbt_buf);
			if ( ret || retlen != MTD_BBT_BLOCK_SIZE )
			{
				return -1;
			}
			
			numblock_in_current_page = MTD_BBT_BLOCK_SIZE/4 < numblocks-numblock_mapped ? MTD_BBT_BLOCK_SIZE/4 : numblocks-numblock_mapped;
			memcpy( ((char *)map->logicalToPhysical) + numblock_mapped*4, blockro->bbt_buf, numblock_in_current_page*4 );
			numblock_mapped += numblock_in_current_page;
		}
	}
	if ( numblocks > numblock_mapped )
	{
		return -1;
	}

	map->numOfGoodBlock = 0;
#ifdef PRINT_GOOD_BLOCK_RESCAN
	{
		int i;

		printk( "%s()-%d: Total number of good block = %d\n", __PRETTY_FUNCTION__, __LINE__, map->numOfGoodBlock );
		printk( "%s()-%d: numblocks = %d\n", __PRETTY_FUNCTION__, __LINE__, numblocks );
		for (i = 0; i < numblocks; i ++ )
		{
			printk( "%s()-%d: Logical Block [%d] to Physical Block [%d]\n", __PRETTY_FUNCTION__, __LINE__, i, map->logicalToPhysical[i] );
		}
	}
#endif

	return 0;
}

int innotab_nand_start_addr_to_phys( struct mtd_info *mtd, struct innotab_blockro_map *map, loff_t start, size_t len, loff_t *retAddr )
{
	loff_t erase_block_idx;

	if ( map->numOfGoodBlock == -1 || map->logicalToPhysical == NULL )
	{
		printk ( "%s() - %d: logicToPhysical map table not initialized!\n", __PRETTY_FUNCTION__, __LINE__ );
		return -1;
	}

	erase_block_idx = ((unsigned int)start) / mtd->erasesize;
	if ( ( ((unsigned int)( start + len - 1 )) / mtd->erasesize ) != erase_block_idx )
	{
		// The input address range not within 1 erase block
		printk ( "%s() - %d: Address range not within a single block!\n", __PRETTY_FUNCTION__, __LINE__ );
		return -1;
	}

	*retAddr = map->logicalToPhysical[erase_block_idx] * mtd->erasesize; 
	*retAddr += start - (erase_block_idx * mtd->erasesize);
	return 0;
}


int mtdblock_open(struct mtd_blktrans_dev *dev)
{
	struct innotab_blockro *blockro = container_of(dev, struct innotab_blockro, dev);
	struct mtd_info *mtd = dev->mtd;
	struct innotab_blockro_map *map = &blockro->map;
	int ret = 0;

	down_interruptible( &blockro->open_lock );
	if ( !blockro->open_cnt )			// Rebuild the logical to physical block mapping the first time this device is opened.
	{
		memset( blockro->map.logicalToPhysical, -1, sizeof(int) * (((unsigned int)mtd->size)/mtd->erasesize) );
		if ( header_for_fast_scan_exist( blockro ) )
		{
#ifdef ENABLE_FAST_SCAN
			if ( fast_rescan_logic_to_phys_table( blockro ) )	// Try block-by-block bad block scanning procedure
			{
#else
			if ( rescan_logic_to_phys_table( mtd, map ) )		// Try block-by-block bad block scanning procedure
			{
#endif
				ret = -1;
				goto mtdblock_open_out;
			}

			blockro->logical_offset = MTD_BBT_BLOCK_SIZE;
		}
		else
		{
			if ( rescan_logic_to_phys_table( mtd, map ) )		// Try block-by-block bad block scanning procedure
			{
				ret = -1;
				goto mtdblock_open_out;
			}
			blockro->logical_offset = 0;
		}
	}
	blockro->open_cnt ++;
mtdblock_open_out:
	up( &blockro->open_lock );
	return ret;
}

int mtdblock_release(struct mtd_blktrans_dev *dev)
{
	struct innotab_blockro *blockro = container_of(dev, struct innotab_blockro, dev);

	down_interruptible( &blockro->open_lock );
	if ( blockro->open_cnt > 0 )
	{
		blockro->open_cnt --;
	}
	up( &blockro->open_lock );
	return 0;
}


static int mtdblock_readsect(struct mtd_blktrans_dev *dev,
			      unsigned long block, char *buf)
{
	size_t retlen;
	loff_t readAddr;
	struct innotab_blockro *blockro = container_of(dev, struct innotab_blockro, dev);

	if ( innotab_nand_start_addr_to_phys( dev->mtd, &blockro->map, (block * 512) + blockro->logical_offset, 512, &readAddr ) )
	{
		printk( "%s() - %d: logical_addr[%lux], physical_addr[%llx]\n", __PRETTY_FUNCTION__, __LINE__, (block * 512) + blockro->logical_offset, readAddr );
		return 1;		// Here it's assumed that the 512 byte block alway lie within a physical erase block on NAND
	}
	if (dev->mtd->read(dev->mtd, readAddr, 512, &retlen, buf))
	{
		return 1;
	}
	return 0;
}

static int mtdblock_writesect(struct mtd_blktrans_dev *dev,
			      unsigned long block, char *buf)
{
	size_t retlen;
	return 1;			// This function should never be called on a read-only device

	if (dev->mtd->write(dev->mtd, (block * 512), 512, &retlen, buf))
		return 1;
	return 0;
}

static void mtdblock_add_mtd(struct mtd_blktrans_ops *tr, struct mtd_info *mtd)
{
	struct innotab_blockro *blockro = vmalloc(sizeof(struct innotab_blockro)+MTD_BBT_BLOCK_SIZE);//kzalloc(sizeof(struct innotab_blockro), GFP_KERNEL);
	struct mtd_blktrans_dev *dev;

	if (!blockro)
		return;
	memset( blockro, 0, sizeof(struct innotab_blockro) );
	sema_init( &blockro->open_lock, 1 );
	blockro->open_cnt = 0;									// For clarity.  In theory redundant since we've memset() the whole structure to 0x00 before.
	blockro->bbt_buf = (char *)(&blockro[1]);
	memset( blockro->bbt_buf, 0, MTD_BBT_BLOCK_SIZE );
	blockro->logical_offset = 0;							// For clarity.  In theory redundant since we've memset() the whole structure to 0x00 before.

	dev = &blockro->dev;

	dev->mtd = mtd;
	dev->devnum = mtd->index;

	dev->size = mtd->size >> 9;
	dev->tr = tr;
	dev->readonly = 1;

	blockro->map.numOfGoodBlock = -1;
	blockro->map.logicalToPhysical = vmalloc( sizeof(int) * (((unsigned int)mtd->size)/mtd->erasesize) );//kzalloc( sizeof(int) * (((unsigned int)mtd->size)/mtd->erasesize), GFP_KERNEL);
	if ( blockro->map.logicalToPhysical == NULL )
	{
		printk ( "%s() - %d: Allocation for logical to physical mapping failed!\n", __PRETTY_FUNCTION__, __LINE__ );
		vfree(blockro);
		return;
	}
/*	memset( blockro->map.logicalToPhysical, 0, sizeof(int) * (((unsigned int)mtd->size)/mtd->erasesize) );

	if ( rescan_logic_to_phys_table( mtd, &blockro->map ) )
	{
		vfree(blockro->map.logicalToPhysical);
		vfree(blockro);
		return;
	}
*/

	add_mtd_blktrans_dev(dev);
}

static void mtdblock_remove_dev(struct mtd_blktrans_dev *dev)
{
	struct innotab_blockro *blockro = container_of(dev, struct innotab_blockro, dev);
	del_mtd_blktrans_dev(dev);

	// Ref prototype: container_of(pointer, container_type, container_field);
	vfree(blockro->map.logicalToPhysical);
	vfree(blockro);
}

static struct mtd_blktrans_ops mtdblock_tr = {
	.name		= "mtdblock",
	.major		= 31,
	.part_bits	= 0,
	.blksize 	= 512,
	.readsect	= mtdblock_readsect,
	.writesect	= mtdblock_writesect,
	.add_mtd	= mtdblock_add_mtd,
	.remove_dev	= mtdblock_remove_dev,
	.open		= mtdblock_open,
	.release	= mtdblock_release,
	.owner		= THIS_MODULE,
};

static int __init mtdblock_init(void)
{
	return register_mtd_blktrans(&mtdblock_tr);
}

static void __exit mtdblock_exit(void)
{
	deregister_mtd_blktrans(&mtdblock_tr);
}

module_init(mtdblock_init);
module_exit(mtdblock_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("David Woodhouse <dwmw2@infradead.org>");
MODULE_DESCRIPTION("Simple read-only block device emulation access to MTD devices");
