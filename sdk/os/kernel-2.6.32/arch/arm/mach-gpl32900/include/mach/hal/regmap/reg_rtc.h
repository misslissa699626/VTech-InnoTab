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
#ifndef _REG_RTC_H_
#define _REG_RTC_H_

#include <mach/hardware.h>                          
#include <mach/typedef.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define	LOGI_ADDR_RTC_REG		(IO0_BASE + 0xb000)

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct rtcReg_s {
	/* volatile UINT8 regOffset[0xb000]; */ /* 0x9000B000 */
	volatile UINT32 rtcCtrl;		/* 0x0000 ~ 0x0003 */
	volatile UINT32 rtcAddr;		/* 0x0004 ~ 0x0007 */
	volatile UINT32 rtcWData;		/* 0x0008 ~ 0x000B */
	volatile UINT32 rtcRWReq;		/* 0x000C ~ 0x000F */
	volatile UINT32 rtcRdy;			/* 0x0010 ~ 0x0013 */
	volatile UINT32 rtcRData;		/* 0x0014 ~ 0x0017 */
	volatile UINT32 rtcTselect;		/* 0x0018 ~ 0x001B */
	
} rtcReg_t;

#endif /* _REG_RTC_H_ */


