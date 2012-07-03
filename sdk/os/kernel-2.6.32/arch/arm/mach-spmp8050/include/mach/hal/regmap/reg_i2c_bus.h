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
 * @file    reg_i2c_bus.h
 * @brief   Regmap of SPMP8050 I2C Bus.
 * @author  junp.zhang
 * @since   2010/10/12
 * @date    2010/10/12
 */
#ifndef _REG_I2C_BUS_H_
#define _REG_I2C_BUS_H_

#include <mach/hardware.h>                          
#include <mach/typedef.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define	LOGI_ADDR_I2C_BUS_REG		(IO2_BASE + 0xB03000)

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct i2cBusReg_s {
	volatile UINT32 iccr;				/* 0x0000 ~ 0x0003 i2c bus control*/
	volatile UINT32 icsr;				/* 0x0004 ~ 0x0007 i2c control/status*/
	volatile UINT32 iar;				/* 0x0008 ~ 0x000B i2c bus address */
	volatile UINT32 idsr;				/* 0x000C ~ 0x000F i2c bus data*/
	volatile UINT32 ideBClk;			/* 0x0010 ~ 0x0013 i2c de-bounce clock*/
	volatile UINT32 txClkLSB;			/* 0x0014 ~ 0x0017 i2c frequency divider LSB part*/
} i2cBusReg_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

#endif /* _REG_I2C_BUS_H_ */
