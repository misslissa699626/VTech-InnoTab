/*
 * Sample disk driver, from the beginning.
 */


#include <linux/kernel.h>       /* printk() */
#include <linux/slab.h>         /* kmalloc() */
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/hdreg.h>        /* HDIO_GETGEO */
#include <linux/kdev_t.h>
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/bio.h>
#include "hal_nand.h"


MODULE_LICENSE("Dual BSD/GPL");

void nand_sema_init(void);
SINT32 nf0_init_intr(void);
void nf0_uninit_intr(void);
SINT32 bch_init_intr(void);
void bch_uninit_intr(void);

static int hal_nand_init(void)
{
	printk("gp_nand_hal insmod !! V 0.1.2 \n");
	nand_sema_init();
	
	nf0_init_intr();
	bch_init_intr();
	
	return 0;
}

static void hal_nand_exit(void)
{
	printk("gp_nand_hal rmmod !!\n");
	bch_uninit_intr();
	nf0_uninit_intr();
}

static struct semaphore nand_sem;
void nand_sema_init(void)
{
	init_MUTEX(&nand_sem); 
}

int nand_sema_lock(void)
{
	return down_interruptible(&nand_sem);
	//down(&nand_sem);
}

void nand_sema_unlock(void)
{
	up(&nand_sem);
}

EXPORT_SYMBOL(nand_sema_init);
EXPORT_SYMBOL(nand_sema_lock);
EXPORT_SYMBOL(nand_sema_unlock);

EXPORT_SYMBOL(Nand_Init);
EXPORT_SYMBOL(Nand_UnInit);
EXPORT_SYMBOL(Nand_Getinfo);
EXPORT_SYMBOL(Nand_ErasePhyBlock);
EXPORT_SYMBOL(Nand_ReadPhyPage);
EXPORT_SYMBOL(Nand_WritePhyPage);
EXPORT_SYMBOL(nand_write_status_get);

EXPORT_SYMBOL(spare_flag_set_L);
EXPORT_SYMBOL(spare_flag_set_H);
EXPORT_SYMBOL(spare_flag_get_L);
EXPORT_SYMBOL(spare_flag_get_H);

EXPORT_SYMBOL(DrvNand_WP_Initial);
EXPORT_SYMBOL(DrvNand_WP_Enable);
EXPORT_SYMBOL(DrvNand_WP_Disable);


EXPORT_SYMBOL(NandParseBootHeader);
EXPORT_SYMBOL(GetAppStartBlkFromBth);


EXPORT_SYMBOL(GetAppSizeOfBlkFromBth);
EXPORT_SYMBOL(GetAppPercentFromBth);
EXPORT_SYMBOL(GetDataBankLogicSizeFromBth);
EXPORT_SYMBOL(GetDataBankRecycleSizeFromBth);
EXPORT_SYMBOL(GetNoFSAreaSectorSizeFromBth);
EXPORT_SYMBOL(GetPartNumFromBth);
EXPORT_SYMBOL(GetPartInfoFromBth);


EXPORT_SYMBOL(nand_bch_err_bits_get);
EXPORT_SYMBOL(bch_mode_set);
EXPORT_SYMBOL(bch_mode_get);
EXPORT_SYMBOL(get_bch_mode);
EXPORT_SYMBOL(nand_bch_get);
EXPORT_SYMBOL(Nand_Chip_Switch);

EXPORT_SYMBOL(nandAdjustDmaTiming);
EXPORT_SYMBOL(good_block_check);
EXPORT_SYMBOL(Nand_sw_bad_block_set);

EXPORT_SYMBOL(dump_buffer);
EXPORT_SYMBOL(nand_get_spare_buf);

module_init(hal_nand_init);
module_exit(hal_nand_exit);
