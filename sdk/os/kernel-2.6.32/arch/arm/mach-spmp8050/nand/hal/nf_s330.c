#ifdef 	__MMP_ECOS__
#include <pkgconf/system.h>

#include <cyg/infra/diag.h>
#include <cyg/hal/hal_io.h> 
#include <cyg/hal/hal_mmp.h>  // Board definitions
#include <cyg/hal/hal_platform_ints.h>
#include <cyg/hal/hal_arch.h>     // Register state info
#include <cyg/hal/hal_intr.h>

#ifndef CYGPKG_REDBOOT
#include <cyg/kernel/kapi.h>//for cyg_mutex_t
#endif

#define	printf	printk

#include <cyg/hal/nf_s330.h>
#include <cyg/romfs/NF_cfgs.h>
#include <cyg/hal/bch_s336.h>
#include <cyg/romfs/storage_op.h>

//#include <cyg/hal/sysinfo.h>

#else
#include <linux/blkdev.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/genhd.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/blkpg.h>
#include <linux/ata.h>
#include <linux/hdreg.h>

#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/clk.h>

#include <linux/spinlock.h>

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/spmp_gpio.h>
#include <mach/regs-scu.h>
#include <mach/general.h>
#include <mach/common.h>
#include <mach/gp_gpio.h>
#include <mach/gp_board.h>	//wp

#include "nf_s330.h"
#include "bch_s336.h"
#include "../champ/storage_op.h"
#include "../champ/NF_type.h"
#include "../champ/NF_Rom_Cache.h"
#endif

#define SUPPORT_MUTI_CHANNEL 0

#define SUPPORT_MUTI_DESC 1
#define SUPPORT_AUTO_SETTINGACTIMING 1
#define SUPPORT_CLOCK_MANAGER 0

//#define SUPPORT_INTERAL_INTERLEAVE 0
//#define SUPPORT_EXTERAL_INTERLEAVE 1
//#define SUPPORT_TWOPLAN 0

#ifdef	CYGPKG_REDBOOT
#define WRITE_AND_GO   0
#define SUPPORT_INTERRUPT 0
	#if ROMFS_SUPPORT_2PLAN_INTERLEAVE
		#define SUPPORT_INTERAL_INTERLEAVE 0
		#define SUPPORT_EXTERAL_INTERLEAVE 1
		#define SUPPORT_TWOPLAN 1
		#define SUPPORT_ONLY_ONE_CHIP 0
	#else
		#define SUPPORT_INTERAL_INTERLEAVE 0
		#define SUPPORT_EXTERAL_INTERLEAVE 0
		#define SUPPORT_TWOPLAN 0
		#define SUPPORT_ONLY_ONE_CHIP 1	
	#endif	
#else
	#define WRITE_AND_GO 0//	NAND_NEED_DOUBLE_BUFFER
	#define SUPPORT_INTERRUPT 1
	#define SUPPORT_INTERAL_INTERLEAVE 0
	#define SUPPORT_EXTERAL_INTERLEAVE 0
	#define SUPPORT_TWOPLAN 0
	#define SUPPORT_ONLY_ONE_CHIP 1
#endif

#if SUPPORT_ONLY_ONE_CHIP
	#define SUPPORT_MAXCS 1
#else
	#define SUPPORT_MAXCS 4
#endif

//#if SUPPORTCOPYMACHINE
unsigned int g_IsCM = 0;
//#endif //#if SUPPORTCOPYMACHINE

#define	_CACHE_ON_ 0			// it is always cache on in eCos 
//void cache_sync(void);

#define				NFD_IDLE			0
#define				NFD_READWRITE			1
#if	WRITE_AND_GO
unsigned int	g_statusFlag = NFD_IDLE;
#endif

extern unsigned int debug_flag ;
//////////////////////////////////////////////////////////////////////////////////////////////////////

#define PAYLOAD_LEN 8192//4096
#define MAX_DESC_SIZE 16//512

#define DEV_CH0 1//bit 0
#define DEV_CH1 2//bit 1

#ifdef	CYGPKG_REDBOOT
#define ENABLE_MUTEX_LOCK_NF_BCH 0
#else
#define ENABLE_MUTEX_LOCK_NF_BCH 1
#endif

#if ENABLE_MUTEX_LOCK_NF_BCH
#define CYG_PRIORITY_NF_DRV_MUTEX_CEILING 3
#endif

static char g_IsWaitingRB = 0;

 nf_mutex_t nf_lock;
 nf_mutex_t bch_lock;
 nf_mutex_t state_lock;
static char g_IsInit_nf = 0;
static int  g_IsTrigger_nf0 = 0;
/* add by mm.li 01-12,2011 clean warning */
#if SUPPORT_MUTI_CHANNEL
/* add end */
static int  g_IsTrigger_nf1 = 0;
/* add by mm.li 01-12,2011 clean warning */
#endif
/* add end */
SmallBlkInfo_t g_sbi;

DECLARE_MUTEX(nf_lock);
DECLARE_MUTEX(bch_lock);
DECLARE_MUTEX(state_lock);
void nf_drv_mutex_init(nf_mutex_t* lock) 
{
#if ENABLE_MUTEX_LOCK_NF_BCH	
	init_MUTEX(lock);
#endif
}
void nf_drv_mutex_lock(nf_mutex_t* lock)
{
#if ENABLE_MUTEX_LOCK_NF_BCH
	//down_interruptible(lock);
#endif	
}
void nf_drv_mutex_unlock(nf_mutex_t* lock)
{
#if ENABLE_MUTEX_LOCK_NF_BCH
	//up(lock);
#endif	
}
void nf_drv_mutex_destroy(nf_mutex_t* lock)
{
#if ENABLE_MUTEX_LOCK_NF_BCH

#endif	
}
#if 0
unsigned char  gp_bchtmpbuf[512];
unsigned char  rPayload[PAYLOAD_LEN];
unsigned char  g_eccBuf[512];
DescInfo_t  g_DescInfo[MAX_DESC_SIZE];
#if SUPPORT_MUTI_CHANNEL
DescInfo_t  g_DescInfo1[MAX_DESC_SIZE];
#endif
#else
unsigned char  *gp_bchtmpbuf;
unsigned char  *rPayload;
unsigned char  *g_eccBuf;
DescInfo_t  *g_DescInfo;
DescInfo_t  *g_DescInfo1;
#endif

psysinfo_t g_stSysInfo;
ppsysinfo_t pstSysInfo;
char g_ChipMap[4];

#define  nand_flag_setbits flag_setbits

#if SUPPORT_INTERRUPT

#define NF_FLAG_DESC_COMPLETE (1<<0)
#define NF_FLAG_DESC_END (1<<1)
#define NF_FLAG_DESC_ERR (1<<2)
#define NF_FLAG_DESC_INVALID (1<<3)
#define NF_FLAG_DESC_RB1_INTR (1<<12)
#define NF_FLAG_DESC_RB2_INTR (1<<13)
#define NF_FLAG_DESC_RB3_INTR (1<<14)
#define NF_FLAG_DESC_RB4_INTR (1<<15)

unsigned int g_nf_ctrl_CH0_flag;
unsigned int gnf0_nStatus;	
wait_queue_head_t s330_queue0;
irqreturn_t nf0_interrupt(int irq, void *dev_id)
{
	unsigned long flags;
	unsigned int nStatus = rFM_INTR_STS;

	//NF_TRACE(debug_flag, "nStatus=%08x\n", nStatus);
	
	local_irq_save(flags);///????
	rFM_INTR_STS = nStatus;
	
	/* Enable	processing of	incoming timer interrupts. */
	if ((nStatus & NF_FLAG_DESC_COMPLETE))
	{
		nand_flag_setbits(&g_nf_ctrl_CH0_flag, NF_FLAG_DESC_COMPLETE);
	}
	if ((nStatus & NF_FLAG_DESC_END))
	{
		nand_flag_setbits(&g_nf_ctrl_CH0_flag, NF_FLAG_DESC_END);
		//wake_up_interruptible(&s330_queue0);
	}
	if ((nStatus & NF_FLAG_DESC_ERR))
	{
		nand_flag_setbits(&g_nf_ctrl_CH0_flag, NF_FLAG_DESC_ERR);
		//wake_up_interruptible(&s330_queue0);
	}
	if ((nStatus & NF_FLAG_DESC_INVALID))
	{
		nand_flag_setbits(&g_nf_ctrl_CH0_flag, NF_FLAG_DESC_INVALID);
		//wake_up_interruptible(&s330_queue0);
	}
	if ((nStatus & NF_FLAG_DESC_RB1_INTR))
	{
		nand_flag_setbits(&g_nf_ctrl_CH0_flag, NF_FLAG_DESC_RB1_INTR);
	}
	if ((nStatus & NF_FLAG_DESC_RB2_INTR))
	{
		nand_flag_setbits(&g_nf_ctrl_CH0_flag, NF_FLAG_DESC_RB1_INTR);
	}
	if ((nStatus & NF_FLAG_DESC_RB3_INTR))
	{
		nand_flag_setbits(&g_nf_ctrl_CH0_flag, NF_FLAG_DESC_RB1_INTR);
	}
	if ((nStatus & NF_FLAG_DESC_RB4_INTR))
	{
		nand_flag_setbits(&g_nf_ctrl_CH0_flag, NF_FLAG_DESC_RB1_INTR);
	}
	//if(g_nf_ctrl_CH0_flag == 0x09)
		//print_ID((unsigned char*) &rPayload[0]);
	wake_up_interruptible(&s330_queue0);
	local_irq_restore(flags);
	//NF_TRACE(debug_flag, "flag=%x\n",  g_nf_ctrl_CH0_flag);
	return IRQ_HANDLED;
}
int nf0_init_intr(void) 
{
	g_nf_ctrl_CH0_flag = 0;	

	//Attech DMA interrupt for ch 0
	SPMP_DEBUG_PRINT("support Interrupt!\n");
	return request_irq(IRQ_NAND0, nf0_interrupt, IRQF_DISABLED, NF_DEVICE_NAME, NULL);
}

#if SUPPORT_MUTI_CHANNEL
unsigned int g_nf_ctrl_CH1_flag;
unsigned int gnf1_nStatus;
wait_queue_head_t s330_queue1;
irqreturn_t nf1_interrupt(int irq, void *dev_id)
{
	unsigned int nStatus = rFM1_INTR_STS;
	rFM1_INTR_STS = nStatus;

	if ((nStatus & NF_FLAG_DESC_COMPLETE))
	{
		nand_flag_setbits(&g_nf_ctrl_CH1_flag, NF_FLAG_DESC_COMPLETE);
		//printk("1<<0 ");
	}
	if ((nStatus & NF_FLAG_DESC_END))
	{
		nand_flag_setbits(&g_nf_ctrl_CH1_flag, NF_FLAG_DESC_END);
		//printk("1<<1 ");
	}
	if ((nStatus & NF_FLAG_DESC_ERR))
	{
		nand_flag_setbits(&g_nf_ctrl_CH1_flag, NF_FLAG_DESC_ERR);
		//printk("1<<2 ");
	}
	if ((nStatus & NF_FLAG_DESC_INVALID))
	{
		nand_flag_setbits(&g_nf_ctrl_CH1_flag, NF_FLAG_DESC_INVALID);
		//printk("1<<3 ");
	}
	if ((nStatus & NF_FLAG_DESC_RB1_INTR))
	{
		nand_flag_setbits(&g_nf_ctrl_CH1_flag, NF_FLAG_DESC_RB1_INTR);
		//	printk("1<<12 ");
	}
	if ((nStatus & NF_FLAG_DESC_RB2_INTR))
	{
		nand_flag_setbits(&g_nf_ctrl_CH1_flag, NF_FLAG_DESC_RB2_INTR);
		//printk("1<<13 ");
	}
	wake_up_interruptible(&s330_queue1);
	
	return IRQ_HANDLED;
}

int nf1_init_intr(void) 
{
	g_nf_ctrl_CH1_flag = 0;	
	return request_irq(IRQ_NAND1, nf1_interrupt, IRQF_DISABLED, NF_DEVICE_NAME, NULL);
}
#endif	//#if SUPPORT_MUTI_CHANNEL

#endif	//#if SUPPORT_INTERRUPT

//////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned long *gp_Desc,*pTemp;
unsigned long gNextDesBP;
unsigned int gIsSingle_CS;

void InitDesc(unsigned char ch)
{
	nand_cache_sync();
	gNextDesBP = 0;
	g_nf_ctrl_CH0_flag = 0;
#if SUPPORT_MUTI_CHANNEL
	g_nf_ctrl_CH1_flag = 0;
	if(ch==DEV_CH1)
	{
		pTemp = (unsigned long* )((unsigned long)(&g_DescInfo1[0]));
	}
		
	else
#endif
	{
		pTemp = (unsigned long* )((unsigned long)(&g_DescInfo[0]));
	}

	#if _CACHE_ON_	
		gp_Desc = (unsigned long* )(((unsigned long)(pTemp)) | 0x10000000);
	#else
		gp_Desc = (unsigned long* )((unsigned long)(pTemp));
	#endif 	
}

#if 0
void InitDesc_ex(unsigned char op)
{
	gNextDesBP = 0;
	if(op==NF_READ)
	{
		pTemp = (unsigned long* )((unsigned long)(&g_DescInfo11[0]));
	}
		
	else
	{
		pTemp = (unsigned long* )((unsigned long)(&g_DescInfo[0]));
	}

	#if _CACHE_ON_	
		gp_Desc = (unsigned long* )(((unsigned long)(pTemp)) | 0x10000000);
	#else
		gp_Desc = (unsigned long* )((unsigned long)(pTemp));
	#endif 	
}
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////

//#define MAX_ID_LEN 32//5
//////////////////////////////////////////////////////////////////////////////////////////////////////
//print fun

void print_Desc(void)
{
	int i;
	unsigned long* ptr;

	if( (debug_flag&0xff) == 0)
		return;
	ptr = (unsigned long* )((unsigned long)(&g_DescInfo[0]));
#if 1
	SPMP_DEBUG_PRINT("Dump NF Reg:\n");
	printk("CSR:\t%08x  \tDESBA: \t%08x \tTIMING:\t%08x\n", rFM_CSR, rFM_DESC_BA, rFM_AC_TIMING);
	//printk("CTRL4:\t%08x\nCTRL5:\t%08x \nCTRL6:\t%08x \nCTRL7:\t%08x \nCTRL8:\t%08x\n",
	//	rFM_PIO_CTRL4, rFM_PIO_CTRL5, rFM_PIO_CTRL6, rFM_PIO_CTRL7, rFM_PIO_CTRL8);
	printk("MSK:\t%08x  \tSTS: \t%08x\n", rFM_INTRMSK, rFM_INTR_STS);
#endif
	SPMP_DEBUG_PRINT("Dump Desc:\n");
	
	for(i=0; i<8; i++)	
	{
		printk("%08lx ",ptr[i]);	
	}
	printk("\n");
	
}
unsigned int nf_memcmp(unsigned char* ptr1, unsigned char* ptr2, int len)
{
	int i;
	int count = (len + 3) >> 2;
	for (i = 0; i < count; i++)
	{
		if (ptr1[i] != ptr2[i])
			return FAILURE;
	}
	return SUCCESS;
}
void print_buf(unsigned char* buf,int len)
{
	int i;
	unsigned char* buf_NC;
#if _CACHE_ON_	
	buf_NC = (unsigned char*)(((unsigned long)(buf)) | 0x10000000);
#else
	buf_NC = (unsigned char*)buf;
#endif
	if(buf==NULL  || len == 0)
	{
		SPMP_DEBUG_PRINT("Buf==NULL or len=%d\n", len);
		return;
	}

	//printk("print_buf->buf_NC:0x%8x\n",buf_NC);
	//printk("buf_NC:%x len:%d\n", buf_NC, len);
	for(i=0;i<len;i++)
	{
		if( (i%16) == 8)
			printk("- ");
		if( (i%16) == 0)	
			printk("\n%04x==>",i);
			
		printk("%02x ",buf_NC[i]);	
	}
	printk("\n");	
}

void print_ID(unsigned char* buf)
{
	int i;	
	for(i=0;i<MAX_ID_LEN;i++)
	{
	
		if( (i%16) == 8)
			printk("- ");
		if( (i%16) == 0)	
			printk("\n");
			
		printk("%2x ",buf[i]);	
	}
	printk("\n");
}
unsigned int valshift(unsigned int x)
{
	int i=0;
	while(x)
	{
		x >>= 1;
		if(x)
			i++;
	}
	return i;
}
#if 0
unsigned int nfvalshift(unsigned int x)
{
	int i=0;
	while(x)
	{
		x >>= 1;
		if(x)
			i++;
	}
	return i;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////
void SetStatus(unsigned long	 status)
{
#if	WRITE_AND_GO
	unsigned long	cpu_sr;
	
	unsigned long cpu_sr;

	disable_irq(cpu_sr);
	g_statusFlag = status;
	enable_irq(cpu_sr);			

#endif //#ifdef		WRITE_AND_GO
}

unsigned long GetStatus(void)
{
#if	WRITE_AND_GO
	unsigned long status;
	unsigned long cpu_sr;

	disable_irq(cpu_sr);

	status = g_statusFlag;
	enable_irq(cpu_sr);

	return status;
#else
	return 0;
#endif	
}

int enterPollingStatech(unsigned char ch)
{
	unsigned int tempFlag = 0;
#if WRITE_AND_GO
	int	status;
	nf_drv_mutex_lock(&state_lock);
	status = GetStatus();	

	if (status != NFD_IDLE)
	{	
#endif				

		if((ch==DEV_CH0) 
#if WRITE_AND_GO
		&& (status&NFD_READWRITE)
#endif		
		)
		{	
			if(g_IsTrigger_nf0==0)
			{
				printk("g_IsTrigger_nf0==0");
				nf_drv_mutex_unlock(&state_lock);
				return 0;
			}
			g_IsTrigger_nf0 =0; 

#if SUPPORT_INTERRUPT

			//cyg_flag_wait(&g_nf_ctrl_CH0_flag, (NF_FLAG_DESC_INVALID|NF_FLAG_DESC_COMPLETE), CYG_FLAG_WAITMODE_AND | CYG_FLAG_WAITMODE_CLR);
//printk("<wait..");
			//cyg_flag_wait(&g_nf_ctrl_CH0_flag, (NF_FLAG_DESC_COMPLETE), CYG_FLAG_WAITMODE_AND | CYG_FLAG_WAITMODE_CLR);
//printk("##");

			//printk("<wait..");
			//NF_TRACE(debug_flag, "wait..., gIsSingle_CS=%d\n", gIsSingle_CS);
#if 1
			if(pstSysInfo->u8Support_Internal_Interleave&& (gIsSingle_CS==0))
			{
				tempFlag = NF_FLAG_DESC_INVALID|NF_FLAG_DESC_COMPLETE|NF_FLAG_DESC_RB1_INTR;
				//cyg_flag_wait(&g_nf_ctrl_CH0_flag, (NF_FLAG_DESC_INVALID|NF_FLAG_DESC_COMPLETE|NF_FLAG_DESC_RB1_INTR), CYG_FLAG_WAITMODE_AND | CYG_FLAG_WAITMODE_CLR);
				wait_event_interruptible_timeout(s330_queue0, g_nf_ctrl_CH0_flag == tempFlag, 10 * 1000);
				
			}
			else if(pstSysInfo->u8Support_External_Interleave && (gIsSingle_CS==0))
			{
				tempFlag = NF_FLAG_DESC_INVALID | NF_FLAG_DESC_COMPLETE | NF_FLAG_DESC_RB1_INTR;
				//cyg_flag_wait(&g_nf_ctrl_CH0_flag, (NF_FLAG_DESC_INVALID|NF_FLAG_DESC_COMPLETE|NF_FLAG_DESC_RB1_INTR), CYG_FLAG_WAITMODE_AND | CYG_FLAG_WAITMODE_CLR);
				wait_event_interruptible_timeout(s330_queue0, g_nf_ctrl_CH0_flag == tempFlag, 10 * 1000);
			}
			else if(g_IsWaitingRB)
			{
				g_IsWaitingRB = 0;
				//cyg_flag_wait(&g_nf_ctrl_CH0_flag, (NF_FLAG_DESC_RB1_INTR), CYG_FLAG_WAITMODE_AND | CYG_FLAG_WAITMODE_CLR);
				tempFlag = NF_FLAG_DESC_RB1_INTR;
				wait_event_interruptible_timeout(s330_queue0, g_nf_ctrl_CH0_flag == tempFlag, 10 * 1000);
			}
			else
			{
				tempFlag = NF_FLAG_DESC_INVALID|NF_FLAG_DESC_COMPLETE;
				wait_event_interruptible_timeout(s330_queue0, (g_nf_ctrl_CH0_flag&tempFlag) == tempFlag, 10 * 1000);
			}
#else
			while(1)
			{
				if( g_nf_ctrl_CH0_flag!=0)
					break;
			}
			mdelay(10);
			
#endif
			//NF_TRACE(debug_flag, "g_nf_ctrl_CH0_flag:%x\n", g_nf_ctrl_CH0_flag);

#else			
			// there is no more descriptor, and we should wait this bit first
			while((rFM_INTR_STS & ND_INTR_DESC_INVALID)!= ND_INTR_DESC_INVALID);//{printk("1");cyg_thread_delay(1)};
		
			while((rFM_INTR_STS& ND_INTR_DESC_DONE)!= ND_INTR_DESC_DONE);//{printk("2");cyg_thread_delay(1)};


			if(g_IsWaitingRB)
				while((rFM_INTR_STS& ND_INTR_DESC_RB1)!= ND_INTR_DESC_RB1);
#if 0		
			if(pstSysInfo->u8Support_Internal_Interleave&& (gIsSingle_CS==0))
				while((rFM_INTR_STS& ND_INTR_DESC_RB1)!= ND_INTR_DESC_RB1);//{printk("3");cyg_thread_delay(1));};


			if(pstSysInfo->u8Support_External_Interleave && (gIsSingle_CS==0))
			{
				while((rFM_INTR_STS& ND_INTR_DESC_RB1)!= ND_INTR_DESC_RB1);//{printk("4");cyg_thread_delay(1));};
				//while((rFM_INTR_STS& ND_INTR_DESC_RB)!= ND_INTR_DESC_RB);
			}
#endif			
	
			
#endif	//SUPPORT_INTERRUPT
				
	
			//rFM_INTR_STS = 0xffff;	
#if WRITE_AND_GO
			status &= ~NFD_READWRITE;
#endif
		}

#if SUPPORT_MUTI_CHANNEL				
		if((ch==DEV_CH1) 
#if WRITE_AND_GO		
		&& (status&(NFD_READWRITE<<4))	
#endif		
		)
		{
			if(g_IsTrigger_nf1==0)
			{
				nf_drv_mutex_unlock(&state_lock);
				return 0;
			}
			g_IsTrigger_nf1 =0; 


#if SUPPORT_INTERRUPT
			if(pstSysInfo->u8Support_Internal_Interleave&& (gIsSingle_CS==0))
			{
				tempFlag = NF_FLAG_DESC_INVALID|NF_FLAG_DESC_COMPLETE|NF_FLAG_DESC_RB1_INTR;
				//cyg_flag_wait(&g_nf_ctrl_CH1_flag, (NF_FLAG_DESC_INVALID|NF_FLAG_DESC_COMPLETE|NF_FLAG_DESC_RB1_INTR), CYG_FLAG_WAITMODE_AND | CYG_FLAG_WAITMODE_CLR);
				wait_event_interruptible_timeout(s330_queue1, g_nf_ctrl_CH1_flag == tempFlag, 10 * 1000);
			}
			else if(pstSysInfo->u8Support_External_Interleave && (gIsSingle_CS==0))
			{
				tempFlag = NF_FLAG_DESC_INVALID|NF_FLAG_DESC_COMPLETE|NF_FLAG_DESC_RB1_INTR|NF_FLAG_DESC_RB2_INTR;
				//cyg_flag_wait(&g_nf_ctrl_CH1_flag, (NF_FLAG_DESC_INVALID|NF_FLAG_DESC_COMPLETE|NF_FLAG_DESC_RB1_INTR|NF_FLAG_DESC_RB2_INTR), CYG_FLAG_WAITMODE_AND | CYG_FLAG_WAITMODE_CLR);
				wait_event_interruptible_timeout(s330_queue1, g_nf_ctrl_CH1_flag == tempFlag, 10 * 1000);
			}
			else
			{
				tempFlag = NF_FLAG_DESC_INVALID|NF_FLAG_DESC_COMPLETE;
				//cyg_flag_wait(&g_nf_ctrl_CH1_flag, (NF_FLAG_DESC_INVALID|NF_FLAG_DESC_COMPLETE), CYG_FLAG_WAITMODE_AND | CYG_FLAG_WAITMODE_CLR);
				wait_event_interruptible_timeout(s330_queue1, g_nf_ctrl_CH1_flag == tempFlag, 10 * 1000);
			}
#else	

			// there is no more descriptor, and we should wait this bit first
			while((rFM1_INTR_STS& ND_INTR_DESC_INVALID)!= ND_INTR_DESC_INVALID);//{printk("rFM_INTR_STS:%x",rFM_INTR_STS);};
		
			while((rFM1_INTR_STS& ND_INTR_DESC_DONE)!= ND_INTR_DESC_DONE);//{printk("*");};

			if(pstSysInfo->u8Support_Internal_Interleave&& (gIsSingle_CS==0))
				while((rFM1_INTR_STS& ND_INTR_DESC_RB1)!= ND_INTR_DESC_RB1);
		
			if(pstSysInfo->u8Support_External_Interleave && (gIsSingle_CS==0))
				while((rFM1_INTR_STS& ND_INTR_DESC_RB)!= ND_INTR_DESC_RB);
#endif
//hal_delay_us(100);

			//rFM1_INTR_STS = 0xffff;	
#if WRITE_AND_GO			
			status &= ~(NFD_READWRITE<<4);	
			
#endif
		}
#endif

#if WRITE_AND_GO
		SetStatus(status);
		//
		
		//printk("enterPollingStatech<<<<<\n");
	}
	nf_drv_mutex_unlock(&state_lock);	
#endif
	//NF_TRACE(debug_flag, "End\n");
	nand_cache_invalidate();
	return 0;
}
int enterPollingState()
{
	nf_drv_mutex_lock(&nf_lock);
	//printk("1\n");	
	enterPollingStatech(DEV_CH0);

#if SUPPORT_MUTI_CHANNEL	
	if(pstSysInfo->u8MultiChannel)
	//printk("2\n");
		enterPollingStatech(DEV_CH1);
#endif
	nf_drv_mutex_unlock(&nf_lock);	
	return 0;	
}

int enterPollingState_ex(void)
{

#if WRITE_AND_GO	
	enterPollingStatech(DEV_CH0);

#if SUPPORT_MUTI_CHANNEL	
	if(pstSysInfo->u8MultiChannel)
	//printk("2\n");
		enterPollingStatech(DEV_CH1);
#endif//#if SUPPORT_MUTI_CHANNEL

#endif//#if WRITE_AND_GO

	return 0;	
}

void trigger(unsigned char ch)
{
	unsigned long physAddr = 0;
#if SUPPORT_MUTI_CHANNEL
		if(ch==DEV_CH1)
		{
#if 1
			while(rFM1_INTR_STS&ND_INTR_DESC_NFC_BUSY)
			{
				SPMP_DEBUG_PRINT("rFM1_INTR_STS:0x%x\n",rFM1_INTR_STS);
			}
			
			while(rFM1_INTR_STS)
			{
				rFM1_INTR_STS = 0xffff;				
			}	
				
		#if SUPPORT_INTERRUPT
			//cyg_flag_maskbits(&g_nf_ctrl_CH0_flag,0);	
			g_nf_ctrl_CH0_flag = 0;
		#endif	

#endif		
			rFM1_DESC_BA = (unsigned long) ((unsigned long*) &g_DescInfo1[0]);
			rFM1_CSR        = 0x19 ;//Trigger	
			g_IsTrigger_nf1 = 1;
		}
		else
#endif
		{
#if 1
//printk("enter trigger(%d)\n",ch);

			while(rFM_INTR_STS&ND_INTR_DESC_NFC_BUSY)
			{
				SPMP_DEBUG_PRINT("rFM_INTR_STS:0x%x\n",rFM_INTR_STS);
			}
			
			while(rFM_INTR_STS)
			{
				rFM_INTR_STS = 0xffff;
				if(rFM_INTR_STS)
				{
					
					//printk("<rFM_INTR_STS:0x%x>",rFM_INTR_STS);
				}
			}
		#if SUPPORT_INTERRUPT
		//printk("cyg_flag_peek(&g_nf_ctrl_CH0_flag):0x%x\n", cyg_flag_peek(&g_nf_ctrl_CH0_flag));
			//cyg_flag_maskbits(&g_nf_ctrl_CH0_flag,0);	
			g_nf_ctrl_CH0_flag = 0;
		#endif
#endif	
			rFM_CSR     = 0 ;
			rFM_DESC_BA = 0;//??
			//printk("Assign Descript to NAND ctrl...\n");
			//rFM_DESC_BA = (unsigned long) ((unsigned long*) &g_DescInfo[0]);
			physAddr = (unsigned long) ((unsigned long*) &g_DescInfo[0]);
			rFM_DESC_BA =  (unsigned long)virt_to_phys((void*)physAddr);
			
			NF_TRACE(debug_flag, "Start to trigger...\n");
			print_Desc();
			rFM_CSR         = 0x19 ;//Trigger	
			
			g_IsTrigger_nf0 = 1;		
		}		



//printk("Trigger\n");

		
}


unsigned char* Cache2NonCacheAddr(unsigned char* addr)
{
#if _CACHE_ON_	
	return (unsigned char *)(((unsigned long)(addr)) | 0x10000000);
#else
	return (unsigned char *)((unsigned long)(addr));
#endif		
}

void PollingAndsetStatus(unsigned ch, unsigned long	 status)
{
#if WRITE_AND_GO	
	unsigned int sta;
	//printk("3\n");
	enterPollingStatech(ch);

#if SUPPORT_MUTI_CHANNEL	
	if(ch==DEV_CH1)
	{
		nf_drv_mutex_lock(&state_lock);
		sta = GetStatus();
		sta |= status<<4;
		SetStatus(sta);	
		nf_drv_mutex_unlock(&state_lock);
	}
	else	
#endif	//#if SUPPORT_MUTI_CHANNEL	
	{
		nf_drv_mutex_lock(&state_lock);
		sta = GetStatus();
		sta |= status;
		SetStatus(sta);
		nf_drv_mutex_unlock(&state_lock);	
	}

#else
	//printk("4\n");
	enterPollingStatech(ch);
#endif	//#ifdef		WRITE_AND_GO	
}

void ReSet(unsigned char ch, unsigned char chip_number)
{
#if WRITE_AND_GO
	PollingAndsetStatus(ch, NFD_READWRITE);
#endif

	//SPMP_DEBUG_PRINT("ReSet(%d, %d)\n", ch, chip_number);

	InitDesc(ch);
	memset(gp_Desc, 0, DESC_OFFSET);
	
	ChipEnable(chip_number);
	if(chip_number>=3) chip_number = 3;

	
	//CMD phase
	gp_Desc[DESC_CMD] = 0x70000000 |((1<<chip_number)<<24)|0xFF00;//(unsigned long)((MAUNCMD_RESET) | ((1<<chip_number)<<24) | (CMD_TYPE_MANUAL_MODE_CMD<<28));
	gp_Desc[DESC_INTERRUPT] = 0;	
	gp_Desc[DESC_ADDR1] =  0xF0000000;
	gp_Desc[DESC_ADDR2] = 0;

	//g_IsWaitingRB = 1;

	trigger(ch);
	//printk("5\n");
	enterPollingStatech(ch);

	//g_IsWaitingRB = 0;
	
	//SPMP_DEBUG_PRINT("exit of ReSet\n");
		
}


unsigned int ReadStatus(void)
{
	unsigned char Status;

	InitDesc(DEV_CH0);
	memset(gp_Desc, 0, DESC_OFFSET);

	printk("ReadStatus ");

	
	//CMD phase
	gp_Desc[DESC_CMD] = 0x40000000 |(1<<24)|0x7000;
	gp_Desc[DESC_INTERRUPT] = 0;	
	gp_Desc[DESC_ADDR1] =  0xF0000000;
	gp_Desc[DESC_ADDR2] = 0;

	trigger(DEV_CH0);
	//enterPollingStatech(DEV_CH0);
	//enterPollingStatech(DEV_CH0);

	//enterPollingStatech(ch);
	
	Status = (unsigned char)(gp_Desc[DESC_CMD]>>8);

	printk(": %d\n", Status);


	return (Status&1);
		
		
}




void ReadID(unsigned char ch, unsigned char chip_number, unsigned long u32PyLdBP)
{
	unsigned long PhysAddr = 0;
#if WRITE_AND_GO
	PollingAndsetStatus(ch, NFD_READWRITE);
#endif
	InitDesc(ch);
	ChipEnable(chip_number);
	if(chip_number>=3) chip_number = 3;

	memset(gp_Desc, 0, MAX_DESC_SIZE * DESC_OFFSET);
	PhysAddr = (unsigned long )virt_to_phys((void*)u32PyLdBP);
	//SPMP_DEBUG_PRINT("ReadID(%d,%d)\n", ch, chip_number);

	//CMD phase
	gp_Desc[DESC_CMD] = (unsigned long)((MAUNCMD_READID) | ((1<<chip_number)<<24) | (CMD_TYPE_MANUAL_MODE_CMD<<28));
	gp_Desc[DESC_INTERRUPT] = (pstSysInfo->u16InterruptMask<<16);	
	gp_Desc[DESC_ADDR1] = ((MANUAL_MODE_OWNER | MANUAL_MODE_LAST_SECTOR | MANUAL_MODE_REDUNT_ENABLE)<<24);
	gp_Desc[DESC_ADDR2] = 0x00|(gNextDesBP+=sizeof(DescInfo_t));

	gp_Desc += DESC_OFFSET;	

	//Addr phase
	gp_Desc[DESC_CMD] = (((1<<chip_number)<<24) | (CMD_TYPE_MANUAL_MODE_ADDR<<28));
	gp_Desc[DESC_INTERRUPT] = (pstSysInfo->u16InterruptMask<<16);	
	gp_Desc[DESC_ADDR1] = ((MANUAL_MODE_OWNER | MANUAL_MODE_LAST_SECTOR | MANUAL_MODE_REDUNT_ENABLE)<<24);
	gp_Desc[DESC_ADDR2] = 0x00|(gNextDesBP+=sizeof(DescInfo_t));

	gp_Desc += DESC_OFFSET;	

	//PYLOAD phase read
	gp_Desc[DESC_CMD] = (((1<<chip_number)<<24) | (CMD_TYPE_MANUAL_MODE_PYLOAD_READ<<28));
	gp_Desc[DESC_INTERRUPT] = (pstSysInfo->u16InterruptMask<<16);	
	
	//g_DescInfo[2].Payload_length = MAX_ID_LEN;
	gp_Desc[DESC_LENGTH] = MAX_ID_LEN<<16;
	
	gp_Desc[DESC_ADDR1] = ((MANUAL_MODE_OWNER | MANUAL_MODE_END_DESC | MANUAL_MODE_LAST_SECTOR | MANUAL_MODE_REDUNT_ENABLE)<<24);
	gp_Desc[DESC_ADDR2] = 0;
	
	//gp_Desc[DESC_PAYLOAD] = u32PyLdBP;	
	gp_Desc[DESC_PAYLOAD] = PhysAddr;	

	nand_cache_sync();
	trigger(ch);
	//printk("6\n");
	enterPollingStatech(ch);

	//printk("rFM1_INTR_STS:0x%x\n",rFM1_INTR_STS);
	
}
unsigned int get_cesize(unsigned char val)
{
	unsigned int ce_size = 0;
	switch(val) 
	{
		case 0xF1:
			ce_size = 0x20000;//1Gb 128MB
			break;
		 case 0xDA:
			ce_size = 0x40000;//2Gb 256MB
			break;
		 case 0xDC:
			ce_size = 0x80000;//4Gb 512MB
			break;						
		 case 0xD3:
			ce_size = 0x100000;//8Gb 1GB
			break;
		 case 0xD5:
			ce_size = 0x200000;//16Gb 2GB
			break;
		 case 0xD7:
			ce_size = 0x400000;//32Gb 4GB
			break;	
		 case 0xD9:
			ce_size = 0x800000;//64Gb 8GB
			break;
		 case 0xDE:
			ce_size = 0x800000;//128Gb 16GB
			break;
		default://UNKNOWN
			ce_size = 0x800000;//128Gb 16GB
			break;	
	}	
	return ce_size;

}
void SetIdInfo(unsigned char* Idbuf)
{
	int i=0;

	//printk("ID : ");
	do
	{
		//printk("%02x ", Idbuf[i]);
		pstSysInfo->IdBuf[i]=Idbuf[i];
		i++;
	}while((Idbuf[i]!=Idbuf[0]) &&(Idbuf[i]!=0) );

	//printk("\n");
	
	pstSysInfo->Id_len = i;

	//return i;
}
//#define IDTYPE0 0
//#define IDTYPE1 1

//unsigned char GetIdType(psysinfo_t* psysInfo)
unsigned char GetIdType(void)
{
	//unsigned char type;

	if(pstSysInfo->IdBuf[0]==0xec && pstSysInfo->Id_len==6)
	{
		//printk("IDTYPE1\n");
		return IDTYPE1;
	}
	else if((pstSysInfo->IdBuf[0]==0x2c || pstSysInfo->IdBuf[0]==0x89) &&(pstSysInfo->IdBuf[1]==0x48 ||pstSysInfo->IdBuf[1]==0x68 || pstSysInfo->IdBuf[1]==0x88 || pstSysInfo->IdBuf[1]==0xA8))
	{
		//printk("IDTYPE2\n");

		return IDTYPE2;
	}
	else if(pstSysInfo->IdBuf[0]==0xAD && pstSysInfo->Id_len==6)
	{
		return IDTYPE3;
	}

	//printk("IDTYPE0\n");

	return IDTYPE0;

}
unsigned int getIdType2_TotalBlkNo(unsigned char ch, unsigned int pagesize)
{
	unsigned int totalsize;
	switch(ch)
	{
		case 0x48:
			totalsize = 2048*4096;		
			break;
		case 0x68:
			totalsize = 4096*4096;	
			break;
		case 0x88:
			totalsize = 8192*4096;
			break;
		case 0xA8:
			totalsize = 16384*4096;
			break;
		default:
			totalsize = 4096*4096;
			break;	
	}	
	return totalsize/pagesize;
}
unsigned int getIdType3_Info(unsigned char* ptr)
{
	unsigned char ch;// = ptr[3];

	unsigned int blksize;
	unsigned int ce_size = get_cesize(ptr[1]);

	pstSysInfo->u16PyldLen = ( 2048 << (ptr[3] & 0x3) );

	ch = ptr[3]>>4;

	switch(ch) 
	{
		case 0:
			blksize = (128*1024);
			break;
		case 1:
			blksize = (256*1024);
			break;	
		case 2:
			blksize = (512*1024);
			break;	
		case 3:
			blksize = (768*1024);
			break;	
		case 8:
			blksize = (1024*1024);
			break;	
		case 9:
			blksize = (2048*1024);
			break;	

		default:
			blksize = 0;
			break;
	}
	
	pstSysInfo->u16PageNoPerBlk = blksize/pstSysInfo->u16PyldLen;

	pstSysInfo->u16TotalBlkNo = ce_size/(blksize>>10);

	pstSysInfo->ecc_mode = 0;//TO DO.............

	//ppf->PageSize = 4096;

	return SUCCESS;
}

void Gereral_PF(unsigned char *prData)
{
  	unsigned int k, index, ce_size;

	//PF_INFO pf;
	
	SetIdInfo(prData);

	k = (pstSysInfo->IdBuf[3]>>4) & 0x03;	
	index  = (pstSysInfo->IdBuf[3]) & 0x03;
	ce_size = get_cesize(pstSysInfo->IdBuf[1]);

	//switch(GetIdType(pstSysInfo)) 
	switch(GetIdType())
	{
		case IDTYPE0:
			pstSysInfo->u16PyldLen	= ID_0_PAGE_SIZE(index);	
			pstSysInfo->u16PageNoPerBlk = (ID_0_BLOCK_SIZE(k) >> ID_0_PAGE_SHIFT(index));
			pstSysInfo->u16TotalBlkNo = (ce_size >> ID_0_BLOCK_SHIFT(k));
			pstSysInfo->ecc_mode = 0;
			break;
	
		case IDTYPE1:
			pstSysInfo->u16PyldLen	= ID_1_PAGE_SIZE(index);	
			pstSysInfo->u16PageNoPerBlk = (ID_1_BLOCK_SIZE(k) >> ID_1_PAGE_SHIFT(index));
			pstSysInfo->u16TotalBlkNo = (ce_size >> ID_1_BLOCK_SHIFT(k));
			pstSysInfo->ecc_mode = 0;
			break;

		case IDTYPE2:
			pstSysInfo->u16PyldLen	= ID_0_PAGE_SIZE(index);
			pstSysInfo->u16PageNoPerBlk = 256;
			pstSysInfo->u16TotalBlkNo = getIdType2_TotalBlkNo(pstSysInfo->IdBuf[1], pstSysInfo->u16PyldLen);
			pstSysInfo->ecc_mode = 0;
			break;

		case IDTYPE3:
			getIdType3_Info((unsigned char *)pstSysInfo->IdBuf);
			break;

		default:
			pstSysInfo->u16PyldLen	= ID_0_PAGE_SIZE(index);	
			pstSysInfo->u16PageNoPerBlk = (ID_0_BLOCK_SIZE(k) >> ID_0_PAGE_SHIFT(index));
			pstSysInfo->u16TotalBlkNo = (ce_size >> ID_0_BLOCK_SHIFT(k));
			pstSysInfo->ecc_mode = 0;
			break;
	}
	//Write2file(pf);

}
void Print_SmallBlkInfo(SmallBlkInfo_t* psbi)
{
//#if 0
	if(getSDevinfo(SMALLBLKFLG))
	{
		printk("######################SmallBlkInfo#######################\n");
		printk("PhyPageLen             : %u\n", psbi->PhyPageLen      		);
		printk("PhyPageNoPerBlk        : %u\n", psbi->PhyPageNoPerBlk 		);
		printk("PhyTotalBlkNo          : %u\n", psbi->PhyTotalBlkNo   		);
		printk("LogPageLen             : %u\n", psbi->LogPageLen     		);
	    printk("LogPageNoPerBlk        : %u\n", psbi->LogPageNoPerBlk 		);
	    printk("LogTotalBlkNo          : %u\n", psbi->LogTotalBlkNo   		);   
	 	printk("PhyPageLenShift        : %u\n", psbi->PhyPageLenShift		);
		printk("PhyPageNoPerBlkShift   : %u\n", psbi->PhyPageNoPerBlkShift	);
		printk("PhyPagePerLogPage      : %u\n", psbi->PhyPagePerLogPage		);	  	
		printk("PhyBlkPerLogBlk        : %u\n", psbi->PhyBlkPerLogBlk		);
	    printk("PhyPagePerLogBlk       : %u\n", psbi->PhyPagePerLogBlk		);
	    printk("PhyPagePerLogPageShift : %u\n", psbi->PhyPagePerLogPageShift);
	    printk("PhyBlkPerLogBlkShift   : %u\n", psbi->PhyBlkPerLogBlkShift	);
	    printk("PhyPagePerLogBlkShift  : %u\n", psbi->PhyPagePerLogBlkShift	);
		printk("PhyReduntLenLog        : %u\n", psbi->PhyReduntLenLog	);
	 	printk("#########################################################\n"); 
	}
//#endif
}

void setsbi( SmallBlkInfo_t* psbi)
{
	psbi->LogPageLen = 2048;
	psbi->LogPageNoPerBlk = 64;

	psbi->PhyPageLenShift = nfvalshift(psbi->PhyPageLen); //nfvalshift(512) -->9
	psbi->PhyPageNoPerBlkShift = nfvalshift(psbi->PhyPageNoPerBlk); //nfvalshift(16) -->4
	
	
	psbi->PhyPagePerLogPage = (psbi->LogPageLen>>psbi->PhyPageLenShift);//(2048>>9)-->4
	psbi->PhyBlkPerLogBlk = psbi->PhyPagePerLogPage *(psbi->LogPageNoPerBlk>>psbi->PhyPageNoPerBlkShift);//(64>>4)-->4
	psbi->PhyPagePerLogBlk = (psbi->LogPageNoPerBlk * psbi->PhyPagePerLogPage);//(4*4)-->16
	
	psbi->PhyPagePerLogPageShift = nfvalshift(psbi->PhyPagePerLogPage);//nfvalshift(4)-->2
	psbi->PhyBlkPerLogBlkShift = nfvalshift(psbi->PhyBlkPerLogBlk);//nfvalshift(4)-->2
	psbi->PhyPagePerLogBlkShift = nfvalshift(psbi->PhyPagePerLogBlk);//nfvalshift(16)-->4
	
	psbi->LogTotalBlkNo = (psbi->PhyTotalBlkNo>>psbi->PhyBlkPerLogBlkShift);//(1024>>4)->64

	psbi->PhyReduntLenLog = (psbi->PhyPageLen>>5);

	pstSysInfo->u16PyldLen	= psbi->LogPageLen; 
	pstSysInfo->u16PageNoPerBlk = psbi->LogPageNoPerBlk;
	pstSysInfo->u16TotalBlkNo = psbi->LogTotalBlkNo;

	Print_SmallBlkInfo(psbi);
}

unsigned int Smallblk(unsigned char val, SmallBlkInfo_t* psbi)
{
	switch(val) 
	{
		case 0xE6:
			psbi->PhyPageLen = 512;
			psbi->PhyPageNoPerBlk = 16;
			psbi->PhyTotalBlkNo = 1024;
			setsbi(psbi);		
			break;
			
		 case 0x73:
			psbi->PhyPageLen  = 512; 
			psbi->PhyPageNoPerBlk = 32;
			psbi->PhyTotalBlkNo = 1024;
			setsbi(psbi);
			break;

		 case 0x75:
			psbi->PhyPageLen  = 512; 
			psbi->PhyPageNoPerBlk = 32;
			psbi->PhyTotalBlkNo = 2048;
			setsbi(psbi);
			break;						

		 case 0x76:
		 	psbi->PhyPageLen  = 512; 
			psbi->PhyPageNoPerBlk = 32;
			psbi->PhyTotalBlkNo = 4096;
			setsbi(psbi);
			break;

		case 0x79:
		default://UNKNOWN
			psbi->PhyPageLen	= 512; 
			psbi->PhyPageNoPerBlk = 32;
			psbi->PhyTotalBlkNo = 8192;
			setsbi(psbi);
			break;		
	}	
	return 1;

}

unsigned char getEccMode(void)
{
	return pstSysInfo->ecc_mode;
}

void setEccMode(unsigned char mode)
{
	SPMP_DEBUG_PRINT("setEccMode(%d)\n", mode);
#if 0	
	if(getSDevinfo(SUPPORTBCHFLG)==0)
	{
		printk("getSDevinfo(SUPPORTBCHFLG)==0\n");
		if(getSDevinfo(SMALLBLKFLG)==0)
		{
			pstSysInfo->ecc_mode = mode;
			pstSysInfo->u16Redunt_Sector_Len = 0;
			pstSysInfo->u16Redunt_Sector_Addr_Offset = 0;
			pstSysInfo->u16ReduntLen = 0;
			pstSysInfo->u16ReduntLenLog = 0;
			pstSysInfo->u16PageSize = pstSysInfo->u16PyldLen;	
		}
		else
		{
			g_sbi.PhyReduntLenLog = 0;
		}	
	}
	else
#endif
	if(mode==BCH_S336_24_BIT)
	{
		SPMP_DEBUG_PRINT("BCH_S336_24_BIT\n");		
		pstSysInfo->ecc_mode = BCH_S336_24_BIT;	
		pstSysInfo->u16Redunt_Sector_Len = 44;
		pstSysInfo->u16Redunt_Sector_Addr_Offset = 64;
		pstSysInfo->u16ReduntLen = 44*((pstSysInfo->u16PyldLen)>>10);
		pstSysInfo->u16ReduntLenLog = (pstSysInfo->u16PyldLen)>>4;
		pstSysInfo->u16PageSize = pstSysInfo->u16PyldLen + pstSysInfo->u16ReduntLenLog;	

	}
	else
	{
		SPMP_DEBUG_PRINT("BCH_S336_16_BIT\n");
		pstSysInfo->ecc_mode = BCH_S336_16_BIT;
		pstSysInfo->u16Redunt_Sector_Len = 32;
		pstSysInfo->u16Redunt_Sector_Addr_Offset = 32;
		pstSysInfo->u16ReduntLen = (pstSysInfo->u16PyldLen)>>5;
		pstSysInfo->u16ReduntLenLog = (pstSysInfo->u16PyldLen)>>5;
		pstSysInfo->u16PageSize = pstSysInfo->u16PyldLen + pstSysInfo->u16ReduntLenLog;	
	}
}

void setEccMode_ex(void)
{
	setEccMode(pstSysInfo->ecc_mode);
}

//#ifdef	CYGPKG_REDBOOT
unsigned int get_eccmode(void)
{
#if 0
	if(GetCmdSet() > RCV_CMDSET1)
	{
		//printk("$$$$$$$$$$$$$$$$$$$$$BCH_S336_24_BIT\n");
		return BCH_S336_24_BIT;
	}
	//printk("$$$$$$$$$$$$$$$$$$$$$BCH_S336_16_BIT\n");

	return BCH_S336_16_BIT;
#else
	return BCH_S336_24_BIT;
#endif	
} 
//#endif
int ReadNandInfoFromProfile(void)
{
	unsigned char *pReadBuf, *pEccBuf;
//	unsigned int addr1,addr2;
	Manage_Info_t* p_mi;

	//unsigned char* ptr;


	pReadBuf = (unsigned char*)rPayload;
	pEccBuf = (unsigned char*)g_eccBuf;

	//ptr = (unsigned char*)(((unsigned long)pReadBuf)+SIZE_1K);

	//printk("pReadBuf:0x%x\n", pReadBuf);
	//printk("ptr:0x%x\n", ptr);

	ReadWritePage_ex(0, (unsigned long*)pReadBuf, (unsigned long*)pEccBuf, NF_READ);

	//print_buf(pReadBuf, 1024);
	//print_buf(ptr, 64);

/* modify by mm.li 01-12,2011 clean warning */
#if 0	
	if(BCHProcess( pReadBuf, (unsigned long*)(((unsigned long)pReadBuf)+SIZE_1K), SIZE_1K, BCH_DECODE, get_eccmode()/*BCH_S336_16_BIT*/)==ret_BCH_OK)
#else
	if(BCHProcess( pReadBuf, (unsigned char*)(((unsigned long)pReadBuf)+SIZE_1K), SIZE_1K, BCH_DECODE, get_eccmode()/*BCH_S336_16_BIT*/)==ret_BCH_OK)
#endif		
/* modify end */
	{
		p_mi = (Manage_Info_t*)Cache2NonCacheAddr(pReadBuf);
		if((p_mi->pf1.heards == PF_HEARDS) && (p_mi->pf1.hearde == PF_HEARDE))
		{
			printk("p_mi->nand_info.ecc_mode:%d\n", p_mi->nand_info.ecc_mode);
			printk("p_mi->nand_info1.ecc_mode:%d\n", p_mi->nand_info1.ecc_mode);


			pstSysInfo->ecc_mode = p_mi->nand_info1.ecc_mode;
		}
	
	}
	else
	{
		printk("ReadNandInfoFromProfile->ReadPage(0) is error!\n");
	}
	return SUCCESS;

}
void print_sysinfo_t(void);
int setSystemPara(unsigned char *prData)
{

//  	int k;
// 	int index  = (prData[3]) & 0x03;
  	//int tmp, Reduntsize;
  	int ret=0;
  	
	if(getSDevinfo(SMALLBLKFLG))
	{
		printk("Smallblk.............\n");
		Smallblk(prData[1], &g_sbi);//??????????????????
		SetIdInfo(prData);
	}
	else
	{
		Gereral_PF(prData);
	}

#if SUPPORT_TWOPLAN		 
		if(((prData[4]>>2)& 0x03) && ((prData[2]>>4)& 0x03))
			pstSysInfo->u8Support_TwoPlan =1;
		else		
#endif
			pstSysInfo->u8Support_TwoPlan =0;
	
	pstSysInfo->u8PagePerBlkShift = nfvalshift(pstSysInfo->u16PageNoPerBlk);
	
	
//	pstSysInfo->u16PageSize = pstSysInfo->u16PyldLen + (pstSysInfo->u16ReduntLen<<1);
	pstSysInfo->u16PageSize = pstSysInfo->u16PyldLen;// + pstSysInfo->u16ReduntLen;

#if SUPPORT_INTERAL_INTERLEAVE		
	pstSysInfo->u8Support_Internal_Interleave = (prData[2]>>6 & 1);
#else
	pstSysInfo->u8Support_Internal_Interleave = 0;
#endif		

#if SUPPORT_ONLY_ONE_CHIP
	pstSysInfo->u8Internal_Chip_Number = 0;
	SPMP_DEBUG_PRINT("XXpstSysInfo->u8Internal_Chip_Number:%d\n",pstSysInfo->u8Internal_Chip_Number);
#endif		
	
	
#if SUPPORT_EXTERAL_INTERLEAVE		
	if(pstSysInfo->u8Internal_Chip_Number)
	{
		pstSysInfo->u8Support_TwoPlan =0;
		pstSysInfo->u8Support_Internal_Interleave = 0;
		pstSysInfo->u8Support_External_Interleave = 1;
	}
	else 
#endif	
	if(pstSysInfo->u8Support_TwoPlan)
	{
		pstSysInfo->u8Support_Internal_Interleave = 0;
		pstSysInfo->u8Support_External_Interleave = 0;
	}

if(pstSysInfo->u16TotalBlkNo<=2048)
{
	pstSysInfo->u8Support_TwoPlan =0;
}

	if(prData[0]==0xec)//SAMSUNG NAND Flash module
	{
		ret = NAND_SAMSUNG;
		pstSysInfo->vendor_no = ret;
	}
	else if(prData[0]==0xad)//HYNIX NAND Flash module
	{
		pstSysInfo->u8Support_TwoPlan =0;
		ret = NAND_HYNIX;
		pstSysInfo->vendor_no = ret;	
	}
	else if(prData[0]==0x20)//ST NAND Flash module
	{
		ret = NAND_ST;
		pstSysInfo->vendor_no = ret;
		//printk("NAND_TYPE: ST\n");
	}
	else if(prData[0]==0x98)//toshiba NAND Flash module
	{
		//pstSysInfo->u16TotalBlkNo = 4096;//default value
		pstSysInfo->u8Support_TwoPlan =0;//toshiba no support 2Plan
		ret = NAND_TOSHIBA;
		pstSysInfo->vendor_no = ret;
		//printk("NAND_TYPE: TOSHIBA\n");
		
	}
	else if(prData[0]==0x2c)//MICRON NAND Flash module
	{		
		//ReSet(); //for micron only
		//pstSysInfo->u16TotalBlkNo = 4096;//default value
		//pstSysInfo->u8Support_TwoPlan =0;
		ret = NAND_MICRON;
		pstSysInfo->vendor_no = ret;
		//printk("NAND_TYPE: MICRON\n");
	}
	else if(prData[0]==0x89)//INTEL NAND Flash module
	{		
		//ReSet(); //for micron only
		//pstSysInfo->u16TotalBlkNo = 4096;//default value
		//pstSysInfo->u8Support_TwoPlan =0;
		ret = NAND_INTEL;
		pstSysInfo->vendor_no = ret;
		SPMP_DEBUG_PRINT("NAND_TYPE: INTEL\n");
	}	
	else
	{
	   	pstSysInfo->u16PageSize = 2112;
		pstSysInfo->u16PyldLen = 2048;
		pstSysInfo->u16ReduntLen = 32;
		//pstSysInfo->u8SpareLen = 32;
		pstSysInfo->u16TotalBlkNo = 4096;//default value
		ret=NAND_UNKNOWN;
		pstSysInfo->vendor_no = ret;
		printk("NAND_TYPE: UNKNOWN. WARNING!\n");
	}

	pstSysInfo->u16TotalBlkNo >>= (pstSysInfo->u8Support_TwoPlan+ pstSysInfo->u8Support_Internal_Interleave);
	
	pstSysInfo->u8TotalBlkNoShift = nfvalshift(pstSysInfo->u16TotalBlkNo);

	//printk("2KING-->pstSysInfo->u16TotalBlkNo:%d\n",pstSysInfo->u16TotalBlkNo);
	//printk("2KING-->pstSysInfo->u8TotalBlkNoShift:%d\n",pstSysInfo->u8TotalBlkNoShift);

#if 0	
	#if 1
			pstSysInfo->u16ReduntLen = (pstSysInfo->u16PyldLen)>>5;
			pstSysInfo->u16Redunt_Sector_Addr_Offset = 32;//???????????????????
			pstSysInfo->u16Redunt_Sector_Len = 32;//???????????????????
			pstSysInfo->ecc_mode = BCH_S336_16_BIT;
			pstSysInfo->u16ReduntLenLog = (pstSysInfo->u16PyldLen)>>5;
	#else
			pstSysInfo->u16ReduntLen = 44*((pstSysInfo->u16PyldLen)>>10);
			pstSysInfo->u16Redunt_Sector_Addr_Offset = 64;//???????????????????
			pstSysInfo->u16Redunt_Sector_Len = 44;//???????????????????
			pstSysInfo->ecc_mode = BCH_S336_24_BIT; 
			pstSysInfo->u16ReduntLenLog = (pstSysInfo->u16PyldLen)>>4;
	#endif	
#else
		//printk("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
		print_sysinfo_t();	
		//printk("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");

		ReadNandInfoFromProfile();
	
		//pstSysInfo->ecc_mode = BCH_S336_16_BIT;
		setEccMode(pstSysInfo->ecc_mode);
#endif



	return ret;

}

unsigned int ReadPage_Test(void)
{
	unsigned char *pReadBuf, *pEccBuf;
	unsigned int addr1,addr2;

	SPMP_DEBUG_PRINT("begin ReadPage_Test\n");
		
	pReadBuf = (unsigned char*)rPayload;
	pEccBuf = (unsigned char*)g_eccBuf;
#if 0
	//	if((g_sysInfo_s.bootInfo_s.version&0xff0000ff)==0x5A0000A5)
	if((GetRomcodeVer() & 0xff0000ff)==0x5A0000A5)
	{
		addr1 = pstSysInfo->u16PageNoPerBlk;
		addr2 = 2*pstSysInfo->u16PageNoPerBlk;
	}
	else
	{
		addr1 = 0;
		addr2 = 2;
	}
#else
	addr1 = pstSysInfo->u16PageNoPerBlk;
	addr2 = 2*pstSysInfo->u16PageNoPerBlk;
#endif
	nand_cache_sync();
	if(ReadPage_storage(addr1, (unsigned long*)pReadBuf, (unsigned long*)pEccBuf)==FAILURE)
	{
		SPMP_DEBUG_PRINT("Test Failed: PageNo=%u\n",addr1);
		//DUMP_NF_BUFFER(debug_flag, pReadBuf,512, pEccBuf, 64);
		return FAILURE;	
	}
	if(ReadPage_storage(addr2, (unsigned long*)pReadBuf, (unsigned long*)pEccBuf)==FAILURE)
	{
		SPMP_DEBUG_PRINT("Test Failed: PageNo=%u\n",addr2);
		//DUMP_NF_BUFFER(debug_flag, pReadBuf,512, pEccBuf, 64);
		return FAILURE;			
	}
	SPMP_DEBUG_PRINT("Test Ok\n");
	//DUMP_NF_BUFFER(debug_flag, pReadBuf,512, pEccBuf, 64);
	return SUCCESS;
}

void AutoSettingACTiming(void)
{
#if 1
	int i;//clock_cycle; 
	//debug_flag = 0x1;
	for(i=0;i <16; i++)
	{
		rFM_AC_TIMING = 0x1f<<16 | i<<12 | i<<8 | i<<4 | i;
		//rFM_AC_TIMING = 0x1fffff;
		if(ReadPage_Test()==SUCCESS)
		{
			i++;
			rFM_AC_TIMING = 0x1f<<16 | i<<12 | i<<8 | i<<4 | i;
			printk("rFM_AC_TIMING:0x%x\n",rFM_AC_TIMING);
			//debug_flag = 0x101;
			break;
		}
		//debug_flag = 0x101;
	}

#if SUPPORT_MUTI_CHANNEL	
	rFM1_AC_TIMING = 0x1f<<16 | i<<12 | i<<8 | i<<4 | i;
#endif	

#else
	
	rFM_AC_TIMING = 0x1f4444;
#endif

	SPMP_DEBUG_PRINT("end of AutoSettingACTiming:0x%x\n",rFM_AC_TIMING);
}
void turnOnClockPipe_NAND(void)
{
#if 1///////////////////mark by king
#if SUPPORT_MUTI_CHANNEL	
	#if SUPPORT_CLOCK_MANAGER
		turnOnClockPipe(SW_CLK_NAND0);
		turnOnClockPipe(SW_CLK_NAND1);
		turnOnClockPipe(SW_CLK_BCH);
	#else
		// enable the clock of NAND0
		rSCU_A_PERI_CLKEN = (rSCU_A_PERI_CLKEN | (SCU_A_CLKEN_NAND0 | SCU_A_CLKEN_NAND1 | SCU_A_CLKEN_BCH | SCU_A_CLKEN_AAHB_M212 | SCU_A_CLKEN_NAND_ABT ));
	#endif
#else
	#if SUPPORT_CLOCK_MANAGER
		turnOnClockPipe(SW_CLK_NAND0);
		turnOnClockPipe(SW_CLK_BCH);
	#else
		rSCU_A_PERI_CLKEN = (rSCU_A_PERI_CLKEN | (SCU_A_CLKEN_NAND0 | SCU_A_CLKEN_BCH | SCU_A_CLKEN_AAHB_M212 | SCU_A_CLKEN_NAND_ABT ));
	#endif
#endif //#if SUPPORT_MUTI_CHANNEL
	
#endif	//#if 0///////////////////mark by king
}
void turnOffClockPipe_NAND(void)
{
#if 1
#if SUPPORT_MUTI_CHANNEL
	#if SUPPORT_CLOCK_MANAGER	
		//turnOffClockPipe(SW_CLK_DMAC0);
		turnOffClockPipe(SW_CLK_NAND0);
		turnOffClockPipe(SW_CLK_NAND1);
		turnOffClockPipe(SW_CLK_BCH);
	#endif	

#else
	#if SUPPORT_CLOCK_MANAGER		
		//turnOffClockPipe(SW_CLK_DMAC0);
		turnOffClockPipe(SW_CLK_NAND0);
		turnOffClockPipe(SW_CLK_BCH);
	#endif			
#endif	
#endif	//#if 0
}

/////////////////////////////////////////////////////////////
#define GPIO_DBG_BASE_ADDRESS 			0x90005000
#define GPIO_DBG_PAD							(GPIO_DBG_BASE_ADDRESS + 0x80)
#define GPIO_DBG_OEN							(GPIO_DBG_BASE_ADDRESS + 0x68)
#define GPIO_DBG_OUT							(GPIO_DBG_BASE_ADDRESS + 0x64)
#define rGPIO_DBG_PAD						(*(volatile unsigned *)GPIO_DBG_PAD)
#define rGPIO_DBG_OEN						(*(volatile unsigned *)GPIO_DBG_OEN)
#define rGPIO_DBG_OUT						(*(volatile unsigned *)GPIO_DBG_OUT)
/////////////////////////////////////////////////////////////
void setgpio_dbg(int idx)
{
	//0x90005080 [31:26]--> 0x3f
	rGPIO_DBG_PAD &= ~(0x3f<<26);
	rGPIO_DBG_PAD |= (0x2a<<26);

	//0x90005068 [10:0]-->0
	rGPIO_DBG_OEN &= ~(0x7ff);

	//0x90005064 [10:0]-->1	
	rGPIO_DBG_OUT &= ~(0x7ff);
	rGPIO_DBG_OUT |= 1<<idx;
}

NFFS_Block_info_t gnfs_block_info;

NFFS_Block_info_t* getmfsxx_block_info(void)
{
	return &gnfs_block_info;
}
void print_gnfs_block_info(void)
{
	if(debug_flag == 0)
		return;
	printk("###################sysinfo###################\n");
	printk("gnfs_block_info.sys_block.start:%u\n",gnfs_block_info.sys_block.start);
	printk("gnfs_block_info.sys_block.count:%u\n",gnfs_block_info.sys_block.count);
	printk("gnfs_block_info.rom_block.start:%u\n",gnfs_block_info.rom_block.start);
	printk("gnfs_block_info.rom_block.count:%u\n",gnfs_block_info.rom_block.count);
	printk("gnfs_block_info.rom1_block.start:%u\n",gnfs_block_info.rom1_block.start);
	printk("gnfs_block_info.rom1_block.count:%u\n",gnfs_block_info.rom1_block.count);
	printk("gnfs_block_info.rom_a_block.start:%u\n",gnfs_block_info.rom_a_block.start);
	printk("gnfs_block_info.rom_a_block.count:%u\n",gnfs_block_info.rom_a_block.count);
	printk("gnfs_block_info.npb_block.start:%u\n",gnfs_block_info.npb_block.start);
	printk("gnfs_block_info.npb_block.count:%u\n",gnfs_block_info.npb_block.count);
	printk("gnfs_block_info.user_block.start:%u\n",gnfs_block_info.user_block.start);
	printk("gnfs_block_info.user_block.count:%u\n",gnfs_block_info.user_block.count);
	printk("gnfs_block_info.page_per_block:%u\n",gnfs_block_info.page_per_block);
	printk("gnfs_block_info.page_size:%u\n",gnfs_block_info.page_size);
	printk("gnfs_block_info.blockshift:%u\n",gnfs_block_info.blockshift);
	printk("gnfs_block_info.pageshift:%u\n",gnfs_block_info.pageshift);
	printk("gnfs_block_info.u8Internal_Chip_Number:%u\n",gnfs_block_info.u8Internal_Chip_Number);
	printk("gnfs_block_info.u8MultiChannel:%u\n",gnfs_block_info.u8MultiChannel);
	printk("#############################################\n");	
}
unsigned short get_DiskstartBlk(void)
{
	
	unsigned char  u8CSR = 0x09;
	unsigned short u16InterMask = 0xf05d; //0xf05c;
	unsigned long u32ACTiming = 0x1f0000;// Set to the fast speed

	psysinfo_t* psysinfo = initDriver(u8CSR,u16InterMask,u32ACTiming);
	
	
	//gnfs_block_info.reserved_blk = 0;//MAX_FIFO_RESERVATIONS <<6;//4*8

	gnfs_block_info.page_per_block = psysinfo->u16PageNoPerBlk;

#if 0
	gnfs_block_info.page_size = psysinfo->u16PyldLen << psysinfo->u8Internal_Chip_Number << psysinfo->u8MultiChannel;//test by king weng
#else
	gnfs_block_info.page_size = psysinfo->u16PyldLen;//test by king weng
#endif

	//add for two paln
	gnfs_block_info.page_size <<= (psysinfo->u8Support_TwoPlan+psysinfo->u8Support_Internal_Interleave);
	//psysinfo->u16TotalBlkNo >>= (psysinfo->u8Support_TwoPlan+psysinfo->u8Support_Internal_Interleave);
	//end add

	gnfs_block_info.blockshift = valshift(psysinfo->u16PageNoPerBlk);// shift block addr to page addr

#if ROMFS_SUPPORT_2PLAN_INTERLEAVE
	gnfs_block_info.pageshift = valshift(gnfs_block_info.page_size);// shift page tp byte	
#else
	gnfs_block_info.pageshift = valshift(psysinfo->u16PyldLen);// shift page tp byte
#endif
	
	gnfs_block_info.u8Internal_Chip_Number = psysinfo->u8Internal_Chip_Number;	
	
	gnfs_block_info.u8MultiChannel = psysinfo->u8MultiChannel;	
	

	gnfs_block_info.sys_block.start=0;	
	gnfs_block_info.sys_block.count=NFFS_SYS_BLOCK_SIZE;//get_sysblksize();//

	gnfs_block_info.rom_block.start=gnfs_block_info.sys_block.count;	
	gnfs_block_info.rom_block.count=ROMFS_MAX_SIZE>>(gnfs_block_info.blockshift+gnfs_block_info.pageshift);
	//gnfs_block_info.rom_block.count=NFRC_Get_MaxRomSize();//>>(gnfs_block_info.blockshift+gnfs_block_info.pageshift);
	//gnfs_block_info.rom_block.count=NFRC_Get_RomFs_Max_BlkCount();
	
	//Info.rom_block.count=RIMFS_MAX_SIZE>>(Info.blockshift+Info.pageshift);

	gnfs_block_info.rom1_block.start=gnfs_block_info.rom_block.start + gnfs_block_info.rom_block.count;	
#if SUPPORT_DUAL_ROM_IMAGE
	gnfs_block_info.rom1_block.count=gnfs_block_info.rom_block.count;
#else
	gnfs_block_info.rom1_block.count=0;
#endif

	gnfs_block_info.rom_a_block.start = gnfs_block_info.rom1_block.start + gnfs_block_info.rom1_block.count;
	gnfs_block_info.rom_a_block.count = 0;//NFRC_Get_RomFs1_Max_BlkCount();

	gnfs_block_info.npb_block.start = gnfs_block_info.rom_a_block.start + gnfs_block_info.rom_a_block.count;
	gnfs_block_info.npb_block.count = NAND_PGBASE_USED_PHYBLK+NAND_PGBASE_SAVEBLK;

#if ROMFS_SUPPORT_2PLAN_INTERLEAVE
#else
	gnfs_block_info.npb_block.start >>= psysinfo->u8Support_TwoPlan;//add by king weng 2008/12/16
#endif	

	gnfs_block_info.user_block.start = gnfs_block_info.npb_block.start + gnfs_block_info.npb_block.count;
	gnfs_block_info.user_block.count = psysinfo->u16TotalBlkNo- gnfs_block_info.user_block.start;

//printk("$$$$$$$$$$$$$gnfs_block_info.user_block.start:%d\n",gnfs_block_info.user_block.start);

	print_gnfs_block_info();

	return gnfs_block_info.user_block.start;
}

void print_sysinfo_t(void)
{
	if(debug_flag == 0)
		return;
	printk("###################sysinfo###################\n");
	printk("pstSysInfo->u16PageNoPerBlk:%d\n",pstSysInfo->u16PageNoPerBlk);
	printk("pstSysInfo->u16PageSize:%d\n",pstSysInfo->u16PageSize);
	printk("pstSysInfo->u16PyldLen:%d\n",pstSysInfo->u16PyldLen);
	printk("pstSysInfo->u16ReduntLen:%d\n",pstSysInfo->u16ReduntLen);
	printk("pstSysInfo->u16Redunt_Sector_Addr_Offset:%d\n",pstSysInfo->u16Redunt_Sector_Addr_Offset);
	printk("pstSysInfo->u16Redunt_Sector_Len:%d\n",pstSysInfo->u16Redunt_Sector_Len);

	printk("pstSysInfo->u16TotalBlkNo:%d\n",pstSysInfo->u16TotalBlkNo);
	printk("pstSysInfo->u16InterruptMask:%d\n",pstSysInfo->u16InterruptMask);

	printk("pstSysInfo->u8TotalBlkNoShift:%d\n",pstSysInfo->u8TotalBlkNoShift);
	printk("pstSysInfo->u8MultiChannel:%d\n",pstSysInfo->u8MultiChannel);
	printk("pstSysInfo->u8Support_Internal_Interleave:%d\n",pstSysInfo->u8Support_Internal_Interleave);
	printk("pstSysInfo->u8Support_External_Interleave:%d\n",pstSysInfo->u8Support_External_Interleave);
	printk("pstSysInfo->u8Internal_Chip_Number:%d\n",pstSysInfo->u8Internal_Chip_Number);
	printk("pstSysInfo->u8PagePerBlkShift:%d\n",pstSysInfo->u8PagePerBlkShift);
	printk("pstSysInfo->u8Support_TwoPlan:%d\n",pstSysInfo->u8Support_TwoPlan);	

	printk("pstSysInfo->ecc_mode :%d\n",pstSysInfo->ecc_mode);	

#if SUPPORT_INTERRUPT	
	printk("SUPPORT_INTERRUPT\n");
#endif	

	printk("#############################################\n");	
}

#define B_KEYSCAN0 0//(1<<0)
#define B_KEYSCAN1 1//(1<<1)
#define B_KEYSCAN2 2//(1<<1)
#define B_KEYSCAN3 3//(1<<1)

#define B_GPIO1 8


#define NAND_CH0_CS_2 B_KEYSCAN1
#define NAND_CH0_CS_3 B_KEYSCAN0

void init_CS_gpio(void)
{
#ifndef	CYGPKG_REDBOOT
#if 0
	gpio_init();

	gpio_en_pin(0, NAND_CH0_CS_2, 0, 0, 0);
	gpio_en_pin(0, NAND_CH0_CS_3, 0, 0, 0);

	scu_change_pin_grp(18, 3);
	scu_change_pin_grp(19, 3);
#elif 0
	int gpio_handle= 0 ;
	unsigned int pin_index = 0;

	pin_index = ;
	gpio_handle = gp_gpio_request();
#else
	return;
#endif
#endif	
}
int nf_wp_switch(unsigned char flag)
{
	gp_board_nand_t*pConfig;

	pConfig = gp_board_get_config("nand", gp_board_nand_t);
	pConfig->set_wp(flag);

	return 0;
}
NFC_StartAddr_Map nf_addr_remap;
ppsysinfo_t initDriver(unsigned char u8CSR ,unsigned short u16InterMask,unsigned long u32ACTiming)
{
	//unsigned char *prPyldNc; 
	unsigned char *prPyld, *prPyldNc, i, j;

	if(g_IsInit_nf)
	{
		//printk("g_IsInit_nf:%d\n",g_IsInit_nf);
		return pstSysInfo;
	}

	init_CS_gpio();//add for change gpio

	g_IsInit_nf = 1;
	gIsSingle_CS = 1;
	if( (nf_addr_remap.bch_virtAddr==0) || (nf_addr_remap.nf_virtAddr==0))
	{
		SPMP_DEBUG_PRINT("Addr remap Err\n");
		return NULL;
	}
	
	nf_drv_mutex_init(&nf_lock);
	nf_drv_mutex_init(&bch_lock);
	nf_drv_mutex_init(&state_lock);
#if (PAGE_SIZE < 4096)
//粗略分类，可以更细化
	g_DescInfo = (unsigned char *) __get_free_pages(GFP_KERNEL, 1);
	if (g_DescInfo==NULL) 
	{
		SPMP_DEBUG_PRINT("__get_free_page failed\n");
		return NULL;
	}
	g_eccBuf = (unsigned char *)gp_bchtmpbuf + 512;
	gp_bchtmpbuf = g_eccBuf + 512;
	rPayload = (unsigned char *) __get_free_pages(GFP_KERNEL, 2);
	if (rPayload==NULL)
	{
		free_page((unsigned long)g_DescInfo);
		SPMP_DEBUG_PRINT("__get_free_page failed\n");
		return NULL;
	}
#else
	g_DescInfo = (DescInfo_t  *)__get_free_page(GFP_KERNEL);
	if (g_DescInfo==NULL) 
	{
		SPMP_DEBUG_PRINT("__get_free_page failed\n");
		return NULL;
	}
	//g_eccBuf = (unsigned char *)gp_bchtmpbuf + 512;
	g_eccBuf = (unsigned char *)__get_free_page(GFP_KERNEL);
	//gp_bchtmpbuf = g_eccBuf + 1024;
	gp_bchtmpbuf = (unsigned char *)__get_free_page(GFP_KERNEL);
	rPayload = (unsigned char *) __get_free_pages(GFP_KERNEL, 1);
	if (rPayload==NULL)
	{
		free_page((unsigned long)g_DescInfo);
		SPMP_DEBUG_PRINT("__get_free_page failed\n");
		return NULL;
	}
	
#endif
	SPMP_DEBUG_PRINT("PAGE_SIZE=%lu %08x %08x %08x %08x\n", PAGE_SIZE, (unsigned int)g_DescInfo, (unsigned int)rPayload,(unsigned int)g_eccBuf, (unsigned int)gp_bchtmpbuf);
#if WRITE_AND_GO
	SetStatus(NFD_IDLE);
#endif

	//turnOnClockPipe_NAND();
	rSCU_A_PERI_CLKEN = (rSCU_A_PERI_CLKEN | (SCU_A_CLKEN_NAND0 | SCU_A_CLKEN_NAND1 | SCU_A_CLKEN_BCH | SCU_A_CLKEN_AAHB_M212 | SCU_A_CLKEN_NAND_ABT ));


	pstSysInfo = &g_stSysInfo;
	
	u16InterMask = 0x5d;//add by king for test	
	pstSysInfo->u16InterruptMask = u16InterMask;
	
	rFM_CSR		  = u8CSR;
	rFM_INTRMSK   = u16InterMask;
	rFM_AC_TIMING = 0x1fffff;
	
	//rFM_RDYBSY_EN |= (2<<4);
	rFM_RDYBSY_EN |= 0xF;//4rd -> 1
	//rFM_RDYBSY_DLY_INT |=(1<<8);

#if SUPPORT_MUTI_CHANNEL	
	rFM1_CSR		  = u8CSR;
	rFM1_INTRMSK   = u16InterMask;
	rFM_AC_TIMING = 0x1fffff;
#endif

#if SUPPORT_INTERRUPT
	g_nf_ctrl_CH0_flag = 0;
	init_waitqueue_head(&s330_queue0);
	if(nf0_init_intr())
	{
		printk("nf: unable to get IRQ%d for the hard disk driver\n", IRQ_NAND0);
		if(rPayload !=NULL)
			free_page((unsigned long)rPayload);
		if(g_DescInfo !=NULL)
			free_page((unsigned long)g_DescInfo);
		return NULL;
	}
#if SUPPORT_MUTI_CHANNEL
	g_nf_ctrl_CH1_flag = 0;
	init_waitqueue_head(&s330_queue1);
	if (nf1_init_intr())
	{
		free_irq(IRQ_NAND0, NULL);

		if(rPayload !=NULL)
			free_page(rPayload);
		if(g_eccBuf !=NULL)
			free_page(g_eccBuf);
		if(g_DescInfo !=NULL)
			free_page(g_DescInfo);
		printk("nf: unable to get IRQ%d for the hard disk driver\n", IRQ_NAND1);
		return NULL;
	}
#endif	//SUPPORT_MUTI_CHANNEL
#endif	//SUPPORT_INTERRUPT

	bch_init_intr();

	prPyld 		= (unsigned char*)&rPayload[0];

	prPyldNc = Cache2NonCacheAddr(prPyld);
	pstSysInfo->u8Internal_Chip_Number = 0;// =2 ???
	pstSysInfo->u8MultiChannel=0;
	
#if SUPPORT_MUTI_CHANNEL
	for(i=0;i<SUPPORT_MAXCS;i++)
	{
		ReadID(DEV_CH1, i, (unsigned long)prPyld);
		if(prPyldNc[0])
		{
			pstSysInfo->u8MultiChannel=1;
			
			if(prPyld[0]==0x2c)//MICRON NAND Flash module
			{	
				int count=10;
				ReSet(DEV_CH1,i);
				while(count--)
				{
					ReadID(DEV_CH1, i, (unsigned long)prPyld);
				}	
			}	
			
			//printk("ReadID(%d, %d)",DEV_CH1, i);
			//print_ID(prPyldNc);
			//printk("######################################\n\n");			
		}
	}	
#endif	
		
	//printk("pstSysInfo->u8MultiChannel : %d\n",pstSysInfo->u8MultiChannel)	;

	for(i=0;i<SUPPORT_MAXCS;i++)
	{
		g_ChipMap[i]= 0xff;	
	}
	
	rFM_INTR_STS = 0xffff;	

	pstSysInfo->u8Internal_Chip_Number = 0;

	for(i=0,j=0;i<SUPPORT_MAXCS;i++)
	{
		memset(prPyldNc,0,32);
		ReadID(DEV_CH0, i, (unsigned long)prPyld);
		if(prPyldNc[0])
		{
			g_ChipMap[j]=i;
			ReSet(DEV_CH0,i);

			j++;
			pstSysInfo->u8Internal_Chip_Number++;
			//pstSysInfo->u8Internal_Chip_Number++;
			//printk("ReadID(%d, %d)",DEV_CH0, i);
			//print_ID(prPyldNc); 
			//printk("######################################\n\n");
		}
	}

	for(i=0; i<SUPPORT_MAXCS; i++)
	{
		printk("g_ChipMap[%d]=%d\n",i , g_ChipMap[i]);
	}

	if(pstSysInfo->u8Internal_Chip_Number)
	{
		pstSysInfo->u8Internal_Chip_Number = nfvalshift(pstSysInfo->u8Internal_Chip_Number);
	}
	else	
	{
		pstSysInfo->u8Internal_Chip_Number = 0xff;
		printk("ERROR : Can't found any chip!\n");
	}	
	
	
	ReadID(DEV_CH0, 0,(unsigned long)prPyld);
	
	//print_ID(prPyldNc);	

	prPyldNc = Cache2NonCacheAddr(prPyld);

	//print_ID(prPyldNc);

	setSystemPara(prPyldNc);
	
	//reset desc
	memset(gp_Desc, 0, MAX_DESC_SIZE * DESC_OFFSET);
	
	//printk("pstSysInfo->u8Internal_Chip_Number:%d\n",pstSysInfo->u8Internal_Chip_Number);
	print_sysinfo_t();	
	
#if SUPPORT_AUTO_SETTINGACTIMING	
	AutoSettingACTiming();
#else	
	rFM_AC_TIMING = u32ACTiming;
	
	#if SUPPORT_MUTI_CHANNEL	
		rFM1_AC_TIMING = u32ACTiming;
	#endif	
	
#endif//SUPPORT_AUTO_SETTINGACTIMING	


	//rFM_AC_TIMING = u32ACTiming;
	
	rFM_INTRMSK   = 0xf05c;

	nf_wp_switch(1);
	return pstSysInfo;

}

//////////////////////////////////////////////////////////////////////////////////////

#define NAND_CM_CS_2 B_KEYSCAN1
#define NAND_CM_CS_3 B_KEYSCAN2
#define NAND_CM_CS_4 B_GPIO1

#define NAND_CS0 0
#define NAND_CS1 1
#define NAND_CS2 2
#define NAND_CS3 3
#define NAND_CS4 4

#if 0
void init_CM_gpio(void)
{
	g_IsCM = 1;

	gpio_init();

	gpio_en_pin(0, NAND_CM_CS_2, 0, 0, 0);
	gpio_en_pin(0, NAND_CM_CS_3, 0, 0, 0);
	gpio_en_pin(0, NAND_CM_CS_4, 0, 0, 0);	

	scu_change_pin_grp(12, 0);
	scu_change_pin_grp(19, 0);
	scu_change_pin_grp(20, 0);	

	gpio_set_data(0, NAND_CM_CS_2, 1);
	gpio_set_data(0, NAND_CM_CS_3, 1);
	gpio_set_data(0, NAND_CM_CS_4, 1);

	
}
#endif

void ChipEnable(unsigned char cs)
{
#ifndef	CYGPKG_REDBOOT
	if(g_IsCM)
	{

		switch(cs)
		{
			case NAND_CS2:
				gpio_set_data(0, NAND_CM_CS_2, 0);
				gpio_set_data(0, NAND_CM_CS_3, 1);
				gpio_set_data(0, NAND_CM_CS_4, 1);
				break;
		
			case NAND_CS3:
				gpio_set_data(0, NAND_CM_CS_2, 1);
				gpio_set_data(0, NAND_CM_CS_3, 0);
				gpio_set_data(0, NAND_CM_CS_4, 1);
				break;
		
			case NAND_CS4:
				gpio_set_data(0, NAND_CM_CS_2, 1);
				gpio_set_data(0, NAND_CM_CS_3, 1);
				gpio_set_data(0, NAND_CM_CS_4, 0);
				break;
		
		
			default:
				gpio_set_data(0, NAND_CM_CS_2, 1);
				gpio_set_data(0, NAND_CM_CS_3, 1);
				gpio_set_data(0, NAND_CM_CS_4, 1);
				break;
		}
	}
#endif	
}


ppsysinfo_t initDriver_ex(unsigned int begin_cs_idx, unsigned int cs_count)
{
	unsigned int rc = FAILURE;
	unsigned char *prPyld, *prPyldNc,i,j;
	unsigned char u8Internal_Chip_Number = 0;

	SPMP_DEBUG_PRINT("initDriver_ex(%d, %d)\n",begin_cs_idx ,cs_count);
#if 0

	initDriver(0x09, 0xf05d, 0x1f0000);

	init_CM_gpio();


	prPyld 		= (unsigned char*)&rPayload[0];

	prPyldNc = Cache2NonCacheAddr(prPyld);
#else
	g_IsInit_nf = 1;
	gIsSingle_CS = 1;
		
	nf_drv_mutex_init(&nf_lock);
	nf_drv_mutex_init(&bch_lock);
	nf_drv_mutex_init(&state_lock);

#if WRITE_AND_GO
	SetStatus(NFD_IDLE);
#endif

	//turnOnClockPipe_NAND();
	rSCU_A_PERI_CLKEN = (rSCU_A_PERI_CLKEN | (SCU_A_CLKEN_NAND0 | SCU_A_CLKEN_NAND1 | SCU_A_CLKEN_BCH | SCU_A_CLKEN_AAHB_M212 | SCU_A_CLKEN_NAND_ABT ));

	pstSysInfo = &g_stSysInfo;
	
	//u16InterMask = 0x5d;//add by king for test	
	pstSysInfo->u16InterruptMask = 0x5d;
	
	rFM_CSR		  = 0x09;
	rFM_INTRMSK   = 0x5d;;
	rFM_AC_TIMING = 0x1fffff;
	
	//rFM_RDYBSY_EN |= (2<<4);
	rFM_RDYBSY_EN |= 0xF;//4rd -> 1
	//rFM_RDYBSY_DLY_INT |=(1<<8);
	

#if SUPPORT_INTERRUPT
	SPMP_DEBUG_PRINT("SUPPORT_INTERRUPT!\n");

	g_nf_ctrl_CH0_flag = 0;
	init_waitqueue_head(&s330_queue0);
	if(nf0_init_intr())
	{
		printk("hd: unable to get IRQ%d for the hard disk driver\n", IRQ_NAND0);
		//if(rPayload !=NULL)
		//	free_page((unsigned long)rPayload);
		//if(g_DescInfo !=NULL)
		//	free_page((unsigned long)g_DescInfo);
		return NULL;
	}
#endif
	bch_init_intr();

	prPyld 		= (unsigned char*)&rPayload[0];

	prPyldNc = Cache2NonCacheAddr(prPyld);
	pstSysInfo->u8Internal_Chip_Number = 0;
	pstSysInfo->u8MultiChannel=0;


#endif

	init_CS_gpio();//add for change gpio


	for(i=0;i<4;i++)
	{
		g_ChipMap[i]= 0xff;	
	}

	u8Internal_Chip_Number = 0;
	for(i=0,j=0; i<cs_count; i++)
	{
		int cs = i + begin_cs_idx;
		//printk("ReadID(%d)\n", cs);
		ReadID(DEV_CH0, cs, (unsigned long)prPyld);
		if(prPyldNc[0])
		{
			rc = SUCCESS;
			g_ChipMap[j]=cs;
			j++;
			u8Internal_Chip_Number++;

			//printk("ReadID(%d, %d)",DEV_CH0, cs);
			//print_ID(prPyldNc);		
#if 1
			//if(prPyldNc[0]==0x2c)//MICRON NAND Flash module
			if(prPyldNc[0]==0x2c || prPyldNc[0]==0x89)//MICRON NAND Flash module
			{	
				ReSet(DEV_CH0,cs);
				printk("ReSet(%d)\n",cs);		
			}
			

	
			//printk("######################################\n\n");
#endif			
		}
	}
	
	if(rc == FAILURE)
	{
		return NULL;	
	}
	
	for(i=0; i<4; i++)
	{
		printk("g_ChipMap[%d]=%d\n",i , g_ChipMap[i]);
	}

	//prPyldNc = Cache2NonCacheAddr(prPyld);
	
	memset(prPyld,0,32);
	ReadID(DEV_CH0, begin_cs_idx,(unsigned long)prPyld);
	
	if(u8Internal_Chip_Number)
	{
		pstSysInfo->u8Internal_Chip_Number = valshift(u8Internal_Chip_Number);
	}
	else	
	{
		pstSysInfo->u8Internal_Chip_Number = 0xff;
		printk("ERROR : Can't found any chip!\n");
	}		
	
	
	//print_ID(prPyldNc);	



	setSystemPara(prPyldNc);

	print_sysinfo_t();

	return pstSysInfo;

}




#define MENU_RWDESC_SIZE (5*DESC_OFFSET)
int manuReadPage(unsigned char ch, unsigned char chip_number, unsigned long u32PhyAddr,
		unsigned long* PyldBuffer, unsigned long* DataBuffer, unsigned char u8RWMode, unsigned char Last)
{
	unsigned long *PhysAddr1 = 0 ;
	unsigned long *PhysAddr2 = 0 ;
	
	InitDesc(ch);

	PhysAddr1 = (unsigned long *)virt_to_phys((void*)PyldBuffer);
	PhysAddr2 = (unsigned long *)virt_to_phys((void*)DataBuffer);
	SPMP_DEBUG_PRINT("manuReadPage(%d, %lu)\n", ch, u32PhyAddr);
	//printk("PyldBuffer=%x(=>%x) DataBuffer=%x(=>%x) \n", PyldBuffer, PhysAddr1, DataBuffer, PhysAddr2);

	//CMD phase
	gp_Desc[DESC_CMD] = (unsigned long) ((MAUNCMD_READ) | ((1 << chip_number) << 24) | (CMD_TYPE_MANUAL_MODE_CMD << 28));
	gp_Desc[DESC_INTERRUPT] = (pstSysInfo->u16InterruptMask << 16);
	gp_Desc[DESC_ADDR1] = ((MANUAL_MODE_OWNER | MANUAL_MODE_LAST_SECTOR | MANUAL_MODE_REDUNT_ENABLE) << 24);
	gp_Desc[DESC_ADDR2] = 0x00 | (gNextDesBP += sizeof(DescInfo_t));
	gp_Desc += DESC_OFFSET;

	//Manual ADDR
	gp_Desc[DESC_CMD] = (unsigned long) (((1 << chip_number) << 24) | (CMD_TYPE_MANUAL_MODE_ADDR << 28));
	gp_Desc[DESC_INTERRUPT] = (pstSysInfo->u16InterruptMask << 16);
#define READWRITE_ADDRNUMBER 4
	gp_Desc[DESC_ADDR1] = (unsigned long)(((MANUAL_MODE_OWNER | MANUAL_MODE_LAST_SECTOR | MANUAL_MODE_REDUNT_ENABLE | READWRITE_ADDRNUMBER)<<24) |u32PhyAddr) ;
	gp_Desc[DESC_ADDR2] = (unsigned long)((gNextDesBP+=sizeof(DescInfo_t))) ;		
	gp_Desc += DESC_OFFSET;

	////Manual CMD(write Conf.)
	gp_Desc[DESC_CMD] = (unsigned long) (MAUNCMD_CONF_READ | ((1 << chip_number) << 24) | (CMD_TYPE_MANUAL_MODE_CMD << 28));
	gp_Desc[DESC_INTERRUPT] = (pstSysInfo->u16InterruptMask << 16);
	gp_Desc[DESC_ADDR1] = ((MANUAL_MODE_OWNER | MANUAL_MODE_END_DESC | MANUAL_MODE_LAST_SECTOR | MANUAL_MODE_REDUNT_ENABLE) << 24);
	gp_Desc[DESC_ADDR2] = 0;

	trigger(ch);

	//hal_delay_us(200);
	//print_Desc();
	PollingAndsetStatus(ch, NFD_READWRITE);
	//printk("\n");

	InitDesc(ch);
	//Manual WPYLD
	gp_Desc[DESC_CMD] = (unsigned long) (((1 << chip_number) << 24) | (CMD_TYPE_MANUAL_MODE_PYLOAD_READ << 28));
	gp_Desc[DESC_INTERRUPT] = (pstSysInfo->u16InterruptMask << 16);
	gp_Desc[DESC_ADDR1] = ((MANUAL_MODE_OWNER | MANUAL_MODE_LAST_SECTOR | MANUAL_MODE_REDUNT_ENABLE) << 24);
	gp_Desc[DESC_ADDR2] = 0x00 | (gNextDesBP += sizeof(DescInfo_t));
	gp_Desc[DESC_LENGTH] = (unsigned long) (pstSysInfo->u16PyldLen << 16);
	//gp_Desc[DESC_PAYLOAD] = (unsigned long) PyldBuffer;
	gp_Desc[DESC_PAYLOAD] = (unsigned long) PhysAddr1;
	gp_Desc += DESC_OFFSET;

	//Manual WREDUT
	gp_Desc[DESC_CMD] = (unsigned long) (((1 << chip_number) << 24) | (CMD_TYPE_MANUAL_MODE_PYLOAD_READ << 28));
	gp_Desc[DESC_INTERRUPT] = (pstSysInfo->u16InterruptMask << 16);
	gp_Desc[DESC_ADDR1] = ((MANUAL_MODE_OWNER | MANUAL_MODE_LAST_SECTOR | MANUAL_MODE_REDUNT_ENABLE) << 24);
	gp_Desc[DESC_ADDR2] = 0x00 | (gNextDesBP += sizeof(DescInfo_t));
	gp_Desc[DESC_LENGTH] = (unsigned long) ((pstSysInfo->u16ReduntLen) << 16);
	gp_Desc[DESC_PAYLOAD] = (unsigned long) PhysAddr2;
	gp_Desc[DESC_REDUNT_INFO] = (pstSysInfo->u16Redunt_Sector_Addr_Offset<<16) |pstSysInfo->u16Redunt_Sector_Len ;

#if 0
	gp_Desc += DESC_OFFSET;


	//Manual WSPARE
	gp_Desc[DESC_CMD] = (unsigned long) (((1 << chip_number) << 24) | (CMD_TYPE_MANUAL_MODE_PYLOAD_READ << 28));
	gp_Desc[DESC_INTERRUPT] = (pstSysInfo->u16InterruptMask << 16);
	gp_Desc[DESC_ADDR1] = ((MANUAL_MODE_OWNER | MANUAL_MODE_END_DESC | MANUAL_MODE_LAST_SECTOR | 
MANUAL_MODE_REDUNT_ENABLE) << 24);
	gp_Desc[DESC_ADDR2] = 0;
	gp_Desc[DESC_LENGTH] = (unsigned long) ((pstSysInfo->u16ReduntLen) << 16);
	//gp_Desc[DESC_PAYLOAD] = (unsigned long) DataBuffer + (pstSysInfo->u16ReduntLen);
	gp_Desc[DESC_PAYLOAD] = (unsigned long) PhysAddr2 + (pstSysInfo->u16ReduntLen);

	//printk("");
	//gp_Desc[DESC_REDUNT] = 0;
	//gp_Desc[DESC_SPARE] = 0;
#endif	
	trigger(ch);
	//printk("$$$$$$$$$$$$$$$$$\n");
	//print_Desc();

	enterPollingStatech(ch);
	//printk("222\n");
	//printk("enterPollingStatech_ex\n");
	/*
	if((debug_flag&0xff) == 1)
	{
		SPMP_DEBUG_PRINT("u32PhyAddr=%lu sector_num=%u\n", u32PhyAddr, debug_flag&0xf00);
		NF_print_buf((unsigned char *)PyldBuffer + (debug_flag&0xf00)*2, 512);
	}
	*/
	return 0;
}

int manuReadWritePage(unsigned char ch, unsigned char chip_number, unsigned long u32PhyAddr, unsigned long* PyldBuffer, unsigned long* DataBuffer, unsigned char u8RWMode, unsigned char Last)
{
	unsigned long *PhysAddr1 = 0 ;
	unsigned long *PhysAddr2 = 0 ;

	PhysAddr1 = (unsigned long *)virt_to_phys((void*)PyldBuffer);
	PhysAddr2 = (unsigned long *)virt_to_phys((void*)DataBuffer);
	
	//InitDesc(ch);
	memset(gp_Desc, 0, MENU_RWDESC_SIZE);
	//CMD phase
	gp_Desc[DESC_CMD] = (unsigned long)((MAUNCMD_WRITE) |((1<<chip_number)<<24) | ( CMD_TYPE_MANUAL_MODE_CMD <<28));
	gp_Desc[DESC_INTERRUPT] = (pstSysInfo->u16InterruptMask<<16);	
	gp_Desc[DESC_ADDR1] = ((MANUAL_MODE_OWNER | MANUAL_MODE_LAST_SECTOR | MANUAL_MODE_REDUNT_ENABLE)<<24);
	gp_Desc[DESC_ADDR2] = 0x00|(gNextDesBP+=sizeof(DescInfo_t));
	gp_Desc += DESC_OFFSET;	
		
	//Manual ADDR
	gp_Desc[DESC_CMD] = (unsigned long)(((1<<chip_number)<<24) | (CMD_TYPE_MANUAL_MODE_ADDR<<28));
	gp_Desc[DESC_INTERRUPT] = (pstSysInfo->u16InterruptMask<<16);	
	#define READWRITE_ADDRNUMBER 4
	gp_Desc[DESC_ADDR1] = (unsigned long)(((MANUAL_MODE_OWNER | MANUAL_MODE_LAST_SECTOR | MANUAL_MODE_REDUNT_ENABLE | READWRITE_ADDRNUMBER)<<24) |u32PhyAddr) ;
	gp_Desc[DESC_ADDR2] = (unsigned long)((gNextDesBP+=sizeof(DescInfo_t))) ;		
	gp_Desc += DESC_OFFSET;	
		
	
	//Manual WPYLD
	gp_Desc[DESC_CMD] = (unsigned long)(((1<<chip_number)<<24) | (CMD_TYPE_MANUAL_MODE_PYLOAD_WRITE<<28));
	gp_Desc[DESC_INTERRUPT] = (pstSysInfo->u16InterruptMask<<16);	
	gp_Desc[DESC_ADDR1] = ((MANUAL_MODE_OWNER | MANUAL_MODE_LAST_SECTOR | MANUAL_MODE_REDUNT_ENABLE)<<24);
	gp_Desc[DESC_ADDR2] = 0x00|(gNextDesBP+=sizeof(DescInfo_t));
	gp_Desc[DESC_LENGTH] = (unsigned long)(pstSysInfo->u16PyldLen<<16);
	//gp_Desc[DESC_PAYLOAD] = (unsigned long)PyldBuffer ; 
	gp_Desc[DESC_PAYLOAD] = (unsigned long)PhysAddr1;
	gp_Desc += DESC_OFFSET;	
	
	//Manual WREDUT
	gp_Desc[DESC_CMD] = (unsigned long)(((1<<chip_number)<<24) | (CMD_TYPE_MANUAL_MODE_REDUNT_WRITE<<28));
	gp_Desc[DESC_INTERRUPT] = (pstSysInfo->u16InterruptMask<<16);	
	gp_Desc[DESC_ADDR1] = ((MANUAL_MODE_OWNER | MANUAL_MODE_LAST_SECTOR | MANUAL_MODE_REDUNT_ENABLE)<<24);
	gp_Desc[DESC_ADDR2] = 0x00|(gNextDesBP+=sizeof(DescInfo_t));
	gp_Desc[DESC_LENGTH] = (unsigned long)((pstSysInfo->u16ReduntLen));
	//gp_Desc[DESC_REDUNT] = (unsigned long)DataBuffer ; 
	gp_Desc[DESC_REDUNT] = (unsigned long)PhysAddr2;
	gp_Desc[DESC_REDUNT_INFO] = (pstSysInfo->u16Redunt_Sector_Addr_Offset<<16) |pstSysInfo->u16Redunt_Sector_Len ;

	gp_Desc += DESC_OFFSET;		
		
	////Manual CMD(write Conf.)
	gp_Desc[DESC_CMD] = (unsigned long)(MAUNCMD_CONF_WRITE | ((chip_number+1)<<24) | (CMD_TYPE_MANUAL_MODE_CMD<<28));
	
	if(Last)
	{
		gp_Desc[DESC_ADDR1] = ((MANUAL_MODE_OWNER | MANUAL_MODE_END_DESC | MANUAL_MODE_LAST_SECTOR | MANUAL_MODE_REDUNT_ENABLE)<<24);
		gp_Desc[DESC_ADDR2] = 0;
		trigger(ch);
	}
	else
	{
		gp_Desc[DESC_ADDR1] = ((MANUAL_MODE_OWNER | MANUAL_MODE_LAST_SECTOR | MANUAL_MODE_REDUNT_ENABLE)<<24);
		gp_Desc[DESC_ADDR2] = 0x00|(gNextDesBP+=sizeof(DescInfo_t));
		gp_Desc += DESC_OFFSET;			
	}
	
	return SUCCESS;
}
int HalfAutoWritePage(unsigned char ch, unsigned char chip_number, unsigned long u32PhyAddr, unsigned long* PyldBuffer, unsigned long* DataBuffer, unsigned char u8RWMode, unsigned char Last)
{
	unsigned long *PhysAddr1 = 0 ;
	unsigned long *PhysAddr2 = 0 ;

	PhysAddr1 = (unsigned long *)virt_to_phys((void*)PyldBuffer);
	PhysAddr2 = (unsigned long *)virt_to_phys((void*)DataBuffer);

	//printk("autoReadWritePage(%d ,%d)\n",u32PhyAddr, u8RWMode);
	memset(gp_Desc, 0, sizeof(DescInfo_t));

	gp_Desc[DESC_CMD] = (unsigned long)(AUTOCMD_WRITE) | ((1<<chip_number)<<24) | (CMD_TYPE_HALFAUTO_WRITE<<28);		

	gp_Desc[DESC_LENGTH] = (unsigned long)((pstSysInfo->u16PyldLen<<16) | pstSysInfo->u16ReduntLen);	

	gp_Desc[DESC_INTERRUPT] = 0 ;

	//gp_Desc[DESC_PAYLOAD] = (unsigned long)PyldBuffer ;
	//gp_Desc[DESC_REDUNT] = (unsigned long)DataBuffer ;
	gp_Desc[DESC_PAYLOAD] = (unsigned long)PhysAddr1;
	gp_Desc[DESC_REDUNT] = (unsigned long)PhysAddr2;
	gp_Desc[DESC_REDUNT_INFO] = (pstSysInfo->u16Redunt_Sector_Addr_Offset<<16) |pstSysInfo->u16Redunt_Sector_Len ;

	if(Last)
	{
		gp_Desc[DESC_INTERRUPT] = (unsigned long)(pstSysInfo->u16InterruptMask<<16) ;
		gp_Desc[DESC_ADDR1] = (unsigned long)((0xF4<<24) | u32PhyAddr) ;
		gp_Desc[DESC_ADDR2] = 0 ;
		trigger(ch);		
	}
	else
	{
		gp_Desc[DESC_ADDR1] = (unsigned long)((0xB4<<24) |u32PhyAddr) ;
		gp_Desc[DESC_ADDR2] = (unsigned long)(gNextDesBP+=sizeof(DescInfo_t)) ;	
		gp_Desc+=DESC_OFFSET;
	}
	
	return SUCCESS;
}

int autoReadWritePage(unsigned char ch, unsigned char chip_number, unsigned long u32PhyAddr, unsigned long* PyldBuffer, unsigned long* DataBuffer, unsigned char u8RWMode, unsigned char Last)
{
	unsigned long *PhysAddr1 = 0 ;
	unsigned long *PhysAddr2 = 0 ;

	PhysAddr1 = (unsigned long *)virt_to_phys((void*)PyldBuffer);
	PhysAddr2 = (unsigned long *)virt_to_phys((void*)DataBuffer);
	//printk("autoReadWritePage(%d, %d, %d ,%d)\n", ch, chip_number, u32PhyAddr, u8RWMode);
	memset(gp_Desc, 0, sizeof(DescInfo_t));
	
	if(u8RWMode==NF_READ)
	{
		gp_Desc[DESC_CMD] = (unsigned long)(AUTOCMD_READ) | ((1<<chip_number)<<24) | (CMD_TYPE_READ<<28);	
	}
	else
	{
		gp_Desc[DESC_CMD] = (unsigned long)(AUTOCMD_WRITE) | ((1<<chip_number)<<24) | (CMD_TYPE_WRITE<<28);		
	}
	gp_Desc[DESC_LENGTH] = (unsigned long)((pstSysInfo->u16PyldLen<<16) | pstSysInfo->u16ReduntLen);	

	gp_Desc[DESC_INTERRUPT] = 0 ;

	//gp_Desc[DESC_PAYLOAD] = (unsigned long)PyldBuffer ;
	//gp_Desc[DESC_REDUNT] = (unsigned long)DataBuffer ;
	gp_Desc[DESC_PAYLOAD] = (unsigned long)PhysAddr1;
	gp_Desc[DESC_REDUNT] = (unsigned long)PhysAddr2;
	gp_Desc[DESC_REDUNT_INFO] = (pstSysInfo->u16Redunt_Sector_Addr_Offset<<16) |pstSysInfo->u16Redunt_Sector_Len ;

	if(Last)
	{
		gp_Desc[DESC_INTERRUPT] = (unsigned long)(pstSysInfo->u16InterruptMask<<16) ;
		gp_Desc[DESC_ADDR1] = (unsigned long)((0xF4<<24) | u32PhyAddr) ;
		gp_Desc[DESC_ADDR2] = 0 ;

		//NF_TRACE(debug_flag, "u16PyldLen=%d u16ReduntLen=%d u16Redunt_Sector_Addr_Offset=%d, u16Redunt_Sector_Len=%d\n", 
		//	pstSysInfo->u16PyldLen , pstSysInfo->u16ReduntLen,
		//	pstSysInfo->u16Redunt_Sector_Addr_Offset, pstSysInfo->u16Redunt_Sector_Len);
		//print_Desc();

		trigger(ch);
		//print_Desc(u8RWMode);
		//trigger_ex(ch,u8RWMode);		
	}
	else
	{
		gp_Desc[DESC_ADDR1] = (unsigned long)((0xB4<<24) |u32PhyAddr) ;
		gp_Desc[DESC_ADDR2] = (unsigned long)(gNextDesBP+=sizeof(DescInfo_t)) ;	
		gp_Desc+=DESC_OFFSET;
	}
	
	return SUCCESS;
}
////////////////////////////
int autoWritePage2Plan(unsigned char ch, unsigned char chip_number, unsigned long u32PhyAddr, unsigned long* PyldBuffer, unsigned long* DataBuffer, unsigned char Mode)
{
	unsigned long *PhysAddr1 = 0 ;
	unsigned long *PhysAddr2 = 0 ;

	PhysAddr1 = (unsigned long *)virt_to_phys((void*)PyldBuffer);
	PhysAddr2 = (unsigned long *)virt_to_phys((void*)DataBuffer);
	//printk("autoWritePage2Plan(%d ,%d, %d)\n", u32PhyAddr, Mode, pstSysInfo->vendor_no);

	if(Mode==NF_BEGIN)
	{
		if(pstSysInfo->vendor_no!=NAND_SAMSUNG)
		{
			gp_Desc[DESC_CMD] = (unsigned long)(AUTOCMD_WRITE) | ((1<<chip_number)<<24) | (CMD_TYPE_WRITE<<28); 	
		}
		else
		{
			gp_Desc[DESC_CMD] = (unsigned long)(AUTOCMD_WRITE_TWOPLAN_BEGIN) | ((1<<chip_number)<<24) | (CMD_TYPE_WRITE<<28);	
		}
	}
	else
	{
//printk("autoWritePage2Plan(%d ,%d, %d)\n",u32PhyAddr, u8RWMode, pstSysInfo->vendor_no);
		if(pstSysInfo->vendor_no!=NAND_SAMSUNG)
		{
			gp_Desc[DESC_CMD] = (unsigned long)(AUTOCMD_WRITE) | ((1<<chip_number)<<24) | (CMD_TYPE_WRITE<<28);		
		}
		else 
		{
			gp_Desc[DESC_CMD] = (unsigned long)(AUTOCMD_WRITE_TWOPLAN_END) | ((1<<chip_number)<<24) | (CMD_TYPE_WRITE<<28); 	

		}
	}

	gp_Desc[DESC_LENGTH] = (unsigned long)((pstSysInfo->u16PyldLen<<16) | pstSysInfo->u16ReduntLen);
	gp_Desc[DESC_INTERRUPT] = 0 ;

	//gp_Desc[DESC_PAYLOAD] = (unsigned long)PyldBuffer ;
	//gp_Desc[DESC_REDUNT] = (unsigned long)DataBuffer ;
	gp_Desc[DESC_PAYLOAD] = (unsigned long)PhysAddr1;
	gp_Desc[DESC_REDUNT]  = (unsigned long)PhysAddr2;
	gp_Desc[DESC_REDUNT_INFO] = (pstSysInfo->u16Redunt_Sector_Addr_Offset<<16) |pstSysInfo->u16Redunt_Sector_Len ;

	if(Mode==NF_END)
	{
		gp_Desc[DESC_INTERRUPT] = (unsigned long)(pstSysInfo->u16InterruptMask<<16) ;	
		gp_Desc[DESC_ADDR1] = (unsigned long)((0xF4<<24) | u32PhyAddr) ;
		gp_Desc[DESC_ADDR2] = 0 ;	
		trigger(ch);		
	}
	else
	{
		//gp_Desc[DESC_ADDR1] = (unsigned long)((0xB4<<24) |(unsigned char)u32PhyAddr) ;
		//gp_Desc[DESC_ADDR2] = (unsigned long)((((u32PhyAddr>>8)&0xff)<<24) |(u32PhyAddr&0xff0000) |(gNextDesBP+=sizeof(DescInfo_t))) ;	
		gp_Desc[DESC_ADDR1] = (unsigned long)((0xB4<<24) | u32PhyAddr) ;
		gp_Desc[DESC_ADDR2] = (unsigned long)(gNextDesBP+=sizeof(DescInfo_t)) ;
		gp_Desc+=DESC_OFFSET;
	}
	
	return SUCCESS;
}

///////////////////////////
//int ReadWritePagech(unsigned char ch, unsigned long u32PhyAddr,unsigned long* PyldBuffer,unsigned long* DataBuffer,unsigned char u8RWMode)
int ReadWritePagech(unsigned char ch, unsigned long u32PhyAddr,unsigned long* PyldBuffer,unsigned long* DataBuffer,unsigned char u8RWMode)
{
	//int count,i;
	
	unsigned int PyldBuffer_offset,DataBuffer_offset; 

//printk("ReadWritePagech(%d ,%d)\n",u32PhyAddr, u8RWMode);
	if(u8RWMode==NF_WRITE)
		nf_wp_switch(0);
#if WRITE_AND_GO	
	PollingAndsetStatus(ch, NFD_READWRITE);	
#endif
	
	InitDesc(ch);
	NF_TRACE(debug_flag, "u8RWMode=%u, gIsSingle_CS=%u, u8Support_TwoPlan=%u\n",	
		u8RWMode, gIsSingle_CS, pstSysInfo->u8Support_TwoPlan);
	if(gIsSingle_CS)
	{
		//count = 1;
		PyldBuffer_offset = pstSysInfo->u16PyldLen;
		DataBuffer_offset = pstSysInfo->u16ReduntLenLog;	
	}
	else
	{	
		//count = 1 << pstSysInfo->u8Internal_Chip_Number;
		PyldBuffer_offset = pstSysInfo->u16PyldLen<<(pstSysInfo->u8Support_TwoPlan+pstSysInfo->u8Support_Internal_Interleave);
		DataBuffer_offset = pstSysInfo->u16ReduntLen<<(pstSysInfo->u8Support_TwoPlan+pstSysInfo->u8Support_Internal_Interleave);
	}
	//i=count;

	if(pstSysInfo->u8Support_TwoPlan && !gIsSingle_CS)//two plan support
	{
		unsigned char chip_number;
		unsigned long blkaddr, pageoffset, addr1, addr2;

		blkaddr = u32PhyAddr >>pstSysInfo->u8PagePerBlkShift; 
		
		chip_number = (blkaddr >> pstSysInfo->u8TotalBlkNoShift);		
		chip_number = g_ChipMap[chip_number];
		
		blkaddr = blkaddr & (pstSysInfo->u16TotalBlkNo-1);
		
		pageoffset =  u32PhyAddr &(pstSysInfo->u16PageNoPerBlk-1);	
		addr1 = ((blkaddr<<pstSysInfo->u8Support_TwoPlan) << pstSysInfo->u8PagePerBlkShift)+pageoffset; 
		addr2 = (((blkaddr<<pstSysInfo->u8Support_TwoPlan)+1) << pstSysInfo->u8PagePerBlkShift)+pageoffset; 
		
		ChipEnable(chip_number);
		if(chip_number>=3) chip_number = 3;


#if 1							
		if(u8RWMode==NF_WRITE)
		{
			PyldBuffer_offset -= pstSysInfo->u16PyldLen;
			DataBuffer_offset -= pstSysInfo->u16ReduntLen;

			autoWritePage2Plan(ch, chip_number, addr1, (unsigned long*)(((unsigned long) PyldBuffer)+PyldBuffer_offset), (unsigned long*)(((unsigned long) DataBuffer)+(DataBuffer_offset)), NF_BEGIN);			
			
			PyldBuffer_offset -= pstSysInfo->u16PyldLen;
			DataBuffer_offset -= pstSysInfo->u16ReduntLen;			
			autoWritePage2Plan(ch, chip_number, addr2, (unsigned long*)(((unsigned long) PyldBuffer)+PyldBuffer_offset), (unsigned long*)(((unsigned long) DataBuffer)+(DataBuffer_offset)), NF_END);					
		}
		else	
#endif		
		{
			PyldBuffer_offset -= pstSysInfo->u16PyldLen;
			DataBuffer_offset -= pstSysInfo->u16ReduntLen;
			
			autoReadWritePage(ch, chip_number, addr1, (unsigned long*)(((unsigned long) PyldBuffer)+PyldBuffer_offset), (unsigned long*)(((unsigned long) DataBuffer)+(DataBuffer_offset)), u8RWMode,0);
	

			PyldBuffer_offset -= pstSysInfo->u16PyldLen;
			DataBuffer_offset -= pstSysInfo->u16ReduntLen;			
			
			autoReadWritePage(ch, chip_number, addr2,(unsigned long*)(((unsigned long) PyldBuffer)+PyldBuffer_offset), (unsigned long*)(((unsigned long) DataBuffer)+(DataBuffer_offset)), u8RWMode,1);
		}

	}
#if 0	
	else if(pstSysInfo->u8Support_External_Interleave && u8RWMode==NF_WRITE && !gIsSingle_CS)
	{

	}
#endif	
	else
	{
		unsigned char chip_number;
		unsigned long blkaddr, pageoffset, addr1;

		blkaddr = u32PhyAddr >>pstSysInfo->u8PagePerBlkShift; 
		
		pageoffset =  u32PhyAddr &(pstSysInfo->u16PageNoPerBlk-1);

		
		chip_number = (blkaddr >> (pstSysInfo->u8TotalBlkNoShift+pstSysInfo->u8Support_TwoPlan));
		blkaddr = blkaddr & ((pstSysInfo->u16TotalBlkNo<<pstSysInfo->u8Support_TwoPlan)-1);	
	

		
		chip_number = g_ChipMap[chip_number];			
		addr1 = ((blkaddr) << pstSysInfo->u8PagePerBlkShift)+pageoffset; 

		ChipEnable(chip_number);
		autoReadWritePage(ch, chip_number, addr1, (unsigned long*)(((unsigned long) PyldBuffer)), (unsigned long*)(((unsigned long) DataBuffer)), u8RWMode,1);

	}
	

#if WRITE_AND_GO
		if(u8RWMode==NF_READ)
#endif		
		{
	//printk(">>");

			enterPollingStatech(ch);
	//printk("<<");			
		}

	if(u8RWMode==NF_WRITE)
		nf_wp_switch(1);
	return SUCCESS;
}
int ReadWriteSmallPage(unsigned ch, unsigned long u32PhyAddr,unsigned long* PyldBuffer,unsigned long* DataBuffer,unsigned char u8RWMode)
{
	unsigned long *PhysAddr1 = 0 ;
	unsigned long *PhysAddr2 = 0 ;

	PhysAddr1 = (unsigned long *)virt_to_phys((void*)PyldBuffer);
	PhysAddr2 = (unsigned long *)virt_to_phys((void*)DataBuffer);
	InitDesc(ch);

	if(u8RWMode==NF_READ)
	{
		//DBGPRINT("ReadSmallPage(%d)",u32PhyAddr);

		gp_Desc[DESC_CMD] = (unsigned long)(AUTOCMD_READ) | (1<<24) | (CMD_TYPE_READ<<28);	
	}
	else
	{
		//DBGPRINT("WriteSmallPage(%d)",u32PhyAddr);

		gp_Desc[DESC_CMD] = (unsigned long)(AUTOCMD_WRITE) | (1<<24) | (CMD_TYPE_WRITE<<28);		
	}
	gp_Desc[DESC_LENGTH] = (unsigned long)((g_sbi.PhyPageLen<<16) | g_sbi.PhyReduntLenLog);	

	gp_Desc[DESC_INTERRUPT] = (unsigned long)(pstSysInfo->u16InterruptMask<<16) ;

	gp_Desc[DESC_ADDR1] = (unsigned long)((unsigned long)(0xF4<<24) | (u32PhyAddr>>8)) ;
	gp_Desc[DESC_ADDR2] = ((u32PhyAddr&0xff)<<24) ;

	//gp_Desc[DESC_PAYLOAD] = (unsigned long)PyldBuffer ;
	//gp_Desc[DESC_REDUNT] = (unsigned long)DataBuffer ;
	gp_Desc[DESC_PAYLOAD] = (unsigned long)PhysAddr1;
	gp_Desc[DESC_REDUNT] = (unsigned long)PhysAddr1 ;
	gp_Desc[DESC_REDUNT_INFO] = (g_sbi.PhyReduntLenLog<<16) |g_sbi.PhyReduntLenLog ;

	trigger(ch);	
	
	enterPollingStatech(ch);
		
	//return SUCCESS;
 	return 0;
}

int ReadWritePage_SB(unsigned long u32PhyAddr,unsigned long* PyldBuffer,unsigned long* DataBuffer,unsigned char u8RWMode)
{

	int i;
	unsigned long phypage = u32PhyAddr*g_sbi.PhyPagePerLogPage;

	unsigned char* pPyldBuffer;
	unsigned char* pbchBuffer;
		
	for(i=0; i<g_sbi.PhyPagePerLogPage; i++)
	{
		pPyldBuffer = (unsigned char*)(((unsigned long)PyldBuffer) + i*g_sbi.PhyPageLen);
		pbchBuffer = (unsigned char*)(((unsigned long)DataBuffer) + i*16);

		//printk("pPyldBuffer:0x%x",pPyldBuffer);
		//printk("pbchBuffer:0x%x",pbchBuffer);


		if(u8RWMode==NF_READ)
		{

			ReadWriteSmallPage( DEV_CH0, phypage+i, (unsigned long*)pPyldBuffer, (unsigned long*)gp_bchtmpbuf, u8RWMode);

			//print_buf(gp_bchtmpbuf, 16);
			memcpy(Cache2NonCacheAddr(pbchBuffer), Cache2NonCacheAddr(gp_bchtmpbuf), 16);
			//memcpy(pbchBuffer, gp_bchtmpbuf, 16);
			//print_buf(pbchBuffer, 16);
		
		}
		else
		{
			memcpy(Cache2NonCacheAddr(gp_bchtmpbuf), Cache2NonCacheAddr(pbchBuffer), 16);
			//memcpy(pbchBuffer, gp_bchtmpbuf, 16);
			//print_buf(pbchBuffer, 16);
			ReadWriteSmallPage( DEV_CH0, phypage+i, (unsigned long*)pPyldBuffer, (unsigned long*)gp_bchtmpbuf, u8RWMode);
			//print_buf(gp_bchtmpbuf, 16);

		}

	}
	return 0;
}



int ReadWritePage(unsigned long u32PhyAddr,unsigned long* PyldBuffer,unsigned long* DataBuffer,unsigned char u8RWMode)
{
	nf_drv_mutex_lock(&nf_lock);
	
	enterPollingState_ex();
	
	gIsSingle_CS = 0;
	ReadWritePagech(DEV_CH0, u32PhyAddr, PyldBuffer, DataBuffer, u8RWMode);


#if SUPPORT_MUTI_CHANNEL	
	if(pstSysInfo->u8MultiChannel)
		ReadWritePagech(DEV_CH1, u32PhyAddr, (unsigned long*)(((unsigned long) PyldBuffer)+ (pstSysInfo->u16PyldLen<<pstSysInfo->u8Internal_Chip_Number) ), (unsigned long*)(((unsigned long) DataBuffer)+(pstSysInfo->u16ReduntLenLog<<pstSysInfo->u8Internal_Chip_Number)), u8RWMode);	
#endif	

	nf_drv_mutex_unlock(&nf_lock);
	
	return SUCCESS;
}

//only support single CS
#define SUPPORT_SINGLE_CS 1
int ReadWritePage_ex(unsigned long u32PhyAddr,unsigned long* PyldBuffer,unsigned long* DataBuffer,unsigned char u8RWMode)
{
	int rc;
	nf_drv_mutex_lock(&nf_lock);
	
	enterPollingState_ex();

	//SPMP_DEBUG_PRINT("Begin\n");
	gIsSingle_CS = 1;
	rc = ReadWritePagech(DEV_CH0, u32PhyAddr, PyldBuffer, DataBuffer, u8RWMode);
	//SPMP_DEBUG_PRINT("End\n");

	nf_drv_mutex_unlock(&nf_lock);
	nand_cache_invalidate();
	return rc;
}

#define MENU_ERASEDESC_SIZE (3*DESC_OFFSET)
//chip_number 0, 1, 2, 3, ........
void manuErase(unsigned ch, unsigned char chip_number, unsigned long u32BlkNo, unsigned char Last)
{
	unsigned long vAddress;

	memset(gp_Desc, 0, MENU_ERASEDESC_SIZE);

	//CMD phase
	gp_Desc[DESC_CMD] = (unsigned long)((MAUNCMD_BLOCK_ERASE) | ((1<<chip_number)<<24) | (CMD_TYPE_MANUAL_MODE_CMD<<28));
	gp_Desc[DESC_INTERRUPT] = (pstSysInfo->u16InterruptMask<<16);	
	gp_Desc[DESC_ADDR1] = ((MANUAL_MODE_OWNER | MANUAL_MODE_LAST_SECTOR | MANUAL_MODE_REDUNT_ENABLE)<<24);
	gp_Desc[DESC_ADDR2] = 0x00|(gNextDesBP+=sizeof(DescInfo_t));

	gp_Desc += DESC_OFFSET;	

	//ADDR phase
	gp_Desc[DESC_CMD] = (unsigned long)(((1<<chip_number)<<24) | (CMD_TYPE_MANUAL_MODE_ADDR<<28));
	gp_Desc[DESC_INTERRUPT] = (pstSysInfo->u16InterruptMask<<16);	
	#define ERASE_ADDRNUMBER 2
	
	vAddress = (u32BlkNo << pstSysInfo->u8PagePerBlkShift);
	
	gp_Desc[DESC_ADDR1] = (((MANUAL_MODE_OWNER | MANUAL_MODE_LAST_SECTOR | MANUAL_MODE_REDUNT_ENABLE | ERASE_ADDRNUMBER)<<24)| vAddress);
	gp_Desc[DESC_ADDR2] = 0x00|(gNextDesBP+=sizeof(DescInfo_t));

	gp_Desc += DESC_OFFSET;	

	
	gp_Desc[DESC_CMD] = (unsigned long)(MAUNCMD_CONF_BLOCK_ERASE | ((1<<chip_number)<<24) | (CMD_TYPE_MANUAL_MODE_CMD<<28));
	gp_Desc[DESC_INTERRUPT] = (pstSysInfo->u16InterruptMask<<16);
	if(Last)
	{
		gp_Desc[DESC_ADDR1] = ((MANUAL_MODE_OWNER | MANUAL_MODE_END_DESC | MANUAL_MODE_LAST_SECTOR | MANUAL_MODE_REDUNT_ENABLE)<<24);
		gp_Desc[DESC_ADDR2] = 0;
		trigger(ch);	
	}
	else
	{
		gp_Desc[DESC_ADDR1] = ((MANUAL_MODE_OWNER | MANUAL_MODE_LAST_SECTOR | MANUAL_MODE_REDUNT_ENABLE)<<24);
		gp_Desc[DESC_ADDR2] = 0x00|(gNextDesBP+=sizeof(DescInfo_t));
		gp_Desc += DESC_OFFSET;			
	}

}

//chip_number 0, 1, 2, 3, ........
void HalfAutoErase(unsigned ch, unsigned char chip_number, unsigned long u32BlkNo, unsigned char Last)
{
	unsigned long vAddress;

	memset(gp_Desc, 0, sizeof(DescInfo_t));

	vAddress = (u32BlkNo << pstSysInfo->u8PagePerBlkShift);
	
	gp_Desc[DESC_CMD] = (unsigned long)((AUTOCMD_BLOCK_ERASE) | ((1<<chip_number)<<24) | (CMD_TYPE_HALFAUTO_ERASE<<28));		

	//gp_Desc[DESC_INTERRUPT] = (unsigned long)(pstSysInfo->u16InterruptMask<<16);		

	if(Last)
	{
		gp_Desc[DESC_INTERRUPT] = (unsigned long)(0x5d<<16);
		gp_Desc[DESC_ADDR2] = 0;
		gp_Desc[DESC_ADDR1] = (unsigned long)((0xF2<<24) | vAddress);
		trigger(ch);
	
	}
	else
	{
		gp_Desc[DESC_INTERRUPT] = 0;
		gp_Desc[DESC_ADDR1] = (unsigned long)((0xB2<<24) | vAddress);			
		gp_Desc[DESC_ADDR2] = (gNextDesBP+=sizeof(DescInfo_t));	
		gp_Desc+=DESC_OFFSET;
	}	
}


void autoErase(unsigned ch, unsigned char chip_number, unsigned long u32BlkNo, unsigned char Last)
{
	unsigned long vAddress;

	memset(gp_Desc, 0, sizeof(DescInfo_t));

	vAddress = (u32BlkNo << pstSysInfo->u8PagePerBlkShift);
	
	gp_Desc[DESC_CMD] = (unsigned long)((AUTOCMD_BLOCK_ERASE) | ((1<<chip_number)<<24) | (CMD_TYPE_ERASE<<28));		

	gp_Desc[DESC_INTERRUPT] = (unsigned long)(pstSysInfo->u16InterruptMask<<16);		

	if(Last)
	{
		gp_Desc[DESC_ADDR2] = 0;
		gp_Desc[DESC_ADDR1] = (unsigned long)((0xF2<<24) | vAddress);
		trigger(ch);
	
	}
	else
	{
		gp_Desc[DESC_ADDR1] = (unsigned long)((0xB2<<24) | vAddress);			
		gp_Desc[DESC_ADDR2] = (gNextDesBP+=sizeof(DescInfo_t));	
		gp_Desc+=DESC_OFFSET;
	}		
}
#define ERASE_ADDRNUMBER 2
void manuErase2Plan(unsigned ch, unsigned char chip_number, unsigned long u32BlkNo)
{
	unsigned long vAddress;

//printk("manuErase2Plan(%d, %d, %d)\n",ch, chip_number, u32BlkNo);

	InitDesc(ch);

	//CMD phase
	gp_Desc[DESC_CMD] = (unsigned long)((MAUNCMD_BLOCK_ERASE) | ((1<<chip_number)<<24) | (CMD_TYPE_MANUAL_MODE_CMD<<28));
	gp_Desc[DESC_INTERRUPT] = (pstSysInfo->u16InterruptMask<<16);	
	gp_Desc[DESC_ADDR1] = ((MANUAL_MODE_OWNER | MANUAL_MODE_LAST_SECTOR | MANUAL_MODE_REDUNT_ENABLE)<<24);
	gp_Desc[DESC_ADDR2] = 0x00|(gNextDesBP+=sizeof(DescInfo_t));

	gp_Desc += DESC_OFFSET;	

	//ADDR phase
	gp_Desc[DESC_CMD] = (unsigned long)(((1<<chip_number)<<24) | (CMD_TYPE_MANUAL_MODE_ADDR<<28));
	gp_Desc[DESC_INTERRUPT] = (pstSysInfo->u16InterruptMask<<16);	
	vAddress = (((u32BlkNo<<pstSysInfo->u8Support_TwoPlan)) << (pstSysInfo->u8PagePerBlkShift));
//printk("manuErase(0x%x)\n",vAddress);

	gp_Desc[DESC_ADDR1] = (((MANUAL_MODE_OWNER | MANUAL_MODE_LAST_SECTOR | MANUAL_MODE_REDUNT_ENABLE | ERASE_ADDRNUMBER)<<24)| vAddress);
	gp_Desc[DESC_ADDR2] = 0x00|(gNextDesBP+=sizeof(DescInfo_t));


	gp_Desc += DESC_OFFSET;	

	//CMD phase
	gp_Desc[DESC_CMD] = (unsigned long)((MAUNCMD_BLOCK_ERASE) | ((1<<chip_number)<<24) | (CMD_TYPE_MANUAL_MODE_CMD<<28));
	gp_Desc[DESC_INTERRUPT] = (pstSysInfo->u16InterruptMask<<16);	
	gp_Desc[DESC_ADDR1] = ((MANUAL_MODE_OWNER | MANUAL_MODE_LAST_SECTOR | MANUAL_MODE_REDUNT_ENABLE)<<24);
	gp_Desc[DESC_ADDR2] = 0x00|(gNextDesBP+=sizeof(DescInfo_t));

	gp_Desc += DESC_OFFSET;	

	//ADDR phase
	gp_Desc[DESC_CMD] = (unsigned long)(((1<<chip_number)<<24) | (CMD_TYPE_MANUAL_MODE_ADDR<<28));
	gp_Desc[DESC_INTERRUPT] = (pstSysInfo->u16InterruptMask<<16);	
	vAddress = (((u32BlkNo<<pstSysInfo->u8Support_TwoPlan)+1) << (pstSysInfo->u8PagePerBlkShift));
//printk("manuErase(0x%x)\n",vAddress);

	gp_Desc[DESC_ADDR1] = (((MANUAL_MODE_OWNER | MANUAL_MODE_LAST_SECTOR | MANUAL_MODE_REDUNT_ENABLE | ERASE_ADDRNUMBER)<<24)| vAddress);
	gp_Desc[DESC_ADDR2] = 0x00|(gNextDesBP+=sizeof(DescInfo_t));

	gp_Desc += DESC_OFFSET;	
	
	gp_Desc[DESC_CMD] = (unsigned long)(MAUNCMD_CONF_BLOCK_ERASE | ((1<<chip_number)<<24) | (CMD_TYPE_MANUAL_MODE_CMD<<28));
	gp_Desc[DESC_INTERRUPT] = (pstSysInfo->u16InterruptMask<<16);

	gp_Desc[DESC_ADDR1] = ((MANUAL_MODE_OWNER | MANUAL_MODE_END_DESC | MANUAL_MODE_LAST_SECTOR | MANUAL_MODE_REDUNT_ENABLE)<<24);
	gp_Desc[DESC_ADDR2] = 0;
	
	trigger(ch);	
	
	
}


void EraseBlockch(unsigned ch, unsigned long u32BlkNo)
{
	int count,i;

//printk("EraseBlockch(%d ,%d)\n",ch, u32BlkNo);

	nf_wp_switch(0);
#if WRITE_AND_GO
	PollingAndsetStatus(ch, NFD_READWRITE);	
#endif
	if(gIsSingle_CS)
	{
		count=1;
	}
	else
	{	
		count = 1 << pstSysInfo->u8Internal_Chip_Number;
	}
	InitDesc(ch);
	
	i=count;


	if(pstSysInfo->u8Support_TwoPlan && !gIsSingle_CS)//two plan support
	{
		unsigned char chip_number;	
		chip_number = u32BlkNo >> pstSysInfo->u8TotalBlkNoShift;
		chip_number = g_ChipMap[chip_number];	
		u32BlkNo = u32BlkNo & (pstSysInfo->u16TotalBlkNo-1);	

		ChipEnable(chip_number);
		if(chip_number>=3) chip_number = 3;		
#if 0		
		manuErase2Plan(ch, chip_number, u32BlkNo);
		//g_IsWaitingRB = 1;
#else
		autoErase(ch, chip_number, u32BlkNo<<1,0);
		autoErase(ch, chip_number, (u32BlkNo<<1) +1,1);
#endif

	}
#if 0	
	else if(pstSysInfo->u8Support_External_Interleave && !gIsSingle_CS)
	{	
		//TODO.....
	
	}
	else if(pstSysInfo->u8Support_Internal_Interleave && !gIsSingle_CS)//Internal Interleave support
	{
		//TODO.....	
	}		
#endif
	else
	{
		unsigned char chip_number;	
		//unsigned int old_blk = u32BlkNo;
			
		chip_number = u32BlkNo >> (pstSysInfo->u8TotalBlkNoShift+pstSysInfo->u8Support_TwoPlan);	
		u32BlkNo = u32BlkNo & ((pstSysInfo->u16TotalBlkNo<<pstSysInfo->u8Support_TwoPlan)-1);		

		chip_number = g_ChipMap[chip_number];

		ChipEnable(chip_number);
		if(chip_number>=3) chip_number = 3;		


		autoErase(ch, chip_number, u32BlkNo,1);

	
	}
#if WRITE_AND_GO
#else	
	enterPollingStatech(ch);
#endif	
	nf_wp_switch(1);
}

void EraseSmallBlock(unsigned ch, unsigned long u32BlkNo)
{
	unsigned long vAddress;

	//DBGPRINT("EraseSmallBlock(%d)",u32BlkNo);
	nf_wp_switch(0);
	InitDesc(ch);

	vAddress = (u32BlkNo << g_sbi.PhyPageNoPerBlkShift); //blk addr to pagr addr

	//DBGPRINT("EraseSmallBlock(%d, %d)",u32BlkNo, vAddress);
	
	gp_Desc[DESC_CMD] = (unsigned long)((AUTOCMD_BLOCK_ERASE) | (1<<24) | (CMD_TYPE_ERASE<<28));		
	gp_Desc[DESC_LENGTH] = 0;
	gp_Desc[DESC_INTERRUPT] = (unsigned long)(pstSysInfo->u16InterruptMask<<16);		

	gp_Desc[DESC_ADDR2] = 0;
	gp_Desc[DESC_ADDR1] = (unsigned long)((unsigned long)(0xF2<<24) | vAddress);
	gp_Desc[DESC_PAYLOAD] = 0;
	gp_Desc[DESC_REDUNT] = 0;
	gp_Desc[DESC_REDUNT_INFO] = 0;
	trigger(ch);

	enterPollingStatech(ch);
	nf_wp_switch(1);
}

void EraseBlock_SB(unsigned long u32BlkNo)
{
	unsigned long blk = (u32BlkNo * g_sbi.PhyBlkPerLogBlk);
	int i;

	for(i=0; i<g_sbi.PhyBlkPerLogBlk; i++)
	{
		EraseSmallBlock(DEV_CH0, blk+i);		
	}
}


void EraseBlock(unsigned long u32BlkNo)
{
	NF_TRACE(debug_flag, "u32BlkNo=%lu\n", u32BlkNo);
	
	nf_drv_mutex_lock(&nf_lock);
	
	enterPollingState_ex();
	
	gIsSingle_CS = 0;
	EraseBlockch(DEV_CH0, u32BlkNo);

#if SUPPORT_MUTI_CHANNEL	
	if(pstSysInfo->u8MultiChannel)
		EraseBlockch(DEV_CH1, u32BlkNo);
#endif
	
	nf_drv_mutex_unlock(&nf_lock);
}
void EraseBlock_ex(unsigned long u32BlkNo)
{
	nf_drv_mutex_lock(&nf_lock);

	enterPollingState_ex();

	gIsSingle_CS = 1;
	
	EraseBlockch(DEV_CH0, u32BlkNo);

	nf_drv_mutex_unlock(&nf_lock);	
}

//int BCHProcess_ex(unsigned char* PyldBuffer, unsigned char* ReduntBuffer, unsigned int len, int op)
int BCHProcess_ex(unsigned long* PyldBuffer, unsigned long* ReduntBuffer, unsigned int len, int op)
{
	int rc;
	unsigned char *PhysAddr1 = 0 ;
	unsigned char *PhysAddr2 = 0 ;

	PhysAddr1 = (unsigned char *)virt_to_phys((void*)PyldBuffer);
	PhysAddr2 = (unsigned char *)virt_to_phys((void*)ReduntBuffer);
	nf_drv_mutex_lock(&bch_lock);
	nand_cache_sync();
	//rc = bch_s336_process((unsigned char*)PyldBuffer, (unsigned char*)ReduntBuffer, len, op, pstSysInfo->ecc_mode);
	rc = bch_s336_process(PhysAddr1, PhysAddr2, len, op, pstSysInfo->ecc_mode);
	nf_drv_mutex_unlock(&bch_lock);	
	return rc;

}

int BCHProcess(unsigned char* PyldBuffer, unsigned char* ReduntBuffer, unsigned int len, int op, int eccmode)
{
	int rc;
	unsigned char *PhysAddr1 = 0 ;
	unsigned char *PhysAddr2 = 0 ;

	PhysAddr1 = (unsigned char *)virt_to_phys((void*)PyldBuffer);
	PhysAddr2 = (unsigned char *)virt_to_phys((void*)ReduntBuffer);
	nf_drv_mutex_lock(&bch_lock);
	//nf_drv_mutex_lock(&nf_lock);
	//rc = bch_s336_process(PyldBuffer, ReduntBuffer, len, op, eccmode);
	nand_cache_sync();
	rc = bch_s336_process(PhysAddr1, PhysAddr2, len, op, eccmode);
	nf_drv_mutex_unlock(&bch_lock);	
	//nf_drv_mutex_unlock(&nf_lock);	
	return rc;

}

int Remove_NFDriver(void)
{
	if(rPayload !=NULL)
		free_page((unsigned long)rPayload);
	
	if(g_DescInfo !=NULL)
		free_page((unsigned long)g_DescInfo);
#if SUPPORT_INTERRUPT
	free_irq(IRQ_NAND0, NULL);
#if SUPPORT_MUTI_CHANNEL
	free_irq(IRQ_NAND1, NULL);
#endif

#endif

	return 0;
}
//######################################################################################
//


#ifdef CYGPKG_POWER 	//NOW redboot not use power manager package.

#define DBGFLG_DYNAMIC g_dbgNF
#define DBGFLG_NF_LVL0 0x1
#define DBGFLG_NF_LVL1 0x2
#define DBGFLG_NF_LVL2 0x4

//DYNDBGTAB_ENTRY(g_dbgNF,0/*DBGFLG_NF_LVL0| DBGFLG_NF_LVL1 | DBGFLG_NF_LVL2*/, "nf_s330.c debug var");

//power manager functions
static void
nf_power_mode_change(
    PowerController* controller,
    PowerMode        desired_mode,
    PowerModeChange  change)
{
	DBGLVL_PRINT(DBGFLG_NF_LVL0, "nf_power_mode_change: enter\n");
	
	PowerMode oldmode = controller->mode;
	cyg_bool allowChange = true;
	
	//NOTE: if device is not initialized yet, always allow change mode
	if(!g_IsInit_nf)
	{
		controller->mode = desired_mode;
		DBGLVL_PRINT(DBGFLG_NF_LVL0, "nf_power_mode_change: exit\n");
		return;
	}
	
	if(controller->mode==PowerMode_Suspend)	
		DBGLVL_PRINT(DBGFLG_NF_LVL1, "controller->mode==PowerMode_Suspend.\n");
		
	//NOTE: Any time power manager is changing MODE, it should change to PowerMode_ChgSpd first,
	//		then change to desired mode. Device programmer should keep in mind this rule on implementing
	//		device power controller.
	switch(desired_mode)
	{
	case PowerMode_ChgSpd:
		//NOTE: As system clock is going to be changed, device driver should avoid
		//		any access to DRAM from now on. If device is in a busy state and can't avoid accessing
		//		DRAM, set allowChange to FALSE to notify power manager not to change clock.
		DBGLVL_PRINT(DBGFLG_NF_LVL1, "PowerMode_ChgSpd\n");
		nf_drv_mutex_lock(&nf_lock);
		nf_drv_mutex_lock(&bch_lock);
		break;
		
	case PowerMode_VideoHSpd:
		//NOTE: System clock has been changed, please re-config device to use new system clock if
		//		device is system clock dependent.
		DBGLVL_PRINT(DBGFLG_NF_LVL1, "PowerMode_VideoHSpd\n");
		if(controller->mode==PowerMode_Suspend)	
		{
			turnOnClockPipe_NAND();
		}
		nf_drv_mutex_unlock(&bch_lock);
		nf_drv_mutex_unlock(&nf_lock);
		break;
		
	case PowerMode_Video:
		//NOTE: System clock has been changed, please re-config device to use new system clock if
		//		device is system clock dependent.
		DBGLVL_PRINT(DBGFLG_NF_LVL1, "PowerMode_Video\n");
		if(controller->mode==PowerMode_Suspend)	
		{
			turnOnClockPipe_NAND();
		}
		nf_drv_mutex_unlock(&bch_lock);
		nf_drv_mutex_unlock(&nf_lock);
		break;
		
	case PowerMode_MP3:
		//NOTE: System clock has been changed, please re-config device to use new system clock if
		//		device is system clock dependent.
		DBGLVL_PRINT(DBGFLG_NF_LVL1, "PowerMode_MP3\n");
		if(controller->mode==PowerMode_Suspend)	
		{
			turnOnClockPipe_NAND();
		}
		nf_drv_mutex_unlock(&bch_lock);
		nf_drv_mutex_unlock(&nf_lock);
		break;
		
	case PowerMode_Idle:
		//NOTE: As platform is going to enter IDLE mode, devices shuold close its clock, except ARM subsystem and DRAM module.	
		DBGLVL_PRINT(DBGFLG_NF_LVL1, "PowerMode_Suspend.\n");
		if(controller->mode==PowerMode_Suspend)	
		{
			turnOnClockPipe_NAND();
		}
		nf_drv_mutex_unlock(&bch_lock);
		nf_drv_mutex_unlock(&nf_lock);
		break;
	case PowerMode_LowSpd:
		DBGLVL_PRINT(DBGFLG_NF_LVL1, "PowerMode_LowSpd.\n");
		if(controller->mode==PowerMode_Suspend)	
		{
			turnOnClockPipe_NAND();
		}
		nf_drv_mutex_unlock(&bch_lock);
		nf_drv_mutex_unlock(&nf_lock);
		break;	
	case PowerMode_Suspend:
		//NOTE: As platform is going to enter SUSPEND mode, all modules shuold close its clock.
		DBGLVL_PRINT(DBGFLG_NF_LVL1, "PowerMode_Suspend.\n");
//		NFTL_CacheFlush();
		NFTL_CacheFlush_ex();
#if NPB_SUPPORT_TABLESAVE			
		NPB_table_save(); //add by king weng
#endif	

		turnOffClockPipe_NAND();
		nf_drv_mutex_lock(&nf_lock);
		nf_drv_mutex_lock(&bch_lock);
		break;
	
	case PowerMode_Off:
		//NOTE: As platform is going to enter SUSPEND mode, all modules shuold close its clock.	
		DBGLVL_PRINT(DBGFLG_NF_LVL1, "PowerMode_Off.\n");		
//		NFTL_CacheFlush();
		NFTL_CacheFlush_ex();
#if NPB_SUPPORT_TABLESAVE			
		NPB_table_save(); //add by king weng
#endif	

		turnOffClockPipe_NAND();
		nf_drv_mutex_lock(&nf_lock);
		nf_drv_mutex_lock(&bch_lock);
		break;

	default:
	//	type = 0;	//lowest speed - sys 27MHz
		DBGLVL_PRINT(DBGFLG_NF_LVL1, "UNKNOWN mode(%d).\n", desired_mode);
		if(controller->mode==PowerMode_Suspend)	
		{
			turnOnClockPipe_NAND();
		}
		nf_drv_mutex_unlock(&bch_lock);
		nf_drv_mutex_unlock(&nf_lock);
		break;		
	} 
			
//	DBGLVL_PRINT(DBGFLG_CX_LVL1, "ceva_power_mode_change: change to type %d\n", type);
	if(allowChange)
		controller->mode = desired_mode;
	//DBGLVL_PRINT(DBGFLG_CX_LVL1, "nf_power_mode_change: exit\n");
	DBGLVL_PRINT(DBGFLG_NF_LVL0, "nf_power_mode_change: exit\n");
}

//CPU power manager definitions
POWER_CONTROLLER(nf_power_controller, \
                        PowerPri_Storage,      \
                        "nand device",         \
                        &nf_power_mode_change);
#endif

