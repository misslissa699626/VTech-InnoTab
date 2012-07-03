/*
 * linux/drivers/usb/gadget/spmp_udc.c
 *
 * GENERALPLUS SPMP8050 series on-chip high speed USB device controllers
 *
 * Copyright (C) 2010 GENERALPLUS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
//#include <linux/gpio.h>

#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include <linux/usb.h>
#include <linux/usb/gadget.h>

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <mach/irqs.h>

#include <mach/hardware.h>
#include <mach/clock_mgr/gp_clock.h>
#include <mach/hal/hal_clock.h>
#include <mach/common.h>
#include <mach/gp_gpio.h>
#include <mach/gp_board.h>
#include <mach/gp_usb.h>
#include <mach/hal/hal_usb.h>
//#include <plat/regs-udc.h>
//#include <plat/udc.h>

#include "spmp_udc.h"
#include <mach/hal/regmap/reg_scu.h>
#include <mach/regs-usbdev.h>
#define DRIVER_DESC	"USB Mass Storage Device"
#define DRIVER_VERSION	"16 Dec 2010"
#define DRIVER_AUTHOR	"Justin.Wang<justinwang@generalplus.com>, " \
			"Allen.Chang<allenchang@generalplus.com>"
#define IS_USBHOST_PRESENT() (udc_read(UDC_LLCS_OFST) & LCS_VBUS_HIGH)

/*************************** DEBUG FUNCTION ***************************/
#define	DMA_ADDR_INVALID	(~(dma_addr_t)0)
#define DEBUG_NORMAL	1
#define DEBUG_VERBOSE	2
#define debug 0
//#define CONFIG_USB_spmp_DEBUG
#ifdef CONFIG_USB_spmp_DEBUG
#define USB_spmp_DEBUG_LEVEL 3

#define dprintk(level,value...) printk(value)
#else
static int dprintk(int level, const char *fmt, ...)
{
	return 0;
}
#endif


void spmp_vbus_gpio_config (struct spmp_udc *udc, unsigned int en, unsigned int configIndex );
unsigned int spmp_vbus_detect( void );
void spmp_udc_irq_config_en( int en );
//static void spmp_udc_switch_buffer( void );
//static void spmp_udc_auto_switch_en( int en);
static DECLARE_WAIT_QUEUE_HEAD(udc_dma_done);

static const char		gadget_name[] = "spmp_udc";
static const char		driver_desc[] = DRIVER_DESC;
static struct spmp_udc	*the_controller;
//static struct clk		*usb_bus_clock;
static void __iomem		*base_addr;
static u64			rsrc_start;
static u64			rsrc_len;
static struct dentry		*spmp_udc_debugfs_root;
static int dma_done = 0;
static int vbus_config = USB_SLAVE_VBUS_POWERON1;
static int gpioHandle = 0;
unsigned int g_first_setup = 0;
static struct semaphore sem, sem_dma;
static int isResetInt = 0, mscReset = 0;
static int usbState = USB_STATE_CBW;
static int usbReadLength = 0;
static int usbWriteLength = 0;
static u8 cbwBuf[64] = {0};
static u8 cbwLength = 0;
static u8 ep1SetHalt = 0;
static u8 ep2SetHalt = 0;
static int usbCmd = 0;

EXPORT_SYMBOL(g_first_setup);

static inline u32 udc_read(u32 reg)
{
	return readl(base_addr + reg);
}

static inline void udc_write(u32 value, u32 reg)
{
	writel(value, base_addr + reg);
}

//static struct spmp_udc_mach_info *udc_info;

static int spmp_udc_debugfs_seq_show(struct seq_file *m, void *p)
{
#if 0
	u32 addr_reg,pwr_reg,ep_int_reg,usb_int_reg;
	u32 ep_int_en_reg, usb_int_en_reg, ep0_csr;
	u32 ep1_i_csr1,ep1_i_csr2,ep1_o_csr1,ep1_o_csr2;
	u32 ep2_i_csr1,ep2_i_csr2,ep2_o_csr1,ep2_o_csr2;

	addr_reg       = udc_read(spmp_UDC_FUNC_ADDR_REG);
	pwr_reg        = udc_read(spmp_UDC_PWR_REG);
	ep_int_reg     = udc_read(spmp_UDC_EP_INT_REG);
	usb_int_reg    = udc_read(spmp_UDC_USB_INT_REG);
	ep_int_en_reg  = udc_read(spmp_UDC_EP_INT_EN_REG);
	usb_int_en_reg = udc_read(spmp_UDC_USB_INT_EN_REG);
	udc_write(0, spmp_UDC_INDEX_REG);
	ep0_csr        = udc_read(spmp_UDC_IN_CSR1_REG);
	udc_write(1, spmp_UDC_INDEX_REG);
	ep1_i_csr1     = udc_read(spmp_UDC_IN_CSR1_REG);
	ep1_i_csr2     = udc_read(spmp_UDC_IN_CSR2_REG);
	ep1_o_csr1     = udc_read(spmp_UDC_IN_CSR1_REG);
	ep1_o_csr2     = udc_read(spmp_UDC_IN_CSR2_REG);
	udc_write(2, spmp_UDC_INDEX_REG);
	ep2_i_csr1     = udc_read(spmp_UDC_IN_CSR1_REG);
	ep2_i_csr2     = udc_read(spmp_UDC_IN_CSR2_REG);
	ep2_o_csr1     = udc_read(spmp_UDC_IN_CSR1_REG);
	ep2_o_csr2     = udc_read(spmp_UDC_IN_CSR2_REG);

	seq_printf(m, "FUNC_ADDR_REG  : 0x%04X\n"
		 "PWR_REG        : 0x%04X\n"
		 "EP_INT_REG     : 0x%04X\n"
		 "USB_INT_REG    : 0x%04X\n"
		 "EP_INT_EN_REG  : 0x%04X\n"
		 "USB_INT_EN_REG : 0x%04X\n"
		 "EP0_CSR        : 0x%04X\n"
		 "EP1_I_CSR1     : 0x%04X\n"
		 "EP1_I_CSR2     : 0x%04X\n"
		 "EP1_O_CSR1     : 0x%04X\n"
		 "EP1_O_CSR2     : 0x%04X\n"
		 "EP2_I_CSR1     : 0x%04X\n"
		 "EP2_I_CSR2     : 0x%04X\n"
		 "EP2_O_CSR1     : 0x%04X\n"
		 "EP2_O_CSR2     : 0x%04X\n",
			addr_reg,pwr_reg,ep_int_reg,usb_int_reg,
			ep_int_en_reg, usb_int_en_reg, ep0_csr,
			ep1_i_csr1,ep1_i_csr2,ep1_o_csr1,ep1_o_csr2,
			ep2_i_csr1,ep2_i_csr2,ep2_o_csr1,ep2_o_csr2
		);
#endif
	return 0;
}

static int spmp_udc_debugfs_fops_open(struct inode *inode,
					 struct file *file)
{
	return single_open(file, spmp_udc_debugfs_seq_show, NULL);
}

static const struct file_operations spmp_udc_debugfs_fops = {
	.open		= spmp_udc_debugfs_fops_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};

/* io macros */


/*------------------------- I/O ----------------------------------*/

/*
 *	spmp_udc_done
 */
static void spmp_udc_done(struct spmp_ep *ep,
		struct spmp_request *req, int status)
{
	unsigned halted = ep->halted;

	list_del_init(&req->queue);

	if (likely (req->req.status == -EINPROGRESS))
		req->req.status = status;
	else
		status = req->req.status;

	ep->halted = 1;
	req->req.complete(&ep->ep, &req->req);
	ep->halted = halted;
}

static void spmp_udc_nuke(struct spmp_udc *udc,
		struct spmp_ep *ep, int status)
{
	/* Sanity check */
	if (&ep->queue == NULL)
		return;

	while (!list_empty (&ep->queue)) {
		struct spmp_request *req;
		req = list_entry (ep->queue.next, struct spmp_request,
				queue);
		spmp_udc_done(ep, req, status);
	}
}

static int spmp_udc_nak_en (u8 ep, u8 en){
	switch (ep) {
	case USB_NAK_EP_EP0:
		/* Clear ACK status */
		udc_write(UDLC_EP0I_IE | UDLC_EP0O_IE, UDC_LLCIF_OFST);
		/* Clear NAK status */
		udc_write(UDLC_EP0N_IE , UDC_LLCIF_OFST);

		if ( en ) {
			/* Enable In/Out Nak Intr */
			udc_write(udc_read( UDC_LLCIE_OFST ) | UDLC_EP0N_IE , UDC_LLCIE_OFST);
		}
		else{
			/* Enable In/Out Nak Intr */
			udc_write(udc_read( UDC_LLCIE_OFST ) & (~UDLC_EP0N_IE) , UDC_LLCIE_OFST);
		}
		break;

	case USB_NAK_EP_BULK_IN:
		//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
		/* Clear ACK status */
		udc_write(UDLC_EP1I_IE, UDC_LLCIF_OFST);
		/* Clear NAK status */
		udc_write(UDLC_EP1N_IE , UDC_LLCIF_OFST);
		/* Enable In/Out Nak Intr */
        //udc_write(udc_read( UDC_LLCIE_OFST ) | UDLC_EP1N_IE , UDC_LLCIE_OFST);
		if ( en ) {
			/* Enable In/Out Nak Intr */
			udc_write(udc_read( UDC_LLCIE_OFST ) | UDLC_EP1N_IE , UDC_LLCIE_OFST);
		}
		else{
			/* Enable In/Out Nak Intr */
			udc_write(udc_read( UDC_LLCIE_OFST ) & (~UDLC_EP1N_IE) , UDC_LLCIE_OFST);
		}
		break;

	case USB_NAK_EP_BULK_OUT:
		//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
		/* Clear ACK status */
		udc_write(UDLC_EP2O_IE, UDC_LLCIF_OFST);
		/* Clear NAK status */
		udc_write(UDLC_EP2N_IE , UDC_LLCIF_OFST);
		/* Enable In/Out Nak Intr */
        //udc_write(udc_read( UDC_LLCIE_OFST ) | UDLC_EP2N_IE , UDC_LLCIE_OFST);
		if ( en ) {
			/* Enable In/Out Nak Intr */
			udc_write(udc_read( UDC_LLCIE_OFST ) | UDLC_EP2N_IE , UDC_LLCIE_OFST);
		}
		else{
			/* Enable In/Out Nak Intr */
			udc_write(udc_read( UDC_LLCIE_OFST ) & (~UDLC_EP2N_IE) , UDC_LLCIE_OFST);
		}
		break;

	default:
		printk("Un-implement NAK EN[%d]\n", ep);
		break;
	}
	return 0;
}


static int spmp_udc_ack_en (u8 ep, u8 en){
	switch (ep) {

	case USB_ACK_EP_BULK_IN:
		//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
		/* Clear ACK status */
		udc_write(UDLC_EP1I_IE, UDC_LLCIF_OFST);
		/* Clear NAK status */
		udc_write(UDLC_EP1N_IE , UDC_LLCIF_OFST);
		/* Enable In/Out Nak Intr */
		if( en ) {
			udc_write(udc_read( UDC_LLCIE_OFST ) | UDLC_EP1I_IE , UDC_LLCIE_OFST);
		}
		else{
			udc_write(udc_read( UDC_LLCIE_OFST ) & (~UDLC_EP1I_IE), UDC_LLCIE_OFST);
		}
		break;

	case USB_ACK_EP_BULK_OUT:
		//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
		/* Clear ACK status */
		udc_write(UDLC_EP2O_IE, UDC_LLCIF_OFST);
		/* Clear NAK status */
		udc_write(UDLC_EP2N_IE , UDC_LLCIF_OFST);
		/* Enable In/Out Nak Intr */
        //udc_write(udc_read( UDC_LLCIE_OFST ) | UDLC_EP2O_IE , UDC_LLCIE_OFST);
		if( en ) {
			udc_write(udc_read( UDC_LLCIE_OFST ) | UDLC_EP2O_IE , UDC_LLCIE_OFST);
		}
		else{
			udc_write(udc_read( UDC_LLCIE_OFST ) & (~UDLC_EP2O_IE), UDC_LLCIE_OFST);
		}
		break;

	default:
		printk("Un-implement NAK EN[%d]\n", ep);
		break;
	}
	return 0;
}


static void
spmp_udc_bulk_event(
	UINT8 isAckorNak
)
{
	UINT8 usbIrqSts;

	usbIrqSts = udc_read(UDC_IF_OFST) & udc_read(UDC_LLCIE_OFST);
	if (isAckorNak == 0) {
		/* For Bulk-ACK status */

		if ((usbIrqSts & UDLC_EP1I_IE) == UDLC_EP1I_IE) {
			//printk("EP1 In Ack int\n");
			/* Clear the interrupt. */
			udc_write(UDLC_EP1I_IE, UDC_LLCIF_OFST); 
		}
		/* Bulk Out-Ack event */
		else if ((usbIrqSts & UDLC_EP2O_IE) == UDLC_EP2O_IE) {
			//printk("EP2 Out Ack int\n");
			/* Clear the interrupt. */
			udc_write(UDLC_EP2O_IE, UDC_LLCIF_OFST); 
		}
	}
	else {
		/* Disable Bulk In/Out Nak Interrupt */
		udc_write(udc_read(UDC_LLCIE_OFST) & ~( UDLC_EP1N_IE | UDLC_EP2N_IE ) , UDC_LLCIE_OFST);

		if ((udc_read(UDC_IF_OFST) & UDLC_EP1N_IE) == UDLC_EP1N_IE) {
			//printk("EP1 In Nak int\n");
			/* This is In-Nak */
			/* Open Bulk-In for data to Host */
			//printk("[%d]\n", __LINE__);
			udc_write(EP12C_DIR_IN | EP12C_ENABLE_BULK,  UDC_EP12C_OFST);

			/* Clear Bulk-In NAK status */
			udc_write(UDLC_EP1N_IF, UDC_LLCIF_OFST); 
		}
		else if ((udc_read(UDC_IF_OFST) & UDLC_EP2N_IE) == UDLC_EP2N_IE) {
			//printk("EP2 Out Nak int\n");
			/* This is Out-Nak */
			/* Open Bulk-Out */
			udc_write((EP12C_ENABLE_BULK) , UDC_EP12C_OFST);	

			/* Clear Bulk-In NAK status */
			udc_write(UDLC_EP2N_IF, UDC_LLCIF_OFST); 
		}
	}
}

static inline int spmp_udc_fifo_count_ep0(void)
{
	int tmp;

	tmp = udc_read(UDC_EP0DC_OFST);
	return tmp;
}

static inline int spmp_udc_fifo_count_ep12(void)
{
	int tmp;

	tmp = udc_read(UDC_EP12FCH_OFST) << 8;
	tmp |=  udc_read(UDC_EP12FCL_OFST);;
	return tmp;
}

static inline int spmp_udc_fifo_count_ep3(void)
{
	int tmp;

	tmp = udc_read(UDC_EP3DC_OFST);
	return tmp;
}
 #define UDC_FLASH_BUFFER_SIZE (1024*64)
/*
 *	spmp_udc_write_packet
 */
static inline int spmp_udc_write_packet(int fifo,
		struct spmp_request *req,
		unsigned max)
{
	unsigned len = min(req->req.length - req->req.actual, max);
	u8 *buf = req->req.buf + req->req.actual;

//	prefetch(buf);

//	dprintk(DEBUG_VERBOSE, "%s %d %d %d %d\n", __func__,
//		req->req.actual, req->req.length, len, req->req.actual + len);

	req->req.actual += len;

	//udelay(5); allenchang 20110506
	writesb(base_addr + fifo, buf, len);
	return len;
}

/*
 *	spmp_udc_write_fifo
 *
 * return:  0 = still running, 1 = completed, negative = errno
 */
static int spmp_udc_write_ep0_fifo(struct spmp_ep *ep,
		struct spmp_request *req)
{
	unsigned	count;
	int		is_last;
	u32		idx, timeout_counter = 0;
	//int		fifo_reg;
	//u32		csr_reg;

	//printk("================================\n");
	//printk("spmp_udc_write_ep0_fifo\n");
	//printk("================================\n");

	idx = ep->bEndpointAddress & 0x7F;
   if(idx !=0)
   	{
		dprintk(DEBUG_NORMAL,"write ep0 idx error\n");
		return -1;
 	}
    //udc_write(0,  UDC_EP0DC_OFST);
    //ep->dev->ep0state=EP0_IDLE;
    udc_write(EP0CS_DIR_IN,  UDC_EP0CS_OFST);
	count = spmp_udc_write_packet(UDC_EP0DP_OFST, req, ep->ep.maxpacket);
	/* last packet is often short (sometimes a zlp) */
	if (count != ep->ep.maxpacket)
		is_last = 1;
	else if (req->req.length != req->req.actual || req->req.zero)
		is_last = 0;
	else
		is_last = 2;

	/* Only ep0 debug messages are interesting */
	if (idx == 0){
		dprintk(DEBUG_NORMAL,
			"Written ep%d %d.%d of %d b [last %d,z %d]\n",
			idx, count, req->req.actual, req->req.length,
			is_last, req->req.zero);
	}

	udc_write(EP0CS_SET_EP0_IVLD | EP0CS_DIR_IN,  UDC_EP0CS_OFST);
	while ((udc_read(UDC_EP0CS_OFST) & EP0CS_IVLD) == EP0CS_IVLD){
		timeout_counter++;
		/*Normal case < 3000. 
		  WHQL case < 109900*/
		if( timeout_counter >300000 || !spmp_vbus_detect() ) {
			printk("Warning! Ep0 in too slow[%s][%d]34c[%d]350[%d]\n", __FUNCTION__, timeout_counter, udc_read(UDC_EP0CS_OFST), spmp_udc_fifo_count_ep0());
			break;
		}
	}
//		dprintk(DEBUG_NORMAL, "spmp_udc_write_ep0_fifo = %08x\n",udc_read(UDC_EP0CS_OFST));
	if (is_last) {
      udc_write(udc_read(UDC_EP0CS_OFST) & ~(EP0CS_DIR_IN),  UDC_EP0CS_OFST);
	   ep->dev->ep0state=EP0_IDLE;
		spmp_udc_done(ep, req, 0);
		is_last = 1;
	} else {

	}
	return is_last;
}

/** 
 * 
 * 
 * @param ep
 * @param req
 * 
 * @return int
 */
static int spmp_udc_bulkin_ep1_dma ( struct spmp_ep *ep, struct spmp_request *req ){
	u8 *buf;
	u32 is_last = 0;
	u32 timeout_counter = 0;
	u32 timeout_counter1 = 0;
	int dma_xferlen = 0;

	//printk("[%s][%d][%x]\n", __FUNCTION__, __LINE__, udc_read(UDC_EP12C_OFST));

	if(req->req.dma == DMA_ADDR_INVALID) {
//    	    dprintk(DEBUG_NORMAL, "map dma\n");
		req->req.dma = dma_map_single(
		ep->dev->gadget.dev.parent,
		req->req.buf,
		req->req.length,
			(ep->bEndpointAddress & USB_DIR_IN)
			? DMA_TO_DEVICE
			: DMA_FROM_DEVICE);
	}

	/*Todo: Change PIO mode to DMA mode. allenchang@generalplus.com*/
	//Disable dma int
	udc_write(udc_read(UDC_IE_OFST) & ~CIE_DMA_IE , UDC_IE_OFST);
	/*Disable EP1 INT*/
	udc_write(udc_read(UDC_LLCIE_OFST) & ~(UDLC_EP1I_IE) , UDC_LLCIE_OFST);
	/*Clear Ep2 INT flag*/
	udc_write(UDLC_EP2O_IF , UDC_LLCIF_OFST);

	while(req->req.actual < req->req.length){
		int tmp;

		buf = (u8*)(req->req.dma+ req->req.actual);

		dma_xferlen = min(req->req.length - req->req.actual, (unsigned)UDC_FLASH_BUFFER_SIZE);
		//printk("dma_buf_addr = %08x , data length = %d\n",buf,dma_xferlen);
		dma_done = 0;
		udc_write((u32)buf,UDC_DMA_DA_OFST);
		udc_write(DMACS_DMA_READ |((dma_xferlen)) ,UDC_DMA_CS_OFST); //Set DMA Read Mode
		//Enable bulk in
		udc_write(EP12C_DIR_IN,  UDC_EP12C_OFST);
		udc_write(udc_read(UDC_DMA_CS_OFST) | DMACS_DMA_EN , UDC_DMA_CS_OFST);//Enable DMA
		udc_write(EP12C_DIR_IN | EP12C_ENABLE_BULK,  UDC_EP12C_OFST);
		//printk("start to wait interrupt\n");
		//wait_event_interruptible(udc_dma_done, dma_done == 1);
		/*Todo: Change PIO mode to DMA mode. allenchang@generalplus.com*/
		#if 1
		while ((udc_read(UDC_IF_OFST) & CIE_DMA_IE) == 0)
		{
			timeout_counter1++;
			#if 1
			if( timeout_counter1 >10000 ) {
				udelay(100);
			}
			if( timeout_counter1 >30000 || !spmp_vbus_detect() || isResetInt ) {
				printk("Warning! DATA in too slow[%s]Rst[%d]Len[%d][%d]Cnt[%d]320[%d]state[%d]\n", __FUNCTION__, isResetInt, req->req.length, req->req.actual, timeout_counter1, 
					   udc_read(UDC_LLCFS_OFST), usbState);
				printk("Warning! cnt[%d]400[%x]320[%x]330[%x]334[%x]364[%x]\n", spmp_udc_fifo_count_ep12(), 
					   udc_read(UDC_DMA_CS_OFST) & 0x3fffff,   udc_read(UDC_LLCFS_OFST),  udc_read(UDC_EP12C_OFST), 
					   udc_read(UDC_EP12PPC_OFST), udc_read(UDC_EP12FS_OFST));
				break;
			}
			#endif
		}
		udc_write(CIE_DMA_IE, UDC_IF_OFST);

		//udc_write(udc_read(UDC_DMA_CS_OFST) | DMACS_DMA_FLUSH, UDC_DMA_CS_OFST);
		while ((udc_read(UDC_DMA_CS_OFST) & DMACS_DMA_EN) != 0)
		{
			timeout_counter1++;
			#if 1
			if( timeout_counter1 >10000 ) {
				udelay(100);
			}
			if( timeout_counter1 >30000 || !spmp_vbus_detect() || isResetInt) {
				printk("Warning! OFlush DATA in too slow[%s]Rst[%d]Len[%d][%d]Cnt[%d]320[%d]state[%d]\n", __FUNCTION__, isResetInt, req->req.length, req->req.actual, timeout_counter1, 
					   udc_read(UDC_LLCFS_OFST), usbState);
				break;
			}
			#endif
		}
		#else
		ret = down_timeout(&sem_dma, 500);
		//pConfig->set_power(0);
		if( ret != 0 ) {
			printk("dma timeout %d, [%d]\n", __LINE__, isResetInt);
			break;
		}
		#endif
		dma_done = 0;
		tmp=0;
		//wait until usb phy finish, only read to host need to wait
		while (udc_read(UDC_LLCFS_OFST) & LCFS_EP1_IVLD)
		{
			timeout_counter++;
			if( timeout_counter >1900 ) {
				udelay(100);
			}
			if( timeout_counter >11900 || !spmp_vbus_detect() || isResetInt ) {
				printk("Warning! DATA in too slow2[%s][%d][%d].\n", __FUNCTION__, timeout_counter, udc_read(UDC_LLCFS_OFST));
				break;
			}
		}
		req->req.actual +=dma_xferlen;
		timeout_counter = 0;
		//udc_write(udc_read(UDC_EP12C_OFST) & ~(EP12C_DIR_IN),  UDC_EP12C_OFST);
		if(!spmp_vbus_detect())
			break;
	}
	
	if (req->req.dma != DMA_ADDR_INVALID) {
//	    dprintk(DEBUG_NORMAL, "unmap dma\n");
		dma_unmap_single(ep->dev->gadget.dev.parent,
		req->req.dma, req->req.length,
						 (ep->bEndpointAddress & USB_DIR_IN)
						 ? DMA_TO_DEVICE
						 : DMA_FROM_DEVICE);
		req->req.dma = DMA_ADDR_INVALID;
	}
	
	/*Clear EP1 INT flag*/
	udc_write(UDLC_EP1I_IF , UDC_LLCIF_OFST);

	udc_write(EP12C_DIR_IN , UDC_EP12C_OFST);
	/*Todo: Change PIO mode to DMA mode. allenchang@generalplus.com*/
	spmp_udc_done(ep, req, 0);
	is_last =1;
	return is_last;
}
/*
 *	spmp_udc_write_fifo
 *
 * return:  0 = still running, 1 = completed, negative = errno
 */
static int spmp_udc_bulkin_ep1_fifo(struct spmp_ep *ep, struct spmp_request *req)
{
	unsigned	count;
	int		is_last = 0;
	u32		idx, timeout_counter = 0;
	static int toggleFlag=0;

	/*Enable bulk in for writing buffer*/
	//printk("[%s][%d][%d][%x][%x][%x][%x]\n", __FUNCTION__, __LINE__, req->req.length, 
	//udc_read(UDC_EP12PPC_OFST), udc_read(UDC_EP12C_OFST), udc_read(UDC_IE_OFST), udc_read(UDC_IF_OFST));

	idx = ep->bEndpointAddress & 0x7F;
	if(idx !=1)
   	{
		dprintk(DEBUG_NORMAL,"write ep0 idx error\n");
		return -1;
 	}
//================================================================
   //  printk("non - dma mode==> data length is %d\n",req->req.length);
doAgain:
	timeout_counter = 0;


	/*Enable bulk in for writing buffer*/
	udc_write(EP12C_DIR_IN, UDC_EP12C_OFST);	

    count = spmp_udc_write_packet(UDC_EP12FDP_OFST, req, ep->ep.maxpacket);//Write data to buffer
	//last packet is often short (sometimes a zlp)//
	if (count != ep->ep.maxpacket)
		is_last = 1;
	else if (req->req.length != req->req.actual || req->req.zero)
	{
		is_last = 0;
	}
	else
	{
		is_last = 2;
	}

	if( spmp_udc_fifo_count_ep12() != req->req.length ) {
		printk("Warning!Ep1 FIFO[%d]320[%d]330[%d]364[%d]len[%d][%d]state[%d]reset[%d]cmd[%x]dma[%d]\n",timeout_counter, 
				   udc_read(UDC_LLCFS_OFST), udc_read(UDC_EP12C_OFST), 
				   udc_read(UDC_EP12FS_OFST), req->req.length, spmp_udc_fifo_count_ep12(), usbState, isResetInt, usbCmd,
			   udc_read(UDC_DMA_CS_OFST) & 0x3fffff);
	}
	udc_write( EP12C_SET_EP1_IVLD | EP12C_DIR_IN | EP12C_ENABLE_BULK,  UDC_EP12C_OFST);//Set Current EP1 IN Buffer Valid Flag
	toggleFlag = 0;
	
	//printk("[%s][%d][%d][%x][%x][%x]\n", __FUNCTION__, __LINE__, spmp_udc_fifo_count_ep12(), udc_read(UDC_EP12C_OFST), udc_read(UDC_IE_OFST), udc_read(UDC_IF_OFST));
	
	//wait until usb phy finish, only read to host need to wait
	while (udc_read(UDC_LLCFS_OFST) & LCFS_EP1_IVLD)
	{
		if(toggleFlag == 0) {
		 //printk("UDC_EP12FS_OFST(0x364) = %8x\n",udc_read(UDC_EP12FS_OFST));
			toggleFlag = 1;
		}
		timeout_counter++;
		if( timeout_counter >1000 ) {
			msleep(10);
		}
		if( timeout_counter >1200 || !spmp_vbus_detect() || isResetInt) {
			printk("Warning! Fifo DATA in too slow[%s][%d]320[%d]330[%d]364[%d]len[%d][%d]state[%d]reset[%d]cmd[%x]dma[%d]\n", __FUNCTION__, timeout_counter, 
				   udc_read(UDC_LLCFS_OFST), udc_read(UDC_EP12C_OFST), 
				   udc_read(UDC_EP12FS_OFST), req->req.length, spmp_udc_fifo_count_ep12(), usbState, isResetInt, usbCmd,
				   udc_read(UDC_DMA_CS_OFST) & 0x3fffff);
			/*Enable next CBW*/
			udc_write((EP12C_ENABLE_BULK) , UDC_EP12C_OFST);	
			spmp_udc_ack_en( USB_ACK_EP_BULK_OUT, 1 );
			/*For Next Command*/
			spmp_udc_nak_en( USB_NAK_EP_BULK_OUT, 1 );
			
			if( isResetInt ) {
				req->req.status = -ECONNRESET;
				spmp_udc_done(ep, req, -ECONNRESET);
			}
			else{
				req->req.status = -ECOMM;
				spmp_udc_done(ep, req, -ECOMM);
			}
			is_last = 0;
			return is_last;
		}
	}
	if (is_last) {
		if( usbReadLength == 0 ) {
			/*Disable bulk in/bulk*/
			udc_write(0, UDC_EP12C_OFST);
		}

		if( req->req.length == 13 ) {
			//spmp_udc_auto_switch_en(0);
		}

		if( usbReadLength == 0 ) {
			if( udc_read(UDC_IF_OFST) | UDLC_EP2N_IE ) {
				/* Open Bulk-Out */
				//printk("Walk Around!!\n");
				spmp_udc_ack_en( USB_ACK_EP_BULK_OUT, 1 );		
				udc_write((EP12C_ENABLE_BULK) , UDC_EP12C_OFST);	

			}
			else{
				//spmp_udc_ack_en( USB_ACK_EP_BULK_OUT );
				spmp_udc_nak_en( USB_NAK_EP_BULK_OUT, 1 );
			}
		}
		else{
			udc_write(EP12C_DIR_IN , UDC_EP12C_OFST);
		}
		//printk("[%s][%d][%x][%x][%x]\n", __FUNCTION__, __LINE__, udc_read(UDC_EP12C_OFST), udc_read(UDC_IE_OFST), udc_read(UDC_IF_OFST));
		spmp_udc_done(ep, req, 0);	
		is_last = 1;
	} else {
		goto doAgain;
	}
	return is_last;
}

/*
 *	spmp_udc_write_fifo
 *
 * return:  0 = still running, 1 = completed, negative = errno
 */
static int spmp_udc_write_ep3_fifo(struct spmp_ep *ep,
		struct spmp_request *req)
{
	unsigned	count;
	int		is_last;
	u32		idx;
	//int		fifo_reg;
	//u32		csr_reg;

	//printk("================================\n");
	//printk("spmp_udc_write_ep3_fifo\n");
	//printk("================================\n");

	idx = ep->bEndpointAddress & 0x7F;
   if(idx !=3)
   	{
		dprintk(DEBUG_NORMAL,"write ep3 idx error\n");
		return -1;
 	}

	count = spmp_udc_write_packet(UDC_EP3DP_OFST, req, ep->ep.maxpacket);

	/* last packet is often short (sometimes a zlp) */
	if (count != ep->ep.maxpacket)
		is_last = 1;
	else if (req->req.length != req->req.actual || req->req.zero)
		is_last = 0;
	else
		is_last = 2;

	/* Only ep0 debug messages are interesting */
	if (idx == 0)
		dprintk(DEBUG_NORMAL,
			"Written ep%d %d.%d of %d b [last %d,z %d]\n",
			idx, count, req->req.actual, req->req.length,
			is_last, req->req.zero);

	if (is_last) {
       udc_write(udc_read(UDC_EP3CS_OFST) |EP3CS_IVLD,  UDC_EP3CS_OFST);
		spmp_udc_done(ep, req, 0);
		is_last = 1;
	} else {
       udc_write(udc_read(UDC_EP3CS_OFST) |EP3CS_IVLD,  UDC_EP3CS_OFST);
	}

	return is_last;
}

static inline int spmp_udc_read_packet_cbwonly(int fifo, u8 *buf, int length)
{
    //udelay(5); allenchang 20110506
	readsb(fifo + base_addr, buf, length);
	return length;
}

static inline int spmp_udc_read_packet(int fifo, u8 *buf,
		struct spmp_request *req, unsigned avail)
{
	unsigned len;

	len = min(req->req.length - req->req.actual, avail);
	req->req.actual += len;

    //udelay(5); allenchang 20110506
	readsb(fifo + base_addr, buf, len);
	return len;
}

/*
 * return:  0 = still running, 1 = queue empty, negative = errno
 */
static int spmp_udc_read_ep0_fifo(struct spmp_ep *ep,
				 struct spmp_request *req)
{
	u8		*buf;
	//u32		ep_csr;
	unsigned	bufferspace;
	int		is_last=1;
	unsigned	avail;
	int		fifo_count = 0;
	u32		idx;
	//int		fifo_reg;

	//printk("================================\n");
	//printk("spmp_udc_read_ep0_fifo\n");
	//printk("================================\n");

	idx = ep->bEndpointAddress & 0x7F;
   if(idx !=0)
   	{
		dprintk(DEBUG_NORMAL,"write ep0 idx error\n");
		return -1;
 	}


	buf = req->req.buf + req->req.actual;
	bufferspace = req->req.length - req->req.actual;
	if (!bufferspace) {
		dprintk(DEBUG_NORMAL, "%s: buffer full!\n", __func__);
		return -1;
	}

//	udc_write(idx, spmp_UDC_INDEX_REG);

	fifo_count = udc_read(UDC_EP0DC_OFST);

	dprintk(DEBUG_NORMAL, "%s fifo count : %d\n", __func__, fifo_count);

	if (fifo_count > ep->ep.maxpacket)
		avail = ep->ep.maxpacket;
	else
		avail = fifo_count;

	fifo_count = spmp_udc_read_packet(UDC_EP0DP_OFST, buf, req, avail);

	/* checking this with ep0 is not accurate as we already
	 * read a control request
	 **/
	if (idx != 0 && fifo_count < ep->ep.maxpacket) {
		is_last = 1;
		/* overflowed this request?  flush extra data */
		if (fifo_count != avail)
			req->req.status = -EOVERFLOW;
	} else {
		is_last = (req->req.length <= req->req.actual) ? 1 : 0;
	}

	fifo_count = udc_read(UDC_EP0DC_OFST);

	/* Only ep0 debug messages are interesting */
	if (idx == 0)
		dprintk(DEBUG_VERBOSE, "%s fifo count : %d [last %d]\n",
			__func__, fifo_count,is_last);

	if (is_last) {
		udc_write(udc_read(UDC_EP0CS_OFST) |EP0CS_CLR_EP0_OVLD,  UDC_EP0CS_OFST);
		ep->dev->ep0state = EP0_IDLE;
		spmp_udc_done(ep, req, 0);
	} else {
		udc_write(udc_read(UDC_EP0CS_OFST) |EP0CS_CLR_EP0_OVLD,  UDC_EP0CS_OFST);
	}

	return is_last;
}

/*
 * return:  0 = still running, 1 = queue empty, negative = errno
 */
static int spmp_udc_bulkout_ep2_fifo(struct spmp_ep *ep,
				 struct spmp_request *req)
{
	u8		*buf;
	//u32		ep_csr;
	unsigned	bufferspace;
	int		is_last = 0;
	unsigned	avail;
	int		fifo_count = 0, ret = SUCCESS, i = 0, dataReadyCount = 0;
	u32		idx;
	u32		timeout_counter = 0;
	struct gp_board_usb_s *pConfig = gp_board_get_config("usb", gp_board_usb_t);
	int usbExitSaveRemoveConfig = (pConfig->get_exit_safe_remove_config != NULL) ? pConfig->get_exit_safe_remove_config():0;

	//printk("[%s][%d][%x][%x][%x]\n", __FUNCTION__, __LINE__, udc_read(UDC_EP12C_OFST), udc_read(UDC_IE_OFST), udc_read(UDC_IF_OFST));
__DO_AGAIN:
	timeout_counter = 0;

	idx = ep->bEndpointAddress & 0x7F;
	if(idx !=2){
		printk("read ep2 idx error\n");
		return -1;
	}
	if( req->req.length != 31 ) {
		/*Disable Ep2 INT*/
		udc_write(udc_read(UDC_LLCIE_OFST) & ~(UDLC_EP2O_IE) , UDC_LLCIE_OFST);
		/*Clear Ep2 INT flag*/
		udc_write(UDLC_EP2O_IF , UDC_LLCIF_OFST);
		/*Enable Bulk Out*/
		if( ((udc_read(UDC_EP12FS_OFST) & EP12C_CLR_EP2_OVLD)== EP12C_CLR_EP2_OVLD))  {
			printk("My GOD!!![%d]364[%x]\n", spmp_udc_fifo_count_ep12(), udc_read(UDC_EP12FS_OFST));
		}
		/*Enable Bulk / Ep2*/
		udc_write((EP12C_ENABLE_BULK) , UDC_EP12C_OFST);
	}

	/*Enable Bulk / Ep2*/
	udc_write((EP12C_ENABLE_BULK) , UDC_EP12C_OFST);

	buf = req->req.buf + req->req.actual;
	bufferspace = req->req.length - req->req.actual;
	if (!bufferspace) {
		printk("%s: buffer full![%d][%d]\n", __func__, req->req.length, req->req.actual);
		return -1;
	}
	if( req->req.length == 31 ) {
		usbState = USB_STATE_CBW;
		//printk("[%s][%d][%x][%x][%x]\n", __FUNCTION__, __LINE__, udc_read(UDC_EP12C_OFST), udc_read(UDC_IE_OFST), udc_read(UDC_IF_OFST));
		for(i=0; i<10000000; i++) {
			ret = down_timeout(&sem, 10);	
			if( ret != 0 && (!spmp_vbus_detect() || isResetInt || mscReset || (usbExitSaveRemoveConfig && gpHalUsbHostSafyRemoved()))) {
				printk("CBW[1]v[%d]r[%d]mr[%d]sm c[%d]sm[%d]\n", 
					   spmp_vbus_detect(), isResetInt, mscReset, usbExitSaveRemoveConfig, gpHalUsbHostSafyRemoved());
				/*Todo: Fixed command timeout fail. Sometimes the interrupt isn't triggered.*/
				if( isResetInt ) {
					req->req.status = -ECONNRESET;
					spmp_udc_done(ep, req, -ECONNRESET);
				}
				else{
					req->req.status = -ECOMM;
					spmp_udc_done(ep, req, -ECOMM);
				}
				//req->req.status = -ECOMM;
				//spmp_udc_done(ep, req, 0);
				//is_last = 1;
				is_last = 0;
				goto __CBW_BREAK;
			}
			else if((ret == 0 && cbwLength != 0) || ( dataReadyCount > 2)){
				//if( dataReadyCount > 2 ) {
				//	printk("pio[%d][%d]\n", i, dataReadyCount);
				//}
				req->req.status = 0;
				break;
			}
			if( spmp_udc_fifo_count_ep12() == 31 || spmp_udc_fifo_count_ep12() == 30 || spmp_udc_fifo_count_ep12() == 32 ) {
				dataReadyCount++;
			}
			#if 0
			/*Debugging Message*/
			if( i >= 50 && ((i % 10) ==0)) {
				printk("[%s][%d]320[%x]364[%x]330[%x]404[%x]400[%x], len[%d][%d]\n", __FUNCTION__, __LINE__, 
					   udc_read(UDC_LLCFS_OFST), udc_read(UDC_EP12FS_OFST),
					   udc_read(UDC_EP12C_OFST), udc_read(UDC_IE_OFST), udc_read(UDC_IF_OFST),
					   spmp_udc_fifo_count_ep12(), cbwLength);
			}
			#endif
		}
		/*Todo: Fixed the 31 byte checking walking around.*/
		if( ret != 0 && spmp_udc_fifo_count_ep12() != 31 ) {
			printk("CBW Timeout[2]\n");
			//printk("330[%x]320[%x]364[%x]IE[%x]len[%d]\n", udc_read(UDC_EP12C_OFST), udc_read(UDC_LLCFS_OFST), udc_read(UDC_EP12FS_OFST), 
			//	   udc_read(UDC_IE_OFST) & UDLC_EP2O_IE & UDLC_EP2N_IE, spmp_udc_fifo_count_ep12());
			goto __CBW_BREAK;
		}
		if( ret != 0 && (spmp_udc_fifo_count_ep12() == 31 || spmp_udc_fifo_count_ep12() == 30 || spmp_udc_fifo_count_ep12() == 32 ) ) {
			udc_write(UDLC_EP2O_IF , UDC_LLCIF_OFST);
			udc_write(udc_read(UDC_LLCIE_OFST) & (~UDLC_EP2O_IE) , UDC_LLCIE_OFST);
			cbwLength = spmp_udc_fifo_count_ep12();
			printk("CBW[%d]\n", cbwLength);
			//printk("320[%x]364[%x]330[%x]404[%x]400[%x], len[%d]\n",  
			//		   udc_read(UDC_LLCFS_OFST), udc_read(UDC_EP12FS_OFST),
			//		   udc_read(UDC_EP12C_OFST), udc_read(UDC_IE_OFST), udc_read(UDC_IF_OFST),
			//		   spmp_udc_fifo_count_ep12());
			spmp_udc_read_packet_cbwonly(UDC_EP12FDP_OFST, buf, cbwLength);
			if(buf[0] != 0x55 || cbwLength == 30 || cbwLength == 32) {
				printk("CMD[%x][%x]\n", buf[0], buf[15]);
				ep1SetHalt = 1;
				ep2SetHalt = 1;
			}
		}
		else{
			memcpy( buf, cbwBuf, cbwLength );
			memset(cbwBuf, 0, 31);
		}
		usbCmd = buf[15];
		req->req.actual = cbwLength;
		cbwLength = 0;
		//printk("[%s][%d][%x][%x][%x]\n", __FUNCTION__, __LINE__, udc_read(UDC_EP12C_OFST), udc_read(UDC_IE_OFST), udc_read(UDC_IF_OFST));
		is_last = 1;
		/*Setting Length for DMA/PIO handling.*/
		if( req->req.length == 31 ) {
			if( buf[15] == 0x28 ) {
				usbReadLength = buf[23] << 9;
				//printk("Len[%d]\n", usbReadLength);
				//udc_write(EP12C_DIR_IN , UDC_EP12C_OFST);	
			}
			else if( buf[15] == 0x2a ) {
				usbWriteLength = buf[23] << 9;
				//udc_write(EP12C_CLR_EP2_OVLD, UDC_EP12C_OFST);	
				//printk("Len[%d]\n", usbReadLength);
			}
			else{
				//udc_write(EP12C_CLR_EP2_OVLD , UDC_EP12C_OFST);	
				usbReadLength = 0;
				usbWriteLength = 0;
			}
		}
		/*Change to next FIFO*/
		#if 0
		if( (udc_read(UDC_EP12C_OFST) & EP12C_EP2_OVLD) != EP12C_EP2_OVLD ) {
			printk("[%s][%d]??[%x]\n", __FUNCTION__, __LINE__, udc_read(UDC_EP12C_OFST));
		}
		#endif
        //printk("cmd[%x]\n", buf[15]);
		//if( (buf[15] == 0x2a) && ((udc_read(UDC_EP12FS_OFST) & 0x20) == 0x20) ){
			//printk("??[%s][%d][%x][%x][%x][%x]\n", __FUNCTION__, __LINE__, udc_read(UDC_EP12C_OFST), udc_read(UDC_LLCFS_OFST), udc_read(UDC_EP12FS_OFST), udc_read(UDC_IE_OFST) & udc_read(UDC_IF_OFST));
		//}
		#if 0
			/*Debugging Message*/
		if( (buf[15] == 0x2a) && ((udc_read(UDC_EP12FS_OFST) & 0x04) != 0x04) ){
			printk("??[%s][%d][%d][%x][%x][%x][%x]\n", __FUNCTION__, __LINE__, 
				   spmp_udc_fifo_count_ep12(), udc_read(UDC_EP12C_OFST), udc_read(UDC_LLCFS_OFST), 
				   udc_read(UDC_EP12FS_OFST), udc_read(UDC_IE_OFST) & udc_read(UDC_IF_OFST));
		}
		#endif
		if( buf[15] == 0x2a ) {
			/*Todo: Not turn off the bulk for command 2a*/
			udc_write(EP12C_CLR_EP2_OVLD | EP12C_ENABLE_BULK, UDC_EP12C_OFST);	
		}
		else{
			udc_write(EP12C_CLR_EP2_OVLD, UDC_EP12C_OFST);	
		}
		#if 0
		if( (buf[15] == 0x2a) && ((udc_read(UDC_EP12FS_OFST) & 0x02) == 0x02) && (spmp_udc_fifo_count_ep12() != 512) ){
			printk("??[%s][%d][%d][%x][%x][%x][%x]\n", __FUNCTION__, __LINE__, spmp_udc_fifo_count_ep12(), 
				   udc_read(UDC_EP12C_OFST), udc_read(UDC_LLCFS_OFST), udc_read(UDC_EP12FS_OFST), udc_read(UDC_IE_OFST) & udc_read(UDC_IF_OFST));
		}
		#endif
		//printk("d[%d]\n", cbwLength);
		spmp_udc_done(ep, req, 0);
__CBW_BREAK:		
		return is_last;
		//to IRQ udc_write((udc_read(UDC_EP12C_OFST) & ~EP12C_ENABLE_BULK) , UDC_EP12C_OFST);	
	}

	fifo_count = spmp_udc_fifo_count_ep12();
	/*Todo allenchang@generalplus.com Fix the while loop.*/
	if( fifo_count == 0 ) {
		while(fifo_count==0) {
			fifo_count = spmp_udc_fifo_count_ep12();
			timeout_counter++;
			if( timeout_counter >100 ) {
				msleep(10);				
			}
			if( timeout_counter >400 || !spmp_vbus_detect() || isResetInt) {
				printk("Warning! Write Fifo data slow[%s][%d][%d][%d]len[%d]rst[%d]\n", __FUNCTION__, timeout_counter, fifo_count, usbState, req->req.length, isResetInt);
				printk("[%s][%d][%x][%x][%x]\n", __FUNCTION__, __LINE__, udc_read(UDC_EP12C_OFST), udc_read(UDC_IE_OFST), udc_read(UDC_IF_OFST));
				break;
			}
		}
	}
	if (fifo_count > ep->ep.maxpacket)
		avail = ep->ep.maxpacket;
	else
		avail = fifo_count;

	fifo_count = spmp_udc_read_packet(UDC_EP12FDP_OFST, buf, req, avail);
	if( fifo_count == 0 ) {
		printk("Error! 330[%s][%d] !!!!!!!!!!!!!!!!!!!!!!!!!!\n", __FUNCTION__, __LINE__);
		return is_last;
	}

	/* checking this with ep0 is not accurate as we already
	 * read a control request
	 **/
	if (idx != 0 && fifo_count < ep->ep.maxpacket) {
		is_last = 1;
		/* overflowed this request?  flush extra data */
		if (fifo_count != avail)
			req->req.status = -EOVERFLOW;
	} else {
		is_last = (req->req.length <= req->req.actual) ? 1 : 0;
	}

	/*Change to next FIFO*/
	#if debug
	if( (udc_read(UDC_EP12C_OFST) & EP12C_EP2_OVLD) == EP12C_EP2_OVLD ) {
		printk("[%s][%d]??[%x]\n", __FUNCTION__, __LINE__, udc_read(UDC_EP12C_OFST));
	}
	#endif
	if( usbWriteLength != 0 ) {
		/*It's must be enable bulk for next DMA transferring.*/
		udc_write(EP12C_CLR_EP2_OVLD | EP12C_ENABLE_BULK,  UDC_EP12C_OFST);
	}
	else{
		udc_write(EP12C_CLR_EP2_OVLD,  UDC_EP12C_OFST);
	}
	if (is_last) {
		spmp_udc_done(ep, req, 0);
	} else {
		/*Change to next FIFO*/
		//udc_write(EP12C_CLR_EP2_OVLD,  UDC_EP12C_OFST);
	}

	/*Clear Ep2 INT flag*/
	udc_write(UDLC_EP2O_IF , UDC_LLCIF_OFST);
	/*Enable Ep2 INT*/
	//udc_write(udc_read(UDC_LLCIE_OFST) | (UDLC_EP2O_IE) , UDC_LLCIE_OFST);
	
	//printk("cmd[%x]\n", buf[15]);
	if( is_last == 0 ) {
		goto  __DO_AGAIN;
	}
	return is_last;
}

/*
 * return:  0 = still running, 1 = queue empty, negative = errno
 * 
 * 
 * @param ep
 * @param req
 * 
 * @return int
 */
static int spmp_udc_bulkout_ep2_dma(struct spmp_ep *ep,
				 struct spmp_request *req)
{
	u8		*buf;
	int		is_last=0;
	u32		idx, timeout_counter = 0, timeout_counter1 = 0;
	idx = ep->bEndpointAddress & 0x7F;

	//Disable dma int
	udc_write(udc_read(UDC_IE_OFST) & (~CIE_DMA_IE) , UDC_IE_OFST);
	/*Disable Ep2 INT*/
	udc_write(udc_read(UDC_LLCIE_OFST) & (~(UDLC_EP2O_IE)) , UDC_LLCIE_OFST);
	/*Clear Ep2 INT flag*/
	//udc_write(UDLC_EP2O_IF , UDC_LLCIF_OFST);
	/*Enable Bulk Out*/
	if( ((udc_read(UDC_EP12FS_OFST) & EP12C_CLR_EP2_OVLD)== EP12C_CLR_EP2_OVLD))  {
		printk("My GOD!!![%d]364[%x]\n", spmp_udc_fifo_count_ep12(), udc_read(UDC_EP12FS_OFST));
	}
	if( ((udc_read(UDC_EP12FS_OFST) & EP12C_SET_EP1_IVLD)== EP12C_SET_EP1_IVLD))  {
		printk("My GOD2!!![%d]364[%x]\n", spmp_udc_fifo_count_ep12(), udc_read(UDC_EP12FS_OFST));
	}

	if(idx !=2)
	{
		dprintk(DEBUG_NORMAL,"write ep2 idx error\n");
		return -1;
 	}
	if(!(udc_read(UDC_EP12C_OFST) & EP12C_MSDC_CMD_VLD))  // force in dma
	{
		int dma_xferlen = 0;
		
		if(req->req.dma == DMA_ADDR_INVALID) {
			req->req.dma = dma_map_single(
				ep->dev->gadget.dev.parent,
				req->req.buf,
				req->req.length,
				(ep->bEndpointAddress & USB_DIR_IN)
					? DMA_TO_DEVICE
					: DMA_FROM_DEVICE);
		}

		while((req->req.actual < req->req.length))
		{
			buf = (u8 *)(req->req.dma+ req->req.actual);
			dma_xferlen = min(req->req.length - req->req.actual, (unsigned)UDC_FLASH_BUFFER_SIZE);
			//dma_xferlen = min(req->req.length - req->req.actual, (unsigned)512);
			dprintk(DEBUG_NORMAL,"dma_xfer read = %08x %d\n",buf,dma_xferlen);
			dma_done = 0;
			udc_write((u32)buf,UDC_DMA_DA_OFST);
			udc_write(DMACS_DMA_WRITE | (dma_xferlen)  ,UDC_DMA_CS_OFST);
		    udc_write(udc_read(UDC_DMA_CS_OFST) | DMACS_DMA_EN , UDC_DMA_CS_OFST);
			//udc_write((udc_read(UDC_EP12C_OFST) & (~EP12C_DIR_IN)) | (EP12C_ENABLE_BULK) , UDC_EP12C_OFST);	
			udc_write((EP12C_ENABLE_BULK) , UDC_EP12C_OFST);	
		    //wait_event_interruptible(udc_dma_done, dma_done == 1);
			/*Todo: Change PIO mode to DMA mode. allenchang@generalplus.com*/
			#if 1
			while (((udc_read(UDC_IF_OFST) & CIE_DMA_IE) == 0))
			{
				timeout_counter++;
				if( timeout_counter >10000 ) {
					udelay(100);
				}
				if( timeout_counter >100000 || !spmp_vbus_detect() || isResetInt ) {
					printk("Warning!DMA[O] Slow!rst[%d]Len[%d][%d][%d][%d]dmacnt[%d]Cnt[%d]320[%d]330[%d]364[%d]state[%d]\n", isResetInt, 
						   req->req.length, spmp_udc_fifo_count_ep12(), req->req.actual, dma_xferlen,  udc_read(UDC_DMA_CS_OFST) & 0x3fffff, 
						   timeout_counter,  udc_read(UDC_LLCFS_OFST), udc_read(UDC_EP12C_OFST), udc_read(UDC_EP12FS_OFST), usbState);
					break;
				}
			}
			udc_write(CIE_DMA_IE, UDC_IF_OFST);
			while ((udc_read(UDC_DMA_CS_OFST) & DMACS_DMA_EN) != 0)
			{
				timeout_counter1++;
				#if 1
				if( timeout_counter1 >10000 ) {
					udelay(100);
				}
				if( timeout_counter1 >30000 || !spmp_vbus_detect() || isResetInt) {
				printk("Warning! OFlush DATA in too slow[%s]Rst[%d]Len[%d][%d]Cnt[%d]320[%d]state[%d]\n", __FUNCTION__, isResetInt, req->req.length, req->req.actual, timeout_counter1, 
					   udc_read(UDC_LLCFS_OFST), usbState);
				break;
				}
				#endif
			}
			#else
			ret = down_timeout(&sem_dma, 500);
			if( ret != 0 ) {
				printk("dma timeout %d\n", __LINE__);
				break;
			}
			#endif
			dma_done = 0;
			req->req.actual +=dma_xferlen;
			if(!spmp_vbus_detect())
				break;
			timeout_counter = 0;
			//udc_write(0 ,UDC_DMA_CS_OFST);
      }
	  if (req->req.dma != DMA_ADDR_INVALID) {
				dma_unmap_single(ep->dev->gadget.dev.parent,
				req->req.dma, req->req.length,
				(ep->bEndpointAddress & USB_DIR_IN)
					? DMA_TO_DEVICE
					: DMA_FROM_DEVICE);
        		req->req.dma = DMA_ADDR_INVALID;
	  }
	  
	  if( usbWriteLength != 0 ) {
		  udc_write(EP12C_ENABLE_BULK, UDC_EP12C_OFST);
	  }
	  spmp_udc_done(ep, req, 0);
	  is_last =1;

	  /*Clear Ep2 INT flag*/
	  udc_write(UDLC_EP2O_IF , UDC_LLCIF_OFST);
	  /*Enable Ep2 INT*/
	  //udc_write(udc_read(UDC_LLCIE_OFST) | (UDLC_EP2O_IE) , UDC_LLCIE_OFST);//ymlin
	  /*Todo: Change PIO mode to DMA mode. allenchang@generalplus.com*/
  	}
	//else{
		//printk("[%s][%d]DMA FAIL\n", __FUNCTION__, __LINE__);
	//}
	return is_last;
}

static int spmp_udc_readep0s_fifo_crq(struct usb_ctrlrequest *crq)
{
	unsigned char *outbuf = (unsigned char*)crq;
	int bytes_read = 0;

//	udc_write(0, spmp_UDC_INDEX_REG);

	bytes_read = 8;

//	dprintk(DEBUG_NORMAL, "%s: fifo_count=%d\n", __func__, bytes_read);

	if (bytes_read > sizeof(struct usb_ctrlrequest))
		bytes_read = sizeof(struct usb_ctrlrequest);

	readsb(UDC_EP0SDP_OFST + base_addr, outbuf, bytes_read);

	dprintk(DEBUG_VERBOSE, "%s: len=%d %02x:%02x {%x,%x,%x}\n", __func__,
		bytes_read, crq->bRequest, crq->bRequestType,
		crq->wValue, crq->wIndex, crq->wLength);

	return bytes_read;
}
/*------------------------- usb state machine -------------------------------*/
static int spmp_udc_set_halt(struct usb_ep *_ep, int value);

static void spmp_udc_handle_ep0s_idle(struct spmp_udc *dev,
					struct spmp_ep *ep,
					struct usb_ctrlrequest *crq,
					u32 ep0csr)
{
	int len, ret;

	/* start control request? */

	spmp_udc_nuke(dev, ep, -EPROTO);
	len = spmp_udc_readep0s_fifo_crq(crq);
	if (len != sizeof(*crq)) {
		dprintk(DEBUG_NORMAL, "setup begin: fifo READ ERROR"
			" wanted %d bytes got %d. Stalling out...\n",
			sizeof(*crq), len);
//		spmp_udc_set_ep0_ss(base_addr);
       udc_write((udc_read(UDC_LLCSTL_OFST) | LCSTL_SETEP0STL) , UDC_LLCSTL_OFST); // error send stall;
		return;
	}

	dprintk(DEBUG_NORMAL, "bRequest = %x bRequestType %x wLength = %d\n",
		crq->bRequest, crq->bRequestType, crq->wLength);

	/* cope with automagic for some standard requests. */
	dev->req_std = (crq->bRequestType & USB_TYPE_MASK)
		== USB_TYPE_STANDARD;
	dev->req_config = 1;
	dev->req_pending = 1;
	switch (crq->bRequest) {
	case USB_REQ_GET_DESCRIPTOR:
		{
			   u32 DescType=((crq->wValue)>>8);
				if(DescType == 0x1)
				{
				   int tmp=0;
					tmp =udc_read(UDC_LLCS_OFST);
					if(tmp & LCS_CURR_SPEED_F)
					{
					   dprintk(DEBUG_NORMAL,"DESCRIPTOR SPeed = USB_SPEED_FULL\n");
					   dev->gadget.speed = USB_SPEED_FULL;
					}
					else
					{
					   dprintk(DEBUG_NORMAL,"DESCRIPTOR SPeed = USB_SPEED_HIGH\n");
					   dev->gadget.speed = USB_SPEED_HIGH;
					}
				}
		}
	   break;
	case USB_REQ_SET_CONFIGURATION:
		dprintk(DEBUG_NORMAL, "USB_REQ_SET_CONFIGURATION ... \n");
		/*It's expected the device has reset the data toggles on all Bulk endpoints after set configuration.*/
		mscReset = 1;
		break;

	case USB_REQ_SET_INTERFACE:
		/*It's expected the device has reset the data toggles on all Bulk endpoints after set configuration.*/
		mscReset = 1;
		break;

	case USB_REQ_SET_ADDRESS:
		dprintk(DEBUG_NORMAL, "USB_REQ_SET_ADDRESS ... \n");
		break;

	case USB_REQ_GET_STATUS:
		dprintk(DEBUG_NORMAL, "USB_REQ_GET_STATUS ... \n");
		break;

	case USB_REQ_CLEAR_FEATURE:
		break;

	case USB_REQ_SET_FEATURE:
		break;

	default:
		break;
	}

	if (crq->bRequestType & USB_DIR_IN)
		dev->ep0state = EP0_IN_DATA_PHASE;
	else
		dev->ep0state = EP0_OUT_DATA_PHASE;

	ret = dev->driver->setup(&dev->gadget, crq);
	//mdelay(10); allenchang 20110506
	if (ret < 0) {
		if (dev->req_config) {
			dprintk(DEBUG_NORMAL, "config change %02x fail %d?\n",
				crq->bRequest, ret);
			return;
		}

		if (ret == -EOPNOTSUPP)
			dprintk(DEBUG_NORMAL, "Operation not supported\n");
		else
			dprintk(DEBUG_NORMAL,
				"dev->driver->setup failed. (%d)\n", ret);
		dev->ep0state = EP0_IDLE;
		/* deferred i/o == no response yet */
	} else if (dev->req_pending) {
		dprintk(DEBUG_VERBOSE, "dev->req_pending... what now?\n");
		dev->req_pending=0;
		/*MSC reset command*/
		if( crq->bRequest == 0xff ) {
			/*Give up current packet.*/
			//mscReset = 1;
			//printk("MSC reset\n");
			/*For invalid CBW, all bulk endpoint should return STALL in USB MASS STORAGE spec until MSC reset.*/
			ep1SetHalt = 0;
			ep2SetHalt = 0;
			//udc_write(EP12C_CLR_EP2_OVLD | EP12C_RESET_PIPO ,UDC_EP12C_OFST);
			//spmp_udc_nak_en( USB_NAK_EP_BULK_OUT, 1 );
			//spmp_udc_ack_en( USB_ACK_EP_BULK_OUT, 1 );
			//udc_write(0 , UDC_EP12C_OFST);
			#if 0
			if( udc_read(UDC_IF_OFST) | UDLC_EP2N_IE ) {
				/* Open Bulk-Out */
				printk("MSC reset Walk Around!!\n");
				spmp_udc_ack_en( USB_ACK_EP_BULK_OUT, 1 );
				udc_write((EP12C_ENABLE_BULK) , UDC_EP12C_OFST);	
			}
			else{
				//spmp_udc_ack_en( USB_ACK_EP_BULK_OUT );
				spmp_udc_nak_en( USB_NAK_EP_BULK_OUT, 1 );
			}
			spmp_udc_nak_en( USB_NAK_EP_BULK_IN, 1 );
			#endif
		}
	}

	dprintk(DEBUG_VERBOSE, "ep0state %s\n", ep0states[dev->ep0state]);
}

static void spmp_udc_handle_ep0s(struct spmp_udc *dev)
{
	u32	ep0csr;
	struct spmp_ep	*ep = &dev->ep[0];
	struct spmp_request	*req;
	struct usb_ctrlrequest	crq;

	if (list_empty(&ep->queue))
		req = NULL;
	else
		req = list_entry(ep->queue.next, struct spmp_request, queue);

    ep0csr = udc_read(UDC_EP0CS_OFST);
	dprintk(DEBUG_NORMAL, "spmp_udc_handle_ep0 =%08x %08x\n",req,ep0csr);

	switch (dev->ep0state) {
	case EP0_IDLE:
		dprintk(DEBUG_NORMAL, "EP0_IN_IDLE_PHASE ... what now?\n");
		spmp_udc_handle_ep0s_idle(dev, ep, &crq, ep0csr);
		break;

	case EP0_END_XFER:
		dprintk(DEBUG_NORMAL, "EP0_END_XFER ... what now?\n");
		dev->ep0state = EP0_IDLE;
		break;

	case EP0_STALL:
		dprintk(DEBUG_NORMAL, "EP0_STALL ... what now?\n");
		dev->ep0state = EP0_IDLE;
		break;
	}
}

/*Todo: Remove it for useless. allenchang@generalplus.com*/
#if 0
static void spmp_udc_handle_ep_out(struct spmp_ep *ep)
{
	struct spmp_request	*req;
	//int			is_in = ep->bEndpointAddress & USB_DIR_IN;
	u32			ep_csr1;
	u32         llcstl;
	u32			idx;

handle_ep_again:


	if (likely (!list_empty(&ep->queue)))
		req = list_entry(ep->queue.next,
				struct spmp_request, queue);
	else
		req = NULL;

	idx = ep->bEndpointAddress & 0x7F;
   if(idx !=2)
   	{
		dprintk(DEBUG_NORMAL, "write ep2 idx error\n");
		return;
 	}
	ep_csr1 = udc_read(UDC_EP12C_OFST);
	dprintk(DEBUG_NORMAL, "Read =%d\n",req ? 1 : 0);
	  if ((ep_csr1 & EP12C_EP2_OVLD) && req) {
	 	spmp_udc_read_ep2_fifo(ep,req);
		if(spmp_udc_fifo_count_ep12())
			goto handle_ep_again;
	 }
//ymlin 0506
    llcstl = udc_read(UDC_LLCSTL_OFST);
	dprintk(DEBUG_NORMAL, "ep%01d rd csr:%02x %08x\n", idx, ep_csr1,llcstl);
	if (llcstl & LCSTL_SETEP2STL) {
    	udc_write(llcstl |LCSTL_CLREP2STL ,UDC_LLCSTL_OFST);
		return;
	}
}
#endif

/*
 *	spmp_udc_irq - interrupt handler
 */
/* from ecos code */
static void clearHwState_UDC(int a_iMode)
{
	int tmp;
	/*INFO: we don't disable udc interrupt when we are clear udc hw state,
	* 1.since when we are clearing, we are in ISR , will not the same interrupt reentry problem.
	* 2.after we finish clearing , we will go into udc ISR again, if there are interrupts occur while we are clearing ,we want to catch them
	*  immediately
	*/
	//===== check udc DMA state, and flush it =======
	if(udc_read(UDC_DMA_CS_OFST) & DMACS_DMA_EN) {
		udc_write(udc_read(UDC_DMA_CS_OFST) | DMACS_DMA_FLUSH,UDC_DMA_CS_OFST);
		while(!(udc_read(UDC_DMA_CS_OFST) & DMACS_DMA_FLUSHEND)){
			tmp++;
			if(tmp> 300000){
				printk("##");
				tmp=0;
			}
		}
	}
	
	/*Disable Interrupt */
	//udc_write(0x0, UDC_LLCIE_OFST); 
	/*Clear Interrupt Flag*/
	udc_write(0xffffff, UDC_LLCIF_OFST); 
	/*Clear Interrupt Statue*/
	udc_write(0xffffff, UDC_LLCIS_OFST); 

	//EP0
	udc_write(EP0CS_CLR_EP0_OVLD, UDC_EP0CS_OFST); //clear ep0 out vld=1, clear set epo in vld=0, set ctl dir to OUT direction=0
	udc_write(0x0,UDC_EP0DC_OFST);
	//EP1
    udc_write(EP1SCS_CLR_IVLD | EP1SCS_RESET_FIFO ,UDC_EP1SCS_OFST);
	//EP2
	udc_write(EP12C_CLR_EP2_OVLD | EP12C_RESET_PIPO ,UDC_EP12C_OFST);
	//EP1/2 Filo CNT
	udc_write(0 ,UDC_EP12FCL_OFST);
	udc_write(udc_read(UDC_EP12FCH_OFST) | EP12FCH_RESET_CNTR, UDC_EP12FCH_OFST);
	udc_write(0 ,UDC_EP12FCH_OFST);
	
	/*Clear Stall Status*/
	udc_write((LCSTL_CLREP3STL | LCSTL_CLREP2STL | LCSTL_CLREP1STL | LCSTL_CLREP0STL), UDC_LLCSTL_OFST);

	/*Filo Auto Switch Enable*/
	//udc_write(udc_read(UDC_EP12PPC_OFST) | EP12PPC_AUTO_SW_EN ,UDC_EP12PPC_OFST);
	//spmp_udc_auto_switch_en(0);

	// 2008/6/26, to prevent when PIPO IS IN , plug off intr occur; or comment this since each new CBW will set it again
	udc_write(0, UDC_EP12C_OFST);	

	/*Recover to normal*/
	spmp_udc_nak_en( USB_NAK_EP_BULK_OUT, 1 );
	spmp_udc_nak_en( USB_NAK_EP_BULK_IN, 1 );
}



static int vbusIntr2_UDC( struct spmp_udc *dev )
{
	unsigned int 	tmp;
	unsigned int 	llcset0;
	tmp = udc_read(UDC_LLCS_OFST);
	/*It always connects with host by none VBUS.*/
	if( (vbus_config == USB_SLAVE_VBUS_NONE) || (tmp & LCS_VBUS_HIGH)){
		llcset0 = udc_read(UDC_LLCSET0_OFST);
       if(llcset0 & LCSET0_SOFT_DISC)
		{
		   gpHalUsbSlaveSwConnect(1);
		   printk("USB Connect 3B0[%x] \n",udc_read(UDC_LLCSET0_OFST));
		}
	}
	else /* host absent */
	{
		gpHalUsbSlaveSwConnect(0);
		dev->driver->disconnect(&dev->gadget);
		printk("USB Disconnect\n");
		clearHwState_UDC(0);
	}

	/*Clear VBUS int source*/
	udc_write(udc_read(UDC_CIS_OFST) | CIS_VBUS_IF, UDC_CIS_OFST);
   return 0;
}

static void spmp_udc_vbus_gpio_irq(void *_dev)
{
	struct spmp_udc *dev = _dev;
	//unsigned long flags;
	unsigned int value = 0;
	unsigned int debounceTime = 0x1fffff;
	//u32 llcset0 = udc_read(UDC_LLCSET0_OFST);

	gp_gpio_get_value(gpioHandle, &value);
	if( value == 1 ) {
		gpHalUsbSlaveSwConnect(1);
		/*Change to detect disconnect*/
		gp_gpio_irq_property(gpioHandle, (GPIO_IRQ_LEVEL_LOW << 8) | GPIO_IRQ_LEVEL_TRIGGER, &debounceTime);
		printk("USB Device Connect -GPIO\n");
	}
	else {
		gpHalUsbSlaveSwConnect(0);
		dev->driver->disconnect(&dev->gadget);
		clearHwState_UDC(0);
		gp_gpio_irq_property(gpioHandle, (GPIO_IRQ_LEVEL_HIGH << 8) | GPIO_IRQ_LEVEL_TRIGGER, &debounceTime);
		printk("USB Device DisConnect -GPIO\n");
	}
	//return IRQ_HANDLED;
}

#if 0
static void spmp_udc_switch_buffer( void )
{
	udc_write( 0x2, UDC_EP12PPC_OFST);
}

static void spmp_udc_auto_switch_en( int en) {

	if ( en) {
		/*Turn On Auto Switching.*/
		//printk("Turn On Auto Switch[%x][%x][%x]\n", udc_read(UDC_EP12C_OFST), udc_read(UDC_IE_OFST), udc_read(UDC_IF_OFST));
		udc_write(udc_read(UDC_EP12PPC_OFST) | EP12PPC_AUTO_SW_EN ,UDC_EP12PPC_OFST);
	}
	else{
		/*Turn Off Auto Switching.*/
		//printk("Turn Off Auto Switch[%x][%x][%x]\n", udc_read(UDC_EP12C_OFST), udc_read(UDC_IE_OFST), udc_read(UDC_IF_OFST));
		udc_write(udc_read(UDC_EP12PPC_OFST) & (~EP12PPC_AUTO_SW_EN) ,UDC_EP12PPC_OFST);
	}
}
#endif
static int irqen = 0;
static int irqstate = 0;
static irqreturn_t spmp_udc_irq(int dummy, void *_dev)
{
	struct spmp_udc *dev = _dev;
	//int usb_status;
	int pwr_reg = 0;
	int ep0csr = 0;
	u32 udc_irq_flags, udc_irq_en1, udc_irq_en2;

	/* ========== force disconnect  interrupt ======== */
	udc_irq_flags = udc_read(UDC_IF_OFST);
	udc_irq_en1 = udc_read(UDC_IE_OFST);
	udc_irq_en2 = udc_read(UDC_LLCIE_OFST);
	//printk(KERN_ERR "IRQ[%x][%x][%x]\n", udc_irq_flags, udc_irq_en1, udc_read(UDC_LLCIE_OFST) & udc_read(UDC_LLCIF_OFST));
	/*UDC_IF_OFST [0:23] is the same with UDC_LLCIF_OFST. But we should write UDC_LLCIF_OFST for clear. 
	  allenchang 20110506*/
	//udc_irq_flags = udc_read(UDC_LLCIF_OFST);
	if(udc_irq_flags & udc_irq_en1 & CIS_FDISCONN_IF )
	{
		//printk(KERN_ERR "CIS_FDISCONN_IF\n");
		udc_write(CIS_FDISCONN_IF, UDC_IF_OFST);  
	}
    /* ==========  force connect  interrupt ========= */
	else if(udc_irq_flags & udc_irq_en1 &  CIS_FCONN_IF)
	{
		//printk(KERN_ERR "CIS_FCONN_IF\n");
		udc_write(CIS_FCONN_IF, UDC_IF_OFST);  
	}
	else if(udc_irq_flags & udc_irq_en1 &  CIS_VBUS_IF)
	{
		/*Clear IRQ status*/
		udc_write(CIS_VBUS_IF, UDC_IF_OFST);
  		/*Software connect or disconnect*/
		vbusIntr2_UDC(dev);
	}
	else if (udc_irq_flags & udc_irq_en1 &  CIS_DMA_IF)
	{
		#if 0
		udc_write(CIS_DMA_IF, UDC_IF_OFST);  //clear DMA IF
		dma_done = 1;
	    wake_up(&udc_dma_done);
		#endif
		printk("UDC irq DMA\n");
		udc_write(CIS_DMA_IF, UDC_IF_OFST);  //clear DMA IF
		udc_write(UDLC_DMA_IE, UDC_LLCIF_OFST);  //clear DMA IF
		dma_done = 1;
		up(&sem_dma);
   	}
	else if( udc_irq_flags & udc_irq_en2 & UDLC_HSS_IE)
	{
		udc_write(UDLC_HSS_IE, UDC_LLCIF_OFST); 
		printk(KERN_ERR "UDC Set Stall\n");
	}
	else if( udc_irq_flags & udc_irq_en2 & UDLC_SCONF_IE)
	{
		udc_write(UDLC_SCONF_IE, UDC_LLCIF_OFST); 
		//printk(KERN_ERR "UDC Set Configuration\n");
		//spmp_udc_handle_ep0s(dev);
		//dev->driver->resume(&dev->gadget);
	}
    // ========== RESET end  interrupt ============
	else if (udc_irq_flags & udc_irq_en2 & UDLC_RESETN_IE)
	{
		udc_write(UDLC_RESETN_IE, UDC_LLCIF_OFST); 
		//printk(KERN_ERR "UDC Reset Release\n");
	}

	//========== SUSPEND interrupt ============
	else if (udc_irq_flags & udc_irq_en2 & UDLC_SUSPEND_IE)
	{
		//printk(KERN_ERR "UDC Suspent Event\n");
		udc_write(UDLC_SUSPEND_IE, UDC_LLCIF_OFST); 
		dev->driver->suspend(&dev->gadget);
	}
	//========== RESET interrupt =================
	else if (udc_irq_flags & udc_irq_en2 & UDLC_RESET_IE)
	{
		/* two kind of reset :
		 * - reset start -> pwr reg = 8
		 * - reset end   -> pwr reg = 0
		 **/
		//printk("UDC Reset [%x][%x][%x][%d][%x]\n", udc_read(UDC_LLCFS_OFST), udc_read(UDC_EP12C_OFST), 
		//	   udc_read(UDC_EP12FS_OFST), usbState, spmp_udc_fifo_count_ep12());
		isResetInt = 1;
		mscReset = 0;
		ep1SetHalt = 0;
		ep2SetHalt = 0;
		udc_write(UDLC_RESET_IE, UDC_LLCIF_OFST); 
		dprintk(DEBUG_NORMAL, "USB reset csr %x pwr %x\n",
			ep0csr, pwr_reg);
#if 0
		dev->gadget.speed = USB_SPEED_UNKNOWN;
		dev->address = 0;
		dev->ep0state = EP0_IDLE;
#endif
		/*Allow LNK to suspend PHY*/
		udc_write(udc_read(UDC_LLCSET0_OFST) | LCSET0_PWR_SUSP_N , UDC_LLCSET0_OFST);

		clearHwState_UDC(0);
		//enable_irq(IRQ_USB_DEV);
		//return IRQ_HANDLED; //ymlin 0506
	}
   // ========== RESUME interrupt ============
	/* RESUME */
	else if (udc_irq_flags & udc_irq_en2 & UDLC_RESUME_IE) {
		dprintk(DEBUG_NORMAL, "USB resume\n");
		udc_write(UDLC_RESUME_IE, UDC_LLCIF_OFST); 
		//printk(KERN_ERR "UDLC_RESUME_IE\n");
		if (dev->gadget.speed != USB_SPEED_UNKNOWN
				&& dev->driver
				&& dev->driver->resume)
		dev->driver->resume(&dev->gadget);

		udc_write(udc_read(UDC_LLCSET0_OFST) | LCSET0_PWR_SUSP_N , UDC_LLCSET0_OFST);
		clearHwState_UDC(0);
	}
   // ========== FLG_EP0_SETUP interrupt ============
	else if ((udc_irq_flags & udc_irq_en2 & UDLC_EP0S_IE))
	{
		//printk("EP0 setup int\n");
		udc_write(UDLC_EP0S_IE, UDC_LLCIF_OFST); 
		mscReset = 1;
		isResetInt = 0;
		set_first_setup(1);
		dev->ep0state=EP0_IDLE;
		spmp_udc_handle_ep0s(dev);
	}
    // ========== FLG_EP0_IN interrupt ============
	else if ((udc_irq_flags & udc_irq_en2 & UDLC_EP0I_IE))// status stage
	{
		//printk(KERN_ERR "UDLC_EP0I_IE\n");
		udc_write(UDLC_EP0I_IE, UDC_LLCIF_OFST); 
        //dprintk(DEBUG_NORMAL, "FLG_EP0_IN interrupt\n");
	}
    // ========== FLG_EP0_OUT interrupt ============
	else if((udc_irq_flags & udc_irq_en2 & UDLC_EP0O_IE))
	{	// OUT
		//printk(KERN_ERR "UDLC_EP0O_IE\n");
        //dprintk(DEBUG_NORMAL, "UDLC_EP0O_IE interrupt\n");
		udc_write(UDLC_EP0O_IE, UDC_LLCIF_OFST); 
		dev->ep0state = EP0_IDLE;
		udc_write(udc_read(UDC_EP0CS_OFST) |EP0CS_CLR_EP0_OVLD,  UDC_EP0CS_OFST);
	}
	
	// ========== Bulk In/Out Aak interrupt ============
	else if(udc_irq_flags & udc_irq_en2 & (UDLC_EP1I_IE | UDLC_EP2O_IE))
	{
		//printk("[%x][%x]\n", (udc_irq_flags & udc_irq_en2 & UDLC_EP1I_IE), (udc_irq_flags & udc_irq_en2 & UDLC_EP2O_IE));
		spmp_udc_bulk_event(0);
		/*Todo: Fixme*/
		if( spmp_udc_fifo_count_ep12() == 31 || spmp_udc_fifo_count_ep12() == 30 || spmp_udc_fifo_count_ep12() == 32 ) {
			cbwLength = spmp_udc_fifo_count_ep12();
			//printk("EP1/2 int 31[%d][%x]\n", spmp_udc_fifo_count_ep12(), udc_read(UDC_EP12PPC_OFST));
			udc_write(udc_read(UDC_LLCIE_OFST) & (~UDLC_EP2O_IE) , UDC_LLCIE_OFST);
			spmp_udc_read_packet_cbwonly(UDC_EP12FDP_OFST, cbwBuf, cbwLength);
			if( cbwBuf[0] != 0x55 || cbwLength == 30 || cbwLength == 32 ) {
				/*For invalid CBW, all bulk endpoint should return STALL in USB MASS STORAGE spec until MSC reset.*/
				//printk("Invalid CBW\n");
				ep1SetHalt = 1;
				ep2SetHalt = 1;
			}
			else{
				/*Todo: Fix me*/
				isResetInt = 0;
			}
			/*Todo: Remove me. For 15 command.*/
			if( cbwBuf[15] == 0x15 ) {
				udc_write(0,  UDC_EP12C_OFST);
			}
			//printk("[%x]\n", cbwBuf[15]);
			up(&sem);
			/*Clear again for avoid next irq*/
			/*Write Tag to CSW In*/
			//udc_write( 0x01, UDC_MSTC_OFST);
			//printk("[%d]CSW cnt[%d]\n", __LINE__, (udc_read(UDC_EP1SCS_OFST)&0xf0)>>4);
		}
		else{
			//printk("EP1/2 int [%d][%x]\n", spmp_udc_fifo_count_ep12(), udc_read(UDC_EP12PPC_OFST));
		}
	}
	// ========== Bulk In/Out Nak interrupt ============
	else if(udc_irq_flags & udc_irq_en2 & (UDLC_EP1N_IE | UDLC_EP2N_IE))
	{
		//printk("EP1/2 Nak int\n");
		spmp_udc_bulk_event(1);
	}
	else if( udc_irq_flags & udc_irq_en2 & UDLC_HCS_IE)
	{
		//printk("Clear Stall1 [%x]\n", udc_read(UDC_LLCSTL_OFST));
		/*Todo: Fix me. For USB IF MSC test. Error Recovery Test*/
		if( ep1SetHalt || ep2SetHalt ) {
			udc_write((udc_read(UDC_LLCSTL_OFST) | LCSTL_SETEP1STL | LCSTL_SETEP2STL) , UDC_LLCSTL_OFST); // error send stall;
		}
		udc_write(UDLC_HCS_IF, UDC_LLCIF_OFST);
	}
	else{
		/*Todo: Fix me: unknown IRQ! Atto bench could reproduce it easily.*/
		printk("Fix me later[%x][%x], old[%x]now[%x]olden[%x]now[%x][%x]\n\n\n", 
			   udc_irq_flags & udc_irq_en1,  udc_irq_flags & udc_irq_en2, 
			   irqstate, udc_irq_flags, irqen, udc_irq_en1, udc_irq_en2);
		//udc_write(udc_irq_flags, UDC_IF_OFST);
		irqstate = udc_irq_flags;
		irqen = udc_irq_en2;
		return IRQ_NONE;
	}
	irqstate = udc_irq_flags;
	irqen = udc_irq_en2;
	return IRQ_HANDLED;
}

/*------------------------- spmp_ep_ops ----------------------------------*/

static inline struct spmp_ep *to_spmp_ep(struct usb_ep *ep)
{
	return container_of(ep, struct spmp_ep, ep);
}

static inline struct spmp_udc *to_spmp_udc(struct usb_gadget *gadget)
{
	return container_of(gadget, struct spmp_udc, gadget);
}

static inline struct spmp_request *to_spmp_req(struct usb_request *req)
{
	return container_of(req, struct spmp_request, req);
}

/*
 *	spmp_udc_ep_enable
 */
static int spmp_udc_ep_enable(struct usb_ep *_ep,
				 const struct usb_endpoint_descriptor *desc)
{
	struct spmp_udc	*dev;
	struct spmp_ep	*ep;
	u32			max, tmp;
	u32			int_en_reg;

	ep = to_spmp_ep(_ep);

	if (!_ep || !desc || ep->desc
			|| _ep->name == ep0name
			|| desc->bDescriptorType != USB_DT_ENDPOINT)
		return -EINVAL;

	dev = ep->dev;
	if (!dev->driver || dev->gadget.speed == USB_SPEED_UNKNOWN)
		return -ESHUTDOWN;

	max = le16_to_cpu(desc->wMaxPacketSize) & 0x1fff;

	_ep->maxpacket = max & 0x7ff;
	ep->desc = desc;
	ep->halted = 0;
	ep->bEndpointAddress = desc->bEndpointAddress;

	#if 1
   int_en_reg = udc_read(UDC_LLCIE_OFST);
	switch (ep->num)
	{
    	   case 0:
       	/* enable irqs */
       	int_en_reg |= UDLC_EP0I_IE |UDLC_EP0O_IE | UDLC_EP0S_IE;
			 	break;
		   case 1:
         	/* enable irqs */
       	int_en_reg |= UDLC_EP1I_IE;
 			 	break;
    	   case 2:
	 		/* enable irqs */
        int_en_reg |= UDLC_EP2O_IE;
			 	break;
		   case 3:
        	/* enable irqs */
       	int_en_reg |= UDLC_EP3I_IE;
			 	break;
		default:
			return -EINVAL;
	}

   udc_write(int_en_reg, UDC_LLCIE_OFST);
   #endif
	/* print some debug message */
	tmp = desc->bEndpointAddress;
	dprintk (DEBUG_NORMAL, "enable %s(%d) ep%x%s-blk max %02x\n",
		 _ep->name,ep->num, tmp,
		 desc->bEndpointAddress & USB_DIR_IN ? "in" : "out", max);

	spmp_udc_set_halt(_ep, 0);
	
	return 0;
}

/*
 * spmp_udc_ep_disable
 */
static int spmp_udc_ep_disable(struct usb_ep *_ep)
{
	struct spmp_ep *ep = to_spmp_ep(_ep);
	u32 int_en_reg;

	if (!_ep || !ep->desc) {
		dprintk(DEBUG_NORMAL, "%s not enabled\n",
			_ep ? ep->ep.name : NULL);
		return -EINVAL;
	}


	dprintk(DEBUG_NORMAL, "ep_disable: %s\n", _ep->name);

	ep->desc = NULL;
	ep->halted = 1;

	spmp_udc_nuke (ep->dev, ep, -ESHUTDOWN);

	/* disable irqs */
   int_en_reg = udc_read(UDC_LLCIE_OFST);
	switch (ep->num)
	{
    	   case 0:
       	/* enable irqs */
       	int_en_reg &= ~(UDLC_EP0I_IE |UDLC_EP0O_IE | UDLC_EP0S_IE);
			 	break;
		   case 1:
         	/* enable irqs */
       	int_en_reg &= ~(UDLC_EP1I_IE);
 			 	break;
    	   case 2:
	 		/* enable irqs */
        int_en_reg &= ~(UDLC_EP2O_IE);
			 	break;
		   case 3:
        	/* enable irqs */
       	int_en_reg &= ~(UDLC_EP3I_IE);
			 	break;
		default:
				return -EINVAL;
	}
   udc_write(int_en_reg, UDC_LLCIE_OFST);

	dprintk(DEBUG_NORMAL, "%s disabled\n", _ep->name);

	return 0;
}

/*
 * spmp_udc_alloc_request
 */
static struct usb_request *
spmp_udc_alloc_request(struct usb_ep *_ep, gfp_t mem_flags)
{
	struct spmp_request *req;

	dprintk(DEBUG_VERBOSE,"%s(%p,%d)\n", __func__, _ep, mem_flags);

	if (!_ep)
		return NULL;

	req = kzalloc (sizeof(struct spmp_request), mem_flags);
	if (!req)
		return NULL;

	req->req.dma = DMA_ADDR_INVALID;
	INIT_LIST_HEAD (&req->queue);
	return &req->req;
}

/*
 * spmp_udc_free_request
 */
static void
spmp_udc_free_request(struct usb_ep *_ep, struct usb_request *_req)
{
	struct spmp_ep	*ep = to_spmp_ep(_ep);
	struct spmp_request	*req = to_spmp_req(_req);

	dprintk(DEBUG_VERBOSE, "%s(%p,%p)\n", __func__, _ep, _req);

	if (!ep || !_req || (!ep->desc && _ep->name != ep0name))
		return;

	WARN_ON (!list_empty (&req->queue));
	kfree(req);
}

/*
 *	spmp_udc_queue
 */
static int spmp_udc_queue(struct usb_ep *_ep, struct usb_request *_req,
		gfp_t gfp_flags)
{
	struct spmp_request	*req = to_spmp_req(_req);
	struct spmp_ep	*ep = to_spmp_ep(_ep);
	struct spmp_udc	*dev;
	u32			ret = FAIL;
	u32			ep_csr = 0;
	int			fifo_count = 0;
	
	//unsigned long		flags;

	if (unlikely (!_ep || (!ep->desc && ep->ep.name != ep0name))) {
		printk("%s: invalid args\n", __func__);
		return -EINVAL;
	}

	dev = ep->dev;
	if (unlikely (!dev->driver
			|| dev->gadget.speed == USB_SPEED_UNKNOWN)) {
		printk("speed unknow\n");
		return -ESHUTDOWN;
	}

	if (unlikely(!_req || !_req->complete
			|| !_req->buf || !list_empty(&req->queue))) {
		if (!_req)
			printk("%s: 1 X X X\n", __func__);
		else {
			printk("%s: 0 %01d %01d %01d\n",
				__func__, !_req->complete,!_req->buf,
				!list_empty(&req->queue));
		}

		return -EINVAL;
	}

	_req->status = -EINPROGRESS;
	_req->actual = 0;

	//printk("%s: ep%x len %d\n", __func__, ep->bEndpointAddress, _req->length);

	//printk("%d, %d\n", ep->bEndpointAddress & 0x7f, _req->length);
	isResetInt = 0;
	//if( _req->length == 512 ) {
	//	printk("%d, %d, %d, %d, [%x]\n", ep->bEndpointAddress & 0x7f, _req->length, _req->actual, usbState, udc_read(UDC_EP12C_OFST));
	//}
	if (ep->bEndpointAddress) {
       switch (ep->bEndpointAddress & 0x7F)
       {
		   case 1:
     	   case 2:
   		       fifo_count = spmp_udc_fifo_count_ep12();
			 //  printk("fifo_count_ep12=%d\n",fifo_count);
              break;
		   case 3:
   		       fifo_count = spmp_udc_fifo_count_ep3();
			   dprintk(DEBUG_NORMAL, "spmp_udc_fifo_count_ep3=%d\n",fifo_count);
			   break;
		   default:
			   return -EINVAL;
       }
	}
	else {
		fifo_count = spmp_udc_fifo_count_ep0();
		dprintk(DEBUG_NORMAL, "spmp_udc_fifo_count_ep0=%d \n",fifo_count);
	}

	/* kickstart this i/o queue? */

	#if 1//ymlin for card remove
	if (list_empty(&ep->queue)/* && !ep->halted*/) {
		#else
if (list_empty(&ep->queue) && !ep->halted) {		
		#endif
		if (ep->bEndpointAddress == 0 /* ep0 */) {
			isResetInt = 0;

			ep_csr = udc_read(UDC_EP0CS_OFST);
			switch (dev->ep0state) {
			case EP0_IN_DATA_PHASE:
				if (!(ep_csr& EP0CS_IVLD)
						&& spmp_udc_write_ep0_fifo(ep,
							req)) {
					dev->ep0state = EP0_IDLE;
					ret = SUCCESS;
				}
				break;

			case EP0_OUT_DATA_PHASE:
				if ((!_req->length)
					|| ((ep_csr & EP0CS_OVLD)
						&& spmp_udc_read_ep0_fifo(ep,
							req))) {
					dev->ep0state = EP0_IDLE;
					ret = SUCCESS;
				}
				break;

			default:
				return -EL2HLT;
			}
			req = NULL;
		}
		else if ((ep->bEndpointAddress & USB_DIR_IN) != 0)
		{
		   switch (ep->bEndpointAddress & 0x7F)
		   {
		      case 1:
				  ep_csr = udc_read(UDC_EP12C_OFST);
				  /*The message means the gadget driver sperates data to two parts.*/
				  if( usbState == USB_STATE_BULKIN && req->req.length != 13 ) {
					  //printk("[%x][%d][%d]\n", udc_read(UDC_EP12FS_OFST), spmp_udc_fifo_count_ep12(), req->req.length);
				  }
				  usbState = USB_STATE_BULKIN;
				  if( usbReadLength != 0 ) {
					  usbReadLength -= req->req.length;
				  }
				  if(!(ep_csr& EP12C_EP1_IVLD)){
					 if( req->req.length >= 512 /*&& usbReadLength == 0*/ ) {
						 if(spmp_udc_bulkin_ep1_dma(ep, req)){
							 ret = SUCCESS;
						 }
						 else{
							 printk("Ep1 DMA fail[%x][%d]\n",ep_csr, req->req.length);
						 }
					 }
					 else{
						 if( req->req.length == 13 ) {
							 usbState = USB_STATE_CSW;
						 }
						 if(spmp_udc_bulkin_ep1_fifo(ep, req)) {
							 ret = SUCCESS;
						 }
						 else{
							 printk("Ep1 PIO fail[%x][%d]\n",ep_csr, req->req.length);
						 }
					 }
				  }
				  else{
					  printk("[%s][%d] Why Invalid??\n", __FUNCTION__, __LINE__);
					  return -EINVAL;
				  }
				  break;
			   case 3:
       			ep_csr = udc_read(UDC_EP3CS_OFST);
                 if((!(ep_csr& EP3CS_IVLD)))
                 {
        			   if(spmp_udc_write_ep3_fifo(ep, req)){
						   ret = SUCCESS;
						   req = NULL;
					   }
                 }
				else{
       			    printk("[%s][%d] Why Invalid??\n", __FUNCTION__, __LINE__);
					return -EINVAL;
				 }
				 break;
			   default:
				return -EINVAL;
		   	}
		}
		else {
			if( ep->bEndpointAddress !=2 ){
			   printk("unknow error [%d]\n", fifo_count);
			   return -EINVAL;
			} 
			else {
				//printk("OHOH 330[%x][%d]%d]\n",ep_csr, req->req.length, fifo_count);
				usbState = USB_STATE_BULKOUT;
				if( usbWriteLength != 0 ) {
					usbWriteLength -= req->req.length;
				}
				if( req->req.length >= 512 /*&& usbWriteLength == 0 */) {
					if(spmp_udc_bulkout_ep2_dma(ep, req)){
						ret = SUCCESS;
						req = NULL;
					}
					else{
						ep_csr = udc_read(UDC_EP12C_OFST);
						printk("Ep2 DMA fail[%x][%d]\n",ep_csr, req->req.length);
					}
				}
				else{
					if( req->req.length == 31 && dev->ep0state == EP0_IDLE ){
						mscReset = 0;
					}
					else{
						if( req->req.length == 31 ) {
							printk("Ep0 ing[%d]\n", dev->ep0state);
						}
					}
					if( spmp_udc_bulkout_ep2_fifo(ep, req) ) {
						ret = SUCCESS;
						req = NULL;
					}
					else{
						//printk("Ep2 PIO Fail[%d]\n", req->req.length);
					}
				}
			}
		}
	}
	
	/* If req != NULL. Retry again with pio or dma irq handler advances the queue. */
	if (likely (ret != SUCCESS)){
		//printk("Retry again? EP[%d]Len[%d]\n", ep->bEndpointAddress, req->req.length);
		//list_add_tail(&req->queue, &ep->queue);
	}
	
	return 0;
}

/*
 *	spmp_udc_dequeue
 */
static int spmp_udc_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct spmp_ep	*ep = to_spmp_ep(_ep);
	struct spmp_udc	*udc;
	int			retval = -EINVAL;
	//unsigned long		flags;
	struct spmp_request	*req = NULL;

	dprintk(DEBUG_VERBOSE, "%s(%p,%p)\n", __func__, _ep, _req);

	if (!the_controller->driver)
		return -ESHUTDOWN;

	if (!_ep || !_req)
		return retval;

	udc = to_spmp_udc(ep->gadget);


	list_for_each_entry (req, &ep->queue, queue) {
		if (&req->req == _req) {
			list_del_init (&req->queue);
			_req->status = -ECONNRESET;
			retval = 0;
			break;
		}
	}

	if (retval == 0) {
		dprintk(DEBUG_VERBOSE,
			"dequeued req %p from %s, len %d buf %p\n",
			req, _ep->name, _req->length, _req->buf);

		spmp_udc_done(ep, req, -ECONNRESET);
	}

	return retval;
}

/*
 * spmp_udc_set_halt
 */
static int spmp_udc_set_halt(struct usb_ep *_ep, int value)
{
	struct spmp_ep	*ep = to_spmp_ep(_ep);
	//unsigned long		flags;
	u32			idx;

	if (unlikely (!_ep || (!ep->desc && ep->ep.name != ep0name))) {
		dprintk(DEBUG_NORMAL, "%s: inval 2\n", __func__);
		return -EINVAL;
	}

	//printk("halt, [%x][%x]\n", ep->bEndpointAddress & 0x7F, value);
	#if 0
	if( (ep->bEndpointAddress & 0x7F) == 0x01 ) {
		ep1SetHalt = value;
	}
	else if( (ep->bEndpointAddress & 0x7F) == 0x02 ) {
		ep2SetHalt = value;
	}
	#endif

	idx = ep->bEndpointAddress & 0x7F;

	if (idx == 0) {
		if (value)
			udc_write((udc_read(UDC_LLCSTL_OFST) | LCSTL_SETEP0STL) , UDC_LLCSTL_OFST); // error send stall;
		else
			udc_write((udc_read(UDC_LLCSTL_OFST) | LCSTL_CLREP0STL) , UDC_LLCSTL_OFST); // clear ep0  stall;
	} 
	else {
       if(value){
		   //printk("udc set halt idx=%x val=%x \n",idx,value);
		   switch (ep->bEndpointAddress & 0x7F){
		   case 1:
			 #if 1 //ymlin for card remove
				 //disable ep12
				 //udc_write( 0, UDC_EP12C_OFST); 
				 //udc_write(((udc_read(UDC_EP12C_OFST) & ~(EP12C_SET_EP1_IVLD))/* | EP12C_RESET_PIPO*/) , UDC_EP12C_OFST);				 
				 spmp_udc_ack_en( USB_ACK_EP_BULK_IN, 0 );
                 spmp_udc_nak_en( USB_NAK_EP_BULK_IN, 0);
				 //set ep1 stall
				 //udc_write((udc_read(UDC_EP1SCS_OFST) | EP1SCS_RESET_FIFO) , UDC_EP1SCS_OFST);  
				 udc_write((udc_read(UDC_LLCSTL_OFST) |  LCSTL_SETEP1STL) , UDC_LLCSTL_OFST); // error send stall;
			#else
				 udc_write(((udc_read(UDC_EP12C_OFST) & ~(EP12C_ENABLE_BULK))) , UDC_EP12C_OFST); 
				 udc_write(((udc_read(UDC_EP12C_OFST) & ~(EP12C_SET_EP1_IVLD)) | EP12C_RESET_PIPO) , UDC_EP12C_OFST);
             #endif
				 break;
		   case 2:
			   //udc_write( 0, UDC_EP12C_OFST); 

			   //printk("320[%x]330[%x]364[%x]\n", udc_read(UDC_LLCFS_OFST),  udc_read(UDC_EP12C_OFST),  udc_read(UDC_EP12FS_OFST));
			   spmp_udc_ack_en( USB_ACK_EP_BULK_OUT, 0);
			   spmp_udc_nak_en( USB_NAK_EP_BULK_OUT, 0);
			   //udc_write((udc_read(UDC_EP1SCS_OFST) | EP1SCS_RESET_FIFO) , UDC_EP1SCS_OFST);        
			   udc_write( LCSTL_SETEP2STL , UDC_LLCSTL_OFST); // error send stall;
				
			   //printk("320[%x]330[%x]364[%x]\n", udc_read(UDC_LLCFS_OFST),  udc_read(UDC_EP12C_OFST),  udc_read(UDC_EP12FS_OFST));
				//udc_write((udc_read(UDC_EP12C_OFST) | EP12C_CLR_EP2_OVLD |EP12C_RESET_PIPO) , UDC_EP12C_OFST);
				break;
		   case 3:
				udc_write((udc_read(UDC_LLCSTL_OFST) | LCSTL_SETEP3STL) , UDC_LLCSTL_OFST); // error send stall;
				//Todo :Fix me  udc_write((udc_read(UDC_EP3CS_OFST) | EP3CS_CLR_IVLD) , UDC_EP12C_OFST);
				break;
		   default:
				return -EINVAL;
			}
	   }
	   else{
		   switch (ep->bEndpointAddress & 0x7F){
		   case 1:
			   if( ep1SetHalt == 0 ) {
				   udc_write((udc_read(UDC_LLCSTL_OFST) | LCSTL_CLREP1STL) , UDC_LLCSTL_OFST); // error send stall;
			   }
            // udc_write(((udc_read(UDC_EP12C_OFST) & ~(EP12C_SET_EP1_IVLD)) |EP12C_RESET_PIPO) , UDC_EP12C_OFST);
             //udc_write((udc_read(UDC_EP12C_OFST) | EP12C_CLR_EP2_OVLD |EP12C_RESET_PIPO) , UDC_EP12C_OFST);        
			 udc_write(0, UDC_EP12C_OFST);         
             //udc_write((udc_read(UDC_EP1SCS_OFST) | EP1SCS_RESET_FIFO) , UDC_EP1SCS_OFST);               
             //udc_write(((udc_read(UDC_EP12C_OFST) | EP12C_ENABLE_BULK)) , UDC_EP12C_OFST); 
			 spmp_udc_nak_en( USB_NAK_EP_BULK_IN, 1);
             break;
		   case 2:
			 if( ep2SetHalt == 0 ) {
				udc_write((udc_read(UDC_LLCSTL_OFST) | LCSTL_CLREP2STL) , UDC_LLCSTL_OFST); // error send stall;
			 }
			 //udc_write(((udc_read(UDC_EP12C_OFST) & ~(EP12C_SET_EP1_IVLD)) |EP12C_RESET_PIPO) , UDC_EP12C_OFST);              
             //udc_write((udc_read(UDC_EP12C_OFST) | EP12C_CLR_EP2_OVLD |EP12C_RESET_PIPO) , UDC_EP12C_OFST);
			 udc_write(0, UDC_EP12C_OFST);
             //udc_write((udc_read(UDC_EP1SCS_OFST) | EP1SCS_RESET_FIFO) , UDC_EP1SCS_OFST);               
             //udc_write(((udc_read(UDC_EP12C_OFST) | EP12C_ENABLE_BULK)) , UDC_EP12C_OFST); 
			 spmp_udc_nak_en( USB_NAK_EP_BULK_OUT, 1);
              break;
		   case 3:
             udc_write((udc_read(UDC_LLCSTL_OFST) | LCSTL_CLREP3STL) , UDC_LLCSTL_OFST); // error send stall;
             udc_write((udc_read(UDC_EP3CS_OFST) | EP3CS_CLR_IVLD) , UDC_EP12C_OFST);
			   break;
			 default:
				return -EINVAL;
		   }
	   }
	}
	
	ep->halted = value ? 1 : 0;
	return 0;
}

static const struct usb_ep_ops spmp_ep_ops = {
	.enable		= spmp_udc_ep_enable,
	.disable	= spmp_udc_ep_disable,

	.alloc_request	= spmp_udc_alloc_request,
	.free_request	= spmp_udc_free_request,

	.queue		= spmp_udc_queue,
	.dequeue	= spmp_udc_dequeue,
	
	.set_halt	= spmp_udc_set_halt,
};

/*------------------------- usb_gadget_ops ----------------------------------*/

/*
 *	spmp_udc_get_frame
 */
static int spmp_udc_get_frame(struct usb_gadget *_gadget)
{
	return -EOPNOTSUPP;
}

/*
 *	spmp_udc_wakeup
 */
static int spmp_udc_wakeup(struct usb_gadget *_gadget)
{
	dprintk(DEBUG_NORMAL, "%s()\n", __func__);
	return -EOPNOTSUPP;
}

/*
 *	spmp_udc_set_selfpowered
 */
static int spmp_udc_set_selfpowered(struct usb_gadget *gadget, int value)
{

	return -EOPNOTSUPP;
}

static void spmp_udc_disable(struct spmp_udc *dev);
static void spmp_udc_enable(struct spmp_udc *dev);

#if 0
static int spmp_udc_set_pullup(struct spmp_udc *udc, int is_on)
{

	return -EOPNOTSUPP;
}
#endif

static int spmp_udc_vbus_session(struct usb_gadget *gadget, int is_active)
{

	return -EOPNOTSUPP;
}

static int spmp_udc_pullup(struct usb_gadget *gadget, int is_on)
{
	return -EOPNOTSUPP;
}

static int spmp_vbus_draw(struct usb_gadget *_gadget, unsigned ma)
{
	return -EOPNOTSUPP;
}

static const struct usb_gadget_ops spmp_ops = {
	.get_frame		= spmp_udc_get_frame,
	.wakeup			= spmp_udc_wakeup,
	.set_selfpowered	= spmp_udc_set_selfpowered,
	.pullup			= spmp_udc_pullup,
	.vbus_session		= spmp_udc_vbus_session,
	.vbus_draw		= spmp_vbus_draw,
};

/*------------------------- gadget driver handling---------------------------*/
/*
 * spmp_udc_disable
 */
static void spmp_udc_disable(struct spmp_udc *dev)
{
    int tmp;
	dprintk(DEBUG_NORMAL, "%s()\n", __func__);
#define USBPHY_XTAL_ENABLE             (1 << 2)
#define USBPHY1_POWER_CTRL             (1 <<12)
	/* Disable all interrupts */
{
	unsigned int tmp;
	tmp = SCUA_USBPHY_CFG | USBPHY_XTAL_ENABLE;
	tmp= tmp & (~(USBPHY1_POWER_CTRL));
	SCUA_USBPHY_CFG = tmp;

	//printk(KERN_ERR "SCUA_USBPHY_CFG is %08x\n",SCUA_USBPHY_CFG);
	//	printf(" scu phy=%0x \n", *rUSB_PHY_BASE_UDC);
	while((SCUA_USBPHY_CFG) & (USBPHY1_POWER_CTRL)){
		dprintk(DEBUG_NORMAL, ".");
		if(!spmp_vbus_detect())
				break;
	}

	/*Disable Software Connection*/
	printk("SW Disconnect\n");
	gpHalUsbSlaveSwConnect(0);
	/*Disable Vbus Interrupt*/
	spmp_udc_irq_config_en(0);

	//enable PWREN_CPU1 in SCUB
	SCUB_PWRC_CFG = SCUB_PWRC_CFG | 0x01;
	//printk(KERN_ERR "SCUB_PWRC_CFG is %08x\n",SCUB_PWRC_CFG);
}
   dprintk(DEBUG_NORMAL, "spmp_udc_disable called 1=%08x %08x\n",udc_read(UDC_LLCS_OFST),udc_read(UDC_LLCSET0_OFST));
   tmp = udc_read(UDC_LLCSET0_OFST);
   udc_write(tmp | LCSET0_SOFT_DISC, UDC_LLCSET0_OFST);  //SET_SOFT_DISCON();
   udc_write(tmp | LCSET0_PWR_SUSP_N, UDC_LLCSET0_OFST);  //SET_PWR_SUSPEND();
   udc_write(0x0, UDC_LLCIE_OFST);  //DIS_ALL_USB_IRQ();
   udc_write(0x0, UDC_IE_OFST); 	//DIS DAM & VBUS IRQ();
   udc_write(0x0, UDC_LLCSET2_OFST);
   udc_write(LCSET0_DISC_SUSP_EN | LCSET0_CPU_WKUP_EN |
	 	        LCSET0_PWR_SUSP_N | LCSET0_SOFT_DISC
	 	        , UDC_LLCSET0_OFST);

   udc_write(LCSET0_SELF_POWER, UDC_LLCSET1_OFST);

   udc_write(EP0CS_CLR_EP0_OVLD, UDC_EP0CS_OFST);

   udc_write(EP12C_RESET_PIPO, UDC_EP12C_OFST);

   tmp = udc_read(UDC_LLCSTL_OFST);
   udc_write(tmp | (LCSTL_CLREP2STL | LCSTL_CLREP1STL | LCSTL_CLREP0STL) ,
	 	       UDC_LLCSTL_OFST);

   udc_write(0, UDC_EP0DC_OFST);
   udc_write(udc_read(UDC_EP12C_OFST) | EP12C_RESET_PIPO, UDC_EP12C_OFST);
	//====clear flags before enable interrupt
    udc_write(0xFFFFFFFF, UDC_LLCIF_OFST);
    udc_write(0xFFFFFFFF, UDC_IF_OFST);//UDC_CIS_OFST);

	/* Good bye, cruel world */
//	if (udc_info && udc_info->udc_command)
//		udc_info->udc_command(spmp_UDC_P_DISABLE);

	/* Set speed to unknown */
	dprintk(DEBUG_NORMAL, "spmp_udc_disable called 3=%08x %08x\n",udc_read(UDC_LLCS_OFST),udc_read(UDC_LLCSET0_OFST));
	dev->gadget.speed = USB_SPEED_UNKNOWN;

	printk("Disable USB1 CLK\n");
	gpHalScuClkEnable(SCU_A_PERI_USB1, SCU_A, 0);

}

/*
 * spmp_udc_reinit
 */
static void spmp_udc_reinit(struct spmp_udc *dev)
{
	u32 i;

	/* device/ep0 records init */
	INIT_LIST_HEAD (&dev->gadget.ep_list);
	INIT_LIST_HEAD (&dev->gadget.ep0->ep_list);
	dev->ep0state = EP0_IDLE;

	for (i = 0; i < SPMP_MAXENDPOINTS; i++) {
		struct spmp_ep *ep = &dev->ep[i];

		if (i != 0)
			list_add_tail (&ep->ep.ep_list, &dev->gadget.ep_list);

		ep->dev = dev;
		ep->desc = NULL;
		ep->halted = 0;
		INIT_LIST_HEAD (&ep->queue);
	}
}

/**
 * @brief   USB Slave VBUS detect
 * @param   void
 * @return  1 connect, 0 deisconnect
 */
unsigned int spmp_vbus_detect( void ) {
	unsigned int value = 0;

	if (vbus_config == USB_SLAVE_VBUS_NONE) {
		/*always connect*/
		return 1;
	}
	else {
		if( gpioHandle ) {
			gp_gpio_get_value(gpioHandle, &value);
		}
		else {
			value = IS_USBHOST_PRESENT();
		}
	}
	return value;
}
EXPORT_SYMBOL(spmp_vbus_detect);

/**
 * @brief   USB Slave VBUS GPIO interrupt config
 * @param   en [IN]: enable
 * @param   configIndex [IN]: gpio index
 * @return  void
 */
void spmp_vbus_gpio_config ( struct spmp_udc *udc, unsigned int en, unsigned int configIndex )
{
	int ret;
	//gpio_content_t ctx;
	//int cbk_id;
	unsigned int value = 0;
	if( en ) {
		//printk("[%s][%d]ENABLE\n", __FUNCTION__, __LINE__);
		gpioHandle = gp_gpio_request( configIndex, (char *)gadget_name );
		//printk("[%s][%d]Handle[%x]Channel[%x]\n", __FUNCTION__, __LINE__, gpioHandle, (configIndex && 0xff0000) >> 24 );
		//cbk_id = gp_gpio_request_irq( (configIndex & 0xff000000)>> 24, (char*)gadget_name, spmp_udc_vbus_gpio_irq, NULL);
		ret = gp_gpio_set_input(gpioHandle, GPIO_PULL_LOW);
		ret = gp_gpio_get_value(gpioHandle, &value);
		printk("VBUS[%x]\n", value);
		//gp_gpio_enable_irq(gpioHandle, 1);
		gp_gpio_register_isr(gpioHandle, spmp_udc_vbus_gpio_irq, udc);
	}
	else {
		//printk("[%s][%d]DISABLE, [%x]\n", __FUNCTION__, __LINE__, gpioHandle);
		if( gpioHandle != 0 ) {
			//ret = gp_gpio_release_irq( configIndex, gpioHandle );
			ret = gp_gpio_unregister_isr(gpioHandle);
			ret = gp_gpio_release( gpioHandle );
			gpioHandle = 0;
		}
	}

}


/*
 *	usb_gadget_register_driver
 */
int usb_gadget_register_driver(struct usb_gadget_driver *driver)
{
	struct spmp_udc *udc = the_controller;
	int		retval;

	dprintk(DEBUG_NORMAL, "usb_gadget_register_driver() '%s'\n",
		driver->driver.name);
    dprintk(DEBUG_NORMAL, "usb_gadget_register_driver() '%s'\n",
		driver->driver.name);
	/* Sanity checks */
	if (!udc)
		return -ENODEV;

	if (udc->driver)
		return -EBUSY;

	if (!driver->bind || !driver->setup
			|| driver->speed < USB_SPEED_FULL) {
		printk(KERN_ERR "Invalid driver: bind %p setup %p speed %d\n",
			driver->bind, driver->setup, driver->speed);
		return -EINVAL;
	}
#if defined(MODULE)
	if (!driver->unbind) {
		printk(KERN_ERR "Invalid driver: no unbind method\n");
		return -EINVAL;
	}
#endif

	/* Hook the driver */
	udc->driver = driver;
	udc->gadget.dev.driver = &driver->driver;

	/* Bind the driver */
	if ((retval = device_add(&udc->gadget.dev)) != 0) {
		printk(KERN_ERR "Error in device_add() : %d\n",retval);
		goto register_error;
	}

	dprintk(DEBUG_NORMAL, "binding gadget driver '%s'\n",
		driver->driver.name);

	if ((retval = driver->bind (&udc->gadget)) != 0) {
		device_del(&udc->gadget.dev);
		goto register_error;
	}

	/* Enable udc */
	spmp_udc_enable(udc);

	return 0;

register_error:
	udc->driver = NULL;
	udc->gadget.dev.driver = NULL;
	return retval;
}

/*
 *	usb_gadget_unregister_driver
 */
int usb_gadget_unregister_driver(struct usb_gadget_driver *driver)
{
	struct spmp_udc *udc = the_controller;

	if (!udc)
		return -ENODEV;

	if (!driver || driver != udc->driver || !driver->unbind)
		return -EINVAL;

	dprintk(DEBUG_NORMAL,"usb_gadget_register_driver() '%s'\n",
		driver->driver.name);

	if (driver->disconnect)
		driver->disconnect(&udc->gadget);

	device_del(&udc->gadget.dev);

	driver->unbind (&udc->gadget);

	udc->driver = NULL;
	/* Disable udc */
	spmp_udc_disable(udc);

	return 0;
}

/*---------------------------------------------------------------------------*/
static struct spmp_udc memory = {
	.gadget = {
		.ops		= &spmp_ops,
		.ep0		= &memory.ep[0].ep,
		.name		= gadget_name,
		.dev = {
			.init_name	= "gadget",
		},
	},

	/* control endpoint */
	.ep[0] = {
		.num		= 0,
		.ep = {
			.name		= ep0name,
			.ops		= &spmp_ep_ops,
			.maxpacket	= EP0_FIFO_SIZE,
		},
		.dev		= &memory,
	},

	/* first group of endpoints */
	.ep[1] = {
		.num		= 1,
		.ep = {
			.name		= "ep1in-bulk",
			.ops		= &spmp_ep_ops,
			.maxpacket	= EP12_FIFO_SIZE64,
		},
		.dev		= &memory,
		.fifo_size	= EP_FIFO_SIZE,
		.bEndpointAddress = USB_DIR_IN | 1,
		.bmAttributes	= USB_ENDPOINT_XFER_BULK,
	},
	.ep[2] = {
		.num		= 2,
		.ep = {
			.name		= "ep2out-bulk",
			.ops		= &spmp_ep_ops,
			.maxpacket	= EP12_FIFO_SIZE64,
		},
		.dev		= &memory,
		.fifo_size	= EP_FIFO_SIZE,
		.bEndpointAddress = 2,
		.bmAttributes	= USB_ENDPOINT_XFER_BULK,
	},

	.ep[3] = {
		.num		= 3,
		.ep = {
			.name		= "ep3in-int",
			.ops		= &spmp_ep_ops,
			.maxpacket	= EP_FIFO_SIZE,
		},
		.dev		= &memory,
		.fifo_size	= EP_FIFO_SIZE,
		.bEndpointAddress = USB_DIR_IN | 3,
		.bmAttributes	= USB_ENDPOINT_XFER_BULK,
	},
	.ep[4] = {
		.num		= 4,
		.ep = {
			.name		= "ep4in-bulk",
			.ops		= &spmp_ep_ops,
			.maxpacket	= EP12_FIFO_SIZE64,
		},
		.dev		= &memory,
		.fifo_size	= EP_FIFO_SIZE,
		.bEndpointAddress = USB_DIR_IN | 4,
		.bmAttributes	= USB_ENDPOINT_XFER_BULK,
	},
	.ep[5] = {
		.num		= 5,
		.ep = {
			.name		= "ep5out-bulk",
			.ops		= &spmp_ep_ops,
			.maxpacket	= EP12_FIFO_SIZE64,
		},
		.dev		= &memory,
		.fifo_size	= EP_FIFO_SIZE,
		.bEndpointAddress = 5,
		.bmAttributes	= USB_ENDPOINT_XFER_BULK,
	},
};

/*
 * spmp_udc_enable
 */
static void spmp_udc_enable(struct spmp_udc *dev)
{
	struct gp_board_usb_s *pConfig = gp_board_get_config("usb", gp_board_usb_t);
	vbus_config = pConfig->get_slave_vbus_config();

	
	printk("Resume PHY\n");
	gpHalScuUsbPhyClkEnable(1);
	gpHalUsbPhyPowerControlSet(1, 0x0);
	printk("Enable SYS_A\n");
	gp_enable_clock((int*)"SYS_A", 1);
	printk("Enable USB1 CLK\n");
	gpHalScuClkEnable(SCU_A_PERI_USB1, SCU_A, 1);
	/*For hardware startup, it needs to sleep 200 ms. allen.chang*/
	//msleep(10);

	if( vbus_config == USB_SLAVE_VBUS_POWERON1 ) {
		udc_write(CIE_DMA_IE , UDC_IE_OFST);
	}
	else if( vbus_config == USB_SLAVE_VBUS_NONE ){
		/*VBUS Interrupt isn't enable.*/
		udc_write((CIE_DMA_IE) , UDC_IE_OFST);
	}
	else {
		/*GPIO Interrupt.*/
		udc_write((CIE_DMA_IE) , UDC_IE_OFST);
	}
	udc_write(UDLC_RESETN_IE | UDLC_RESUME_IE |UDLC_SUSPEND_IE | UDLC_EP2O_IE | UDLC_EP0I_IE |
			  UDLC_EP0O_IE | UDLC_EP0S_IE | UDLC_RESET_IE | UDLC_HCS_IE |
			  UDLC_HSS_IE , UDC_LLCIE_OFST);
	dev->gadget.speed = USB_SPEED_UNKNOWN;

	/*Enable VBUS Interrupt*/
	spmp_udc_irq_config_en(1);
	/*Enable Software Connection*/
	
	if(spmp_vbus_detect()) {
		printk("SW Connect\n");
		gpHalUsbSlaveSwConnect(1);
	}
	dprintk(DEBUG_NORMAL, "spmp_udc_enable called 5\n");
	dprintk(DEBUG_NORMAL, "spmp_udc_enable called6\n");

	printk("INTERRUPT MODE\n");
}

/*
 *	probe - binds to the platform device
 */
static int spmp_udc_probe(struct platform_device *pdev)
{
	struct spmp_udc *udc = &memory;
	struct device *dev = &pdev->dev;
	int retval;

   dprintk(DEBUG_NORMAL, "got and enabled clocks\n");
	spin_lock_init (&udc->lock);
//	udc_info = pdev->dev.platform_data;

	rsrc_start = 0x93006000;
	rsrc_len   = 0x1000;
#if 0
	/*The memory region was requested in device.c*/
	if (!request_mem_region(rsrc_start, rsrc_len, gadget_name))
		return -EBUSY;
#endif
	base_addr = ioremap(rsrc_start, rsrc_len);
	if (!base_addr) {
		retval = -ENOMEM;
		goto err_mem;
	}
	//printk(KERN_ERR "ioremap done\n");

	device_initialize(&udc->gadget.dev);
	udc->gadget.dev.parent = &pdev->dev;
	udc->gadget.dev.dma_mask = pdev->dev.dma_mask;

	the_controller = udc;
	platform_set_drvdata(pdev, udc);

	spmp_udc_reinit(udc);

	udc->vbus = 0;
	if (spmp_udc_debugfs_root) {
		udc->regs_info = debugfs_create_file("registers", S_IRUGO,
				spmp_udc_debugfs_root,
				udc, &spmp_udc_debugfs_fops);
		if (!udc->regs_info)
			dev_warn(dev, "debugfs file creation failed\n");
	}
	sema_init( &sem, 0);
	sema_init( &sem_dma, 0);
	dev_dbg(dev, "probe ok\n");
	return 0;

err_mem:
	release_mem_region(rsrc_start, rsrc_len);

	return retval;
}

/*
 *	spmp_udc_remove
 */
static int spmp_udc_remove(struct platform_device *pdev)
{
	struct spmp_udc *udc = platform_get_drvdata(pdev);
	//unsigned int irq;
	dev_dbg(&pdev->dev, "%s()\n", __func__);
	if (udc->driver)
		return -EBUSY;

	debugfs_remove(udc->regs_info);

	iounmap(base_addr);
	release_mem_region(rsrc_start, rsrc_len);

	platform_set_drvdata(pdev, NULL);

	dev_dbg(&pdev->dev, "%s: remove ok\n", __func__);
	return 0;
}

#ifdef CONFIG_PM
static int spmp_udc_suspend(struct platform_device *pdev, pm_message_t message)
{
//	if (udc_info && udc_info->udc_command)
//		udc_info->udc_command(spmp_UDC_P_DISABLE);

	return 0;
}

static int spmp_udc_resume(struct platform_device *pdev)
{
//	if (udc_info && udc_info->udc_command)
//		udc_info->udc_command(spmp_UDC_P_ENABLE);

	return 0;
}
#else
#define spmp_udc_suspend	NULL
#define spmp_udc_resume	NULL
#endif

void spmp_udc_irq_config_en( int en )
{
	struct spmp_udc *udc = &memory;
	struct gp_board_usb_s *pConfig = gp_board_get_config("usb", gp_board_usb_t);
	int retval  = 0;
	vbus_config = pConfig->get_slave_vbus_config();

	//printk("[%s][%d], [%d][%d]\n", __FUNCTION__, __LINE__, vbus_config, en);
	if( en ) {
		//spmp_udc_disable(udc);
		/* irq setup after old hardware state is cleaned up *///
		retval = request_irq(IRQ_USB_DEV, spmp_udc_irq,
							 IRQF_DISABLED, gadget_name, udc);
		if (retval != 0) {
			printk("Error! Request USB irq Fail[%d]\n", retval);
		}
		if( vbus_config == USB_SLAVE_VBUS_POWERON1 ) {
			
			udc_write((CIE_FDISCONN_IE |CIE_FCONN_IE |  CIE_VBUS_IE) , UDC_IE_OFST);
		}
		else if( vbus_config == USB_SLAVE_VBUS_NONE) {
		}
		else{
			/*GPIO Interrupt. Note:Config is a ramdom value. It's used for GPIO INDEX.*/
			spmp_vbus_gpio_config( udc, 1, vbus_config);
		}
	}
	else{
		if( vbus_config != USB_SLAVE_VBUS_POWERON1 && vbus_config != USB_SLAVE_VBUS_NONE ) {
			spmp_vbus_gpio_config( udc, 0, vbus_config );
		}
		else if(vbus_config == USB_SLAVE_VBUS_POWERON1) {
			
		}
		free_irq(IRQ_USB_DEV, udc);
	}
}
EXPORT_SYMBOL(spmp_udc_irq_config_en);


static struct platform_driver udc_driver_spmp = {
	.driver		= {
		.name	= "spmp-udc",
		.owner	= THIS_MODULE,
	},
	.probe		= spmp_udc_probe,
	.remove		= spmp_udc_remove,
	.suspend	= spmp_udc_suspend,
	.resume		= spmp_udc_resume,
};

static int __init udc_init(void)
{
	int retval;
    dprintk(DEBUG_NORMAL, "udc_init\n");
	dprintk(DEBUG_NORMAL, "%s: version %s\n", gadget_name, DRIVER_VERSION);
	retval = platform_driver_register(&udc_driver_spmp);
	if (retval)
		goto err;

	return 0;

err:
	printk(KERN_ERR " udc_int error! \n");
	debugfs_remove(spmp_udc_debugfs_root);
	return retval;
}

static void __exit udc_exit(void)
{
	platform_driver_unregister(&udc_driver_spmp);
	debugfs_remove(spmp_udc_debugfs_root);
}

EXPORT_SYMBOL(usb_gadget_unregister_driver);
EXPORT_SYMBOL(usb_gadget_register_driver);

module_init(udc_init);
module_exit(udc_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:spmp-usbgadget");
MODULE_ALIAS("platform:spmp-usbgadget");
