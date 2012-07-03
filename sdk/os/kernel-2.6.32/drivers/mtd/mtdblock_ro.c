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
		if ( ret < 0 )
		{
			return -1;
		}
		else if ( ret > 0 )
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
#define MTD_SKIP_END_MARKER_OFF						(MTD_SIG_LEN)
#define MTD_HAVE_BBT_OFF							(MTD_SKIP_END_MARKER_OFF+1)
#define MTD_LAST_PARA_OFF							(MTD_HAVE_BBT_OFF)					// Currently the last item is the "have BBT" attribute
#define BBT_IN_SINLE_EB								1
int bbt_sig_found( struct mtd_info *mtd, int blkaddr, int offset, char *readBuf, int readBufSize, char *sig )
{
	int crc_from_flash;
	size_t retlen;

	if ( MTD_BBT_BLOCK_SIZE < MTD_LAST_PARA_OFF + 1 + 4 )			// Integrity check for program bug: always have room for signature + parameter + crc
	{
		printk ( "%s() - %d: MTD_BBT_BLOCK_SIZE too small to accomodate signature + parameter + crc\n", __PRETTY_FUNCTION__, __LINE__ );
		INNOTAB_HANG
	}

	if ( sizeof(int) != 4 )											// Check for bug
	{
		printk ( "%s() - %d: Current code is developed based on the assumption of 32-bit integer.\n", __PRETTY_FUNCTION__, __LINE__ );
		INNOTAB_HANG
	}

	if ( readBufSize < MTD_BBT_BLOCK_SIZE )							// Check for bug
	{
		printk ( "%s() - %d: readBufSize too small.  Should be program bug.\n", __PRETTY_FUNCTION__, __LINE__ );
		INNOTAB_HANG
	}

	if ( mtd->read(mtd, blkaddr + offset, MTD_BBT_BLOCK_SIZE, &retlen, readBuf) || retlen != MTD_BBT_BLOCK_SIZE )
	{
		return 0;
	}
		
	if ( memcmp( sig, readBuf, MTD_SIG_LEN ) )
	{
		return 0;
	}

	crc_from_flash = *((int *)(&readBuf[ MTD_BBT_BLOCK_SIZE - 4 ]));
	if ( crc_from_flash != crc32( MTDBLOCK_CRC_INIT, readBuf, MTD_BBT_BLOCK_SIZE - 4 ) )
	{
		//INNOTAB_REACH_HERE//
		return 0;
	}

/*	{
		char temp;
		// To be removed later...
		temp = readBuf[MTD_HAVE_BBT_OFF];

		readBuf[MTD_HAVE_BBT_OFF] = BBT_IN_SINLE_EB;
		printk( "%s() - %d: ========================================================\n", __PRETTY_FUNCTION__, __LINE__ );
		printk( "%s() - %d: New magic number: 0x%x\n", __PRETTY_FUNCTION__, __LINE__, crc32( MTDBLOCK_CRC_INIT, readBuf, MTD_BBT_BLOCK_SIZE - 4 ) );
		printk( "%s() - %d: ========================================================\n", __PRETTY_FUNCTION__, __LINE__ );

		readBuf[MTD_HAVE_BBT_OFF] = temp;
		// To be removed later...
	}
*/
	return 1;
}

#define MAX_FIRST_GOOD_BLK_ADDR( device_size )		((device_size)/8)			//1st good block must be within the first 1/8 of the device
#define BBT_REGION_SIZE( device_size )				((device_size)/8)			//Bad block table must be within the last 1/8 of the device
static char gInnoTabBBTSig[MTD_SIG_LEN]	= { 'I', 'n', 'n', 'o', 'T', 'a', 'b', ' ', 'B', 'B', 'T', 0, 0, 0, 0, 0 };			// Signature for the BBT (bad block table)

int header_for_fast_scan_exist( struct innotab_blockro *blockro, int *must_have_end )//, int *bbt_exist )
{
	struct mtd_info *mtd = blockro->dev.mtd;
	loff_t blkaddr;

	for ( blkaddr = 0; blkaddr <= MAX_FIRST_GOOD_BLK_ADDR((unsigned int)mtd->size); blkaddr += mtd->erasesize )
	{
		int ret;

		ret = mtd->block_isbad( mtd, blkaddr );
		if ( ret > 0 )
		{
#ifdef PRINT_BAD_BLOCK_WARNING_RESCAN
			printk (KERN_WARNING "Bad eraseblock at 0x%08x during BBT scan\n", (unsigned int) blkaddr);
#endif
			continue;
		}
		else if ( ret < 0 )
		{
			*must_have_end = 0;			// Redundant, just to make the return condition more predictable for ease of debugging in future.
			//*bbt_exist = 0;				// Redundant, just to make the return condition more predictable for ease of debugging in future.
			return -1;
		}

		if ( bbt_sig_found( mtd, blkaddr, 0, blockro->bbt_buf, MTD_BBT_BLOCK_SIZE, gInnoTabBBTSig ) )		// The valid signature must be at the 1st good block.
		{
			*must_have_end = blockro->bbt_buf[MTD_SKIP_END_MARKER_OFF] ? 0 : 1;
			//*bbt_exist = blockro->bbt_buf[MTD_HAVE_BBT_OFF] ? 1 : 0;
			break;		// Signature found
		}
		else
		{
			*must_have_end = 0;
			//*bbt_exist = 0;
			return 0;
		}
	}
	if ( blkaddr > MAX_FIRST_GOOD_BLK_ADDR((unsigned int)mtd->size) )
	{
		return 0;
	}

	return 1;
}

int end_mark_found( struct innotab_blockro *blockro, int *bbt_found )
{
	struct mtd_info *mtd = blockro->dev.mtd;
	//struct innotab_blockro_map *map = &blockro->map;
	int firstBBTBlkAddr, lastBBTBlkAddr;
	loff_t blkaddr;
	int found = 0;

	*bbt_found = 0;
	lastBBTBlkAddr = ((unsigned int)mtd->size) - mtd->erasesize;
	firstBBTBlkAddr = ((unsigned int)mtd->size)
						- ( ( BBT_REGION_SIZE((unsigned int)mtd->size) + mtd->erasesize - 1 ) / mtd->erasesize ) * mtd->erasesize;

	for ( blkaddr = lastBBTBlkAddr; blkaddr >= firstBBTBlkAddr; blkaddr -= mtd->erasesize ) {
		int ret;

		ret = mtd->block_isbad( mtd, blkaddr );
		if ( ret > 0 )
		{
#ifdef PRINT_BAD_BLOCK_WARNING_RESCAN
			printk (KERN_WARNING "Bad eraseblock at 0x%08x during BBT scan\n", (unsigned int) blkaddr);
#endif
			continue;
		}
		else if ( ret < 0 )
		{
			found = 0;
			break;
		}

		if ( bbt_sig_found( mtd, blkaddr, mtd->erasesize - MTD_BBT_BLOCK_SIZE, blockro->bbt_buf, MTD_BBT_BLOCK_SIZE, gInnoTabBBTSig ) )
		{
			size_t retlen;

			found = 1;

			if ( mtd->read(mtd, blkaddr + mtd->erasesize - MTD_BBT_BLOCK_SIZE, MTD_BBT_BLOCK_SIZE, &retlen, blockro->bbt_buf) || retlen != MTD_BBT_BLOCK_SIZE )
			{
				//INNOTAB_REACH_HERE//
				found = 0;					// Pretend that "valid" end-mark is not found if read error occurred.
			}
			else if ( blockro->bbt_buf[ MTD_HAVE_BBT_OFF ] == BBT_IN_SINLE_EB )			// Fall back to full-scan of the flash for bad block information in all situations otherwise...
			{
				int num_eb = ((unsigned int)mtd->size)/mtd->erasesize;
				
/*				printk( "%s() - %d: num_eb[%d], read_addr[0x%llx], read_size[%d]\n", __PRETTY_FUNCTION__, __LINE__, num_eb, blkaddr + mtd->erasesize - MTD_BBT_BLOCK_SIZE - num_eb * sizeof(int),
						num_eb * sizeof(int) );*/

				if ( mtd->read(mtd, blkaddr + mtd->erasesize - MTD_BBT_BLOCK_SIZE - num_eb * sizeof(int), num_eb * sizeof(int), &retlen, (unsigned char *)blockro->map.logicalToPhysical) || 
					 retlen != num_eb * sizeof(int) )
				{
					//INNOTAB_REACH_HERE//
					found = 0;
				}
				else
				{
					blockro->map.numOfGoodBlock = 0;
					//INNOTAB_REACH_HERE//
					*bbt_found = BBT_IN_SINLE_EB;
				}
			}
			//INNOTAB_REACH_HERE//
		}
		else
		{
			//INNOTAB_REACH_HERE//
			found = 0;
		}
		break;
	}

	return found;
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

	if ( map->logicalToPhysical[erase_block_idx] == -1 )
	{
		printk ( "%s() - %d: Logical block %lld mapped to invalid address!\n", __PRETTY_FUNCTION__, __LINE__, erase_block_idx );
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
	int ret = 0, must_have_end = 0, bbt_found = 0, fast_scan_header_found = -1;

	down_interruptible( &blockro->open_lock );
	if ( !blockro->open_cnt )			// Rebuild the logical to physical block mapping the first time this device is opened.
	{
		memset( blockro->map.logicalToPhysical, -1, sizeof(int) * (((unsigned int)mtd->size)/mtd->erasesize) );
		fast_scan_header_found = header_for_fast_scan_exist( blockro, &must_have_end );//, &bbt_found );
		if ( fast_scan_header_found == 1 )
		{
			map->numOfGoodBlock = -1;
			if ( must_have_end && ! end_mark_found( blockro, &bbt_found ) )			// Try block-by-block bad block scanning procedure
			{
				//INNOTAB_REACH_HERE//
				ret = -1;
				goto mtdblock_open_out;
			}

			//printk( "%s() - %d: bbt_found[%d]\n", __PRETTY_FUNCTION__, __LINE__, bbt_found );
			if ( !bbt_found && rescan_logic_to_phys_table( mtd, map ) )				// Try block-by-block bad block scanning procedure
			{
				//INNOTAB_REACH_HERE
				ret = -1;
				goto mtdblock_open_out;
			}

			//INNOTAB_REACH_HERE//
			blockro->logical_offset = MTD_BBT_BLOCK_SIZE;
		}
		else if ( fast_scan_header_found == 0 )
		{
			if ( rescan_logic_to_phys_table( mtd, map ) )				// Try block-by-block bad block scanning procedure
			{
				//INNOTAB_REACH_HERE
				ret = -1;
				goto mtdblock_open_out;
			}
			//INNOTAB_REACH_HERE//
			blockro->logical_offset = 0;
		}
		else
		{
			//INNOTAB_REACH_HERE
			ret = -1;
			goto mtdblock_open_out;										// Error reading
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
		printk( "%s() - %d: logical_addr[%lx], physical_addr[%llx]\n", __PRETTY_FUNCTION__, __LINE__, (block * 512) + blockro->logical_offset, readAddr );
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
