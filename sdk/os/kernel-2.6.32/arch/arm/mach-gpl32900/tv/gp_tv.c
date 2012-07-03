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
 * @file    gp_tv.c
 * @brief   Implement of tv module driver.
 * @author  Cater Chen
 * @since   2010-11-18
 * @date    2010-11-18
 */
 
#include <linux/module.h> 
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <mach/gp_chunkmem.h>
#include <mach/hal/hal_tv.h>
#include <mach/gp_tv.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
**************************************************************************/
#define	TV_MINOR		0
#define TV_NR_DEVS	1
#define VIC_TV1     20
/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#if 1
	#define DEBUG	printk
#else
	#define DEBUG(...)
#endif
 
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

 /**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
int gp_tv_open(struct inode *inode, struct file *filp);
int gp_tv_release(struct inode *inode, struct file *filp);
int gp_tv_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
//static irqreturn_t gp_tv1_irq_handler(int irq, void *dev_id);
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
struct file_operations tv_fops = {
	.owner = THIS_MODULE,
	.open = gp_tv_open,
	.ioctl = gp_tv_ioctl,
	.release = gp_tv_release,
};

typedef struct gp_tv_dev_s {
	struct cdev c_dev;
	wait_queue_head_t tv1_wait_queue;
	bool done;	
} gp_tv_dev_t;

int tv_major;
static gp_tv_dev_t *tv_devices=NULL;
struct class *tv_class;  
 /**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
/**
* @brief	       gp tv1 initial
* @param 	none
* @return 	SUCCESS/ERROR_ID.
*/
signed int gp_tv1_init(void)
{
	    int nRet=0;
      
      #if GP_TV1_HARDWARE_MODULE == GP_TV1_ENABLE
         gpHalTVinit();
         #if 0
         nRet = request_irq(VIC_TV1,gp_tv1_irq_handler,SA_SHIRQ,"TV1_IRQ",NULL);
         #else
         //nRet = request_irq(VIC_TV1,gp_tv1_irq_handler,IRQF_SHARED,"TV1_IRQ",NULL);
         #endif
         return nRet; 
      #else
         return -1; 
      #endif   
}
EXPORT_SYMBOL(gp_tv1_init);

/**
* @brief	       gp tv1 set display buffer
* @param 	buffer_ptr[in]:TV display buffer set.
* @return 	SUCCESS/ERROR_ID.
*/
signed int gp_tv1_set_buffer(unsigned int buffer_ptr)
{
	    static unsigned int temp;
	    
      #if GP_TV1_HARDWARE_MODULE == GP_TV1_ENABLE
         temp = (unsigned int)gp_user_va_to_pa((unsigned short *)buffer_ptr);
         gpHalTvFramebufferset(temp);        
         return 0; 
      #else
         return -1; 
      #endif   
}
EXPORT_SYMBOL(gp_tv1_set_buffer);

/**
* @brief	       gp tv1 set display color
* @param 	buffer_color_mode[in]:TV display buffer color type set.
* @return 	SUCCESS/ERROR_ID.
*/
signed int gp_tv1_set_color(BUFFER_COLOR_FORMAT buffer_color_mode)
{    
      #if GP_TV1_HARDWARE_MODULE == GP_TV1_ENABLE
         gpHalPicDisplaycolor(buffer_color_mode);        
         return 0; 
      #else
         return -1; 
      #endif   
}
EXPORT_SYMBOL(gp_tv1_set_color);

/**
* @brief	  gp tv1 display to reverse
* @param 	  mode[in]:0:disable,1:enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int gp_tv1_set_reverse(unsigned int mode)
{    
      #if GP_TV1_HARDWARE_MODULE == GP_TV1_ENABLE
         gpHalTvFramebufferReverse(mode);        
         return 0; 
      #else
         return -1; 
      #endif   
}
EXPORT_SYMBOL(gp_tv1_set_reverse);

/**
* @brief	       gp tv1 start
* @param 	nTvStd[in]:TV display type set.0:TVSTD_NTSC_M, 1:TVSTD_NTSC_J, 2:TVSTD_NTSC_N, 3:TVSTD_PAL_M, 4:TVSTD_PAL_B, 5:TVSTD_PAL_N, 6:TVSTD_PAL_NC, 7:TVSTD_NTSC_J_NONINTL, 8:TVSTD_PAL_B_NONINTL 
* @param 	nTvStd[in]:TV display resolution set.0:TV_QVGA, 1:TV_HVGA, 2:TV_D1
* @param 	nTvStd[in]:TV display non-interlace set.0:TV_INTERLACE, 1:TV_NON_INTERLACE
* @return 	SUCCESS/ERROR_ID.
*/
signed int gp_tv1_start(signed int nTvStd, signed int nResolution, signed int nNonInterlace)
{
      #if GP_TV1_HARDWARE_MODULE == GP_TV1_ENABLE
         gpHalTVstart(nTvStd, nResolution, nNonInterlace);
         return 0; 
      #else
         return -1; 
      #endif                
}
EXPORT_SYMBOL(gp_tv1_start);

/**
* @brief	  gp tv1 display buffer state
* @return 	SUCCESS/ERROR_ID.
*/
signed int gp_tv1_buffer_state(void)
{
	    signed int temp;
	    
      #if GP_TV1_HARDWARE_MODULE == GP_TV1_ENABLE
         temp = gpHalTvFramebufferstate();        
         return temp; 
      #else
         return -1; 
      #endif   
}
EXPORT_SYMBOL(gp_tv1_buffer_state);

/**
* @brief	  gp tv1 ypbpr set
* @return 	SUCCESS/ERROR_ID.
*/
signed int gp_tv1_set_ypbpr(unsigned int mode,unsigned int enable)
{
	    
      #if GP_TV1_HARDWARE_MODULE == GP_TV1_ENABLE
         gpHalYbpbcrenable(mode,enable);        
         return 0; 
      #else
         return -1; 
      #endif   
}
EXPORT_SYMBOL(gp_tv1_set_ypbpr);

/**
 * @brief 	PPU irq function.
* @return 	SUCCESS/ERROR_ID.
*/
/*
static irqreturn_t 
gp_tv1_irq_handler(
	int irq, 
	void *dev_id
)
{
	      signed int ret=-1;
	      
	      ret=gpHalTvIsr();
	      
	      if(ret==TV1_IRQ_START)
	      {
	          tv_devices->done = 1;
		        wake_up_interruptible(&tv_devices->tv1_wait_queue);
	      }
	      else
	        return IRQ_NONE;
	      
	      return IRQ_HANDLED;	
}
*/
/**
 * \brief Open tv device
 */
int
gp_tv_open(
	struct inode *inode,
	struct file *filp
)
{
	/* Success */
	filp->private_data = tv_devices;
	printk(KERN_WARNING "TV1 open \n");

	return 0;
}
int
gp_tv_release(
	struct inode *inode,
	struct file *filp
)
{
	/* Success */
	printk(KERN_WARNING "TV1 release \n");
	
	return 0;
}

int
gp_tv_ioctl(
	struct inode *inode,
	struct file *filp,
	unsigned int cmd,
	unsigned long arg
)
{
  TV1_MOUDLE_STRUCT *tv1_register_set;
  int ret = 0;

  /* initial tv register parameter set structure */
  tv1_register_set = (TV1_MOUDLE_STRUCT *)arg;	
  
  switch(cmd)
  {
  	  case TV_INIT:
  	       ret = gp_tv1_init();
  	       break;  	  
  	  
  	  case TV_ENABLE_START:
  	       ret = gp_tv1_start((signed int)tv1_register_set->TV1_type_mode, (signed int)tv1_register_set->TV1_resolution_mode, 
  	       (signed int)tv1_register_set->TV1_noninterlace_mode);
  	       break;
  	       
  	  case TV_SET_BUFFER:
  	       ret = gp_tv1_set_buffer((unsigned int)tv1_register_set->TV1_display_buffer_ptr);
  	       break;  	       
   	  
   	  case TV_SET_COLOR:
  	       ret = gp_tv1_set_color(tv1_register_set->TV1_buffer_color_mode);
  	       break;

   	  case TV_SET_REVERSE:
  	       ret = gp_tv1_set_reverse(tv1_register_set->TV1_type_mode);
  	       break;  	       

   	  case TV_BUFFER_STATE:
  	       ret = gp_tv1_buffer_state();
  	       break; 
   	  
   	  case TV_SET_YPBPR:
  	       ret = gp_tv1_set_ypbpr(tv1_register_set->TV1_type_mode,tv1_register_set->TV1_resolution_mode);
  	       break;   	         	        	
 		  
 		  default:
			     ret = -ENOIOCTLCMD;
			     break;                      
  }
	
	return ret;  	
}
void __exit
gp_tv_module_exit(
	void
)
{
	dev_t devno = MKDEV(tv_major, TV_MINOR);
	cdev_del(&(tv_devices->c_dev));
	kfree(&tv_devices);
	unregister_chrdev_region(devno, TV_NR_DEVS);
  printk(KERN_WARNING "TV1 module exit \n");
}

/**
 * \brief Initialize display device
 */
int __init
gp_tv_module_init(
	void
)
{
	int result;
	dev_t dev;
	int devno;

	result = alloc_chrdev_region(&dev, TV_MINOR, 1, "TV1");
	if( result<0 )	{
		printk(KERN_WARNING "TV1: can't get major \n");
		return result;
	}
	tv_major = MAJOR(dev);
	tv_class = class_create(THIS_MODULE, "TV1");
	
	tv_devices = kmalloc(sizeof(gp_tv_dev_t), GFP_KERNEL);
	if(!tv_devices) {
		printk(KERN_WARNING "TV1: can't kmalloc \n");
		result = -ENOMEM;
		goto fail;
	}
	memset(tv_devices, 0, sizeof(gp_tv_dev_t));
	
	devno = MKDEV(tv_major, TV_MINOR);
	cdev_init(&(tv_devices->c_dev), &tv_fops);
	tv_devices->c_dev.owner = THIS_MODULE;
	tv_devices->c_dev.ops = &tv_fops;
	result = cdev_add(&(tv_devices->c_dev), devno, 1);
	device_create(tv_class, NULL, devno, NULL, "tv%d", 1);
	
	if(result)
		printk(KERN_WARNING "Error adding tv1 \n");

  #if GP_TV1_HARDWARE_MODULE == GP_TV1_ENABLE
   // Initiate TV1 hardware and driver
   gpHalTVinit();
  #endif
	printk(KERN_WARNING "TV1 module init \n");
		
	return 0;

fail:
	
	printk(KERN_WARNING "TV1 module init failed \n");
	kfree(tv_devices);
	unregister_chrdev_region(dev, TV_NR_DEVS);

	return result;

}

module_init(gp_tv_module_init);
module_exit(gp_tv_module_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("Generalplus TV1 Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
