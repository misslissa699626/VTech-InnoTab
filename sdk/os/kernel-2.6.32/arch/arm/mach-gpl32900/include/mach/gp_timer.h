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
 * @file    gp_timer.h
 * @brief   Declaration of timer/counter driver.
 * @author  zaimingmeng
 */

#ifndef _SPMP_TIMER_H_
#define _SPMP_TIMER_H_

#include <mach/typedef.h>
/******************************************************************
*		CONSTANTS		*
******************************************************************/

/*Ioctl for device node define */
#define SPMP_TIMER_MAGIC		'T'

#define TC_IOCTL_ENABLE			_IO(SPMP_TIMER_MAGIC,0x01)
#define TC_IOCTL_DISABLE		_IO(SPMP_TIMER_MAGIC,0x02)

#define TC_IOCTL_ENABLE_OUTPUT		_IOW(SPMP_TIMER_MAGIC,0x03,unsigned int)
#define TC_IOCTL_SET_COUNT_MODE		_IOW(SPMP_TIMER_MAGIC,0x04,unsigned int)
#define TC_IOCTL_SET_COUNT_SELECTION	_IOW(SPMP_TIMER_MAGIC,0x05,unsigned int)
#define TC_IOCTL_SET_PULSE_MODE		_IOW(SPMP_TIMER_MAGIC,0x06,unsigned int)
#define TC_IOCTL_SET_ACTIVE_EDGE	_IOW(SPMP_TIMER_MAGIC,0x07,unsigned int)
#define TC_IOCTL_SET_OPERATION_MODE	_IOW(SPMP_TIMER_MAGIC,0x08,unsigned int)
#define TC_IOCTL_SET_RELOAD		_IOW(SPMP_TIMER_MAGIC,0x09,unsigned int)
#define TC_IOCTL_GET_CURRENT_VALUE	_IOR(SPMP_TIMER_MAGIC,0x0a,unsigned int)
#define TC_IOCTL_SET_PRESCALE		_IOW(SPMP_TIMER_MAGIC,0x0b,unsigned int)
#define TC_IOCTL_GET_PRESCALE		_IOR(SPMP_TIMER_MAGIC,0x0c,unsigned int)
#define TC_IOCTL_SET_COMPARE		_IOW(SPMP_TIMER_MAGIC,0x0d,unsigned int)
#define TC_IOCTL_GET_COMPARE		_IOR(SPMP_TIMER_MAGIC,0x0e,unsigned int)
#define TC_IOCTL_SET_OUTPUTMODE		_IOW(SPMP_TIMER_MAGIC,0x0f,unsigned int)
#define TC_IOCTL_GET_OUTPUTMODE		_IOR(SPMP_TIMER_MAGIC,0x10,unsigned int)


/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
/**
 * @brief timer/counter enable function
 * @param handle [in] timer/counter handle
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_enable(int handle);

/**
 * @brief timer/counter disable function
 * @param handle [in] timer/counter handle
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_disable(int handle);

/**
 * @brief timer/counter enable/disable interrupter
 * @param handle [in] timer/counter handle
 * @param enable [in] 0:disable interrupter; 1:enable interrupter
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_enable_int(int handle,int enable);

/**
 * @brief timer/counter enable/disable output
 * @param handle [in] timer/counter handle
 * @param enable [in] 0:disable timer output; 1:enable timer output
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_enable_output(int handle,int enable);

/**
 * @brief timer/counter(PWM) mode set
 * @param handle [in] timer/counter handle
 * @param mode [in] 0:enable timer output; 1:enable PWM 
 *  			 output
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_set_output_mode(int handle, int mode);


/**
 * @brief timer/counter(PWM) mode set
 * @param handle [in] timer/counter handle
 * @param enable [in] 0:disable timer output; 1:enable timer output
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_get_output_mode(int handle, int *mode);

/**
 * @brief timer/counter count up or down selection
 * @param handle [in] timer/counter handle
 * @param dir [in] dir value: 0:down counting; 1:up counting;
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_set_count_mode(int handle,int dir);

/**
 * @brief up/down counting control selection set
 * @param handle [in] timer/counter handle
 * @param mode [in] dir value: 0:up/down control by bit4 TxCTR;
 *                 1:up/down control by EXTUDx input;
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_set_count_selection(int handle,int mode);

/**
 * @brief timer/counter output mode set,only effect when timer output is in normal mode.
 * @param handle [in] timer/counter handle
 * @param mode [in] 0:toggle mode; 1:pulse mode
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_set_pulse_mode(int handle,int mode);

/**
 * @brief external input active edge set
 * @param handle [in] timer/counter handle
 * @param edge [in] dir value: 0:positive edge; 1:negative edge;
 *                 2:both edge;
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_set_active_edge(int handle,int edge);

/**
 * @brief timer operating mode set
 * @param handle [in] timer/counter handle
 * @param mode [in]  mode value: 0:free running time mode; 1:period timer mode; 
 *		2:free running counter mode; 3:period counter mode
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_set_operation_mode(int handle,int mode);

/**
 * @brief load value set
 * @param handle [in] timer/counter handle
 * @param value [in] timer/counter load value
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_set_load(int handle,int value);

/**
 * @brief load value get
 * @param handle [in] timer/counter handle
 * @param value [out] timer/counter load value
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_get_load(int handle,int *value);

/**
 * @brief timer compare value set(used only in pwm mode)
 * @param handle [in] timer/counter handle
 * @param value [in] timer/counter compare value
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_set_compare(int handle,int value);

/**
 * @brief timer compare value get;used only in pwm mode
 * @param handle [in] timer/counter handle
 * @param value [out] timer/counter compare value
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_get_compare(int handle,int *value);

/**
 * @brief timer/counter clock prescale set
 * @param handle [in] timer/counter handle
 * @param value [in] timer/counter prescale value
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_set_prescale(int handle,int value);

/**
 * @brief timer/counter clock prescale get
 * @param handle [in] timer/counter handle
 * @param value [out] timer/counter prescale value
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_get_prescale(int handle,int *value);

/**
 * @brief use to clear interrupt flags
 * @param handle [in] timer/counter handle
 * @param value [in] 0:clear interrupter flag
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_set_int_state(int handle,int value);

/**
 * @brief get interrupt flags
 * @param handle [in] timer/counter handle
 * @param value [out]  interrupter flag
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_get_int_state(int handle,int *value);

/**
 * @brief the counter register of timer/counter init function;init load/prescale/cmp register
 * @param freq[in]:timer value of the counter reload(hz)
 * @param duty[in]:int timer mode is 0;in PWM mode is(0~100)
 */
void gp_tc_set_freq_duty(int handle,UINT32 freq,UINT32 duty);

/**
 * @brief request the timer/counter resource function
 * @param timer_id [in] timer/counter device id
 * @param name [in] caller name
 * @return success: 0,  erro: NULL
 */
int gp_tc_request(int timer_id,char *name);

/**
 * @brief release the timer/counter resource function
 * @param handle [in] timer/counter handle
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_release(int handle);

/**
 * @brief the timer/counter interrupt register function
 * @param handle [in] timer/counter handle
 * @param irq_handler [in] interrupt function handle
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_irq_register(int handle,void(*irq_handler)(int,void *));

/**
 * @brief the timer/counter device init function
 * @param handle [in] timer/counter handle
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_init(int handle);

/**
 * @brief the timer/counter device suspend function
 * @param devId [in] device id
 * @return success: 0,  erro: ERROR_ID
 */
void gp_tc_suspend_set( int devId );

/**
 * @brief the timer/counter device resume function
 * @param devId [in] device id
 * @return success: 0,  erro: ERROR_ID
 */
void gp_tc_resume_set( int devId );

#endif	/*_SPMP_TIMER_H_*/
