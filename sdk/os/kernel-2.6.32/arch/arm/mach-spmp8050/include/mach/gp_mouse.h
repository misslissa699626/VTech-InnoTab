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
 * @file    gp_mouse.h
 * @brief   Declaration of ps2 mouse driver.
 * @author  zaimingmeng
 */

#ifndef _GP_MOUSE_H_
#define _GP_MOUSE_H_

#include <mach/typedef.h>
#include "../../../../../../../openplatform/platform/gplus/evm8050/platform.h"
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/* define the gpio of the spi */
#define MOUSE_CLK_GPIO					((ps2mouse_set_clk_channel<<24)|(ps2mouse_set_clk_func<<16)|(ps2mouse_set_clk_gid<<8)|ps2mouse_set_clk_pin)
#define MOUSE_DATA_GPIO				((ps2mouse_set_dat_channel<<24)|(ps2mouse_set_dat_func<<16)|(ps2mouse_set_dat_gid<<8)|ps2mouse_set_dat_pin)


//for MouseSystemFlag
#define MOUSEInitFlag0		0x01
#define MOUSEInitFlag1		0x02
#define MOUSEInitFlag2		0x04
#define MOUSEInitFlag3		0x08
#define MOUSEInitFlag4		0x10
#define MOUSEInitFlag5		0x20
#define MOUSEInitFlag6		0x40
#define MOUSEInitFlag7		0x80
#define MOUSEInitFlag8		0x100
#define MOUSEInitFlag9		0x200
#define MOUSEInitFlag10		0x400
#define MOUSEInitFlag11		0x800
#define MOUSEInitFlag12		0x1000
#define MOUSEInitFlag13		0x2000
#define MOUSEInitFlag14		0x4000
#define MOUSEStepFlag		0x8000

//for MouseIrqFlag
#define ReceiveFlag 	0x01
#define SendFlag		0x02
#define SendOver    	0x04
#define ReceiveOver    	0x08

#endif	/*_GP_MOUSE_H_*/
