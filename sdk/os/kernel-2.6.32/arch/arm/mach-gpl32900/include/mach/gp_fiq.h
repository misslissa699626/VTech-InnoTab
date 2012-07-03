/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Technology Co., Ltd.         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Technology Co., Ltd. All rights are reserved by Generalplus Technology*
 *  Co., Ltd. This software may only be used in accordance with the       *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Technology Co., Ltd. reserves the right to modify this    *
 *  software without notice.                                              *
 *                                                                        *
 *  Generalplus Technology Co., Ltd.                                      *
 *  19, Innovation First Road, Science-Based Industrial Park,             *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 **************************************************************************/

 /**
 * @file gp_fiq.h
 */

#ifndef _GP_FIQ_H_
#define _GP_FIQ_H_


#include <mach/typedef.h>
#include <linux/spinlock.h>
#include <linux/irqreturn.h>

#define FIQ_HANDLER_ENTRY() \
	asm __volatile__ (\
		"mov     ip, sp ;"\
		"stmdb	sp!, {r0-r12,  lr};"\
		/* !! THIS SETS THE FRAME, adjust to > sizeof locals */\
		"sub     fp, ip, #1024 ;"\
		:\
		:\
		);
		

#define FIQ_HANDLER_END() \
	asm __volatile__ (\
		"ldmia	sp!, {r0-r12, lr};"\
		/* return */\
		"subs	pc, lr, #4;"\
	);
	

typedef irqreturn_t (*fiq_handler_t)(int, void *);

struct fiqaction {
	fiq_handler_t handler;
	const char *name;
	void *dev_id;
	int fiq;
};

struct fiq_desc {
	unsigned int		fiq;
	struct fiqaction	action;
	struct irq_chip		*chip;
	spinlock_t		    lock;
};

extern int __must_check request_fiq(unsigned int fiq, fiq_handler_t handler, const char *name, void *dev);
extern void free_fiq(unsigned int fiq);
extern void gp_fiq_init(void);
extern int set_fiq_chip(unsigned int fiq, struct irq_chip *chip);

#endif	/*_GP_FIQ_H_*/
