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
 * @file    gp_dc2dc.h
 * @brief   Declaration of DC2DC driver.
 * @author  Daniel Huang
 */

#ifndef _GP_DC2DC_H_
#define _GP_DC2DC_H_

#include <mach/typedef.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/*
 * Ioctl for device node definition
 */
#define SPMP_DC2DC_MAGIC		'D'

#define DC2DC_IOCTL_ENABLE_PWM0	                     _IO(SPMP_DC2DC_MAGIC,0x01)		/*!< @brief dc2dc enable/disable PWM0 control */	
#define DC2DC_IOCTL_ENABLE_PWM1	                     _IO(SPMP_DC2DC_MAGIC,0x02)		/*!< @brief dc2dc enable/disable PWM1 control */	
#define DC2DC_IOCTL_SET_FB_VOLTAGE		    _IO(SPMP_DC2DC_MAGIC,0x03)		/*!< @brief dc2dc set feedback voltage */	

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**
 * @brief dc2dc request function
 * @return success: dc2dc handle,  erro: NULL
 */
int gp_dc2dc_request(void);

/**
 * @brief dc2dc release function
 * @param handle [in] dc2dc handle
 * @return success: 0,  erro: erro_id
 */
int gp_dc2dc_release(int handle);

/**
 * @brief dc2dc enable pwm0 function
 * @param handle [in] dc2dc handle
 * @return success: 0,  erro: erro_id
 */
int gp_dc2dc_enable_pwm0(int handle);

/**
 * @brief dc2dc disable pwm0 function
 * @param handle [in] dc2dc handle
 * @return success: 0,  erro: erro_id
 */
int gp_dc2dc_disable_pwm0(int handle);

/**
 * @brief dc2dc enable pwm1 function
 * @param handle [in] dc2dc handle
 * @return success: 0,  erro: erro_id
 */
int gp_dc2dc_enable_pwm1(int handle);

/**
 * @brief dc2dc disable pwm1 function
 * @param handle [in] dc2dc handle
 * @return success: 0,  erro: erro_id
 */
int gp_dc2dc_disable_pwm1(int handle);

/**
 * @brief dc2dc disable function
 * @param handle [in] dc2dc handle
 * @return success: 0,  erro: erro_id
 */
int gp_dc2dc_disable(int handle);

#endif	/*_GP_DC2DC_H_*/
