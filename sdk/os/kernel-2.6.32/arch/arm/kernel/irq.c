/*
 *  linux/arch/arm/kernel/irq.c
 *
 *  Copyright (C) 1992 Linus Torvalds
 *  Modifications for ARM processor Copyright (C) 1995-2000 Russell King.
 *
 *  Support for Dynamic Tick Timer Copyright (C) 2004-2005 Nokia Corporation.
 *  Dynamic Tick Timer written by Tony Lindgren <tony@atomide.com> and
 *  Tuukka Tikkanen <tuukka.tikkanen@elektrobit.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  This file contains the code used by various IRQ handling routines:
 *  asking for different IRQ's should be done through these routines
 *  instead of just grabbing them. Thus setups with different IRQ numbers
 *  shouldn't result in any weird surprises, and installing new handlers
 *  should be easier.
 *
 *  IRQ's are in fact implemented a bit like signal handlers for the kernel.
 *  Naturally it's not a 1:1 relation, but there are similarities.
 */
#include <linux/kernel_stat.h>
#include <linux/module.h>
#include <linux/signal.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/smp.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/kallsyms.h>
#include <linux/proc_fs.h>

#include <asm/system.h>
#include <asm/mach/irq.h>
#include <asm/mach/time.h>
#include <mach/hal/regmap/reg_vic.h>

/*
 * No architecture-specific irq_finish function defined in arm/arch/irqs.h.
 */
#ifndef irq_finish
#define irq_finish(irq) do { } while (0)
#endif

void (*init_arch_irq)(void) __initdata = NULL;
unsigned long irq_err_count;

int show_interrupts(struct seq_file *p, void *v)
{
	int i = *(loff_t *) v, cpu;
	struct irqaction * action;
	unsigned long flags;

	if (i == 0) {
		char cpuname[12];

		seq_printf(p, "    ");
		for_each_present_cpu(cpu) {
			sprintf(cpuname, "CPU%d", cpu);
			seq_printf(p, " %10s", cpuname);
		}
		seq_putc(p, '\n');
	}

	if (i < NR_IRQS) {
		spin_lock_irqsave(&irq_desc[i].lock, flags);
		action = irq_desc[i].action;
		if (!action)
			goto unlock;

		seq_printf(p, "%3d: ", i);
		for_each_present_cpu(cpu)
			seq_printf(p, "%10u ", kstat_irqs_cpu(i, cpu));
		seq_printf(p, " %10s", irq_desc[i].chip->name ? : "-");
		seq_printf(p, "  %s", action->name);
		for (action = action->next; action; action = action->next)
			seq_printf(p, ", %s", action->name);

		seq_putc(p, '\n');
unlock:
		spin_unlock_irqrestore(&irq_desc[i].lock, flags);
	} else if (i == NR_IRQS) {
#ifdef CONFIG_FIQ
		show_fiq_list(p, v);
#endif
#ifdef CONFIG_SMP
		show_ipi_list(p);
		show_local_irqs(p);
#endif
		seq_printf(p, "Err: %10lu\n", irq_err_count);
	}
	return 0;
}

#define IRQ_USE_VIC_PRIORITY 0

#if IRQ_USE_VIC_PRIORITY
#define VIC0_ADDRESS		     (*(volatile unsigned int*)(IO0_BASE + 0x10F00)) 
#define VIC1_ADDRESS  		     (*(volatile unsigned int*)(IO0_BASE + 0x20F00))
#endif

#define IRQ_DEBUG 1


#if IRQ_DEBUG == 0
#define IRQ_INTERVER_CHECK  0
#define IRQ_CLEAN_CHECK  	0
#define IRQ_STATUS_CHECK  	0
#define IRQ_GPIODEBUGK  	0
#endif

#if IRQ_DEBUG
#define IRQ_INTERVER_CHECK  0
#define IRQ_CLEAN_CHECK  	1
#define IRQ_STATUS_CHECK  	1
#define IRQ_GPIODEBUGK  	0

#define DEBUGPRN(fmt,args...) printk(fmt,##args)

#define VIC0_SOFTINT_ADDR 	 	 (IO0_BASE + 0x10018)

#define VIC0_IRQSTATUS           (*(volatile unsigned int*)(IO0_BASE + 0x10000))
#define VIC1_IRQSTATUS           (*(volatile unsigned int*)(IO0_BASE + 0x20000))
#define VIC0_FIQSTATUS           (*(volatile unsigned int*)(IO0_BASE + 0x10004))
#define VIC1_FIQSTATUS           (*(volatile unsigned int*)(IO0_BASE + 0x20004))
#define VIC0_IRQRAWSTATUS		 (*(volatile unsigned int*)(IO0_BASE + 0x10008)) 
#define VIC1_IRQRAWSTATUS		 (*(volatile unsigned int*)(IO0_BASE + 0x20008))
#define VIC0_INTSELECT		     (*(volatile unsigned int*)(IO0_BASE + 0x1000C)) 
#define VIC1_INTSELECT    		 (*(volatile unsigned int*)(IO0_BASE + 0x2000C))
#define VIC0_INTENABLE		     (*(volatile unsigned int*)(IO0_BASE + 0x10010)) 
#define VIC1_INTENABLE   		 (*(volatile unsigned int*)(IO0_BASE + 0x20010))
#define VIC0_INTENCLEAR		     (*(volatile unsigned int*)(IO0_BASE + 0x10014)) 
#define VIC1_INTENCLEAR   		 (*(volatile unsigned int*)(IO0_BASE + 0x20014))
#define VIC0_SOFTINT		     (*(volatile unsigned int*)(IO0_BASE + 0x10018)) 
#define VIC1_SOFTINT   		     (*(volatile unsigned int*)(IO0_BASE + 0x20018))
#define VIC0_SOFTINTCLEAR		 (*(volatile unsigned int*)(IO0_BASE + 0x1001C)) 
#define VIC1_SOFTINTCLEAR  		 (*(volatile unsigned int*)(IO0_BASE + 0x2001C))
#define VIC0_PROTECTION		     (*(volatile unsigned int*)(IO0_BASE + 0x10020)) 
#define VIC1_PROTECTION  		 (*(volatile unsigned int*)(IO0_BASE + 0x20020))
#define VIC0_PRIORITYMASK		 (*(volatile unsigned int*)(IO0_BASE + 0x10024)) 
#define VIC1_PRIORITYMASK  		 (*(volatile unsigned int*)(IO0_BASE + 0x20024))

/*
#define USBD_IRQSTATUS           (*(volatile unsigned int*)(IO3_BASE + 0x06084))
#define USBD_IRQMASK             (*(volatile unsigned int*)(IO3_BASE + 0x06088))
*/

#define MODE_MASK	0x0000001f
#define USR_MODE	0x00000010
	
	
char * isrname[] = { "sdma0", "sdma1", "xdma0" , "xdma1", "xdma2", "xdma3" , "reserved", "timer0" ,
                     "timer1", "timer2", "wdt0" , "wdt1", "wdt2", "i2stx" , "i2srx", "cdsp" ,
                     "usbD", "ehci", "ohci" , "csi", "ppu", "tvout" , "scaler", "lcd" ,
                     "apbdma0", "apbdma1", "apbdma2" , "apbdma3", "dsp0", "dsp1" , "dsp3", "dsp4" ,
                     "gpio0", "gpio1", "pwrc" , "i2c0", "i2c1", "rtc" , "pwrc", "xdma" ,
                     "sd0", "sd1", "mipi" , "bch", "ovg", "gpio2" , "gpio3", "reserved" ,
                     "cir" ,"uart", "nand", "spu" , "ssp0", "ssp1", "ms" , "i2c", 
                     "uart", "aes", "uart" , "sar", "apbdmac0", "apbdmac1" , "apbdmac2", "apbdmac3" ,
                   };
#if IRQ_INTERVER_CHECK
#include <mach/regs-timer.h>
#include <mach/hal/regmap/reg_scu.h>
#define SUMMARY_COUNTER 4000

typedef struct isrcount
{
    unsigned int max;
    unsigned int min;
    unsigned int sum;
    unsigned int count;
} ISRCOUNT;


static ISRCOUNT isr[NR_IRQS];
static unsigned int isrcounter = 0xFFFFFFFF ;

#define SCUB_TIMER0_4_CLKENABLE ((0x01<<9))
static void gpl32900_timer4_setup (void)
{
    int i = 0 ;
  
    TMCTR_4 = 0;
    SCUB_B_PERI_CLKEN |= SCUB_TIMER0_4_CLKENABLE;
    TMPSR_4  = 99 ; // 1 ms
    TMCTR_4 = TMR_ENABLE | TMR_UD_UP | TMR_M_FREE_TIMER ;

    memset(isr,0x00,sizeof(ISRCOUNT)*NR_IRQS);
    for ( i = 0; i < NR_IRQS; i++)
        isr[i].min = 0xFFFF;

}
#endif
#endif /* end of #if IRQ_DEBUG */
/*
 * do_IRQ handles all hardware IRQ's.  Decoded IRQs should not
 * come via this function.  Instead, they should provide their
 * own 'handler'
 */
asmlinkage void __exception asm_do_IRQ(unsigned int irq, struct pt_regs *regs)
{
	struct pt_regs *old_regs = set_irq_regs(regs);
	
#if IRQ_DEBUG

#if IRQ_INTERVER_CHECK
    unsigned int timer0 ; 
    unsigned int timer1 ;
#endif
	
#if IRQ_STATUS_CHECK	
	unsigned int vic0_status = VIC0_IRQSTATUS ;	
	unsigned int vic1_status = VIC1_IRQSTATUS ;	
	unsigned int vic0_select = VIC0_INTSELECT ;	
	unsigned int vic1_select = VIC1_INTSELECT ;	
	unsigned int vic0_enable = VIC0_INTENABLE ;	
	unsigned int vic1_enable = VIC1_INTENABLE ;	
	
	unsigned int temp;
	unsigned int irqnr = 0 ;
	unsigned int status = 0;
	
	int i = 0 ,j=0, rt_irq;
	
#if 0
	long mode = regs->ARM_cpsr & MODE_MASK;

	if ( mode == USR_MODE ) {
		DEBUGPRN(KERN_ALERT "ISR in USER MODE\n");
	}
#endif

#if IRQ_GPIODEBUGK
#ifndef READ32
#define READ32(_reg_)           (*((volatile UINT32 *)(_reg_)))
#endif

#ifndef WRITE32
#define WRITE32(_reg_, _value_) (*((volatile UINT32 *)(_reg_)) = (_value_))
#endif

    WRITE32(0xFC005088,((READ32(0xFC005088) & 0xFFFFFFFC) | 0x00000002)); //GID
    WRITE32(0xFC00A000,  READ32(0xFC00A000) | 0x40000000); //GPIO mode
    WRITE32(0xFC00A020,  READ32(0xFC00A020) & 0xBFFFFFFF); //Output mode
    WRITE32(0xFC00A010 , READ32(0xFC00A010) & 0xBFFFFFFF); //Output Low
#endif
	
	irq_enter();
	
    /* Check Software INT */
	for ( i = 0 ; i < 2 ; i++ ) {
		temp = *(volatile unsigned int*)(VIC0_SOFTINT_ADDR + 0x10000*i) ;
		if ( temp != 0 ) {
			DEBUGPRN(KERN_ALERT "[ERROR] VIC%d SOFTINT(0x%x) not zero\n",i,temp);
		}	
	}
	
	/* Check IRQ INT */
	if ( vic0_select & vic0_status ) {
		DEBUGPRN(KERN_ALERT "[ERROR] VIC0 SELECT(0x%x),STATUS(0x%x)\n",vic0_select,vic0_status);
	}
	if ( vic1_select & vic1_status ) {
		DEBUGPRN(KERN_ALERT "[ERROR] VIC1 SELECT(0x%x),STATUS(0x%x)\n",vic1_select,vic1_status);
	}
	if ( (~vic0_enable) & vic0_status ) {
		DEBUGPRN(KERN_ALERT "[ERROR] VIC0 ENABLE(0x%x),STATUS(0x%x)\n",vic0_enable,vic0_status);
	}
	if ( (~vic1_enable) & vic1_status ) {
		DEBUGPRN(KERN_ALERT "[ERROR] VIC1 ENABLE(0x%x),STATUS(0x%x)\n",vic1_enable,vic1_status);
	}
	vic0_status = ( vic0_status & vic0_enable ) & (~vic0_select) ;
	vic1_status = ( vic1_status & vic1_enable ) & (~vic1_select) ;
	
	if (( vic0_status == 0 ) && ( vic1_status == 0 ) ) {
		DEBUGPRN(KERN_ALERT "[ERROR] NO IRQ Status , but have INT\n");
		goto IRQ_CHECK_END;
	}
	
	i = 0 ;	
	if ( vic0_status == 0 ) {
		status = vic1_status ;
		irqnr = 32 ;
	} else {
		status = vic0_status ;
		irqnr = 0 ;	
	}
	
	if (( status & 0xFFFF ) == 0 ) {
		i = 16 ;
	} else if (( status & 0xFF ) == 0 ) {
		i = 8 ;
	}
	
RE_CHECK_IRQ:
	rt_irq = 0;	
	/* Check Audio */	
	
	if ( irqnr == 0 ) {
#if 1
		temp = VIC0_IRQSTATUS & ( 1 << IRQ_APBDMA_A_CH0 ) & (~vic0_select) ;
		if ( temp != 0 ) {
			irq = IRQ_APBDMA_A_CH0 ;
			status = status & ( ~(1 << IRQ_APBDMA_A_CH0) ) ;
			rt_irq = 1;
			goto REALTIME_IRQ ;
		}
		temp = VIC0_IRQSTATUS & ( 1 << IRQ_APBDMA_A_CH1 ) & (~vic0_select) ;
		if ( temp != 0 ) {
			irq = IRQ_APBDMA_A_CH1 ;
			status = status & ( ~(1 << IRQ_APBDMA_A_CH1) ) ;
			rt_irq = 1;
			goto REALTIME_IRQ ;
		}
#endif	
		temp = VIC0_IRQSTATUS & ( 1 << IRQ_PIU_CM ) & (~vic0_select) ;
		if ( temp != 0 ) {
			irq = IRQ_PIU_CM ;
			status = status & ( ~(1 << IRQ_PIU_CM) ) ;
			rt_irq = 1;
			goto REALTIME_IRQ ;
		}
	}	
	while (1) {
		if ( i < 28 ) {
			temp = 0xF << i ;
			if ( ( status & temp ) != 0 )
				break;
			i = i + 4 ;
		} else
			break;
	}
	j=i;
	while (1) {
		temp = 1 << i ;
		if (i>31) {
			printk(KERN_WARNING"i=%d, %d\n",i, temp);
		}
		if ( ( status & temp ) != 0 )
			break;
		i = i + 1 ;
	}
	irq = irqnr + i ;
	status = status & ( ~temp ) ;
	
	//DEBUGPRN(KERN_ALERT "IRQ%2u %s\n",irq,isrname[irq]);
	
REALTIME_IRQ:
	
#endif	

    
#if IRQ_INTERVER_CHECK
    timer0 = TMLDR_4 ; 
    timer1 = 0 ;
    if ( isrcounter == 0xFFFFFFFF ) {
        gpl32900_timer4_setup();
        timer0 = TMLDR_4 ;
    }
#endif

#endif /* end of #if IRQ_DEBUG */

	/*
	 * Some hardware gives randomly wrong interrupts.  Rather
	 * than crashing, do something sensible.
	 */
    if (unlikely(irq >= NR_IRQS)) {
    	printk(KERN_WARNING "temp =%d \n", temp);
    	printk(KERN_WARNING "j =%d \n", j);
		if (printk_ratelimit())
			printk(KERN_WARNING "Bad IRQ%u\n", irq);
		ack_bad_irq(irq);
	} else {
		generic_handle_irq(irq);
	}
	
	
#if IRQ_DEBUG
	
#endif /* end of #if IRQ_DEBUG */


#if IRQ_USE_VIC_PRIORITY
	if (irq < 32) {
		VIC0_ADDRESS = 0x0;
	}
	else {
		VIC1_ADDRESS = 0x0;  
	}
#endif
	
#if IRQ_DEBUG
#if IRQ_INTERVER_CHECK
    timer1 = TMLDR_4 ;
    if ( timer1 < timer0 ) {
        timer1 = 0xFFFF - timer0 + timer1 ;
    } else {
        timer1 = timer1 - timer0 ;
    }
    
    if ( isr[irq].max < timer1 ) {
        isr[irq].max = timer1 ;
    }

    if ( isr[irq].min  > timer1 ) {
        isr[irq].min = timer1 ;
    }

    isr[irq].sum = isr[irq].sum + timer1 ;
    isr[irq].count  = isr[irq].count + 1 ;
    isrcounter = isrcounter + 1 ;
    if ( isrcounter == SUMMARY_COUNTER ) {
        DEBUGPRN(KERN_ALERT "\n");
        DEBUGPRN(KERN_ALERT "IRQ# {count,max,avg,min}us,clk_arm_apb==100MHz\n");
        for ( timer0 = 0 ; timer0 < NR_IRQS ; timer0++ ) {
            if ( isr[timer0].count > 0) {
                DEBUGPRN(KERN_ALERT "IRQ%2u{%5u,%5u,%5u,%5u} %s\n", timer0  , isr[timer0].count , isr[timer0].max , isr[timer0].sum  / isr[timer0].count, isr[timer0].min , isrname[timer0]);
            }
        }
        memset(isr,0x00,sizeof(ISRCOUNT)*NR_IRQS);
        for ( isrcounter = 0; isrcounter < NR_IRQS; isrcounter++)
            isr[isrcounter].min = 0xFFFF;

        isrcounter = 0 ;
        /* set TCK to low */
        if ( ( SCUB_PIN_MUX & 0x1 ) == 0 ) { /* disable ICE, JTAG pin will return to IOB[25:20] */
            DEBUGPRN(KERN_ALERT "[ERROR] ====> Somewhere check SCUB_PIN_MUX\n");
        } else if ( SCUB_GPIO1_IE & 0x3F00000 ) { /* IOB[25:20] input enable, always low of IO status */
            DEBUGPRN(KERN_ALERT "[ERROR] ====> Somewhere check SCUB_GPIO1_IE\n");
        }

    }
#endif


#if IRQ_STATUS_CHECK

	if ( status != 0 ) {
		if(!rt_irq)
			i = i + 1 ;
						
		goto RE_CHECK_IRQ;
	} else {
		if ( ( irqnr == 0 ) && ( vic1_status != 0 )) {
			irqnr = 32 ;
			i = 0 ;	
			status = vic1_status ;
			if (( status & 0xFFFF ) == 0 ) {
				i = 16 ;
			} else if (( status & 0xFF ) == 0 ) {
				i = 8 ;
			}
			goto RE_CHECK_IRQ;
		}
	}

IRQ_CHECK_END:

	/* AT91 specific workaround */
	irq_finish(irq);

	irq_exit();

#if IRQ_GPIODEBUGK
    WRITE32(0xFC00A010 , READ32(0xFC00A010) | 0x40000000); //Output High
    __asm__ __volatile__ ( " nop");
    __asm__ __volatile__ ( " nop");
    __asm__ __volatile__ ( " nop");
    __asm__ __volatile__ ( " nop");
    __asm__ __volatile__ ( " nop");
#endif

#endif	
	
#endif /* end of #if IRQ_DEBUG */	

	set_irq_regs(old_regs);
}

void set_irq_flags(unsigned int irq, unsigned int iflags)
{
	struct irq_desc *desc;
	unsigned long flags;

	if (irq >= NR_IRQS) {
		printk(KERN_ERR "Trying to set irq flags for IRQ%d\n", irq);
		return;
	}

	desc = irq_desc + irq;
	spin_lock_irqsave(&desc->lock, flags);
	desc->status |= IRQ_NOREQUEST | IRQ_NOPROBE | IRQ_NOAUTOEN;
	if (iflags & IRQF_VALID)
		desc->status &= ~IRQ_NOREQUEST;
	if (iflags & IRQF_PROBE)
		desc->status &= ~IRQ_NOPROBE;
	if (!(iflags & IRQF_NOAUTOEN))
		desc->status &= ~IRQ_NOAUTOEN;
	spin_unlock_irqrestore(&desc->lock, flags);
}

void __init init_IRQ(void)
{
	int irq;

	for (irq = 0; irq < NR_IRQS; irq++)
		irq_desc[irq].status |= IRQ_NOREQUEST | IRQ_NOPROBE;

	init_arch_irq();
}

#ifdef CONFIG_HOTPLUG_CPU

static void route_irq(struct irq_desc *desc, unsigned int irq, unsigned int cpu)
{
	pr_debug("IRQ%u: moving from cpu%u to cpu%u\n", irq, desc->node, cpu);

	spin_lock_irq(&desc->lock);
	desc->chip->set_affinity(irq, cpumask_of(cpu));
	spin_unlock_irq(&desc->lock);
}

/*
 * The CPU has been marked offline.  Migrate IRQs off this CPU.  If
 * the affinity settings do not allow other CPUs, force them onto any
 * available CPU.
 */
void migrate_irqs(void)
{
	unsigned int i, cpu = smp_processor_id();

	for (i = 0; i < NR_IRQS; i++) {
		struct irq_desc *desc = irq_desc + i;

		if (desc->node == cpu) {
			unsigned int newcpu = cpumask_any_and(desc->affinity,
							      cpu_online_mask);
			if (newcpu >= nr_cpu_ids) {
				if (printk_ratelimit())
					printk(KERN_INFO "IRQ%u no longer affine to CPU%u\n",
					       i, cpu);

				cpumask_setall(desc->affinity);
				newcpu = cpumask_any_and(desc->affinity,
							 cpu_online_mask);
			}

			route_irq(desc, i, newcpu);
		}
	}
}
#endif /* CONFIG_HOTPLUG_CPU */
