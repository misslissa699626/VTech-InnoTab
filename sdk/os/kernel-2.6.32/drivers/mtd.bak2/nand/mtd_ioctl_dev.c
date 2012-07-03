#include <linux/module.h> 
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/platform_device.h>

#include <mach/kernel.h>
#include <mach/diag.h>
#include <mach/gp_gpio.h>
#include <mach/gp_chunkmem.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/compatmac.h>

#ifdef CONFIG_MTD_PARTITIONS
#include <linux/mtd/partitions.h>
#endif

#include <../vtech_mtd/nand_innotab.h>
#include "../support.InnoTab.h"
#include "nandchip.innotab.h"
#include "mtd_ioctl_dev.h"

#define DEBUG_PRINT	printk
//#define DEBUG_PRINT(...)

#define TRACE_LINE	{DEBUG_PRINT(KERN_WARNING "{TRACE_LINE %s:%d:%s()}\n", __FILE__, __LINE__, __FUNCTION__);}
//#define TRACE_LINE	

#define OOPS	{*(int*)0 = 0;}

#define	VMTD_MINOR					0
#define VMTD_NR_DEVS				1

typedef struct vmtd_dev_s {
	struct cdev c_dev;
	int ref_cnt;
} vmtd_dev_t;

int vmtd_refcnt = -1;
int vmtd_major;
vmtd_dev_t *vmtd_device = 0;
struct class *vmtd_class = 0;
static DECLARE_MUTEX(vmtd_ioctl_in_use);

int vmtd_fops_open(struct inode *inode, struct file *filp);
int vmtd_fops_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long cmd_arg);
int vmtd_fops_release(struct inode *inode, struct file *filp);

struct file_operations vmtd_fops = {
	.owner = THIS_MODULE,
	.open = vmtd_fops_open,
	.ioctl = vmtd_fops_ioctl,
	.release = vmtd_fops_release,
};

int vmtd_fops_open(struct inode *inode, struct file *filp)
{	
	down_interruptible( &vmtd_ioctl_in_use );
	if ( vmtd_refcnt >= 0 )
		vmtd_refcnt ++;
	else
	{	// For the case when vmtd_refcnt = -1, i.e. the device hase not been initialized properly.
		up( &vmtd_ioctl_in_use );
		return -1;
	}
	up( &vmtd_ioctl_in_use );

	filp->private_data = vmtd_device;
	
	DEBUG_PRINT(KERN_WARNING "%s(): VMTD open \n", __PRETTY_FUNCTION__ );
	return 0;
}

int vmtd_fops_release(struct inode *inode, struct file *filp)
{
	/* Success */
	down_interruptible( &vmtd_ioctl_in_use );
	if ( vmtd_refcnt > 0 )
		vmtd_refcnt --;
	up( &vmtd_ioctl_in_use );

	DEBUG_PRINT(KERN_WARNING "%s(): VMTD release \n", __PRETTY_FUNCTION__ );
	return 0;
}

static void vmtd_device_release(struct device *dev)                       
{       
	DIAG_INFO("remove VMTD device ok\n");                                      
}                                                                           

static struct platform_device vmtd_platform_device;
static struct platform_device vmtd_platform_device_init_value = {                             
	.name	= "vmtd",                                                         
	.id	= 0,                                                                  
	.dev	= {                                                                 
		.release = vmtd_device_release,                                       
	},                                                                        
	.num_resources  = 0,
	.resource       = 0,
};   

static struct platform_driver vmtd_platform_driver;
static struct platform_driver vmtd_platform_driver_init_value = {                                                                             
	.driver	= {                                                               
		.owner	= THIS_MODULE,                                                  
		.name	= "vmtd"                                                        
	}                                                                     
};                                                                          

//void __exit vmtd_module_exit(void)
void vmtd_file_exit(void)
{
	dev_t devno = MKDEV(vmtd_major, VMTD_MINOR);

	down_interruptible( &vmtd_ioctl_in_use );
	if ( vmtd_refcnt == -1 )		// vmtd_refcnt = -1 => previous initialization failure and nothing to exit.
		goto vmtd_file_exit_out;
	vmtd_refcnt = -1;	// No one can open /dev/vmtd or do any ioctl() anymore

	platform_driver_unregister(&vmtd_platform_driver);
	platform_device_unregister(&vmtd_platform_device);
	device_destroy( vmtd_class, devno );
	cdev_del(&(vmtd_device->c_dev));
	kfree(vmtd_device);
	vmtd_device = NULL;
	class_destroy(vmtd_class);
	vmtd_class = NULL;
	unregister_chrdev_region(devno, VMTD_NR_DEVS);

vmtd_file_exit_out:
	up( &vmtd_ioctl_in_use );
	INNOTAB_REACH_HERE

	DEBUG_PRINT(KERN_WARNING "VMTD module exit \n");
}

//int __init vmtd_module_init(void)
int vmtd_file_init(void)
{
	void *tmpPtr;
	int result;
	dev_t dev, devno = 0;
	int chrdev_alloc_ok = 0;
	int cdev_add_ok = 0;
	int device_create_ok = 0;
	int device_register_ok = 0;
	int driver_register_ok = 0;

	down_interruptible( &vmtd_ioctl_in_use );
	if ( vmtd_refcnt != -1 )		
	{
		up( &vmtd_ioctl_in_use );	// Do nothing if device already initailzied.
		return -1;
	}
	up( &vmtd_ioctl_in_use );

	result = alloc_chrdev_region(&dev, VMTD_MINOR, 1, "VMTD");
	if (result < 0) {
		DEBUG_PRINT(KERN_WARNING "VMTD: can't get major \n");
		goto fail;
	}
	else
		chrdev_alloc_ok = 1;

	vmtd_major = MAJOR(dev);
	vmtd_class = class_create(THIS_MODULE, "VMTD");
	if ( IS_ERR(vmtd_class) )
	{
		DEBUG_PRINT(KERN_WARNING "VMTD: class create error \n");
		vmtd_class = NULL;
		result = -1;
		goto fail;
	}
	
	vmtd_device = kmalloc(sizeof(vmtd_dev_t), GFP_KERNEL);
	if (!vmtd_device) {
		DEBUG_PRINT(KERN_WARNING "VMTD: kmalloc failed \n");
		result = -ENOMEM;
		goto fail;
	}
	memset(vmtd_device, 0, sizeof(vmtd_dev_t));
	devno = MKDEV(vmtd_major, VMTD_MINOR);
	cdev_init(&(vmtd_device->c_dev), &vmtd_fops);
	vmtd_device->c_dev.owner = THIS_MODULE;
	vmtd_device->c_dev.ops = &vmtd_fops;
	result = cdev_add(&(vmtd_device->c_dev), devno, 1);
	if ( result < 0 )
	{
		DEBUG_PRINT(KERN_WARNING "VMTD: cdev_add() fail \n");
		result = -1;
		goto fail;
	}
	else
		cdev_add_ok = 1;

	tmpPtr = device_create(vmtd_class, NULL, devno, NULL, "vmtd%d", 0);
	if ( IS_ERR( tmpPtr ) )
	{
		DEBUG_PRINT(KERN_WARNING "VMTD: device_create() fail \n");
		result = -1;
		goto fail;
	}
	else
		device_create_ok = 1;
	
	memcpy( &vmtd_platform_device, &vmtd_platform_device_init_value, sizeof( vmtd_platform_device ) );
	result = platform_device_register(&vmtd_platform_device);
	if ( result )
	{
		DEBUG_PRINT(KERN_WARNING "VMTD: platform_device_register() fail \n");
		goto fail;
	}
	else
		device_register_ok = 1;

	memcpy( &vmtd_platform_driver, &vmtd_platform_driver_init_value, sizeof( vmtd_platform_driver ) );
	result = platform_driver_register(&vmtd_platform_driver);
	if ( result )
	{
		DEBUG_PRINT(KERN_WARNING "VMTD: platform_driver_register() fail \n");
		goto fail;
	}
	else
		driver_register_ok = 1;

	vmtd_refcnt = 0;			// From this point on, allow open() to /dev/vmtd
	DEBUG_PRINT(KERN_WARNING "VMTD module init\n");
	return result;

fail:
	DEBUG_PRINT(KERN_WARNING "VMTD module init failed \n");
	if ( driver_register_ok )
		platform_driver_unregister(&vmtd_platform_driver);
	if ( device_register_ok )
		platform_device_unregister(&vmtd_platform_device);
	if ( device_create_ok )
		device_destroy( vmtd_class, devno );
	if ( cdev_add_ok )
		cdev_del(&(vmtd_device->c_dev));
	if ( vmtd_device )
	{
		kfree(vmtd_device);
		vmtd_device = NULL;
	}
	if ( vmtd_class )
	{
		class_destroy(vmtd_class);
		vmtd_class = NULL;
	}
	if ( chrdev_alloc_ok )
		unregister_chrdev_region(dev, VMTD_NR_DEVS);

	return result;
}

int vmtd_fops_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long cmd_arg)
{
	int ret = 0;
	unsigned int mtd_options = 0;
	int phyChipNo;
	int mtdret = 0;
	struct cmd_add_mtd *mtd_cmd = NULL;

	down_interruptible( &vmtd_ioctl_in_use );
	if ( vmtd_refcnt == -1 )
	{
		ret = -ENOIOCTLCMD;
		goto vmtd_ioctl_out;
	}
	mtd_cmd = kzalloc( sizeof( struct cmd_add_mtd ) + MAX_NUM_PART * MAX_LEN_PART_NAME, GFP_KERNEL);
	if ( mtd_cmd == NULL )
	{
		ret = -ENOIOCTLCMD;
		goto vmtd_ioctl_out;
	}
	copy_from_user( mtd_cmd, (const char __user *) cmd_arg, sizeof( struct cmd_add_mtd ) );
	phyChipNo = mtd_cmd->phyChipNo;
	if ( VMTD_IOC_ADD_MTD == cmd )
	{
		if ( mtd_cmd->param )
			mtd_options = MTD_ALLOW_ERASE_BB;
	}

	switch (cmd) {
	case VMTD_IOC_ADD_MTD:
		{
			int i;
			char *ptr;

			ptr = ((char *)mtd_cmd) + sizeof( struct cmd_add_mtd );
			for ( i = 0; i < MAX_NUM_PART; i ++ )
			{
				if ( mtd_cmd->config.part[i].name && strncpy_from_user ( ptr, (const char __user *) mtd_cmd->config.part[i].name, MAX_LEN_PART_NAME ) < 0 )
				{
					ret = -ENOIOCTLCMD;
					break;
				}
				mtd_cmd->config.part[i].name = ptr;
				ptr += MAX_LEN_PART_NAME;
			}
		}
		if ( !ret )
			mtdret = innotab_nand_chip_add_mtd( phyChipNo, &mtd_cmd->config, mtd_options, 1 );
		break;
	case VMTD_IOC_DEL_MTD:
		mtdret = innotab_nand_chip_del_mtd( phyChipNo, 1 );
		break;
	case VMTD_IOC_ADD_DEV:
		mtdret = innotab_nand_chip_init( phyChipNo, 1 );
		break;
	case VMTD_IOC_DEL_DEV:
		mtdret = innotab_nand_chip_release( phyChipNo, 1 );
		break;
	case VMTD_IOC_MARK_BAD:
		printk( "mtd_cmd->param = %d\n", mtd_cmd->param );
		mtdret = innotab_nand_chip_mark_bad( phyChipNo, mtd_cmd->param, 1 );
		break;
	default:
		ret = -ENOIOCTLCMD;
		break;                      
	}
	if ( mtdret )
		ret = -ENOIOCTLCMD;

vmtd_ioctl_out:
	up ( &vmtd_ioctl_in_use );
	kfree( mtd_cmd );
	return ret;
}

//module_init(vmtd_module_init);
//module_exit(vmtd_module_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

//MODULE_AUTHOR("VTech");
//MODULE_DESCRIPTION("VTech VMTD Driver");
//MODULE_LICENSE("GPL");
//MODULE_VERSION("1.0");
