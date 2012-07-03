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

#ifndef __HAL_SAR_H__
#define __HAL_SAR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <mach/typedef.h>

#define MODE_TP_AUTO	0	/** touch screen automatic conversion mode */
#define MODE_TP_MANUAL	1	/** touch screen manual conversion mode */
#define MODE_AUX	2	/** AUX data conversion */


typedef struct adc_init_s {
	UINT8	clk_tog_en;	/** 1-always toggling clock,0-toggling only in measurement*/
	UINT8	conv_dly;	/** conversion delay in SARCLK cycles*/
	UINT8	chkdly;		/** pen status re-check delay */
	UINT8	x2y_dly;	/** auto conversion mode,internal delay from x to y conversion*/
	UINT16	interval_dly;	/** auto conversion mode,internal delay between to x/y conversion*/
	UINT16	debounce_dly;	/** SARADC de-bounce delay value*/
	UINT32	clock_rate;	/** SARADC clock rate, should between 384KHz and 2MHz*/
}adc_init_t;

/*export functions*/
/**
 *@brief SARADC interrupt enable control
 *@param mask[in]: mask bits of interrupts, 1-effected,0-ignored
 *@param newstate[in]: new states of interrupts, 1-enable,0-disable
 *@return none
 */
void gpHalAdcSetIntEn(UINT32 mask, UINT32 newState);

/**
 *@brief read SARADC interrupt enable status
 *@param mask[in]: mask bits of interrupts, 1-effected,0-ignored
 *@return SARADC interrupt enable state
 */
UINT32 gpHalAdcGetIntEn(void);

/**
 *@brief read SARADC interrupt flags
 *@param mask[in]: mask bits of interrupts, 1-effected,0-ignored
 *@return SARADC interrupt flags
 */
UINT32 gpHalAdcGetIntFlag(void);

/**
 *@brief start SARADC AD conversion
 *@param mode[in]: ad convert mode, should be one of touch auto/touch manual/aux manual
 *@param arg[in]: in aux manual mode, specify the aux channel, other mode, please keep it 0
 *@return none
 */
void gpHalAdcStartConv(UINT32 mode, UINT32 arg);

/**
 *@brief stop SARADC AD conversion
 *@param mode[in]: ad convert mode, should be one of touch auto/touch manual/aux manual
 *@return none
 */
void gpHalAdcStopConv(UINT32 mode);

/**
 *@brief  set SARADC clock rate
 *@param clk_rate[in]: clock rate of SARADC, value should between 384KHz-2MHz
 *@return none
 */
void gpHalAdcSetClkRate(UINT32 clk_rate);

/**
 *@brief  SARADC initialize
 *@param psarInit[in]: point of sar_init_t struct, for adc initialize parameters
 *@return none
 */
void gpHalAdcInit(adc_init_t* pAdcInit);

/**
 *@brief  get touch panel pen location(co-ordination)
 *@return combinated x/y location of touch panel pen location
 */
UINT32 gpHalAdcGetPNL(void);

/**
 *@brief  get touch panel aux channel convert result
 *@return aux channel convert result
 */
UINT32 gpHalAdcGetAUX(void);


#ifdef __cplusplus
}
#endif
#endif
