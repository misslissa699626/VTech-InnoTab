#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/compatmac.h>
#include <linux/interrupt.h>
#include <linux/bitops.h>
#include <linux/leds.h>
#include <asm/io.h>
#include <linux/vmalloc.h>

#include <mach/typedef.h>
#include <mach/gp_chunkmem.h>

#include <../gp_nand/vtech_mtd/nand_innotab.h>
#include "../support.InnoTab.h"

#define SIM_ERR

#ifdef SIM_ERR
int simerr_innotab_nand_read_ecc( int dev_no, loff_t start, size_t len, size_t *retlen, 
						       u_char *buf, u_char *oobbuf );
int simerr_innotab_nand_write_page( int dev_no, int page, u_char *buf, u_char *oob_buf );
int simerr_innotab_nand_erase_block( int dev_no, int page );
#endif

#define NUM_OF_MEM_DEV			4

#define MEM_DEV_PAGE_SIZE_0		512
#define MEM_DEV_ERASE_SIZE_0	(16*1024)
#define MEM_DEV_CHIP_SIZE_0		(16*1024*1024)
#define MEM_DEV_OOB_SIZE_0		(16)
#define MEM_DEV_OOB_AVAIL_0		(8)
#define MEM_DEV_MEM_SIZE_0		(((MEM_DEV_CHIP_SIZE_0/MEM_DEV_PAGE_SIZE_0)*MEM_DEV_OOB_SIZE_0)+MEM_DEV_CHIP_SIZE_0)

#define MEM_DEV_PAGE_SIZE_1		1024
#define MEM_DEV_ERASE_SIZE_1	(32*1024)
#define MEM_DEV_CHIP_SIZE_1		(8*1024*1024)
#define MEM_DEV_OOB_SIZE_1		(32)
#define MEM_DEV_OOB_AVAIL_1		(8)
#define MEM_DEV_MEM_SIZE_1		(((MEM_DEV_CHIP_SIZE_1/MEM_DEV_PAGE_SIZE_1)*MEM_DEV_OOB_SIZE_1)+MEM_DEV_CHIP_SIZE_1)

int MEM_DEV_PAGE_SIZE[NUM_OF_MEM_DEV] = { MEM_DEV_PAGE_SIZE_0, MEM_DEV_PAGE_SIZE_1, MEM_DEV_PAGE_SIZE_1, MEM_DEV_PAGE_SIZE_1  };
int MEM_DEV_ERASE_SIZE[NUM_OF_MEM_DEV] = { MEM_DEV_ERASE_SIZE_0, MEM_DEV_ERASE_SIZE_1, MEM_DEV_ERASE_SIZE_1, MEM_DEV_ERASE_SIZE_1 } ;
int MEM_DEV_CHIP_SIZE[NUM_OF_MEM_DEV] = { MEM_DEV_CHIP_SIZE_0, MEM_DEV_CHIP_SIZE_1, MEM_DEV_CHIP_SIZE_1, MEM_DEV_CHIP_SIZE_1 };
int MEM_DEV_OOB_SIZE[NUM_OF_MEM_DEV] = { MEM_DEV_OOB_SIZE_0, MEM_DEV_OOB_SIZE_1, MEM_DEV_OOB_SIZE_1, MEM_DEV_OOB_SIZE_1 };
int MEM_DEV_OOB_AVAIL[NUM_OF_MEM_DEV] = { MEM_DEV_OOB_AVAIL_0, MEM_DEV_OOB_AVAIL_1, MEM_DEV_OOB_AVAIL_1, MEM_DEV_OOB_AVAIL_1 }; 
int MEM_DEV_MEM_SIZE[NUM_OF_MEM_DEV] = { MEM_DEV_MEM_SIZE_0, MEM_DEV_MEM_SIZE_1, MEM_DEV_MEM_SIZE_1, MEM_DEV_MEM_SIZE_1 };

chunk_block_t mem_dev_blk[NUM_OF_MEM_DEV] = { { 0 } };
struct innotab_nand_chip_param mem_dev_param[NUM_OF_MEM_DEV] = { { 0 } };

static DECLARE_MUTEX(dummy_nand_in_use);

static int mem_alloc_for_dev( int dev_no )
{
	memset(&mem_dev_blk[dev_no], 0, sizeof(chunk_block_t));
	mem_dev_blk[dev_no].size = MEM_DEV_MEM_SIZE[dev_no];
	//mem_dev_blk[dev_no].addr = vmalloc( mem_dev_blk[dev_no].size );//, GFP_KERNEL );
	//printk( TERMCOL_green"%s() - %d: dev_no = %d, mem_dev_blk[dev_no].addr = %p, mem_dev_blk[dev_no].size = %d\n", __PRETTY_FUNCTION__, __LINE__, dev_no, mem_dev_blk[dev_no].addr, mem_dev_blk[dev_no].size );
	mem_dev_blk[dev_no].addr = gp_chunk_malloc( current->tgid, mem_dev_blk[dev_no].size );
	printk( TERMCOL_green"%s() - %d: dev_no = %d, mem_dev_blk[dev_no].addr = %p, phy_addr = %x, mem_dev_blk[dev_no].size = %d\n"TERMCOL_white, 
		__PRETTY_FUNCTION__, __LINE__, dev_no, mem_dev_blk[dev_no].addr, gp_chunk_pa( mem_dev_blk[dev_no].addr ), mem_dev_blk[dev_no].size );
	if ( mem_dev_blk[dev_no].addr )
	{
		memset( mem_dev_blk[dev_no].addr, 0xcc, mem_dev_blk[dev_no].size );		// All block should be block unless it NAND_ROM
		return 0;
	}
	else
	{
		return -1;
	}
	return -1;
}

static int mem_free_for_dev( int dev_no )
{
	//vfree( mem_dev_blk[dev_no].addr );
	gp_chunk_free( mem_dev_blk[dev_no].addr );
	memset(&mem_dev_blk[dev_no], 0, sizeof(chunk_block_t));
	return 0;
}

void memcpy_to_nand( char *dst, char *src, int len )
{
	int i;

	for ( i = 0; i < len; i ++ )
	{
		dst[i] = dst[i] & src[i];
	}
}

int innotab_nand_init( int dev_no, struct innotab_nand_chip_param *param )
{
	down_interruptible( &dummy_nand_in_use );
	if ( mem_alloc_for_dev( dev_no ) )
	{
		INNOTAB_REACH_HERE
		up( &dummy_nand_in_use );
		return -1;
	}

	mem_dev_param[dev_no].nand_maf_id = param->nand_maf_id = 0;
	mem_dev_param[dev_no].nand_dev_id = param->nand_dev_id = 0;
	mem_dev_param[dev_no].numchips = param->numchips = 1;
	mem_dev_param[dev_no].chipsize = param->chipsize = MEM_DEV_CHIP_SIZE[dev_no];
	mem_dev_param[dev_no].memoryType = param->memoryType = NAND_FLASH;//NAND_ROM;//
	mem_dev_param[dev_no].erasesize = param->erasesize = MEM_DEV_ERASE_SIZE[dev_no];
	mem_dev_param[dev_no].oobblock = param->oobblock = MEM_DEV_PAGE_SIZE[dev_no];
	mem_dev_param[dev_no].oobsize = param->oobsize = MEM_DEV_OOB_SIZE[dev_no];
	mem_dev_param[dev_no].oobavail = param->oobavail = MEM_DEV_OOB_AVAIL[dev_no];
	if ( INNOTAB_OOB_SIZE > MEM_DEV_OOB_AVAIL[dev_no] || MEM_DEV_OOB_AVAIL[dev_no] >= MEM_DEV_OOB_SIZE[dev_no] )
	{
		INNOTAB_HANG
	}
	mem_dev_param[dev_no].oobavailstart = param->oobavailstart = 0;			// Must review the whole file if this value is non-zero

	{	// So that all blocks are good on initialization
		int i;
		int numOfPage;
		char *addr;

		numOfPage = mem_dev_param[dev_no].chipsize / mem_dev_param[dev_no].oobblock;
		addr = (char *)mem_dev_blk[dev_no].addr;
		for ( i = 0; i < numOfPage; i ++ )
		{
			//printk( "%s()-%d: Mark good block - page = %d, addr = %p, len = %d\n",  __PRETTY_FUNCTION__, __LINE__, i,
			//	addr + mem_dev_param[dev_no].oobblock + mem_dev_param[dev_no].oobavail, mem_dev_param[dev_no].oobsize - mem_dev_param[dev_no].oobavail );
			memset( addr + mem_dev_param[dev_no].oobblock + mem_dev_param[dev_no].oobavail, 0xff, mem_dev_param[dev_no].oobsize - mem_dev_param[dev_no].oobavail );
			addr += mem_dev_param[dev_no].oobblock + mem_dev_param[dev_no].oobsize;
		}
	}

	up( &dummy_nand_in_use );
	return 0;
}

void innotab_nand_close( int dev_no )
{
	down_interruptible( &dummy_nand_in_use );
	mem_free_for_dev( dev_no );
	memset( &mem_dev_param[dev_no], 0, sizeof( struct innotab_nand_chip_param ) );
	up( &dummy_nand_in_use );
}

int innotab_nand_read_ecc( int dev_no, loff_t start, size_t len, size_t * retlen, 
						   u_char *buf, u_char *oobbuf )
{
	int i, numOfPage;
	char *off = NULL;
#ifdef SIM_ERR
	u_char *original_buf = buf, *original_oobbuf = oobbuf;
#endif

	down_interruptible( &dummy_nand_in_use );
	numOfPage = len / mem_dev_param[dev_no].oobblock;
	if ( !mem_dev_blk[dev_no].addr || ( ((unsigned int)start) % mem_dev_param[dev_no].oobblock ) || ( ((unsigned int)len) % mem_dev_param[dev_no].oobblock ) || ((start+len) > mem_dev_param[dev_no].chipsize) )
	{
		*retlen = 0;
		up( &dummy_nand_in_use );
		return -1;
	}

	off = ((char *)mem_dev_blk[dev_no].addr) + ( ((unsigned int)start) / mem_dev_param[dev_no].oobblock ) * ( mem_dev_param[dev_no].oobsize + mem_dev_param[dev_no].oobblock );
	for ( i = 0; i < numOfPage; i ++ )
	{
		memcpy( buf, off, mem_dev_param[dev_no].oobblock );
		buf += mem_dev_param[dev_no].oobblock;
		if ( oobbuf )
		{
			memcpy( oobbuf, off + mem_dev_param[dev_no].oobblock, mem_dev_param[dev_no].oobavail );
			oobbuf += mem_dev_param[dev_no].oobavail;
		}
		off += mem_dev_param[dev_no].oobsize + mem_dev_param[dev_no].oobblock;
	}
	*retlen = len;
	up( &dummy_nand_in_use );
#ifdef SIM_ERR
	return simerr_innotab_nand_read_ecc( dev_no, start, len, retlen, original_buf, original_oobbuf );
#endif
	return 0;
}

int innotab_nand_read_oob( int dev_no, int page, int off, size_t len, size_t * retlen, u_char *buf )
{
	char *src = NULL;

	down_interruptible( &dummy_nand_in_use );
	if ( !mem_dev_blk[dev_no].addr || ((off+len) > mem_dev_param[dev_no].oobavail) || (page+1)*mem_dev_param[dev_no].oobblock > mem_dev_param[dev_no].chipsize )
	{
		*retlen = 0;
		up( &dummy_nand_in_use );
		return -1;
	}

	src = ((char *)mem_dev_blk[dev_no].addr) + page * ( mem_dev_param[dev_no].oobsize + mem_dev_param[dev_no].oobblock ) + mem_dev_param[dev_no].oobblock + off;
	memcpy( buf, src, len );
	*retlen = len;
	up( &dummy_nand_in_use );
	return 0;
}

int innotab_nand_write_page( int dev_no, int page, u_char *buf, u_char *oob_buf )
{
	char *dst = NULL;

	down_interruptible( &dummy_nand_in_use );
	if ( !mem_dev_blk[dev_no].addr || (page+1)*mem_dev_param[dev_no].oobblock > mem_dev_param[dev_no].chipsize )
	{
		up( &dummy_nand_in_use );
		return -1;
	}

	dst = ((char *)mem_dev_blk[dev_no].addr) + page * ( mem_dev_param[dev_no].oobsize + mem_dev_param[dev_no].oobblock );
	memcpy_to_nand( dst, buf, mem_dev_param[dev_no].oobblock );
	if ( oob_buf )
		memcpy_to_nand( dst + mem_dev_param[dev_no].oobblock, oob_buf, mem_dev_param[dev_no].oobavail );

	up( &dummy_nand_in_use );
#ifdef SIM_ERR
	return simerr_innotab_nand_write_page( dev_no, page, buf, oob_buf );
#endif
	return 0;
}

int innotab_nand_write_oob( int dev_no, int page, int offset, size_t len, size_t * retlen, 
				const u_char * oobbuf)
{
	char *dst = NULL;

	down_interruptible( &dummy_nand_in_use );
	if ( !mem_dev_blk[dev_no].addr || ((offset+len) > mem_dev_param[dev_no].oobavail) || (page+1)*mem_dev_param[dev_no].oobblock > mem_dev_param[dev_no].chipsize )
	{
		*retlen = 0;
		up( &dummy_nand_in_use );
		return -1;
	}

	dst = ((char *)mem_dev_blk[dev_no].addr) + page * ( mem_dev_param[dev_no].oobsize + mem_dev_param[dev_no].oobblock ) + mem_dev_param[dev_no].oobblock + offset;
	memcpy_to_nand( dst, (char *)oobbuf, len );
	*retlen = len;
	up( &dummy_nand_in_use );
	return 0;
}

int innotab_nand_erase_block( int dev_no, int page )
{
	char *dst = NULL;

	down_interruptible( &dummy_nand_in_use );
	if ( !mem_dev_blk[dev_no].addr || (page+1)*mem_dev_param[dev_no].oobblock > mem_dev_param[dev_no].chipsize )
	{
		up( &dummy_nand_in_use );
		return -1;
	}

	dst = ((char *)mem_dev_blk[dev_no].addr) + page * ( mem_dev_param[dev_no].oobsize + mem_dev_param[dev_no].oobblock );
	memset( dst, 0xff, (mem_dev_param[dev_no].oobblock + mem_dev_param[dev_no].oobsize)*(MEM_DEV_ERASE_SIZE[dev_no]/MEM_DEV_PAGE_SIZE[dev_no]) );

	up( &dummy_nand_in_use );
#ifdef SIM_ERR
	return simerr_innotab_nand_erase_block( dev_no, page );
#endif
	return 0;
}

int innotab_nand_mark_bd_oob( int dev_no, int page )
{
	char *addr;

	down_interruptible( &dummy_nand_in_use );
	if ( mem_dev_param[dev_no].oobsize > mem_dev_param[dev_no].oobavail )
	{
		if ( !mem_dev_blk[dev_no].addr || (page+1)*mem_dev_param[dev_no].oobblock > mem_dev_param[dev_no].chipsize )
		{
			INNOTAB_REACH_HERE
			up( &dummy_nand_in_use );
			return -1;
		}

		addr = ((char *)mem_dev_blk[dev_no].addr) + page * ( mem_dev_param[dev_no].oobsize + mem_dev_param[dev_no].oobblock ) + mem_dev_param[dev_no].oobblock;
		addr[ mem_dev_param[dev_no].oobsize - 1 ] = 0;
		INNOTAB_REACH_HERE
		printk( "dev_no = %d, page = %d, addr = %p\n", dev_no, page, addr );
		INNOTAB_REACH_HERE
		up( &dummy_nand_in_use );
		return 0;
	}
	INNOTAB_REACH_HERE
	up( &dummy_nand_in_use );
	return -1;
}

int	innotab_nand_check_bd_oob( int dev_no, int page )
{
	char *addr;

	down_interruptible( &dummy_nand_in_use );
	if ( mem_dev_param[dev_no].oobsize > mem_dev_param[dev_no].oobavail )
	{
		if ( !mem_dev_blk[dev_no].addr || (page+1)*mem_dev_param[dev_no].oobblock > mem_dev_param[dev_no].chipsize )
		{
			INNOTAB_REACH_HERE
			up( &dummy_nand_in_use );
			return -1;
		}

		addr = ((char *)mem_dev_blk[dev_no].addr) + page * ( mem_dev_param[dev_no].oobsize + mem_dev_param[dev_no].oobblock ) + mem_dev_param[dev_no].oobblock;
		if ( addr[ mem_dev_param[dev_no].oobsize - 1 ] == 0xff )
		{
			//INNOTAB_REACH_HERE
			//printk( "%s()-%d: Good block - page = %d, addr = %p, value = %x", __PRETTY_FUNCTION__, __LINE__, page, &addr[ mem_dev_param[dev_no].oobsize - 1 ], addr[ mem_dev_param[dev_no].oobsize - 1 ] );
			up( &dummy_nand_in_use );
			return 0;
		}
		else
		{
			//INNOTAB_REACH_HERE
			printk( "%s()-%d: Bad block - page = %d, addr = %p, value = %x", __PRETTY_FUNCTION__, __LINE__, page, &addr[ mem_dev_param[dev_no].oobsize - 1 ], addr[ mem_dev_param[dev_no].oobsize - 1 ] );
			up( &dummy_nand_in_use );
			return 1;
		}
	}
	up( &dummy_nand_in_use );
	return 0;
}

int innotab_nand_check_wp( int dev_no )
{
	// Reminder: need to add "down_interruptible( &dummy_nand_in_use );" and "up( &dummy_nand_in_use );" if non-trivial operations are added in future.
	return 0;
}

/*
** Condition 
*/
#ifdef SIM_ERR

#define READ_ERR_BLK0		0
#define READ_ERR_BLK1		1
#define READ_ERR_BLK2		2

#define WRITE_ERR_BLK0		0x10
#define WRITE_ERR_BLK1		0x11

#define ERASE_ERR_BLK0		0x20
#define ERASE_ERR_BLK1		0x21

#define INNOTAB_SIM_ERR_MSG	INNOTAB_REACH_HERE
int simerr_innotab_nand_read_ecc( int dev_no, loff_t start, size_t len, size_t *retlen, 
						       u_char *buf, u_char *oobbuf )
{
	if ( ((unsigned int)start) == mem_dev_param[dev_no].erasesize * READ_ERR_BLK0 )
	{
	// #2, disabling this condition allows the mtd_pagetest.ko to verify other error conditions
	// Caught by mtd_pagetest, mtd_torturetest
	// NOT SURE if this can be caught by mtd_stresstest, since mtd_stresstest read at random location and with random length, and it never happened to read page 0 during my test.
		INNOTAB_SIM_ERR_MSG
		return -1;
	}
	else if ( ((unsigned int)start) == mem_dev_param[dev_no].erasesize * READ_ERR_BLK1 + 1 * mem_dev_param[dev_no].oobblock )
	{	// Caught by mtd_pagetest, mtd_stresstest, mtd_torturetest
		static int cnt = 0;

		INNOTAB_SIM_ERR_MSG
		cnt ++;
		if ( cnt < 2 )
		{
			printk( TERMCOL_red"%s() - %d: cnt = %d, EUCLEAN = %d\n"TERMCOL_white, __PRETTY_FUNCTION__, __LINE__, cnt, EUCLEAN );
			return -1;
		}
		printk( TERMCOL_red"%s() - %d: cnt = %d\n"TERMCOL_white, __PRETTY_FUNCTION__, __LINE__, cnt );
	}
	else if ( ((unsigned int)start) == mem_dev_param[dev_no].erasesize * READ_ERR_BLK2 + 2 * mem_dev_param[dev_no].oobblock  && 1 < *retlen )
	{
		// Caught by mtd_pagetest and mtd_torturetest, but NOT mtd_stresstest
		static int cnt = 0;

		INNOTAB_SIM_ERR_MSG
		cnt ++;
		if ( cnt < 2 )
		{
			printk( TERMCOL_red"%s() - %d: cnt = %d\n"TERMCOL_white, __PRETTY_FUNCTION__, __LINE__, cnt );
			buf[1] = ~buf[1];
		}
		printk( TERMCOL_red"%s() - %d: cnt = %d\n"TERMCOL_white, __PRETTY_FUNCTION__, __LINE__, cnt );
	}
	return 0;
}

int simerr_innotab_nand_write_page( int dev_no, int page, u_char *buf, u_char *oob_buf )
{
	if ( page == mem_dev_param[dev_no].erasesize / mem_dev_param[dev_no].oobblock * WRITE_ERR_BLK0 )
	{
		//INNOTAB_SIM_ERR_MSG			// Caught by mtd_stresstest.  Note: The error-address reported by mtd_stresstest may correspond to 1 block before this address.
									// Caught by mtd_torturetest, after disabling #1 and run a few times (to skip some occur-once simulated error.)
									// Caught by mtd_pagetest
		//return -1;
	}
	else if ( page == mem_dev_param[dev_no].erasesize / mem_dev_param[dev_no].oobblock * WRITE_ERR_BLK1 + 1 )
	{
		static int cnt = 0;

		cnt ++;
		if ( cnt < 2 )
		{	// Can be detected by mtd_pagetest, mtd_torturetest
			// CANNOT be detected by mtd_stresstest. This test program seem not to do read back test in general.


			//Disabling the folloiwng lines allow torturetest to catch other errors.
			//char *dst = ((char *)mem_dev_blk[dev_no].addr) + page * ( mem_dev_param[dev_no].oobsize + mem_dev_param[dev_no].oobblock );
			//dst[1] = ~dst[1];
			//INNOTAB_SIM_ERR_MSG
			//printk( TERMCOL_red"%s() - %d: page = %d, NAND location = %x\n"TERMCOL_white, __PRETTY_FUNCTION__, __LINE__, page, page * mem_dev_param[dev_no].oobblock );
		}
	}
	return 0;
}

int simerr_innotab_nand_erase_block( int dev_no, int page )
{
	if ( page == mem_dev_param[dev_no].erasesize / mem_dev_param[dev_no].oobblock * ERASE_ERR_BLK0 )
	{
		//INNOTAB_SIM_ERR_MSG			// #1: Caught by mtd_stresstest, mtd_pagetest, mtd_torturetest (but may block other test conditions, disable before proceed...)
		//return -1;
	}
	else if ( page == mem_dev_param[dev_no].erasesize / mem_dev_param[dev_no].oobblock * ERASE_ERR_BLK1 )
	{
		static int cnt = 0;

		cnt ++;
		if ( cnt < 2 )
		{	// Caught by mtd_torturetest after disabling #1
			// Can be detected by mtd_pagetest
			// CANNOT be detected by mtd_stresstest. This test program seem not to do read back test in general.


			// Disabling the following 3 line allow mtd_torturetest.ko to catch other error conditions
			//char *dst = ((char *)mem_dev_blk[dev_no].addr) + page * ( mem_dev_param[dev_no].oobsize + mem_dev_param[dev_no].oobblock );
			//dst[1] = ~dst[1];
			//INNOTAB_SIM_ERR_MSG
		}
	}
	return 0;
}
#endif