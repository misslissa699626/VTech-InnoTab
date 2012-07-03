/*
 * Initialization routines for Storio 2 NAND chip driver.
 */
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
#include <linux/crc32.h>

#ifdef CONFIG_MTD_PARTITIONS
#include <linux/mtd/partitions.h>
#endif

#include "../support.InnoTab.h"
#include "innotab.nand_int.h"
#include <../gp_nand/vtech_mtd/nand_innotab.h>
#include "nandchip.innotab.h"
#include "mtd_ioctl_dev.h"

/************************************************************************************
 * For replacement of function in nand_base.c
 ************************************************************************************/

/**
 * For replacement of nand_select_chip()
 */
static void innotab_select_chip(struct mtd_info *mtd, int chip)
{
	return;
}

/**
 * For replacement of nand_block_bad()
 */
static int innotab_block_bad( struct nand_chip *chip, int page )
{
	int res = 0;

	if ( ((struct innotab_nand_chip *)(((struct innotab_nand_int *)(chip->priv))->priv))->param.memoryType == NAND_ROM )
	{	// Always good block for NAND ROM
		return 0;
	}

	res = innotab_nand_check_bd_oob( CHIP_NO(chip), page );
	return res;
}

/*
 * For replacement of nand_wait()
 */
static int innotab_nand_wait(struct mtd_info *mtd, struct nand_chip *chip)
{
	return 0;			/* Always as if the device is ready and without error, for all timing 
						   are implemented at an even lower level
						*/
}

/**
 * Replacement of nand_write_page() 
 */
static int innotab_write_page(struct mtd_info *mtd, struct nand_chip *chip,
			   const uint8_t *buf, int page, int cached, int raw)
{
	int ret;
	struct innotab_nand_chip *this_innotab = (struct innotab_nand_chip*)(((struct innotab_nand_int*)(chip->priv))->priv);

	// Always assume the first 6 bytes are valid OOB data.  Other data don't care
	memcpy( this_innotab->dmaBuf, buf, mtd->writesize );
	ret = innotab_nand_write_page( CHIP_NO(chip), page, (u_char *)this_innotab->dmaBuf, NULL ); //chip->oob_poi );

	// $$ Start: To be removed later
	/*{
		int ret_read;
		int readsize = 0;
		memset( this_innotab->dmaBuf, 0xff, mtd->writesize );
		ret_read = innotab_nand_read_ecc( CHIP_NO(chip), page << chip->page_shift, mtd->writesize, &readsize, 
 	  						     this_innotab->dmaBuf, NULL );
		if ( ret_read != 0 || mtd->writesize != readsize || memcmp(this_innotab->dmaBuf,buf,mtd->writesize) )
		{
			int i, j;

			printk( "%s()-%d: Read-back test error\n", __PRETTY_FUNCTION__, __LINE__ );

			printk( "Attempt to write:\n" );
			for ( i = 0; i < 2; i ++ )
			{
				for ( j = 0; j < 16; j ++ )
				{
					printk( "%d", buf[j+i*16] );
					if ( j != 15 )
					{
						printk( ", " );
					}
					else
					{
						printk( "\n" );
					}
				}
			}

			printk( "Data read back:\n" );
			for ( i = 0; i < 2; i ++ )
			{
				for ( j = 0; j < 16; j ++ )
				{
					printk( "%d", ((char*)this_innotab->dmaBuf)[j+i*16] );
					if ( j != 15 )
					{
						printk( ", " );
					}
					else
					{
						printk( "\n" );
					}
				}
			}

		}
	}*/
	// $$ End: To be removed later

	return ret;
}


/**
 * To replace nand_read_page().  Will be called directly from nand_base.c
 */
static int innotab_read_page(struct mtd_info *mtd, struct nand_chip *chip, 
							 uint8_t *buf, int page)
{
	size_t readsize;
	int ret;
	struct innotab_nand_chip *this_innotab = (struct innotab_nand_chip*)(((struct innotab_nand_int*)(chip->priv))->priv);
	
	ret = innotab_nand_read_ecc( CHIP_NO(chip), page << chip->page_shift, mtd->writesize, &readsize, 
 	  						     this_innotab->dmaBuf, NULL ); //chip->oob_poi );
	if ( mtd->writesize != readsize )
	{
		return -EINVAL;			// Should not happen unless read beyond end of device
	}
	memcpy( buf, this_innotab->dmaBuf, mtd->writesize );
	memset( chip->oob_poi, 0xff, this_innotab->param.oobsize );
	return ret;
}

static int innotab_read_subpage(struct mtd_info *mtd, struct nand_chip *chip, uint32_t data_offs, uint32_t readlen, uint8_t *bufpoi)
{
	INNOTAB_HANG
	return 0;
}

static int innotab_read_oob(struct mtd_info *mtd, struct nand_chip *chip,
			     int page, int sndcmd )
{
#ifdef READ_WRITE_OOB_SUPPORT
	// $$ 16-Jun-2011: Code disabled due to lack of support from Ge+.  The code in this section is not reviewed yet.  So must re-check if need to enable in future.
	size_t retlen;
	int ret;

	ret = innotab_nand_read_oob( CHIP_NO(chip), page, 0, mtd->oobsize, &retlen, chip->oob_poi );
	if ( ret || retlen != mtd->oobsize )
		return -EINVAL;
	else
		return 0;
#else
	//printk( TERMCOL_red"%s() - %d: Should not be here!\n"TERMCOL_white, __PRETTY_FUNCTION__, __LINE__ );
	return -EINVAL;
#endif
}


static int innotab_write_oob(struct mtd_info *mtd, struct nand_chip *chip,
			      int page)
{
#ifdef READ_WRITE_OOB_SUPPORT
	// $$ 16-Jun-2011: Code disabled due to lack of support from Ge+.  The code in this section is not reviewed yet.  So must re-check if need to enable in future.
	int ret;
	size_t retlen;

	ret = innotab_nand_write_oob( CHIP_NO(chip), page, 0, mtd->oobsize, &retlen, chip->oob_poi);
	if ( ret || retlen != mtd->oobsize )
		return -EIO;
	else
		return 0;
#else
	//printk( TERMCOL_red"%s() - %d: Should not be here!\n"TERMCOL_white, __PRETTY_FUNCTION__, __LINE__ );
	return -EIO;
#endif
}

/*
** For replacement of nand_command()
*/
static void innotab_nand_command (struct mtd_info *mtd, unsigned command, int column, int page_addr)
{
	// dummy function so that standard code from MTD can run.
}

/**
 *  Board specific functions to be provided by chip drivers.
 */
static void innotab_cmd_ctrl(struct mtd_info *mtd, int dat, unsigned int ctrl)
{
}

static int innotab_device_ready(struct mtd_info *mtd)
{
	return 1;
}

static int	innotab_ecc_calculate(struct mtd_info *mtd, const uint8_t *dat, uint8_t *ecc_code)
{
	INNOTAB_HANG
	return 0;
}

static int innotab_ecc_correct(struct mtd_info *mtd,uint8_t *dat, uint8_t *read_ecc, uint8_t *calc_ecc)
{
	INNOTAB_HANG
	return 0;
}

static void innotab_ecc_hwctl(struct mtd_info *mtd, int mode)
{
	INNOTAB_HANG
}

// Member functions for struct innotab_nand_int
static int innotab_check_wp(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;

	return innotab_nand_check_wp( CHIP_NO(this) );
}

static int innotab_erase_wait(struct nand_chip *chip, int page)
{
	int ret;
	struct innotab_nand_int *nand_int = chip->priv;
	struct innotab_nand_chip *this_innotab = (struct innotab_nand_chip*)(((struct innotab_nand_int*)(chip->priv))->priv);

	ret = innotab_nand_erase_block( CHIP_NO(chip), page );
	if ( ! ret )
	{
		int i, j;
		size_t readsize;

		// printk( "%s()-%d: nand_int->erasesize[%d], nand_int->oobblock[%d], page[%d]\n", __PRETTY_FUNCTION__, __LINE__, nand_int->erasesize, nand_int->oobblock, page );
		for ( i = 0; i < nand_int->erasesize / nand_int->oobblock; i ++ )
		{
			ret = innotab_nand_read_ecc( CHIP_NO(chip), (page + i) << chip->page_shift, nand_int->oobblock, &readsize, this_innotab->dmaBuf, NULL );
			if ( ret || readsize != nand_int->oobblock )
			{
				ret = NAND_STATUS_FAIL;
				break;
			}

			for ( j = 0; j < nand_int->oobblock; j ++ )
			{
				if ( ((unsigned char *)this_innotab->dmaBuf)[j] != 0xff )
				{
					ret = NAND_STATUS_FAIL;
					break;
				}
			}
			if ( ret )
			{
				break;
			}
		}
	}
	return ret;
}

static int innotab_mark_bad( struct nand_chip *chip, int page )
{
	int ret;

	ret = innotab_nand_mark_bd_oob( CHIP_NO(chip), page );
	if ( ret || innotab_nand_check_bd_oob( CHIP_NO(chip), page ) != 1 )	// Always read-back to ensure mark bad block success
		return -EIO;
	return 0;
}

static int innotab_allow_erase_bb( struct mtd_info *mtd )
{
	struct nand_chip *this = mtd->priv;
	struct innotab_nand_int *this_innotab_int = this->priv;
	struct innotab_nand_chip *this_innotab = this_innotab_int->priv;
	
	return this_innotab->allow_erase_bb;
}

// Master list of all NAND device, single device each
static struct mtd_info *innotab_mtd[INNOTAB_NUM_OF_CHIP] = { 0 };
static struct nand_hw_control gInnoTab_HWControl;
static DECLARE_MUTEX(innotab_nand_in_use);

static void innotab_nand_driver_init( void )
{
	spin_lock_init(&gInnoTab_HWControl.lock);
	gInnoTab_HWControl.active = NULL;
	init_waitqueue_head(&gInnoTab_HWControl.wq);
}

int innotab_nand_chip_init( int phyChipNo, int lock )
{
	struct mtd_info *this_mtd = NULL;
	struct nand_chip *this = NULL;
	struct innotab_nand_int *this_innotab_int = NULL;
	struct innotab_nand_chip *this_innotab = NULL;
	int err = 0, total_buf_size, chipIdx, i;
	int ret;

	// Note: Only the "priv" fields of this_mtd and this is initialized in this function.

	if ( lock )
	{
		down_interruptible( &innotab_nand_in_use );
	}
	chipIdx = -1;
	for ( i = 0; i < INNOTAB_NUM_OF_CHIP; i ++ )
	{
		if ( innotab_mtd[i] != 0 )
		{
			struct nand_chip *temp = innotab_mtd[i]->priv;
			struct innotab_nand_int *temp_int = temp != 0 ? temp->priv : 0;
			struct innotab_nand_chip *temp_innotab = temp_int != 0 ? temp_int->priv : 0;

			if ( temp_innotab == 0 )
			{	// Should not happen for properly initialized structure
				printk ("%s() - %d: InnoTab NAND MTD device table corrupted: %d.\n", __PRETTY_FUNCTION__, __LINE__, i );
				err = -ENXIO;
				goto innotab_out;
			}

			if ( temp_innotab->phyChipNo == phyChipNo )
			{
				printk ("%s() - %d: InnoTab NAND MTD device already initalized: %d.\n", __PRETTY_FUNCTION__, __LINE__, i );
				err = -ENXIO;
				goto innotab_out;
			}
		}
		else
		{	// Free slot found 
			if ( chipIdx == -1 )
				chipIdx = i;
		}
	}
	if ( chipIdx == -1 )
	{
		printk ("%s() - %d: Too many innotab device!\n", __PRETTY_FUNCTION__, __LINE__ );
		err = -ENXIO;
		goto innotab_out;
	}

	/* Allocate memory for MTD device structure and private data */
	total_buf_size = sizeof(struct mtd_info) + sizeof (struct nand_chip) + sizeof(struct innotab_nand_int) + sizeof(struct innotab_nand_chip);
	this_mtd = kmalloc ( total_buf_size, GFP_KERNEL);
	if (!this_mtd) {
		printk ("%s() - %d: Unable to allocate InnoTab NAND MTD device structure.\n", __PRETTY_FUNCTION__, __LINE__ );
		err = -ENOMEM;
		goto innotab_out;
	}
	memset( (char *)this_mtd, 0, total_buf_size );

	/* Get pointer to private data */
	this = (struct nand_chip *) (&this_mtd[1]);
	this_innotab_int = (struct innotab_nand_int *)(&this[1]);
	this_innotab = (struct innotab_nand_chip *) (&this_innotab_int[1]);

	/* Link the private data with the MTD structure */
	this_mtd->priv = this;
	this->priv = this_innotab_int;
	this_innotab_int->priv = this_innotab;

	this_innotab->phyChipNo = phyChipNo;			// Theoretically redundant, duplicate for efficiency only
	ret = innotab_nand_init( phyChipNo, &this_innotab->param );
	if ( ret )
	{
		kfree( this_mtd );
		err = -ENXIO;
		goto innotab_out;
	}
	// Start: Currently these values from the low level drivers are useless.  Use fake values for convenience.
	this_innotab->param.oobsize = INNOTAB_OOB_SIZE;
	this_innotab->param.oobavail = INNOTAB_OOB_SIZE;
	this_innotab->param.oobavailstart = 0;
	// Start: Currently these values from the low level drivers are useless.  Use fake values for convenience.
	this_innotab->dmaBuf = kmalloc(this_innotab->param.oobblock, GFP_DMA);//GFP_KERNEL);//
	if ( !this_innotab->dmaBuf )
	{
		innotab_nand_close( phyChipNo );
		kfree( this_mtd );
		err = -ENXIO;
		goto innotab_out;
	}
	this_innotab->in_mtd = 0;

	this_innotab_int->check_wp = innotab_check_wp;
	this_innotab_int->erase_wait = innotab_erase_wait;
	this_innotab_int->mark_bad = innotab_mark_bad;
	this_innotab_int->check_bad = innotab_block_bad;
	this_innotab_int->allow_erase_bb = innotab_allow_erase_bb;
	memset( this_innotab_int->name, 0, MAX_LEN_CHIP_NAME );
	this_innotab_int->chipsize = this_innotab->param.chipsize;
	this_innotab_int->erasesize = this_innotab->param.erasesize;
	this_innotab_int->oobblock = this_innotab->param.oobblock;
	this_innotab_int->oobsize = this_innotab->param.oobavail;

	innotab_mtd[chipIdx] = this_mtd;

innotab_out:
	if ( lock )
	{
		up ( &innotab_nand_in_use );
	}
	return err;
}

int innotab_findchip( int phyChipNo )
{
	struct nand_chip *this = NULL;
	struct innotab_nand_int *this_innotab_int = NULL;
	struct innotab_nand_chip *this_innotab = NULL;
	int i;
	int chipIdx = -1;

	for ( i = 0; i < INNOTAB_NUM_OF_CHIP; i ++ )
	{
		if ( innotab_mtd[i] != 0 )
		{
			this = innotab_mtd[i]->priv;
			this_innotab_int = this != 0 ? this->priv : 0;
			this_innotab = this_innotab_int != 0 ? this_innotab_int->priv : 0;

			if ( this_innotab->phyChipNo == phyChipNo )
			{
				if ( chipIdx == -1 )
					chipIdx = i;
				else
				{
					printk ("%s() - %d: Duplicated entries in InnoTab NAND MTD device table: %d, %d.\n", 
						__PRETTY_FUNCTION__, __LINE__, i, chipIdx );
					chipIdx = -1;		// Force error handing at caller
					break;
				}
			}
		}
	}
	return chipIdx;
}

int innotab_nand_chip_release( int phyChipNo, int lock )
{
	int chipIdx;
	int err = 0;
	struct innotab_nand_chip *this_innotab;

	if ( lock )
	{
		down_interruptible( &innotab_nand_in_use );
	}
	chipIdx = innotab_findchip( phyChipNo );
	if ( chipIdx == -1 )
	{
		err = -1;
		printk( "%s() - %d: InnoTab NAND MTD device not found: %d\n", __PRETTY_FUNCTION__, __LINE__, phyChipNo );
		goto chip_release_out;
	}
	if ( ((struct innotab_nand_chip *)(((struct innotab_nand_int *)(((struct nand_chip *)(innotab_mtd[chipIdx]->priv))->priv))->priv))->in_mtd )
	{
		err = -1;
		printk( "%s() - %d: InnoTab NAND MTD device still used by MTD: %d\n", __PRETTY_FUNCTION__, __LINE__, phyChipNo );
		goto chip_release_out;
	}
	this_innotab = ((struct innotab_nand_int *)(((struct nand_chip *)(innotab_mtd[chipIdx]->priv))->priv))->priv;
	kfree( this_innotab->dmaBuf );
	kfree( innotab_mtd[chipIdx] );
	innotab_mtd[chipIdx] = 0;
	innotab_nand_close( phyChipNo );
chip_release_out:
	if ( lock )
	{
		up ( &innotab_nand_in_use );
	}
	return err;
}

static struct nand_ecclayout innotab_oob = {
	.eccbytes = 0,
	.eccpos = {0},
	.oobfree = {
		{.offset = 0,
		 .length = INNOTAB_OOB_SIZE}}
};

char gCartPartitionNameHead[MAX_LEN_PART_NAME] = "Inno_?";
int add_if_dlg_2011_partition( struct mtd_info *this_mtd );
int innotab_nand_chip_add_mtd( int phyChipNo, struct innotab_nand_chip_config *config, unsigned int mtd_options, int lock )
{
	int err = 0;
	struct mtd_info *this_mtd;
	struct nand_chip *this = NULL;
	struct innotab_nand_int *this_innotab_int = NULL;
	struct innotab_nand_chip *this_innotab = NULL;
	int chipIdx = -1;
	
	if ( lock )
	{
		down_interruptible( &innotab_nand_in_use );
	}
	chipIdx = innotab_findchip( phyChipNo );
	if ( chipIdx == -1 )
	{
		printk ("Trying to add an uninitailized device to MTD: %d.\n", phyChipNo );
		err = -ENOMEM;
		goto innotab_add_mtd_out;
	}
	this_mtd = innotab_mtd[chipIdx];
	this = (struct nand_chip *)(this_mtd->priv);
	this_innotab_int = (struct innotab_nand_int *)(this->priv);
	this_innotab = (struct innotab_nand_chip *)(this_innotab_int->priv);

	if ( this_innotab->in_mtd )
	{
		printk ("Device already in MTD: %d.\n", phyChipNo );
		err = -ENOMEM;
		goto innotab_add_mtd_out;
	}

	// Reinitalize all structure visible to MTD core to zero */
	memset( this_mtd, 0, sizeof( *this_mtd ) );
	memset( this, 0, sizeof( *this ) );
	this_mtd->priv = this;
	this->priv = this_innotab_int;

	// Reset the chip name, if necessary
	memcpy( this_innotab_int->name, config->chipName, MAX_LEN_CHIP_NAME );

	// fields for innotab_nand_chip structure, except allow_erase_bb
	this_innotab->numOfPart = config->numOfPart;
	memcpy( this_innotab->part, config->part, MAX_NUM_PART * sizeof( struct mtd_partition ) );
	memset( this_innotab->partName, 0, sizeof(this_innotab->partName) );
	{
		int i;
		char *ptr = this_innotab->partName;

		for ( i = 0; i < MAX_NUM_PART; i ++ )
		{
			this_innotab->part[i].name = ptr;
			if ( i < this_innotab->numOfPart )
			{
				strncpy( ptr, config->part[i].name, MAX_LEN_PART_NAME );
				ptr[MAX_LEN_PART_NAME-1] = 0;		// Ensure NULL terminator.
			}

			//printk( "%s()-%d: gCartPartitionNameHead[] of length %d: \"%s\"\n", __PRETTY_FUNCTION__, __LINE__, strlen(gCartPartitionNameHead), gCartPartitionNameHead );
			if ( strlen(gCartPartitionNameHead) <= MAX_LEN_PART_NAME &&					// For safety, usually this condition is guaranteed to be true by suitable choice of signature 
				 !strncmp( gCartPartitionNameHead, this_innotab->part[i].name, strlen(gCartPartitionNameHead)-1 ) &&
				 this_innotab->part[i].name[strlen(gCartPartitionNameHead)-1] == '?' )
			{
				if ( this_innotab->param.memoryType == NAND_ROM )
					this_innotab->part[i].name[strlen(gCartPartitionNameHead)-1] = 'R';
				else
					this_innotab->part[i].name[strlen(gCartPartitionNameHead)-1] = 'F';
			}

			ptr += MAX_LEN_PART_NAME;
		}
	}
	if ( !this_innotab->phyChipNo )
		add_if_dlg_2011_partition( this_mtd );

	// All BOARDSPECIFIC fields
	this->IO_ADDR_R = 0;
	this->IO_ADDR_W = 0;
	this->cmd_ctrl = innotab_cmd_ctrl;
	this->dev_ready = innotab_device_ready;
	/* Field not used.  Fill in something that will likely to crash the system, for easy debug */
	this->chip_delay = 0x7FFFFFFF;		
	this->options = 0; 			/* $$ possible to use NAND_USE_FLASH_BBT in future? */
	this->options |= NAND_NO_SUBPAGE_WRITE;
	if ( mtd_options & MTD_ALLOW_ERASE_BB)
	{
		this->options |= NAND_SKIP_BBTSCAN;
		this_innotab->allow_erase_bb = 1;
	}
	else
		this_innotab->allow_erase_bb = 0;

	this->ecc.mode = NAND_ECC_NONE;

	// Initialization of unused parameters, for ease of debugging
	this->ecc.prepad = 0;
	this->ecc.postpad = 0;
	this->ecc.hwctl = innotab_ecc_hwctl;
	this->ecc.calculate = innotab_ecc_calculate;
	this->ecc.correct = innotab_ecc_correct;

	// Standard layout for all Innotab NAND
	this->ecc.layout = &innotab_oob;

	// Should not be called, and seems to be  uninitailized in MTD for NAND_ECC_NONE.
	// So point to a function that prints message for ease of debugging
	this->ecc.read_subpage = innotab_read_subpage;	

	// Do NOT initialize the followings so that they will point to MTD default
	// version and hang up when invoked, for ease of debugging.
	this->ecc.write_page = this->ecc.write_page_raw = 0;

	// Read / write operations for page / oob data
	this->ecc.read_page = this->ecc.read_page_raw = innotab_read_page;
	this->ecc.read_oob = innotab_read_oob;
	this->ecc.write_oob = innotab_write_oob;

	// In cut down version of nand_based.c, this is the entry point for write page.
	this->write_page = innotab_write_page;

	this->cmdfunc = innotab_nand_command;
	this->waitfunc = innotab_nand_wait;
	this->select_chip = innotab_select_chip;
	this->controller = &gInnoTab_HWControl;

	this_mtd->subpage_sft = 0;

	this_mtd->owner = THIS_MODULE;

	err = nand_scan(this_mtd, 1);
	if ( err )
	{
		printk( "%s() - %d: nand_scan() failure.  Chip: %d\n", __PRETTY_FUNCTION__, __LINE__, phyChipNo );
		nand_release( this_mtd );
		goto innotab_add_mtd_out;
	}
	add_mtd_partitions(this_mtd, this_innotab->part, this_innotab->numOfPart );

	this_innotab->in_mtd = 1;

innotab_add_mtd_out:
	if ( lock )
	{
		up ( &innotab_nand_in_use );
	}
	return err;
}

int nand_release_with_err_detection(struct mtd_info *mtd);
int innotab_nand_chip_del_mtd( int phyChipNo, int lock )
{
	int chipIdx;
	int err = 0;
	struct innotab_nand_chip *this_innotab = NULL;

	if ( lock )
	{
		down_interruptible( &innotab_nand_in_use );
	}
	chipIdx = innotab_findchip( phyChipNo );
	if ( chipIdx == -1 )
	{
		err = -1;
		printk( "%s() - %d: InnoTab NAND MTD device not found: %d\n", __PRETTY_FUNCTION__, __LINE__, phyChipNo );
		goto del_mtd_out;
	}
	this_innotab = (struct innotab_nand_chip *)(((struct innotab_nand_int *)(((struct nand_chip *)(innotab_mtd[chipIdx]->priv))->priv))->priv);
	if ( this_innotab->in_mtd == 0 )
	{
		err = -1;
		printk( "%s() - %d: InnoTab NAND MTD device already detached: %d\n", __PRETTY_FUNCTION__, __LINE__, phyChipNo );
		goto del_mtd_out;
	}
	err = nand_release_with_err_detection( innotab_mtd[chipIdx] );
	if ( err )
	{
		err = -1;
		printk( "%s() - %d: Innotab nand_release() error.  Chip number: %d\n", __PRETTY_FUNCTION__, __LINE__, phyChipNo );
		goto del_mtd_out;
	}
	this_innotab->in_mtd = 0;
	this_innotab->numOfPart = 0;
	memset( this_innotab->part, 0, MAX_NUM_PART * sizeof( struct mtd_partition ) );
	this_innotab->allow_erase_bb = 0;
del_mtd_out:
	if ( lock )
	{
		up ( &innotab_nand_in_use );
	}
	return err;
}

int innotab_nand_chip_mark_bad( int phyChipNo, int blkno, int lock )
{
	int chipIdx;
	int err = 0;
	int page;
	struct innotab_nand_chip *this_innotab;

	if ( lock )
	{
		down_interruptible( &innotab_nand_in_use );
	}

	chipIdx = innotab_findchip( phyChipNo );
	if ( chipIdx == -1 )
	{
		err = -1;
		printk( "%s() - %d: InnoTab NAND MTD device not found: %d\n", __PRETTY_FUNCTION__, __LINE__, phyChipNo );
		goto mark_bad_out;
	}
	this_innotab = (struct innotab_nand_chip *)(((struct innotab_nand_int *)(((struct nand_chip *)(innotab_mtd[chipIdx]->priv))->priv))->priv);
	if ( this_innotab->in_mtd )
	{
		err = -1;
		printk( "%s() - %d: InnoTab NAND MTD device still used by MTD: %d.  Unmount before mark bad.\n", __PRETTY_FUNCTION__, __LINE__, phyChipNo );
		goto mark_bad_out;
	}
	if ( this_innotab->param.erasesize * blkno >= this_innotab->param.chipsize )
	{
		err = -1;
		printk( "%s() - %d: Mark bad block beyond end of device %d.  Max blk num: %lu, Attempted blk no: %d\n", __PRETTY_FUNCTION__, __LINE__, phyChipNo,  
				this_innotab->param.chipsize / this_innotab->param.erasesize - 1, blkno );
		goto mark_bad_out;
	}
	page = ( this_innotab->param.erasesize / this_innotab->param.oobblock ) * blkno;
	if ( innotab_nand_mark_bd_oob( phyChipNo, page ) )
	{
		err = -1;
		printk( "%s() - %d: InnoTab NAND MTD mark bad block failure.  Chip: %d, Block: %d, Page: %d\n", __PRETTY_FUNCTION__, __LINE__, phyChipNo, blkno, page );
		goto mark_bad_out;					// Redundant, but convenient for maintenance to always have "goto mark_bad_out" explicitly.
	}
mark_bad_out:
	if ( lock )
	{
		up ( &innotab_nand_in_use );
	}
	return err;
}

// $$ Temporary only: initialize MTD nand structure, so that we don't need to write a separate module
int nand_base_init(void);
void nand_base_exit(void);
// $$ End - Temporary only: initialize MTD nand structure, so that we don't need to write a separate module

//$$ Start - Temporary only - to mount the internal partitions
int activate_internal_nand( void );
//$$ End - Temporary only - to mount the internal partitions

static int innotab_all_nand_init( void )
{
	int ret;
	int err = 0;

	// $$ Temporary only: initialize MTD nand structure, so that we don't need to write a separate module
	ret = nand_base_init();
	if ( ret )
	{
		return ret;
	}
	// $$ End - Temporary only: initialize MTD nand structure, so that we don't need to write a separate module

	// Initialize global variables
	innotab_nand_driver_init();

	// Create device file
	ret = vmtd_file_init();			// $$ Not good programming practice.  All call sequence should be top down.
	if ( ret )
	{
		err = -1;
		goto all_nand_init_out;
	}
	
all_nand_init_out:
	// $$ Temporary only: initialize MTD nand structure, so that we don't need to write a separate module
	if ( err )
		nand_base_exit();
	// $$ End - Temporary only: initialize MTD nand structure, so that we don't need to write a separate module


	// $$ Start - Temporary only - to mount the internal partitions
	if ( !err )		// Not to create internal partition if there's error
	{
		int ret = activate_internal_nand();		// Go ahead whether successful or not.
		if ( ret )
			printk( "%s() - %d: Fatal error during MTD initailzation.\n", __PRETTY_FUNCTION__, __LINE__ );
	}
	//$$ End - Temporary only - to mount the internal partitions

	return err;
}

module_init(innotab_all_nand_init);

/*
 * Clean up routine
 */
#ifdef MODULE
static void __exit innotab_all_nand_cleanup (void)
{
	int i;

	// Removal of device file
	vmtd_file_exit();

	down_interruptible( &innotab_nand_in_use );
	for ( i = 0; i < INNOTAB_NUM_OF_CHIP; i ++ )
	{
		if ( innotab_mtd[i] != 0 )
		{
			struct nand_chip *temp = innotab_mtd[i]->priv;
			struct innotab_nand_int *temp_int = temp != 0 ? temp->priv : 0;
			struct innotab_nand_chip *temp_innotab = temp_int != 0 ? temp_int->priv : 0;

			innotab_nand_chip_del_mtd( temp_innotab->phyChipNo, 0 );
			innotab_nand_chip_release( temp_innotab->phyChipNo, 0 );
		}
	}
	up ( &innotab_nand_in_use );

	nand_base_exit();
}
module_exit(innotab_all_nand_cleanup);
#endif

//$$ Start - Temporary only - to mount the internal partitions
#define PART_SIZE_0_0		(45*1024*1024)
#define PART_SIZE_0_1		(30*1024*1024)
#define PART_SIZE_0_2		(30*1024*1024)
#define PART_SIZE_0_3		(16*1024*1024)

#define INNOTAB_NUM_OF_INT_PART			7
struct innotab_nand_chip_config gIntNandChip = {
	  .numOfPart = INNOTAB_NUM_OF_INT_PART,
	  .part = { { .name		= "Inno_I0_Reserved",		// For kernel.bin and boot loader, etc.
				.mask_flags = 0,
				.ecclayout = 0,
	  			.offset		= 0,
				.size		= PART_SIZE_0_0	  },

				{ .name		= "Inno_I_SysBin0",
				.mask_flags = 0,
				.ecclayout = 0,
	  			.offset		= PART_SIZE_0_0,
				.size		= PART_SIZE_0_1	  },

				{ .name		= "Inno_I_SysBin1",
				.mask_flags = 0,
				.ecclayout = 0,
	  			.offset		= PART_SIZE_0_0 + PART_SIZE_0_1,
				.size		= PART_SIZE_0_2	  },

				{ .name		= "Inno_I_Storage0",
				.mask_flags = 0,
				.ecclayout = 0,
	  			.offset		= PART_SIZE_0_0 + PART_SIZE_0_1 + PART_SIZE_0_2,
				.size		= PART_SIZE_0_3	  },

				{ .name		= "Inno_I_Bundle0",		// For game safe, etc.
				.mask_flags = 0,
				.ecclayout = 0,
	  			.offset		= PART_SIZE_0_0 + PART_SIZE_0_1 + PART_SIZE_0_2 + PART_SIZE_0_3,
				.size		= 0	  },

				{ .name		= "Inno_I_IF_DLG_2011",		// For game safe, etc.
				.mask_flags = 0,
				.ecclayout = 0,
	  			.offset		= 0,
				.size		= 0	  },
				
				{ .name		= "Inno_I_DEBUG_2011",		// For game safe, etc.
				.mask_flags = 0,
				.ecclayout = 0,
	  			.offset		= 0,
				.size		= 0	  },
	  },
	  .chipName = "Internal NAND"
};

int activate_internal_nand( void )
{
	int ret = 0;
	
	ret = innotab_nand_chip_init( 0, 1 );
	if ( !ret )
	{
		ret = innotab_nand_chip_add_mtd( 0, &gIntNandChip, MTD_ALLOW_ERASE_BB, 1 );
		if ( ret )
			innotab_nand_chip_release( 0, 1 );
	}

	return ret;
}
//$$ End - Temporary only - to mount the internal partitions
unsigned short crcFast(const unsigned short *message, unsigned long nWords);

static int get_last_good_block( int end, struct innotab_nand_chip *this_innotab )
{
	int page, page_shift, erase_shift;
	
	page_shift = ffs( this_innotab->param.oobblock ) - 1; 
	erase_shift = ffs( this_innotab->param.erasesize ) - 1; 
	for ( page = ( ( end >> erase_shift ) - 1 ) << ( erase_shift - page_shift ); 
	      page >= 0; page -= (this_innotab->param.erasesize>>page_shift) )
	{
		if ( !innotab_nand_check_bd_oob( this_innotab->phyChipNo, page )  )
		{
			break;
		}
	}
	return page >> ( erase_shift - page_shift ); 
}

#define IF_DLG_2011_HD_SIZE		512
static const char *if_dlg_2011_hd = "InnoTab IF DLG 2011 OK MARKER";
static void make_if_dlg_2011_hd( unsigned char *buf )
{
	unsigned int crc;
	
	memset( buf, 0, IF_DLG_2011_HD_SIZE );
	strcpy( buf, if_dlg_2011_hd );
	crc = crcFast( (const unsigned short *) buf, ( IF_DLG_2011_HD_SIZE - 2 ) / 2 ) ;
	buf[ IF_DLG_2011_HD_SIZE - 2 ] = (crc & 0xff);
	buf[ IF_DLG_2011_HD_SIZE - 1 ] = ((crc>>8)&0xff);
	/*printk( "%s-%s()-%d: crc[%x], upper[%x], lower[%x]\n", 
			__FILE__, __FUNCTION__, __LINE__, crc, (unsigned int)buf[ IF_DLG_2011_HD_SIZE - 1 ], (unsigned int)buf[ IF_DLG_2011_HD_SIZE - 2 ] );*/ //@@
}

static int check_if_dlg_2011_hd( unsigned char *buf )
{
	unsigned int crc;
	
	if ( strncmp( buf, if_dlg_2011_hd, strlen(if_dlg_2011_hd) ) )
	{
		return -1;
	}
	crc = crcFast( (const unsigned short *) buf, ( IF_DLG_2011_HD_SIZE - 2 ) / 2 ) ;
	if ( buf[ IF_DLG_2011_HD_SIZE - 2 ] != (crc & 0xff) || 
		 buf[ IF_DLG_2011_HD_SIZE - 1 ] != ((crc>>8)&0xff) )
	{
		INNOTAB_REACH_HERE
		return -1;
	}
	return 0;
}
 
int innotab_nand_chip_check_if_dlg_2011_ready( int phyChipNo, int lock )
{
	int chipIdx;
	int err = 0, rw_len;
	int page, prev_page;
	struct innotab_nand_chip *this_innotab;
	unsigned char *page_buf = NULL;

	if ( lock )
	{
		down_interruptible( &innotab_nand_in_use );
	}

	chipIdx = innotab_findchip( phyChipNo );
	if ( chipIdx == -1 )
	{
		err = -1;
		// printk( "%s() - %d: InnoTab NAND MTD device not found: %d\n", __PRETTY_FUNCTION__, __LINE__, phyChipNo ); //@@
		goto out;
	}
	this_innotab = (struct innotab_nand_chip *)(((struct innotab_nand_int *)(((struct nand_chip *)(innotab_mtd[chipIdx]->priv))->priv))->priv);
	if ( this_innotab->in_mtd == 0 )
	{
		err = -1;
		//INNOTAB_REACH_HERE	//@@@@
		goto out;
	}
	//INNOTAB_REACH_HERE //@


#ifdef CHECK_FOR_PROG_BUG
	if ( this_innotab->param.oobblock < IF_DLG_2011_HD_SIZE )
	{
		INNOTAB_HANG
	}
#endif
	page_buf = kmalloc(this_innotab->param.oobblock, GFP_DMA);
	if ( page_buf == NULL )
	{
		err = -1;
		printk( "%s() - %d: kmalloc() failure. Chip: %d\n", __PRETTY_FUNCTION__, __LINE__, phyChipNo );
		goto out;
	}
	page = get_last_good_block( this_innotab->param.chipsize, this_innotab ) << (ffs(this_innotab->param.erasesize) - ffs(this_innotab->param.oobblock));
	prev_page = get_last_good_block( page << (ffs(this_innotab->param.oobblock)-1), this_innotab ) << (ffs(this_innotab->param.erasesize) - ffs(this_innotab->param.oobblock));
	//INNOTAB_REACH_HERE //@
	/*printk( "%s() - %d: page[%d], prev_page[%d], page << (ffs(this_innotab->param.oobblock)-1)[%d], prev_page << (ffs(this_innotab->param.oobblock)-1))[%d],"
			"this_innotab->part[this_innotab->numOfPart - 1].offset[%lld]\n", __PRETTY_FUNCTION__, __LINE__, page, prev_page, page << (ffs(this_innotab->param.oobblock)-1),
			prev_page << (ffs(this_innotab->param.oobblock)-1), this_innotab->part[this_innotab->numOfPart - 1].offset ); */ //@@
	if ( ( prev_page << (ffs(this_innotab->param.oobblock)-1) ) < this_innotab->part[this_innotab->numOfPart - 1].offset || 
		 innotab_nand_read_ecc( phyChipNo, page << (ffs(this_innotab->param.oobblock)-1), this_innotab->param.oobblock, &rw_len, page_buf, NULL ) ||
		 rw_len != this_innotab->param.oobblock ||
		 check_if_dlg_2011_hd(page_buf) )
	{
		err = -1;
		//INNOTAB_REACH_HERE  //@
		printk( "%s() - %d: InnoTab NAND MTD read if_dlg_2011_mark failure. Chip: %d, Page: %d\n", 
				__PRETTY_FUNCTION__, __LINE__, this_innotab->phyChipNo, page );
		goto out; 
	}

out:
	if ( page_buf != NULL )
	{
		kfree( page_buf );
		page_buf = NULL;
	}
	if ( lock )
	{
		up ( &innotab_nand_in_use );
	}
	return err;
}

int innotab_nand_chip_mark_if_dlg_2011_ready( int phyChipNo, int lock )
{
	int chipIdx;
	int err = 0, rw_len;
	int page, prev_page;
	struct innotab_nand_chip *this_innotab;
	unsigned char *page_buf = NULL;
	unsigned char *temp_buf = NULL;

	if ( lock )
	{
		down_interruptible( &innotab_nand_in_use );
	}

	chipIdx = innotab_findchip( phyChipNo );
	if ( chipIdx == -1 )
	{
		err = -1;
		// printk( "%s() - %d: InnoTab NAND MTD device not found: %d\n", __PRETTY_FUNCTION__, __LINE__, phyChipNo ); //@@@
		goto mark_if_dlg_out;
	}
	this_innotab = (struct innotab_nand_chip *)(((struct innotab_nand_int *)(((struct nand_chip *)(innotab_mtd[chipIdx]->priv))->priv))->priv);
	if ( this_innotab->in_mtd == 0 )
	{
		err = -1;
		//INNOTAB_REACH_HERE		//@@
		goto mark_if_dlg_out;
	}
	//INNOTAB_REACH_HERE	//@

	page_buf = kzalloc(this_innotab->param.oobblock, GFP_DMA);
	temp_buf = kzalloc(this_innotab->param.oobblock, GFP_DMA);
	if ( page_buf == NULL || temp_buf == NULL )
	{
		err = -1;
		printk( "%s() - %d: kmalloc() failure. Chip: %d\n", __PRETTY_FUNCTION__, __LINE__, phyChipNo );
		goto mark_if_dlg_out;
	}
	page = get_last_good_block( this_innotab->param.chipsize, this_innotab ) << (ffs(this_innotab->param.erasesize) - ffs(this_innotab->param.oobblock));
	prev_page = get_last_good_block( page << (ffs(this_innotab->param.oobblock)-1), this_innotab ) << (ffs(this_innotab->param.erasesize) - ffs(this_innotab->param.oobblock));

	// Write the header if it does not already exist	
	memset( page_buf, 0, this_innotab->param.oobblock );
	make_if_dlg_2011_hd( page_buf );	
	memset( temp_buf, 0, this_innotab->param.oobblock );
	make_if_dlg_2011_hd( temp_buf );
	if ( ( prev_page << (ffs(this_innotab->param.oobblock)-1) ) < this_innotab->part[this_innotab->numOfPart - 1].offset ||
		 innotab_nand_erase_block( phyChipNo, page ) ||
		 innotab_nand_write_page( phyChipNo, page, page_buf, NULL ) ||
		 innotab_nand_read_ecc( phyChipNo, page << (ffs(this_innotab->param.oobblock)-1), this_innotab->param.oobblock, &rw_len, page_buf, NULL ) || 
		 rw_len != this_innotab->param.oobblock ||
		 memcmp( temp_buf, page_buf, this_innotab->param.oobblock ) )
	{
		err = -1;
		INNOTAB_REACH_HERE		// @ By force emulation of error by adding "page > certain number" in the condition above
		goto erase_and_out;
	}
mark_if_dlg_out:
	if ( page_buf != NULL )
	{
		kfree( page_buf );
		page_buf = NULL;
	}
	if ( temp_buf != NULL )
	{
		kfree( temp_buf );
		temp_buf = NULL;
	}
	if ( lock )
	{
		up ( &innotab_nand_in_use );
	}
	return err;
	
// Put it here so that normally flow will not come cross it....
erase_and_out:
	if ( ( page << (ffs(this_innotab->param.oobblock)-1) ) >= this_innotab->part[this_innotab->numOfPart - 1].offset && 
		 !innotab_nand_check_bd_oob( phyChipNo, page ) )
	{
		innotab_nand_erase_block( phyChipNo, page );
		innotab_nand_mark_bd_oob( phyChipNo, page );
		INNOTAB_REACH_HERE			// @@@
	}
	goto mark_if_dlg_out; 	
}

#define IF_DLG_2011_MARGIN							(55*1024*1024)
#define MTDBLOCK_CRC_INIT							0xFFFFFFFFU
#define MTD_BBT_BLOCK_SIZE							512
#define MTD_SIG_LEN									16
#define MTD_SKIP_END_MARKER_OFF						(MTD_SIG_LEN)
#define MTD_HAVE_BBT_OFF							(MTD_SKIP_END_MARKER_OFF+1)
#define MTD_LAST_PARA_OFF							(MTD_HAVE_BBT_OFF)					// Currently the last item is the "have BBT" attribute
#define BBT_IN_SINLE_EB								1
static char gInnoTabBBTSig[MTD_SIG_LEN]	= { 'I', 'n', 'n', 'o', 'T', 'a', 'b', ' ', 'B', 'B', 'T', 0, 0, 0, 0, 0 };			// Signature for the BBT (bad block table)
int add_if_dlg_2011_partition( struct mtd_info *this_mtd )
{
	int err = 0, end_page_bundle, page, i, page_per_block_shift, page_per_block, page_size_shift, rw_len, crc_from_flash;
	unsigned char *page_buf = NULL, *page_buf_ver = NULL;
	struct nand_chip *this = NULL;
	struct innotab_nand_int *this_innotab_int = NULL;
	struct innotab_nand_chip *this_innotab = NULL;

	this = (struct nand_chip *)(this_mtd->priv);
	this_innotab_int = (struct innotab_nand_int *)(this->priv);
	this_innotab = (struct innotab_nand_chip *)(this_innotab_int->priv);
	
	page_per_block = this_innotab->param.erasesize / this_innotab->param.oobblock; 
	page_per_block_shift = ffs(this_innotab->param.erasesize) - ffs(this_innotab->param.oobblock);
	page_size_shift = ffs(this_innotab->param.oobblock) - 1;

#ifdef CHECK_FOR_PROG_BUG
	if ( this_innotab->param.oobblock < IF_DLG_2011_HD_SIZE || 
		 this_innotab->param.oobblock < MTD_BBT_BLOCK_SIZE )
	{
		INNOTAB_HANG
	}
#endif
	page_buf = kzalloc(this_innotab->param.oobblock, GFP_DMA);
	page_buf_ver = kzalloc(this_innotab->param.oobblock, GFP_DMA); 
	if ( page_buf == NULL || page_buf_ver == NULL )
	{
		err = -1;
		printk( "%s() - %d: kmalloc() failure. Chip: %d\n", __PRETTY_FUNCTION__, __LINE__, this_innotab->phyChipNo );
		goto prepare_new_partition;
	}

//chk_if_dlg_2011_marker:
	page = get_last_good_block( this_innotab->param.chipsize, this_innotab ) << page_per_block_shift;
	if ( innotab_nand_read_ecc( this_innotab->phyChipNo, page << page_size_shift, this_innotab->param.oobblock, &rw_len, 
			page_buf, NULL ) || rw_len != this_innotab->param.oobblock )
	{
		err = -1;
		INNOTAB_REACH_HERE
		printk( "%s() - %d: InnoTab NAND MTD read if_dlg_2011_mark failure. Chip: %d, Page: %d\n", 
				__PRETTY_FUNCTION__, __LINE__, this_innotab->phyChipNo, page );
		goto prepare_new_partition;					
	}
	if ( !check_if_dlg_2011_hd(page_buf) )
	{
		INNOTAB_REACH_HERE
		goto out;
	}

//chk_bundle_end_mark:
	if ( innotab_nand_read_ecc( this_innotab->phyChipNo, ( page + page_per_block - 1 ) << page_size_shift, 
			this_innotab->param.oobblock, &rw_len, page_buf, NULL ) && rw_len != this_innotab->param.oobblock )
	{
		err = -1;
		printk( "%s() - %d: InnoTab NAND MTD read btt failure. Chip: %d, Page: %d\n", 
				__PRETTY_FUNCTION__, __LINE__, this_innotab->phyChipNo, page );
		goto prepare_new_partition;
	}
	crc_from_flash = *((int *)(&page_buf[ this_innotab->param.oobblock - 4 ]));
	if ( memcmp( gInnoTabBBTSig, page_buf + this_innotab->param.oobblock - MTD_BBT_BLOCK_SIZE, MTD_SIG_LEN ) ||
	     crc_from_flash != crc32( MTDBLOCK_CRC_INIT, page_buf + this_innotab->param.oobblock - MTD_BBT_BLOCK_SIZE, MTD_BBT_BLOCK_SIZE - 4 ) )
	{
		INNOTAB_REACH_HERE				// @ run in factory resetter environment
		goto prepare_new_partition;
	}

//copy_bundle_end_mark:
	page = get_last_good_block( this_innotab->param.chipsize, this_innotab ) << page_per_block_shift;
	end_page_bundle = get_last_good_block( this_innotab->param.chipsize - IF_DLG_2011_MARGIN, this_innotab ) << page_per_block_shift;
	if ( innotab_nand_erase_block( this_innotab->phyChipNo, end_page_bundle ) && 
		 innotab_nand_erase_block( this_innotab->phyChipNo, end_page_bundle ) )
	{	// Lazy way to secure the erasure
		err = -1;
		INNOTAB_REACH_HERE
		goto copy_out;
	}
	for ( i = 0; i < page_per_block; i ++ )
	{
		if ( innotab_nand_read_ecc( this_innotab->phyChipNo, ( page + i ) << page_size_shift, this_innotab->param.oobblock, &rw_len, 
			 page_buf, NULL ) || rw_len != this_innotab->param.oobblock )
		{
			err = -1;
			INNOTAB_REACH_HERE
			goto copy_out;
		}
		// FIXME. Very ad-hoc To be adjust later
		if ( i == page_per_block - 1 )
		{
			//INNOTAB_REACH_HERE	//@@ run in ugd environment
			if ( page_buf[this_innotab->param.oobblock - MTD_BBT_BLOCK_SIZE + MTD_HAVE_BBT_OFF] == BBT_IN_SINLE_EB )
			{
				int new_crc;
				//INNOTAB_REACH_HERE //@@ run in ugd environment
				page_buf[this_innotab->param.oobblock - MTD_BBT_BLOCK_SIZE + MTD_HAVE_BBT_OFF] = 0;
				new_crc = crc32( MTDBLOCK_CRC_INIT, page_buf + this_innotab->param.oobblock - MTD_BBT_BLOCK_SIZE, 
								 MTD_BBT_BLOCK_SIZE - 4 );
				*((int *)(&page_buf[ this_innotab->param.oobblock - 4 ])) = new_crc;
			}
		}		
		if ( innotab_nand_write_page( this_innotab->phyChipNo, end_page_bundle + i, page_buf, NULL ) || 
			 innotab_nand_read_ecc( this_innotab->phyChipNo, ( end_page_bundle + i ) << page_size_shift, this_innotab->param.oobblock, &rw_len, page_buf_ver, NULL ) || 
			 rw_len != this_innotab->param.oobblock ||
			 memcmp( page_buf, page_buf_ver, this_innotab->param.oobblock ) )
		{
			err = -1;
			INNOTAB_REACH_HERE
			goto copy_out;
		}
	}
copy_out:
	if ( err )
	{
		INNOTAB_REACH_HERE
		innotab_nand_erase_block( this_innotab->phyChipNo, end_page_bundle );
		innotab_nand_erase_block( this_innotab->phyChipNo, end_page_bundle );
		if ( innotab_nand_mark_bd_oob( this_innotab->phyChipNo, end_page_bundle ) )
		{
			INNOTAB_REACH_HERE
			innotab_nand_erase_block( this_innotab->phyChipNo, end_page_bundle );
			innotab_nand_erase_block( this_innotab->phyChipNo, end_page_bundle );
			innotab_nand_mark_bd_oob( this_innotab->phyChipNo, end_page_bundle );
		}
		goto prepare_new_partition;
	}
	//INNOTAB_REACH_HERE	//@@
	
prepare_new_partition:
	//INNOTAB_REACH_HERE	//@@
	for ( i = this_innotab->param.chipsize - IF_DLG_2011_MARGIN; i < this_innotab->param.chipsize; i += this_innotab->param.erasesize )
	{
		page = i >> page_size_shift;
		// Lazy wait of retry
		if ( !innotab_nand_check_bd_oob( this_innotab->phyChipNo, page ) ) // FIXME. Bug just fixed?
		{
			int ret1;//, ret2;

			//@  INNOTAB_REACH_HERE
			//@  printk( "%s()-%d: Erase page[%d], block[%d]\n", __FUNCTION__, __LINE__, page, page >> page_per_block_shift );
			ret1 = innotab_nand_erase_block( this_innotab->phyChipNo, page );
			// ret2 = innotab_nand_erase_block( this_innotab->phyChipNo, page );
			if ( ret1 )// || ret2 )
			{
				INNOTAB_REACH_HERE
				if ( innotab_nand_mark_bd_oob( this_innotab->phyChipNo, page ) )
				{
					INNOTAB_REACH_HERE
					innotab_nand_erase_block( this_innotab->phyChipNo, page );
					innotab_nand_erase_block( this_innotab->phyChipNo, page );
					innotab_nand_mark_bd_oob( this_innotab->phyChipNo, page );
				}
				err = -1;
			}
		}
	}
	
out:
	this_innotab->part[this_innotab->numOfPart-3].size = this_innotab->param.chipsize - this_innotab->part[this_innotab->numOfPart-3].offset - IF_DLG_2011_MARGIN;

	page = get_last_good_block( this_innotab->param.chipsize, this_innotab ) << page_per_block_shift; 
	page = get_last_good_block( page << page_size_shift, this_innotab ) << page_per_block_shift;

	this_innotab->part[this_innotab->numOfPart-2].offset = this_innotab->part[this_innotab->numOfPart-3].offset + this_innotab->part[this_innotab->numOfPart-3].size; 
	this_innotab->part[this_innotab->numOfPart-2].size = ( page << page_size_shift ) - this_innotab->part[this_innotab->numOfPart-2].offset;
	
	this_innotab->part[this_innotab->numOfPart-1].offset = this_innotab->part[this_innotab->numOfPart-2].offset + this_innotab->part[this_innotab->numOfPart-2].size;
	this_innotab->part[this_innotab->numOfPart-1].size = this_innotab->param.chipsize - this_innotab->part[this_innotab->numOfPart-1].offset;

	for ( i = 0; i < this_innotab->numOfPart; i ++ )
	{
		printk( "%s()-%d: Partition[%d]: %lld, %lld\n", __FUNCTION__, __LINE__, i, this_innotab->part[i].offset, this_innotab->part[i].size );
	}
	
	if ( page_buf != NULL )
	{
		kfree( page_buf );
		page_buf = NULL;
	}
	if ( page_buf_ver != NULL )
	{
		kfree( page_buf_ver );
		page_buf_ver = NULL;
	}
	
	return err;	
}

/**********************************************************************
 *
 * Filename:    crc.c
 * 
 * Description: Slow and fast implementations of the CRC standards.
 *
 * Notes:       The parameters for each supported CRC standard are
 *				defined in the header file crc.h.  The implementations
 *				here should stand up to further additions to that list.
 *
 * 
 * Copyright (c) 2000 by Michael Barr.  This software is placed into
 * the public domain and may be used for any purpose.  However, this
 * notice must not be changed or removed and no warranty is either
 * expressed or implied by its publication or distribution.
 **********************************************************************/
/* Modified by Vic Siu on 2010-03-27: this is CRC16-CCITT*/

const unsigned short crcTable[256] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7, 
	0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef, 
	0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6, 
	0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de, 
	0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485, 
	0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d, 
	0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4, 
	0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc, 
	0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823, 
	0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b, 
	0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12, 
	0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a, 
	0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41, 
	0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49, 
	0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70, 
	0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78, 
	0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f, 
	0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067, 
	0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e, 
	0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256, 
	0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d, 
	0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 
	0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c, 
	0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634, 
	0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab, 
	0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3, 
	0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a, 
	0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92, 
	0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 
	0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1, 
	0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8, 
	0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0, 
};


/*********************************************************************
 *
 * Function:    crcFast()
 * 
 * Description: Compute the CRC of a given message.
 *
 * Notes:		crcInit() must be called first.
 *
 * Returns:		The CRC of the message.
 *
 *********************************************************************/
unsigned short crcFast2(const unsigned short *message, unsigned long nWords, unsigned short rem)
{
	unsigned short remainder = rem;
	unsigned short data;
	unsigned short byte;

	/*
	 * Divide the message by the polynomial, a byte at a time.
	 */
	while (nWords-- > 0) {
		data = *message++;
		byte = (data & 0x00ff) ^ (remainder >> 8);
		remainder = crcTable[byte] ^ (remainder << 8);

		byte = (data >> 8) ^ (remainder >> 8);
		remainder = crcTable[byte] ^ (remainder << 8);
	}

	/*
	 * The final remainder is the CRC.
	 */
	return remainder;
}

unsigned short crcFast(const unsigned short *message, unsigned long nWords)
{
	return crcFast2(message, nWords, 0xffff);
}   /* crcFast() */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("VTech");
MODULE_DESCRIPTION("I am alien!!!!!!!");
