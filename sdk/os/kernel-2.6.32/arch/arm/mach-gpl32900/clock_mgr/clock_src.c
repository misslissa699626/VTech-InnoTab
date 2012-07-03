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
 * @file    clock_src.c
 * @brief   clock control function
 * @author  Roger Hsu
 */
#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/diag.h>
#include <mach/hardware.h>

#include <mach/clock_mgr/gp_clock_private.h>
#include <mach/hal/sysregs.h>
#include <mach/hal/hal_clock.h>
#include <mach/gp_reg.h>
#include <mach/hal/regmap/reg_scu.h>
// for test 
#include <mach/general.h>
#include <mach/regs-interrupt.h>
//#include <linux/spinlock.h> 
extern void gp_sync_cache(void);

#define NOT_USE		0
#define MOVE_HAL	0
/* clock information */

static LIST_HEAD(clocks);

/* We originally used an mutex here, but some contexts (see resume)
 * are calling functions such as clk_set_parent() with IRQs disabled
 * causing an BUG to be triggered.
 */
DEFINE_SPINLOCK(clocks_lock);

/* enable and disable calls for use with the clk struct */

static int clk_null_enable(struct clk *clk, int enable)
{
	return 0;
}

/* Clock API calls */

struct clk *clk_get(struct device *dev, const char *id)
{
	struct clk *p;
	struct clk *clk = ERR_PTR(-ENOENT);
	int idno;

	if (dev == NULL || dev->bus != &platform_bus_type)
		idno = -1;
	else
		idno = to_platform_device(dev)->id;

	//spin_lock(&clocks_lock);

	list_for_each_entry(p, &clocks, list) {
		if (p->id == idno &&
		    strcmp(id, p->name) == 0 &&
		    try_module_get(p->owner)) {
			clk = p;
			break;
		}
	}

	/* check for the case where a device was supplied, but the
	 * clock that was being searched for is not device specific */

	if (IS_ERR(clk)) {
		list_for_each_entry(p, &clocks, list) {
			if (p->id == -1 && strcmp(id, p->name) == 0 &&
			    try_module_get(p->owner)) {
				clk = p;
				break;
			}
		}
	}

	//spin_unlock(&clocks_lock);
	return clk;
}

void clk_put(struct clk *clk)
{
	module_put(clk->owner);
}

int clk_enable(struct clk *clk)
{
	if (IS_ERR(clk) || clk == NULL)
		return -EINVAL;

	if(clk->clkchain)
	{
		struct clk *cclk;
		int i,cnt;
		cnt = clk->clkchain->cnt;
		for(i = cnt-1 ; i >= 0 ; i--)
		{
			cclk = clk_get(NULL, clk->clkchain->chainlist[i]);
			clk_enable(cclk);
		}
	}

	//printk("1 Current clk=[%s] parrent = [%s], usage=%d\n",clk->name,  (clk->parent == NULL) ? "NULL" : clk->parent->name, clk->usage);

	// enable the clock source
	if (clk->parent) {
		clk_enable(clk->parent);
	}
	
	//printk("2 Current clk=[%s] parrent = [%s], usage=%d\n",clk->name,  (clk->parent == NULL) ? "NULL" : clk->parent->name, clk->usage);

	//spin_lock(&clocks_lock);

	if ((clk->usage++) == 0)
	{
		if(clk->enable)
		{
			(clk->enable)(clk, 1);
		}

	}

	//spin_unlock(&clocks_lock);
	//printk("3 Current clk=[%s] parrent = [%s], usage=%d\n",clk->name,  (clk->parent == NULL) ? "NULL" : clk->parent->name, clk->usage);

	return 0;
}

void clk_disable(struct clk *clk)
{
	if (IS_ERR(clk) || clk == NULL)
		return;

	// enable the clock source
	if (clk->parent) {
		clk_disable(clk->parent);
	}

	//spin_lock(&clocks_lock);
	if (!(clk->usage <=0) && (--clk->usage) == 0)
	//if (!(clk->usage <=0) && (clk->usage) == 0)
	{
		(clk->enable)(clk, 0);
	}
    if(clk->usage <0)
    	printk("error clk_disable \n");
	//spin_unlock(&clocks_lock);
	
#if 0
    if(clk->clkchain)
    {
       struct clk *cclk;
	   int i,cnt;
	   cnt = clk->clkchain->cnt;
	   for(i = 0; i < cnt; i++)
	   {
	      cclk = clk_get(NULL, clk->clkchain->chainlist[i]);
    	  clk_disable(cclk);
	   }
    }	   
#endif

}


unsigned long clk_get_rate(struct clk *clk)
{
	if (IS_ERR(clk))
		return 0;

	//printk("current=[%s] parent=[%s], rate=%d Hz\n",(clk->name == NULL) ? "NULL" : clk->name, 
	//(clk->parent == NULL) ? "NULL" : clk->parent->name ,(int)clk->rate);

	if (clk->get_rate != NULL)
		return (clk->get_rate)(clk);

	if (clk->parent != NULL)
		return clk_get_rate(clk->parent);

	return clk->rate;
}

long clk_round_rate(struct clk *clk, unsigned long rate)
{
	if (!IS_ERR(clk) && clk->round_rate)
		return (clk->round_rate)(clk, rate);

	return rate;
}

static DEFINE_SPINLOCK(clockfw_lock);

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	int ret;
	unsigned long flags; 

	if (IS_ERR(clk))
		return -EINVAL;

	/* We do not default just do a clk->rate = rate as
	 * the clock may have been made this way by choice.
	 */

	WARN_ON(clk->set_rate == NULL);

	if (clk->set_rate == NULL)
		return -EINVAL;

	spin_lock_irqsave(&clockfw_lock, flags);

	//gp_sync_cache();
	ret = (clk->set_rate)(clk, rate);

	spin_unlock_irqrestore(&clockfw_lock, flags);

	return ret;
}

struct clk *clk_get_parent(struct clk *clk)
{
	return clk->parent;
}

int clk_set_parent(struct clk *clk, struct clk *parent)
{
	int ret = 0;

	if (IS_ERR(clk))
		return -EINVAL;

	spin_lock(&clocks_lock);

	if (clk->set_parent)
		ret = (clk->set_parent)(clk, parent);

	spin_unlock(&clocks_lock);

	return ret;
}
EXPORT_SYMBOL(clk_get);
EXPORT_SYMBOL(clk_put);
EXPORT_SYMBOL(clk_enable);
EXPORT_SYMBOL(clk_disable);
EXPORT_SYMBOL(clk_get_rate);
EXPORT_SYMBOL(clk_round_rate);
EXPORT_SYMBOL(clk_set_rate);
EXPORT_SYMBOL(clk_get_parent);
EXPORT_SYMBOL(clk_set_parent);
/* base clocks */


static int clk_default_setrate(struct clk *clk, unsigned long rate)
{
	printk("[%s][%d] clk = %s ,Can not suport set rate:%d\n",__FUNCTION__, __LINE__, (clk == NULL) ? "NULL" : clk->name ,((int)rate));

	//clk->rate = rate;
	return 0;
}

#if NOT_USE
static int apll_setrate(struct clk *clk, unsigned long rate)
{

	switch (rate)
	{
	 case 67737600:
      	SCUA_APLL_CFG |= 0x2;
       	clk->rate = 67737600;		
	   break; 	
	 case 73728000:
     default:
      	SCUA_APLL_CFG &= ~(0x2);
       	clk->rate = 73728000;				
	   break;
	}
	return 0;
}
#endif //NOT_USE

static unsigned long apll_get_rate(struct clk *clk)
{
	unsigned int apll_sel;
	apll_sel = (SCUA_APLL_CFG & 0x2) >> 1;
	switch (apll_sel)
	{
	 case 1:
	 	clk->rate = 67737600;
	   break; 	
	 case 0:
     default:
	 	clk->rate = 73728000;
	   break;
	}
	return clk->rate;
}

/**
* @brief 	apll clock enable function
* @param 	clk[in]: clock source
* @param 	rate[in]: clock rate to set , unit : HZ
* @return 	SUCCESS(0)/FAIL.
*/
static int apll_set_rate(struct clk *clk, unsigned long rate)
{

	if (rate <= 44100)
	 	gpHalApllSetRate(AUDIO_APLL_SEL_44K);
	else
		gpHalApllSetRate(AUDIO_APLL_SEL_48K);

	if (clk)
    	clk->rate = rate;

   	return 0;
}

/**
* @brief 	apll clock enable function
* @param 	clk[in]: clock source
* @param 	enable[in]: 0: diable , 1 : enable
* @return 	SUCCESS/FAIL.
*/
static int apll_clk_enable(struct clk *clk, int enable)
{
	return gpHalApllClkEnable(enable);
}

#if NOT_USE
static int spll_setrate(struct clk *clk, unsigned long rate)
{
 //Undo 
 return clk->rate;
}
#endif //NOT_USE

static unsigned long spll_get_rate(struct clk *clk)
{
	unsigned int M,N,R;

	M = (SCUB_SPLL_CFG0 & 0x7C000000) >> 26;
	N = (SCUB_SPLL_CFG0 & 0x3FC) >> 2;
	
	if ((SCUB_SPLL_CFG0 & 0x2) == 0) {
		R = 8;
	}
	else {
		R = 4;
	}

	clk->rate = (XTAL_RATE/M) * N * R;	
	//printk("[%s][%d]clk = %s , rate=%d Hz\n",__FUNCTION__, __LINE__, (clk == NULL) ? "NULL" : clk->name ,clk->rate);
	return clk->rate;
}

static unsigned long spll2_get_rate(struct clk *clk)
{
	unsigned int M,N,R;
	
	M = (SCUB_SPLL_CFG2 & 0x1F);
	N = (SCUB_SPLL_CFG1 & 0xFF);
	
	if ((SCUB_SPLL_CFG0 & 0x800) == 0) {
		R = 8;
	}
	else {
		R = 4;
	}
	
	clk->rate = (XTAL_RATE/M) * N * R;	
	return clk->rate;
}


/* Clock for ARM Subsystem */

static unsigned long clk_ref_arm_get_rate(struct clk *clk)
{
    unsigned int asel;

	asel = (SCUB_SPLL_CFG0 & 0x70000) >> 16;
    switch (asel)
    {
      case 1:	  	
	  	  return 32.768 * 1000;
	  case 2:
      	  return (clk->parent->rate / 2);			
	  case 3:
      	  return (clk->parent->rate / 3);
      case 4:
      case 5:
      case 6 :			
	  case 7:
      	  return (clk->parent->rate / 1);
	  default:
	  	return XTAL_RATE;
	}
}

/**
* @brief 	apll clock enable function
* @param 	clk[in]: clock source
* @param 	rate[in]: clock rate to set , unit : HZ
* @return 	SUCCESS(0)/FAIL.
*/
int clk_ref_arm_set_rate(struct clk *clk, unsigned long rate)
{
    unsigned int asel;
    if (clk == NULL)
    	return -EINVAL;
	asel = (SCUB_SPLL_CFG0 & 0x70000) >> 16;
    switch (rate)
    {
      case 32768:	  	
	  	{
		   SCUB_SPLL_CFG0 &= ~(7<<16);				
		   SCUB_SPLL_CFG0 |= (1<<16);
	  	   clk->rate = 32.768 * 1000;		   
		   break;
      	}
	  case XTAL_RATE:
	  	{
		   SCUB_SPLL_CFG0 &= ~(7<<16);								
	  	   clk->rate = XTAL_RATE;		   
		   break;
	  	}
	  default:
	  	{
		  if(rate >= (clk->parent->rate/ 2))
		  {
		   SCUB_SPLL_CFG0 &= ~(7<<16);
		   SCUB_SPLL_CFG0 |= (2<<16);
	  	   clk->rate = clk->parent->rate/2;
		  }
		  else if(rate >= (clk->parent->rate/ 3))
		  {
		   SCUB_SPLL_CFG0 &= ~(7<<16);
		   SCUB_SPLL_CFG0 |= (3<<16);  
	  	   clk->rate = clk->parent->rate/3;
		  }
		  else
		  {
		   SCUB_SPLL_CFG0 &= ~(7<<16);
		   SCUB_SPLL_CFG0 |= (4<<16);			  
	  	   clk->rate = clk->parent->rate/1;
		  }
		  break;
	  	}
	}
   	return 0;
}

static unsigned long clk_arm_get_rate(struct clk *clk)
{
    unsigned int arm_ratio;
	unsigned long parent_rate;	
	
	parent_rate = clk_get_rate(clk->parent);	
	arm_ratio = (SCUB_ARM_RATIO & 0x3F);
	clk->rate = (parent_rate / (arm_ratio + 1));
    return clk->rate;
}

static int clk_arm_set_rate(struct clk *clk, unsigned long rate)
{
    unsigned int real_rate;
    int ret;

    if (clk == NULL)
    	return -EINVAL;
    if (clk->parent == NULL)
    	return -EINVAL;

	ret = gpHalClkArmSetRate(clk->parent->rate, rate, &real_rate);
	if (ret == 0)
		clk->rate = real_rate;	

    return ret;
}

static unsigned long clk_arm_ahb_get_rate(struct clk *clk)
{
    unsigned int arm_ahb_ratio;
	unsigned long parent_rate;	
	
	parent_rate = clk_get_rate(clk->parent);		
	arm_ahb_ratio = (SCUB_ARM_AHB_RATIO & 0x3F);
    return (parent_rate / (arm_ahb_ratio + 1));
}

static int clk_arm_ahb_set_rate(struct clk *clk, unsigned long rate)
{
    unsigned int real_rate;
	int ret;

    if (clk == NULL)
    	return -EINVAL;
    if (clk->parent == NULL)
    	return -EINVAL;

	ret = gpHalClkArmAhbSetRate(clk->parent->rate, rate, &real_rate);
	if (ret == 0)
		clk->rate = real_rate;	

    return ret;
}

static unsigned long clk_arm_apb_get_rate(struct clk *clk)
{
    unsigned int arm_apb_ratio;
	unsigned long parent_rate;	
	
	parent_rate = clk_get_rate(clk->parent);			
	arm_apb_ratio = (SCUB_ARM_APB_RATIO & 0x3F);
    return (parent_rate / (arm_apb_ratio + 1));
}

static int clk_arm_apb_set_rate(struct clk *clk, unsigned long rate)
{
    unsigned int real_rate;
	int ret;

    if (clk == NULL)
    	return -EINVAL;
    if (clk->parent == NULL)
    	return -EINVAL;

	ret = gpHalClkArmApbSetRate(clk->parent->rate, rate, &real_rate);
	if (ret == 0)
		clk->rate = real_rate;	

    return ret;
}


/* Clock for CEVA Subsystem */
/* @todo : do not on clock tree */
static unsigned long gp_clk_ref_ceva_get_rate(struct clk *clk)
{
    unsigned int csel;
	csel = (SCUB_SPLL_CFG0 & 0x7000) >> 12;
    switch (csel)
    {
      case 1:	  	
	  	  return 32768;
	  case 2:
      	  return (clk->parent->rate / 2);			
	  case 3:
      	  return (clk->parent->rate / 3);
      case 4:
      case 5:
      case 6:
	  case 7:			
      	  return (clk->parent->rate / 1);
	  default:
	  	return XTAL_RATE;
	}
}

static unsigned long clk_ceva_get_rate(struct clk *clk)
{
    unsigned int ceva_ratio;
	unsigned long parent_rate;	
	
	parent_rate = clk_get_rate(clk->parent);		
	ceva_ratio = (SCUC_CEVA_RATIO & 0x3F);
    return (parent_rate / (ceva_ratio + 1));
}

static int clk_ceva_set_rate(struct clk *clk, unsigned long rate)
{
    unsigned int real_rate;
	int ret;

    if (clk == NULL)
    	return -EINVAL;
    if (clk->parent == NULL)
    	return -EINVAL;

	ret = gpHalClkCevaSetRate(clk->parent->rate, rate, &real_rate);
	if (ret == 0)
		clk->rate = real_rate;	

    return ret;
}

static unsigned long clk_ceva_ahb_get_rate(struct clk *clk)
{
    unsigned int ceva_ahb_ratio;
	unsigned long parent_rate;	
	
	parent_rate = clk_get_rate(clk->parent);		
	ceva_ahb_ratio = (SCUC_CEVA_AHB_RATIO & 0x3F);
    return (parent_rate / (ceva_ahb_ratio + 1));
}

static int clk_ceva_ahb_set_rate(struct clk *clk, unsigned long rate)
{
    unsigned int real_rate;
	int ret;

    if (clk == NULL)
    	return -EINVAL;
    if (clk->parent == NULL)
    	return -EINVAL;

	ret = gpHalClkCevaAhbSetRate(clk->parent->rate, rate, &real_rate);
	if (ret == 0)
		clk->rate = real_rate;	

    return ret;
}

static unsigned long clk_ceva_apb_get_rate(struct clk *clk)
{
    unsigned int ceva_apb_ratio;
	unsigned long parent_rate;	
	
	parent_rate = clk_get_rate(clk->parent);		
	ceva_apb_ratio = (SCUC_CEVA_APB_RATIO & 0x3F);
    return (parent_rate / (ceva_apb_ratio + 1));
}

static int clk_ceva_apb_set_rate(struct clk *clk, unsigned long rate)
{
    unsigned int real_rate;
	int ret;

    if (clk == NULL)
    	return -EINVAL;
    if (clk->parent == NULL)
    	return -EINVAL;

	ret = gpHalClkCevaApbSetRate(clk->parent->rate, rate, &real_rate);
	if (ret == 0)
		clk->rate = real_rate;	

    return ret;
}


/* Clock for SYS Subsystem */

static unsigned long clk_sys_get_rate(struct clk *clk)
{
    unsigned int sys_ratio;
	unsigned long parent_rate;	
	
	parent_rate = clk_get_rate(clk->parent);	
	sys_ratio = (SCUC_SYS_RATIO & 0x3F);
    return (parent_rate / (sys_ratio + 1));
}


static unsigned long clk_sys_ahb_get_rate(struct clk *clk)
{
    unsigned int sys_ahb_ratio;
	unsigned long parent_rate;	
	
	parent_rate = clk_get_rate(clk->parent);	
	sys_ahb_ratio = (SCUC_SYS_AHB_RATIO & 0x3F);
    return (parent_rate / (sys_ahb_ratio + 1));
}

static int clk_sys_ahb_set_rate(struct clk *clk, unsigned long rate)
{
    unsigned int real_rate;
	int ret;

    if (clk == NULL)
    	return -EINVAL;
    if (clk->parent == NULL)
    	return -EINVAL;

	ret = gpHalClkSysAhbSetRate(clk->parent->rate, rate, &real_rate);
	if (ret == 0)
		clk->rate = real_rate;	

    return ret;
}

static unsigned long clk_sys_apb_get_rate(struct clk *clk)
{
    unsigned int sys_apb_ratio;
	unsigned long parent_rate;	
	
	parent_rate = clk_get_rate(clk->parent);	
	//printk("[%s][%d] parrent = %s , parent_rate=%d Hz\n",__FUNCTION__, __LINE__,
	// (clk->parent == NULL) ? "NULL" : clk->parent->name ,parent_rate);
	sys_apb_ratio = (SCUC_SYS_APB_RATIO & 0x3F);
    return (parent_rate / (sys_apb_ratio + 1));
}

static int clk_sys_apb_set_rate(struct clk *clk, unsigned long rate)
{
    unsigned int real_rate;
	int ret;

    if (clk == NULL)
    	return -EINVAL;
    if (clk->parent == NULL)
    	return -EINVAL;

	ret = gpHalClkSysApbSetRate(clk->parent->rate, rate, &real_rate);
	if (ret == 0)
		clk->rate = real_rate;	

    return ret;
}

static unsigned long clk_lcd_get_src_rate(struct clk *clk)
{
	unsigned int lcd_cfg;
	unsigned long src_rate;	
	
	lcd_cfg = 	(SCUA_LCD_CLK_CFG >> 16)& 0x7;
	if (lcd_cfg == 0x0)
		src_rate = clk_get_rate(clk->parent);	
	else if (lcd_cfg == 0x6)
		src_rate = USBPHY_RATE;
	else
		src_rate = XTAL_RATE;

    return src_rate;

}

static unsigned long clk_lcd_get_rate(struct clk *clk)
{
    unsigned int lcd_cfg;
	unsigned long src_rate;

	src_rate = clk_lcd_get_src_rate(clk);
	lcd_cfg = (SCUA_LCD_CLK_CFG & 0xFF);
    return (src_rate / (lcd_cfg + 1));
}

static int clk_lcd_set_rate(struct clk *clk, unsigned long rate)
{
    unsigned int real_rate;
	unsigned long src_rate;	
	int ret;

    if (clk == NULL)
    	return -EINVAL;

	src_rate = clk_lcd_get_src_rate(clk);
	ret = gpHalClkLcdSetRate(src_rate, rate, &real_rate);
	if (ret == 0)
		clk->rate = real_rate;	

    return ret;
}

static int  clk_uclk_set_rate(struct clk *clk, unsigned long rate)
{
    unsigned int uart_cfg;
	unsigned long parent_rate;	
	unsigned long setrate;

	//only support external crysital(27MHz)
	//if (rate != XTAL_RATE) {
	if (0) {
		parent_rate = clk_get_rate(clk->parent);	
		// force change rate to this
		setrate = (13 * (parent_rate / XTAL_RATE)) -1;
		SCUA_UART_CFG = (setrate &0xFF);
		udelay(10);
		SCUA_UART_CFG |= 0x100;
		udelay(10);	
		uart_cfg = (SCUA_UART_CFG & 0xFF);
    	return (parent_rate / (uart_cfg + 1));
	}
	else {
		SCUA_UART_CFG = 0;
		SCUA_UART_CFG |= 0x50000; 
		SCUA_UART_CFG |= 0x100;
		return 115200;
	}
}

static unsigned long clk_uclk_get_rate(struct clk *clk)
{
    unsigned int uart_cfg;
	unsigned long parent_rate;	
	
	if (SCUA_UART_CFG & 0x40000) {
		return XTAL_RATE;	
	}
	else {
		parent_rate = clk_get_rate(clk->parent);	
		uart_cfg = (SCUA_UART_CFG & 0xFF);
    	return (parent_rate / (uart_cfg + 1));
	}
}

static unsigned long clk_csi_get_rate(struct clk *clk)
{
    unsigned int csi_cfg;
	unsigned long parent_rate;	
	
	parent_rate = clk_get_rate(clk->parent);	
	csi_cfg = (SCUA_CSI_CLK_CFG & 0xFF);
    return (parent_rate / (csi_cfg + 1));
}

static int clk_csi_set_rate(struct clk *clk, unsigned long rate)
{
    unsigned int real_rate;
	int ret;

    if (clk == NULL)
    	return -EINVAL;
    if (clk->parent == NULL)
    	return -EINVAL;

	ret = gpHalClkCsiSetRate(clk->parent->rate, rate, &real_rate);
	if (ret == 0)
		clk->rate = real_rate;	

    return ret;
}

static unsigned long clk_i2s_mclk_get_rate(struct clk *clk)
{
    unsigned int app_cfg, da_ratio;
	unsigned long parent_rate;	
	
	parent_rate = clk_get_rate(clk->parent);		
	app_cfg = (SCUA_APLL_CFG & 0x70000) >> 16;
	switch (app_cfg)
	{
	  case 6: da_ratio = 32; break;
	  case 5: da_ratio = 24; break;
	  case 4: da_ratio = 18; break;
	  case 3: da_ratio = 12; break;
	  case 2: da_ratio = 9;  break;
	  case 1: da_ratio = 6;  break;
	  case 0: da_ratio = 3;  break;
	  default : da_ratio = 1;
              
	}
    return (parent_rate / da_ratio);
}

static int clk_i2s_mclk_set_rate(struct clk *clk, unsigned long rate)
{
    unsigned int real_rate;
	int ret;

    if (clk == NULL)
    	return -EINVAL;
    if (clk->parent == NULL)
    	return -EINVAL;

	ret = gpHalClkI2sSetRate(clk->parent->rate, rate, &real_rate);
	if (ret == 0)
		clk->rate = real_rate;	

    return ret;
}
/* @todo:check all clock source enable function */
static struct clk xtal =
{
	.name		= "xtal",
	.id			= -1,
	.rate		= XTAL_RATE,
	.parent		= NULL,
	.usage		= 0,
}
;

#if NOT_USE
static struct clk clk_rtc =
{
	.name		= "clk_rtc",
	.id			= -1,
	.rate		= RTC_RATE,
	.parent		= NULL,
	.ctrlbit	= 0,
};
#endif //NOT_USE


static struct clk clk_apll =
{
	.name		= "apll",
	.id			= -1,
	.parent		= NULL,
	.set_rate	= apll_set_rate,	
	.get_rate	= apll_get_rate,		
	.enable		= apll_clk_enable,
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_BASE_SYS,
	.ctrlbit	= 0xFF,
};

static struct clk clk_spll =
{
	.name		= "spll",
	.id			= -1,	
	.parent		= NULL,
	.set_rate	= clk_default_setrate,	/* @todo : implement in sram */
	.get_rate	= spll_get_rate,		
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_BASE_SYS,
	.ctrlbit	= 0xFF,
};

static struct clk clk_spll2 =
{
	.name		= "spll2",
	.id			= -1,	
	.parent		= NULL,
	.set_rate	= clk_default_setrate,	
	.get_rate	= spll2_get_rate,		
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_BASE_SYS,
	.ctrlbit	= 0xFF,
};

static struct clk clk_ref_arm =
{
	.name		= "clk_ref_arm",
	.id			= -1,
	.parent		= &clk_spll,
	.set_rate	= clk_ref_arm_set_rate,	
	.get_rate	= clk_ref_arm_get_rate,		
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_BASE_SYS,
	.ctrlbit	= 0xFF,
};

struct clk clk_arm =
{
	.name		= "clk_arm",
	.id			= -1,
	.parent		= &clk_ref_arm,	
	.set_rate	= clk_arm_set_rate,
	.get_rate	= clk_arm_get_rate,	
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_BASE_SYS,
	.ctrlbit	= 0xFF,
};

struct clk clk_arm_ahb =
{
	.name		= "clk_arm_ahb",
	.id			= -1,
	.parent		= &clk_arm,	
	.set_rate	= clk_arm_ahb_set_rate,
	.get_rate	= clk_arm_ahb_get_rate,		
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_BASE_SYS,
	.ctrlbit	= 0xFF,
};

struct clk clk_arm_apb =
{
	.name		= "clk_arm_apb",
	.id			= -1,
	.parent		= &clk_arm_ahb,	
	.set_rate	= clk_arm_apb_set_rate,
	.get_rate	= clk_arm_apb_get_rate,		
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_BASE_SYS,
	.ctrlbit	= 0xFF,
};

static struct clk clk_ref_ceva =
{
	.name		= "clk_ref_ceva",
	.id			= -1,
	.rate		= 0,
	.parent		= &clk_spll,	
	.get_rate	= gp_clk_ref_ceva_get_rate,		
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_BASE_CEVA,
	.ctrlbit	= 0xFF,
};


struct clk clk_ceva =
{
	.name		= "clk_ceva",
	.id			= -1,
	.parent		= &clk_ref_ceva,	
	.set_rate	= clk_ceva_set_rate,
	.get_rate	= clk_ceva_get_rate,		
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_BASE_CEVA,
	.ctrlbit	= CEVA_EN,
};

struct clk clk_ceva_ahb =
{
	.name		= "clk_ceva_ahb",
	.id			= -1,
	.parent		= &clk_ceva,	
	.set_rate	= clk_ceva_ahb_set_rate,
	.get_rate	= clk_ceva_ahb_get_rate,		
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_BASE_CEVA,
	.ctrlbit	= CEVA_AHB_EN,
};


struct clk clk_ceva_apb =
{
	.name		= "clk_ceva_apb",
	.id			= -1,
	.parent		= &clk_ceva_ahb,	
	.set_rate	= clk_ceva_apb_set_rate,
	.get_rate	= clk_ceva_apb_get_rate,		
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_BASE_CEVA,
	.ctrlbit	= CEVA_APB_EN,
};

struct clk clk_sys =
{
	.name		= "clk_sys",
	.id			= -1,
	.parent		= &clk_ref_ceva,	
	.set_rate	= clk_default_setrate, /* @todo: implement on sram */
	.get_rate	= clk_sys_get_rate,		
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_BASE_SYS,
	.ctrlbit	= SYS_EN,
};

struct clk clk_sys_ahb =
{
	.name		= "clk_sys_ahb",
	.id			= -1,
	.parent		= &clk_sys,	
	.set_rate	= clk_sys_ahb_set_rate,
	.get_rate	= clk_sys_ahb_get_rate,			
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_BASE_SYS,
	.ctrlbit	= SYS_AHB_EN,
};

struct clk clk_sys_apb =
{
	.name		= "clk_sys_apb",
	.id			= -1,
	.parent		= &clk_sys_ahb,	
	.set_rate	= clk_sys_apb_set_rate,
	.get_rate	= clk_sys_apb_get_rate,				
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_BASE_SYS,
	.ctrlbit	= SYS_APB_EN,
};


struct clk clk_lcd =
{
	.name		= "clk_lcd",
	.id			= -1,
	.parent		= &clk_ref_ceva,	
	.set_rate	= clk_lcd_set_rate,
	.get_rate	= clk_lcd_get_rate,
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_A,
	.ctrlbit	= SCU_A_PERI_LCD_CTRL,
};


struct clk clk_uart =
{
	.name		= "clk_uart",
	.id			= -1,
	.parent		= &clk_ref_ceva,	
	.set_rate	= clk_uclk_set_rate,
	.get_rate	= clk_uclk_get_rate,						
};


struct clk clk_csi =
{
	.name		= "clk_csi",
	.id			= -1,
	.parent		= &clk_ref_ceva,	
	.set_rate	= clk_csi_set_rate,
	.get_rate	= clk_csi_get_rate,						
};

struct clk clk_i2s_mclk =
{
	.name		= "clk_i2s_mclk",
	.id			= -1,
	.parent		= &clk_apll,	
	.set_rate	= clk_lcd_set_rate,
	.get_rate	= clk_lcd_get_rate,						
	.usage		= 0,
	.enable_func= gpHalScuClkEnable,
	.clock_class= SCU_A,
	.ctrlbit	= SCU_A_PERI_LCD_CTRL,
	.set_rate	= clk_i2s_mclk_set_rate,
	.get_rate	= clk_i2s_mclk_get_rate,					
};

static struct clk *init_src_clocks[] = {
	&xtal,
	&clk_apll,
	&clk_spll,
	&clk_spll2,
	&clk_ref_arm,
	&clk_arm,
	&clk_arm_ahb,
	&clk_arm_apb,
	&clk_ref_ceva,
	&clk_ceva,
	&clk_ceva_ahb,
	&clk_ceva_apb,
	&clk_sys,
	&clk_sys_ahb,
	&clk_sys_apb,
	&clk_lcd,
	&clk_uart,
	&clk_csi,
	&clk_i2s_mclk,
};

// device clock list
gp_clk_t *device_clocks[] = {
	&clk_lcd_ctrl,
	&clk_ppu_spr,
	&clk_ppu,
	&clk_ppu_reg1,
	&clk_sys_a,
	&clk_nand_abt,
	&clk_rtabt212,
	&clk_realtimeabt,
	&clk_apbdma_a,
	&clk_2dscaabt,
	&clk_ppu_reg,
	&clk_ppu_fb,
	&clk_ppu_tv,
	&clk_apbdma_c,
};

// for test
static struct clk *test_src_clocks[] = {
	//&xtal,
#if 1
	&clk_spll2,
	&clk_spll,
	&clk_apll,
	//&clk_ref_arm,
	&clk_arm_apb,
	&clk_arm_ahb,
	&clk_arm,
	//&clk_ref_ceva,
	&clk_ceva_apb,
	&clk_ceva_ahb,
	&clk_ceva,
#endif
	&clk_sys_apb,
	&clk_sys_ahb,
	&clk_sys,
	&clk_lcd,
	&clk_uart,
	&clk_csi,
};

// for test
static struct clk *arm_src_clocks[] = {
	//&clk_ref_arm,
	&clk_arm_apb,
	&clk_arm_ahb,
	&clk_arm,
};

// for test
static struct clk *ceva_src_clocks[] = {
	//&clk_ref_ceva,
	&clk_ceva_apb,
	&clk_ceva_ahb,
	&clk_ceva,
};

// for test
static struct clk *sys_src_clocks[] = {
	&clk_sys_apb,
	&clk_sys_ahb,
	&clk_sys,
};

/* initialise the clock system */

int gp_register_clock(struct clk *clk)
{
	clk->owner = THIS_MODULE;

	if (clk->enable == NULL)
		clk->enable = clk_null_enable;

	/* add to the list of available clocks */

	/* Quick check to see if this clock has already been registered. */
	BUG_ON(clk->list.prev != clk->list.next);

	spin_lock(&clocks_lock);
	list_add(&clk->list, &clocks);
	spin_unlock(&clocks_lock);

	return 0;
}

int gp_register_clocks(struct clk **clks, int nr_clks)
{
	int fails = 0;

	for (; nr_clks > 0; nr_clks--, clks++) {
		if (gp_register_clock(*clks) < 0)
			fails++;
	}

	return fails;
}

/* initalise all the clocks */

static void __init gp_init_clock(struct clk *clk)
{
    if (strcmp(clk->name,"clk_ref_arm") == 0) {
    	if ((SCUB_SPLL_CFG1 & (1<<24)) == 0) {
    		clk->parent = &clk_spll; /* arm use spll */
    	}
    	else {
    		clk->parent = &clk_spll2; /* arm use spll2 */
    	}
    }
    clk->rate = clk_get_rate(clk);
}


int __init gp_register_baseclocks(unsigned long xtal)
{
	struct clk **clkp;
	int ret;	
	for (clkp = init_src_clocks; clkp < init_src_clocks + ARRAY_SIZE(init_src_clocks); clkp++)
	{
		/* ensure that we note the clock state */
		
		ret = gp_register_clock(*clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       (*clkp)->name, ret);
		}
	}	

	//register device clock
	for (clkp = device_clocks; clkp < device_clocks + ARRAY_SIZE(device_clocks); clkp++)
	{
		/* ensure that we note the clock state */
		
		ret = gp_register_clock(*clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register device clock %s (%d)\n",
			       (*clkp)->name, ret);
		}
	}	
	
	return 0;
}

void __init gp_setup_clocks(void)
{

	struct clk **clkp;
	
	for (clkp = init_src_clocks; clkp < init_src_clocks + ARRAY_SIZE(init_src_clocks); clkp++)
	{
		/* ensure that we note the clock state */
		
		gp_init_clock(*clkp);		
	}	
	
}


void __init dumpclk(void)
{
	struct clk *p;
	struct clk **clkp;
	spin_lock(&clocks_lock);


	/* check for the case where a device was supplied, but the
	 * clock that was being searched for is not device specific */

	for (clkp = init_src_clocks; clkp < init_src_clocks + ARRAY_SIZE(init_src_clocks); clkp++)
	{
		p = *clkp;
		//printk("[%s] parrent = %s , rate=%d Hz\n",p->name,  (p->parent == NULL) ? "NULL" : p->parent->name ,(int)clk_get_rate(p));
		printk("[%s] parrent = %s , rate=%d Hz, usage=%d\n",p->name,  (p->parent == NULL) ? "NULL" : p->parent->name ,(int)clk_get_rate(p), p->usage);
	}	
#if 0 
	list_for_each_entry(p, &clocks, list) {
		printk("[%s] usage=%d\n",p->name, p->usage);
	}
#endif

	spin_unlock(&clocks_lock);
}

EXPORT_SYMBOL(clk_ref_arm_set_rate);


#if 0 //for test
/**
 * @brief   clock rate change function.
 * @param   clock_tree[in]: clock_tree array
 * @return  SUCCESS(0)/ERROR_ID
 */
int gp_all_clock_change(int clk_sel, int src)
{
	unsigned int rate, new_rate;
	printk("[%s][%d] run : 0x%x\n", __FUNCTION__, __LINE__, clk_sel);
	//hal set clock value
	//gpHalClockChange(clk_sel);
		
	struct clk **clkp;
	int ret;
	for (clkp = test_src_clocks; clkp < test_src_clocks + ARRAY_SIZE(test_src_clocks); clkp++)
	{
		//struct clk *clkParent = ((*clkp)->parent);
		/* ensure that we note the clock state */
		rate = clk_get_rate(*clkp);
		if (rate == 0) {
			printk(KERN_ERR "Failed to get clock %s (%d)\n",
			       (*clkp)->name, ret);
			return 0;
		}
		// test for half clock divide
		new_rate = rate >> 1;
		printk("[%s][%d] set [%s] rate=%d, new_rate=%d\n", __FUNCTION__, __LINE__, (*clkp)->name, rate, new_rate);
		ret = clk_set_rate(*clkp, new_rate);
		if (ret < 0) {
			printk(KERN_ERR "Failed to set new rate at %s (%d)\n",
			       (*clkp)->name, ret);
		}
#if 1
		rate = clk_get_rate(*clkp);
		if (rate == 0) {
			printk(KERN_ERR "Failed to get clock %s (%d)\n",
			       (*clkp)->name, ret);
			return 0;
		}
#endif
		printk("[%s][%d] After update, [%s] rate=%d\n", __FUNCTION__, __LINE__, (*clkp)->name, rate);
		
	}	

	return 0;
}

#else
/**
 * @brief   clock rate change function.
 * @param   clock_tree[in]: clock_tree array
 * @return  SUCCESS(0)/ERROR_ID
 */
int gp_all_clock_change(int clk_sel, int src)
{
	unsigned int rate, new_rate;
	struct clk **clkp;
	struct clk **op, **cond;
	int ret;

	printk("[%s][%d] run : 0x%x\n", __FUNCTION__, __LINE__, clk_sel);
	
	switch(src) {
	case 1:
		op = arm_src_clocks;
		cond = arm_src_clocks + ARRAY_SIZE(arm_src_clocks);
		break;
	case 2:
		op = ceva_src_clocks;
		cond = ceva_src_clocks + ARRAY_SIZE(ceva_src_clocks);
		break;
	case 3:
		op = sys_src_clocks;
		cond = sys_src_clocks + ARRAY_SIZE(sys_src_clocks);
		break;
	default :
		op = test_src_clocks;
		cond = test_src_clocks + ARRAY_SIZE(test_src_clocks);
		break;
	}
	
	//for (clkp = test_src_clocks; clkp < test_src_clocks + ARRAY_SIZE(test_src_clocks); clkp++)
	for (clkp = op; clkp < cond; clkp++)
	{
		rate = clk_get_rate(*clkp);
		if (rate == 0) {
			printk(KERN_ERR "Failed to get clock %s\n",
			       (*clkp)->name);
			return 0;
		}
		new_rate = rate/clk_sel;
		printk("[%s][%d] Set [%s] rate=%d, new_rate=%d\n", __FUNCTION__, __LINE__, (*clkp)->name, rate, new_rate);
		ret = clk_set_rate(*clkp, new_rate);
		if (ret < 0) {
			printk(KERN_ERR "Failed to set new rate at %s (%d)\n",
			       (*clkp)->name, ret);
		}
		rate = clk_get_rate(*clkp);
		if (rate == 0) {
			printk(KERN_ERR "Failed to get clock %s (%d)\n",
			       (*clkp)->name, ret);
			return 0;
		}
		printk("[%s][%d] After update, [%s] rate=%d\n", __FUNCTION__, __LINE__, (*clkp)->name, rate);
		
	}	

	return 0;
}
#endif
EXPORT_SYMBOL(gp_all_clock_change);

/**
 * @brief   clock rate change function.
 * @param 	clock_name[in]: base/device clock name
 * @return  SUCCESS(0)/ERROR_ID
 */
int gp_dump_clock_usage(int *clock_name)
{
	gp_clk_t *clkp;

	if (clock_name == 0) {
		for (clkp = (gp_clk_t *)init_src_clocks; clkp < (gp_clk_t *)init_src_clocks + ARRAY_SIZE(init_src_clocks); clkp++) {
			list_for_each_entry(clkp, &clocks, list) {
				printk("[%s] usage=%d\n",clkp->name, clkp->usage);
			}
		}
	}
	else {
		printk("[%s][%d] run, clock_name=[%s]\n", __FUNCTION__, __LINE__, (char *)clock_name);
	
		clkp = clk_get(NULL, (char *)clock_name);
		if (IS_ERR(clkp) || clkp == NULL){
			printk(KERN_ERR "Failed to Get clock %s\n",(char *)clock_name);
	    	return -ENOENT;
	    }
	    printk("[%s] usage=%d\n",clkp->name, clkp->usage);
	}
	return 0;

}
EXPORT_SYMBOL(gp_dump_clock_usage);