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
 * @file    hal_ceva.h
 * @brief   Declaration of Ceva HAL API.
 * @author  qinjian
 * @since   2010/10/7
 * @date    2010/10/7
 */
#ifndef _HAL_CEVA_H_
#define _HAL_CEVA_H_

#include <mach/hal/hal_common.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define PIU_INTMSK_C0RDHIE_EN   0x00000001
#define PIU_INTMSK_C1RDHIE_EN   0x00000002
#define PIU_INTMSK_C2RDHIE_EN   0x00000004
#define PIU_INTMSK_R0WRHIE_EN   0x00000008
#define PIU_INTMSK_R1WRHIE_EN   0x00000010
#define PIU_INTMSK_R2WRHIE_EN   0x00000020
#define PIU_INTMSK_C0WRCXIE_EN  0x00000040
#define PIU_INTMSK_C1WRCXIE_EN  0x00000080
#define PIU_INTMSK_C2WRCXIE_EN  0x00000100
#define PIU_INTMSK_R0RDCXIE_EN  0x00000200
#define PIU_INTMSK_R1RDCXIE_EN  0x00000400
#define PIU_INTMSK_R2RDCXIE_EN  0x00000800

#define PIU_STATUS_C0RDS        0x00000001
#define PIU_STATUS_C1RDS        0x00000002
#define PIU_STATUS_C2RDS        0x00000004
#define PIU_STATUS_R0WRS        0x00000008
#define PIU_STATUS_R1WRS        0x00000010
#define PIU_STATUS_R2WRS        0x00000020
#define PIU_STATUS_C0WRS        0x00000040
#define PIU_STATUS_C1WRS        0x00000080
#define PIU_STATUS_C2WRS        0x00000100
#define PIU_STATUS_R0RDS        0x00000200
#define PIU_STATUS_R1RDS        0x00000400
#define PIU_STATUS_R2RDS        0x00000800

#define PIU_SNP_EN_SNP0RDIE     0x00000001
#define PIU_SNP_EN_SNP1RDIE     0x00000002
#define PIU_SNP_EN_SNP0WRIE     0x00000004
#define PIU_SNP_EN_SNP1WRIE     0x00000008

#define PIU_SNP_STAT_SNP0RDS    0x00000001
#define PIU_SNP_STAT_SNP1RDS    0x00000002
#define PIU_SNP_STAT_SNP0WRS    0x00000004
#define PIU_SNP_STAT_SNP1WRS    0x00000008

/* physical memory region to hold ceva external image */
#define CEVA_EXT_BASE           0x000F0000
#define CEVA_EXT_SIZE           0x00100000

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**
 * @brief   Ceva hardware irq enable/disable
 * @param   enable [in] disable(0)/enable(1)
 * @param   intMask [in] interrupt mask
 * @return  None
 * @see
 */
void gpHalCevaEnableIrq(UINT32 enable, UINT32 intMask);

/**
 * @brief   Ceva hardware status setting function
 * @param   status [in] status value
 * @return  None
 * @see
 */
void gpHalCevaSetStatus(UINT32 status);

/**
 * @brief   Ceva hardware status getting function
 * @return  status value
 * @see
 */
UINT32 gpHalCevaGetStatus(void);

/**
 * @brief   Ceva hardware status flags clear function
 * @param   status [in] status flags
 * @return  None
 * @see
 */
void gpHalCevaClearStatus(UINT32 status);

/**
 * @brief   Ceva hardware command trigger
 * @param   idx [in] index of command register
 * @param   cmd [in] start address of command data
 * @return  None
 * @see
 */
void gpHalCevaSetCmd(UINT32 idx, UINT32 cmd);

/**
 * @brief   Ceva hardware command reply getting function
 * @param   idx [in] index of reply register
 * @return  start address of reply data, 0 for ERROR
 * @see
 */
UINT32 gpHalCevaGetReply(UINT32 idx);

/**
 * @brief   Ceva hardware reset
 * @param   entryAddr [in] boot code entry address
 * @return  None
 * @see
 */
void gpHalCevaReset(UINT32 entryAddr);

/**
 * @brief   Ceva hardware lock
 * @return  None
 * @see
 */
void gpHalCevaLock(void);

/**
 * @brief   Ceva hardware waiting status flags
 * @param   status [in] status flags to wait
 * @param   ms [in] waiting timeout in ms
 * @return  flags have been set(>=0) / waiting time out(<0)
 * @see
 */
SINT32 gpHalCevaWaitStatus(UINT32 status, unsigned int ms);

/**
 * @brief   Ceva hardware waiting for READY
 * @param   ms [in] waiting timeout in ms
 * @return  READY(>=0) / waiting time out(<0)
 * @see
 */
SINT32 gpHalCevaWaitReady(unsigned int ms);

/**
 * @brief   Ceva hardware waiting for BOOTED
 * @param   ms [in] waiting timeout in ms
 * @return  BOOTED(>=0) / waiting time out(<0)
 * @see
 */
SINT32 gpHalCevaWaitBooted(unsigned int ms);

/**
 * @brief   Ceva clock enable/disable function
 * @param   enable[in]: disable(0)/enable(1)
 * @return  None
 * @see
 */
void gpHalCevaClkEnable(UINT32 enable);

/**
 * @brief   Ceva clock setting function
 * @param   cevaRatio [in] CEVA clock ratio
 * @param   cevaAhbRatio [in] CEVA AHB clock ratio
 * @param   cevaApbRatio [in] CEVA APB clock ratio
 * @return  None
 */
void gpHalCevaSetClk(UINT32 cevaRatio, UINT32 cevaAhbRatio, UINT32 cevaApbRatio);

#endif /* _HAL_CEVA_H_ */
