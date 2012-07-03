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
 * @file    gp_wdt.h
 * @brief   Declaration of watchdog driver.
 * @author  zaimingmeng
 */

#ifndef _GP_WDT_H_
#define _GP_WDT_H_

#include <mach/typedef.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/*
 * Ioctl for device node definition
 */
#define GP_WDT_MAGIC		'W'

#define WDIOC_CTRL		_IO(GP_WDT_MAGIC,0x01)		/*!< @brief watchdog enable/disable control */	
#define WDIOC_KEEPALIVE		_IO(GP_WDT_MAGIC,0x02)		/*!< @brief watchdog feed */	
#define WDIOC_FORCERESET	_IO(GP_WDT_MAGIC,0x03)		/*!< @brief watchdog force reset system */	
#define WDIOC_SETTIMEROUT	_IOW(GP_WDT_MAGIC,0x04,unsigned int)	/*!< @brief watchdog set timerout */
#define WDIOC_GETTIMEROUT	_IOR(GP_WDT_MAGIC,0x05,unsigned int)	/*!< @brief watchdog get timerout */


/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**
 * @brief wdt request function
 * @return success: wdt handle,  erro: NULL
 */
int gp_wdt_request(void);

/**
 * @brief wdt release function
 * @param handle [in] wdt handle
 * @return success: 0,  erro: erro_id
 */
int gp_wdt_release(int handle);

/**
 * @brief wdt enable function
 * @param handle [in] wdt handle
 * @return success: 0,  erro: erro_id
 */
int gp_wdt_enable(int handle);

/**
 * @brief wdt disable function
 * @param handle [in] wdt handle
 * @return success: 0,  erro: erro_id
 */
int gp_wdt_disable(int handle);

/**
 * @brief set watchdog timer timeout period function
 * @param handle [in] wdt handle
 * @param period [in] wdt reset period in seconds
 * @return success: 0,  erro: erro_id
 */
int gp_wdt_set_timeout(int handle,int period);

/**
 * @brief get watchdog timer timeout period function
 * @param handle [in] wdt handle
 * @param period [out] wdt reset period in seconds
 * @return success: 0,  erro: erro_id
 */
int gp_wdt_get_timeout(int handle,int *period);

/**
 * @brief feed watchdog,prevent watchdog reset
 * @param handle [in] wdt handle
 * @return success: 0,  erro: erro_id
 */
int gp_wdt_keep_alive(int handle);


/**
 * @brief watchdog force reset system
 * @param handle [in] wdt handle
 * @return success: 0,  erro: erro_id
 */
int gp_wdt_force_reset(int handle);


#endif	/*_GP_WDT_H_*/
