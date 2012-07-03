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
 * @file    reg_scale.h
 * @brief   Regmap of SPMP8050 Scale Engine.
 * @author  qinjian
 * @since   2010/10/7
 * @date    2010/10/7
 */
#ifndef _REG_SCALE_H_
#define _REG_SCALE_H_

#include <mach/hardware.h>
#include <mach/typedef.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define LOGI_ADDR_SCALE_REG     IO2_ADDRESS(0x7000)

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct scaleReg_s {
	volatile UINT32 scaleFbAddr;        /* 0x0000 ~ 0x0003 Frame Buffer Address */
	volatile UINT32 scaleCbAddr;        /* 0x0004 ~ 0x0007 Cb Start Address */
	volatile UINT32 scaleCrAddr;        /* 0x0008 ~ 0x000B Cr Start Address */
	volatile UINT32 scaleWbAddr;        /* 0x000C ~ 0x000F Write Back Address */
	volatile UINT32 scaleImgPitch;      /* 0x0010 ~ 0x0013 Image Pitch (byte) */
	volatile UINT32 scaleImgRes;        /* 0x0014 ~ 0x0017 Image Resolution */
	volatile UINT32 scaleMcbPitch;      /* 0x0018 ~ 0x001B Macro-block Pitch (byte) */
	volatile UINT32 scaleCbCrPitch;     /* 0x001C ~ 0x001F Image(CbCr) Pitch (byte) */

	volatile UINT32 scaleOutPitch;      /* 0x0020 ~ 0x0023 Output Pitch (byte) */
	volatile UINT32 scaleOutRes;        /* 0x0024 ~ 0x0027 Output Resolution */
	volatile UINT32 scaleParamH;        /* 0x0028 ~ 0x002B Scale Parameter (Horizontal) */
	volatile UINT32 scaleParamV;        /* 0x002C ~ 0x002F Scale Parameter (Vertical) */

	volatile UINT32 scaleDitherMap0;    /* 0x0030 ~ 0x0033 Dither Map #0 */
	volatile UINT32 scaleDitherMap1;    /* 0x0034 ~ 0x0037 Dither Map #1 */
	volatile UINT32 dummy1[2];          /* 0x0038 ~ 0x003F */

	volatile UINT32 scaleCtrl;          /* 0x0040 ~ 0x0043 Scale Control */
	volatile UINT32 dummy2[15];         /* 0x0044 ~ 0x007F */

	volatile UINT32 scaleIntEn;         /* 0x0080 ~ 0x0083 Interrupt Enable */
	volatile UINT32 scaleIntSrc;        /* 0x0084 ~ 0x0087 Interrupt Source */
} scaleReg_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

#endif /* _REG_SCALE_H_ */
