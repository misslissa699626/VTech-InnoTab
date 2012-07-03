//#include <linux/io.h>
#include <linux/module.h>
#include <linux/moduleparam.h> 
#include <linux/cdev.h>
//#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/fs.h>
//#include <linux/hdreg.h> 		/* HDIO_GETGEO */
#include <linux/blkdev.h>

#include <linux/platform_device.h>

#include <mach/kernel.h>
#include <mach/diag.h>
//#include <mach/gp_ppu.h>
#include <mach/gp_gpio.h>
#include <mach/gp_chunkmem.h>
//#include <mach/gp_tv.h>
//#include <mach/gp_ppu_irq.h>
#include <mach/hal/hal_clock.h>

#include <mach/vppu.h>

//#define DEBUG_PRINT	printk
#define DEBUG_PRINT(...)

//#define TRACE_LINE	{DEBUG_PRINT(KERN_WARNING "{TRACE_LINE %s:%d:%s()}\n", __FILE__, __LINE__, __FUNCTION__);}
#define TRACE_LINE	


#define	PPU_MINOR		         0
#define PPU_NR_DEVS	             1
#define VIC_PPU                  20

#define VPPU_REG_START	0x93020000
#define VPPU_REG_END	0x93027BFF
#define VPPU_REG_LEN	(VPPU_REG_END - VPPU_REG_START + 1)

int vppu_fops_open(struct inode *inode, struct file *filp);
int vppu_fops_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
int vppu_fops_release(struct inode *inode, struct file *filp);
ssize_t vppu_fops_read(struct file *filp, char __user *buff, size_t count, loff_t *offp);
ssize_t vppu_fops_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp);

static irqreturn_t isr_ppu(int irq, void *dev_id);


struct file_operations ppu_fops = {
	.owner = THIS_MODULE,
	//.poll = vppu_poll,
	.open = vppu_fops_open,
	.ioctl = vppu_fops_ioctl,
	.release = vppu_fops_release,
	.read = vppu_fops_read,
	.write = vppu_fops_write,
};


#define N_REGISTER_REGIONS	6

static const ppu_register_region_t ppu_register_regions[N_REGISTER_REGIONS] = {
	{0x0000, 0x00f0},
	{0x00f8, 0x0140},
	{0x0188, 0x0190},
	{0x01c0, 0x0200},
	{0x0300, 0x03b0},
	{0x0400, 0x7c00},
};

static const ppu_register_region_t default_register_filling_regions[19] = {//[20] = {//
	{0x0000, 0x0078+4},
	{0x0080, 0x0090+4},
	{0x00a0, 0x00a8+4},
	{0x00c0, 0x00c0+4},
	{0x00d8, 0x00e8+4},
	{0x00f8, 0x00fc+4},
	{0x0104, 0x0108+4},
	{0x0120, 0x013c+4},
	{0x01e0, 0x01e0+4},
	{0x01e8, 0x01e8+4},
	{0x01f4, 0x01fc+4},
	
	
	{0x0300, 0x035c+4},
	//{0x0300, 0x0338+4},
	//{0x0340, 0x035c+4},


	{0x036c, 0x0370+4},
	{0x037c, 0x0384+4},
	{0x03a8, 0x03ac+4},
	{0x0400, 0x07bc+4},
	{0x0800, 0x0f7c+4},
	{0x1000, 0x7bc0+4},
	{0, 0}

//	{0x0188, 0x018c+4},	//IRQ_EN and IRQ_STATUS
//	{0x01c0, 0x01d0+4},	//SPRITE_DMA and HB
//	{0x01f0, 0x01f0+4},	//FB_GO
};


typedef struct gp_ppu_dev_s {
	struct cdev c_dev;
	wait_queue_head_t ppu_wait_queue;
	wait_queue_head_t ppu_wait_irq_queue;
	struct semaphore ppu_draw_sem;
	bool done;
	char *ioremap_ptr;
	int n_openfile;
	int drawing_id;
	unsigned int vb_count;
} gp_ppu_dev_t;

typedef struct vppu_dev_s {
	int open_id;
	int frame_count;
	unsigned int register_file[VPPU_REG_LEN / 4];
} vppu_dev_t;

typedef struct vppu_dev_directmode_s {
	int open_id;
	int frame_count;
} vppu_dev_directmode_t;

int ppu_major;
static gp_ppu_dev_t *ppu_devices=NULL;
struct class *ppu_class;
static int g_next_open_id = 1;

extern const unsigned int vppu_default_regvalues[VPPU_REG_LEN / 4];	//defined in vppu_default_regvalue.c

/**
 * @brief   PPU clock enable/disable
 * @param   enable [in] 0:disable, 1:enable
 * @return  None
 * @see
 */
static void ppu_clock_enable(int enable)
{
		 gpHalScuClkEnable( SCU_A_PERI_LINEBUFFER | SCU_A_PERI_SAACC | 
		 		 		 		 		    SCU_A_PERI_NAND_ABT | SCU_A_PERI_REALTIME_ABT | 
		 		 		 		 		    SCU_A_PERI_SCA, SCU_A, enable);
		 gpHalScuClkEnable( SCU_A_PERI_CEVA_L2RAM | SCU_A_PERI_PPU | 
		 		 		 		 		    SCU_A_PERI_PPU_REG | SCU_A_PERI_PPU_FB, SCU_A2, enable);
		 gpHalScuClkEnable(SCU_C_PERI_2DSCALEABT, SCU_C, enable);		 		 		 		    
} 


static int _init_ioremap(void)
{
	if (ppu_devices->ioremap_ptr == 0) {
		//request_mem_region
#if 0
		int i;
		for (i = 0; i < N_REGISTER_REGIONS; i++) {
			unsigned int start = VPPU_REG_START + ppu_register_regions[i].start;
			unsigned int len = ppu_register_regions[i].end - ppu_register_regions[i].start;
			if (request_mem_region(start, len, "VPPU") == 0) {
				return -1;
			}
		}
#endif
		//ioremap
		ppu_devices->ioremap_ptr = (char*)ioremap_nocache(VPPU_REG_START, VPPU_REG_LEN);
		if (ppu_devices->ioremap_ptr == 0) {
			return -1;
		}
		//DEBUG_PRINT(KERN_WARNING "VPPU: ioremap(0x%X, 0x%X) returns 0x%08X\n", VPPU_REG_START, VPPU_REG_LEN, (unsigned int)ppu_devices->ioremap_ptr);
	}
	return 0;
}

static int _term_ioremap(void)
{
	if (ppu_devices->ioremap_ptr) {
		//iounmap
		iounmap(ppu_devices->ioremap_ptr);
		ppu_devices->ioremap_ptr = 0;
		//release_mem_region
#if 0
		int i;
		for (i = 0; i < N_REGISTER_REGIONS; i++) {
			unsigned int start = VPPU_REG_START + ppu_register_regions[i].start;
			unsigned int len = ppu_register_regions[i].end - ppu_register_regions[i].start;
			release_mem_region(start, len);
		}
#endif
	}
	return 0;
}

static void put_reg(unsigned int offset, unsigned int value)
{
	iowrite32(value, ppu_devices->ioremap_ptr + offset);
}

static unsigned int get_reg(unsigned int offset)
{
	return ioread32(ppu_devices->ioremap_ptr + offset);
}


int vppu_fops_open(struct inode *inode, struct file *filp)
{
	if (ppu_devices->ioremap_ptr == 0) {
		if (_init_ioremap() < 0) {
			_term_ioremap();
			DEBUG_PRINT(KERN_WARNING "VPPU: failed in _init_ioremap()\n");
			return -1;
		}

		put_reg(VPPU_IRQ_EN, 0);			//disable all interrupts
		put_reg(VPPU_IRQ_STATUS, ~0);		//clear all interrupt status

		if (request_irq(VIC_PPU, isr_ppu, IRQF_SHARED, "VPPU", (void*)ppu_devices) != 0) {
			_term_ioremap();
			DEBUG_PRINT(KERN_WARNING "VPPU: failed to install IRQ handler\n");
			return -1;
		}
		
		ppu_devices->n_openfile = 0;
		init_MUTEX(&ppu_devices->ppu_draw_sem);
	}

	if (filp->f_flags & O_SYNC) {
		vppu_dev_directmode_t *vppu_dev;
		//open in sync mode
		if (down_interruptible(&ppu_devices->ppu_draw_sem) != 0) {
			return -ERESTARTSYS;	//interrupted by signal
		}
		vppu_dev = kmalloc(sizeof(vppu_dev_directmode_t), GFP_KERNEL);
		if (!vppu_dev) {
			if (ppu_devices->n_openfile == 0) {
				_term_ioremap();
			}
			DEBUG_PRINT(KERN_WARNING "PPU: kmalloc fails \n");
			return -ENOMEM;
		}
		vppu_dev->open_id = g_next_open_id++;
		filp->private_data = vppu_dev;

	} else {
		vppu_dev_t *vppu_dev;
		vppu_dev = kmalloc(sizeof(vppu_dev_t), GFP_KERNEL);
		if (!vppu_dev) {
			if (ppu_devices->n_openfile == 0) {
				_term_ioremap();
			}
			DEBUG_PRINT(KERN_WARNING "PPU: kmalloc fails \n");
			return -ENOMEM;
		}
		memcpy(vppu_dev->register_file, vppu_default_regvalues, VPPU_REG_LEN);
		vppu_dev->open_id = g_next_open_id++;
		filp->private_data = vppu_dev;
	}
	ppu_devices->n_openfile++;
	
	//DEBUG_PRINT(KERN_WARNING "PPU open [%d]%s\n", ppu_devices->n_openfile, (filp->f_flags & O_SYNC)? " sync-mode": "");
	return 0;
}

int vppu_fops_release(struct inode *inode, struct file *filp)
{
	vppu_dev_t *vppu_dev = filp->private_data;
	kfree(vppu_dev);

	ppu_devices->n_openfile--;
	if (ppu_devices->n_openfile <= 0) {
		free_irq(VIC_PPU, (void*)ppu_devices);
		put_reg(VPPU_IRQ_EN, 0);			//disable all interrupts
		put_reg(VPPU_IRQ_STATUS, ~0);		//clear all interrupt status
		_term_ioremap();
		ppu_devices->n_openfile = 0;
	}

	/* Success */
	if (filp->f_flags & O_SYNC) {
		up(&ppu_devices->ppu_draw_sem);
	}
	//DEBUG_PRINT(KERN_WARNING "PPU release [%d]\n", ppu_devices->n_openfile + 1);
	return 0;
}


static void read_batch_ppu_registers(unsigned int *register_file)
{
	const ppu_register_region_t *rgn = default_register_filling_regions;
	while (rgn->end != 0) {
		//DEBUG_PRINT(KERN_WARNING "%s: rgn={0x%08X, 0x%08X}\n", __FUNCTION__, rgn->start, rgn->end);
#if 1
	//memcpy_fromio((char*)register_file + rgn->start, ppu_devices->ioremap_ptr + rgn->start, rgn->end - rgn->start);
	int i;

	for ( i = 0; i < ( rgn->end - rgn->start ) / 4; i ++ )		// Assume starting address and data size are multiple of 4 
	{
		*((int *)((char*)register_file + rgn->start + 4 * i )) = ioread32( ppu_devices->ioremap_ptr + rgn->start + i * 4 );
	}
#else
		unsigned int *src = (unsigned int*)(ppu_devices->ioremap_ptr + rgn->start);
		unsigned int *dst = register_file + (rgn->start / 4);
		int count = (rgn->end - rgn->start) / 4;
		while (count-- > 0) {
			*dst++ = ioread32(src++);
		}
#endif		
		rgn++;
	}
}

static void write_batch_ppu_registers(unsigned int *register_file, const ppu_register_region_t __user *update_seq)
{
	ppu_register_region_t rgn;
	const ppu_register_region_t *p_rgn = default_register_filling_regions;

	if (update_seq) {
		p_rgn = update_seq;
		copy_from_user(&rgn, p_rgn, sizeof(rgn));
	} else {
		rgn = *p_rgn;
	}
	p_rgn++;
	
	while (rgn.end != 0) {
		//DEBUG_PRINT(KERN_WARNING "%s: rgn={0x%08X, 0x%08X}\n", __FUNCTION__, rgn.start, rgn.end);
#if 1
	//memcpy_toio(ppu_devices->ioremap_ptr + rgn.start, (char*)register_file + rgn.start, rgn.end - rgn.start);
	int i;

	for ( i = 0; i < ( rgn.end - rgn.start ) / 4; i ++ )		// Assume starting address and data size are multiple of 4 
	{
		iowrite32( *((int *)( (char*)register_file + rgn.start + 4 * i )), ppu_devices->ioremap_ptr + rgn.start + i * 4 );
	}
#if 0
		//printk( KERN_WARNING "%s()-%d: rgn.start = %x\n",__PRETTY_FUNCTION__, __LINE__, rgn.start );
		if ( rgn.start == 0x1000 )
		{
			int k;
			printk( /*KERN_WARNING*/ "%s()-%d: Data: ", __PRETTY_FUNCTION__, __LINE__ );
			for ( k = 0; k < 16; k ++ )
			{
				printk( /*KERN_WARNING*/ "%x, ", *((char*)register_file + rgn.start + k) );
			    if ( k < 16 - 1 )
				{
					printk( /*KERN_WARNING*/ ", " );
				}
			}
			printk( /*KERN_WARNING*/ "\n" );
		}
#endif
#else
		unsigned int *src = register_file + (rgn->start / 4);
		unsigned int *dst = (unsigned int*)(ppu_devices->ioremap_ptr + rgn->start);
		int count = (rgn->end - rgn->start) / 4;
		while (count-- > 0) {
			iowrite32(*src++, dst++);
		}
#endif		
		if (update_seq) {
			copy_from_user(&rgn, p_rgn, sizeof(rgn));
		} else {
			rgn = *p_rgn;
		}
		p_rgn++;
	}
}

int vppu_fops_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	vppu_dev_t *vppu_dev = filp->private_data;
TRACE_LINE
	
	switch (cmd) {
	case VPPU_IOC_DRAWFB:
		{
			unsigned int prev_vb_count;
TRACE_LINE
			//gain access to ppu draw
			if ((filp->f_flags & O_SYNC) == 0) {
				const ppu_register_region_t *update_seq = (const ppu_register_region_t*)arg;
				//non-sync mode
				if (down_interruptible(&ppu_devices->ppu_draw_sem) != 0) {
					return -ERESTARTSYS;	//interrupted by signal
				}
				write_batch_ppu_registers(vppu_dev->register_file, update_seq);
			}
			ppu_devices->drawing_id = vppu_dev->open_id;
	
TRACE_LINE
			prev_vb_count = ppu_devices->vb_count;
			put_reg(VPPU_IRQ_STATUS, VPPU_IRQSTATUS_VB);	 					//clear VBLANK in IRQ_STATUS
			put_reg(VPPU_IRQ_EN, get_reg(VPPU_IRQ_EN) | VPPU_IRQSTATUS_VB);		//enable VBLANK in IRQ_EN
			put_reg(VPPU_FB_GO, 1);		 						//start drawing
	
TRACE_LINE
			if (wait_event_interruptible(ppu_devices->ppu_wait_irq_queue, ppu_devices->vb_count != prev_vb_count) != 0) {
TRACE_LINE
				ret = -EINTR;
			} else {
TRACE_LINE
				if ((filp->f_flags & O_SYNC) == 0) {
					//non-sync mode
					read_batch_ppu_registers(vppu_dev->register_file);	//read back the PPU values
				}
			}
TRACE_LINE
			ppu_devices->drawing_id = 0;
			if ((filp->f_flags & O_SYNC) == 0) 
			{
				up(&ppu_devices->ppu_draw_sem);
			}
			wake_up_interruptible(&ppu_devices->ppu_wait_queue);
		}
		break;
		
// 	case VPPU_IOC_WAIT_DRAWING:
// 		if (wait_event_interruptible(ppu_devices->ppu_wait_queue, ppu_devices->vb_count != prev_vb_count) != 0) {
// 			ret = -ERESTARTSYS;
// 		}
// 
// 	case VPPU_IOC_IS_DRAWING:
// 		ret = (ppu_devices->vb_count != prev_vb_count)? 0: 1;
// 		break;

	default:
		ret = -ENOIOCTLCMD;
		break;                      
	}

	return ret;
}






static void gp_ppu_device_release(struct device *dev)                       
{                                                                           
	DIAG_INFO("remove vppu device ok\n");                                      
}                                                                           

static struct resource gp_ppu_resources[] = {
	[0] = {
		.start  = VPPU_REG_START,
		.end	= VPPU_REG_END,
		.flags  = IORESOURCE_MEM,
	},
};

static struct platform_device gp_ppu_device = {                             
	.name	= "vppu",                                                         
	.id	= 0,                                                                  
	.dev	= {                                                                 
		.release = gp_ppu_device_release,                                       
	},                                                                        
	.num_resources  = ARRAY_SIZE(gp_ppu_resources),
	.resource       = gp_ppu_resources,
};   

static struct platform_driver gp_ppu_driver = {                                                                             
	//.suspend = gp_ppu_suspend,                                                
	//.resume = gp_ppu_resume,                                                  
	.driver	= {                                                               
		.owner	= THIS_MODULE,                                                  
		.name	= "vppu"                                                        
	}                                                                     
};                                                                          

void __exit vppu_module_exit(void)
{
	dev_t devno = MKDEV(ppu_major, PPU_MINOR);
	cdev_del(&(ppu_devices->c_dev));
	kfree(&ppu_devices);
	ppu_devices = 0;
	unregister_chrdev_region(devno, PPU_NR_DEVS);

	platform_device_unregister(&gp_ppu_device);
	platform_driver_unregister(&gp_ppu_driver);

	DEBUG_PRINT(KERN_WARNING "VPPU module exit \n");
}

int __init vppu_module_init(void)
{
	int result;
	dev_t dev;
	int devno;

	result = alloc_chrdev_region(&dev, PPU_MINOR, 1, "PPU");
	if (result < 0) {
		DEBUG_PRINT(KERN_WARNING "VPPU: can't get major \n");
		return result;
	}
	ppu_major = MAJOR(dev);
	ppu_class = class_create(THIS_MODULE, "VPPU");
	
	ppu_devices = kmalloc(sizeof(vppu_dev_t), GFP_KERNEL);
	if (!ppu_devices) {
		DEBUG_PRINT(KERN_WARNING "VPPU: kmalloc failed \n");
		result = -ENOMEM;
		goto fail;
	}
	memset(ppu_devices, 0, sizeof(gp_ppu_dev_t));
	ppu_devices->vb_count = 0;	//avoid compiler warning

	// Initialize wait queue before adding character devices
	init_waitqueue_head(&ppu_devices->ppu_wait_queue);
	init_waitqueue_head(&ppu_devices->ppu_wait_irq_queue);
	
	devno = MKDEV(ppu_major, PPU_MINOR);
	cdev_init(&(ppu_devices->c_dev), &ppu_fops);
	ppu_devices->c_dev.owner = THIS_MODULE;
	ppu_devices->c_dev.ops = &ppu_fops;
	result = cdev_add(&(ppu_devices->c_dev), devno, 1);
	device_create(ppu_class, NULL, devno, NULL, "ppu%d", 0);
	
	if (result)
		DEBUG_PRINT(KERN_WARNING "Error adding ppu");
  
/*	init_waitqueue_head(&ppu_devices->ppu_wait_queue);
	init_waitqueue_head(&ppu_devices->ppu_wait_irq_queue);*/
  
	platform_device_register(&gp_ppu_device);
	result = platform_driver_register(&gp_ppu_driver);
	
	//enable ppu clocks
	ppu_clock_enable(1);

	{ //enable P_STN_CTRL0, otherwise PPU does not draw
		#define P_STN_CTRL0		0x9302017C
		unsigned int *stn_en = (unsigned int*)ioremap_nocache(P_STN_CTRL0, 4);
		if (!stn_en) {
			DEBUG_PRINT(KERN_WARNING "VPPU: failed to enable STN\n");
		} else {
			iowrite32(1, stn_en);
			iounmap(stn_en);				
		}
	}
	
	{ //enable P_SCUC_PERI_CLKEN bit22, to make PPU can write to DRAM
		#define P_SCUC_PERI_CLKEN		0x92005004
		unsigned int *peri_clken = (unsigned int*)ioremap_nocache(P_SCUC_PERI_CLKEN, 4);
		if (!peri_clken) {
			DEBUG_PRINT(KERN_WARNING "VPPU: failed to set P_SCUC_PERI_CLKEN.22 = 1\n");
		} else {
			unsigned int n = ioread32(peri_clken);
			n |= (1 << 22);
			iowrite32(n, peri_clken);
			iounmap(peri_clken);				
		}
	}
	
	
	DEBUG_PRINT(KERN_WARNING "VPPU module init\n");
	return result;

fail:
	DEBUG_PRINT(KERN_WARNING "VPPU module init failed \n");
	kfree(ppu_devices);
	unregister_chrdev_region(dev, PPU_NR_DEVS);
	return result;
}

ssize_t vppu_fops_read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
	vppu_dev_t *vppu_dev = filp->private_data;
	
	if (*offp >= VPPU_REG_LEN) {
		return 0;
	}
	if ((*offp & 3) != 0) {
		return 0;
	}
	count &= ~3;	//4-byte per unit to read
	if (*offp + count >= VPPU_REG_LEN) {
		count = VPPU_REG_LEN - *offp;
	}

	if (count > 0) {
		if (filp->f_flags & O_SYNC) {
			//sync mode
			size_t cnt = 0;
			while (cnt < count) {
				unsigned int n = 0;
				n = get_reg(*offp);
				copy_to_user(buff, &n, 4);
				buff += 4;
				*offp += 4;
				cnt += 4;
			}
		} else {
			copy_to_user(buff, (char*)(vppu_dev->register_file) + *offp, count);
			*offp += count;
		}
	}
	return count;
}

ssize_t vppu_fops_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp)
{
	vppu_dev_t *vppu_dev = filp->private_data;

	if (*offp >= VPPU_REG_LEN) {
		return 0;
	}
	if ((*offp & 3) != 0) {
		return 0;
	}
	count &= ~3;	//4-byte per unit to write
	if (*offp + count >= VPPU_REG_LEN) {
		count = VPPU_REG_LEN - *offp;
	}

	if (count > 0) {
		if (filp->f_flags & O_SYNC) {
			//sync mode
			size_t cnt = 0;
			while (cnt < count) {
				unsigned int n;
				copy_from_user(&n, buff, 4);
				put_reg(*offp, n);		
				buff += 4;
				*offp += 4;
				cnt += 4;
			}
		} else {
			copy_from_user((char*)(vppu_dev->register_file) + *offp, buff, count);
			*offp += count;
		}
	}
	return count;
}

static irqreturn_t isr_ppu(int irq, void *dev_id)
{
	gp_ppu_dev_t *dev = (gp_ppu_dev_t*)dev_id;
	DEBUG_PRINT(KERN_WARNING "%s(%d, 0x%08X)\n", __FUNCTION__, irq, (unsigned int)dev_id);
	if (dev) {
		//Only handle PPU VB interrupt
		unsigned int irq_status = get_reg(VPPU_IRQ_STATUS);
		DEBUG_PRINT(KERN_WARNING "%s: irq_status=0x%08X\n", __FUNCTION__, irq_status);
		if (irq_status & VPPU_IRQSTATUS_VB) {
			ppu_devices->vb_count++;
			wake_up_interruptible(&dev->ppu_wait_irq_queue);			//wake any waiting process
			
			put_reg(VPPU_IRQ_EN, get_reg(VPPU_IRQ_EN) & ~VPPU_IRQSTATUS_VB); // jackson: disable ONLY VB interrupt
			put_reg(VPPU_IRQ_STATUS, VPPU_IRQSTATUS_VB); // jackson: clear ONLY VB interrupt 
		}
		
		// jackson: don't disable all interrupt put_reg(VPPU_IRQ_EN, 0);			//disable all interrupts
		// jackson: don't clear all interrupt put_reg(VPPU_IRQ_STATUS, ~0);		//clear all interrupt status
		
		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}


module_init(vppu_module_init);
module_exit(vppu_module_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

MODULE_AUTHOR("VTech");
MODULE_DESCRIPTION("VTech PPU Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

