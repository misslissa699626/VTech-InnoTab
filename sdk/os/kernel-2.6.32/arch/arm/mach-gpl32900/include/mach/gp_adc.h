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
 * @file arch/arm/mach-spmp8050/include/mach/gp_adc.h
 * @brief ADC device core header file
 * @author zh.l
 */
#ifndef __GP_ADC_H__
#define __GP_ADC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <mach/typedef.h>

#define GP_ADC_MAGIC	'G'
#define IOCTL_GP_ADC_START		_IOW(GP_ADC_MAGIC,0x01,unsigned long)	/*!< start ad conversion*/
#define IOCTL_GP_ADC_STOP		_IO(GP_ADC_MAGIC,0x02)	/*!<stop ad conversion*/

/**
ADC channel define
*/
#define ADC_ChANNEL_TPXP 0
#define ADC_ChANNEL_TPXN 1
#define ADC_ChANNEL_TPYP 2
#define ADC_ChANNEL_TPYN 3
#define ADC_ChANNEL_AUX1 4
#define ADC_ChANNEL_AUX2 5
#define ADC_ChANNEL_AUX3 6
#define ADC_ChANNEL_AUX4 7

/**
 *@brife request an ADC client
 *@param is_ts[in]: 0-this client is not touch screen, others-touch screen client
 *@param ts_cb[in]: callback function for touch screen conversion end,
 *                  not used for none touch screent client
 *@return -ENOMEM no memory for new client, request failed
 *        others: handle of the new adc client
 */
int gp_adc_request(
	unsigned int is_ts,
	void (*ts_cb)(int handle,  unsigned int val, unsigned int event));

/**
 *@brife release ADC client
 *@param handle[in]: the ADC client to release
 *@return none
 */
void gp_adc_release(int handle);

/**
 *@brife ADC client start conversion
 *@param handle[in]: handle of the ADC client
 *@param channel[in]: which adc channel to start conversion
 *@return 0-OK
 *        -EINVAL invalid parameter
 *        -EAGAIN client have alreay started
 */
int gp_adc_start(int handle, unsigned channel);

/**
 *@brife ADC client stop conversion
 *@param handle[in]: handle of the ADC client
 *@return 0-OK
 */
int gp_adc_stop(int handle);

/**
 *@brife read ADC conversion result
 *@param handle[in]: handle of the ADC client
 *@return -ETIMEOUT timeout, no valid result
 *        >=0, AD result
 */
int gp_adc_read(int handle);

/**
 *@brief  Returns the last ADC conversion result data for AUX channel.
 *@param  handle: handle of the adc resource
 *@param  to_jiffies: timeout (number of jiffies)
 *@return -ETIMEOUT timeout, no valid result
 *        >=0, AD result
 */
int gp_adc_read_timeout(int handle, unsigned int to_jiffies);
#ifdef __cplusplus
}
#endif
#endif /*__GP_ADC_H__*/
