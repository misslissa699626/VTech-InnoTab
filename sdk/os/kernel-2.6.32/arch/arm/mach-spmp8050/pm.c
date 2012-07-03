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

struct spmp_cpu_pm_fns *spmp_cpu_pm_fns;
static gp_board_system_t *gp_pm_func;
static unsigned long *sleep_save;

int spmp_pm_enter(suspend_state_t state)
{
	unsigned long sleep_save_checksum = 0, checksum = 0;
	int i;

	/* skip registers saving for standby */
	if (state != PM_SUSPEND_STANDBY) {
		spmp_cpu_pm_fns->save(sleep_save);
		/* before sleeping, calculate and save a checksum */
		for (i = 0; i < spmp_cpu_pm_fns->save_count - 1; i++)
			sleep_save_checksum += sleep_save[i];
	}

	/* *** go zzz *** */	
	spmp_cpu_pm_fns->enter(state);
	cpu_init();

	if (state != PM_SUSPEND_STANDBY) {
		/* after sleeping, validate the checksum */
		for (i = 0; i < spmp_cpu_pm_fns->save_count - 1; i++)
			checksum += sleep_save[i];

		/* if invalid, display message and wait for a hardware reset */
		if (checksum != sleep_save_checksum) {
			while (1)
				spmp_cpu_pm_fns->enter(state);
		}
		spmp_cpu_pm_fns->restore(sleep_save);
	}

	printk("*** made it back from resume\n");

	return 0;
}

EXPORT_SYMBOL_GPL(spmp_pm_enter);

unsigned long sleep_phys_sp(void *sp)
{
	return virt_to_phys(sp);
}

static int spmp_pm_begin(suspend_state_t state)
{
	gp_pm_func = gp_board_get_config("sys_pwr",gp_board_system_t);
	if (gp_pm_func == NULL) {
		printk(KERN_ERR "Can not get System Power config function\n");
		return -EINVAL;
	}

	return 0;
}

static int spmp_pm_prepare(void)
{
	int ret = 0;

	if (gp_pm_func->prepare) {
		ret = gp_pm_func->prepare();
	}

	return ret;
}

static void spmp_pm_finish(void)
{
	if (gp_pm_func->finish) {
		gp_pm_func->finish();
	}
}

static void spmp_pm_end(void)
{
	//do not thing now
	return;
}

static struct platform_suspend_ops spmp_pm_ops = {
	.valid		= suspend_valid_only_mem,
	.begin		= spmp_pm_begin,
	.enter		= spmp_pm_enter,
	.prepare	= spmp_pm_prepare,
	.finish		= spmp_pm_finish,
	.end		= spmp_pm_end,
};

static int __init spmp_pm_init(void)
{
	if (!spmp_cpu_pm_fns) {
		printk(KERN_ERR "no valid spmp_cpu_pm_fns defined\n");
		return -EINVAL;
	}

	sleep_save = kmalloc(spmp_cpu_pm_fns->save_count * sizeof(unsigned long),
			     GFP_KERNEL);
	if (!sleep_save) {
		printk(KERN_ERR "failed to alloc memory for pm save\n");
		return -ENOMEM;
	}

	suspend_set_ops(&spmp_pm_ops);
	return 0;
}

device_initcall(spmp_pm_init);

