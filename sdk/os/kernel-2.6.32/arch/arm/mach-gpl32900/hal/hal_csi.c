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
#include <mach/hal/regmap/reg_scu.h>
#include <mach/hal/hal_clock.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/


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
static csiReg_t *pCsiReg = (csiReg_t *)(LOGI_ADDR_CSI_REG);
static scuaReg_t *pscuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;
static UINT32 CsiRecUsbPhyCfg = 0;

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void
gpHalCsiSetMclk(
	UINT32 clk_sel, 
	UINT32 clko_div,
	UINT32 pclk_dly,
	UINT32 pclk_revb
)
{
	/*Setting CSI_CLK_O Clock,[8]:Enable CSI_CLK_O,[7:0]: Clock_Ratio */
	UINT32 reg = 0;

	if(clko_div)
	{
		reg = (clko_div & 0xFF) | (1 << 8);
		reg |= (clk_sel & 1) << 16;
		reg |= (pclk_dly & 0xF) << 24;
		reg |= (pclk_revb & 1) << 28;
		if(clk_sel) //0: SPLL, 1: USB
		{
			CsiRecUsbPhyCfg = pscuaReg->scuaUsbPhyCfg;
			pscuaReg->scuaUsbPhyCfg |= (1<<2);	//USBPHY Xtal enable;
			pscuaReg->scuaUsbPhyCfg |= (2<<12); //USBP1 enable; 
		}
	}
	else
	{
		if(CsiRecUsbPhyCfg)
		{
			pscuaReg->scuaUsbPhyCfg = CsiRecUsbPhyCfg;
			CsiRecUsbPhyCfg = 0;
		}
	}
	
	pscuaReg->scuaCsiClkCfg = 0;
	pscuaReg->scuaCsiClkCfg = reg;
}

void
gpHalCsiGetMclk(
	UINT8 *clk_sel, 
	UINT8 *clko_div,
	UINT8 *pclk_dly,
	UINT8 *pclk_revb
)
{
	UINT32 reg = pscuaReg->scuaCsiClkCfg;
	
	if(reg & (1 << 8))
		*clko_div = reg & 0xFF;
	else
		*clko_div = 0;
	
	*clk_sel = (reg & (1<<16)) >> 16;
	*pclk_dly = (reg & (0xF << 24)) >> 24;
	*pclk_revb = (reg & (1<<28)) >> 28;
}

UINT32
gpHalCsiClearisr(
//	UINT32 idx,
//	UINT32 addr
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
	//	pCsiReg->csicr0 |= 0x80000000 | idx<<24;
		pCsiReg->csicr0 |= 0x80000000 | 0<<24;
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
gpHalCsiSetFormat(
	UINT32 fmt
)
{
	pCsiReg->csicr0 &= ~CSI_EN;
	pCsiReg->csicr1 = 0xC;
	pCsiReg->csicr0 &= ~( CSI_YUVIN | CSI_INSEQ_YUYV | CSI_RGB1555 );
	pCsiReg->csicr0 |= fmt | CSI_FRAME_DIS | CSI_UPDATESET;
	pCsiReg->csicr0 |= CSI_EN;
}
EXPORT_SYMBOL(gpHalCsiSetFormat);

void
gpHalCsiSetBuf(
//	UINT32 bufnum,
	UINT32 addr
)
{
//	if( bufnum==0 )
		pCsiReg->csifbadr0 = addr;
//	else if(bufnum==1)
//		pCsiReg->csifbadr1 = (int)addr;
//	else
//		pCsiReg->csifbadr2 = (int)addr;
}
EXPORT_SYMBOL(gpHalCsiSetBuf);

void
gpHalCsiSetResolution(
	UINT32 hset,
	UINT32 vset,
	UINT32 stp,
	UINT32 hoffset,
	UINT32 voffset
)
{
	pCsiReg->csihset = hset | hoffset<<16;
	pCsiReg->csivset = vset | voffset<<16;
	pCsiReg->csilstp = stp;	
}
EXPORT_SYMBOL(gpHalCsiSetResolution);

void
gpHalCsiStart(
	void
)
{	
	pCsiReg->csicr0 &= ~CSI_FRAME_DIS;
	pCsiReg->csiirqen=0x4;
}
EXPORT_SYMBOL(gpHalCsiStart);

void
gpHalCsiStop(
	void
)
{
	pCsiReg->csicr0 |= CSI_FRAME_DIS;
	pCsiReg->csiirqen=0x0;
}
EXPORT_SYMBOL(gpHalCsiStop);

void
gpHalCsiSetCtrl(
	UINT32 ctrl
)
{
	pCsiReg->csicr0 &= ~CSI_EN;
	pCsiReg->csicr1 = 0xC;
	pCsiReg->csicr0 = ctrl | CSI_FRAME_DIS | CSI_UPDATESET;
	pCsiReg->csicr0 |= CSI_EN;
}
EXPORT_SYMBOL(gpHalCsiSetCtrl);

void
gpHalCsiGetCtrl(
	UINT32 *ctrl
)
{
	*ctrl = pCsiReg->csicr0;
}
EXPORT_SYMBOL(gpHalCsiGetCtrl);

void
gpHalCsiSuspend(
	void
)
{
	gpHalCsiStop();
	gpHalCsiClose();
}
EXPORT_SYMBOL(gpHalCsiSuspend);

void
gpHalCsiResume(
	void
)
{
	gpHalCsiInit();
}
EXPORT_SYMBOL(gpHalCsiResume);

void
gpHalCsiClose(
	void
)
{
#if 0		//only for debug
	pscuaReg->scuaCsiClkCfg &= ~CSI_CNT_EN;
	pscuaReg->scuaPeriDgClkEn &= ~CSI_CLKEN;
	
	gpHalScuClkEnable(SCU_A_PERI_CMOS_CTRL, SCU_A, 0);
#endif
//	pscuaReg->scuaPeriClkEn &= ~CSI_CLKEN;
}
EXPORT_SYMBOL(gpHalCsiClose);

void
gpHalCsiInit(
	void
)
{
	gpHalScuClkEnable(SCU_A_PERI_CMOS_CTRL, SCU_A, 1);
//	pscuaReg->scuaPeriClkEn |= CSI_CLKEN;
	pscuaReg->scuaPeriDgClkEn |= CSI_CLKEN;

	pscuaReg->scuaCsiClkCfg = 30;
	pscuaReg->scuaCsiClkCfg |= CSI_CNT_EN;

	pscuaReg->scuaSysSel |= CSI_SEL_8050;		//Only for GPL32900

	gpHalCsiReset();
}
EXPORT_SYMBOL(gpHalCsiInit);
