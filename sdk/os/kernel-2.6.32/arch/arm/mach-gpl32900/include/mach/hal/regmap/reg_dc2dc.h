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
 * @file reg_dc2dc.h
 * @brief dc2dc register define 
 * @author Daniel Huang
 */
#ifndef _REG_DC2DC_H_
#define _REG_DC2DC_H_

#include <mach/hardware.h>                          
#include <mach/typedef.h>

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define	LOGI_ADDR_DC2DC_REG		(IO0_BASE + 0x5140)

#define DC2DC_CLK6M_ENABLE     0x0001
#define DC2DC_PWM0_ENABLE     0x0002
#define DC2DC_PWM1_ENABLE     0x0004

#define DC2DC_VSET0_ENABLE     0x0010
#define DC2DC_VSET1_ENABLE     0x0020
#define DC2DC_VSET2_ENABLE     0x0040

#define DC2DC_VSET_MASK	(DC2DC_VSET0_ENABLE | DC2DC_VSET1_ENABLE | DC2DC_VSET2_ENABLE)
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct dc2dcReg_s {
	volatile UINT32 dc2dcCtr;		/* 0x0000 ~ 0x0003 control register */	
} dc2dcReg_t;

#endif /* _REG_DC2DC_H_ */
