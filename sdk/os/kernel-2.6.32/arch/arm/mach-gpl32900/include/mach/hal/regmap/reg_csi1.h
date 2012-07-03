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
 
#ifndef _REG_CSI1_H_
#define _REG_CSI1_H_

#include <mach/hardware.h>
#include <mach/typedef.h>

#define	LOGI_ADDR_CSI1_REG		(IO3_BASE + 0x20240)
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct csi1Reg_s {
	volatile UINT32 csicr0;		/* 0x0000 ~ 0x0003 */
	volatile UINT32 csicr1;		/* 0x0004 ~ 0x0007 */
	volatile UINT32 csihlstart;	/* 0x0008 ~ 0x000B */
	volatile UINT32 csihend;	/* 0x000C ~ 0x000F */
	volatile UINT32 csivl0start;/* 0x0010 ~ 0x0013 */
	volatile UINT32 csimdfbaddr;/* 0x0014 ~ 0x0017 */
	volatile UINT32 csivend;	/* 0x0018 ~ 0x001B */
	volatile UINT32 csihstart;	/* 0x001C ~ 0x001F */
	volatile UINT32 csirgbl;	/* 0x0020 ~ 0x0023 */
	volatile UINT32 csisencr;   /* 0x0024 ~ 0x0027 */
	volatile UINT32 csibsupper; /* 0x0028 ~ 0x002B */
	volatile UINT32 csibslower; /* 0x002C ~ 0x002F */
	volatile UINT32 csirgbh;	/* 0x0030 ~ 0x0033 */
	volatile UINT32 csimdcr;    /* 0x0034 ~ 0x0037 */
	volatile UINT32 csifbsaddr; /* 0x0038 ~ 0x003B */
	volatile UINT32 rsv07c;     /* 0x003C ~ 0x003F */
	volatile UINT32 csivl1start;/* 0x0040 ~ 0x0043 */
	volatile UINT32 csihwidth;  /* 0x0044 ~ 0x0047 */
	volatile UINT32 csivheight; /* 0x0048 ~ 0x004B */
	volatile UINT32 csicutstart;/* 0x004C ~ 0x004F */
	volatile UINT32 csicutsize;	/* 0x0050 ~ 0x0053 */
	volatile UINT32 csivstart;  /* 0x0054 ~ 0x0057 */
	volatile UINT32 csifbaddrh; /* 0x0058 ~ 0x005B */
	volatile UINT32 csihratio;  /* 0x005C ~ 0x005F */
	volatile UINT32 csivratio;	/* 0x0060 ~ 0x0063 */
	volatile UINT32 csihpos;    /* 0x0064 ~ 0x0067 */
	volatile UINT32 csivpos;    /* 0x0068 ~ 0x006B */
} csi1Reg_t;                   

#endif /* _REG_CSI_H_ */
