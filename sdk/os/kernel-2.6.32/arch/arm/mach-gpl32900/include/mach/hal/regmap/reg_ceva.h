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
 * @file    reg_ceva.h
 * @brief   Regmap of SPMP8050 CevaX1620
 * @author  qinjian
 * @since   2010/10/7
 * @date    2010/10/7
 */
#ifndef _REG_CEVA_H_
#define _REG_CEVA_H_

#include <mach/hardware.h>
#include <mach/typedef.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define LOGI_ADDR_CEVA_REG      IO0_ADDRESS(0x530000)

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct cevaReg_s {
	/* PMU: 0x0000 ~ 0x03FF */
	volatile UINT8 pmu[0x0400];         /* 0x0000 ~ 0x03FF */

	/* ICU: 0x0400 ~ 0x07FF */
	volatile UINT8 dummy1[0x0068];      /* 0x0400 ~ 0x0467 */
	volatile UINT32 cevaCtrl;           /* 0x0468 ~ 0x046B */
	volatile UINT32 cevaVect;           /* 0x046C ~ 0x046F */
	volatile UINT8 dummy2[0x0390];      /* 0x0470 ~ 0x07FF */

	/* TIMERD0: 0x0800 ~ 0x0BFF */
	volatile UINT8 timerD0[0x0400];     /* 0x0800 ~ 0x0BFF */

	/* TIMERD1: 0x0C00 ~ 0x0FFF */
	volatile UINT8 timerD1[0x0400];     /* 0x0C00 ~ 0x0FFF */

	/* Reserved: 0x1000 ~ 0x17FF */
	volatile UINT8 dummy3[0x0800];      /* 0x1000 ~ 0x17FF */

	/* TIMERD2: 0x1800 ~ 0x1BFF */
	volatile UINT8 timerD2[0x0400];     /* 0x1800 ~ 0x1BFF */

	/* TIMERD3: 0x1C00 ~ 0x1FFF */
	volatile UINT8 timerD3[0x0400];     /* 0x1C00 ~ 0x1FFF */

	/* PIU: 0x2000 ~ 0x23FF */
	volatile UINT32 piuSem0S;           /* 0x2000 ~ 0x2003 */
	volatile UINT32 piuSem1S;           /* 0x2004 ~ 0x2007 */
	volatile UINT32 piuSem2S;           /* 0x2008 ~ 0x200B */
	volatile UINT32 piuSem0C;           /* 0x200C ~ 0x200F */
	volatile UINT32 piuSem1C;           /* 0x2010 ~ 0x2013 */
	volatile UINT32 piuSem2C;           /* 0x2014 ~ 0x2017 */
	volatile UINT32 piuMcuMask0;        /* 0x2018 ~ 0x201B */
	volatile UINT32 piuMcuMask1;        /* 0x201C ~ 0x201F */
	volatile UINT32 piuMcuMask2;        /* 0x2020 ~ 0x2023 */
	volatile UINT32 piuCxMask0;         /* 0x2024 ~ 0x2027 */
	volatile UINT32 piuCxMask1;         /* 0x2028 ~ 0x202B */
	volatile UINT32 piuCxMask2;         /* 0x202C ~ 0x202F */
	volatile UINT32 piuCom0;            /* 0x2030 ~ 0x2033 */
	volatile UINT32 piuCom1;            /* 0x2034 ~ 0x2037 */
	volatile UINT32 piuCom2;            /* 0x2038 ~ 0x203B */
	volatile UINT32 piuRep0;            /* 0x203C ~ 0x203F */
	volatile UINT32 piuRep1;            /* 0x2040 ~ 0x2043 */
	volatile UINT32 piuRep2;            /* 0x2044 ~ 0x2047 */

	volatile UINT32 piuIntMask;         /* 0x2048 ~ 0x204B */
	volatile UINT32 piuStatus;          /* 0x204C ~ 0x204F */
	volatile UINT32 piuSnpBase0;        /* 0x2050 ~ 0x2054 */
	volatile UINT32 piuSnpBase1;        /* 0x2054 ~ 0x2058 */
	volatile UINT32 piuSnpMask0;        /* 0x2058 ~ 0x205B */
	volatile UINT32 piuSnpMask1;        /* 0x205C ~ 0x205F */
	volatile UINT32 piuSnpEn;           /* 0x2060 ~ 0x2064 */
	volatile UINT32 piuSnpStat;         /* 0x2064 ~ 0x2067 */
} cevaReg_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

#endif /* _REG_CEVA_H_ */
