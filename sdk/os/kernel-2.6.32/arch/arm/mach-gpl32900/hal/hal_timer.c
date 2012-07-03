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
 * @file    hal_timer.h
 * @brief   Declaration of timer/counter hal interface.
 * @author  zaimingmeng
 */

#include <mach/hardware.h>
#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/hal/regmap/reg_timer.h>
#include <mach/hal/hal_timer.h>


static int g_timer_load_value[5] = {0};
/*
 * HAL interface
 */

/**
 * @brief the timer/counter regs clean function
 * @param handle [in] timer/counter handle
 */
void gpHalTimerInit(int id)
{
	timerReg_t *ptimerReg = (timerReg_t *)(LOGI_ADDR_TIMER_REG + id*LOGI_TIMER_OFFSET);
	ptimerReg->tmCtr = 0;
	ptimerReg->tmPsr = 0;
	ptimerReg->tmLdrVlr = 0;
	ptimerReg->tmIsr = 0;
	ptimerReg->tmCmp = 0;
}

/**
 * @brief the timer/counter get base clock
 * @return base clk
 */
int gpHalTimerGetBaseClk(void)
{
	UINT32 apbHz;
	struct clk *apbClk;

	apbClk = clk_get(NULL,"clk_arm_apb");
	if(apbClk){
		apbHz = clk_get_rate(apbClk);
		clk_put(apbClk);
	}
	else{
		apbHz = 21200000;
	}
	
	return apbHz;
}

/**
 * @brief the timer/counter set control register function;provide for pwm module
 * @param value [in] control register value
 */
void gpHalTimerSetCtrl(int id, int value)
{
	timerReg_t *ptimerReg = (timerReg_t *)(LOGI_ADDR_TIMER_REG + id*LOGI_TIMER_OFFSET);
	
	ptimerReg->tmCtr = value;
}
EXPORT_SYMBOL(gpHalTimerSetCtrl);

/**
 * @brief the timer/counter enable/disable function
 * @param enable [in] 0:disable; 1:enable
 */
void gpHalTimerEn(int id, int enable)
{
	int temp;
	timerReg_t *ptimerReg = (timerReg_t *)(LOGI_ADDR_TIMER_REG + id*LOGI_TIMER_OFFSET);
	
	temp = ptimerReg->tmCtr;
	
	if(enable){
		ptimerReg->tmCtr = temp|TMR_ENABLE;
	}
	else{
		ptimerReg->tmCtr = temp&(~TMR_ENABLE);
	}
}

/**
 * @brief timer/counter output enable regsiter get
 * @param id [in] timer/counter index
 * @return mode 0:disable; 1:enable
 */
int gpHalTimerEnGet(int id)
{
	timerReg_t *ptimerReg = (timerReg_t *)(LOGI_ADDR_TIMER_REG + id*LOGI_TIMER_OFFSET);
	
	if( (ptimerReg->tmCtr & TMR_ENABLE) != 0 ) {
		return 1;
	}
	else {
		return 0;
	}
}
EXPORT_SYMBOL(gpHalTimerEnGet);

/**
 * @brief the timer/counter interrupter enable/disable function
 * @param enable [in] 0:disable; 1:enable
 */
void gpHalTimerIntEn(int id, int enable)
{
	int temp;
	timerReg_t *ptimerReg = (timerReg_t *)(LOGI_ADDR_TIMER_REG + id*LOGI_TIMER_OFFSET);
	
	temp = ptimerReg->tmCtr;

	if(enable){
		ptimerReg->tmCtr = temp|TMR_IE_ENABLE;
	}
	else{
		ptimerReg->tmCtr = temp&(~TMR_IE_ENABLE);
	}	
}

/**
 * @brief timer/counter output regsiter set
 * @param id [in] timer/counter index
 * @param enable [in] 0:disable timer output; 1:enable timer output
 */
void gpHalTimerOeEnSet(int id, int enable)
{
	int temp;	
	timerReg_t *ptimerReg = (timerReg_t *)(LOGI_ADDR_TIMER_REG + id*LOGI_TIMER_OFFSET);
	
	temp = ptimerReg->tmCtr;

	if(enable){
		ptimerReg->tmCtr = temp|TMR_OE_ENABLE;
	}
	else{
		ptimerReg->tmCtr = temp&(~TMR_OE_ENABLE);
	}	
}

/**
 * @brief timer/counter output mode regsiter set
 * @param id [in] timer/counter index
 * @param mode [in] 0:normal mode; 1:PWM mode
 */
void gpHalTimerOutputModeSet(int id, int mode)
{
	int temp;	
	timerReg_t *ptimerReg = (timerReg_t *)(LOGI_ADDR_TIMER_REG + id*LOGI_TIMER_OFFSET);
	
	temp = ptimerReg->tmCtr;

	if(mode){
		ptimerReg->tmCtr = temp|TMR_OE_PWM;
	}
	else{
		ptimerReg->tmCtr = temp&(~TMR_OE_PWM);
	}
}

/**
 * @brief timer/counter clock source select
 * @param id [in] timer/counter index
 * @param mode [in] 0:PCLK, 1:32768 Hz, 2:1.6875MHz
 */
void gpHalTimerClkSrcSet(int id, int clkSrc)
{
	int temp;	
	timerReg_t *ptimerReg = (timerReg_t *)(LOGI_ADDR_TIMER_REG + id*LOGI_TIMER_OFFSET);
	
	temp = ptimerReg->tmCtr;
	
	if( clkSrc == 0 ){
		ptimerReg->tmCtr = temp|TMR_CLK_SRC_PCLK;
	}
	else if( clkSrc == 1 ) {
		ptimerReg->tmCtr = temp|TMR_CLK_SRC_32768;
	}
	else if( clkSrc == 2 ) {
		ptimerReg->tmCtr = temp|TMR_CLK_SRC_1P6875;
	}
}

/**
 * @brief timer/counter output mode regsiter get
 * @param id [in] timer/counter index
 * @return mode 0:normal mode; 1:PWM mode
 */
int gpHalTimerOutputModeGet(int id)
{
	timerReg_t *ptimerReg = (timerReg_t *)(LOGI_ADDR_TIMER_REG + id*LOGI_TIMER_OFFSET);
	
	if( (ptimerReg->tmCtr & TMR_OE_PWM) == TMR_OE_PWM ) {
		/*PWM Mode*/
		return 1;
	}
	else {
		/*Timer Mode*/
		return 0;
	}
}
EXPORT_SYMBOL(gpHalTimerOutputModeGet);

/**
 * @brief timer/counter count up or down selection set
 * @param id [in] timer/counter handle
 * @param mode [in] dir value: 0:down counting; 1:up counting;
 */
void gpHalTimerUdSet(int id, int mode)
{
	int temp;	
	timerReg_t *ptimerReg = (timerReg_t *)(LOGI_ADDR_TIMER_REG + id*LOGI_TIMER_OFFSET);
	
	temp = ptimerReg->tmCtr;

	if(mode){
		ptimerReg->tmCtr = temp|TMR_UD_UP;
	}
	else{
		ptimerReg->tmCtr = temp&(~TMR_UD_UP);
	}
}

/**
 * @brief up/down counting control selection set
 * @param id [in] timer/counter index
 * @param mode [in] dir value: 0:up/down control by bit4 TxCTR;
 *                 1:up/down control by EXTUDx input;
 */
void gpHalTimerUdsSet(int id, int mode)
{
	int temp;	
	timerReg_t *ptimerReg = (timerReg_t *)(LOGI_ADDR_TIMER_REG + id*LOGI_TIMER_OFFSET);
	
	temp = ptimerReg->tmCtr;

	if(mode){
		ptimerReg->tmCtr = temp|TMR_UDS_EXTUD;
	}
	else{
		ptimerReg->tmCtr = temp&(~TMR_UDS_EXTUD);
	}	
}

/**
 * @brief timer/counter output mode set,only effect when timer output is in normal mode.
 * @param id [in] timer/counter index
 * @param mode [in] dir value: 0:toggle mode; 1:pulse mode
 */
void gpHalTimerOmSet(int id, int mode)
{
	int temp;	
	timerReg_t *ptimerReg = (timerReg_t *)(LOGI_ADDR_TIMER_REG + id*LOGI_TIMER_OFFSET);
	
	temp = ptimerReg->tmCtr;

	if(mode){
		ptimerReg->tmCtr = temp|TMR_OM_PULSE;
	}
	else{
		ptimerReg->tmCtr = temp&(~TMR_OM_PULSE);
	}
}

/**
 * @brief external input active edge set
 * @param id [in] timer/counter index
 * @param mode [in] dir value: 0:positive edge; 1:negative edge;
 *                 2:both edge;
 */
void gpHalTimerEsSet(int id, int mode)
{
	int temp;	
	timerReg_t *ptimerReg = (timerReg_t *)(LOGI_ADDR_TIMER_REG + id*LOGI_TIMER_OFFSET);
	
	temp = ptimerReg->tmCtr;

	temp = temp&(~(TMR_ES_NE|TMR_ES_BOTH));
	if(2 == mode){	
		ptimerReg->tmCtr = temp|TMR_ES_BOTH;
	}
	else if(1 == mode){
		ptimerReg->tmCtr = temp|TMR_ES_NE;
	}
	else{
		ptimerReg->tmCtr = temp;
	}
}

/**
 * @brief timer operating mode set
 * @param id [in] timer/counter index
 * @param mode [in]  mode value: 0:free running time mode; 1:period timer mode; 
 *		2:free running counter mode; 3:period counter mode
 */
void gpHalTimerMSet(int id, int mode)
{
	int temp;	
	timerReg_t *ptimerReg = (timerReg_t *)(LOGI_ADDR_TIMER_REG + id*LOGI_TIMER_OFFSET);
	
	if(mode > 3)
		return;

	temp = ptimerReg->tmCtr;
	temp = temp&(~TMR_M_PERIOD_COUNTER);
	
	ptimerReg->tmCtr = temp|(mode<<10);	
	
}

/**
 * @brief load value set
 * @param id [in] timer/counter index
 * @param value [in] timer/counter load value
 */
void gpHalTimerLoadSet(int id, int value)
{
	timerReg_t *ptimerReg = (timerReg_t *)(LOGI_ADDR_TIMER_REG + id*LOGI_TIMER_OFFSET);

	ptimerReg->tmLdrVlr = value;
	/*Save value for the data storing in suspend.*/
	g_timer_load_value[id] = value;
}

/**
 * @brief load value get
 * @param id [in] timer/counter index
 * @param value [out] timer/counter load value
 */
void gpHalTimerLoadGet(int id, int *value)
{
	timerReg_t *ptimerReg = (timerReg_t *)(LOGI_ADDR_TIMER_REG + id*LOGI_TIMER_OFFSET);

	*value = ptimerReg->tmLdrVlr;
}

/**
 * @brief timer compare value set(used only in pwm mode)
 * @param id [in] timer/counter index
 * @param value [in] timer/counter compare value
 */
void gpHalTimerCmpSet(int id, int value)
{	
	timerReg_t *ptimerReg = (timerReg_t *)(LOGI_ADDR_TIMER_REG + id*LOGI_TIMER_OFFSET);

	ptimerReg->tmCmp = value;
}

/**
 * @brief timer compare value get(used only in pwm mode)
 * @param id [in] timer/counter index
 * @param value [out] timer/counter compare value
 */
void gpHalTimerCmpGet(int id, int *value)
{	
	timerReg_t *ptimerReg = (timerReg_t *)(LOGI_ADDR_TIMER_REG + id*LOGI_TIMER_OFFSET);

	*value = ptimerReg->tmCmp;
}

/**
 * @brief timer/counter clock prescale set
 * @param id [in] timer/counter index
 * @param value [in] timer/counter prescale value
 */
void gpHalTimerPrescaleSet(int id, int value)
{	
	timerReg_t *ptimerReg = (timerReg_t *)(LOGI_ADDR_TIMER_REG + id*LOGI_TIMER_OFFSET);

	ptimerReg->tmPsr = value;
}

/**
 * @brief timer/counter clock prescale get
 * @param id [in] timer/counter index
 * @param value [out] timer/counter prescale value
 */
void gpHalTimerPrescaleGet(int id, int *value)
{
	timerReg_t *ptimerReg = (timerReg_t *)(LOGI_ADDR_TIMER_REG + id*LOGI_TIMER_OFFSET);

	*value = ptimerReg->tmPsr;
}

/**
 * @brief use to clear interrupt flags
 * @param id [in] timer/counter index
 * @param value [in] 0:clear interrupter flag
 */
void gpHalTimerInterruptSet(int id, int value)
{	
	timerReg_t *ptimerReg = (timerReg_t *)(LOGI_ADDR_TIMER_REG + id*LOGI_TIMER_OFFSET);

	ptimerReg->tmIsr = value;
}

/**
 * @brief get interrupt flags
 * @param id [in] timer/counter index
 * @param value [out]  interrupter flag
 */
void gpHalTimerInterruptGet(int id, int *value)
{
	timerReg_t *ptimerReg = (timerReg_t *)(LOGI_ADDR_TIMER_REG + id*LOGI_TIMER_OFFSET);

	*value = ptimerReg->tmIsr;
}


/**
 * @brief Save register value to ptr
 * @param id [in] timer/counter index
 * @param ptr [out] register point
 */
void gpHalTimerRegSave( int id, int* ptr ){

	int j = 0;
	timerReg_t *ptimerReg = (timerReg_t *)(LOGI_ADDR_TIMER_REG + id*LOGI_TIMER_OFFSET);

	ptr[j++] = ptimerReg->tmCtr;
	ptr[j++] = ptimerReg->tmPsr;
	/*We store the loading value at the value setting.*/
	ptr[j++] = g_timer_load_value[id];
	ptr[j++] = ptimerReg->tmIsr;
	ptr[j++] = ptimerReg->tmCmp;	
	
}
EXPORT_SYMBOL(gpHalTimerRegSave);

/**
 * @brief Restore register value from ptr
 * @param id [in] timer/counter index
 * @param ptr [out] register point
 */
void gpHalTimerRegRestore ( int id, int* ptr ){
	timerReg_t *ptimerReg = (timerReg_t *)(LOGI_ADDR_TIMER_REG + id*LOGI_TIMER_OFFSET);

	ptimerReg->tmPsr = ptr[1];
	ptimerReg->tmLdrVlr = ptr[2];
	ptimerReg->tmIsr = ptr[3];
	ptimerReg->tmCmp = ptr[4];
	/*Enable timer/pwm in the end.*/
    // disable pwm ctrl first 
	ptimerReg->tmCtr = 0x0;
	ptimerReg->tmCtr = ptr[0];
}
EXPORT_SYMBOL(gpHalTimerRegRestore);

