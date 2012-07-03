#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/highmem.h>
#include <linux/blkdev.h>
#include <linux/string.h>
#include <linux/autoconf.h>

#include <linux/mtd/nand.h> //Added on 2011-01-25
#include <asm/errno.h> //Added on 2011-01-25
#include "gp_nand_hal/hal_nand.h"
#include "gp_nand_hal/hal_nand_smart_id.h"
#include "gp_nand_hal/hal_nand_bch.h"
#include "gp_nand_hal/hal_nand_bthdr.h"
#include "gp_nand_hal/hal_nand_bthdr_ext.h"

#include "nand_innotab.h"

//#define USE_BOOT_HEADER

#define INNOTAB_DEBUG	1

#if (INNOTAB_DEBUG > 0)
	#define ERR_PRINT(fmt,arg...)	printk("[INNOTAB_NAND] FUNC(%s) LINE(%d) "fmt, __FUNCTION__, __LINE__ ,##arg)
	#if (INNOTAB_DEBUG > 1)
		#define DB_PRINT(fmt,arg...)	printk("[INNOTAB_NAND] FUNC(%s) LINE(%d) "fmt, __FUNCTION__, __LINE__ ,##arg)
	#else
	 	#define DB_PRINT(fmt,arg...)	
	#endif
#else
 	#define DB_PRINT(fmt,arg...)
 	#define ERR_PRINT(fmt,arg...)
#endif


#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define	NAND_USER_BAD_TAG	0x44
#define NAND_GOOD_TAG		0xff

#define MTD_START_OFFSET	(32*1024*1024) //32 MByte

DECLARE_MUTEX(innotab_nand_sem);
EXPORT_SYMBOL(innotab_nand_sem);

static SINT32 g_nand_init_flags[2] = {FALSE, FALSE};
static struct innotab_nand_chip_param g_innotab_nand_info[2] = {{0},{0}};
static UINT8 *gp_innotab_nand_buffer = NULL;
extern Physical_NandInfo* gPhysical_NandInfo;
extern HEADER_NF_INFO  nand_info;

SINT32 innotab_nand_read_oob_lock( SINT32 dev_no, SINT32 page, SINT32 off, size_t len, size_t * retlen, u_char *buf, SINT32 need_to_lock);
SINT32 innotab_nand_write_oob_lock( SINT32 dev_no, SINT32 page, SINT32 offset, size_t len, size_t * retlen, const u_char * oobbuf, SINT32 need_to_lock);

static inline SINT32 nand_access_ok(SINT32 dev_no, SINT32 page_no)
{
	//check data area start page
	SINT32 block_no = page_no / nand_block_size_get();

//	return ((block_no >= g_innotab_nand_info[dev_no].firstMTDBlk) && (block_no < gPhysical_NandInfo->wNandBlockNum));
	return (block_no >= g_innotab_nand_info[dev_no].firstMTDBlk);
}

static SINT32 nand_get_start_block(SINT32 dev_no)
{
	SINT32 app_start = 0, app_size = 0, app_spare_start = 0, app_spare_percent = 0;
	SINT32 app_spare_size = 0, data_start = 0, block_size_in_byte = 0;
	
	if(dev_no == 0) //internal nand
	{
#ifdef USE_BOOT_HEADER
		app_start = GetAppStartBlkFromBth();
		app_size = GetAppSizeOfBlkFromBth();
		app_spare_start = app_start + app_size;
		app_spare_percent = GetAppPercentFromBth();
		app_spare_size = app_size/app_spare_percent;
		data_start = app_spare_start + app_spare_size;
		return data_start;
#else

		block_size_in_byte = nand_block_size_get() * nand_page_size_get();
		data_start = (MTD_START_OFFSET + block_size_in_byte - 1)/ block_size_in_byte;

		printk( "%s() - %d: nand_block_size_get() = %d, nand_page_size_get() = %d\n", __PRETTY_FUNCTION__, __LINE__, nand_block_size_get(), nand_page_size_get() );
		return 0;
		return data_start;
#endif
	}
	else //external nand card
	{
		return 0;
	}
}

/*
 * Initialization routine.  Carries out chip detection and returns NAND chip parameters.
 * 
 * @dev_no:	device number, currently always 0.
 * @param:	structure containing device parameters
 *
 * return:	0		chip detected successfully, parameters in @param.
 *			-1		chip cannot be detected or unrecognized type of chip
 */
void Temp_Nand_Set_EraseCycle( int dev_no, int cycle );

SINT32 innotab_nand_init( SINT32 dev_no, struct innotab_nand_chip_param *param )
{
	UINT8 * nand_header_buf = NULL;
	UINT32 nand_mainid = 0;
	
	DB_PRINT("\n");	

	if(!param)
	{
		ERR_PRINT("Got NULL pointer!\n");
		return (-1);
	}

	if(dev_no > 1 || dev_no < 0)
	{
		ERR_PRINT("Device number out of range [0,1]\n");
		return (-1);
	}

	if(down_interruptible(&innotab_nand_sem))//Add semaphore, modified by Kevin
	{
		ERR_PRINT("Semaphore error report.\n" );
 		return (-1);
	}

	Nand_Chip_Switch(dev_no);

//	if (g_nand_init_flags[dev_no] == TRUE)
//	{
//		memcpy(param, &g_innotab_nand_info[dev_no], sizeof(struct innotab_nand_chip_param));
//		goto succ_exit;
//	}

//	DrvNand_WP_Initial();

	if (DrvL1_Nand_Init(NAND_NON_SHARE) != STATUS_OK)
	{
		ERR_PRINT("Nand init fail!\n");
		up(&innotab_nand_sem);
		return (-1);
	}

#ifdef USE_BOOT_HEADER
	if(dev_no == 1) //external nand card, get nand info with smart id
	{
		nand_smart_id_init(Nand_MainId_Get(), Nand_VendorId_Get());
	}
	else //internal nand, get nand info with header
	{
		nand_header_buf = (UINT8 *)kmalloc(4*1024, GFP_DMA);
		if(!nand_header_buf)
		{
			ERR_PRINT("Nand init fail!\n");
			up(&innotab_nand_sem);
			return (-1);
		}
		if(NandParseBootHeader(nand_header_buf))
		{
			ERR_PRINT("Nand init fail!\n");
			kfree(nand_header_buf);
			nand_header_buf = NULL;
			up(&innotab_nand_sem);
			return (-1);
		}
		//the nand info should be filled in "extern Physical_NandInfo* gPhysical_NandInfo" and "extern HEADER_NF_INFO  nand_info" now;
		kfree(nand_header_buf);
		nand_header_buf = NULL;
	}
#else
	nand_smart_id_init(Nand_MainId_Get(), Nand_VendorId_Get());
	{
		int erase_cycle = 0;
		
		if (Nand_MainId_Get()==0xD198)
		{
			erase_cycle = 3;
		}
		else if (Nand_MainId_Get()==0xDA98)
		{
			erase_cycle = 3;
		}
		printk( "%s() - %d: dev_no[%d], Id[%d], erase_cycle[%d]\n", __PRETTY_FUNCTION__, __LINE__, dev_no, Nand_MainId_Get(), erase_cycle );
		Temp_Nand_Set_EraseCycle( dev_no, erase_cycle );
	}
#endif

	nand_mainid = Nand_MainId_Get();
	
	g_innotab_nand_info[dev_no].firstMTDBlk = nand_get_start_block(dev_no);
	g_innotab_nand_info[dev_no].nand_maf_id = nand_mainid&0xFF;
	g_innotab_nand_info[dev_no].nand_dev_id = (nand_mainid>>8)&0xFF;
	g_innotab_nand_info[dev_no].numchips = 1;
	g_innotab_nand_info[dev_no].chipsize = gPhysical_NandInfo->wNandBlockNum * gPhysical_NandInfo->wBlkPageNum * gPhysical_NandInfo->wPageSize;
	g_innotab_nand_info[dev_no].erasesize = gPhysical_NandInfo->wBlkPageNum * gPhysical_NandInfo->wPageSize;
	g_innotab_nand_info[dev_no].oobblock = gPhysical_NandInfo->wPageSize;
	g_innotab_nand_info[dev_no].oobsize = GEPLUS_OOB_SIZE;
	g_innotab_nand_info[dev_no].oobavail = INNOTAB_OOB_AVAIL;		/* Amount of OOB available to application (i.e. excluding the ECC) */
	g_innotab_nand_info[dev_no].oobavailstart = 2;	/* Starting of the area within OOB for user data */


	if((nand_mainid&0xFF) == 0xC2)
	{
		g_innotab_nand_info[dev_no].memoryType = NAND_ROM;
	}
	else if((nand_mainid&0xFF) == 0x45)
	{
		if((nand_mainid&0xFF) == 0x04 || (nand_mainid&0xFF) == 0x05)
		{
			g_innotab_nand_info[dev_no].memoryType = NAND_ROM;
		}
	}
	else
	{
		g_innotab_nand_info[dev_no].memoryType = NAND_FLASH;
	}

	memcpy(param, &g_innotab_nand_info[dev_no], sizeof(struct innotab_nand_chip_param));
	g_nand_init_flags[dev_no] = TRUE;

	if(!gp_innotab_nand_buffer)
	{
		gp_innotab_nand_buffer =(UINT8 *)kmalloc(4*1024, GFP_DMA);//4K buffer size
		if(!gp_innotab_nand_buffer)
		{
			ERR_PRINT("Alloc memory for nand buffer failed!\n");
			up(&innotab_nand_sem);
			return (-1);
		}
	}

succ_exit:
	up(&innotab_nand_sem);

	ERR_PRINT("g_innotab_nand_info[%d].nand_maf_id: 0x%02X\n", dev_no, g_innotab_nand_info[dev_no].nand_maf_id);
	ERR_PRINT("g_innotab_nand_info[%d].nand_dev_id: 0x%02X\n", dev_no, g_innotab_nand_info[dev_no].nand_dev_id);
	ERR_PRINT("g_innotab_nand_info[%d].numchips: %d\n", dev_no, g_innotab_nand_info[dev_no].numchips);
	ERR_PRINT("g_innotab_nand_info[%d].chipsize: %ld\n", dev_no, g_innotab_nand_info[dev_no].chipsize);
	ERR_PRINT("g_innotab_nand_info[%d].erasesize: %d\n", dev_no, g_innotab_nand_info[dev_no].erasesize);
	ERR_PRINT("g_innotab_nand_info[%d].oobblock: %d\n", dev_no, g_innotab_nand_info[dev_no].oobblock);
	ERR_PRINT("g_innotab_nand_info[%d].oobsize: %d\n", dev_no, g_innotab_nand_info[dev_no].oobsize);
	ERR_PRINT("g_innotab_nand_info[%d].firstMTDBlk: %d\n", dev_no, g_innotab_nand_info[dev_no].firstMTDBlk);
	ERR_PRINT("mainID: 0x%x\t venderID: 0x%x\n", Nand_MainId_Get(), Nand_VendorId_Get());

	return 0;
}


/* 
 * Read data from NAND at the given offset, relative the start of the device.
 * All data are supposed to be ecc corrected.
 *
 * @dev_no:	device number, currently always 0.
 * @start:	start address to read from, relative to start of the device (in byte)
 * @len:	number of bytes to read, excluding the OOB data
 * @retlen:	pointer to variable to store the number of read bytes, excluding OOB data
 * @buf:	the data buffer to put data
 * @oobbuf:	the data buffer for oob data, if non-NULL.  Otherwise throw away the OOB data.
 *
 * return:	0			success
 *			-EINVAL		end address (i.e. len + start - 1) beyond the end of device.
 *			-EBADMSG	incorrectable ecc error
*/
SINT32 innotab_nand_read_ecc( SINT32 dev_no, loff_t start, size_t len, size_t * retlen, u_char *buf, u_char *oobbuf )
{
	SINT32 read_len= 0, ret = 0;
	SINT32 cur_page = 0, last_page = -1, pos_in_page = 0, oobpos = 0;
	UINT32 spare_flag[2] = {0};

	if(down_interruptible(&innotab_nand_sem))//Add semaphore, modified by Kevin
	{
		ERR_PRINT("start[%lld], len[%d], retlen[%p], buf[%p], oobbuf[%p]\n", start, len, retlen, buf, oobbuf);
		ERR_PRINT("IO Error!\n");
		ERR_PRINT("Semaphore error report.\n" );
  		return -EINVAL;
	}

	Nand_Chip_Switch(dev_no);

#if 0
	if(!nand_access_ok(dev_no, ((SINT32)start)/g_innotab_nand_info[dev_no].oobblock))//g_innotab_nand_info[dev_no].oobblock <--> page size
	{
		ERR_PRINT("Access denied!! Access block should >= %d(current: %d)\n", 
				g_innotab_nand_info[dev_no].firstMTDBlk, ((SINT32)start)/g_innotab_nand_info[dev_no].erasesize );
				
		up(&innotab_nand_sem);
		return -EINVAL;
	}
#endif

	if(start >= g_innotab_nand_info[dev_no].chipsize)
	{
		ERR_PRINT("start[%lld], len[%d], retlen[%p], buf[%p], oobbuf[%p]\n", start, len, retlen, buf, oobbuf);
		up(&innotab_nand_sem);
		return -EINVAL;
	}
	
	while((read_len < len) && (start + read_len < g_innotab_nand_info[dev_no].chipsize))
	{
		pos_in_page = ((SINT32)start + read_len) % g_innotab_nand_info[dev_no].oobblock;
		cur_page = ((SINT32)start + read_len) / g_innotab_nand_info[dev_no].oobblock;//oobblock <-> page size

		ret = Nand_ReadPhyPage(cur_page, (UINT32)gp_innotab_nand_buffer);		// Wallace: Is this a bug?  If Nand_ReadPhyPage() return -1, this function may return 0 - for success...
		if(ret)
		{
			if(ret == 0x1)
			{
//				ERR_PRINT("start[%lld], len[%d], retlen[%p], buf[%p], oobbuf[%p]\n", start, len, retlen, buf, oobbuf);
//				ERR_PRINT("Read from dev:%d block:%d page%d failed! ECC error\n", dev_no, cur_block, cur_page);
				ERR_PRINT("ECC error report.\n" );
				up(&innotab_nand_sem);
				return -EBADMSG; 
			}
			ERR_PRINT("start[%lld], len[%d], retlen[%p], buf[%p], oobbuf[%p]\n", start, len, retlen, buf, oobbuf);
			ERR_PRINT("Read from dev:%d page%d failed!\n", dev_no, cur_page);
			break;
		}

		if((len - read_len) > (g_innotab_nand_info[dev_no].oobblock - pos_in_page))
		{
			memcpy(&buf[read_len], gp_innotab_nand_buffer + pos_in_page, g_innotab_nand_info[dev_no].oobblock - pos_in_page);
			read_len += g_innotab_nand_info[dev_no].oobblock - pos_in_page;
		}
		else
		{
			memcpy(&buf[read_len], gp_innotab_nand_buffer + pos_in_page, len - read_len);
			read_len += len - read_len;
		}
		*retlen = read_len;

		if(oobbuf && (last_page != cur_page))
		{
			spare_flag[0] = spare_flag_get_L();
			spare_flag[1] = spare_flag_get_H();
			oobbuf[oobpos++] = (spare_flag[0]>>16) & 0xFF; //skip first 2 bytes
			oobbuf[oobpos++] = (spare_flag[0]>>24) & 0xFF;
			oobbuf[oobpos++] = spare_flag[1] & 0xFF;
			oobbuf[oobpos++] = (spare_flag[1]>>8) & 0xFF;
			oobbuf[oobpos++] = (spare_flag[1]>>16) & 0xFF;
			oobbuf[oobpos++] = (spare_flag[1]>>24) & 0xFF;
			
			last_page = cur_page;
		}
	}

	up(&innotab_nand_sem);

	return 0;
}



/*
 * Read OOB only
 *
 * @dev_no:	device number, currently always 0.
 * @page:	Start to read from this page.  Page number counted from the beginning of the device.
 * @off:	Start to read from the "off"-th byte of the OOB area.
 * @len:	Number of bytes of OOB data to be read.
 * @retlen:	pointer to variable to store the number of read bytes
 * @buf:	the data buffer to put data
 *
 * return:	0			success
 *			-EINVAL		error, attempt to read beyond the boundary of the device
 */
SINT32 innotab_nand_read_oob_lock( SINT32 dev_no, SINT32 page, SINT32 off, size_t len, size_t * retlen, u_char *buf, SINT32 need_to_lock)
{
	ERR_PRINT("NOT IMPLEMENT!\n");
	return -EINVAL;
}

SINT32 innotab_nand_read_oob( SINT32 dev_no, SINT32 page, SINT32 off, size_t len, size_t * retlen, u_char *buf )
{
	return innotab_nand_read_oob_lock( dev_no, page, off, len, retlen, buf, 1);
}

/* 
 * Read raw data from NAND at the given offset, relative the starting of the device.
 *
 * @dev_no:	device number, currently always 0.
 * @page:	1st page to be read, relative to start of the device
 * @numpage:number of pages to be read
 * @buf:	buffer to hold the return data, with the following format:
 *			User data of page 0, OOB data of page 0, User data of page 1, OOB data of page 1, etc...
 *
 * return:	0			success
 *			-EINVAL		read beyond the boundary of the device
*/
SINT32 innotab_nand_read_raw_page( SINT32 dev_no, SINT32 page, SINT32 numpage, uint8_t *buf )
{
	ERR_PRINT("NOT IMPLEMENT!\n");
	return -EINVAL;
}




/* 
 * Write data to NAND at the given page, relative to the start of the device.
 *
 * ECC is to be generated internally.  Other data in OOB area is from the oob_buf.
 *
 * @dev_no:		device number, currently always 0.
 * @page:		page number of the location to write.  Start of the device has page number = 0.
 * @buf:		the data to write
 * @oob_buf:	out of band data buffer to write
 *
 * return:	0			success
 *			-EIO		write failure
*/
SINT32 innotab_nand_write_page( SINT32 dev_no, SINT32 page, u_char *buf, u_char *oob_buf )
{
	UINT32 spare_flag[2] = {0x0000FFFF, 0x00000000};

//	DB_PRINT("Normal - page[%d], buf[0x%X], oob_buf[0x%X], block_no[%d], page_no[%d]\n", page, buf, oob_buf, block_no, page_no);

	if(down_interruptible(&innotab_nand_sem))//Add semaphore, modified by Kevin
	{
		ERR_PRINT("IO Error!\n");
		ERR_PRINT("Semaphore error report.\n" );
  		return -EIO;
	}

	Nand_Chip_Switch(dev_no);

	if(!nand_access_ok(dev_no, page))
	{
		ERR_PRINT("Access denied!! Access block should >= %d(current: %d)\n", 
				g_innotab_nand_info[dev_no].firstMTDBlk, page/nand_block_size_get());
		up(&innotab_nand_sem);
		return -EIO;
	}

	if(oob_buf)
	{
		spare_flag[0] |= (oob_buf[0]<<16) | (oob_buf[1]<<24);
		spare_flag[1] |= oob_buf[2] | (oob_buf[3]<<8) | (oob_buf[4]<<16) | (oob_buf[5]<<24);
	}
	else
	{
		spare_flag[0] = 0xFFFFFFFF;
		spare_flag[1] = 0xFFFFFFFF;
	}
	spare_flag_set_L(spare_flag[0]);
	spare_flag_set_H(spare_flag[1]);

	if(Nand_WritePhyPage(page, (UINT32)buf))
	{
		ERR_PRINT("Write FAIL!! page[%d], buf[%p], oob_buf[%p]\n", page, buf, oob_buf);
		up(&innotab_nand_sem);
		return -EIO;
	}
	
	up(&innotab_nand_sem);

	return 0;
}

/*
 * Write OOB data to NAND flash at a particular offset, relative to the start of the device.
 *
 * @dev_no:	device number, currently always 0.
 * @page:	Page number of the location to write.  Start of the device has page number = 0.
 * @offset:	Offset within the OOB area of the specified page to start writing data.
 * @len:	Number of bytes to write
 * @retlen:	pointer to variable to store the number of written bytes
 * @oobbuf:	the data to write
 *
 * return:	0			success
 *			-EINVAL		error, attempt to write beyond the range of the current page
 *			-EIO		write failure
*/
SINT32 innotab_nand_write_oob_lock( SINT32 dev_no, SINT32 page, SINT32 offset, size_t len, size_t * retlen, const u_char * oobbuf, SINT32 need_to_lock)
{
	ERR_PRINT("NOT IMPLEMENT!\n");
	return -EINVAL;
}

SINT32 innotab_nand_write_oob( SINT32 dev_no, SINT32 page, SINT32 offset, size_t len, size_t * retlen, const u_char * oobbuf)
{
	return innotab_nand_write_oob_lock(dev_no, page, offset, len, retlen, oobbuf, 1);
}


/*
 * Erase a single page
 *
 * @dev_no:	device number, currently always 0.
 * @page:	page number of the 1st page of the block to be erased
 *
 * return:	0					success
 *			NAND_STATUS_FAIL	failed
*/
SINT32 innotab_nand_erase_block( SINT32 dev_no, SINT32 page )
{
//	DB_PRINT("page[%d], block_no[%d]\n", page, block_no);

	SINT32 block_no;
	unsigned short bd_block_flag = 0;
	
	if(down_interruptible(&innotab_nand_sem))//Add semaphore, modified by Kevin
	{
		ERR_PRINT("IO Error!\n");
		ERR_PRINT("Semaphore error report.\n" );
  		return NAND_STATUS_FAIL;
	}

	Nand_Chip_Switch(dev_no);

	block_no = page / nand_block_size_get();

	if(!nand_access_ok(dev_no, page))
	{
		ERR_PRINT("Access denied!! Access block should >= %d(current: %d)\n", 
				g_innotab_nand_info[dev_no].firstMTDBlk, block_no);
		up(&innotab_nand_sem);
		return NAND_STATUS_FAIL;
	}

	bd_block_flag = good_block_check(block_no, (UINT32)gp_innotab_nand_buffer);

	if(bd_block_flag != NAND_GOOD_TAG && bd_block_flag != NAND_USER_BAD_TAG)//do not erase the original bad block
	{
		ERR_PRINT("DO NOT erase original bad block!\n");
		up(&innotab_nand_sem);
		return NAND_STATUS_FAIL;
	}

	if(Nand_ErasePhyBlock(block_no))
	{
		ERR_PRINT("page[%d], block_no[%d]\n", page, block_no);
		up(&innotab_nand_sem);
		return NAND_STATUS_FAIL;
	}

	up(&innotab_nand_sem);
	
	return 0;
}



/*
 * Write the bad block marker to OOB area of a page.
 *
 * @dev_no:	device number, currently always 0.
 * @page:	page number of the location to be marked bad.
 *
 * return:	0			success
 *			-EIO		write failure
 *
 * FIXME	Should pass the bad block pattern as well?
 */
SINT32 innotab_nand_mark_bd_oob( SINT32 dev_no, SINT32 page )
{
	SINT32 block_no;
	
	DB_PRINT("page[%d]\n", page);	

	if(down_interruptible(&innotab_nand_sem))//Add semaphore, modified by Kevin
	{
		ERR_PRINT("page[%d]\n", page);	
		ERR_PRINT("IO Error!\n");
		return -EIO;
	}

	Nand_Chip_Switch(dev_no);

	block_no = page / nand_block_size_get();

	if(!nand_access_ok(dev_no, page))
	{
		ERR_PRINT("Access denied!! Access block should >= %d(current: %d)\n", 
				g_innotab_nand_info[dev_no].firstMTDBlk, block_no);
		up(&innotab_nand_sem);
#if 0
		return -EIO;
#else
		return 0;
#endif
	}

	if(Nand_sw_bad_block_set(block_no, (UINT32)gp_innotab_nand_buffer, NAND_USER_BAD_TAG))
	{
		ERR_PRINT("Mark BadBlock fail!! Block no: %d\n", block_no);
		up(&innotab_nand_sem);
		return -EIO;
	}

	up(&innotab_nand_sem);

	return 0;
}


/*
 * Check the OOB area to see if it is a bad block
 *
 * @dev_no:	device number, currently always 0.
 * @page:	Check bad block marker at this page.
 *
 * return:	0			good block
 *			1			bad block
 *
 * remark:	This functions should check all the followings:
 *			1.  Factory-marked bad block marker (chip-specific)
 *			2.  Bad block marker immediately after erase.
 *			3.	Pattern after calling innotab_nand_mark_bd_oob() to a block that may be bad
 */
SINT32	innotab_nand_check_bd_oob( SINT32 dev_no, SINT32 page )
{
	SINT32 block_no;
	unsigned short bd_block_flag = 0;

//	DB_PRINT("page[%d]\n", page);	

	if(down_interruptible(&innotab_nand_sem))//Add semaphore, modified by Kevin
	{
		ERR_PRINT("page[%d]\n", page);	
		ERR_PRINT("IO Error!\n");
		return -EIO;
	}

	Nand_Chip_Switch(dev_no);
	
	block_no = page / nand_block_size_get();
	
	if(!nand_access_ok(dev_no, page))
	{
//		ERR_PRINT("Access denied!! Access block should >= %d(current: %d)\n", 
//				g_innotab_nand_info[dev_no].firstMTDBlk, block_no);
		up(&innotab_nand_sem);
#if 0
		return -EIO;
#else
		return 1;
#endif
	}

	if(g_innotab_nand_info[dev_no].memoryType == NAND_ROM)
	{
		up(&innotab_nand_sem);
		return 0;
	}

	bd_block_flag = good_block_check(block_no, (UINT32)gp_innotab_nand_buffer);

	if (bd_block_flag == 0xFF)
	{
		//DB_PRINT("page[%d], block_no[%d]\n", page, block_no);
		up(&innotab_nand_sem);
		return 0;
	}
	else
	{
		up(&innotab_nand_sem);
		return 1;
	}
}

/*
 * Erease all chips of the current device.
 *
 * @dev_no:	device number, currently always 0.
 */
SINT32 innotab_nand_erase_all( SINT32 dev_no )
{
	SINT32 block_no;
	unsigned short bd_block_flag = 0;
	DB_PRINT("\n");
	

	if(down_interruptible(&innotab_nand_sem))//Add semaphore, modified by Kevin
	{
		ERR_PRINT("IO Error!\n");
		return -EIO;
	}

	Nand_Chip_Switch(dev_no);

	for(block_no = g_innotab_nand_info[dev_no].firstMTDBlk; block_no < gPhysical_NandInfo->wNandBlockNum; block_no ++)
	{
		bd_block_flag = good_block_check(block_no, (UINT32)gp_innotab_nand_buffer);
	
		if(bd_block_flag != NAND_GOOD_TAG && bd_block_flag != NAND_USER_BAD_TAG)//do not erase the original bad block
		{
			DB_PRINT("DO NOT erase original bad block!\n");
			continue;
		}

		if(Nand_ErasePhyBlock(block_no))
		{
			ERR_PRINT("Block erase fail!! block_no: %d\n", block_no);
		}
	}

	up(&innotab_nand_sem);
	return 0;
}

/*
 * @dev_no:	device number, currently always 0.
 * @page:	page number of the page for which dirty flag is to be checked.
 *
 * return:  0		clean page, still writable without erase
 *			1		dirty page, cannot be written without erase
 */
SINT32 innotab_nand_page_dirty( SINT32 dev_no, SINT32 page )
{
	DB_PRINT("NOT IMPLEMENT!\n");
	return 0;
}

/*
 * Check whether the device is write protected.  Assumed to be unchanged after initialization.
 *
 * return:	0		writable
 *			1		write-protected
 */ 
SINT32 innotab_nand_check_wp( SINT32 dev_no )
{
//	DB_PRINT("NOT IMPLEMENT!\n");
	return 0;
}


/*
 * After the NandCard unplug, we should call this function to release internal resource
 */
void innotab_nand_close( SINT32 dev_no )
{
	DB_PRINT("dev_no: %d\n", dev_no);

	if(down_interruptible(&innotab_nand_sem))//Add semaphore, modified by Kevin
	{
		ERR_PRINT("IO Error!\n");
		return;
	}

	Nand_Chip_Switch(dev_no);

	memset(gPhysical_NandInfo, 0, sizeof(Physical_NandInfo));
	memset(&g_innotab_nand_info[dev_no], 0, sizeof(g_innotab_nand_info[dev_no]));
	g_nand_init_flags[dev_no] = FALSE;

	up(&innotab_nand_sem);
}



//EXPORT SYMBOLs
//---------------------------------------------------------
EXPORT_SYMBOL(innotab_nand_read_ecc);

EXPORT_SYMBOL(innotab_nand_read_oob);//N/A

EXPORT_SYMBOL(innotab_nand_read_raw_page);//N/A

EXPORT_SYMBOL(innotab_nand_write_page);

EXPORT_SYMBOL(innotab_nand_write_oob);//N/A

EXPORT_SYMBOL(innotab_nand_erase_block);

EXPORT_SYMBOL(innotab_nand_mark_bd_oob);

EXPORT_SYMBOL(innotab_nand_check_bd_oob);

EXPORT_SYMBOL(innotab_nand_erase_all);//N/A

EXPORT_SYMBOL(innotab_nand_init);

EXPORT_SYMBOL(innotab_nand_page_dirty);//N/A

EXPORT_SYMBOL(innotab_nand_check_wp);//N/A

EXPORT_SYMBOL(innotab_nand_close);
