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
 * @file    gpl32900.c
 * @brief   GPL32900 IC base driver
 * @author  Roger hsu
 */

#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/diag.h>

#include <mach/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <mach/clock_mgr/gp_clock_private.h>
#include <mach/gp_sram.h>
#include <mach/pm.h>
#include <mach/hal/hal_usb.h>
#include <mach/hal/regmap/reg_scu.h>
#include <mach/gp_fiq.h>

int gpl32900_READY = 0;
volatile int idle_counter;

#define		MMP_IRQ_VIC0_BASE			IO0_ADDRESS(0x10000)
#define		MMP_IRQ_VIC1_BASE			IO0_ADDRESS(0x20000)
#define	    MMP_IRQENABLE				0x10
#define     MMP_IRQENABLECLEAR		0x14

extern void __init gpl32900_devinit(void); 
extern void gpl32900_power_off(void);

#define CYG_DEVICE_IRQ0_EnableSet \
    (*(volatile unsigned int *) (MMP_IRQ_VIC0_BASE + MMP_IRQENABLE))
    // Enable (1's only), write only
#define CYG_DEVICE_IRQ0_EnableClear \
    (*(volatile unsigned int *) (MMP_IRQ_VIC0_BASE + MMP_IRQENABLECLEAR))
    // Disable (1's only), write only

#define CYG_DEVICE_IRQ1_EnableSet \
    (*(volatile unsigned int *) (MMP_IRQ_VIC1_BASE + MMP_IRQENABLE))
    // Enable (1's only), write only
#define CYG_DEVICE_IRQ1_EnableClear \
    (*(volatile unsigned int *) (MMP_IRQ_VIC1_BASE + MMP_IRQENABLECLEAR))
    // Disable (1's only), write only

#define MAX_KMALLOC_SIZE 0x20000
#define CEVA_ADDR_START 0x0f0000
#define CEVA_ADDR_END   0x1fffff
#define CEVA_LOOP_COUNT ((CEVA_ADDR_END - CEVA_ADDR_START + 1 + MAX_KMALLOC_SIZE - 1) / MAX_KMALLOC_SIZE)

int ceva_memory_alloc(void)
{
	int i;
	int *buf_ptr = 0;
	int buf_count = 0;
	int ceva_ptr[CEVA_LOOP_COUNT];
	int ceva_count = 0;
	unsigned int addr, phy_addr;


	printk("[%s] start\n", __FUNCTION__);

	for (i = 0; ceva_count < CEVA_LOOP_COUNT; i++) {
		addr = (unsigned int)kmalloc(MAX_KMALLOC_SIZE, GFP_KERNEL);
		if (addr == 0) {
			printk("[%s][%d] kmalloc fail at loop %d, size = %d \n", __FUNCTION__, __LINE__, i, MAX_KMALLOC_SIZE);
			break;
		}
		phy_addr = (unsigned int)__pa(addr);

		/* Check if located in memory for ceva */
		if (((phy_addr >= CEVA_ADDR_START) && (phy_addr <= CEVA_ADDR_END)) ||
			((phy_addr + MAX_KMALLOC_SIZE - 1 >= CEVA_ADDR_START) && (phy_addr + MAX_KMALLOC_SIZE - 1 <= CEVA_ADDR_END))) {
			//printk("[%s][%d] ceva_ptr[%d] %08x\n", __FUNCTION__, __LINE__, ceva_count, phy_addr);
			ceva_ptr[ceva_count++] = addr;
		}
		else {
			if (buf_ptr == 0) {
				buf_ptr = (int*)addr;
			}
			else {
				buf_ptr[buf_count++] = addr;
			}
			//printk("[%s][%d] buf_ptr(%d)  %08x\n", __FUNCTION__, __LINE__, buf_count, phy_addr);
		}
	}
	
	if (ceva_count < CEVA_LOOP_COUNT) {
		printk("Ceva memory not full reserved!\n");
	}
	
	if (buf_ptr) {
		for (i = 0; i < buf_count; i++) {
			kfree((void*)buf_ptr[i]);
		}
		kfree(buf_ptr);
	}
	
#if 0
	buf_index = kmalloc(PAGE_SIZE, GFP_KERNEL);

	for (i = 0 ; i < 4096 ; i ++)
	{
		buf_index[i] = (int)kmalloc(MAX_KMALLOC_SIZE, GFP_KERNEL);
		if (buf_index[i] == 0) {
			printk("[%s][%d] kmalloc fail at %d, size = %d \n", __FUNCTION__, __LINE__, i, MAX_KMALLOC_SIZE);
			return 0;
		}

		//printk("[%s][%d][%d] ptr=0x%x, pa=0x%x\n", __FUNCTION__, __LINE__, i, buf_index[i], __pa(buf_index[i]));
		// check and keep memory for ceva
		if ( ( __pa(buf_index[i]) >= 0xe0000) && ( __pa(buf_index[i]) <= 0x200000) ) {
			int timeout = 0x1000;

			//printk("[%s][%d] keep buffer [%d]\n", __FUNCTION__, __LINE__, i);
			kfree((int *)buf_index[i]);
			i--;

			//for (j = 0 ; j < CEVA_LOOP_COUNT ; j ++) {
			j=0;
			do {
				ceva_ptr[j] = (int)kmalloc(MAX_KMALLOC_SIZE, GFP_KERNEL);
				if (ceva_ptr[j] == 0) {
					printk("[%s][%d] kmalloc fail at %d\n", __FUNCTION__, __LINE__, j);
					return 0;
				}
				//printk("[%s][%d][%d] ceva_ptr ptr=0x%x, pa=0x%x\n", __FUNCTION__, __LINE__, j, ceva_ptr[j], __pa(ceva_ptr[j]));
				if ((j == 0) && (__pa(ceva_ptr[j]) != 0xe0000)) {
					i++;
					buf_index[i] = ceva_ptr[j];
					continue;
				}
				j++ ;
				if (timeout-- == 0) {
					printk("ERROR !!! ceva memory allocate fail\n");
				}

			} while (j < CEVA_LOOP_COUNT);
			if ( (__pa(ceva_ptr[0]) != 0xe0000) && (__pa(ceva_ptr[8]) != 0x1e0000) ) {
				printk("[%s][%d][%d] ceva_ptr ptr=0x%x, pa=0x%x\n", __FUNCTION__, __LINE__, 0, ceva_ptr[0], (unsigned int)__pa(ceva_ptr[0]));
				printk("[%s][%d][%d] ceva_ptr ptr=0x%x, pa=0x%x\n", __FUNCTION__, __LINE__, 8, ceva_ptr[8], (unsigned int)__pa(ceva_ptr[8]));
			}

			break;
		}
	}
	//printk("[%s][%d][%d]\n", __FUNCTION__, __LINE__, i);
	
	for ( ; i >= 0 ; i --)
	{
		//printk("[%s][%d] start release i=[%d], buf_index[i]=0x%x\n", __FUNCTION__, __LINE__, i, buf_index[i]);
		if (buf_index[i]) 
			 kfree((int *)buf_index[i]);
	}
	//printk("[%s][%d] end release i=[%d]\n", __FUNCTION__, __LINE__, i);
#endif

#if 0
	for (i = 0 ; i < 10240 ; i ++)
	{
		buf_index[i] = kmalloc(0x1000, GFP_KERNEL);
		if (buf_index[i] == NULL) {
			printk("[%s][%d] kmalloc fail at %d, size = %d \n", __FUNCTION__, __LINE__, i, MAX_KMALLOC_SIZE);
			return 0;
		}
		printk("[%s][%d][%d] ptr=0x%x, pa=0x%x\n", __FUNCTION__, __LINE__, i, buf_index[i], __pa(buf_index[i]));
		if ( ( __pa(buf_index[i]) >= 0xe0000) && ( __pa(buf_index[i]) <= 0x200000) ) {
			printk("!!!!!!!!!!!!!!!!!!!!!!!!!! [%s][%d] still alloc ceva [%d]\n", __FUNCTION__, __LINE__, i);
			return 0;
		}
	}
#endif
	
#if 0
	if (buf_index)
		kfree(buf_index);		
#endif

	printk("[%s] finish\n", __FUNCTION__);
	return 0;
}
static void __init gpl32900_init(void)
{
    gpl32900_devinit();
    
    /* set TCK to low */
    SCUB_PIN_MUX |= 0x1; /* disable ICE, JTAG pin will return to IOB[25:20] */
    SCUB_GPIO1_IE &= (~0x3F00000); /* IOB[25:20] input enable, always low of IO status */
    //SCUB_PGS0 |= 0x1; /* with ARM clock on */
    SCUB_WFI &= (~0x1); /* with ARM clock off */
    idle_counter = 0; /*0:enable , others:disable*/
    pm_power_off = gpl32900_power_off;	
}

void gpl32900_mask_irq(unsigned int irq)
{
	if (irq & 0x20) 
	{
		CYG_DEVICE_IRQ1_EnableClear = (1<<(irq & 0x1F));
	}
	else 
	{
		CYG_DEVICE_IRQ0_EnableClear = (1<<irq);
   	}		
}

void gpl32900_unmask_irq(unsigned int irq)
{
	if (irq & 0x20)
	{
		CYG_DEVICE_IRQ1_EnableSet = (1<<(irq & 0x1F));
	}
	else
	{
   		CYG_DEVICE_IRQ0_EnableSet =  (1<<irq);    
   	}			
}

void disable_do_idle(void)
{
	idle_counter++;
}
EXPORT_SYMBOL(disable_do_idle);

void enable_do_idle(void)
{
	if (idle_counter > 0) {
		idle_counter--;
	}
}
EXPORT_SYMBOL(enable_do_idle);
//
// This routine is called to respond to a hardware interrupt (IRQ).  It
// should interrogate the hardware and return the IRQ vector number.

#ifdef CONFIG_PM

/* state for IRQs over sleep */

/* default is to allow for EINT0..EINT15, and IRQ_RTC as wakeup sources
 *
 * set bit to 1 in allow bitfield to enable the wakeup settings on it
*/

unsigned long gp_irqwake_int1mask	= 0xffffffffL;
unsigned long gp_irqwake_int2mask	= 0xffffffdcL;

void (*gp_sram_suspend)(void) = NULL;
void __init gp_init_pm(void)
{
	printk("[%s][%d] run\n", __FUNCTION__, __LINE__);
	gp_sram_suspend = gp_sram_push(gpl32900_cpu_suspend,
				   gpl32900_cpu_suspend_sz);
	//gp_cpu_pm_fns = &gpl32900_cpu_pm_fns;
}

int
gpl32900_irq_wake(unsigned int irqno, unsigned int state)
{	
	if (!state)
	{
	  if (irqno & 0x20)
	  {
		gp_irqwake_int2mask |= (1<<(irqno & 0x1F));
	  }
	  else
	  {
   		gp_irqwake_int1mask |=  (1<<irqno);    
      }
	}	
	else
	{
	  if (irqno & 0x20)
	  {
		gp_irqwake_int2mask &= ~(1<<(irqno & 0x1F));
	  }
	  else
	  {
   		gp_irqwake_int1mask &=  ~(1<<irqno);    
      }
	}
	return 0;
}

#else
#define gpl32900_irq_wake NULL
inline void gp_init_pm(void) {}
#endif

static struct irq_chip gpl32900_irq_chip = {
	.name	= "GPL32900",
	.mask	= gpl32900_mask_irq,
	.mask_ack = gpl32900_mask_irq,
	.unmask = gpl32900_unmask_irq,
	.set_wake	= gpl32900_irq_wake,	
};

void gp_init_irq(void)
{
	unsigned int i;
	
	for (i = 0; i < NR_IRQS; i++) {
		set_irq_chip(i, &gpl32900_irq_chip);
		set_fiq_chip(i, &gpl32900_irq_chip);
		set_irq_handler(i, handle_level_irq);
		set_irq_flags(i, IRQF_VALID | IRQF_PROBE);
	}
}

static struct map_desc gpl32900_io_desc[] __initdata = {
	{
		.virtual	= IO0_BASE,
		.pfn		= __phys_to_pfn(IO0_START),
		.length		= IO0_SIZE,
		.type		= MT_DEVICE
	},
	{
        	.virtual	= IO2_BASE,
		.pfn		= __phys_to_pfn(IO2_START),
		.length		= IO2_SIZE,
		.type		= MT_DEVICE
	},
	{
		.virtual	= IO3_BASE,
		.pfn		= __phys_to_pfn(IO3_START),
		.length		= IO3_SIZE,
		.type		= MT_DEVICE		
	},		
	{
		.virtual	= ROM_BASE,
		.pfn		= __phys_to_pfn(ROM_START),
		.length		= ROM_SIZE,
		.type		= MT_DEVICE		
	},				
};

static void __init gp_map_io(void)
{	
	iotable_init( gpl32900_io_desc, ARRAY_SIZE( gpl32900_io_desc));
#ifdef CONFIG_PM
	local_flush_tlb_all();
	flush_cache_all();		
    gp_sram_init();
#endif
	/*Disable USB Connect At the Beginning. allenchang@generalplus.com*/
	gpHalUsbSlaveSwConnect(0);	
	gp_register_baseclocks(0);
    gp_setup_clocks();
    gp_baseclk_add();
    dumpclk();	
    gpl32900_READY = 1;
}

extern struct sys_timer gpl32900_timer;

MACHINE_START(GPL32900, "GPL32900")
	.phys_io	= 0x90000000,
	.io_pg_offst	= ((0xfc000000) >> 18) & 0xfffc,
	.boot_params	= 0x00200100,
	.map_io		= gp_map_io,
	.init_irq	= gp_init_irq,
	.init_machine	= gpl32900_init,
	.timer		= &gpl32900_timer,
MACHINE_END

