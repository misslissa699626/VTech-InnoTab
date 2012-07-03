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
/**
 * @file    hal_2d.c
 * @brief   Implement of 2D HAL API.
 * @author  clhuang
 * @since   2010-10-07
 * @date    2010-10-07
 */

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/console.h>

#include <linux/types.h>
#include <asm/io.h>
#include <mach/diag.h>
#include <mach/common.h>
#include <mach/typedef.h>
#include <mach/hal/regmap/reg_2d.h>
#include <mach/hal/regmap/reg_scu.h>
#include <mach/hal/hal_2d.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define ATTR_2D_SRC_SCL_WIDTH_MSK			0x0FFF
#define ATTR_2D_SRC_SCL_WIDTH_OFFSET			16
#define ATTR_2D_SRC_SCL_WIDTH_OFFMSK		0x0FFF0000

#define ATTR_2D_SRC_SCL_HEIGHT_MSK			0x0FFF
#define ATTR_2D_SRC_SCL_HEIGHT_OFFSET		0
#define ATTR_2D_SRC_SCL_HEIGHT_OFFMSK		0x00000FFF

#define ATTR_2D_SRC_PITCH_MSK					0x3FFF
#define ATTR_2D_SRC_PITCH_OFFSET				0
#define ATTR_2D_SRC_PITCH_OFFMSK				0x00003FFF

#define ATTR_2D_DST_WIDTH_MSK				0x0FFF
#define ATTR_2D_DST_WIDTH_OFFSET				16
#define ATTR_2D_DST_WIDTH_OFFMSK				0x0FFF0000

#define ATTR_2D_DST_PITCH_MSK					0x3FFF
#define ATTR_2D_DST_PITCH_OFFSET				0
#define ATTR_2D_DST_PITCH_OFFMSK				0x00003FFF

#define ATTR_2D_DST_HEIGHT_MSK				0x0FFF
#define ATTR_2D_DST_HEIGHT_OFFSET			0
#define ATTR_2D_DST_HEIGHT_OFFMSK			0x00000FFF

#define ATTR_2D_RECT_WIDTH_MSK				0x0FFF
#define ATTR_2D_RECT_WIDTH_OFFSET			16
#define ATTR_2D_RECT_WIDTH_OFFMSK			0x0FFF0000

#define ATTR_2D_RECT_HEIGHT_MSK				0x0FFF
#define ATTR_2D_RECT_HEIGHT_OFFSET			0
#define ATTR_2D_RECT_HEIGHT_OFFMSK			0x00000FFF

#define ATTR_2D_STARTX_MSK					0x0FFF
#define ATTR_2D_STARTX_OFFSET					16
#define ATTR_2D_STARTX_OFFMSK					0x0FFF0000

#define ATTR_2D_STARTY_MSK					0x0FFF
#define ATTR_2D_STARTY_OFFSET					0
#define ATTR_2D_STARTY_OFFMSK					0x00000FFF

#define ATTR_2D_MSK_PITCH_MSK					0x1FF
#define ATTR_2D_MSK_PITCH_OFFSET				16
#define ATTR_2D_MSK_PITCH_OFFMSK				0x01FF0000

#define ATTR_2D_MSK_HEIGHT_MSK				0x0FFF
#define ATTR_2D_MSK_HEIGHT_OFFSET			0
#define ATTR_2D_MSK_HEIGHT_OFFMSK			0x00000FFF

#define ATTR_2D_PALT_CFMT_MSK					0x00000007
#define ATTR_2D_PALT_CFMT_OFFSET				0

#define ATTR_2D_PALT_OFFSET_MSK				0xF
#define ATTR_2D_PALT_OFFSET_OFFSET			8

#define ATTR_2D_PALT_LENGTH_MSK				0x03FF
#define ATTR_2D_PALT_LENGTH_OFFSET			16

#define ATTR_2D_SRC_REF_X_MSK					0x0FFF
#define ATTR_2D_SRC_REF_X_OFFSET				16
#define ATTR_2D_SRC_REF_X_OFFMSK				0x0FFF0000

#define ATTR_2D_SRC_REF_Y_MSK					0x0FFF
#define ATTR_2D_SRC_REF_Y_OFFSET				0
#define ATTR_2D_SRC_REF_Y_OFFMSK				0x00000FFF

#define ATTR_2D_DST_REF_X_MSK					0x0FFF
#define ATTR_2D_DST_REF_X_OFFSET				16
#define ATTR_2D_DST_REF_X_OFFMSK				0x0FFF0000

#define ATTR_2D_DST_REF_Y_MSK					0x0FFF
#define ATTR_2D_DST_REF_Y_OFFSET				0
#define ATTR_2D_DST_REF_Y_OFFMSK				0x00000FFF

#define ATTR_2D_TROP_MSK					0x0000000F

#define ATTR_2D_COLORKEY_MSK				0x00FFFFFF

#define ATTR_2D_CLIP_TOP_MSK				0x0FFF
#define ATTR_2D_CLIP_TOP_OFFSET			16
#define ATTR_2D_CLIP_TOP_OFFMSK			0x0FFF0000

#define ATTR_2D_CLIP_BTM_MSK				0x0FFF
#define ATTR_2D_CLIP_BTM_OFFSET			0
#define ATTR_2D_CLIP_BTM_OFFMSK			0x00000FFF

#define ATTR_2D_CLIP_LEFT_MSK				0x0FFF
#define ATTR_2D_CLIP_LEFT_OFFSET			16
#define ATTR_2D_CLIP_LEFT_OFFMSK			0x0FFF0000

#define ATTR_2D_CLIP_RIGHT_MSK			0x0FFF
#define ATTR_2D_CLIP_RIGHT_OFFSET			0
#define ATTR_2D_CLIP_RIGHT_OFFMSK			0x00000FFF

#define ATTR_2D_BPRDMASK_MSK				0x00FFFFFF
#define ATTR_2D_BPWRMASK_MSK				0x00FFFFFF

#define ATTR_2D_HDELTA_COLOR_MSK				0xFFFF
#define ATTR_2D_HDELTA_COLOR_OFFSET			16
#define ATTR_2D_HDELTA_COLOR_OFFMSK			0xFFFF0000

#define ATTR_2D_VDELTA_COLOR_MSK				0xFFFF
#define ATTR_2D_VDELTA_COLOR_OFFSET			0
#define ATTR_2D_VDELTA_COLOR_OFFMSK			0x0000FFFF

#define ATTR_2D_SCALE_INIT_MSK			0xFFFF
#define ATTR_2D_SCALE_INIT_OFFSET			16
#define ATTR_2D_SCALE_INIT_OFFMSK			0xFFFF0000

#define ATTR_2D_SCALE_FACT_MSK			0xFFFF
#define ATTR_2D_SCALE_FACT_OFFSET		0
#define ATTR_2D_SCALE_FACT_OFFMSK		0x0000FFFF

#define ATTR_2D_ROP_MSK					0xFF
#define ATTR_2D_ROP_OFFSET				24
#define ATTR_2D_ROP_OFFMSK				0xFF000000

#define ATTR_2D_ROP_PATTERN_MSK			0x00FFFFFF
#define ATTR_2D_ROP_PATTERN_OFFSET		0

#define ATTR_2D_CONST_ALPHA_DST_MSK			0x01FF
#define ATTR_2D_CONST_ALPHA_DST_OFFSET		16
#define ATTR_2D_CONST_ALPHA_DST_OFFMSK		0x01FF0000

#define ATTR_2D_CONST_ALPHA_SRC_MSK			0x01FF
#define ATTR_2D_CONST_ALPHA_SRC_OFFSET		0
#define ATTR_2D_CONST_ALPHA_SRC_OFFMSK		0x000001FF

#define ATTR_2D_CFMT_DST_MSK				0xF
#define ATTR_2D_CFMT_DST_OFFSET			8
#define ATTR_2D_CFMT_DST_OFFMSK			0x00000F00

#define ATTR_2D_CFMT_SRC_MSK				0x1F
#define ATTR_2D_CFMT_SRC_OFFSET			0
#define ATTR_2D_CFMT_SRC_OFFMSK			0x0000001F

#define ATTR_2D_ALPHA_FMT_MSK			0x3
#define ATTR_2D_ALPHA_FMT_OFFSET			8
#define ATTR_2D_ALPHA_FMT_OFFMSK			0x0300

#define ATTR_2D_BLEND_OP_MSK				0x7
#define ATTR_2D_BLEND_OP_OFFSET			0
#define ATTR_2D_BLEND_OP_OFFMSK			0x0007

#define ATTR_2D_DMA_PRIO_MSK				0x0000000F

#define ATTR_2D_EXEC_EN					0x00000001

#define ATTR_2D_RESET_EN					0x00000001

#define ATTR_2D_STATUS_FIN					0x00000001
#define ATTR_2D_STATUS_NOP				0x00000002
#define ATTR_2D_STATUS_S_RST_FIN			0x00000004
#define ATTR_2D_STATUS_AHB_ERR			0x00000008 /* useless */

#define ATTR_2D_INT_EN_MSK				0x0000000F
#define ATTR_2D_INT_FIN_EN					0x00000001
#define ATTR_2D_INT_NOP_EN				0x00000002
#define ATTR_2D_INT_S_RST_FIN_EN			0x00000004
#define ATTR_2D_INT_AHB_ERR_EN			0x00000008 /* useless */

#define G2DENG_CTRL_YUV_MODE_FLG			0x10000000
#define G2DENG_CTRL_YUV_MODE_AVG			0x00000000
#define G2DENG_CTRL_YUV_MODE_EVEN		0x10000000

#define G2DENG_CTRL_VERT_SCALE_EN		0x08000000
#define G2DENG_CTRL_HOR_SCALE_EN			0x04000000

#define G2DENG_CTRL_VERT_SHAD_FLG		0x02000000
#define G2DENG_CTRL_VERT_SHAD_TB			0x00000000
#define G2DENG_CTRL_VERT_SHAD_BT			0x02000000

#define G2DENG_CTRL_HOR_SHAD_FLG			0x01000000
#define G2DENG_CTRL_HOR_SHAD_LR			0x00000000
#define G2DENG_CTRL_HOR_SHAD_RL			0x01000000

#define G2DENG_CTRL_VERT_SHAD_EN			0x00800000
#define G2DENG_CTRL_HOR_SHAD_EN			0x00400000

#define G2DENG_CTRL_MSKFIFO_FLG			0x00200000
#define G2DENG_CTRL_MSKFIFO_REG			0x00000000
#define G2DENG_CTRL_MSKFIFO_MEM			0x00200000

#define G2DENG_CTRL_STIPPLECONT_EN		0x00100000

#define G2DENG_CTRL_STIPPLETYPE_FLG		0x00080000
#define G2DENG_CTRL_STIPPLETYPE_ORI		0x00000000
#define G2DENG_CTRL_STIPPLETYPE_OFFSET	0x00080000

#define G2DENG_CTRL_DITHER_EN				0x00040000

#define G2DENG_CTRL_DSTCONSTALPHAFILL	0x00020000

#define G2DENG_CTRL_BP_RD_MSK_EN			0x00010000
#define G2DENG_CTRL_BP_WR_MSK_EN			0x00008000

#define G2DENG_CTRL_CLIP_MSK            0x00006000
#define G2DENG_CTRL_SCR_CLIP_EN			0x00004000
#define G2DENG_CTRL_RECT_CLIP_EN			0x00002000

#define G2DENG_CTRL_MIRROR_ROT_EN		0x00001000

#define G2DENG_CTRL_MIRROR_ON_EN			0x00000800

#define G2DENG_CTRL_ROT_OFFSET          0x9
#define G2DENG_CTRL_ROT_MSK				0x00000600
#define G2DENG_CTRL_ROT_DEG_0				0x00000000
#define G2DENG_CTRL_ROT_DEG_90			0x00000200
#define G2DENG_CTRL_ROT_DEG_180			0x00000400
#define G2DENG_CTRL_ROT_DEG_270			0x00000600

#define G2DENG_CTRL_TRANSBLT_EN			0x00000100

#define G2DENG_CTRL_FULLYROP_EN		0x00000080

#define G2DENG_CTRL_ALPHABLEND_EN		0x00000040

#define G2DENG_CTRL_GRADFILL_EN			0x00000008

#define G2DENG_CTRL_BLTSEL_MSK			0x00000006
#define G2DENG_CTRL_BLTSEL_NOMSK			0x00000000
#define G2DENG_CTRL_BLTSEL_STIPPLEMSK	0x00000004
#define G2DENG_CTRL_BLTSEL_MSKBLT			0x00000006

#define G2DENG_CTRL_PALT_LOAD_EN			0x00000001

/* 2D engine status bits. */
#define G2DENG_STATUS_FIN					0x00000001
#define G2DENG_STATUS_NOP					0x00000002
#define G2DENG_STATUS_S_RST_FIN			0x00000004
#define G2DENG_STATUS_AHB_ERR			0x00000008 /* useless */

/* 2D engine interrupt enable bits. */
#define G2DENG_INT_ALL_EN					0x0000000F
#define G2DENG_INT_FIN_EN					0x00000001
#define G2DENG_INT_NOP_EN					0x00000002
#define G2DENG_INT_S_RST_FIN_EN			0x00000004

/* Color format index for source and destination buffer. */
#define G2DENG_SRC_CFMT_MSK			0x1F
#define G2DENG_SRC_CFMT_OFFSET		0x0
#define G2DENG_SRC_CFMT_OFFMSK		0x0000001F
#define G2DENG_SRC_CFMT_MAX         0x14

#define G2DENG_DST_CFMT_MSK			0xF
#define G2DENG_DST_CFMT_OFFSET		5
#define G2DENG_DST_CFMT_OFFMSK		0x000001E0
#define G2DENG_DST_CFMT_MAX         0x10

/* Color format for palette table. */
#define G2DENG_PALT_CFMT_MSK			0x7
#define G2DENG_PALT_CFMT_OFFSET		9
#define G2DENG_PALT_CFMT_OFFMSK		0x00000E00
#define G2DENG_PALT_CFMT_MAX         0x10

#define G2DENG_CFMT_RGB565		0x00000000
#define G2DENG_CFMT_RGAB5515		0x00000001
#define G2DENG_CFMT_ARGB1555		0x00000002
#define G2DENG_CFMT_ARGB4444		0x00000003
#define G2DENG_CFMT_RGB888		0x00000004
#define G2DENG_CFMT_ARGB8888		0x00000005
#define G2DENG_CFMT_BGR565		0x00000006
#define G2DENG_CFMT_BGAR5515		0x00000007
#define G2DENG_CFMT_ABGR1555		0x00000008
#define G2DENG_CFMT_ABGR4444		0x00000009
#define G2DENG_CFMT_BGR888		0x0000000a
#define G2DENG_CFMT_ABGR8888		0x0000000b
#define G2DENG_CFMT_YCbCr			0x0000000c
#define G2DENG_CFMT_YUV			0x0000000d
#define G2DENG_CFMT_YCbYCr		0x0000000e
#define G2DENG_CFMT_YUYV			0x0000000f
#define G2DENG_CFMT_256_COLOR		0x00000012
#define G2DENG_CFMT_16_COLOR		0x00000013

/* Color format for palette table. */
#define G2DENG_PALT_CFMT_RGB565		0x00000000
#define G2DENG_PALT_CFMT_ARGB1555	0x00000001
#define G2DENG_PALT_CFMT_ARGB4444	0x00000002
#define G2DENG_PALT_CFMT_BGR565		0x00000003
#define G2DENG_PALT_CFMT_ABGR1555	0x00000004
#define G2DENG_PALT_CFMT_ABGR4444	0x00000005
#define G2DENG_PALT_CFMT_RGB888		0x00000006
#define G2DENG_PALT_CFMT_BGR888		0x00000007

#define G2DENG_RESET_EN 0x01
#define G2DENG_REG_END  0xac

#define DISP_TYPE_LINE_BUFFER_MODE_MSK  0x0000000F
#define LINEBUF_MODE_DDGG   0x2

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define ceil(x,y) (((x)/(y)) + (((x)%(y) == 0)  ? 0 : 1))
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct g2dHalCtx_s {
    UINT32 ctrlReg;
    UINT32 dstX;
    UINT32 dstY;
    UINT32 dstWidth;
    UINT32 dstHeight;
    UINT32 ropEnable;
    UINT32 srcClrFmt;
}g2dHalCtx_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
/**
* @brief Set line buffer mode
* @return : SP_OK(0)/SP_FAIL
*/
UINT32 gpHalLinebufferSetMode(UINT32 mode);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static g2dReg_t *g2dReg = (g2dReg_t *)G2D_BASE;
static g2dHalCtx_t g2dHalCtxVar;
static g2dHalCtx_t* g2dHalCtx = &g2dHalCtxVar;
static scuaReg_t* gp_scuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;
/*static scubReg_t* gp_scubReg = (scubReg_t *)LOGI_ADDR_SCU_B_REG;*/
static scucReg_t* gp_scucReg = (scucReg_t *)LOGI_ADDR_SCU_C_REG;

static
UINT32
gpHalConvertColorFmtToHal(
    UINT8 colorFmt
)
{
    UINT8 halColorFmt = G2DENG_SRC_CFMT_MSK;

    switch (colorFmt) {
    case SP_BITMAP_RGB565:
        halColorFmt = G2DENG_CFMT_RGB565;
        break;
    case SP_BITMAP_RGAB5515:
        halColorFmt = G2DENG_CFMT_RGAB5515;
        break;
    case SP_BITMAP_ARGB1555:
        halColorFmt = G2DENG_CFMT_ARGB1555;
        break;
    case SP_BITMAP_ARGB4444:
        halColorFmt = G2DENG_CFMT_ARGB4444;
        break;
    case SP_BITMAP_RGB888:
        halColorFmt = G2DENG_CFMT_RGB888;
        break;
    case SP_BITMAP_ARGB8888:
        halColorFmt = G2DENG_CFMT_ARGB8888;
        break;
    case SP_BITMAP_BGR565:
        halColorFmt = G2DENG_CFMT_BGR565;
        break;
    /*case SP_BITMAP_RGB555:
        !!!not support;
        break;*/
    case SP_BITMAP_BGAR5515:
        halColorFmt = G2DENG_CFMT_BGAR5515;
        break;
    case SP_BITMAP_ABGR1555:
        halColorFmt = G2DENG_CFMT_ABGR1555;
        break;
    case SP_BITMAP_ABGR4444:
        halColorFmt = G2DENG_CFMT_ABGR4444;
        break;
    case SP_BITMAP_BGR888:
        halColorFmt = G2DENG_CFMT_BGR888;
        break;
    case SP_BITMAP_ABGR8888:
        halColorFmt = G2DENG_CFMT_ABGR8888;
        break;
    /*case SP_BITMAP_1BPP:
        !!!not support;
        break;
    case SP_BITMAP_2BPP:
        !!!not support;
        break;*/
    case SP_BITMAP_4BPP:
        halColorFmt = G2DENG_CFMT_16_COLOR;
        break;
    case SP_BITMAP_8BPP:
        halColorFmt = G2DENG_CFMT_256_COLOR;
        break;
    case SP_BITMAP_YCbCr:
        halColorFmt = G2DENG_CFMT_YCbCr;
        break;
    case SP_BITMAP_YUV:
        halColorFmt = G2DENG_CFMT_YUV;
        break;
    case SP_BITMAP_YCbYCr:
        halColorFmt = G2DENG_CFMT_YCbYCr;
        break;
    case SP_BITMAP_YUYV:
        halColorFmt = G2DENG_CFMT_YUYV;
        break;
    /*case SP_BITMAP_4Y4U4Y4V:
        !!!not support;
        break;
    case SP_BITMAP_4Y4Cb4Y4Cr:
        !!!not support;
        break;
    case SP_BITMAP_YCbCr400:
         !!!not support;
        break;*/
    default:
        halColorFmt = G2DENG_SRC_CFMT_MSK;
        break;
    }

    return halColorFmt;
}

static
UINT32
gpHalCalPitchByFormat(
    UINT32 format,
    UINT32 width
)
{
	UINT32 pitch;

	switch (format) {
	case G2DENG_CFMT_RGB888:
	case G2DENG_CFMT_BGR888:
	case G2DENG_CFMT_YCbCr:
	case G2DENG_CFMT_YUV:
		pitch = width * 3;
		break;
	case G2DENG_CFMT_ARGB8888:
	case G2DENG_CFMT_ABGR8888:
		pitch = width * 4;
		break;
	case G2DENG_CFMT_256_COLOR:
		pitch = width;
		break;
	case G2DENG_CFMT_16_COLOR:
		pitch = width >> 1;
		break;
	/*
	case G2DENG_CFMT_RGB565 :
	case G2DENG_CFMT_RGAB5515	:
	case G2DENG_CFMT_ARGB1555	:
	case G2DENG_CFMT_ARGB4444	:
	case G2DENG_CFMT_BGR565 :
	case G2DENG_CFMT_BGAR5515	:
	case G2DENG_CFMT_ABGR1555	:
	case G2DENG_CFMT_ABGR4444	:
	case G2DENG_CFMT_YCbYCr :
	case G2DENG_CFMT_YUYV :
	*/
	default:
		pitch = width << 1;
		break;
	}

	return pitch;
}

static
UINT32
gpHalConvNegToUnsigned(
    SINT32 negValue,
    UINT32 bitMask
)
{
	UINT32 result;

	if (negValue < 0) {
		result = -negValue;

		result ^= bitMask;

		result &= bitMask;

		result++;
	}
	else {
		result = negValue;
	}

	return result;

}

/**
* @brief dump 2D graphic engine register value
* @return : none
*/
void
gpHal2dDump (
)
{
    UINT32 iter;

    DIAG_INFO("\ndump 2d registers");
	for (iter = 0; iter < G2DENG_REG_END; iter += 4) {
        if(iter % 16 == 0) {
            DIAG_INFO("\n");
        }
        DIAG_INFO("%08x ", *(UINT32*)(((UINT8*)g2dReg) + iter));
	}

}

/**
* @brief initial 2D graphic engine.
* @param resetEngine [in] : 1, reset 2D engine; 0, not reset 2D engine
* @return : none
*/
void
gpHal2dInit (
    UINT8 resetEngine
)
{
    UINT32 iter;

    /*structure g2dHalCtx_t must 4bytes align*/
    for (iter = 0; iter < sizeof(g2dHalCtx_t) / sizeof(UINT32); iter++) {
        ((UINT32*)g2dHalCtx)[iter] = 0;
    }
	for (iter = 0; iter < G2DENG_REG_END; iter += 4)
	{
		*(UINT32*)(((UINT8*)g2dReg)+iter) = 0;
	}
   
    if (resetEngine != 0) {
        g2dReg->reset = ATTR_2D_RESET_EN;
    }

}

/**
* @brief start 2D graphic engine.
* @return : none
*/
void
gpHal2dExec(
    void
)
{
    g2dReg->ctrlReg = g2dHalCtx->ctrlReg;
    g2dReg->run = ATTR_2D_EXEC_EN;
}

/**
* @brief enable 2D engine interrupt.
* @param enable [in] : 1, enable interrupt; 0, disable interrupt
* @return : none
*/
void
gpHal2dEnableInterrupt (
    UINT8 enable
)
{
    if (enable != 0) {
        g2dReg->intEn = 0xff;
    }
    else {
        g2dReg->intEn = 0x0;
    }
}

/**
* @brief clear 2D engine interrupt.
* @return : none
*/
void
gpHal2dClearInterrupt (
    void
)
{
    g2dReg->statusReg = 0x0;
}

/**
* @brief Set 2D source bitmap, operation rectangle may equal to destination, if not, scale enable.
* @param imgAddr [in] : Pointer of source bitmap data
* @param imgSize [in] : Size of source bitmap
* @param colorFmt [in] : Image color format of source bitmap
* @return : SP_OK(0)/SP_FAIL
*/
UINT32
gpHal2dSetSrcBitmap (
    UINT32 imgAddr,
    spRectSize_t imgSize,
    UINT8 colorFmt,
    spRect_t opRect
)
{
    UINT32 uStartX;
    UINT32 uStartY;
    UINT16 pitch;
    UINT8 halColorFmt;

    halColorFmt = gpHalConvertColorFmtToHal(colorFmt);
    if (halColorFmt >= G2DENG_SRC_CFMT_MAX) {
        return SP_ERR_FORMAT_UNSUPPORTED;
    }
    pitch = gpHalCalPitchByFormat(halColorFmt, imgSize.width);
    uStartX = gpHalConvNegToUnsigned(opRect.x, ATTR_2D_STARTX_MSK);
    uStartY = gpHalConvNegToUnsigned(opRect.y, ATTR_2D_STARTY_MSK);

    g2dHalCtx->srcClrFmt = halColorFmt;
    g2dReg->srcBaseAdr = imgAddr;
    g2dReg->srcPitch = pitch;
    g2dReg->srcStartYX = (uStartX & ATTR_2D_STARTX_MSK) << ATTR_2D_STARTX_OFFSET
        | (uStartY & ATTR_2D_STARTY_MSK) << ATTR_2D_STARTY_OFFSET;
    g2dReg->srcDstColorFormat &= ~(ATTR_2D_CFMT_SRC_OFFMSK);
    g2dReg->srcDstColorFormat |= (halColorFmt & ATTR_2D_CFMT_SRC_MSK) << ATTR_2D_CFMT_SRC_OFFSET;

    return SP_OK;

}

/**
* @brief Set 2D source bitmap palette when color format is 16-color or 256-color.
* @param palAddr [in] : Address of palette data
* @param palOffset [in] : Offset by bytes of palette data
* @param colorFmt [in] : Palette data format
* @param loadEnable [in] : Enable load palette into 2D engine
* @return : SP_OK(0)/SP_FAIL
*/
UINT32
gpHal2dSetPalette (
    UINT32 palAddr,
    UINT32 palOffset,
    UINT8 colorFmt,
    UINT8 loadEnable
)
{
    UINT16 palLen;
    UINT16 halColorFmt;

    switch (colorFmt) {
    case SP_BITMAP_RGB565:
        halColorFmt = G2DENG_PALT_CFMT_RGB565;
        break;
    case SP_BITMAP_ARGB1555:
        halColorFmt = G2DENG_PALT_CFMT_ARGB1555;
        break;
    case SP_BITMAP_ARGB4444:
        halColorFmt = G2DENG_PALT_CFMT_ARGB4444;
        break;
    case SP_BITMAP_BGR565:
        halColorFmt = G2DENG_PALT_CFMT_BGR565;
        break;
    case SP_BITMAP_ABGR1555:
        halColorFmt = G2DENG_PALT_CFMT_ABGR1555;
        break;
    case SP_BITMAP_ABGR4444:
        halColorFmt = G2DENG_PALT_CFMT_ABGR4444;
        break;
    case SP_BITMAP_RGB888:
        halColorFmt = G2DENG_PALT_CFMT_RGB888;
        break;
    case SP_BITMAP_BGR888:
        halColorFmt = G2DENG_PALT_CFMT_BGR888;
        break;
    default:
        halColorFmt = G2DENG_PALT_CFMT_MAX;
        return SP_ERR_FORMAT_UNSUPPORTED;
    }

	switch (halColorFmt)	{
	case G2DENG_PALT_CFMT_RGB888 :
	case G2DENG_PALT_CFMT_BGR888 :
		palLen = 3;
		break;
	default: /* For all other format. */
		palLen = 2;
		break;
	}

	/* Check source  color format. */
	switch (g2dHalCtx->srcClrFmt) {
	case G2DENG_CFMT_16_COLOR:
		palLen <<= 4;
		palOffset &= 0x70;
		palOffset >>= 4;
		break;
	case G2DENG_CFMT_256_COLOR:
		palLen <<= 8;
		palOffset = 0;
		break;
    default:
        return SP_ERR_NOT_SUPPORTED;
	}

    g2dReg->srcBaseAdru = palAddr;
    g2dReg->paltFormatOffsetLenth = ((palLen & ATTR_2D_PALT_LENGTH_MSK) << ATTR_2D_PALT_LENGTH_OFFSET)
        | ((palOffset & ATTR_2D_PALT_OFFSET_MSK) << ATTR_2D_PALT_OFFSET_OFFSET)
        | (halColorFmt & ATTR_2D_PALT_CFMT_MSK);

    return SP_OK;
}

/**
* @brief Set 2D destination bitmap.
* @param imgAddr [in] : Pointer of destination bitmap data
* @param imgSize [in] : Size of destination bitmap
* @param colorFmt [in] : Image color format of destination bitmap
* @param opRect [in] : operation rectangle of destination bitmap
* @return : SP_OK(0)/SP_FAIL
*/
UINT32
gpHal2dSetDstBitmap (
    UINT32 imgAddr,
    spRectSize_t *pimgSize,
    UINT8 colorFmt,
    spRect_t *popRect
)
{
    UINT32 uStartX;
    UINT32 uStartY;
    UINT16 pitch;
    UINT8 halColorFmt;

	if ((NULL == pimgSize) || (NULL == popRect)) {
		return SP_FAIL;
	}
	
    halColorFmt = gpHalConvertColorFmtToHal(colorFmt);
    if (halColorFmt >= G2DENG_DST_CFMT_MAX) {
        return SP_ERR_FORMAT_UNSUPPORTED;
    }
    pitch = gpHalCalPitchByFormat(halColorFmt, pimgSize->width);
    uStartX = gpHalConvNegToUnsigned(popRect->x, ATTR_2D_STARTX_MSK);
    uStartY = gpHalConvNegToUnsigned(popRect->y, ATTR_2D_STARTY_MSK);
    g2dHalCtx->dstWidth = pimgSize->width;
    g2dHalCtx->dstHeight = pimgSize->height;
    g2dHalCtx->dstX = popRect->x;
    g2dHalCtx->dstY= popRect->y;
    /*g2dHalCtx->ctrlReg |= G2DENG_CTRL_YUV_MODE_EVEN;*/

    g2dReg->dstBaseAdr = imgAddr;
    g2dReg->dstPitchWidth = pimgSize->width << 16 | pitch;
    g2dReg->dstHeight = pimgSize->height;
    g2dReg->dstStartYX = (uStartX & ATTR_2D_STARTX_MSK) << ATTR_2D_STARTX_OFFSET
        | (uStartY & ATTR_2D_STARTY_MSK) << ATTR_2D_STARTY_OFFSET;
    g2dReg->rectHeightWidth = (popRect->width << ATTR_2D_RECT_WIDTH_OFFSET) | popRect->height;
    g2dReg->srcDstColorFormat &= ~(ATTR_2D_CFMT_DST_OFFMSK);
    g2dReg->srcDstColorFormat |= (halColorFmt & ATTR_2D_CFMT_DST_MSK) << ATTR_2D_CFMT_DST_OFFSET;
  
    return SP_OK;
}

/**
* @brief set 2D internal 8x8 stipple mask register
* @param stippleMskLo [in] : Low part of stipple register
* @param stippleMskHi [in] : High part of stipple register
*  @return : SP_OK(0)/SP_FAIL
*/
UINT32
gpHal2dSetStippleMsk (
    UINT32 stippleMskLo,
    UINT32 stippleMskHi
)
{
    g2dReg->stippleMsk0 = stippleMskLo;
    g2dReg->stippleMsk1 = stippleMskHi;

    return SP_OK;
}


/**
* @brief enable stipple mask or mask blt function, if fifo mode disable and size 8*8, set internal mask register
* @param enableType [in] : enable mask type
*                        (G2DENG_MSK_DISABLE/ G2DENG_MSK_ENABLE_STIPPLE/ G2DENG_MSK_ENABLE_MSKBLT)
* @param fifoMode [in] : 1, fifo mode, use external mask memory data; 0, user internal mask register
* @param bgOp [in] : Background raster operator
* @param bgPattern [in] : Background color pattern
* @param imgAddr [in] : Address of mask bitmap data
* @param imgSize [in] : Size of mask bitmap
*  @return : SP_OK(0)/SP_FAIL
*/
UINT32
gpHal2dEnableMsk (
    UINT8 enableType,
    UINT8 fifoMode,
    UINT8 bgRop,
    UINT32 bgPattern,
    UINT32 imgAddr,
    spRectSize_t *pimgSize
)
{
	if (NULL == pimgSize) {
		return SP_FAIL;
	}
	
    g2dHalCtx->ctrlReg &= ~(G2DENG_CTRL_MSKFIFO_FLG | G2DENG_CTRL_STIPPLETYPE_FLG
        | G2DENG_CTRL_STIPPLECONT_EN|G2DENG_CTRL_BLTSEL_MSK);

    switch ( enableType) {
    case G2DENG_MSK_ENABLE_MSKBLT:
        g2dReg->bgPatternRop = ((bgRop & ATTR_2D_ROP_MSK)<<ATTR_2D_ROP_OFFSET)
			| (bgPattern&ATTR_2D_ROP_PATTERN_MSK);
		break;
	case G2DENG_MSK_ENABLE_STIPPLE:
		g2dReg->bgPatternRop = ((bgRop & ATTR_2D_ROP_MSK)<<ATTR_2D_ROP_OFFSET)
			| (bgPattern&ATTR_2D_ROP_PATTERN_MSK);
        g2dHalCtx->ctrlReg |= enableType;
        break;
    case G2DENG_MSK_DISABLE:
        return SP_OK;
    default:
        return SP_ERR;
    }

    if ( fifoMode != 0) {
        g2dHalCtx->ctrlReg |= G2DENG_CTRL_MSKFIFO_FLG;
        g2dReg->maskBaseAdr = imgAddr;
        g2dReg->maskHeightPitch = ((pimgSize->width >> 3) & ATTR_2D_MSK_PITCH_MSK) << ATTR_2D_MSK_PITCH_OFFSET
            | (pimgSize->height & ATTR_2D_MSK_HEIGHT_MSK);
    }

    return SP_OK;
}

/**
* @brief enable 2D ROP function.
* @param fgOp [in] : Foreground raster operator
* @param fgPattern [in] : Foreground color pattern
* @return : SP_OK(0)/SP_FAIL
*/
UINT32
gpHal2dEnableRop(
    UINT8 fgRop,
    UINT32 fgPattern
)
{
    g2dHalCtx->ropEnable = TRUE;
    g2dHalCtx->ctrlReg &= ~(G2DENG_CTRL_ALPHABLEND_EN);
    /*disable full rop for alpha part
 	g2dHalCtx->ctrlReg |= G2DENG_CTRL_FULLYROP_EN;*/
    g2dReg->fgPatternRop =
        ((fgRop & ATTR_2D_ROP_MSK) << ATTR_2D_ROP_OFFSET) | (fgPattern&ATTR_2D_ROP_PATTERN_MSK);

    return SP_OK;
}

/**
* @brief enable bit plane.
* @param bpRdEnable [in] : Enable read bit plane
* @param bpRdMsk [in] : Read bit plane mask
* @param bpWrEnable [in] : Enable write bit plane
* @param bpWrMsk [in] : Write bit plane mask
* @return : SP_OK(0)/SP_FAIL
*/
UINT32
gpHal2dEnableBitPlane(
    UINT8 bpRdEnable,
    UINT32 bpRdMsk,
    UINT8 bpWrEnable,
    UINT32 bpWrMsk
)
{
    if ( bpRdEnable != 0 ) {
        g2dHalCtx->ctrlReg |= G2DENG_CTRL_BP_RD_MSK_EN;
    }
    else {
        g2dHalCtx->ctrlReg &= ~(G2DENG_CTRL_BP_RD_MSK_EN);
    }
    if ( bpWrEnable != 0 ) {
        g2dHalCtx->ctrlReg |= G2DENG_CTRL_BP_WR_MSK_EN;
    }
    else {
        g2dHalCtx->ctrlReg &= ~(G2DENG_CTRL_BP_WR_MSK_EN);
    }
    g2dReg->bprdMask = bpRdMsk;
    g2dReg->bpwrMask = bpWrMsk;

    return SP_OK;
}

/**
* @brief enable 2D alpha blend function.
* @param alphaFmt [in] : Alpha format, per-pixel-alpha/srcconstalpha/dstconstalpha
* @param blendOp [in] : Blend operation
* @param srcAlpha [in] : Srcconstalpha
* @param dstAlpha [in] : Dstconstalpha
* @return : SP_OK(0)/SP_FAIL
*/
UINT32
gpHal2dEnableBlend(
    UINT8 alphaFmt,
    UINT8 blendOp,
    UINT16 srcAlpha,
    UINT16 dstAlpha
)
{
    g2dHalCtx->ropEnable = FALSE;
    g2dHalCtx->ctrlReg |= G2DENG_CTRL_ALPHABLEND_EN;
    g2dReg->blendOpAlphaFormat = (alphaFmt & ATTR_2D_ALPHA_FMT_MSK) << ATTR_2D_ALPHA_FMT_OFFSET
        | (blendOp & ATTR_2D_BLEND_OP_MSK);
    g2dReg->srcDstConstAlpha = (dstAlpha & ATTR_2D_CONST_ALPHA_DST_MSK) << ATTR_2D_CONST_ALPHA_DST_OFFSET
    | (srcAlpha & ATTR_2D_CONST_ALPHA_SRC_MSK) << ATTR_2D_CONST_ALPHA_SRC_OFFSET;

    return SP_OK;
}

/**
* @brief enable 2D Trop function.
* @param srcHiColorKey [in] : Source color key range high
* @param srcLoColorKey [in] : Source color key range low
* @param dstHiColorKey [in] : Destination color key range high
* @param dstLoColorKey [in] : Destination color key range low
* @param trop [in] : Trop operation code
* @return : SP_OK(0)/SP_FAIL
*/
UINT32
gpHal2dEnableTrop(
    UINT32 srcHiColorKey,
    UINT32 srcLoColorKey,
    UINT32 dstHiColorKey,
    UINT32 dstLoColorKey,
    UINT8 trop
)
{
    g2dHalCtx->ctrlReg |= G2DENG_CTRL_TRANSBLT_EN;
    g2dReg->srcHiColorKey = srcHiColorKey;
    g2dReg->srcLoColorKey = srcLoColorKey;
    g2dReg->dstHiColorKey = dstHiColorKey;
    g2dReg->dstLoColorKey = dstLoColorKey;
    g2dReg->trop = trop & ATTR_2D_TROP_MSK;

    return SP_OK;
}

/**
* @brief Set scale parameter and enable scale, the function will compute horizontal/vertical scale factor and
*       correspond horizontal/vertical scale enable by the src/dst width/height and set relative register.
* @param srcWidth [in] : Source scale width
* @param srcHeight [in] : Source scale height
* @param dstWidth [in] : Destination width
* @param dstHeight [in] : Destination height
* @return : SP_OK(0)/SP_FAIL
*/
UINT32
gpHal2dEnableScale(
    UINT32 srcWidth,
    UINT32 srcHeight,
    UINT32 dstWidth,
    UINT32 dstHeight
)
{
    UINT32 scaleValue = 0x0;

	/* if src size and dst size are same size,need not to scale*/
	if ((srcWidth == dstWidth) && (srcHeight == dstHeight)) {
		return SP_OK;
	}
	
    /*source scale size may not large than real source (equal to dst) bitmap size*/
	if( (dstWidth > g2dHalCtx->dstWidth) ||(dstHeight > g2dHalCtx->dstHeight))
		return SP_FAIL;

    if (srcWidth != dstWidth) {
        g2dHalCtx->ctrlReg |= G2DENG_CTRL_HOR_SCALE_EN;
        if (srcWidth > dstWidth) {
            /*horizontal scale down*/
            g2dReg->HFactorValue = ((scaleValue & ATTR_2D_SCALE_INIT_MSK) << ATTR_2D_SCALE_INIT_OFFSET)
                | (ceil((dstWidth<<16), srcWidth) & ATTR_2D_SCALE_FACT_MSK);
        }
        else {
            g2dReg->HFactorValue = ((scaleValue & ATTR_2D_SCALE_INIT_MSK) << ATTR_2D_SCALE_INIT_OFFSET)
                | (ceil((srcWidth<<16), dstWidth) & ATTR_2D_SCALE_FACT_MSK);
        }
    }

    if (srcHeight != dstHeight) {
        g2dHalCtx->ctrlReg |= G2DENG_CTRL_VERT_SCALE_EN;
        if (srcHeight > dstHeight) {
            /*vertical scale down*/
            g2dReg->VFactorValue = ((scaleValue & ATTR_2D_SCALE_INIT_MSK) << ATTR_2D_SCALE_INIT_OFFSET)
                | (ceil((dstHeight<<16), srcHeight) & ATTR_2D_SCALE_FACT_MSK);
        }
        else {
            g2dReg->VFactorValue = ((scaleValue & ATTR_2D_SCALE_INIT_MSK) << ATTR_2D_SCALE_INIT_OFFSET)
                | (ceil((srcHeight<<16), dstHeight) & ATTR_2D_SCALE_FACT_MSK);
        }
    }
    
    /*g2dHalCtx->ctrlReg |= G2DENG_CTRL_DITHER_EN;*/
    g2dReg->srcScalHeightWidth = (srcWidth & ATTR_2D_SRC_SCL_WIDTH_MSK) << ATTR_2D_SRC_SCL_WIDTH_OFFSET
        | (srcHeight & ATTR_2D_SRC_SCL_HEIGHT_MSK) << ATTR_2D_SRC_SCL_HEIGHT_OFFSET;

    /*gpHalLinebufferSetMode(LINEBUF_MODE_DDGG);*/

    return SP_OK;
}

/**
* @brief enable 2D gradient fill function.
* @param grdtDir [in] : horizontal or vertical gradient direction
* @param grdtCtx [in] : gradient context include delta color and flip direction
* @return : SP_OK(0)/SP_FAIL
*/
UINT32
gpHal2dEnableGradientFill(
    UINT8 grdtDir,
    gp2dGrdtCtx_t *pgrdtCtx
)
{
    if (g2dHalCtx->ropEnable != TRUE) {
        return SP_FAIL;
    }
	if (NULL == pgrdtCtx) {
		return SP_FAIL;
	}

    switch ( grdtDir ) {
    case G2DENG_GRDT_HORIZONTAL:
        g2dHalCtx->ctrlReg |= G2DENG_CTRL_HOR_SHAD_EN;
        g2dReg->VHDeltaR &= ~(ATTR_2D_HDELTA_COLOR_MSK << ATTR_2D_HDELTA_COLOR_OFFSET);
        g2dReg->VHDeltaG &= ~(ATTR_2D_HDELTA_COLOR_MSK << ATTR_2D_HDELTA_COLOR_OFFSET);
        g2dReg->VHDeltaB &= ~(ATTR_2D_HDELTA_COLOR_MSK << ATTR_2D_HDELTA_COLOR_OFFSET);
        g2dReg->VHDeltaR |= (pgrdtCtx->deltaR & ATTR_2D_HDELTA_COLOR_MSK) << ATTR_2D_HDELTA_COLOR_OFFSET;
        g2dReg->VHDeltaG |= (pgrdtCtx->deltaG & ATTR_2D_HDELTA_COLOR_MSK) << ATTR_2D_HDELTA_COLOR_OFFSET;
        g2dReg->VHDeltaB |= (pgrdtCtx->deltaB & ATTR_2D_HDELTA_COLOR_MSK) << ATTR_2D_HDELTA_COLOR_OFFSET;
        if ( pgrdtCtx->flipDir == 0) {
            g2dHalCtx->ctrlReg &= ~(G2DENG_CTRL_HOR_SHAD_FLG);
        }
        else {
            g2dHalCtx->ctrlReg |= G2DENG_CTRL_HOR_SHAD_FLG;
        }
        break;
    case G2DENG_GRDT_VERTICAL:
        g2dHalCtx->ctrlReg |= G2DENG_CTRL_VERT_SHAD_EN;
        g2dReg->VHDeltaR &= ~(ATTR_2D_VDELTA_COLOR_MSK << ATTR_2D_VDELTA_COLOR_OFFSET);
        g2dReg->VHDeltaG &= ~(ATTR_2D_VDELTA_COLOR_MSK << ATTR_2D_VDELTA_COLOR_OFFSET);
        g2dReg->VHDeltaB &= ~(ATTR_2D_VDELTA_COLOR_MSK << ATTR_2D_VDELTA_COLOR_OFFSET);
        g2dReg->VHDeltaR |= (pgrdtCtx->deltaR & ATTR_2D_VDELTA_COLOR_MSK) << ATTR_2D_VDELTA_COLOR_OFFSET;
        g2dReg->VHDeltaG |= (pgrdtCtx->deltaG & ATTR_2D_VDELTA_COLOR_MSK) << ATTR_2D_VDELTA_COLOR_OFFSET;
        g2dReg->VHDeltaB |= (pgrdtCtx->deltaB & ATTR_2D_VDELTA_COLOR_MSK) << ATTR_2D_VDELTA_COLOR_OFFSET;
        if ( pgrdtCtx->flipDir == 0) {
            g2dHalCtx->ctrlReg &= ~(G2DENG_CTRL_VERT_SHAD_FLG);
        }
        else {
            g2dHalCtx->ctrlReg |= G2DENG_CTRL_VERT_SHAD_FLG;
        }
        break;
    default:
        return SP_FAIL;
    }

    return SP_OK;
}

/**
* @brief enable 2D rotate and mirror function.
* @param rotateType [in] : rotate type
* @param mirrorEnable [in] : enable mirror function or not
* @param srcRefPos [in] : Source reference point
* @param dstRefPos [in] : Destination reference point
* @return : SP_OK(0)/SP_FAIL
*/
UINT32
gpHal2dEnableRotate(
    UINT8 rotateType,
    UINT8 mirrorEnable,
    spPoint_t *psrcRefPos,
    spPoint_t *pdstRefPos,
    spRect_t *popRect
)
{
    if ((rotateType == G2DENG_ROTATE_0) && (mirrorEnable == 0)) {
        return SP_FAIL;
    }
	if ((NULL == psrcRefPos) || (NULL == pdstRefPos) || (NULL == popRect)) {
		return SP_FAIL;
	}

    g2dReg->dstStartYX = (popRect->x & ATTR_2D_STARTX_MSK) << ATTR_2D_STARTX_OFFSET
        | (popRect->y & ATTR_2D_STARTY_MSK) << ATTR_2D_STARTY_OFFSET;
    g2dReg->rectHeightWidth = (popRect->width << ATTR_2D_RECT_WIDTH_OFFSET) | popRect->height;

    g2dHalCtx->ctrlReg |= G2DENG_CTRL_MIRROR_ROT_EN;
    g2dHalCtx->ctrlReg &= ~(G2DENG_CTRL_ROT_MSK);
    g2dHalCtx->ctrlReg |= (rotateType << G2DENG_CTRL_ROT_OFFSET);
    if (mirrorEnable != 0) {
        g2dHalCtx->ctrlReg |= G2DENG_CTRL_MIRROR_ON_EN;
    }
    g2dReg->plStartYX = (psrcRefPos->x & ATTR_2D_SRC_REF_X_MSK) << ATTR_2D_SRC_REF_X_OFFSET
        | (psrcRefPos->y & ATTR_2D_SRC_REF_Y_MSK);
    g2dReg->plendYX = (pdstRefPos->x & ATTR_2D_DST_REF_X_MSK) << ATTR_2D_DST_REF_X_OFFSET
        | (pdstRefPos->y & ATTR_2D_DST_REF_Y_MSK);

    return SP_OK;

}

/**
* @brief enable 2D rotate and mirror function.
* @param enableType [in] : enable screen or rectangle bound clipping
* @param clipRect [in] : clipping rectangle
* @return : SP_OK(0)/SP_FAIL
*/
UINT32
gpHal2dEnableClip(
    UINT8 enableType,
    spRect_t *pclipRect
)
{
    if (enableType > G2DENG_CLIP_SCREEN) {
        return SP_FAIL;
    }
	if (NULL == pclipRect) {
		return SP_FAIL;
	}
	
    g2dHalCtx->ctrlReg &= ~(G2DENG_CTRL_CLIP_MSK);
    g2dHalCtx->ctrlReg |= (G2DENG_CTRL_RECT_CLIP_EN << enableType);
    if (enableType == G2DENG_CLIP_RECTANGLE){
        g2dReg->clipBottomTop = ((pclipRect->y & ATTR_2D_CLIP_TOP_MSK) << ATTR_2D_CLIP_TOP_OFFSET)
            | ((pclipRect->y + pclipRect->height) & ATTR_2D_CLIP_BTM_MSK);
        g2dReg->clipRightLeft = ((pclipRect->x & ATTR_2D_CLIP_LEFT_MSK) << ATTR_2D_CLIP_LEFT_OFFSET)
            | ((pclipRect->x + pclipRect->width) & ATTR_2D_CLIP_RIGHT_MSK);
    }

    return SP_OK;
}

/**
* @brief get 2D engine status.
* @return : status reg value
*/
UINT32
gpHal2dGetStatus(
void
)
{
    return g2dReg->statusReg;
}

/**
 * @brief   2D clock enable/disable function
 * @param   enable[in]: disable(0)/enable(1)
 * @return  None
 * @see
 */
void
gpHal2dClkEnable(
	UINT32 enable
)
{
	if (enable){
		gp_scucReg->scucPeriClkEn |= (SCU_C_PERI_2D_ENGIN | SCU_C_PERI_2DSCALEABT);
		/*gp_scuaReg->scuaPeriClkEn |= SCU_A_PERI_LINEBUFFER ;
              gp_scubReg->scubSysCntEn |= (1 << 0);*/
	}
	else{
		gp_scucReg->scucPeriClkEn &= ~(SCU_C_PERI_2D_ENGIN | SCU_C_PERI_2DSCALEABT);
		/*gp_scuaReg->scuaPeriClkEn &= ~SCU_A_PERI_LINEBUFFER ;
              gp_scubReg->scubSysCntEn &= ~(1 << 0);*/
	}
}

/**
* @brief Set line buffer mode
* @return : SP_OK(0)/SP_FAIL
*/
UINT32 gpHalLinebufferSetMode(UINT32 mode)
{
    UINT32 value;

    value = gp_scuaReg->scuaDispType;
    value &= ~(DISP_TYPE_LINE_BUFFER_MODE_MSK);
    value |= (mode & DISP_TYPE_LINE_BUFFER_MODE_MSK);
    gp_scuaReg->scuaDispType = value;

    return SP_OK;
}

/**
* @brief Enable/disable dither
* @param enable [in] : Zero for disable, other enable
* @return : SP_OK(0)/SP_FAIL
*/
UINT32
gpHal2dEnableDither(
    UINT32 enable
)
{    
    if (enable) {
        g2dHalCtx->ctrlReg |= G2DENG_CTRL_DITHER_EN;
    }
    else {
        g2dHalCtx->ctrlReg &= ~G2DENG_CTRL_DITHER_EN;
    }

    return SP_OK;
}


