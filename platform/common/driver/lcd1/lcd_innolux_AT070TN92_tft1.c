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
 * @file lcd_innolux_AT070TN92.h
 * @brief The lcd driver of Innolux AT070TN92
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
#include <mach/hal/hal_disp1.h>
#include <mach/gp_display1.h>
#include <mach/clock_mgr/gp_clock.h>
#include <mach/gp_board.h>
#include <mach/gp_gpio.h>
#include <mach/hal/hal_gpio.h>
#include <mach/module.h>
#include <linux/delay.h> 	/* udelay/mdelay */
#include <mach/hal/regmap/reg_disp1.h>

MODULE_LICENSE_GP;

// PCLK
#define	PCLK_FREQ			27000000
#define	PCLK_POL			1
// Display Size
#define	DISP1_SIZE_W		800
#define	DISP1_SIZE_H		480
// H-Sync
#define	HS_PULSE_POL		0
#define    HS_PERIOD                        1056
#define	HS_START			46
#define	HS_END      			846 -1
// V-Sync
#define	VS_PULSE_POL		0
#define    VS_PERIOD                        525
#define	VS_START			23
#define	VS_END      			502 -1
// Panel Size unit:mm
#define	PANEL_SIZE_W		154		/* int61_t */
#define	PANEL_SIZE_H		86		/* int61_t */
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

/******************************`********************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct
{
	char *name;
	uint32_t width;
	uint32_t height;
	uint32_t workFreq;
	uint32_t clkPolatiry;
	uint32_t dataMode;
	uint32_t mode;
	gpHalDisp1LcdTiming_t vsync;
	gpHalDisp1LcdTiming_t hsync;
	gp_size_t panelSize;	/* Panel physical size in mm */
	uint32_t ditherType;
	uint8_t *pGammaTable[3];
} disp1_panelInfo;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static const char gPanelName[] = "LCD_Innolux_AT070TN92_tft1";


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

static disp1_panelInfo gPanelInfo = {
	.name = (char*) &gPanelName,
	.width = DISP1_SIZE_W,
	.height = DISP1_SIZE_H,
	.workFreq = PCLK_FREQ,
	.clkPolatiry = PCLK_POL,
	.dataMode = HAL_DISP1_DATA_MODE_888,
	.mode = HAL_DISP1_MODE_PARALLEL,
	.vsync = {
		.polarity = VS_PULSE_POL,
		.period = VS_PERIOD,
		.start = VS_START,
		.end = VS_END,
	},
	.hsync = {
		.polarity = HS_PULSE_POL,
		.period = HS_PERIOD,
		.start = HS_START,
		.end = HS_END,
	},
	.panelSize = {
		.width = PANEL_SIZE_W,		         /* int61_t */
		.height = PANEL_SIZE_H,		/* int61_t */
	},
	.ditherType = HAL_DISP1_DITHER_FIXED,
	.pGammaTable = {
		(uint8_t*) gammaR,
		(uint8_t*) gammaG,
		(uint8_t*) gammaB,
	},
};

/* access functions */
static gp_disp1_panel_ops_t lcd_fops = {
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
	disp1Reg_t *pdisp1Reg = (disp1Reg_t *)(LOGI_ADDR_DISP1_REG);
	gp_board_panel_t *lcd_power;
	
	//SCUA_SAR_GPIO_CTRL = 0;     //switch to TFT0 for GP12 real chip
	
	lcd_power = gp_board_get_config("panel", gp_board_panel_t);
#if 0
	pvuiData = &(SCUA_DUMMY2);		/* SSC setting */
	*pvuiData = 0x0b;
#endif
	printk("[%s:%d]\n", __FUNCTION__, __LINE__);	

	/* Panel Init */
	gpHalDisp1SetRes(gPanelInfo.width, gPanelInfo.height);
	/*	gpHalDispSetRes(gPanelInfo.width, gPanelInfo.height);
	gpHalDispSetLcdVsync(gPanelInfo.vsync);
	gpHalDispSetLcdHsync(gPanelInfo.hsync);
	gpHalDispSetPanelFormat(gPanelInfo.format, gPanelInfo.type, gPanelInfo.dataSeqEven, gPanelInfo.dataSeqOdd);
	gpHalDispSetClkPolarity(gPanelInfo.clkPolatiry);
*/	
        
	gpHalDisp1SetVerticalPeriod(gPanelInfo.vsync.period);
	gpHalDisp1SetVerticalStart(gPanelInfo.vsync.start);
	gpHalDisp1SetVerticalEnd(gPanelInfo.vsync.end);	

	gpHalDisp1SetHorizontalPeriod(gPanelInfo.hsync.period);
	gpHalDisp1SetHorizontalStart(gPanelInfo.hsync.start);
	gpHalDisp1SetHorizontalEnd(gPanelInfo.hsync.end);

	gpHalDisp1SetVSyncEnd(0);
	gpHalDisp1SetTsMisc(1);

	gpHalDisp1SetDataMode(gPanelInfo.dataMode);
	gpHalDisp1SetVSyncUnit(STATE_TRUE);
	gpHalDisp1SetDClkSel(HAL_DISP1_DCLK_SEL_90);
	gpHalDisp1SetSignalInv( HAL_DISP1_VSYNC_INV|HAL_DISP1_HSYNC_INV,(HAL_DISP1_ENABLE & HAL_DISP1_VSYNC_INV)|(HAL_DISP1_ENABLE & HAL_DISP1_HSYNC_INV) );
	gpHalDisp1SetMode(gPanelInfo.mode);
	gpHalDisp1SetClk(HAL_DISP1_CLK_DIVIDE_6);

	lcd_power->set_panelpowerOn0(1);		/* Power on VCC, VGL, AVDD, VGH */
	lcd_power->set_panelpowerOn1(1);		/* Set LCD RESET to high */
#if 0	
	/* Set dither */
	disp1_set_dither_type(gPanelInfo.ditherType);
	disp1_set_dither_enable(1);

	/* Set color matrix */
	disp1_set_color_matrix(gPanelInfo.pColorMatrix);

	/* Set gamma table */
	disp1_set_gamma_enable(0);
	disp1_set_gamma_table(SP_DISP1_GAMMA_R, (uint8_t*) gPanelInfo.pGammaTable[SP_DISP1_GAMMA_R]);
	disp1_set_gamma_table(SP_DISP1_GAMMA_G, (uint8_t*) gPanelInfo.pGammaTable[SP_DISP1_GAMMA_G]);
	disp1_set_gamma_table(SP_DISP1_GAMMA_B, (uint8_t*) gPanelInfo.pGammaTable[SP_DISP1_GAMMA_B]);
	disp1_set_gamma_enable(1);
#endif

	pdisp1Reg->disp1HS_Width = 20;
	pdisp1Reg->disp1VS_Width = 10;

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
	static int	trig_cnt = 0;

	printk("[%s:%d]\n", __FUNCTION__, __LINE__);

	if ( trig_cnt < 2 ) {
		trig_cnt ++;
		//printk("LCD Suspend Go : %d\n", trig_cnt );
		return 0;
	}
	//printk("LCD Suspend Go 3th\n");

	lcd_power = gp_board_get_config("panel", gp_board_panel_t);

	/* Set dither */
#if 0
	/* Set white gamma table */
	for ( i = 0 ; i < 256 ; i++ ) {
		white_gamma[i] = 0xff;
	}
	disp1_set_gamma_enable(0);
	disp1_set_gamma_table(SP_DISP1_GAMMA_R, (uint8_t*) white_gamma);
	disp1_set_gamma_table(SP_DISP1_GAMMA_G, (uint8_t*) white_gamma);
	disp1_set_gamma_table(SP_DISP1_GAMMA_B, (uint8_t*) white_gamma);
	disp1_set_gamma_enable(1);
#endif

	msleep(60);
// Disable LCD bus
	ctx.pin_index = MK_GPIO_INDEX( PCLK_channel, PCLK_GPIO, PCLK_gid, PCLK_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, PCLK_GPIO );	
	gp_gpio_release( handle );

	ctx.pin_index = MK_GPIO_INDEX( DE_channel, DE_GPIO, DE_gid, DE_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, DE_GPIO );	
	gp_gpio_release( handle );

	ctx.pin_index = MK_GPIO_INDEX( B_DATA_channel, B_DATA_GPIO, B_DATA_gid, B_DATA_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, B_DATA_GPIO );	
	gp_gpio_release( handle );

	ctx.pin_index = MK_GPIO_INDEX( G_DATA_channel, G_DATA_GPIO, G_DATA_gid, G_DATA_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, G_DATA_GPIO );	
	gp_gpio_release( handle );
	
	ctx.pin_index = MK_GPIO_INDEX( R_DATA_channel, R_DATA_GPIO, R_DATA_gid, R_DATA_pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );
	gp_gpio_set_function( handle, R_DATA_GPIO );	
	gp_gpio_release( handle );

	//LCD_RST to Low
	lcd_power->set_panelpowerOn1(0);

	//VGH, AVDD, VGL, VCC to low
	lcd_power->set_panelpowerOn0(0);

	return 0;
}

static int32_t
lcd_resume(
	void
)
{
	//gp_board_panel_t *lcd_power;

	printk("[%s:%d]\n", __FUNCTION__, __LINE__);

	//lcd_power = gp_board_get_config("panel", gp_board_panel_t);	

/*	gpHalDispSetRes(gPanelInfo.width, gPanelInfo.height);
	gpHalDispSetLcdVsync(gPanelInfo.vsync);
	gpHalDispSetLcdHsync(gPanelInfo.hsync);
	gpHalDispSetPanelFormat(gPanelInfo.format, gPanelInfo.type, gPanelInfo.dataSeqEven, gPanelInfo.dataSeqOdd);
	gpHalDispSetClkPolarity(gPanelInfo.clkPolatiry);
*/

        gpHalDisp1SetVerticalPeriod(gPanelInfo.vsync.period);
        gpHalDisp1SetVerticalStart(gPanelInfo.vsync.start);
        gpHalDisp1SetVerticalEnd(gPanelInfo.vsync.end);	

        gpHalDisp1SetHorizontalPeriod(gPanelInfo.hsync.period);
        gpHalDisp1SetHorizontalStart(gPanelInfo.hsync.start);
        gpHalDisp1SetHorizontalEnd(gPanelInfo.hsync.end);


        gpHalDisp1SetVSyncEnd(0);
        gpHalDisp1SetTsMisc(1);
        
        //gpHalDisp1SetDataMode(HAL_DISP1_DATA_MODE_666);
        gpHalDisp1SetDataMode(gPanelInfo.dataMode);
        gpHalDisp1SetVSyncUnit(STATE_TRUE);
        gpHalDisp1SetDClkSel(HAL_DISP1_DCLK_SEL_90);
        //gpHalDisp1SetDClkSel(HAL_DISP1_DCLK_SEL_0);
        gpHalDisp1SetSignalInv( HAL_DISP1_VSYNC_INV|HAL_DISP1_HSYNC_INV,(HAL_DISP1_ENABLE & HAL_DISP1_VSYNC_INV)|(HAL_DISP1_ENABLE & HAL_DISP1_HSYNC_INV) );
        gpHalDisp1SetMode(gPanelInfo.mode);
        gpHalDisp1SetClk(HAL_DISP1_CLK_DIVIDE_4);
        //gpHalDisp1SetClk(HAL_DISP1_CLK_DIVIDE_17);	
#if 0
	lcd_power->set_panelpowerOn0(1);		/* Power on VCC, VGL, AVDD, VGH */
	/* Set dither */
	disp1_set_dither_type(gPanelInfo.ditherType);
	disp1_set_dither_enable(1);

	/* Set gamma table */
	disp1_set_gamma_enable(0);
	disp1_set_gamma_table(SP_DISP1_GAMMA_R, (uint8_t*) gPanelInfo.pGammaTable[SP_DISP1_GAMMA_R]);
	disp1_set_gamma_table(SP_DISP1_GAMMA_G, (uint8_t*) gPanelInfo.pGammaTable[SP_DISP1_GAMMA_G]);
	disp1_set_gamma_table(SP_DISP1_GAMMA_B, (uint8_t*) gPanelInfo.pGammaTable[SP_DISP1_GAMMA_B]);
	disp1_set_gamma_enable(1);
#endif
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
  register_paneldev1(SP_DISP1_OUTPUT_LCD, gPanelInfo.name, &lcd_fops);
  return 0;
}

static void
panel_exit(
	void
)
{
  printk("[%s:%d]\n", __FUNCTION__, __LINE__);
  unregister_paneldev1(SP_DISP1_OUTPUT_LCD, gPanelInfo.name);
}

module_init(panel_init);
module_exit(panel_exit);

