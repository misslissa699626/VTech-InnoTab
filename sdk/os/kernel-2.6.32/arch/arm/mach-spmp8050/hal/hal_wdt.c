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
 * @file hal_wdt.c
 * @brief watchdog HAL interface 
 * @author zaimingmeng
 */

#include <mach/kernel.h>
#include <mach/hal/regmap/reg_wdt.h>

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define GP_WDT_FEED_VAL			(0x1225)
#define GP_WDT_RESET_VAL		(~0x1225)

/**
 * watchdog HAL interface
 */

/**
 * @brief the watchdog init function
 */
void gpHalWdtInit(void)
{
	wdtReg_t *pwdtReg = (wdtReg_t *)LOGI_ADDR_WDT_REG;

	pwdtReg->wdtCtr = 0;
	pwdtReg->wdtPsr = 0;
	pwdtReg->wdtLdr = 0;
	pwdtReg->wdtVlr = 0;
	pwdtReg->wdtCmp = 0;

}

/**
 * @brief the watchdog get base clock function
 * @return watchdog base clock
 */
static UINT32 gpHalWdtGetBaseClk(void)
{
	unsigned long clk_rate;
	/*struct clk *pclk;

	pclk = clk_get(NULL,"xtal");

	if(pclk){
		clk_rate = clk_get_rate(pclk);	
		clk_put(pclk);
	}
	*/
	clk_rate = 27000000;
	return clk_rate;
}

/**
 * @brief the watchdog enable function
 * @param enable [in] 0:disable; 1:enable
 */
void gpHalWdtEnable(int enable)
{
	wdtReg_t *pwdtReg = (wdtReg_t *)LOGI_ADDR_WDT_REG;

	if(enable){
		pwdtReg->wdtCtr = WDT_RE_ENABLE|WDT_ENABLE;
	}
	else{
		pwdtReg->wdtCtr = 0;
	}
}

/**
 * @brief set watchdog prescale function
 * @param value [in] wdt prescale register value
 */
void gpHalWdtSetPrescale(int value)
{
	wdtReg_t *pwdtReg = (wdtReg_t *)LOGI_ADDR_WDT_REG;

	pwdtReg->wdtPsr = value;
}

/**
 * @brief set watchdog load value function
 * @param value [in] wdt reload register value
 */
void gpHalWdtSetLoad(int value)
{
	wdtReg_t *pwdtReg = (wdtReg_t *)LOGI_ADDR_WDT_REG;

	pwdtReg->wdtLdr = value;
}

/**
 * @brief feed watchdog
 */
void gpHalWdtKeepAlive(void)
{
	wdtReg_t *pwdtReg = (wdtReg_t *)LOGI_ADDR_WDT_REG;

	pwdtReg->wdtVlr = GP_WDT_FEED_VAL;
}

/**
 * @brief reset system
 */
void gpHalWdtForceReset(void)
{
	wdtReg_t *pwdtReg = (wdtReg_t *)LOGI_ADDR_WDT_REG;

	pwdtReg->wdtVlr = GP_WDT_RESET_VAL;
}


/**
 * @brief set watchdog timeout function
 * @param period [in] wdt timeout in second
 */
void gpHalWdtSetTimeout(int period)
{
	unsigned long freq;
	unsigned long count;
	unsigned long prescale = 1;

	freq = gpHalWdtGetBaseClk();

	count = 0xfffful*0xfffful/freq;

	if((period < 1)||(period > count)){
		printk("watchdog set timeout error\n");
		return;
	}
	
	count = period*freq;

	prescale = (uint16_t)(count>>16);
	
	if(prescale){
		count = count/(prescale + 1);
	}

	gpHalWdtSetPrescale(prescale);

	gpHalWdtSetLoad(count);
}



