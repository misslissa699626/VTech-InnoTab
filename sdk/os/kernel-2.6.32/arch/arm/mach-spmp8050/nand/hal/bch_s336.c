
#include <linux/blkdev.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ioport.h>

#include <linux/spinlock.h>

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/spmp_gpio.h>
#include <mach/regs-scu.h>
#include <mach/general.h>

#include "bch_s336.h"

#ifndef	 CYGPKG_REDBOOT
#define BCH_SUPPORT_INTERRUPT 0
#else
#define BCH_SUPPORT_INTERRUPT 0
#endif


#if BCH_SUPPORT_INTERRUPT
unsigned int g_bch_ctrl_flag;
unsigned int gbch_nStatus;
wait_queue_head_t bch_queue;

#define BCH_INTERRUPT (1<<0)
irqreturn_t bch_interrupt(int irq, void *dev_id)
{
	unsigned long flags;
	unsigned int nStatus = rBCH_S336_INT_STATUS;

	local_irq_save(flags);
	rBCH_S336_INT_STATUS = nStatus;

	if((nStatus&BCH_INTERRUPT))
	{
		flag_setbits(&g_bch_ctrl_flag, BCH_INTERRUPT);	
	}
	wake_up_interruptible(&bch_queue);
	local_irq_restore(flags);
}
#endif

int bch_init_intr(void) 
{
#if BCH_SUPPORT_INTERRUPT

	printk("BCH_SUPPORT_INTERRUPT\n");

	g_bch_ctrl_flag= 0;	
	return request_irq(IRQ_BCH, bch_interrupt, IRQF_DISABLED, NF_DEVICE_NAME, NULL)
#endif		
	/* add by mm.li 01-12,2011 clean warning */
	return 0;
	/* add end */
	
}


void bch_s336_init(void)
{
#if 0
	unsigned int val;
	HAL_READ_UINT32(rSCU_A_PERI_CLKEN, val);
	val |= BCH_ON_OFF(1) | ARBITOR_ON_OFF(1) | AAHB_M212_ON_OFF(1);
	HAL_WRITE_UINT32(rSCU_A_PERI_CLKEN, val);
#endif	
}

void bch_s336_close(void)
{
#if 0
	unsigned int val;
	HAL_READ_UINT32(rSCU_A_PERI_CLKEN, val);
	val |= BCH_ON_OFF(0) | ARBITOR_ON_OFF(0) | AAHB_M212_ON_OFF(0);
	HAL_WRITE_UINT32(rSCU_A_PERI_CLKEN, val);
#endif	
}

extern unsigned int debug_flag;
extern void print_buf(unsigned char * buf, int len);
int bch_s336_process(unsigned char *data, unsigned char *parity, int data_size, int codec_mode, int correct_mode)
{
	unsigned int val;
/* add by mm.li 01-12,2011 clean warning */
	#if BCH_SUPPORT_INTERRUPT
/* add end */	
	unsigned int tempFlag = 0;
/* add by mm.li 01-12,2011 clean warning */
	#endif
/* add end */		
	if( (debug_flag&0xff) == 1)
		SPMP_DEBUG_PRINT("(0x%x, 0x%x, %d ,%d, %d)\n",
		(unsigned int)data, (unsigned int)parity, data_size, codec_mode, correct_mode);

	rBCH_S336_DATA_PTR =(unsigned int) data;
	rBCH_S336_PARITY_PTR =(unsigned int) parity;

#if BCH_SUPPORT_INTERRUPT
	g_bch_ctrl_flag = 0;	
	val = BCH_S336_FINISH_MASK(1) | BCH_S336_DECODE_FAIL_MASK(0);
#else
	val = BCH_S336_FINISH_MASK(0) | BCH_S336_DECODE_FAIL_MASK(0);
#endif
	
	rBCH_S336_INT_MASK= val;

	//val = BCH_S336_SECTOR_NUMBER(data_size) |
	val = BCH_S336_SECTOR_NUMBER((data_size >> 10) - 1) |
		  BCH_S336_CORRECT_MODE(correct_mode) |
		  BCH_S336_ENC_DEC(codec_mode)  | BCH_S336_START;
	if( (debug_flag&0xff) == 1)
		SPMP_DEBUG_PRINT("data_size=%d,  val=%u\n", data_size, val);
	rBCH_S336_CFG = val;

#if BCH_SUPPORT_INTERRUPT
	tempFlag = BCH_INTERRUPT;
	wait_event_interruptible_timeout(bch_queue, g_bch_ctrl_flag == tempFlag, 1 * 1000);
#else	
	// polling busy
	while (1) {
		val = rBCH_S336_INT_STATUS;
		if ((val & BCH_S336_BUSY) == 0)
			break;
	}
	if( (debug_flag&0xff) == 1)
		SPMP_DEBUG_PRINT("polling busy: rBCH_S336_INT_STATUS = %x\n", val);
#endif

	val = rBCH_S336_REPORT_STATUS;
	if ((val & BCH_S336_DECODE_FAIL) == 0)
	{
		if( (debug_flag&0xff) == 1)
		{
			SPMP_DEBUG_PRINT("ECC OK: rBCH_S336_REPORT_STATUS=(0x%x)\n", val);
		}
		return 1;
	}
	else
	{
		//if( (debug_flag&0xff00)!=0)
		//	debug_flag = 1;
		if( (debug_flag&0xff00) == 0x400)
		{
			SPMP_DEBUG_PRINT("ECC Err: rBCH_S336_REPORT_STATUS=(0x%x)\n", val);
		}
		rBCH_S336_SOFT_RESET = 1;
		
		return -1;
	}
}

