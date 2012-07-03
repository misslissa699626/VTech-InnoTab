/*
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 * This is the low-level hd interrupt support. It traverses the
 * request-list, using interrupts to jump between functions. As
 * all the functions are called within interrupts, we may not
 * sleep. Special care is recommended.
 *
 *  modified by Drew Eckhardt to check nr of hd's from the CMOS.
 *
 *  Thanks to Branko Lankester, lankeste@fwi.uva.nl, who found a bug
 *  in the early extended-partition checks and added DM partitions
 *
 *  IRQ-unmask, drive-id, multiple-mode, support for ">16 heads",
 *  and general streamlining by Mark Lord.
 *
 *  Removed 99% of above. Use Mark's ide driver for those options.
 *  This is now a lightweight ST-506 driver. (Paul Gortmaker)
 *
 *  Modified 1995 Russell King for ARM processor.
 *
 *  Bugfix: max_sectors must be <= 255 or the wheels tend to come
 *  off in a hurry once you queue things up - Paul G. 02/2001
 */

/* Uncomment the following if you want verbose error reports. */
/* #define VERBOSE_ERRORS */

/*
 * Sample disk driver, from the beginning.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>       /* printk() */
#include <linux/slab.h>         /* kmalloc() */
#include <linux/fs.h>           /* everything... */
#include <linux/errno.h>        /* error codes */
#include <linux/timer.h>
#include <linux/types.h>        /* size_t */
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/hdreg.h>        /* HDIO_GETGEO */
#include <linux/kdev_t.h>
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>		/*struct request*/
#include <linux/buffer_head.h>  /* invalidate_bdev */
#include <linux/bio.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/blkpg.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>

#include <mach/irqs.h>

#include "NFTL/nand_pgbase.h"
#include "champ/nand_FDisk.h"
#include "champ/storage_op.h"
//#include "spmp8k_nand.h"

#include "hal/hal_base.h"

MODULE_LICENSE("Dual BSD/GPL");
#define NAND_DEVICE_NAME "nand"

#if SPMP_NF_S330

nf_disk_func nf_nftl_op ={
	.detect = NFTL_Init,
	.read   = NFTL_Read,
	.write  = NFTL_Write,
	.erase  = NULL,
	.CacheFlush = NFTL_CacheFlush_ex,
	.remove = NFTL_Close,
	.fdisk  = NFTL_FDisk,
};

static nf_disk nand0_platdata =
{
	.devinfo = &nf_Disk0, 
	.func    = &nf_nftl_op,
};

static struct resource spmp_resources_nand0[] =
{
	[0] =
	{
		.start = 0x93008000,
		.end   = 0x93008fff,
		.flags = IORESOURCE_MEM,
	},
	[1] =
	{
		.start = IRQ_NAND0,
		.end   = IRQ_NAND0,
		.flags = IORESOURCE_IRQ,
	},
};

static u64 spmp_nand0_dma_mask = DMA_BIT_MASK(32);
struct platform_device spmp_nand0_device = {
		.name = NAND_DEVICE_NAME "a",
		.id = 0,
		.dev = {
			.platform_data = &nand0_platdata,
			.dma_mask = &spmp_nand0_dma_mask,
			.coherent_dma_mask = DMA_BIT_MASK(32),
		},
		.num_resources = ARRAY_SIZE(spmp_resources_nand0),
		.resource = spmp_resources_nand0,
};
#if (SUPPORT_NF_DISK1==1)
static nf_disk nand1_platdata = {
		.devinfo = &nf_Disk1,
 		.func    = &nf_nftl_op,
};

static struct resource spmp_resources_nand1[] = {
	[0] =
	{
		.start = 0x93009000,
		.end = 0x93009fff,
		.flags = IORESOURCE_MEM,
	},
	[1] =
	{
		.start = IRQ_NAND1,
		.end = IRQ_NAND1,
		.flags = IORESOURCE_IRQ,
	},
};

static u64 spmp_nand1_dma_mask = DMA_BIT_MASK(32);
struct platform_device spmp_nand1_device = {
		.name = NAND_DEVICE_NAME "b",
		.id = 1,
		.dev =
		{
			.platform_data = &nand1_platdata,
			.dma_mask = &spmp_nand1_dma_mask,
			.coherent_dma_mask = DMA_BIT_MASK(32),
		},
		.num_resources = ARRAY_SIZE(spmp_resources_nand1),
		.resource = spmp_resources_nand1,
};
#endif	//SUPPORT_NF_DISK1
#endif	//SPMP_NF_S330

#define NF_REQUEST_USE_MAKE
#define NF_TEST_DEBUG_ENABLE 1

static int spmp_nand_major = 0;
/* del by mm.li 01-12,2011 clean warning */
/*
static int hardsect_size = 512;
*/
/* del end */
static int nsectors = 1024*2*1024; /* How big the drive is */
static int ndevices = 1;

/*
 * The different "request modes" we can use.
 */
enum
{
	RM_SIMPLE = 0, /* The extra-simple request function */
	RM_FULL = 1, /* The full-blown version */
	RM_NOQUEUE = 2,
/* Use make_request */
};
static int request_mode = RM_SIMPLE;

#if 0
module_param(hardsect_size, int, 0);
module_param(nsectors, int, 0);
module_param(spmp_nand_major, int, 0);
module_param(ndevices, int, 0);
module_param(request_mode, int, 0);
#endif
/*
 * Minor number and partition management.
 */
//#define SBULL_MINORS    1
#define MINOR_SHIFT     4
#define DEVNUM(kdevnum) (MINOR(kdev_t_to_nr(kdevnum)) >> MINOR_SHIFT

/*
 * We can tweak our hardware sector size, but the kernel talks to us
 * in terms of small sectors, always.
 */
#define KERNEL_SECTOR_SIZE      512

/*
 * After this much idle time, the driver will simulate a media change.
 */
#define INVALIDATE_DELAY        30*HZ

/*
 * The internal representation of our device.
 */
struct spmp_nand_dev
{
	int size; /* Device size in sectors */
	u8 *data; /* The data array */
	short users; /* How many users */
	short media_change; /* Flag a media change? */
	struct platform_device* platdata;
	void __iomem *base;
	void __iomem *base_ecc;
	spinlock_t lock; /* For mutual exclusion */
	struct request_queue *queue; /* The device request queue */
	struct gendisk *gd; /* The gendisk structure */
	struct timer_list timer; /* For simulated media changes */
	struct task_struct *thread;

	//////////////////////////////
	Chip_Info_t NandChip;
	
	char spmp_nand_open_flag;
	unsigned char *AP_Read_buffer;
	unsigned char *AP_Write_buffer;
	gp_nand_phy_partition_t nf_partition;
	unsigned long nf_ioctl_write_count;
	unsigned long nf_ioctl_read_count;
	unsigned int nandCacheFlag ;
	
#if (NF_TEST_DEBUG_ENABLE==1)
	unsigned char * g_read_data;
	unsigned char * g_ecc_data;
#endif
};

static struct spmp_nand_dev *Devices = NULL;

#define DEBUG_PRINT_BIN 1
#define DEBUG_PRINT_OTX 2
#define DEBUG_PRINT_HEX 3
void dump_print(unsigned char *hint, unsigned char *data, unsigned int Datalen, unsigned char PrintType)
{
	int idx = 0;

	if(hint!=NULL)
		printk("%s: ", hint);
	else
		printk("Data: ");
	for (idx = 0; idx < Datalen; idx++)
	{
		if ((idx != 0) && (idx % 8 == 0))
			printk("\t");
		if (idx % 16 == 0)
			printk("\n");
		switch (PrintType)
		{
			case DEBUG_PRINT_BIN://bin
				break;
			case DEBUG_PRINT_OTX://otx
				break;
			case DEBUG_PRINT_HEX://hex
				printk("%02x  ", data[idx]);
				break;
			default:
				break;
		}
	}
	printk("\n");
}

unsigned int debug_flag = 0;
extern void EraseBlock(unsigned long u32BlkNo);
extern int ReadWritePage(unsigned long u32PhyAddr, unsigned long* PyldBuffer, unsigned long* DataBuffer, unsigned char u8RWMode);
extern int BCHProcess_ex(unsigned long* PyldBuffer, unsigned long* ReduntBuffer, unsigned int len, int op);
extern int Bch(unsigned long* PyldBuffer, unsigned long* ReduntBuffer, unsigned int len, int op);
extern NPB_Info_t g_npb_info;
extern void NPB_Erase_block(unsigned short blk);

#define AP_DEBUG_BUFFER_SIZE  8*1


extern void print_buf(unsigned char * buf, int len);

#if (NF_TEST_DEBUG_ENABLE==1)
#include "./hal/nf_s330.h"
/*
	test code
*/

int nf_test_memcmp(unsigned char* ptr1, unsigned char ptr2, int len)
{
	int i;
	int count = (len + 3) >> 2;

	for (i = 0; i < count; i++)
	{
		if (ptr1[i] != ptr2)
			return i;
	}

	return 0;
}

unsigned long nf_test_get_mask(char mask_bitnum)
{
	int i = 0;
	unsigned long nf_mask = 0;

	for(i = 0; i<mask_bitnum; i++)
		nf_mask |= 1<<i;

	return nf_mask;
}
/*
	PhysAddr: Byte Aligned Addr
*/
void nf_test_read_phys_sector(struct spmp_nand_dev *dev, unsigned long PageAddr, unsigned long SectorAddr, int datalen)
{
	int ret=0;
	int page_num = 0, i= 0, j=0;
	unsigned int block_addr, page_addr, sector_addr, byte_addr;
	int read_len = 0, read_addr= 0;

	block_addr = PageAddr>>g_npb_info.blockshift;
	page_addr = PageAddr;
	sector_addr = SectorAddr;
	byte_addr = 0;
	
	SPMP_DEBUG_PRINT("PhysAddr: %08x  (block=%x page=%x sector=%x, byte=%x) page_per_block=%d pagesize=%d\n", 
			(unsigned int)PageAddr, block_addr,page_addr	, sector_addr, byte_addr, g_npb_info.page_per_block, g_npb_info.pagesize);

	page_num = datalen / 2048;
	if(datalen % 2048)
		page_num ++;

	read_addr = page_addr;
	
	for(j = 0; j<page_num; j++)
	{
		memset(dev->g_read_data, 0, g_npb_info.pagesize);
		memset(dev->g_ecc_data, 0, g_npb_info.pagesize);
		if( j==0 )
			debug_flag |= (sector_addr&0xf)<<8;
		ReadWritePage((unsigned long)(read_addr + j), (unsigned long*)dev->g_read_data, (unsigned long*)dev->g_ecc_data, 2);//NF_READ
		debug_flag &= 0xff;
		
		ret = BCHProcess_ex((unsigned long*)dev->g_read_data, (unsigned long*)dev->g_ecc_data, g_npb_info.pagesize, 7);//decode
		//ret = Bch((unsigned long*)g_read_data, (unsigned long*)g_ecc_data, g_npb_info.pagesize, 7);//decode
		if(ret != 1)
		{
			SPMP_DEBUG_PRINT("ECC Failed !\n");
		}
		
		if(j==0)
			i = sector_addr;
		else
			i = 0;
		for(; i < 4 ; i++)
		{
			if(read_len >= datalen)
				break;
			if(read_len + 512 > datalen)
				print_buf(dev->g_read_data + i*512, datalen - read_len);
			else
				print_buf(dev->g_read_data + i*512, 512);
			read_len+=512;
		}
		if(read_len>=datalen)
			break;
	}
}

/*
	PageAddr: Byte Aligned Addr
*/
void nf_test_write_phys_sector(struct spmp_nand_dev *dev, unsigned long PageAddr, unsigned long SectorAddr, int datalen, unsigned char pattern)
{
	unsigned int ret = SUCCESS;
	unsigned int sector_per_page = g_npb_info.sector_per_page;
	unsigned int sect_not_align = SectorAddr;
	unsigned int startAddr = PageAddr;
	unsigned int writeSize = 0;
	unsigned int SectorNum=0;

	SectorNum = datalen/(1<<g_npb_info.sectorhift)  ;
	if( datalen % (1<<g_npb_info.sectorhift) )
	{
		SectorNum++;
	}
	EraseBlock(PageAddr>>g_npb_info.blockshift);
	if (sect_not_align)
	{
		writeSize = (sector_per_page - sect_not_align);

		if (SectorNum < writeSize)
		{
			writeSize = SectorNum;
		}
		memset(dev->g_read_data, 0, g_npb_info.pagesize);
		memset(dev->g_ecc_data, 0, g_npb_info.pagesize);
		
		ReadWritePage((unsigned long)startAddr, (unsigned long*)dev->g_read_data, (unsigned long*)dev->g_ecc_data, 2);//READ
		ret = BCHProcess_ex((unsigned long*)dev->g_read_data, (unsigned long*)dev->g_ecc_data, g_npb_info.pagesize, 7);//decode
		if(ret != 1)
		{
			SPMP_DEBUG_PRINT("ECC Failed !\n");
		}
		memset(dev->g_read_data + (sect_not_align << g_npb_info.sectorhift), pattern, writeSize<< g_npb_info.sectorhift);
		memset(dev->g_ecc_data, 0, g_npb_info.pagesize);
		ret = BCHProcess_ex((unsigned long*)dev->g_read_data, (unsigned long*)dev->g_ecc_data, g_npb_info.pagesize, 6);//encode
		if(ret != 1)
		{
			SPMP_DEBUG_PRINT("ECC Failed !\n");
		}
		ReadWritePage((unsigned long)startAddr, (unsigned long*)dev->g_read_data, (unsigned long*)dev->g_ecc_data, 3);//write


		startAddr ++;
		SectorNum -= writeSize;
	}

	while (SectorNum >= sector_per_page)
	{
		memset(dev->g_read_data, pattern,  g_npb_info.pagesize);
		memset(dev->g_ecc_data, 0, g_npb_info.pagesize);
		ret = BCHProcess_ex((unsigned long*)dev->g_read_data, (unsigned long*)dev->g_ecc_data, g_npb_info.pagesize, 6);//encode
		if(ret != 1)
		{
			SPMP_DEBUG_PRINT("ECC Failed !\n");
		}
		ReadWritePage((unsigned long)startAddr, (unsigned long*)dev->g_read_data, (unsigned long*)dev->g_ecc_data, 3);//write
		startAddr++;
		SectorNum -= sector_per_page;
	}

	if (SectorNum)
	{
		memset(dev->g_read_data, 0, g_npb_info.pagesize);
		memset(dev->g_ecc_data, 0, g_npb_info.pagesize);
		
		ReadWritePage((unsigned long)startAddr, (unsigned long*)dev->g_read_data, (unsigned long*)dev->g_ecc_data, 2);//READ
		ret = BCHProcess_ex((unsigned long*)dev->g_read_data, (unsigned long*)dev->g_ecc_data, g_npb_info.pagesize, 7);//decode
		if(ret != 1)
		{
			SPMP_DEBUG_PRINT("ECC Failed !\n");
		}
		memset(dev->g_read_data, pattern, SectorNum << g_npb_info.sectorhift);
		memset(dev->g_ecc_data, 0, g_npb_info.pagesize);
		ret = BCHProcess_ex((unsigned long*)dev->g_read_data, (unsigned long*)dev->g_ecc_data, g_npb_info.pagesize, 6);//encode
		if(ret != 1)
		{
			SPMP_DEBUG_PRINT("ECC Failed !\n");
		}
		ReadWritePage((unsigned long)startAddr, (unsigned long*)dev->g_read_data, (unsigned long*)dev->g_ecc_data, 3);//write
	}
}

/*
	PhysAddr: Byte Aligned Addr
*/
void nf_test_read_phys_page(struct spmp_nand_dev *dev, unsigned long PhysAddr, int datalen)
{
	int ret=0;
	int page_num = 0, i= 0, j=0;
	unsigned int block_addr, page_addr, sector_addr, byte_addr;
	int read_len = 0, read_addr= 0;

	block_addr = (PhysAddr>>(g_npb_info.blockshift+g_npb_info.pageshift));
	page_addr = (PhysAddr>>g_npb_info.pageshift)&nf_test_get_mask(g_npb_info.blockshift);
	sector_addr = (PhysAddr>>g_npb_info.sectorhift)&nf_test_get_mask(g_npb_info.pageshift-g_npb_info.sectorhift);
	byte_addr = (PhysAddr)&nf_test_get_mask(g_npb_info.sectorhift);
	
	SPMP_DEBUG_PRINT("PhysAddr: %08x  (block=%x page=%x sector=%x, byte=%x) page_per_block=%d pagesize=%d\n", 
			 (unsigned int)PhysAddr, block_addr,page_addr	, sector_addr, byte_addr, g_npb_info.page_per_block, g_npb_info.pagesize);

	page_num = datalen / 2048;
	if(datalen % 2048)
		page_num ++;

	read_addr = block_addr<<g_npb_info.blockshift;
	read_addr += page_addr;
	
	for(j = 0; j<page_num; j++)
	{
		memset(dev->g_read_data, 0, 2048);
		ReadWritePage(read_addr + j, (unsigned long*)dev->g_read_data, (unsigned long*)dev->g_ecc_data, 2);//NF_READ
		
		ret = BCHProcess_ex((unsigned long*)dev->g_read_data, (unsigned long*)dev->g_ecc_data, 2048, 7);//decode
		if(ret != 1)
		{
			SPMP_DEBUG_PRINT("ECC Failed !\n");
		}
		
		if(j==0)
			i = sector_addr;
		else
			i = 0;
		for(; i < 4 ; i++)
		{
			if(read_len >= datalen)
				break;
			if(read_len + 512 > datalen)
				print_buf(dev->g_read_data + i*512, datalen - read_len);
			else
				print_buf(dev->g_read_data + i*512, 512);
			read_len+=512;
		}
		if(read_len>=datalen)
			break;
	}
}

/*
	PhysAddr: Byte Aligned Addr
*/
void nf_test_write_phys_page(struct spmp_nand_dev *dev, unsigned long PhysAddr, int datalen, unsigned char pattern)
{
	int ret=0;
	int page_num = 0,  j=0;
	unsigned int block_addr, page_addr, sector_addr, byte_addr;
	int write_len = 0, read_addr= 0;

	block_addr = (PhysAddr>>(g_npb_info.blockshift+g_npb_info.pageshift));
	page_addr = (PhysAddr>>g_npb_info.pageshift)&nf_test_get_mask(g_npb_info.blockshift);
	sector_addr = (PhysAddr>>g_npb_info.sectorhift)&nf_test_get_mask(g_npb_info.pageshift-g_npb_info.sectorhift);
	byte_addr = (PhysAddr)&nf_test_get_mask(g_npb_info.sectorhift);
	
	SPMP_DEBUG_PRINT("PhysAddr: %08x  (block=%x page=%x sector=%x, byte=%x) page_per_block=%d pagesize=%d\n", 
			 (unsigned int)PhysAddr, block_addr, page_addr, sector_addr, byte_addr, g_npb_info.page_per_block, g_npb_info.pagesize);

	page_num = datalen / 2048;
	if(datalen % 2048)
		page_num ++;

	read_addr = block_addr<<g_npb_info.blockshift;
	read_addr += page_addr;
	
	for(j = 0; j<page_num; j++)
	{
		if(write_len>=datalen)
			break;
		memset(dev->g_read_data, pattern, 2048);
		if( (datalen - write_len) < 2048)
		{
			ReadWritePage((unsigned long)(read_addr + j), (unsigned long*)dev->g_read_data,(unsigned long*)dev->g_ecc_data, 2);//NF_READ
			ret = BCHProcess_ex((unsigned long*)dev->g_read_data, (unsigned long*)dev->g_ecc_data, 2048, 7);//decode
			if(ret != 1)
			{
				SPMP_DEBUG_PRINT("ECC Failed !\n");
			}
			memset(dev->g_read_data, pattern, datalen - write_len);
		}
		ret = BCHProcess_ex((unsigned long*)dev->g_read_data, (unsigned long*)dev->g_ecc_data, 2048, 6);//decode
		if(ret != 1)
		{
			SPMP_DEBUG_PRINT("ECC Failed !\n");
		}
		ReadWritePage((unsigned long)(read_addr + j), (unsigned long*)dev->g_read_data, (unsigned long*)dev->g_ecc_data, 3);//NF_READ
		write_len+=2048;
	}
}

/*
	LogicAddr: Byte Aligned Addr
*/
extern int nand_read_page(unsigned long phyPage, unsigned char* pReadPyldBuffer, unsigned char* pECCBuffer);
extern SINT32 nand_set_invalid(void);

void nf_test_read_logic_sector(struct spmp_nand_dev *dev, unsigned long SectorAddr, int datalen, void *pdev)
{
	
	int SectorNum = 0, j=0;
	int read_len = 0;

	SPMP_DEBUG_PRINT("SectorAddr: %08x  DataLen=%d\n", (unsigned int)SectorAddr, datalen);

	SectorNum = datalen / 512;
	if(datalen % 512)
		SectorNum ++;

	for(j = 0; j < SectorNum; j++)
	{
		memset(dev->g_read_data, 0, 4096);
		//debug_flag |= ((SectorAddr + j)&(g_npb_info.sector_per_page -1))<<8;
		NFTL_Read( SectorAddr + j, 1, dev->g_read_data);
		//debug_flag &=0xff;
		if(read_len >= datalen)
			break;
		if(read_len + 512 > datalen)
			print_buf(dev->g_read_data , datalen - read_len);
		else
			print_buf(dev->g_read_data , 512);
		read_len += 512;

		if(read_len >= datalen)
			break;
	}
}
void nf_test_write_logic_sector(struct spmp_nand_dev *dev, unsigned long SectorAddr, int datalen, unsigned char pattern,void *pdev)
{
	int SectorNum = 0, j=0;
	int write_len = 0;

	SPMP_DEBUG_PRINT("SectorAddr: %08x  DataLen=%d pattern=%02x\n", (unsigned int)SectorAddr, datalen, pattern);

	SectorNum = datalen / 512;
	if(datalen % 512)
		SectorNum ++;

	for(j = 0; j < SectorNum; j++)
	{
		if(write_len >= datalen)
			break;
		memset(dev->g_read_data, pattern, 4096);
		if( (datalen - write_len) < 512)
		{
			NFTL_Read(SectorAddr + j, 1, dev->g_read_data);
			memset(dev->g_read_data, pattern, datalen - write_len);
		}
		NFTL_Write(SectorAddr + j, 1, dev->g_read_data);

		write_len += 512;
	}
}

extern NFFS_Block_info_t gnfs_block_info;
void nf_test_write_all_sector(struct spmp_nand_dev *dev, unsigned char pattern, unsigned char flag)
{
	unsigned long TotalSecotr=0;
	int j=0;
	int errTotal=0;
	
	TotalSecotr = (gnfs_block_info.user_block.count + gnfs_block_info.npb_block.count -2)<<
		(gnfs_block_info.blockshift + gnfs_block_info.pageshift-9);
	SPMP_DEBUG_PRINT("TotalSecotr: %lu  TotalBlocks=%d Sector_per_Block=%d\n", TotalSecotr, 
		gnfs_block_info.user_block.count + gnfs_block_info.npb_block.count -2, 
		(gnfs_block_info.blockshift + gnfs_block_info.pageshift-9));
	
	for(j = 4; j < TotalSecotr; j++)
	{
		memset(dev->g_read_data, pattern, g_npb_info.pagesize);
		NFTL_Write(j, g_npb_info.sector_per_page, dev->g_read_data);
		if(j%(1024*2*50)==0)
				printk("CheckSize=%d, errTotal=%d\n", j<<9, errTotal);
	}
	NFTL_CacheFlush_ex();
	if(flag)
	{
		for(j = 4; j < TotalSecotr; j++)
		{
			memset(dev->g_read_data, 0, g_npb_info.pagesize);
			NFTL_Read(j, g_npb_info.sector_per_page, dev->g_read_data);
			if(nf_test_memcmp(dev->g_read_data, pattern, g_npb_info.pagesize)!=0)
			{
				errTotal++;
			}
			if(j%(1024*2*50)==0)
				printk("CheckSize=%d, errTotal=%d\n", j<<9, errTotal);
		}
		SPMP_DEBUG_PRINT("errTotal=%d\n", errTotal);
	}
}

#define NF_TEST_ASSIGNED_SECTOR   200
#define NF_TEST_WTITE_NUMBER	1

void nf_test_write_logsector_1(struct spmp_nand_dev *dev, unsigned long logSector, unsigned long WriteNum)
{
//	unsigned long TotalSecotr=0;
	int j=0, i=0;
	int errTotal=0;
	unsigned char *PTestBuffer;

	PTestBuffer = memAlloc(64*1024);
	if(PTestBuffer==NULL)
	{
		SPMP_DEBUG_PRINT("malloc failed\n");
		return;
	}
	if(logSector<4)
		logSector = NF_TEST_ASSIGNED_SECTOR;
	if(WriteNum==0)
		WriteNum=NF_TEST_WTITE_NUMBER;
	
	SPMP_DEBUG_PRINT("logSector=%lu WriteNum=%lu\n",logSector,WriteNum);
	for(j = 0; j < WriteNum; j++)
	{
		//write
		SPMP_DEBUG_PRINT("Write No.%d \n", j+1);
		for(i = 1; i <= 64; i++)
		{
			memset(PTestBuffer, i, i*1024);
			NFTL_Write(logSector, i*2, PTestBuffer);
		}
		NFTL_CacheFlush_ex();
		i=64;
		//compare
		//for(i = 0; i < 64; i++)
		{
			memset(PTestBuffer, 0, i*1024);
			NFTL_Read(logSector, i*2, PTestBuffer);
			if(nf_test_memcmp(PTestBuffer, i,i*1024)!=0)
			{
				errTotal++;
				SPMP_DEBUG_PRINT("errTotal=%d \n", errTotal);
			}
		}
	}
	memFree(PTestBuffer);
	SPMP_DEBUG_PRINT("errTotal=%d \n", errTotal);
}

void nf_test_write_logsector_2(struct spmp_nand_dev *dev, unsigned long logSector, unsigned long WriteNum)
{
//	unsigned long TotalSecotr=0;
	int j=0, i=0;
	int errTotal=0;
	unsigned char *PTestBuffer;

	PTestBuffer = memAlloc(64*1024);
	if(PTestBuffer==NULL)
	{
		SPMP_DEBUG_PRINT("malloc failed\n");
		return;
	}
	if(logSector<4)
		logSector = NF_TEST_ASSIGNED_SECTOR;
	if(WriteNum==0)
		WriteNum=NF_TEST_WTITE_NUMBER;
	
	SPMP_DEBUG_PRINT("logSector=%lu WriteNum=%lu\n",logSector,WriteNum);
	for(j = 0; j < WriteNum; j++)
	{
		//write
		SPMP_DEBUG_PRINT("Write No.%d \n", j+1);
		for(i = 1; i <= 64; i++)
		{
			memset(PTestBuffer, i, i*1024);
			NFTL_Write(logSector, i*2, PTestBuffer);
			NFTL_CacheFlush_ex();

			memset(PTestBuffer, 0x5a, 1024);
			NFTL_Write(logSector, 2, PTestBuffer);
			NFTL_CacheFlush_ex();
		}
		
		i=64;
		//compare
		{
			memset(PTestBuffer, 0, 1024);
			NFTL_Read(logSector, 2, PTestBuffer);
			if(nf_test_memcmp(PTestBuffer, 0x5a,1024)!=0)
			{
				errTotal++;
				SPMP_DEBUG_PRINT("errTotal=%d \n", errTotal);
			}
		}
	}
	memFree(PTestBuffer);
	SPMP_DEBUG_PRINT("errTotal=%d \n", errTotal);
}

typedef struct nf_test_cmd_head_{
	char cmd;
	char para_num;
	char para_len;
}nf_test_cmd_head;
typedef struct nf_test_cmd_{
	nf_test_cmd_head head;
	char hint[64];
	unsigned long para[8];
}nf_test_cmd;
void nf_test_func( unsigned long test_pkg, struct spmp_nand_dev *dev)
{
	nf_disk *pnand = (nf_disk *) (dev->platdata->dev.platform_data);
	nf_test_cmd pkg;
	nf_test_cmd_head *pkg_head;
	unsigned char *pkg_data;
	int i = 0;

	copy_from_user(&pkg, (unsigned char *)test_pkg, sizeof(nf_test_cmd));
	pkg_head = &(pkg.head);
	pkg_data =(unsigned char *)pkg.para;
	SPMP_DEBUG_PRINT("%s\n", pkg.hint);

	printk("para: \t"); for(i=0; i<pkg_head->para_len; i++)	printk("%02x ", pkg_data[i]);	printk("\n");

	switch(pkg_head->cmd)
	{
		case 25:
		{
			unsigned long *logSector = (unsigned long *)(pkg_data);
			unsigned long *WriteNum = (unsigned long *)(pkg_data+4);
			
			//nf_test_write_logsector_1(pnand->devinfo, *logSector, *WriteNum);
			nf_test_write_logsector_2(dev, *logSector, *WriteNum);
		}
			break;
		case 24:
		{
			unsigned char Pattern = pkg_data[0];
			unsigned char flag = pkg_data[4];
			
			nf_test_write_all_sector(dev, Pattern, flag);
		}
			break;
		case 11:
		{
			unsigned long *PageAddr = (unsigned long *)(pkg_data);
			unsigned long *SectorAddr = (unsigned long *)(pkg_data+4);
			unsigned long *DataLen = (unsigned long *)(pkg_data+8);
			unsigned char Pattern = pkg_data[12];

			SPMP_DEBUG_PRINT("PageAddr=%08x, SectorAddr=%08x, DataLen=%lu\n", (unsigned int)*PageAddr, (unsigned int)*SectorAddr , *DataLen);
			if( (((*PageAddr)<<g_npb_info.pageshift) + (*DataLen) ) > (dev->size << 9)  )
			{
				SPMP_DEBUG_PRINT("Beyond Nand addr arrange: %08x %08x\n", (unsigned int)(((*PageAddr)<<g_npb_info.pageshift) + *DataLen), (unsigned int)(dev->size << 9) );
				break;
			}
			nf_test_write_phys_sector(dev, *PageAddr, *SectorAddr, *DataLen, Pattern );
		}
			break;
		case 12:
		{
			unsigned long *PageAddr = (unsigned long *)(pkg_data);
			unsigned long *SectorAddr = (unsigned long *)(pkg_data+4);
			unsigned long *DataLen = (unsigned long *)(pkg_data+8);

			SPMP_DEBUG_PRINT("PageAddr=%08x, SectorAddr=%08x, DataLen=%lu\n", (unsigned int)*PageAddr, (unsigned int)*SectorAddr , *DataLen);
			if( (((*PageAddr)<<g_npb_info.pageshift) + (*DataLen) ) > (dev->size << 9)  )
			{
				SPMP_DEBUG_PRINT("Beyond Nand addr arrange: %08x %08x\n", (unsigned int)(((*PageAddr)<<g_npb_info.pageshift) + *DataLen), (unsigned int)(dev->size << 9) );
				break;
			}
			nf_test_read_phys_sector(dev, *PageAddr, *SectorAddr, *DataLen);
		}
			break;
			
		case 8:
		{
			unsigned long *PageAddr = (unsigned long *)(pkg_data);
			unsigned long *DataLen = (unsigned long *)(pkg_data+4);

			SPMP_DEBUG_PRINT("PageAddr=%08x, DataLen=%lu\n", (unsigned int)*PageAddr, *DataLen);
			if( (*PageAddr  + *DataLen) > (dev->size << 9)  )
			{
				SPMP_DEBUG_PRINT("Beyond Nand addr arrange: %08x %08x\n", (unsigned int)(*PageAddr + *DataLen), (unsigned int)(dev->size << 9) );
				break;
			}
			nf_test_read_phys_page(dev, *PageAddr, *DataLen);
		}
			break;
		case 9:
		{
			unsigned long *SectorAddr = (unsigned long *)(pkg_data);
			unsigned long *DataLen = (unsigned long *)(pkg_data+4);
			unsigned char *Pattern = pkg_data+8;

			SPMP_DEBUG_PRINT("SectorAddr=%08x, DataLen=%lu Pattern=%02x\n", (unsigned int)*SectorAddr, *DataLen, *Pattern);
			if( (*SectorAddr  + *DataLen) > (dev->size << 9)  )
			{
				SPMP_DEBUG_PRINT("Beyond Nand addr arrange: %08x %08x\n", (unsigned int)(*SectorAddr + *DataLen), (unsigned int)(dev->size << 9) );
				break;
			}
			nf_test_write_logic_sector(dev, *SectorAddr, *DataLen,*Pattern, pnand->devinfo);
		}
			break;
		case 10:
		{
			unsigned long *SectorAddr = (unsigned long *)(pkg_data);
			unsigned long *DataLen = (unsigned long *)(pkg_data+4);

			SPMP_DEBUG_PRINT("SectorAddr=%08x, DataLen=%d\n", (unsigned int)*SectorAddr, (int)*DataLen);
			if( (*SectorAddr  + *DataLen) > (dev->size << 9)  )
			{
				SPMP_DEBUG_PRINT("Beyond Nand addr arrange: %08x %08x\n", (unsigned int)(*SectorAddr + *DataLen), (unsigned int)(dev->size << 9) );
				break;
			}
			nf_test_read_logic_sector(dev, *SectorAddr, *DataLen, pnand->devinfo);
		}
			break;	
		case 20:
			nand_set_invalid();
			break;
		case 21:
			NFTL_FDisk();
			break;
		case 23:
			NFTL_CacheFlush_ex();
			break;
		default:
			SPMP_DEBUG_PRINT("Not Support\n");
			break;
	}
	return;
}
#endif

static int spmp_nand_transfer(struct spmp_nand_dev *dev, unsigned long sector, unsigned long nsect, char *buffer, int RW_Flag)
{
	nf_disk* pnand = (nf_disk *) (dev->platdata->dev.platform_data);
	unsigned long offset = sector * KERNEL_SECTOR_SIZE;
	unsigned long nbytes = nsect * KERNEL_SECTOR_SIZE;
//	int ret = 0;

	//sector += FS_FDISK_RESERVE_MB * 2048;
	//SPMP_DEBUG_PRINT("RW_Flag = %d offset = %ld, nbytes = %ld sector = %ld nsect = %ld\n", RW_Flag, offset, nbytes, sector, nsect);
	//if(RW_Flag)
	//	SPMP_DEBUG_PRINT("RW_Flag = %d  sector = %lu nsect=%lu\n", RW_Flag,  sector, nsect);
	

	if (dev == NULL || buffer == NULL)
	{
		SPMP_DEBUG_PRINT("para error!\n");
		return 0;
	}
	if ((offset + nbytes) > (dev->size << 9) )
	{
		printk("Beyond-end write (%lu %lu)\n", offset, nbytes);
		return 0;
	}
	//debug_flag = 1;
	//debug_app(pnand, buffer, RW_Flag);
	//return 1;
#if  0
	if(sector==2048)
		debug_flag = 0x301;
	else
		debug_flag = 0;
#endif
	if (RW_Flag)//write
	{
#if 0
		if(nsect > AP_DEBUG_BUFFER_SIZE)
		{
			SPMP_DEBUG_PRINT("write size=%lu\n", nsect*512);
			pnand->func->write(sector, nsect, buffer);
		}else{
			memcpy(AP_Write_buffer, buffer, nsect*512);
			//print_buf(AP_Write_buffer, 512);
			pnand->func->write(sector, nsect, AP_Write_buffer);
		}
#else
		pnand->func->write(sector, nsect, buffer);
#endif
		dev->nandCacheFlag++;
		if(dev->nandCacheFlag > 256*5)
		{
			dev->nandCacheFlag=0;
			NFTL_CacheFlush_ex();
		}
	} else//read
	{
#if 0
		if(nsect > AP_DEBUG_BUFFER_SIZE)
		{
			SPMP_DEBUG_PRINT("read size=%lu\n", nsect*512);
			pnand->func->read(sector, nsect, buffer);
		}else{
			pnand->func->read(sector, nsect, AP_Read_buffer);
			memcpy(buffer, AP_Read_buffer, nsect*512);
		}
#else
		pnand->func->read(sector, nsect, buffer);
#endif
	}
	
/*	
	if( (debug_flag&0xff) == 1)
	{
		DUMP_NF_BUFFER(debug_flag, AP_Read_buffer, 512, NULL, 0);
	}else
		SPMP_DEBUG_PRINT("read sector=%lu OK\n", sector);
	

	if(buffer[510]==0x55 && buffer[511]==0xaa)
	{
		SPMP_DEBUG_PRINT("RW_Flag = %d offset = %ld, nbytes = %ld sector = %ld nsect = %ld\n", RW_Flag, offset, nbytes, sector, nsect);
		print_buf(buffer, 512);
	}
*/
	return 1;
}

/*
 * The simple form of the request function.
 */
 void print_req(struct request *q)
{
	printk("cpu:  \t\t\t%08x\n", (unsigned int)q->cpu);
	
	printk("atomic_flags:  \t\t%08x\n", (unsigned int)q->atomic_flags);
	printk("__sector:  \t\t%08x\n", (unsigned int)q->__sector);
	printk("__data_len:  \t\t%08x\n", (unsigned int)q->__data_len);
	
	printk("start_time:  \t\t%08x\n", (unsigned int)q->start_time);
	printk("nr_phys_segments:  \t%08x\n", (unsigned int)q->nr_phys_segments);
	printk("ioprio:  \t%08x\n", (unsigned int)q->ioprio);

	printk("cmd:  \t\t%s\n", q->cmd);
	printk("resid_len:  \t%08x\n", q->resid_len);
	printk("extra_len:  \t%08x\n", q->extra_len);
	printk("sense_len:  \t%08x\n", q->sense_len);
	printk("timeout:  \t%08x\n", q->timeout);
}

static int nand_xfer_bio(struct spmp_nand_dev *ndev, struct bio *bio)
{
	int i;
	struct bio_vec *bvec;
	int ret;
	sector_t sector = bio->bi_sector;

	/* ----- Do each segment independently. ----- */
	bio_for_each_segment(bvec, bio, i) 
	{
		char *buffer = __bio_kmap_atomic(bio, i, KM_USER0);

		if(bio_data_dir(bio)==0)
		{
			ret = spmp_nand_transfer(ndev, sector, bio_cur_bytes(bio) >> 9, buffer, 0);
			if(ret)
				return  ret;
		}
		else
		{
			ret = spmp_nand_transfer(ndev, sector, bio_cur_bytes(bio) >> 9, buffer, 1);
			if(ret)
				return  ret;	
		}	
		sector += bio_cur_bytes(bio) >> 9;
		__bio_kunmap_atomic(bio, KM_USER0);
	}
	return 0; /* Always "succeed" */
}
static int spmp_nand_make_request(struct request_queue *q, struct bio *bio)
{
	struct spmp_nand_dev *tr= (struct spmp_nand_dev*)q->queuedata;
	int status;
	status = nand_xfer_bio(tr, bio);
	bio_endio(bio, status);
	return 0;
}


static void spmp_nand_simple_request(struct request_queue *rq)
{
	struct spmp_nand_dev *tr = rq->queuedata;
	wake_up_process(tr->thread);	
}


#if 0
/*
 * Transfer a single BIO.
 */
 static inline unsigned int bio_cur_sectors(struct bio *bio)
{
	if (bio->bi_vcnt)
		return bio_iovec(bio)->bv_len >> 9;
	else /* dataless requests such as discard */
		return bio->bi_size >> 9;
}

static int spmp_nand_xfer_bio(struct spmp_nand_dev *dev, struct bio *bio)
{
	int i;
	struct bio_vec *bvec;
	sector_t sector = bio->bi_sector;

	SPMP_DEBUG_PRINT("Step 1 \n");
	/* Do each segment independently. */
	bio_for_each_segment(bvec, bio, i)
	{
		char *buffer = __bio_kmap_atomic(bio, i, KM_USER0);
		spmp_nand_transfer(dev, sector, bio_cur_sectors(bio), buffer, bio_data_dir(bio) == WRITE);
		sector += bio_cur_sectors(bio);
		__bio_kunmap_atomic(bio, KM_USER0);
	}
	return 0; /* Always "succeed" */
}

/*
 * Transfer a full request.
 */
static int spmp_nand_xfer_request(struct spmp_nand_dev *dev, struct request *req)
{
	struct req_iterator iter;
	int nsect = 0;
	struct bio_vec *bvec;

	SPMP_DEBUG_PRINT("Step 1 \n");
	/* Macro rq_for_each_bio is gone.
	 * In most cases one should use rq_for_each_segment.
	 */
	if (dev == NULL || req == NULL)
	{
		SPMP_DEBUG_PRINT("para error!\n");
		return -EINVAL;
	}
	rq_for_each_segment(bvec, req, iter)
	{
		char *buffer = __bio_kmap_atomic(iter.bio, iter.i, KM_USER0);
		sector_t sector = iter.bio->bi_sector;
		spmp_nand_transfer(dev, sector, bio_cur_sectors(iter.bio), buffer, bio_data_dir(iter.bio) == WRITE);
		sector += bio_cur_sectors(iter.bio);
		__bio_kunmap_atomic(iter.bio, KM_USER0);
		nsect += iter.bio->bi_size / KERNEL_SECTOR_SIZE;
	}
	return nsect;
}


/*
 * Smarter request function that "handles clustering".
 */
static void spmp_nand_full_request(struct request_queue *q)
{
	struct request *req;
	int sectors_xferred;
	struct spmp_nand_dev *dev = q->queuedata;

	SPMP_DEBUG_PRINT("Step 1 \n");
	if (q == NULL)
	{
		SPMP_DEBUG_PRINT("para error!\n");
		return;
	}
	while ((req = blk_peek_request(q)) != NULL)
	{
		if (!blk_fs_request(req))
		{
			printk("Skip non-fs request\n");
			//end_request(req, 0);
			continue;
		}
		sectors_xferred = spmp_nand_xfer_request(dev, req);
		//end_request(req, 1);
		/* The above includes a call to add_disk_randomness(). */
	}
}

/*
 * The direct make request version.
 */
static int spmp_nand_make_request(struct request_queue *q, struct bio *bio)
{
	struct spmp_nand_dev *dev = q->queuedata;
	int status;

	SPMP_DEBUG_PRINT("Step 1 \n");
	if (q == NULL || bio == NULL)
	{
		SPMP_DEBUG_PRINT("para error!\n");
		return -EINVAL;
	}
	status = spmp_nand_xfer_bio(dev, bio);
	bio_endio(bio, status);
	return 0;
}
#endif
/*
 * Open and close.
 */

static int spmp_nand_open(struct block_device *bdev, fmode_t mode)
{
	struct spmp_nand_dev *dev = NULL;
	struct platform_device* platdata = NULL;
	nf_disk *pnand = NULL;
	
	struct resource *presource;
	unsigned long ret=0;
	
	if (bdev == NULL)
	{
		SPMP_DEBUG_PRINT("para error!\n");
		return -EINVAL;
	}
	dev = bdev->bd_disk->private_data;
	
/*
	if(AP_Read_buffer==NULL)
		AP_Read_buffer = memAlloc(512*AP_DEBUG_BUFFER_SIZE);
	if(AP_Write_buffer==NULL)
		AP_Write_buffer = memAlloc(512*AP_DEBUG_BUFFER_SIZE);
*/
#if (NF_TEST_DEBUG_ENABLE==1)
	if(dev->g_read_data==NULL)
		dev->g_read_data = memAlloc(512*AP_DEBUG_BUFFER_SIZE);
	if(dev->g_ecc_data==NULL)
		dev->g_ecc_data = memAlloc(512*AP_DEBUG_BUFFER_SIZE);
#endif
/*
	if(AP_Read_buffer==NULL || AP_Write_buffer==NULL)
	{
		SPMP_DEBUG_PRINT("AP_Read_buffer=%x, AP_Write_buffer=%x\n",(unsigned int)AP_Read_buffer, (unsigned int)AP_Write_buffer);
		return -ENOMEM;
	}
*/
#if (NF_TEST_DEBUG_ENABLE==1)
	if(dev->g_read_data==NULL || dev->g_ecc_data==NULL)
	{
		SPMP_DEBUG_PRINT("g_read_data=%x, g_ecc_data=%x\n", (unsigned int)dev->g_read_data, (unsigned int)dev->g_ecc_data);
		return -ENOMEM;
	}
#endif
	
	platdata = dev->platdata;
	presource = platform_get_resource(platdata, IORESOURCE_MEM, 0);
	if (!presource)
	{
		SPMP_DEBUG_PRINT("No I/O memory resource defined\n");
		return -EIO;
	}
	nf_addr_remap.bch_physAddr = 0x9300A000;
	nf_addr_remap.nf_physAddr = presource->start;
	
	dev->base = ioremap(nf_addr_remap.nf_physAddr, resource_size(presource));
	if (!dev->base)
	{
		SPMP_DEBUG_PRINT("Unable to map spmp  I/O memory\n");
		return -EIO;
	}

	dev->base_ecc= ioremap(nf_addr_remap.bch_physAddr, resource_size(presource));
	if (!dev->base_ecc)
	{
		SPMP_DEBUG_PRINT("Unable to map spmp ecc I/O memory\n");
		return -EIO;
	}

	del_timer_sync(&dev->timer);
	if (!dev->users)
		check_disk_change(bdev);

	dev->nandCacheFlag=0;
	pnand = (nf_disk *) (platdata->dev.platform_data);
	
	nf_addr_remap.bch_virtAddr= (unsigned long)dev->base_ecc;
	nf_addr_remap.nf_virtAddr= (unsigned long)dev->base;

	SPMP_DEBUG_PRINT("NF MapAddr:   Phys %08x ==> VirtAddr %08x\n", (unsigned int)nf_addr_remap.nf_physAddr, (unsigned int)nf_addr_remap.nf_virtAddr);
	SPMP_DEBUG_PRINT("BCH MapAddr:  Phys %08x ==> VirtAddr %08x\n", (unsigned int)nf_addr_remap.bch_physAddr, (unsigned int)nf_addr_remap.bch_virtAddr);
	
	if ( (ret = pnand->func->detect(&(dev->NandChip))) == FAIL)
	{
		SPMP_DEBUG_PRINT("Open Device Failed!\n");
		return -ENODEV;
	}
	debug_flag = 0x400;
	dev->users++;
	
	dev->spmp_nand_open_flag  = 1;

	SPMP_DEBUG_PRINT("NAND_ChipInfo:  block_count=%u page_per_block=%u pagesize=%u sector_per_page=%u\n", 
		dev->NandChip.block_count , dev->NandChip.page_per_block, dev->NandChip.pagesize, dev->NandChip.sector_per_page);
	
	
	dev->size = dev->NandChip.block_count <<pnand->devinfo->nrBitsSectorPerBlk;
	set_capacity(dev->gd,  dev->size);
	SPMP_DEBUG_PRINT("dev->size=%x  block_count=%u nrBitsSectorPerBlk=%u\n", dev->size, 
		dev->NandChip.block_count, pnand->devinfo->nrBitsSectorPerBlk);
#if 0
	//test
	debug_flag = 0x301;
	nf_test_read_logic_sector(0,512,pnand->devinfo);

	pnand->func->read(0, 1, AP_Read_buffer);
	if(AP_Read_buffer[510]!=0x55 || AP_Read_buffer[511]!=0xaa)
	{
		//nandFDisk(pnand->devinfo);
		nand_set_invalid();
		//nf_test_read_logic_sector(0,512,pnand->devinfo);
		//return -ENODEV;
	}

	debug_flag = 0;
#endif
	return 0;
}

static int spmp_nand_release(struct gendisk *disk, fmode_t mode)
{
	struct spmp_nand_dev *dev = NULL;

	if (disk == NULL)
	{
		SPMP_DEBUG_PRINT("para error!\n");
		return -EINVAL;
	}
	dev = disk->private_data;
	spin_lock(&dev->lock);
	dev->users--;

	if (!dev->users)
	{
		dev->timer.expires = jiffies + INVALIDATE_DELAY;
		add_timer(&dev->timer);
	}
	spin_unlock(&dev->lock);
	if(dev->spmp_nand_open_flag)
	{
		//SPMP_DEBUG_PRINT("Flush NAND Cache Data\n");
		NFTL_CacheFlush_ex();
	}
	if(dev->users == 0)
	{
		/*
		if(AP_Read_buffer!=NULL)
		{
			memFree(AP_Read_buffer);
			AP_Read_buffer = NULL;
		}
		if(AP_Write_buffer!=NULL)
		{
			memFree(AP_Write_buffer);
			AP_Write_buffer=NULL;
		}
		*/
	#if (NF_TEST_DEBUG_ENABLE==1)
		if(dev->g_read_data!=NULL)
		{
			memFree(dev->g_read_data );
			dev->g_read_data =NULL;
		}
		if(dev->g_ecc_data!=NULL)
		{
			memFree(dev->g_ecc_data);
			dev->g_ecc_data=NULL;
		}
	#endif
	}
	return 0;
}

/*
 * Look for a (simulated) media change.
 */
int spmp_nand_media_changed(struct gendisk *gd)
{
	struct spmp_nand_dev *dev = gd->private_data;

	return dev->media_change;
}

/*
 * Revalidate.  WE DO NOT TAKE THE LOCK HERE, for fear of deadlocking
 * with open.  That needs to be reevaluated.
 */
int spmp_nand_revalidate(struct gendisk *gd)
{
	struct spmp_nand_dev *dev = gd->private_data;

	if (gd == NULL)
	{
		SPMP_DEBUG_PRINT("para error!\n");
		return -EINVAL;
	}
	if (dev->media_change)
	{
		dev->media_change = 0;
		memset(dev->data, 0, dev->size);
	}
	return 0;
}

/*
 * The "invalidate" function runs out of the device timer; it sets
 * a flag to simulate the removal of the media.
 */
void spmp_nand_invalidate(unsigned long ldev)
{
	struct spmp_nand_dev *dev = (struct spmp_nand_dev *) ldev;

	spin_lock(&dev->lock);
	if (dev->users || !dev->data)
	{
		if(dev->spmp_nand_open_flag == 1)
		{
			//SPMP_DEBUG_PRINT("Flush Nand Cache Data\n");
			NFTL_CacheFlush_ex();
		}
	}
	else
		dev->media_change = 1;
	spin_unlock(&dev->lock);
}


static int spmp_nand_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	struct spmp_nand_dev *dev = bdev->bd_disk->private_data;
	nf_disk *pnand = (nf_disk *) (dev->platdata->dev.platform_data);
#if 0
	geo->heads = 16;
	geo->sectors = 63;//pnand->devinfo->nrBlks <<  pnand->devinfo->nrBitsSectorPerBlk;
	geo->cylinders = ((pnand->devinfo->nrPhyBlks+16)<<  pnand->devinfo->nrBitsSectorPerBlk)/(geo->heads * geo->sectors );
	
	geo->start = pnand->devinfo->firstBlk << pnand->devinfo->nrBitsSectorPerBlk;
#else
	geo->heads = 16;
	geo->sectors = 63;//pnand->devinfo->nrBlks <<  pnand->devinfo->nrBitsSectorPerBlk;
	geo->cylinders = ((pnand->devinfo->nrPhyBlks+16)<<  pnand->devinfo->nrBitsSectorPerBlk)/(geo->heads * geo->sectors );
	
	geo->start = pnand->devinfo->firstBlk << pnand->devinfo->nrBitsSectorPerBlk;
#endif 
	
	SPMP_DEBUG_PRINT("Nand GeoInfo: \n");
	printk("nrBlks=%d\n", pnand->devinfo->nrPhyBlks);
	printk("firstBlk=%d\n", pnand->devinfo->firstBlk);
	printk("nrBitsSectorPerBlk=%d\n", pnand->devinfo->nrBitsSectorPerBlk);
	printk("geo.sectors=%d\n", geo->sectors);
	printk("geo.start=%d\n", (int)geo->start);
	printk("geo.heads=%d\n", geo->heads);
	printk("geo.cylinders=%d\n", geo->cylinders);
	return 0;
}

#define SUPPORT_NF_LINUX_ISP
#ifdef SUPPORT_NF_LINUX_ISP
int isp_nf_read_1k(unsigned long* ptrPyldData, unsigned long* ptrEccData, unsigned int pageNo)
{
	ReadWritePage_ex(pageNo, ptrPyldData, ptrEccData, NF_READ);

	if(BCHProcess((unsigned char *)ptrPyldData, (unsigned char*)ptrEccData, SIZE_1K, BCH_DECODE, 1) == -1)
	{
		return 0;
	}

	return 1;
}

int isp_nf_write_1k(unsigned long* ptrPyldData, unsigned long* ptrEccData,unsigned int pageNo)
{
	BCHProcess((unsigned char *)ptrPyldData, (unsigned char*)ptrEccData, SIZE_1K, BCH_ENCODE, 1);
	ReadWritePage_ex(pageNo,  ptrPyldData, ptrEccData, NF_WRITE);
	
	return 1;
}
int isp_nf_page_read(struct spmp_nand_dev *dev,unsigned long* ptrPyldData, unsigned long* ptrEccData,unsigned int pageNo)
{

	unsigned int count = 0;

	while (count < 3)
	{
		ReadWritePage_ex(pageNo,  ptrPyldData, ptrEccData, NF_READ);
		if(BCHProcess((unsigned char*)ptrPyldData, (unsigned char*)ptrEccData, dev->NandChip.pagesize, BCH_DECODE, 0) == -1)
		{
			// something wrong
			SPMP_DEBUG_PRINT("Error at read page %u\n", pageNo);
			count++;
		}
		else 
		{
			break;
		}
	}	

	if (count >= 3)
	{
		SPMP_DEBUG_PRINT("xxxReadFlashPage(0x%x, %u) is error\n", (unsigned int)ptrPyldData, pageNo);
		return 0;		// there is something wrong
	}	
	return 1;
}

int isp_nf_page_write(struct spmp_nand_dev *dev,unsigned long* ptrPyldData, unsigned long* ptrEccData, unsigned int pageNo)
{
	SPMP_DEBUG_PRINT("run, pageNo=%d\n", pageNo);

	BCHProcess((unsigned char *)ptrPyldData,  (unsigned char *)ptrEccData, dev->NandChip.pagesize, BCH_ENCODE, 0);
	ReadWritePage_ex(pageNo, ptrPyldData,  ptrEccData, NF_WRITE);	
	return 1;
}
int isp_nf_block_erase(unsigned short blk)
{
	SPMP_DEBUG_PRINT("run, blk=%d\n", blk);
	EraseBlock_ex(blk);
	return 1;
}
int isp_nf_block_getinfo(struct spmp_nand_dev *dev, gp_nand_blk_info_t *nf_info)
{
	psysinfo_t* psysinfo = NULL;
	psysinfo = initstorage();

	nf_info->page_size = psysinfo->u16PyldLen;
	nf_info->block_size = (nf_info->page_size) * (psysinfo->u16PageNoPerBlk);

	return 0;
}
int isp_nf_phys_partition_open(struct spmp_nand_dev *dev, unsigned long arg, unsigned char open_flag)
{
	psysinfo_t* psysinfo = NULL;
	unsigned long TotalBlkSize = 0, startblk = 0; 
	unsigned long StartPage = 0, i=0, bytesPerBlockShift = 0, block_mask;

	if(dev->nf_partition.size!=0)//only open one partition in same tine
	{
		SPMP_DEBUG_PRINT("partition(%x, %x) is not close\n", dev->nf_partition.offset, dev->nf_partition.size);
		return -EPERM;
	}
	if(dev->AP_Read_buffer==NULL)
		dev->AP_Read_buffer = memAlloc(512*AP_DEBUG_BUFFER_SIZE);
	if(dev->AP_Write_buffer==NULL)
		dev->AP_Write_buffer = memAlloc(512*AP_DEBUG_BUFFER_SIZE);
	if(dev->AP_Read_buffer==NULL || dev->AP_Write_buffer==NULL)
	{
		SPMP_DEBUG_PRINT("AP_Read_buffer=%x, AP_Write_buffer=%x\n",
			(unsigned int)dev->AP_Read_buffer, (unsigned int)dev->AP_Write_buffer);
		return -ENOMEM;
	}
	psysinfo = initstorage();
	if (copy_from_user(&(dev->nf_partition), (void __user *) arg, sizeof(dev->nf_partition)))
	{
		return -EFAULT;
	}
	StartPage = dev->nf_partition.offset>> valshift(psysinfo->u16PyldLen);
	startblk = StartPage >> psysinfo->u8PagePerBlkShift;
	bytesPerBlockShift = psysinfo->u8PagePerBlkShift + valshift(psysinfo->u16PyldLen);
	block_mask = (1<<bytesPerBlockShift) - 1 ;
	TotalBlkSize = (dev->nf_partition.size + block_mask) >> bytesPerBlockShift;

	SPMP_DEBUG_PRINT("offset=%x, size=%x TotalBlkSize=%lu block_mask=%lx startblk=%lu bytesPerBlockShift=%lu\n", 
		dev->nf_partition.offset, dev->nf_partition.size, TotalBlkSize, block_mask, startblk, bytesPerBlockShift);
	if(open_flag == 1)
	{
		for(i = 0; i< TotalBlkSize ; i++)
		{
			EraseBlock_ex(startblk + i);
		}
	}
	dev->nf_ioctl_write_count = 0;
	dev->nf_ioctl_read_count = 0;

	return 0;
}

int isp_nf_phys_partition_close(struct spmp_nand_dev * dev, unsigned long arg)
{
	SPMP_DEBUG_PRINT("Close Partition: offset=%x, size=%x\n", dev->nf_partition.offset, dev->nf_partition.size);
	
	memset((unsigned char *)&(dev->nf_partition), 0 , sizeof(dev->nf_partition));
	dev->nf_ioctl_write_count = 0;
	dev->nf_ioctl_read_count = 0;

	if(dev->AP_Read_buffer!=NULL)
	{
		memFree(dev->AP_Read_buffer);
		dev->AP_Read_buffer = NULL;
	}
	if(dev->AP_Write_buffer!=NULL)
	{
		memFree(dev->AP_Write_buffer);
		dev->AP_Write_buffer=NULL;
	}

	return 0;
}

int isp_nf_phys_partition_write(struct spmp_nand_dev * dev, unsigned int cmd, unsigned long arg)
{
	gp_nand_write_buffer_t UserData;
	unsigned long ret = 0, PageNo = 0;
	psysinfo_t* psysinfo = NULL;

	psysinfo = initstorage();
	if(dev->nf_partition.size ==0 )
	{
		return -EFAULT;
	}
	if (copy_from_user(&UserData, (void __user *) arg, sizeof(UserData)))
	{
		return -EFAULT;
	}
	if( (dev->nf_ioctl_write_count + UserData.size) > dev->nf_partition.size)
	{
		SPMP_DEBUG_PRINT("nf_partition.size=%x, nf_ioctl_write_count=%x , UserData.size=%x\n", 
			(unsigned int)dev->nf_partition.size, (unsigned int)dev->nf_ioctl_write_count , (unsigned int)UserData.size);
		return -EFAULT;
	}
	if (copy_from_user(dev->AP_Write_buffer, (void __user *) UserData.buf, UserData.size))
	{
		return -EFAULT;
	}
	
	PageNo = (dev->nf_ioctl_write_count + dev->nf_partition.offset) >> valshift(psysinfo->u16PyldLen);
	if(cmd == IOCTL_NAND_ISP_WRITE_1K)
		ret = isp_nf_write_1k((unsigned long *)dev->AP_Write_buffer,  (unsigned long *)dev->AP_Read_buffer, PageNo);
	else
		ret = WritePage_storage(PageNo, (unsigned long *)dev->AP_Write_buffer, (unsigned long *)dev->AP_Read_buffer); 
	dev->nf_ioctl_write_count += UserData.size;

	return 0;
}

int isp_nf_phys_partition_read(struct spmp_nand_dev * dev, unsigned int cmd, unsigned long arg)
{
	gp_nand_write_buffer_t UserData;
	unsigned long ret = 0, PageNo = 0;
	psysinfo_t* psysinfo = NULL;

	psysinfo = initstorage();
	if(dev->nf_partition.size ==0 )
	{
		return -EFAULT;
	}
	if (copy_from_user(&UserData, (void __user *) arg, sizeof(UserData)))
	{
		return -EFAULT;
	}
	if( (dev->nf_ioctl_read_count + UserData.size) > dev->nf_partition.size)
	{
		SPMP_DEBUG_PRINT("nf_partition.size=%x, nf_ioctl_read_count=%x , UserData.size=%x\n", 
			(unsigned int)dev->nf_partition.size, (unsigned int)dev->nf_ioctl_read_count , (unsigned int)UserData.size);
		return -EFAULT;
	}
	PageNo = (dev->nf_ioctl_read_count + dev->nf_partition.offset) >> valshift(psysinfo->u16PyldLen);
	if(cmd == IOCTL_NAND_ISP_READ_1K)
		ret = isp_nf_read_1k((unsigned long *)dev->AP_Read_buffer,  (unsigned long *)dev->AP_Write_buffer, PageNo);
	else
		ret = ReadPage_storage(PageNo, (unsigned long *)dev->AP_Read_buffer, (unsigned long *)dev->AP_Write_buffer);

	if (copy_to_user((void __user *) UserData.buf, dev->AP_Read_buffer, UserData.size))
	{
		return -EFAULT;
	}
	dev->nf_ioctl_read_count += UserData.size;

	return 0;
}
#endif//SUPPORT_NF_LINUX_ISP

/*
 * The ioctl() implementation
 */
int spmp_nand_ioctl(struct block_device *bdev, fmode_t mode, unsigned int cmd, unsigned long arg)
{
	//	long size;
	struct hd_geometry geo;
	struct spmp_nand_dev *dev = NULL;
	nf_disk *pnand = NULL;

	int ret = 0;

	if (bdev == NULL)
	{
		SPMP_DEBUG_PRINT("para error!\n");
		return -EINVAL;
	}
	dev = bdev->bd_disk->private_data;
	pnand = (nf_disk *) (dev->platdata->dev.platform_data);
	switch (cmd)
	{
		case HDIO_GETGEO:
			spmp_nand_getgeo(bdev, &geo);
			if (copy_to_user((void __user *) arg, &geo, sizeof(geo)))
				ret = -EFAULT;
			break;
		case NF_IOCTL_FLUASH:
			NFTL_CacheFlush_ex();
			break;
#if (NF_TEST_DEBUG_ENABLE==1)
		case NF_IOCTL_TEST:
			nf_test_func(arg, dev);
			break;
#endif
		case NF_IOCTL_EREASEALL:
			break;
		case IOCTL_NAND_GET_BLK_INFO://获取物理分区信息
		{
			gp_nand_blk_info_t nf_info;

			ret = isp_nf_block_getinfo(dev, &nf_info);
			if(ret!=0)
				break;
			if (copy_to_user((void __user *) arg, &nf_info, sizeof(nf_info)))
				ret = -EFAULT;
		}
			break;
		case IOCTL_NAND_PHY_OPEN://need erase partition
		{
			ret = isp_nf_phys_partition_open(dev, arg, 1);
		}
			break;
		case IOCTL_NAND_ISP_OPEN://just open, don't erease partition
			{
				ret = isp_nf_phys_partition_open(dev, arg, 0);
			}
			break;
		case IOCTL_NAND_PHY_CLOSE:
			ret = isp_nf_phys_partition_close(dev, arg);
			break;
		case IOCTL_NAND_ISP_WRITE_1K:
		case IOCTL_NAND_ISP_WRITE_PAGE:
		case IOCTL_NAND_PHY_WRITE:
		{
			ret = isp_nf_phys_partition_write(dev, cmd, arg);
		}
			break;
		case IOCTL_NAND_ISP_READ_1K:
		case IOCTL_NAND_ISP_READ_PAGE:
		case IOCTL_NAND_PHY_READ:
		{
			ret = isp_nf_phys_partition_read(dev,  cmd, arg);
		}
			break;
		/*
		case IOCTL_NAND_ISP_READ_1K:
		{
			ret = isp_nf_read_1k(dev->AP_Read_buffer,  dev->AP_Write_buffer,  arg);
		}
			break;
		case IOCTL_NAND_ISP_WRITE_1K:
		{
			ret = isp_nf_write_1k(dev->AP_Write_buffer,  dev->AP_Write_buffer, arg);
		}
			break;
		
		case IOCTL_NAND_ISP_READ_PAGE:
		{
			ret = isp_nf_page_read(dev,  dev->AP_Read_buffer,  dev->AP_Write_buffer, arg);
		}
			break;
		case IOCTL_NAND_ISP_WRITE_PAGE:
		{
			ret = isp_nf_page_write(dev,  dev->AP_Write_buffer,  dev->AP_Write_buffer, arg);
		}
			break;	
		*/
		case NF_IOCTL_DBG_MSG://change debug option
			debug_flag = arg;
			break;
		default:/* unknown command */
			ret = -ENOTTY;
			break;
	}

	return ret; 
}
static void nand_end_request(struct request *req, int uptodate)
{
	int error = 0;
	unsigned int nr_sectors = blk_rq_cur_sectors(req);

	//if (uptodate <= 0)
	//	error = uptodate ? uptodate : -EIO;

	__blk_end_request(req, error, nr_sectors << 9);
	//SPMP_DEBUG_PRINT("uptodate=%d\n", uptodate);

	//blk_end_request(req, 0, nr_sectors << 9);
}
void nf_DumpRequest(struct request *req)
{
	SPMP_DEBUG_PRINT("Dump Request Structer:\n");

	printk("atomic_flags:%lu\n",req->atomic_flags);
/* modify by mm.li 01-12,2011 clean warning */
	/*
	printk("req->bio->bi_cnt %d\n", req->bio->bi_cnt);
	*/
	printk("req->bio->bi_cnt.counter %d\n", req->bio->bi_cnt.counter);
/* modify end */	
	printk("req->bio->bi_comp_cpu %u\n", req->bio->bi_comp_cpu);
	printk("req->bio->bi_flags %lu\n", req->bio->bi_flags);
	printk("req->bio->bi_idx %u\n", req->bio->bi_idx);
	printk("req->bio->bi_phys_segments %u\n", req->bio->bi_phys_segments);
	printk("req->bio->bi_rw %lx\n", req->bio->bi_rw);
	printk("req->bio->bi_sector %u\n", (unsigned int)req->bio->bi_sector);
	printk("req->bio->bi_seg_back_size %u\n", req->bio->bi_seg_back_size);
	printk("req->bio->bi_seg_front_size %u\n", req->bio->bi_seg_front_size);
	printk("req->bio->bi_size %u\n", req->bio->bi_size);
	printk("req->bio->bi_vcnt %u\n", req->bio->bi_vcnt);

	printk("req->cmd: %s\n", req->cmd);
	printk("req->cmd_flags: %u\n", req->cmd_flags);
	printk("req->cmd_len: %u\n", req->cmd_len);
	printk("req->cmd_type: %u\n", req->cmd_type);
	printk("req->cpu: %u\n", req->cpu);
	printk("req->deadline: %lu\n", req->deadline);
	printk("req->errors: %u\n", req->errors);
	printk("req->extra_len: %u\n", req->extra_len);
	printk("req->ioprio: %u\n", req->ioprio);
	printk("req->nr_phys_segments: %u\n", req->nr_phys_segments);
	printk("req->ref_count: %u\n", req->ref_count);
	printk("req->resid_len: %u\n", req->resid_len);
	printk("req->retries: %u\n", req->retries);
	printk("req->sense_len: %u\n", req->sense_len);
	printk("req->start_time: %lu\n", req->start_time);
	printk("req->tag: %u\n", req->tag);
	printk("req->timeout: %u\n", req->timeout);
	printk("req->__data_len: %u\n", req->__data_len);
	printk("req->__sector: %u\n", (unsigned int)(req->__sector));
}

static int spmp_nand_thread(void *arg)
{
	struct request *req;
	int res = 0;
	struct spmp_nand_dev *tr = arg;
	struct request_queue *rq = tr->queue;

	current->flags |= PF_MEMALLOC;
	spin_lock_irq(rq->queue_lock);
	while (!kthread_should_stop()) {
		req = blk_fetch_request(rq);
		if (!req) {
			set_current_state(TASK_INTERRUPTIBLE);
			spin_unlock_irq(rq->queue_lock);
			schedule();
			spin_lock_irq(rq->queue_lock);
			continue;
		}

		spin_unlock_irq(rq->queue_lock);

		do{
			res = spmp_nand_transfer(tr, blk_rq_pos(req), blk_rq_cur_sectors(req), req->buffer, rq_data_dir(req));
			if(res==0)
				break;
		}while(blk_update_request(req, 0, blk_rq_cur_bytes(req) ));
		spin_lock_irq(rq->queue_lock);
		nand_end_request(req, res);
	}
	spin_unlock_irq(rq->queue_lock);

	return 0;
}



/*
 * The device operations structure.
 */
static struct block_device_operations spmp_nand_ops = { 
	.owner = THIS_MODULE, 
	.open = spmp_nand_open,
	.release = spmp_nand_release, 
	.getgeo = spmp_nand_getgeo, 
	.media_changed = spmp_nand_media_changed,
	.revalidate_disk = spmp_nand_revalidate, 
	.ioctl = spmp_nand_ioctl 
};

static struct platform_device *spmp_nf_device[] = { 
	&spmp_nand0_device,
#if (SUPPORT_NF_DISK1==1)
	&spmp_nand1_device
#endif
};

nf_disk_func* get_nf_disk_op(int disk_num)
{
	nf_disk *nfd = (nf_disk*)(spmp_nf_device[disk_num]->dev.platform_data);
	return nfd->func;
}
/*
 * Set up our internal device.
 */
static void setup_nand(struct spmp_nand_dev *dev, int which)
{
	/*
	 * Get some memory.
	 */
	nf_disk *pnand = NULL;
	if (dev == NULL)
	{
		SPMP_DEBUG_PRINT("para error!\n");
		return;
	}
	SPMP_DEBUG_PRINT("dev Addr = %x\n", (unsigned int)dev);
	memset(dev, 0, sizeof(struct spmp_nand_dev));
	
	dev->spmp_nand_open_flag = 0;
	/*
	 * The timer which "invalidates" the device.
	 */
	init_timer(&dev->timer);
	dev->timer.data = (unsigned long) dev;
	dev->timer.function = spmp_nand_invalidate;
	dev->platdata = spmp_nf_device[which];
	dev->platdata->id = which;

	pnand = (nf_disk *) (dev->platdata->dev.platform_data);
	//dev->size = pnand->devinfo->nrBlks << pnand->devinfo->nrBitsSectorPerBlk;
	SPMP_DEBUG_PRINT("nrBlks = %d nrBitsSectorPerBlk=%d\n", pnand->devinfo->nrBlks , pnand->devinfo->nrBitsSectorPerBlk);
	dev->size = nsectors*512;

	spin_lock_init(&dev->lock);	
	/*
	 * The I/O queue, depending on whether we are using our own
	 * make_request function or not.
	 */
	switch (request_mode)
	{
		case RM_NOQUEUE:
			
			dev->queue = blk_alloc_queue(GFP_KERNEL);
			if (dev->queue == NULL)
			{
				SPMP_DEBUG_PRINT("NO MEMORY: queue\n");	
				goto out_vfree;
			}
			blk_queue_make_request(dev->queue, spmp_nand_make_request);
			break;
#if 0
//this codes need to repair
		case RM_FULL:
			dev->queue = blk_init_queue(spmp_nand_full_request, &dev->lock);
			if (dev->queue == NULL)
				goto out_vfree;
			break;
#endif
		default:
			printk("Bad request mode %d, using simple\n", request_mode);

		case RM_SIMPLE:
		{

			dev->queue = blk_init_queue(spmp_nand_simple_request, &dev->lock);
			if (dev->queue == NULL)
				goto out_vfree;

			break;
		}
	}
	//blk_queue_hardsect_size(dev->queue, hardsect_size);
	blk_queue_logical_block_size(dev->queue, 512);
	dev->queue->queuedata = dev;
	/*
	 * And the gendisk structure.
	 */
	dev->gd = alloc_disk(16);
	if (!dev->gd)
	{
		SPMP_DEBUG_PRINT("alloc_disk failure\n");
		goto out_vfree;
	}
	dev->gd->major = spmp_nand_major;
	dev->gd->minors= 16;
	dev->gd->first_minor = which * dev->gd->minors +1;
	
	dev->gd->fops = &spmp_nand_ops;
	dev->gd->queue = dev->queue;
	dev->gd->private_data = dev;
	snprintf(dev->gd->disk_name, 32, NAND_DEVICE_NAME "%c", which + 'a');

	if(request_mode == RM_SIMPLE)
	{
		dev->thread = kthread_run(spmp_nand_thread, dev, "%s", dev->gd->disk_name);
		if (IS_ERR(dev->thread)) 
		{
			SPMP_DEBUG_PRINT("Init thread failed \n");
			return ;
		}
	}

	
	set_capacity(dev->gd,  nsectors);
	SPMP_DEBUG_PRINT("dev size = %d\n", dev->size);
	add_disk(dev->gd);

	return;

out_vfree:
	if (dev->data)
		vfree(dev->data);
}

int spmp_nand_init(void)
{
	int i;
//	int ret = 0;

	/*
	 * Get registered.
	 */
	spmp_nand_major = register_blkdev(spmp_nand_major, NAND_DEVICE_NAME);
	SPMP_DEBUG_PRINT("spmp_nand_major = %d\n", spmp_nand_major);
	if (spmp_nand_major < 0)
	{
		SPMP_DEBUG_PRINT("unable to get major number\n");
		return -EBUSY;
	}
	/*
	 * Allocate the device array, and initialize each one.
	 */
	ndevices = ARRAY_SIZE(spmp_nf_device);
	Devices = kmalloc(ndevices * sizeof(struct spmp_nand_dev), GFP_KERNEL);
	if (Devices == NULL)
		goto out_unregister;

	for (i = 0; i < ndevices; i++)
		setup_nand(Devices + i, i);

	debug_flag = 0;
	return 0;

out_unregister:
	unregister_blkdev(spmp_nand_major, NAND_DEVICE_NAME);
	return -ENOMEM;
}
extern UINT32 nandCacheDisable(void);
void spmp_nand_exit(void)
{
	int i;

	NFTL_Close();
	SPMP_DEBUG_PRINT("Remove Modules\n");
	for (i = 0; i < ndevices; i++)
	{
		struct spmp_nand_dev *dev = Devices + i;

		del_timer_sync(&dev->timer);
		if (dev->gd)
		{
			del_gendisk(dev->gd); 
			put_disk(dev->gd);
		}
		if (dev->queue)
		{
			if (request_mode == RM_NOQUEUE)
				kobject_put(&dev->queue->kobj);
			/* blk_put_queue() is no longer an exported symbol */
			else
				blk_cleanup_queue(dev->queue);
		}
		if (dev->data)
			vfree(dev->data);
	}
	unregister_blkdev(spmp_nand_major, NAND_DEVICE_NAME);
	kfree(Devices);
}

EXPORT_SYMBOL(spmp_nand_init);
EXPORT_SYMBOL(spmp_nand_exit);
//module_init(spmp_nand_init);
//module_exit(spmp_nand_exit);
