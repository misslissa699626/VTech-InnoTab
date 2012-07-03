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
 * @file hal_disp2.c
 * @brief Display2 HAL interface 
 * @author Anson Chuang
 */
/*******************************************************************************
*                         H E A D E R   F I L E S
*******************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <asm/io.h>

#include <mach/hal/sysregs.h>
#include <mach/hal/hal_disp2.h>
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
static UINT32 gDisp2DevType;
static UINT16 gDisp2Width;
static UINT16 gDisp2Height;

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/

/**/
UINT32
gpHalDisp2Init(
	void
)
{
	return 0;
}
EXPORT_SYMBOL(gpHalDisp2Init);

void
gpHalDisp2Deinit(
	void
)
{
	return;
}
EXPORT_SYMBOL(gpHalDisp2Deinit);

void
gpHalDisp2SetDevType(
	UINT32 devType
)
{
	gDisp2DevType = devType;
}
EXPORT_SYMBOL(gpHalDisp2SetDevType);

UINT32
gpHalDisp2GetDevType(
	void
)
{
	return gDisp2DevType;
}
EXPORT_SYMBOL(gpHalDisp2GetDevType);

void
gpHalDisp2UpdateParameter(
	void
)
{
	return;
}
EXPORT_SYMBOL(gpHalDisp2UpdateParameter);

void
gpHalDisp2SetIntEnable(
	UINT32 field
)
{
	gpHalPPUSetIrqEnable(field);
}
EXPORT_SYMBOL(gpHalDisp2SetIntEnable);

void
gpHalDisp2SetIntDisable(
	UINT32 field
)
{
	gpHalPPUSetIrqDisable(field);
}
EXPORT_SYMBOL(gpHalDisp2SetIntDisable);

void
gpHalDisp2ClearIntFlag(
	UINT32 field
)
{
	gpHalPPUClearIrqFlag(field);
}
EXPORT_SYMBOL(gpHalDisp2ClearIntFlag);

UINT32
gpHalDisp2GetIntStatus(
	void
)
{
	return gpHalPPUGetIrqStatus();
}
EXPORT_SYMBOL(gpHalDisp2GetIntStatus);

void
gpHalDisp2SetPriFrameAddr(
	UINT32 addr
)
{
	gpHalPPUSetTvBufferAddr(addr);
}
EXPORT_SYMBOL(gpHalDisp2SetPriFrameAddr);

void
gpHalDisp2SetFlip(
	UINT32 value
)
{
	gpHalPPUSetFlip(value);
}
EXPORT_SYMBOL(gpHalDisp2SetFlip);

/* TV panel */
UINT32
gpHalDisp2SetRes(
	UINT32 mode
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);
	UINT32 regVal;

	regVal = pdisp2Reg->disp2Ctrl;
	regVal &= ~(0x3 << 5);
	regVal |= (mode << 5);
	pdisp2Reg->disp2Ctrl = regVal;

	if (mode == HAL_DISP2_RES_320_240) {
		gDisp2Width = 320;
		gDisp2Height = 240;
		gpHalPPUSetVgaEnable(0);
		gpHalPPUSetRes(0);
	}
	else if (mode == HAL_DISP2_RES_640_480) {
		gDisp2Width = 640;
		gDisp2Height = 480;
		gpHalPPUSetVgaEnable(1);
		gpHalPPUSetRes(0);
	}
	else if (mode == HAL_DISP2_RES_720_480) {
		gDisp2Width = 720;
		gDisp2Height = 480;
		gpHalPPUSetVgaEnable(0);
		gpHalPPUSetRes(4);
	}

	return 0;
}
EXPORT_SYMBOL(gpHalDisp2SetRes);

void
gpHalDisp2GetRes(
	UINT16 *width,
	UINT16 *height
)
{
	*width = gDisp2Width;
	*height = gDisp2Height;
}
EXPORT_SYMBOL(gpHalDisp2GetRes);

void
gpHalDisp2SetInterlace(
	UINT32 mode
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);

	if (mode == HAL_DISP2_TV_INTERLACE)
		pdisp2Reg->disp2Ctrl &= ~(0x01 << 4);
	else
		pdisp2Reg->disp2Ctrl |= (0x01 << 4);
}
EXPORT_SYMBOL(gpHalDisp2SetInterlace);

UINT32
gpHalDisp2GetInterlace(
	void
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);

	return (pdisp2Reg->disp2Ctrl >> 4) & 0x01;
}
EXPORT_SYMBOL(gpHalDisp2GetInterlace);

void
gpHalDisp2SetTvType(
	UINT32 type
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);
	UINT32 regVal;

	regVal = pdisp2Reg->disp2Ctrl;
	regVal &= ~(0x07 << 1);
	regVal |= (type << 1);
	pdisp2Reg->disp2Ctrl = regVal;
}
EXPORT_SYMBOL(gpHalDisp2SetTvType);

UINT32
gpHalDisp2GetTvType(
	void
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);

	return (pdisp2Reg->disp2Ctrl >> 1) & 0x07;
}
EXPORT_SYMBOL(gpHalDisp2GetTvType);

void
gpHalDisp2SetEnable(
	UINT32 enable
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);

	if (enable)
		pdisp2Reg->disp2Ctrl |= 0x01;
	else
		pdisp2Reg->disp2Ctrl &= ~0x01;
}
EXPORT_SYMBOL(gpHalDisp2SetEnable);

UINT32
gpHalDisp2GetEnable(
	void
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);

	return pdisp2Reg->disp2Ctrl & 0x01;
}
EXPORT_SYMBOL(gpHalDisp2GetEnable);

void
gpHalDisp2SetSaturation(
	UINT32 value
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);

	pdisp2Reg->disp2Saturation = value;
}
EXPORT_SYMBOL(gpHalDisp2SetSaturation);

UINT32
gpHalDisp2GetSaturation(
	void
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);

	return pdisp2Reg->disp2Saturation;
}
EXPORT_SYMBOL(gpHalDisp2GetSaturation);

void
gpHalDisp2SetHue(
	UINT32 value
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);

	pdisp2Reg->disp2Hue = value;
}
EXPORT_SYMBOL(gpHalDisp2SetHue);

UINT32
gpHalDisp2GetHue(
	void
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);

	return pdisp2Reg->disp2Hue;
}
EXPORT_SYMBOL(gpHalDisp2GetHue);

void
gpHalDisp2SetBrightness(
	UINT32 value
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);

	pdisp2Reg->disp2Brightness = value;
}
EXPORT_SYMBOL(gpHalDisp2SetBrightness);

UINT32
gpHalDisp2GetBrightness(
	void
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);

	return pdisp2Reg->disp2Brightness;
}
EXPORT_SYMBOL(gpHalDisp2GetBrightness);

void
gpHalDisp2SetSharpness(
	UINT32 value
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);

	pdisp2Reg->disp2Sharpness = value;
}
EXPORT_SYMBOL(gpHalDisp2SetSharpness);

UINT32
gpHalDisp2GetSharpness(
	void
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);

	return pdisp2Reg->disp2Sharpness;
}
EXPORT_SYMBOL(gpHalDisp2GetSharpness);

void
gpHalDisp2SetYGain(
	UINT32 value
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);

	pdisp2Reg->disp2YGain = value;
}
EXPORT_SYMBOL(gpHalDisp2SetYGain);

UINT32
gpHalDisp2GetYGain(
	void
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);

	return pdisp2Reg->disp2YGain;
}
EXPORT_SYMBOL(gpHalDisp2GetYGain);

void
gpHalDisp2SetYDelay(
	UINT32 value
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);

	pdisp2Reg->disp2YDelay = value;
}
EXPORT_SYMBOL(gpHalDisp2SetYDelay);

UINT32
gpHalDisp2GetYDelay(
	void
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);

	return pdisp2Reg->disp2YDelay;
}
EXPORT_SYMBOL(gpHalDisp2GetYDelay);

void
gpHalDisp2SetVPosition(
	UINT32 value
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);

	pdisp2Reg->disp2VPosition = value;
}
EXPORT_SYMBOL(gpHalDisp2SetVPosition);

UINT32
gpHalDisp2GetVPosition(
	void
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);

	return pdisp2Reg->disp2VPosition;
}
EXPORT_SYMBOL(gpHalDisp2GetVPosition);

void
gpHalDisp2SetHPosition(
	UINT32 value
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);

	pdisp2Reg->disp2HPosition = value;
}
EXPORT_SYMBOL(gpHalDisp2SetHPosition);

UINT32
gpHalDisp2GetHPosition(
	void
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);

	return pdisp2Reg->disp2HPosition;
}
EXPORT_SYMBOL(gpHalDisp2GetHPosition);

void
gpHalDisp2SetVideoDac(
	UINT32 value
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);

	pdisp2Reg->disp2Videodac = value;
}
EXPORT_SYMBOL(gpHalDisp2SetVideoDac);

UINT32
gpHalDisp2GetVedeoDac(
	void
)
{
	disp2Reg_t *pdisp2Reg = (disp2Reg_t *)(LOGI_ADDR_DISP2_REG);

	return pdisp2Reg->disp2Videodac;
}
EXPORT_SYMBOL(gpHalDisp2GetVedeoDac);
