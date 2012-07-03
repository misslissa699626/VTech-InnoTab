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
#ifndef _GP_MS_H_
#define _GP_MS_H_

#include <mach/gp_apbdma0.h>

#define MS_CLK_SET	_IOW('D', 0x01, unsigned int)		/* Set MS clock*/
#define MS_BUS_MODE	_IOW('D', 0x02, unsigned int)		/* Set MS bus width*/
#define MS_XFR_MODE	_IOW('D', 0x03, unsigned int)		/* Set MS transfer mode*/

struct ms_dev {
	int           cd_pin;
	int           present;
	int           users;                    /* How many users */
	unsigned int  db_cnt;
	spinlock_t    lock;                     /* For mutual exclusion */
	struct request_queue    *queue;         /* The device request queue */
	struct gendisk          *gd;            /* The gendisk structure */
	struct timer_list 		timer;        	/* For simulated media changes */
	struct work_struct		init;
	struct work_struct		uninit;
	int 					handle_dma;			/* Dma handle*/
	struct task_struct		*thread;
	struct semaphore		thread_sem;
	unsigned int			flags;
	struct scatterlist		*sg;
	struct request			*req;
};

#endif /* _GP_MS_H_ */