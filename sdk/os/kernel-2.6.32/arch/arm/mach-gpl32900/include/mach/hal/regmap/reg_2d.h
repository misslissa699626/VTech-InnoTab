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
 * @file    reg_2d.h
 * @brief   Regmap of SPMP8050 2D Engine
 * @author  clhuang
 * @since   2010-10-07
 * @date    2010-10-07
 */
#ifndef _REG_2D_H_
#define _REG_2D_H_

#include <mach/hardware.h>
#include <mach/typedef.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define G2D_BASE   IO2_ADDRESS(0x6000)

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct g2dReg_s {

	volatile UINT32 srcBaseAdr;           /* 0x0000 ~ 0x0003 */
	volatile UINT32 srcBaseAdru;          /* 0x0004 ~ 0x0007 */
	volatile UINT32 srcBaseAdrv;          /* 0x0008 ~ 0x000b */
	volatile UINT32 srcPitch;             /* 0x000c ~ 0x000f */
	
    volatile UINT32 srcScalHeightWidth;   /* 0x0010 ~ 0x0013 */
	volatile UINT32 srcStartYX;           /* 0x0014 ~ 0x0017 */
	volatile UINT32 dstBaseAdr;           /* 0x0018 ~ 0x001b */
	volatile UINT32 dstPitchWidth;        /* 0x001c ~ 0x001f */
	volatile UINT32 dstHeight;            /* 0x0020 ~ 0x0023 */
	
	volatile UINT32 dstStartYX;           /* 0x0024 ~ 0x0027 */
    volatile UINT32 rectHeightWidth;            /* 0x0028 ~ 0x002b */
	volatile UINT32 maskBaseAdr;          /* 0x002c ~ 0x002f */
	
	volatile UINT32 maskHeightPitch;      /* 0x0030 ~ 0x0033 */
	volatile UINT32 paltFormatOffsetLenth;/* 0x0034 ~ 0x0034 */
	volatile UINT32 plStartYX;            /* 0x0038 ~ 0x003b */
	volatile UINT32 plendYX;              /* 0x003c ~ 0x003f */ 
	
	volatile UINT32 trop;                 /* 0x0040 ~ 0x0043 */  
	volatile UINT32 srcHiColorKey;        /* 0x0044 ~ 0x0047 */  
	volatile UINT32 srcLoColorKey;        /* 0x0048 ~ 0x004b */  
	volatile UINT32 dstHiColorKey;        /* 0x004c ~ 0x004f */ 
	
	volatile UINT32 dstLoColorKey;        /* 0x0050 ~ 0x0053 */ 
	volatile UINT32 clipBottomTop;        /* 0x0054 ~ 0x0057 */
	volatile UINT32 clipRightLeft;        /* 0x0058 ~ 0x006b */
	volatile UINT32 bprdMask;             /* 0x005c ~ 0x005f */ 
	
	volatile UINT32 bpwrMask;             /* 0x0060 ~ 0x0063 */   
	volatile UINT32 stippleMsk0;          /* 0x0064~ 0x0067 */  
	volatile UINT32 stippleMsk1;          /* 0x0068 ~ 0x006b */  
	volatile UINT32 stipplePeriod;        /* 0x006c ~ 0x006f */ /*not know usage*/
	
	volatile UINT32 VHDeltaR;             /* 0x0070 ~ 0x0073 */
	volatile UINT32 VHDeltaG;             /* 0x0074 ~ 0x0077 */
	volatile UINT32 VHDeltaB;             /* 0x0078 ~ 0x007b */
	volatile UINT32 HFactorValue;         /* 0x007c ~ 0x007F */
	
	volatile UINT32 VFactorValue;         /* 0x0080 ~ 0x0083 */  
	volatile UINT32 bgPatternRop;         /* 0x0084 ~ 0x0087 */  
	volatile UINT32 fgPatternRop;         /* 0x0088 ~ 0x008b */  
	volatile UINT32 srcDstConstAlpha;     /* 0x008c ~ 0x008f */ 
	
	volatile UINT32 srcDstColorFormat;    /* 0x0090 ~ 0x0093 */ 
    volatile UINT32  blendOpAlphaFormat;  /* 0x0094 ~ 0x0097 */  
	volatile UINT32 ctrlReg;              /* 0x0098 ~ 0x009b */  
	volatile UINT32 dmaPriority;          /* 0x009c ~ 0x009f */
	
	volatile UINT32 run;                  /* 0x00a0 ~ 0x00a3 */
	volatile UINT32 reset;                /* 0x00a4 ~ 0x00a7 */
	volatile UINT32 statusReg;            /* 0x00a8 ~ 0x00ab */
	volatile UINT32 intEn;                /* 0x00ac ~ 0x00af */
} g2dReg_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

#endif /* _REG_2D_H_ */
