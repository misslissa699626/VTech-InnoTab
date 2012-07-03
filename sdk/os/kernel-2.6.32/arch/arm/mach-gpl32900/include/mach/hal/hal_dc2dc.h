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
 *  Generalplus Inc.                                                      *
 *  3F, No.8, Dusing Rd., Hsinchu Science Park,                           *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
 
/**
 * @file hal_dc2dc.h
 * @brief DC2DC HAL Operation API header
 * @author Daniel Huang
 */

#ifndef _HAL_DC2DC_H_
#define _HAL_DC2DC_H_

#include <mach/hal/hal_common.h>

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
void gpHalDc2dcInit(void);
void gpHalDc2dcDisable(void);
void gpHalDc2dcEnablePWM0(int enable);
void gpHalDc2dcEnablePWM1(int enable);
void gpHalDc2dcSetFeedbackVoltage(int voltage);
void gpHalDc2dcEnableCLK6M(int enable);

#endif	/*_HAL_DC2DC_H_*/
