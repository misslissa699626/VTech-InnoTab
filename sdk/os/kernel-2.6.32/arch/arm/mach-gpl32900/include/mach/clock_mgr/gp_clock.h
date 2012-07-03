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

#ifdef __cplusplus
extern "C" {
#endif

#include <mach/typedef.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define NOT_USE	0 //for re-factory

#define GPIO_IRQ_ACTIVE_BOTH    2

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
/* Ioctl for device node definition */
#define GP_CLOCK_MAGIC	'O'
#define IOCTL_GP_CLOCK_SET			_IOW(GP_CLOCK_MAGIC,1,unsigned int)
#define IOCTL_GP_CLOCK_ARM			_IOW(GP_CLOCK_MAGIC,2,unsigned int)
#define IOCTL_GP_CLOCK_CEVA			_IOW(GP_CLOCK_MAGIC,3,unsigned int)
#define IOCTL_GP_CLOCK_SYS			_IOW(GP_CLOCK_MAGIC,4,unsigned int)
#define IOCTL_GP_CLOCK_ENABLE		_IOW(GP_CLOCK_MAGIC,5,unsigned int)
#define IOCTL_GP_CLOCK_DISABLE		_IOW(GP_CLOCK_MAGIC,6,unsigned int)
#define IOCTL_GP_CLOCK_USAGE_DUMP	_IOW(GP_CLOCK_MAGIC,7,unsigned int)
#define IOCTL_GP_CLOCK_DUMP_ALL		_IOW(GP_CLOCK_MAGIC,8,unsigned int)

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
#if NOT_USE
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
#endif //NOT_USE

// should replase gp_clock_get_rate/gp_clock_set_rate
/**
* @brief 	Clock get functino 
* @param 	clock_name[in]: base/device clock name to get current base clock
 * @param   freq[out]: clock real setting value
 * @return  SP_OK(0)/ERROR_ID
*/
int gp_clk_get_rate(int *clock_name, int*freq);

/**
* @brief 	Clock Set function
* @param 	clock_name[in]: base/device clock name to set current base clock
* @param 	rate[in]: clock rate to change
* @param 	realRate[in]: real clock rate after change
* @param 	device_clk_func[in]: device clock adjust function, passing two parameter to function : old rate, new rate 
* @return 	SUCCESS/FAIL.
*/
int gp_clk_set_rate(char *clock_name, unsigned int rate, unsigned int *real_rate, int *device_clock_func);

/**
 * @brief   spll spread mode enable function.
 * @param   modules[in]: 0:spll,1:spll2
 * @return  0
 */
int gp_enable_spll_spread_mode(int modules);

/**
 * @brief   spll spread mode disable function.
 * @param   modules[in]: 0:spll,1:spll2
 * @return  0
 */
int gp_disable_spll_spread_mode(int modules);

/**
 * @brief   set spll spread modulation clock function.
 * @param   modules[in]: 0:spll,1:spll2
 * @param   mc1[in]: modulation clock 1(HZ,ex:125000)
 * @return  0 or -1
 */
int gp_set_spll_spread_mc(int modules, int mc1);

/**
 * @brief   set spll spread modulation rate function.
 * @param   modules[in]: 0:spll,1:spll2
 * @param   mr[in]: modulation rate (1~90)
 * @return  0 or -1
 */
int gp_set_spll_spread_mr(int modules, int mr);

int gp_set_spll_spread_mrc(int modules, int mrc);

int gp_set_spll_spread_mcc(int modules, int mcc);

/**
* @brief 	Enable/Disable clock interface function
* @param 	clock_name[in]: base/device clock name
* @param 	enable[in]:  1: enable, 0 : diable
* @return 	SUCCESS/FAIL.
*/
int gp_enable_clock(int *clock_name, int enable);

/**
 * @brief   clock rate change function.
 * @param 	clock_name[in]: base/device clock name
 * @return  SUCCESS(0)/ERROR_ID
 */
int gp_dump_clock_usage(int *clock_name);

#endif /* _GP_CLOCK_MGR_H_ */
