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
#include <linux/delay.h>
#include <asm/system.h> /* cli(), *_flags */
#include <asm/uaccess.h> /* copy_from/to_user */

#include <mach/hal/sysregs.h>
#include <mach/panel_cfg.h>
#include <mach/hardware.h>
#include <mach/clock_mgr/gp_clock.h>
#include <mach/hal/hal_clock.h>
#include <mach/hal/hal_disp.h>
#include <mach/gp_display.h>
#include <mach/spmp_gpio.h>
#include <mach/regs-gpio.h>
#include <mach/module.h>
MODULE_LICENSE_GP;


void lcm_SPFD5420A_setup(void);

typedef struct
{
	char *name;
	uint32_t width;
	uint32_t height;
	uint32_t format;
	uint32_t type;
	uint32_t dataSeqEven;
	uint32_t dataSeqOdd;
	uint32_t interface;
	uint32_t mode;
	uint32_t dataSelect;
	gpHalDispLcmTiming_t acTiming;
	gp_size_t panelSize;
	uint32_t ditherType;
	gp_disp_ditherparam_t *pDitherParam;
	gp_disp_colormatrix_t *pColorMatrix;
	uint8_t *pGammaTable[3];
	void (*lcmSetup)(void);
} disp_lcmInfo;

static const char gPanelName[] = "LCM_SPFD5420A";

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
	.a00 = 1,
	.a01 = 0,
	.a02 = 0,
	.a10 = 0,
	.a11 = 1,
	.a12 = 0,
	.a20 = 0,
	.a21 = 0,
	.a22 = 1,
	.b0 = 0,
	.b1 = 0,
	.b2 = 0,
};

static const uint8_t gammaR[256] = {1};
static const uint8_t gammaG[256] = {1};
static const uint8_t gammaB[256] = {1};

static disp_lcmInfo gPanelInfo = {
	.name = (char*) &gPanelName,
	.width = 240,
	.height = 400,
	.format = HAL_DISP_OUTPUT_FMT_RGB,
	.type = HAL_DISP_OUTPUT_TYPE_RGB565,
	.dataSeqEven = 0,
	.dataSeqOdd = 0,
	.interface = HAL_DISP_LCM_16BIT,
	.mode = HAL_DISP_LCM_8080,
	.dataSelect = 1,
	.acTiming = {
		.addrSetup = 1,
		.addrHold = 1,
		.csSetup = 1,
		.csHold = 1,
		.cycLength = 1,
	},
#if 0
	.panelSize = {
		.width = 0,
		.height = 0,
	},
#endif
	.ditherType = HAL_DISP_DITHER_FIXED,
	.pDitherParam = (gp_disp_ditherparam_t *) &gDitherParam,
	.pColorMatrix = (gp_disp_colormatrix_t *) &gColorMatrix,
	.pGammaTable = {
		(uint8_t*) &gammaR,
		(uint8_t*) &gammaG,
		(uint8_t*) &gammaB,
	},
	.lcmSetup = lcm_SPFD5420A_setup,
};

void
lcm_SPFD5420A_setup(
	void
)
{
	#define Command(gw_cmd) 		*pcmd = gw_cmd
	#define Data(gw_data)			*pdata = gw_data

	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	volatile unsigned short *pcmd = (volatile unsigned short *) &pdispReg->dispLcmPgmIo00;
	volatile unsigned short *pdata = (volatile unsigned short *) &pdispReg->dispLcmPgmIo30;

	Command(0x0606);   Data(0x0000);
	mdelay(10);
	Command(0x0007);   Data(0x0001);
	mdelay(10);
	Command(0x0110);   Data(0x0001);
	mdelay(10);
	Command(0x0100);   Data(0x17B0);
	Command(0x0101);   Data(0x0147);
	Command(0x0102);   Data(0x019D);
	Command(0x0103);   Data(0x3600);
	Command(0x0281);   Data(0x0010);
	mdelay(10);
	Command(0x0102);   Data(0x01BD);
	mdelay(10);
	Command(0x0000);   Data(0x0000); 
	Command(0x0001);   Data(0x0000); 
	Command(0x0002);   Data(0x0100); 
	Command(0x0003);   Data(0x10A0); 
	Command(0x0008);   Data(0x0808); 
	Command(0x0009);   Data(0x0001);
	Command(0x000B);   Data(0x0010);
	Command(0x000C);   Data(0x0000); 
	Command(0x000F);   Data(0x0000); 
	Command(0x0007);   Data(0x0001); 

	//--------Panel interface control 1~6 --------//
	Command(0x0010);   Data(0x0018);
	Command(0x0011);   Data(0x0202);
	Command(0x0012);   Data(0x0300);
	Command(0x0020);   Data(0x021E);
	Command(0x0021);   Data(0x0202);
	Command(0x0022);   Data(0x0100);
	Command(0x0090);   Data(0x8000); 

	//---------- Power control 1~6 ---------------//
	Command(0x0100);   Data(0x14B0);
	Command(0x0101);   Data(0x0147);
	Command(0x0102);   Data(0x01BD);
	Command(0x0103);   Data(0x3000);
	Command(0x0107);   Data(0x0000);
	Command(0x0110);   Data(0x0001);

	//---------- GRAM Address  ---------------//
	Command(0x0200);   Data(0x0000); 
	Command(0x0201);   Data(0x0000);
	
	Command(0x0210);   Data(0x0000); 
	Command(0x0211);   Data(0x00EF); 
	Command(0x0212);   Data(0x0000); 
	Command(0x0213);   Data(0x018F); 

	Command(0x0280);   Data(0x0000);
	Command(0x0281);   Data(0x0002);
	Command(0x0282);   Data(0x0000); 
	//-------------- Gamma 2.2 control -----------//
#if 1
	Command(0x0300);   Data(0x0101);
	Command(0x0301);   Data(0x0024);
	Command(0x0302);   Data(0x1326);
	Command(0x0303);   Data(0x2313);
	Command(0x0304);   Data(0x2400);
	Command(0x0305);   Data(0x0100);
	Command(0x0306);   Data(0x1704);
	Command(0x0307);   Data(0x0417);
	Command(0x0308);   Data(0x0007);
	Command(0x0309);   Data(0x0105);
	Command(0x030A);   Data(0x0F05);
	Command(0x030B);   Data(0x0F01);
	Command(0x030C);   Data(0x010F);
	Command(0x030D);   Data(0x050F);
	Command(0x030E);   Data(0x0501);
	Command(0x030F);   Data(0x0700);
#else
	Command(0x0300);  Data(0x0101);
	Command(0x0301);  Data(0x0129);
	Command(0x0302);  Data(0x132D);
	Command(0x0303);  Data(0x2D13);
	Command(0x0304);  Data(0x2910);
	Command(0x0305);  Data(0x0101);
	Command(0x0306);  Data(0x0B07);
	Command(0x0307);  Data(0x070B);
	Command(0x0308);  Data(0x0007);
	Command(0x0309);  Data(0x0104);
	Command(0x030A);  Data(0x0F05);
	Command(0x030B);  Data(0x0F01);
	Command(0x030C);  Data(0x010F);
	Command(0x030D);  Data(0x050F);
	Command(0x030E);  Data(0x0401);
	Command(0x030F);  Data(0x0700);
#endif

	Command(0x0400);   Data(0x3100);
	Command(0x0401);   Data(0x0001);
	Command(0x0404);   Data(0x0000);
	//-------------- Partial display ------------//
	Command(0x0500);   Data(0x0000);
	Command(0x0501);   Data(0x0000);
	Command(0x0502);   Data(0x0000);
	Command(0x0503);   Data(0x0000);
	Command(0x0504);   Data(0x0000);
	Command(0x0505);   Data(0x0000);

	Command(0x0600);   Data(0x0000); 
	Command(0x0606);   Data(0x0000); 
	Command(0x06F0);   Data(0x0000); 
	//--------------- Orise mode ----------------//
	Command(0x07F0);   Data(0x5420);
	Command(0x07DE);   Data(0x0000);
	Command(0x07F2);   Data(0x00DF);
	Command(0x07F3);   Data(0x0610);
	Command(0x07F4);   Data(0x0022);
	Command(0x07F5);   Data(0x0001);
	Command(0x07F0);   Data(0x0000);
	Command(0x0007);   Data(0x0173);

	Command(0x0202);
}

#define		DISP_TYPE_MSK						0x00030000
#define		DISP_TYPE_RGB565					0x00000000
#define		DISP_TYPE_RGB666					0x00010000
#define		DISP_TYPE_LCM						0x00020000
#define		DISP_TYPE_YPbPr						0x00030000

void scu_set_disp_clk(uint32_t aDivFactor)
{
    uint32_t val = 0;
	
	val = SCUA_LCD_CLK_CFG;
	val &= ~(0x100);  //LCD_CLK_CNT_EN = 0x100
	SCUA_LCD_CLK_CFG = val;
	val &= 0xFFFFFF00;
	val |= (aDivFactor-1);
	SCUA_LCD_CLK_CFG = val;	
	val |= 0x100;
	SCUA_LCD_CLK_CFG = val;	
}

void scu_gpio_init(uint32_t aDivFactor)
{
    uint32_t val = 0;
    val = SCUB_B_PERI_CLKEN;
    val |= SCU_B_PERI_GPIO;
    SCUB_B_PERI_CLKEN = val;
}

void scu_lcd_clk_on(void)
{
#ifdef CONFIG_PM
	gp_enable_clock((int*)"READLTIME_ABT", 1);
	gpHalScuClkEnable(SCU_A_PERI_LCD_CTRL, SCU_A, 1);
	/*Line buffer clock was opened by self module*/
#else
	uint32_t val = 0;
	val = SCUA_A_PERI_CLKEN;
	val |= (SCU_A_PERI_LCD_CTRL |SCU_A_PERI_LINEBUFFER |SCU_A_PERI_REALTIME_ABT);
	SCUA_A_PERI_CLKEN = val;
#endif
   	//scu_gpio_init(0);
}

void scu_lcm_enable(uint32_t aDivFactor)
{
	uint32_t val = 0;

	scu_set_disp_clk( aDivFactor);
	val = SCUA_LCD_TYPE_SEL;
	val &= ~(DISP_TYPE_MSK);
	val |= DISP_TYPE_LCM; /* enable LCM mode. */
	SCUA_LCD_TYPE_SEL = val;

	scu_lcd_clk_on();
}


int32_t lcm_init(
	void
)
{
	printk("[%s:%d]\n", __FUNCTION__, __LINE__);

	gpHalDispSetDevType(HAL_DISP_DEV_LCM);

	/* Enable lcm clock */
	SCUA_LCD_CLK_CFG |= 0x00010000;
	scu_lcm_enable(2);

	gpHalDispSetRes(gPanelInfo.width, gPanelInfo.height);
	gpHalDispSetPanelFormat(gPanelInfo.format, gPanelInfo.type, gPanelInfo.dataSeqEven, gPanelInfo.dataSeqOdd);
	gpHalDispSetLcmInterface(gPanelInfo.interface);
	gpHalDispSetLcmMode(gPanelInfo.mode);
	gpHalDispSetLcmAcTiming(gPanelInfo.acTiming);
	gpHalDispSetLcmDataSelect(gPanelInfo.dataSelect);

	scu_change_pin_grp(6, 2);
	scu_change_pin_grp(7, 2);
	scu_change_pin_grp(8, 2);
	scu_change_pin_grp(9, 2);
	scu_change_pin_grp(10, 2);

	(gPanelInfo.lcmSetup)();
#if 0
	/* Set dither */
	disp_set_dither_type(gPanelInfo.ditherType);
	disp_set_dither_param(gPanelInfo.pDitherParam);
	disp_set_dither_enable(0);

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

/* access functions */
gp_disp_panel_ops_t lcm_fops = {
	.init = lcm_init,
	.suspend = NULL,
	.resume = NULL,
	.get_size = NULL,
};

static int32_t lcmPanel_init(void) {
  printk("[%s:%d]\n", __FUNCTION__, __LINE__);
  register_paneldev(SP_DISP_OUTPUT_LCM, gPanelInfo.name, &lcm_fops);
  return 0;
}

static void lcmPanel_exit(void) {
  printk("[%s:%d]\n", __FUNCTION__, __LINE__);
  unregister_paneldev(SP_DISP_OUTPUT_LCM, gPanelInfo.name);
}

module_init(lcmPanel_init);
module_exit(lcmPanel_exit);
