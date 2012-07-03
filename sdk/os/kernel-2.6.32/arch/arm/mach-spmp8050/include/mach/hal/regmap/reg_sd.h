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
#ifndef _REG_SD_H_
#define _REG_SD_H_

#include <mach/hardware.h>                          
#include <mach/typedef.h>

#define	LOGI_ADDR_SD0_REG		IO2_ADDRESS(0xB0B000)
#define	LOGI_ADDR_SD1_REG		IO2_ADDRESS(0xB0C000)
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct sdReg_s {
	/* volatile UINT8 regOffset[0xB0B000]; */ /* 0x92B0B000 */
	/* volatile UINT8 regOffset[0xB0C000]; */ /* 0x92B0C000 */
	volatile UINT32 sdDatTx;		/* P_SDC0_DATTX: 0x92B0B000, P_SDC1_DATTX: 0x92B0C000 */
	volatile UINT32 sdDatRx;		/* P_SDC0_DATRX: 0x92B0B004, P_SDC1_DATRX: 0x92B0C004 */
	volatile UINT32 sdCmd;			/* P_SDC0_CMD: 0x92B0B008, P_SDC1_CMD: 0x92B0C008 */ 
	volatile UINT32 sdArg;			/* P_SDC0_ARG: 0x92B0B00C, P_SDC1_ARG: 0x92B0C00C */
	volatile UINT32 sdResp;			/* P_SDC0_RESP: 0x92B0B010, P_SDC1_RESP: 0x92B0C010 */
	volatile UINT32 sdStatus;		/* P_SDC0_STATUS: 0x92B0B014, P_SDC1_STATUS: 0x92B0C014 */
	volatile UINT32 sdCtrl;			/* P_SDC0_CTRL: 0x92B0B018, P_SDC1_CTRL: 0x92B0C018 */
	volatile UINT32 sdIntEn;		/* P_SDC0_INTEN: 0x92B0B01C, P_SDC1_INTEN: 0x92B0C01C */
	
} sdReg_t;

#endif /* _REG_SD_H_ */


