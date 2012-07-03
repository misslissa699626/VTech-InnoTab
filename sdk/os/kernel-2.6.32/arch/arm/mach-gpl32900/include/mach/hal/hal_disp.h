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
#ifndef _HAL_DISP_H_
#define _HAL_DISP_H_

/**************************************************************************
 *                         H E A D E R   F I L E S                        *
 **************************************************************************/
#include <mach/typedef.h>


/**************************************************************************
*                           C O N S T A N T S                             *
 **************************************************************************/
enum {
	HAL_DISP_DEV_LCD = 0,
	HAL_DISP_DEV_LCM,
	HAL_DISP_DEV_TV,
	HAL_DISP_DEV_MAX,
};

enum {
	HAL_DISP_CLK_TYPE_RGB888 = 0,
	HAL_DISP_CLK_TYPE_RGB565 = 0,
	HAL_DISP_CLK_TYPE_RGB666 = 1,
	HAL_DISP_CLK_TYPE_LCM = 2,
	HAL_DISP_CLK_TYPE_YPBPR = 3,
};

enum {
	HAL_DISP_INT_DISPLAY_OFF = 0x01,
	HAL_DISP_INT_FRAME_END = 0x04,
	HAL_DISP_INT_FIELD_END = 0x8,
	HAL_DISP_INT_UPDATE_FAIL = 0x10,
};

enum {
	HAL_DISP_INPUT_FMT_RGB = 0,
	HAL_DISP_INPUT_FMT_YCbCr = 1,
};

enum {
	HAL_DISP_INPUT_TYPE_RGB565 = 0,
	HAL_DISP_INPUT_TYPE_RGB555 = 1,
	HAL_DISP_INPUT_TYPE_RGB888 = 2,

	HAL_DISP_INPUT_TYPE_YCbYCr = 0,
	HAL_DISP_INPUT_TYPE_4Y4Cb4Y4Cr = 1,
	HAL_DISP_INPUT_TYPE_YCbCr = 2,
};

enum {
	/* LCD Output Format */
	HAL_DISP_OUTPUT_FMT_RGB = 0,
	HAL_DISP_OUTPUT_FMT_YCbCr = 1,
	HAL_DISP_OUTPUT_FMT_YUV = 2,
};

enum {
	/* LCD Output Type */
	HAL_DISP_OUTPUT_TYPE_PRGB888 = 0,
	HAL_DISP_OUTPUT_TYPE_PRGB565 = 1,
	HAL_DISP_OUTPUT_TYPE_SRGB888 = 2,
	HAL_DISP_OUTPUT_TYPE_SRGBM888 = 3,

	HAL_DISP_OUTPUT_TYPE_YCbCr24 = 0,
	HAL_DISP_OUTPUT_TYPE_YCbCr16 = 1,
	HAL_DISP_OUTPUT_TYPE_YCbCr8 = 2,

	HAL_DISP_OUTPUT_TYPE_YUV24 = 0,
	HAL_DISP_OUTPUT_TYPE_YUV16 = 1,
	HAL_DISP_OUTPUT_TYPE_YUV8 = 2,

	/* LCM Output Type */
	HAL_DISP_OUTPUT_TYPE_RGB666 = 0,
	HAL_DISP_OUTPUT_TYPE_RGB565 = 1,
	HAL_DISP_OUTPUT_TYPE_RGB444 = 2,
	HAL_DISP_OUTPUT_TYPE_RGB332 = 3,
};

enum {
	HAL_DISP_PRGB888_RGB = 0,
	HAL_DISP_PRGB888_BGR = 1,

	HAL_DISP_PRGB565_RGB = 0,
	HAL_DISP_PRGB565_BGR = 1,

	HAL_DISP_SRGB888_RGB = 0,
	HAL_DISP_SRGB888_GBR = 1,
	HAL_DISP_SRGB888_BRG = 2,
	HAL_DISP_SRGB888_RBG = 3,
	HAL_DISP_SRGB888_BGR = 4,
	HAL_DISP_SRGB888_GRB = 5,

	HAL_DISP_SRGBM888_RGBM = 0,
	HAL_DISP_SRGBM888_GBRM = 1,
	HAL_DISP_SRGBM888_BRGM = 2,
	HAL_DISP_SRGBM888_RBGM = 3,
	HAL_DISP_SRGBM888_BGRM = 4,
	HAL_DISP_SRGBM888_GRBM = 5,

	HAL_DISP_YCBCR24_YCbCr = 0,
	HAL_DISP_YCBCR24_YCrCb = 1,
	HAL_DISP_YCBCR24_CbYCr = 2,
	HAL_DISP_YCBCR24_CrYCb = 3,
	HAL_DISP_YCBCR24_CbCrY = 4,
	HAL_DISP_YCBCR24_CrCbY = 5,

	HAL_DISP_YCBCR16_YCbYCr = 0,
	HAL_DISP_YCBCR16_YCrYCb = 1,
	HAL_DISP_YCBCR16_CbYCrY = 2,
	HAL_DISP_YCBCR16_CrYCbY = 3,

	HAL_DISP_YCBCR8_YCbYCr = 0,
	HAL_DISP_YCBCR8_YCrYCb = 1,
	HAL_DISP_YCBCR8_CbYCrY = 2,
	HAL_DISP_YCBCR8_CrYCbY = 3,

	HAL_DISP_YUV24_YUV = 0,
	HAL_DISP_YUV24_YVU = 1,
	HAL_DISP_YUV24_UYV = 2,
	HAL_DISP_YUV24_VYV = 3,
	HAL_DISP_YUV24_UVY = 4,
	HAL_DISP_YUV24_VUY = 5,

	HAL_DISP_YUV16_YUYV = 0,
	HAL_DISP_YUV16_YVYU = 1,
	HAL_DISP_YUV16_UYVY = 2,
	HAL_DISP_YUV16_VYUY = 3,

	HAL_DISP_YUV8_YUYV = 0,
	HAL_DISP_YUV8_YVYU = 1,
	HAL_DISP_YUV8_UYVY = 2,
	HAL_DISP_YUV8_VYUY = 3,
};

enum {
	HAL_DISP_LCM_16BIT = 0,
	HAL_DISP_LCM_8BIT = 1,
};

enum {
	HAL_DISP_LCM_8080 = 0,
	HAL_DISP_LCM_6800 = 1,
};

enum {
	HAL_DISP_TV_TYPE_NTSC = 0,
	HAL_DISP_TV_TYPE_PAL = 1,
};

enum {
	HAL_DISP_TV_PULSE6_5PULSE = 0,
	HAL_DISP_TV_PULSE6_6PULSE = 1,
};

enum {
	HAL_DISP_TV_SCANSEL_NONINTERLACED = 0,
	HAL_DISP_TV_SCANSEL_INTERLACED = 1,
	HAL_DISP_TV_SCANSEL_PROGRESSIVE = 2,
};

enum {
	HAL_DISP_TV_FSCTYPE_NTSCMJ = 0,
	HAL_DISP_TV_FSCTYPE_PALBDGHIN = 1,
	HAL_DISP_TV_FSCTYPE_NTSC443_PAL60 = 2,
	HAL_DISP_TV_FSCTYPE_PALM = 3,
	HAL_DISP_TV_FSCTYPE_PALNC = 4,
};

enum {
	HAL_DISP_TV_LINESEL_262_525 = 0,
	HAL_DISP_TV_LINESEL_312_625 = 1,
};

enum {
	HAL_DISP_TV_CBWIDTH_252 = 0,
	HAL_DISP_TV_CBWIDTH_225 = 1,
};

enum {
	HAL_DISP_TV_CBSEL_NTSCMJ = 0,
	HAL_DISP_TV_CBSEL_PALM = 1,
	HAL_DISP_TV_CBSEL_PALBDGHINNC = 2,
	HAL_DISP_TV_CBSEL_DISABLE = 3,
};

enum {
	HAL_DISP_DMA_PROGRESSIVE = 0,
	HAL_DISP_DMA_INTERLACED = 1,
};

enum {
	HAL_DISP_DITHER_FIXED = 0,
	HAL_DISP_DITHER_WHEEL = 1,
	HAL_DISP_DITHER_HERRDIFFUSTION = 2,
};

enum {
	HAL_DISP_OSD_0 = 0,
	HAL_DISP_OSD_1 = 1,
	HAL_DISP_OSD_MAX = 2,
};

enum {
	HAL_DISP_OSD_FMT_RGB565 = 0,
	HAL_DISP_OSD_FMT_RGB5515 = 1,
	HAL_DISP_OSD_FMT_RGB1555 = 2,
};

enum {
	HAL_DISP_OSD_TYPE_16BPP = 0,
	HAL_DISP_OSD_TYPE_8BPP = 1,
	HAL_DISP_OSD_TYPE_4BPP = 2,
	HAL_DISP_OSD_TYPE_1BPP = 3,
};

/*******************************************************************************
*                          D A T A    T Y P E S
*******************************************************************************/
typedef struct {
	UINT16 width;
	UINT16 height;
} gpHalDispRes_t;

typedef struct {
	UINT16 top;
	UINT16 bottom;
	UINT16 left;
	UINT16 right;
	UINT32 pattern;
} gpHalDispBlankInfo_t;

typedef struct {
	UINT16 srcWidth;
	UINT16 srcHeight;
	UINT16 dstWidth;
	UINT16 dstHeight;
	UINT16 hInit;
	UINT16 vInit0;
	UINT16 vInit1;
} gpHalDispSclInfo_t;

typedef struct {
	UINT16 polarity;
	UINT16 fPorch;
	UINT16 bPorch;
	UINT16 width;
} gpHalDispLcdTiming_t;

typedef struct {
	UINT32 addrSetup;
	UINT32 addrHold;
	UINT32 csSetup;
	UINT32 csHold;
	UINT32 cycLength;
} gpHalDispLcmTiming_t;

typedef struct {
	UINT32 luminance;
	UINT32 blank;
	UINT32 burst;
} gpHalDispTvAmpAdj_t;

typedef struct {
	UINT32 vAct0;
	UINT32 vAct1;
	UINT32 hAct;
} gpHalDispTvPosAdj_t;


/*******************************************************************************
*               F U N C T I O N    D E C L A R A T I O N S
*******************************************************************************/
/* hal_disp.c */
UINT32 gpHalDispInit(void);
void gpHalDispDeinit(void);
void gpHalDispSetDevType(UINT32 devType);
void gpHalDispUpdateParameter(void);
void gpHalDispSetIntEnable(UINT32 field);
void gpHalDispSetIntDisable(UINT32 field);
void gpHalDispClearIntFlag(UINT32 field);
void gpHalDispSetClkType(UINT32 type);

/* frame rate */
void gpHalDispSetFrameRate(SINT32 framerate);
SINT32 gpHalDispGetFrameRate(void);

void scu_change_pin_grp(UINT32 aPinGrp, UINT32 aPinNum);

/* Primary layer */
void gpHalDispSetPriEnable(UINT32 enable);
UINT32 gpHalDispGetPriEnable(void);
void gpHalDispSetPriBlank(gpHalDispBlankInfo_t blankInfo);
void gpHalDispSetPriFrameAddr(UINT32 addr);
void gpHalDispSetPriPitch(UINT16 src, UINT16 act);
void gpHalDispSetPriRes(UINT16 width, UINT16 height);
void gpHalDispSetPriSclInfo(gpHalDispSclInfo_t scale);
void gpHalDispSetPriSclEnable(UINT32 hEnable, UINT32 vEnable);
void gpHalDispSetPriInputInfo(UINT32 format, UINT32 type);
void gpHalDispSetPriBurst(UINT32 burst);
void gpHalDispSetPriDmaType(UINT32 type);

/* Dither */
void gpHalDispSetDitherEnable(UINT32 enable);
UINT32 gpHalDispGetDitherEnable(void);
void gpHalDispSetDitherType(UINT32 type);
UINT32 gpHalDispGetDitherType(void);
void gpHalDispSetDitherMap(UINT32 map0, UINT32 map1);

/* Color matrix */
void gpHalDispSetColorMatrix(UINT16 *matrix);

/* Gamma */
void gpHalDispSetGammaEnable(UINT32 enable);
UINT32 gpHalDispGetGammaEnable(void);
void gpHalDispSetGammaTable(UINT32 id, UINT8 *table);

/* Color bar */
void gpHalDispSetColorBarEnable(UINT32 enable);
UINT32 gpHalDispGetColorBarEnable(void);
void gpHalDispSetColorBar(UINT32 type, UINT32 size, UINT32 color);

/* Osd layer */
void gpHalDispSetOsdEnable(UINT32 layerNum, UINT32 enable);
UINT32 gpHalDispGetOsdEnable(UINT32 layerNum);
void gpHalDispSetOsdXY(UINT32 layerNum, UINT16 x, UINT16 y);
void gpHalDispSetOsdFrameAddr(UINT32 layerNum, UINT32 addr);
void gpHalDispSetOsdPitch(UINT32 layerNum, UINT16 src, UINT16 act);
void gpHalDispSetOsdRes(UINT32 layerNum, UINT16 width, UINT16 height);
void gpHalDispSetOsdSclInfo(UINT32 layerNum, gpHalDispSclInfo_t scale);
void gpHalDispSetOsdSclEnable(UINT32 layerNum, UINT32 hEnable,UINT32 vEnable);
void gpHalDispSetOsdInputFmt(UINT32 layerNum, UINT32 format);
void gpHalDispSetOsdInputType(UINT32 layerNum, UINT32 type);
void gpHalDispSetOsdBurst(UINT32 layerNum, UINT32 burst);
void gpHalDispSetOsdAlpha(UINT32 layerNum, UINT32 consta, UINT32 ppamd, UINT32 alpha);
void gpHalDispSetOsdColorKey(UINT32 layerNum, UINT32 color);
void gpHalDispSetOsdPalette(UINT32 layerNum, UINT32 startIndex, UINT32 count, UINT32 *pColorTable);
void gpHalDispSetOsdPaletteOffset(UINT32 layerNum, UINT32 offset);
UINT32 gpHalDispGetOsdPaletteOffset(UINT32 layerNum);
void gpHalDispSetOsdDmaType(UINT32 layerNum, UINT32 type);

/* LCD panel */
void gpHalDispSetRes(UINT16 width, UINT16 height);
void gpHalDispGetRes(UINT16 *width, UINT16 *height);
void gpHalDispSetLcdVsync(gpHalDispLcdTiming_t vsync);
void gpHalDispSetLcdHsync(gpHalDispLcdTiming_t hsync);
void gpHalDispSetPanelFormat(UINT32 format, UINT32 type, UINT32 seq0, UINT32 seq1);
void gpHalDispSetClkPolarity(UINT32 polarity);

/* LCM panel */
void gpHalDispSetLcmInterface(UINT32 interface);
void gpHalDispSetLcmMode(UINT32 mode);
void gpHalDispSetLcmDataSelect(UINT32 sel);
void gpHalDispSetLcmAcTiming(gpHalDispLcmTiming_t timing);
void gpHalDispSetLcmSeqSel(UINT32 sel);

/* TV */
void gpHalDispSetTvType(UINT32 type);
void gpHalDispSetTvPulse(UINT32 pulse);
void gpHalDispSetTvScan(UINT32 scan);
void gpHalDispSetTvFscType(UINT32 fsc);
void gpHalDispSetTvFix625(UINT32 fix);
void gpHalDispSetTvLine(UINT32 line);
void gpHalDispSetTvColorBurstWidth(UINT32 width);
void gpHalDispSetTvColorBurstSel(UINT32 sel);
void gpHalDispSetTvCftType(UINT32 type);
void gpHalDispSetTvCupType(UINT32 type);
void gpHalDispSetTvYupType(UINT32 type);
void gpHalDispSetTvAmpAdj(gpHalDispTvAmpAdj_t amp);
void gpHalDispSetTvPosAdj(gpHalDispTvPosAdj_t pos);

void gpHalDispDumpRegister(void);

void gpHalDispSetSSC( UINT32 enable, UINT32 mode, UINT32 stage );
void gpHalDispGetSSC( UINT32 *enable, UINT32 *mode, UINT32 *stage );

/* Flip */
void gpHalDispSetFlip( UINT32 value );

#endif  /* __HAL_DISP_H__ */

