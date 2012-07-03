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
 * @file    hal_scale.c
 * @brief   Implement of SPMP8050 Scale HAL API
 * @author  qinjian
 * @since   2010/10/8
 * @date    2010/10/8
 */
#include <mach/kernel.h>
#include <mach/hal/hal_scale.h>
#include <mach/hal/regmap/reg_scale.h>
#include <mach/hal/regmap/reg_scu.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define SCALE_METHOD_DUP_DROP       0
#define SCALE_METHOD_INTERPOLATION  1

#define SCALE_HW_WIDTH_LIMIT        1024

#define R_IMG_SRC_PITCH_OFFMSK		0xFFFF0000
#define R_IMG_SRC_PITCH_MSK			0xFFFF
#define R_IMG_SRC_PITCH_OFFSET		16
#define R_IMG_ACT_PITCH_OFFMSK		0x0000FFFF
#define R_IMG_ACT_PITCH_MSK			0xFFFF
#define R_IMG_ACT_PITCH_OFFSET		0

#define R_IMG_ACT_WIDTH_OFFMSK		0xFFFF0000
#define R_IMG_ACT_WIDTH_MSK			0xFFFF
#define R_IMG_ACT_WIDTH_OFFSET		16
#define R_IMG_ACT_HEIGHT_OFFMSK		0x0000FFFF
#define R_IMG_ACT_HEIGHT_MSK		0xFFFF
#define R_IMG_ACT_HEIGHT_OFFSET		0

#define R_IMG_Cb_PITCH_OFFMSK		0xFFFF0000
#define R_IMG_Cb_PITCH_MSK			0xFFFF
#define R_IMG_Cb_PITCH_OFFSET		16

#define R_IMG_Cr_PITCH_OFFMSK		0x0000FFFF
#define R_IMG_Cr_PITCH_MSK			0xFFFF
#define R_IMG_Cr_PITCH_OFFSET		0

#define R_MCB_OUT_PITCH_OFFMSK		0xFFFF0000
#define R_MCB_OUT_PITCH_MSK			0xFFFF
#define R_MCB_OUT_PITCH_OFFSET		16
#define R_MCB_IMG_PITCH_OFFMSK		0x0000FFFF
#define R_MCB_IMG_PITCH_MSK			0xFFFF
#define R_MCB_IMG_PITCH_OFFSET		0

#define R_OUT_DST_PITCH_OFFMSK		0xFFFF0000
#define R_OUT_DST_PITCH_MSK			0xFFFF
#define R_OUT_DST_PITCH_OFFSET		16
#define R_OUT_SCL_PITCH_OFFMSK		0x0000FFFF
#define R_OUT_SCL_PITCH_MSK			0xFFFF
#define R_OUT_SCL_PITCH_OFFSET		0

#define R_OUT_SCLWIDTH_MSK			0xFFF
#define R_OUT_SCLWIDTH_OFFSET		16
#define R_OUT_SCLHEIGHT_MSK			0xFFF
#define R_OUT_SCLHEIGHT_OFFSET		0

#define R_SCALE_HF_HINITIAL_OFFMSK	0xFFFF0000
#define R_SCALE_HF_HINITIAL_MSK		0xFFFF
#define R_SCALE_HF_HINITIAL_OFFSET	16
#define R_SCALE_HF_HFACTOR_MSK		0xFFFF
#define R_SCALE_HF_HFACTOR_OFFSET	0

#define R_SCALE_VF_VINITIAL_OFFMSK	0xFFFF0000
#define R_SCALE_VF_VINITIAL_MSK		0xFFFF
#define R_SCALE_VF_VINITIAL_OFFSET	16
#define R_SCALE_VF_VFACTOR_MSK		0xFFFF
#define R_SCALE_VF_VFACTOR_OFFSET	0

/* Dither map */
#define R_DITHER_MAP_DXX_MSK		0xF
#define R_DITHER_MAP_D00_OFFSET		28
#define R_DITHER_MAP_D01_OFFSET		24
#define R_DITHER_MAP_D02_OFFSET		20
#define R_DITHER_MAP_D03_OFFSET		16
#define R_DITHER_MAP_D10_OFFSET		12
#define R_DITHER_MAP_D11_OFFSET		8
#define R_DITHER_MAP_D12_OFFSET		4
#define R_DITHER_MAP_D13_OFFSET		0

#define R_DITHER_MAP_D20_OFFSET		28
#define R_DITHER_MAP_D21_OFFSET		24
#define R_DITHER_MAP_D22_OFFSET		20
#define R_DITHER_MAP_D23_OFFSET		16
#define R_DITHER_MAP_D30_OFFSET		12
#define R_DITHER_MAP_D31_OFFSET		8
#define R_DITHER_MAP_D32_OFFSET		4
#define R_DITHER_MAP_D33_OFFSET		0

/* Scale control mask */
#define R_SCLCTRL_BTYPE_OFFMSK		(0x3<<28)
#define R_SCLCTRL_BTYPE_SINGLE		(0)
#define R_SCLCTRL_BTYPE_INCR4		(0x1<<28)
#define R_SCLCTRL_BTYPE_INCR8		(0x2<<28)
#define R_SCLCTRL_BTYPE_INCR16		(0x3<<28)

#define R_SCLCTRL_VENABLE_FLG		(0x1<<27)
#define R_SCLCTRL_VSELECT_FLG		(0x1<<26)
#define R_SCLCTRL_VSELECT_UP		(0x0)
#define R_SCLCTRL_VSELECT_DN		(0x1<<26)
#define R_SCLCTRL_HENABLE_FLG		(0x1<<25)
#define R_SCLCTRL_HSELECT_FLG		(0x1<<24)
#define R_SCLCTRL_HSELECT_UP		(0x0)
#define R_SCLCTRL_HSELECT_DN		(0x1<<24)
#define R_SCLCTRL_SCLFUNC_FLG		(0x1<<23)	/* 0: Interpolation. 1:Duplication or Drop. */
#define R_SCLCTRL_VBB_FLG			(0x1<<20)	/* Vertically scaling up block by block. */

#define R_SCLCTRL_CTYPE_OFFMSK		(0x3<<18)
#define R_SCLCTRL_CTYPE_OP0			(0x0)		/* RGB565, YCbCr,		YCbCr400 */
#define R_SCLCTRL_CTYPE_OP1			(0x1<<18)	/* RGB555, 4Y4Cb4Y4YCr,	YCbCr420*/
#define R_SCLCTRL_CTYPE_OP2			(0x2<<18)	/* RGB888, YCbCr,		YCbCr422 */
#define R_SCLCTRL_CTYPE_OP3			(0x3<<18)	/* RGB888, YCbCr,		YCbCr444 */

#define R_SCLCTRL_CFORMAT_OFFMSK	(0x3<<16)	/* 0: RGB color space, 1: YCbCr color space, 2: YCbCr Separation format. */
#define R_SCLCTRL_CFORMAT_RGB		0
#define R_SCLCTRL_CFORMAT_YCbCr		(0x1<<16)
#define R_SCLCTRL_CFORMAT_YCbCrSEP	(0x2<<16)

#define R_SCLCTRL_ODITHSEQ_OFFMSK	(0x3<<10)
#define R_SCLCTRL_ODITHSEQ_OFFSET	10
#define R_SCLCTRL_ODITHSEQ_MSK		0x3
#define R_SCLCTRL_ODITHSEL_FLG		(0x1<<9)
#define R_SCLCTRL_ODITHSEL_OFFSET	9
#define R_SCLCTRL_ODITHSEL_ORDER	0
#define R_SCLCTRL_ODITHSEL_HERRDIFF	(0x1<<9)
#define R_SCLCTRL_ODITHER_FLG		(0x1<<8)

#define R_SCLCTRL_ODDLINE_FLG		(0x1<<7)	/* For YUV420, the start row is set on the odd row (count from 0). */

#define R_SCLCTRL_OFUNC_FLG			(0x1<<3)	/* 0: 0-255 RGB <-> 0-255 YCbCr, 1: 0-255 RGB <-> 16-235 YCbCr */

#define R_SCLCTRL_OTYPE_OFFMSK		(0x3<<1)
#define R_SCLCTRL_OTYPE_OP0			0			/* RGB565, YCbYCr */
#define R_SCLCTRL_OTYPE_OP1			(0x1<<1)	/* RGB1555, 4Y4Cb4Y4YCr */
#define R_SCLCTRL_OTYPE_OP2			(0x2<<1)	/* RGB888, YCbCr */
#define R_SCLCTRL_OTYPE_OP3			(0x3<<1)	/* RGB5515, YCbYCr(avg: caculate the average of Cb, Cr.) */

#define R_SCLCTRL_OFORMAT_OFFMSK	0x1
#define R_SCLCTRL_OFORMAT_RGB		0
#define R_SCLCTRL_OFORMAT_YCbCr		1

#define R_INTSRC_RESET_FLG			(0x4)
#define R_INTSRC_SCLDONE_FLG		(0x2)
#define R_INTSRC_SCLSTART_FLG		(0x1)

#define SCALE_INT_DISABLE           0
#define SCALE_INT_ENABLE            2

/* Scale color space */
#define SE_CSPACE_RGB				0
#define SE_CSPACE_YCbCr				1
#define SE_CSPACE_YCbCrSEP			2

/* Scale color format */
#define SE_CFMT_RGB565				0x00
#define SE_CFMT_RGB1555		    	0x01
#define SE_CFMT_RGB888				0x02
#define SE_CFMT_RGB5515		    	0x03

#define SE_CFMT_YCbYCr				0x10
#define SE_CFMT_4Y4Cb4Y4Cr			0x11
#define SE_CFMT_YCbCr				0x12

#define SE_CFMT_YCbCr400			0x20
#define SE_CFMT_YCbCr420			0x21
#define SE_CFMT_YCbCr422			0x22
#define SE_CFMT_YCbCr444			0x23

#define SE_CFMT_UNSUPPORTED         0xFF

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

#define WRITE_REG_BITS(reg, mask, shift, value) \
	do { \
		reg = (reg & ~((mask) << (shift))) | (((value) & (mask)) << (shift)); \
	} while (0)

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

static scaleReg_t *scaleReg = (scaleReg_t *)LOGI_ADDR_SCALE_REG;
static scucReg_t *scucReg = (scucReg_t *)LOGI_ADDR_SCU_C_REG;

/**
 * @brief   Get scale color format by bitmap type
 * @param   type [in] bitmap type
 * @return  success: scale color format, fail: SE_CFMT_UNSUPPORTED
 * @see     scaleSetSrcImg
 */
static UINT32
scaleGetFmtByBmpType(
	UINT32 type
)
{
	UINT32 result;

	switch (type) {

	case SP_BITMAP_RGB565:
		result = SE_CFMT_RGB565;
		break;
	case SP_BITMAP_ARGB1555:
		result = SE_CFMT_RGB1555;
		break;
	case SP_BITMAP_RGAB5515:
		result = SE_CFMT_RGB5515;
		break;
	case SP_BITMAP_RGB888:
		result = SE_CFMT_RGB888;
		break;

	case SP_BITMAP_YUYV:
	case SP_BITMAP_YCbYCr:
		result = SE_CFMT_YCbYCr;
		break;
	case SP_BITMAP_4Y4U4Y4V:
	case SP_BITMAP_4Y4Cb4Y4Cr:
		result = SE_CFMT_4Y4Cb4Y4Cr;
		break;
	case SP_BITMAP_YUV:
	case SP_BITMAP_YCbCr:
		result = SE_CFMT_YCbCr;
		break;

	case SP_BITMAP_YUV400:
	case SP_BITMAP_YCbCr400:
		result = SE_CFMT_YCbCr400;
		break;
	case SP_BITMAP_YUV420:
	case SP_BITMAP_YCbCr420:
		result = SE_CFMT_YCbCr420;
		break;
	case SP_BITMAP_YUV422:
	case SP_BITMAP_YCbCr422:
		result = SE_CFMT_YCbCr422;
		break;
	case SP_BITMAP_YUV444:
	case SP_BITMAP_YCbCr444:
		result = SE_CFMT_YCbCr444;
		break;

	default:
		result = SE_CFMT_UNSUPPORTED;
		break;
	}

	return result;
}

/**
 * @brief   Get bpp by scale color format
 * @param   format [in] scale color format
 * @return  success: bpp,  fail: 0(unsupported color format)
 * @see     scaleSetSrcImg
 */
static UINT32
scaleGetBppByFmt(
	UINT32 format
)
{
	UINT32 result;

	switch (format) {
	case SE_CFMT_RGB888:
	case SE_CFMT_YCbCr:
		result = 3;
		break;

	case SE_CFMT_RGB565:
	case SE_CFMT_RGB1555:
	case SE_CFMT_RGB5515:
	case SE_CFMT_YCbYCr:
	case SE_CFMT_4Y4Cb4Y4Cr:
		result = 2;
		break;

	case SE_CFMT_YCbCr400:
	case SE_CFMT_YCbCr420:
	case SE_CFMT_YCbCr422:
	case SE_CFMT_YCbCr444:
		result = 1; /* Only consider Y component. */
		break;

	default:
		result = 0;
		break;
	}

	return result;
}

/**
 * @brief   Scale source image color format setting
 * @param   format [in] color format
 * @return  SP_OK(0)/SP_FAIL
 * @see     scaleSetSrcImg
 */
static UINT32
scaleSetSrcFmt(
	UINT32 format
)
{
	UINT32 regVal;

	regVal = scaleReg->scaleCtrl;
	regVal &= ~(R_SCLCTRL_CFORMAT_OFFMSK | R_SCLCTRL_CTYPE_OFFMSK);

	switch (format) {
	case SE_CFMT_RGB565:
		regVal |= (R_SCLCTRL_CFORMAT_RGB | R_SCLCTRL_CTYPE_OP0);
		break;
	case SE_CFMT_RGB1555:
		regVal |= (R_SCLCTRL_CFORMAT_RGB | R_SCLCTRL_CTYPE_OP1);
		break;
	case SE_CFMT_RGB888:
		regVal |= (R_SCLCTRL_CFORMAT_RGB | R_SCLCTRL_CTYPE_OP2);
		break;
	case SE_CFMT_RGB5515:
		regVal |= (R_SCLCTRL_CFORMAT_RGB | R_SCLCTRL_CTYPE_OP3);
		break;

	case SE_CFMT_YCbYCr:
		regVal |= (R_SCLCTRL_CFORMAT_YCbCr | R_SCLCTRL_CTYPE_OP0);
		break;
	case SE_CFMT_4Y4Cb4Y4Cr:
		regVal |= (R_SCLCTRL_CFORMAT_YCbCr | R_SCLCTRL_CTYPE_OP1);
		break;
	case SE_CFMT_YCbCr:
		regVal |= (R_SCLCTRL_CFORMAT_YCbCr | R_SCLCTRL_CTYPE_OP2);
		break;

	case SE_CFMT_YCbCr400:
		regVal |= (R_SCLCTRL_CFORMAT_YCbCrSEP | R_SCLCTRL_CTYPE_OP0);
		break;
	case SE_CFMT_YCbCr420:
		regVal |= (R_SCLCTRL_CFORMAT_YCbCrSEP | R_SCLCTRL_CTYPE_OP1);
		break;
	case SE_CFMT_YCbCr422:
		regVal |= (R_SCLCTRL_CFORMAT_YCbCrSEP | R_SCLCTRL_CTYPE_OP2);
		break;
	case SE_CFMT_YCbCr444:
		regVal |= (R_SCLCTRL_CFORMAT_YCbCrSEP | R_SCLCTRL_CTYPE_OP3);
		break;

	default:
		return SP_FAIL; /* unsupported color format */
	}

	scaleReg->scaleCtrl = regVal;

	return SP_OK;
}

/**
 * @brief   Scale destination image color format setting
 * @param   format [in] color format
 * @return  SP_OK(0)/SP_FAIL
 * @see     scaleSetDstImg
 */
static UINT32
scaleSetDstFmt(
	UINT32 format
)
{
	UINT32 regVal;

	regVal = scaleReg->scaleCtrl;
	regVal &= ~(R_SCLCTRL_OFORMAT_OFFMSK | R_SCLCTRL_OTYPE_OFFMSK);

	switch (format) {
	case SE_CFMT_RGB565:
		regVal |= (R_SCLCTRL_OFORMAT_RGB | R_SCLCTRL_OTYPE_OP0);
		break;
	case SE_CFMT_RGB1555:
		regVal |= (R_SCLCTRL_OFORMAT_RGB | R_SCLCTRL_OTYPE_OP1);
		break;
	case SE_CFMT_RGB888:
		regVal |= (R_SCLCTRL_OFORMAT_RGB | R_SCLCTRL_OTYPE_OP2);
		break;
	case SE_CFMT_RGB5515:
		regVal |= (R_SCLCTRL_OFORMAT_RGB | R_SCLCTRL_OTYPE_OP3);
		break;

	case SE_CFMT_YCbYCr:
		regVal |= (R_SCLCTRL_OFORMAT_YCbCr | R_SCLCTRL_OTYPE_OP0);
		break;
	case SE_CFMT_4Y4Cb4Y4Cr:
		regVal |= (R_SCLCTRL_OFORMAT_YCbCr | R_SCLCTRL_OTYPE_OP1);
		break;
	case SE_CFMT_YCbCr:
		regVal |= (R_SCLCTRL_OFORMAT_YCbCr | R_SCLCTRL_OTYPE_OP2);
		break;

	default:
		return SP_FAIL; /* unsupported color format */
	}

	scaleReg->scaleCtrl = regVal;

	return SP_OK;
}

/**
 * @brief   Scale hardware source image setting
 * @param   srcImg [in] source image bitmap
 * @param   clipRgn [in] clip region
 * @return  SP_OK(0)/SP_FAIL
 * @see
 */
static UINT32
scaleSetSrcImg(
	gp_bitmap_t *srcImg,
	gp_rect_t *clipRgn
)
{
	UINT32 fmt, bpp;
	UINT32 srcPitch, actPitch;
	UINT32 startAlignMask, widthAlignMask;

	/* check parameter */
	fmt = scaleGetFmtByBmpType(srcImg->type);
	if (fmt == SE_CFMT_UNSUPPORTED) {
		DIAG_ERROR("[scaleSetSrcImg] unsupported color format.\n");
		return SP_ERR_FORMAT_UNSUPPORTED;
	}
	/* check source pitch */
	if ((srcImg->bpl & 0x03) != 0) {
		DIAG_ERROR("[scaleSetSrcImg] srcPitch is invalid.\n");
		return SP_ERR_HW_LIMIT;
	}
	/* check clip region */
	if ((clipRgn->x < 0)
		|| (clipRgn->y < 0)
		|| ((clipRgn->x + clipRgn->width) > srcImg->width)
		|| ((clipRgn->y + clipRgn->height) > srcImg->height)) {
		DIAG_ERROR("[scaleSetSrcImg] clip region must in src image.\n");
		return SP_ERR_PARAM;
	}

	/* refine clip region */
	switch (fmt) {
	case SE_CFMT_YCbCr420: /* width of yuv420 and must align 16. */
	case SE_CFMT_YCbCr422:
	case SE_CFMT_4Y4Cb4Y4Cr:
		startAlignMask = (~0x7);
		widthAlignMask = (~0xF);
		break;
	case SE_CFMT_YCbCr400:
	case SE_CFMT_YCbCr444:
	case SE_CFMT_RGB888:
	case SE_CFMT_YCbCr:
		startAlignMask = widthAlignMask = (~0x3);
		break;	
	default:
		startAlignMask = widthAlignMask = (~0x1);
		break;
	}
	clipRgn->x &= startAlignMask;
	clipRgn->width &= (clipRgn->width < 0xf) ? (~0x1) : widthAlignMask;
	DIAG_VERB("Src clip region: %d,%d %dx%d\n", clipRgn->x, clipRgn->y, clipRgn->width, clipRgn->height);

	/* set source image color format */
	scaleSetSrcFmt(fmt);

	/* set source image Y/U/V address */
	bpp = scaleGetBppByFmt(fmt);
	scaleReg->scaleFbAddr = (UINT32)srcImg->pData + \
		clipRgn->y * srcImg->bpl + clipRgn->x * bpp;

	scaleReg->scaleCtrl &= ~R_SCLCTRL_ODDLINE_FLG; /* clear oddline flag */

	switch (fmt) {
	case SE_CFMT_YCbCr420:
		scaleReg->scaleCbAddr = (UINT32)srcImg->pDataU + \
			(clipRgn->y >> 1) * srcImg->strideUV + (clipRgn->x >> 1);
		scaleReg->scaleCrAddr = (UINT32)srcImg->pDataV + \
			(clipRgn->y >> 1) * srcImg->strideUV + (clipRgn->x >> 1);

		if ((clipRgn->y & 1) != 0) {
			scaleReg->scaleCtrl |= R_SCLCTRL_ODDLINE_FLG; /* set oddline flag */
		}
		break;

	case SE_CFMT_YCbCr422:
		scaleReg->scaleCbAddr = (UINT32)srcImg->pDataU + \
			clipRgn->y * srcImg->strideUV + (clipRgn->x >> 1);
		scaleReg->scaleCrAddr = (UINT32)srcImg->pDataV + \
			clipRgn->y * srcImg->strideUV + (clipRgn->x >> 1);
		break;

	case SE_CFMT_YCbCr444:
		scaleReg->scaleCbAddr = (UINT32)srcImg->pDataU + \
			clipRgn->y * srcImg->strideUV + clipRgn->x;
		scaleReg->scaleCrAddr = (UINT32)srcImg->pDataV + \
			clipRgn->y * srcImg->strideUV + clipRgn->x;
		break;

	default:
		scaleReg->scaleCbAddr = 0;
		scaleReg->scaleCrAddr = 0;
		break;
	}

	/* set source image resolution */
	scaleReg->scaleImgRes = (clipRgn->width << R_IMG_ACT_WIDTH_OFFSET) \
		| (clipRgn->height << R_IMG_ACT_HEIGHT_OFFSET);

	/* set source image pitches */
	srcPitch = srcImg->bpl;
	actPitch = clipRgn->width * bpp;
	scaleReg->scaleImgPitch = (srcPitch << R_IMG_SRC_PITCH_OFFSET) \
		| (actPitch << R_IMG_ACT_PITCH_OFFSET); /* source & actual pitch */
	scaleReg->scaleCbCrPitch = (srcImg->strideUV << R_IMG_Cb_PITCH_OFFSET) \
		| (srcImg->strideUV << R_IMG_Cr_PITCH_OFFSET); /* CbCr pitch */

	return SP_OK;
}

#if 0
/**
 * @brief   Scale hardware vertical scale resolution setting
 * @param   sclHeight [in] dest height
 * @param   actWidth [in] source width
 * @param   actHeight [in] source height
 * @return  SP_OK(0)/SP_FAIL
 * @see
 */
static UINT32
scaleSetDstVertRes(
	UINT32 sclHeight,
	UINT32 actWidth,
	UINT32 actHeight,
)
{
	UINT32 sclFactorV;
	UINT32 sclCtrl;

	/* set scale resolution */
	scaleReg->scaleOutRes = (actWidth << R_OUT_SCLWIDTH_OFFSET) \
		| (sclHeight << R_OUT_SCLHEIGHT_OFFSET);

	/* init scale control */
	sclCtrl = scaleReg->scaleCtrl;
	sclCtrl &= ~(R_SCLCTRL_BTYPE_OFFMSK | R_SCLCTRL_SCLFUNC_FLG | R_SCLCTRL_VBB_FLG
				| R_SCLCTRL_HENABLE_FLG | R_SCLCTRL_VENABLE_FLG);
	sclCtrl |= R_SCLCTRL_VBB_FLG;     /* enable vertically scaling up block by block */
	sclCtrl |= R_SCLCTRL_BTYPE_INCR4; /* VBB mode can only use INCR4 */

	/* set vertical scaling factor */
	sclFactorV = 0;
	if (sclHeight != actHeight) {
		if (sclHeight > actHeight) {    /* vertical scaling up */
			sclFactorV = (actHeight << 16) / sclHeight;
			sclCtrl |= (R_SCLCTRL_VSELECT_UP | R_SCLCTRL_VENABLE_FLG);
		}
		else {                          /* vertical scaling down */
			sclFactorV = ((sclHeight << 16) + (actHeight - 1)) / actHeight;
			sclCtrl |= (R_SCLCTRL_VSELECT_DN | R_SCLCTRL_VENABLE_FLG);
		}

		if (sclFactorV > R_SCALE_VF_VFACTOR_MSK) { /* scale factor error. */
			sclCtrl &= ~(R_SCLCTRL_VENABLE_FLG); /* turn off vertical scaling */
			sclFactorV = 0;
		}
	}
	WRITE_REG_BITS(scaleReg->scaleParamV,
				   R_SCALE_VF_VFACTOR_MSK,
				   R_SCALE_VF_VFACTOR_OFFSET,
				   sclFactorV);

	/* update scale control register */
	scaleReg->scaleCtrl = sclCtrl;

	return SP_OK;
}
#endif

/**
 * @brief   Scale hardware destination scale resolution setting
 * @param   sclWidth [in] dest width
 * @param   sclHeight [in] dest height
 * @param   actWidth [in] source width
 * @param   actHeight [in] source height
 * @return  SP_OK(0)/SP_FAIL
 * @see
 */
static UINT32
scaleSetDstRes(
	UINT32 sclWidth,
	UINT32 sclHeight,
	UINT32 actWidth,
	UINT32 actHeight
)
{
	UINT32 sclFactorH, sclFactorV;
	UINT32 sclCtrl;
	UINT32 scaleMethod = SCALE_METHOD_INTERPOLATION; /* default value */

	/* set scale resolution */
	scaleReg->scaleOutRes = (sclWidth << R_OUT_SCLWIDTH_OFFSET) \
		| (sclHeight << R_OUT_SCLHEIGHT_OFFSET);

	/* init scale control */
	sclCtrl = scaleReg->scaleCtrl;
	sclCtrl &= ~(R_SCLCTRL_BTYPE_OFFMSK | R_SCLCTRL_SCLFUNC_FLG | R_SCLCTRL_VBB_FLG
				| R_SCLCTRL_HSELECT_FLG | R_SCLCTRL_VSELECT_FLG
				| R_SCLCTRL_HENABLE_FLG | R_SCLCTRL_VENABLE_FLG);
	sclCtrl |= R_SCLCTRL_BTYPE_INCR8; /* default value */

	if ((sclWidth > SCALE_HW_WIDTH_LIMIT) && (sclWidth != actWidth) && (sclHeight != actHeight)) {
		/* use Dup/Drop method instead Interpolation */
		scaleMethod = SCALE_METHOD_DUP_DROP;
		sclCtrl |= R_SCLCTRL_SCLFUNC_FLG;
	}

	/* set horizontal scaling factor */
	sclFactorH = 0;
	if (sclWidth != actWidth) {
		if (scaleMethod == SCALE_METHOD_DUP_DROP) {
			sclFactorH = ((actWidth - 1) << 11) / (sclWidth - 1);
			sclCtrl |= R_SCLCTRL_HENABLE_FLG;
		}
		else {
			if (sclWidth > actWidth) {      /* horizontal scaling up */
				sclFactorH = (actWidth << 16) / sclWidth;
				sclCtrl |= (R_SCLCTRL_HSELECT_UP | R_SCLCTRL_HENABLE_FLG);
			}
			else {                          /* horizontal scaling down */
				sclFactorH = ((sclWidth << 16) + (actWidth - 1)) / actWidth;
				sclCtrl |= (R_SCLCTRL_HSELECT_DN | R_SCLCTRL_HENABLE_FLG);
			}
		}

		if (sclFactorH > R_SCALE_HF_HFACTOR_MSK) { /* scale factor error */
			sclCtrl &= ~(R_SCLCTRL_HENABLE_FLG); /* turn off horizontal scaling */
			sclFactorH = 0;
		}
	}
#if 0
	WRITE_REG_BITS(scaleReg->scaleParamH,
				   R_SCALE_HF_HFACTOR_MSK,
				   R_SCALE_HF_HFACTOR_OFFSET,
				   sclFactorH);
#else
	scaleReg->scaleParamH = sclFactorH;
#endif

	/* set vertical scaling factor */
	sclFactorV = 0;
	if (sclHeight != actHeight) {
		if (scaleMethod == SCALE_METHOD_DUP_DROP) {
			sclFactorV = ((actHeight - 1) << 11) / (sclHeight - 1);
			sclCtrl |= R_SCLCTRL_VENABLE_FLG;
		}
		else {
			if (sclHeight > actHeight) {    /* vertical scaling up */
				sclFactorV = (actHeight << 16) / sclHeight;
				sclCtrl |= (R_SCLCTRL_VSELECT_UP | R_SCLCTRL_VENABLE_FLG);
			}
			else {                          /* vertical scaling down */
				sclFactorV = ((sclHeight << 16) + (actHeight - 1)) / actHeight;
				sclCtrl |= (R_SCLCTRL_VSELECT_DN | R_SCLCTRL_VENABLE_FLG);
			}
		}

		if (sclFactorV > R_SCALE_VF_VFACTOR_MSK) { /* scale factor error. */
			sclCtrl &= ~(R_SCLCTRL_VENABLE_FLG); /* turn off vertical scaling */
			sclFactorV = 0;
		}
	}
#if 0
	WRITE_REG_BITS(scaleReg->scaleParamV,
				   R_SCALE_VF_VFACTOR_MSK,
				   R_SCALE_VF_VFACTOR_OFFSET,
				   sclFactorV);
#else
	scaleReg->scaleParamV = sclFactorV;
#endif

	/* update scale control register */
	scaleReg->scaleCtrl = sclCtrl | 0x30000000; /* Burst type: INCR16 */

	return SP_OK;
}

/**
 * @brief   Scale hardware destination image setting
 * @param   dstImg [in] destination image bitmap
 * @param   scaleRgn [in] scale region
 * @return  SP_OK(0)/SP_FAIL
 * @see
 */
static UINT32
scaleSetDstImg(
	gp_bitmap_t *dstImg,
	gp_rect_t *scaleRgn
)
{
	UINT32 fmt, bpp;
	UINT32 dstPitch, sclPitch;
	UINT32 sclWidth, sclHeight;
	UINT32 actWidth, actHeight;
	UINT32 startAlignMask, widthAlignMask;

	/* check parameter */
	fmt = scaleGetFmtByBmpType(dstImg->type);
	if (fmt == SE_CFMT_UNSUPPORTED) {
		DIAG_ERROR("[scaleSetDstImg] unsupported color format.\n");
		return SP_ERR_FORMAT_UNSUPPORTED;
	}
	/* check scale region */
	if ((scaleRgn->x < 0)
		|| (scaleRgn->y < 0)
		|| ((scaleRgn->x + scaleRgn->width) > dstImg->width)
		|| ((scaleRgn->y + scaleRgn->height) > dstImg->height)) {
		DIAG_ERROR("[scaleSetDstImg] scale region must in dst image.\n");
		return SP_ERR_PARAM;
	}

	/* refine scale region */
	switch (fmt) {
	case SE_CFMT_4Y4Cb4Y4Cr:
		startAlignMask = (~0x7);
		widthAlignMask = (~0xF);
		break;
	case SE_CFMT_RGB888:
	case SE_CFMT_YCbCr:
		startAlignMask = widthAlignMask = (~0x3);
		break;
	default:
		startAlignMask = widthAlignMask = (~0x1);
		break;
	}
	scaleRgn->x &= startAlignMask;
	scaleRgn->width &= widthAlignMask;
#if 0
	/* If scale pitch exceeds line buffer width, must use VBB scale instead. */
	if (scaleRgn->width > SCALE_HW_WIDTH_LIMIT) {
		scaleRgn->width &= (~0xF); /* Width must align to 16 while using VBB. */
	}
#endif
	DIAG_VERB("Dst scale region: %d,%d %dx%d\n", scaleRgn->x, scaleRgn->y, scaleRgn->width, scaleRgn->height);

	/* set destination image color format */
	scaleSetDstFmt(fmt);

	/* set destination image address */
	bpp = scaleGetBppByFmt(fmt);
	scaleReg->scaleWbAddr = (UINT32)dstImg->pData + \
		scaleRgn->y * dstImg->bpl + scaleRgn->x * bpp;

	/* set destination image resolution */
	sclWidth  = scaleRgn->width;
	sclHeight = scaleRgn->height;
	actWidth  = (scaleReg->scaleImgRes >> R_IMG_ACT_WIDTH_OFFSET) & R_IMG_ACT_WIDTH_MSK;
	actHeight = (scaleReg->scaleImgRes >> R_IMG_ACT_HEIGHT_OFFSET) & R_IMG_ACT_HEIGHT_MSK;
	if (sclWidth == 0) {
		sclWidth = actWidth;
	}
	if (sclHeight == 0) {
		sclHeight = actHeight;
	}

#if 1
	scaleSetDstRes(sclWidth, sclHeight, actWidth, actHeight);
#else
	scaleSetDstVertRes(sclHeight, actWidth, actHeight);
	/* set marco-block pitch */
	{
		UINT32 actPitch = (scaleReg->scaleImgPitch >> R_IMG_ACT_PITCH_OFFSET) & R_IMG_ACT_PITCH_MSK;
		UINT32 imgPitch = 16 * (actPitch / actWidth);
		UINT32 outPitch = 16 * bpp;
		scaleReg->scaleMcbPitch = (imgPitch << R_MCB_IMG_PITCH_OFFSET) \
			| (outPitch << R_MCB_OUT_PITCH_OFFSET);
	}
#endif

	/* set destination image pitches */
	dstPitch = dstImg->bpl;
	sclPitch = sclWidth * bpp;
	scaleReg->scaleOutPitch = (dstPitch << R_OUT_DST_PITCH_OFFSET) \
		| (sclPitch << R_OUT_SCL_PITCH_OFFSET); /* dest & scale pitch */

	return SP_OK;
}

/**
 * @brief   Scale hardware start scaling
 * @return  None
 * @see
 */
static void
scaleStart(
	void
)
{
	scaleReg->scaleIntSrc = R_INTSRC_SCLSTART_FLG | R_INTSRC_SCLDONE_FLG;
}

/**
 * @brief   Scale hardware dither setting
 * @param   mode [in] dither mode (SCALE_DITHER_MODE_xxx)
 * @param   seqNum [in] dither sequence number
 * @param   upperPart [in] upper part of dither map
 * @param   lowerPart [in] lower part of dither map
 * @return  SP_OK(0)/SP_FAIL
 * @see
 */
UINT32
gpHalScaleSetDither(
	UINT32 mode,
	UINT32 seqNum,
	UINT32 upperPart,
	UINT32 lowerPart
)
{
	UINT32 regVal;

	/* check parameter */
	if (mode > SCALE_DITHER_MODE_HERRDIFF) {
		DIAG_ERROR("[scaleSetDstImg] dither mode is invalid.\n");
		return SP_ERR_PARAM;
	}

	regVal = scaleReg->scaleCtrl;
	regVal &= ~(R_SCLCTRL_ODITHER_FLG | R_SCLCTRL_ODITHSEL_FLG | R_SCLCTRL_ODITHSEQ_OFFMSK);

	/* set dither mode */
	switch (mode) {
	case SCALE_DITHER_MODE_ORDER:
		regVal |= (R_SCLCTRL_ODITHER_FLG | R_SCLCTRL_ODITHSEL_ORDER);
		break;
	case SCALE_DITHER_MODE_HERRDIFF:
		regVal |= (R_SCLCTRL_ODITHER_FLG | R_SCLCTRL_ODITHSEL_HERRDIFF);
		break;
	default:
		goto out;
	}

	/* set dither sequence */
	regVal |= (seqNum & R_SCLCTRL_ODITHSEQ_MSK) << R_SCLCTRL_ODITHSEQ_OFFSET;

	/* set dither map */
	scaleReg->scaleDitherMap0 = upperPart;
	scaleReg->scaleDitherMap1 = lowerPart;

out:
	scaleReg->scaleCtrl = regVal;
	return SP_OK;
}

/**
 * @brief   Scale hardware scaling execute
 * @param   srcImg [in] source image bitmap
 * @param   clipRgn [in] source clip region
 * @param   dstImg [in] destination image bitmap
 * @param   scaleRgn [in] destination scale region
 * @return  SP_OK(0)/SP_FAIL
 * @see
 */
UINT32
gpHalScaleExec(
	gp_bitmap_t *srcImg,
	gp_rect_t *clipRgn,
	gp_bitmap_t *dstImg,
	gp_rect_t *scaleRgn
)
{
	UINT32 ret = SP_OK;

	if (scaleReg->scaleIntSrc & R_INTSRC_SCLSTART_FLG) {
		gpHalScaleReset();
	}

	/* scaleSetSrcImg() must be called before scaleSetDstImg() */
	ret = scaleSetSrcImg(srcImg, clipRgn);
	if (ret != SP_OK) {
		DIAG_ERROR("scaleSetSrcImg fail: %d\n", ret);
		goto out;
	}
	ret = scaleSetDstImg(dstImg, scaleRgn);
	if (ret != SP_OK) {
		DIAG_ERROR("scaleSetDstImg fail: %d\n", ret);
		goto out;
	}

	scaleStart();
out:
	return ret;
}

/**
 * @brief   Scale hardware interrupt enable/disable
 * @param   enable [in] diable(0)/enable(1)
 * @return  None
 * @see
 */
void
gpHalScaleEnableIrq(
	UINT32 enable
)
{
	if (enable) {
		scaleReg->scaleIntEn = SCALE_INT_ENABLE;
	}
	else {
		scaleReg->scaleIntEn = SCALE_INT_DISABLE;
	}
}

/**
 * @brief   Scale hardware clear scaling done flag
 * @return  None
 * @see
 */
void
gpHalScaleClearDone(
	void
)
{
	scaleReg->scaleIntSrc = R_INTSRC_SCLDONE_FLG;
}

/**
 * @brief   Scale hardware check scaling finish
 * @return  not finish(0)/finished(1)
 * @see
 */
UINT32
gpHalScaleDone(
	void
)
{
	return (scaleReg->scaleIntSrc & R_INTSRC_SCLDONE_FLG) >> 1;
}

/**
 * @brief   Scale clock enable/disable function
 * @param   enable[in]: disable(0)/enable(1)
 * @return  None
 * @see
 */
void
gpHalScaleClkEnable(
	UINT32 enable
)
{
	if (enable){
		scucReg->scucPeriClkEn |= SCU_C_PERI_SCALING | SCU_C_PERI_2DSCALEABT;
	}
	else{
		scucReg->scucPeriClkEn &= ~SCU_C_PERI_SCALING; /* SCU_C_PERI_2DSCALEABT */
	}
}

/**
 * @brief   Scale hardware reset function
 */
void
gpHalScaleReset(
	void
)
{
	DIAG_WARN("!!!!!!!!!!!!!! gpHalScaleReset !!!!!!!!!!!!!!!\n");
	#if 1
	scaleReg->scaleIntSrc |= R_INTSRC_RESET_FLG;
	while (scaleReg->scaleIntSrc & R_INTSRC_RESET_FLG) {
		udelay(1);
	}
	#else
	scucReg->scucPeriRst |= SCU_C_PERI_SCALING;
	udelay(1);
	scucReg->scucPeriRst &= ~SCU_C_PERI_SCALING;
	/* need re-enable interrupt */
	gpHalScaleEnableIrq(1);
	#endif
}

/**
 * @brief   Scale hardware register dump function
 */
void
gpHalScaleRegDump(
	void
)
{
	DIAG_PRINTF("======== SCALER1 REG DUMP ========\n");
	DIAG_PRINTF("scaleFbAddr     = %08X\n", scaleReg->scaleFbAddr);
	DIAG_PRINTF("scaleCbAddr     = %08X\n", scaleReg->scaleCbAddr);
	DIAG_PRINTF("scaleCrAddr     = %08X\n", scaleReg->scaleCrAddr);
	DIAG_PRINTF("scaleWbAddr     = %08X\n", scaleReg->scaleWbAddr);
	DIAG_PRINTF("scaleImgPitch   = %08X\n", scaleReg->scaleImgPitch);
	DIAG_PRINTF("scaleImgRes     = %08X\n", scaleReg->scaleImgRes);
	DIAG_PRINTF("scaleMcbPitch   = %08X\n", scaleReg->scaleMcbPitch);
	DIAG_PRINTF("scaleCbCrPitch  = %08X\n", scaleReg->scaleCbCrPitch);
	DIAG_PRINTF("scaleOutPitch   = %08X\n", scaleReg->scaleOutPitch);
	DIAG_PRINTF("scaleOutRes     = %08X\n", scaleReg->scaleOutRes);
	DIAG_PRINTF("scaleParamH     = %08X\n", scaleReg->scaleParamH);
	DIAG_PRINTF("scaleParamV     = %08X\n", scaleReg->scaleParamV);
	DIAG_PRINTF("scaleDitherMap0 = %08X\n", scaleReg->scaleDitherMap0);
	DIAG_PRINTF("scaleDitherMap1 = %08X\n", scaleReg->scaleDitherMap1);
	DIAG_PRINTF("scaleCtrl       = %08X\n", scaleReg->scaleCtrl);
	DIAG_PRINTF("scaleIntEn      = %08X\n", scaleReg->scaleIntEn);
	DIAG_PRINTF("scaleIntSrc     = %08X\n", scaleReg->scaleIntSrc);
}

