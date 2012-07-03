#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/diag.h>
#include <mach/hardware.h>
#include <mach/spmp_clock.h>
#include <mach/regs-scu.h>


#define NOT_USE		0
#define XTAL_RATE 27000000  //27 Mhz
#define RTC_RATE  32768     //32.768Khz
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

	spin_lock(&clocks_lock);

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

	spin_unlock(&clocks_lock);
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

	spin_lock(&clocks_lock);

	if ((clk->usage++) == 0)
	{
	   if(clk->enable)
	   	{
     		(clk->enable)(clk, 1);
	   	}
	}
	spin_unlock(&clocks_lock);
	return 0;
}

void clk_disable(struct clk *clk)
{
	if (IS_ERR(clk) || clk == NULL)
		return;

	spin_lock(&clocks_lock);

	if (!(clk->usage <=0) && (--clk->usage) == 0)
	{
		(clk->enable)(clk, 0);
	}
    if(clk->usage <0)  printk("error clk_disable \n");
	spin_unlock(&clocks_lock);
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
}


unsigned long clk_get_rate(struct clk *clk)
{
	if (IS_ERR(clk))
		return 0;

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

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	int ret;

	if (IS_ERR(clk))
		return -EINVAL;

	/* We do not default just do a clk->rate = rate as
	 * the clock may have been made this way by choice.
	 */

	WARN_ON(clk->set_rate == NULL);

	if (clk->set_rate == NULL)
		return -EINVAL;

	spin_lock(&clocks_lock);
	ret = (clk->set_rate)(clk, rate);
	spin_unlock(&clocks_lock);

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
	clk->rate = rate;
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

#if NOT_USE
static int spll_setrate(struct clk *clk, unsigned long rate)
{
 //Undo 
 return clk->rate;
}
#endif //NOT_USE

static unsigned long spll_get_rate(struct clk *clk)
{
	unsigned int OD,F,R;
	OD = (SCUB_SPLL_CFG & 0x200) >> 9;
	F  = (SCUB_SPLL_CFG & 0xFC)  >> 2;
	R  = (SCUB_SPLL_CFG & 0x03);
	clk->rate = ((XTAL_RATE * (F+1)) / ((R+1) * (OD+1)));	
	return clk->rate;
}


/* Clock for ARM Subsystem */

static unsigned long clk_ref_arm_get_rate(struct clk *clk)
{
    unsigned int asel;

	asel = (SCUB_SPLL_CFG & 0x70000) >> 16;
    switch (asel)
    {
      case 1:	  	
	  	  return 32.768 * 1000;
	  case 2:
      	  return (clk->parent->rate / 2);			
	  case 3:
      	  return (clk->parent->rate / 3);			
	  case 7:
      	  return (clk->parent->rate / 1);
	  default:
	  	return XTAL_RATE;
	}
}

#if NOT_USE
static int clk_ref_arm_setrate(struct clk *clk, unsigned long rate)
{
    unsigned int asel;
	asel = (SCUB_SPLL_CFG & 0x70000) >> 16;
    switch (rate)
    {
      case 32768:	  	
	  	{
		   SCUB_SPLL_CFG &= ~(7<<16);				
		   SCUB_SPLL_CFG |= (1<<16);
	  	   clk->rate = 32.768 * 1000;		   
		   break;
      	}
	  case XTAL_RATE:
	  	{
		   SCUB_SPLL_CFG &= ~(7<<16);								
	  	   clk->rate = XTAL_RATE;		   
		   break;
	  	}
	  default:
	  	{
		  if(rate >= (clk->parent->rate/ 2))
		  {
		   SCUB_SPLL_CFG &= ~(7<<16);
		   SCUB_SPLL_CFG |= (2<<15);
	  	   clk->rate = clk->parent->rate/2;
		  }
		  else if(rate >= (clk->parent->rate/ 3))
		  {
		   SCUB_SPLL_CFG &= ~(7<<16);
		   SCUB_SPLL_CFG |= (3<<15);  
	  	   clk->rate = clk->parent->rate/3;
		  }
		  else
		  {
		   SCUB_SPLL_CFG &= ~(7<<16);
		   SCUB_SPLL_CFG |= (4<<15);			  
	  	   clk->rate = clk->parent->rate/1;
		  }
		  break;
	  	}
	}
    return clk->rate;   
}
#endif //NOT_USE

static unsigned long clk_arm_get_rate(struct clk *clk)
{
    unsigned int arm_ratio;
	unsigned long parent_rate;	
	
	parent_rate = clk_get_rate(clk->parent);	
	arm_ratio = (SCUB_ARM_RATIO & 0x3F);
	clk->rate = (parent_rate / (arm_ratio + 1));
    return clk->rate;
}

#if NOT_USE
static int clk_arm_setrate(struct clk *clk, unsigned long rate)
{
    unsigned int arm_ratio;
	arm_ratio = ((clk->parent->rate/ rate) -1 ) & 0x3f;    
	SCUB_ARM_RATIO = arm_ratio;
	clk->rate = (clk->parent->rate / (arm_ratio + 1));	
    return clk->rate;
}
#endif //NOT_USE

static unsigned long clk_arm_ahb_get_rate(struct clk *clk)
{
    unsigned int arm_ahb_ratio;
	unsigned long parent_rate;	
	
	parent_rate = clk_get_rate(clk->parent);		
	arm_ahb_ratio = (SCUB_ARM_AHB_RATIO & 0x3F);
    return (parent_rate / (arm_ahb_ratio + 1));
}

#if NOT_USE
static int clk_arm_ahb_setrate(struct clk *clk, unsigned long rate)
{
    unsigned int arm_ahb_ratio;
	arm_ahb_ratio = ((clk->parent->rate/ rate) -1 ) & 0x3f;    
	SCUB_ARM_AHB_RATIO = arm_ahb_ratio;
	clk->rate = (clk->parent->rate / (arm_ahb_ratio + 1));	
    return clk->rate;
}
#endif //NOT_USE

static unsigned long clk_arm_apb_get_rate(struct clk *clk)
{
    unsigned int arm_apb_ratio;
	unsigned long parent_rate;	
	
	parent_rate = clk_get_rate(clk->parent);			
	arm_apb_ratio = (SCUB_ARM_APB_RATIO & 0x3F);
    return (parent_rate / (arm_apb_ratio + 1));
}

#if NOT_USE
static int clk_arm_apb_setrate(struct clk *clk, unsigned long rate)
{
    unsigned int arm_apb_ratio;
	arm_apb_ratio = ((clk->parent->rate/ rate) -1 ) & 0x3f;    
	SCUB_ARM_APB_RATIO = arm_apb_ratio;
	clk->rate = (clk->parent->rate / (arm_apb_ratio + 1));	
    return clk->rate;
}
#endif //NOT_USE

/* Clock for CEVA Subsystem */
static unsigned long spmp_clk_ref_ceva_get_rate(struct clk *clk)
{
    unsigned int csel;
	csel = (SCUB_SPLL_CFG & 0x7000) >> 12;
    switch (csel)
    {
      case 1:	  	
	  	  return 32768;
	  case 2:
      	  return (clk->parent->rate / 2);			
	  case 3:
      	  return (clk->parent->rate / 3);
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

#if NOT_USE
static int clk_ceva_setrate(struct clk *clk, unsigned long rate)
{
    unsigned int ceva_ratio;
	ceva_ratio = ((clk->parent->rate/ rate) -1 ) & 0x3f;    
	SCUC_CEVA_RATIO = ceva_ratio;
	clk->rate = (clk->parent->rate / (ceva_ratio + 1));	
    return clk->rate;
}
#endif //NOT_USE

static unsigned long clk_ceva_ahb_get_rate(struct clk *clk)
{
    unsigned int ceva_ahb_ratio;
	unsigned long parent_rate;	
	
	parent_rate = clk_get_rate(clk->parent);		
	ceva_ahb_ratio = (SCUC_CEVA_AHB_RATIO & 0x3F);
    return (parent_rate / (ceva_ahb_ratio + 1));
}

#if NOT_USE
static int clk_ceva_ahb_setrate(struct clk *clk, unsigned long rate)
{
    unsigned int ceva_ahb_ratio;
	ceva_ahb_ratio = ((clk->parent->rate/ rate) -1 ) & 0x3f;    
	SCUC_CEVA_AHB_RATIO = ceva_ahb_ratio;
	clk->rate = (clk->parent->rate / (ceva_ahb_ratio + 1));	
    return clk->rate;
}
#endif //NOT_USE

static unsigned long clk_ceva_apb_get_rate(struct clk *clk)
{
    unsigned int ceva_apb_ratio;
	unsigned long parent_rate;	
	
	parent_rate = clk_get_rate(clk->parent);		
	ceva_apb_ratio = (SCUC_CEVA_APB_RATIO & 0x3F);
    return (parent_rate / (ceva_apb_ratio + 1));
}

#if NOT_USE
static int clk_ceva_apb_setrate(struct clk *clk, unsigned long rate)
{
    unsigned int arm_apb_ratio;
	arm_apb_ratio = ((clk->parent->rate/ rate) -1 ) & 0x3f;    
	SCUC_CEVA_APB_RATIO = arm_apb_ratio;
	clk->rate = (clk->parent->rate / (arm_apb_ratio + 1));	
    return clk->rate;
}
#endif //NOT_USE
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

#if NOT_USE
static int clk_sys_ahb_setrate(struct clk *clk, unsigned long rate)
{
    unsigned int sys_ahb_ratio;
	sys_ahb_ratio = ((clk->parent->rate/ rate) -1 ) & 0x3f;    
	SCUC_SYS_AHB_RATIO = sys_ahb_ratio;
	clk->rate = (clk->parent->rate / (sys_ahb_ratio + 1));	
    return clk->rate;
}
#endif //NOT_USE

static unsigned long clk_sys_apb_get_rate(struct clk *clk)
{
    unsigned int sys_apb_ratio;
	unsigned long parent_rate;	
	
	parent_rate = clk_get_rate(clk->parent);	
	sys_apb_ratio = (SCUC_SYS_APB_RATIO & 0x3F);
    return (parent_rate / (sys_apb_ratio + 1));
}

#if NOT_USE
static int clk_sys_apb_setrate(struct clk *clk, unsigned long rate)
{
    unsigned int sys_apb_ratio;
	sys_apb_ratio = ((clk->parent->rate/ rate) -1 ) & 0x3f;    
	SCUC_SYS_APB_RATIO = sys_apb_ratio;
	clk->rate = (clk->parent->rate / (sys_apb_ratio + 1));	
    return clk->rate;
}
#endif //NOT_USE

static unsigned long clk_lcd_get_rate(struct clk *clk)
{
    unsigned int lcd_cfg;
	unsigned long parent_rate;	
	
	parent_rate = clk_get_rate(clk->parent);	
	lcd_cfg = (SCUA_LCD_CLK_CFG & 0xFF);
    return (parent_rate / (lcd_cfg + 1));
}

static int  clk_uclk_set_rate(struct clk *clk, unsigned long rate)
{
    unsigned int uart_cfg;
	unsigned long parent_rate;	
	unsigned long setrate;
	parent_rate = clk_get_rate(clk->parent);	
	// force change rate to this
	setrate = (13 * (parent_rate / 27000000)) -1;
	SCUA_UART_CFG = (setrate &0xFF);
	udelay(10);
	SCUA_UART_CFG |= 0x100;
	udelay(10);	
	uart_cfg = (SCUA_UART_CFG & 0xFF);
    return (parent_rate / (uart_cfg + 1));
}

static unsigned long clk_uclk_get_rate(struct clk *clk)
{
    unsigned int uart_cfg;
	unsigned long parent_rate;	
	
	parent_rate = clk_get_rate(clk->parent);	
	uart_cfg = (SCUA_UART_CFG & 0xFF);
    return (parent_rate / (uart_cfg + 1));
}

static unsigned long clk_csi_get_rate(struct clk *clk)
{
    unsigned int csi_cfg;
	unsigned long parent_rate;	
	
	parent_rate = clk_get_rate(clk->parent);	
	csi_cfg = (SCUA_CSI_CLK_CFG & 0xFF);
    return (parent_rate / (csi_cfg + 1));
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

static unsigned long clk_i2s_bck_get_rate(struct clk *clk)
{
    unsigned int bck_cfg;
	unsigned long parent_rate;	
	
	parent_rate = clk_get_rate(clk->parent);		
	bck_cfg = (SCUA_I2S_BCK_CFG & 0xFF);
    return (parent_rate / (bck_cfg + 1));
}

static struct clk xtal =
{
	.name		= "xtal",
	.id			= -1,
	.rate		= XTAL_RATE,
	.parent		= NULL,
	.ctrlbit	= 0,
};

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
	.set_rate	= clk_default_setrate,	
	.get_rate	= apll_get_rate,		
};

static struct clk clk_spll =
{
	.name		= "spll",
	.id			= -1,	
	.parent		= NULL,
	.set_rate	= clk_default_setrate,	
	.get_rate	= spll_get_rate,		
};

static struct clk clk_ref_arm =
{
	.name		= "clk_ref_arm",
	.id			= -1,
	.parent		= &clk_spll,
	.set_rate	= clk_default_setrate,	
	.get_rate	= clk_ref_arm_get_rate,		
};

struct clk clk_arm =
{
	.name		= "clk_arm",
	.id			= -1,
	.parent		= &clk_ref_arm,	
	.set_rate	= clk_default_setrate,
	.get_rate	= clk_arm_get_rate,	
};

struct clk clk_arm_ahb =
{
	.name		= "clk_arm_ahb",
	.id			= -1,
	.parent		= &clk_arm,	
	.set_rate	= clk_default_setrate,
	.get_rate	= clk_arm_ahb_get_rate,		
};

struct clk clk_arm_apb =
{
	.name		= "clk_arm_apb",
	.id			= -1,
	.parent		= &clk_arm_ahb,	
	.set_rate	= clk_default_setrate,
	.get_rate	= clk_arm_apb_get_rate,		
};

static struct clk clk_ref_ceva =
{
	.name		= "clk_ref_ceva",
	.id			= -1,
	.rate		= 0,
	.parent		= &clk_spll,	
	.get_rate	= spmp_clk_ref_ceva_get_rate,		
};


struct clk clk_ceva =
{
	.name		= "clk_ceva",
	.id			= -1,
	.parent		= &clk_ref_ceva,	
	.set_rate	= clk_default_setrate,
	.get_rate	= clk_ceva_get_rate,		
};

struct clk clk_ceva_ahb =
{
	.name		= "clk_ceva_ahb",
	.id			= -1,
	.parent		= &clk_ceva,	
	.set_rate	= clk_default_setrate,
	.get_rate	= clk_ceva_ahb_get_rate,		
};


struct clk clk_ceva_apb =
{
	.name		= "clk_ceva_apb",
	.id			= -1,
	.parent		= &clk_ceva_ahb,	
	.set_rate	= clk_default_setrate,
	.get_rate	= clk_ceva_apb_get_rate,		
};

struct clk clk_sys =
{
	.name		= "clk_sys",
	.id			= -1,
	.parent		= &clk_ref_ceva,	
	.set_rate	= clk_default_setrate,
	.get_rate	= clk_sys_get_rate,		
};

struct clk clk_sys_ahb =
{
	.name		= "clk_sys_ahb",
	.id			= -1,
	.parent		= &clk_sys,	
	.set_rate	= clk_default_setrate,
	.get_rate	= clk_sys_ahb_get_rate,			
};

struct clk clk_sys_apb =
{
	.name		= "clk_sys_apb",
	.id			= -1,
	.parent		= &clk_sys_ahb,	
	.set_rate	= clk_default_setrate,
	.get_rate	= clk_sys_apb_get_rate,				
};


struct clk clk_lcd =
{
	.name		= "clk_lcd",
	.id			= -1,
	.parent		= &clk_ref_ceva,	
	.set_rate	= clk_default_setrate,
	.get_rate	= clk_lcd_get_rate,						
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
	.set_rate	= clk_default_setrate,
	.get_rate	= clk_csi_get_rate,						
};

struct clk clk_i2s_mclk =
{
	.name		= "clk_i2s_mclk",
	.id			= -1,
	.parent		= &clk_apll,	
	.set_rate	= clk_default_setrate,
	.get_rate	= clk_i2s_mclk_get_rate,					
};

struct clk clk_i2s_bck =
{
	.name		= "clk_i2s_bck",
	.id			= -1,
	.parent		= &clk_i2s_mclk,	
	.set_rate	= clk_default_setrate,
	.get_rate	= clk_i2s_bck_get_rate,					
};

static struct clk *init_src_clocks[] = {
	&xtal,
	&clk_apll,
	&clk_spll,
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
	&clk_i2s_bck,
};


/* initialise the clock system */

int spmp_register_clock(struct clk *clk)
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

int spmp8000_register_clocks(struct clk **clks, int nr_clks)
{
	int fails = 0;

	for (; nr_clks > 0; nr_clks--, clks++) {
		if (spmp_register_clock(*clks) < 0)
			fails++;
	}

	return fails;
}

/* initalise all the clocks */

static void __init spmp_init_clock(struct clk *clk)
{
    clk->rate = clk_get_rate(clk);
}


int __init spmp_register_baseclocks(unsigned long xtal)
{
	struct clk **clkp;
	int ret;	
	for (clkp = init_src_clocks; clkp < init_src_clocks + ARRAY_SIZE(init_src_clocks); clkp++)
	{
		/* ensure that we note the clock state */
		
		ret = spmp_register_clock(*clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       (*clkp)->name, ret);
		}
	}	
	
	return 0;
}

void __init spmp_setup_clocks(void)
{

	struct clk **clkp;
	
	for (clkp = init_src_clocks; clkp < init_src_clocks + ARRAY_SIZE(init_src_clocks); clkp++)
	{
		/* ensure that we note the clock state */
		
		spmp_init_clock(*clkp);		
	}	
	
}


void __init dumpclk(void)
{
	struct clk *p;
	struct clk **clkp;
	spin_lock(&clocks_lock);


	/* check for the case where a device was supplied, but the
	 * clock that was being searched for is not device specific */
#if 1

	for (clkp = init_src_clocks; clkp < init_src_clocks + ARRAY_SIZE(init_src_clocks); clkp++)
	{
		p = *clkp;
		printk("[%s] parrent = %s , rate=%d Hz\n",p->name,  (p->parent == NULL) ? "NULL" : p->parent->name ,(int)clk_get_rate(p));
	}	

#else
		list_for_each_entry(p, &clocks, list) {
           printk("[%s] usage = %08x  %d, rate=%d\n",p->name, p->pValAddr, p->usage,p->rate);
		}
#endif
	spin_unlock(&clocks_lock);
}

