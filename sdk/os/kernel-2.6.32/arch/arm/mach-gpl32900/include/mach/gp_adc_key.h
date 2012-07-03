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
 * @file gp_adc_key.h
 * @brief ADC key driver header file
 * @author zh.l
 */

#ifndef _GP_ADC_KEYS_H
#define _GP_ADC_KEYS_H 

#ifdef __cplusplus
extern "C" {
#endif

/** adc per key data*/
struct gp_adc_keys_button {
	/* Configuration parameters */
	int code;		/* input event code (KEY_*, SW_*) */
	int min;		/*key ad min value*/
	int max;		/*key ad max value*/
	char *desc;
	int pressed		/*key is pressed or not*/
};

struct gp_adc_keys_platform_data {
	struct gp_adc_keys_button *buttons;	/** button data*/
	int nr_buttons;		/** number of buttons*/
	int channel;		/** which channel?*/
};

#ifdef __cplusplus
}
#endif
#endif



