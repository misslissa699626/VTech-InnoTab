/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Inc. reserves the right to modify this software           *
 *  without notice.                                                       *
 *                                                                        *
 *  Generalplus Inc.                                                      *
 *  3F, No.8, Dusing Rd., Hsinchu Science Park,                           *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
/**
 * @file    gp_cache.c
 * @brief   Implement of cache flush driver
 * @author  Daolong Li
 */
#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/cdev.h>
#include <mach/diag.h>


#if 0
#define DEBUG0 DIAG_INFO
#else
#define DEBUG0(...)
#endif


#define DEVICE_NAME "cache"

/* Ioctl for device node definition */
#define CACHE_IOCTL_MAGIC	'C'
#define DCACHE_FLUSH	_IOWR(CACHE_IOCTL_MAGIC, 1, unsigned int)
#define TLB_FLUSH	_IOW(CACHE_IOCTL_MAGIC,  2, unsigned int)


typedef struct gp_cache_s {
	struct miscdevice dev;     				
	spinlock_t lock;
} gp_cache_t;


 static gp_cache_t* gp_cache_info = NULL;
 
static long gp_cache_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	unsigned long oldIrq;
	
	spin_lock(&gp_cache_info->lock);
	
	switch ( cmd ) {
	case DCACHE_FLUSH:
		DEBUG0("DCACHE FLUSH\n");
		local_irq_save(oldIrq);
		flush_cache_all();
		local_irq_restore(oldIrq);
		break;
	case TLB_FLUSH:
		DEBUG0("TLB FLUSH\n");
		local_irq_save(oldIrq);
		flush_tlb_all();
		local_irq_restore(oldIrq);
		break;
	default:	
		DEBUG0("Unknow io ctrl\n");
		ret = -ENOIOCTLCMD;	
		break;
	}
	
	spin_unlock(&gp_cache_info->lock);
	return ret;
}

static int gp_cache_open(struct inode *inode, struct file *file)
{
	DEBUG0("%s:Enter!\n", __FUNCTION__);
	return 0;
}

static int gp_cache_release(struct inode *inode, struct file *file)
{
	DEBUG0("%s:Enter!\n", __FUNCTION__);
	return 0;
}

static struct file_operations gp_cache_fops = {
	.owner          = THIS_MODULE,
	.open           = gp_cache_open,
	.release        = gp_cache_release,
	.unlocked_ioctl = gp_cache_ioctl,
};


static int __init init_gp_cache(void)
{	
	int ret = 0;
	
	gp_cache_info = kzalloc(sizeof(gp_cache_t),GFP_KERNEL);
	if ( NULL == gp_cache_info ) {
		return -ENOMEM;
	}	

	spin_lock_init(&gp_cache_info->lock);

	gp_cache_info->dev.name = DEVICE_NAME;
	gp_cache_info->dev.minor = MISC_DYNAMIC_MINOR;
	gp_cache_info->dev.fops  = &gp_cache_fops;
	
	
	ret = misc_register(&gp_cache_info->dev);
	if ( ret != 0 ) {
		DIAG_ERROR(KERN_ALERT "misc register fail\n");
		kfree(gp_cache_info);
		return ret;
	}
	
	return 0;
}

static void __exit free_gp_cache(void)
{	
	misc_deregister(&gp_cache_info->dev);
	kfree(gp_cache_info);
	gp_cache_info = NULL;
}

module_init(init_gp_cache);
module_exit(free_gp_cache);

MODULE_LICENSE_GP;
MODULE_AUTHOR("GeneralPlus");
MODULE_DESCRIPTION("gp cache driver!");

