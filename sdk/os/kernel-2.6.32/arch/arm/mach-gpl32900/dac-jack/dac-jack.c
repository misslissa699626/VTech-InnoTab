#include <linux/module.h> 
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/platform_device.h>
//#include <linux/timer.h>
//#include <linux/kthread.h>

#include <mach/kernel.h>
#include <mach/diag.h>
//#include <mach/gp_gpio.h>
#include <mach/gp_chunkmem.h>
#include <mach/spu.h>

#include <mach/dac-jack.h>

//#define DEBUG_PRINT	printk
#define DEBUG_PRINT(...)

#define ERROR_PRINT	printk
//#define ERROR_PRINT(...)

//#define TRACE_LINE	{DEBUG_PRINT(KERN_WARNING "{TRACE_LINE %s:%d:%s()}\n", __FILE__, __LINE__, __FUNCTION__);}
#define TRACE_LINE	

#define OOPS	{*(int*)0 = 0;}

#define	DACJACK_MINOR				0
#define NR_DEVS				1

#define DAC_REG_START	0x93000000
#define DAC_REG_END		0x93020000
#define DAC_REG_LEN		(DAC_REG_END - DAC_REG_START)

#define P_DAC_HDPHN		0x9301F038

static int master_vol = 0;		//initial mute
module_param(master_vol, int, S_IRUGO);

static int output_mode = 0;
module_param(output_mode, int, S_IRUGO);

static int dac_major;
static struct class *dac_class;
static struct cdev c_dev;
static char *ioremap_ptr = 0;


int dac_fops_open(struct inode *inode, struct file *filp);
int dac_fops_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long cmd_arg);
int dac_fops_release(struct inode *inode, struct file *filp);

struct file_operations dac_fops = {
	.owner = THIS_MODULE,
	.open = dac_fops_open,
	.ioctl = dac_fops_ioctl,
	.release = dac_fops_release,
};


static void SetVolume(int vol)
{
	unsigned int leftvol = 31 - vol;
	unsigned int rightvol = output_mode? 31 - vol: 31;
	unsigned int value = 3 | (rightvol << 2) | (leftvol << 7);
	
	iowrite32(value, ioremap_ptr + (P_DAC_HDPHN - DAC_REG_START));

	if (output_mode == DACJACK_OUTPUTMODE_SPEAKER) {
		printk(KERN_WARNING "[SPEAKER %d]\n", vol);
	} else {
		printk(KERN_WARNING "[HEADPHONE %d]\n", vol);
	}
}

int _init_ioremap(void)
{
	if (ioremap_ptr == 0) {
		//ioremap
		ioremap_ptr = (char*)ioremap_nocache(DAC_REG_START, DAC_REG_LEN);
		if (ioremap_ptr == 0) {
			ERROR_PRINT(KERN_WARNING "dac-jack: ioremap failed!\n");
			return -1;
		}
		//DEBUG_PRINT(KERN_WARNING "dac-jack: ioremap(0x%08X, 0x%08X) returns 0x%08X\n", DAC_REG_START, DAC_REG_LEN, (unsigned int)ioremap_request_ptr);
	}
	return 0;
}

int _term_ioremap(void)
{
	if (ioremap_ptr) {
		//iounmap
		iounmap(ioremap_ptr);
		ioremap_ptr = 0;
	}
	return 0;
}

int dac_fops_open(struct inode *inode, struct file *filp)
{
	filp->private_data = 0;
	
	DEBUG_PRINT(KERN_WARNING "dac-jack open \n");
	return 0;
}

int dac_fops_release(struct inode *inode, struct file *filp)
{
	/* Success */
	DEBUG_PRINT(KERN_WARNING "dac-jack release \n");
	return 0;
}

static int init_DAC(void)
{
	//halI2sTxFIFOClear();
	{
		int i;
		for (i = 0; i < 32; i++)
			iowrite32(0, ioremap_ptr + (0x93012004 - DAC_REG_START));
	}

	{
		char *p;
		
		//iowrite32(0xFFFFFFFF, ioremap_ptr + (0x93007004 - DAC_REG_START));
		unsigned int n = ioread32(ioremap_ptr + (0x93007004 - DAC_REG_START));
		n |= (1 << 17) | (1 << 18) | (1 << 25);
		iowrite32(n, ioremap_ptr + (0x93007004 - DAC_REG_START));

		p = ioremap_ptr + (0x930070E0 - DAC_REG_START);
		iowrite32(0x02000081, p);

		iowrite32(0x0500012d, ioremap_ptr + (0x93007044 - DAC_REG_START));
		iowrite32(0x0010c425, ioremap_ptr + (0x93012000 - DAC_REG_START)); // jackson: move to DAC driver

		p = ioremap_ptr + (0x93007040 - DAC_REG_START);
		
		iowrite32(0xc03, ioremap_ptr + (0x9301F02c - DAC_REG_START)); // jackson: move to DAC driver
		iowrite32(0x7db, ioremap_ptr + (0x9301F020 - DAC_REG_START)); // jackson: move to DAC driver
	}

	SetVolume(master_vol);

	return 0;
}

void __exit dac_module_exit(void)
{
	{
		dev_t devno = MKDEV(dac_major, DACJACK_MINOR);
		cdev_del(&c_dev);
		unregister_chrdev_region(devno, NR_DEVS);
	}
	DEBUG_PRINT(KERN_WARNING "dac-jack module exit \n");
}

int __init dac_module_init(void)
{
	int result = 0;
	dev_t dev;
	int devno;

	DEBUG_PRINT(KERN_WARNING "dac-jack module init\n");

	result = alloc_chrdev_region(&dev, DACJACK_MINOR, 1, "dac-jack");
	if (result < 0) {
		ERROR_PRINT(KERN_WARNING "dac-jack: can't get major \n");
		goto fail;
	}
	dac_major = MAJOR(dev);
	dac_class = class_create(THIS_MODULE, "dac-jack");
	
	devno = MKDEV(dac_major, DACJACK_MINOR);
	cdev_init(&c_dev, &dac_fops);
	c_dev.owner = THIS_MODULE;
	c_dev.ops = &dac_fops;
	result = cdev_add(&c_dev, devno, 1);
	device_create(dac_class, NULL, devno, NULL, "dac-jack%d", 0);
	
	//check master_vol range, because it is a module parameter
	if (master_vol < 0) master_vol = 0;
	if (master_vol >= 31) master_vol = 31;

	if (_init_ioremap() < 0) {
		_term_ioremap();
		ERROR_PRINT(KERN_WARNING "dac-jack: failed in _init_ioremap()\n");
		result = -1;
		goto fail;
	}
	
	init_DAC();

	DEBUG_PRINT(KERN_WARNING "dac-jack module init succeeded\n");
	return result;

fail:
	ERROR_PRINT(KERN_WARNING "dac-jack module init failed \n");
	unregister_chrdev_region(dev, NR_DEVS);
	return result;
}

int dac_fops_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long cmd_arg)
{
	int ret = 0;
	
	switch (cmd) {
	case DACJACK_IOC_GETVOL:
		ret = master_vol;

	case DACJACK_IOC_SETVOL:
		{
			master_vol = (unsigned int)cmd_arg;
			if (master_vol > 31) master_vol = 31;
			SetVolume(master_vol);
		}
		break;
	case DACJACK_IOC_SET_OUTPUTMODE:
		output_mode = cmd_arg? DACJACK_OUTPUTMODE_HEADPHONE: DACJACK_OUTPUTMODE_SPEAKER;
		SetVolume(master_vol);
		break;
		
	case DACJACK_IOC_GET_OUTPUTMODE:
		ret = output_mode;
		break;
		
	default:
		ret = -ENOTTY; /* Inappropriate ioctl for device */
		break;
	}
	return ret;
}

module_init(dac_module_init);
module_exit(dac_module_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

MODULE_AUTHOR("VTech");
MODULE_DESCRIPTION("VTech DAC Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

