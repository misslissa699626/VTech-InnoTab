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
 * @file    gp_clock.h
 * @brief   Declaration of Clock management driver
 * @author  Roger hsu
 * @date    2010-9-27
 */
#ifndef _GP_CLOCK_MGR_H_
#define _GP_CLOCK_MGR_H_

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define GPIO_IRQ_ACTIVE_BOTH    2

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
enum {
	CLOCK_LCD		,
	CLOCK_LCM		,
	CLOCK_TVOUT  	,
	CLOCK_AUDIO  	,
	CLOCK_MAX_NUM
};

/** 
 * @brief clock manager operation interface structure
 * @param : clock_set_func	: clock setting function
 * @param : clock_get_func	: clock getting function
 */
typedef struct gp_clock_function_s {
	int 	(*clock_set_func)(int freq);
	int 	(*clock_get_func)(void);
} gp_clock_function_t;

/** 
 * @brief clock handle sructure
 * @param : handle_state : 0 : release state, 1 : request state
 * @param : clodk_id : clock id information
 */
typedef struct gp_clock_handle_s {
	int 	handle_state;
	int 	clock_id;
} gp_clock_handle_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**
 * @brief   Clock manager request function.
 * @param   None
 * @return  HANDLE(non-negative value)/ERROR_ID(-1)
 */
int gp_clock_mgr_request(int clock_id);

/**
 * @brief   Clock manager release function.
 * @param   Handle[in]: clock manager handle to release
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_clock_mgr_release(int handle);

/**
 * @brief   clock frequence setting function.
 * @param   handle[in]: clock handle
 * @param   fre[in]: clock frequence setting
 * @param   real_freq[out]: clock real setting value
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_clock_set_rate(int handle, int freq, int *real_freq);

/**
 * @brief   clock rate getting function.
 * @param   handle[in]: clock handle
 * @param   freq[out]: clock real setting value
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_clock_get_rate(int handle, int *freq);

#endif /* _GP_CLOCK_MGR_H_ */
