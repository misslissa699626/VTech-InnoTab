/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2011 by Generalplus Inc.                         *
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/irq.h>

#include <asm/ptrace.h>
#include <asm/fiq.h>
#include <mach/irqs.h>
#include <mach/gp_fiq.h>
#include <linux/cache.h>
#include <asm-generic/errno.h>
#include <mach/hal/hal_vic.h>

/*****************************************************
 
*****************************************************/
#define SIZEOF_FIQ_JUMP 8

static int gpfiq_stack[1024];

struct fiq_desc fiq_desc[NR_IRQS] __cacheline_aligned_in_smp = {
	[0 ... NR_IRQS-1] = {
		.chip = &no_irq_chip,
		.lock = __SPIN_LOCK_UNLOCKED(fiq_desc->lock)
	}
};

struct fiq_desc *fiq_to_desc(unsigned int fiq)
{
	return (fiq < NR_IRQS) ? fiq_desc + fiq : NULL;
}

static void __attribute__ ((naked)) gp_FIQ_branch(void)
{
	asm __volatile__ (
		"mov pc, r8 ; "
	);
}

static void __attribute__ ((naked)) gp_fiq_handle(void)
{
	unsigned int base,fiqnr,fiqstat,pending;
	struct fiq_desc *desc;
	
	FIQ_HANDLER_ENTRY()
	
	while(1) { /* get fiq number */
		asm __volatile__ (
			"mov    %3,#0;"
			"ldr	%0,=0xfc010004;"
			"mov	%1 , #0x0;"
			"ldr	%2, [%0];"
			"ldr	%0,=0xfc020004;"
			"teq	%2, #0;"
			"ldreq	%2, [%0];"
			"moveq	%1  , #0x20;"
      "1001: tst	%2, #15;"
			"bne	1002f;"
			"add	%1, %1, #4;"
			"movs	%2, %2, lsr #4;"
			"bne	1001b;"
      "1002: tst	%2, #1;"
			"bne	1003f;"
			"add	%1, %1, #1;"
			"movs	%2, %2, lsr #1;"
		    "bne	1002b;"
      "1003: movne  %3,#1;" /* EQ will be set if no fiqs pending */
    	    : "=r" (base), "=r" (fiqnr), "=r" (fiqstat), "=r" (pending)
    	    :
		);
		if (pending) { /* fiq pending */
			desc = fiq_to_desc(fiqnr);
			desc->action.handler(fiqnr,desc->action.dev_id);
		}
		else {
			break;
		}
	}
	
	FIQ_HANDLER_END()	
}

int __must_check
request_fiq(unsigned int fiq, fiq_handler_t handler, const char *name, void *dev)
{
	struct fiq_desc *desc;
	unsigned long flags;
	
	if (!dev) {
		return -EINVAL;
	}
		
	desc = fiq_to_desc(fiq);
	
	if (!desc)
		return -EINVAL;
		
	if (desc->chip == &no_irq_chip)
		return -ENOSYS;
		
	if (!handler)
		return -EINVAL;
	
	spin_lock_irqsave(&desc->lock, flags);
	desc->fiq = fiq;
	desc->action.handler = handler;
	desc->action.name = name;
	desc->action.dev_id = dev;
	desc->action.fiq = fiq;
	gpHalVicIntSel(fiq,1);
	desc->chip->enable(fiq);
	spin_unlock_irqrestore(&desc->lock, flags);
	
	return 0;	
}
EXPORT_SYMBOL(request_fiq);

void free_fiq(unsigned int fiq)
{
	struct fiq_desc *desc = fiq_to_desc(fiq);

	if (!desc)
		return;

	desc->chip->disable(fiq);
	gpHalVicIntSel(fiq,0);
}
EXPORT_SYMBOL(free_fiq);


/**
 *	set_fiq_chip - set the fiq chip for an irq
 *	@irq:	irq number
 *	@chip:	pointer to irq chip description structure
 */
int set_fiq_chip(unsigned int fiq, struct irq_chip *chip)
{
	struct fiq_desc *desc = fiq_to_desc(fiq);
	unsigned long flags;

	if (!desc) {
		WARN(1, KERN_ERR "Trying to install chip for IRQ%d\n", fiq);
		return -EINVAL;
	}

	if (!chip)
		chip = &no_irq_chip;

	spin_lock_irqsave(&desc->lock, flags);
	desc->chip = chip;
	spin_unlock_irqrestore(&desc->lock, flags);

	return 0;
}
EXPORT_SYMBOL(set_fiq_chip);

void gp_fiq_init(void)
{
	struct pt_regs regs;
	printk("Enabling FIQ\n");
	
	local_fiq_disable();
	
	set_fiq_handler(gp_FIQ_branch, SIZEOF_FIQ_JUMP);
	
	regs.ARM_r8 = (unsigned int)gp_fiq_handle;
	regs.ARM_sp = (unsigned int)gpfiq_stack + sizeof(gpfiq_stack) - 4;
	set_fiq_regs(&regs);
	
	local_fiq_enable();
}
