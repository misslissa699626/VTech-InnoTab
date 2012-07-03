#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/diag.h>

#include <mach/hardware.h>
#include <mach/spmp_clock.h>
#include <mach/regs-scu.h>
#include <mach/clock_mgr/gp_clock.h>
#include <mach/hal/hal_clock.h>

static gp_clock_handle_t g_clock_handle = {0, -1};
static gp_clock_function_t	clock_func[CLOCK_MAX_NUM];

#define NOT_USE		0

/*******************************************************************************/
//char * list_LCD_CTRL[] = {"REALTIME_ABT", "SCUA_LCD_CLK_EN", "FABRIC_A",        "SCUB_SYS_EN"};
char * list_LCD_CTRL[] = {"LINEBUFFER","REALTIME_ABT", "SCUA_LCD_CLK_EN", "FABRIC_A",        "SCUB_SYS_EN"};
//char * list_TVOUT[]    = {"REALTIME_ABT", "FABRIC_A",        "SCUB_SYS_EN"};		
char * list_TVOUT[]    = {"LINEBUFFER","REALTIME_ABT", "FABRIC_A",        "SCUB_SYS_EN"};		
char * list_CMOS_CTRL[]= {"REALTIME_ABT", "SCUA_CSI_CLK_EN", "FABRIC_A",        "SCUB_SYS_EN"};
char * list_I2S[]      = {"APBDMA_A",     "REALTIME_ABT",    "SCUA_I2S_BCK_EN", "FABRIC_A",        "SCUB_SYS_APB_EN", "SCUB_SYS_AHB_EN", "SCUB_SYS_EN"};
char * list_I2SRX[]    = {"APBDMA_A",     "REALTIME_ABT",    "FABRIC_A",        "SCUB_SYS_APB_EN", "SCUB_SYS_AHB_EN", "SCUB_SYS_EN"};
char * list_NAND0[]    = {"NAND_ABT",     "AAHBM212",        "FABRIC_A",        "SCUB_SYS_AHB_EN", "SCUB_SYS_EN"};
char * list_NAND1[]    = {"NAND_ABT",     "AAHBM212",        "FABRIC_A",        "SCUB_SYS_AHB_EN", "SCUB_SYS_EN"};
char * list_SAACC[]    = {"APBDMA_A",     "FABRIC_A",        "SCUB_SYS_APB_EN", "SCUB_SYS_EN"};
char * list_SCUA[]     = {"FABRIC_A",     "SCUB_SYS_EN"};
char * list_BCH[]      = {"NAND_ABT",     "AAHBM212",        "FABRIC_A",        "SCUB_SYS_AHB_EN", "SCUB_SYS_EN"};
char * list_DRM[]      = {"AAHBM212",     "FABRIC_A",        "SCUB_SYS_AHB_EN", "SCUB_SYS_EN"};
char * list_USBHOST[]  = {"AAHBM212",     "FABRIC_A",        "SCUB_SYS_AHB_EN", "SCUB_SYS_EN"};
char * list_USBDEV[]   = {"AAHBM212",     "FABRIC_A",        "SCUB_SYS_AHB_EN", "SCUB_SYS_EN"};

char * list_SCUC[]      = {"SCUB_SYS_EN"};
char * list_DMAC0[]     = {"SCUB_SYS_EN"};
char * list_DMAC1[]     = {"SCUB_SYS_EN"};
char * list_DRAM_CTRL[] = {"SCUB_SYS_EN"};
//char * list_2D_ENGINE[] = {"2DSCALEABT", "SCUB_SYS_EN"};
char * list_2D_ENGINE[] = {"2DSCALEABT","LINEBUFFER", "SCUB_SYS_EN"};
//char * list_SCALING[]   = {"2DSCALEABT", "SCUB_SYS_EN"};
char * list_SCALING[]   = {"2DSCALEABT","LINEBUFFER", "SCUB_SYS_EN"};
char * list_CIR[] = {"APBDMA_C"};
char * list_ROTATOR[] = {"2DSCALEABT","LINEBUFFER"};
char * list_NOR_CTRL[]  = {"SCUB_SYS_EN"};
char * list_CF[]        = {"APBDMA_C", "SCUB_SYS_APB_EN", "SCUB_SYS_AHB_EN", "SCUB_SYS_EN"};
char * list_MS[]        = {"APBDMA_C", "SCUB_SYS_APB_EN", "SCUB_SYS_AHB_EN", "SCUB_SYS_EN"};
char * list_UART0_2[]   = {"APBDMA_C", "SCUA_UART_CFG",   "SCUB_SYS_APB_EN", "SCUB_SYS_AHB_EN", "SCUB_SYS_EN"};
char * list_SSP0_1[]    = {"APBDMA_C", "SCUB_SYS_APB_EN", "SCUB_SYS_AHB_EN", "SCUB_SYS_EN"};
char * list_SD0_1[]     = {"APBDMA_C", "SCUB_SYS_APB_EN", "SCUB_SYS_AHB_EN", "SCUB_SYS_EN"};
char * list_SYS_I2C[]   = {"APBDMA_C", "SCUB_SYS_APB_EN", "SCUB_SYS_AHB_EN", "SCUB_SYS_EN"};
char * list_TI2C[]      = {"APBDMA_C", "SCUB_SYS_APB_EN", "SCUB_SYS_AHB_EN", "SCUB_SYS_EN"};

//ceva 
char * list_CXSYSTEM[]   = {"CXMP_SL",    "CXMD_SL",   "CXS_SL" ,"SCUC_CX_APB_EN", "SCUC_CX_AHB_EN", "SCUC_CX1620_EN", "SCUB_SYS_EN"};
char * list_CXL2[]       = {"CXS_SL" ,"SCUC_CX_AHB_EN", "SCUB_SYS_EN"};

struct clkchain_s seq_LCD_CTRL= {
   .cnt = 5,
   .chainlist = list_LCD_CTRL,
};
struct clkchain_s seq_TVOUT= {
   .cnt = 4,
   .chainlist = list_TVOUT,
};
struct clkchain_s seq_CMOS_CTRL= {
   .cnt = 4,
   .chainlist = list_CMOS_CTRL,
};

struct clkchain_s seq_I2S= {
   .cnt = 7,
   .chainlist = list_I2S,
};

struct clkchain_s seq_I2SRX= {
   .cnt = 6,
   .chainlist = list_I2SRX,
};

struct clkchain_s seq_NAND0= {
   .cnt = 5,
   .chainlist = list_NAND0,
};

struct clkchain_s seq_NAND1= {
   .cnt = 5,
   .chainlist = list_NAND1,
};

struct clkchain_s seq_SAACC= {
   .cnt = 4,
   .chainlist = list_SAACC,
};

struct clkchain_s seq_SCUA= {
   .cnt = 2,
   .chainlist = list_SCUA,
};

struct clkchain_s seq_BCH= {
   .cnt = 5,
   .chainlist = list_BCH,
};

struct clkchain_s seq_DRM= {
   .cnt = 4,
   .chainlist = list_DRM,
};

struct clkchain_s seq_USBHOST= {
   .cnt = 4,
   .chainlist = list_USBHOST,
};

struct clkchain_s seq_USBDEV= {
   .cnt = 4,
   .chainlist = list_USBDEV,
};

struct clkchain_s seq_SCUC= {
   .cnt = 1,
   .chainlist = list_SCUC,
};

struct clkchain_s seq_DMAC0= {
   .cnt = 1,
   .chainlist = list_DMAC0,
};

struct clkchain_s seq_DMAC1= {
   .cnt = 1,
   .chainlist = list_DMAC1,
};

struct clkchain_s seq_DRAM_CTRL= {
   .cnt = 1,
   .chainlist = list_DRAM_CTRL,
};

struct clkchain_s seq_2D_ENGINE= {
   .cnt = 3,
   .chainlist = list_2D_ENGINE,
};

struct clkchain_s seq_SCALING= {
   .cnt = 3,
   .chainlist = list_SCALING,
};

struct clkchain_s seq_CIR= {
   .cnt = 1,
   .chainlist = list_CIR,
};

struct clkchain_s seq_ROTATOR= {
   .cnt = 1,
   .chainlist = list_ROTATOR,
};

struct clkchain_s seq_NOR_CTRL= {
   .cnt = 1,
   .chainlist = list_NOR_CTRL,
};

struct clkchain_s seq_CF= {
   .cnt = 4,
   .chainlist = list_CF,
};

struct clkchain_s seq_MS= {
   .cnt = 4,
   .chainlist = list_MS,
};

struct clkchain_s seq_UART0_2= {
   .cnt = 5,
   .chainlist = list_UART0_2,
};


struct clkchain_s seq_SSP0_1= {
   .cnt = 4,
   .chainlist = list_SSP0_1,
};

struct clkchain_s seq_SD0_1= {
   .cnt = 4,
   .chainlist = list_SD0_1,
};

struct clkchain_s seq_SYS_I2C= {
   .cnt = 4,
   .chainlist = list_SYS_I2C,
};

struct clkchain_s seq_TI2C= {
   .cnt = 4,
   .chainlist = list_TI2C,
};

struct clkchain_s seq_CXSYSTEM= {
   .cnt = 7,
   .chainlist = list_CXSYSTEM,
};

struct clkchain_s seq_CXL2= {
   .cnt = 3,
   .chainlist = list_CXL2,
};

static int default_clkcon_enable(struct clk *clk, int enable)
{
	unsigned int clocks = clk->ctrlbit;
	volatile unsigned int* pAddr;
	unsigned long clkcon;

    pAddr = (volatile unsigned int *)clk->pValAddr;
	clkcon = *pAddr;

	//printk(KERN_INFO "NOTICE!!! [%s] call from [%s], need to remove to module driver\n",__FUNCTION__, (clk->name==NULL) ? "UNKNOW" : clk->name);

	if (enable)
	{
		clkcon |= clocks;
	}
	else
	{
		clkcon &= ~clocks;
	}

	*pAddr = clkcon;

	return 0;
}

static struct clk init_pseud_clocks[] = {
	{
		.name		= "SCUA_UART_CFG",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_UART_CFG,
		.ctrlbit	= (1<<8),
	},	
	{
		.name		= "SCUB_SYS_EN",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUB_SYS_CNT_EN,	
		.ctrlbit	= (1<<0),
	},	
	{
		.name		= "SCUB_SYS_AHB_EN",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUB_SYS_CNT_EN,		
		.ctrlbit	= (1<<2),
	},	
	{
		.name		= "SCUB_SYS_APB_EN",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUB_SYS_CNT_EN,		
		.ctrlbit	= (1<<3),
	},	
	{
		.name		= "SCUA_I2S_BCK_EN",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_I2S_BCK_CFG,			
		.ctrlbit	= (1<<8),
	},	
	{
		.name		= "SCUA_LCD_CLK_EN",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_LCD_CLK_CFG,			
		.ctrlbit	= (1<<8),
	},	
	{
		.name		= "SCUA_CSI_CLK_EN",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_CSI_CLK_CFG,			
		.ctrlbit	= (1<<8),
	},	
	{
		.name		= "SCUC_CX1620_EN",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_CEVA_CNT_EN,			
		.ctrlbit	= (1<<0),
	},		
	{
		.name		= "SCUC_CX_AHB_EN",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_CEVA_CNT_EN,			
		.ctrlbit	= (1<<1),
	},	
	{
		.name		= "SCUC_CX_APB_EN",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_CEVA_CNT_EN,			
		.ctrlbit	= (1<<2),
	},	{
		.name		= "CXSYSTEM_EN",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_CXSYSTEM,		
		.usage		= 0,
		.enable		= NULL,
	}, {
		.name		= "CXL2_EN",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_CXL2,		
		.usage		= 0,
		.enable		= NULL,
	},		
};


static struct clk init_scua_clocks[] = {
	{
		.name		= "SYSA",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,			
		.ctrlbit	= SCU_A_PERI_SYSA,
	}, {
		.name		= "LCD_CTRL",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_LCD_CTRL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_LCD_CTRL,
	}, {
		.name		= "DRM",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_DRM,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_DRM,
	}, {
		.name		= "USB_HOST",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_USBHOST,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_USB0,
	}, {
		.name		= "USB_DEVICE",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_USBDEV,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_USB1,
	}, {
		.name		= "LINEBUFFER",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_USBDEV,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_LINEBUFFER,
	}, {	
		.name		= "SCUA",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_SCUA,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_SCUA,
	}, {
		.name		= "TVOUT",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_TVOUT,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_TVOUT,
	}, {
		.name		= "APBDMA_A",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_APBDMA_A,
	}, {
		.name		= "CMOS_CTRL",
		.id			= 0,
		.parent		= NULL,
		.clkchain   = &seq_CMOS_CTRL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_CMOS_CTRL,
	}, {
		.name		= "NAND0",
		.id			= 0,
		.parent		= NULL,
		.clkchain   = &seq_NAND0,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_NAND0,
	}, {
		.name		= "NAND1",
		.id			= 1,
		.parent		= NULL,
		.clkchain   = &seq_NAND1,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_NAND1,
	}, {
		.name		= "BCH",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_BCH,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_BCH,
	}, {
		.name		= "APLL",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_APLL,
	}, {
		.name		= "UART_CNCT",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,			
		.ctrlbit	= SCU_A_PERI_UART_CNCT,
	}, {
		.name		= "AAHBM212",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,			
		.ctrlbit	= SCU_A_PERI_AAHBM_SLICE,
	}, {
		.name		= "I2S",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_I2S,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_I2S,
	}, {
		.name		= "I2SRX",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_I2SRX,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_I2SRX,
	}, {
		.name		= "SAACC",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_SAACC,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_SAACC,
	}, {
		.name		= "NAND_ABT",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_NAND_ABT,
	}, {
		.name		= "REALTIME_ABT",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_REALTIME_ABT,
	}, {
		.name		= "RTABT212",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_RTABT212,
	}, {
		.name		= "CAHBM212",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_CAHBM212,
	}		
};


static struct clk init_scub_clocks[] = {
	{
		.name		= "TCM_BIST",  //clk_int_conn
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUB_B_PERI_CLKEN,	
		.ctrlbit	= SCU_B_PERI_TCM_BIST,
	}, {
		.name		= "TCM_CTRL",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUB_B_PERI_CLKEN,
		.ctrlbit	= SCU_B_PERI_TCM_CTRL,
	}, {
		.name		= "AHB2AHB",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUB_B_PERI_CLKEN,
		.ctrlbit	= SCU_B_PERI_AHB2AHB,
	}, {
		.name		= "AHB_SW",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUB_B_PERI_CLKEN,
		.ctrlbit	= SCU_B_PERI_AHB_SW,
	}, {
		.name		= "VIC0",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUB_B_PERI_CLKEN,
		.ctrlbit	= SCU_B_PERI_VIC0,
	}, {
		.name		= "VIC1",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUB_B_PERI_CLKEN,
		.ctrlbit	= SCU_B_PERI_VIC1,
	}, {
		.name		= "DPM",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUB_B_PERI_CLKEN,
		.ctrlbit	= SCU_B_PERI_DPM,
	}, {
		.name		= "APB_BRG",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUB_B_PERI_CLKEN,
		.ctrlbit	= SCU_B_PERI_APB_BRG,
	}, {
		.name		= "ARM926",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUB_B_PERI_CLKEN,
		.ctrlbit	= SCU_B_PERI_ARM926,
	}, {
		.name		= "TIMER0",
		.id			= -1,
		.parent		= &clk_arm_apb,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUB_B_PERI_CLKEN,
		.ctrlbit	= SCU_B_PERI_TIMER0,
	}, {
		.name		= "TIMER1",
		.id			= -1,
		.rate		= 27000000,		
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUB_B_PERI_CLKEN,
		.ctrlbit	= SCU_B_PERI_TIMER1,
	}, {
		.name		= "UART",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUB_B_PERI_CLKEN,
		.ctrlbit	= SCU_B_PERI_UART,
	}, {
		.name		= "ARM_I2C",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUB_B_PERI_CLKEN,
		.ctrlbit	= SCU_B_PERI_I2C,
	}, {
		.name		= "RAND",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUB_B_PERI_CLKEN,
		.ctrlbit	= SCU_B_PERI_RAND,
	}, {
		.name		= "GPIO",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUB_B_PERI_CLKEN,
		.ctrlbit	= SCU_B_PERI_GPIO,
	}, {
		.name		= "RTC",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUB_B_PERI_CLKEN,
		.ctrlbit	= SCU_B_PERI_RTC,
	}, 		
};

static struct clk init_scuc_clocks[] = {
	{
		.name		= "FABRIC_C",  //clk_int_conn
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_FABRIC_C,
	}, {
		.name		= "DMAC0",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_DMAC0,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_DMAC0,
	}, {
		.name		= "DMAC1",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_DMAC1,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_DMAC1,
	}, {
		.name		= "MEM_CTRL",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_MEM_CTRL,
	}, {
		.name		= "DRAM_CTRL",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_DRAM_CTRL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_DRAM_CTRL,
	}, {
		.name		= "SCU_C",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_SCUC,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_SCU_C,
	}, {
		.name		= "I2C_CFG",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_I2C_CFG,
	}, {
		.name		= "APBDMA_C",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_APBDMA,
	}, {
		.name		= "2D_ENGINE",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_2D_ENGINE,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_2D_ENGIN,
	}, {
		.name		= "EXT_MEM",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_NOR_CTRL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_NOR,
	}, {
		.name		= "CF",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_CF,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_CF,
	}, {
		.name		= "MS",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_MS,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_MS,
	}, {
		.name		= "INT_MEM",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_RAM,
	}, {
		.name		= "UARTC",
		.id			= 0,
		.parent		= &clk_uart,
		.clkchain   = &seq_UART0_2,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_UART_C0,
	}, {
		.name		= "UARTC",
		.id			= 1,
		.parent		= &clk_uart,
		.clkchain   = &seq_UART0_2,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_UART_C1,
	}, {
		.name		= "UARTC",
		.id			= 2,
		.parent		= &clk_uart,
		.clkchain   = &seq_UART0_2,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_UART_C2,
	}, {
		.name		= "SSP",  //clk_int_conn
		.id			= 0,
		.parent		= NULL,
		.clkchain   = &seq_SSP0_1,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_SSP0,
	}, {
		.name		= "SSP",
		.id			= 1,
		.parent		= NULL,
		.clkchain   = &seq_SSP0_1,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_SSP1,
	}, {
		.name		= "SD",
		.id			= 0,
		.parent		= &clk_sys_apb,
		.clkchain   = &seq_SD0_1,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_SD0,
	}, {
		.name		= "SD1",
		.id			= 1,
		.parent		= &clk_sys_apb,
		.clkchain   = &seq_SD0_1,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_SD1,
	}, {
		.name		= "SYS_I2C",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_SYS_I2C,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_I2C,
	}, {
		.name		= "SCALING",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_SCALING,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_SCALING,
	}, {
		.name		= "2DSCALEABT",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_2DSCALEABT,
	}, {
		.name		= "TI2C",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_TI2C,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_TI2C,
	}, {
		.name		= "FABRIC_A",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_FRBRIC_A,
	}, {
		.name		= "CXMP_SL",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_CXMP_SL,
	}, {
		.name		= "CXMD_SL",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_CXMD_SL,
	},	{
		.name		= "CIR",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_CIR,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_CIR,
	}, {
		.name		= "ROTATOR",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_ROTATOR,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_ROTATOR,
	},	
};


/* initalise all base clocks */

static int __init spmp_enable_baseclk(char *name)
{
	struct clk *clkp;
	clkp = clk_get(NULL, name);
	if (IS_ERR(clkp) || clkp == NULL){
		printk(KERN_ERR "Failed to Get clock %s\n",name);
    	return 0;		
    }	
	else
	{
	   clk_enable(clkp);
    	return 0;	   
	} 
}

#if NOT_USE
static int __init spmp_disable_baseclk(char *name)
{
	struct clk *clkp;
	clkp = clk_get(NULL, name);
	if (IS_ERR(clkp) || clkp == NULL){
		printk(KERN_ERR "Failed to Get clock %s\n",name);
    	return 0;		
    }	
	else
	{
	   clk_disable(clkp);
    	return 0;	   
	} 
}
#endif //NOT_USE

int __init spmp_baseclk_add(void)
{
	struct clk *clkp;
	int ret;
	int ptr;
	
	clkp = init_pseud_clocks;
	for (ptr = 0; ptr < ARRAY_SIZE(init_pseud_clocks); ptr++, clkp++) {
		/* ensure that we note the clock state */
		
		ret = spmp_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
	}	

	clkp = init_scua_clocks;
	for (ptr = 0; ptr < ARRAY_SIZE(init_scua_clocks); ptr++, clkp++) {
		/* ensure that we note the clock state */
		
		ret = spmp_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
	}	

	clkp = init_scub_clocks;
	for (ptr = 0; ptr < ARRAY_SIZE(init_scub_clocks); ptr++, clkp++) {
		/* ensure that we note the clock state */
		
		ret = spmp_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
	}	

	clkp = init_scuc_clocks;
	for (ptr = 0; ptr < ARRAY_SIZE(init_scuc_clocks); ptr++, clkp++) {
		/* ensure that we note the clock state */
		
		ret = spmp_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
	}		

    spmp_enable_baseclk("CAHBM212");
    spmp_enable_baseclk("BCH");
	
    spmp_enable_baseclk("TCM_BIST");
    spmp_enable_baseclk("TCM_CTRL");
    spmp_enable_baseclk("AHB2AHB");
    spmp_enable_baseclk("AHB_SW");
    spmp_enable_baseclk("VIC0");
    spmp_enable_baseclk("VIC1");
    spmp_enable_baseclk("DPM");
    spmp_enable_baseclk("APB_BRG");
    spmp_enable_baseclk("ARM926");
    spmp_enable_baseclk("TIMER0");
    spmp_enable_baseclk("TIMER1");
    spmp_enable_baseclk("UART");
    spmp_enable_baseclk("ARM_I2C");
    spmp_enable_baseclk("RAND");
	
    spmp_enable_baseclk("MEM_CTRL");
    spmp_enable_baseclk("I2C_CFG");
    spmp_enable_baseclk("EXT_MEM");
    spmp_enable_baseclk("INT_MEM");	
    spmp_enable_baseclk("SYS_I2C");
    spmp_enable_baseclk("DRAM_CTRL");
    spmp_enable_baseclk("FABRIC_A");	
	
    spmp_enable_baseclk("RTABT212");	
    spmp_enable_baseclk("AAHBM212");				
    spmp_enable_baseclk("APLL");			
    /* temp enable for audio */
    SCUA_I2S_BCK_CFG = 0;
	SCUA_I2S_BCK_CFG = 0x107;
//    spmp_enable_baseclk("SCUA_I2S_BCK_EN");				
	
	/* clear peripherals clock function point */
	memset(clock_func, 0 , sizeof(gp_clock_function_t) * CLOCK_MAX_NUM);

	/* enumerate peripherals clock function*/
	clock_func[CLOCK_LCD].clock_set_func = &gpHalLcdScuEnable;
	clock_func[CLOCK_LCD].clock_get_func = &gpHalLcdGetFreq;
	
	return 0;
}


/**
 * @brief   Clock manager request function.
 * @param   None
 * @return  HANDLE(non-negative value)/ERROR_ID(-1)
 */
int gp_clock_mgr_request(int clock_id)
{
	
	if (g_clock_handle.handle_state != 0) {
		printk(KERN_INFO "[%s]ERROR!!clock busy\n",__FUNCTION__);
		return -EBUSY;
	}

	g_clock_handle.handle_state = 1;
	g_clock_handle.clock_id = clock_id;

	return (int)&g_clock_handle;
}

/**
 * @brief   Clock manager release function.
 * @param   Handle[in]: clock manager handle to release
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_clock_mgr_release(int handle)
{
	gp_clock_handle_t *clock_handle = (gp_clock_handle_t *)handle;
	clock_handle->handle_state = 0;
	return 0;
}

/**
 * @brief   clock frequence setting function.
 * @param   handle[in]: clock handle
 * @param   fre[in]: clock frequence setting
 * @param   real_freq[out]: clock real setting value
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_clock_set_rate(int handle, int freq, int *real_freq)
{
	gp_clock_handle_t *clock_handle = (gp_clock_handle_t *)handle;

	if (clock_handle->handle_state == 0)
		return -EBUSY;

	if (clock_func[clock_handle->clock_id].clock_set_func == NULL)
		return -EIO;

	clock_func[clock_handle->clock_id].clock_set_func(freq);
	
	*real_freq = clock_func[clock_handle->clock_id].clock_get_func();
	
	return 0;
}

/**
 * @brief   clock rate getting function.
 * @param   handle[in]: clock handle
 * @param   freq[out]: clock real setting value
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_clock_get_rate(int handle, int *freq)
{
	gp_clock_handle_t *clock_handle = (gp_clock_handle_t *)handle;

	if (clock_handle->handle_state == 0)
		return -EBUSY;

	if (clock_func[clock_handle->clock_id].clock_get_func == NULL)
		return -EIO;

	*freq = clock_func[clock_handle->clock_id].clock_get_func();

	return 0;
}

EXPORT_SYMBOL(gp_clock_mgr_request);
EXPORT_SYMBOL(gp_clock_mgr_release);
EXPORT_SYMBOL(gp_clock_set_rate);
EXPORT_SYMBOL(gp_clock_get_rate);

