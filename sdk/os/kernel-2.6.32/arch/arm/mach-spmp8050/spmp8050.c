/* arch/arm/mach-spmp8050/spmp8050.c
**
** Copyright (C) 2007 Google, Inc.
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/input.h>
#include <asm/tlb.h>
#include <asm/cacheflush.h>
#include <mach/hardware.h>
#include <asm/io.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/flash.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>
#include <mach/spmp_clock.h>
#include <mach/spmp_sram.h>
int spmp8050_READY = 0;

#define		MMP_IRQ_VIC0_BASE			IO0_ADDRESS(0x10000)
#define		MMP_IRQ_VIC1_BASE			IO0_ADDRESS(0x20000)
#define	    MMP_IRQENABLE				0x10
#define     MMP_IRQENABLECLEAR		0x14

extern void __init spmp8050_devinit(void); 
extern void spmp8050_power_off(void);

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
#define CEVA_LOOP_COUNT (0x110000/0x20000) + 1

int ceva_memory_alloc(void)
{
	int i, j;
	int *buf_index;
	int ceva_ptr[9];


	printk("[%s] start\n", __FUNCTION__);
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
	

	if (buf_index)
		kfree(buf_index);
	printk("[%s] finish\n", __FUNCTION__);
	return 0;
}
static void __init spmp_init(void)
{
    spmp8050_devinit();
    pm_power_off = spmp8050_power_off;	
}

void spmp8050_mask_irq(unsigned int irq)
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

void spmp8050_unmask_irq(unsigned int irq)
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

//
// This routine is called to respond to a hardware interrupt (IRQ).  It
// should interrogate the hardware and return the IRQ vector number.

#ifdef CONFIG_PM

/* state for IRQs over sleep */

/* default is to allow for EINT0..EINT15, and IRQ_RTC as wakeup sources
 *
 * set bit to 1 in allow bitfield to enable the wakeup settings on it
*/

unsigned long spmp_irqwake_int1mask	= 0xffffffffL;
unsigned long spmp_irqwake_int2mask	= 0xffffffdcL;


int
spmp8050_irq_wake(unsigned int irqno, unsigned int state)
{	
	if (!state)
	{
	  if (irqno & 0x20)
	  {
		spmp_irqwake_int2mask |= (1<<(irqno & 0x1F));
	  }
	  else
	  {
   		spmp_irqwake_int1mask |=  (1<<irqno);    
      }
	}	
	else
	{
	  if (irqno & 0x20)
	  {
		spmp_irqwake_int2mask &= ~(1<<(irqno & 0x1F));
	  }
	  else
	  {
   		spmp_irqwake_int1mask &=  ~(1<<irqno);    
      }
	}
	return 0;
}

#else
#define spmp8050_irq_wake NULL
#endif

static struct irq_chip spmp8050_irq_chip = {
	.name	= "SPMP8050",
	.mask	= spmp8050_mask_irq,
	.mask_ack = spmp8050_mask_irq,
	.unmask = spmp8050_unmask_irq,
	.set_wake	= spmp8050_irq_wake,	
};

void spmp_init_irq(void)
{
	unsigned int i;
	
	for (i = 0; i < NR_IRQS; i++) {
		set_irq_chip(i, &spmp8050_irq_chip);
		set_irq_handler(i, handle_level_irq);
		set_irq_flags(i, IRQF_VALID | IRQF_PROBE);
	}
}

static struct map_desc spmp8050_io_desc[] __initdata = {
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
};

static void __init spmp_map_io(void)
{	
	iotable_init( spmp8050_io_desc, ARRAY_SIZE( spmp8050_io_desc));
#ifdef CONFIG_PM
	local_flush_tlb_all();
	flush_cache_all();		
        spmp_sram_init();
#endif
	spmp_register_baseclocks(0);
    spmp_setup_clocks();
    spmp_baseclk_add();
    dumpclk();	
    spmp8050_READY = 1;
}

extern struct sys_timer spmp8050_timer;

MACHINE_START(SPMP8050, "SPMP8050")
	.phys_io	= 0x90000000,
	.io_pg_offst	= ((0xfc000000) >> 18) & 0xfffc,
	.boot_params	= 0x00200100,
	.map_io		= spmp_map_io,
	.init_irq	= spmp_init_irq,
	.init_machine	= spmp_init,
	.timer		= &spmp8050_timer,
MACHINE_END
