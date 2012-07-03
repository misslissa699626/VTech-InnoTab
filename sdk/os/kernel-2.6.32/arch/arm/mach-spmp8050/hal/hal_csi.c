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
 * @file gp_csi.c
 * @brief CSI hardware interface
 * @author Simon Hsu
 */
#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/hal/hal_csi.h>
#include <mach/hal/hal_gpio.h>
#include <mach/hal/sysregs.h>
#include <mach/hal/regmap/reg_csi.h>
#include <mach/regs-scu.h>


/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/


/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define CSI_CLKEN	(1<<10)
#define CSI_CNT_EN	(1<<8)

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
static csiReg_t *pCsiReg = (csiReg_t *)(LOGI_ADDR_CSI_REG);
static scuaReg_t *pscuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/

UINT32
gpHalCsiClearisr(
	UINT32 idx,
	UINT32 addr
)
{
	UINT32 status;
	
	status = pCsiReg->csiirqsts;
	if( status & 0x2 )
	{
		pCsiReg->csiirqsts = 0x2;
		return 0x2;
	}
	if( status & 0x4 )
	{
		pCsiReg->csiirqsts = 0x4;
	
		pCsiReg->csicr0 &= 0xFCFFFFFF;
		pCsiReg->csicr0 |= 0x80000000 | idx<<24;
	}
	return 0x3;
}
EXPORT_SYMBOL(gpHalCsiClearisr);

void
gpHalCsiReset(
	void
)
{
	pCsiReg->csicr0 = 0x0;
	pCsiReg->csicr1 = 0x1;
	pCsiReg->csicr0 = 0x1;
	pCsiReg->csicr1 = 0x0;
	pCsiReg->csicr0 = 0x0;
}

void
gpHalCsiSetBuf(
	UINT32 bufnum,
	UINT16 *addr
)
{
	if( bufnum==0 )
		pCsiReg->csifbadr0 = (int)addr;
	else if(bufnum==1)
		pCsiReg->csifbadr1 = (int)addr;
	else
		pCsiReg->csifbadr2 = (int)addr;
}
EXPORT_SYMBOL(gpHalCsiSetBuf);

void
gpHalCsiStart(
	void
)
{
	gpHalCsiReset();
	
	pCsiReg->csicr1 = 0xC;

	pCsiReg->csicr0 =0x01|(0<<1)|(1<<2)|(0<<3)|(0<<5)|(1<<8)|(1<<9)|(0<<10)|(0<<12)|(1<<31);
	pCsiReg->csicr0&=(~(1<<27));

	pCsiReg->csiirqen=0x4;//0x02|0x04;
}
EXPORT_SYMBOL(gpHalCsiStart);

void
gpHalCsiStop(
	void
)
{
	pCsiReg->csiirqen=0x0;
}
EXPORT_SYMBOL(gpHalCsiStop);

void
gpHalCsiClose(
	void
)
{
	pscuaReg->scuaCsiClkCfg &= ~CSI_CNT_EN;
	pscuaReg->scuaPeriDgClkEn &= ~CSI_CLKEN;
	pscuaReg->scuaPeriClkEn &= ~CSI_CLKEN;
}
EXPORT_SYMBOL(gpHalCsiClose);

void
gpHalCsiInit(
	void
)
{
	UINT32 val;
	
	pscuaReg->scuaPeriClkEn |= CSI_CLKEN;
	pscuaReg->scuaPeriDgClkEn |= CSI_CLKEN;
	pscuaReg->scuaCsiClkCfg |= CSI_CNT_EN;
	
	val = pscuaReg -> scuaCsiClkCfg;
	val &= 0xFFFFFF00;
	val = 21;
	pscuaReg -> scuaCsiClkCfg = val;
	pscuaReg -> scuaCsiClkCfg |= 0x100;

	val = pCsiReg->csicr0;
	val |= 0x80000000;
	pCsiReg->csicr0 |= val;
	
	val = pscuaReg -> scuaCsiClkCfg;
	
	pCsiReg->csihset = 639;
	pCsiReg->csivset = 479;
	pCsiReg->csilstp = 1280;
}
EXPORT_SYMBOL(gpHalCsiInit);
