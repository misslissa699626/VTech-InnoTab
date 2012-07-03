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
#ifndef _HAL_DISP1_H_
#define _HAL_DISP1_H_

/**************************************************************************
 *                         H E A D E R   F I L E S                        *
 **************************************************************************/
#include <mach/typedef.h>


/**************************************************************************
*                           C O N S T A N T S                             *
 **************************************************************************/
enum{
        HAL_DISP1_DATA_MODE_8 = 0x0,
        HAL_DISP1_DATA_MODE_565 = 0x400,
        HAL_DISP1_DATA_MODE_666 = 0x800,
        HAL_DISP1_DATA_MODE_888 = 0xC00,
};

enum{
        STATE_FAULT = 0,
        STATE_TRUE = 1,
};

enum{
        HAL_DISP1_DCLK_SEL_0 = 0x0,
        HAL_DISP1_DCLK_SEL_90 = 0x400,
        HAL_DISP1_DCLK_SEL_180 = 0x800,
        HAL_DISP1_DCLK_SEL_270 = 0xC00,
};

enum{
        HAL_DISP1_MODE_UPS051 = 0x0,
        HAL_DISP1_MODE_UPS052 = 0x10,
        HAL_DISP1_MODE_CCIR656 = 0x20,
        HAL_DISP1_MODE_PARALLEL = 0x30,
        HAL_DISP1_MODE_TCON = 0x40,
};

enum {
	HAL_DISP1_DEV_LCD = 0,
	HAL_DISP1_DEV_LCM,
	HAL_DISP1_DEV_TV,
	HAL_DISP1_DEV_MAX,
};

enum {
	HAL_DISP1_INT_DISPLAY_OFF = 0x01,
	HAL_DISP1_INT_FRAME_END = 0x04,
	HAL_DISP1_INT_FIELD_END = 0x8,
	HAL_DISP1_INT_UPDATE_FAIL = 0x10,
};

enum {
	HAL_DISP1_INPUT_FMT_RGB = 0,
	HAL_DISP1_INPUT_FMT_YCbCr = 1,
};

enum {
	HAL_DISP1_INPUT_TYPE_RGB565 = 0,
	HAL_DISP1_INPUT_TYPE_RGB555 = 1,
	HAL_DISP1_INPUT_TYPE_RGB888 = 2,

	HAL_DISP1_INPUT_TYPE_YCbYCr = 0,
	HAL_DISP1_INPUT_TYPE_4Y4Cb4Y4Cr = 1,
	HAL_DISP1_INPUT_TYPE_YCbCr = 2,
};

enum {
	/* LCD Output Format */
	HAL_DISP1_OUTPUT_FMT_RGB = 0,
	HAL_DISP1_OUTPUT_FMT_YCbCr = 1,
	HAL_DISP1_OUTPUT_FMT_YUV = 2,
};

enum {
	/* LCD Output Type */
	HAL_DISP1_OUTPUT_TYPE_PRGB888 = 0,
	HAL_DISP1_OUTPUT_TYPE_PRGB565 = 1,
	HAL_DISP1_OUTPUT_TYPE_SRGB888 = 2,
	HAL_DISP1_OUTPUT_TYPE_SRGBM888 = 3,

	HAL_DISP1_OUTPUT_TYPE_YCbCr24 = 0,
	HAL_DISP1_OUTPUT_TYPE_YCbCr16 = 1,
	HAL_DISP1_OUTPUT_TYPE_YCbCr8 = 2,

	HAL_DISP1_OUTPUT_TYPE_YUV24 = 0,
	HAL_DISP1_OUTPUT_TYPE_YUV16 = 1,
	HAL_DISP1_OUTPUT_TYPE_YUV8 = 2,

	/* LCM Output Type */
	HAL_DISP1_OUTPUT_TYPE_RGB666 = 0,
	HAL_DISP1_OUTPUT_TYPE_RGB565 = 1,
	HAL_DISP1_OUTPUT_TYPE_RGB444 = 2,
	HAL_DISP1_OUTPUT_TYPE_RGB332 = 3,
};

enum {
	HAL_DISP1_PRGB888_RGB = 0,
	HAL_DISP1_PRGB888_BGR = 1,

	HAL_DISP1_PRGB565_RGB = 0,
	HAL_DISP1_PRGB565_BGR = 1,

	HAL_DISP1_SRGB888_RGB = 0,
	HAL_DISP1_SRGB888_GBR = 1,
	HAL_DISP1_SRGB888_BRG = 2,
	HAL_DISP1_SRGB888_RBG = 3,
	HAL_DISP1_SRGB888_BGR = 4,
	HAL_DISP1_SRGB888_GRB = 5,

	HAL_DISP1_SRGBM888_RGBM = 0,
	HAL_DISP1_SRGBM888_GBRM = 1,
	HAL_DISP1_SRGBM888_BRGM = 2,
	HAL_DISP1_SRGBM888_RBGM = 3,
	HAL_DISP1_SRGBM888_BGRM = 4,
	HAL_DISP1_SRGBM888_GRBM = 5,

	HAL_DISP1_YCBCR24_YCbCr = 0,
	HAL_DISP1_YCBCR24_YCrCb = 1,
	HAL_DISP1_YCBCR24_CbYCr = 2,
	HAL_DISP1_YCBCR24_CrYCb = 3,
	HAL_DISP1_YCBCR24_CbCrY = 4,
	HAL_DISP1_YCBCR24_CrCbY = 5,

	HAL_DISP1_YCBCR16_YCbYCr = 0,
	HAL_DISP1_YCBCR16_YCrYCb = 1,
	HAL_DISP1_YCBCR16_CbYCrY = 2,
	HAL_DISP1_YCBCR16_CrYCbY = 3,

	HAL_DISP1_YCBCR8_YCbYCr = 0,
	HAL_DISP1_YCBCR8_YCrYCb = 1,
	HAL_DISP1_YCBCR8_CbYCrY = 2,
	HAL_DISP1_YCBCR8_CrYCbY = 3,

	HAL_DISP1_YUV24_YUV = 0,
	HAL_DISP1_YUV24_YVU = 1,
	HAL_DISP1_YUV24_UYV = 2,
	HAL_DISP1_YUV24_VYV = 3,
	HAL_DISP1_YUV24_UVY = 4,
	HAL_DISP1_YUV24_VUY = 5,

	HAL_DISP1_YUV16_YUYV = 0,
	HAL_DISP1_YUV16_YVYU = 1,
	HAL_DISP1_YUV16_UYVY = 2,
	HAL_DISP1_YUV16_VYUY = 3,

	HAL_DISP1_YUV8_YUYV = 0,
	HAL_DISP1_YUV8_YVYU = 1,
	HAL_DISP1_YUV8_UYVY = 2,
	HAL_DISP1_YUV8_VYUY = 3,
};

enum {
	HAL_DISP1_LCM_16BIT = 0,
	HAL_DISP1_LCM_8BIT = 1,
};

enum {
	HAL_DISP1_LCM_8080 = 0,
	HAL_DISP1_LCM_6800 = 1,
};

enum {
	HAL_DISP1_DITHER_FIXED = 0,
	HAL_DISP1_DITHER_WHEEL = 1,
	HAL_DISP1_DITHER_HERRDIFFUSTION = 2,
};


/*******************************************************************************
*                          D A T A    T Y P E S
*******************************************************************************/
#define HAL_DISP1_EN                     (1<<0)
#define HAL_DISP1_CLK_SEL           (7<<1)
#define HAL_DISP1_MODE                (0xF<<4)
#define HAL_DISP1_VSYNC_INV           (1<<8)
#define HAL_DISP1_HSYNC_INV           (1<<9)
#define HAL_DISP1_DCLK_INV            (1<<10)
#define HAL_DISP1_DE_INV              (1<<11)
#define HAL_DISP1_H_COMPRESS          (1<<12)
#define HAL_DISP1_MEM_BYTE_EN         (1<<13)
#define HAL_DISP1_INTERLACE_MOD       (1<<14)
#define HAL_DISP1_VSYNC_UNIT          (1<<15)

#define HAL_DISP1_REG_POL             (1<<4)
#define HAL_DISP1_REG_REV             (1<<5)
#define HAL_DISP1_UD_I                      (1<<6)
#define HAL_DISP1_RL_I                      (1<<7)
#define HAL_DISP1_DITH_EN               (1<<8)
#define HAL_DISP1_DITH_MODE           (1<<9)
#define HAL_DISP1_DATA_MODE           (3<<10)
#define HAL_DISP1_SWITCH_EN           (1<<12)
#define HAL_DISP1_GAMMA_EN            (1<<13)
#define HAL_DISP1_DCLK_SEL            (3<<14)
#define HAL_DISP1_DCLK_DELAY          (7<<18)
#define HAL_DISP1_SLIDE_EN            (1<<21)


#define HAL_DISP1_ENABLE            0xFFFFFFFF
#define HAL_DISP1_DIS                   0

#define HAL_DISP1_CLK_DIVIDE_1        0x0
#define HAL_DISP1_CLK_DIVIDE_2        0x2
#define HAL_DISP1_CLK_DIVIDE_3        0x4
#define HAL_DISP1_CLK_DIVIDE_4        0x6
#define HAL_DISP1_CLK_DIVIDE_5        0x8
#define HAL_DISP1_CLK_DIVIDE_6        0xA
#define HAL_DISP1_CLK_DIVIDE_7        0xC
#define HAL_DISP1_CLK_DIVIDE_8        0xE
#define HAL_DISP1_CLK_DIVIDE_9        0x10
#define HAL_DISP1_CLK_DIVIDE_10       0x12
#define HAL_DISP1_CLK_DIVIDE_11       0x14
#define HAL_DISP1_CLK_DIVIDE_12       0x16
#define HAL_DISP1_CLK_DIVIDE_13       0x18
#define HAL_DISP1_CLK_DIVIDE_14       0x1A
#define HAL_DISP1_CLK_DIVIDE_15       0x1C
#define HAL_DISP1_CLK_DIVIDE_16       0x1E
#define HAL_DISP1_CLK_DIVIDE_17       0x20
#define HAL_DISP1_CLK_DIVIDE_18       0x22
#define HAL_DISP1_CLK_DIVIDE_19       0x24
#define HAL_DISP1_CLK_DIVIDE_20       0x26
#define HAL_DISP1_CLK_DIVIDE_21       0x28
#define HAL_DISP1_CLK_DIVIDE_22       0x2A
#define HAL_DISP1_CLK_DIVIDE_23       0x2C
#define HAL_DISP1_CLK_DIVIDE_24       0x2E
#define HAL_DISP1_CLK_DIVIDE_25       0x30
#define HAL_DISP1_CLK_DIVIDE_26       0x32
#define HAL_DISP1_CLK_DIVIDE_27       0x34
#define HAL_DISP1_CLK_DIVIDE_28       0x36
#define HAL_DISP1_CLK_DIVIDE_29       0x38
#define HAL_DISP1_CLK_DIVIDE_30       0x3A
#define HAL_DISP1_CLK_DIVIDE_31       0x3C
#define HAL_DISP1_CLK_DIVIDE_32       0x3E

typedef struct {
	UINT16 width;
	UINT16 height;
} gpHalDisp1Res_t;

typedef struct {
	UINT16 top;
	UINT16 bottom;
	UINT16 left;
	UINT16 right;
	UINT32 pattern;
} gpHalDisp1BlankInfo_t;

typedef struct {
	UINT16 srcWidth;
	UINT16 srcHeight;
	UINT16 dstWidth;
	UINT16 dstHeight;
	UINT16 hInit;
	UINT16 vInit0;
	UINT16 vInit1;
} gpHalDisp1SclInfo_t;

typedef struct {
	UINT16 polarity;
	UINT16 period;
	UINT16 start;
	UINT16 end;
} gpHalDisp1LcdTiming_t;

typedef struct {
	UINT32 addrSetup;
	UINT32 addrHold;
	UINT32 csSetup;
	UINT32 csHold;
	UINT32 cycLength;
} gpHalDisp1LcmTiming_t;

typedef struct {
	UINT32 luminance;
	UINT32 blank;
	UINT32 burst;
} gpHalDisp1TvAmpAdj_t;

typedef struct {
	UINT32 vAct0;
	UINT32 vAct1;
	UINT32 hAct;
} gpHalDisp1TvPosAdj_t;


/*******************************************************************************
*               F U N C T I O N    D E C L A R A T I O N S
*******************************************************************************/
/* hal_disp1.c */
UINT32 gpHalDisp1Init(void);
void gpHalDisp1Deinit(void);
void gpHalDisp1SetDevType(UINT32 devType);
void gpHalDisp1UpdateParameter(void);
void gpHalDisp1SetIntEnable(UINT32 field);
void gpHalDisp1SetIntDisable(UINT32 field);
void gpHalDisp1ClearIntFlag(UINT32 field);


void scu_change_pin_grp_1(UINT32 aPinGrp, UINT32 aPinNum);

/* Primary layer */
void gpHalDisp1SetPriEnable(UINT32 enable);
UINT32 gpHalDisp1GetPriEnable(void);
void gpHalDisp1SetPriBlank(gpHalDisp1BlankInfo_t blankInfo);
void gpHalDisp1SetPriFrameAddr(UINT32 addr);
void gpHalDisp1SetPriPitch(UINT16 src, UINT16 act);
void gpHalDisp1SetPriRes(UINT16 width, UINT16 height);
void gpHalDisp1SetPriSclInfo(gpHalDisp1SclInfo_t scale);
void gpHalDisp1SetPriSclEnable(UINT32 hEnable, UINT32 vEnable);
void gpHalDisp1SetPriInputInfo(UINT32 format, UINT32 type);
void gpHalDisp1SetPriBurst(UINT32 burst);
void gpHalDisp1SetPriDmaType(UINT32 type);

/* Dither */
void gpHalDisp1SetDitherEnable(UINT32 enable);
UINT32 gpHalDisp1GetDitherEnable(void);
void gpHalDisp1SetDitherType(UINT32 type);
UINT32 gpHalDisp1GetDitherType(void);
void gpHalDisp1SetDitherMap(UINT32 map0, UINT32 map1);

/* Color matrix */
void gpHalDisp1SetColorMatrix(UINT16 *matrix);

/* Gamma */
void gpHalDisp1SetGammaEnable(UINT32 enable);
UINT32 gpHalDisp1GetGammaEnable(void);
void gpHalDisp1SetGammaTable(UINT32 id, UINT8 *table);

/* Color bar */
void gpHalDisp1SetColorBarEnable(UINT32 enable);
UINT32 gpHalDisp1GetColorBarEnable(void);
void gpHalDisp1SetColorBar(UINT32 type, UINT32 size, UINT32 color);

/* LCD panel */
void gpHalDisp1SetRes(UINT16 width, UINT16 height);
void gpHalDisp1GetRes(UINT16 *width, UINT16 *height);
void gpHalDisp1SetLcdVsync(gpHalDisp1LcdTiming_t vsync);
void gpHalDisp1SetLcdHsync(gpHalDisp1LcdTiming_t hsync);
void gpHalDisp1SetPanelFormat(UINT32 format, UINT32 type, UINT32 seq0, UINT32 seq1);
void gpHalDisp1SetClkPolarity(UINT32 polarity);

/* LCM panel */
void gpHalDisp1SetLcmInterface(UINT32 interface);
void gpHalDisp1SetLcmMode(UINT32 mode);
void gpHalDisp1SetLcmDataSelect(UINT32 sel);
void gpHalDisp1SetLcmAcTiming(gpHalDisp1LcmTiming_t timing);

/* TV */
void gpHalDisp1SetTvType(UINT32 type);
void gpHalDisp1SetTvPulse(UINT32 pulse);
void gpHalDisp1SetTvScan(UINT32 scan);
void gpHalDisp1SetTvFscType(UINT32 fsc);
void gpHalDisp1SetTvFix625(UINT32 fix);
void gpHalDisp1SetTvLine(UINT32 line);
void gpHalDisp1SetTvColorBurstWidth(UINT32 width);
void gpHalDisp1SetTvColorBurstSel(UINT32 sel);
void gpHalDisp1SetTvCftType(UINT32 type);
void gpHalDisp1SetTvCupType(UINT32 type);
void gpHalDisp1SetTvYupType(UINT32 type);
void gpHalDisp1SetTvAmpAdj(gpHalDisp1TvAmpAdj_t amp);
void gpHalDisp1SetTvPosAdj(gpHalDisp1TvPosAdj_t pos);

void gpHalDisp1DumpRegister(void);

void gpHalDisp1SetSSC( UINT32 enable, UINT32 mode, UINT32 stage );
void gpHalDisp1GetSSC( UINT32 *enable, UINT32 *mode, UINT32 *stage );

/* Flip */
void gpHalDisp1SetFlip( UINT32 value );


/* Disp1 */
void gpHalDisp1SetVerticalStart(UINT32 disp1V_Start);
UINT32 gpHalDisp1GetVerticalStart(void);
void gpHalDisp1SetHorizontalStart(UINT32 disp1H_Start);
UINT32 gpHalDisp1GetHorizontalStart(void);
void gpHalDisp1SetVerticalEnd(UINT32 disp1V_End);
UINT32 gpHalDisp1GetVerticalEnd(void);
void gpHalDisp1SetHorizontalEnd(UINT32 disp1H_End);
UINT32 gpHalDisp1GetHorizontalEnd(void);
void gpHalDisp1SetVerticalPeriod(UINT32 disp1V_Period);
UINT32 gpHalDisp1GetVerticalPeriod(void);
void gpHalDisp1SetHorizontalPeriod(UINT32 disp1H_Period);
UINT32 gpHalDisp1GetHorizontalPeriod(void);

void gpHalDisp1SetVSyncStart(UINT32 disp1VS_Start);
UINT32 gpHalDisp1GetVSyncStart(void);
void gpHalDisp1SetVSyncEnd(UINT32 disp1VS_End);
UINT32 gpHalDisp1GetVSyncEnd(void);
void gpHalDisp1SetHSyncStart(UINT32 disp1HS_Start);
UINT32 gpHalDisp1GetHSyncStart(void);
void gpHalDisp1SetHSyncEnd(UINT32 disp1HS_End);
UINT32 gpHalDisp1GetHSyncEnd(void);
void gpHalDisp1SetTsMisc(UINT32 disp1TS_MISC);
UINT32 gpHalDisp1GetTsMisc(void);
void gpHalDisp1SetDataMode(UINT32 disp1DataMode);
UINT32 gpHalDisp1GetDataMode(void);
void gpHalDisp1SetVSyncUnit(UINT32 status);
void gpHalDisp1SetDClkSel(UINT32 sel);
void gpHalDisp1SetSignalInv(UINT32 mask, UINT32 value);
void gpHalDisp1SetMode(UINT32 mode);
void gpHalDisp1SetClk(UINT32 clk);
void gpHalDisp1SetSlideEn(UINT32 status);
void gpHalDisp1SetEnable(UINT32 status);
void gpHalDisp1EnablePWM(UINT32 status);
#endif  /* __HAL_DISP1_H__ */

