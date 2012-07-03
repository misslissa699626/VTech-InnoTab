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
 * @file lcd_hannstar_HSD070IDW1.h
 * @brief The lcd driver of Hannstar HSD070IDW1
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
#include <mach/gp_display.h>
#include <mach/clock_mgr/gp_clock.h>
#include <mach/gp_board.h>
#include <mach/gp_gpio.h>
#include <mach/hal/hal_gpio.h>
#include <mach/module.h>

MODULE_LICENSE_GP;


// PCLK
#define	PCLK_FREQ			30000000
#define	PCLK_POL			1
// Display Size
#define	DISP_SIZE_W			800
#define	DISP_SIZE_H			480
// H-Sync
#define	HD_PULSE_W			48
#define	HD_PULSE_POL		0
#define	HD_FRONT_P			40
#define	HD_BACK_P			86
// V-Sync
#define	VD_PULSE_W			3
#define	VD_PULSE_POL		0
#define	VD_FRONT_P			13
#define	VD_BACK_P			31
// Panel Size unit:mm
#define	PANEL_SIZE_W		1920		/* int61_t */
#define	PANEL_SIZE_H		1083		/* int61_t */
// GPIO Setting : PCLK, V-sync, H-sync
#define	PCLK_channel		GPIO_CHANNEL_3
#define	PCLK_func			2
#define	PCLK_gid			6
#define	PCLK_pin			3
#define	PCLK_GPIO			0
// GPIO Setting : DE
#define	DE_channel			GPIO_CHANNEL_0
#define	DE_func				2
#define	DE_gid				7
#define	DE_pin				15
#define	DE_GPIO				0
// GPIO Setting : B-Data
#define	B_DATA_channel		GPIO_CHANNEL_1
#define	B_DATA_func			2
#define	B_DATA_gid			8
#define	B_DATA_pin			0
#define	B_DATA_GPIO			0
// GPIO Setting : G-Data
#define	G_DATA_channel		GPIO_CHANNEL_2
#define	G_DATA_func			2
#define	G_DATA_gid			9
#define	G_DATA_pin			8
#define	G_DATA_GPIO			0
// GPIO Setting : R-Data
#define	R_DATA_channel		GPIO_CHANNEL_2
#define	R_DATA_func			2
#define	R_DATA_gid			10
#define	R_DATA_pin			12
#define	R_DATA_GPIO			0

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
static const char gPanelName[] = "LCD_Hannstar_HSD070IDW1";

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
	.a01 = 0x0000,
	.a02 = 0x0000,
	.a10 = 0x0000,
	.a11 = 0x0100,
	.a12 = 0x0000,
	.a20 = 0x0000,
	.a21 = 0x0000,
	.a22 = 0x0100,
	.b0 = 0x0000,
	.b1 = 0x0000,
	.b2 = 0x0000,
};

static const uint8_t gammaR[256] = {
0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x04, 0x07, 0x0a, 
0x0d, 0x10, 0x11, 0x12, 0x13, 0x14, 0x16, 0x17, 0x18, 0x19, 0x1b, 0x1c, 0x1d, 0x1f, 0x20, 0x21, 
0x22, 0x22, 0x23, 0x24, 0x24, 0x25, 0x26, 0x27, 0x28, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 
0x2f, 0x30, 0x31, 0x31, 0x32, 0x32, 0x33, 0x34, 0x34, 0x35, 0x36, 0x36, 0x37, 0x38, 0x38, 0x39, 
0x3a, 0x3b, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x3f, 0x40, 0x41, 0x41, 0x42, 0x43, 0x43, 0x44, 0x44, 
0x45, 0x46, 0x46, 0x47, 0x48, 0x48, 0x49, 0x4a, 0x4a, 0x4b, 0x4c, 0x4d, 0x4d, 0x4e, 0x4f, 0x50, 
0x50, 0x51, 0x52, 0x53, 0x53, 0x54, 0x55, 0x56, 0x57, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5c, 
0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 
0x6d, 0x6e, 0x70, 0x71, 0x72, 0x73, 0x75, 0x76, 0x78, 0x79, 0x7a, 0x7c, 0x7d, 0x7e, 0x80, 0x81, 
0x82, 0x84, 0x85, 0x86, 0x87, 0x89, 0x8a, 0x8b, 0x8c, 0x8e, 0x8f, 0x90, 0x91, 0x93, 0x94, 0x95, 
0x96, 0x97, 0x99, 0x9a, 0x9b, 0x9c, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa4, 0xa5, 0xa6, 0xa7, 0xa9, 
0xaa, 0xab, 0xac, 0xae, 0xaf, 0xb0, 0xb1, 0xb3, 0xb4, 0xb5, 0xb6, 0xb8, 0xb9, 0xba, 0xbc, 0xbd, 
0xbe, 0xc0, 0xc1, 0xc2, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 
0xce, 0xcf, 0xd0, 0xd1, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 
0xdd, 0xde, 0xdf, 0xe1, 0xe2, 0xe4, 0xe5, 0xe7, 0xe8, 0xea, 0xeb, 0xed, 0xef, 0xf1, 0xf8, 0xff, 
};

static const uint8_t gammaG[256] = {
0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x05, 0x07, 0x09, 0x0b, 0x0d, 0x10, 0x11, 0x11, 0x12, 
0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 
0x22, 0x23, 0x23, 0x24, 0x25, 0x25, 0x26, 0x27, 0x28, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2c, 0x2d, 
0x2e, 0x2f, 0x30, 0x31, 0x31, 0x32, 0x32, 0x33, 0x33, 0x34, 0x35, 0x35, 0x36, 0x36, 0x37, 0x38, 
0x38, 0x39, 0x3a, 0x3a, 0x3b, 0x3c, 0x3d, 0x3d, 0x3e, 0x3f, 0x40, 0x40, 0x41, 0x41, 0x42, 0x42, 
0x43, 0x44, 0x44, 0x45, 0x45, 0x46, 0x47, 0x47, 0x48, 0x49, 0x49, 0x4a, 0x4a, 0x4b, 0x4c, 0x4c, 
0x4d, 0x4e, 0x4f, 0x4f, 0x50, 0x51, 0x51, 0x52, 0x53, 0x54, 0x54, 0x55, 0x56, 0x57, 0x57, 0x58, 
0x59, 0x5a, 0x5b, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 
0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x74, 0x75, 0x76, 0x78, 0x79, 
0x7a, 0x7c, 0x7d, 0x7e, 0x80, 0x81, 0x82, 0x83, 0x85, 0x86, 0x87, 0x88, 0x8a, 0x8b, 0x8c, 0x8d, 
0x8f, 0x90, 0x91, 0x92, 0x93, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9b, 0x9c, 0x9d, 0x9e, 0xa0, 0xa1, 
0xa2, 0xa3, 0xa5, 0xa6, 0xa7, 0xa8, 0xaa, 0xab, 0xac, 0xad, 0xaf, 0xb0, 0xb1, 0xb3, 0xb4, 0xb5, 
0xb6, 0xb8, 0xb9, 0xba, 0xbc, 0xbd, 0xbe, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 
0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd8, 0xd9, 
0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xe0, 0xe1, 0xe3, 0xe5, 0xe7, 0xe9, 0xeb, 0xed, 0xef, 0xf6, 0xff,
};

static const uint8_t gammaB[256] = {
0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x04, 0x05, 0x07, 0x08, 0x0a, 0x0c, 0x0e, 0x0f, 
0x10, 0x11, 0x12, 0x12, 0x13, 0x14, 0x14, 0x15, 0x16, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1b, 
0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23, 0x24, 0x24, 0x25, 0x25, 0x26, 
0x26, 0x27, 0x28, 0x28, 0x29, 0x2a, 0x2a, 0x2b, 0x2c, 0x2c, 0x2d, 0x2e, 0x2e, 0x2f, 0x30, 0x30, 
0x31, 0x31, 0x32, 0x32, 0x33, 0x33, 0x34, 0x34, 0x35, 0x35, 0x36, 0x37, 0x37, 0x38, 0x38, 0x39, 
0x39, 0x3a, 0x3b, 0x3b, 0x3c, 0x3c, 0x3d, 0x3e, 0x3e, 0x3f, 0x40, 0x40, 0x41, 0x41, 0x42, 0x42, 
0x43, 0x43, 0x44, 0x44, 0x45, 0x46, 0x46, 0x47, 0x47, 0x48, 0x48, 0x49, 0x4a, 0x4a, 0x4b, 0x4b, 
0x4c, 0x4d, 0x4d, 0x4e, 0x4e, 0x4f, 0x50, 0x50, 0x51, 0x52, 0x53, 0x53, 0x54, 0x55, 0x55, 0x56, 
0x57, 0x58, 0x58, 0x59, 0x5a, 0x5b, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x5f, 0x60, 0x61, 0x62, 0x63, 
0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x72, 0x73, 0x74, 
0x76, 0x77, 0x79, 0x7a, 0x7b, 0x7d, 0x7e, 0x80, 0x81, 0x82, 0x83, 0x85, 0x86, 0x87, 0x88, 0x8a, 
0x8b, 0x8c, 0x8d, 0x8f, 0x90, 0x91, 0x93, 0x94, 0x95, 0x96, 0x98, 0x99, 0x9a, 0x9c, 0x9d, 0x9e, 
0xa0, 0xa1, 0xa2, 0xa4, 0xa5, 0xa6, 0xa8, 0xa9, 0xaa, 0xac, 0xad, 0xaf, 0xb0, 0xb2, 0xb3, 0xb5, 
0xb6, 0xb8, 0xb9, 0xbb, 0xbc, 0xbe, 0xbf, 0xc1, 0xc2, 0xc3, 0xc5, 0xc6, 0xc7, 0xc9, 0xca, 0xcb, 
0xcd, 0xce, 0xcf, 0xd1, 0xd3, 0xd5, 0xd6, 0xd8, 0xda, 0xdc, 0xde, 0xe0, 0xe4, 0xe8, 0xed, 0xff, 
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

	lcd_power = gp_board_get_config("panel", gp_board_panel_t);
#if 0
	pvuiData = &(SCUA_DUMMY2);		/* SSC setting */
	*pvuiData = 0x0b;
#endif
	printk("[%s:%d]\n", __FUNCTION__, __LINE__);

	gpHalDispSetSSC( 1, 0, 3 );

	gpHalDispSetDevType(HAL_DISP_DEV_LCD);

	// set lcd source : SPLL
	gpHalClkLcdSetSrc(HAL_LCD_CLK_SPLL);
	// Set lcd clock rate, must enable before lcd clk on
	gp_clk_set_rate("clk_lcd", gPanelInfo.workFreq, &real_freq, NULL);	// must enable before lcd clk on

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
	gp_gpio_release( handle );

	ctx.pin_index = MK_GPIO_INDEX( DE_channel, DE_func, DE_gid, DE_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, DE_func );
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

// BL, B_SD_CARD2 to high
	//lcd_power->set_backlight(1);

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

	//LCD_RST to Low
	lcd_power->set_panelpowerOn1(0);

// Disable LCD bus
	ctx.pin_index = MK_GPIO_INDEX( PCLK_channel, PCLK_GPIO, PCLK_gid, PCLK_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, PCLK_func );	
	gp_gpio_release( handle );

	ctx.pin_index = MK_GPIO_INDEX( DE_channel, DE_GPIO, DE_gid, DE_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, DE_func );	
	gp_gpio_release( handle );

	ctx.pin_index = MK_GPIO_INDEX( B_DATA_channel, B_DATA_GPIO, B_DATA_gid, B_DATA_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, B_DATA_func );	
	gp_gpio_release( handle );

	ctx.pin_index = MK_GPIO_INDEX( G_DATA_channel, G_DATA_GPIO, G_DATA_gid, G_DATA_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, G_DATA_func );	
	gp_gpio_release( handle );
	
	ctx.pin_index = MK_GPIO_INDEX( R_DATA_channel, R_DATA_GPIO, R_DATA_gid, R_DATA_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, R_DATA_func );	
	gp_gpio_release( handle );

	/* Disable lcd clock */
	gpHalLcdClkEnable(0);
#ifdef CONFIG_PM
	gp_enable_clock((int*)"READLTIME_ABT", 0);
#endif

	//VGH, AVDD, VGL, VCC to low
	lcd_power->set_panelpowerOn0(0);

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

	// set lcd source : SPLL
	gpHalClkLcdSetSrc(HAL_LCD_CLK_SPLL);
	// Set lcd clock rate, must enable before lcd clk on
	gp_clk_set_rate("clk_lcd", gPanelInfo.workFreq, &real_freq, NULL);	// must enable before lcd clk on

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

	ctx.pin_index = MK_GPIO_INDEX( DE_channel, DE_func, DE_gid, DE_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, DE_func );	
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

