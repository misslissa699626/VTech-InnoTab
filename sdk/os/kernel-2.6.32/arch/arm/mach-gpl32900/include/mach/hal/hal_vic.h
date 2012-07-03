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
 * @file hal_vic.h
 * @brief VIC HAL Operation API header
 * @author 
 */

#ifndef _HAL_VIC_H_
#define _HAL_VIC_H_

#include <mach/common.h>

#define VIC0 0
#define VIC1 1

#define VIC_PRIORITY0  0
#define VIC_PRIORITY1  1
#define VIC_PRIORITY2  2
#define VIC_PRIORITY3  3
#define VIC_PRIORITY4  4
#define VIC_PRIORITY5  5
#define VIC_PRIORITY6  6
#define VIC_PRIORITY7  7
#define VIC_PRIORITY8  8
#define VIC_PRIORITY9  9
#define VIC_PRIORITY10 10
#define VIC_PRIORITY11 11
#define VIC_PRIORITY12 12
#define VIC_PRIORITY13 13
#define VIC_PRIORITY14 14
#define VIC_PRIORITY15 15

/**
* @brief Set priority of interrupt source
* @param[vicId] 0:VIC0/1:VIC1 
* @param[vicSrc] interrupt source number(0~31)
* @param[priority] VIC_PRIORITY0~VIC_PRIORITY15
* @return 0:success/-1:failed
*/
SINT32 gpHalVicSetPriority(
	UINT32 vicId,
	UINT32 vicSrc,
	UINT32 priority
);

/**
* @brief Get priority of interrupt source
* @param[vicId] 0:VIC0/1:VIC1 
* @param[vicSrc] interrupt source number(0~31)
* @return -1:failed/VIC_PRIORITY0~VIC_PRIORITY15
*/
SINT32 gpHalVicGetPriority(
	UINT32 vicId,
	UINT32 vicSrc
);

/**
* @brief Select type of interrupt
* @param[vicSrc] interrupt source (0~63)
* @param[type] 0:IRQ/1:FIQ 
* @return -1:failed/0:success
*/
SINT32 gpHalVicIntSel(
	UINT32 vicSrc,
	UINT32 type
);

#endif