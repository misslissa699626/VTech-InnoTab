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
 * @file    gp_clock_private.h
 * @brief   Declaration of Clock private definition
 * @author  Roger hsu
 */
#ifndef _GP_CLOCK_PRI_H_
#define _GP_CLOCK_PRI_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <mach/kernel.h>
#include <mach/clock_mgr/gp_clock.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct clkchain_s
{
    int cnt;
	char ** chainlist;
}clkchain_t;

struct clk {
	struct list_head    list;
	struct module       *owner;
	struct clk          *parent;
	clkchain_t    		*clkchain;
	const char			*name;
	int					id;
	int					usage;
	unsigned long		rate;
	unsigned long		ctrlbit;
    unsigned int		pValAddr;
	int					(*enable)(struct clk *, int enable);
	int					(*set_rate)(struct clk *c, unsigned long rate);
	unsigned long		(*get_rate)(struct clk *c);
	unsigned long		(*round_rate)(struct clk *c, unsigned long rate);
	int					(*set_parent)(struct clk *c, struct clk *parent);
	// new member
	int					(*enable_func)(unsigned int bitMask, unsigned char scu, unsigned char enable);
    unsigned int		enable_statue;	// to count number fo enable child node
	unsigned int		clock_class;	//gpClockClass_t
	//struct clk          *parent1;		// additional parent
};

typedef struct clk	gp_clk_t;
/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
extern spinlock_t clocks_lock;

extern struct clk clk_arm;
extern struct clk clk_arm_ahb;
extern struct clk clk_arm_apb;
extern struct clk clk_ceva;
extern struct clk clk_ceva_ahb;
extern struct clk clk_ceva_apb;
extern struct clk clk_sys;
extern struct clk clk_sys_ahb;
extern struct clk clk_sys_apb;

extern struct clk clk_lcd;
extern struct clk clk_uart;
extern struct clk clk_csi;
extern struct clk clk_i2s_mclk;
extern struct clk clk_i2s_bck;

extern int __init gp_register_baseclocks(unsigned long xtal);
extern void __init gp_setup_clocks(void);
extern int __init gp_baseclk_add(void);
extern void __init dumpclk(void);
extern int gp_register_clock(struct clk *clk);


//clock tree list declare
extern gp_clk_t clk_lcd_ctrl;
extern gp_clk_t clk_ppu_spr;
extern gp_clk_t clk_ppu;
extern gp_clk_t clk_ppu_reg1;
extern gp_clk_t clk_sys_a;
extern gp_clk_t clk_nand_abt;
extern gp_clk_t clk_rtabt212;
extern gp_clk_t clk_realtimeabt;
extern gp_clk_t clk_apbdma_a;
extern gp_clk_t clk_2dscaabt;
extern gp_clk_t clk_ppu_reg;
extern gp_clk_t clk_ppu_fb;
extern gp_clk_t clk_ppu_tv;
extern gp_clk_t clk_apbdma_c;
/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

#endif /* _GP_CLOCK_PRI_H_ */
