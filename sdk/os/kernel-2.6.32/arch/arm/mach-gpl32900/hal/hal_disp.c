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
 * @file hal_disp.c
 * @brief Display HAL interface 
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
#include <mach/hal/hal_disp.h>
#include <mach/hal/hal_clock.h>
#include <mach/clock_mgr/gp_clock.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
/* Index of osd register in osdRegArray. */
#define DISP_OSD_ADDR		0
#define DISP_OSD_PITCH		1
#define DISP_OSD_RES		2
#define DISP_OSD_XY			3
#define DISP_OSD_FMT		4
#define DISP_OSD_CTRL		5
#define DISP_OSD_HPARAM		6
#define DISP_OSD_VPARAM0	7
#define DISP_OSD_VPARAM1	8
#define DISP_OSD_SCLRES		9
#define DISP_OSD_MAX		10


#define HAL_DISP_DEFAULT_BLANK 8 /* for fixing gamma enable issue */
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
typedef struct {
	int framerate;
	int clk_src;
} gpHalDispFramerateInfo_t;


/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/



/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static UINT32 gDispDevType;
static volatile UINT32 *osdRegArray[DISP_OSD_MAX][2];

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void
scu_change_pin_grp(
	UINT32 aPinGrp,
	UINT32 aPinNum
)
{
	UINT32 numReg = 0;
	UINT32 numInner = 0;
	UINT32 bitShift = 0;
	UINT32 bitMsk = 3;
	UINT32 val = 0;
	
	numReg = aPinGrp / 16;
	numInner = aPinGrp % 16;
	bitShift = numInner << 1;
	bitMsk <<= bitShift;
	aPinNum <<= bitShift;
	
	switch(numReg){	
		case 0:
			val = SCUB_PGS0;
			val &= ~bitMsk;
			val |= aPinNum;
        	SCUB_PGS0 = val;			
			break;
		case 1:
			val = SCUB_PGS1;
			val &= ~bitMsk;
			val |= aPinNum;
        	SCUB_PGS1 = val;		
			break;
		case 2:
			val = SCUB_PGS2;
			val &= ~bitMsk;
			val |= aPinNum;
        	SCUB_PGS2 = val;		
			break;
		case 3:
			val = SCUB_PGS3;
			val &= ~bitMsk;
			val |= aPinNum;
        	SCUB_PGS3 = val;		
			break;
	}
	
}
EXPORT_SYMBOL(scu_change_pin_grp);

static void
gpHalDispOsdRegInit(
	void
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);

	osdRegArray[DISP_OSD_ADDR][0] = &pdispReg->dispOsd0Addr;
	osdRegArray[DISP_OSD_ADDR][1] = &pdispReg->dispOsd1Addr;
	osdRegArray[DISP_OSD_PITCH][0] = &pdispReg->dispOsd0Pitch;
	osdRegArray[DISP_OSD_PITCH][1] = &pdispReg->dispOsd1Pitch;
	osdRegArray[DISP_OSD_RES][0] = &pdispReg->dispOsd0Res;
	osdRegArray[DISP_OSD_RES][1] = &pdispReg->dispOsd1Res;
	osdRegArray[DISP_OSD_XY][0] = &pdispReg->dispOsd0XY;
	osdRegArray[DISP_OSD_XY][1] = &pdispReg->dispOsd1XY;
	osdRegArray[DISP_OSD_FMT][0] = &pdispReg->dispOsd0Fmt;
	osdRegArray[DISP_OSD_FMT][1] = &pdispReg->dispOsd1Fmt;
	osdRegArray[DISP_OSD_CTRL][0] = &pdispReg->dispOsd0Ctrl;
	osdRegArray[DISP_OSD_CTRL][1] = &pdispReg->dispOsd1Ctrl;
	osdRegArray[DISP_OSD_HPARAM][0] = &pdispReg->dispOsd0SclHParam;
	osdRegArray[DISP_OSD_HPARAM][1] = &pdispReg->dispOsd1SclHParam;
	osdRegArray[DISP_OSD_VPARAM0][0] = &pdispReg->dispOsd0SclVParam0;
	osdRegArray[DISP_OSD_VPARAM0][1] = &pdispReg->dispOsd1SclVParam0;
	osdRegArray[DISP_OSD_VPARAM1][0] = &pdispReg->dispOsd0SclVParam1;
	osdRegArray[DISP_OSD_VPARAM1][1] = &pdispReg->dispOsd1SclVParam1;
	osdRegArray[DISP_OSD_SCLRES][0] = &pdispReg->dispOsd0SclRes;
	osdRegArray[DISP_OSD_SCLRES][1] = &pdispReg->dispOsd1SclRes;
}

void
gpHalDispSetClkType(
	UINT32 type
)
{
	UINT32 regVal;

	regVal = SCUA_LCD_TYPE_SEL;
	regVal &= ~(0x03 << 16);
	regVal |= (type << 16);
	SCUA_LCD_TYPE_SEL = regVal;
}
EXPORT_SYMBOL(gpHalDispSetClkType);

/**/
UINT32
gpHalDispInit(
	void
)
{
	gpHalDispOsdRegInit();

	return 0;
}
EXPORT_SYMBOL(gpHalDispInit);

void
gpHalDispDeinit(
	void
)
{
	return;
}
EXPORT_SYMBOL(gpHalDispDeinit);

void
gpHalDispSetDevType(
	UINT32 devType
)
{
	gDispDevType = devType;
}
EXPORT_SYMBOL(gpHalDispSetDevType);

UINT32
gpHalDispGetDevType(
	void
)
{
	return gDispDevType;
}
EXPORT_SYMBOL(gpHalDispGetDevType);

void
gpHalDispUpdateParameter(
	void
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	pdispReg->dispIrqSrc |= 0x02;
}
EXPORT_SYMBOL(gpHalDispUpdateParameter);

void
gpHalDispSetIntEnable(
	UINT32 field
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	pdispReg->dispIrqEn |= field;
}
EXPORT_SYMBOL(gpHalDispSetIntEnable);

void
gpHalDispSetIntDisable(
	UINT32 field
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	pdispReg->dispIrqEn &= ~field;
}
EXPORT_SYMBOL(gpHalDispSetIntDisable);

void
gpHalDispClearIntFlag(
	UINT32 field
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	regVal = pdispReg->dispIrqSrc;
	regVal &= ~(0x2);
	regVal |= field;
	pdispReg->dispIrqSrc = regVal;
}
EXPORT_SYMBOL(gpHalDispClearIntFlag);

static SINT32
gpHalDispCalculatFrameRate(
	int clk_src,
	int clk_target,
	int width, 
	int height
)
{
	int ratio;
	int clk_real;
	int framerate;

	ratio = clk_src/clk_target;
	if (ratio > 0xff) {
		return 0;
	}

	clk_real = clk_src / ratio;
	framerate = clk_real / (width * height);
	return framerate;
}

void
gpHalDispSetFrameRate(
	SINT32 framerate
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	gpHalDispFramerateInfo_t info[3];
	int devType = gpHalDispGetDevType();
	int width, height;
	int clk_target, real_freq;
	int clk_ceva;
	int i;
	int candidate;
	int ret;

	if (devType != HAL_DISP_DEV_LCD) {
		return;
	}

	width = ((pdispReg->dispRes >> 16) & 0xFFFF) + 
		((pdispReg->dispLcdHsync >> 18) & 0x3FF) + 
		((pdispReg->dispLcdHsync >> 8) & 0x3FF);
	height = (pdispReg->dispRes & 0xFFFF) + 
		((pdispReg->dispLcdVsync >> 18) & 0x3FF) + 
		((pdispReg->dispLcdVsync >> 8) & 0x3FF);

	for (i=0; i<3; i++)
		info[i].framerate = 0;
	i = 0;
	clk_target = width * height * framerate;
	if (clk_target <= XTAL_RATE) {
		if ((ret = gpHalDispCalculatFrameRate(XTAL_RATE, clk_target, width, height)) > 0) {
			info[i].framerate = ret;
			info[i].clk_src = HAL_LCD_CLK_XTAL;
			i++;
		}
	}
	gp_clk_get_rate((int*)"clk_ref_ceva", &clk_ceva);
	if (clk_target <= clk_ceva) {
		if ((ret = gpHalDispCalculatFrameRate(clk_ceva, clk_target, width, height)) > 0) {
			info[i].framerate = ret;
			info[i].clk_src = HAL_LCD_CLK_SPLL;
			i++;
		}
	}
	if (clk_target <= USBPHY_RATE) {
		if ((ret = gpHalDispCalculatFrameRate(USBPHY_RATE, clk_target, width, height)) > 0) {
			info[i].framerate = ret;
			info[i].clk_src = HAL_LCD_CLK_USBPHY;
			i++;
		}
	}

	if (info[0].framerate == 0) {
		printk("Set framerate fail\n");
		return;
	}
	candidate = 0;
	for (i=1; i<3; i++) {
		if (info[i].framerate == 0)
			break;
		if ((info[i].framerate - framerate) < (info[i-1].framerate - framerate)) {
			candidate = i;
		}
	}

	/* set lcd clk */
	gpHalClkLcdSetSrc(info[candidate].clk_src);
	// Set lcd clock rate, must enable before lcd clk on
	ret = gp_clk_set_rate("clk_lcd", clk_target, &real_freq, NULL);	// must enable before lcd clk on
}
EXPORT_SYMBOL(gpHalDispSetFrameRate);

SINT32
gpHalDispGetFrameRate(
	void
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	int devType = gpHalDispGetDevType();
	int width, height;
	int real_freq;
	int framerate;

	if (devType != HAL_DISP_DEV_LCD) {
		return -1;
	}

	width = ((pdispReg->dispRes >> 16) & 0xFFFF) + 
		((pdispReg->dispLcdHsync >> 18) & 0x3FF) + 
		((pdispReg->dispLcdHsync >> 8) & 0x3FF);
	height = (pdispReg->dispRes & 0xFFFF) + 
		((pdispReg->dispLcdVsync >> 18) & 0x3FF) + 
		((pdispReg->dispLcdVsync >> 8) & 0x3FF);

	gp_clk_get_rate((int*)"clk_lcd", &real_freq);
	framerate = real_freq / (width * height);
	return framerate;
}
EXPORT_SYMBOL(gpHalDispGetFrameRate);

/* Primary layer */
void
gpHalDispSetPriEnable(
	UINT32 enable
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 devType;
	UINT32 regVal;

	DEBUG("[%s:%d], enable=%d\n", __FUNCTION__, __LINE__, enable);

	if (enable) {
		regVal = pdispReg->dispCtrl;
		regVal &= ~(0x3 << 30);

		devType = gpHalDispGetDevType();
		if (devType == HAL_DISP_DEV_LCM)
			regVal |= (1 << 30);
		else if (devType == HAL_DISP_DEV_LCD)
			regVal |= (2 << 30);
		else if (devType == HAL_DISP_DEV_TV)
			regVal |= (3 << 30);

		pdispReg->dispCtrl = regVal;
	}
	else {
		pdispReg->dispCtrl &= ~(0x3 << 30);
	}

}
EXPORT_SYMBOL(gpHalDispSetPriEnable);

UINT32
gpHalDispGetPriEnable(
	void
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	return (pdispReg->dispCtrl & (0x3 << 30)) >> 30;
}
EXPORT_SYMBOL(gpHalDispGetPriEnable);

void
gpHalDispSetPriBlank(
	gpHalDispBlankInfo_t blankInfo
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	regVal = (blankInfo.top << 16) | (blankInfo.bottom);
	pdispReg->dispVBlank = regVal;

	if (gpHalDispGetDevType() == HAL_DISP_DEV_LCD)
		regVal = (blankInfo.left << 16) | (blankInfo.right + HAL_DISP_DEFAULT_BLANK);
	else
		regVal = (blankInfo.left << 16) | (blankInfo.right);
	pdispReg->dispHBlank = regVal;

	pdispReg->dispBlankData = blankInfo.pattern;
}
EXPORT_SYMBOL(gpHalDispSetPriBlank);

void
gpHalDispSetPriFrameAddr(
	UINT32 addr
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
	
	pdispReg->dispPriAddr = addr;

}
EXPORT_SYMBOL(gpHalDispSetPriFrameAddr);

void
gpHalDispSetPriPitch(
	UINT16 src,
	UINT16 act
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
	
	pdispReg->dispPriPitch = (src << 16) | act;

}
EXPORT_SYMBOL(gpHalDispSetPriPitch);

void
gpHalDispSetPriRes(
	UINT16 width,
	UINT16 height
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
	
	pdispReg->dispPriRes = (width << 16) | height;

}
EXPORT_SYMBOL(gpHalDispSetPriRes);

void
gpHalDispSetPriSclInfo(
	gpHalDispSclInfo_t scale
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;
	UINT16 factor;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	/* Clear scale control register (0x028) */
	regVal = pdispReg->dispPriSclCtrl;
	regVal &= ~((0xFFFF << 16) | (0x1 << 2) | (0x1)); /* Clear vinit1[31:16], v select[2], h select[0] */

	/* Set scale resolution */
	pdispReg->dispPriSclRes = (scale.dstWidth << 16) | scale.dstHeight;

	/* Calculate & set h factor, h scale up/down flag(0x028) bit[0] */
	factor = 0;
	if (scale.dstWidth != scale.srcWidth) {
		if (scale.dstWidth > scale.srcWidth) {
			factor = (scale.srcWidth << 16) / scale.dstWidth;

			/* No need to update regVal (0x028) bit[0], since the flag of h scaling up is 0*/
		}
		else {
			factor = ((scale.dstWidth << 16) + (scale.srcWidth - 1)) / scale.srcWidth;
			
			/* Update regVal (0x028) bit[0], scaling down */
			regVal |= 0x01;
		}
	}
	DEBUG("[%s:%d], hfactor=%d, scl=%d\n", __FUNCTION__, __LINE__, factor, regVal & 0x01);
	pdispReg->dispPriSclHParam = (scale.hInit << 16) | factor;


	/* Calculate & set v factor, v scale up/down flag(0x028) bit[2] */
	factor = 0;
	if (scale.dstHeight != scale.srcHeight) {
		if (scale.dstHeight > scale.srcHeight) {
			factor = (scale.srcHeight << 16) / scale.dstHeight;
			
			/* No need to update regVal (0x028) bit[2], since the flag of v scaling up is 0*/
		}
		else {
			factor = ((scale.dstHeight << 16) + (scale.srcHeight - 1)) / scale.srcHeight;
			
			/* Update regVal (0x028) bit[2], scaling down */
			regVal |= 0x04;
		}
	}
	DEBUG("[%s:%d], vfactor=%d, scl=%d\n", __FUNCTION__, __LINE__, factor, regVal & 0x04);
	pdispReg->dispPriSclVParam = (scale.vInit0 << 16) | factor;

	/* Set v init1 */
	regVal |= scale.vInit1;
	pdispReg->dispPriSclCtrl = regVal;

}
EXPORT_SYMBOL(gpHalDispSetPriSclInfo);

void
gpHalDispSetPriSclEnable(
	UINT32 hEnable,
	UINT32 vEnable
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);

	DEBUG("[%s:%d], h=%d, v=%d\n", __FUNCTION__, __LINE__, hEnable, vEnable);

	if (hEnable)
		pdispReg->dispPriSclCtrl |= 0x2;
	else
		pdispReg->dispPriSclCtrl &= ~0x2;

	if (vEnable)
		pdispReg->dispPriSclCtrl |= 0x8;
	else
		pdispReg->dispPriSclCtrl &= ~0x8;

}
EXPORT_SYMBOL(gpHalDispSetPriSclEnable);

void
gpHalDispSetPriInputInfo(
	UINT32 format,
	UINT32 type
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	regVal = pdispReg->dispFmt;
	regVal &= ~((0x1 << 2) | (0x3));
	regVal |= (format << 2);
	regVal |= type;
	pdispReg->dispFmt = regVal;

}
EXPORT_SYMBOL(gpHalDispSetPriInputInfo);

void
gpHalDispSetPriBurst(
	UINT32 burst
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	regVal = pdispReg->dispCtrl;
	regVal &= ~(0x3 << 24);
	regVal |= (burst << 24);
	pdispReg->dispCtrl |= regVal;

}
EXPORT_SYMBOL(gpHalDispSetPriBurst);

void
gpHalDispSetPriDmaType(
	UINT32 type
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	regVal = pdispReg->dispCtrl;
	regVal &= ~(0x1 << 28);
	regVal |= (type << 28);
	pdispReg->dispCtrl = regVal;

}
EXPORT_SYMBOL(gpHalDispSetPriDmaType);


/* Dither */
void
gpHalDispSetDitherType(
	UINT32 type
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	DEBUG("[%s:%d], type=%d\n", __FUNCTION__, __LINE__, type);

	regVal = pdispReg->dispFmt;
	regVal &= ~(0x3 << 12);
	regVal |= (type << 12);
	pdispReg->dispFmt = regVal;

}
EXPORT_SYMBOL(gpHalDispSetDitherType);

/* Flip */
void
gpHalDispSetFlip(
	UINT32 value
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);

	DEBUG("[%s:%d], type=%d\n", __FUNCTION__, __LINE__, value);

	pdispReg->rsv098[0] = value;

}
EXPORT_SYMBOL(gpHalDispSetFlip);

UINT32
gpHalDispGetDitherType(
	void
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	return (pdispReg->dispFmt >> 12) & 0x03;
}
EXPORT_SYMBOL(gpHalDispGetDitherType);

void
gpHalDispSetDitherMap(
	UINT32 map0,
	UINT32 map1
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);

	DEBUG("[%s:%d], %d, %d\n", __FUNCTION__, __LINE__, map0, map1);

	pdispReg->dispDitherMap0 = map0;
	pdispReg->dispDitherMap1 = map1;

}
EXPORT_SYMBOL(gpHalDispSetDitherMap);

void
gpHalDispGetDitherMap(
	UINT32 *map0,
	UINT32 *map1
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	*map0 = pdispReg->dispDitherMap0;
	*map1 = pdispReg->dispDitherMap1;

}
EXPORT_SYMBOL(gpHalDispGetDitherMap);

void
gpHalDispSetDitherEnable(
	UINT32 enable
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);

	DEBUG("[%s:%d], %d\n", __FUNCTION__, __LINE__, enable);

	if (enable)
		pdispReg->dispFmt |= (0x1 << 15);
	else
		pdispReg->dispFmt &= ~(0x1 << 15);

}
EXPORT_SYMBOL(gpHalDispSetDitherEnable);

UINT32
gpHalDispGetDitherEnable(
	void
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	return ((pdispReg->dispFmt >> 15) & 0x1);
}
EXPORT_SYMBOL(gpHalDispGetDitherEnable);


/* Color Matrix */
void
gpHalDispSetColorMatrix(
	UINT16 *matrix
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
	
	pdispReg->dispCMatrix0 = matrix[0] | (matrix[1] << 16);
	pdispReg->dispCMatrix1 = matrix[2] | (matrix[3] << 16);
	pdispReg->dispCMatrix2 = matrix[4] | (matrix[5] << 16);
	pdispReg->dispCMatrix3 = matrix[6] | (matrix[7] << 16);
	pdispReg->dispCMatrix4 = matrix[8] | (matrix[9] << 16);
	pdispReg->dispCMatrix5 = matrix[10] | (matrix[11] << 16);

}
EXPORT_SYMBOL(gpHalDispSetColorMatrix);


/* Gamma */
void
gpHalDispSetGammaTable(
	UINT32 id,
	UINT8 *table
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal, i;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	/* Set gamma bank */
	regVal = pdispReg->dispIrqSrc;
	regVal &= ~((0x3 << 16) | (0x1F));
	regVal |= (id << 16);
	pdispReg->dispIrqSrc = regVal;

	for(i=0; i<256; ++i) {
		pdispReg->dispGammaPtr[i] = (UINT32) table[i];
	}

}
EXPORT_SYMBOL(gpHalDispSetGammaTable);

void
gpHalDispSetGammaEnable(
	UINT32 enable
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);

	DEBUG("[%s:%d], %d\n", __FUNCTION__, __LINE__, enable);

	if (enable)
		pdispReg->dispFmt |= (0x1 << 9);
	else
		pdispReg->dispFmt &= ~(0x1 << 9);

}
EXPORT_SYMBOL(gpHalDispSetGammaEnable);

UINT32
gpHalDispGetGammaEnable(
	void
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	return ((pdispReg->dispFmt >> 9) & 0x1);
}
EXPORT_SYMBOL(gpHalDispGetGammaEnable);


/* Osd layer */
void
gpHalDispSetOsdEnable(
	UINT32 layerNum,
	UINT32 enable
)
{
	DEBUG("[%s:%d], osd %d enable=%d\n", __FUNCTION__, __LINE__, layerNum, enable);

	if (enable) {
		*osdRegArray[DISP_OSD_CTRL][layerNum] |= (0x1 << 31);
	}
	else {
		*osdRegArray[DISP_OSD_CTRL][layerNum] &= ~(0x1 << 31);
	}

}
EXPORT_SYMBOL(gpHalDispSetOsdEnable);

UINT32
gpHalDispGetOsdEnable(
	UINT32 layerNum
)
{
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	return (*osdRegArray[DISP_OSD_CTRL][layerNum] & (0x1 << 31)) >> 31;
}
EXPORT_SYMBOL(gpHalDispGetOsdEnable);

void
gpHalDispSetOsdXY(
	UINT32 layerNum,
	UINT16 x,
	UINT16 y
)
{
	UINT32 regVal;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	regVal = (x << 16) | (y);
	*osdRegArray[DISP_OSD_XY][layerNum] = regVal;
}
EXPORT_SYMBOL(gpHalDispSetOsdXY);

void
gpHalDispSetOsdFrameAddr(
	UINT32 layerNum,
	UINT32 addr
)
{
	DEBUG("[%s:%d] %d, 0x%x\n", __FUNCTION__, __LINE__, layerNum, addr);
	
	*osdRegArray[DISP_OSD_ADDR][layerNum] = addr;
}
EXPORT_SYMBOL(gpHalDispSetOsdFrameAddr);

void
gpHalDispSetOsdPitch(
	UINT32 layerNum,
	UINT16 src,
	UINT16 act
)
{
	DEBUG("[%s:%d] %d\n", __FUNCTION__, __LINE__, layerNum);
	
	*osdRegArray[DISP_OSD_PITCH][layerNum] = (src << 16) | act;
}
EXPORT_SYMBOL(gpHalDispSetOsdPitch);

void
gpHalDispSetOsdRes(
	UINT32 layerNum,
	UINT16 width,
	UINT16 height
)
{
	DEBUG("[%s:%d] %d\n", __FUNCTION__, __LINE__, layerNum);

	*osdRegArray[DISP_OSD_RES][layerNum] = (width << 16) | height;
}
EXPORT_SYMBOL(gpHalDispSetOsdRes);

void
gpHalDispSetOsdSclInfo(
	UINT32 layerNum,
	gpHalDispSclInfo_t scale
)
{
	UINT16 factor;

	DEBUG("[%s:%d] %d\n", __FUNCTION__, __LINE__, layerNum);

	/* Set scale resolution */
	*osdRegArray[DISP_OSD_SCLRES][layerNum] = (scale.dstWidth << 16) | scale.dstHeight;

	/* Calculate & set h factor / initial value */
	factor = 0;
	if (scale.dstWidth != scale.srcWidth) {
		factor = ((scale.srcWidth - 1) << 11) / (scale.dstWidth - 1);
	}
	DEBUG("[%s:%d], hfactor=%d\n", __FUNCTION__, __LINE__, factor);
	*osdRegArray[DISP_OSD_HPARAM][layerNum] = (scale.hInit << 16) | factor;

	/* Calculate & set v factor / initial value */
	factor = 0;
	if (scale.dstHeight != scale.srcHeight) {
		if (gDispDevType == HAL_DISP_DEV_TV)
			factor = ((scale.srcHeight - 1) << 12) / (scale.dstHeight - 1);
		else
			factor = ((scale.srcHeight - 1) << 11) / (scale.dstHeight - 1);
	}
	DEBUG("[%s:%d], vfactor=%d\n", __FUNCTION__, __LINE__, factor);
	*osdRegArray[DISP_OSD_VPARAM0][layerNum] = (scale.vInit0 << 16) | factor;
	*osdRegArray[DISP_OSD_VPARAM1][layerNum] = scale.vInit1 << 16;

}
EXPORT_SYMBOL(gpHalDispSetOsdSclInfo);

void
gpHalDispSetOsdSclEnable(
	UINT32 layerNum,
	UINT32 hEnable,
	UINT32 vEnable
)
{
	DEBUG("[%s:%d] %d, h=%d, v=%d\n", __FUNCTION__, __LINE__, layerNum, hEnable, vEnable);

	if (hEnable)
		*osdRegArray[DISP_OSD_CTRL][layerNum] |= (0x1 << 17);
	else
		*osdRegArray[DISP_OSD_CTRL][layerNum] &= ~(0x1 << 17);

	if (vEnable)
		*osdRegArray[DISP_OSD_CTRL][layerNum] |= (0x1 << 16);
	else
		*osdRegArray[DISP_OSD_CTRL][layerNum] &= ~(0x1 << 16);

}
EXPORT_SYMBOL(gpHalDispSetOsdSclEnable);

void
gpHalDispSetOsdInputFmt(
	UINT32 layerNum,
	UINT32 format
)
{
	UINT32 regVal;

	DEBUG("[%s:%d] %d, fmt=%d\n", __FUNCTION__, __LINE__, layerNum, format);

	regVal = *osdRegArray[DISP_OSD_FMT][layerNum];
	regVal &= ~(0x3 << 30);
	regVal |= (format << 30);
	*osdRegArray[DISP_OSD_FMT][layerNum] = regVal;
}
EXPORT_SYMBOL(gpHalDispSetOsdInputFmt);

void
gpHalDispSetOsdInputType(
	UINT32 layerNum,
	UINT32 type
)
{
	UINT32 regVal;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	regVal = *osdRegArray[DISP_OSD_CTRL][layerNum];
	regVal &= ~(0x3 << 12);
	regVal |= (type << 12);
	*osdRegArray[DISP_OSD_CTRL][layerNum] = regVal;
}
EXPORT_SYMBOL(gpHalDispSetOsdInputType);

void
gpHalDispSetOsdBurst(
	UINT32 layerNum,
	UINT32 burst
)
{
	UINT32 regVal;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	regVal = *osdRegArray[DISP_OSD_CTRL][layerNum];
	regVal &= ~(0x3 << 28);
	regVal |= (burst << 28);
	*osdRegArray[DISP_OSD_CTRL][layerNum] = regVal;

}
EXPORT_SYMBOL(gpHalDispSetOsdBurst);

void
gpHalDispSetOsdAlpha(
	UINT32 layerNum,
	UINT32 consta,
	UINT32 ppamd,
	UINT32 alpha
)
{
	UINT32 regVal;
	UINT32 newAlpha;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	if (alpha > 100)
		alpha = 100;
	newAlpha = (0x40 * alpha) / 100;

	regVal = *osdRegArray[DISP_OSD_FMT][layerNum];
	regVal &= ~((0x1 << 27) | (0x3 << 24) | (0xff << 16));
	regVal |= ((consta << 27) | (ppamd << 24) | (newAlpha << 16));
	*osdRegArray[DISP_OSD_FMT][layerNum] = regVal;

}
EXPORT_SYMBOL(gpHalDispSetOsdAlpha);

void
gpHalDispSetOsdColorKey(
	UINT32 layerNum,
	UINT32 color
)
{
	UINT32 regVal;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	regVal = *osdRegArray[DISP_OSD_FMT][layerNum];
	regVal &= ~(0xffff);
	regVal |= (color & 0xffff);
	*osdRegArray[DISP_OSD_FMT][layerNum] = regVal;

}
EXPORT_SYMBOL(gpHalDispSetOsdColorKey);

void
gpHalDispSetOsdPalette(
	UINT32 layerNum,
	UINT32 startIndex,
	UINT32 count,
	UINT32 *pColorTable
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 maxIndex;
	UINT32 i;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	maxIndex = startIndex + count;

	if (layerNum == HAL_DISP_OSD_0) {
		pdispReg->dispIrqSrc &= ~(0x1 << 20);

		/* The max palette index of osd0 = 256 */
		if (maxIndex > 256)
			maxIndex = 256;
	}
	else if (layerNum == HAL_DISP_OSD_1) {
		pdispReg->dispIrqSrc |= (0x1 << 20);

		/* The max palette index of osd1 = 16 */
		if (maxIndex > 16)
			maxIndex = 16;
	}
	else {
		DERROR("[%s:%d] Error osd number %d\n", __FUNCTION__, __LINE__, layerNum);
		return;
	}

	DEBUG("start index=%d, max index=%d\n", startIndex, maxIndex);
	for (i=startIndex; i<maxIndex; i++)
		pdispReg->dispPalettePtr[i] = pColorTable[i - startIndex];
		
}
EXPORT_SYMBOL(gpHalDispSetOsdPalette);

void
gpHalDispSetOsdPaletteOffset(
	UINT32 layerNum,
	UINT32 offset
)
{
	UINT32 regVal;

	regVal = *osdRegArray[DISP_OSD_CTRL][layerNum];
	regVal &= ~(0xff);
	regVal |= offset;
	*osdRegArray[DISP_OSD_CTRL][layerNum] = regVal;

}
EXPORT_SYMBOL(gpHalDispSetOsdPaletteOffset);

UINT32
gpHalDispGetOsdPaletteOffset(
	UINT32 layerNum
)
{
	return (*osdRegArray[DISP_OSD_CTRL][layerNum]) & 0xff;
}
EXPORT_SYMBOL(gpHalDispGetOsdPaletteOffset);

void
gpHalDispSetOsdDmaType(
	UINT32 layerNum,
	UINT32 type
)
{
	UINT32 regVal;

	regVal = *osdRegArray[DISP_OSD_CTRL][layerNum];
	regVal &= ~(0x1 << 30);
	regVal |= (type << 30);
	*osdRegArray[DISP_OSD_CTRL][layerNum] = regVal;
}
EXPORT_SYMBOL(gpHalDispSetOsdDmaType);


/* Lcd panel */
void
gpHalDispSetRes(
	UINT16 width,
	UINT16 height
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	if (gpHalDispGetDevType() == HAL_DISP_DEV_LCD)
		pdispReg->dispRes = ((width + HAL_DISP_DEFAULT_BLANK) << 16) | (height);
	else
		pdispReg->dispRes = (width << 16) | (height);
	DEBUG("[%s:%d], w=%d, h=%d, regVal=0x%x\n", __FUNCTION__, __LINE__, width, height, pdispReg->dispRes);

}
EXPORT_SYMBOL(gpHalDispSetRes);

void
gpHalDispGetRes(
	UINT16 *width,
	UINT16 *height
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	regVal = pdispReg->dispRes;
	if (gpHalDispGetDevType() == HAL_DISP_DEV_LCD)
		*width = ((regVal >> 16) - HAL_DISP_DEFAULT_BLANK)& 0xfff;
	else
		*width = (regVal >> 16)& 0xfff;
	*height = (regVal) & 0xfff;
	DEBUG("[%s:%d], w=%d, h=%d, regVal=0x%x\n", __FUNCTION__, __LINE__, *width, *height, regVal);

}
EXPORT_SYMBOL(gpHalDispGetRes);

void
gpHalDispSetLcdVsync(
	gpHalDispLcdTiming_t vsync
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	DEBUG("[%s:%d], p=%d, fPorch=%d, bPorch=%d, w=%d\n", __FUNCTION__, __LINE__, vsync.polarity, vsync.fPorch, vsync.bPorch, vsync.width);
	pdispReg->dispLcdVsync = (vsync.polarity << 28) | (vsync.fPorch << 18) | (vsync.bPorch << 8) | (vsync.width);

}
EXPORT_SYMBOL(gpHalDispSetLcdVsync);

void
gpHalDispGetLcdVsync(
	gpHalDispLcdTiming_t *vsync
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	regVal = pdispReg->dispLcdVsync;
	vsync->polarity = (regVal >> 28) & 0x01;
	vsync->fPorch = (regVal >> 18) & 0x3ff;
	vsync->bPorch = (regVal >> 8)  & 0x3ff;
	vsync->width = regVal & 0xff;

}
EXPORT_SYMBOL(gpHalDispGetLcdVsync);

void
gpHalDispSetLcdHsync(
	gpHalDispLcdTiming_t hsync
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	DEBUG("[%s:%d], p=%d, fPorch=%d, bPorch=%d, w=%d\n", __FUNCTION__, __LINE__, hsync.polarity, hsync.fPorch, hsync.bPorch, hsync.width);
	pdispReg->dispLcdHsync = (hsync.polarity << 28) | (hsync.fPorch << 18) | (hsync.bPorch << 8) | (hsync.width);

}
EXPORT_SYMBOL(gpHalDispSetLcdHsync);

void
gpHalDispGetLcdHsync(
	gpHalDispLcdTiming_t *hsync
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	regVal = pdispReg->dispLcdHsync;
	hsync->polarity = (regVal >> 28) & 0x01;
	hsync->fPorch = (regVal >> 18) & 0x3ff;
	hsync->bPorch = (regVal >> 8)  & 0x3ff;
	hsync->width = regVal & 0xff;

}
EXPORT_SYMBOL(gpHalDispGetLcdHsync);

void
gpHalDispSetPanelFormat(
	UINT32 format,
	UINT32 type,
	UINT32 seq0,
	UINT32 seq1
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	regVal = pdispReg->dispFmt;
	regVal &= ~((0x3 << 28) | (0x3 << 24) | (0x7 << 20) | (0x7 << 16));
	regVal |= ((format << 28) | (type << 24) | (seq0 << 20) | (seq1 << 16));
	pdispReg->dispFmt = regVal;

}
EXPORT_SYMBOL(gpHalDispSetPanelFormat);

void
gpHalDispSetClkPolarity(
	UINT32 polarity
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	regVal = pdispReg->dispCtrl;
	regVal &= ~(0x1 << 23);
	regVal |= (polarity << 23);
	pdispReg->dispCtrl = regVal;

}
EXPORT_SYMBOL(gpHalDispSetClkPolarity);


/* Lcm panel */
void
gpHalDispSetLcmInterface(
	UINT32 interface
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	DEBUG("[%s:%d], %d\n", __FUNCTION__, __LINE__, interface);

	regVal = pdispReg->dispCtrl;
	regVal &= ~(0x1 << 14);
	regVal |= (interface << 14);
	pdispReg->dispCtrl = regVal;

}
EXPORT_SYMBOL(gpHalDispSetLcmInterface);

void
gpHalDispSetLcmMode(
	UINT32 mode
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	DEBUG("[%s:%d], %d\n", __FUNCTION__, __LINE__, mode);

	regVal = pdispReg->dispCtrl;
	regVal &= ~(0x1 << 13);
	regVal |= (mode << 13);
	pdispReg->dispCtrl = regVal;

}
EXPORT_SYMBOL(gpHalDispSetLcmMode);

void
gpHalDispSetLcmDataSelect(
	UINT32 sel
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	DEBUG("[%s:%d], %d\n", __FUNCTION__, __LINE__, sel);

	regVal = pdispReg->dispCtrl;
	regVal &= ~(0x1 << 12);
	regVal |= (sel << 12);
	pdispReg->dispCtrl = regVal;

}
EXPORT_SYMBOL(gpHalDispSetLcmDataSelect);

void
gpHalDispSetLcmAcTiming(
	gpHalDispLcmTiming_t timing
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	pdispReg->dispLcmAcTiming = (timing.addrSetup << 28) |
								(timing.addrHold << 16) |
								(timing.csSetup << 12) |
								(timing.csHold << 8) |
								(timing.cycLength << 0);
}
EXPORT_SYMBOL(gpHalDispSetLcmAcTiming);

void
gpHalDispSetLcmSeqSel(
	UINT32 sel
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	DEBUG("[%s:%d], %d\n", __FUNCTION__, __LINE__, sel);

	regVal = pdispReg->dispFmt;
	regVal &= ~(0x1 << 11);
	regVal |= (sel << 11);
	pdispReg->dispFmt = regVal;

}
EXPORT_SYMBOL(gpHalDispSetLcmSeqSel);

/* Tv */
void
gpHalDispSetTvType(
	UINT32 type
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	regVal = pdispReg->dispCtrl;
	regVal &= ~(0x1 << 11);
	regVal |= (type << 11);
	pdispReg->dispCtrl = regVal;

}
EXPORT_SYMBOL(gpHalDispSetTvType);

void
gpHalDispSetTvPulse(
	UINT32 pulse
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	regVal = pdispReg->dispCtrl;
	regVal &= ~(0x1 << 10);
	regVal |= (pulse << 10);
	pdispReg->dispCtrl = regVal;

}
EXPORT_SYMBOL(gpHalDispSetTvPulse);

void
gpHalDispSetTvScan(
	UINT32 scan
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	regVal = pdispReg->dispCtrl;
	regVal &= ~(0x3 << 8);
	regVal |= (scan << 8);
	pdispReg->dispCtrl = regVal;

}
EXPORT_SYMBOL(gpHalDispSetTvScan);

void
gpHalDispSetTvFscType(
	UINT32 fsc
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	regVal = pdispReg->dispCtrl;
	regVal &= ~(0x7 << 5);
	regVal |= (fsc << 5);
	pdispReg->dispCtrl = regVal;

}
EXPORT_SYMBOL(gpHalDispSetTvFscType);

void
gpHalDispSetTvFix625(
	UINT32 fix
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	regVal = pdispReg->dispCtrl;
	regVal &= ~(0x1 << 4);
	regVal |= (fix << 4);
	pdispReg->dispCtrl = regVal;

}
EXPORT_SYMBOL(gpHalDispSetTvFix625);

void
gpHalDispSetTvLine(
	UINT32 line
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	regVal = pdispReg->dispCtrl;
	regVal &= ~(0x1 << 3);
	regVal |= (line << 3);
	pdispReg->dispCtrl = regVal;

}
EXPORT_SYMBOL(gpHalDispSetTvLine);

void
gpHalDispSetTvColorBurstWidth(
	UINT32 width
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	regVal = pdispReg->dispCtrl;
	regVal &= ~(0x1 << 2);
	regVal |= (width << 2);
	pdispReg->dispCtrl = regVal;

}
EXPORT_SYMBOL(gpHalDispSetTvColorBurstWidth);

void
gpHalDispSetTvColorBurstSel(
	UINT32 sel
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	regVal = pdispReg->dispCtrl;
	regVal &= ~(0x3 << 0);
	regVal |= (sel << 0);
	pdispReg->dispCtrl = regVal;

}
EXPORT_SYMBOL(gpHalDispSetTvColorBurstSel);

void
gpHalDispSetTvCftType(
	UINT32 type
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	regVal = pdispReg->dispFmt;
	regVal &= ~(0x1 << 8);
	regVal |= (type << 8);
	pdispReg->dispFmt = regVal;

}
EXPORT_SYMBOL(gpHalDispSetTvCftType);

void
gpHalDispSetTvCupType(
	UINT32 type
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	regVal = pdispReg->dispFmt;
	regVal &= ~(0x3 << 6);
	regVal |= (type << 6);
	pdispReg->dispFmt = regVal;

}
EXPORT_SYMBOL(gpHalDispSetTvCupType);

void
gpHalDispSetTvYupType(
	UINT32 type
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	regVal = pdispReg->dispFmt;
	regVal &= ~(0x3 << 4);
	regVal |= (type << 4);
	pdispReg->dispFmt = regVal;

}
EXPORT_SYMBOL(gpHalDispSetTvYupType);

void
gpHalDispSetTvAmpAdj(
	gpHalDispTvAmpAdj_t amp
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	pdispReg->dispTvAmp = (amp.luminance << 20) | (amp.blank << 10) | (amp.burst);

}
EXPORT_SYMBOL(gpHalDispSetTvAmpAdj);

void
gpHalDispSetTvPosAdj(
	gpHalDispTvPosAdj_t pos
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	pdispReg->dispTvPos = (pos.vAct0 << 16) | (pos.vAct1 << 8) | (pos.hAct);

}
EXPORT_SYMBOL(gpHalDispSetTvPosAdj);



/* Color bar */
void
gpHalDispSetColorBarEnable(
	UINT32 enable
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	if (enable) {
		pdispReg->dispCBarCtrl |= (0x1 << 31);
	}
	else {
		pdispReg->dispCBarCtrl &= ~(0x1 << 31);
	}

}
EXPORT_SYMBOL(gpHalDispSetColorBarEnable);

UINT32
gpHalDispGetColorBarEnable(
	void
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	return (pdispReg->dispCBarCtrl & (0x1 << 31)) >> 31;
}
EXPORT_SYMBOL(gpHalDispGetColorBarEnable);

void
gpHalDispSetColorBar(
	UINT32 type,
	UINT32 size,
	UINT32 color
)
{
	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	UINT32 regVal;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	regVal = pdispReg->dispCBarCtrl;

	regVal &= ~((0xf << 8) | (0xFF));
	regVal |= ((type << 8) | (size));
	pdispReg->dispCBarCtrl = regVal;

	pdispReg->dispCBar = color;
}
EXPORT_SYMBOL(gpHalDispSetColorBar);

void
gpHalDispDumpRegister(
	void
)
{
	UINT32 *pAddr = (UINT32 *)(LOGI_ADDR_DISP_REG);
	UINT32 i;

	for (i=0; i<64; i++) {
		if (i%8 == 0) {
			printk("\n");
			printk("addr = 0x%08x\t", 0x93000000+i*4);
		}
		printk("0x%08x,  ", (UINT32) pAddr[i]);
	}
	printk("\n");

	/* Clock */
	printk("SCUA_A_PERI_RST=0x%x\n", SCUA_A_PERI_RST);
	printk("SCUA_A_PERI_CLKEN=0x%x\n", SCUA_A_PERI_CLKEN);
	printk("SCUA_LCD_TYPE_SEL=0x%x\n", SCUA_LCD_TYPE_SEL);
	printk("SCUA_LCD_CLK_CFG=0x%x\n", SCUA_LCD_CLK_CFG);
	printk("SCUB_B_PERI_CLKEN=0x%x\n", SCUB_B_PERI_CLKEN);
	printk("SCUB_PGS0=0x%x\n", SCUB_PGS0);
	printk("SCUA_SAR_GPIO_CTRL=0x%x\n", SCUA_SAR_GPIO_CTRL);

}
EXPORT_SYMBOL(gpHalDispDumpRegister);

void
gpHalDispSetSSC(
	UINT32 enable,
	UINT32 mode,
	UINT32 stage
)
{
	UINT32 value = 0, temp;

	temp = ( enable & 0x01 );
	value |= temp;
	value <<= 1;

	temp = ( mode & 0x01 );
	value |= temp;
	value <<= 2;

	temp = ( stage & 0x03 );
	value |= temp;
	
	SCUA_DUMMY2 = value;

}
EXPORT_SYMBOL(gpHalDispSetSSC);

void
gpHalDispGetSSC(
	UINT32 *enable,
	UINT32 *mode,
	UINT32 *stage
)
{
	UINT32 value;

	value = SCUA_DUMMY2;

	*stage = ( value & 0x03 );
	value >>= 2;

	*mode = ( value & 0x01 );
	value >>= 1;

	*enable = ( value & 0x01 );

}
EXPORT_SYMBOL(gpHalDispGetSSC);


