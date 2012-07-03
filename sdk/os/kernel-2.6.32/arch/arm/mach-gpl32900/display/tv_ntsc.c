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

/*!
 * @file tv_ntsc.h
 * @brief The tv driver
 */
#include <linux/init.h>
#include <linux/configfs.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <asm/system.h> /* cli(), *_flags */
#include <asm/uaccess.h> /* copy_from/to_user */

#include <mach/panel_cfg.h>
#include <mach/hardware.h>
#include <mach/hal/hal_clock.h>
#include <mach/hal/hal_disp.h>
#include <mach/hal/sysregs.h>
#include <mach/gp_display.h>
#include <mach/clock_mgr/gp_clock.h>

MODULE_LICENSE("GPL");

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static int32_t tv_init(void);
static int32_t tv_suspend(void);
static int32_t tv_resume(void);
static int32_t tv_set_mode(void *data);

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define		VDAC_PDALL					(1 << 0)
#define		VDAC_TEST					(1 << 1)
#define		VDAC_UD						(1 << 2)
#define     VDAC_CRYSTAL_EN             (1 << 3)

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct
{
	char *name;
	uint32_t width;
	uint32_t height;
	uint32_t workFreq;
	uint32_t dmaType;
	uint32_t tvType;
	uint32_t pulse6;
	uint32_t scanSel;
	uint32_t fscType;
	uint32_t fix625;
	uint32_t lineSel;
	uint32_t cbWidth;
	uint32_t cbSel;
	uint32_t cftType;
	uint32_t cupType;
	uint32_t yupType;
	uint32_t cfunc;
	gpHalDispTvAmpAdj_t ampAdj;
	gpHalDispTvPosAdj_t posAdj;
	uint32_t ditherType;
	gp_disp_ditherparam_t *pDitherParam;
	gp_disp_colormatrix_t *pColorMatrix;
	uint8_t *pGammaTable[3];
} disp_panelInfo;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static const char gPanelName[] = "TV";

static const gp_disp_ditherparam_t gDitherParam = {
	.d00 = 1,
	.d01 = 1,
	.d02 = 1,
	.d03 = 1,
	.d10 = 1,
	.d11 = 1,
	.d12 = 1,
	.d13 = 1,
	.d20 = 1,
	.d21 = 1,
	.d22 = 1,
	.d23 = 1,
	.d30 = 1,
	.d31 = 1,
	.d32 = 1,
	.d33 = 1,
};

static const gp_disp_colormatrix_t gColorMatrix = {
	.a00 = 0x50,
	.a01 = 0xFFBD,
	.a02 = 0xFFF3,
	.a10 = 0xFFED,
	.a11 = 0xFFDA,
	.a12 = 0x39,
	.a20 = 0x27,
	.a21 = 0x4C,
	.a22 = 0xF,
	.b0 = 0,
	.b1 = 0,
	.b2 = 0,
};

static const uint8_t gammaR[256] = {1};
static const uint8_t gammaG[256] = {1};
static const uint8_t gammaB[256] = {1};

static disp_panelInfo gPanelInfo = {
	.name = (char*) &gPanelName,
	.width = 720,
	.height = 480,
	.workFreq = 27000000,
	.dmaType = HAL_DISP_DMA_INTERLACED,
	.tvType = HAL_DISP_TV_TYPE_NTSC,
	.pulse6 = HAL_DISP_TV_PULSE6_6PULSE,
	.scanSel = HAL_DISP_TV_SCANSEL_INTERLACED,
	.fscType = HAL_DISP_TV_FSCTYPE_NTSCMJ,
	.fix625 = 0,
	.lineSel = HAL_DISP_TV_LINESEL_262_525,
	.cbWidth = HAL_DISP_TV_CBWIDTH_252,
	.cbSel = HAL_DISP_TV_CBSEL_NTSCMJ,
	.cftType = 0,
	.cupType = 0,
	.yupType = 0,
	.cfunc = 0,
	.ampAdj = {
		.luminance = 240,
		.blank = 42,
		.burst = 112,
	},
	.posAdj = {
		.vAct0 = 26,
		.vAct1 = 26,
		.hAct = 32,
	},
	.ditherType = HAL_DISP_DITHER_FIXED,
	.pDitherParam = (gp_disp_ditherparam_t *) &gDitherParam,
	.pColorMatrix = (gp_disp_colormatrix_t *) &gColorMatrix,
	.pGammaTable = {
		(uint8_t*) &gammaR,
		(uint8_t*) &gammaG,
		(uint8_t*) &gammaB,
	},
};

/* access functions */
gp_disp_panel_ops_t tv_fops = {
	.init = tv_init,
	.suspend = tv_suspend,
	.resume = tv_resume,
	.get_size = NULL,
	.set_param = tv_set_mode,
};

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static void
scu_tvout_vdec_enable(
	void
)
{
	int32_t val = 0;
	val = SCUA_VDAC_CFG;
	if(0 == (val & VDAC_CRYSTAL_EN)){
		val |= VDAC_CRYSTAL_EN;
		SCUA_VDAC_CFG = val;
		val &= (~VDAC_PDALL);
		SCUA_VDAC_CFG = val;
	}
}

static void
scu_tvout_vdec_down(
	void
)
{
    uint32_t val = 0;
	
    val = SCUA_VDAC_CFG;
    if(val & VDAC_CRYSTAL_EN){
        val |= VDAC_PDALL;
        SCUA_VDAC_CFG = val;
        val &= (~VDAC_CRYSTAL_EN);
        SCUA_VDAC_CFG = val;
    }
}

static int32_t
tv_common_init(
	void
)
{
	int32_t real_freq=0;

	printk("[%s:%d]\n", __FUNCTION__, __LINE__);

	gpHalDispSetDevType(HAL_DISP_DEV_TV);

	// set lcd source : XTAL(27MHZ)
	gpHalClkLcdSetSrc(HAL_LCD_CLK_XTAL);
	// Set lcd clock rate, must enable before lcd clk on
	gp_clk_set_rate("clk_lcd", gPanelInfo.workFreq, &real_freq, NULL);	// must enable before lcd clk on

#ifdef CONFIG_PM
	gp_enable_clock((int*)"READLTIME_ABT", 1);
#endif
	gpHalLcdClkEnable(1);
	scu_tvout_vdec_enable();

	gpHalDispSetRes(gPanelInfo.width, gPanelInfo.height);
	gpHalDispSetTvType(gPanelInfo.tvType);
	gpHalDispSetTvPulse(gPanelInfo.pulse6);
	gpHalDispSetTvScan(gPanelInfo.scanSel);
	gpHalDispSetTvFscType(gPanelInfo.fscType);
	gpHalDispSetTvFix625(gPanelInfo.fix625);
	gpHalDispSetTvLine(gPanelInfo.lineSel);
	gpHalDispSetTvColorBurstWidth(gPanelInfo.cbWidth);
	gpHalDispSetTvColorBurstSel(gPanelInfo.cbSel);
	gpHalDispSetTvCftType(gPanelInfo.cftType);
	gpHalDispSetTvCupType(gPanelInfo.cupType);
	gpHalDispSetTvYupType(gPanelInfo.yupType);
	gpHalDispSetTvAmpAdj(gPanelInfo.ampAdj);
	gpHalDispSetTvPosAdj(gPanelInfo.posAdj);

	gpHalDispSetPriDmaType(gPanelInfo.dmaType);
	gpHalDispSetOsdDmaType(0, gPanelInfo.dmaType);
	gpHalDispSetOsdDmaType(1, gPanelInfo.dmaType);

	return 0;
}

static int32_t
tv_init(
	void
)
{
	tv_common_init();

	/* Set color matrix */
	disp_set_color_matrix(gPanelInfo.pColorMatrix);

#if 0
	/* Set dither */
	disp_set_dither_type(gPanelInfo.ditherType);
	disp_set_dither_param(gPanelInfo.pDitherParam);
	gpHalDispSetDitherEnable(0);

	/* Set color matrix */
	disp_set_color_matrix(gPanelInfo.pColorMatrix);

	/* Set gamma table */
	disp_set_gamma_table(SP_DISP_GAMMA_R, (uint8_t*) &gPanelInfo.pGammaTable[SP_DISP_GAMMA_R]);
	disp_set_gamma_table(SP_DISP_GAMMA_G, (uint8_t*) &gPanelInfo.pGammaTable[SP_DISP_GAMMA_G]);
	disp_set_gamma_table(SP_DISP_GAMMA_B, (uint8_t*) &gPanelInfo.pGammaTable[SP_DISP_GAMMA_B]);
	disp_set_gamma_enable(0);
#endif

	return 0;
}

static int32_t
tv_suspend(
	void
)
{
	gpHalLcdClkEnable(0);
#ifdef CONFIG_PM
	gp_enable_clock((int*)"READLTIME_ABT", 0);
#endif
	scu_tvout_vdec_down();

	return 0;
}

static int32_t
tv_resume(
	void
)
{
	return tv_common_init();
}

static int32_t
tv_set_mode(
	void *data
)
{
	int32_t mode;

	mode = *(uint32_t *) data;

	if (mode == SP_DISP_TV_MODE_NTSC) {
		gPanelInfo.height = 480;
		gPanelInfo.tvType = HAL_DISP_TV_TYPE_NTSC;
		gPanelInfo.pulse6 = HAL_DISP_TV_PULSE6_6PULSE;
		gPanelInfo.scanSel = HAL_DISP_TV_SCANSEL_INTERLACED;
		gPanelInfo.fscType = HAL_DISP_TV_FSCTYPE_NTSCMJ;
		gPanelInfo.fix625 = 0;
		gPanelInfo.lineSel = HAL_DISP_TV_LINESEL_262_525;
		gPanelInfo.cbWidth = HAL_DISP_TV_CBWIDTH_252;
		gPanelInfo.cbSel = HAL_DISP_TV_CBSEL_NTSCMJ;
		gPanelInfo.ampAdj.luminance = 240;
		gPanelInfo.ampAdj.blank = 42;
		gPanelInfo.ampAdj.burst = 112;
		gPanelInfo.posAdj.vAct0 = 26;
		gPanelInfo.posAdj.vAct1 = 26;
		gPanelInfo.posAdj.hAct = 32;
	}
	else if (mode == SP_DISP_TV_MODE_PAL) {
		gPanelInfo.height = 576;
		gPanelInfo.tvType = HAL_DISP_TV_TYPE_PAL;
		gPanelInfo.pulse6 = HAL_DISP_TV_PULSE6_6PULSE;
		gPanelInfo.scanSel = HAL_DISP_TV_SCANSEL_INTERLACED;
		gPanelInfo.fscType = HAL_DISP_TV_FSCTYPE_PALBDGHIN;
		gPanelInfo.fix625 = 1;
		gPanelInfo.lineSel = HAL_DISP_TV_LINESEL_312_625;
		gPanelInfo.cbWidth = HAL_DISP_TV_CBWIDTH_225;
		gPanelInfo.cbSel = HAL_DISP_TV_CBSEL_PALBDGHINNC;
		gPanelInfo.ampAdj.luminance = 240;
		gPanelInfo.ampAdj.blank = 42;
		gPanelInfo.ampAdj.burst = 117;
		gPanelInfo.posAdj.vAct0 = 32;
		gPanelInfo.posAdj.vAct1 = 32;
		gPanelInfo.posAdj.hAct = 52;
	}
	else {
		printk("[%s:%d], unknow mode=%d\n", __FUNCTION__, __LINE__, mode);
		return -1;
	}

	return 0;
}

static int32_t
tvPanel_init(
	void
)
{
  printk("[%s:%d]\n", __FUNCTION__, __LINE__);
  register_paneldev(SP_DISP_OUTPUT_TV, gPanelInfo.name, &tv_fops);
  return 0;
}

static void
tvPanel_exit(
	void
)
{
  printk("[%s:%d]\n", __FUNCTION__, __LINE__);
  unregister_paneldev(SP_DISP_OUTPUT_TV, gPanelInfo.name);
}

module_init(tvPanel_init);
module_exit(tvPanel_exit);
