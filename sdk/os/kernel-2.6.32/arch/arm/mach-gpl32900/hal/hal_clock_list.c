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
 * @file    hal_clock_list.c
 * @brief   Implement of clock hardware list
 * @author  Roger Hsu
 */
#include <mach/kernel.h>
#include <mach/hal/hal_common.h>
#include <mach/hal/regmap/reg_scu.h>
#include <mach/hal/regmap/reg_i2c_bus.h>

#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <mach/hardware.h>
#include <mach/hal/hal_clock.h>

#include <mach/hal/sysregs.h>
#include <mach/typedef.h>
#include <mach/hal/hal_common.h>

//should move to hal
#include <mach/clock_mgr/gp_clock_private.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

gp_clk_t clk_lcd_ctrl = 
{
	.name		= "LCD_CTRL1",
	.id			= -1,
	.parent		= &clk_ppu_spr,
	//.clkchain   = &seq_LCD_CTRL,
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_A,
	.ctrlbit	= SCU_A_PERI_LCD_CTRL,
};

gp_clk_t clk_ppu_spr = 
{
	.name		= "PPU_SPR1",
	.id			= -1,
	.parent		= &clk_ppu,
	//.clkchain   = NULL,		
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_A,
	.ctrlbit	= 1<<3,//SCU_A_PERI_PPU_SPR,
};

gp_clk_t clk_ppu = 
{
	.name		= "PPU1",
	.id			= -1,
	.parent		= &clk_ppu_reg,
	//.clkchain   = NULL,		
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_A,
	.ctrlbit	= SCU_A_PERI_PPU,
};

gp_clk_t clk_ppu_reg1 = 
{
	.name		= "PPU_REG1",
	.id			= -1,
	.parent		= NULL,
	//.clkchain   = NULL,		
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_A,
	.ctrlbit	= SCU_A_PERI_PPU_REG,
};


gp_clk_t clk_sys_a = 
{
	.name		= "SYS_A",
	.id			= -1,
	.parent		= NULL,
	//.clkchain   = NULL,		
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_C,
	.ctrlbit	= SCU_C_PERI_SYS_A,
};

gp_clk_t clk_nand_abt = 
{
	.name		= "NAND_ABT",
	.id			= -1,
	.parent		= &clk_sys_a,
	//.clkchain   = NULL,		
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_A,
	.ctrlbit	= SCU_A_PERI_NAND_ABT,
};

/*For Audio - 93007044[12] == 1, 
  For RTARB - dram_address[31] == 1,
  then enable RTABT212.*/
gp_clk_t clk_rtabt212 = 
{
	.name		= "RTABT212",
	.id			= -1,
	.parent		= &clk_sys_a,
	//.clkchain   = NULL,		
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_A,
	.ctrlbit	= SCU_A_PERI_RTABT212,
};

gp_clk_t clk_realtimeabt = 
{
	.name		= "READLTIME_ABT",
	.id			= -1,
	.parent		= &clk_rtabt212,
	//.clkchain   = NULL,		
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_A,
	.ctrlbit	= SCU_A_PERI_REALTIME_ABT,
};

gp_clk_t clk_apbdma_a = 
{
	.name		= "APBDMA_A",
	.id			= -1,
	.parent		= &clk_realtimeabt,
	//.clkchain   = NULL,		
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_A,
	.ctrlbit	= SCU_A_PERI_APBDMA_A,
};

gp_clk_t clk_2dscaabt = 
{
	.name		= "2DSCAABT",
	.id			= -1,
	.parent		= NULL,
	//.clkchain   = NULL,		
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_C,
	.ctrlbit	= SCU_C_PERI_2DSCALEABT,
};

gp_clk_t clk_ppu_reg = 
{
	.name		= "PPU_REG",
	.id			= -1,
	.parent		= &clk_2dscaabt,
	//.clkchain   = NULL,		
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_A2,
	.ctrlbit	= SCU_A_PERI_PPU_REG,
};

gp_clk_t clk_ppu_fb = 
{
	.name		= "PPU_FB",
	.id			= -1,
	.parent		= &clk_ppu_reg,
	//.clkchain   = NULL,		
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_A2,
	.ctrlbit	= SCU_A_PERI_PPU_FB,
};

gp_clk_t clk_ppu_tv = 
{
	.name		= "PPU_TV",
	.id			= -1,
	.parent		= &clk_ppu_fb,
	//.clkchain   = NULL,		
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_A2,
	.ctrlbit	= SCU_A_PERI_PPU_TV,
};

/*It was enabled by apbdma0 module.*/
gp_clk_t clk_apbdma_c = 
{
	.name		= "APBDMA_C",
	.id			= -1,
	.parent		= NULL,
	//.clkchain   = NULL,		
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_C,
	.ctrlbit	= SCU_C_PERI_APBDMA,
};



