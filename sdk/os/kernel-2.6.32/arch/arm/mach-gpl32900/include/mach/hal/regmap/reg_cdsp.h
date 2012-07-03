/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Technology Co., Ltd.         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Technology Co., Ltd. All rights are reserved by Generalplus Technology*
 *  Co., Ltd. This software may only be used in accordance with the       *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Technology Co., Ltd. reserves the right to modify this    *
 *  software without notice.                                              *
 *                                                                        *
 *  Generalplus Technology Co., Ltd.                                      *
 *  19, Innovation First Road, Science-Based Industrial Park,             *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 **************************************************************************/
#ifndef _REG_CDSP_H_
#define _REG_CDSP_H_
	
#include <mach/hardware.h>
#include <mach/typedef.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define	LOGI_ADDR_CDSP_REG		IO3_ADDRESS(0x1000)

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct cdspReg0_s 
{
	volatile UINT32 cdspMacroCtrl;		/* R_CDSP_MACRO_CTRL, 0x93001000 */
	volatile UINT32 cdspBadPixCtrl;		/* R_CDSP_BADPIX_CTRL, 0x93001004 */
	volatile UINT32 cdspBadPixThr;		/* R_CDSP_BADPIX_CTHR, 0x93001008 */
	volatile UINT32 cdspYuvSpEffOffset;	/* R_CDSP_YUVSP_EFFECT_OFFSET, 0x9300100C */

	volatile UINT32 cdspImgType;		/* R_CDSP_IMG_TYPE, 0x93001010 */
	volatile UINT32 cdspYuvSpEffScale;	/* R_CDSP_YUVSP_EFFECT_SCALER, 0x93001014 */
	volatile UINT32 cdspOpbCtrl;		/* R_CDSP_OPB_CTRL, 0x93001018 */
	volatile UINT32 cdspYuvSpEffBThr;	/* R_CDSP_YUVSP_EFFECT_BTHR, 0x9300101C */

	volatile UINT32 cdspOpbType;		/* R_CDSP_OPB_TYPE, 0x93001020 */
	volatile UINT32 cdspOpbHOffset;		/* R_CDSP_OPB_HOFFSET, 0x93001024 */
	volatile UINT32 cdspOpbVOffset;		/* R_CDSP_OPB_VOFFSET, 0x93001028 */
	volatile UINT32 reserved00;			/* Reserved, 0x9300102C */

	volatile UINT32 reserved01;			/* Reserved, 0x93001030 */
	volatile UINT32 reserved02;			/* Reserved, 0x93001034 */
	volatile UINT32 reserved03;			/* Reserved, 0x93001038 */
	volatile UINT32 reserved04;			/* Reserved, 0x9300103C */

	volatile UINT32 cdspOpbRAvg;		/* R_CDSP_OPB_RAVG, 0x93001040 */
	volatile UINT32 cdspOpbGrAvg;		/* R_CDSP_OPB_GRAVG, 0x93001044 */
	volatile UINT32 cdspOpbBAvg;		/* R_CDSP_OPB_BAVG, 0x93001048 */
	volatile UINT32 cdspOpbGbAvg;		/* R_CDSP_OPB_GBAVG, 0x9300104C */
		
	volatile UINT32 cdspDummy;			/* R_CDSP_OPB_GBAVG, 0x93001050 */
	volatile UINT32 reserved05;			/* Reserved, 0x93001054 */
	volatile UINT32 reserved06;			/* Reserved, 0x93001058 */
	volatile UINT32 reserved07;			/* Reserved, 0x9300105C */

	volatile UINT32 reserved08;			/* Reserved, 0x93001060 */
	volatile UINT32 reserved09;			/* Reserved, 0x93001064 */
	volatile UINT32 reserved0A;			/* Reserved, 0x93001068 */
	volatile UINT32 reserved0B;			/* Reserved, 0x9300106C */

	volatile UINT32 reserved0C;			/* Reserved, 0x93001070 */
	volatile UINT32 reserved0D;			/* Reserved, 0x93001074 */
	volatile UINT32 reserved0E;			/* Reserved, 0x93001078 */
	volatile UINT32 reserved0F;			/* Reserved, 0x9300107C */
	
	volatile UINT32 cdspLensCmpCtrl;	/* R_CDSP_LENS_CMP_CTRL, 0x93001080 */
	volatile UINT32 cdspVValid;			/* R_CDSP_CDSP_VVALID, 0x93001084 */
	volatile UINT32 cdspLensCmpXOffset;	/* R_CDSP_LENS_CMP_XOFFSET_MAP, 0x93001088 */
	volatile UINT32 cdspLensCmpYOffset;	/* R_CDSP_LENS_CMP_YOFFSET_MAP, 0x9300108C */

	volatile UINT32 cdspLensCmpYInStep;	/* R_CDSP_LENS_CMP_YINSTEP_MAP, 0x93001090 */
	volatile UINT32 cdspHscaleEvenPval;	/* R_CDSP_HSCALE_EVEN_PVAL, 0x93001094 */
	volatile UINT32 cdspImgXCent;		/* R_CDSP_IM_XCENT, 0x93001098 */
	volatile UINT32 cdspImgYCent;		/* R_CDSP_IM_YCENT, 0x9300109C */

	volatile UINT32 cdspHRawScaleFactor;/* R_CDSP_HRAW_SCLDW_FACTOR, 0x930010A0 */
	volatile UINT32 cdspHScaleOddPVal;	/* R_CDSP_HSCALE_ODD_PVAL, 0x930010A4 */
	volatile UINT32 cdspVYuvScaleFactor;/* R_CDSP_VYUV_SCLDW_FACTOR, 0x930010A8 */
	volatile UINT32 cdspVScaleAccInit;	/* R_CDSP_VSCALE_ACC_INIT, 0x930010AC */

	volatile UINT32 cdspHYuvScaleFactor;/* R_CDSP_HYUV_SCLDW_FACTOR, 0x930010B0 */
	volatile UINT32 cdspHScaleAccInit;	/* R_CDSP_HSCALE_ACC_INIT, 0x930010B4 */
	volatile UINT32 cdspScaleDownCtrl;	/* R_CDSP_SCLDW_CTRL, 0x930010B8 */
	volatile UINT32 cdspScaleFactorCtrl;/* R_CDSP_SCALE_FACTOR_CTRL, 0x930010BC */

	volatile UINT32 cdspWhbalRSet;		/* R_CDSP_WHBAL_RSETTING, 0x930010C0 */
	volatile UINT32 cdspWhbalGrSet;		/* R_CDSP_WHBAL_GRSETTING, 0x930010C4 */
	volatile UINT32 cdspWhbalBSet;		/* R_CDSP_WHBAL_BSETTING, 0x930010C8 */
	volatile UINT32 cdspWhbalGbSet;		/* R_CDSP_WHBAL_GBSETTING, 0x930010CC */

	volatile UINT32 cdspYuvSpMode;		/* R_CDSP_YUVSPEC_MODE, 0x930010D0 */
	volatile UINT32 cdspGlobalGain;		/* R_CDSP_GLOBAL_GAIN, 0x930010D4 */
	volatile UINT32 cdspImgCropCtrl;	/* R_CDSP_IMCROP_CTRL, 0x930010D8 */
	volatile UINT32 cdspImgCropHOffset;	/* R_CDSP_IMCROP_HOFFSET, 0x930010DC */

	volatile UINT32 cdspImgCropHSize;	/* R_CDSP_IMCROP_HSIZE, 0x930010E0 */
	volatile UINT32 cdspImgCropVOffset;	/* R_CDSP_IMCROP_VOFFSET, 0x930010E4 */
	volatile UINT32 cdspImgCropVSize;	/* R_CDSP_IMCROP_VSIZE, 0x930010E8 */
	volatile UINT32 cdspInpDenoiseThr;	/* R_CDSP_INP_DENOISE_THR, 0x930010EC */

	volatile UINT32 cdspInpMirCtrl;		/* R_CDSP_INP_MIRROR_CTRL, 0x930010F0 */
	volatile UINT32 cdspRgbSpMode;		/* R_CDSP_RGB_SPEC_ROT_MODE, 0x930010F4 */
	volatile UINT32 reserved10;			/* reserved, 0x930010F8 */
	volatile UINT32 reserved11;			/* reserved, 0x930010FC */

	volatile UINT32 cdspHPFLCoef0;		/* R_CDSP_HPF_LCOEF0, 0x93001100 */
	volatile UINT32 cdspHPFLCoef1;		/* R_CDSP_HPF_LCOEF1, 0x93001104 */
	volatile UINT32 cdspHPFLCoef2;		/* R_CDSP_HPF_LCOEF2, 0x93001108 */
	volatile UINT32 cdspLHDivCtrl;		/* R_CDSP_LH_DIV_CTRL, 0x9300110C */

	volatile UINT32 cdspInpEdgeCtrl;	/* R_CDSP_INP_EDGE_CTRL, 0x93001110 */
	volatile UINT32 cdspInpQThr;		/* R_CDSP_INP_QTHR, 0x93001114 */
	volatile UINT32 cdspInpQCnt;		/* R_CDSP_INP_QCNT, 0x93001118 */
	volatile UINT32 cdspCcCof0;			/* R_CDSP_CC_COF0, 0x9300111C */

	volatile UINT32 cdspCcCof1;			/* R_CDSP_CC_COF0, 0x93001120 */
	volatile UINT32 cdspCcCof2;			/* R_CDSP_CC_COF0, 0x93001124 */
	volatile UINT32 cdspCcCof3;			/* R_CDSP_CC_COF0, 0x93001128 */
	volatile UINT32 cdspCcCof4;			/* R_CDSP_CC_COF0, 0x9300112C */

	volatile UINT32 reserved12;			/* reserved, 0x93001130 */
	volatile UINT32 cdspRbClampCtrl;	/* R_CDSP_RB_CLAMP_CTRL, 0x93001134 */
	volatile UINT32 cdspUvScaleCond0;	/* R_CDSP_UVSCALE_COND0, 0x93001138 */
	volatile UINT32 cdspUvScaleCond1;	/* R_CDSP_UVSCALE_COND1, 0x9300113C */

	volatile UINT32 cdspYuvCtrl;		/* R_CDSP_YUV_CTRL, 0x93001140 */
	volatile UINT32 cdspBistEn;			/* R_CDSP_BIST_EN, 0x93001144 */
	volatile UINT32 cdspDenoiseSet;		/* R_CDSP_DENOISE_SETTING, 0x93001148 */
	volatile UINT32 cdspHutRotU;		/* R_CDSP_HUE_ROT_U, 0x9300114C */

	volatile UINT32 cdspHutRotV;		/* R_CDSP_HUE_ROT_V, 0x93001150 */
	volatile UINT32 cdspYuvRange;		/* R_CDSP_YUV_RANGE, 0x93001154 */
	volatile UINT32 cdspIntEn;			/* R_CDSP_INTEN, 0x93001158 */
	volatile UINT32 cdspGatingClkCtrl;	/* R_CDSP_GATING_CLK_CTRL, 0x9300115C */

	volatile UINT32 cdspYuvCoringSet;	/* R_CDSP_YUV_CORING_SETTING, 0x93001160 */
	volatile UINT32 cdspInt;			/* R_CDSP_INT, 0x93001164 */
	volatile UINT32 cdspBistFail;		/* R_CDSP_BIST_FAIL, 0x93001168 */
	volatile UINT32 cdspProbeCtrl;		/* R_CDSP_PROBE_CTRL, 0x9300116C */

	volatile UINT32 cdspExtBankSize;	/* R_CDSP_EXT_BANK_SIZE, 0x93001170 */
	volatile UINT32 cdspYuvAvgLpfType;	/* R_CDSP_YUV_AVG_LPF_TYPE, 0x93001174 */
	volatile UINT32 cdspExtLineSize;	/* R_CDSP_EXT_LINE_SIZE, 0x93001178 */
	volatile UINT32 cdspReset;			/* R_CDSP_RST, 0x9300117C */
}cdspReg0_t;

typedef struct cdspReg1_s 
{
	volatile UINT8 offset00[0x0200];	/* offset00, 0x93001000 ~ 0x93001200*/
	
	volatile UINT32 cdspRawSubSampleSet;/* R_CDSP_RAW_SUBSAMPLE_SETTING, 0x93001200 */
	volatile UINT32 cdspRawMirSet;		/* R_CDSP_RAW_MIRROR_SETTING, 0x93001204 */
	volatile UINT32 cdspClampSet;		/* R_CDSP_CLAMP_SETTING, 0x93001208 */
	volatile UINT32 cdspRbHSize;		/* R_CDSP_RB_HSIZE, 0x9300120C */

	volatile UINT32 cdspRbHOffset;		/* R_CDSP_RB_HOFFSET, 0x93001210 */
	volatile UINT32 cdspRbVSize;		/* R_CDSP_RB_VSIZE, 0x93001214 */
	volatile UINT32 cdspRbVOffset;		/* R_CDSP_RB_VOFFSET, 0x93001218 */
	volatile UINT32 cdspWdramHOffset;	/* R_CDSP_WDRAM_HOFFSET, 0x9300121C */

	volatile UINT32 cdspWdramVOffset;	/* R_CDSP_WDRAM_VOFFSET, 0x93001220 */
	volatile UINT32 cdspLineInterval;	/* R_CDSP_LINE_INTERVAL, 0x93001224 */
	volatile UINT32 cdspDo;				/* R_CDSP_DO, 0x93001228 */
	volatile UINT32 cdspTvMode;			/* R_CDSP_TV_MODE, 0x9300122C */

	volatile UINT32 cdspWsramThr;		/* R_CDSP_WSRAM_THR, 0x93001230 */
	volatile UINT32 reserved00;			/* reserved00, 0x93001234 */
	volatile UINT32 reserved01;			/* reserved01, 0x93001238 */
	volatile UINT32 reserved02;			/* reserved02, 0x9300123c */
	
	volatile UINT32 cdspDataFmt;		/* R_CDSP_DATA_FORMAT, 0x93001240 */
	volatile UINT32 cdspTsRxCtrl;		/* R_CDSP_TSRX_CTRL, 0x93001244 */
	volatile UINT32 cdspGInt;			/* R_CDSP_GINT, 0x93001248 */	
	volatile UINT32 reserved03;			/* reserved03, 0x9300124C */

	volatile UINT8 offset01[0x0030];	/* offset01, 0x93001250 ~ 0x93001280*/

	volatile UINT32 cdspYcorCtrl0;		/* R_CDSP_YCOR_CTRL0, 0x93001280 */
	volatile UINT32 cdspYcorCtrl1;		/* R_CDSP_YCOR_CTRL1, 0x93001284 */
	volatile UINT32 cdspYcorCtrl2;		/* R_CDSP_YCOR_CTRL2, 0x93001288 */
	volatile UINT32 cdspYcorCtrl3;		/* R_CDSP_YCOR_CTRL3, 0x9300128C */

	volatile UINT32 cdspYcorCtrl4;		/* R_CDSP_YCOR_CTRL4, 0x93001290 */
	volatile UINT32 cdspYcorCtrl5;		/* R_CDSP_YCOR_CTRL5, 0x93001294 */
	volatile UINT32 cdspYcorCtrl6;		/* R_CDSP_YCOR_CTRL6, 0x93001298 */
	volatile UINT32 cdspYcorCtrl7;		/* R_CDSP_YCOR_CTRL7, 0x9300129C */

	volatile UINT32 cdspYcorEn;			/* R_CDSP_YCOR_EN, 0x930012A0 */
	volatile UINT32 reserved04;			/* reserved04, 0x930012A4 */
	volatile UINT32 reserved05;			/* reserved05, 0x930012A8 */
	volatile UINT32 reserved06;			/* reserved06, 0x930012AC */

	volatile UINT8 offset02[0x0050];	/* offset02, 0x930012B0 ~ 0x93001300*/

	volatile UINT32 cdspDmaYuvASAddr;	/* R_CDSP_DMA_YUVABUF_SADDR, 0x93001300 */
	volatile UINT32 cdspDmaYuvAHVSize;	/* R_CDSP_DMA_YUVABUF_HVSIZE, 0x93001304 */
	volatile UINT32 cdspDmaYuvBSAddr;	/* R_CDSP_DMA_YUVBBUF_SADDR, 0x93001308 */
	volatile UINT32 cdspDmaYuvBHVSize;	/* R_CDSP_DMA_YUVBBUF_HVSIZE, 0x9300130C */

	volatile UINT32 cdspDmaRawSAddr;	/* R_CDSP_DMA_RAWBUF_SADDR, 0x93001310 */
	volatile UINT32 cdspDmaRawHVSize;	/* R_CDSP_DMA_RAWBUF_HVSIZE, 0x93001314 */
	volatile UINT32 cdspDmaRawHOffset;	/* R_CDSP_DMA_RAWBUF_HOFFSET, 0x93001318 */
	volatile UINT32 cdspDmaDfHOffset;	/* R_CDSP_DMA_DF_HOFFSET, 0x9300131C */

	volatile UINT32 cdspDmaDfSAddr;		/* R_CDSP_DMA_DF_SADDR, 0x93001320 */
	volatile UINT32 cdspDmaConfig;		/* R_CDSP_DMA_COFG, 0x93001324 */
}cdspReg1_t;

typedef struct cdspReg3a_s 
{
	volatile UINT8 offset00[0x0400];	/* offset00, 0x93001000 ~ 0x93001400*/

	volatile UINT32 cdspAwbWinRgGain2;	/* R_CDSP_AWB_WIN_RGGAIN2, 0x93001400 */
	volatile UINT32 cdspAwbWinBgGain2;	/* R_CDSP_AWB_WIN_BGAIN2, 0x93001404 */
	volatile UINT32 cdspAeAwbWinCtrl;	/* R_CDSP_AE_AWB_WIN_CTRL, 0x93001408 */
	volatile UINT32 cdspAeWinSize;		/* R_CDSP_AE_WIN_SIZE, 0x9300140C */

	volatile UINT32 cdspRgbWinHCtrl;	/* R_CDSP_RGB_WINH_CTRL, 0x93001410 */
	volatile UINT32 cdspRgbWinVCtrl;	/* R_CDSP_RGB_WINV_CTRL, 0x93001414 */
	volatile UINT32 cdspAeWinBufAddrA;	/* R_CDSP_AE_WIN_ABUFADDR, 0x93001418 */
	volatile UINT32 cdspAeWinBufAddrB;	/* R_CDSP_AE_WIN_BBUFADDR, 0x9300141C */

	volatile UINT32 cdspHistgmCtrl;		/* R_CDSP_HISTGM_CTRL, 0x93001420 */
	volatile UINT32 cdspHistgmLCnt;		/* R_CDSP_HISTGM_LCNT, 0x93001424 */
	volatile UINT32 cdspHistgmHCnt;		/* R_CDSP_HISTGM_HCNT, 0x93001428 */
	volatile UINT32 cdspAfWin1HVOffset;	/* R_CDSP_AF_WIN1_HVOFFSET, 0x9300142C */

	volatile UINT32 cdspAfWin1HVSize;	/* R_CDSP_AF_WIN1_HVSIZE, 0x93001430 */
	volatile UINT32 cdspAfWinCtrl;		/* R_CDSP_AF_WIN_CTRL, 0x93001434 */
	volatile UINT32 cdspAfWin2HVOffset;	/* R_CDSP_AF_WIN2_HVOFFSET, 0x93001438 */
	volatile UINT32 cdspAfWin3HVOffset;	/* R_CDSP_AF_WIN3_HVOFFSET, 0x9300143C */

	volatile UINT32 reserved00;			/* reserved00, 0x93001440 */
	volatile UINT32 reserved01;			/* reserved01, 0x93001444 */
	volatile UINT32 reserved02;			/* reserved02, 0x93001448 */
	volatile UINT32 reserved03;			/* reserved02, 0x9300144C */

	volatile UINT32 reserved04;			/* reserved02, 0x93001450 */
	volatile UINT32 cdspAwbWinCtrl;		/* R_CDSP_AWB_WIN_CTRL, 0x93001454 */
	volatile UINT32 cdspAwbSpWinYThr;	/* R_CDSP_AWB_SPECWIN_Y_THR, 0x93001458 */
	volatile UINT32 cdspAwbSpWinUvThr1;	/* R_CDSP_AWB_SPECWIN_UV_THR1, 0x9300145C */

	volatile UINT32 cdspAwbSpWinUvThr2;	/* R_CDSP_AWB_SPECWIN_UV_THR2, 0x93001460 */
	volatile UINT32 cdspAwbSpWinUvThr3;	/* R_CDSP_AWB_SPECWIN_UV_THR3, 0x93001464 */
	volatile UINT32 cdspSumCnt1;		/* R_CDSP_SUM_CNT1, 0x93001468 */
	volatile UINT32 cdspSumG1L;			/* R_CDSP_SUM_G1_L, 0x9300146c */

	volatile UINT32 cdspSumG1H;			/* R_CDSP_SUM_G1_H, 0x93001470 */
	volatile UINT32 cdspSumRg1L;		/* R_CDSP_SUM_RG1_L, 0x93001474 */
	volatile UINT32 cdspSumRg1H;		/* R_CDSP_SUM_RG1_H, 0x93001478 */
	volatile UINT32 cdspSumBg1L;		/* R_CDSP_SUM_BG1_L, 0x9300147C */

	volatile UINT32 cdspSumBg1H;		/* R_CDSP_SUM_BG1_H, 0x93001480 */
	volatile UINT32 cdspSumCnt2;		/* R_CDSP_SUM_CNT2, 0x93001484 */
	volatile UINT32 cdspSumG2L;			/* R_CDSP_SUM_G2_L, 0x93001488 */
	volatile UINT32 cdspSumG2H;			/* R_CDSP_SUM_G2_H, 0x9300148C */
	
	volatile UINT32 cdspSumRg2L;		/* R_CDSP_SUM_RG2_L, 0x93001490 */
	volatile UINT32 cdspSumRg2H;		/* R_CDSP_SUM_RG2_H, 0x93001494 */
	volatile UINT32 cdspSumBg2L;		/* R_CDSP_SUM_BG2_L, 0x93001498 */
	volatile UINT32 cdspSumBg2H;		/* R_CDSP_SUM_BG2_H, 0x9300149C */

	volatile UINT32 cdspSumCnt3;		/* R_CDSP_SUM_CNT3, 0x930014A0 */
	volatile UINT32 cdspSumG3L;			/* R_CDSP_SUM_G3_L, 0x930014A4 */
	volatile UINT32 cdspSumG3H;			/* R_CDSP_SUM_G3_H, 0x930014A8 */
	volatile UINT32 cdspSumRg3L;		/* R_CDSP_SUM_RG3_L, 0x930014AC */

	volatile UINT32 cdspSumRg3H;		/* R_CDSP_SUM_RG3_H, 0x930014B0 */
	volatile UINT32 cdspSumBg3L;		/* R_CDSP_SUM_BG3_L, 0x930014B4 */
	volatile UINT32 cdspSumBg3H;		/* R_CDSP_SUM_BG3_H, 0x930014B8 */
	volatile UINT32 reserved05;			/* reserved02, 0x930014BC */
	
	volatile UINT32 cdspAfWin1HValL;	/* R_CDSP_AF_WIN1_HVALUE_L, 0x930014C0 */
	volatile UINT32 cdspAfWin1HValH;	/* R_CDSP_AF_WIN1_HVALUE_H, 0x930014C4 */
	volatile UINT32 cdspAfWin1VValL;	/* R_CDSP_AF_WIN1_VVALUE_L, 0x930014C8 */
	volatile UINT32 cdspAfWin1VValH;	/* R_CDSP_AF_WIN1_VVALUE_H, 0x930014CC */

	volatile UINT32 cdspAfWin2HValL;	/* R_CDSP_AF_WIN2_HVALUE_L, 0x930014D0 */
	volatile UINT32 cdspAfWin2HValH;	/* R_CDSP_AF_WIN2_HVALUE_H, 0x930014D4 */
	volatile UINT32 cdspAfWin2VValL;	/* R_CDSP_AF_WIN2_VVALUE_L, 0x930014D8 */
	volatile UINT32 cdspAfWin2VValH;	/* R_CDSP_AF_WIN2_VVALUE_H, 0x930014DC */

	volatile UINT32 cdspAfWin3HValL;	/* R_CDSP_AF_WIN3_HVALUE_L, 0x930014E0 */
	volatile UINT32 cdspAfWin3HValH;	/* R_CDSP_AF_WIN3_HVALUE_H, 0x930014E4 */
	volatile UINT32 cdspAfWin3VValL;	/* R_CDSP_AF_WIN3_VVALUE_L, 0x930014E8 */
	volatile UINT32 cdspAfWin3VValH;	/* R_CDSP_AF_WIN3_VVALUE_H, 0x930014EC */

	volatile UINT32 reserved06;			/* reserved06, 0x930014F0 */
	volatile UINT32 reserved07;			/* reserved07, 0x930014F4 */
	volatile UINT32 reserved08;			/* reserved08, 0x930014F8 */
	volatile UINT32 cdspAefWinTest;		/* R_CDSP_AEF_WIN_TEST, 0x930014FC */		
}cdspReg3a_t;	

typedef struct cdspRegFront_s 
{
	volatile UINT8 offset00[0x0600];	/* offset00, 0x93001000 ~ 0x93001600*/

	volatile UINT32 cdspFrontCtrl0;		/* R_CDSP_FRONT_CTRL0, 0x93001600 */
	volatile UINT32 cdspFlashWidth;		/* R_CDSP_FLASH_WIDTH, 0x93001604 */
	volatile UINT32 cdspFlashTrigNum;	/* R_CDSP_FLASH_TRIG_NUM, 0x93001608 */
	volatile UINT32 cdspSnapCtrl;		/* R_CDSP_SNAP_CTRL, 0x9300160C */

	volatile UINT32 cdspFrontCtrl1;		/* R_CDSP_FRONT_CTRL1, 0x93001610 */
	volatile UINT32 cdspHSyncFrEdge;	/* R_CDSP_HSYNC_FREDGE, 0x93001614 */
	volatile UINT32 cdspVSyncFrEdge;	/* R_CDSP_VSYNC_FREDGE, 0x93001618 */
	volatile UINT32 cdspFrontCtrl2;		/* R_CDSP_FRONT_CTRL2, 0x9300161C */

	volatile UINT32 cdspFrameHSet;		/* R_CDSP_FRAME_H_SETTING, 0x93001620 */
	volatile UINT32 cdspFrameVSet;		/* R_CDSP_FRAME_V_SETTING, 0x93001624 */
	volatile UINT32 cdspTgLineCtrl;		/* R_CDSP_TG_LINE_CTRL, 0x93001628 */
	volatile UINT32 cdspFrameCtrl;		/* R_CDSP_FRAME_CTRL, 0x9300162C */

	volatile UINT32 cdspExthSepa;		/* R_CDSP_EXTH_SEPA, 0x93001630 */
	volatile UINT32 cdspTgLsLineNum;	/* R_CDSP_TG_LS_LINE_NUM, 0x93001634 */
	volatile UINT32 cdspFrontCtrl3;		/* R_CDSP_FRONT_CTRL3, 0x93001638 */
	volatile UINT32 cdspTgZero;			/* R_CDSP_TG_ZERO, 0x9300163C */

	volatile UINT32 cdspTgGpioSelOen;	/* R_CDSP_TG_GPIO_SEL_OEN, 0x93001640 */
	volatile UINT32 cdspTgGpioOutIn;	/* R_CDSP_TG_GPIO_OUT_IN, 0x93001644 */
	volatile UINT32 cdspTgGpioREvent;	/* R_CDSP_TG_GPIO_REVENT, 0x93001648 */
	volatile UINT32 cdspTgGpioFEvent;	/* R_CDSP_TG_GPIO_FEVENT, 0x9300164C */

	volatile UINT32 cdspMipiCtrl;		/* R_CDSP_MIPI_CTRL, 0x93001650 */
	volatile UINT32 cdspMipiHVOffset;	/* R_CDSP_MIPI_HVOFFSET, 0x93001654 */
	volatile UINT32 cdspMipiHVSize;		/* R_CDSP_MIPI_HVSIZE, 0x93001658 */
	volatile UINT32 cdspFrontCtrl4;		/* R_CDSP_FRONT_CTRL4, 0x9300165C */
	
	volatile UINT32 cdspSonySenData;	/* R_CDSP_SONY_SEN_DATA, 0x93001660 */
	volatile UINT32 cdspFrontGCLK;		/* R_CDSP_FRONT_GCLK, 0x93001664 */
	volatile UINT32 cdspSenCtrlSig;		/* R_CDSP_SEN_CTRL_SIG, 0x93001668 */
	volatile UINT32 cdspInt;			/* R_CDSP_FRONT_INT, 0x9300166C */

	volatile UINT32 cdspVdRfoccInt;		/* R_CDSP_VD_RFOCC_INT, 0x93001670 */
	volatile UINT32 cdspInthNum;		/* R_CDSP_INTH_NUM, 0x93001674 */
	volatile UINT32 cdspIntEn;			/* R_CDSP_FRONT_INTEN, 0x93001678 */
	volatile UINT32 cdspVdrfInt;		/* R_CDSP_FRONT_VDRF_INT, 0x9300167C */

	volatile UINT32 cdspVdrfIntEn;		/* R_CDSP_FRONT_VDRF_INTEN, 0x93001680 */
	volatile UINT32 cdspSigGen;			/* R_CDSP_SIG_GEN, 0x93001684 */
	volatile UINT32 cdspProbeCtrl;		/* R_CDSP_FRONT_PROBE_CTRL, 0x93001688 */
	volatile UINT32 cdspDummy;			/* R_CDSP_FRONT_DUMMY, 0x9300168C */

	volatile UINT32 cdspFpiCnt;			/* R_CDSP_FPICNT, 0x93001690 */
	volatile UINT32 cdspExtRgb;			/* R_CDSP_EXTRGB, 0x93001694 */
}cdspRegFront_t;	

typedef struct cdspRegLensCmp_s 
{
	volatile UINT8 offset[0x0800];		/* offset00, 0x93001000 ~ 0x93001800*/
	volatile UINT32 LensTable[256];		/* 0x00~0xFF */        
}cdspRegLensCmp_t;

typedef struct cdspRegLutGamma_s 
{
	volatile UINT8 offset[0x0800];		/* offset00, 0x93001000 ~ 0x93001800*/
	volatile UINT32 GammaTable[128];	/* 0x000~0x3FF */        
}cdspRegLutGamma_t;

typedef struct cdspRegLutEdge_s 
{
	volatile UINT8 offset[0x0800];		/* offset00, 0x93001000 ~ 0x93001800*/
	volatile UINT32 EdgeTable[256];		/* 0x000~0x3FF */        
}cdspRegLutEdge_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
 
#endif /* _REG_CDSP_H_ */



