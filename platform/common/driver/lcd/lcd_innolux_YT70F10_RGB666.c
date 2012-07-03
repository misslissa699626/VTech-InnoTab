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
 * @file lcd_innolux_YT70F10_RGB666.h
 * @brief The lcd driver of Innolux YT70F10_RGB666
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
#include <linux/delay.h> 	/* udelay/mdelay */

#include <mach/panel_cfg.h>
#include <mach/hardware.h>
#include <mach/hal/hal_clock.h>
#include <mach/hal/hal_disp.h>
#include <mach/gp_display.h>
#include <mach/clock_mgr/gp_clock.h>
#include <mach/gp_board.h>
#include <mach/gp_gpio.h>
#include <mach/hal/hal_gpio.h>
#include <mach/module.h>
#include <mach/hal/regmap/reg_scu.h>
MODULE_LICENSE_GP;


// PCLK
#define	PCLK_FREQ			30000000
#define	PCLK_POL			1
// Display Size
#define	DISP_SIZE_W		800
#define	DISP_SIZE_H		480
// H-Sync
#define	HD_PULSE_W		48
#define	HD_PULSE_POL	0
#define	HD_FRONT_P		40
#define	HD_BACK_P			88
// V-Sync
#define	VD_PULSE_W		3
#define	VD_PULSE_POL	0
#define	VD_FRONT_P		13
#define	VD_BACK_P			32
// Panel Size unit:mm
#define	PANEL_SIZE_W	154		/* int61_t */
#define	PANEL_SIZE_H	87		/* int61_t */
// GPIO Setting : PCLK, V-sync, H-sync
#define	PCLK_channel		GPIO_CHANNEL_3
#define	PCLK_func				2
#define	PCLK_gid				6
#define	PCLK_pin				3				/* IOD0 ~ IOD10 */
#define	PCLK_GPIO				0
// GPIO Setting : B-Data
#define	B_DATA_channel		GPIO_CHANNEL_1
#define	B_DATA_func			2
#define	B_DATA_gid			8
#define	B_DATA_pin			0				/* IOB0 ~ IOB7 */
#define	B_DATA_GPIO			0
// GPIO Setting : G-Data
#define	G_DATA_channel		GPIO_CHANNEL_2
#define	G_DATA_func			2
#define	G_DATA_gid			9
#define	G_DATA_pin			8				/* IOC8 ~ IOC11 */
#define	G_DATA_GPIO			0
// GPIO Setting : R-Data
#define	R_DATA_channel		GPIO_CHANNEL_2
#define	R_DATA_func			2
#define	R_DATA_gid			10
#define	R_DATA_pin			12			/* IOC12 ~ IOC15 */
#define	R_DATA_GPIO			0
// GPIO Setting : LCD_DE
#define	DEM_channel		GPIO_CHANNEL_0
#define	DEM_func			2
#define	DEM_gid			7
#define	DEM_pin			15
#define	DEM_GPIO			2

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
static const char gPanelName[] = "LCD_Innolux_YT70F10_RGB666";

static const gp_disp_ditherparam_t gDitherParam = {
	.d00 = 0x0,
	.d01 = 0xe,
	.d02 = 0x2,
	.d03 = 0xc,
	.d10 = 0xa,
	.d11 = 0x4,
	.d12 = 0x8,
	.d13 = 0x6,
	.d20 = 0xc,
	.d21 = 0x2,
	.d22 = 0xe,
	.d23 = 0x0,
	.d30 = 0x6,
	.d31 = 0x8,
	.d32 = 0x4,
	.d33 = 0xa,
};

static const gp_disp_colormatrix_t gColorMatrix = {
	.a00 = 0x0100,
	.a01 = 0x0,
	.a02 = 0x0,
	.a10 = 0x0,
	.a11 = 0x0100,
	.a12 = 0x0,
	.a20 = 0x0000,
	.a21 = 0x0000,
	.a22 = 0x0100,
	.b0 = 0x0000,
	.b1 = 0x0000,
	.b2 = 0x0000,
};

static const uint8_t gammaR[256] = { 0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x0F,0x10,0x10,0x11,0x11,0x12,0x13,0x13,0x13,0x14,0x14,0x15,0x15,0x16,
0x16,0x17,0x18,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x27,0x28,0x29,0x2A,0x2A,0x2B,0x2C,0x2D,0x2D,0x2E,0x2F,0x30,
0x30,0x31,0x32,0x33,0x34,0x34,0x35,0x35,0x36,0x36,0x37,0x38,0x38,0x39,0x39,0x3A,0x3B,0x3B,0x3C,0x3C,0x3D,0x3E,0x3E,0x3F,0x40,0x41,0x41,0x42,0x43,0x44,0x44,0x45,
0x46,0x47,0x47,0x48,0x49,0x4A,0x4B,0x4B,0x4C,0x4D,0x4E,0x4E,0x4F,0x50,0x51,0x51,0x52,0x53,0x54,0x54,0x55,0x56,0x57,0x58,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
0x60,0x61,0x62,0x63,0x65,0x66,0x67,0x68,0x6A,0x6B,0x6D,0x6F,0x70,0x72,0x74,0x75,0x77,0x79,0x7B,0x7C,0x7E,0x80,0x81,0x83,0x85,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,
0x8E,0x8F,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,0xA0,0xA1,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAC,0xAD,0xAE,0xB0,0xB1,
0xB2,0xB4,0xB5,0xB6,0xB8,0xB9,0xBA,0xBC,0xBD,0xBE,0xC0,0xC1,0xC3,0xC4,0xC5,0xC7,0xC8,0xCA,0xCB,0xCD,0xCE,0xD0,0xD1,0xD3,0xD4,0xD6,0xD7,0xD9,0xDA,0xDC,0xDD,0xDE,
0xDF,0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xED,0xEE,0xEE,0xEF,0xEF,0xF0,0xF1,0xF1,0xF2,0xF2,0xF3,0xF4,0xF4,0xF5,0xF5,0xF6,0xF7,
};

static const uint8_t gammaG[256] = { 0x04,0x04,0x05,0x05,0x06,0x07,0x07,0x08,0x09,0x09,0x0A,0x0B,0x0B,0x0C,0x0D,0x0D,0x0E,0x0F,0x0F,0x10,0x10,0x11,0x11,0x12,0x12,0x13,0x13,0x14,0x14,0x15,0x15,0x16,
0x16,0x17,0x18,0x18,0x19,0x1A,0x1B,0x1C,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x21,0x22,0x23,0x24,0x25,0x26,0x26,0x27,0x28,0x29,0x29,0x2A,0x2B,0x2C,0x2C,0x2D,0x2E,0x2F,
0x2F,0x30,0x31,0x32,0x33,0x33,0x34,0x34,0x35,0x35,0x36,0x37,0x37,0x38,0x38,0x39,0x3A,0x3A,0x3B,0x3B,0x3C,0x3D,0x3D,0x3E,0x3E,0x3F,0x40,0x40,0x41,0x42,0x42,0x43,
0x44,0x44,0x45,0x46,0x46,0x47,0x48,0x48,0x49,0x4A,0x4B,0x4C,0x4C,0x4D,0x4E,0x4F,0x50,0x51,0x51,0x52,0x53,0x54,0x55,0x56,0x56,0x57,0x58,0x59,0x59,0x5A,0x5B,0x5C,
0x5D,0x5E,0x5F,0x60,0x62,0x63,0x64,0x65,0x67,0x68,0x6A,0x6C,0x6E,0x6F,0x71,0x73,0x75,0x76,0x78,0x7A,0x7C,0x7D,0x7F,0x81,0x83,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,
0x8C,0x8D,0x8E,0x8F,0x90,0x91,0x92,0x93,0x94,0x95,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xAA,0xAB,0xAC,0xAE,0xAF,
0xB1,0xB2,0xB4,0xB5,0xB7,0xB8,0xBA,0xBB,0xBD,0xBE,0xC0,0xC1,0xC3,0xC4,0xC5,0xC7,0xC8,0xCA,0xCB,0xCC,0xCE,0xCF,0xD1,0xD2,0xD3,0xD5,0xD6,0xD8,0xD9,0xDB,0xDB,0xDC,
0xDD,0xDE,0xDF,0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEB,0xEC,0xEC,0xED,0xED,0xEE,0xEF,0xEF,0xF0,0xF0,0xF1,0xF2,0xF2,0xF3,0xF3,0xF4,0xF5,
};

static const uint8_t gammaB[256] = { 0x03,0x03,0x04,0x05,0x05,0x06,0x07,0x07,0x08,0x09,0x0A,0x0A,0x0B,0x0C,0x0C,0x0D,0x0E,0x0F,0x0F,0x0F,0x10,0x10,0x11,0x11,0x12,0x12,0x13,0x13,0x14,0x14,0x15,0x15,
0x16,0x16,0x17,0x17,0x18,0x19,0x1A,0x1A,0x1B,0x1C,0x1D,0x1D,0x1E,0x1F,0x20,0x20,0x21,0x22,0x23,0x24,0x24,0x25,0x26,0x27,0x27,0x28,0x29,0x2A,0x2A,0x2B,0x2C,0x2D,
0x2D,0x2E,0x2F,0x30,0x31,0x31,0x32,0x32,0x33,0x33,0x34,0x35,0x35,0x36,0x36,0x37,0x38,0x38,0x39,0x39,0x3A,0x3B,0x3B,0x3C,0x3C,0x3D,0x3D,0x3E,0x3F,0x3F,0x40,0x40,
0x41,0x42,0x42,0x43,0x43,0x44,0x45,0x45,0x46,0x47,0x48,0x49,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4E,0x4F,0x50,0x51,0x52,0x53,0x53,0x54,0x55,0x56,0x56,0x57,0x58,0x59,
0x5A,0x5B,0x5C,0x5D,0x5F,0x60,0x61,0x62,0x64,0x65,0x67,0x69,0x6B,0x6D,0x6E,0x70,0x72,0x74,0x76,0x78,0x79,0x7B,0x7D,0x7F,0x81,0x83,0x84,0x85,0x86,0x87,0x89,0x8A,
0x8B,0x8C,0x8E,0x8F,0x90,0x91,0x93,0x94,0x95,0x96,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,0xA0,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAB,0xAC,0xAE,0xAF,0xB1,
0xB3,0xB4,0xB6,0xB8,0xB9,0xBB,0xBD,0xBE,0xC0,0xC2,0xC3,0xC5,0xC7,0xC8,0xCA,0xCC,0xCD,0xCF,0xD1,0xD2,0xD4,0xD6,0xD8,0xD9,0xDB,0xDD,0xDE,0xE0,0xE2,0xE4,0xE5,0xE6,
0xE7,0xE8,0xE9,0xEB,0xEC,0xED,0xEE,0xEF,0xF0,0xF2,0xF3,0xF4,0xF5,0xF6,0xF8,0xF8,0xF8,0xF9,0xF9,0xFA,0xFA,0xFA,0xFB,0xFB,0xFC,0xFC,0xFC,0xFD,0xFD,0xFE,0xFE,0xFF,
};

static disp_panelInfo gPanelInfo = {
	.name = (char*) &gPanelName,
	.width = DISP_SIZE_W,
	.height = DISP_SIZE_H,
	.workFreq = PCLK_FREQ,
	.dmaType = HAL_DISP_DMA_PROGRESSIVE,
	.clkPolatiry = PCLK_POL,
	.clkType = HAL_DISP_CLK_TYPE_RGB666,
	.format = HAL_DISP_OUTPUT_FMT_RGB,
	.type = HAL_DISP_OUTPUT_TYPE_PRGB888,
	.dataSeqEven = HAL_DISP_PRGB888_RGB,
	.dataSeqOdd = HAL_DISP_PRGB888_RGB,
	.vsync = {
		.polarity = VD_PULSE_POL,
		.fPorch = VD_FRONT_P,
		.bPorch = VD_BACK_P,
		.width = VD_PULSE_W,
	},
	.hsync = {
		.polarity = HD_PULSE_POL,
		.fPorch = HD_FRONT_P,
		.bPorch = HD_BACK_P,
		.width = HD_PULSE_W,
	},
	.panelSize = {
		.width = PANEL_SIZE_W,		/* int61_t */
		.height = PANEL_SIZE_H,		/* int61_t */
	},
	.ditherType = HAL_DISP_DITHER_FIXED,
	.pDitherParam = (gp_disp_ditherparam_t *) &gDitherParam,
	.pColorMatrix = (gp_disp_colormatrix_t *) &gColorMatrix,
	.pGammaTable = {
		(uint8_t*) gammaR,
		(uint8_t*) gammaG,
		(uint8_t*) gammaB,
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
lcd_init(
	void
)
{
	int32_t real_freq = 0;
	//volatile unsigned int *pvuiData;
	gp_board_panel_t *lcd_power;
	int handle;
	gpio_content_t ctx;


	SCUA_SAR_GPIO_CTRL = 0;     //switch to TFT0 for GP12 real chip
	lcd_power = gp_board_get_config("panel", gp_board_panel_t);
	
#if 0
	pvuiData = &(SCUA_DUMMY2);		/* SSC setting */
	*pvuiData = 0x0b;
#endif
	printk("[%s:%d]\n", __FUNCTION__, __LINE__);
#if 0
	gpHalDispSetSSC( 1, 0, 3 );
#endif
	gpHalDispSetDevType(HAL_DISP_DEV_LCD);

	// set lcd source : XTAL(27MHZ)
	gpHalClkLcdSetSrc(HAL_LCD_CLK_SPLL);
	// Set lcd clock rate, must enable before lcd clk on
	gp_clk_set_rate("clk_lcd", gPanelInfo.workFreq, &real_freq, NULL);	

	gpHalDispSetClkType(gPanelInfo.clkType);

#ifdef CONFIG_PM
	gp_enable_clock((int*)"READLTIME_ABT", 1);
#endif
	gpHalLcdClkEnable(1);		

	/* Panel Init */
	gpHalDispSetRes(gPanelInfo.width, gPanelInfo.height);
	gpHalDispSetLcdVsync(gPanelInfo.vsync);
	gpHalDispSetLcdHsync(gPanelInfo.hsync);
	gpHalDispSetPanelFormat(gPanelInfo.format, gPanelInfo.type, gPanelInfo.dataSeqEven, gPanelInfo.dataSeqOdd);
	gpHalDispSetClkPolarity(gPanelInfo.clkPolatiry);
	
	lcd_power->set_panelpowerOn0(1);		/* Power on VCC, VGL, AVDD, VGH */

	ctx.pin_index = MK_GPIO_INDEX( PCLK_channel, PCLK_func, PCLK_gid, PCLK_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, PCLK_func );
	gp_gpio_set_output( handle, 0, 0 );
	gp_gpio_release( handle );

	ctx.pin_index = MK_GPIO_INDEX( B_DATA_channel, B_DATA_func, B_DATA_gid, B_DATA_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, B_DATA_func );	
	gp_gpio_release( handle );

	ctx.pin_index = MK_GPIO_INDEX( G_DATA_channel, G_DATA_func, G_DATA_gid, G_DATA_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, G_DATA_func );	
	gp_gpio_release( handle );
	
	ctx.pin_index = MK_GPIO_INDEX( R_DATA_channel, R_DATA_func, R_DATA_gid, R_DATA_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, R_DATA_func );	
	gp_gpio_release( handle );
	
	ctx.pin_index = MK_GPIO_INDEX( DEM_channel, DEM_GPIO, DEM_gid, DEM_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, DEM_GPIO );
	gp_gpio_set_output( handle, 0, 0 );
	gp_gpio_release( handle );	

	lcd_power->set_panelpowerOn1(1);		/* Set LCD RESET to high */
	
	/* Set dither */
	disp_set_dither_type(gPanelInfo.ditherType);
	disp_set_dither_param(gPanelInfo.pDitherParam);
	disp_set_dither_enable(0);

	/* Set color matrix */
	disp_set_color_matrix(gPanelInfo.pColorMatrix);

	/* Set gamma table */
	disp_set_gamma_enable(0);
	disp_set_gamma_table(SP_DISP_GAMMA_R, (uint8_t*) gPanelInfo.pGammaTable[SP_DISP_GAMMA_R]);
	disp_set_gamma_table(SP_DISP_GAMMA_G, (uint8_t*) gPanelInfo.pGammaTable[SP_DISP_GAMMA_G]);
	disp_set_gamma_table(SP_DISP_GAMMA_B, (uint8_t*) gPanelInfo.pGammaTable[SP_DISP_GAMMA_B]);
	disp_set_gamma_enable(0);

// BL, B_SD_CARD2 to high
//	lcd_power->set_backlight(1);

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
	uint8_t	white_gamma[256];
	uint16_t	i;
	gp_board_panel_t *lcd_power;
	int handle;
	gpio_content_t ctx;

	printk("[%s:%d]\n", __FUNCTION__, __LINE__);

	lcd_power = gp_board_get_config("panel", gp_board_panel_t);

	/* Set dither */
	//disp_set_dither_type(gPanelInfo.ditherType);
	//disp_set_dither_param(gPanelInfo.pDitherParam);
	//disp_set_dither_enable(1);

	/* Set color matrix */
	//disp_set_color_matrix(gPanelInfo.pColorMatrix);

	/* Set white gamma table */
	for ( i = 0 ; i < 256 ; i++ ) {
		white_gamma[i] = 0xff;
	}
	disp_set_gamma_enable(0);
	disp_set_gamma_table(SP_DISP_GAMMA_R, (uint8_t*) white_gamma);
	disp_set_gamma_table(SP_DISP_GAMMA_G, (uint8_t*) white_gamma);
	disp_set_gamma_table(SP_DISP_GAMMA_B, (uint8_t*) white_gamma);
	disp_set_gamma_enable(1);

	//msleep(60);
	disp_wait_frame_end();
	disp_wait_frame_end();
	//LCD_RST to Low
	lcd_power->set_panelpowerOn1(0);

// Disable LCD bus
	ctx.pin_index = MK_GPIO_INDEX( PCLK_channel, PCLK_GPIO, PCLK_gid, PCLK_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, PCLK_GPIO );	
	gp_gpio_release( handle );

	for ( i = 0 ; i <= 10 ; i++ ) {
		ctx.pin_index = MK_GPIO_INDEX( PCLK_channel, PCLK_GPIO, PCLK_gid, i );
		handle = gp_gpio_request( ctx.pin_index, NULL );
		gp_gpio_set_output( handle, 0, 0 );
		gp_gpio_release( handle );	
	}

	ctx.pin_index = MK_GPIO_INDEX( B_DATA_channel, B_DATA_GPIO, B_DATA_gid, B_DATA_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, B_DATA_GPIO );	
	gp_gpio_release( handle );

	for ( i = 0 ; i <= 7 ; i++ ) {
		ctx.pin_index = MK_GPIO_INDEX( B_DATA_channel, B_DATA_GPIO, B_DATA_gid, i );
		handle = gp_gpio_request( ctx.pin_index, NULL );
		gp_gpio_set_output( handle, 0, 0 );
		gp_gpio_release( handle );	
	}

	ctx.pin_index = MK_GPIO_INDEX( G_DATA_channel, G_DATA_GPIO, G_DATA_gid, G_DATA_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, G_DATA_GPIO );	
	gp_gpio_release( handle );

	for ( i = 8 ; i <= 11 ; i++ ) {
		ctx.pin_index = MK_GPIO_INDEX( G_DATA_channel, G_DATA_GPIO, G_DATA_gid, i );
		handle = gp_gpio_request( ctx.pin_index, NULL );
		gp_gpio_set_output( handle, 0, 0 );
		gp_gpio_release( handle );
	}
	
	ctx.pin_index = MK_GPIO_INDEX( R_DATA_channel, R_DATA_GPIO, R_DATA_gid, R_DATA_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, R_DATA_GPIO );	
	gp_gpio_release( handle );
	
	for ( i = 12 ; i <= 15 ; i++ ) {
		ctx.pin_index = MK_GPIO_INDEX( R_DATA_channel, R_DATA_GPIO, R_DATA_gid, i );
		handle = gp_gpio_request( ctx.pin_index, NULL );
		gp_gpio_set_output( handle, 0, 0 );
		gp_gpio_release( handle );
	}

	//VGH, AVDD, VGL, VCC to low
	lcd_power->set_panelpowerOn0(0);

	/* Disable lcd clock */
	gpHalLcdClkEnable(0);	
#ifdef CONFIG_PM
	gp_enable_clock((int*)"READLTIME_ABT", 0);
#endif

	return 0;
}

static int32_t
lcd_resume(
	void
)
{
	int32_t real_freq = 0;
	gp_board_panel_t *lcd_power;
	int handle;
	gpio_content_t ctx;

	printk("[%s:%d]\n", __FUNCTION__, __LINE__);

	lcd_power = gp_board_get_config("panel", gp_board_panel_t);

	gpHalDispSetDevType(HAL_DISP_DEV_LCD);

	// set lcd source : XTAL(27MHZ)
	gpHalClkLcdSetSrc(HAL_LCD_CLK_XTAL);
	// Set lcd clock rate, must enable before lcd clk on
	gp_clk_set_rate("clk_lcd", gPanelInfo.workFreq, &real_freq, NULL);	

	gpHalDispSetClkType(gPanelInfo.clkType);

#ifdef CONFIG_PM
	gp_enable_clock((int*)"READLTIME_ABT", 1);
#endif
	gpHalLcdClkEnable(1);		

	gpHalDispSetRes(gPanelInfo.width, gPanelInfo.height);
	gpHalDispSetLcdVsync(gPanelInfo.vsync);
	gpHalDispSetLcdHsync(gPanelInfo.hsync);
	gpHalDispSetPanelFormat(gPanelInfo.format, gPanelInfo.type, gPanelInfo.dataSeqEven, gPanelInfo.dataSeqOdd);
	gpHalDispSetClkPolarity(gPanelInfo.clkPolatiry);

	lcd_power->set_panelpowerOn0(1);		/* Power on VCC, VGL, AVDD, VGH */

	ctx.pin_index = MK_GPIO_INDEX( PCLK_channel, PCLK_func, PCLK_gid, PCLK_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, PCLK_func );	
	gp_gpio_release( handle );

	ctx.pin_index = MK_GPIO_INDEX( B_DATA_channel, B_DATA_func, B_DATA_gid, B_DATA_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, B_DATA_func );	
	gp_gpio_release( handle );

	ctx.pin_index = MK_GPIO_INDEX( G_DATA_channel, G_DATA_func, G_DATA_gid, G_DATA_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, G_DATA_func );	
	gp_gpio_release( handle );
	
	ctx.pin_index = MK_GPIO_INDEX( R_DATA_channel, R_DATA_func, R_DATA_gid, R_DATA_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, R_DATA_func );	
	gp_gpio_release( handle );

	lcd_power->set_panelpowerOn1(1);		/* Set LCD RESET to high */

	/* Set dither */
	disp_set_dither_type(gPanelInfo.ditherType);
	disp_set_dither_param(gPanelInfo.pDitherParam);
	disp_set_dither_enable(1);

	/* Set color matrix */
	disp_set_color_matrix(gPanelInfo.pColorMatrix);

	/* Set gamma table */
	disp_set_gamma_enable(0);
	disp_set_gamma_table(SP_DISP_GAMMA_R, (uint8_t*) gPanelInfo.pGammaTable[SP_DISP_GAMMA_R]);
	disp_set_gamma_table(SP_DISP_GAMMA_G, (uint8_t*) gPanelInfo.pGammaTable[SP_DISP_GAMMA_G]);
	disp_set_gamma_table(SP_DISP_GAMMA_B, (uint8_t*) gPanelInfo.pGammaTable[SP_DISP_GAMMA_B]);
	disp_set_gamma_enable(1);

	gpHalDispSetPriDmaType(gPanelInfo.dmaType);
	gpHalDispSetOsdDmaType(0, gPanelInfo.dmaType);
	gpHalDispSetOsdDmaType(1, gPanelInfo.dmaType);

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

