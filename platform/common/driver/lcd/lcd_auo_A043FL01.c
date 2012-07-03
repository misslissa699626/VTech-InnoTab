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
 * @file lcd_auo_A043FL01.h
 * @brief The lcd driver of AUO A043FL01
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
#include <mach/hal/hal_gpio.h>
#include <mach/hal/hal_clock.h>
#include <mach/hal/hal_disp.h>
#include <mach/gp_display.h>
#include <mach/clock_mgr/gp_clock.h>
#include <mach/gp_board.h>
#include <mach/module.h>

MODULE_LICENSE_GP;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static int32_t lcd_init(void);
static int32_t lcd_suspend(void);
static int32_t lcd_resume(void);
static int32_t lcd_get_size(gp_size_t *size);

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define R_PPU_IRQ_EN        		(*((volatile unsigned int *) IO3_ADDRESS(0x20188)))
#define R_PPU_IRQ_STATUS    		(*((volatile unsigned int *) IO3_ADDRESS(0x2018C)))
#define R_TFT1_CTRL       	    	(*((volatile unsigned int *) IO3_ADDRESS(0x20140)))
#define R_SCUA_SYS_SEL			    (*((volatile unsigned int *) IO3_ADDRESS(0x070E0)))
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
	uint32_t clkPolatiry;
	uint32_t clkType;
	uint32_t format;
	uint32_t type;
	uint32_t dataSeqEven;
	uint32_t dataSeqOdd;
	gpHalDispLcdTiming_t vsync;
	gpHalDispLcdTiming_t hsync;
	gp_size_t panelSize;	/* Panel physical size in mm */
	uint32_t ditherType;
	gp_disp_ditherparam_t *pDitherParam;
	gp_disp_colormatrix_t *pColorMatrix;
	uint8_t *pGammaTable[3];
} disp_panelInfo;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static const char gPanelName[] = "LCD_AUO_A043FL01";
static int32_t g_pin_func_handle;

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
	.a00 = 0x0100,
	.a01 = 0,
	.a02 = 0,
	.a10 = 0,
	.a11 = 0x0100,
	.a12 = 0,
	.a20 = 0,
	.a21 = 0,
	.a22 = 0x0100,
	.b0 = 0,
	.b1 = 0,
	.b2 = 0,
};

static const uint8_t gammaR[256] = {1};
static const uint8_t gammaG[256] = {1};
static const uint8_t gammaB[256] = {1};

static disp_panelInfo gPanelInfo = {
	.name = (char*) &gPanelName,
	.width = 480,
	.height = 272,
	.workFreq = 9000000,
	.dmaType = HAL_DISP_DMA_PROGRESSIVE,
	.clkPolatiry = 1,
	.clkType = HAL_DISP_CLK_TYPE_RGB888,
	.format = HAL_DISP_OUTPUT_FMT_RGB,
	.type = HAL_DISP_OUTPUT_TYPE_PRGB888,
	.dataSeqEven = HAL_DISP_PRGB888_RGB,
	.dataSeqOdd = HAL_DISP_PRGB888_RGB,
	.vsync = {
		.polarity = 0,
		.fPorch = 4,
		.bPorch = 12,
		.width = 8,
	},
	.hsync = {
		.polarity = 0,
		.fPorch = 8,
		.bPorch = 45,
		.width = 41,
	},
	.panelSize = {
		.width = 95,
		.height = 54,
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
static gp_disp_panel_ops_t lcd_fops = {
	.init = lcd_init,
	.suspend = lcd_suspend,
	.resume = lcd_resume,
	.get_size = lcd_get_size,
};

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static int32_t
auo_a043fl01_init(
	void
)
{
	int32_t real_freq = 0;

	printk("[%s:%d]\n", __FUNCTION__, __LINE__);

	g_pin_func_handle = gp_board_pin_func_request(GP_PIN_DISP0_LCD, GP_BOARD_WAIT_FOREVER);
	if (g_pin_func_handle < 0) {
		printk("[%s:%d] Error!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	/* initial panel parameter */
	gpHalDispSetDevType(HAL_DISP_DEV_LCD);

	// set lcd source : XTAL(27MHZ)
	gpHalClkLcdSetSrc(HAL_LCD_CLK_XTAL);
	// Set lcd clock rate to 9 MHz, must enable before lcd clk on
	gp_clk_set_rate("clk_lcd", gPanelInfo.workFreq, &real_freq, NULL);
	printk("[%s][%d] LCD CLOCK=%d Hz\n",__FUNCTION__, __LINE__, real_freq);

	gpHalDispSetClkType(gPanelInfo.clkType);

	// enable lcd clock
#ifdef CONFIG_PM
	gp_enable_clock((int*)"READLTIME_ABT", 1);
#endif
	gpHalLcdClkEnable(1);

	gpHalDispSetRes(gPanelInfo.width, gPanelInfo.height);
	gpHalDispSetLcdVsync(gPanelInfo.vsync);
	gpHalDispSetLcdHsync(gPanelInfo.hsync);
	gpHalDispSetPanelFormat(gPanelInfo.format, gPanelInfo.type, gPanelInfo.dataSeqEven, gPanelInfo.dataSeqOdd);
	gpHalDispSetClkPolarity(gPanelInfo.clkPolatiry);

	gpHalDispSetPriDmaType(gPanelInfo.dmaType);
	gpHalDispSetOsdDmaType(0, gPanelInfo.dmaType);
	gpHalDispSetOsdDmaType(1, gPanelInfo.dmaType);

	return 0;
}

static int32_t
lcd_init(
	void
)
{
	int32_t ret;

	ret = auo_a043fl01_init();
	if (ret < 0) {
		return ret;
	}

	/* Set color matrix */
	disp_set_color_matrix(gPanelInfo.pColorMatrix);

#if 0
	/* Set dither */
	disp_set_dither_type(gPanelInfo.ditherType);
	disp_set_dither_param(gPanelInfo.pDitherParam);
	disp_set_dither_enable(0);

	/* Set gamma table */
	disp_set_gamma_table(SP_DISP_GAMMA_R, (uint8_t*) gPanelInfo.pGammaTable[SP_DISP_GAMMA_R]);
	disp_set_gamma_table(SP_DISP_GAMMA_G, (uint8_t*) gPanelInfo.pGammaTable[SP_DISP_GAMMA_G]);
	disp_set_gamma_table(SP_DISP_GAMMA_B, (uint8_t*) gPanelInfo.pGammaTable[SP_DISP_GAMMA_B]);
	disp_set_gamma_enable(0);
#endif

	return 0;
}

static int32_t
auo_a043fl01_resume_init(
	void
)
{
	int32_t real_freq = 0;
	
	/* initial panel parameter */
	gpHalDispSetDevType(HAL_DISP_DEV_LCD);

	// set lcd source : XTAL(27MHZ)
	gpHalClkLcdSetSrc(HAL_LCD_CLK_XTAL);
	// Set lcd clock rate to 9 MHz, must enable before lcd clk on
	gp_clk_set_rate("clk_lcd", gPanelInfo.workFreq, &real_freq, NULL);
	printk("[%s][%d] LCD CLOCK=%d Hz\n",__FUNCTION__, __LINE__, real_freq);

	gpHalDispSetClkType(gPanelInfo.clkType);

	// enable lcd clock
#ifdef CONFIG_PM
	gp_enable_clock((int*)"READLTIME_ABT", 1);
#endif
	gpHalLcdClkEnable(1);

	gpHalDispSetRes(gPanelInfo.width, gPanelInfo.height);
	gpHalDispSetLcdVsync(gPanelInfo.vsync);
	gpHalDispSetLcdHsync(gPanelInfo.hsync);
	gpHalDispSetPanelFormat(gPanelInfo.format, gPanelInfo.type, gPanelInfo.dataSeqEven, gPanelInfo.dataSeqOdd);
	gpHalDispSetClkPolarity(gPanelInfo.clkPolatiry);

	gpHalDispSetPriDmaType(gPanelInfo.dmaType);
	gpHalDispSetOsdDmaType(0, gPanelInfo.dmaType);
	gpHalDispSetOsdDmaType(1, gPanelInfo.dmaType);

	return 0;
}

static int32_t
lcd_suspend(
	void
)
{
	printk("[%s:%d]\n", __FUNCTION__, __LINE__);
	
	gpHalLcdClkEnable(0);
#ifdef CONFIG_PM
	gp_enable_clock((int*)"READLTIME_ABT", 0);
#endif
	
	if (R_TFT1_CTRL & 0x1) {
    	R_SCUA_SYS_SEL |= 0x2;
    }
    else {
    	/* board config : release pin function */
		gp_board_pin_func_release(g_pin_func_handle);	
	}
	return 0;
}

static int32_t
lcd_resume(
	void
)
{
    int i = 0;
    unsigned int cnt = 0;
    unsigned int ppuirq = R_PPU_IRQ_EN;
    
	if (R_TFT1_CTRL & 0x1) {
		auo_a043fl01_resume_init();
		if ((ppuirq & 0x2000) == 0) {
			R_PPU_IRQ_EN |= 0x2000;
		}
		while(1) {
			if ((R_PPU_IRQ_STATUS & 0x2000)) {
				R_PPU_IRQ_STATUS = 0x2000;
				if (i >= 1) {
					R_SCUA_SYS_SEL &= ~0x2;
					break;
				}
				i++;
				cnt = 0;
			}
			if (cnt++ > 0x700000) {
				printk("wait TFT1 blanking timeout\n");
				break;
			} 
		}
		R_PPU_IRQ_EN = ppuirq;
		R_TFT1_CTRL = 0;
	}
	else {
		auo_a043fl01_init();
	}	
	
	return 0;
}

static int32_t
lcd_get_size(
	gp_size_t *size
)
{
	if (gPanelInfo.panelSize.width && gPanelInfo.panelSize.height) {
		size->width = gPanelInfo.panelSize.width;
		size->height = gPanelInfo.panelSize.height;
		return 0;
	}
	else {
		return -1;
	}
}

static int32_t
panel_init(
	void
)
{
  printk("[%s:%d]\n", __FUNCTION__, __LINE__);
  register_paneldev(SP_DISP_OUTPUT_LCD, gPanelInfo.name, &lcd_fops);
  return 0;
}

static void
panel_exit(
	void
)
{
  printk("[%s:%d]\n", __FUNCTION__, __LINE__);
  unregister_paneldev(SP_DISP_OUTPUT_LCD, gPanelInfo.name);
}

module_init(panel_init);
module_exit(panel_exit);
