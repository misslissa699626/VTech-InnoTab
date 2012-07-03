#include <linux/clk.h>	/*clk_get*/
#include <linux/module.h>	/*EXPORT_SYMBOL*/
#include <mach/common.h>	/*data types*/
#include <mach/hardware.h>
#include <mach/regs-saradc.h>	/*registers*/
#include <mach/hal/hal_sar.h>	/*functions*/

typedef struct regSARADC_s {
	volatile UINT32	SARCTRL;
	volatile UINT32	CONDLY;
	volatile UINT32	AUTODLY;
	volatile UINT32	DEBTIME;
	volatile UINT32	PNL;
	volatile UINT32	AUX;
	volatile UINT32	INTEN;
	volatile UINT32	INTF;
}regSARADC_t;

static volatile UINT32 saradc_intf = 0;
static regSARADC_t *this = (regSARADC_t *)(IO3_ADDRESS(0x1F000));

/**
 *@brief SARADC interrupt enable control
 *@param mask[in]: mask bits of interrupts, 1-effected,0-ignored
 *@param newstate[in]: new states of interrupts, 1-enable,0-disable
 *@return none
 */
void gpHalSarSetIntEn(UINT32 mask,UINT32 newstate)
{
	UINT32 temp;
	temp = this->INTEN;
	temp &= ~mask;
	temp |= newstate;
	this->INTEN = temp;
}
EXPORT_SYMBOL(gpHalSarSetIntEn);

/**
 *@brief read SARADC interrupt enable status
 *@param mask[in]: mask bits of interrupts, 1-effected,0-ignored
 *@return SARADC interrupt enable state
 */
UINT32 gpHalSarGetIntEn(UINT32 mask)
{
	return (this->INTEN & mask);
}
EXPORT_SYMBOL(gpHalSarGetIntEn);

/**
 *@brief read SARADC interrupt flags
 *@param mask[in]: mask bits of interrupts, 1-effected,0-ignored
 *@return SARADC interrupt flags
 */
UINT32 gpHalSarGetIntFlag(UINT32 mask)
{
	//saradc_intf |= this->INTF;	/*INTF is read clear register*/
	//return (saradc_intf & mask);
	return (this->INTF) ;
}
EXPORT_SYMBOL(gpHalSarGetIntFlag);

/**
 *@brief clear SARADC interrupt flags
 *@param mask[in]: mask bits of interrupts, 1-effected,0-ignored
 *@return nont
 */
void gpHalSarClrIntFlag(UINT32 mask)
{
	saradc_intf &= ~mask;
}
EXPORT_SYMBOL(gpHalSarClrIntFlag);

/**
 *@brief start SARADC AD conversion
 *@param mode[in]: ad convert mode, should be one of touch auto/touch manual/aux manual
 *@param arg[in]: in aux manual mode, specify the aux channel, other mode, please keep it 0
 *@return none
 */
void gpHalSarStartConv(UINT32 mode,UINT32 arg)
{
	UINT32 temp;
	switch(mode) {
	case MODE_TP_AUTO:/*touch pannel auto mode*/
		this->SARCTRL |= SARCTL_SAR_AUTO_CON_ON;
		break;
	case MODE_TP_MANUAL:/*touch pannel manaul mode*/
		if( 0==arg ) {
			temp = this->SARCTRL;
			temp &= 0xFFFF80;
			temp |= SARCTL_SAR_MAN_CON_ON | (0x03<<SAACC_SAR_TPS_OFST );
			this->SARCTRL = temp;
		}
		else {
			temp = this->SARCTRL;
			temp &= 0xFFFF80;
			temp |= SARCTL_SAR_MAN_CON_ON | (0x02<<SAACC_SAR_TPS_OFST );
			this->SARCTRL = temp;
		}
		break;
	case MODE_AUX:/*AUX channel*/
		temp = this->SARCTRL;
	
		temp &= ~( (SAACC_SAR_TPS_MASK << SAACC_SAR_TPS_OFST) |
				(SAACC_SAR_SARS_MASK << SAACC_SAR_SARS_OFST)	);
		temp |= 0x01 << SAACC_SAR_TPS_OFST;	/*normal ADC*/
		temp |= (arg & SAACC_SAR_SARS_MASK) << SAACC_SAR_SARS_OFST;	/*set SARS channel*/
		temp |= SARCTL_SAR_MAN_CON_ON;	/*manual convert*/

		this->SARCTRL = temp;
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(gpHalSarStartConv);

/**
 *@brief stop SARADC AD conversion
 *@param mode[in]: ad convert mode, should be one of touch auto/touch manual/aux manual
 *@return none
 */
void gpHalSarStopConv(UINT32 mode)
{
	switch(mode) {
	case MODE_TP_AUTO:/*touch pannel auto mode*/
		this->SARCTRL &= ~SARCTL_SAR_AUTO_CON_ON;
		break;
	case MODE_TP_MANUAL:/*touch pannel manaul mode*/
		/*nothing to do,after conversion, user should start another conversion*/
		break;
	case MODE_AUX:/*AUX channel*/
		this->SARCTRL &= ~SARCTL_SAR_MAN_CON_ON;
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(gpHalSarStopConv);

/**
 *@brief  SARADC initialize
 *@param psarInit[in]: point of sar_init_t struct, for adc initialize parameters
 *@return none
 */
void gpHalSarInit(sar_init_t* psarInit)
{
	UINT32 temp;

	temp = (psarInit->interval_dly<<5) | (psarInit->x2y_dly);
	this->AUTODLY = temp;

	temp = (psarInit->chkdly<<16) | (psarInit->debounce_dly);
	this->DEBTIME = temp;

	this->CONDLY = psarInit->conv_dly;

	gpHalSarSetClkRate(psarInit->clock_rate);

	temp = this->SARCTRL;
	temp &= ~(SAACC_SAR_TOGEN_MASK << SAACC_SAR_TOGEN_OFST);
	temp |= (psarInit->clk_tog_en) ? (SAACC_SAR_TOGEN_MASK << SAACC_SAR_TOGEN_OFST) : 0;
	this->SARCTRL = temp;
}
EXPORT_SYMBOL(gpHalSarInit);

/**
 *@brief  set SARADC clock rate
 *@param clk_rate[in]: clock rate of SARADC, value should between 384KHz-2MHz
 *@return none
 */
void gpHalSarSetClkRate(UINT32 clk_rate)
{
	unsigned long pclk_rate=0;
	struct clk* pclk;
	UINT32 temp;

	pclk = clk_get(NULL, "clk_arm_apb");
	if( pclk ) {
		pclk_rate = clk_get_rate( pclk );
		clk_put(pclk);
	}
	else {
		pclk_rate = 27000000ul;	/*default to 27MHz*/
	}
	/*limit the sar clock between 384K-2MHz*/
	if( clk_rate < 384000ul )
		clk_rate = 384000ul;
	else if( clk_rate > 2000000ul )
		clk_rate = 2000000ul;

	clk_rate = pclk_rate/clk_rate;
	
	temp = this->SARCTRL;
	temp &= ~(SAACC_SAR_DIVNUM_MASK<<SAACC_SAR_DIVNUM_OFST);
	temp |= ((clk_rate-1)<<SAACC_SAR_DIVNUM_OFST);
	this->SARCTRL = temp;
}
EXPORT_SYMBOL(gpHalSarSetClkRate);

/**
 *@brief  get touch panel pen location(co-ordination)
 *@return combinated x/y location of touch panel pen location
 */
UINT32 gpHalSarGetPNL(void)
{
	return this->PNL;
}
EXPORT_SYMBOL(gpHalSarGetPNL);

/**
 *@brief  get touch panel aux channel convert result
 *@return aux channel convert result
 */
UINT32 gpHalSarGetAUX(void)
{
	return this->AUX;
}
EXPORT_SYMBOL(gpHalSarGetAUX);

