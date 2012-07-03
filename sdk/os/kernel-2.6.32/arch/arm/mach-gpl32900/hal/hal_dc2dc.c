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
 *  Generalplus Inc.                                                      *
 *  3F, No.8, Dusing Rd., Hsinchu Science Park,                           *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
 
/**
 * @file hal_dc2dc.c
 * @brief dc2dc HAL interface 
 * @author Daniel Huang
 */

#include <mach/kernel.h>
#include <mach/hal/regmap/reg_dc2dc.h>

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**
 * dc2dc HAL interface
 */

/**
 * @brief the dc2dc init function
 */
void gpHalDc2dcInit(void)
{
	dc2dcReg_t *pdc2dcReg = (dc2dcReg_t *)LOGI_ADDR_DC2DC_REG;

	pdc2dcReg->dc2dcCtr = 0;
}

/**
 * @brief the dc2dc enable PWM0 function
 * @param enable [in] 0:disable; 1:enable
 */
void gpHalDc2dcEnablePWM0(int enable)
{
	dc2dcReg_t *pdc2dcReg = (dc2dcReg_t *)LOGI_ADDR_DC2DC_REG;
	UINT32 temp = pdc2dcReg->dc2dcCtr;

	if(enable) {
		temp |= (DC2DC_CLK6M_ENABLE | DC2DC_PWM0_ENABLE);
	}
	else {
		temp &= ~DC2DC_PWM0_ENABLE;
		if( !(temp & DC2DC_PWM1_ENABLE) ) /*if PWM1 also disabled, then shut down clock*/
			temp &= ~DC2DC_CLK6M_ENABLE;
	}
	pdc2dcReg->dc2dcCtr = temp;
}

/**
 * @brief the dc2dc enable PWM1 function
 * @param enable [in] 0:disable; 1:enable
 */
void gpHalDc2dcEnablePWM1(int enable)
{
	dc2dcReg_t *pdc2dcReg = (dc2dcReg_t *)LOGI_ADDR_DC2DC_REG;

	UINT32 temp = pdc2dcReg->dc2dcCtr;

	if(enable) {
		temp |= (DC2DC_CLK6M_ENABLE | DC2DC_PWM1_ENABLE 
			| DC2DC_VSET0_ENABLE | DC2DC_VSET1_ENABLE | DC2DC_VSET2_ENABLE);
	}
	else {
		temp &= ~DC2DC_PWM1_ENABLE;
		if( !(temp & DC2DC_PWM0_ENABLE) ) /*if PWM0 also disabled, then shut down clock*/
			temp &= ~DC2DC_CLK6M_ENABLE;
	}
	pdc2dcReg->dc2dcCtr = temp;
}

/**
 * @brief the dc2dc disable function
 * @param enable [in] 0:disable; 1:enable
 */
void gpHalDc2dcDisable(void)
{
	dc2dcReg_t *pdc2dcReg = (dc2dcReg_t *)LOGI_ADDR_DC2DC_REG;

	pdc2dcReg->dc2dcCtr = 0;
}

/**
 * @brief the dc2dc setting feedback voltage function
 * @param voltage [in]
 */
void gpHalDc2dcSetFeedbackVoltage(int voltage)
{
	dc2dcReg_t *pdc2dcReg = (dc2dcReg_t *)LOGI_ADDR_DC2DC_REG;
	UINT32 val = pdc2dcReg->dc2dcCtr & (~DC2DC_VSET_MASK);
	
	if(voltage > 7) {
	        val |= (7<<4);
	}
	else if(voltage < 1) {
	        val |= (1<<4);
	}
	else {
	        val |= (voltage<<4);
	}
	pdc2dcReg->dc2dcCtr = val;
}

/**
 * @brief the dc2dc enable CLK6M_PWM function
 * @param enable [in] 0:disable; 1:enable
 */
void gpHalDc2dcEnableCLK6M(int enable)
{
	dc2dcReg_t *pdc2dcReg = (dc2dcReg_t *)LOGI_ADDR_DC2DC_REG;

	UINT32 temp = pdc2dcReg->dc2dcCtr;

	if(enable) {
		temp |= DC2DC_CLK6M_ENABLE;
	}else {
		temp &= ~DC2DC_CLK6M_ENABLE;
	}
	pdc2dcReg->dc2dcCtr = temp;
}