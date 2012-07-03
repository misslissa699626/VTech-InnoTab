#include <linux/module.h> 
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/fs.h>

#include <mach/kernel.h>
#include <mach/diag.h>
#include <mach/gp_gpio.h>
#include <mach/gp_chunkmem.h>

#define DEBUG_PRINT	printk
//#define DEBUG_PRINT(...)

#define TRACE_LINE	{DEBUG_PRINT(KERN_WARNING "{TRACE_LINE %s:%d:%s()}\n", __FILE__, __LINE__, __FUNCTION__);}
//#define TRACE_LINE	

#define OOPS	{*(int*)0 = 0;}

#define	DEV_MINOR			0
#define NR_DEVS				1

#define REG_START			0x93000000
#define REG_END				0x94000000
#define REG_LEN				(REG_END - REG_START)

static struct cdev c_dev;
static char *ioremap_ptr = 0;

static int major;
static struct class *vpad_debug_class = 0;

static unsigned int chunkmem_phyaddr_start = 0;
static unsigned int chunkmem_phyaddr_size = 0;
unsigned int vpad_debug_chunkmem_get_phyaddr_start(void);
unsigned int vpad_debug_chunkmem_get_phyaddr_size(void);
static char *ioremap_chunkmem_ptr = 0;




int vpad_debug_open(struct inode *inode, struct file *filp);
int vpad_debug_release(struct inode *inode, struct file *filp);
ssize_t static vpad_debug_read(struct file *filp, char __user *buff, size_t count, loff_t *offp);
ssize_t static vpad_debug_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp);

static struct file_operations vpad_debug_fops = {
	.owner = THIS_MODULE,
	//.poll = vppu_poll,
	.open = vpad_debug_open,
	//.ioctl = vpad_debug_ioctl,
	.release = vpad_debug_release,
	.read = vpad_debug_read,
	.write = vpad_debug_write,
};

int vpad_debug_open(struct inode *inode, struct file *filp)
{
	return 0;
}

int vpad_debug_release(struct inode *inode, struct file *filp)
{
	return 0;
}

void __exit vpad_debug_module_exit(void)
{
	dev_t devno = MKDEV(major, DEV_MINOR);
	cdev_del(&c_dev);
	unregister_chrdev_region(devno, NR_DEVS);
	
	if (ioremap_ptr) {
		iounmap(ioremap_ptr);
	}
	if (ioremap_chunkmem_ptr)
		iounmap(ioremap_chunkmem_ptr);

	
	DEBUG_PRINT(KERN_WARNING "VPAD-DEBUG module exit \n");
}

int __init vpad_debug_module_init(void)
{
	int result;
	dev_t dev;
	int devno;

	ioremap_ptr = 0;
	
	result = alloc_chrdev_region(&dev, DEV_MINOR, 1, "VPAD-DEBUG");
	if (result < 0) {
		DEBUG_PRINT(KERN_WARNING "VPAD-DEBUG: can't get major \n");
		return result;
	}
	major = MAJOR(dev);
	vpad_debug_class = class_create(THIS_MODULE, "VPAD-DEBUG");
	
	devno = MKDEV(major, DEV_MINOR);
	cdev_init(&c_dev, &vpad_debug_fops);
	c_dev.owner = THIS_MODULE;
	c_dev.ops = &vpad_debug_fops;
	result = cdev_add(&c_dev, devno, 1);
	device_create(vpad_debug_class, NULL, devno, NULL, "vpad-debug");
	
	if (result) {
		goto fail;

	} else {
		//ioremap
		ioremap_ptr = (char*)ioremap_nocache(REG_START, REG_LEN);
		if (ioremap_ptr == 0) {
			DEBUG_PRINT(KERN_WARNING "VPAD-DEBUG: failed to ioremap(0x%X, 0x%X)\n", REG_START, REG_LEN);
			goto fail;
		}
		//DEBUG_PRINT(KERN_WARNING "VPPU: ioremap(0x%X, 0x%X) returns 0x%08X\n", VPPU_REG_START, VPPU_REG_LEN, (unsigned int)ppu_devices->ioremap_ptr);

		chunkmem_phyaddr_start = vpad_debug_chunkmem_get_phyaddr_start();
		chunkmem_phyaddr_size = vpad_debug_chunkmem_get_phyaddr_size();
		if (chunkmem_phyaddr_start != 0 && chunkmem_phyaddr_size != 0) {
			ioremap_chunkmem_ptr = (char*)ioremap_nocache(chunkmem_phyaddr_start, chunkmem_phyaddr_size);
			if (ioremap_chunkmem_ptr == 0) {
				DEBUG_PRINT(KERN_WARNING "VPAD-DEBUG: failed to ioremap(0x%X, 0x%X) for chunkmem\n", chunkmem_phyaddr_start, chunkmem_phyaddr_size);
			}
		}
	}

  
	DEBUG_PRINT(KERN_WARNING "VPAD-DEBUG module init\n");
	return 0;

fail:
	DEBUG_PRINT(KERN_WARNING "VPAD-DEBUG module init failed \n");
	unregister_chrdev_region(dev, NR_DEVS);
	return result;
}

static void put_reg(unsigned int offset, unsigned int value)
{
	if (ioremap_chunkmem_ptr) {
		if (offset >= chunkmem_phyaddr_start && offset < chunkmem_phyaddr_start + chunkmem_phyaddr_size) {
			//in chunkmem
			iowrite32(value, ioremap_chunkmem_ptr + (offset - chunkmem_phyaddr_start));
		}
	}
		
	if (ioremap_ptr) {
		if (offset >= REG_START && offset < REG_END) {
			iowrite32(value, ioremap_ptr + (offset - REG_START));
		}
	}
}

static unsigned int get_reg(unsigned int offset)
{
	if (ioremap_chunkmem_ptr) {
		if (offset >= chunkmem_phyaddr_start && offset < chunkmem_phyaddr_start + chunkmem_phyaddr_size) {
			//in chunkmem
			//DEBUG_PRINT(KERN_WARNING "ioremap_chunkmem_ptr: ioread32(0x%08X + (0x%08X - 0x%08X))\n", (unsigned int)ioremap_chunkmem_ptr, offset, chunkmem_phyaddr_start);
			return ioread32(ioremap_chunkmem_ptr + (offset - chunkmem_phyaddr_start));
		}		
	}
	if (ioremap_ptr) {
		if (offset >= REG_START && offset < REG_END) {
			//DEBUG_PRINT(KERN_WARNING "ioremap_ptr: ioread32(0x%08X + (0x%08X - 0x%08X))\n", (unsigned int)ioremap_ptr, offset, REG_START);
			return ioread32(ioremap_ptr + (offset - REG_START));
		}
	}
	return 0;
}

ssize_t vpad_debug_read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
	unsigned int start = REG_START;
	unsigned int end = REG_END;
	unsigned int offset = (unsigned int)(*offp);
	
	if (ioremap_chunkmem_ptr) {
		if (offset >= chunkmem_phyaddr_start && offset < chunkmem_phyaddr_start + chunkmem_phyaddr_size) {
			start = chunkmem_phyaddr_start;
			end = chunkmem_phyaddr_start + chunkmem_phyaddr_size;
		}
	}
	
	if (offset < start || offset >= end) {
		return 0;
	}
	if ((offset & 3) != 0) {
		return 0;
	}
	count &= ~3;	//4-byte per unit to read
	if (offset + count > end) {
		count = end - offset;
	}

	if (count > 0) {
		size_t cnt = 0;
		while (cnt < count) {
			unsigned int n = 0;
			n = get_reg(offset);
			copy_to_user(buff, &n, 4);
			buff += 4;
			offset += 4;
			cnt += 4;
		}
		*offp = (loff_t)offset;
	}
	return count;
}

ssize_t vpad_debug_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp)
{
	unsigned int start = REG_START;
	unsigned int end = REG_END;
	unsigned int offset = (unsigned int)(*offp);
	
	if (ioremap_chunkmem_ptr) {
		if (offset >= chunkmem_phyaddr_start && offset < chunkmem_phyaddr_start + chunkmem_phyaddr_size) {
			start = chunkmem_phyaddr_start;
			end = chunkmem_phyaddr_start + chunkmem_phyaddr_size;
		}
	}
	
	if (offset < start || offset >= end) {
		return 0;
	}
	if ((offset & 3) != 0) {
		return 0;
	}
	count &= ~3;	//4-byte per unit to read
	if (offset + count > end) {
		count = end - offset;
	}

	if (count > 0) {
		size_t cnt = 0;
		while (cnt < count) {
			unsigned int n;
			copy_from_user(&n, buff, 4);
			put_reg(offset, n);		
			buff += 4;
			offset += 4;
			cnt += 4;
		}
		*offp = (loff_t)offset;
	}
	return count;
}


module_init(vpad_debug_module_init);
module_exit(vpad_debug_module_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

MODULE_AUTHOR("VTech");
MODULE_DESCRIPTION("VTech VPAD DEBUG Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

