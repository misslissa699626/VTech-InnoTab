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
#include <mach/gp_cache.h>


#if 0
#define DEBUG0 DIAG_INFO
#else
#define DEBUG0(...)
#endif


#define DEVICE_NAME "cache"



typedef struct gp_cache_s {
	struct miscdevice dev;     				
	spinlock_t lock;
} gp_cache_t;


static gp_cache_t* gp_cache_info = NULL;

/**
 * @brief   Cache clean function
 * @param   start [in]: start address
 * @param   size [in]: size of range
 * @return  None
 */
void gp_clean_dcache_range(unsigned int start,unsigned int size) 
{
	unsigned long oldIrq;
	unsigned int start_line;
	unsigned int end_line;
	unsigned int val = 0;
	
	start_line = start & ~0x1F;
	end_line = (start + size - 1) & ~0x1F;
	
	local_irq_save(oldIrq);
	asm volatile ( 
    	"mcrr p15,0,%1,%0,c12;"  
        "mcr  p15,0,%2,c7,c10,4;"               
        :                                                               
        : "r" (start_line), "r" (end_line), "r" (val)                                
    );
    local_irq_restore(oldIrq);
}
EXPORT_SYMBOL(gp_clean_dcache_range);

/**
 * @brief   Cache invalidate function
 * @param   start [in]: start address
 * @param   size [in]: size of range
 * @return  None
 */
void gp_invalidate_dcache_range(unsigned int start,unsigned int size) 
{ 
	unsigned long oldIrq;
	unsigned int start_line;
	unsigned int end_line;
	unsigned int val = 0;
	
	start_line = start & ~0x1F;
	end_line = (start + size - 1) & ~0x1F;
	
	local_irq_save(oldIrq);
	if ((start & 0x1F) || (size < 32)) {
		asm volatile (
			"mcrr p15,0,%0,%0,c12;" 
			"mcr  p15,0,%1,c7,c10,4;"
			:
			: "r" (start_line), "r" (val)
		);
	}
		
	if (((start+size) & 0x1F) && (start_line != end_line)) {
		asm volatile (
			"mcrr p15,0,%0,%0,c12;" 
			"mcr  p15,0,%1,c7,c10,4;"
			:
			: "r" (end_line), "r" (val)
		);
	}	
	
	asm volatile ( 
    	"mcrr p15,0,%1,%0,c6;" 
        :                                                               
        : "r" (start_line), "r" (end_line)                                                                                               
    ); 
    local_irq_restore(oldIrq);             
} 
EXPORT_SYMBOL(gp_invalidate_dcache_range);

static long gp_cache_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	unsigned long oldIrq;
	struct gp_cache_address_s cache_address;
	
	spin_lock(&gp_cache_info->lock);
	
	switch ( cmd ) {
	case DCACHE_FLUSH:
		DEBUG0("DCACHE FLUSH\n");
		local_irq_save(oldIrq);
		flush_cache_all();
		local_irq_restore(oldIrq);
		break;
	#if 0
	case TLB_FLUSH:
		DEBUG0("TLB FLUSH\n");
		local_irq_save(oldIrq);
		flush_tlb_all();
		local_irq_restore(oldIrq);
		break;
	#endif
	case DCACHE_CLEAN_RANGE:
		if (copy_from_user(&cache_address, (void __user*)arg, sizeof(cache_address))) {
			ret = -EFAULT;
			break;
		}
		gp_clean_dcache_range(cache_address.start,cache_address.size);
		break;
	case DCACHE_INVALIDATE_RANGE:
		if (copy_from_user(&cache_address, (void __user*)arg, sizeof(cache_address))) {
			ret = -EFAULT;
			break;
		}
		gp_invalidate_dcache_range(cache_address.start,cache_address.size);
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

