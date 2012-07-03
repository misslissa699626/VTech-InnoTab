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

#include <linux/module.h> 
#include <mach/hal/regmap/reg_vic.h>
#include <mach/hal/hal_vic.h>
#include <mach/irqs.h>

/**
* @brief Set priority of interrupt source
* @param[vicId] 0:VIC0/1:VIC1 
* @param[vicSrc] interrupt source number(0~31)
* @param[priority] VIC_PRIORITY0~VIC_PRIORITY15
* @return 0:success/-1:failed
*/
SINT32 gpHalVicSetPriority(
	UINT32 vicId,
	UINT32 vicSrc,
	UINT32 priority
)
{
	volatile UINT32 *pprio;
	
	if (vicId > 1 || vicSrc > 31) {
		return -1;
	}
	pprio = (volatile UINT32 *)(VIC_PRIORITY_BASE + vicSrc*4 + vicId*VIC_OFFSET);
	
	*pprio = priority; 

	return 0;
}
EXPORT_SYMBOL(gpHalVicSetPriority);

/**
* @brief Get priority of interrupt source
* @param[vicId] 0:VIC0/1:VIC1 
* @param[vicSrc] interrupt source number(0~31)
* @return -1:failed/VIC_PRIORITY0~VIC_PRIORITY15
*/
SINT32 gpHalVicGetPriority(
	UINT32 vicId,
	UINT32 vicSrc
)
{
	volatile UINT32 *pprio;
	
	if (vicId > 1 || vicSrc > 31) {
		return -1;
	}
	pprio = (volatile UINT32 *)(VIC_PRIORITY_BASE + vicSrc*4 + vicId*VIC_OFFSET);
	
	return *pprio; 
}
EXPORT_SYMBOL(gpHalVicGetPriority);

/**
* @brief Select type of interrupt
* @param[vicSrc] interrupt source (0~63)
* @param[type] 0:IRQ/1:FIQ 
* @return -1:failed/0:success
*/
SINT32 gpHalVicIntSel(
	UINT32 vicSrc,
	UINT32 type
)
{
	vicReg_t *pvicReg;
	
	if (vicSrc > MAX_IRQ_NUM) {
		return -1;
	}
	
	if (vicSrc > 31) {
		pvicReg = (vicReg_t *)(LOGI_ADDR_VIC_REG + VIC_OFFSET);
	}
	else {
		pvicReg = (vicReg_t *)(LOGI_ADDR_VIC_REG);
	}
	
	if (type) {
		pvicReg->vicIntSelect |= (1 << (vicSrc & 0x1F));
	}
	else {
		pvicReg->vicIntSelect &= ~(1 << (vicSrc & 0x1F));
	}
	
	return 0; 
}
EXPORT_SYMBOL(gpHalVicIntSel);

