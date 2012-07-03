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

void lcm_ILI9325_setup(void);

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

static const char gPanelName[] = "LCM_ILI9325";

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
	.width = 320,
	.height = 240,
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
	.lcmSetup = lcm_ILI9325_setup,
};

void
lcm_ILI9325_setup(
	void
)
{
	#define Command(gw_cmd) 		*pcmd = gw_cmd
	#define Data(gw_data)			*pdata = gw_data

	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	volatile unsigned short *pcmd = (volatile unsigned short *) &pdispReg->dispLcmPgmIo00;
	volatile unsigned short *pdata = (volatile unsigned short *) &pdispReg->dispLcmPgmIo10;//dispLcmPgmIo30;

	// Start Initial Sequence
	Command(0x00E3);	Data(0x3008); // Set internal timing
	Command(0x00E7);	Data(0x0012); // Set internal timing
	Command(0x00EF);	Data(0x1231); // Set internal timing
	Command(0x0001);	Data(0x0100); // set SS and SM bit
	Command(0x0002);	Data(0x0700); // set 1 line inversion
	Command(0x0003);	Data(0x1018); // set GRAM write direction and BGR=1.
	Command(0x0004);	Data(0x0000); // Resize register
	Command(0x0008);	Data(0x0207); // set the back porch and front porch
	Command(0x0009);	Data(0x0000); // set non-display area refresh cycle ISC[3:0]
	Command(0x000A);	Data(0x0000); // FMARK function
	Command(0x000C);	Data(0x0000); // RGB interface setting
	Command(0x000D);	Data(0x0000); // Frame marker Position
	Command(0x000F);	Data(0x0000); // RGB interface polarity
	// Power On sequence
	Command(0x0010);	Data(0x0000); // SAP, BT[3:0], AP, DSTB, SLP, STB
	Command(0x0011);	Data(0x0007); // DC1[2:0], DC0[2:0], VC[2:0]
	Command(0x0012);	Data(0x0000); // VREG1OUT voltage
	Command(0x0013);	Data(0x0000); // VDV[4:0] for VCOM amplitude
	mdelay(10); // Dis-charge capacitor power voltage
	Command(0x0010);	Data(0x1490); // SAP, BT[3:0], AP, DSTB, SLP, STB
	Command(0x0011);	Data(0x0227); // DC1[2:0], DC0[2:0], VC[2:0]
	mdelay(10); // Delay 50ms
	Command(0x0012);	Data(0x001D); // Internal reference voltage= Vci;
	mdelay(10); // Delay 50ms
	Command(0x0013);	Data(0x0800); // Set VDV[4:0] for VCOM amplitude
	Command(0x0029);	Data(0x0045); // Set VCM[5:0] for VCOMH
	Command(0x002B);	Data(0x000D); // Set Frame Rate
	mdelay(10); // Delay 50ms
	Command(0x0020);	Data(0x0000); // GRAM horizontal Address
	Command(0x0021);	Data(0x0000); // GRAM Vertical Address
	// Adjust the Gamma Curve
	Command(0x0030);	Data(0x0007);
	Command(0x0031);	Data(0x0707);
	Command(0x0032);	Data(0x0006);
	Command(0x0035);	Data(0x0704);
	Command(0x0036);	Data(0x1F04);
	Command(0x0037);	Data(0x0004);
	Command(0x0038);	Data(0x0000);
	Command(0x0039);	Data(0x0706);
	Command(0x003C);	Data(0x0701);
	Command(0x003D);	Data(0x000F);
	// Set GRAM area
	Command(0x0050);	Data(0x0000); // Horizontal GRAM Start Address
	Command(0x0051);	Data(0x00EF); // Horizontal GRAM End Address
	Command(0x0052);	Data(0x0000); // Vertical GRAM Start Address
	Command(0x0053);	Data(0x013F); // Vertical GRAM Start Address
	Command(0x0060);	Data(0xA700); // Gate Scan Line
	Command(0x0061);	Data(0x0001); // NDL,VLE, REV
	Command(0x006A);	Data(0x0000); // set scrolling line
	// Partial Display Control
	Command(0x0080);	Data(0x0000);
	Command(0x0081);	Data(0x0000);
	Command(0x0082);	Data(0x0000);
	Command(0x0083);	Data(0x0000);
	Command(0x0084);	Data(0x0000);
	Command(0x0085);	Data(0x0000);
	// Panel Control
	Command(0x0090);	Data(0x0010);
	Command(0x0092);	Data(0x0600);
	Command(0x0093);	Data(0x0003);
	Command(0x0095);	Data(0x0110);
	Command(0x0097);	Data(0x0000);
	Command(0x0098);	Data(0x0000);
	Command(0x0007);	Data(0x0133); // 262K color and display ON

	Command(0x0022);
}

#define		DISP_TYPE_MSK						0x00030000
#define		DISP_TYPE_RGB565					0x00000000
#define		DISP_TYPE_RGB666					0x00010000
#define		DISP_TYPE_LCM						0x00020000
#define		DISP_TYPE_YPbPr						0x00030000

#if 1
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
#endif

int32_t lcm_init(
	void
)
{
	int clk_ceva;

	printk("[%s:%d]\n", __FUNCTION__, __LINE__);

	gpHalDispSetDevType(HAL_DISP_DEV_LCM);

	/* Enable lcm clock */
	//SCUA_SAR_GPIO_CTRL &= ~0x02;	/* use tft0 */
	SCUA_LCD_CLK_CFG |= 0x00010000;		/*27MHz*/
	//SCUA_LCD_CLK_CFG |= 0x00000000;		/*SPLL*/
	gp_clk_get_rate((int*)"clk_ref_ceva", &clk_ceva);
	printk("clk_ceva = %d\n", clk_ceva);
	scu_lcm_enable(2);

	gpHalDispSetRes(gPanelInfo.width, gPanelInfo.height);
	gpHalDispSetPanelFormat(gPanelInfo.format, gPanelInfo.type, gPanelInfo.dataSeqEven, gPanelInfo.dataSeqOdd);
	gpHalDispSetLcmInterface(gPanelInfo.interface);
	gpHalDispSetLcmMode(gPanelInfo.mode);
	gpHalDispSetLcmAcTiming(gPanelInfo.acTiming);
	gpHalDispSetLcmDataSelect(gPanelInfo.dataSelect);
	gpHalDispSetLcmSeqSel(1);

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
