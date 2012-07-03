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
 * @file    gp_pm.c
 * @brief   Implement of power manager
 * @author  Roger Hsu
 */
#include <mach/kernel.h>

/** @todo using platform structure to replace spmp_cpu_pm_fns */
#include <mach/pm.h>

/**@todo move to hal */
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/regs-sd.h>
#include <mach/spmp_gpio.h>
#include <mach/regs-gpio.h>
#include <mach/gp_sram.h>
#include <mach/hal/regmap/reg_scu.h>
//struct spmp_cpu_pm_fns *spmp_cpu_pm_fns;
static gp_board_system_t *gp_pm_func;
static unsigned long *sleep_save;

/**@todo move to hal */
extern unsigned long gp_irqwake_int1mask;
extern unsigned long gp_irqwake_int2mask;

#define SAVE(x)		sleep_save[SLEEP_SAVE_##x] = x
#define RESTORE(x)	x = sleep_save[SLEEP_SAVE_##x]
#define MMP_IRQ_VIC0_BASE			IO0_ADDRESS(0x10000)
#define MMP_IRQ_VIC1_BASE			IO0_ADDRESS(0x20000)
#define MMP_IRQENABLE				0x10
#define MMP_IRQENABLECLEAR		0x14


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
/*
 * List of global PXA peripheral registers to preserve.
 * More ones like CP and general purpose register values are preserved
 * with the stack pointer in sleep.S.
 */
enum {
	SLEEP_SAVE_SCUA_A_PERI_CLKEN,
	SLEEP_SAVE_SCUB_B_PERI_CLKEN,
	SLEEP_SAVE_SCUC_C_PERI_CLKEN,
	SLEEP_SAVE_COUNT
};
static int save_count	= SLEEP_SAVE_COUNT;

extern void (*gp_sram_suspend)(void);
static void gp_cpu_pm_suspend(void)
{
    extern void  gp_cpu_suspend(void);
    unsigned long gp_irqwakeup_int1mask;
    unsigned long gp_irqwakeup_int2mask;
	local_irq_disable();
	local_fiq_disable();
      gp_irqwakeup_int1mask = CYG_DEVICE_IRQ0_EnableSet;
      gp_irqwakeup_int2mask = CYG_DEVICE_IRQ1_EnableSet;
      CYG_DEVICE_IRQ0_EnableClear = CYG_DEVICE_IRQ0_EnableSet & gp_irqwake_int1mask;
      CYG_DEVICE_IRQ1_EnableClear = CYG_DEVICE_IRQ1_EnableSet & gp_irqwake_int2mask;
	flush_cache_all();
	gp_sram_suspend();
      CYG_DEVICE_IRQ0_EnableSet = gp_irqwakeup_int1mask;
      CYG_DEVICE_IRQ1_EnableSet = gp_irqwakeup_int2mask;
	local_irq_enable();
	local_fiq_enable();
	printk("suspend = %p\n",gp_sram_suspend);
}

static void gp_pm_cpu_save(unsigned long *sleep_save)
{
	/**@todo move to hal */
	SAVE(SCUA_A_PERI_CLKEN);
	SAVE(SCUB_B_PERI_CLKEN);
	SAVE(SCUC_C_PERI_CLKEN);
}

static void gp_pm_cpu_restore(unsigned long *sleep_save)
{
	/**@todo move to hal */
	RESTORE(SCUA_A_PERI_CLKEN);
	RESTORE(SCUB_B_PERI_CLKEN);
	RESTORE(SCUC_C_PERI_CLKEN);
}

static void gp_pm_cpu_enter(suspend_state_t state)
{
	printk("[%s][%d] run\n", __FUNCTION__, __LINE__);
	switch (state) {
	case PM_SUSPEND_MEM:
		/**@todo move to hal */
	    gp_cpu_pm_suspend();
		printk("exit suspend \n");
		break;
	}
}

int gp_pm_enter(suspend_state_t state)
{
	unsigned long sleep_save_checksum = 0, checksum = 0;
	int i;
	printk("[%s][%d] run\n", __FUNCTION__, __LINE__);

	/* skip registers saving for standby */
	if (state != PM_SUSPEND_STANDBY) {
		//spmp_cpu_pm_fns->save(sleep_save);
		gp_pm_cpu_save(sleep_save);
		/* before sleeping, calculate and save a checksum */
		//for (i = 0; i < spmp_cpu_pm_fns->save_count - 1; i++)
		for (i = 0; i < save_count - 1; i++)
			sleep_save_checksum += sleep_save[i];
	}

	/* *** go zzz *** */	
	//spmp_cpu_pm_fns->enter(state);
	gp_pm_cpu_enter(state);
	cpu_init();

	if (state != PM_SUSPEND_STANDBY) {
		/* after sleeping, validate the checksum */
		//for (i = 0; i < spmp_cpu_pm_fns->save_count - 1; i++)
		for (i = 0; i < save_count - 1; i++)
			checksum += sleep_save[i];

		/* if invalid, display message and wait for a hardware reset */
		if (checksum != sleep_save_checksum) {
			while (1)
				//spmp_cpu_pm_fns->enter(state);
				gp_pm_cpu_enter(state);
		}
		//spmp_cpu_pm_fns->restore(sleep_save);
		gp_pm_cpu_restore(sleep_save);
	}

	printk("*** made it back from resume\n");

	return 0;
}

EXPORT_SYMBOL_GPL(gp_pm_enter);

unsigned long sleep_phys_sp(void *sp)
{
	return virt_to_phys(sp);
}

static int gp_pm_begin(suspend_state_t state)
{
	printk("[%s][%d] run\n", __FUNCTION__, __LINE__);
	gp_pm_func = gp_board_get_config("sys_pwr",gp_board_system_t);
	if (gp_pm_func == NULL) {
		printk(KERN_ERR "Can not get System Power config function\n");
		return -EINVAL;
	}

	return 0;
}

static int gp_pm_prepare(void)
{
	int ret = 0;

	printk("[%s][%d] run\n", __FUNCTION__, __LINE__);

	if (gp_pm_func->prepare) {
		ret = gp_pm_func->prepare();
	}

	return ret;
}

static void gp_pm_finish(void)
{
	printk("[%s][%d] run\n", __FUNCTION__, __LINE__);
	if (gp_pm_func->finish) {
		gp_pm_func->finish();
	}
}

static void gp_pm_end(void)
{
	//do not thing now
	return;
}

static struct platform_suspend_ops gp_pm_ops = {
	.valid		= suspend_valid_only_mem,
	.begin		= gp_pm_begin,
	.enter		= gp_pm_enter,
	.prepare	= gp_pm_prepare,
	.finish		= gp_pm_finish,
	.end		= gp_pm_end,
};

static int __init gp_pm_init(void)
{
	//if (!spmp_cpu_pm_fns) {
	//	printk(KERN_ERR "no valid spmp_cpu_pm_fns defined\n");
	//	return -EINVAL;
	//}

	//sleep_save = kmalloc(spmp_cpu_pm_fns->save_count * sizeof(unsigned long), GFP_KERNEL);
	sleep_save = kmalloc(save_count * sizeof(unsigned long), GFP_KERNEL);
	if (!sleep_save) {
		printk(KERN_ERR "failed to alloc memory for pm save\n");
		return -ENOMEM;
	}

	suspend_set_ops(&gp_pm_ops);
	return 0;
}

device_initcall(gp_pm_init);

