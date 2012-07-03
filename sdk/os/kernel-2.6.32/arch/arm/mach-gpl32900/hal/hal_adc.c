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
 * @file arch/arm/mach-spmp8050/hal/hal_adc.c
 * @brief ADC hareware abstract level
 * @author zh.l
 */

#include <mach/hal/hal_adc.h>	/*functions*/
#include <mach/hal/regmap/reg_adc.h>
#include <linux/clk.h>	/*clk_xxx*/
#include <linux/delay.h>

/**
 *@brief SARADC interrupt enable control
 *@param mask[in]: mask bits of interrupts, 1-effected,0-ignored
 *@param newstate[in]: new states of interrupts, 1-enable,0-disable
 *@return none
 */
void gpHalAdcSetIntEn(UINT32 mask, UINT32 newState)
{
	UINT32 temp;
	regADC_t *pAdc = (regADC_t *)(LOGI_ADDR_ADC_REG);

	temp = pAdc->INTEN;
	temp &= ~mask;
	temp |= newState;
	pAdc->INTEN = temp;
}

/**
 *@brief read SARADC interrupt enable status
 *@param mask[in]: mask bits of interrupts, 1-effected,0-ignored
 *@return SARADC interrupt enable state
 */
UINT32 gpHalAdcGetIntEn(void)
{
	regADC_t *pAdc = (regADC_t *)(LOGI_ADDR_ADC_REG);
	return (pAdc->INTEN);
}

/**
 *@brief read SARADC interrupt flags
 *@return SARADC interrupt flags
 */
UINT32 gpHalAdcGetIntFlag()
{
	regADC_t *pAdc = (regADC_t *)(LOGI_ADDR_ADC_REG);
	return (pAdc->INTF);
}

/**
 *@brief start SARADC AD conversion
 *@param mode[in]: ad convert mode, should be one of touch auto/touch manual/aux manual
 *@param arg[in]: in aux manual mode, specify the aux channel, other mode, please keep it 0
 *@return none
 */
void gpHalAdcStartConv(UINT32 mode, UINT32 arg)
{
	UINT32 temp;
	regADC_t *pAdc = (regADC_t *)(LOGI_ADDR_ADC_REG);

	switch(mode) {
	case MODE_TP_AUTO:/*touch pannel auto mode*/
		pAdc->SARCTRL |= ADC_CTL_AUTO_CON_ON;
		break;
		
	case MODE_TP_MANUAL:/*touch pannel manaul mode*/
		if( 0 == arg ) {
			temp = pAdc->SARCTRL;
			temp &= 0xFFFF80;
			temp |= ADC_CTL_MAN_CON_ON | (0x03 << ADC_CTL_TPS_OFST );
			pAdc->SARCTRL = temp;
		}
		else {
			temp = pAdc->SARCTRL;
			temp &= 0xFFFF80;
			temp |= ADC_CTL_MAN_CON_ON | (0x02 << ADC_CTL_TPS_OFST );
			pAdc->SARCTRL = temp;
		}
		break;
		
	case MODE_AUX:/*AUX channel*/
		temp = pAdc->SARCTRL;
	
		temp &= ~((ADC_CTL_TPS_MASK << ADC_CTL_TPS_OFST) |
				(ADC_CTL_SARS_MASK << ADC_CTL_SARS_OFST));
		temp |= 0x01 << ADC_CTL_TPS_OFST;	/*normal ADC*/
		//temp |= 0x00 << ADC_CTL_TPS_OFST;	/*normal ADC*/
		if(8 == arg) {
			temp |= (ADC_CTL_CH8_SEL_MASK << ADC_CTL_CH8_SEL_OFST);
		}else{
			temp &= ~(ADC_CTL_CH8_SEL_MASK << ADC_CTL_CH8_SEL_OFST);
		}
		temp |= (arg & ADC_CTL_SARS_MASK) << ADC_CTL_SARS_OFST;	/*set SARS channel*/
		temp |= ADC_CTL_MAN_CON_ON;	/*manual convert*/

		pAdc->SARCTRL = temp;
		break;
		
	default:
		break;
	}
}

/**
 *@brief stop SARADC AD conversion
 *@param mode[in]: ad convert mode, should be one of touch auto/touch manual/aux manual
 *@return none
 */
void gpHalAdcStopConv(UINT32 mode)
{
	regADC_t *pAdc = (regADC_t *)(LOGI_ADDR_ADC_REG);

	switch(mode) {
	case MODE_TP_AUTO:/*touch pannel auto mode*/
		pAdc->SARCTRL &= ~ADC_CTL_AUTO_CON_ON;
		break;
		
	case MODE_TP_MANUAL:/*touch pannel manaul mode*/
		/*what should I do?*/
		break;
		
	case MODE_AUX:/*AUX channel*/
		pAdc->SARCTRL &= ~ADC_CTL_MAN_CON_ON;
		break;
		
	default:
		break;
		
	}
}

/**
 *@brief  set SARADC clock rate
 *@param clk_rate[in]: clock rate of SARADC, value should between 384KHz-2MHz
 *@return none
 */
void gpHalAdcSetClkRate(UINT32 clk_rate)
{
	regADC_t *pAdc = (regADC_t *)(LOGI_ADDR_ADC_REG);
	unsigned long pclk_rate=0;
	struct clk* pclk;
	UINT32 temp;

	pclk = clk_get(NULL, "clk_sys_apb");
	if( pclk ) {
		pclk_rate = clk_get_rate( pclk );
		clk_put(pclk);
	}
	else {
		pclk_rate = 27000000ul;	/*default to 27MHz*/
	}
	/*limit the sar clock between 384K-2MHz*/
	if( clk_rate < 384000ul ) {
		clk_rate = 384000ul;
	}
	else if( clk_rate > 2000000ul ) {
		clk_rate = 2000000ul;
	}

	clk_rate = pclk_rate / 2 /clk_rate;
	
	temp = pAdc->SARCTRL;
	temp &= ~(ADC_CTL_DIVNUM_MASK << ADC_CTL_DIVNUM_OFST);
	temp |= ((clk_rate - 1) << ADC_CTL_DIVNUM_OFST);
	pAdc->SARCTRL = temp;
}

/**
 *@brief  SARADC initialize
 *@param psarInit[in]: point of sar_init_t struct, for adc initialize parameters
 *@return none
 */
void gpHalAdcInit(adc_init_t* pAdcInit)
{
	UINT32 temp;
	regADC_t *pAdc = (regADC_t *)(LOGI_ADDR_ADC_REG);

	temp = (pAdcInit->interval_dly << 5) | (pAdcInit->x2y_dly);
	pAdc->AUTODLY = temp;

	temp = (pAdcInit->chkdly << 16) | (pAdcInit->debounce_dly);
	pAdc->DEBTIME = temp;

	pAdc->CONDLY = pAdcInit->conv_dly;

	gpHalAdcSetClkRate(pAdcInit->clock_rate);

	temp = pAdc->SARCTRL;
	temp &= ~(ADC_CTL_TOGEN_MASK << ADC_CTL_TOGEN_OFST);
	temp |= (pAdcInit->clk_tog_en) ? (ADC_CTL_TOGEN_MASK << ADC_CTL_TOGEN_OFST) : 0;
	pAdc->SARCTRL = temp;
}

/**
 *@brief  get touch panel pen location(co-ordination)
 *@return combinated x/y location of touch panel pen location
 */
UINT32 gpHalAdcGetPNL(void)
{
	regADC_t *pAdc = (regADC_t *)(LOGI_ADDR_ADC_REG);
	return pAdc->PNL;
}

/**
 *@brief  get touch panel aux channel convert result
 *@return aux channel convert result
 */
UINT32 gpHalAdcGetAUX(void)
{
	regADC_t *pAdc = (regADC_t *)(LOGI_ADDR_ADC_REG);
	return pAdc->AUX;
}

