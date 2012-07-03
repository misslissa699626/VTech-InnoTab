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

 /**
 * @file gp_pwm.h
 * @brief pwm driver interface 
 * @author zaimingmeng
 */

#ifndef _GP_PWM_H_
#define _GP_PWM_H_

#include <mach/typedef.h>
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define GP_PWM_MAGIC		'P'
#define PWM_IOCTL_CTRL		_IOW(GP_PWM_MAGIC,1,unsigned int)
#define PWM_IOCTL_SET_FREQ	_IOW(GP_PWM_MAGIC,2,unsigned int)
#define PWM_IOCTL_SET_DUTY	_IOW(GP_PWM_MAGIC,3,unsigned int)
#define PWM_IOCTL_GET_CURRENT_VALUE _IOR(GP_PWM_MAGIC,4,unsigned int)

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct gp_pwm_config_s {
	UINT32 freq;
	UINT32 duty;
}gp_pwm_config_t;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**
 * @brief pwm enable function
 * @param handle [in] pwm handle
 * @return success: 0,  erro: ERROR_ID
 */
int gp_pwm_enable(int handle);

/**
 * @brief pwm disable function
 * @param handle [in] pwm handle
 * @return success: 0,  erro: ERROR_ID
 */
int gp_pwm_disable(int handle);

/**
 * @brief pwm config set function
 * @param handle [in] pwm handle
 * @param config [in] config struct value(freq and duty)
 * @return success: 0,  erro: ERROR_ID
 */
int gp_pwm_set_config(int handle,struct gp_pwm_config_s *config);

/**
 * @brief pwm request function
 * @param pwm_id [in] pwm channel index
 * @return success: 0,  erro: ERROR_ID
 */
int gp_pwm_request(int pwm_id);

/**
 *@brief pwm free function
 *@param handle [in] pwm handle
 * @return success: 0,  erro: ERROR_ID
 */
int gp_pwm_release(int handle);

#endif	/*_GP_PWM_H_*/
