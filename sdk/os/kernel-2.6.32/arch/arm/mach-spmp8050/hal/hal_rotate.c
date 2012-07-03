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
 * @file    hal_rotate.c
 * @brief   Implement of SPMP8050 Rotate HAL API.
 * @author  qinjian
 * @since   2010/10/14
 * @date    2010/10/14
 */
#include <mach/kernel.h>
#include <mach/hal/hal_rotate.h>
#include <mach/hal/regmap/reg_rotate.h>
#include <mach/hal/regmap/reg_scu.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define ROTATE_MODE_RGB             0
#define ROTATE_MODE_YCBYCR          1

/* Rotation options */
#define ROTATE_OPT_NONE             0
#define ROTATE_OPT_DEGREE_0         0
#define ROTATE_OPT_DEGREE_90        1
#define ROTATE_OPT_DEGREE_180       2
#define ROTATE_OPT_DEGREE_270       3
#define ROTATE_OPT_DEGREE_MASK      0x3
#define ROTATE_OPT_MIRROR           4
#define ROTATE_OPT_FLIP             8

/* Rotate control register */
#define R_ROTCTRL_YCBYCR_SHIFT      10
#define R_ROTCTRL_BPP_MASK          0x3
#define R_ROTCTRL_BPP_SHIFT         8
#define R_ROTCTRL_OPT_MASK          0xF
#define R_ROTCTRL_OPT_SHIFT         0

/* Rotate interrupt source register */
#define R_INTSRC_RESET_FLG			0x2
#define R_INTSRC_ROTDONE_FLG		0x2
#define R_INTSRC_ROTSTART_FLG		0x1

#define ROTATE_INT_DISABLE          0
#define ROTATE_INT_ENABLE           2

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

static rotateReg_t *rotateReg = (rotateReg_t *)LOGI_ADDR_ROTATE_REG;
static scucReg_t *scucReg = (scucReg_t *)LOGI_ADDR_SCU_C_REG;

/**
 * @brief   Scale hardware start scaling
 * @return  None
 * @see
 */
static void
rotateStart(
	void
)
{
	rotateReg->rotateIntSrc = R_INTSRC_ROTSTART_FLG | R_INTSRC_ROTDONE_FLG;
}

/**
 * @brief   Rotate hardware rotating execute
 * @param   srcImg [in] source image bitmap
 * @param   clipRgn [in] source clip region
 * @param   dstImg [in] destination image bitmap
 * @param   dstPos [in] destination position
 * @param   opt [in] rotate option
 * @return  SP_OK(0)/SP_FAIL
 * @see
 */
UINT32
gpHalRotateExec(
	gp_bitmap_t *srcImg,
	gp_rect_t *clipRgn,
	gp_bitmap_t *dstImg,
	gp_point_t *dstPos,
	UINT32 opt
)
{
	UINT32 bpp, mode;
	UINT32 mcuX, mcuY;
	UINT32 offX, offY, dstW, dstH;

	/* check parameters */
	if ((clipRgn->width & 0x0F) != 0 || (clipRgn->height & 0x0F) != 0) {
		DIAG_ERROR("[gpHalRotateExec] width & height must multiple by 16.\n");
		return SP_ERR_HW_LIMIT;
	}
	if (srcImg->type != dstImg->type) {
		DIAG_ERROR("[gpHalRotateExec] source image & dest image must be same color format.\n");
		return SP_ERR_NOT_SUPPORTED;
	}
	/* check regions */
	if ((clipRgn->x < 0)
		|| (clipRgn->y < 0)
		|| ((clipRgn->x + clipRgn->width) > srcImg->width)
		|| ((clipRgn->y + clipRgn->height) > srcImg->height)) {
		DIAG_ERROR("[gpHalRotateExec] clip region must in src image.\n");
		return SP_ERR_PARAM;
	}
	switch (opt & ROTATE_OPT_DEGREE_MASK) {
	case ROTATE_OPT_DEGREE_0:
	case ROTATE_OPT_DEGREE_180:
		dstW = clipRgn->width;
		dstH = clipRgn->height;
		break;
	case ROTATE_OPT_DEGREE_90:
	case ROTATE_OPT_DEGREE_270:
		dstW = clipRgn->height;
		dstH = clipRgn->width;
		break;
	}
	if ((dstPos->x < 0)
		|| (dstPos->y < 0)
		|| ((dstPos->x + dstW) > dstImg->width)
		|| ((dstPos->y + dstH) > dstImg->height)) {
		DIAG_ERROR("[gpHalRotateExec] dst region must in dst image.\n");
		return SP_ERR_PARAM;
	}

	/* check color format */
	switch (srcImg->type) {
	case SP_BITMAP_8BPP:
		bpp  = 1;
		mode = ROTATE_MODE_RGB;
		break;
	case SP_BITMAP_ARGB1555:
	case SP_BITMAP_RGAB5515:
	case SP_BITMAP_RGB565:
		bpp  = 2;
		mode = ROTATE_MODE_RGB;
		break;
	case SP_BITMAP_YUYV:
	case SP_BITMAP_YCbYCr:
		bpp  = 2;
		mode = ROTATE_MODE_YCBYCR;
		break;
	case SP_BITMAP_RGB888:
		bpp  = 3;
		mode = ROTATE_MODE_RGB;
		break;
	case SP_BITMAP_ARGB8888:
		bpp  = 4;
		mode = ROTATE_MODE_RGB;
		break;
	default:
		DIAG_ERROR("[gpHalRotateExec] unsupported color format.\n");
		return SP_ERR_FORMAT_UNSUPPORTED;
	}

	/* set macro block count */
	mcuX = clipRgn->width >> 4;
	mcuY = clipRgn->height >> 4;
	if (mcuX > 0xFFFF || mcuY > 0xFFFF) {
		DIAG_ERROR("[gpHalRotateExec] width or height is too big.\n");
		return SP_ERR_HW_LIMIT;
	}
	rotateReg->rotateImgBlock = (mcuX << 16) | mcuY;

	/* set source image address */
	rotateReg->rotateImgAddr = (UINT32)srcImg->pData + \
		clipRgn->y * srcImg->bpl + clipRgn->x * bpp;

	/* set source image pitch */
	rotateReg->rotateImgPitch = (srcImg->bpl << 16) | (bpp << 4);

	/* set destination image address */
	switch (opt & ROTATE_OPT_DEGREE_MASK) {
	case ROTATE_OPT_DEGREE_0:
		dstW = mcuX - 1;
		dstH = mcuY - 1;
		offX = 0;
		offY = 0;
		break;
	case ROTATE_OPT_DEGREE_90:
		dstW = mcuY - 1;
		dstH = mcuX - 1;
		offX = dstW;
		offY = 0;
		break;
	case ROTATE_OPT_DEGREE_180:
		dstW = mcuX - 1;
		dstH = mcuY - 1;
		offX = dstW;
		offY = dstH;
		break;
	case ROTATE_OPT_DEGREE_270:
		dstW = mcuY - 1;
		dstH = mcuX - 1;
		offX = 0;
		offY = dstH;
		break;
	}
	if ((opt & ROTATE_OPT_MIRROR) != 0) {
		offX = dstW - offX; /* horizontal mirror */
	}
	if ((opt & ROTATE_OPT_FLIP) != 0) {
		offY = dstH - offY; /* vertical flip */
	}
	rotateReg->rotateOutAddr = (UINT32)dstImg->pData + \
		(dstPos->y + (offY << 4)) * dstImg->bpl + \
		(dstPos->x + (offX << 4)) * bpp;

	/* set destination image pitch */
	rotateReg->rotateOutPitch = (dstImg->bpl << 16) | (bpp << 4);

	/* set rotate control */
	rotateReg->rotateCtrl = ((opt & R_ROTCTRL_OPT_MASK) << R_ROTCTRL_OPT_SHIFT)
						  | ((bpp - 1) << R_ROTCTRL_BPP_SHIFT)
						  |	(mode << R_ROTCTRL_YCBYCR_SHIFT);

	/* start rotating */
	rotateStart();

	return SP_OK;
}

/**
 * @brief   Rotate hardware interrupt enable/disable
 * @param   enable [in] diable(0)/enable(1)
 * @return  None
 * @see
 */
void
gpHalRotateEnableIrq(
	UINT32 enable
)
{
	if (enable) {
		rotateReg->rotateIntEn = ROTATE_INT_ENABLE;
	}
	else {
		rotateReg->rotateIntEn = ROTATE_INT_DISABLE;
	}
}

/**
 * @brief   Rotate hardware clear rotating done flag
 * @return  None
 * @see
 */
void
gpHalRotateClearDone(
	void
)
{
	rotateReg->rotateIntSrc = R_INTSRC_ROTDONE_FLG;
}

/**
 * @brief   Rotate hardware check rotating finish
 * @return  not finish(0)/finished(1)
 * @see
 */
UINT32
gpHalRotateDone(
	void
)
{
	return (rotateReg->rotateIntSrc & R_INTSRC_ROTDONE_FLG) >> 1;
}

/**
 * @brief   Rotate clock enable/disable function
 * @param   enable[in]: disable(0)/enable(1)
 * @return  None
 * @see
 */
void
gpHalRotateClkEnable(
	UINT32 enable
)
{
	if (enable){
		scucReg->scucPeriClkEn |= SCU_C_PERI_ROTATOR | SCU_C_PERI_2DSCALEABT;
	}
	else{
		scucReg->scucPeriClkEn &= ~SCU_C_PERI_ROTATOR;
	}
}
