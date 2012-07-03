//------------------------------------------------------------
// Copyright (C), 2010, VTECH Co., Ltd.
// File name:     innotab_nand_debug.c
// Author:        VTE/PST/Kevin Su
// Description: innotab_nand_debug
//   
// Remark:  
// Function List:
// History: 
//------------------------------------------------------------


/*#include <linux/init.h>
#include <linux/slab.h>
#include <asm/errno.h>
#include <asm/delay.h>
#include <linux/mm.h>
#include <linux/poll.h>
#include <linux/module.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <linux/cdev.h>
#include <asm/arch/irqs.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <asm/arch/regs-clock.h>
#include <linux/device.h>*/

#include <media/v4l2-dev.h>
#include <media/v4l2-subdev.h>

#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/diag.h>
//#include <mach/gp_csi.h>
//#include <mach/gp_gpio.h>
//#include <mach/hal/hal_csi.h>
//#include <mach/gp_chunkmem.h>
//#include <mach/sensor_mgr.h>


#include "nand_innotab.h"


/***********************************************************************
 * Debug only
************************************************************************/
typedef struct
{
	int dev_no;
	loff_t start_offset;
	size_t len;
	int page;
	int numpage;
//	size_t *retlen;
	u_char *buf;
	u_char *oobbuf;
	struct innotab_nand_chip_param *param;
}st_nand_param_t;

#define ST_NAND_READ_ECC			_IOR('n', 1, st_nand_param_t)
#define ST_NAND_READ_OOB			_IOR('n', 2, st_nand_param_t)
#define ST_NAND_READ_RAW_PAGE	_IOR('n', 3, st_nand_param_t)
#define ST_NAND_WRITE_PAGE		_IOW('n', 4, st_nand_param_t)
#define ST_NAND_WRITE_OOB			_IOW('n', 5, st_nand_param_t)
#define ST_NAND_ERASE_BLOCK		_IOW('n', 6, st_nand_param_t)
#define ST_NAND_MARK_BD_OOB		_IOW('n', 7, st_nand_param_t)
#define ST_NAND_CHECK_BD_OOB	_IOR('n', 8, st_nand_param_t)
#define ST_NAND_ERASE_ALL			_IOW('n', 9, st_nand_param_t)
#define ST_NAND_INIT					_IOWR('n', 10, st_nand_param_t)
#define ST_NAND_PAGE_DIRTY		_IOR('n', 13, st_nand_param_t)
#define ST_NAND_NAND_CHECK_WP	_IOR('n', 14, st_nand_param_t)
#define ST_NAND_NAND_CLOSE	_IOR('n', 15, st_nand_param_t)




////////////////////////////////////
//global variable definition
///////////////////////////////////
#define NAND_DATA_BUF_SIZE	(1024*4)
#define NAND_OOB_BUF_SIZE		(32)

static int open_cnt = 0;
static int nand_debug_major = 0, nand_debug_minor = 0;
static struct cdev nand_debug_dev;
static char *pdata_buf=NULL, *poob_buf=NULL;


int nand_debug_open(struct inode *inode, struct file *filp)
{
	if(open_cnt > 0)
	{
		goto exit;
	}
	printk("[NAND_DEBUG_DRV] nand_debug_open().\n");
	
	pdata_buf = (char *)kmalloc(NAND_DATA_BUF_SIZE, GFP_DMA);
	if(!pdata_buf)
	{
		printk("[NAND_DEBUG_DRV] LINE(%d) kmalloc failed!\n", __LINE__);
		return -ENOMEM;
	}
	poob_buf = (char *)kmalloc(NAND_OOB_BUF_SIZE, GFP_KERNEL);
	if(!poob_buf)
	{
		printk("[NAND_DEBUG_DRV] LINE(%d) kmalloc failed!\n", __LINE__);
		kfree(pdata_buf);
		pdata_buf = NULL;
		return -ENOMEM;
	}

	memset(pdata_buf, 0, NAND_DATA_BUF_SIZE);
	memset(poob_buf, 0, NAND_OOB_BUF_SIZE);

exit:
  open_cnt++;
  return 0;
}


int nand_debug_close(struct inode *inode, struct file *filp)
{
	open_cnt --;
	printk("[NAND_DEBUG_DRV] nand_debug_close()\n");
	if(pdata_buf)
	{
		kfree(pdata_buf);
		pdata_buf = NULL;
	}
	if(poob_buf)
	{
		kfree(poob_buf);
		poob_buf = NULL;
	}

	return 0;
}


static int nand_debug_ioctl(struct inode *inodep, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret, retlen = 0;
	st_nand_param_t param;
	struct innotab_nand_chip_param mtd_param;

	ret = copy_from_user(&param, (st_nand_param_t *)arg, sizeof(st_nand_param_t));
	if(ret)
	{
		printk("[NAND_DEBUG_DRV] LINE(%d) copy_from_user() failed!\n", __LINE__);
		return -EINVAL;
	}
	
	switch(cmd)
	{
		case ST_NAND_READ_ECC:
			memset(pdata_buf, 0, NAND_DATA_BUF_SIZE);
			memset(poob_buf, 0, NAND_OOB_BUF_SIZE);
			ret = innotab_nand_read_ecc(param.dev_no, param.start_offset, param.len, &retlen, pdata_buf, poob_buf);
			if(ret)
			{
				printk("[NAND_DEBUG_DRV] innotab_nand_read_ecc() failed!\n");
				return ret;
			}
			else
			{
				printk("[NAND_DEBUG_DRV] retlen: %d\n", retlen);
				ret = copy_to_user(param.buf, pdata_buf, param.len);
				if(ret)
				{
					printk("[NAND_DEBUG_DRV] LINE(%d) copy_to_user() failed!\n", __LINE__);
					return -EINVAL;
				}
				ret = copy_to_user(param.oobbuf, poob_buf, NAND_OOB_BUF_SIZE);
				if(ret)
				{
					printk("[NAND_DEBUG_DRV] LINE(%d) copy_to_user() failed!\n", __LINE__);
					return -EINVAL;
				}
			}
			
			break;

		case ST_NAND_READ_OOB:
			printk("NOT IMPLEMENT!\n");
			return -EINVAL;
			
			memset(poob_buf, 0, NAND_OOB_BUF_SIZE);
			//ret = innotab_nand_read_oob( param.dev_no, param.page, param.start_offset, param.len, &retlen, poob_buf );
			if(ret)
			{
				printk("[NAND_DEBUG_DRV] innotab_nand_read_oob() failed!\n");
				return ret;
			}
			else
			{
				printk("[NAND_DEBUG_DRV] retlen: %d\n", retlen);
				ret = copy_to_user(param.oobbuf, poob_buf, param.len);
				if(ret)
				{
					printk("[NAND_DEBUG_DRV] LINE(%d) copy_to_user() failed!\n", __LINE__);
					return -EINVAL;
				}				
			}
			break;

		case ST_NAND_READ_RAW_PAGE:
			printk("NOT IMPLEMENT!\n");
			return -EINVAL;
			
			memset(pdata_buf, 0, NAND_DATA_BUF_SIZE);
			//ret = innotab_nand_read_raw_page( param.dev_no, param.page, param.numpage, pdata_buf );
			if(ret)
			{
				printk("[NAND_DEBUG_DRV] innotab_nand_read_raw_page() failed!\n");
				return ret;
			}
			else
			{
				ret = copy_to_user(param.buf, pdata_buf, NAND_DATA_BUF_SIZE);
				if(ret)
				{
					printk("[NAND_DEBUG_DRV] LINE(%d) copy_to_user() failed!\n", __LINE__);
					return -EINVAL;
				}
			}
			break;
			
		case ST_NAND_WRITE_PAGE:
			ret = copy_from_user(pdata_buf, param.buf, NAND_DATA_BUF_SIZE);
			if(ret)
			{
				printk("[NAND_DEBUG_DRV] LINE(%d) copy_from_user() failed!\n", __LINE__);
				return -EINVAL;
			}
			ret = copy_from_user(poob_buf, param.oobbuf, NAND_OOB_BUF_SIZE);
			if(ret)
			{
				printk("[NAND_DEBUG_DRV] LINE(%d) copy_from_user() failed!\n", __LINE__);
				return -EINVAL;
			}
			ret = innotab_nand_write_page( param.dev_no, param.page, pdata_buf, poob_buf );
			if(ret)
			{
				printk("[NAND_DEBUG_DRV] innotab_nand_write_page() failed!\n");
				return ret;
			}
			break;

		case ST_NAND_WRITE_OOB:
			printk("NOT IMPLEMENT!\n");
			return -EINVAL;
			
			ret = copy_from_user(poob_buf, param.oobbuf, NAND_OOB_BUF_SIZE);
			if(ret)
			{
				printk("[NAND_DEBUG_DRV] LINE(%d) copy_from_user() failed!\n", __LINE__);
				return -EINVAL;
			}
			//ret = innotab_nand_write_oob( param.dev_no, param.page, param.start_offset, param.len, &retlen, poob_buf);
			if(ret)
			{
				printk("[NAND_DEBUG_DRV] innotab_nand_write_oob() failed!\n");
				return ret;
			}
			break;

		case ST_NAND_ERASE_BLOCK:
			ret = innotab_nand_erase_block( param.dev_no, param.page );
			if(ret)
			{
				printk("[NAND_DEBUG_DRV] innotab_nand_erase_block() failed!\n");
				return ret;
			}
			break;
			
		case ST_NAND_MARK_BD_OOB:
			ret = innotab_nand_mark_bd_oob( param.dev_no, param.page );
			if(ret)
			{
				printk("[NAND_DEBUG_DRV] innotab_nand_mark_bd_oob() failed!\n");
				return ret;
			}
			break;

		case ST_NAND_CHECK_BD_OOB:
			ret = innotab_nand_check_bd_oob( param.dev_no, param.page );
			break;
			
		case ST_NAND_ERASE_ALL:
			ret = innotab_nand_erase_all( param.dev_no );
			if(ret)
			{
				printk("[NAND_DEBUG_DRV] innotab_nand_erase_all() failed!\n");
				return ret;
			}
			break;
			
		case ST_NAND_INIT:
			memset(&mtd_param, 0, sizeof(mtd_param));
			ret = innotab_nand_init( param.dev_no, &mtd_param);
			if(ret)
			{
				printk("[NAND_DEBUG_DRV] innotab_nand_init() failed!\n");
				return ret;
			}
			ret = copy_to_user(param.param, &mtd_param, sizeof(mtd_param));
			if(ret)
			{
				printk("[NAND_DEBUG_DRV] LINE(%d) copy_to_user() failed!\n", __LINE__);
				return -EINVAL;
			}
			break;

		case ST_NAND_PAGE_DIRTY:
			printk("NOT IMPLEMENT!\n");
			return -EINVAL;
			//ret = innotab_nand_page_dirty( param.dev_no, param.page );
			break;
			
		case ST_NAND_NAND_CHECK_WP:
			printk("NOT IMPLEMENT!\n");
			return -EINVAL;
			//ret = innotab_nand_check_wp( param.dev_no );
			break;

		case ST_NAND_NAND_CLOSE:
			innotab_nand_close( param.dev_no );
			ret = 0;
			break;

		default:
			printk("[NAND_DEBUG_DRV] unsupport operation!(cmd: %d)\n",cmd);
			return -ENOTTY;
	}
	printk("[NAND_DEBUG_DRV] ret = %d\n",ret);
	return ret;
}


static ssize_t nand_debug_read(struct file *filp, char __user *buf, size_t size, loff_t *fpos)
{
	printk("[NAND_DEBUG_DRV] nand_debug_read().\n");
	return -EINVAL;
}


static ssize_t nand_debug_write(struct file *filp, const char __user *buf, size_t size, loff_t *fpos)
{
	printk("[NAND_DEBUG_DRV] nand_debug_write().\n");
	return -EINVAL;	
}


static const struct file_operations nand_debug_fops = 
{
	.owner = THIS_MODULE,
	.open = nand_debug_open,
	.release = nand_debug_close,
	.read = nand_debug_read,
	.write = nand_debug_write,
	.ioctl = nand_debug_ioctl
};

struct class *nand_debug_class = NULL;

int __init nand_debug_init(void)
#ifdef MTD_AS_ROOT			// If using MTD as rootfs, the module init and exit are dummy.  The corresponding initialization routines will be run from nand.ko
{
}

void init_for_mtd(void)
#endif
{
	int result, ret;
	struct device *device = NULL;

	dev_t devno = MKDEV(nand_debug_major, nand_debug_minor);

	if(nand_debug_major)
	{
		result = register_chrdev_region(devno, 1, "nand_debug");
	}
	else
	{
		result = alloc_chrdev_region(&devno, 0, 1, "nand_debug");
		nand_debug_major = MAJOR(devno);
	}

	if(result < 0)
	{
		printk(KERN_ERR "[NAND_DEBUG_DRV] LINE(%d) Register device failed!\n", __LINE__);
		goto fail_init;
	}

	/* create class for csi character device */
	nand_debug_class = class_create(THIS_MODULE, "nand_debug");
	if (IS_ERR(nand_debug_class))
	{
		printk(KERN_ERR "nand_debug: can't create class\n");
		ret = -EFAULT;
		goto fail_create_class;
	}

	memset(&nand_debug_dev, 0, sizeof(nand_debug_dev));
	cdev_init(&nand_debug_dev, &nand_debug_fops);
	nand_debug_dev.owner = THIS_MODULE;
	nand_debug_dev.ops = &nand_debug_fops;

	if(cdev_add(&nand_debug_dev, devno, 1))
	{
		printk(KERN_ERR "[NAND_DEBUG_DRV] LINE(%d) Error adding Nand_debug device!\n", __LINE__);
		goto fail_device_register;
	}
	
	/* create character device node */
	devno = MKDEV(nand_debug_major, 0);
	device = device_create( nand_debug_class, NULL, devno, NULL, "nand_debug");
	if(!device){
		printk(KERN_ERR"nand_debug: device_create error\n");
		goto fail_device_create;
	}

	printk(KERN_INFO "[NAND_DEBUG_DRV] Major:%d, Minor:%d\n",MAJOR(devno),MINOR(devno));	
	printk(KERN_INFO "[NAND_DEBUG_DRV] Nand_debug driver has been initialized successfully!\n");

	return 0;
	
fail_device_create:	
	cdev_del(&nand_debug_dev);
fail_device_register:
	class_destroy(nand_debug_class);
fail_create_class:
	unregister_chrdev_region(devno, 1);
fail_init:
	return ret;	
}


void __exit nand_debug_exit(void)
#ifdef MTD_AS_ROOT			// If using MTD as rootfs, the module init and exit are dummy.  The corresponding initialization routines will be run from nand.ko
{
}

void exit_for_mtd(void)
#endif
{
	dev_t devno;
	devno = MKDEV(nand_debug_major, 0);
	device_destroy(nand_debug_class, devno);
	cdev_del(&nand_debug_dev);
	class_destroy(nand_debug_class);
	unregister_chrdev_region(devno,1);
}

#ifdef MTD_AS_ROOT
EXPORT_SYMBOL(init_for_mtd);
EXPORT_SYMBOL(exit_for_mtd);
#endif

module_init(nand_debug_init);
module_exit(nand_debug_exit);

MODULE_AUTHOR("Kevin Su");
MODULE_LICENSE("Dual BSD/GPL");
