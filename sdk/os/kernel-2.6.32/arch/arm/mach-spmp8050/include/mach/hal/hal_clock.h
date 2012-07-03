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
 * @file    hal_sd.h
 * @brief   Implement of SD HAL API header file.
 * @author  Roger Hsu
 * @since   2010-10-26
 * @date    2010-10-26
 */
 
#ifndef _HAL_CLOCK_H_
#define _HAL_CLOCK_H_

/**************************************************************************
 *                         H E A D E R   F I L E S                        *
 **************************************************************************/
 
#include <mach/typedef.h>

/**************************************************************************
*                           C O N S T A N T S                             *
 **************************************************************************/

/*******************************************************************************
*                          D A T A    T Y P E S
*******************************************************************************/
enum {
	SCU_A,
	SCU_B,
	SCU_C,
	SCU_D,
	SCN_MAX
};

/*******************************************************************************
*               F U N C T I O N    D E C L A R A T I O N S
*******************************************************************************/

/**
* @brief 	LCD clock config hardware function.
* @param 	workFreq[in]: LCD frequency
* @return 	none
*/
SINT32 gpHalLcdScuEnable(SINT32 workFreq);

/**
* @todo 	jimmy's interface, merge/remove later
* @brief 	LCD clock enable hardware function.
* @param 	devinfo[in]: device into
* @param 	enable[in]: enable
* @return 	SUCCESS/ERROR_ID.
*/
SP_BOOL gpHalLcdClkEnable(void* devinfo, SP_BOOL enable);

/**
* @brief 	LCD clock get hardware function.
* @param 	none
* @return 	lcd clock
*/
SINT32 
gpHalLcdGetFreq(
	void
);

/**
* @brief 	SCU A clock enable function
* @param 	bitMask[in]: enable bits
* @param 	scu[in]: 0 : SCU_A, 1 : SCU_B, 2: SCU_C
* @param 	enable[in]: 1 : enable, 0 : disable
* @return 	SUCCESS/SP_FAIL.
*/
SINT32
gpHalScuClkEnable(
	UINT32 bitMask, UINT8 scu, UINT8 enable
);

/**
* @brief 	spi clock enable hardware function.
* @param 	devinfo[in]: device into
* @param 	enable[in]: enable
* @return 	SUCCESS/ERROR_ID.
*/
SP_BOOL gpSpiClkEnable(void* devinfo, SP_BOOL enable);

/**
* @todo 	jimmy's interface, merge/remove later
*/
SP_BOOL tvout_clk_enable(void* devinfo, SP_BOOL enable);
SP_BOOL lcm_clk_enable(void* devinfo, SP_BOOL enable);
SP_BOOL audioplay_clk_enable(void* devinfo, SP_BOOL enable);
SP_BOOL audiorec_clk_enable(void* devinfo, SP_BOOL enable);



#endif  /* _HAL_SD_H */