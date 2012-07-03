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
 * @file    hal_pwrc.h
 * @brief   Implement of PWRC HAL API header file.
 * @author  Zhao Dong
 * @since   2011-02-22
 * @date    2011-02-22
 */

#ifndef _HAL_PWRC_H_
#define _HAL_PWRC_H_

/**************************************************************************
 *                         H E A D E R   F I L E S                        *
 **************************************************************************/
#include <mach/typedef.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

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
 * @brief PWRON1 value get function
 * @param void
 * @return PWRON1 pin value
 * @see
 */
SINT32 gpHalPwron1Get(void);

/**
 * @brief PWRON0 value get function
 * @param void
 * @return PWRON0 pin value
 * @see
 */
SINT32 gpHalPwron0Get(void);

/**
 * @brief battery voltage detect enable function
 * @param en [IN] 0:disable, 1:enable
 * @return void
 * @see
 */
void gpHalBatDetEnable(SP_BOOL enable);

/**
 * @brief battery select function
 * @param battery [IN] 0:alkaline, 1:li-ion
 * @return void
 * @see
 */
void gpHalBatSelect(SP_BOOL battery);

/**
 * @brief operation mode set function
 * @param battery [IN] 0:suspend mode, 1:operation mode
 * @return void
 * @see
 */
void gpHalOperationModeSet(SP_BOOL mode);

/**
 * @brief DCDC enable function
 * @param battery [IN] 0:suspend, 1:enable
 * @return void
 * @see
 */
void gpHalDcdcEnable(SP_BOOL enable);

#endif  /* _HAL_PWRC_H_ */
