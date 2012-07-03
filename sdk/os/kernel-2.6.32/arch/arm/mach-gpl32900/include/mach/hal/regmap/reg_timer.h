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
 * @file reg_timer.h
 * @brief timer/counter register define 
 * @author zaimingmeng
 */
#ifndef _REG_TIMER_H_
#define _REG_TIMER_H_

#include <mach/hardware.h>                          
#include <mach/typedef.h>

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define	LOGI_ADDR_TIMER_REG		(IO0_BASE + 0x0000)
#define LOGI_TIMER_OFFSET		(0x20)

/* definitation for Timer Controller Register*/
/* bit 0 Timer enable */
#define TMR_ENABLE	0x0001
#define TMR_DISABLE	0x0000

/* bit 1 Interrupt enable */
#define TMR_IE_DISABLE	0x0000
#define TMR_IE_ENABLE	0x0002

/* bit 2 Output enable */
#define TMR_OE_ENABLE	0x0004   
#define TMR_OE_DISABLE	0x0000

/* bit 3 Output mode of timer */
#define TMR_OE_NORMAL	0x0000  
#define TMR_OE_PWM	0x0008

/* bit4 Up/Down counting selection */
#define TMR_UD_DOWN	0x0000   
#define TMR_UD_UP	0x0010 

/* bit5 Up/Down counting control selection */
#define TMR_UDS_UD	0x0000
#define TMR_UDS_EXTUD	0x0020

/* bit6 Time output mode */
#define TMR_OM_TOGGLE	0x0000
#define TMR_OM_PULSE	0x0040

/* bit 8..9 External input active edge selection */
#define TMR_ES_PE	0x0000
#define TMR_ES_NE	0x0100
#define TMR_ES_BOTH	0x0200

/* bit 10..11 Operating mode */
#define TMR_M_FREE_TIMER	0x0000
#define TMR_M_FREE_COUNTER	0x0800
#define TMR_M_PERIOD_TIMER	0x0400
#define TMR_M_PERIOD_COUNTER	0x0c00

/* bit 14..15 Clk source */
#define TMR_CLK_SRC_PCLK		0x0000 // PCLK
#define TMR_CLK_SRC_32768		0x4000 // 32768 Hz
#define TMR_CLK_SRC_1P6875		0x8000 // 1.6875MHz

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct timerReg_s {
	volatile UINT32 tmCtr;		/* 0x0000 ~ 0x0003 timer control register */
	volatile UINT32 tmPsr;		/* 0x0004 ~ 0x0007 timer pre-scale register */
	volatile UINT32 tmLdrVlr;		/* 0x0008 ~ 0x000B timer load value register */
	volatile UINT32 tmIsr;		/* 0x000C ~ 0x000F timer interrupte register */
	volatile UINT32 tmCmp;		/* 0x0010 ~ 0x0013 timer compare register */
	
} timerReg_t;

#endif /* _REG_TIMER_H_ */


