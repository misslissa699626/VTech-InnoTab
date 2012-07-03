/* arch/arm/mach-spmp8000/timer.c
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
#include <mach/timex.h>
#include <mach/regs-wdt.h>
#include <mach/regs-scu.h>
#include <mach/regs-interrupt.h>
#include <mach/regs-timer.h>
#include <linux/platform_device.h>

/*****************************************************
 
*****************************************************/
#define SCUB_TIMER1_CLKENABLE ((0x01<<10))

#define MSEC_10                 (1000 * 10)  // 10 ms

#define SYSTEM_CLOCK            (27*1000000)   // Watchdog Clk = 27 Mhz
#define PRESCALER_USEC_1      	((SYSTEM_CLOCK / 1000000) - 1)
#define TICKS_PER_USEC          (SYSTEM_CLOCK / (PRESCALER_USEC_1+1) / 1000000)
#define TIMER_INTERVAL          ((TICKS_PER_USEC * MSEC_10) -1) //10ms

#define TICKS2USECS(x)          ( (x) / TICKS_PER_USEC)

#define TIMER_USEC_SHIFT 16


static irqreturn_t spmp8050_timer_interrupt(int irq, void *dev_id)
{
	WDTCTR_1 = 0x1f & (WDTCTR_1 & (~(WDT_IE_ENABLE)));
    while(WDTCTR_1 & WDT_IE_ENABLE);     //loop self wait until IEEnable is disable
    timer_tick();		
	WDTCTR_1 = 0x1f & (WDTCTR_1 | WDT_IE_ENABLE);
    while(!(WDTCTR_1 & WDT_IE_ENABLE)); //loop self wait until IEEnable is enable	
    
	return IRQ_HANDLED;
}


static struct irqaction spmp8000_timer_irq = {
	.name		= "spmp8000 Timer",
	.flags		= IRQF_DISABLED| IRQF_TIMER | IRQF_IRQPOLL,
	.handler	= spmp8050_timer_interrupt,
//	.dev_id		= &spmp8000_clockevent,
};

static inline unsigned long timer_ticks_to_usec(unsigned long ticks)
{
	unsigned long res;

	res = TICKS2USECS(ticks);
	res += 1 << (TIMER_USEC_SHIFT - 4);	/* round up slightly */

	return res >> TIMER_USEC_SHIFT;
}

#if 0
#ifdef CONFIG_PM
static void spmp8050_timer_suspend(void)
{
}

static void spmp8050_timer_resume(void)
{
}
#else
#define spmp8050_timer_suspend NULL
#define spmp8050_timer_resume NULL
#endif
#endif

static unsigned long spmp8050_gettimeoffset (void)
{
   	unsigned long tdone;
	unsigned long irqpend;
	unsigned long tval;
	/* work out how many ticks have gone since last timer interrupt */
    tval    = WDTVLR_1;
	tdone   = tval;
    irqpend = VIC0_IRQSTATUS;
	   
	if (irqpend & IRQ_WDT0) {
		/* re-read the timer, and try and fix up for the missed
		 * interrupt. Note, the interrupt may go off before the
		 * timer has re-loaded from wrapping.
		 */

		tval =  WDTVLR_1;
		tdone = tval;

		if (tval != 0)
			tdone += TIMER_INTERVAL;
	}
	return timer_ticks_to_usec(tdone);
}



static void spmp8050_timer_setup (void)
{
    TMCTR_0 = 0;
    SCUB_B_PERI_CLKEN &= ~(SCUB_TIMER1_CLKENABLE);
    WDTCTR_1  = WDT_DISABLE; 	
    WDTPSR_1  = PRESCALER_USEC_1;
    WDTLDR_1  = TIMER_INTERVAL;	
	WDTCTR_1  = WDT_ENABLE | WDT_IE_ENABLE | WDT_PWMON_PWM;
    SCUB_B_PERI_CLKEN |= SCUB_TIMER1_CLKENABLE;	
}


static void __init spmp8050_timer_init(void)
{
	int res;
	
	spmp8050_timer_setup();
	res = setup_irq(IRQ_WDT1, &spmp8000_timer_irq);
	if (res)
		printk(KERN_ERR "spmp8000_timer_init: setup_irq failed\n");

//	spmp8000_clockevent.cpumask = cpumask_of_cpu(0);
//	clockevents_register_device(&spmp8000_clockevent);

//	spmp8000_timer_ready = 1;
}

struct sys_timer spmp8050_timer = {
	.init		= spmp8050_timer_init,
	.offset		= spmp8050_gettimeoffset,
	.resume		= spmp8050_timer_setup
};

