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
 * @file hal_disp1.c
 * @brief Display1 HAL interface 
 * @author Daniel Huang
 */
/*******************************************************************************
*                         H E A D E R   F I L E S
*******************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <asm/io.h>

#include <mach/hal/sysregs.h>
#include <mach/hal/hal_disp1.h>
#include <mach/hal/regmap/reg_ppu.h>
#include <mach/hal/hal_ppu.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/



/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#if 0
#define DEBUG	printk
#else
#define DEBUG(...)
#endif

#define DERROR printk



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
static UINT32 gDisp1DevType;
static UINT16 gDisp1Width;
static UINT16 gDisp1Height;

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/

/**/
UINT32
gpHalDisp1Init(
	void
)
{
	return 0;
}
EXPORT_SYMBOL(gpHalDisp1Init);

void
gpHalDisp1Deinit(
	void
)
{
	return;
}
EXPORT_SYMBOL(gpHalDisp1Deinit);

void
gpHalDisp1SetDevType(
	UINT32 devType
)
{
	gDisp1DevType = devType;
}
EXPORT_SYMBOL(gpHalDisp1SetDevType);

UINT32
gpHalDisp1GetDevType(
	void
)
{
	return gDisp1DevType;
}
EXPORT_SYMBOL(gpHalDisp1GetDevType);

void
gpHalDisp1UpdateParameter(
	void
)
{
	return;
}
EXPORT_SYMBOL(gpHalDisp1UpdateParameter);

void
gpHalDisp1SetIntEnable(
	UINT32 field
)
{
	gpHalPPUSetIrqEnable(field);
}
EXPORT_SYMBOL(gpHalDisp1SetIntEnable);

void
gpHalDisp1SetIntDisable(
	UINT32 field
)
{
	gpHalPPUSetIrqDisable(field);
}
EXPORT_SYMBOL(gpHalDisp1SetIntDisable);

void
gpHalDisp1ClearIntFlag(
	UINT32 field
)
{
	gpHalPPUClearIrqFlag(field);
}
EXPORT_SYMBOL(gpHalDisp1ClearIntFlag);

UINT32
gpHalDisp1GetIntStatus(
	void
)
{
	return gpHalPPUGetIrqStatus();
}
EXPORT_SYMBOL(gpHalDisp1GetIntStatus);

void
gpHalDisp1SetPriFrameAddr(
	UINT32 addr
)
{
	gpHalPPUSetTftBufferAddr(addr);
}
EXPORT_SYMBOL(gpHalDisp1SetPriFrameAddr);


/* Dither */
void
gpHalDisp1SetDitherType(
	UINT32 type
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	UINT32 regVal;

	DEBUG("[%s:%d], type=%d\n", __FUNCTION__, __LINE__, type);

	regVal = pdisp1Reg->disp1TS_MISC;
	regVal &= ~(0x1 << 9);
	regVal |= (type << 9);
	pdisp1Reg->disp1TS_MISC = regVal;

}
EXPORT_SYMBOL(gpHalDisp1SetDitherType);

UINT32
gpHalDisp1GetDitherType(
	void
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	return (pdisp1Reg->disp1TS_MISC >> 9) & 0x01;
}
EXPORT_SYMBOL(gpHalDisp1GetDitherType);

void
gpHalDisp1SetDitherEnable(
	UINT32 enable
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);

	DEBUG("[%s:%d], %d\n", __FUNCTION__, __LINE__, enable);

	if (enable)
		pdisp1Reg->disp1TS_MISC |= (0x1 << 8);
	else
		pdisp1Reg->disp1TS_MISC &= ~(0x1 << 8);

}
EXPORT_SYMBOL(gpHalDisp1SetDitherEnable);

UINT32
gpHalDisp1GetDitherEnable(
	void
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	return ((pdisp1Reg->disp1TS_MISC >> 8) & 0x1);
}
EXPORT_SYMBOL(gpHalDisp1GetDitherEnable);

void
gpHalDisp1SetDitherMap(
	UINT32 map0,
	UINT32 map1,
	UINT32 map2,
	UINT32 map3
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);

	DEBUG("[%s:%d], %d, %d, %d, %d\n", __FUNCTION__, __LINE__, map0, map1, map2, map3);

	pdisp1Reg->disp1TAB0 = map0;
	pdisp1Reg->disp1TAB1 = map1;
	pdisp1Reg->disp1TAB2 = map2;
	pdisp1Reg->disp1TAB3 = map3;

}
EXPORT_SYMBOL(gpHalDisp1SetDitherMap);

/* Gamma */
void
gpHalDisp1SetGammaTable(
	UINT32 id,
	UINT8 *table
)
{
	ppuFunReg_t *pPPUFunreg = (ppuFunReg_t *)(PPU_BASE_REG);
	UINT32 i;
	volatile UINT32 *dest_ptr = NULL;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	if (id == 0) {
		dest_ptr = (volatile UINT32 *) (pPPUFunreg->ppuColormappingRam);
	}
	else if(id == 1){
		dest_ptr = (volatile UINT32 *) (pPPUFunreg->ppuColormappingRamG);
	}
	else if(id == 2){
		dest_ptr = (volatile UINT32 *) (pPPUFunreg->ppuColormappingRamR);
	}
	else {
		return ;
	}

	for(i=0; i<256; i++) {
		*dest_ptr++ = (UINT32) table[i];
	}
}
EXPORT_SYMBOL(gpHalDisp1SetGammaTable);

void
gpHalDisp1SetGammaEnable(
	UINT32 enable
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);

	DEBUG("[%s:%d], %d\n", __FUNCTION__, __LINE__, enable);

	if (enable)
		pdisp1Reg->disp1TS_MISC |= (0x1 << 13);
	else
		pdisp1Reg->disp1TS_MISC &= ~(0x1 << 13);

}
EXPORT_SYMBOL(gpHalDisp1SetGammaEnable);

UINT32
gpHalDisp1GetGammaEnable(
	void
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	return ((pdisp1Reg->disp1TS_MISC >> 13) & 0x1);
}
EXPORT_SYMBOL(gpHalDisp1GetGammaEnable);

/* Lcd panel */
UINT32
gpHalDisp1SetRes(
	UINT32 mode
)
{
	gpHalPPUSetRes(mode);

	if (mode == HAL_DISP1_RES_320_240) {
		gDisp1Width = 320;
		gDisp1Height = 240;
	}
	else if (mode == HAL_DISP1_RES_640_480) {
		gDisp1Width = 640;
		gDisp1Height = 480;
	}
	else if (mode == HAL_DISP1_RES_480_234) {
		gDisp1Width = 480;
		gDisp1Height = 234;
	}
	else if (mode == HAL_DISP1_RES_480_272) {
		gDisp1Width = 480;
		gDisp1Height = 272;
	}
	else if (mode == HAL_DISP1_RES_720_480) {
		gDisp1Width = 720;
		gDisp1Height = 480;
	}
	else if (mode == HAL_DISP1_RES_800_480) {
		gDisp1Width = 800;
		gDisp1Height = 480;
	}
	else if (mode == HAL_DISP1_RES_800_600) {
		gDisp1Width = 800;
		gDisp1Height = 600;
	}
	else if (mode == HAL_DISP1_RES_1024_768) {
		gDisp1Width = 1024;
		gDisp1Height = 768;
	}
	else {
		return -1;
	}

	return 0;
}
EXPORT_SYMBOL(gpHalDisp1SetRes);

void
gpHalDisp1GetRes(
	UINT16 *width,
	UINT16 *height
)
{
	*width = gDisp1Width;
	*height = gDisp1Height;
}
EXPORT_SYMBOL(gpHalDisp1GetRes);

void
gpHalDisp1SetVerticalStart(
	UINT32 disp1V_Start
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	DEBUG("[%s:%d], disp1V_Start=%d\n", __FUNCTION__, __LINE__, disp1V_Start);
	pdisp1Reg->disp1V_Start = disp1V_Start;

}
EXPORT_SYMBOL(gpHalDisp1SetVerticalStart);

UINT32
gpHalDisp1GetVerticalStart(
	void
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
	
	return pdisp1Reg->disp1V_Start;

}
EXPORT_SYMBOL(gpHalDisp1GetVerticalStart);

void
gpHalDisp1SetHorizontalStart(
	UINT32 disp1H_Start
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	DEBUG("[%s:%d], disp1H_Start=%d\n", __FUNCTION__, __LINE__, disp1H_Start);
	pdisp1Reg->disp1H_Start = disp1H_Start;

}
EXPORT_SYMBOL(gpHalDisp1SetHorizontalStart);

UINT32
gpHalDisp1GetHorizontalStart(
	void
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
	
	return pdisp1Reg->disp1H_Start;

}
EXPORT_SYMBOL(gpHalDisp1GetHorizontalStart);

void
gpHalDisp1SetVerticalEnd(
	UINT32 disp1V_End
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	DEBUG("[%s:%d], disp1V_End=%d\n", __FUNCTION__, __LINE__, disp1V_End);
	pdisp1Reg->disp1V_End = disp1V_End;

}
EXPORT_SYMBOL(gpHalDisp1SetVerticalEnd);

UINT32
gpHalDisp1GetVerticalEnd(
	void
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
	
	return pdisp1Reg->disp1V_End;

}
EXPORT_SYMBOL(gpHalDisp1GetVerticalEnd);

void
gpHalDisp1SetHorizontalEnd(
	UINT32 disp1H_End
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	DEBUG("[%s:%d], disp1H_End=%d\n", __FUNCTION__, __LINE__, disp1H_End);
	pdisp1Reg->disp1H_End = disp1H_End;

}
EXPORT_SYMBOL(gpHalDisp1SetHorizontalEnd);

UINT32
gpHalDisp1GetHorizontalEnd(
	void
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
	
	return pdisp1Reg->disp1H_End;

}
EXPORT_SYMBOL(gpHalDisp1GetHorizontalEnd);

void
gpHalDisp1SetVerticalPeriod(
	UINT32 disp1V_Period
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	DEBUG("[%s:%d], disp1V_Period=%d\n", __FUNCTION__, __LINE__, disp1V_Period);
	pdisp1Reg->disp1V_Period = disp1V_Period;

}
EXPORT_SYMBOL(gpHalDisp1SetVerticalPeriod);

UINT32
gpHalDisp1GetVerticalPeriod(
	void
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
	
	return pdisp1Reg->disp1V_Period;

}
EXPORT_SYMBOL(gpHalDisp1GetVerticalPeriod);

void
gpHalDisp1SetHorizontalPeriod(
	UINT32 disp1H_Period
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	DEBUG("[%s:%d], disp1H_Period=%d\n", __FUNCTION__, __LINE__, disp1H_Period);
	pdisp1Reg->disp1H_Period = disp1H_Period;

}
EXPORT_SYMBOL(gpHalDisp1SetHorizontalPeriod);

UINT32
gpHalDisp1GetHorizontalPeriod(
	void
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
	
	return pdisp1Reg->disp1H_Period;

}
EXPORT_SYMBOL(gpHalDisp1GetHorizontalPeriod);

void
gpHalDisp1SetVSyncWidth(
	UINT32 disp1VS_Width
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	DEBUG("[%s:%d], disp1VS_Width=%d\n", __FUNCTION__, __LINE__, disp1VS_Width);
	pdisp1Reg->disp1VS_Width = disp1VS_Width;

}
EXPORT_SYMBOL(gpHalDisp1SetVSyncWidth);

UINT32
gpHalDisp1GetVSyncWidth(
	void
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
	
	return pdisp1Reg->disp1VS_Width;

}
EXPORT_SYMBOL(gpHalDisp1GetVSyncWidth);

void
gpHalDisp1SetHSyncWidth(
	UINT32 disp1HS_Width
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	DEBUG("[%s:%d], disp1HS_Width=%d\n", __FUNCTION__, __LINE__, disp1HS_Width);
	pdisp1Reg->disp1HS_Width = disp1HS_Width;

}
EXPORT_SYMBOL(gpHalDisp1SetHSyncWidth);

UINT32
gpHalDisp1GetHSyncWidth(
	void
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
	
	return pdisp1Reg->disp1HS_Width;

}
EXPORT_SYMBOL(gpHalDisp1GetHSyncWidth);

void
gpHalDisp1SetVSyncStart(
	UINT32 disp1VS_Start
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	DEBUG("[%s:%d], disp1VS_Start=%d\n", __FUNCTION__, __LINE__, disp1VS_Start);
	pdisp1Reg->disp1VS_Start = disp1VS_Start;

}
EXPORT_SYMBOL(gpHalDisp1SetVSyncStart);

UINT32
gpHalDisp1GetVSyncStart(
	void
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
	
	return pdisp1Reg->disp1VS_Start;

}
EXPORT_SYMBOL(gpHalDisp1GetVSyncStart);


void
gpHalDisp1SetVSyncEnd(
	UINT32 disp1VS_End
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	DEBUG("[%s:%d], disp1VS_End=%d\n", __FUNCTION__, __LINE__, disp1VS_End);
	pdisp1Reg->disp1VS_End = disp1VS_End;

}
EXPORT_SYMBOL(gpHalDisp1SetVSyncEnd);

UINT32
gpHalDisp1GetVSyncEnd(
	void
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
	
	return pdisp1Reg->disp1VS_End;

}
EXPORT_SYMBOL(gpHalDisp1GetVSyncEnd);

void
gpHalDisp1SetHSyncStart(
	UINT32 disp1HS_Start
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	DEBUG("[%s:%d], disp1HS_Start=%d\n", __FUNCTION__, __LINE__, disp1HS_Start);
	pdisp1Reg->disp1HS_Start = disp1HS_Start;

}
EXPORT_SYMBOL(gpHalDisp1SetHSyncStart);

UINT32
gpHalDisp1GetHSyncStart(
	void
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
	
	return pdisp1Reg->disp1HS_Start;

}
EXPORT_SYMBOL(gpHalDisp1GetHSyncStart);


void
gpHalDisp1SetHSyncEnd(
	UINT32 disp1HS_End
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	DEBUG("[%s:%d], disp1HS_End=%d\n", __FUNCTION__, __LINE__, disp1HS_End);
	pdisp1Reg->disp1HS_End = disp1HS_End;

}
EXPORT_SYMBOL(gpHalDisp1SetHSyncEnd);

UINT32
gpHalDisp1GetHSyncEnd(
	void
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
	
	return pdisp1Reg->disp1HS_End;

}
EXPORT_SYMBOL(gpHalDisp1GetHSyncEnd);

void
gpHalDisp1SetDataMode(
	UINT32 disp1DataMode
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	
	pdisp1Reg->disp1TS_MISC &= ~HAL_DISP1_DATA_MODE;
	pdisp1Reg->disp1TS_MISC |= disp1DataMode;
	
	DEBUG("[%s:%d], disp1DataMode=%d\n", __FUNCTION__, __LINE__, disp1DataMode);
	
	SCUA_LCD_TYPE_SEL &= ~0x30000;
	if (disp1DataMode == HAL_DISP1_DATA_MODE_666) {
		SCUA_LCD_TYPE_SEL |= 0x10000;
	}
}
EXPORT_SYMBOL(gpHalDisp1SetDataMode);


void
gpHalDisp1SetVSyncUnit(
	UINT32 status
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	
	if (status == STATE_TRUE) {
		pdisp1Reg->disp1Ctrl |= HAL_DISP1_VSYNC_UNIT;
	}
	else {
		pdisp1Reg->disp1Ctrl &= ~HAL_DISP1_VSYNC_UNIT;
	}
}
EXPORT_SYMBOL(gpHalDisp1SetVSyncUnit);


void
gpHalDisp1SetDClkSel(
	UINT32 sel
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	
	pdisp1Reg->disp1TS_MISC &= ~HAL_DISP1_DCLK_SEL;
	pdisp1Reg->disp1TS_MISC |= sel;
}
EXPORT_SYMBOL(gpHalDisp1SetDClkSel);

void
gpHalDisp1SetSignalInv(
	UINT32 mask, 
	UINT32 value
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	
	/*set vsync,hsync,dclk and DE inv */
	pdisp1Reg->disp1Ctrl &= ~mask;
	pdisp1Reg->disp1Ctrl |= (mask & value);
}
EXPORT_SYMBOL(gpHalDisp1SetSignalInv);

void
gpHalDisp1SetMode(
	UINT32 mode
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	
	pdisp1Reg->disp1Ctrl &= ~HAL_DISP1_MODE;
	pdisp1Reg->disp1Ctrl |= mode;
}
EXPORT_SYMBOL(gpHalDisp1SetMode);

void
gpHalDisp1SetClk(
	UINT32 clk
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	
	pdisp1Reg->disp1Ctrl &= ~HAL_DISP1_CLK_SEL;
	pdisp1Reg->disp1TS_MISC &= ~0xC0;
	
	if (clk < HAL_DISP1_CLK_DIVIDE_9) {
		pdisp1Reg->disp1Ctrl |= clk;
	}
	else {
		pdisp1Reg->disp1Ctrl |= clk & 0xF;
		pdisp1Reg->disp1TS_MISC |= (clk & 0x20) << 1;
		pdisp1Reg->disp1TS_MISC |= (clk & 0x10) << 3;
	}	
}
EXPORT_SYMBOL(gpHalDisp1SetClk);

void
gpHalDisp1SetSlideEn(
        UINT32 status
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	
	if (status == STATE_TRUE) {
		pdisp1Reg->disp1TS_MISC |= HAL_DISP1_SLIDE_EN;
	}
	else {
		pdisp1Reg->disp1TS_MISC &= ~HAL_DISP1_SLIDE_EN;
	}
}
EXPORT_SYMBOL(gpHalDisp1SetSlideEn);

void
gpHalDisp1SetEnable(
        UINT32 status
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	
	if (status == STATE_TRUE) {
		pdisp1Reg->disp1Ctrl |= HAL_DISP1_EN;
	}
	else {
		pdisp1Reg->disp1Ctrl &= ~HAL_DISP1_EN;
	}
}
EXPORT_SYMBOL(gpHalDisp1SetEnable);

UINT32
gpHalDisp1GetEnable(
	void
)
{
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);

	return (pdisp1Reg->disp1Ctrl & 0x01);
}
EXPORT_SYMBOL(gpHalDisp1GetEnable);
