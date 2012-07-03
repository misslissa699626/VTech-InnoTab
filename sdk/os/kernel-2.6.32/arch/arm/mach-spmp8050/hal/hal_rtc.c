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
 * @file hal_rtc.c
 * @brief RTC HAL interface 
 * @author chao.chen
 */

#include <mach/kernel.h>
#include <mach/diag.h>
#include <mach/hal/regmap/reg_scu.h>
#include <mach/hal/regmap/reg_rtc.h>
#include <mach/hal/hal_rtc.h>
#include <mach/hal/hal_common.h>

/**************************************************************************
 *                              CONSTANTS                               *
 **************************************************************************/
#define HAL_RTC_MACRO_RW_TIMEOUT (100)		/*In ms*/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
 #if 0
#define HAL_BUSY_WAITING(cond, ms)	\
	{	\
		SINT32 timeout = ms;	\
		while (!(cond)){	\
			if (timeout < 0){	\
				DIAG_ERROR("[%s] HAL_BUSY_WAITING Time Out!!\n", __FUNCTION__);	\
				break;	\
			}	\
				\
			mdelay(10);	\
			timeout -= 10;	\
		}	\
	}	
#endif


static UINT32 
rtcMacroRead(
	UINT32 addr
)
{
	rtcReg_t *prtcReg = (rtcReg_t *)(LOGI_ADDR_RTC_REG);
	prtcReg->rtcAddr = addr;
	prtcReg->rtcRWReq = RTC_CTLREAD;

	/*check rtc ctller ready*/
	#if 0
	while (!(prtcReg->rtcRdy & RTC_MARCO_READY)){
		;
	}
	#else
	HAL_BUSY_WAITING((prtcReg->rtcRdy & RTC_MARCO_READY), HAL_RTC_MACRO_RW_TIMEOUT);
	#endif

	return prtcReg->rtcRData;
}

static void 
rtcMacroWrite(
	UINT32 addr, 
	UINT8 wdata
)
{
	rtcReg_t *prtcReg = (rtcReg_t *)(LOGI_ADDR_RTC_REG);
	prtcReg->rtcAddr = addr;
	prtcReg->rtcWData = wdata;
	prtcReg->rtcRWReq = RTC_CTLWRITE;
	
	/*check rtc ctller ready*/
	#if 0
	while (!(prtcReg->rtcRdy & RTC_MARCO_READY)){
		;
	}
	#else
	HAL_BUSY_WAITING((prtcReg->rtcRdy & RTC_MARCO_READY), HAL_RTC_MACRO_RW_TIMEOUT);
	#endif
}

/**
* @brief RTC Interrupt source read and clear.
* @return : Last Interrupt source.
*/
UINT32 
gpHalRtcIntSrcGet(
	void
)
{
	/* clear interrupt sources */
	UINT32 rtsr = rtcMacroRead(RTC_INTR_STATUS_MARCO);
	rtcMacroWrite(RTC_INTR_STATUS_MARCO, ~(SEC_INTR_STATUS | ALARM_INTR_STATUS)); 
	
	return rtsr;
}
EXPORT_SYMBOL(gpHalRtcIntSrcGet);

/**
* @brief Enable Control for RTC Interrupt(Sec / Alarm).
* @param secIntEn[in] : Sec Interrupt enable (0 disable/1 enable/invalid operation).
* @param secIntEn[in] : Alarm Interrupt enable (0 disable/1 enable/invalid operation).
*/
void 
gpHalRtcIntEnable(
	UINT8 secIntEn, 
	UINT8 alarmIntEn
)
{
	UINT8 val = (UINT8)rtcMacroRead(RTC_INTR_ENABLE_MARCO);
	if (secIntEn == 1){
		val |= RTC_SEC_INT_ENABLE;
	}
	else if (secIntEn == 0){
		val &= ~RTC_SEC_INT_ENABLE;
	}

	if (alarmIntEn == 1){
		val |= RTC_ALARM_INT_ENABLE;
	}
	else if (alarmIntEn == 0){
		val &= ~RTC_ALARM_INT_ENABLE;
	}

	rtcMacroWrite(RTC_INTR_ENABLE_MARCO, val);	
	
}
EXPORT_SYMBOL(gpHalRtcIntEnable);

/**
* @brief Get current RTC time value.
* @return : Time value. 
*/
UINT32 
gpHalRtcGetTime(
	void
)
{
	UINT32 valLow, valHigh;

	valLow = rtcMacroRead(RTC_TIMERCNT_7_0_MARCO);
	valLow |= (rtcMacroRead(RTC_TIMERCNT_15_8_MARCO) << 8);
	valLow |= (rtcMacroRead(RTC_TIMERCNT_23_16_MARCO) << 16);
	valLow |= (rtcMacroRead(RTC_TIMERCNT_31_24_MARCO) << 24);	
	valHigh =  rtcMacroRead(RTC_TIMERCNT_39_32_MARCO);
	valHigh |= (rtcMacroRead(RTC_TIMERCNT_47_40_MARCO) << 8);
	
	/*rtc clk =32k , sec = 32k count*/
	return (valHigh << 17) | (valLow >> 15);   
}
EXPORT_SYMBOL(gpHalRtcGetTime);

/**
* @brief Set RTC time value.
* @param time[in] : Time value to set.
* @return : Real time value after set.
*/
UINT32 
gpHalRtcSetTime(
	UINT32 time
)
{
	UINT8 val;
	UINT32 valLow, valHigh;

	#if 1	
	val = (UINT8) rtcMacroRead(RTC_CTL_MARCO);
	val = val |CTL_WRITE_LOAD;
	rtcMacroWrite(RTC_CTL_MARCO, val);	
	#endif
	
	valLow = (time & 0x1FFFF) << 15;
	rtcMacroWrite(RTC_LOADCNTBIT_7_0_MARCO, valLow & 0xFF);
	rtcMacroWrite(RTC_LOADCNTBIT_15_8_MARCO, (valLow >> 8) & 0xFF);
	rtcMacroWrite(RTC_LOADCNTBIT_23_16_MARCO, (valLow >> 16) & 0xFF);
	rtcMacroWrite(RTC_LOADCNTBIT_31_24_MARCO, (valLow >> 24) & 0xFF);
	
	valHigh = time >> 17;
	rtcMacroWrite(RTC_LOADCNTBIT_39_32_MARCO, valHigh & 0xFF);
	rtcMacroWrite(RTC_LOADCNTBIT_47_40_MARCO, (valHigh >> 8) & 0x3F);

	/*loading*/
	rtcMacroWrite(RTC_LOAD_START_VALUE_MARCO, 1);

	return gpHalRtcGetTime();
}
EXPORT_SYMBOL(gpHalRtcSetTime);

/**
* @brief Get RTC Alarm Interrupt settings.
* @param enable[out] : Alarm Interrupt enable (0 disable/1 enable).
* @param pending[out] : Alarm Interrupt pending (0 not pending/1 pending).
* @param time[out] : Alarm time value.
*/
void 
gpHalRtcGetAlarmStatus(
	UINT8 *enable, 
	UINT8 *pending, 
	UINT32 *time
)
{
	UINT32 valLow, valHigh;

	valLow  = (UINT8) rtcMacroRead(RTC_ALARM_7_0_MARCO);
	valLow  |= ((UINT8) rtcMacroRead(RTC_ALARM_15_8_MARCO)) << 8;
	valLow  |= ((UINT8) rtcMacroRead(RTC_ALARM_23_16_MARCO)) << 16;
	valLow  |= ((UINT8) rtcMacroRead(RTC_ALARM_31_24_MARCO)) << 24;

	valHigh = (UINT8) rtcMacroRead(RTC_ALARM_39_32_MARCO);
	valHigh |= ((UINT8) rtcMacroRead(RTC_ALARM_47_40_MARCO) & 0x7F) << 8;
	
	/*rtc clk =32k , sec = 32k count*/
	*time = (valHigh << 17) | (valLow >> 15);   
     	
	*enable = !!(rtcMacroRead(RTC_INTR_ENABLE_MARCO) & RTC_ALARM_INT_ENABLE);		
}
EXPORT_SYMBOL(gpHalRtcGetAlarmStatus);

/**
* @brief Set RTC Alarm Interrupt settings.
* @param enable[in] : Alarm Interrupt enable (0 disable/1 enable).
* @param pending[in] : Alarm Interrupt pending (0 not pending/1 pending).
* @param time[in] : Alarm time value.
*/
void 
gpHalRtcSetAlarmStatus(
	UINT8 enable, 
	UINT8 pending, 
	UINT32 time
)
{
	UINT8 val;
	UINT32 valLow, valHigh;

	valLow = (time & 0x1FFFF) << 15;
	rtcMacroWrite(RTC_ALARM_7_0_MARCO, valLow & 0xFF);
	rtcMacroWrite(RTC_ALARM_15_8_MARCO, (valLow >> 8) & 0xFF);
	rtcMacroWrite(RTC_ALARM_23_16_MARCO, (valLow >> 16) & 0xFF);
	rtcMacroWrite(RTC_ALARM_31_24_MARCO, (valLow >> 24) & 0xFF); 	
	
	valHigh = time >> 17;
	rtcMacroWrite(RTC_ALARM_39_32_MARCO, valHigh & 0xFF);
	rtcMacroWrite(RTC_ALARM_47_40_MARCO, (valHigh >> 8) & 0x3F);		
	val = rtcMacroRead(RTC_INTR_ENABLE_MARCO);
	
	if (enable){
		val |= RTC_ALARM_INT_ENABLE;
	}
	else{
		val &= ~RTC_ALARM_INT_ENABLE;
	}
	
	rtcMacroWrite(RTC_INTR_ENABLE_MARCO, val);
}
EXPORT_SYMBOL(gpHalRtcSetAlarmStatus);

/**
* @brief RTC enable/disable Control, if enable, check reliable code first; if fail, reset RTC registers.
* @param enable[in] :(0 disable/1 enable).
*/
void 
gpHalRtcEnable(
	UINT8 enable
)
{
	UINT8 val;
	rtcReg_t *prtcReg = (rtcReg_t *)(LOGI_ADDR_RTC_REG);

	if (!enable){
		val = (UINT8) rtcMacroRead(RTC_CTL_MARCO);
		rtcMacroWrite(RTC_CTL_MARCO, (~CTL_RTC_CLKEN) & val);
	} 
	else{
		#if 0
		writel(RTC_MACRO_CLK_ENABLE, base + RTC_SIEN_OFST);
		#else
		prtcReg->rtcSien = RTC_MACRO_CLK_ENABLE;
		#endif
		
		/*check rtc ctller ready*/
		#if 0
		while (!(readl(base + RTC_RDY_OFST) & RTC_MARCO_READY)){
			;
		}
		#else
		while (!(prtcReg->rtcRdy & RTC_MARCO_READY)){
			;
		}
		#endif

		/*init rtc , trig hardware pulse*/
		rtcMacroWrite(RTC_CTL_MARCO, CTL_RTC_CLKEN | CTL_COUNT_UP);

		/*enable clk*/
		val = (UINT8) rtcMacroRead(RTC_RELIABLECODE_MARCO);
		if (RELIABLE_CODE_CHECK_NUMBER != val){
			rtcMacroWrite(RTC_CTL_MARCO, CTL_RTC_CLKEN | CTL_RTCRST | CTL_COUNT_UP);
			rtcMacroWrite(RTC_CTL_MARCO, CTL_RTC_CLKEN | CTL_COUNT_UP);
			rtcMacroWrite(RTC_FDEN_MARCO, FD_ENABLE); 	/*enable FD*/
			rtcMacroWrite(RTC_INTR_ENABLE_MARCO, ~RTC_INT_ENABLE_MASK); 		/*disable rtc int*/
			rtcMacroWrite(RTC_INTR_STATUS_MARCO, ~RTC_INT_ENABLE_MASK);		/*clear intr status*/	
			rtcMacroWrite(RTC_RELIABLECODE_MARCO, RELIABLE_CODE_CHECK_NUMBER);		/*clear intr status*/	     	
		}
		/*rtcMacroWrite(rtc, RTC_INTR_ENABLE_MARCO, RTC_SEC_INT_ENABLE | RTC_ALARM_INT_ENABLE | RTC_WAKEUP_INT_ENABLE); 	disable rtc int*/
	}
}
EXPORT_SYMBOL(gpHalRtcEnable);

/**
* @brief RTC enable/disable system clk source.
* @param enable[in] :(0 disable/1 enable).
*/
void
gpHalRtcClkEnable(
	SP_BOOL enable
)
{
	scubReg_t *pScubReg = (scubReg_t *)LOGI_ADDR_SCU_B_REG;

	if (enable){
		pScubReg->scubPeriClkEn |= SCU_B_PERI_RTC;
	}
	else{
		pScubReg->scubPeriClkEn &= ~SCU_B_PERI_RTC;
	}
}
EXPORT_SYMBOL(gpHalRtcClkEnable);

