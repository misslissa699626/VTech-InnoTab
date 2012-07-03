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

#define HAL_RTC_SUCCESS 0
#define HAL_RTC_FAIL 1

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define HAL_RTC_PWR_PROTECTION \
	({ \
		prtcReg->rtcCtrl = 0; \
		prtcReg->rtcCtrl = RTC_MACRO_CLK_ENABLE; \
	})



static UINT32 
rtcMacroRead(
	UINT32 addr,
	UINT32 *pdata
)
{
	rtcReg_t *prtcReg = (rtcReg_t *)(LOGI_ADDR_RTC_REG);
	
	HAL_RTC_PWR_PROTECTION;
	
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

	if (prtcReg->rtcRdy & RTC_MARCO_READY) {
		*pdata = prtcReg->rtcRData;
		return HAL_RTC_SUCCESS;
	}
	else {
		//prtcReg->rtcRWReq = ~(RTC_CTLWRITE | RTC_CTLREAD);
		return HAL_RTC_FAIL;
	}
}

static UINT32
rtcMacroWrite(
	UINT32 addr,
	UINT8 wdata
)
{
	rtcReg_t *prtcReg = (rtcReg_t *)(LOGI_ADDR_RTC_REG);

	HAL_RTC_PWR_PROTECTION;
	
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

	if (prtcReg->rtcRdy & RTC_MARCO_READY) {
		return HAL_RTC_SUCCESS;
	}
	else {
		//prtcReg->rtcRWReq = ~(RTC_CTLWRITE | RTC_CTLREAD);
		return HAL_RTC_FAIL;
	}
}


static inline UINT32 rtcMacroWriteRequest(
	UINT32 addr
)
{
	UINT32 data;
	UINT8 val;
	UINT32 ret = rtcMacroRead(RTC_CTL_MARCO, &data);
	if (ret != HAL_RTC_SUCCESS) {
		DIAG_ERROR("rtc hal error in [%s:%d]\n", __FUNCTION__, __LINE__);
		return ret;
	}

	val = (UINT8)data;

	if (val & CTL_BUSY) {	
		DIAG_ERROR("Clear RTC busy\n");

		ret = rtcMacroWrite(RTC_CTL_MARCO, val);
		if (ret != HAL_RTC_SUCCESS) {
			DIAG_ERROR("rtc hal error in [%s:%d]\n", __FUNCTION__, __LINE__);
			return ret;
		}
	}

	return HAL_RTC_SUCCESS;
}


static inline UINT32 rtcMacroWriteCheck(
	UINT32 addr
)
{
	/*if write in these address, need wait 32k domain..*/
	if ( addr == 0xc0 || addr == 0xd0 || (addr >= 0x10 && addr <= 0x15) ) {	

		UINT32 timeout = 100;		/*not stable,  any better solution?*/
		UINT32 data;
		UINT8 val;
		UINT32 ret;

		do {
			ret = rtcMacroRead(RTC_CTL_MARCO, &data);
			if (ret != HAL_RTC_SUCCESS) {
				DIAG_ERROR("rtc hal error in [%s:%d]\n", __FUNCTION__, __LINE__);
				return HAL_RTC_FAIL;	
			}

			val = (UINT8)data;
			timeout--;
		} while ( (val & CTL_BUSY) && (timeout > 0) );
		
		if ( (timeout <= 0) && (val & CTL_BUSY) ) {
			DIAG_ERROR("wait 32k busy timeout in [%s:%d]\n", __FUNCTION__, __LINE__);
			if (HAL_RTC_SUCCESS != rtcMacroWrite(RTC_CTL_MARCO, val)) {
				DIAG_ERROR("rtc hal error in [%s:%d]\n", __FUNCTION__, __LINE__);
				return HAL_RTC_FAIL;					
			}
		}
	}
	return HAL_RTC_SUCCESS;
}


static UINT32 rtcMacroWriteIn32KDomain(
	UINT32 addr,
	UINT8 wdata
)
{
	UINT32 ret;
	
	ret = rtcMacroWriteRequest(addr);
	if (HAL_RTC_SUCCESS == ret) {
		ret = rtcMacroWrite(addr, wdata); 
		if (ret == HAL_RTC_SUCCESS) {
			ret = rtcMacroWriteCheck(addr);
		}
	}

	return ret;
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
	UINT32 ret; 
	UINT32 rtsr;

	ret = rtcMacroRead(RTC_INTR_STATUS_MARCO, &rtsr);
	if (ret != HAL_RTC_SUCCESS) {
		DIAG_ERROR("rtc hal error in [%s:%d]\n", __FUNCTION__, __LINE__);
		return 0;
	}

	ret = rtcMacroWriteIn32KDomain(RTC_INTR_STATUS_MARCO, ~(SEC_INTR_STATUS | ALARM_INTR_STATUS)); 

	if (ret != HAL_RTC_SUCCESS){
		DIAG_ERROR("rtc hal error when clear int src [%s:%d]\n", __FUNCTION__, __LINE__);
	}
	
	return rtsr;
}
EXPORT_SYMBOL(gpHalRtcIntSrcGet);

/**
* @brief Enable Control for RTC Interrupt(Sec / Alarm).
* @param IntEn[in] : interrupt enable state (0 disable/1 enable/invalid operation).
* @param IntEnMask[in] : interrupt enable mask bits (0 no change/1 ).
*/
void 
gpHalRtcIntEnable(
	UINT8 IntEn, 
	UINT8 IntMask
)
{
	UINT32 ret;
	UINT32 data;
	UINT8 val;

	ret = rtcMacroRead(RTC_INTR_ENABLE_MARCO, &data);
	if (ret != HAL_RTC_SUCCESS) {
		DIAG_ERROR("rtc hal error in [%s:%d]\n", __FUNCTION__, __LINE__);
		return;		
	}

	val = (UINT8)(data & ~(IntMask & RTC_INT_ENABLE_MASK));

	if (IntEn & RTC_SEC_INT_ENABLE){
		val |= RTC_SEC_INT_ENABLE;
	}

	if (IntEn & RTC_ALARM_INT_ENABLE){
		val |= RTC_ALARM_INT_ENABLE;
	}
	
	if (IntEn & RTC_WAKEUP_INT_ENABLE){
		val |= RTC_WAKEUP_INT_ENABLE;
	}

	ret = rtcMacroWriteIn32KDomain(RTC_INTR_ENABLE_MARCO, val); 

	if (ret != HAL_RTC_SUCCESS){
		DIAG_ERROR("rtc hal error in [%s:%d]\n", __FUNCTION__, __LINE__);
	}	
}
EXPORT_SYMBOL(gpHalRtcIntEnable);

/**
* @brief Get current RTC time value.
* @param pvalue[out] : Time value
* @return : HAL_RTC_SUCCESS / HAL_RTC_FAIL. 
*/
UINT32 
gpHalRtcGetTime(
	UINT32 *pvalue
)
{
	UINT32 valLow, valHigh;
	UINT32 ret;

	ret =  rtcMacroRead(RTC_TIMERCNT_7_0_MARCO, &valLow);
	if (ret == HAL_RTC_SUCCESS) {
		UINT32 data;

		ret =  rtcMacroRead(RTC_TIMERCNT_15_8_MARCO, &data);	
		if (ret == HAL_RTC_SUCCESS) {
			valLow |= data << 8;
			ret =  rtcMacroRead(RTC_TIMERCNT_23_16_MARCO, &data);	
			if (ret == HAL_RTC_SUCCESS) {
				valLow |= data << 16;
				ret =  rtcMacroRead(RTC_TIMERCNT_31_24_MARCO, &data);
				if (ret == HAL_RTC_SUCCESS) {
					valLow |= data << 24;
					ret =  rtcMacroRead(RTC_TIMERCNT_39_32_MARCO, &data);
					if (ret == HAL_RTC_SUCCESS) {
						valHigh = data;
						ret =  rtcMacroRead(RTC_TIMERCNT_47_40_MARCO, &data);
						if (ret == HAL_RTC_SUCCESS) {
							valHigh |= data << 8;
						}
					}
				}
			}
		}
		
	}

	if (ret != HAL_RTC_SUCCESS) {
		DIAG_ERROR("rtc hal error in [%s:%d]\n", __FUNCTION__, __LINE__);
		return ret;
	}

	/*rtc clk =32k , sec = 32k count*/
	*pvalue = (valHigh << 17) | (valLow >> 15);
	return ret;
}
EXPORT_SYMBOL(gpHalRtcGetTime);

/**
* @brief Set RTC time value.
* @param time[in] : Time value to set.
* @return : HAL_RTC_SUCCESS / HAL_RTC_FAIL. 
*/
UINT32 
gpHalRtcSetTime(
	UINT32 time
)
{
	UINT32 valLow, valHigh;
	UINT32 ret;

	#if 0	
	val = (UINT8) rtcMacroRead(RTC_CTL_MARCO);
	val = val |CTL_WRITE_LOAD;
	rtcMacroWrite(RTC_CTL_MARCO, val);	
	#endif
	
	valLow = (time & 0x1FFFF) << 15;
	valHigh = time >> 17;

	ret = rtcMacroWriteIn32KDomain(RTC_LOADCNTBIT_7_0_MARCO, valLow & 0xFF);
	if (ret == HAL_RTC_SUCCESS) {
		ret = rtcMacroWriteIn32KDomain(RTC_LOADCNTBIT_15_8_MARCO, (valLow >> 8) & 0xFF);
		if (ret == HAL_RTC_SUCCESS) {
			ret = rtcMacroWriteIn32KDomain(RTC_LOADCNTBIT_23_16_MARCO, (valLow >> 16) & 0xFF);
			if (ret == HAL_RTC_SUCCESS) {
				ret = rtcMacroWriteIn32KDomain(RTC_LOADCNTBIT_31_24_MARCO, (valLow >> 24) & 0xFF);
				if (ret == HAL_RTC_SUCCESS) {
					ret = rtcMacroWriteIn32KDomain(RTC_LOADCNTBIT_39_32_MARCO, valHigh & 0xFF);	
					if (ret == HAL_RTC_SUCCESS) {
						ret = rtcMacroWriteIn32KDomain(RTC_LOADCNTBIT_47_40_MARCO, (valHigh >> 8) & 0x3F);	
					}
				}
			}		
		}		
	}

	if (ret != HAL_RTC_SUCCESS) {
		DIAG_ERROR("rtc hal error in [%s:%d]\n", __FUNCTION__, __LINE__);
		return ret;
	}

	/*loading*/
	/*rtcMacroWrite(RTC_LOAD_START_VALUE_MARCO, 1);*/	

	return ret;
}
EXPORT_SYMBOL(gpHalRtcSetTime);

/**
* @brief Get RTC Alarm Interrupt settings.
* @param enable[out] : Alarm Interrupt enable (0 disable/1 enable).
* @param pending[out] : Alarm Interrupt pending (0 not pending/1 pending).
* @param time[out] : Alarm time value.
* @return : HAL_RTC_SUCCESS / HAL_RTC_FAIL. 
*/
UINT32 
gpHalRtcGetAlarmStatus(
	UINT8 *enable, 
	UINT8 *pending, 
	UINT32 *time
)
{
	UINT32 valLow, valHigh;
	UINT32 ret;
	UINT32 data;
	
	ret =  rtcMacroRead(RTC_ALARM_7_0_MARCO, &valLow);
	if (ret == HAL_RTC_SUCCESS) {
		ret =  rtcMacroRead(RTC_ALARM_15_8_MARCO, &data);	
		if (ret == HAL_RTC_SUCCESS) {
			valLow |= data << 8;
			ret =  rtcMacroRead(RTC_ALARM_23_16_MARCO, &data);	
			if (ret == HAL_RTC_SUCCESS) {
				valLow |= data << 16;
				ret =  rtcMacroRead(RTC_ALARM_31_24_MARCO, &data);
				if (ret == HAL_RTC_SUCCESS) {
					valLow |= data << 24;
					ret =  rtcMacroRead(RTC_ALARM_39_32_MARCO, &data);
					if (ret == HAL_RTC_SUCCESS) {
						valHigh = data;
						ret =  rtcMacroRead(RTC_ALARM_47_40_MARCO, &data);
						if (ret == HAL_RTC_SUCCESS) {
							valHigh |= (data & 0x7F) << 8;
						}
					}
				}
			}
		}
		
	}

	if (ret != HAL_RTC_SUCCESS) {
		DIAG_ERROR("rtc hal error in [%s:%d]\n", __FUNCTION__, __LINE__);
		return ret;
	}	
	
	/*rtc clk =32k , sec = 32k count*/
	*time = (valHigh << 17) | (valLow >> 15);   

	ret = rtcMacroRead(RTC_INTR_ENABLE_MARCO, &data);
	if (ret != HAL_RTC_SUCCESS) {
		DIAG_ERROR("rtc hal error in [%s:%d]\n", __FUNCTION__, __LINE__);
		return ret;
	}	
	
	*enable = !!(data & RTC_ALARM_INT_ENABLE);

	return ret;
}
EXPORT_SYMBOL(gpHalRtcGetAlarmStatus);

/**
* @brief Set RTC Alarm Interrupt settings.
* @param enable[in] : Alarm Interrupt enable (0 disable/1 enable).
* @param pending[in] : Alarm Interrupt pending (0 not pending/1 pending).
* @param time[in] : Alarm time value.
* @return : HAL_RTC_SUCCESS / HAL_RTC_FAIL. 
*/
UINT32 
gpHalRtcSetAlarmStatus(
	UINT8 enable, 
	UINT8 pending, 
	UINT32 time
)
{
	UINT8 val;
	UINT32 valLow, valHigh;
	UINT32 data;
	UINT32 ret;

	valLow = (time & 0x1FFFF) << 15;
	valHigh = time >> 17;
	
	ret = rtcMacroWriteIn32KDomain(RTC_ALARM_7_0_MARCO, valLow & 0xFF);
	if (ret == HAL_RTC_SUCCESS) {
		ret = rtcMacroWriteIn32KDomain(RTC_ALARM_15_8_MARCO, (valLow >> 8) & 0xFF);
		if (ret == HAL_RTC_SUCCESS) {
			ret = rtcMacroWriteIn32KDomain(RTC_ALARM_23_16_MARCO, (valLow >> 16) & 0xFF);
			if (ret == HAL_RTC_SUCCESS) {
				ret = rtcMacroWriteIn32KDomain(RTC_ALARM_31_24_MARCO, (valLow >> 24) & 0xFF);
				if (ret == HAL_RTC_SUCCESS) {
					ret = rtcMacroWriteIn32KDomain(RTC_ALARM_39_32_MARCO, valHigh & 0xFF);	
					if (ret == HAL_RTC_SUCCESS) {
						ret = rtcMacroWriteIn32KDomain(RTC_ALARM_47_40_MARCO, (valHigh >> 8) & 0x3F);	
					}
				}
			}		
		}		
	}

	if (ret != HAL_RTC_SUCCESS) {
		DIAG_ERROR("rtc hal error in [%s:%d]\n", __FUNCTION__, __LINE__);
		return ret;
	}
	
	ret = rtcMacroRead(RTC_INTR_ENABLE_MARCO, &data);
	if (ret != HAL_RTC_SUCCESS) {
		DIAG_ERROR("rtc hal error in [%s:%d]\n", __FUNCTION__, __LINE__);
		return ret;
	}		

	val = (UINT8)data;
	if (enable){
		val |= RTC_ALARM_INT_ENABLE;
	}
	else{
		val &= ~RTC_ALARM_INT_ENABLE;
	}
	
	ret = rtcMacroWriteIn32KDomain(RTC_INTR_ENABLE_MARCO, val);
	if (ret != HAL_RTC_SUCCESS) {
		DIAG_ERROR("rtc hal error in [%s:%d]\n", __FUNCTION__, __LINE__);
		return ret;
	}		

	return ret;
}
EXPORT_SYMBOL(gpHalRtcSetAlarmStatus);

/**
* @brief RTC enable/disable Control, if enable, check reliable code first; if fail, reset RTC registers.
* @param enable[in] :(0 disable/1 enable).
* @return : HAL_RTC_SUCCESS / HAL_RTC_FAIL. 
*/
UINT32 
gpHalRtcEnable(
	UINT8 enable
)
{
	UINT8 val;
	UINT32 ret;
	UINT32 data;
	rtcReg_t *prtcReg = (rtcReg_t *)(LOGI_ADDR_RTC_REG);

	if (!enable){
		ret = rtcMacroRead(RTC_CTL_MARCO, &data);
		if (ret != HAL_RTC_SUCCESS) {
			DIAG_ERROR("rtc hal error in [%s:%d]\n", __FUNCTION__, __LINE__);
			return ret;
		}
		
		val = (UINT8)data;
		ret = rtcMacroWriteIn32KDomain(RTC_CTL_MARCO, (~CTL_RTC_CLKEN) & val);
		if (ret != HAL_RTC_SUCCESS) {
			DIAG_ERROR("rtc hal error in [%s:%d]\n", __FUNCTION__, __LINE__);
			return ret;
		}			
	} 
	else{
		#if 0
		writel(RTC_MACRO_CLK_ENABLE, base + RTC_SIEN_OFST);
		#else
		prtcReg->rtcCtrl = RTC_MACRO_CLK_ENABLE;
		#endif
		
		/*check rtc ctller ready*/
		#if 0
		while (!(readl(base + RTC_RDY_OFST) & RTC_MARCO_READY)){
			;
		}
		#else
		HAL_BUSY_WAITING((prtcReg->rtcRdy & RTC_MARCO_READY), HAL_RTC_MACRO_RW_TIMEOUT);
		if (!(prtcReg->rtcRdy & RTC_MARCO_READY)) {
			DIAG_ERROR("rtc hal error in [%s:%d]\n", __FUNCTION__, __LINE__);
			return HAL_RTC_FAIL;
		}	
		#endif

		/*init rtc , trig hardware pulse*/
		ret = rtcMacroWriteIn32KDomain(RTC_CTL_MARCO,  CTL_RTC_CLKEN | CTL_COUNT_UP | CTL_BUSY);
		if (ret != HAL_RTC_SUCCESS) {
			DIAG_ERROR("rtc hal error in [%s:%d]\n", __FUNCTION__, __LINE__);
			return ret;
		}		

		/*enable clk*/
		ret = rtcMacroRead(RTC_RELIABLECODE_MARCO, &data);
		if (ret != HAL_RTC_SUCCESS) {
			DIAG_ERROR("rtc hal error in [%s:%d]\n", __FUNCTION__, __LINE__);
			return ret;
		}
		
		val = (UINT8)data;
		DIAG_ERROR("rtc reliable code: 0x%02x\n", val);
		if (RELIABLE_CODE_CHECK_NUMBER != val){
			DIAG_ERROR("reliable code not match, reset rtc [%s:%d]\n", __FUNCTION__, __LINE__);
			ret = rtcMacroWriteIn32KDomain(RTC_CTL_MARCO, CTL_RTC_CLKEN | CTL_RTCRST | CTL_COUNT_UP);
			if (ret == HAL_RTC_SUCCESS) {
				ret = rtcMacroWriteIn32KDomain(RTC_CTL_MARCO, CTL_RTC_CLKEN | CTL_COUNT_UP);
				if (ret == HAL_RTC_SUCCESS) {
					ret = rtcMacroWriteIn32KDomain(RTC_FDEN_MARCO, FD_ENABLE);		/*enable FD*/
					if (ret == HAL_RTC_SUCCESS) {
						ret = rtcMacroWriteIn32KDomain(RTC_INTR_ENABLE_MARCO, ~RTC_INT_ENABLE_MASK);	/*disable rtc int*/
						if (ret == HAL_RTC_SUCCESS) {
							ret = rtcMacroWriteIn32KDomain(RTC_INTR_STATUS_MARCO,  ~RTC_INT_ENABLE_MASK);		/*clear intr status*/
							if (ret == HAL_RTC_SUCCESS) {
								ret = rtcMacroWriteIn32KDomain(RTC_RELIABLECODE_MARCO, RELIABLE_CODE_CHECK_NUMBER);	/*reset reliable code value*/
							}
						}
					}		
				}		
			}	     	
		}
		/*rtcMacroWrite(rtc, RTC_INTR_ENABLE_MARCO, RTC_SEC_INT_ENABLE | RTC_ALARM_INT_ENABLE | RTC_WAKEUP_INT_ENABLE); 	disable rtc int*/

		if (ret != HAL_RTC_SUCCESS) {
			DIAG_ERROR("rtc hal error in [%s:%d]\n", __FUNCTION__, __LINE__);
			return ret;
		}		
	}

	return HAL_RTC_SUCCESS;
}
EXPORT_SYMBOL(gpHalRtcEnable);


