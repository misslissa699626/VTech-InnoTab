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

/**************************************************************************
 *                         H E A D E R   F I L E S                        *
 **************************************************************************/
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <mach/hardware.h>
#include <mach/hal/hal_pwrc.h>
#include <mach/hal/sysregs.h>
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
SINT32 gpHalPwron1Get(void)
{
	return ( SCUB_PWRC_CFG >> 29 ) & 0x01;
}
EXPORT_SYMBOL(gpHalPwron1Get);

/**
 * @brief PWRON0 value get function
 * @param void
 * @return PWRON0 pin value
 * @see
 */
SINT32 gpHalPwron0Get(void)
{
	return ( SCUB_PWRC_CFG >> 28 ) & 0x01;
}
EXPORT_SYMBOL(gpHalPwron0Get);

/**
 * @brief battery voltage detect enable function
 * @param en [IN] 0:disable, 1:enable
 * @return void
 * @see
 */
void gpHalBatDetEnable(SP_BOOL enable)
{
	if (enable) {
		SCUB_PWRC_CFG = SCUB_PWRC_CFG | 0x08;
	} else {
		SCUB_PWRC_CFG = SCUB_PWRC_CFG & 0xfffffff7;
	}
}
EXPORT_SYMBOL(gpHalBatDetEnable);

/**
 * @brief battery select function
 * @param battery [IN] 0:alkaline, 1:li-ion
 * @return void
 * @see
 */
void gpHalBatSelect(SP_BOOL battery)
{
	if (battery) {
		SCUB_PWRC_CFG = SCUB_PWRC_CFG | 0x04;
	} else {
		SCUB_PWRC_CFG = SCUB_PWRC_CFG & 0xfffffffb;
	}
}
EXPORT_SYMBOL(gpHalBatSelect);

/**
 * @brief operation mode set function
 * @param battery [IN] 0:suspend mode, 1:operation mode
 * @return void
 * @see
 */
void gpHalOperationModeSet(SP_BOOL mode)
{
	if (mode) {
		SCUB_PWRC_CFG = SCUB_PWRC_CFG | 0x02;
	} else {
		SCUB_PWRC_CFG = SCUB_PWRC_CFG & 0xfffffffd;
	}
}
EXPORT_SYMBOL(gpHalOperationModeSet);

/**
 * @brief DCDC enable function
 * @param battery [IN] 0:suspend, 1:enable
 * @return void
 * @see
 */
void gpHalDcdcEnable(SP_BOOL enable)
{
	if (enable) {
		SCUB_PWRC_CFG = SCUB_PWRC_CFG | 0x01;
	} else {
		SCUB_PWRC_CFG = SCUB_PWRC_CFG & 0xfffffffe;
	}
}
EXPORT_SYMBOL(gpHalDcdcEnable);
