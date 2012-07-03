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
 * @file    hal_ceva.c
 * @brief   Implement of Ceva HAL API.
 * @author  qinjian
 * @since   2010/10/11
 * @date    2010/10/11
 */
#include <mach/kernel.h>
#include <mach/hal/hal_ceva.h>
#include <mach/hal/regmap/reg_ceva.h>
#include <mach/hal/regmap/reg_scu.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define CXRD    0x43585244
#define CXBT    0x43584254

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

static cevaReg_t *cevaReg = (cevaReg_t *)LOGI_ADDR_CEVA_REG;
static scucReg_t *scucReg = (scucReg_t *)LOGI_ADDR_SCU_C_REG;
static scudReg_t *scudReg = (scudReg_t *)LOGI_ADDR_SCU_D_REG;

/**
 * @brief   Ceva hardware irq enable/disable
 * @param   enable [in] disable(0)/enable(1)
 * @param   intMask [in] interrupt mask
 * @return  None
 * @see
 */
void
gpHalCevaEnableIrq(
	UINT32 enable,
	UINT32 intMask
)
{
	if (enable) {
		cevaReg->piuIntMask |= intMask;
	}
	else {
		cevaReg->piuIntMask &= ~intMask;
	}
}

/**
 * @brief   Ceva hardware status setting function
 * @param   status [in] status value
 * @return  None
 * @see
 */
void
gpHalCevaSetStatus(
	UINT32 status
)
{
	cevaReg->piuStatus = status;
}

/**
 * @brief   Ceva hardware status getting function
 * @return  status value
 * @see
 */
UINT32
gpHalCevaGetStatus(
	void
)
{
	return cevaReg->piuStatus;
}

/**
 * @brief   Ceva hardware status flags clear function
 * @param   status [in] status flags
 * @return  None
 * @see
 */
void
gpHalCevaClearStatus(
	UINT32 status
)
{
	#if 0
	cevaReg->piuStatus &= ~status;
	#else
	cevaReg->piuStatus = status; /* ??? !!! */
	#endif
}

/**
 * @brief   Ceva hardware command trigger
 * @param   idx [in] index of command register
 * @param   cmd [in] start address of command data
 * @return  None
 * @see
 */
void
gpHalCevaSetCmd(
	UINT32 idx,
	UINT32 cmd
)
{
	switch (idx) {
	case 0:
		cevaReg->piuCom0 = cmd;
		break;
	case 1:
		cevaReg->piuCom1 = cmd;
		break;
	case 2:
		cevaReg->piuCom2 = cmd;
		break;
	}
}

/**
 * @brief   Ceva hardware command reply getting function
 * @param   idx [in] index of reply register
 * @return  start address of reply data
 * @see
 */
UINT32
gpHalCevaGetReply(
	UINT32 idx
)
{
	UINT32 ret;

	switch (idx) {
	case 0:
		ret = cevaReg->piuRep0;
		break;
	case 1:
		ret = cevaReg->piuRep1;
		break;
	case 2:
		ret = cevaReg->piuRep2;
		break;
	default:
		ret = 0;
		break;
	}

	return ret;
}

/**
 * @brief   Ceva hardware reset
 * @param   entryAddr [in] boot code entry address
 * @return  None
 * @see
 */
void
gpHalCevaReset(
	UINT32 entryAddr
)
{
	/* clear ceva READY/BOOTED status */
	cevaReg->piuSem0C = 0xFFFFFFFF;
	cevaReg->piuRep2 = 0;

	/* before release ceva, clear ceva L2 mapping settings */
	scudReg->scudSb0Rgn = 0;
	scudReg->scudSb1Rgn = 0;

	/* Following step is reset CEVA
     * 1. First lock ceva via write 0x0 to register cevaCtrl
     * 2. Set 'EntryAddr' to register cevaVect
     * 3. Update register cevaCtrl's value using 0x1
     * 4. Unlock ceva via write 0x101 to register cevaCtrl
     */
	cevaReg->cevaCtrl = 0;
	cevaReg->cevaVect = entryAddr;
	udelay(1);
	cevaReg->cevaCtrl |= 0x1;
	udelay(1);
	cevaReg->cevaCtrl = 0x101;
}

/**
 * @brief   Ceva hardware lock
 * @return  None
 * @see
 */
void
gpHalCevaLock(
	void
)
{
	//cevaReg->cevaCtrl &= ~0x100;
	cevaReg->cevaCtrl = 0;
}

/**
 * @brief   Ceva hardware waiting status flags
 * @param   status [in] status flags to wait
 * @param   ms [in] waiting timeout in ms
 * @return  flags have been set(>=0) / waiting time out(<0)
 * @see
 */
SINT32
gpHalCevaWaitStatus(
	UINT32 status,
	unsigned int ms
)
{
	SINT32 ret;

	ret = HAL_BUSY_WAITING((cevaReg->piuStatus & status) != 0, ms);
	if (ret >= 0) {
		gpHalCevaClearStatus(status);
	}

	return ret;
}

/**
 * @brief   Ceva hardware waiting for READY
 * @param   ms [in] waiting timeout in ms
 * @return  READY(>=0) / waiting time out(<0)
 * @see
 */
SINT32
gpHalCevaWaitReady(
	unsigned int ms
)
{
	return HAL_BUSY_WAITING(cevaReg->piuSem0C == CXRD, ms);
}

/**
 * @brief   Ceva hardware waiting for BOOTED
 * @param   ms [in] waiting timeout in ms
 * @return  BOOTED(>=0) / waiting time out(<0)
 * @see
 */
SINT32
gpHalCevaWaitBooted(
	unsigned int ms
)
{
	return HAL_BUSY_WAITING(cevaReg->piuRep2 == CXBT, ms);
}

/**
 * @brief   Ceva clock enable/disable function
 * @param   enable[in]: disable(0)/enable(1)
 * @return  None
 * @see
 */
void
gpHalCevaClkEnable(
	UINT32 enable
)
{
	if (enable) {
		scucReg->scucPeriClkEn |= (SCU_C_PERI_CXMP_SL | SCU_C_PERI_CXMD_SL | SCU_C_PERI_DMAC1);
		scucReg->scucCevaCntEn |= 0x07; /* SCUC_CX_APB_EN | SCUC_CX_AHB_EN | SCUC_CX1620_EN */
	}
	else {
		scucReg->scucPeriClkEn &= ~(SCU_C_PERI_CXMP_SL | SCU_C_PERI_CXMD_SL | SCU_C_PERI_DMAC1);
		scucReg->scucCevaCntEn &= ~0x07; /* SCUC_CX_APB_EN | SCUC_CX_AHB_EN | SCUC_CX1620_EN */
	}
}

/**
 * @brief   Ceva clock setting function
 * @param   cevaRatio [in] CEVA clock ratio
 * @param   cevaAhbRatio [in] CEVA AHB clock ratio
 * @param   cevaApbRatio [in] CEVA APB clock ratio
 * @return  None
 */
void
gpHalCevaSetClk(
	UINT32 cevaRatio,
	UINT32 cevaAhbRatio,
	UINT32 cevaApbRatio
)
{
	scucReg->scucCevaCntEn      = 0x00; /* disable ceva clock */
	scucReg->scucCevaRatio      = cevaRatio & 0x3F;
	scucReg->scucCevaAhbRatio   = cevaAhbRatio & 0x3F;
	scucReg->scucCevaApbRatio   = cevaApbRatio & 0x3F;
#if 0
	scucReg->scucCevaCntEn      = 0x07;
	udelay(1000);
#endif
	scucReg->scucSysRatioUpdate = 0x7F;
}
