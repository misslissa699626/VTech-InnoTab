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

void lcm_FTP300D62N_setup(void);

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

static const char gPanelName[] = "LCM_FTP300D62N";

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
	.width = 400,//240,
	.height = 240,//400,
	.format = HAL_DISP_OUTPUT_FMT_RGB,
	.type = HAL_DISP_OUTPUT_TYPE_RGB565,
	.dataSeqEven = 0,
	.dataSeqOdd = 0,
	.interface = HAL_DISP_LCM_8BIT,
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
	.lcmSetup = lcm_FTP300D62N_setup,
};

void
lcm_FTP300D62N_setup(
	void
)
{
	#define Command(gw_cmd) 		*pcmd = gw_cmd
	#define Data(gw_data)			*pdata = gw_data

	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	volatile unsigned short *pcmd = (volatile unsigned short *) &pdispReg->dispLcmPgmIo00;
	volatile unsigned short *pdata = (volatile unsigned short *) &pdispReg->dispLcmPgmIo10;//dispLcmPgmIo30;

	Command(0x00);  Command(0x00);  Data(0x00);  Data(0x00);
	Command(0x00);  Command(0x00);  Data(0x00);  Data(0x00);
	Command(0x00);  Command(0x00);  Data(0x00);  Data(0x00);
	Command(0x00);  Command(0x00);  Data(0x00);  Data(0x00);
	mdelay(20);

	Command(0x04);  Command(0x00);  Data(0x62);  Data(0x00);
	mdelay(20);
	Command(0x00);  Command(0x08);  Data(0x08);  Data(0x08);
	Command(0x03);  Command(0x00);  Data(0x0c);  Data(0x00);
	Command(0x03);  Command(0x01);  Data(0x5a);  Data(0x0b);
	Command(0x03);  Command(0x02);  Data(0x09);  Data(0x06);
	Command(0x03);  Command(0x03);  Data(0x10);  Data(0x17);
	Command(0x03);  Command(0x04);  Data(0x23);  Data(0x00);
	Command(0x03);  Command(0x05);  Data(0x17);  Data(0x00);
	Command(0x03);  Command(0x06);  Data(0x63);  Data(0x09);
	Command(0x03);  Command(0x07);  Data(0x0c);  Data(0x09);
	Command(0x03);  Command(0x08);  Data(0x10);  Data(0x0c);
	Command(0x03);  Command(0x09);  Data(0x22);  Data(0x32);

	Command(0x00);  Command(0x10);  Data(0x00);  Data(0x16);
	Command(0x00);  Command(0x11);  Data(0x01);  Data(0x01);
	Command(0x00);  Command(0x12);  Data(0x00);  Data(0x00);
	Command(0x00);  Command(0x13);  Data(0x00);  Data(0x01);

	Command(0x01);  Command(0x00);  Data(0x07);  Data(0x30);
	Command(0x01);  Command(0x01);  Data(0x02);  Data(0x37);
	Command(0x01);  Command(0x03);  Data(0x0d);  Data(0x00);
	Command(0x02);  Command(0x80);  Data(0x67);  Data(0x00);
	Command(0x01);  Command(0x02);  Data(0xc9);  Data(0xb0);
	mdelay(20);

	Command(0x00);  Command(0x01);  Data(0x00);  Data(0x00);
	Command(0x00);  Command(0x02);  Data(0x01);  Data(0x00);
	Command(0x00);  Command(0x03);  Data(0x50);  Data(0x38);//Data(0x30);
	Command(0x00);  Command(0x09);  Data(0x00);  Data(0x01);
	Command(0x00);  Command(0x0c);  Data(0x00);  Data(0x00);

	Command(0x00);  Command(0x90);  Data(0x08);  Data(0x00);
	Command(0x00);  Command(0x0f);  Data(0x00);  Data(0x00);

	Command(0x02);  Command(0x10);  Data(0x00);  Data(0x00);
	Command(0x02);  Command(0x11);  Data(0x00);  Data(0xef);
	Command(0x02);  Command(0x12);  Data(0x00);  Data(0x00);
	Command(0x02);  Command(0x13);  Data(0x01);  Data(0x8f);

	Command(0x05);  Command(0x00);  Data(0x00);  Data(0x00);
	Command(0x05);  Command(0x01);  Data(0x00);  Data(0x00);
	Command(0x05);  Command(0x02);  Data(0x00);  Data(0x5f);

	Command(0x04);  Command(0x01);  Data(0x00);  Data(0x01);
	Command(0x04);  Command(0x04);  Data(0x00);  Data(0x00);
	mdelay(20);
	Command(0x00);  Command(0x07);  Data(0x01);  Data(0x00);
	mdelay(20);
	Command(0x02);  Command(0x00);  Data(0x00);  Data(0x00);
	Command(0x02);  Command(0x01);  Data(0x00);  Data(0x00);

	Command(0x02);  Command(0x02);
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
	printk("[%s:%d]\n", __FUNCTION__, __LINE__);

	gpHalDispSetDevType(HAL_DISP_DEV_LCM);

	/* Enable lcm clock */
	//SCUA_SAR_GPIO_CTRL &= ~0x02;	/* use tft0 */
	SCUA_LCD_CLK_CFG |= 0x00010000;		/*27MHz*/
	scu_lcm_enable(1);

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
