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
 * @file hal_ms.h
 * @brief ms HAL Operation API header
 * @author 
 */

#ifndef _HAL_MSPRO_H_
#define _HAL_MSPRO_H_

#include <mach/common.h>

void gpHalMsInit(void);
void gpHalMsChangeIfMode(UINT8 mode);
void gpHalMsChangeClock(UINT8 div);
void gpHalMsSendCmd(UINT32 MsCmd,UINT32 Data,UINT8 Size);
UINT32 gpHalMsGetStatus(void);
UINT32 gpHalMsReadReg(void);
void gpHalMsReadData(UINT32 *Data, UINT32 Size);
void gpHalMsWriteData(UINT32 *Data, UINT32 Size);
void gpHalMsWriteReg(UINT32 Data);
void gpHalMsSetFifoLevel(UINT32 TxLevel,UINT32 RxLevel);
void gpHalMsClearStatus(void);

#endif