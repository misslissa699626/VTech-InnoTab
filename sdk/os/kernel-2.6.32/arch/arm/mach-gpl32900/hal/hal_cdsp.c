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

/*******************************************************************************
*                         H E A D E R   F I L E S
*******************************************************************************/
#include <linux/io.h>
#include <mach/hal/regmap/reg_cdsp.h>
#include <mach/hal/regmap/reg_scu.h>
#include <mach/hal/hal_cdsp.h>

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
static cdspReg0_t *pCdspReg0 = (cdspReg0_t *)(LOGI_ADDR_CDSP_REG);
static cdspReg1_t *pCdspReg1 = (cdspReg1_t *)(LOGI_ADDR_CDSP_REG);
static cdspReg3a_t *pCdspReg3a = (cdspReg3a_t *)(LOGI_ADDR_CDSP_REG);
static cdspRegFront_t *pCdspRegFront = (cdspRegFront_t *)(LOGI_ADDR_CDSP_REG);
static cdspRegLensCmp_t *pLensData = (cdspRegLensCmp_t *)(LOGI_ADDR_CDSP_REG);
static cdspRegLutGamma_t *pGammaData = (cdspRegLutGamma_t *)(LOGI_ADDR_CDSP_REG);
static cdspRegLutEdge_t *pEdgeLutData = (cdspRegLutEdge_t *)(LOGI_ADDR_CDSP_REG);
static scuaReg_t *pScuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;
static UINT32 CdspRecUsbPhyCfg = 0;

/**
 * @brief	cdsp module reset
 * @param 	enable[in]: enable
 * @return 	none
*/
void
gpHalCdspSetModuleReset(
	UINT32 enable
)
{
	if(enable)
	{
		pScuaReg->scuaPeriRst |= (1<<30);	/* cdsp module reset */
		pScuaReg->scuaPeriRst &= ~(1<<30);
	}
}

/**
 * @brief	cdsp module clock enable
 * @param 	mclk_en[in]: enable
 * @param 	mclk_div[in]: mclk divider, (clko_div+1)
 * @param 	pclk_dly[in]: pclk delay
 * @param 	pclk_revb[in]: pclk reverse 
 * @return 	none
*/
void
gpHalCdspSetMclk(
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
			CdspRecUsbPhyCfg = pScuaReg->scuaUsbPhyCfg;
			pScuaReg->scuaUsbPhyCfg |= (1<<2);	//USBPHY Xtal enable;
			pScuaReg->scuaUsbPhyCfg |= (3<<12); //USBP1 enable; 
		}
	}
	else
	{
		if(CdspRecUsbPhyCfg)
		{
			pScuaReg->scuaUsbPhyCfg = CdspRecUsbPhyCfg;
			CdspRecUsbPhyCfg = 0;
		}
	}
	
	pScuaReg->scuaCsiClkCfg = 0;
	pScuaReg->scuaCsiClkCfg = reg;
}

void
gpHalCdspGetMclk(
	UINT8 *clk_sel, 
	UINT8 *clko_div,
	UINT8 *pclk_dly,
	UINT8 *pclk_revb
)
{
	UINT32 reg = pScuaReg->scuaCsiClkCfg;

	if(reg & (1<<8))
		*clko_div = reg & 0xFF;
	else
		*clko_div = 0;
	
	*clk_sel = reg & (1<<16) >> 16;
	*pclk_dly = (reg & (0xF << 24)) >> 24;
	*pclk_revb = (reg & (1<<28)) >> 28;
}

/**
* @brief	cdsp clock set
* @param 	mode[in]: 0: cdsp system, 1: front sensor, 2: mipi if
* @param 	type[in]: 1: yuv 0: raw
* @return 	none
*/
void 
gpHalCdspSetClk(
	UINT8 mode, 
	UINT8 type
)
{
	switch(mode)
	{
	case C_CDSP_CLK_ENABLE:
		//pScuaReg->scuaPeriClkEn |= (1<<29);	/* CDSP CLK Enable Setting, [29] : CDSP AHB CLK EN */
		pScuaReg->scuaCdspPclk = 0;			/* [8]: CDSP Pixel CLK EN,[10]:cdsp_clk_sw:sysclk */
		pScuaReg->scuaCdspPclk = 0x100|type;	

		pScuaReg->scuaSysSel &= ~(1<<6);	/* 0: Disable, 1: enable, CSI_MCLK */
		pScuaReg->scuaSysSel &= ~(1<<8);	/* 0: Disable, 1: enable, CDSP clock enable */
		pScuaReg->scuaSysSel &= ~(1<<9);	/* 0: Raw, 1: YUV, CDSP YUV data format mode */
		pScuaReg->scuaSysSel &= ~(1<<10);	/* 0: Disable, 1: enable, CDSP post processing enable */
		pScuaReg->scuaSysSel &= ~(1<<11);	/* 0: From csi1, 1: From csi2, CDSP clock enable when using 2th interface */
		pScuaReg->scuaSysSel &= ~(1<<12);	/* 0: Clock from csi, 1: Clock from Mipi,  */
		break;
	case C_CDSP_CLK_DISABLE:
		//pScuaReg->scuaPeriClkEn &= ~(1<<29);
		pScuaReg->scuaCdspPclk = 0;
		pScuaReg->scuaSysSel &= ~(1<<6);
		pScuaReg->scuaSysSel &= ~(1<<8);
		pScuaReg->scuaSysSel &= ~(1<<9);
		pScuaReg->scuaSysSel &= ~(1<<10);
		pScuaReg->scuaSysSel &= ~(1<<11);	
		pScuaReg->scuaSysSel &= ~(1<<12);
		break;
	case C_CDSP_CLK_FB:
		pScuaReg->scuaSysSel &= ~(1<<6);
		pScuaReg->scuaSysSel |= (1<<8);	
		if(type == 1) 
 			pScuaReg->scuaSysSel |= (1<<9) ; 
		else
			pScuaReg->scuaSysSel &= ~(1<<9); 

		pScuaReg->scuaSysSel |= (1<<10); 	
		pScuaReg->scuaSysSel &= ~(1<<11);	
 		pScuaReg->scuaSysSel &= ~(1<<12);	
		break;
	case C_CDSP_CLK_FRONT: 
		pScuaReg->scuaSysSel |= (1<<6);	
		pScuaReg->scuaSysSel |= (1<<8);
		if(type == 1) 
 			pScuaReg->scuaSysSel |= (1<<9);
		else
			pScuaReg->scuaSysSel &= ~(1<<9);
		 
		pScuaReg->scuaSysSel &= ~(1<<10);
		pScuaReg->scuaSysSel &= ~(1<<11);
 		pScuaReg->scuaSysSel &= ~(1<<12);
		break;
	case C_CDSP_CLK_MIPI:
 		pScuaReg->scuaSysSel |= (1<<6);
		pScuaReg->scuaSysSel |= (1<<8);
		if(type == 1) 
 			pScuaReg->scuaSysSel |= (1<<9);
		else
			pScuaReg->scuaSysSel &= ~(1<<9);

		pScuaReg->scuaSysSel &= ~(1<<10);
		pScuaReg->scuaSysSel &= ~(1<<11);
		pScuaReg->scuaSysSel |= (1<<12);
		break;
	}
}

/**
* @brief	cdsp reset
* @param 	none
* @return 	none
*/
void 
gpHalCdspReset(
	void
)
{
	pCdspReg0->cdspReset = 0x01;
	pCdspReg0->cdspReset = 0x00;
}

/**
* @brief	cdsp raw data format
* @param	format [in]: format
* @return	none
*/
void 
gpHalCdspSetRawDataFormat(
	UINT8 format
)
{
	format &= 0x03;
	pCdspReg0->cdspImgType = format;
}

UINT32
gpHalCdspGetRawDataFormat(
	void 
)
{
	return pCdspReg0->cdspImgType &= 0x03;
}

/**
* @brief	cdsp yuv range
* @param	signed_yuv_en [in]: yuv signed/unsigned set
* @return	none
*/
void 
gpHalCdspSetYuvRange(
	UINT8 signed_yuv_en
)
{
	UINT32 reg;
	
	reg = pCdspReg0->cdspYuvRange;
	reg &= ~0x07;
	reg |= (signed_yuv_en & 0x07);
	pCdspReg0->cdspYuvRange = reg;
}

UINT32 
gpHalCdspGetYuvRange(
	void 
)
{
	return pCdspReg0->cdspYuvRange & 0x07;
}

/**
* @brief	cdsp image source
* @param	image_source [in]: image input source, 0:front, 1:sdram, 2:mipi
* @return	none
*/
void 
gpHalCdspDataSource(
	UINT8 image_source
)
{
	UINT32 reg;

	reg = pCdspReg1->cdspDo;
	reg &= ~(0x03 << 4);
	reg |= (image_source & 0x3) << 4;
	pCdspReg1->cdspDo = reg;
}

UINT32 
gpHalCdspGetDataSource(
	void
)
{
	UINT32 reg = pCdspReg1->cdspDo;
	return ((reg >> 4) & 0x03);
}

/**
* @brief	cdsp post-process triger
* @param	docdsp [in]: enable
* @return	none
*/
void 
gpHalCdspRedoTriger(
	UINT8 docdsp
)
{
	if(docdsp)
		pCdspReg1->cdspDo |= 0x01;
	else
		pCdspReg1->cdspDo &= ~0x01;
}

/**
* @brief	cdsp interrupt enable
* @param	enable [in]: enable / disable
* @param	bit [in]: interrupt bit
* @return	none
*/
void
gpHalCdspSetIntEn(
	UINT8 enable, 
	UINT8 bit
)
{
	if(enable)
		pCdspReg0->cdspIntEn |= bit;
	else
		pCdspReg0->cdspIntEn &= ~bit;
}

/**
* @brief	cdsp interrupt clear status
* @param	bit [in]: clear interrupt bit
* @return	none
*/
void
gpHalCdspClrIntStatus(
	UINT8 bit
)
{
	pCdspReg0->cdspInt |= bit;
}

/**
* @brief	cdsp get int status
* @param	none
* @return	int status
*/
UINT32 
gpHalCdspGetIntStatus(
	void
)
{
	UINT32 irq = 0;
	
	if(pCdspReg0->cdspInt & pCdspReg0->cdspIntEn & CDSP_FACWR)
	{
		pCdspReg0->cdspInt |= CDSP_FACWR;
		irq |= CDSP_FACWR;
	}
		
	if(pCdspReg0->cdspInt & pCdspReg0->cdspIntEn & CDSP_OVERFOLW)
	{
		pCdspReg0->cdspInt |= CDSP_OVERFOLW;
		irq |= CDSP_OVERFOLW;
	}
	
	if(pCdspReg0->cdspInt & pCdspReg0->cdspIntEn & CDSP_EOF)
	{
		pCdspReg0->cdspInt |= CDSP_EOF;
		irq |= CDSP_EOF;
	}
		
	if(pCdspReg0->cdspInt & pCdspReg0->cdspIntEn & CDSP_AEWIN_SEND)
	{
		pCdspReg0->cdspInt |= CDSP_AEWIN_SEND;
		irq |= CDSP_AEWIN_SEND;
	}
		
	if(pCdspReg0->cdspInt & pCdspReg0->cdspIntEn & CDSP_AWBWIN_UPDATE)
	{
		pCdspReg0->cdspInt |= CDSP_AWBWIN_UPDATE;
		irq |= CDSP_AWBWIN_UPDATE;
	}
		
	if(pCdspReg0->cdspInt & pCdspReg0->cdspIntEn & CDSP_AFWIN_UPDATE)
	{
		pCdspReg0->cdspInt |= CDSP_AFWIN_UPDATE;
		irq |= CDSP_AFWIN_UPDATE;
	}
	return irq;
}

/**
* @brief	cdsp get front vd int status
* @param	none
* @return	int status
*/
UINT32 
gpHalCdspGetFrontVdIntStatus(
	void
)
{
	UINT32 irq = 0;
	
	if(pCdspRegFront->cdspVdrfInt & pCdspRegFront->cdspVdrfIntEn & FRONT_VD_RISE)
	{
		pCdspRegFront->cdspVdrfInt |= FRONT_VD_RISE;
		irq |= FRONT_VD_RISE;
	} 
		
	if(pCdspRegFront->cdspVdrfInt & pCdspRegFront->cdspVdrfIntEn & FRONT_VD_FALL) 
	{
		pCdspRegFront->cdspVdrfInt |= FRONT_VD_FALL;
		irq |= FRONT_VD_FALL;
	}
	return irq;
}

/**
* @brief	cdsp get front int status
* @param	none
* @return	int status
*/
UINT32 
gpHalCdspGetFrontIntStatus(
	void
)
{
	UINT32 irq = 0;
	
	if(pCdspRegFront->cdspInt & pCdspRegFront->cdspIntEn & VDR_EQU_VDRINT_NO) 
	{
		pCdspRegFront->cdspInt |= VDR_EQU_VDRINT_NO;
		irq |= VDR_EQU_VDRINT_NO;
	} 
	
	if(pCdspRegFront->cdspInt & pCdspRegFront->cdspIntEn & VDF_EQU_VDFINT_NO) 
	{
		pCdspRegFront->cdspInt |= VDF_EQU_VDFINT_NO;
		irq |= VDR_EQU_VDRINT_NO;
	}
	
	if(pCdspRegFront->cdspInt & pCdspRegFront->cdspIntEn & SERIAL_DONE) 
	{
		pCdspRegFront->cdspInt |= SERIAL_DONE;
		irq |= VDR_EQU_VDRINT_NO;
	}
	
	if(pCdspRegFront->cdspInt & pCdspRegFront->cdspIntEn & SNAP_DONE) 
	{
		pCdspRegFront->cdspInt |= SNAP_DONE;
		irq |= VDR_EQU_VDRINT_NO;
	}
	
	if(pCdspRegFront->cdspInt & pCdspRegFront->cdspIntEn & CLR_DO_CDSP) 
	{
		pCdspRegFront->cdspInt |= CLR_DO_CDSP;
		irq |= VDR_EQU_VDRINT_NO;
	}
	
	if(pCdspRegFront->cdspInt & pCdspRegFront->cdspIntEn & EHDI_FS_EQU_INTH_NO) 
	{
		pCdspRegFront->cdspInt |= EHDI_FS_EQU_INTH_NO;
		irq |= VDR_EQU_VDRINT_NO;
	}
	
	if(pCdspRegFront->cdspInt & pCdspRegFront->cdspIntEn & FRONT_VVALID_RISE) 
	{
		pCdspRegFront->cdspInt |= FRONT_VVALID_RISE;
		irq |= VDR_EQU_VDRINT_NO;
	}
	
	if(pCdspRegFront->cdspInt & pCdspRegFront->cdspIntEn & FRONT_VVALID_FALL) 
	{
		pCdspRegFront->cdspInt |= FRONT_VVALID_FALL;
		irq |= VDR_EQU_VDRINT_NO;
	}
	
	if(pCdspRegFront->cdspInt & pCdspRegFront->cdspIntEn & OVERFLOW_RISE) 
	{
		pCdspRegFront->cdspInt |= OVERFLOW_RISE;
		irq |= VDR_EQU_VDRINT_NO;
	}
	
	if(pCdspRegFront->cdspInt & pCdspRegFront->cdspIntEn & OVERFLOW_FALL) 
	{
		pCdspRegFront->cdspInt |= OVERFLOW_FALL;
		irq |= VDR_EQU_VDRINT_NO;
	}
	return irq;
}

/**
* @brief	cdsp get global int status
* @param	none
* @return	int status
*/
UINT32
gpHalCdspGetGlbIntStatus(
	void
)
{
	UINT32 irq = 0;
	
	if(pCdspReg1->cdspGInt & CDSP_INT_BIT)
		irq |= CDSP_INT_BIT;

	if(pCdspReg1->cdspGInt & FRONT_VD_INT_BIT) 
		irq |= FRONT_VD_INT_BIT;

	if(pCdspReg1->cdspGInt & FRONT_INT_BIT)
		irq |= FRONT_INT_BIT;
	
	return irq;
}

/**
* @brief	cdsp bad pixel enable
* @param	badpixen [in]: bad pixel enable
* @param	badpixen [in]: bad pixel mirror enable, bit1: right, bit0: left, 
* @return	none
*/
void 
gpHalCdspSetBadPixelEn(
	UINT8 badpixen, 
	UINT8 badpixmiren
)
{
	badpixen &= 0x01;
	badpixmiren &= 0x03;
	pCdspReg0->cdspBadPixCtrl = (badpixen << 3)|badpixmiren;
}

void 
gpHalCdspGetBadPixelEn(
	UINT8 *badpixen, 
	UINT8 *badpixmiren
)
{
	UINT32 reg = pCdspReg0->cdspBadPixCtrl;
	*badpixen = (reg & (0x01 << 3)) >> 3;
	*badpixmiren = reg & 0x03;
}

/**
* @brief	cdsp bad pixel threshold set
* @param	bprthr [in]: R threshold
* @param	bpgthr [in]: G threshold 
* @param	bpbthr [in]: B threshold 
* @return	none
*/
void 
gpHalCdspSetBadPixel(
	UINT16 bprthr, 
	UINT16 bpgthr, 
	UINT16 bpbthr
)
{
	bprthr = (bprthr & 0x3FF) >> 2;
	bpgthr = (bpgthr & 0x3FF) >> 2;
	bpbthr = (bpbthr & 0x3FF) >> 2;
	pCdspReg0->cdspBadPixThr = ((UINT32)bpbthr << 16)|((UINT32)bpgthr << 8)|bprthr;
}

void 
gpHalCdspGetBadPixel(
	UINT16 *bprthr, 
	UINT16 *bpgthr, 
	UINT16 *bpbthr
)
{
	UINT32 reg = pCdspReg0->cdspBadPixThr;
	
	*bprthr = reg & 0xFF;
	*bpgthr = (reg >> 8) & 0xFF;
	*bpbthr = (reg >> 16) & 0xFF;
}

/**
* @brief	cdsp manual optical black enable
* @param	manuoben [in]: manual optical black enable
* @param	manuob [in]: manual optical subtracted value 
* @return	none
*/
void 
gpHalCdspSetManuOB(
	UINT8 manuoben, 
	UINT16 manuob
)
{
	manuoben &= 0x1;
	manuob &= 0x7FF;
	pCdspReg0->cdspOpbCtrl = ((UINT32)manuoben << 15)|manuob;
}

void 
gpHalCdspGetManuOB(
	UINT8 *manuoben, 
	UINT16 *manuob
)
{
	UINT32 reg = pCdspReg0->cdspOpbCtrl;
	
	*manuoben = (reg >> 15) & 0x01;
	*manuob = reg & 0x7FF;
}
/**
* @brief	cdsp auto optical black enable
* @param	autooben [in]: auto optical black enable
* @param	obtype [in]: auto optical accumulation block type
* @param	obHOffset [in]: auto optical accumulation block horizontal offset
* @param	obVOffset [in]: auto optical accumulation block vertical offset
* @return	none
*/
void 
gpHalCdspSetAutoOB(
	UINT8 autooben, 
	UINT8 obtype, 
	UINT16 obHOffset, 
	UINT16 obVOffset
)
{
	obtype &= 0x7;
	autooben &= 0x1;
	obHOffset &= 0xFFF;
	obVOffset &= 0xFFF;
	pCdspReg0->cdspOpbType = (autooben << 3)|obtype; 
	pCdspReg0->cdspOpbHOffset = obHOffset;
	pCdspReg0->cdspOpbVOffset = obVOffset;
}

void 
gpHalCdspGetAutoOB(
	UINT8 *autooben, 
	UINT8 *obtype, 
	UINT16 *obHOffset, 
	UINT16 *obVOffset
)
{
	UINT32 reg = pCdspReg0->cdspOpbType;

	*autooben = (reg >> 3) & 0x01;
	*obtype = reg & 0x07;
	*obHOffset = pCdspReg0->cdspOpbHOffset;
	*obVOffset = pCdspReg0->cdspOpbVOffset;
}

/**
* @brief	cdsp get auto optical black average
* @param	Ravg [out]: r average
* @param	GRavg [out]: gr average
* @param	Bavg [out]: b average
* @param	GBavg [out]: gb average
* @return	none
*/
void 
gpHalCdspGetAutoOBAvg(
	UINT16 *Ravg, 
	UINT16 *GRavg, 
	UINT16 *Bavg, 
	UINT16 *GBavg
)
{
	*Ravg = pCdspReg0->cdspOpbRAvg;
	*GRavg = pCdspReg0->cdspOpbGrAvg;
	*Bavg = pCdspReg0->cdspOpbBAvg;
	*GBavg = pCdspReg0->cdspOpbGbAvg;
}

/**
* @brief	cdsp lens compensation enable
* @param	plensdata [in]: lens compensation table pointer
* @return	none
*/
void 
gpHalCdspInitLensCmp(
	UINT16 *plensdata
)
{
	SINT32 i;
	
	pCdspReg0->cdspMacroCtrl = 0x11;
	for(i=0; i<256; i++)
		pLensData->LensTable[i] = *plensdata++;

	pCdspReg0->cdspMacroCtrl = 0x00;
}

/**
* @brief	cdsp lens compensation enable
* @param	lcen [in]: lens compensation enable
* @return	none
*/
void 
gpHalCdspSetLensCmpEn(
	UINT8 lcen
)
{
	if(lcen)
		pCdspReg0->cdspLensCmpCtrl |= 1 << 4;
	else
		pCdspReg0->cdspLensCmpCtrl &= ~(1 << 4);
}

UINT32 
gpHalCdspGetLensCmpEn(
	void 
)
{
	UINT32 reg = pCdspReg0->cdspLensCmpCtrl;
	return ((reg >> 4) & 0x01);		
}

/**
* @brief	cdsp lens compensation postion
* @param	centx [in]: X center 
* @param	centy [in]: Y center
* @param	xmoffset [in]: X offset
* @param	ymoffset [in]: Y offset
* @return	none
*/
void 
gpHalCdspSetLensCmpPos(
	UINT16 centx, 
	UINT16 centy, 
	UINT16 xmoffset, 
	UINT16 ymoffset 
)
{
	UINT32 reg;
	
	centx &= 0x1FFF;
	centy &= 0x1FFF;
	pCdspReg0->cdspImgXCent = centx;
	pCdspReg0->cdspImgYCent = centy;

	reg = pCdspReg0->cdspLensCmpXOffset;
	reg &= ~0xFFF;
	reg |= xmoffset;
	pCdspReg0->cdspLensCmpXOffset = reg;	
	ymoffset &= 0xFFF;
	pCdspReg0->cdspLensCmpYOffset = ymoffset;
}

void 
gpHalCdspGetLensCmpPos(
	UINT16 *centx, 
	UINT16 *centy, 
	UINT16 *xmoffset, 
	UINT16 *ymoffset 
)
{
	*centx = pCdspReg0->cdspImgXCent & 0x1FFF;
	*centy = pCdspReg0->cdspImgYCent & 0x1FFF;
	*xmoffset = pCdspReg0->cdspLensCmpXOffset & 0xFFF;
	*ymoffset = pCdspReg0->cdspLensCmpYOffset & 0xFFF;
}

/**
* @brief	cdsp lens compensation enable
* @param	stepfactor [in]: step unit between entries of len shading compensation LUT
* @param	xminc [in]: X increase step
* @param	ymoinc [in]: Y increase step odd line
* @param	ymeinc [in]: Y increase step even lin
* @return	none
*/
void 
gpHalCdspSetLensCmp(
	UINT8 stepfactor,
	UINT8 xminc, 
	UINT8 ymoinc, 
	UINT8 ymeinc
)
{	
	UINT32 reg;

	reg = pCdspReg0->cdspLensCmpCtrl;
	reg &= ~0x7; 
	reg |= (stepfactor & 0x7); 
	pCdspReg0->cdspLensCmpCtrl = reg;

	reg = pCdspReg0->cdspLensCmpXOffset;
	reg &= ~(0xF << 12);
	reg |= ((UINT32)(xminc & 0xF) << 12);
	pCdspReg0->cdspLensCmpXOffset = reg;

	ymeinc &= 0xF;
	ymoinc &= 0xF;
	pCdspReg0->cdspLensCmpYInStep = ((UINT32)ymeinc << 4)|ymoinc;
}

void 
gpHalCdspGetLensCmp(
	UINT8 *stepfactor,
	UINT8 *xminc, 
	UINT8 *ymoinc, 
	UINT8 *ymeinc
)
{	
	UINT32 reg = pCdspReg0->cdspLensCmpCtrl;

	*stepfactor = reg & 0x07;
	reg = pCdspReg0->cdspLensCmpXOffset;
	*xminc = (reg >> 12) & 0x0F;
	reg = pCdspReg0->cdspLensCmpYInStep;
	*ymeinc = (reg >> 4) & 0xF;
	*ymoinc = reg & 0xF;
}

/**
* @brief	cdsp yuv lens compensation path set
* @param	yuvlens [in]: 0:yuv path2, 1:yuv path5
* @return	none
*/
void
gpHalCdspSetLensCmpPath(
	UINT8 yuvlens
)
{
	if(yuvlens)
		pCdspReg0->cdspLensCmpCtrl |= 0x20;
	else
		pCdspReg0->cdspLensCmpCtrl &= ~0x20;
}

UINT32
gpHalCdspGetLensCmpPath(
	void
)
{
	UINT32 reg = pCdspReg0->cdspLensCmpCtrl;
	return ((reg >> 5) & 0x01);
}

/**
* @brief	cdsp crop function enable
* @param	hv_crop_en [in]: h/v crop enable
* @return	none
*/
void 
gpHalCdspSetCropEn(
	UINT8 hv_crop_en
)
{
	UINT32 reg;
	
	if(hv_crop_en) hv_crop_en = 0x3;
	if(pCdspReg0->cdspImgCropHOffset == 0)
		hv_crop_en &= ~0x01;
	
	if(pCdspReg0->cdspImgCropVOffset == 0)
		hv_crop_en &= ~0x02;

	reg = hv_crop_en; 	
	/* reflected at vd update */
	reg |= (1 << 4);
	pCdspReg0->cdspImgCropCtrl = reg;
	/* vdupdate */
	pCdspRegFront->cdspFrontCtrl0 |= (1<<4);
}

UINT32 
gpHalCdspGetCropEn(
	void
)
{
	UINT32 reg = pCdspReg0->cdspImgCropCtrl;
	return (reg & 0x03);
}

/**
* @brief	cdsp crop function
* @param	crop_hoffset [in]: h offset set
* @param	crop_voffset [in]: v offset set
* @param	crop_hsize [in]: h crop size
* @param 	crop_vsize [in]: v crop size
* @return	none
*/
void 
gpHalCdspSetCrop(
	UINT16 crop_hoffset, 
	UINT16 crop_voffset, 
	UINT16 crop_hsize, 
	UINT16 crop_vsize
)
{
	crop_hoffset &= 0xFFF;
	crop_voffset &= 0xFFF;
	crop_hsize &= 0xFFF;
	crop_vsize &= 0xFFF;
	pCdspReg0->cdspImgCropHOffset = crop_hoffset;
	pCdspReg0->cdspImgCropVOffset = crop_voffset;
	pCdspReg0->cdspImgCropHSize= crop_hsize;
	pCdspReg0->cdspImgCropVSize = crop_vsize;
}

void 
gpHalCdspGetCrop(
	UINT16 *crop_hoffset, 
	UINT16 *crop_voffset, 
	UINT16 *crop_hsize, 
	UINT16 *crop_vsize
)
{
	*crop_hoffset = pCdspReg0->cdspImgCropHOffset & 0xFFF;
	*crop_voffset = pCdspReg0->cdspImgCropVOffset & 0xFFF;
	*crop_hsize = pCdspReg0->cdspImgCropHSize & 0xFFF;
	*crop_vsize = pCdspReg0->cdspImgCropVSize & 0xFFF;
}

/**
* @brief	cdsp raw horizontal scale down enable
* @param	hscale_en [in]: horizontal scale down enable 
* @param	hscale_mode [in]: scale down mode, 0:drop, 1:filter
* @return	none
*/
void 
gpHalCdspSetRawHScaleEn(
	UINT8 hscale_en, 
	UINT8 hscale_mode
)
{
	UINT32 reg;

	reg = pCdspReg0->cdspScaleDownCtrl;
	reg &= ~(1 << 3 | 1 << 0);
	reg |= ((hscale_en & 0x1) << 3)|(hscale_mode & 0x1);
	pCdspReg0->cdspScaleDownCtrl = reg;
}

void 
gpHalCdspGetRawHScaleEn(
	UINT8 *hscale_en, 
	UINT8 *hscale_mode
)
{
	UINT32 reg = pCdspReg0->cdspScaleDownCtrl;

	*hscale_en = (reg >> 3) & 0x01;
	*hscale_mode = reg & 0x01;
}

/**
* @brief	cdsp raw horizontal scale down set
* @param	src_hsize [in]: source width 
* @param	dst_hsize [in]: densting width
* @return	none
*/
void 
gpHalCdspSetRawHScale(
	UINT8 src_hsize, 
	UINT8 dst_hsize
)
{
	UINT32 factor;
	
	if (dst_hsize >= src_hsize) 
	{
		pCdspReg0->cdspHRawScaleFactor = 0x0000;
	}
	else 
	{
		factor = (dst_hsize << 16) / (src_hsize) + 1;
		pCdspReg0->cdspHscaleEvenPval = (factor/2)+0x8000;
		pCdspReg0->cdspHScaleOddPVal = factor;
		pCdspReg0->cdspHRawScaleFactor = factor;
	}
	
	/* reflected at next vaild vd edge */
	pCdspReg0->cdspScaleFactorCtrl |= (1 << 0); 
	/* vdupdate */
	pCdspRegFront->cdspFrontCtrl0 |= (1<<4);
}

/**
* @brief	cdsp whitle balance offset set
* @param	roffset [in]: R offset 
* @param	groffset [in]: Gr offset
* @param	boffset [in]: B offset
* @param	gboffset [in]: Gb offset
* @return	none
*/
void 
gpHalCdspSetWbOffset(
	UINT8 wboffseten, 
	UINT8 roffset, 
	UINT8 groffset, 
	UINT8 boffset, 
	UINT8 gboffset
)
{
	UINT32 reg;

	reg = pCdspReg0->cdspWhbalRSet; 
	reg &= ~(0xFF << 12);
	reg |= (UINT32)roffset << 12;
	pCdspReg0->cdspWhbalRSet = reg;

	reg = pCdspReg0->cdspWhbalGrSet; 
	reg &= ~(0xFF << 12);
	reg |= (UINT32)groffset << 12;
	pCdspReg0->cdspWhbalGrSet = reg;

	reg = pCdspReg0->cdspWhbalBSet; 
	reg &= ~(0xFF << 12);
	reg |= (UINT32)boffset << 12;
	pCdspReg0->cdspWhbalBSet = reg;

	reg = pCdspReg0->cdspWhbalGbSet; 
	reg &= ~(0xFF << 12);
	reg |= (UINT32)gboffset << 12;
	pCdspReg0->cdspWhbalGbSet = reg;

	if(wboffseten)
		pCdspReg0->cdspYuvSpMode |= (1 << 6);	
	else
		pCdspReg0->cdspYuvSpMode &= ~(1 << 6);
}

void 
gpHalCdspGetWbOffset(
	UINT8 *wboffseten, 
	UINT8 *roffset, 
	UINT8 *groffset, 
	UINT8 *boffset, 
	UINT8 *gboffset
)
{
	UINT32 reg;
	
	reg = pCdspReg0->cdspYuvSpMode;
	*wboffseten = (reg >> 6) & 0x01;
	reg = pCdspReg0->cdspWhbalRSet;
	*roffset = ( reg >> 12) & 0xFF;
	reg = pCdspReg0->cdspWhbalGrSet;
	*groffset = (reg >> 12) & 0xFF;
	reg = pCdspReg0->cdspWhbalBSet;
	*boffset = (reg >> 12) & 0xFF;
	reg = pCdspReg0->cdspWhbalGbSet;
	*gboffset = (reg >> 12) & 0xFF;	
}

/**
* @brief	cdsp whitle balance offset set
* @param	r_gain [in]: R gain 
* @param	gr_gain [in]: Gr gain
* @param	b_gain [in]: B gain
* @param	gb_gain [in]: Gb gain
* @return 	none
*/
void
gpHalCdspSetWbGain(
	UINT8 wbgainen,
	UINT16 rgain, 
	UINT16 grgain, 
	UINT16 bgain, 
	UINT16 gbgain
)
{
	UINT32 reg;

	reg = pCdspReg0->cdspWhbalRSet; 
	reg &= ~0x1FF;
	reg |= rgain & 0x1FF;
	pCdspReg0->cdspWhbalRSet = reg;

	reg = pCdspReg0->cdspWhbalGrSet; 
	reg &= ~0x1FF;
	reg |= grgain & 0x1FF;
	pCdspReg0->cdspWhbalGrSet = reg;

	reg = pCdspReg0->cdspWhbalBSet; 
	reg &= ~0x1FF;
	reg |= bgain & 0x1FF;
	pCdspReg0->cdspWhbalBSet = reg;
	
	reg = pCdspReg0->cdspWhbalGbSet; 
	reg &= ~0x1FF;
	reg |= gbgain & 0x1FF;
	pCdspReg0->cdspWhbalGbSet = reg;

	reg = pCdspReg0->cdspYuvSpMode;
	if(wbgainen)
		reg |= (1 << 7);	
	else
		reg &= ~(1 << 7);

	/* reflected at vd update */	
	reg |= (1 << 4); 
	pCdspReg0->cdspYuvSpMode = reg;
	/* vdupdate */
	pCdspRegFront->cdspFrontCtrl0 |= (1<<4);
}

void
gpHalCdspGetWbGain(
	UINT8 *wbgainen,
	UINT16 *rgain, 
	UINT16 *grgain, 
	UINT16 *bgain, 
	UINT16 *gbgain
)
{
	UINT32 reg = pCdspReg0->cdspYuvSpMode;
	
	*wbgainen = (reg >> 7) & 0x01;
	*rgain = pCdspReg0->cdspWhbalRSet & 0x1FF;
	*grgain = pCdspReg0->cdspWhbalGrSet & 0x1FF;
	*bgain = pCdspReg0->cdspWhbalBSet & 0x1FF;
	*gbgain = pCdspReg0->cdspWhbalGbSet & 0x1FF;
}

/**
* @brief	cdsp whitle balance global gain set
* @param	global_gain [in]: wb global gain set
* @return	none
*/
void 
gpHalCdspSetGlobalGain(
	UINT8 global_gain
)
{
	pCdspReg0->cdspGlobalGain = global_gain;
}

UINT32 
gpHalCdspGetGlobalGain(
	void
)
{
	return pCdspReg0->cdspGlobalGain;
}

/**
* @brief	cdsp lut gamma table enable
* @param	pGammaTable [in]: lut gamma table pointer 
* @return	none
*/
void 
gpHalCdspInitGamma(
	UINT32 *pGammaTable
)
{
	SINT32 i;

	pCdspReg0->cdspMacroCtrl= 0x22;
	for(i=0; i<128; i++) 
		pGammaData->GammaTable[i] = *pGammaTable++;
	
	pCdspReg0->cdspMacroCtrl = 0;
}

/**
* @brief	cdsp lut gamma table enable
* @param	lut_gamma_en [in]: lut gamma table enable 
* @return	none
*/
void 
gpHalCdspSetLutGammaEn(
	UINT8 lut_gamma_en
)
{
	if(lut_gamma_en)
		pCdspReg0->cdspYuvSpMode |= 0x100;
	else
		pCdspReg0->cdspYuvSpMode &= ~0x100;
}

UINT32 
gpHalCdspGetLutGammaEn(
	void
)
{
	UINT32 reg = pCdspReg0->cdspYuvSpMode;
	return ((reg >> 8) & 0x01);
}

/**
* @brief	cdsp set line buffer
* @param	path [in]: 0: raw data, 1: YUV data	
* @return	none
*/
void
gpHalCdspSetLineCtrl(
	UINT8 ybufen	
)
{
	if(ybufen)
		pCdspReg0->cdspYuvCtrl |= 0x80;	
	else
		pCdspReg0->cdspYuvCtrl &= ~0x80;
}

/**
* @brief	cdsp set external line and bank
* @param	linesize [in]: external line size
* @param	lineblank [in]: external bank size
* @return	none
*/
void
gpHalCdspSetExtLine(
	UINT16 linesize,
	UINT16 lineblank
)
{	
	pCdspReg0->cdspExtLineSize = linesize;
	pCdspReg0->cdspExtBankSize= lineblank;
}
	
/**
* @brief	cdsp external line path set
* @param	extinen [in]: external line enable
* @param	path [in]: 0:interpolution, 1:uvsuppression
* @return	none
*/
void
gpHalCdspSetExtLinePath(
	UINT8 extinen,
	UINT8 path
)
{
	UINT32 reg;

	reg = pCdspReg0->cdspInpMirCtrl;
	if(extinen)
		reg |= 0x20;
	else
		reg &= ~0x20;

	if(path)
		reg |= 0x40;
	else
		reg &= ~0x40;
	
	pCdspReg0->cdspInpMirCtrl = reg;
}

/**
* @brief	cdsp interpolation mirror enable
* @param	intplmiren [in]: mirror enable, bit0:left, bit1:right, bit2:top, bit3:down
* @param	intplmirvsel [in]: vertical down mirror postion select
* @param	intplcnt2sel [in]: initial count select 0:0x0, 1:0x7
* @return	none
*/
void 
gpHalCdspSetIntplMirEn(
	UINT8 intplmiren, 
	UINT8 intplmirvsel, 
	UINT8 intplcnt2sel
)
{
	UINT32 reg;
	
	intplcnt2sel &= 0x1;
	intplmirvsel &= 0x1;
	intplmiren &= 0x0F;
	reg = pCdspReg0->cdspInpMirCtrl; 
	reg &= ~(1 << 7 | 1 << 4 | 0x0F);
	reg |= (intplcnt2sel << 7)|(intplmirvsel << 4)|intplmiren;
	pCdspReg0->cdspInpMirCtrl = reg;
}

void 
gpHalCdspGetIntplMirEn(
	UINT8 *intplmiren, 
	UINT8 *intplmirvsel, 
	UINT8 *intplcnt2sel
)
{
	UINT32 reg = pCdspReg0->cdspInpMirCtrl;
	
	*intplcnt2sel = (reg >> 7) & 0x1;
	*intplmirvsel = (reg >> 4) & 0x1;
	*intplmiren = reg & 0x0F;
}

/**
* @brief	cdsp interpolation threshold set
* @param	int_low_thr [in]: low threshold
* @param	int_hi_thr [in]: heig threshold
* @return	none
*/
void 
gpHalCdspSetIntplThr(
	UINT8 int_low_thr, 
	UINT8 int_hi_thr
)
{
	pCdspReg0->cdspInpDenoiseThr = ((UINT16)int_hi_thr << 8)|int_low_thr;
}

void 
gpHalCdspGetIntplThr(
	UINT8 *int_low_thr, 
	UINT8 *int_hi_thr
)
{
	UINT32 reg = pCdspReg0->cdspInpDenoiseThr;

	*int_low_thr = reg & 0xFF;
	*int_hi_thr = (reg >> 8) & 0xFF;
}

/**
* @brief	cdsp raw special mode set
* @param	rawspecmode [in]: raw special mode
* @return	none
*/
void 
gpHalCdspSetRawSpecMode(
	UINT8 rawspecmode
)
{
	UINT32 reg;
	
	reg = pCdspReg0->cdspRgbSpMode;
	reg &= ~0x07;
	if(rawspecmode > 6) rawspecmode = 6;
	reg |= rawspecmode;
	
	/* reflected at vd update */ 
	reg |= (1 << 3);
	pCdspReg0->cdspRgbSpMode = reg;
	/* vdupdate */
	pCdspRegFront->cdspFrontCtrl0 |= (1<<4);
}

UINT32 
gpHalCdspGetRawSpecMode(
	void
)
{
	UINT32 reg = pCdspReg0->cdspRgbSpMode;
	return (reg & 0x07);
}

/**
* @brief	cdsp edge source set
* @param	posyedgeen [in]: 0:raw, 1: YUV
* @return	none
*/
void
gpHalCdspSetEdgeSrc(
	UINT8 posyedgeen
)
{
	if(posyedgeen)
		pCdspReg0->cdspInpEdgeCtrl |= 0x02;
	else
		pCdspReg0->cdspInpEdgeCtrl &= ~0x02;
}

UINT32
gpHalCdspGetEdgeSrc(
	void
)
{
	UINT32 reg = pCdspReg0->cdspInpEdgeCtrl;
	return ((reg >> 1) & 0x01);
}

/**
* @brief	cdsp edge enable
* @param	edgeen [in]: edge enable, effective when raw data input
* @return	none
*/
void 
gpHalCdspSetEdgeEn(
	UINT8 edgeen
)
{
	if(edgeen)
		pCdspReg0->cdspInpEdgeCtrl |= 0x01;
	else
		pCdspReg0->cdspInpEdgeCtrl &= ~0x01;
}

UINT32
gpHalCdspGetEdgeEn(
	void
)
{
	UINT32 reg = pCdspReg0->cdspInpEdgeCtrl;
	return (reg & 0x01);
}

/**
* @brief	cdsp edge HPF matrix set
* @param	index [in]: 0 ~ 2
* @param	L0 [in]: Matrix L0
* @param	L1 [in]: Matrix L1
* @param	L2 [in]: Matrix L2
* @return	none
*/
void 
gpHalCdspSetEdgeFilter(
	UINT8 index,
	UINT8 L0,
	UINT8 L1,
	UINT8 L2
)
{
	UINT32 temp;

	/* vaild when R_INP_EDGE_CTRL.bit0 = 1 */	
	L0 &= 0xF;
	L1 &= 0xF;
	L2 &= 0xF;
	temp = ((UINT32)L2 << 8)|(L1 << 4)| L0;
	
	if(index == 0)
		pCdspReg0->cdspHPFLCoef0 = temp;
	else if(index == 1)
		pCdspReg0->cdspHPFLCoef1 = temp;
	else
		pCdspReg0->cdspHPFLCoef2 = temp;
}

void 
gpHalCdspGetEdgeFilter(
	UINT8 index,
	UINT8 *L0,
	UINT8 *L1,
	UINT8 *L2
)
{
	UINT32 reg;

	if(index == 0)
		reg = pCdspReg0->cdspHPFLCoef0;
	else if(index == 1)
		reg = pCdspReg0->cdspHPFLCoef1;
	else
		reg = pCdspReg0->cdspHPFLCoef2;

	*L0 = reg & 0xF;
	*L1 = (reg >> 4) & 0xF;
	*L2 &= (reg >> 8) & 0xF;
}

/**
* @brief	cdsp edge scale set
* @param	lhdiv [in]: L edge enhancement edge vale scale
* @param	lhtdiv [in]: L edge enhancement edge vale scale
* @param	lhcoring [in]: L core ring threshold
* @param	lhmode [in]: 1: default matrix, 0: enable paroramming matrix
* @return	none
*/
void 
gpHalCdspSetEdgeLCoring(
	UINT8 lhdiv, 
	UINT8 lhtdiv, 
	UINT8 lhcoring, 
	UINT8 lhmode
)
{
	UINT8 lh, lht;

	if(lhdiv <= 1) lh = 0;
	else if(lhdiv <= 2) lh = 1;
	else if(lhdiv <= 4) lh = 2;
	else if(lhdiv <= 8) lh = 3;
	else if(lhdiv <= 16) lh = 4;
	else if(lhdiv <= 32) lh = 5;
	else if(lhdiv <= 64) lh = 6;
	else if(lhdiv <= 128) lh = 7;
	else lh = 7;
	
	if(lhtdiv <= 1) lht = 0;
	else if(lhtdiv <= 2) lht = 1;
	else if(lhtdiv <= 4) lht = 2;
	else if(lhtdiv <= 8) lht = 3;
	else if(lhtdiv <= 16) lht= 4;
	else if(lhtdiv <= 32) lht = 5;
	else if(lhtdiv <= 64) lht = 6;
	else if(lhtdiv <= 128) lht = 7;
	else lht = 7;
	
	lhmode &= 0x1;
	pCdspReg0->cdspLHDivCtrl = ((UINT32)lhmode << 16)|((UINT32)lhcoring << 8)|(lht << 4)|lh; 
}

void 
gpHalCdspGetEdgeLCoring(
	UINT8 *lhdiv, 
	UINT8 *lhtdiv, 
	UINT8 *lhcoring, 
	UINT8 *lhmode
)
{
	UINT32 temp, reg = pCdspReg0->cdspLHDivCtrl;

	temp = reg & 0x0F;
	*lhdiv = (1 << temp);
	temp = (reg >> 4) & 0x0F;
	*lhtdiv = (1 << temp); 
	*lhcoring = (reg >> 8) & 0xFF;
	*lhmode = (reg >> 16) & 0x01;
}

/**
* @brief	cdsp edge amp set
* @param	ampga [in]: 0:1, 1:2, 2:3, 3:4
* @param	edgedomain [in]: 0:add edge on y value, 1:add edge on rgb value
* @return	none
*/
void 
gpHalCdspSetEdgeAmpga(
	UINT8 ampga
)
{
	UINT32 reg;
	
	ampga &= 0x0F;
	if(ampga <= 4)
		ampga -= 1;
	else 
		ampga = 3;

	reg = pCdspReg0->cdspInpEdgeCtrl;
	reg &= ~(0x03 << 6);
	reg |= (ampga << 6);
	pCdspReg0->cdspInpEdgeCtrl = reg;
}

UINT32 
gpHalCdspGetEdgeAmpga(
	void 
)
{
	UINT32 reg = pCdspReg0->cdspInpEdgeCtrl;
	return ((reg >> 6) & 0x03);
}

/**
* @brief	cdsp edge domain set
* @param	edgedomain [in]: 0:add edge on y value, 1:add edge on rgb value
* @return	none
*/
void
gpHalCdspSetEdgeDomain(
	UINT8 edgedomain
)
{
	if(edgedomain)
		pCdspReg0->cdspInpEdgeCtrl |= (1 << 2);
	else
		pCdspReg0->cdspInpEdgeCtrl &= ~(1 << 2);
}

UINT32
gpHalCdspGetEdgeDomain(
	void
)
{
	UINT32 reg = pCdspReg0->cdspInpEdgeCtrl;
	return ((reg >> 2) & 0x01);
}

/**
* @brief	cdsp edge Q threshold set
* @param	Qthr [in]: edge threshold 
* @return	none
*/
void
gpHalCdspSetEdgeQthr(
	UINT8 Qthr
)
{
	pCdspReg0->cdspInpQThr &= 0xFF00;
	pCdspReg0->cdspInpQThr |= Qthr;
}

UINT32
gpHalCdspGetEdgeQCnt(
	void
)
{
	return pCdspReg0->cdspInpQCnt;
}

/**
* @brief	cdsp edge lut table enable
* @param	eluten [in]: edge lut table enable
* @return	none
*/
void 
gpHalCdspSetEdgeLutTableEn(
	UINT8 eluten
)
{
	if(eluten)
		pCdspReg0->cdspInpEdgeCtrl |= 0x10;
	else
		pCdspReg0->cdspInpEdgeCtrl &= ~0x10;
}

UINT32 
gpHalCdspGetEdgeLutTableEn(
	void
)
{
	UINT32 reg = pCdspReg0->cdspInpEdgeCtrl;
	return ((reg >> 4) & 0x01);
}

/**
* @brief	cdsp edge lut table init
* @param	pLutEdgeTable [in]: table pointer
* @return	none
*/
void
gpHalCdspInitEdgeLut(
	UINT8 *pLutEdgeTable
)
{
	SINT32 i;

	pCdspReg0->cdspMacroCtrl = 0x44;
	for(i = 0; i < 256; i++) 
		pEdgeLutData->EdgeTable[i] = *pLutEdgeTable++;
		
	pCdspReg0->cdspMacroCtrl = 0x00;
}

/**
* @brief	cdsp set pre r and b clamp set
* @param	pre_rb_clamp [in]: clamp value
* @return	none
*/
void
gpHalCdspSetPreRBClamp(
	UINT8 pre_rb_clamp
)
{
	UINT32 reg;

	reg = pCdspReg0->cdspInpQThr;
	reg &= ~(0xFF << 8);
	reg |= ((UINT32)pre_rb_clamp << 8);	
	pCdspReg0->cdspInpQThr = reg;
}

UINT32
gpHalCdspGetPreRBClamp(
	void
)
{
	UINT32 reg = pCdspReg0->cdspInpQThr;
	return ((reg >> 8) & 0xFF);
}

/**
* @brief	cdsp color matrix enable
* @param	colcorren [in]: color matrix enable
* @return	none
*/
void 
gpHalCdspSetColorMatrixEn(
	UINT8 colcorren
)
{
	if(colcorren)
		pCdspReg0->cdspCcCof4 |= 0x800;
	else
		pCdspReg0->cdspCcCof4 &= ~0x800;
}

UINT32 
gpHalCdspGetColorMatrixEn(
	void
)
{
	UINT32 reg = pCdspReg0->cdspCcCof4;
	return ((reg >> 11) & 0x1);
}

/**
* @brief	cdsp color matrix enable
* @param	index [in]: 0 ~ 2
* @param	A1 [in]: Matrix A1
* @param	A2 [in]: Matrix A2
* @param	A3 [in]: Matrix A3
* @return	none
*/
void 
gpHalCdspSetColorMatrix(
	UINT8 index,
	UINT16 A1,
	UINT16 A2,
	UINT16 A3
)
{	
	UINT32 reg;
	
	A1 &= 0x3FF;
	A2 &= 0x3FF;
	A3 &= 0x3FF;
	if(index == 0)
	{
		pCdspReg0->cdspCcCof0 = ((UINT32)A2 << 12) | A1;
		reg = pCdspReg0->cdspCcCof1;
		reg &= ~(0x3FF);
		reg |= A3;
		pCdspReg0->cdspCcCof1 = reg;
	}
	else if(index == 1)
	{
		reg = pCdspReg0->cdspCcCof1; 
		reg &= ~(0x3FF << 12);
		reg |= ((UINT32)A1 << 12);
		pCdspReg0->cdspCcCof1 = reg;
		pCdspReg0->cdspCcCof2 = ((UINT32)A3 << 12)|A2;
	}
	else 
	{
		pCdspReg0->cdspCcCof3 = ((UINT32)A2 << 12)|A1;
		reg = pCdspReg0->cdspCcCof4; 
		reg &= ~0x3FF;
		reg |= A3;
		pCdspReg0->cdspCcCof4 = reg;
	}
}

void 
gpHalCdspGetColorMatrix(
	UINT8 index,
	UINT16 *A1,
	UINT16 *A2,
	UINT16 *A3
)
{	
	UINT32 reg;
	
	if(index == 0)
	{
		reg = pCdspReg0->cdspCcCof0;
		*A1 = reg & 0x3FF;
		*A2 = (reg >> 12) & 0x3FF;
		reg = pCdspReg0->cdspCcCof1;
		*A3 = reg & 0x3FF;		
	}
	else if(index == 1)
	{
		reg = pCdspReg0->cdspCcCof1;
		*A1 = (reg >> 12) & 0x3FF;
		reg = pCdspReg0->cdspCcCof2;
		*A2 = reg & 0x3FF;
		*A3 = (reg >> 12) & 0x3FF;
	}
	else 
	{
		reg = pCdspReg0->cdspCcCof3;
		*A1 = reg & 0x3FF;
		*A2 = (reg >> 12) & 0x3FF;
		reg = pCdspReg0->cdspCcCof4; 
		*A3 = reg & 0x3FF;
	}
}

/**
* @brief	cdsp a and b clamp set
* @param	rbclampen [in]: clamp enable
* @param	rbclamp [in]: clamp size set
* @return	none
*/
void 
gpHalCdspSetRBClamp(
	UINT8 rbclampen,
	UINT8 rbclamp
)
{
	rbclampen &= 0x1;
	pCdspReg0->cdspRbClampCtrl = (rbclampen << 8)|rbclamp;
}

void 
gpHalCdspGetRBClamp(
	UINT8 *rbclampen,
	UINT8 *rbclamp
)
{
	UINT32 reg = pCdspReg0->cdspRbClampCtrl;

	*rbclampen = (reg >> 8) & 0x01;
	 *rbclamp = reg & 0xFF;
}

/**
* @brief	cdsp uv division set
* @param	uvDiven [in]: un div function enable
* @return	none
*/
void
gpHalCdspSetUvDivideEn(
	UINT8 uvDiven
)
{
	if(uvDiven)
		pCdspReg0->cdspYuvCtrl |= 0x01;
	else
		pCdspReg0->cdspYuvCtrl &= ~0x01;
}

UINT32
gpHalCdspGetUvDivideEn(
	void
)
{
	UINT32 reg = pCdspReg0->cdspYuvCtrl;
	return (reg & 0x01);
}

/**
* @brief	cdsp uv division set
* @param	yfrcuvdiv1_8 [in]: y value T1
* @param	yfrcuvdiv2_8 [in]: y value T2
* @param	yfrcuvdiv3_8 [in]: y value T3
* @param	yfrcuvdiv4_8 [in]: y value T4
* @param	yfrcuvdiv5_8 [in]: y value T5
* @param	yfrcuvdiv6_8 [in]: y value T6
* @return	none
*/
void
gpHalCdspSetUvDivide(
	UINT8 yfrcuvdiv1_8,
	UINT8 yfrcuvdiv2_8,
	UINT8 yfrcuvdiv3_8,
	UINT8 yfrcuvdiv4_8,
	UINT8 yfrcuvdiv5_8,
	UINT8 yfrcuvdiv6_8
)
{
	pCdspReg0->cdspUvScaleCond0 = ((UINT32)yfrcuvdiv1_8 << 16)|((UINT32)yfrcuvdiv2_8 << 8)|yfrcuvdiv3_8;
	pCdspReg0->cdspUvScaleCond1 = ((UINT32)yfrcuvdiv4_8 << 16)|((UINT32)yfrcuvdiv5_8 << 8)|yfrcuvdiv6_8;
}

void
gpHalCdspGetUvDivide(
	UINT8 *yfrcuvdiv1_8,
	UINT8 *yfrcuvdiv2_8,
	UINT8 *yfrcuvdiv3_8,
	UINT8 *yfrcuvdiv4_8,
	UINT8 *yfrcuvdiv5_8,
	UINT8 *yfrcuvdiv6_8
)
{
	UINT32 reg = pCdspReg0->cdspUvScaleCond0;

	*yfrcuvdiv1_8 = (reg >> 16) & 0xFF;
	*yfrcuvdiv2_8 = (reg >> 8) & 0xFF;
	*yfrcuvdiv3_8 = reg & 0xFF;
	reg = pCdspReg0->cdspUvScaleCond1;
	*yfrcuvdiv4_8 = (reg >> 16) & 0xFF;
	*yfrcuvdiv5_8 = (reg >> 8) & 0xFF;
	*yfrcuvdiv6_8 = reg & 0xFF;
}

/**
* @brief	cdsp yuv mux path set
* @param	redoedge [in]: Set mux, 0:yuv path, 1:yuv path6
* @return	none
*/
void 
gpHalCdspSetMuxPath(
	UINT8 redoedge
)
{
	if(redoedge)
		pCdspReg1->cdspDo |= 0x02;		
	else
		pCdspReg1->cdspDo &= ~0x02;
}

/**
* @brief	cdsp yuv 444 insert enable
* @param	yuvinserten [in]: yuv 444 insert enable
* @return	none
*/
void 
gpHalCdspSetYuv444InsertEn(
	UINT8 yuvinserten
)
{
	if(yuvinserten)
		pCdspReg0->cdspYuvCtrl |= 0x08;
	else
		pCdspReg0->cdspYuvCtrl &= ~0x08;
}

UINT32 
gpHalCdspGetYuv444InsertEn(
	void
)
{
	UINT32 reg = pCdspReg0->cdspYuvCtrl;
	return ((reg >> 3) & 0x01);
}

/**
* @brief	cdsp yuv coring threshold value set
* @param	y_corval_coring [in]: y coring threshold value
* @param	u_corval_coring [in]: y coring threshold value
* @param	v_corval_coring [in]: y coring threshold value
* @return	none
*/
void 
gpHalCdspSetYuvCoring(
	UINT8 y_corval_coring, 
	UINT8 u_corval_coring, 
	UINT8 v_corval_coring
)
{
	pCdspReg0->cdspYuvCoringSet = (v_corval_coring << 16)|(u_corval_coring << 8)|y_corval_coring;
}

void 
gpHalCdspGetYuvCoring(
	UINT8 *y_corval_coring, 
	UINT8 *u_corval_coring, 
	UINT8 *v_corval_coring
)
{
	UINT32 reg = pCdspReg0->cdspYuvCoringSet;

	*y_corval_coring = reg & 0xFF;
	*u_corval_coring = (reg >> 8) & 0xFF;
	*v_corval_coring = (reg >> 16) & 0xFF;	
}

/**
* @brief	cdsp h average function
* @param	yuvhavgmiren [in]: mirror enable, bit0:left, bit1: right
* @param	ytype [in]: Y horizontal LPF type, 0:disable, 1:3tap, 2:5tap
* @param	utype [in]: U horizontal LPF type, 0:disable, 1:3tap, 2:5tap
* @param	vtype [in]: V horizontal LPF type, 0:disable, 1:3tap, 2:5tap
* @return	none
*/
void 
gpHalCdspSetYuvHAvg(
	UINT8 yuvhavgmiren, 
	UINT8 ytype, 
	UINT8 utype, 
	UINT8 vtype
)
{
	yuvhavgmiren &= 0x03;
	ytype &= 0x03;
	utype &= 0x03;
	vtype &= 0x03;
	pCdspReg0->cdspYuvAvgLpfType = ((UINT32)yuvhavgmiren << 8)|(vtype << 4)|(utype << 2)|ytype;
}

void 
gpHalCdspGetYuvHAvg(
	UINT8 *yuvhavgmiren, 
	UINT8 *ytype, 
	UINT8 *utype, 
	UINT8 *vtype
)
{
	UINT32 reg = pCdspReg0->cdspYuvAvgLpfType;
	
	*yuvhavgmiren = (reg >> 8) & 0x03;
	*ytype = reg & 0x03;
	*utype = (reg >> 2) & 0x03;
	*vtype = (reg >> 4) & 0x03;
}

/**
* @brief	cdsp yuv special mode set
* @param	yuvspecmode [in]: yuv special mode
* @return	none
*/
void 
gpHalCdspSetYuvSpecMode(
	UINT8 yuvspecmode
)
{
	UINT32 reg;
	if(yuvspecmode > 7) yuvspecmode = 0;
	reg = pCdspReg0->cdspYuvSpMode;
	reg &= ~0x07;
	reg |= yuvspecmode;
	
	/* reflected at vd update */
	reg |= (1 << 4);
	pCdspReg0->cdspYuvSpMode = reg;
	/* vdupdate */
	pCdspRegFront->cdspFrontCtrl0 |= (1<<4);
}

UINT32
gpHalCdspGetYuvSpecMode(
	void
)
{
	UINT32 reg = pCdspReg0->cdspYuvSpMode;
	return (reg & 0x07);
}

/**
* @brief	cdsp yuv special mode Binary threshold set
* @param	binarthr [in]: Binary threshold set
* @return	none
*/
void 
gpHalCdspSetYuvSpecModeBinThr(
	UINT8 binarthr
)
{
	/* vaild when special mode = 2, (binarize) */
	pCdspReg0->cdspYuvSpEffBThr = binarthr;	
}

UINT32 
gpHalCdspGetYuvSpecModeBinThr(
	void
)
{
	return pCdspReg0->cdspYuvSpEffBThr;	
}

/**
* @brief	cdsp yuv special mode brightness and contrast adjust enable
* @param	YbYcEn [in]: enable y brightness and contrast adjust 
* @return	none
*/
void 
gpHalCdspSetBriContEn(
	UINT8 YbYcEn
)
{
	/* vaild when yuv special mode = 0x3 */
	if(YbYcEn) 
		pCdspReg0->cdspYuvRange |= 0x10; 
	else
		pCdspReg0->cdspYuvRange &= ~0x10;	
}

UINT32 
gpHalCdspGetBriContEn(
	void
)
{
	UINT32 reg = pCdspReg0->cdspYuvRange;
	return ((reg >> 4) & 0x01);
}

/**
* @brief	cdsp yuv special mode offset set
* @param	y_offset [in]: Y offset set 
* @param	u_offset [in]: U offset set 
* @param	v_offset [in]: V offset set 
* @return	none
*/
void 
gpHalCdspSetYuvSPEffOffset(
	UINT8 y_offset, 
	UINT8 u_offset, 
	UINT8 v_offset
)
{
	pCdspReg0->cdspYuvSpEffOffset = ((UINT32)v_offset << 16)|((UINT32)u_offset << 8)|y_offset;
}

void 
gpHalCdspGetYuvSPEffOffset(
	UINT8 *y_offset, 
	UINT8 *u_offset, 
	UINT8 *v_offset
)
{
	UINT32 reg = pCdspReg0->cdspYuvSpEffOffset;

	*y_offset = reg & 0xFF;
	*u_offset = (reg >> 8) & 0xFF;
	*v_offset = (reg >> 16) & 0xFF;
}

/**
* @brief	cdsp yuv special mode offset set
* @param	y_scale [in]: Y scale set 
* @param	u_scale [in]: U scale set 
* @param	v_scale [in]: V scale set 
* @return	none
*/
void 
gpHalCdspSetYuvSPEffScale(
	UINT8 y_scale, 
	UINT8 u_scale, 
	UINT8 v_scale
)
{
	pCdspReg0->cdspYuvSpEffScale = ((UINT32)v_scale << 16)|((UINT32)u_scale << 8)|y_scale;
}

void 
gpHalCdspGetYuvSPEffScale(
	UINT8 *y_scale, 
	UINT8 *u_scale, 
	UINT8 *v_scale
)
{
	UINT32 reg = pCdspReg0->cdspYuvSpEffScale;

	*y_scale = reg & 0xFF;
	*u_scale = (reg >> 8) & 0xFF;
	*v_scale = (reg >> 16) & 0xFF;
}

/**
* @brief	cdsp yuv special mode hue set
* @param	u_huesindata [in]: sin data for hue rotate for u
* @param	u_huecosdata [in]: cos data for hue rotate for u
* @param	v_huesindata [in]: sin data for hue rotate for v
* @param	v_huecosdata [in]: cos data for hue rotate for v
* @return 	none
*/
void gpHalCdspSetYuvSPHue(
	UINT8 u_huesindata, 
	UINT8 u_huecosdata,	
	UINT8 v_huesindata, 
	UINT8 v_huecosdata
)
{
	pCdspReg0->cdspHutRotU = ((UINT32)u_huecosdata << 8)|u_huesindata;
	pCdspReg0->cdspHutRotV = ((UINT32)v_huecosdata << 8)|v_huesindata;
}

void gpHalCdspGetYuvSPHue(
	UINT8 *u_huesindata, 
	UINT8 *u_huecosdata,	
	UINT8 *v_huesindata, 
	UINT8 *v_huecosdata
)
{
	UINT32 reg = pCdspReg0->cdspHutRotU;

	*u_huesindata = reg & 0xFF;
	*u_huecosdata = (reg >> 8) & 0xFF;
	
	reg = pCdspReg0->cdspHutRotV;
	*v_huesindata = reg & 0xFF;
	*v_huecosdata = (reg >> 8) & 0xFF;
}

/**
* @brief	cdsp yuv h scale down enable
* @param	yuvhscale_en [in]: yuv h scale enable
* @param	yuvhscale_mode [in]: yuv h scale skip pixel mode 0: drop, 1:filter
* @return	none
*/
void 
gpHalCdspSetYuvHScaleEn(
	UINT8 yuvhscale_en, 
	UINT8 yuvhscale_mode
)
{
	UINT32 reg;
	
	yuvhscale_en &= 0x1;	
	yuvhscale_mode &= 0x1;	
	reg = pCdspReg0->cdspScaleDownCtrl; 
	reg &= ~(1 << 11 | 1 << 8);	
	reg |= ((UINT32)yuvhscale_en << 11)|((UINT32)yuvhscale_mode << 8);	
	pCdspReg0->cdspScaleDownCtrl = reg;

	/* reflected at next vaild vd edge */	
	pCdspReg0->cdspScaleFactorCtrl |= 0x01;
	/* vdupdate */
	pCdspRegFront->cdspFrontCtrl0 |= (1<<4);
}

void 
gpHalCdspGetYuvHScaleEn(
	UINT8 *yuvhscale_en, 
	UINT8 *yuvhscale_mode
)
{
	UINT32 reg = pCdspReg0->cdspScaleDownCtrl;
	
	*yuvhscale_en = (reg >> 11) & 0x01;
	*yuvhscale_mode = (reg >> 8) & 0x01;
}

/**
* @brief	cdsp yuv v scale down enable
* @param	vscale_en [in]: yuv v scale enable
* @param	vscale_mode [in]: yuv v scale skip pixel mode 0: drop, 1:filter
* @return	none
*/
void 
gpHalCdspSetYuvVScaleEn(
	UINT8 vscale_en, 
	UINT8 vscale_mode
)
{
	UINT32 reg;
	
	vscale_en &= 0x1;	
	vscale_mode &= 0x1;	
	reg = pCdspReg0->cdspScaleDownCtrl; 
	reg &= ~(1 << 7 | 1 << 4);	
	reg |= ((UINT32)vscale_en << 7)|((UINT32)vscale_mode << 4);				
	pCdspReg0->cdspScaleDownCtrl = reg;
	
	/* reflected at next vaild vd edge */	
	pCdspReg0->cdspScaleFactorCtrl |= 0x01;
	/* vdupdate */
	pCdspRegFront->cdspFrontCtrl0 |= (1<<4);
}

void 
gpHalCdspGetYuvVScaleEn(
	UINT8 *vscale_en, 
	UINT8 *vscale_mode
)
{
	UINT32 reg = pCdspReg0->cdspScaleDownCtrl;
	
	*vscale_en = (reg >> 7) & 0x01;
	*vscale_mode = (reg >> 4) & 0x01;
}

/**
* @brief	cdsp yuv h scale down set
* @param	hscaleaccinit [in]: yuv h scale accumation init vale set
* @param	yuvhscalefactor [in]: yuv h scale factor set
* @return	none
*/
void 
gpHalCdspSetYuvHScale(
	UINT16 hscaleaccinit, 
	UINT16 yuvhscalefactor
)
{
	pCdspReg0->cdspHScaleAccInit = hscaleaccinit;
	pCdspReg0->cdspHYuvScaleFactor = yuvhscalefactor;
}

/**
* @brief	cdsp yuv v scale down set
* @param	vscaleaccinit [in]: yuv v scale accumation init vale set
* @param	yuvvscalefactor [in]: yuv v scale factor set
* @return	none
*/
void 
gpHalCdspSetYuvVScale(
	UINT16 vscaleaccinit, 
	UINT16 yuvvscalefactor
)
{
	pCdspReg0->cdspVScaleAccInit = vscaleaccinit;
	pCdspReg0->cdspVYuvScaleFactor = yuvvscalefactor;
}

/**
* @brief	cdsp uv suppression enable
* @param	suppressen [in]: uv suppression enable, effective when yuv data input.
* @return	none
*/
void 
gpHalCdspSetUvSupprEn(
	UINT8 suppressen
)
{
	if(suppressen)
		pCdspReg0->cdspYuvCtrl |= 0x10;
	else
		pCdspReg0->cdspYuvCtrl &= ~0x10;
}

UINT32 
gpHalCdspGetUvSupprEn(
	void
)
{
	UINT32 reg = pCdspReg0->cdspYuvCtrl;
	return ((reg >> 4) & 0x01);
}

/**
* @brief	cdsp uv suppression set
* @param	yuvsupmirvsel [in]: 1:cnt3eq2, 0:cnt3eq1
* @param	fstextsolen [in]: enable first sol when extened 2 line
* @param	yuvsupmiren [in]: suppression enable, bit0:top, bit1:down, bit2:left, bit3:right
* @return	none
*/
void 
gpHalCdspSetUvSuppr(
	UINT8 yuvsupmirvsel, 
	UINT8 fstextsolen, 
	UINT8 yuvsupmiren
)
{
	UINT32 reg;
	
	reg = pCdspReg0->cdspYuvRange; 
	reg &= ~(1 << 13 | 1 << 12 | 0xF << 8);
	yuvsupmiren &= 0x0F;
	yuvsupmirvsel &= 0x1;
	fstextsolen &= 0x1;
	reg |= ((UINT16)fstextsolen << 13)|((UINT16)yuvsupmirvsel << 12)|(yuvsupmiren << 8);
	pCdspReg0->cdspYuvRange = reg;
}

void 
gpHalCdspGetUvSuppr(
	UINT8 *yuvsupmirvsel, 
	UINT8 *fstextsolen, 
	UINT8 *yuvsupmiren
)
{
	UINT32 reg = pCdspReg0->cdspYuvRange;

	*yuvsupmirvsel = (reg >> 12) & 0x01;
	*fstextsolen = (reg >> 13) & 0x01;
	*yuvsupmiren = (reg >> 8) & 0x0F;
}

/**
* @brief	cdsp y denoise enable
* @param	denoisen [in]: y denoise enable
* @return	none
*/
void 
gpHalCdspSetYDenoiseEn(
	UINT8 denoisen
)
{
	if(denoisen)
		pCdspReg0->cdspYuvCtrl |= 0x20;
	else
		pCdspReg0->cdspYuvCtrl &= ~0x20;
}

UINT32
gpHalCdspGetYDenoiseEn(
	void
)
{
	UINT32 reg = pCdspReg0->cdspYuvCtrl;
	return ((reg >> 5) & 0x01);
}

/**
* @brief	cdsp y denoise set
* @param	denoisethrl [in]: y denoise low threshold
* @param	denoisethrwth [in]: y denoise bandwidth set
* @param	yhtdiv [in]: y denoise divider
* @return	none
*/
void 
gpHalCdspSetYDenoise(
	UINT8 denoisethrl, 
	UINT8 denoisethrwth, 
	UINT8 yhtdiv
)
{
	UINT8 yht, thrwth;

	if(denoisethrwth <= 1) thrwth = 0;
	else if(denoisethrwth <= 2) thrwth = 1;
	else if(denoisethrwth <= 4) thrwth = 2;
	else if(denoisethrwth <= 8) thrwth = 3;
	else if(denoisethrwth <= 16) thrwth = 4;
	else if(denoisethrwth <= 32) thrwth = 5;
	else if(denoisethrwth <= 64) thrwth = 6;
	else if(denoisethrwth <= 128) thrwth = 7;
	else thrwth = 7;

	if(yhtdiv <= 1) yht = 0;
	else if(yhtdiv <= 2) yht = 1;
	else if(yhtdiv <= 4) yht = 2;
	else if(yhtdiv <= 8) yht = 3;
	else if(yhtdiv <= 16) yht = 4;
	else if(yhtdiv <= 32) yht = 5;
	else if(yhtdiv <= 64) yht = 6;
	else if(yhtdiv <= 128) yht = 7;
	else yht = 7;

	pCdspReg0->cdspDenoiseSet = ((UINT32)yht << 12)|((UINT32)thrwth << 8)|denoisethrl;
}

void 
gpHalCdspGetYDenoise(
	UINT8 *denoisethrl, 
	UINT8 *denoisethrwth, 
	UINT8 *yhtdiv
)
{
	UINT32 temp, reg = pCdspReg0->cdspDenoiseSet;

	*denoisethrl = reg & 0xFF;
	temp = (reg >> 8) & 0x0F;
	*denoisethrwth = (1 << temp);
	temp = (reg >> 12) & 0x0F;
	*yhtdiv = (1 << temp);
}

/**
* @brief	cdsp y LPF enable
* @param	lowyen [in]: y LPF enable
* @return	none
*/
void 
gpHalCdspSetYLPFEn(
	UINT8 lowyen
)
{
	if(lowyen)
		pCdspReg0->cdspYuvCtrl |= 0x40;
	else
		pCdspReg0->cdspYuvCtrl &= ~0x40;
}

UINT32
gpHalCdspGetYLPFEn(
	void
)
{
	UINT32 reg = pCdspReg0->cdspYuvCtrl;
	return ((reg >> 6) & 0x01);
}

/**
* @brief	cdsp wb gain2 enable
* @param	wbgain2en [in]: enable 
* @return	none
*/
void 
gpHalCdspSetWbGain2En(
	UINT8 wbgain2en
)
{
	if(wbgain2en)
		pCdspReg3a->cdspAwbWinBgGain2 |= 1 << 11;
	else
		pCdspReg3a->cdspAwbWinBgGain2 &= ~(1 << 11);
}

UINT32 
gpHalCdspGetWbGain2En(
	void
)
{
	UINT32 reg = pCdspReg3a->cdspAwbWinBgGain2;
	return ((reg >> 11) & 0x01);
}

/**
* @brief	cdsp wb gain2 set
* @param	rgain2 [in]: R gain
* @param	ggain2 [in]: G gain
* @param	bgain2 [in]: B gain
* @return	none
*/
void
gpHalCdspSetWbGain2(
	UINT16 rgain2,
	UINT16 ggain2,
	UINT16 bgain2
)
{
	UINT32 reg;
	
	rgain2 &= 0x1FF;
	ggain2 &= 0x1FF;
	bgain2 &= 0x1FF;
	pCdspReg3a->cdspAwbWinRgGain2 = rgain2|((UINT32)ggain2 << 12);	

	reg = pCdspReg3a->cdspAwbWinBgGain2;
	reg &= ~0x1FF;
	reg |= bgain2;
	pCdspReg3a->cdspAwbWinBgGain2 = reg; 
}

void
gpHalCdspGetWbGain2(
	UINT16 *rgain2,
	UINT16 *ggain2,
	UINT16 *bgain2
)
{
	UINT32 reg = pCdspReg3a->cdspAwbWinRgGain2;
	
	*rgain2 = reg & 0x1FF;
	*ggain2 = (reg >> 12) & 0x1FF;
	reg = pCdspReg3a->cdspAwbWinBgGain2;
	*bgain2 = reg & 0x1FF;
}

/**
* @brief	cdsp auto focus enable
* @param	af_en [in]: af enable
* @param	af_win_hold [in]: af hold
* @return	none
*/
void 
gpHalCdspSetAFEn(
	UINT8 af_en, 
	UINT8 af_win_hold
)
{
	UINT32 reg;

	reg = pCdspReg3a->cdspAfWinCtrl; 
	if(af_win_hold)
		reg |= (1 << 16);
	else
		reg &= ~(1 << 16);
	
	if(af_en)
		reg |= (1 << 19);
	else
		reg &= ~(1 << 19);
	
	pCdspReg3a->cdspAfWinCtrl = reg;
}

void 
gpHalCdspGetAFEn(
	UINT8 *af_en, 
	UINT8 *af_win_hold
)
{
	UINT32 reg = pCdspReg3a->cdspAfWinCtrl;

	*af_en = (reg >> 19) & 0x01;
	*af_win_hold = (reg >> 16) & 0x01;
}

/**
* @brief	cdsp auto focus window 1 set
* @param	hoffset [in]: h offset
* @param	voffset [in]: v offset
* @param	hsize [in]: h size
* @param	vsize [in]: v size
* @return	none
*/
void 
gpHalCdspSetAfWin1(
	UINT16 hoffset, 
	UINT16 voffset, 
	UINT16 hsize, 
	UINT16 vsize
)
{
	hoffset &= 0xFFF;
	voffset &= 0xFFF;
	hsize &= 0xFFF;
	vsize &= 0xFFF;
	pCdspReg3a->cdspAfWin1HVOffset= ((UINT32)voffset << 12)|hoffset; 
	pCdspReg3a->cdspAfWin1HVSize= ((UINT32)vsize << 12)|hsize;
}

void 
gpHalCdspGetAfWin1(
	UINT16 *hoffset, 
	UINT16 *voffset, 
	UINT16 *hsize, 
	UINT16 *vsize
)
{
	UINT32 reg = pCdspReg3a->cdspAfWin1HVOffset;

	*hoffset = reg & 0xFFF;
	*voffset = (reg >> 12) & 0xFFF;
	reg = pCdspReg3a->cdspAfWin1HVSize;
	*hsize = reg & 0xFFF;
	*vsize = (reg >> 12) & 0xFFF;
}

/**
* @brief	cdsp auto focus window 2 set
* @param	hoffset [in]: h offset
* @param	voffset [in]: v offset
* @param	hsize [in]: h size, 256, 512, 1024, 64, 2048
* @param	vsize [in]: v size, 256, 512, 1024, 64, 2048
* @return	none
*/
void 
gpHalCdspSetAfWin2(
	UINT16 hoffset, 
	UINT16 voffset, 
	UINT16 hsize, 
	UINT16 vsize
)
{
	UINT8 h_size, v_size;
	UINT32 reg;
	
	hoffset = (hoffset >> 2) & 0x3FF;	/* offset unit is 4 pixel */
	voffset = (voffset >> 2) & 0x3FF;
	pCdspReg3a->cdspAfWin2HVOffset = ((UINT32)voffset << 12)|hoffset; 
	
	if(hsize <= 64) h_size = 3;
	else if(hsize <= 256) h_size = 0;
	else if(hsize <= 512) h_size = 1;
	else if(hsize <= 1024) h_size = 2;
	else if(hsize <= 2048) h_size = 4;
	else h_size = 4;

	if(vsize <= 64) v_size = 3;
	else if(vsize <= 256) v_size = 0;
	else if(vsize <= 512) v_size = 1;
	else if(vsize <= 1024) v_size = 2;
	else if(vsize <= 2048) v_size = 4;
	else v_size = 4;

	reg = pCdspReg3a->cdspAfWinCtrl;
	reg &= ~(0x07 << 4 | 0x07);
	reg |= (v_size << 4)|h_size;
	pCdspReg3a->cdspAfWinCtrl = reg;
}

void 
gpHalCdspGetAfWin2(
	UINT16 *hoffset, 
	UINT16 *voffset, 
	UINT16 *hsize, 
	UINT16 *vsize
)
{
	UINT32 temp, reg = pCdspReg3a->cdspAfWin2HVOffset;
	
	*hoffset = (reg & 0x3FF) << 2;
	*voffset = ((reg >> 12) & 0x3FF) << 2;;

	reg = pCdspReg3a->cdspAfWinCtrl;
	temp = reg & 0x07;
	if(temp == 3) *hsize = 64;
	else if(temp == 0) *hsize = 256;
	else if(temp == 1) *hsize = 512;
	else if(temp == 2) *hsize = 1024;
	else if(temp == 4) *hsize = 2048;
	else *hsize = 2048;

	temp = (reg >> 4) & 0x07;
	if(temp == 3) *vsize = 64;
	else if(temp == 0) *vsize = 256;
	else if(temp == 1) *vsize = 512;
	else if(temp == 2) *vsize = 1024;
	else if(temp == 4) *vsize = 2048;
	else *vsize = 2048;
}

/**
* @brief	cdsp auto focus window 3 set
* @param	hoffset [in]: h offset
* @param	voffset [in]: v offset
* @param	hsize [in]: h size, 256, 512, 1024, 64, 2048
* @param	vsize [in]: v size, 256, 512, 1024, 64, 2048
* @return	none
*/
void 
gpHalCdspSetAfWin3(
	UINT16 hoffset, 
	UINT16 voffset, 
	UINT16 hsize, 
	UINT16 vsize
)
{
	UINT8 h_size, v_size;
	UINT32 reg;
	
	hoffset = (hoffset >> 2) & 0x3FF;	/* offset unit is 4 pixel */
	voffset = (voffset >> 2) & 0x3FF;
	pCdspReg3a->cdspAfWin3HVOffset = ((UINT32)voffset << 12)|hoffset; 
	
	if(hsize <= 64) h_size = 3;
	else if(hsize <= 256) h_size = 0;
	else if(hsize <= 512) h_size = 1;
	else if(hsize <= 1024) h_size = 2;
	else if(hsize <= 2048) h_size = 4;
	else h_size = 4;

	if(vsize <= 64) v_size = 3;
	else if(vsize <= 256) v_size = 0;
	else if(vsize <= 512) v_size = 1;
	else if(vsize <= 1024) v_size = 2;
	else if(vsize <= 2048) v_size = 4;
	else v_size = 4;
	
	reg = pCdspReg3a->cdspAfWinCtrl;
	reg &= ~(0x07 << 12 | 0x07 << 8);
	reg |= ((UINT32)v_size << 12)|((UINT32)h_size << 8);
	pCdspReg3a->cdspAfWinCtrl = reg;
}	

void 
gpHalCdspGetAfWin3(
	UINT16 *hoffset, 
	UINT16 *voffset, 
	UINT16 *hsize, 
	UINT16 *vsize
)
{
	UINT32 temp, reg = pCdspReg3a->cdspAfWin3HVOffset;
	
	*hoffset = (reg & 0x3FF) << 2;
	*voffset = ((reg >> 12) & 0x3FF) << 2;;

	reg = pCdspReg3a->cdspAfWinCtrl;
	temp = (reg >> 8) & 0x07;
	if(temp == 3) *hsize = 64;
	else if(temp == 0) *hsize = 256;
	else if(temp == 1) *hsize = 512;
	else if(temp == 2) *hsize = 1024;
	else if(temp == 4) *hsize = 2048;
	else *hsize = 2048;

	temp = (reg >> 12) & 0x07;
	if(temp == 3) *vsize = 64;
	else if(temp == 0) *vsize = 256;
	else if(temp == 1) *vsize = 512;
	else if(temp == 2) *vsize = 1024;
	else if(temp == 4) *vsize = 2048;
	else *vsize = 2048;
}

/**
* @brief	cdsp auto white balance enable
* @param	awb_en [in]: awb enable
* @param	awb_win_hold [in]: awb hold
* @return	none
*/
void 
gpHalCdspSetAWBEn(
	UINT8 awb_en, 
	UINT8 awb_win_hold
)
{
	UINT32 reg;
	
	reg = pCdspReg3a->cdspAeAwbWinCtrl;
	if(awb_win_hold)
		reg |= (1 << 2); 
	else
		reg &= ~(1 << 2);
	
	if(awb_en)
		reg |= (1 << 11);
	else
		reg &= ~(1 << 11);
	
	pCdspReg3a->cdspAeAwbWinCtrl = reg;
}

void 
gpHalCdspGetAWBEn(
	UINT8 *awb_en, 
	UINT8 *awb_win_hold
)
{
	UINT32 reg = pCdspReg3a->cdspAeAwbWinCtrl;

	*awb_en = (reg >> 11) & 0x01;
	*awb_win_hold = (reg >> 2) & 0x01;
}

/**
* @brief	cdsp auto white balance set
* @param	awbclamp_en [in]: awb special window clamp enable.
* @param	sindata [in]: sin data for AWB
* @param	cosdata [in]: cos data for AWB
* @param	awbwinthr [in]: AWB winwow accumulation threshold
* @return	none
*/
void 
gpHalCdspSetAWB(
	UINT8 awbclamp_en, 
	UINT8 sindata, 
	UINT8 cosdata, 
	UINT8 awbwinthr
)
{
	pCdspReg3a->cdspAwbWinCtrl = ((UINT32)awbclamp_en << 24)|((UINT32)awbwinthr << 16)|
								((UINT32)cosdata << 8)|sindata;
}

void 
gpHalCdspGetAWB(
	UINT8 *awbclamp_en, 
	UINT8 *sindata, 
	UINT8 *cosdata, 
	UINT8 *awbwinthr
)
{
	UINT32 reg = pCdspReg3a->cdspAwbWinCtrl;

	*awbclamp_en = reg >> 24;
	*sindata = reg & 0xFF;
	*cosdata = (reg >> 8) & 0xFF;
	*awbwinthr = (reg >> 16) & 0xFF;
}

/**
* @brief	cdsp awb special windows Y threshold set
* @param	Ythr0 [in]: AWB Y threshold0
* @param	Ythr1 [in]: AWB Y threshold1
* @param	Ythr2 [in]: AWB Y threshold2
* @param	Ythr3 [in]: AWB Y threshold3
* @return	none
*/
void 
gpHalCdspSetAwbYThr(
	UINT8 Ythr0,
	UINT8 Ythr1,
	UINT8 Ythr2,
	UINT8 Ythr3
)
{
	UINT32 temp;
	temp = ((UINT32)Ythr3 << 24)|((UINT32)Ythr2 << 16)|((UINT32)Ythr1 << 8)|Ythr0;
	pCdspReg3a->cdspAwbSpWinYThr = temp;
}

void 
gpHalCdspGetAwbYThr(
	UINT8 *Ythr0,
	UINT8 *Ythr1,
	UINT8 *Ythr2,
	UINT8 *Ythr3
)
{
	UINT32 reg = pCdspReg3a->cdspAwbSpWinYThr;

	*Ythr0 = reg & 0xFF;
	*Ythr1 = (reg >> 8) & 0xFF;
	*Ythr2 = (reg >> 16) & 0xFF;
	*Ythr3 = (reg >> 24) & 0xFF;
}

/**
* @brief	cdsp awb special windows uv threshold set
* @param	UL1N [in]: AWB U low threshold
* @param	UL1P [in]: AWB U high threshold
* @param	VL1N [in]: AWB V low threshold
* @param	VL1P [in]: AWB V high threshold
* @return	none
*/
void 
gpHalCdspSetAwbUVThr(
	UINT8 section, 
	UINT16 UL1N, 
	UINT16 UL1P,
	UINT16 VL1N,
	UINT16 VL1P
)
{
	UINT32 temp;

	UL1P >>= 1;	/* uint 2 */
	UL1N >>= 1;
	VL1P >>= 1;
	VL1N >>= 1;

	temp = ((UINT32)VL1P << 24)|((UINT32)VL1N << 16)|((UINT32)UL1P << 8)|UL1N;
	if(section == 1)
		pCdspReg3a->cdspAwbSpWinUvThr1 = temp;
	else if(section == 2)
		pCdspReg3a->cdspAwbSpWinUvThr2 = temp;
	else 
		pCdspReg3a->cdspAwbSpWinUvThr3 = temp;
}

void 
gpHalCdspGetAwbUVThr(
	UINT8 section, 
	UINT16 *UL1N, 
	UINT16 *UL1P,
	UINT16 *VL1N,
	UINT16 *VL1P
)
{
	UINT32 reg;

	if(section == 1)
		reg = pCdspReg3a->cdspAwbSpWinUvThr1;
	else if(section == 2)
		reg = pCdspReg3a->cdspAwbSpWinUvThr2;
	else 
		reg = pCdspReg3a->cdspAwbSpWinUvThr3;

	*UL1N = (reg & 0xFF) << 1;
	*UL1P = ((reg >> 8) & 0xFF) << 1;
	*VL1N = ((reg >> 16) & 0xFF) << 1;
	*VL1P = ((reg >> 24) & 0xFF) << 1;
}

/**
* @brief	cdsp ae/awb source set
* @param	raw_en [in]: ae/awb windows set, 0:from poswb, 1:form awb line ctrl
* @return	none
*/
void 
gpHalCdspSetAeAwbSrc(
	UINT8 raw_en 
)
{
	if(raw_en)
		pCdspReg3a->cdspAeAwbWinCtrl |= 1 << 12;
	else
		pCdspReg3a->cdspAeAwbWinCtrl &= ~(1 << 12);
}

UINT32 
gpHalCdspGetAeAwbSrc(
	void 
)
{
	UINT32 reg = pCdspReg3a->cdspAeAwbWinCtrl;
	return ((reg >> 12) & 0x01);
}

/**
* @brief	cdsp ae/awb subsample set
* @param	subample [in]: 0:disable, 2:1/2, 4:1/4 subsample
* @return	none
*/
void 
gpHalCdspSetAeAwbSubSample(
	UINT8 subample
)
{
	UINT8 sample;
	UINT32 reg;
	
	if(subample == 0) sample = 0;
	else if(subample <= 2) sample = 1;
	else if(subample <= 4) sample = 2;
	else if(subample <= 8) sample = 3;
	else sample = 0;

	reg = pCdspReg3a->cdspAeAwbWinCtrl;
	reg &= ~(3 << 13);
	reg |= (UINT32)sample << 13;  
	pCdspReg3a->cdspAeAwbWinCtrl = reg;
}

UINT32 
gpHalCdspGetAeAwbSubSample(
	void
)
{
	UINT32 temp, reg = pCdspReg3a->cdspAeAwbWinCtrl;
	temp = (reg >> 13) & 0x03;
	return (1 << temp);
}

/**
* @brief	cdsp auto expore enable
* @param	ae_en [in]: ae enable
* @param	ae_win_hold [in]: ae hold
* @return	none
*/
void 
gpHalCdspSetAEEn(
	UINT8 ae_en, 
	UINT8 ae_win_hold
)
{
	UINT32 reg;

	reg = pCdspReg3a->cdspAeAwbWinCtrl;
	if(ae_win_hold)
		reg |= 0x1; 
	else
		reg &= ~(0x1);

	if(ae_en)
		reg |= (1 << 8);
	else
		reg &= ~(1 << 8);	

	/* reflected at next vaild vd edge */ 
	reg |= (1 << 4);
	pCdspReg3a->cdspAeAwbWinCtrl = reg;
	/* vdupdate */
	pCdspRegFront->cdspFrontCtrl0 |= (1<<4);
}

void 
gpHalCdspGetAEEn(
	UINT8 *ae_en, 
	UINT8 *ae_win_hold
)
{
	UINT32 reg = pCdspReg3a->cdspAeAwbWinCtrl;

	*ae_en = (reg >> 8) & 0x01;
	*ae_win_hold = reg & 0x01;
}

/**
* @brief	cdsp auto expore set
* @param	phaccfactor [in]: pseudo h window size for ae windows
* @param	pvaccfactor [in]: pseudo v window size for ae windows
* @return	none
*/
void 
gpHalCdspSetAEWin(
	UINT8 phaccfactor, 
	UINT8 pvaccfactor
)
{
	UINT8 h_factor, v_factor;
	
	if(phaccfactor <= 4) h_factor = 0;
	else if(phaccfactor <= 8) h_factor = 1;
	else if(phaccfactor <= 16) h_factor = 2;
	else if(phaccfactor <= 32) h_factor = 3;
	else if(phaccfactor <= 64) h_factor = 4;
	else if(phaccfactor <= 128) h_factor = 5;
	else h_factor = 5;

	if(pvaccfactor <= 4) v_factor = 0;
	else if(pvaccfactor <= 8) v_factor = 1;
	else if(pvaccfactor <= 16) v_factor = 2;
	else if(pvaccfactor <= 32) v_factor = 3;
	else if(pvaccfactor <= 64) v_factor = 4;
	else if(pvaccfactor <= 128) v_factor = 5;
	else v_factor = 5;
		
	pCdspReg3a->cdspAeWinSize = (v_factor << 4)|h_factor;
}

void 
gpHalCdspGetAEWin(
	UINT8 *phaccfactor, 
	UINT8 *pvaccfactor
)
{
	UINT32 temp, reg = pCdspReg3a->cdspAeWinSize;

	temp = reg & 0x0F;
	*phaccfactor = (1 << temp);
	temp = (reg >> 4) & 0x0F; 
	*pvaccfactor = (1 << temp);
}

/**
* @brief	cdsp ae buffer address set
* @param	winaddra [in]: AE a buffer address set 
* @param	winaddrb [in]: AE b buffer address set
* @return	none
*/
void 
gpHalCdspSetAEBuffAddr(
	UINT32 winaddra, 
	UINT32 winaddrb
)
{
	winaddra >>= 1;
	winaddrb >>= 1;
	pCdspReg3a->cdspAeWinBufAddrA = winaddra;
	pCdspReg3a->cdspAeWinBufAddrB = winaddrb;
}

/**
* @brief	cdsp ae buffer address sett 
* @return	0: ae a buffer ready, 1: ae b buffer ready
*/
UINT32 
gpHalCdspGetAEActBuff(
	void
)
{
	if(pCdspReg3a->cdspAeAwbWinCtrl & 0x8000)
		return 1;	/* buffer b active */
	else
		return 0;	/* buffer a active */
}

/**
* @brief	cdsp rgb window set
* @param	hwdoffset [in]: rgb window h offset
* @param	vwdoffset [in]: rgb window v offset
* @param	hwdsize [in]: rgb window h size
* @param	vwdsize [in]: rgb window v size
* @return	none
*/
void 
gpHalCdspSetRGBWin(
	UINT16 hwdoffset, 
	UINT16 vwdoffset, 
	UINT16 hwdsize, 
	UINT16 vwdsize
)
{
	hwdoffset &= 0x1FFF;
	hwdsize &= 0x3FF;
	vwdoffset &= 0x1FFF;
	vwdsize &= 0x3FF;
	pCdspReg3a->cdspRgbWinHCtrl = ((UINT32)hwdoffset << 12)|hwdsize;
	pCdspReg3a->cdspRgbWinVCtrl = ((UINT32)vwdoffset << 12)|vwdsize;
}

void 
gpHalCdspGetRGBWin(
	UINT16 *hwdoffset, 
	UINT16 *vwdoffset, 
	UINT16 *hwdsize, 
	UINT16 *vwdsize
)
{
	UINT32 reg = pCdspReg3a->cdspRgbWinHCtrl;

	*hwdsize = reg & 0x3FF;
	*hwdoffset = (reg >> 12) & 0x1FFF;
	reg = pCdspReg3a->cdspRgbWinVCtrl;
	*vwdsize = reg & 0x3FF;
	*vwdoffset = (reg >> 12) & 0x1FFF;
}

/**
* @brief	cdsp ae/af test windows enable
* @param	AeWinTest [in]: ae test window enable
* @param	AfWinTest [in]: af test window enable
* @return	none
*/
void 
gpHalCdspSet3ATestWinEn(
	UINT8 AeWinTest, 
	UINT8 AfWinTest
)
{
	AfWinTest &= 0x1;
	AeWinTest &= 0x1;
	pCdspReg3a->cdspAefWinTest = (AeWinTest << 3)|AfWinTest;
}

void 
gpHalCdspGet3ATestWinEn(
	UINT8 *AeWinTest, 
	UINT8 *AfWinTest
)
{
	UINT32 reg = pCdspReg3a->cdspAefWinTest;
	*AeWinTest = (reg >> 3) & 0x01;
	*AfWinTest = reg & 0x01;
}

/**
* @brief	cdsp histgm enable
* @param	his_en [in]: histgm enable
* @param	his_hold_en [in]: histgm hold
* @return	none
*/
void 
gpHalCdspSetHistgmEn(
	UINT8 his_en, 
	UINT8 his_hold_en
)
{
	if(his_hold_en)
		pCdspReg3a->cdspAeAwbWinCtrl |= 1 << 1;	
	else
		pCdspReg3a->cdspAeAwbWinCtrl &= ~(1 << 1);

	if(his_en)
		pCdspReg3a->cdspHistgmCtrl |= (1 << 16);
	else
		pCdspReg3a->cdspHistgmCtrl &= ~(1 << 16);
}

void 
gpHalCdspGetHistgmEn(
	UINT8 *his_en, 
	UINT8 *his_hold_en
)
{
	UINT32 reg = pCdspReg3a->cdspHistgmCtrl;

	*his_en = reg >> 16;
	reg = pCdspReg3a->cdspAeAwbWinCtrl;
	*his_hold_en = ((reg >> 1) & 0x01);	
}

/**
* @brief	cdsp histgm statistics set
* @param	hislowthr [in]: histgm low threshold set
* @param	hishighthr [in]: histgm high threshold set
* @return	none
*/
void 
gpHalCdspSetHistgm(
	UINT8 hislowthr, 
	UINT8 hishighthr
)
{
	UINT32 reg;
	
	reg = pCdspReg3a->cdspHistgmCtrl; 
	reg &= ~0xFFFF;
	reg |= ((UINT32)hishighthr << 8)|hislowthr;
	pCdspReg3a->cdspHistgmCtrl = reg;
}

void 
gpHalCdspGetHistgm(
	UINT8 *hislowthr, 
	UINT8 *hishighthr
)
{
	UINT32 reg = pCdspReg3a->cdspHistgmCtrl;

	*hislowthr = reg & 0xFF;
	*hishighthr = (reg >> 8) & 0xFF;
}

void 
gpHalCdspGetHistgmCount(
	UINT32 *hislowcnt, 
	UINT32 *hishighcnt
)
{
	*hislowcnt = pCdspReg3a->cdspHistgmLCnt;
	*hishighcnt = pCdspReg3a->cdspHistgmHCnt;
}

/**
* @brief	cdsp get awb cnt
* @param	section [in]: index = 1, 2, 3
* @param	sumcnt [out]: count get
* @return	SUCCESS/ERROR
*/
SINT32
gpHalCdspGetAwbSumCnt(
	UINT8 section,
	UINT32 *sumcnt 
)
{
	if(section == 1)
		*sumcnt = pCdspReg3a->cdspSumCnt1;
	else if(section == 2)
		*sumcnt = pCdspReg3a->cdspSumCnt2;
	else if(section == 3)
		*sumcnt = pCdspReg3a->cdspSumCnt3;
	else
		return -1;
	return 0;
}

/**
* @brief	cdsp get awb g
* @param	section [in]: index = 1, 2, 3
* @param	sumgl [out]: sum g1 low 
* @param	sumgl [out]: sum g1 high 
* @return	SUCCESS/ERROR
*/
SINT32
gpHalCdspGetAwbSumG(
	UINT8 section,
	UINT32 *sumgl,
	UINT32 *sumgh
)
{
	if(section == 1)
	{
		*sumgl = pCdspReg3a->cdspSumG1L;
		*sumgh = pCdspReg3a->cdspSumG1H;
	}
	else if(section == 2)
	{
		*sumgl = pCdspReg3a->cdspSumG2L;
		*sumgh = pCdspReg3a->cdspSumG2H;
	}
	else if(section == 3)
	{
		*sumgl = pCdspReg3a->cdspSumG2L;
		*sumgh = pCdspReg3a->cdspSumG2H;
	}
	else
		return -1;
	return 0;
}

/**
* @brief	cdsp get awb rg
* @param	section [in]: section = 1, 2, 3
* @param	sumrgl [out]: sum rg low 
* @param	sumrgl [out]: sum rg high 
* @return	SUCCESS/ERROR
*/
SINT32
gpHalCdspGetAwbSumRG(
	UINT8 section,
	SINT32 *sumrgl,
	SINT32 *sumrgh
)
{
	if(section == 1)
	{
		*sumrgl = pCdspReg3a->cdspSumRg1L;
		*sumrgh = pCdspReg3a->cdspSumRg1H;
	}
	else if(section == 2)
	{
		*sumrgl = pCdspReg3a->cdspSumRg2L;
		*sumrgh = pCdspReg3a->cdspSumRg2H;
	}
	else if(section == 3)
	{
		*sumrgl = pCdspReg3a->cdspSumRg3L;
		*sumrgh = pCdspReg3a->cdspSumRg3H;
	}
	else
		return -1;
	return 0;
}

/**
* @brief	cdsp get awb bg
* @param	section [in]: section = 1, 2, 3
* @param	sumbgl [out]: sum bg low 
* @param	sumbgl [out]: sum bg high 
* @return	SUCCESS/ERROR
*/
SINT32
gpHalCdspGetAwbSumBG(
	UINT8 section,
	SINT32 *sumbgl,
	SINT32 *sumbgh
)
{
	if(section == 1)
	{
		*sumbgl = pCdspReg3a->cdspSumBg1L;
		*sumbgh = pCdspReg3a->cdspSumBg1H;
	}
	else if(section == 2)
	{
		*sumbgl = pCdspReg3a->cdspSumBg2L;
		*sumbgh = pCdspReg3a->cdspSumBg2H;
	}
	else if(section == 3)
	{
		*sumbgl = pCdspReg3a->cdspSumBg3L;
		*sumbgh = pCdspReg3a->cdspSumBg3H;
	}
	else
		return -1;
	return 0;
}

/**
* @brief	cdsp get af window statistics
* @param	index [in]: index = 1, 2, 3
* @param	h_value_l[out]: h value low get
* @param	h_value_h[out]: h value high get
* @param	v_value_l[out]: v value low get
* @param	v_value_h[out]: v value high get
* @return	SUCCESS/ERROR
*/
SINT32
gpHalCdspGetAFWinVlaue(
	UINT8 index,
	UINT32 *h_value_l,
	UINT32 *h_value_h,
	UINT32 *v_value_l,
	UINT32 *v_value_h
)
{
	if(index == 1)
	{
		*h_value_l = pCdspReg3a->cdspAfWin1HValL;
		*h_value_h = pCdspReg3a->cdspAfWin1HValH;
		*v_value_l = pCdspReg3a->cdspAfWin1VValL;
		*v_value_h = pCdspReg3a->cdspAfWin1VValH;
	}
	else if(index == 2)
	{
		*h_value_l = pCdspReg3a->cdspAfWin2HValL;
		*h_value_h = pCdspReg3a->cdspAfWin2HValH;
		*v_value_l = pCdspReg3a->cdspAfWin2VValL;
		*v_value_h = pCdspReg3a->cdspAfWin2VValH;
	}
	else if(index == 3)
	{
		*h_value_l = pCdspReg3a->cdspAfWin3HValL;
		*h_value_h = pCdspReg3a->cdspAfWin3HValH;
		*v_value_l = pCdspReg3a->cdspAfWin3VValL;
		*v_value_h = pCdspReg3a->cdspAfWin3VValH;
	}
	else 
	{
		return -1;
	}
	return 0;
}

/**
* @brief	cdsp raw data path set
* @param	raw_mode [in]: raw data path set, 0:disable, 1:RGB_Path1, 3:RGB_Path2, 5:RGB_Path3 
* @param	cap_mode [in]: set 0:raw8, 1:raw10 
* @param	yuv_mode [in]: set 0:8y4u4v, 1:yuyv set
* @return	none
*/
void 
gpHalCdspSetRawPath(
	UINT8 raw_mode, 
	UINT8 cap_mode, 
	UINT8 yuv_mode
)
{
	UINT32 reg;
	
	raw_mode &= 0x07;
	cap_mode &= 0x1;
	yuv_mode &= 0x1;
	reg = pCdspReg1->cdspDataFmt; 
	reg &= ~(1 << 5 | 1 << 4 | 0x7);
	reg |= (yuv_mode << 5)|(cap_mode << 4)|raw_mode;	
	pCdspReg1->cdspDataFmt = reg;
}

/**
* @brief	cdsp sram fifo threshold
* @param	overflowen [in]: overflow enable
* @param	sramthd [in]: sram threshold
* @return	none
*/
void
gpHalCdspSetSRAM(
	UINT8 overflowen,
	UINT16 sramthd
)
{
	overflowen &= 0x1;
	if(sramthd > 0x1FF) sramthd = 0x100;
	pCdspReg1->cdspWsramThr = ((UINT32)overflowen << 11)|sramthd;
}

/**
* @brief	cdsp dma clamp size set
* @param	clamphsizeen [in]: clamp enable
* @param	Clamphsize [in]: clamp size set
* @return	none
*/
void 
gpHalCdspSetClampEn(
	UINT8 clamphsizeen,
	UINT16 Clamphsize
)
{
	clamphsizeen &= 0x1;
	pCdspReg1->cdspClampSet = ((UINT32)clamphsizeen << 12)|Clamphsize;
}

/**
* @brief	cdsp line interval set
* @param	line_interval [in]: line number
* @return	none
*/
void
gpHalCdspSetLineInterval(
	UINT16 line_interval
)
{
	pCdspReg1->cdspLineInterval = line_interval;
}

/**
* @brief	cdsp dma yuv buffer a set
* @param	width [in]: dma buffer width
* @param	height [in]: dma buffer height
* @param	buffe_addr [in]: dma buffer address
* @return	none
*/
void 
gpHalCdspSetYuvBuffA(
	UINT16 width,
	UINT16 height,
	UINT32 buffer_addr
)
{
	width &= 0xFFF;
	height &= 0xFFF;
	pCdspReg1->cdspDmaYuvAHVSize = ((UINT32)height << 12)|width;
	pCdspReg1->cdspDmaYuvASAddr = buffer_addr;
}

/**
* @brief	cdsp get dma yuv buffer size
* @param	width [out]: dma buffer width
* @param	height [out]: dma buffer height
* @return	none
*/
void 
gpHalCdspGetYuvBuffASize(
	UINT16 *width,
	UINT16 *height
)
{
	UINT32 size = pCdspReg1->cdspDmaYuvAHVSize; 
	*width = size & 0xFFF;
	*height = size >> 12; 
}

/**
* @brief	cdsp dma yuv buffer b set
* @param	width [in]: dma buffer width
* @param	height [in]: dma buffer height
* @param	buffe_addr [in]: dma buffer address
* @return	none
*/
void 
gpHalCdspSetYuvBuffB(
	UINT16 width,
	UINT16 height,
	UINT32 buffer_addr
)
{
	width &= 0xFFF;
	height &= 0xFFF;
	pCdspReg1->cdspDmaYuvBHVSize = ((UINT32)height << 12)|width;
	pCdspReg1->cdspDmaYuvBSAddr = buffer_addr;
}

/**
* @brief	cdsp get dma yuv buffer size
* @param	width [out]: dma buffer width
* @param	height [out]: dma buffer height
* @return	none
*/
void 
gpHalCdspGetYuvBuffBSize(
	UINT16 *width,
	UINT16 *height
)
{
	UINT32 size = pCdspReg1->cdspDmaYuvBHVSize;
	*width = size & 0xFFF;
	*height = size >> 12; 
}

/**
* @brief	cdsp dma raw buffer set
* @param	width [in]: dma buffer width
* @param	height [in]: dma buffer height
* @param	hoffset [in]: dma buffer h offset
* @param	buffe_addr [in]: dma buffer address
* @return	none
*/
void 
gpHalCdspSetRawBuff(
	UINT16 width,
	UINT16 height,
	UINT32 hoffset,
	UINT32 buffer_addr
)
{
	width = (width >> 1) & 0xFFF;
	height = (height >> 1) & 0xFFF; 
	pCdspReg1->cdspDmaRawHVSize = ((UINT32)height << 12)|width;
	pCdspReg1->cdspDmaRawHOffset = hoffset;
	pCdspReg1->cdspDmaRawSAddr = buffer_addr;
}

/**
* @brief	cdsp dma yuv buffer mode set
* @param	buffer_mode [in]: dma buffer mode
* @return	none
*/
void 
gpHalCdspSetDmaBuff(
	UINT8 buffer_mode
)
{
	if(buffer_mode == RD_A_WR_A)
		pCdspReg1->cdspDmaConfig = 0x00;
	else if(buffer_mode == RD_A_WR_B)
		pCdspReg1->cdspDmaConfig = 0x01;
	else if(buffer_mode == RD_B_WR_B)
		pCdspReg1->cdspDmaConfig = 0x03;
	else if(buffer_mode == RD_B_WR_A)
		pCdspReg1->cdspDmaConfig = 0x02;
	else
		pCdspReg1->cdspDmaConfig = 0x10;
}

/**
* @brief	cdsp read back size set
* @param	hoffset [in]: read back h offset
* @param	voffset [in]: read back v offset
* @param	hsize [in]: read back h size
* @param	vsize [in]: read back v size
* @return	none
*/
void 
gpHalCdspSetReadBackSize(
	UINT16 hoffset,
	UINT16 voffset,
	UINT16 hsize,
	UINT16 vsize
)
{
	hoffset &= 0xFFF;
	voffset &= 0xFFF;
	hsize &= 0xFFF;
	vsize &= 0xFFF;
	pCdspReg1->cdspWdramHOffset = hoffset;
	pCdspReg1->cdspWdramVOffset = voffset;
	pCdspReg1->cdspRbHOffset = hoffset;
	pCdspReg1->cdspRbVOffset = voffset;
	pCdspReg1->cdspRbHSize = hsize;
	pCdspReg1->cdspRbVSize = vsize;
}

/**
* @brief	cdsp write register
* @param 	reg[in]: register address
* @param 	value[in]: register value
* @return 	SUCCESS/ERROR
*/
SINT32
gpHalCdspWriteReg(
	UINT32 reg,
	UINT32 value
)
{
	reg -= IO3_START + 0x1000;
	if(reg > 0x700)
		return -1;

	(*(volatile unsigned *)(LOGI_ADDR_CDSP_REG + reg)) = value;
	return 0;
}

/**
* @brief	cdsp read register
* @param 	reg[in]: register address
* @param 	value[in]: register value
* @return 	SUCCESS/ERROR
*/
SINT32
gpHalCdspReadReg(
	UINT32 reg,
	UINT32 *value
)
{
	reg -= IO3_START + 0x1000;
	if(reg > 0x700)
		return -1;
	
	*value = (*(volatile unsigned *)(LOGI_ADDR_CDSP_REG + reg));
	return 0;
}

/**
* @brief		cdsp sensor interface reset
* @return	none
*/
void 
gpHalCdspFrontReset(
	void
)
{
	pCdspRegFront->cdspFrontGCLK |= 0x100;
	pCdspRegFront->cdspFrontGCLK &= ~0x100;
}

/**
 * @brief	cdsp set front sensor input format
 * @param	format [in]: input format
 * @return	0: success, other: fail
*/
SINT32 
gpHalCdspFrontSetInputFormat(
	UINT32 format
)
{	
	switch(format)
	{
		case C_SDRAM_FMT_RAW8:
			pCdspReg1->cdspDataFmt |= 0x320;
			pCdspReg1->cdspDataFmt |= 1 << 4;	/* Raw8 */
			pCdspRegFront->cdspTgZero = 0x1FF;
			gpHalCdspSetMuxPath(0);
			break;
		case C_SDRAM_FMT_RAW10:
			pCdspReg1->cdspDataFmt |= 0x320;
			pCdspReg1->cdspDataFmt &= ~(1 << 4);/* Raw10 */
			pCdspRegFront->cdspTgZero = 0x1FF;
			gpHalCdspSetMuxPath(0);
			break;
		case C_SDRAM_FMT_VY1UY0:
			pCdspReg1->cdspDataFmt |= 0x320;
			pCdspReg1->cdspDataFmt |= 1 << 4;
			pCdspRegFront->cdspTgZero = 0x1FF;
			gpHalCdspSetMuxPath(1);
			break;
		case C_FRONT_FMT_RAW8:
			pCdspReg1->cdspDataFmt |= 0x320;
			pCdspReg1->cdspDataFmt |= 1 << 4;
			pCdspRegFront->cdspFrontCtrl3 &= ~0xE00;
			pCdspRegFront->cdspTgZero = 0x1FC;
			gpHalCdspSetMuxPath(0);
			break;
		case C_FRONT_FMT_RAW10:
			pCdspReg1->cdspDataFmt |= 0x320;
			pCdspReg1->cdspDataFmt &= ~(1 << 4);
			pCdspRegFront->cdspFrontCtrl3 &= ~0xE00;
			pCdspRegFront->cdspTgZero = 0x1FC;
			gpHalCdspSetMuxPath(0);
			break;
		case C_FRONT_FMT_UY1VY0:
			pCdspReg1->cdspDataFmt |= 0x320;
			pCdspReg1->cdspDataFmt |= 1 << 4;
			pCdspRegFront->cdspFrontCtrl3 &= ~0x03;
			pCdspRegFront->cdspFrontCtrl3 |= 0xE00;
			pCdspRegFront->cdspTgZero = 0x1E0;
			gpHalCdspSetMuxPath(1);
			break;
		case C_FRONT_FMT_Y1VY0U:
			pCdspReg1->cdspDataFmt |= 0x320;
			pCdspReg1->cdspDataFmt |= 1 << 4;
			pCdspRegFront->cdspFrontCtrl3 &= ~0x03;
			pCdspRegFront->cdspFrontCtrl3 |= 0xE01;
			pCdspRegFront->cdspTgZero = 0x1E0;
			gpHalCdspSetMuxPath(1);
			break;
		case C_FRONT_FMT_VY1UY0:
			pCdspReg1->cdspDataFmt |= 0x320;
			pCdspReg1->cdspDataFmt |= 1 << 4;
			pCdspRegFront->cdspFrontCtrl3 &= ~0x03;
			pCdspRegFront->cdspFrontCtrl3 |= 0xE02;
			pCdspRegFront->cdspTgZero = 0x1E0;
			gpHalCdspSetMuxPath(1);
			break;
		case C_FRONT_FMT_Y1UY0V:		
			pCdspReg1->cdspDataFmt |= 0x320;
			pCdspReg1->cdspDataFmt |= 1 << 4;
			pCdspRegFront->cdspFrontCtrl3 &= ~0x03;
			pCdspRegFront->cdspFrontCtrl3 |= 0xE03;
			pCdspRegFront->cdspTgZero = 0x1E0;
			gpHalCdspSetMuxPath(1);
			break;	
		case C_MIPI_FMT_RAW8:
			pCdspReg1->cdspDataFmt |= 0x320;
			pCdspReg1->cdspDataFmt |= 1 << 4;
			pCdspRegFront->cdspFrontCtrl3 &= ~0xE00;
			gpHalCdspSetMuxPath(0);
			pCdspRegFront->cdspTgZero = 0x1FF;
			pCdspRegFront->cdspFrontGCLK |= 0x3F;
			pCdspRegFront->cdspMipiCtrl = 0x0000;
			break;
		case C_MIPI_FMT_RAW10:
			pCdspReg1->cdspDataFmt |= 0x320;
			pCdspReg1->cdspDataFmt &= ~(1 << 4);
			pCdspRegFront->cdspFrontCtrl3 &= ~0xE00;
			gpHalCdspSetMuxPath(0);
			pCdspRegFront->cdspTgZero = 0x1FF;
			pCdspRegFront->cdspFrontGCLK |= 0x3F;
			pCdspRegFront->cdspMipiCtrl = 0x0000;
			break;
		case C_MIPI_FMT_Y1VY0U:
			pCdspReg1->cdspDataFmt |= 0x320;
			pCdspReg1->cdspDataFmt |= 1 << 4;
			pCdspRegFront->cdspFrontCtrl3 &= ~0x03;
			pCdspRegFront->cdspFrontCtrl3 |= 0xE60;
			gpHalCdspSetMuxPath(1);
			pCdspRegFront->cdspTgZero = 0x1FF;
			pCdspRegFront->cdspFrontGCLK |= 0x3F;
			pCdspRegFront->cdspMipiCtrl = 0x0001;
			break;	
		default:
			return -1;
	}
	return 0;
}

/**
* @brief	cdsp set front sensor input size
* @param	hoffset [in]: h offset
* @param	voffset [in]: v offset
* @param	hsize [in]: h size
* @param	vsize [in]: v size
* @return	none
*/
void 
gpHalCdspFrontSetFrameSize(
	UINT32 hoffset, 
	UINT32 voffset, 
	UINT32 hsize,	
	UINT32 vsize
)
{
	hsize &= 0xFFF;
	vsize &= 0xFFF;
	if(hoffset == 0) hoffset = 1;
	if(voffset == 0) voffset = 1;
	pCdspRegFront->cdspFrameHSet = (hoffset << 16)|hsize;
	pCdspRegFront->cdspFrameVSet = (voffset << 16)|vsize;
}

/**
* @brief	cdsp set front harizontal reshape
* @param	HReshEn [in]: h reshape enable
* @param	Hrise [in]: h rise size
* @param	Hfall [in]: h fall size
* @return	none
*/
void 
gpHalCdspFrontSetHReshape(
	UINT32 HReshEn, 
	UINT32 Hrise,	
	UINT32 Hfall
)
{
	pCdspRegFront->cdspHSyncFrEdge = ((Hfall & 0x1FFF) << 16) | (Hrise & 0x1FFF);
	pCdspRegFront->cdspFrontCtrl2 &= ~0x01;
	pCdspRegFront->cdspFrontCtrl2 |= (HReshEn & 0x1);
}

/**
* @brief	cdsp set front vertical reshape
* @param	VReshEn [in]: v reshape enable
* @param	Vrise [in]: v rise size
* @param	Vfall [in]: v fall size
* @return	none
*/
void 
gpHalCdspFrontSetVReshape(
	UINT32 VReshEn, 
	UINT32 Vrise,	
	UINT32 Vfall
)
{
	pCdspRegFront->cdspVSyncFrEdge = ((Vfall & 0x1FFF) << 16) | (Vrise & 0x1FFF);
	pCdspRegFront->cdspFrontCtrl2 &= ~0x02;
	pCdspRegFront->cdspFrontCtrl2 |= (VReshEn & 0x1) << 1;
}

/**
* @brief	cdsp set front h/v polarity
* @param	CCIR656En [in]: 0: CCIR601, 1: CCIR656
* @param	hpolarity [in]: 0: HACT, 1: LACT
* @param	vpolarity [in]: 0: HACT, 1: LACT
* @param	sync_en [in]: 0: disable, 1: enable
* @return	none
*/
void 
gpHalCdspFrontSetInterface(
	UINT32 CCIR656En,
	UINT32 hsync_act, 
	UINT32 vsync_act,
	UINT32 sync_en
)
{
	UINT32 reg_value = pCdspRegFront->cdspFrontCtrl3;
	
	if(CCIR656En)
	{
		reg_value |= (1 << 4);
		reg_value &= ~(1 << 6 | 1 << 5); //use Tvctr interface
		hsync_act = vsync_act = 0;
	}
	else 
	{	//CCIR601 & Href
		reg_value &= ~(1 << 4);	
		reg_value |= (1 << 6 | 1 << 5); //use hoffset/voffset
	}

	pCdspRegFront->cdspFrontCtrl3 = reg_value;
	reg_value = pCdspRegFront->cdspFrontCtrl1;
	if(hsync_act) 
	{
		reg_value |= 1 << 4;
		reg_value &= ~(1 << 8);
	}
	else
	{
		reg_value &= ~(1 << 4);
		reg_value |= (1 << 8);
	}
	
	if(vsync_act)
	{
		reg_value |= 1 << 5;
		reg_value &= ~(1 << 9);
	}
	else
	{
		reg_value &= ~(1 << 5);
		reg_value |= (1 << 9);
	}
	
	if(sync_en)
		reg_value |= 1 << 12;
	else
		reg_value &= ~(1 << 12);
	
	pCdspRegFront->cdspFrontCtrl1 = reg_value;
}

void 
gpHalCdspFrontSetInterlace(
	UINT32 field,
	UINT32 interlace
)
{
	UINT32 reg_value = pCdspReg1->cdspTvMode;
	
	if(interlace)
	{
		reg_value |= (1 << 0);
	}
	else
	{
		reg_value &= ~(1 << 0);
	}

	if(field)
	{
		reg_value |= (1<<2) | (1<<3);
	}
	else
	{
		reg_value &= ~((1<<3) | (1<<2));
	}

	pCdspReg1->cdspTvMode = reg_value;
}

/**
* @brief	cdsp set front mipi sensor input size
* @param	hoffset [out]: h offset
* @param	voffset [out]: v offset
* @param	hsize [out]: h size
* @param	vsize [out]: v size
* @return	none
*/
void 
gpHalCdspFrontSetMipiFrameSize(
	UINT32 hoffset, 
	UINT32 voffset, 
	UINT32 hsize,	
	UINT32 vsize
)
{
	hoffset &= 0xFFF;
	voffset &= 0xFFF;
	hsize &= 0xFFF;
	vsize &= 0xFFF;	
	pCdspRegFront->cdspMipiHVOffset = (voffset << 12) | hoffset;
	pCdspRegFront->cdspMipiHVSize = (vsize << 12) | hsize;
}

