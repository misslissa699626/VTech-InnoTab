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
 * @file    reg_rotate.h
 * @brief   Regmap of SPMP8050 Rotation Engine.
 * @author  qinjian
 * @since   2010/10/14
 * @date    2010/10/14
 */
#ifndef _REG_ROTATE_H_
#define _REG_ROTATE_H_

#include <mach/hardware.h>
#include <mach/typedef.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define LOGI_ADDR_ROTATE_REG    IO2_ADDRESS(0x2000)

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct rotateReg_s {
	volatile UINT32 rotateImgAddr;      /* 0x0000 ~ 0x0003 Image Start Address */
	volatile UINT32 rotateImgPitch;     /* 0x0004 ~ 0x0007 Image Pitch (byte) */
	volatile UINT32 rotateImgBlock;     /* 0x0008 ~ 0x000B Image Macro-Block Count */
	volatile UINT32 dummy1;             /* 0x000C ~ 0x000F */
	volatile UINT32 rotateOutAddr;      /* 0x0010 ~ 0x0013 Output Start Address */
	volatile UINT32 rotateOutPitch;     /* 0x0014 ~ 0x0017 Output Pitch (byte) */
	volatile UINT32 dummy2[10];         /* 0x0018 ~ 0x003F */

	volatile UINT32 rotateCtrl;         /* 0x0040 ~ 0x0043 Rotator Control */
	volatile UINT32 dummy3[15];         /* 0x0044 ~ 0x007F */

	volatile UINT32 rotateIntEn;        /* 0x0080 ~ 0x0083 Interrupt Enable */
	volatile UINT32 rotateIntSrc;       /* 0x0084 ~ 0x0087 Interrupt Source */
} rotateReg_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

#endif /* _REG_ROTATE_H_ */
