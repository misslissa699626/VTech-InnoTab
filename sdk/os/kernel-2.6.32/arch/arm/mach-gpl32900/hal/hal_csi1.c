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
#include <mach/hal/hal_csi1.h>
#include <mach/hal/hal_gpio.h>
#include <mach/hal/regmap/reg_ppu.h>
#include <mach/hal/sysregs.h>
#include <mach/hal/regmap/reg_csi1.h>
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
static csi1Reg_t *pCsi1Reg = (csi1Reg_t *)(LOGI_ADDR_CSI1_REG);
static scuaReg_t *pscuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;
static ppuFunIrq_t *pPPUIrqreg = (ppuFunIrq_t *)(PPU_BASE_REG);
static UINT32 Csi1RecUsbPhyCfg = 0;

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void
gpHalCsi1SetMclk(
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
		reg |= (clk_sel & 1)<< 16;
		reg |= (pclk_dly & 0xF) << 24 ;
		reg |= (pclk_revb & 1) << 28;
		if(clk_sel) //0: SPLL, 1: USB
		{	
			Csi1RecUsbPhyCfg = pscuaReg->scuaUsbPhyCfg;
			pscuaReg->scuaUsbPhyCfg |= (1<<2);	//USBPHY Xtal enable;
			pscuaReg->scuaUsbPhyCfg |= (3<<12); //USBP1 enable; 
		}
	}
	else 
	{
		if(Csi1RecUsbPhyCfg)
		{
			pscuaReg->scuaUsbPhyCfg = Csi1RecUsbPhyCfg;
			Csi1RecUsbPhyCfg = 0;
		}
	}
	
	pscuaReg->scuaCsiClkCfg = 0;
	pscuaReg->scuaCsiClkCfg = reg;
}

void
gpHalCsi1GetMclk(
	UINT8 *clk_sel, 
	UINT8 *clko_div,
	UINT8 *pclk_dly,
	UINT8 *pclk_revb
)
{
	UINT32 reg = pscuaReg->scuaCsiClkCfg;

	if(reg & (1<<8))
		*clko_div = reg & 0xFF;
	else
		*clko_div = 0;
	
	*clk_sel = reg & (1<<16) >> 16;
	*pclk_dly = (reg & (0xF << 24)) >> 24;
	*pclk_revb = (reg & (1<<28)) >> 28;
}

static unsigned gCsiPpuIrqEnable = 0;

UINT32
gpHalCsi1Clearisr(
	void
)
{
	UINT32 enable, status;

	//enable = pPPUIrqreg->ppuIrqEn;
	enable = gCsiPpuIrqEnable;
	status = pPPUIrqreg->ppuIrqState;

	//printk("irq status (0x%08x)\n", status);

	status &= enable;
	pPPUIrqreg->ppuIrqState = status;
	return status;
}

void
gpHalCsi1SetIRQ(
	UINT32 bits,
	UINT32 enable
)
{
	if(enable) {
		pPPUIrqreg->ppuIrqState = bits;
		pPPUIrqreg->ppuIrqEn |= bits;
		gCsiPpuIrqEnable |= bits;
	} else {
		pPPUIrqreg->ppuIrqEn &= ~bits;
		pPPUIrqreg->ppuIrqState = bits;
		gCsiPpuIrqEnable &= ~bits;
	}
}

void
gpHalCsi1SetBuf(
	UINT32 addr
)
{
	pCsi1Reg->csifbsaddr = addr;
}

void
gpHalCsi1SetResolution(
	UINT16 hsize,
	UINT16 vsize
)
{
	pCsi1Reg->csihwidth = hsize & 0xFFF;
	pCsi1Reg->csivheight = vsize & 0xFFF;
}

void 
gpHalCsi1SetEnable(
	UINT32 enable
)
{
	if(enable){
		pCsi1Reg->csicr0 |= CSI1_EN;
	} else {
		pCsi1Reg->csicr0 &= ~CSI1_EN;
	}
}

void
gpHalCsi1Start(
	void
)
{
	if(pCsi1Reg->csihwidth & 0x1F)
		pCsi1Reg->csisencr &= ~(1 << 7); //csi1 long burst disable
	else
		pCsi1Reg->csisencr |= (1 << 7); //csi1 long burst enable
	
	pCsi1Reg->csicr1 |= CSI1_HIGHPRI | CSI1_NOSTOP | CSI1_CLKOEN | CSI1_D_TYPE0;
	pCsi1Reg->csicr0 |= CSI1_EN | CSI1_CAP;
	pPPUIrqreg->ppuIrqState = CSI1_FRAME_END | CSI1_UNDER_RUN;
	pPPUIrqreg->ppuIrqEn |= CSI1_FRAME_END;
	gCsiPpuIrqEnable |= CSI1_FRAME_END;
}

void
gpHalCsi1Stop(
	void
)
{
	pCsi1Reg->csicr0 = 0x0;
	pCsi1Reg->csicr1 = 0x0;
	pPPUIrqreg->ppuIrqEn &= ~(CSI1_FRAME_END | CSI1_UNDER_RUN);
	gCsiPpuIrqEnable &= ~(CSI1_FRAME_END | CSI1_UNDER_RUN);
	pPPUIrqreg->ppuIrqState = CSI1_FRAME_END | CSI1_UNDER_RUN;
	pCsi1Reg->csisencr &= ~(1 << 7); //csi1 long burst disable
}

void
gpHalCsi1Close(
	void
)
{
	pscuaReg->scuaSysSel &= ~(CSI_SEL_32900 | PPU_27M_EN);
	pCsi1Reg->csicr0 = 0x0;
	pCsi1Reg->csicr1 = 0x0;
	pPPUIrqreg->ppuIrqEn &= ~(CSI1_FRAME_END | CSI1_UNDER_RUN);
	gCsiPpuIrqEnable &= ~(CSI1_FRAME_END | CSI1_UNDER_RUN);
	pPPUIrqreg->ppuIrqState = CSI1_FRAME_END | CSI1_UNDER_RUN;
}

void
gpHalCsi1Init(
	void
)
{
	pscuaReg->scuaSysSel |= CSI_SEL_32900 | PPU_27M_EN;		//Only for GPL32900
	pCsi1Reg->csicr0 = 0;
	pCsi1Reg->csicr1 = 0;
	pPPUIrqreg->ppuIrqEn &= ~(CSI1_FRAME_END | CSI1_UNDER_RUN);
	gCsiPpuIrqEnable &= ~(CSI1_FRAME_END | CSI1_UNDER_RUN);
	pPPUIrqreg->ppuIrqState = CSI1_FRAME_END | CSI1_UNDER_RUN;
}

SINT32
gpHalCsi1SetInputFmt(
	UINT32 fmt
)
{
	UINT32 reg = pCsi1Reg->csicr0;

	if(fmt == CSI1_UYVY_IN)
	{
		reg &= ~(CSI1_YUV_YUYV | CSI1_RGB565IN);
		reg |= CSI1_YUVIN;
	}
	else if(fmt == CSI1_YUYV_IN)
	{
		reg &= ~(CSI1_RGB565IN);
		reg |= CSI1_YUV_YUYV | CSI1_YUVIN;
	}
	else if(fmt == CSI1_BGRG_IN)
	{
		reg &= ~(CSI1_YUV_YUYV | CSI1_YUVIN | CSI1_RGB565IN);
	}
	else if(fmt == CSI1_GBGR_IN)
	{
		reg &= ~(CSI1_YUVIN | CSI1_RGB565IN);
		reg |= CSI1_YUV_YUYV;
	}
	else if(fmt == CSI1_RGB565_IN)
	{
		reg &= ~(CSI1_YUV_YUYV | CSI1_YUVIN);
		reg |= CSI1_RGB565IN;
	}
	else
		return -1;

	pCsi1Reg->csicr0 = reg;
	return 0;
}

SINT32
gpHalCsi1SetOutputFmt(
	UINT32 fmt
)
{
	UINT32 reg0 = pCsi1Reg->csicr0;
	UINT32 reg1 = pCsi1Reg->csicr1;

	if(fmt == CSI1_VYUY_OUT)
	{
		reg0 |= CSI1_YUVOUT;
		reg1 &= ~CSI1_RGB1555;
		reg1 &= ~CSI1_YONLY;
	}
	else if(fmt == CSI1_YUYV_OUT)
	{
		reg0 |= CSI1_YUVOUT;
		reg1 |= CSI1_RGB1555;
		reg1 &= ~CSI1_YONLY;
	}
	else if(fmt == CSI1_RGB565_OUT)
	{
		reg0 &= ~CSI1_YUVOUT;
		reg1 &= ~CSI1_RGB1555;
		reg1 &= ~CSI1_YONLY;
	}
	else if(fmt == CSI1_RGB1555_OUT)
	{
		reg0 &= ~CSI1_YUVOUT;
		reg1 |= CSI1_RGB1555;
		reg1 &= ~CSI1_YONLY;
	}
	else if(fmt == CSI1_YONLY_OUT)
	{
		reg0 &= ~CSI1_YUVOUT;
		reg1 &= ~CSI1_RGB1555;
		reg1 |= CSI1_YONLY;
	}
	else
		return -1;

	pCsi1Reg->csicr0 = reg0;
	pCsi1Reg->csicr1 = reg1;
	return 0;
}

void
gpHalCsi1SetHVSync(
	UINT32 HsyncAct,
	UINT32 VsyncAct,
	UINT32 clki_inv
)
{
	UINT32 reg = pCsi1Reg->csicr0;

	reg &= ~(CSI1_FGET_RISE | CSI1_HRST_RISE | CSI1_VADD_RISE | CSI1_VRST_RISE);
	if(HsyncAct)
		reg |= CSI1_HRST_RISE | CSI1_VADD_RISE;
	else
		reg |= CSI1_HRST_FALL | CSI1_VADD_FALL;

	if(VsyncAct)
		reg |= CSI1_VRST_RISE;
	else
		reg |= CSI1_VRST_FALL;
	
	if(clki_inv)
		reg |= CSI1_CLKIINV;
	else 
		reg &= ~CSI1_CLKIINV;

	pCsi1Reg->csicr0 = reg;	
}

void
gpHalCsi1SetDataLatchTiming(
	UINT8 d_type
)
{
	UINT32 reg1 = pCsi1Reg->csicr1;;
	
	reg1 &= ~(0x07);
	if(d_type > CSI1_D_TYPE3)
		d_type = CSI1_D_TYPE3;
	
	reg1 |= d_type;
	pCsi1Reg->csicr1 = reg1;
}

SINT32
gpHalCsi1SetInterface(
	UINT32 filed_interlace,
	UINT32 field_inv,
	UINT32 iface
)
{
	UINT32 reg = pCsi1Reg->csicr0;
	
	if(filed_interlace)
		reg |= CSI1_INTERLACE;
	else
		reg &= ~CSI1_INTERLACE;

	if(field_inv)
		reg |= CSI1_FIELDINV;
	else
		reg &= ~CSI1_FIELDINV;
		
	switch(iface)
	{
	case 0: /* ccir 601 */
		reg &= ~CSI1_HREF;
		reg &= ~CSI1_CCIR656;
		break;
	case 1: /* ccir 656 */
		reg &= ~CSI1_HREF;
		reg |= CSI1_CCIR656;
		break;
	case 2: /* href */
		reg |= CSI1_HREF;
		reg &= ~CSI1_CCIR656;
		break;	
	default:
		return -1;
	}

	pCsi1Reg->csicr0 = reg;
	return 0;
}

UINT32
gpHalCsi1GetCtrl(
	UINT32 n
)
{
	UINT32 *addr, reg;

	addr = (UINT32 *)pCsi1Reg;
	addr += n;
	reg = *addr;
	return reg;
}

void
gpHalCsi1Suspend(
	void
)
{
	gpHalCsi1Stop();
	gpHalCsi1Close();
}

void
gpHalCsi1Resume(
	void
)
{
	gpHalCsi1Init();
}

void
gpHalCsi1SetInvYUV(
	UINT8 inv_yuvin,
	UINT8 inv_yuvout
)
{
	UINT32 reg = pCsi1Reg->csicr1;

	if(inv_yuvin)
		reg |= CSI1_INVYUVI;
	else
		reg &= ~CSI1_INVYUVI;

	if(inv_yuvout)
		reg |= CSI1_INVYUVO;
	else
		reg &= ~CSI1_INVYUVO;

	pCsi1Reg->csicr1 = reg;		
}

void 
gpHalCsi1SetHVStart(
	UINT16 hlstart,
	UINT16 vl0start,
	UINT16 vl1start
)
{
	pCsi1Reg->csihlstart = hlstart & 0xFFF;
	pCsi1Reg->csivl0start = vl0start & 0xFFF;
	pCsi1Reg->csivl1start = vl1start & 0xFFF;
}

SINT32 
gpHalCsi1SetScaleDown(
	UINT16 tar_width,
	UINT16 tar_height
) 
{
	UINT32 tempH, tempL;

	tempH = tar_width;
	while(1)
	{
		if(tempH % 2)
			break;
		tempH >>= 1;
	}

	tempL = pCsi1Reg->csihwidth;
	while(1)
	{
		if(tempL % 2)
			break;
		tempL >>= 1;
	}

	if(tempH > 0x7F || tempL > 0x7F) return -1;
	pCsi1Reg->csihratio = (tempH << 8) | (tempL & 0x7F);
	
	tempH = tar_height;
	while(1)
	{
		if(tempH % 2)
			break;
		tempH >>= 1;
	}

	tempL = pCsi1Reg->csivheight;
	while(1)
	{
		if(tempL % 2)
			break;
		tempL >>= 1;
	}

	if(tempH > 0x7F || tempL > 0x7F) return -1;
	pCsi1Reg->csivratio = (tempH << 8) | (tempL & 0x7F);
	return 0;
}

void
gpHalCsi1SetScreenCut(
	UINT16 h_cutstartline,	/* 16 align */
	UINT16 v_cutstartline,	/* 16 align */
	UINT16 h_cutsize,		/* 16 align */
	UINT16 v_cutsize		/* 16 align */
) 
{
	pCsi1Reg->csicutstart = ((v_cutstartline >> 4) << 8) | ((h_cutstartline >> 4) & 0xFF);
	pCsi1Reg->csicutsize = ((v_cutsize >> 4) << 8) | ((h_cutsize >> 4) & 0xFF);
}

void
gpHalCsi1SetCubic(
	UINT8 CubicEn,
	UINT8 Cubic32	/*0:64x64, 1: 32x32 */
)
{
	UINT32 reg = pCsi1Reg->csicr1;

	if(CubicEn)
		reg |= CSI1_CELL;
	else
		reg &= ~CSI1_CELL;

	if(Cubic32)
		reg |= CSI1_CELL32X32;
	else
		reg &= ~CSI1_CELL32X32;

	pCsi1Reg->csicr1 = reg;		
}

void
gpHalCsi1GetCubic(
	UINT32 *CubicEn,
	UINT32 *Cubic32
)
{
	UINT32 reg = pCsi1Reg->csicr1;

	*CubicEn = (reg >> 12) & 0x01;
	*Cubic32 = (reg >> 13) & 0x01;	
}

void
gpHalCsi1SetBlackScreen(
	UINT16 bs_hstart,
	UINT16 bs_vstart,
	UINT16 view_hsize,
	UINT16 view_vsize
) 
{
	pCsi1Reg->csihstart = bs_hstart;
	pCsi1Reg->csihend = bs_hstart + view_hsize;
	pCsi1Reg->csivstart = bs_vstart;
	pCsi1Reg->csivend = bs_vstart + view_vsize;
}

void
gpHalCsi1GetBlackScreen(
	UINT32 *bs_hstart,
	UINT32 *bs_vstart,
	UINT32 *view_hsize,
	UINT32 *view_vsize
) 
{
	*bs_hstart = pCsi1Reg->csihstart;
	*bs_vstart = pCsi1Reg->csivstart;
	*view_hsize = pCsi1Reg->csihend - *bs_hstart;
	*view_vsize = pCsi1Reg->csivend - *bs_vstart;
}

void
gpHalCsi1SetBlueScreen(
	UINT8 BlueScreenEn,
	UINT8 r_upper,	
	UINT8 g_upper,
	UINT8 b_upper,
	UINT8 r_lower,	
	UINT8 g_lower,
	UINT8 b_lower
) 
{
	if(BlueScreenEn)
		pCsi1Reg->csicr0 |= CSI1_BSEN;
	else
		pCsi1Reg->csicr0 &= ~CSI1_BSEN;

	pCsi1Reg->csibsupper = ((r_upper & 0x1F) << 10) | ((g_upper & 0x1F) << 5) | (b_upper & 0x1F);
	pCsi1Reg->csibslower = ((r_lower & 0x1F) << 10) | ((g_lower & 0x1F) << 5) | (b_lower & 0x1F);
}

void
gpHalCsi1GetBlueScreen(
	UINT32 *BlueScreenEn,
	UINT32 *r_upper,	
	UINT32 *g_upper,
	UINT32 *b_upper,
	UINT32 *r_lower,	
	UINT32 *g_lower,
	UINT32 *b_lower
) 
{
	UINT32 reg = pCsi1Reg->csicr0; 
	*BlueScreenEn = (reg >> 7) & 0x01;

	reg = pCsi1Reg->csibsupper;
	*r_upper = (reg >> 10) & 0x1F;
	*g_upper = (reg >> 5) & 0x1F;
	*b_upper = reg & 0x1F;
	
	reg = pCsi1Reg->csibslower;
	*r_lower = (reg >> 10) & 0x1F;
	*g_lower = (reg >> 5) & 0x1F;
	*b_lower = reg & 0x1F;
}

void
gpHalCsi1SetBlending(
	UINT8 blend_en,
	UINT8 blend_level // 0 ~ 63	
) 
{
	UINT32 reg = pCsi1Reg->csisencr;

	reg &= ~(1<<4);
	reg |= (blend_en & 1) << 4;
	reg &= ~(0x3f << 10);
	reg |= (UINT32)(blend_level & 0x3f) << 10;
	pCsi1Reg->csisencr = reg;
}

void
gpHalCsi1GetBlending(
	UINT32 *blend_en,
	UINT32 *blend_level // 0 ~ 63	
) 
{
	UINT32 reg = pCsi1Reg->csisencr;

	*blend_en = (reg >> 4) & 0x1;
	*blend_level = (reg >> 10) & 0x3F;
}

void
gpHalCsi1SetMDEn(
	UINT8 MD_En,
	UINT8 MD_Frame,	
	UINT8 MD_VGA,
	UINT8 MD_yuv,
	UINT8 MD_mode,
	UINT8 MDBlk8x8,		
	UINT8 threshold
)
{
	pCsi1Reg->csimdcr = (threshold & 0x7F) << 9 |
						(MDBlk8x8 & 0x1) << 8 |
						(MD_mode & 0x3) << 6 |
						(MD_yuv & 0x1) << 5 |
						(MD_VGA & 0x1) << 4 |
						(MD_Frame & 0x3) << 2 |
						(MD_En & 0x1) << 1;
}

void
gpHalCsi1SetMDHVPos(
	UINT16 md_h_pos,
	UINT16 md_v_pos
) 
{
	pCsi1Reg->csihpos = md_h_pos & 0x3FF;
	pCsi1Reg->csivpos = md_v_pos & 0x3FF;
}

void
gpHalCsi1SetMDFbAddr(
	UINT32 md_fb_addr
) 
{
	pCsi1Reg->csimdfbaddr = md_fb_addr;
}


