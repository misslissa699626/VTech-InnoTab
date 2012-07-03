/* arch/arm/mach-gpl32900/timer.c
**
** Copyright (C) 2007 Google, Inc.
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
*/

#include <linux/clockchips.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#include <mach/timer.h>
#include <asm/mach/time.h>
#include <mach/hardware.h>
#include <asm/io.h>
#include <mach/hal/hal_clock.h>
#include <mach/timex.h>
#include <mach/regs-wdt.h>
#include <mach/regs-interrupt.h>
#include <mach/regs-timer.h>
#include <linux/platform_device.h>
#include <mach/hal/regmap/reg_scu.h>
/*****************************************************
 
*****************************************************/
#define SCUB_TIMER0_CLKENABLE ((0x01<<9))
#define MSEC_10                 (1000 * 10)  // 10 ms
#define TIMER_USEC_SHIFT 16

static int g_apb_clk = 100*1000000; /*PLK = arm_apb*/
static int g_prescaler_usec = 99;
static int g_ticks_per_usec = 1;
static int g_time_interval = 10000;

static int TICKS2USECS(int x){
	return x / g_ticks_per_usec;
}


static irqreturn_t gpl32900_timer_interrupt(int irq, void *dev_id)
{
#if 0
	if (TMISR_0 == 1) {
		timer_tick();
		TMISR_0 = 0;
		return IRQ_HANDLED;
	}
    return IRQ_NONE;
#else
    timer_tick();
    TMISR_0 = 0;
    return IRQ_HANDLED;
#endif
}


static struct irqaction gpl32900_timer_irq = {
	.name		= "gpl32900 Timer",
	.flags		= IRQF_DISABLED| IRQF_TIMER | IRQF_IRQPOLL,
	.handler	= gpl32900_timer_interrupt,
//	.dev_id		= &gpl32900_clockevent,
};

static inline unsigned long timer_ticks_to_usec(unsigned long ticks)
{
	unsigned long res;

	res = TICKS2USECS(ticks);
	
	return res;
}

#if 0
#ifdef CONFIG_PM
static void gpl32900_timer_suspend(void)
{
}

static void gpl32900_timer_resume(void)
{
}
#else
#define gpl32900_timer_suspend NULL
#define gpl32900_timer_resume NULL
#endif
#endif

#if 0
static unsigned long gpl32900_gettimeoffset (void)
{
	unsigned long t;
	unsigned long irqpend;
	unsigned long tval;
	/* work out how many ticks have gone since last timer interrupt */
	tval = g_time_interval - TMVLR_0;
	t   = tval;
    irqpend = VIC0_IRQSTATUS;
	
	if (irqpend & IRQ_TIMERINT0) {
		tval = g_time_interval - TMVLR_0;
		t = tval;
		if( t != 0 ) {
			t += g_time_interval;
		}
	}
	return timer_ticks_to_usec(t);
}
#else

static unsigned long gpl32900_gettimeoffset(void)
{
    unsigned long value = g_time_interval - TMVLR_0;

	return ((tick_nsec / 1000) * value) / g_time_interval;
}

#endif
static unsigned long gp32900_timer_arm_get_rate( int parentRate )
{
    unsigned int asel;

	asel = (SCUB_SPLL_CFG0 & 0x70000) >> 16;
    switch (asel)
    {
      case 1:	  	
	  	  return 32.768 * 1000;
	  case 2:
      	  return (parentRate / 2);			
	  case 3:
      	  return (parentRate / 3);
      case 4:
      case 5:
      case 6 :			
	  case 7:
      	  return (parentRate / 1);
	  default:
	  	return XTAL_RATE;
	}
}

static unsigned long gp32900_timer_plk_rate_get( void )
{
	unsigned int M,N,R, arm_ahb_ratio, arm_apb_ratio;
	unsigned long spll2Rate = 0;
	unsigned long armRate = 0;
	unsigned long armAhbRate = 0;
	unsigned long armApbRate = 0;
	
	M = (SCUB_SPLL_CFG2 & 0x1F);
	N = (SCUB_SPLL_CFG1 & 0xFF);
	
	if ((SCUB_SPLL_CFG0 & 0x800) == 0) {
		R = 8;
	}
	else {
		R = 4;
	}

	spll2Rate = (XTAL_RATE/M) * N * R;

	armRate = gp32900_timer_arm_get_rate( spll2Rate );

	arm_ahb_ratio = (SCUB_ARM_AHB_RATIO & 0x3F);
	armAhbRate = armRate / (arm_ahb_ratio + 1);

	arm_apb_ratio = (SCUB_ARM_APB_RATIO & 0x3F);
	armApbRate = armAhbRate / (arm_apb_ratio + 1);
	return armApbRate;	
}

static void gpl32900_timer_setup (void)
{
	g_apb_clk = gp32900_timer_plk_rate_get() / SCUB_ARM_APB_RATIO / SCUB_ARM_AHB_RATIO;
	g_prescaler_usec = ((g_apb_clk / 1000000) - 1);
	g_ticks_per_usec = (g_apb_clk / (g_prescaler_usec+1) / 1000000);
	g_time_interval =  (g_ticks_per_usec * MSEC_10); // 10ms

	printk("GPL32900 system timer init, PCLK(arm_apb)[%d]Prescaler[%d]LDR[%d]\n", g_apb_clk, g_prescaler_usec, g_time_interval);
	SCUB_B_PERI_CLKEN |= SCUB_TIMER0_CLKENABLE;
	TMCTR_0 = 0; 
	TMPSR_0 = g_prescaler_usec; // 1MHz tick
	TMLDR_0 = g_time_interval;  // 10000 ticks = 10 ms
	TMCTR_0 = TMR_ENABLE | TMR_IE_ENABLE | TMR_OM_PULSE | TMR_UD_DOWN | TMR_M_PERIOD_TIMER;
}


static void __init gpl32900_timer_init(void)
{
	int res;
	
	gpl32900_timer_setup();
	res = setup_irq(IRQ_TIMERINT0, &gpl32900_timer_irq);
	if (res)
		printk(KERN_ERR "gpl32900_timer_init: setup_irq failed\n");
}

struct sys_timer gpl32900_timer = {
	.init		= gpl32900_timer_init,
	.offset		= gpl32900_gettimeoffset,
	.resume		= gpl32900_timer_setup
};

