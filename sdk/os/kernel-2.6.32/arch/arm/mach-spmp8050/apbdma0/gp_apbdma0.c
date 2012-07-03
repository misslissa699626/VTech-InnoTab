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
 * @file    gp_apbdma0.c
 * @brief   Implement of apbdma0 module driver.
 * @author  Dunker Chen
 */
 
#include <mach/io.h>
#include <mach/module.h>
#include <mach/kernel.h>
#include <mach/irqs.h>
#include <mach/gp_apbdma0.h>

/**************************************************************************
*                           C O N S T A N T S                             *
**************************************************************************/

#define APBDMA0_MAX_CH		4

static const int MultiplyDeBruijnBitPosition[32] =
{
	0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
	31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
};

/**************************************************************************
*                              M A C R O S                                *
**************************************************************************/

#if 0
#define DEBUG	printk
#else
#define DEBUG(...)
#endif

#define DERROR printk 

#define GetLowestBitPos(x) (MultiplyDeBruijnBitPosition[((unsigned)((x & -x) * 0x077CB531)) >> 27])

/**************************************************************************
*                          D A T A    T Y P E S                           *
**************************************************************************/
typedef struct gpAPBCHInfo_s
{
	gpApbdma0Module_t 		module;			/*!< @brief Peripheral module */
	volatile int			irq;			/*!< @brief IRQ status */
	wait_queue_head_t 		finish_queue;	/*!< @brief Dma finish queue */
	gp_apbdma0_irq_handle_t func;			/*!< @brief IRQ function */
}gpAPBCHInfo_t;

typedef struct gpAPBInfo_s{
	unsigned int 		CH_STATUS;			/*!< @brief Channel status */
	unsigned int 		wait_usr;			/*!< @brief Wait user number */
	wait_queue_head_t	wait_queue;			/*!< @brief Wait queue */ 
	spinlock_t			lock;               /*!< @brief For mutual exclusion */
	gpAPBCHInfo_t		ch[APBDMA0_MAX_CH];	/*!< @brief Dma channel parametet */
}gpAPBInfo_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
**************************************************************************/

/**************************************************************************
*               F U N C T I O N    D E C L A R A T I O N S                *
**************************************************************************/
 
/**************************************************************************
*                         G L O B A L    D A T A                          *
**************************************************************************/

static gpAPBInfo_t *g_apb;

/**************************************************************************
*             F U N C T I O N    I M P L E M E N T A T I O N S            *
**************************************************************************/

/**
* @brief 	Get free channel.
* @return:	0 for no channel, others mean channel number + 1.
*/
static unsigned int gp_apbdma0_getfree(void)
{
	unsigned int free_ch;
	spin_lock(&g_apb->lock);
	free_ch = GetLowestBitPos(~g_apb->CH_STATUS);
	if((g_apb->CH_STATUS==0xFFFFFFFF)||(free_ch>=APBDMA0_MAX_CH))
		free_ch = 0;
	else
		free_ch++;
	if(free_ch)
		g_apb->CH_STATUS |= 1<<(free_ch-1);	
	spin_unlock(&g_apb->lock);	
	return free_ch;
}

/**
* @brief 	Check dma finish or not.
* @return:	0 for not finish, 1 means finish.
*/
static int gp_apbdma0_ckfinish(unsigned int ch_num)
{
	gpAPBCHInfo_t* ch = (gpAPBCHInfo_t*)&g_apb->ch[ch_num];
	int ret;
	disable_irq(IRQ_APBDMA_C_CH0+ch_num);
	ret = ch->irq;
	ch->irq = 0; 
	enable_irq(IRQ_APBDMA_C_CH0+ch_num);
	return ret;
}

/**
* @brief 	Acquire apbdma0 handle.
* @param 	timeout[in]: timeout (unit: 10ms).
* @return:	Apbdma handle(non-zero and non-negative)/ERROR_ID(-1).
*/
int gp_apbdma0_request(unsigned short timeout)
{
	unsigned free_ch;

	/* ----- Get free channel ----- */
	if((free_ch = gp_apbdma0_getfree())==0)
	{
		g_apb->wait_usr ++;
		/* ----- Wait for channel free ----- */
		if(timeout)
			wait_event_interruptible_timeout(g_apb->wait_queue, (free_ch = gp_apbdma0_getfree())!=0, timeout*HZ/100 );
		else
			wait_event_interruptible(g_apb->wait_queue, (free_ch = gp_apbdma0_getfree())!=0);
		g_apb->wait_usr --;
	}
	DEBUG("APBDMA: get channel %d\n",free_ch);
	return free_ch;	
}
EXPORT_SYMBOL(gp_apbdma0_request);

/**
* @brief 	Release apbdma0 handle.
* @param 	handle [in]: apbdma handle.
* @return:	None.
*/
void gp_apbdma0_release(int handle)
{
	handle --;
	DEBUG("APBDMA: release channel %d\n",handle);
	if(handle<APBDMA0_MAX_CH)
	{	
		/* ----- Reset channel ----- */
		disable_irq(IRQ_APBDMA_C_CH0+handle);
		gpHalApbdmaRst(handle);
		g_apb->ch[handle].irq = 0;
		g_apb->ch[handle].func = 0;
		enable_irq(IRQ_APBDMA_C_CH0+handle);
		/* ----- Clear Channel status ----- */
		spin_lock(&g_apb->lock);
		g_apb->CH_STATUS &= ~(1<<handle);
		spin_unlock(&g_apb->lock);
		if(g_apb->wait_usr)
		{
			wake_up_interruptible(&g_apb->wait_queue);
		}
	}
}
EXPORT_SYMBOL(gp_apbdma0_release);

/**
* @brief 	Enable apbdma0.
* @param 	handle [in]: Apbdma handle.
* @param	param [in]: Apbdma parameter. When buffer 1 address is null or length = 0, it will be set as single buffer mode.
* @return: 	None.
*/
void gp_apbdma0_en(unsigned int handle, gpApbdma0Param_t param)
{
	handle --;
	if(handle<APBDMA0_MAX_CH)
	{	
		disable_irq(IRQ_APBDMA_C_CH0+handle);
		g_apb->ch[handle].irq = 0;
		gpHalApbdmaEn(handle, &param);
		enable_irq(IRQ_APBDMA_C_CH0+handle);
		//DEBUG("channel %d enable, buf = 0x%x, ln = %d, dir = %d\n", handle, (unsigned int)param.buf0, param.ln0, param.dir);
	}	
}
EXPORT_SYMBOL(gp_apbdma0_en);

/**
* @brief 	Wait until apbdma0 finish.
* @param 	timeout [in]: timeout (unit: 10ms).
* @param 	handle [in]: apbdma handle.
* @return: 	SUCCESS(0)/ERROR_ID
*/
int gp_apbdma0_wait(unsigned int handle, unsigned short timeout)
{
	handle --;
	if(handle<APBDMA0_MAX_CH)
	{
		gpAPBCHInfo_t* ch = (gpAPBCHInfo_t*)&g_apb->ch[handle];
		
		if(timeout)
			wait_event_interruptible_timeout(ch->finish_queue, ch->irq, timeout*HZ/100 );
		else
			wait_event_interruptible(ch->finish_queue, ch->irq);
	}
	if(gp_apbdma0_ckfinish(handle))
	{
		gpHalApbdmaRst(handle);
		return 0;	
	}
	else
		return -ETIMEDOUT;
}
EXPORT_SYMBOL(gp_apbdma0_wait);

/**
* @brief 	Check for apbdma finish or not.
* @param 	handle [in]: apbdma handle.
* @return: 	SUCCESS(0)/ERROR_ID
*/
int gp_apbdma0_trywait(unsigned int handle)
{
	handle --;
	if(handle<APBDMA0_MAX_CH)
	{
		if(gp_apbdma0_ckfinish(handle))
		{
			gpHalApbdmaRst(handle);
			return 0;	
		}
		else
			return -EALREADY;
	}
	return -EIO;
}
EXPORT_SYMBOL(gp_apbdma0_trywait);

/**
* @brief 	Attach IRQ function to channel.
* @param 	handle [in]: apbdma handle.
* @param	func[in]: IRQ function with which buffer number is finished as input argument.
* @return: 	None.
*/
void gp_apbdma0_irq_attach(unsigned int handle, gp_apbdma0_irq_handle_t func)
{
	handle --;
	if(handle<APBDMA0_MAX_CH)
	{	
		g_apb->ch[handle].func = func;		
	}		
}
EXPORT_SYMBOL(gp_apbdma0_irq_attach);

/**
* @brief	Set apbdma0 buffer
* @param	handle [in]: Apbdma handle.
* @param	buf_num[in]: Buffer number 0 or 1.
* @param	addr[in]: Buffer start address.
* @param	ln[in]: Buffer size.
* @return: 	None.
*/
void gp_apbdma0_setbuf(unsigned int handle, unsigned int buf_num, unsigned char* addr, unsigned int ln)
{
	handle --;
	if(handle<APBDMA0_MAX_CH)
	{
		gpHalApbdmaSetBuf(handle, buf_num, addr, ln);
	}
}
EXPORT_SYMBOL(gp_apbdma0_setbuf);

/**
* @brief	Force apbdma0 stop
* @param	handle [in]: Apbdma handle.
* @return: 	None.
*/
void gp_apbdma0_stop(unsigned int handle)
{
	handle --;
	if(handle<APBDMA0_MAX_CH)
	{
		gpHalApbdmaRst(handle);
	}		
}
EXPORT_SYMBOL(gp_apbdma0_stop);

/**
* @brief 	Apbdma0 IRQ function.
* @param 	irq[in]: IRQ number.
* @param	dev_id[in]: Device ID.
* @return:	IRQ status.
*/
static irqreturn_t gp_apbdma0_irq(int irq, void *dev_id)
{
	int ch_num = irq-IRQ_APBDMA_C_CH0;
	gpAPBCHInfo_t* ch = (gpAPBCHInfo_t*)dev_id;
	
	gpHalApbdmaClearIRQFlag(ch_num);
	ch->irq = 1;
	if(ch->func)
		ch->func(gpHalApbdmaBufStatus(ch_num)?0:1);
	/* ----- Wake up queue ----- */
	wake_up_interruptible(&ch->finish_queue);
	return IRQ_HANDLED;		
}

/**
* @brief 	Apbdma0 initial function.
* @return	None.
*/
int __init gp_apbdma0_init(void)
{
	int i;
	int ret;
	char irq_name[32];
	
	/* ----- Initial apbdma0 module ----- */
	gpHalApbdmaInit();
	/* ----- Allocate the apbdma array, and initialize each one. ----- */
	g_apb = kmalloc(sizeof (gpAPBInfo_t), GFP_KERNEL);
	if (g_apb == NULL)
		return -ENOMEM;
	memset(g_apb, 0x00, sizeof (gpAPBInfo_t));
	/* ----- temp change from 1 to reserve one channel for old dma due to BCM4319 WiFi ----- */
	g_apb->CH_STATUS = 0x01;
	/* ----- Initial spinlock ----- */
	spin_lock_init(&g_apb->lock);
	/* ----- Initial queue ----- */
	init_waitqueue_head(&g_apb->wait_queue);
	/* ----- Initial channel parameter ----- */   
	for(i=1; i<APBDMA0_MAX_CH; i++)
	{
		init_waitqueue_head(&g_apb->ch[i].finish_queue);
		snprintf (irq_name, 32, "APBDMA-CH%d", i);
		/* ----- Set IRQ function  ----- */
		ret = request_irq(IRQ_APBDMA_C_CH0+i, gp_apbdma0_irq, IRQF_DISABLED, irq_name, &g_apb->ch[i]);
		if(ret)
		{
			DERROR("APBDMA CH%d request_irq error\n",i);
			goto out_irq;
		}		
	}
	return 0;
out_irq:
	if(i)
	{
		/* ----- free irq function ----- */
		for(;i>=0;i--)
			free_irq(IRQ_APBDMA_C_CH0+i, &g_apb->ch[i]);	
	}
	kfree(g_apb);
	return ret;
}

/**
* @brief 	Apbdma0 driver exit function.
* @return 	None.
*/
static void __exit gp_apbdma0_exit(void)
{
	int i;
	/* ----- free irq function ----- */
	for(i=0; i<APBDMA0_MAX_CH; i++)
	{
		free_irq(IRQ_APBDMA_C_CH0+i, &g_apb->ch[i]);	
	}
	if(g_apb)
		kfree(g_apb);
}

module_init(gp_apbdma0_init);
module_exit(gp_apbdma0_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("Generalplus APBDMA0 Driver");
MODULE_LICENSE_GP;
