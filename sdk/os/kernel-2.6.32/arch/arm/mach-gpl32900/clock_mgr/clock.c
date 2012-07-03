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
/**
 * @file    clock.c
 * @brief   clock module interface function
 * @author  Roger Hsu
 */
#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/diag.h>
#include <mach/cdev.h>
#include <mach/hardware.h>
//#include <asm/irq.h>


#include <mach/typedef.h>
#include <mach/clock_mgr/gp_clock_private.h>
#include <mach/hal/hal_clock.h>
#include <mach/hal/regmap/reg_scu.h>
#include <mach/general.h>

//static gp_clock_handle_t g_clock_handle = {0, -1};
static gp_clock_function_t	clock_func[CLOCK_MAX_NUM];

 typedef struct gp_clock_s {
	struct miscdevice dev;     				
	spinlock_t lock;
	struct semaphore sem;
	int open_count;
} gp_clock_t;

static gp_clock_t* gp_clock_info = NULL;

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
char * list_SAACC[]    = {"APBDMA_A",     "FABRIC_A",        "SCUB_SYS_APB_EN", "SCUB_SYS_EN"};
char * list_BCH[]      = {"NAND_ABT",     "AAHBM212",        "FABRIC_A",        "SCUB_SYS_AHB_EN", "SCUB_SYS_EN"};
char * list_USBHOST[]  = {"AAHBM212",     "FABRIC_A",        "SCUB_SYS_AHB_EN", "SCUB_SYS_EN"};
char * list_USBDEV[]   = {"AAHBM212",     "FABRIC_A",        "SCUB_SYS_AHB_EN", "SCUB_SYS_EN"};

char * list_SCUC[]      = {"SCUB_SYS_EN"};
char * list_DMAC0[]     = {"SCUB_SYS_EN"};
char * list_DMAC1[]     = {"SCUB_SYS_EN"};
char * list_DRAM_CTRL[] = {"SCUB_SYS_EN"};
//char * list_SCALING[]   = {"2DSCALEABT", "SCUB_SYS_EN"};
char * list_SCALING[]   = {"2DSCALEABT","LINEBUFFER", "SCUB_SYS_EN"};
char * list_CIR[] = {"APBDMA_C"};
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

struct clkchain_s seq_SAACC= {
   .cnt = 4,
   .chainlist = list_SAACC,
};

struct clkchain_s seq_BCH= {
   .cnt = 5,
   .chainlist = list_BCH,
};

struct clkchain_s seq_USBHOST= {
   .cnt = 4,
   .chainlist = list_USBHOST,
};

struct clkchain_s seq_USBDEV= {
   .cnt = 4,
   .chainlist = list_USBDEV,
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

struct clkchain_s seq_SCALING= {
   .cnt = 3,
   .chainlist = list_SCALING,
};

struct clkchain_s seq_CIR= {
   .cnt = 1,
   .chainlist = list_CIR,
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

#ifndef CONFIG_PM
static int default_clkcon_enable(struct clk *clk, int enable)
{
	unsigned int clocks = clk->ctrlbit;
	volatile unsigned int* pAddr;
	unsigned long clkcon;

    pAddr = (volatile unsigned int *)clk->pValAddr;
	clkcon = *pAddr;

	//printk("NOTICE!!! [%s] call from [%s], need to remove to module driver\n",__FUNCTION__, (clk->name==NULL) ? "UNKNOW" : clk->name);

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
#endif

#ifndef CONFIG_PM
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
		/* @todo : using clk_lcd in clock_Src.c */
		.name		= "LCD_CTRL",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_LCD_CTRL,
		.set_rate	= &gpHalLcdScuEnable,
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_LCD_CTRL,
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
	},  {
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
		.name		= "BCH",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_BCH,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_BCH,
	},  {
		.name		= "AAHBM212",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,			
		.ctrlbit	= SCU_A_PERI_AAHBM212,
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
		.name		= "SPU",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_SPU,
	}, {
		.name		= "SCA",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_SCA,
	}, {
		.name		= "OVG",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_OVG,
	}, {
		.name		= "MIPI",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_MIPI,
	}, {
		.name		= "CDSP",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_CDSP,
	}, {
		.name		= "AES",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN,	
		.ctrlbit	= SCU_A_PERI_AES,
	}, {
		.name		= "PPU_SPR",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN2,	
		.ctrlbit	= SCU_A_PERI_PPU_SPR,
	}, {
		.name		= "CEVA_L2RAM",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN2,	
		.ctrlbit	= SCU_A_PERI_CEVA_L2RAM,
	},  {
		.name		= "PPU",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN2,	
		.ctrlbit	= SCU_A_PERI_PPU,
	}, {
		.name		= "PPU_REG",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN2,	
		.ctrlbit	= SCU_A_PERI_PPU_REG,
	}, {
		.name		= "PPU_TFT",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN2,	
		.ctrlbit	= SCU_A_PERI_PPU_TFT,
	}, {
		.name		= "PPU_STN",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN2,	
		.ctrlbit	= SCU_A_PERI_PPU_STN,
	}, {
		.name		= "PPU_TV",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN2,	
		.ctrlbit	= SCU_A_PERI_PPU_TV,
	}, {
		.name		= "PPU_FB",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUA_A_PERI_CLKEN2,	
		.ctrlbit	= SCU_A_PERI_PPU_FB,
	}
};


static struct clk init_scub_clocks[] = {
	{
		.name		= "AHB2AHB",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUB_B_PERI_CLKEN,
		.ctrlbit	= SCU_B_PERI_AHB2AHB,
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
		.rate		= XTAL_RATE,		
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUB_B_PERI_CLKEN,
		.ctrlbit	= SCU_B_PERI_TIMER1,
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
		.name		= "DRAM_CTRL",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = &seq_DRAM_CTRL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_DRAM_CTRL,
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
		.ctrlbit	= SCU_C_PERI_INT_MEM,
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
		.name		= "SYS_A",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_SYS_A,
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
		.name		= "EFUSE",
		.id			= -1,
		.parent		= NULL,
		.clkchain   = NULL,		
		.usage		= 0,
		.enable		= default_clkcon_enable,
		.pValAddr   = (unsigned int) &SCUC_C_PERI_CLKEN,
		.ctrlbit	= SCU_C_PERI_EFUSE,
	},	
};
#endif

/* initalise all base clocks */
#ifndef CONFIG_PM
static int __init gp_enable_baseclk(char *name)
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
#endif

#if NOT_USE
static int __init gp_disable_baseclk(char *name)
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

int __init gp_baseclk_add(void)
{
#ifndef CONFIG_PM	
	struct clk *clkp;
	int ret;
	int ptr;

	clkp = init_pseud_clocks;
	for (ptr = 0; ptr < ARRAY_SIZE(init_pseud_clocks); ptr++, clkp++) {
		/* ensure that we note the clock state */
		
		ret = gp_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
	}	

	clkp = init_scua_clocks;
	for (ptr = 0; ptr < ARRAY_SIZE(init_scua_clocks); ptr++, clkp++) {
		/* ensure that we note the clock state */
		
		ret = gp_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
	}	

	clkp = init_scub_clocks;
	for (ptr = 0; ptr < ARRAY_SIZE(init_scub_clocks); ptr++, clkp++) {
		/* ensure that we note the clock state */
		
		ret = gp_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
	}	

	clkp = init_scuc_clocks;
	for (ptr = 0; ptr < ARRAY_SIZE(init_scuc_clocks); ptr++, clkp++) {
		/* ensure that we note the clock state */
		
		ret = gp_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
	}		
#endif
	//gp_reg_dump(0x93007000, 0x20, 4);
	//gp_reg_dump(0x90005020, 0x10, 4);
	//gp_reg_dump(0x92005000, 0x10, 4);

#ifdef CONFIG_PM
	unsigned int scua_peri_clock=0,scub_peri_clock=0,scuc_peri_clock=0,scua2_peri_clock=0;
	//check
	gpHalPeriClokcCheck(&scua_peri_clock, &scub_peri_clock, &scuc_peri_clock, &scua2_peri_clock);

	scua_peri_clock |= ((SCU_A_PERI_AAHBM212));
	scub_peri_clock |= ((SCU_B_PERI_RTC)|(SCU_B_PERI_TIMER0)|(SCU_B_PERI_VIC0)|(SCU_B_PERI_VIC1)|(SCU_B_PERI_AHB2AHB));
	scuc_peri_clock |= ((SCU_C_PERI_SYS_A)|(SCU_C_PERI_UART_C2)|(SCU_C_PERI_INT_MEM)|(SCU_C_PERI_APBDMA)|(SCU_C_PERI_DRAM_CTRL));
	// Work-around for suspend/resume
	scua2_peri_clock|= (SCU_A_PERI_PPU_SPR);

	// enable necessary clock
	gpHalScuClkEnable(~(scua_peri_clock ), SCU_A, 0);
	gpHalScuClkEnable(~(scub_peri_clock ), SCU_B, 0);
	gpHalScuClkEnable(~(scuc_peri_clock ), SCU_C, 0);
	gpHalScuClkEnable(~(scua2_peri_clock), SCU_A2, 0);

#endif

#ifndef CONFIG_PM
	gp_enable_baseclk("LINEBUFFER");
    gp_enable_baseclk("AAHBM212");
    gp_enable_baseclk("NAND_ABT");
    gp_enable_baseclk("REALTIME_ABT");
    gp_enable_baseclk("RTABT212");
    gp_enable_baseclk("VIC0");
    gp_enable_baseclk("VIC1");
    gp_enable_baseclk("TIMER0");
    gp_enable_baseclk("TIMER1");
    gp_enable_baseclk("INT_MEM");	
    gp_enable_baseclk("DRAM_CTRL");
    gp_enable_baseclk("AHB2AHB");	
	gp_enable_baseclk("2DSCALEABT");
	gp_enable_baseclk("SYS_A");	gp_enable_baseclk("OVG");
#endif
    SCUB_WFI = 0;  /* arm1176jzf-s with dynamic gated clock */ 
    
    SCUC_CEVA_RATIO = 0x1;
    SCUC_CEVA_AHB_RATIO = 0x0;
    SCUC_SYS_RATIO_UPDATE = 0x70;
    
    /* temp enable for audio */
    SCUA_I2S_BCK_CFG = 0;
	SCUA_I2S_BCK_CFG = 0x107;
//    gp_enable_baseclk("SCUA_I2S_BCK_EN");				
	
	/* clear peripherals clock function point */
	memset(clock_func, 0 , sizeof(gp_clock_function_t) * CLOCK_MAX_NUM);

	/* enumerate peripherals clock function*/
	//clock_func[CLOCK_LCD].clock_set_func = &gpHalLcdScuEnable;
	//clock_func[CLOCK_LCD].clock_get_func = &gpHalLcdGetFreq;
	
	//gp_reg_dump(0x93007000, 0x20, 4);
	//gp_reg_dump(0x90005020, 0x10, 4);
	//gp_reg_dump(0x92005000, 0x10, 4);
	
	return 0;
}

#if NOT_USE
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
#endif //NOT_USE

/**
* @brief 	Clock get functino 
* @param 	clock_name[in]: base/device clock name to get current base clock
 * @param   freq[out]: clock real setting value
 * @return  SP_OK(0)/ERROR_ID
*/
int gp_clk_get_rate(int *clock_name, int*freq)
{
	struct clk *clkp;

	if (down_interruptible(&gp_clock_info->sem) != 0) {
		return -EBUSY;
	}

	clkp = clk_get(NULL, (char *)clock_name);
	if (IS_ERR(clkp) || clkp == NULL){
		printk("ERROR clock name[%s]", (clkp == NULL) ? "NULL" : (char *)clkp);
		return -ENOENT;
	}

	*freq = clk_get_rate(clkp);

	up(&gp_clock_info->sem);
	return 0;

}
EXPORT_SYMBOL(gp_clk_get_rate);

/**
* @brief 	Clock Set function
* @param 	clock_name[in]: base/device clock name to set current base clock
* @param 	rate[in]: clock rate to change
* @param 	realRate[in]: real clock rate after change
* @param 	device_clk_func[in]: device clock adjust function, passing two parameter to function : old rate, new rate 
* @return 	SUCCESS/FAIL.
*/
int gp_clk_set_rate(char *clock_name, unsigned int rate, unsigned int *real_rate, int *device_clock_func)
{
	struct clk *clkp;
	int ret ;

	if (down_interruptible(&gp_clock_info->sem) != 0) {
		return -EBUSY;
	}

	clkp = clk_get(NULL, (char *)clock_name);
	if (IS_ERR(clkp) || clkp == NULL){
		printk("ERROR clock name[%s]", (clkp == NULL) ? "NULL" : (char *)clkp);
		return -ENOENT;
	}

	ret = clk_set_rate(clkp, rate);

	// return real clock rate
	*real_rate = clkp->rate;
	
	//@todo : call device_clock_func
	//if (device_clock_func != NULL)
	//device_clock_func()

	up(&gp_clock_info->sem);
	return 0;
}
EXPORT_SYMBOL(gp_clk_set_rate);

/**
* @brief 	Enable/Disable clock interface function
* @param 	clock_name[in]: base/device clock name
* @param 	enable[in]:  1: enable, 0 : diable
* @return 	SUCCESS/FAIL.
*/
int gp_enable_clock(int *clock_name, int enable)
{
	struct clk *clkp;
	int ret = SP_SUCCESS;

//	printk("[%s][%d] run, clock_name=[%s]\n", __FUNCTION__, __LINE__, (char *)clock_name);

	clkp = clk_get(NULL, (char *)clock_name);
	if (IS_ERR(clkp) || clkp == NULL){
		printk(KERN_ERR "Failed to Get clock %s\n",(char *)clock_name);
    	return -ENOENT;
    }

	if (down_interruptible(&gp_clock_info->sem) != 0) {
		return -EBUSY;
	}
	spin_lock(&clocks_lock);

	ret = gpHalClockEnable(clkp, enable);
	//clk_enable(clkp, enable);

	spin_unlock(&clocks_lock);

	up(&gp_clock_info->sem);

	return ret;
}
EXPORT_SYMBOL(gp_enable_clock);


/**
 * @brief   spll spread mode enable function.
 * @param   modules[in]: 0:spll,1:spll2
 * @return  0
 */
int gp_enable_spll_spread_mode(int modules)
{
	if (modules == 0) {
		SCUB_SPLL_CFG1 |= (1<<25); /* SPLL spread mode enable */
	}		
	else if (modules == 1) {
		SCUB_SPLL_CFG1 |= (1<<26); /* SPLL2 spread mode enable */
	}
	
	return 0;	
}
EXPORT_SYMBOL(gp_enable_spll_spread_mode);

/**
 * @brief   spll spread mode disable function.
 * @param   modules[in]: 0:spll,1:spll2
 * @return  0
 */
int gp_disable_spll_spread_mode(int modules)
{
	if (modules == 0) {
		SCUB_SPLL_CFG1 &= ~(1<<25); /* SPLL spread mode disable */
	}		
	else if (modules == 1) {
		SCUB_SPLL_CFG1 &= ~(1<<26); /* SPLL2 spread mode disable */
	}
	
	return 0;	
}
EXPORT_SYMBOL(gp_disable_spll_spread_mode);

/**
 * @brief   set spll spread modulation clock function.
 * @param   modules[in]: 0:spll,1:spll2
 * @param   mc1[in]: modulation clock 1(HZ,ex:125000)
 * @return  0 or -1
 */
int gp_set_spll_spread_mc(int modules, int mc1)
{
	int M,FPFD,MDS;
	
	if (mc1 == 0) {
		return -1;
	}
	
	if (modules == 0) {
		M = (SCUB_SPLL_CFG0 & 0x7C000000) >> 26;
	}
	else if(modules == 1) {
		M = (SCUB_SPLL_CFG2 & 0x1F);	
	}
	else {
		return -1;
	}
	
	FPFD = XTAL_RATE/M;
	
	MDS = FPFD/(4*mc1) - 2;
	
	if (MDS < 0) {
		MDS = 0;
	}
	
	MDS &= 0x7;
	
	gp_set_spll_spread_mcc(modules,MDS);
	
	return 0;
}
EXPORT_SYMBOL(gp_set_spll_spread_mc);

/**
 * @brief   set spll spread modulation rate function.
 * @param   modules[in]: 0:spll,1:spll2
 * @param   mr[in]: modulation rate (1~90)
 * @return  0 or -1
 */
int gp_set_spll_spread_mr(int modules, int mr)
{
	int deltn,N;
	
	if ((mr == 0) || mr > 90) {
		return -1;
	}
	
	if (modules == 0) {
		N = (SCUB_SPLL_CFG0 & 0x3FC) >> 2;
	}
	else if(modules == 1) {
		N = (SCUB_SPLL_CFG1 & 0xFF);	
	}
	else {
		return -1;
	}
		
	deltn = mr*N/100;
	
	if (!deltn) {
		deltn = 1;
	}
	else if (deltn & 0x1) {
		deltn += 1;
	}
	
	gp_set_spll_spread_mrc(modules,deltn/2);
	
	return 0;
}
EXPORT_SYMBOL(gp_set_spll_spread_mr);

int gp_set_spll_spread_mrc(int modules, int mrc)
{
	if (modules == 0) {
		SCUB_SPLL_CFG1 &= ~0xF00;
		SCUB_SPLL_CFG1 |= ((mrc&0xF) << 8);
	}		
	else if (modules == 1) {
		SCUB_SPLL_CFG1 &= ~0xF0000;
		SCUB_SPLL_CFG1 |= ((mrc&0xF) << 16);
	}
	return 0;
}

int gp_set_spll_spread_mcc(int modules, int mcc)
{
	if (modules == 0) {
		SCUB_SPLL_CFG1 &= ~0x7000;
		SCUB_SPLL_CFG1 |= ((mcc&0x7) << 12);
	}		
	else if (modules == 1) {
		SCUB_SPLL_CFG1 &= ~0x700000;
		SCUB_SPLL_CFG1 |= ((mcc&0x7) << 20);
	}
	return 0;
}
#if NOT_USE
EXPORT_SYMBOL(gp_clock_mgr_request);
EXPORT_SYMBOL(gp_clock_mgr_release);
EXPORT_SYMBOL(gp_clock_set_rate);
EXPORT_SYMBOL(gp_clock_get_rate);
#endif //NOT_USE

static int gp_clock_fops_open(struct inode *inode, struct file *file)
{
	if (!gp_clock_info) {
		DIAG_ERROR("Driver not initial\n");
		return -ENXIO;
	}

	gp_clock_info->open_count++;

	return 0;
}

static int gp_clock_fops_release(struct inode *inode, struct file *file)
{
	if (gp_clock_info->open_count <= 0) {
		DIAG_ERROR("Clock device already close\n");
		gp_clock_info->open_count = 0;
		return -ENXIO;
	}
	else {
		gp_clock_info->open_count -- ;
		return 0;
	}
}

//for test
extern int gp_all_clock_change(int clk_sel, int src);

static long gp_clock_fops_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = -ENOTTY;

	switch (cmd) {
	case IOCTL_GP_CLOCK_SET:	
		// for test 
		ret = gp_all_clock_change(arg, 0);
		break;
	case IOCTL_GP_CLOCK_ARM:
		ret = gp_all_clock_change(arg, 1);
		break;
	case IOCTL_GP_CLOCK_CEVA:
		ret = gp_all_clock_change(arg, 2);
		break;
	case IOCTL_GP_CLOCK_SYS:
		ret = gp_all_clock_change(arg, 3);
		break;
	case IOCTL_GP_CLOCK_ENABLE:
		ret = gp_enable_clock((int *)arg, 1);
		break;
	case IOCTL_GP_CLOCK_DISABLE:
		ret = gp_enable_clock((int *)arg, 0);
		break;
	case IOCTL_GP_CLOCK_USAGE_DUMP:
		gp_dump_clock_usage((int *)arg);
		break;
	case IOCTL_GP_CLOCK_DUMP_ALL:
		dumpclk();
		break;
	default:
		break;
	}
	return ret;
}

static void gp_clock_device_release(struct device *dev)
{
	return ;
}

#ifdef CONFIG_PM
static int gp_clock_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int gp_clock_resume(struct platform_device *pdev)
{
	return 0;
}

#else
#define gp_clock_suspend NULL
#define gp_clock_resume NULL
#endif

static struct file_operations gp_clock_fops = {
	.owner		= THIS_MODULE,
	.open		= gp_clock_fops_open,
	.release	= gp_clock_fops_release,
	.unlocked_ioctl = gp_clock_fops_ioctl,
};


static struct platform_device gp_clock_device = {
	.name	= "gp-clock",
	.id	= -1,
    .dev	= {
		.release = gp_clock_device_release,
    }
};

static struct platform_driver gp_clock_driver = {
	.driver		= {
		.name	= "gp-clock",
		.owner	= THIS_MODULE,
	},
	.suspend	= gp_clock_suspend,
	.resume		= gp_clock_resume,
};

static int __init gp_clock_module_init(void)
{
	int ret = 0;

	gp_clock_info = kzalloc(sizeof(gp_clock_t),GFP_KERNEL);
	if ( NULL == gp_clock_info ) {
		return -ENOMEM;
	}	

	platform_device_register(&gp_clock_device);
	ret = platform_driver_register(&gp_clock_driver);
	if (ret) {
		DIAG_ERROR("%s: failed to add adc driver\n", __func__);
		return ret;
	}


	init_MUTEX(&gp_clock_info->sem);

	/* register misc device */
	gp_clock_info->dev.name = "clock-mgr";
	gp_clock_info->dev.minor = MISC_DYNAMIC_MINOR;
	gp_clock_info->dev.fops  = &gp_clock_fops;
	ret = misc_register(&gp_clock_info->dev);
	if ( ret != 0 ) {
		DIAG_ERROR(KERN_ALERT "misc register fail\n");
		kfree(gp_clock_info);
		return ret;
	}

	return ret;
}

static void __exit gp_clock_module_exit(void)
{
	platform_device_unregister(&gp_clock_device);
	platform_driver_unregister(&gp_clock_driver);

	misc_deregister(&gp_clock_info->dev);

	if (gp_clock_info) 
		kfree(gp_clock_info);
	gp_clock_info = NULL;
}

module_init(gp_clock_module_init);
module_exit(gp_clock_module_exit);
MODULE_LICENSE_GP;

