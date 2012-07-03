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
#ifndef _REG_MS_H_
#define _REG_MS_H_

#include <mach/hardware.h>                          
#include <mach/typedef.h>

#define	LOGI_ADDR_MS_REG		(IO2_BASE + 0xB0F000)
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct msReg_s {
	/* volatile UINT8 regOffset[0xB0F000]; */ /* 0x92B0F000 */ 
	volatile UINT32 msDataTx;		/* R_MS_DATATX, 0x92B0F000*/
	volatile UINT32 msDataRx;		/* R_MS_DATARX, 0x92B0F004 */
	volatile UINT32 msComand;		/* R_MS_COMMAND, 0x92B0F008 */
	volatile UINT32 msWriteReg;		/* R_MS_WRITEREG, 0x92B0F00C */
	volatile UINT32 msReadReg;		/* R_MS_READREG, 0x92B0F010 */
	volatile UINT32 msStatus;		/* R_MS_STATUS, 0x92B0F014 */
	volatile UINT32 msControl;		/* R_MS_CONTROL, 0x92B0F018 */
	volatile UINT32 msIntEn;		/* R_MS_INTEN, 0x92B0F01C */
} msReg_t;


#endif /* _REG_MS_H_ */