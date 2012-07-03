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


void lcm_GPM765H0_setup(void);

typedef struct
{
	char *name;
	uint32_t width;
	uint32_t height;
	uint32_t format;
	uint32_t type;
	uint32_t seqSel;
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
	.width = 128,
	.height = 160,
	.format = HAL_DISP_OUTPUT_FMT_RGB,
	.type = HAL_DISP_OUTPUT_TYPE_RGB565,
	.seqSel = 1,
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
	.lcmSetup = lcm_GPM765H0_setup,
};

void
lcm_GPM765H0_setup(
	void
)
{
	#define Command(gw_cmd) 		*pcmd = gw_cmd
	#define Data(gw_data)			*pdata = gw_data

	#define PANEL_128_160	0
	#define PANEL_160_128	1
	#define PANEL_USEMODE	PANEL_128_160

	dispReg_t *pdispReg = (dispReg_t *)(LOGI_ADDR_DISP_REG);
	volatile unsigned short *pcmd = (volatile unsigned short *) &pdispReg->dispLcmPgmIo00;
	volatile unsigned short *pdata = (volatile unsigned short *) &pdispReg->dispLcmPgmIo10;

	Command(0x11);	//sleep out
	mdelay(1000);
	
#if 0
	Command(0x26);
	Data(0x04);
	
	Command(0xCD);
	Data(0x04);
	
	Command(0xB1);	//Frame Rate control
	Data(0x0e);
	Data(0x14);
	
	Command(0xc0);	//GVDD step = 0.05V
	Data(0x08);
	//Data(0x03);	//GPM765I0 initial value	
	Data(0x00);
	
	Command(0xc1);	//AVDD VGL 
	Data(0x05);	//VGH = 16.5v, VGL = -8.25v
	
	Command(0xc5);	//set VCOMh and VCOMl
	//Data(0xC0);	//GPM765I0 initial value	
	Data(0x3a);
	Data(0x2d);
#endif

	Command(0x3A);	//interface format
	Data(0x05);	//16bit
	//Data(0x06);		//GPM765I0 initial value	//18bit
#if 0	
	Command(0x2A);
	Data(0x00);
	Data(0x00);
	Data(0x00);
	Data(0x7f);
	
    Command(0x2B);
	Data(0x00);
	Data(0x00);
	Data(0x00);
	Data(0x9f);
	
    Command(0xb4);
	Data(0x00);	 
	
	Command(0xF2);  //Gamma set E0.E1 enable control
	Data(0x01); //01 = enable    00=disable
	//Data(0x00); //01 = enable    00=disable		//GPM765I0 initial value
    
  	Command(0xE0);  //0Eh gamma set
	Data(0x3F); //V0     -> V63
	Data(0x2b); //V1     -> V62
	Data(0x28); //V2     -> V61
	Data(0x30); //V4     -> V59
	Data(0x27); //V6     -> V57
	Data(0x0d); //V13    -> V50
	Data(0x53); //V20    -> V43
	Data(0xfa); //V36 27 -> V27 36
	Data(0x3c); //V43    -> V20
	Data(0x19); //V50    -> V13
	Data(0x22); //V57    -> V6
	Data(0x1e); //V59    -> V4
	Data(0x02); //V61    -> V2
	Data(0x01); //V62    -> V1
	Data(0x00); //V63    ->	V0
	 	
	Command(0xE1);  //E1h gamma set
	Data(0x00); //V0	->V63
	Data(0x1b); //V1	->V62
	Data(0x1f); //V2	->V61
	Data(0x0f); //V4    ->V59
	Data(0x16); //V6 	->V57
	Data(0x13); //V13	->V50
	Data(0x30); //V20	->V43
	Data(0x84); //V36 V27  ->V27 36
	Data(0x43); //V43   ->V20
	Data(0x06); //V50   ->V13
	Data(0x1d); //V57   ->V6
	Data(0x21); //V59   ->V4
	Data(0x3d); //V61   ->V2
	Data(0x3e); //V62   ->V1
	Data(0x3f); //V63   ->V0
#endif	//#if 0
	
	//memory access control
	Command(0x36);
#if PANEL_USEMODE == PANEL_128_160	
	Data(0x08);
	//Data(0x80);	//GPM765I0 initial value
#elif  PANEL_USEMODE == PANEL_160_128	
	Data(0xA8);
#endif

	//column address set 
   	Command(0x2A);
   	Data(0x00);
   	Data(0x00);
   	Data(0x00);
#if PANEL_USEMODE == PANEL_128_160	
   	Data(0x7F);	//127
#elif  PANEL_USEMODE == PANEL_160_128		
   	Data(0x9F); 	//159 
#endif

   	//row address set
   	Command(0x2B);
   	Data(0x00);
   	Data(0x00);
   	Data(0x00);   
#if PANEL_USEMODE == PANEL_128_160	
	Data(0x9F);
#elif  PANEL_USEMODE == PANEL_160_128		
	Data(0x7F);
#endif

 	//start show
   	Command(0x29); //display on
   	
   	//Command(0x35);	//TE on
	//Data(0x01);	//mode 1
	//Data(0x00);	//mode 0
	
    Command(0x2C); //Memory write
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
	gpHalDispSetLcmSeqSel(gPanelInfo.seqSel);

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
