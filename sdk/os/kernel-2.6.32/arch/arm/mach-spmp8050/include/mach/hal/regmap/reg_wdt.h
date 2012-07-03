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
 * @file reg_wdt.h
 * @brief watchdog register define 
 * @author zaimingmeng
 */
#ifndef _REG_WDT_H_
#define _REG_WDT_H_

#include <mach/hardware.h>                          
#include <mach/typedef.h>

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define	LOGI_ADDR_WDT_REG		(IO0_BASE + 0x1000)

/* For Watch Dog Timer Control Timer Register (CTR) */
/* bit 0 Timer enable */
#define WDT_ENABLE     0x0001
#define WDT_DISABLE    0x0000

/* bit 1 Interrupt enable */
#define WDT_IE_ENABLE   0x0002
#define WDT_IE_DISABLE  0x0000

/* bit 2 Output enable */
#define WDT_OE_ENABLE  0x0004
#define WDT_OE_DISABLE 0x0000

/* bit 3 Reset enable */
#define WDT_RE_ENABLE   0x0008
#define WDT_RE_DISABLE  0x0000

/* bit 4 Output mode of timer */
#define WDT_PWMON_WDT   0x0000
#define WDT_PWMON_PWM   0x0010

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct wdtReg_s {
	volatile UINT32 wdtCtr;		/* 0x0000 ~ 0x0003 control register */
	volatile UINT32 wdtPsr;		/* 0x0004 ~ 0x0007 pre-scale register */
	volatile UINT32 wdtLdr;		/* 0x0008 ~ 0x000B load value register */
	volatile UINT32 wdtVlr;		/* 0x000C ~ 0x000F current counter value register */
	volatile UINT32 wdtCmp;		/* 0x0010 ~ 0x0013 compare register */
	
} wdtReg_t;

#endif /* _REG_WDT_H_ */
