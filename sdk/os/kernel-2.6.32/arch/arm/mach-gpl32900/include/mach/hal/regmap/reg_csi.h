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
 *  3F, No.8, Dusing Rd., Science-Based Industrial Park,                  *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 **************************************************************************/
 
#ifndef _REG_CSI_H_
#define _REG_CSI_H_

#include <mach/hardware.h>
#include <mach/typedef.h>

#define	LOGI_ADDR_CSI_REG		(IO3_BASE + 0x3000)
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct csiReg_s {
	/* volatile UINT8 regOffset[0xB03000]; */ /* 0x92B03000 */
	volatile UINT32 csicr0;			/* 0x0000 ~ 0x0003 */
	volatile UINT32 csicr1;			/* 0x0004 ~ 0x0007 */
	volatile UINT32 csihset;		/* 0x0008 ~ 0x000B */
	volatile UINT32 csivset;		/* 0x000C ~ 0x000F */
	volatile UINT32 rsv010[3];	/* 0x0010 ~ 0x001B */
	volatile UINT32 csilstp;		/* 0x001C ~ 0x001F */
	volatile UINT32 csifbadr0;	/* 0x0020 ~ 0x0023 */
	volatile UINT32 csifbadr1;	/* 0x0024 ~ 0x0027 */
	volatile UINT32 csifbadr2;	/* 0x0028 ~ 0x002B */
	volatile UINT32 csihold;		/* 0x002C ~ 0x002F */
	volatile UINT32 rsv030[18];	/* 0x0030 ~ 0x0077 */
	volatile UINT32 csiirqen;		/* 0x0078 ~ 0x007B */
	volatile UINT32 csiirqsts;	/* 0x007C ~ 0x007F */
} csiReg_t;

#endif /* _REG_CSI_H_ */
