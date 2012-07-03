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
 * @file hal_timer.h
 * @brief watchdog HAL Operation API header
 * @author zaimingmeng
 */

#ifndef _HAL_TIMER_H_
#define _HAL_TIMER_H_

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
void gpHalTimerInit(int id);

int gpHalTimerGetBaseClk(void);

void gpHalTimerSetCtrl(int id, int value);

void gpHalTimerEn(int id, int enable);

void gpHalTimerIntEn(int id, int enable);

void gpHalTimerOeEnSet(int id, int enable);

void gpHalTimerOeSet(int id, int mode);

void gpHalTimerUdSet(int id, int mode);

void gpHalTimerUdsSet(int id, int mode);

void gpHalTimerOmSet(int id, int mode);

void gpHalTimerEsSet(int id, int mode);

void gpHalTimerMSet(int id, int mode);

void gpHalTimerLoadSet(int id, int value);

void gpHalTimerLoadGet(int id, int *value);

void gpHalTimerCmpSet(int id, int value);

void gpHalTimerCmpGet(int id, int *value);

void gpHalTimerPrescaleSet(int id, int value);

void gpHalTimerPrescaleGet(int id, int *value);

void gpHalTimerInterruptSet(int id, int value);

void gpHalTimerInterruptGet(int id, int *value);


#endif	/*_HAL_TIMER_H_*/
