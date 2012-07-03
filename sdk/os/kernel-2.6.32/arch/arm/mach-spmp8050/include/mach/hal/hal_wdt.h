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
 * @file hal_wdt.h
 * @brief watchdog HAL Operation API header
 * @author zaimingmeng
 */

#ifndef _HAL_WDT_H_
#define _HAL_WDT_H_

#include <mach/hal/hal_common.h>

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
void gpHalWdtInit(void);

void gpHalWdtEnable(int enable);

void gpHalWdtSetPrescale(int value);

void gpHalWdtSetLoad(int value);

void gpHalWdtKeepAlive(void);

void gpHalWdtForceReset(void);

void gpHalWdtSetTimeout(int period);


#endif	/*_HAL_WDT_H_*/
