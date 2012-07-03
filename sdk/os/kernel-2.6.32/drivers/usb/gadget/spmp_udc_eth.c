/*
 * linux/drivers/usb/gadget/spmp_udc.c
 *
 * Samsung S3C24xx series on-chip full speed USB device controllers
 *
 * Copyright (C) 2004-2007 Herbert Pötzl - Arnaud Patard
 *	Additional cleanups by Ben Dooks <ben-linux@fluff.org>
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


/* TODO 
  1.  notify SETCONFIG INFO to gadget (to see irq)
  2.  dma may be implemented by tasklet that is fine.
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

//#include <plat/regs-udc.h>
//#include <plat/udc.h>


#include "spmp_udc.h"
#include <mach/regs-scu.h>
#include <mach/regs-usbdev.h>
#define DRIVER_DESC	"spmp USB Device Controller Gadget"
#define DRIVER_VERSION	"29 Apr 2007"
#define DRIVER_AUTHOR	"Herbert Pötzl <herbert@13thfloor.at>, " \
			"Arnaud Patard <arnaud.patard@rtp-net.org>"

static const char		gadget_name[] = "spmp_udc";
static const char		driver_desc[] = DRIVER_DESC;

static struct spmp_udc	*the_controller;
static struct clk		*udc_clock;
static struct clk		*usb_bus_clock;
static void __iomem		*base_addr;
static u64			rsrc_start;
static u64			rsrc_len;
static struct dentry		*spmp_udc_debugfs_root;
static int dma_done = 0;
static DECLARE_WAIT_QUEUE_HEAD(udc_dma_done);

static void spmp_udc_handle_ep_out(struct spmp_ep *ep);
static inline u32 udc_read(u32 reg)
{
	return readl(base_addr + reg);
}

static inline void udc_write(u32 value, u32 reg)
{
	writel(value, base_addr + reg);
}


//static struct spmp_udc_mach_info *udc_info;

/*************************** DEBUG FUNCTION ***************************/
#define	DMA_ADDR_INVALID	(~(dma_addr_t)0)
#define DEBUG_NORMAL	1
#define DEBUG_VERBOSE	2
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

#define IS_USBHOST_PRESENT() (udc_read(UDC_LLCS_OFST) & LCS_VBUS_HIGH) 

/*------------------------- I/O ----------------------------------*/

/*
 *	spmp_udc_done
 */
 
 static void udc_tasklet_read(unsigned long param)
{
	struct spmp_udc *dev = (struct spmp_udc *)param;
	unsigned long flags;
	local_irq_save (flags);
    spmp_udc_handle_ep_out(&dev->ep[2]);	
	local_irq_restore (flags);	
}
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
 #define UDC_FLASH_BUFFER_SIZE ((1024*64))
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

	udelay(5);
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
	u32		idx;
	int		fifo_reg;
	u32		csr_reg;
	idx = ep->bEndpointAddress & 0x7F;
   if(idx !=0)
   	{
		dprintk(DEBUG_NORMAL,"write ep0 idx error\n");
		return -1;
 	}
//    udc_write(0,  UDC_EP0DC_OFST);
//    ep->dev->ep0state=EP0_IDLE;
    udc_write(udc_read(UDC_EP0CS_OFST) |EP0CS_DIR_IN,  UDC_EP0CS_OFST);
	count = spmp_udc_write_packet(UDC_EP0DP_OFST, req, ep->ep.maxpacket);
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

       udc_write(udc_read(UDC_EP0CS_OFST) |EP0CS_SET_EP0_IVLD,  UDC_EP0CS_OFST);
    	while(udc_read(UDC_EP0CS_OFST) & EP0CS_IVLD)
     	{				
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

/*
 *	spmp_udc_write_fifo
 *
 * return:  0 = still running, 1 = completed, negative = errno
 */
static int spmp_udc_write_ep1_fifo(struct spmp_ep *ep,
		struct spmp_request *req)
{
	unsigned	count;
	int		is_last = 0;
	u8       *buf;
	u32		idx;
	int       i;
	int		fifo_reg;
	u32		csr_reg;
   int  CSW_UDC_FLAG=0;	
	idx = ep->bEndpointAddress & 0x7F;
   if(idx !=1)
   	{
		dprintk(DEBUG_NORMAL,"write ep0 idx error\n");
		return -1;
 	}
#if 1	 
#if 0
    if(req->req.length >= 512)  // force in dma
    {
      int dma_xferlen = 0;    
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
      while(req->req.actual < req->req.length)
      {
         int tmp;
          buf = req->req.dma+ req->req.actual;
          udc_write(udc_read(UDC_EP12C_OFST) |EP12C_DIR_IN,  UDC_EP12C_OFST);		
		   
	       dma_xferlen = min(req->req.length - req->req.actual, UDC_FLASH_BUFFER_SIZE);
	       dprintk(DEBUG_NORMAL, "dma_xfer write = %08x %d\n",buf,dma_xferlen);
			dma_done = 0;					 
	       udc_write((u32)buf,UDC_DMA_DA_OFST);
 	       udc_write(DMACS_BURST_INCADDR |((dma_xferlen & 0xFFFF) -1) ,UDC_DMA_CS_OFST);
        	udc_write(udc_read(UDC_DMA_CS_OFST) | DMACS_DMA_EN , UDC_DMA_CS_OFST);					 
		    //wait_event(udc_dma_done, dma_done == 1);
			while(udc_read(UDC_DMA_CS_OFST) & DMACS_DMA_EN);	
			tmp=0;
			//wait until usb phy finish, only read to host need to wait
			while (udc_read(UDC_LLCFS_OFST) & LCFS_EP1_IVLD) 
			{ 
			}	
			req->req.actual +=dma_xferlen;
       	udc_write(udc_read(UDC_EP12C_OFST) & ~(EP12C_DIR_IN),  UDC_EP12C_OFST);							
      }
		if (req->req.dma != DMA_ADDR_INVALID) {
//			    dprintk(DEBUG_NORMAL, "unmap dma\n");
				dma_unmap_single(ep->dev->gadget.dev.parent,
				req->req.dma, req->req.length,
				(ep->bEndpointAddress & USB_DIR_IN)
					? DMA_TO_DEVICE
					: DMA_FROM_DEVICE);
        		req->req.dma = DMA_ADDR_INVALID;
		}			
		spmp_udc_done(ep, req, 0);			
	   is_last =1;
       return is_last;

    }
else
#endif	
{
   udc_write(udc_read(UDC_EP12C_OFST) |EP12C_DIR_IN,  UDC_EP12C_OFST);	
   while(req->req.actual < req->req.length)
   {
	  	count = spmp_udc_write_packet(UDC_EP12FDP_OFST, req, ep->ep.maxpacket);
	   	dprintk(DEBUG_NORMAL, "spmp_udc_write CBW_UDC = %d\n",count);	 			
		/* last packet is often short (sometimes a zlp) */
	    udc_write(udc_read(UDC_EP12C_OFST) |EP12C_SET_EP1_IVLD,  UDC_EP12C_OFST);	
	   	while(udc_read(UDC_EP12FS_OFST) & (EP12FS_EP1_IVLD | EP12FS_P_EP1_IVLD)) /* must check it skip pipo*/
//	   	while(udc_read(UDC_EP12FS_OFST) & (EP12FS_EP1_IVLD)) /* must check it skip pipo*/
	   	{				
			/* sometimes will be handup here*/
	    } 			 		
		if (count != ep->ep.maxpacket){
			is_last = 1; break;
		}
		else if (req->req.length != req->req.actual || req->req.zero)
		{
			is_last = 0;
		}
		else
		{
			is_last = 2; break;
	       } 				
   	}	
	       	udc_write(udc_read(UDC_EP12C_OFST) & ~(EP12C_DIR_IN),  UDC_EP12C_OFST);	
	if (is_last) {
//	   udc_write(EP1SCS_CLR_IVLD | EP1SCS_RESET_FIFO ,UDC_EP1SCS_OFST);		
	   	   dprintk(DEBUG_NORMAL, " spmp_udc_write_ep1_fifo done\n");			 
//		printk("done\n");
//		udelay(100);
			spmp_udc_done(ep, req, 0);
			is_last = 1;
		}
		return is_last;	
}
#else
// force in dma
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
      while(req->req.actual < req->req.length)
      {
          buf = req->req.dma+ req->req.actual;
          udc_write(udc_read(UDC_EP12C_OFST) |EP12C_DIR_IN,  UDC_EP12C_OFST);		
		   
	       dma_xferlen = min(req->req.length - req->req.actual, UDC_FLASH_BUFFER_SIZE);
	       dprintk(DEBUG_NORMAL,"dma_xfer write = %08x %d\n",buf,dma_xferlen);
			dma_done = 0;					 
	       udc_write((u32)buf,UDC_DMA_DA_OFST);
 	       udc_write(DMACS_BURST_INCADDR |(((dma_xferlen -1) & 0xFFFF))  ,UDC_DMA_CS_OFST);
        	udc_write(udc_read(UDC_DMA_CS_OFST) | DMACS_DMA_EN , UDC_DMA_CS_OFST);					 
//		    wait_event_interruptible(udc_dma_done, dma_done == 1);
//		    wait_event(udc_dma_done, dma_done == 1);
//			printk("dma write\n");
			while(udc_read(UDC_DMA_CS_OFST) & DMACS_DMA_EN)
			{
				printk("dma = %d\n",udc_read(UDC_DMA_CS_OFST));
				}
			dma_done = 0;	
			//wait until usb phy finish, only read to host need to wait
			while (udc_read(UDC_LLCFS_OFST) & LCFS_EP1_IVLD) 
			{ 
						printk("dma write xxx\n");
			}	
			req->req.actual +=dma_xferlen;
       	udc_write(udc_read(UDC_EP12C_OFST) & ~(EP12C_DIR_IN),  UDC_EP12C_OFST);							
      }		
		if (req->req.dma != DMA_ADDR_INVALID) {
				dma_unmap_single(ep->dev->gadget.dev.parent,
				req->req.dma, req->req.length,
				(ep->bEndpointAddress & USB_DIR_IN)
					? DMA_TO_DEVICE
					: DMA_FROM_DEVICE);
        		req->req.dma = DMA_ADDR_INVALID;
		}			
//     	while(udc_read(UDC_EP12FS_OFST) & (EP12FS_EP1_IVLD | EP12FS_P_EP1_IVLD))
//     	{				
//
//       } 						
		spmp_udc_done(ep, req, 0);			
	   is_last =1;
       return is_last;
#endif
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
	int		fifo_reg;
	u32		csr_reg;

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

static inline int spmp_udc_read_packet(int fifo, u8 *buf,
		struct spmp_request *req, unsigned avail)
{
	unsigned len;

	len = min(req->req.length - req->req.actual, avail);
	req->req.actual += len;

   udelay(5);
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
	u32		ep_csr;
	unsigned	bufferspace;
	int		is_last=1;
	unsigned	avail;
	int		fifo_count = 0;
	u32		idx;
	int		fifo_reg;

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
static int spmp_udc_read_ep2_fifo(struct spmp_ep *ep,
				 struct spmp_request *req)
{
	u8		*buf;
	u32		ep_csr;
	unsigned	bufferspace;
	int		is_last=1;
	unsigned	avail;
	int		fifo_count = 0;
	u32		idx;
	int		fifo_reg;

	idx = ep->bEndpointAddress & 0x7F;
   if(idx !=2)
   	{
		dprintk(DEBUG_NORMAL,"read ep2 idx error\n");
		return -1;
 	}

 while(req->req.actual < req->req.length)
 {
	buf = req->req.buf + req->req.actual;
	bufferspace = req->req.length - req->req.actual;
	if (!bufferspace) {
		dprintk(DEBUG_NORMAL, "%s: buffer full!\n", __func__);
		return -1;
	}

	fifo_count = spmp_udc_fifo_count_ep12();

	dprintk(DEBUG_NORMAL, "%s fifo count : %d %08x\n", __func__, fifo_count,buf);
	if (fifo_count > ep->ep.maxpacket)
		avail = ep->ep.maxpacket;
	else
		avail = fifo_count;

	fifo_count = spmp_udc_read_packet(UDC_EP12FDP_OFST, buf, req, avail);

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
//		is_last = 1;
	if (is_last) {
//		printk("spmp_udc_read_ep2_fifo is last = %d\n",fifo_count);
		udc_write(udc_read(UDC_EP12C_OFST) |EP12C_CLR_EP2_OVLD,  UDC_EP12C_OFST);
		spmp_udc_done(ep, req, 0);
		break;
	} else {
		udc_write(udc_read(UDC_EP12C_OFST) |EP12C_CLR_EP2_OVLD,  UDC_EP12C_OFST);
	}
//	if((fifo_count = spmp_udc_fifo_count_ep12()) == 0) break;
//	if(!(udc_read(UDC_EP12C_OFST) & EP12C_EP2_OVLD)) break;
	if(!(udc_read(UDC_EP12FS_OFST) & (EP12FS_EP2_OVLD | EP12FS_N_EP2_OVLD))) break;
 }
	return is_last;
}

/*
 * return:  0 = still running, 1 = queue empty, negative = errno
 */
static int spmp_udc_read_ep2_dma(struct spmp_ep *ep,
				 struct spmp_request *req)
{
	u8		*buf;
	u32		ep_csr;
	unsigned	bufferspace;
	int		is_last=0;
	unsigned	avail;
	int		fifo_count = 0;
	u32		idx;
	int		fifo_reg;
   int tmp;
	idx = ep->bEndpointAddress & 0x7F;
   if(idx !=2)
   	{
		dprintk(DEBUG_NORMAL,"write ep0 idx error\n");
		return -1;
 	}
#if 1	
//  if(!(udc_read(UDC_EP12C_OFST) & EP12C_MSDC_CMD_VLD) && ((fifo_count = spmp_udc_fifo_count_ep12()) >= 512))  // force in dma
//  printk("read_ep2_dma = %d\n",spmp_udc_fifo_count_ep12());
  if(((fifo_count = spmp_udc_fifo_count_ep12()) >= 512))  // force in dma
  {
      int dma_xferlen = 0;    
//      udc_write(udc_read(UDC_LLCIE_OFST) & ~(UDLC_EP2O_IE) , UDC_LLCIE_OFST);					
      if(req->req.dma == DMA_ADDR_INVALID) {
			req->req.dma = dma_map_single(
				ep->dev->gadget.dev.parent,
				req->req.buf,
				req->req.length,
				(ep->bEndpointAddress & USB_DIR_IN)
					? DMA_TO_DEVICE
					: DMA_FROM_DEVICE);
	   }
//      while((req->req.actual < req->req.length) && ((fifo_count = spmp_udc_fifo_count_ep12()) > 0))
//      while((req->req.actual < req->req.length))
      {
          buf = req->req.dma+ req->req.actual;
          dma_xferlen = min(req->req.length - req->req.actual, 512);
			dma_xferlen = min(fifo_count,dma_xferlen); 		
	       dprintk(DEBUG_NORMAL,"dma_xfer read = %08x %d\n",buf,dma_xferlen);
			dma_done = 0;					 
	       udc_write((u32)buf,UDC_DMA_DA_OFST);
 	       udc_write(DMACS_BURST_INCADDR | DMACS_DMA_WRITE | (((dma_xferlen -1) & 0xFFFF))  ,UDC_DMA_CS_OFST);		 
        	udc_write(udc_read(UDC_DMA_CS_OFST) | DMACS_DMA_EN , UDC_DMA_CS_OFST);					 
//		    wait_event_interruptible(udc_dma_done, dma_done == 1);
//			wait_event(udc_dma_done, dma_done == 1);
//			while(dma_done == 0);	
//			printk("dma Read\n");
			while(udc_read(UDC_DMA_CS_OFST) & DMACS_DMA_EN);	
			dma_done = 0;						
			req->req.actual +=dma_xferlen;
      } 					
		if (req->req.dma != DMA_ADDR_INVALID) {
				dma_unmap_single(ep->dev->gadget.dev.parent,
				req->req.dma, req->req.length,
				(ep->bEndpointAddress & USB_DIR_IN)
					? DMA_TO_DEVICE
					: DMA_FROM_DEVICE);
        		req->req.dma = DMA_ADDR_INVALID;
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

	if (is_last) {
		udc_write(udc_read(UDC_EP12C_OFST) |EP12C_CLR_EP2_OVLD,  UDC_EP12C_OFST);
		spmp_udc_done(ep, req, 0);
	} else {
		udc_write(udc_read(UDC_EP12C_OFST) |EP12C_CLR_EP2_OVLD,  UDC_EP12C_OFST);
	}

//	printk("readdma is last = %d\n",is_last);
//	spmp_udc_done(ep, req, 0);							
//    udc_write(udc_read(UDC_LLCIE_OFST) | (UDLC_EP2O_IE) , UDC_LLCIE_OFST);				
//  	is_last =1;
    return is_last;
  	}
	else
	{
   		 	return spmp_udc_read_ep2_fifo(ep,req);
	}
#else 
	return spmp_udc_read_ep2_fifo(ep,req);
#endif
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
	int len, ret, tmp;

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

	dprintk(DEBUG_NORMAL, "bRequest = %d bRequestType %d wLength = %d\n",
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
		break;

	case USB_REQ_SET_INTERFACE:
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
	}

	dprintk(DEBUG_VERBOSE, "ep0state %s\n", ep0states[dev->ep0state]);
}

static void spmp_udc_handle_ep0s(struct spmp_udc *dev)
{
	u32			ep0csr, epstall;
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

static void spmp_udc_handle_ep0read(struct spmp_udc *dev)
{
	u32			ep0csr, epstall;
	struct spmp_ep	*ep = &dev->ep[0];
	struct spmp_request	*req;
	struct usb_ctrlrequest	crq;

	if (list_empty(&ep->queue))
		req = NULL;
	else
		req = list_entry(ep->queue.next, struct spmp_request, queue);

    ep0csr = udc_read(UDC_LLCSTL_OFST);
	/* clear stall status */
	if (ep0csr & LCSTL_SETEP0STL) {
		spmp_udc_nuke(dev, ep, -EPIPE);
		printk("... clear SENT_STALL ...\n");
       udc_write((udc_read(UDC_LLCSTL_OFST) | LCSTL_CLREP0STL) , UDC_LLCSTL_OFST); // error send stall;
		dev->ep0state = EP0_IDLE;
		return;
	}
	
    ep0csr = udc_read(UDC_EP0CS_OFST);
		
	dprintk(DEBUG_NORMAL, "spmp_udc_handle_ep0 =%08x %08x %08x\n",req,ep0csr,udc_read(UDC_LLCSTL_OFST));
	if ((ep0csr & EP0CS_OVLD) && req ) {
		spmp_udc_read_ep0_fifo(ep,req);
	}
	else
	{
		udc_write(udc_read(UDC_EP0CS_OFST) |EP0CS_CLR_EP0_OVLD,  UDC_EP0CS_OFST);
		dprintk(DEBUG_NORMAL, "EP0_Read_DATA_PHASE ... what now?\n");
	}

}

static void spmp_udc_handle_ep0(struct spmp_udc *dev)
{
	u32			ep0csr, epstall;
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
		break;

	case EP0_IN_DATA_PHASE:			/* GET_DESCRIPTOR etc */
		dprintk(DEBUG_NORMAL, "EP0_IN_DATA_PHASE ... what now?\n");
		if (!(ep0csr & EP0CS_IVLD) && req) {
			spmp_udc_write_ep0_fifo(ep, req);
		}
		break;

	case EP0_OUT_DATA_PHASE:		/* SET_DESCRIPTOR etc */
		dprintk(DEBUG_NORMAL, "EP0_OUT_DATA_PHASE ... what now?\n");
		if ((ep0csr & EP0CS_OVLD) && req ) {
			spmp_udc_read_ep0_fifo(ep,req);
		}
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

static void spmp_udc_handle_ep_in(struct spmp_ep *ep)
{
	struct spmp_request	*req;
	int			is_in = ep->bEndpointAddress & USB_DIR_IN;
	u32			ep_csr1;
	u32         llcstl;
	u32			idx;
	if (likely (!list_empty(&ep->queue)))
		req = list_entry(ep->queue.next,
				struct spmp_request, queue);
	else
		req = NULL;
	 
	idx = ep->bEndpointAddress & 0x7F;
   if(idx !=1)
   	{
		dprintk(DEBUG_NORMAL, "write ep2 idx error\n");
		return;
 	}		
	ep_csr1 = udc_read(UDC_EP12C_OFST);	
    llcstl = udc_read(UDC_LLCSTL_OFST);
	dprintk(DEBUG_NORMAL, "ep%01d rd csr:%02x %08x\n", idx, ep_csr1,llcstl);
	if (llcstl & LCSTL_SETEP1STL) {
    	udc_write(llcstl |LCSTL_CLREP1STL ,UDC_LLCSTL_OFST);
		return;
	}
    if (!(ep_csr1& EP12C_EP1_IVLD) && req) {
	 	spmp_udc_write_ep1_fifo(ep,req);
	 }
}


static void spmp_udc_handle_ep_out(struct spmp_ep *ep)
{
	struct spmp_request	*req;
	int			is_in = ep->bEndpointAddress & USB_DIR_IN;
	u32			ep_csr1;
	u32         llcstl;
	u32			idx;
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
    llcstl = udc_read(UDC_LLCSTL_OFST);
	dprintk(DEBUG_NORMAL, "ep%01d rd csr:%02x %08x\n", idx, ep_csr1,llcstl);
	if (llcstl & LCSTL_SETEP2STL) {
    	udc_write(llcstl |LCSTL_CLREP2STL ,UDC_LLCSTL_OFST);
		return;
	}
	dprintk(DEBUG_NORMAL, "Read =%d\n",req ? 1 : 0);
	  if ((ep_csr1 & EP12C_EP2_OVLD) && req) {
	 	spmp_udc_read_ep2_fifo(ep,req);
//			spmp_udc_read_ep2_dma(ep,req);
	 }
}

/*
 *	spmp_udc_irq - interrupt handler
 */
/* from ecos code */
static void clearHwState_UDC(int a_iMode)
{
//	int clr;
	int cnt;
   unsigned int dma_cs;
   unsigned int reg_val;
	/*INFO: we don't disable udc interrupt when we are clear udc hw state, 
	* 1.since when we are clearing, we are in ISR , will not the same interrupt reentry problem. 
	* 2.after we finish clearing , we will go into udc ISR again, if there are interrupts occur while we are clearing ,we want to catch them 
	*  immediately
	*/
	//===== check udc DMA state, and flush it =======	
	/*if  dma is transfering data */
	dma_cs = udc_read(UDC_DMA_CS_OFST);
	if (dma_cs & DMACS_DMA_EN)
		{	
			unsigned int tmp;
			
			//when read/write we stop dma first , then flush dma, for safety
			cnt=0;

		   /*  DMA is WRITE , USB is OUT*/
			if(DMACS_DMA_WRITE & dma_cs)
				{	
	            dprintk(DEBUG_NORMAL, "---------------------- WRITE ---------------------------\n");
            	 dprintk(DEBUG_NORMAL, "[1] rUDC_DMA=%x  rEP12_CTRL=%x rUDC_DIS=%x \n ", udc_read(UDC_DMA_CS_OFST), udc_read(UDC_EP12C_OFST), udc_read(UDC_CIS_OFST));
               dprintk(DEBUG_NORMAL, "[2] rUDC_SP=%x  rUDC_BIT_OP0=%x rUDC_AS_CTRL=%x \n",udc_read(UDC_CS_OFST), udc_read(UDC_LLCFS_OFST), udc_read(UDC_EP12PPC_OFST));	
            	 dprintk(DEBUG_NORMAL, "[3] rEP0_CTRL=%x  rEP12_CNTRL=%x rEP12_CNTRH=%x \n",udc_read(UDC_EP0CS_OFST), udc_read(UDC_EP12FCL_OFST), udc_read(UDC_EP12FCH_OFST));	
	            dprintk(DEBUG_NORMAL, "===================================================\n");	
					//ensure  1.UDC EP2 out fifo is empty 
					//			2. DMA finish a 512 bytes transfer 
					tmp = udc_read(UDC_EP12FS_OFST);
					while( (tmp & (EP12FS_EP2_OVLD |EP12FS_N_EP2_OVLD)) 
						|| (!((dma_cs & (0x1FF)) == 0x1FF)))
						{
						}
				}
			/*  DMA is READ , USB is IN*/
			else
				{
				    while(dma_cs & DMACS_DMA_EN)
				    {
						cnt++;
						if(cnt>400000)
						{
	                        dprintk(DEBUG_NORMAL, "------------------------ READ -------------------------\n");
            	              dprintk(DEBUG_NORMAL, "[1] rUDC_DMA=%x  rEP12_CTRL=%x rUDC_DIS=%x \n ", udc_read(UDC_DMA_CS_OFST), udc_read(UDC_EP12C_OFST), udc_read(UDC_CIS_OFST));
                            dprintk(DEBUG_NORMAL, "[2] rUDC_SP=%x  rUDC_BIT_OP0=%x rUDC_AS_CTRL=%x \n",udc_read(UDC_CS_OFST), udc_read(UDC_LLCFS_OFST), udc_read(UDC_EP12PPC_OFST));	
                        	   dprintk(DEBUG_NORMAL, "[3] rEP0_CTRL=%x  rEP12_CNTRL=%x rEP12_CNTRH=%x \n",udc_read(UDC_EP0CS_OFST), udc_read(UDC_EP12FCL_OFST), udc_read(UDC_EP12FCH_OFST));	
                            dprintk(DEBUG_NORMAL, "===================================================\n");								
								cnt=0;
						}
        				tmp = udc_read(UDC_EP12C_OFST);						
						tmp = tmp | EP12C_RESET_PIPO;
						udc_write(tmp,UDC_EP12C_OFST);
                   	dma_cs = udc_read(UDC_DMA_CS_OFST);						
					}
				}
          dma_cs = udc_read(UDC_DMA_CS_OFST);		
			dma_cs = dma_cs |DMACS_DMA_FLUSH;//flush dma
			//wait dma flush end
			tmp=0;
          dma_cs = udc_read(UDC_DMA_CS_OFST);			
			while(!(dma_cs & DMACS_DMA_FLUSHEND))
			{
				tmp++;
				if(tmp> 300000)
				{
					dprintk(DEBUG_NORMAL, "@#  ");
					tmp=0;
				}		
				//check if read 
				if(!(dma_cs & DMACS_DMA_WRITE)) 
				{
				   int tmp1;
        	   		tmp1 = udc_read(UDC_EP12C_OFST);						
					tmp1 = tmp1 | EP12C_RESET_PIPO;
           	   udc_write(tmp1,UDC_EP12C_OFST);							 
				}
              dma_cs = udc_read(UDC_DMA_CS_OFST);		
			}			
			/* follow is so strange */
          dma_cs = udc_read(UDC_DMA_CS_OFST);
			dma_cs=dma_cs & (~(DMACS_DMA_FLUSH));
			udc_write(dma_cs,UDC_DMA_CS_OFST);
			
          dma_cs = udc_read(UDC_DMA_CS_OFST);			
			dma_cs=dma_cs | ((DMACS_DMA_FLUSHEND));
			udc_write(dma_cs,UDC_DMA_CS_OFST);			
			
          dma_cs = udc_read(UDC_DMA_CS_OFST);						
			dma_cs=dma_cs & (~(DMACS_DMA_FLUSHEND));
			udc_write(dma_cs,UDC_DMA_CS_OFST);			
			
	}	

		
	//== EP0 
	udc_write(EP0CS_CLR_EP0_OVLD,UDC_EP0CS_OFST); //clear ep0 out vld=1, clear set epo in vld=0, set ctl dir to OUT direction=0
	udc_write(0x0,UDC_EP0DC_OFST);
	
	//=EP0 setup 
	//rEP0_SETUP_CTRL
	//== EP1S
   udc_write(EP1SCS_CLR_IVLD | EP1SCS_RESET_FIFO ,UDC_EP1SCS_OFST);
	//==EP12
	udc_write(EP12C_CLR_EP2_OVLD | EP12C_RESET_PIPO | EP12C_ENABLE_BULK ,UDC_EP12C_OFST);
	
    reg_val = udc_read(UDC_EP12PPC_OFST);	
	reg_val= reg_val |EP12PPC_AUTO_SW_EN; //set auto switch
//	reg_val= reg_val & ~(EP12PPC_AUTO_SW_EN); //set auto switch
	udc_write(reg_val,UDC_EP12PPC_OFST);
	
	//= ep12 cnter
//	rEP12_CNTRL=0;
	udc_write(0 ,UDC_EP12FCL_OFST);
	
    reg_val = udc_read(UDC_EP12FCH_OFST);	
	reg_val=reg_val |EP12FCH_RESET_CNTR;
	udc_write(reg_val,UDC_EP12FCH_OFST);	
	
	udc_write(0 ,UDC_EP12FCH_OFST);
	
	//== EP3 	
    reg_val = udc_read(UDC_LLCSTL_OFST);	
	reg_val=reg_val |(LCSTL_CLREP3STL | LCSTL_CLREP2STL | LCSTL_CLREP1STL | LCSTL_CLREP0STL);
	udc_write(reg_val,UDC_LLCSTL_OFST);		

    reg_val = udc_read(UDC_LLCIE_OFST);	
	reg_val=reg_val |UDLC_EP2O_IE;
	udc_write(reg_val,UDC_LLCIE_OFST);	

	// 2008/6/26, to prevent when PIPO IS IN , plug off intr occur; or comment this since each new CBW will set it again
    reg_val = udc_read(UDC_EP12C_OFST);	
	reg_val= reg_val & 0xFE;
	udc_write(reg_val,UDC_EP12C_OFST);	
	
}



static int vbusIntr2_UDC(void)
{
    unsigned int 	tmp;
    unsigned int 	llcset0;				
    tmp	 = udc_read(UDC_LLCS_OFST);
    if( tmp & LCS_VBUS_HIGH)
	{								
       llcset0	 = udc_read(UDC_LLCSET0_OFST);							
       if(llcset0 & LCSET0_SOFT_DISC)
		{ 
		   llcset0 &= 0xFE;
        	dprintk(DEBUG_NORMAL, "vbusIntr2_UDC Connect \n");					 
          udc_write(llcset0, UDC_LLCSET0_OFST); 
       }
	} 
	else /* host absent */
	{					
		//must disconnect first
       llcset0	 = udc_read(UDC_LLCSET0_OFST);						
       if(!(udc_read(UDC_LLCSET0_OFST) & LCSET0_SOFT_DISC))
		{
		   llcset0 |= LCSET0_SOFT_DISC;
			dprintk(DEBUG_NORMAL, "vbusIntr2_UDC DisConnect\n");
          udc_write(llcset0, UDC_LLCSET0_OFST); 
       }	
		clearHwState_UDC(0);				
	}

    //		g_ep_state[0]= S_IDLE;		
    tmp	 = udc_read(UDC_CIS_OFST);
	tmp |= CIS_VBUS_IF;
   udc_write(tmp, UDC_CIS_OFST); 	
   return 0;	
}

static irqreturn_t spmp_udc_irq(int dummy, void *_dev)
{
	struct spmp_udc *dev = _dev;
	int usb_status;
	int usbd_status;
	int pwr_reg;
	int ep0csr;
	int i;
	u32 tmp,tmpx,udc_irq_flags;
	unsigned long flags;
//    int g_pviNeedUpAsyn =0;
	spin_lock_irqsave(&dev->lock, flags);
 // ========== dma_if interrupt ============
	tmp = udc_read(UDC_CIS_OFST);

   if ((tmp & CIS_DMA_IF))
   {
    	dprintk(DEBUG_NORMAL, "spmp_udc_irq DMA =%08x\n",tmp); 
//		printk("spmp_udc_irq DMA =%08x\n",tmp); 
		udc_write((tmp | CIS_DMA_IF), UDC_CIS_OFST);  //clear DMA IF
		dma_done = 1;
//	    wake_up_interruptible(&udc_dma_done);			
//       tmp = udc_read(UDC_DMA_CS_OFST);
//       printk("DMA =%08x\n",tmp);
//	    wake_up(&udc_dma_done);			
   	}
	tmp = udc_read(UDC_CIS_OFST);	 
//	dprintk(DEBUG_NORMAL, "spmp_udc_irq called 2 =%08x\n",tmp); 	
/* ========== force disconnect  interrupt ======== */
	if(tmp & CIS_FDISCONN_IF)
	{
		udc_write((tmp | CIS_FDISCONN_IF), UDC_CIS_OFST);  //clear DMA IF	   
	}
	tmp = udc_read(UDC_CIS_OFST);	
//	dprintk(DEBUG_NORMAL, "spmp_udc_irq called 3 =%08x\n",tmp); 	
/* ==========  force connect  interrupt ========= */
	if(tmp & CIS_FCONN_IF)
	{
		udc_write((tmp | CIS_FCONN_IF), UDC_CIS_OFST);  //clear DMA IF	   	   
	}
	tmp = udc_read(UDC_CIS_OFST);	
//	dprintk(DEBUG_NORMAL, "spmp_udc_irq called 4 =%08x\n",tmp); 	
/* ========== usb_vbus_if interrupt ===========*/
	if(tmp & CIS_VBUS_IF) 
	{
  		dprintk(DEBUG_NORMAL, "spmp_udc_irq called CIS_VBUS_IF =%08x\n",tmp); 
	     if (0 != vbusIntr2_UDC())
	     {
           tmp	 = udc_read(UDC_CIS_OFST);
           tmp |= CIS_VBUS_IF;
           udc_write(tmp, UDC_CIS_OFST); 	
           spin_unlock_irqrestore(&dev->lock, flags); 			 
     		 return IRQ_HANDLED;
	     }
	}

/**************************************************************************
 *                 						process	 other UDC_IRQ_FLAG 
 **************************************************************************/
	tmp = udc_read(UDC_CIS_OFST); 
//	dprintk(DEBUG_NORMAL, "spmp_udc_irq called 5 =%08x\n",tmp); 
 	if (!(tmp & CIS_UDLC_IF))
	{ 
//   		udc_write((tmp | CIS_UDLC_IF), UDC_CIS_OFST);  //clear DMA IF	   	   
        spin_unlock_irqrestore(&dev->lock, flags);
		 return IRQ_HANDLED;
	}

/** have irq flag pending in linker layer */

	udc_irq_flags = udc_read(UDC_LLCIF_OFST);	
    udc_write(udc_irq_flags,UDC_LLCIF_OFST);  // clear flag;	
//	printk("spmp_udc_irq called 10 =%08x\n",udc_irq_flags);     
	if( udc_irq_flags & UDLC_SCONF_IE)
	{
	    //todo :  we will notify it to gadget.
	}
    // ========== RESET end  interrupt ============	
	if (udc_irq_flags & UDLC_RESETN_IE)
	{	
	}				
   //========== SUSPEND interrupt ============
	if (udc_irq_flags & UDLC_SUSPEND_IE)
	{		
	}
	//========== RESET interrupt =================
	if (udc_irq_flags & UDLC_RESET_IE) 
	{
		/* two kind of reset :
		 * - reset start -> pwr reg = 8
		 * - reset end   -> pwr reg = 0
		 **/
       unsigned int 	llcset0;				 
		dprintk(DEBUG_NORMAL, "USB reset csr %x pwr %x\n",
			ep0csr, pwr_reg);

		dev->gadget.speed = USB_SPEED_UNKNOWN;
		dev->address = 0;
		dev->ep0state = EP0_IDLE;
       llcset0	 = udc_read(UDC_LLCSET0_OFST);						
		llcset0 |= 	LCSET0_PWR_SUSP_N;
		udc_write(llcset0,UDC_LLCSET0_OFST);		
       clearHwState_UDC(0);
		spin_unlock_irqrestore(&dev->lock, flags);
		return IRQ_HANDLED;
	}
   // ========== RESUME interrupt ============ 
	/* RESUME */
	if (udc_irq_flags & UDLC_RESUME_IE) {
		dprintk(DEBUG_NORMAL, "USB resume\n");

		if (dev->gadget.speed != USB_SPEED_UNKNOWN
				&& dev->driver
				&& dev->driver->resume)
			dev->driver->resume(&dev->gadget);

    	 //SET_PWR_SUSPEND();//prevent suspend	
    	 udc_write(udc_read(UDC_LLCSET0_OFST) | LCSET0_PWR_SUSP_N , UDC_LLCSET0_OFST);
//	    clearHwState_UDC(0);			
	}
   // ========== FLG_EP0_SETUP interrupt ============
	if ((udc_irq_flags & UDLC_EP0S_IE))
	{
		spmp_udc_handle_ep0s(dev);
	}	
    // ========== FLG_EP0_IN interrupt ============
	if ((udc_irq_flags & UDLC_EP0I_IE))// status stage 
	{			
        dprintk(DEBUG_NORMAL, "FLG_EP0_IN interrupt\n");
       spmp_udc_handle_ep0(dev);
	}
    // ========== FLG_EP0_OUT interrupt ============
	if((udc_irq_flags & UDLC_EP0O_IE)) 
	{	// OUT	
        dprintk(DEBUG_NORMAL, "UDLC_EP0O_IE interrupt\n");
//      spmp_udc_handle_ep0read(dev);				
		dev->ep0state = EP0_IDLE;
		udc_write(udc_read(UDC_EP0CS_OFST) |EP0CS_CLR_EP0_OVLD,  UDC_EP0CS_OFST);
	}	
   // ========== FLG_EP2_OUT interrupt ============
	if((udc_irq_flags & UDLC_EP2O_IE)) 
	{
        dprintk(DEBUG_NORMAL, "UDLC_EP2O_IE interrupt =%08x\n",udc_read(UDC_EP12C_OFST));
//		 if(udc_read(UDC_EP12C_OFST) & EP12C_MSDC_CMD_VLD)
//    		 spmp_udc_handle_ep_out(&dev->ep[2]);
    		tasklet_schedule(&dev->dma_task);
	}
   // ========== UDLC_EP1I_IE interrupt ============
	if((udc_irq_flags & UDLC_EP1I_IE)) 
	{			
        dprintk(DEBUG_NORMAL, "UDLC_EP1I_IE interrupt\n");	
//		spmp_udc_handle_ep_in(&dev->ep[1]);
	}	
	spin_unlock_irqrestore(&dev->lock, flags);

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
	unsigned long		flags;
	u32			csr1,csr2;
	u32			int_en_reg,reg_val;

	ep = to_spmp_ep(_ep);

	if (!_ep || !desc || ep->desc
			|| _ep->name == ep0name
			|| desc->bDescriptorType != USB_DT_ENDPOINT)
		return -EINVAL;

	dev = ep->dev;
	if (!dev->driver || dev->gadget.speed == USB_SPEED_UNKNOWN)
		return -ESHUTDOWN;

	max = le16_to_cpu(desc->wMaxPacketSize) & 0x1fff;

	local_irq_save (flags);
	_ep->maxpacket = max & 0x7ff;
	ep->desc = desc;
	ep->halted = 0;
	ep->bEndpointAddress = desc->bEndpointAddress;

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
	/* print some debug message */
	tmp = desc->bEndpointAddress;
	dprintk (DEBUG_NORMAL, "enable %s(%d) ep%x%s-blk max %02x\n",
		 _ep->name,ep->num, tmp,
		 desc->bEndpointAddress & USB_DIR_IN ? "in" : "out", max);

	local_irq_restore (flags);
	spmp_udc_set_halt(_ep, 0);

	return 0;
}

/*
 * spmp_udc_ep_disable
 */
static int spmp_udc_ep_disable(struct usb_ep *_ep)
{
	struct spmp_ep *ep = to_spmp_ep(_ep);
	unsigned long flags;
	u32 int_en_reg;

	if (!_ep || !ep->desc) {
		dprintk(DEBUG_NORMAL, "%s not enabled\n",
			_ep ? ep->ep.name : NULL);
		return -EINVAL;
	}

	local_irq_save(flags);

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

	local_irq_restore(flags);

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
	u32			ep_csr = 0;
	int			fifo_count = 0;
	unsigned long		flags;

	if (unlikely (!_ep || (!ep->desc && ep->ep.name != ep0name))) {
		dprintk(DEBUG_NORMAL, "%s: invalid args\n", __func__);
		return -EINVAL;
	}

	dev = ep->dev;
	if (unlikely (!dev->driver
			|| dev->gadget.speed == USB_SPEED_UNKNOWN)) {
		return -ESHUTDOWN;
	}

	local_irq_save (flags);

	if (unlikely(!_req || !_req->complete
			|| !_req->buf || !list_empty(&req->queue))) {
		if (!_req)
			dprintk(DEBUG_NORMAL, "%s: 1 X X X\n", __func__);
		else {
			dprintk(DEBUG_NORMAL, "%s: 0 %01d %01d %01d\n",
				__func__, !_req->complete,!_req->buf,
				!list_empty(&req->queue));
		}

		local_irq_restore(flags);
		return -EINVAL;
	}

	_req->status = -EINPROGRESS;
	_req->actual = 0;

    dprintk(DEBUG_NORMAL, "%s: ep%x len %d\n",
		 __func__, ep->bEndpointAddress, _req->length);
	if (ep->bEndpointAddress) {
       switch (ep->bEndpointAddress & 0x7F)
       {
		   case 1:
     	   case 2:
   		       fifo_count = spmp_udc_fifo_count_ep12();				 	
    dprintk(DEBUG_NORMAL, "spmp_udc_fifo_count_ep12=%d\n",fifo_count);								 
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
    dprintk(DEBUG_NORMAL, "spmp_udc_fifo_count_ep0=%d\n",fifo_count);		
	}

	/* kickstart this i/o queue? */
	if (list_empty(&ep->queue) && !ep->halted) {
		if (ep->bEndpointAddress == 0 /* ep0 */) {
			ep_csr = udc_read(UDC_EP0CS_OFST);			
			switch (dev->ep0state) {
			case EP0_IN_DATA_PHASE:
				if (!(ep_csr& EP0CS_IVLD)
						&& spmp_udc_write_ep0_fifo(ep,
							req)) {
					dev->ep0state = EP0_IDLE;
					req = NULL;
				}
				break;

			case EP0_OUT_DATA_PHASE:
				if ((!_req->length)
					|| ((ep_csr & EP0CS_OVLD)
						&& spmp_udc_read_ep0_fifo(ep,
							req))) {
					dev->ep0state = EP0_IDLE;
					req = NULL;
				}
				break;

			default:
				local_irq_restore(flags);
				return -EL2HLT;
			}
		} 
		else if ((ep->bEndpointAddress & USB_DIR_IN) != 0)
		{		
		   switch (ep->bEndpointAddress & 0x7F)
		   {
		      case 1:
       			ep_csr = udc_read(UDC_EP12C_OFST);
                 dprintk(DEBUG_NORMAL, "ep1 csr =%d\n",ep_csr);						
                 if((!(ep_csr& EP12C_EP1_IVLD)))
                 	{					                 	
        			   if(spmp_udc_write_ep1_fifo(ep, req))
           			    req = NULL;								 	
                 	}
					else
       			    req = NULL;
				 	break;				 
			   case 3:
       			ep_csr = udc_read(UDC_EP3CS_OFST);
                 if((!(ep_csr& EP3CS_IVLD)))
                 	{
        			   if(spmp_udc_write_ep3_fifo(ep, req))
           			    req = NULL;								 	
                 	}
					else
       			    req = NULL;
				 	break;
				default:
				return -EINVAL;
		   	}
		} 
		else if (fifo_count)
		{
		   if(ep->bEndpointAddress !=2)
				return -EINVAL;			 	
    		else
    		{
      			ep_csr = udc_read(UDC_EP12C_OFST);    		
                 dprintk(DEBUG_NORMAL, "ep2 csr =%d\n",ep_csr);												
                 if(ep_csr & EP12C_EP2_OVLD)
                 	{              	
        			   if(spmp_udc_read_ep2_fifo(ep, req))
           			    req = NULL;								 	
                 	}
					else
       			    req = NULL;
    		}
		}
	}else
		{
//		     printk("queue empty \n");
		}

	/* pio or dma irq handler advances the queue. */
	if (likely (req != 0))
		list_add_tail(&req->queue, &ep->queue);

	dprintk(DEBUG_VERBOSE, "%s ok\n", __func__);
	local_irq_restore(flags);

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
	unsigned long		flags;
	struct spmp_request	*req = NULL;

	dprintk(DEBUG_VERBOSE, "%s(%p,%p)\n", __func__, _ep, _req);

	if (!the_controller->driver)
		return -ESHUTDOWN;

	if (!_ep || !_req)
		return retval;

	udc = to_spmp_udc(ep->gadget);

	local_irq_save (flags);

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

	local_irq_restore (flags);
	return retval;
}

/*
 * spmp_udc_set_halt
 */
static int spmp_udc_set_halt(struct usb_ep *_ep, int value)
{
	struct spmp_ep	*ep = to_spmp_ep(_ep);
	unsigned long		flags;
	u32			idx;

	if (unlikely (!_ep || (!ep->desc && ep->ep.name != ep0name))) {
		dprintk(DEBUG_NORMAL, "%s: inval 2\n", __func__);
		return -EINVAL;
	}

	local_irq_save (flags);

	idx = ep->bEndpointAddress & 0x7F;

	if (idx == 0) {
       udc_write((udc_read(UDC_LLCSTL_OFST) | LCSTL_SETEP0STL) , UDC_LLCSTL_OFST); // error send stall;
	} else {
       if(value)
       {
         switch (ep->bEndpointAddress & 0x7F)
         {
		   case 1:
             udc_write((udc_read(UDC_LLCSTL_OFST) | LCSTL_SETEP1STL) , UDC_LLCSTL_OFST); // error send stall;
             udc_write(((udc_read(UDC_EP12C_OFST) & ~(EP12C_SET_EP1_IVLD)) |EP12C_RESET_PIPO) , UDC_EP12C_OFST); 
             break;
     	   case 2:
             udc_write((udc_read(UDC_LLCSTL_OFST) | LCSTL_SETEP2STL) , UDC_LLCSTL_OFST); // error send stall;
             udc_write((udc_read(UDC_EP12C_OFST) | EP12C_CLR_EP2_OVLD |EP12C_RESET_PIPO) , UDC_EP12C_OFST);        
              break;				 	
		   case 3:
             udc_write((udc_read(UDC_LLCSTL_OFST) | LCSTL_SETEP3STL) , UDC_LLCSTL_OFST); // error send stall;
             udc_write((udc_read(UDC_EP3CS_OFST) | EP3CS_CLR_IVLD) , UDC_EP12C_OFST);                          
			   break;
			default:
				return -EINVAL;
         }
       }
		else
		{
         switch (ep->bEndpointAddress & 0x7F)
         {
		   case 1:
             udc_write((udc_read(UDC_LLCSTL_OFST) | LCSTL_CLREP1STL) , UDC_LLCSTL_OFST); // error send stall;
             udc_write(((udc_read(UDC_EP12C_OFST) & ~(EP12C_SET_EP1_IVLD)) |EP12C_RESET_PIPO) , UDC_EP12C_OFST);              
             break;
     	   case 2:
             udc_write((udc_read(UDC_LLCSTL_OFST) | LCSTL_CLREP2STL) , UDC_LLCSTL_OFST); // error send stall;
             udc_write((udc_read(UDC_EP12C_OFST) | EP12C_CLR_EP2_OVLD |EP12C_RESET_PIPO) , UDC_EP12C_OFST);               
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
	local_irq_restore (flags);

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

static int spmp_udc_set_pullup(struct spmp_udc *udc, int is_on)
{

	return -EOPNOTSUPP;
}

static int spmp_udc_vbus_session(struct usb_gadget *gadget, int is_active)
{

	return -EOPNOTSUPP;
}

static int spmp_udc_pullup(struct usb_gadget *gadget, int is_on)
{
	return -EOPNOTSUPP;
}

static irqreturn_t spmp_udc_vbus_irq(int irq, void *_dev)
{
	return IRQ_HANDLED;
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

	tasklet_kill(&dev->dma_task);	
	tmp = SCUA_USBPHY_CFG |USBPHY_XTAL_ENABLE;
	tmp= tmp & (~(USBPHY1_POWER_CTRL));
	SCUA_USBPHY_CFG = tmp;

//	printf(" scu phy=%0x \n", *rUSB_PHY_BASE_UDC);
	while((SCUA_USBPHY_CFG) & (USBPHY1_POWER_CTRL))
		{
			dprintk(DEBUG_NORMAL, ".");
		}

	//enable PWREN_CPU1 in SCUB 
	SCUB_PWRC_CFG = 
		SCUB_PWRC_CFG |0x01;		
}
	dprintk(DEBUG_NORMAL, "spmp_udc_disable called 1=%08x %08x\n",udc_read(UDC_LLCS_OFST),udc_read(UDC_LLCSET0_OFST));		
   tmp = udc_read(UDC_LLCSET0_OFST	);
   udc_write(tmp | LCSET0_SOFT_DISC, UDC_LLCSET0_OFST);  //SET_SOFT_DISCON();
   udc_write(tmp | LCSET0_PWR_SUSP_N, UDC_LLCSET0_OFST);  //SET_PWR_SUSPEND();
   udc_write(0x0, UDC_LLCIE_OFST);  //DIS_ALL_USB_IRQ();   
   udc_write(0x0, UDC_IE_OFST); 
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
    udc_write(0xFFFFFFFF, UDC_CIS_OFST); 		 
	/* Good bye, cruel world */
//	if (udc_info && udc_info->udc_command)
//		udc_info->udc_command(spmp_UDC_P_DISABLE);

	/* Set speed to unknown */
	dprintk(DEBUG_NORMAL, "spmp_udc_disable called 3=%08x %08x\n",udc_read(UDC_LLCS_OFST),udc_read(UDC_LLCSET0_OFST));			
	dev->gadget.speed = USB_SPEED_UNKNOWN;
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

/*
 * spmp_udc_enable
 */
static void spmp_udc_enable(struct spmp_udc *dev)
{
    int tmp;
    int llcset0;		
//	spmp_udc_disable(dev);
	dev->gadget.speed = USB_SPEED_UNKNOWN;
//    udc_write((CIE_FDISCONN_IE |CIE_FCONN_IE |CIE_DMA_IE | CIE_VBUS_IE) , UDC_IE_OFST); 	 			
    udc_write((CIE_FDISCONN_IE |CIE_FCONN_IE  | CIE_VBUS_IE) , UDC_IE_OFST); 	 			
//	dprintk(DEBUG_NORMAL, "spmp_udc_enable called 3\n");

//    udc_write(UDLC_RESUME_IE |UDLC_SUSPEND_IE |UDLC_EP2O_IE |
//			 UDLC_EP0S_IE | UDLC_RESET_IE, UDC_LLCIE_OFST); 	 	

    udc_write(UDLC_RESUME_IE |UDLC_SUSPEND_IE |UDLC_SCONF_IE |UDLC_EP2O_IE |UDLC_EP0I_IE |
			 UDLC_EP0O_IE | UDLC_EP0S_IE | UDLC_RESET_IE, UDC_LLCIE_OFST); 	 	
	 tmp = udc_read(UDC_CS_OFST);		
	 tmp |= 0xFFFF;
//	dprintk(DEBUG_NORMAL, "spmp_udc_enable called 3 =%08x\n",tmp);	 
    udc_write(tmp, UDC_CS_OFST); 	 	
//	dprintk(DEBUG_NORMAL, "spmp_udc_enable called 4 =%08x\n",udc_read(UDC_CS_OFST));			
//	dprintk(DEBUG_NORMAL, "spmp_udc_disable called KK=%08x %08x\n",udc_read(UDC_LLCS_OFST),udc_read(UDC_LLCSET0_OFST));			
	tmp = udc_read(UDC_LLCS_OFST);		
    llcset0	 = udc_read(UDC_LLCSET0_OFST);		
//	dprintk(DEBUG_NORMAL, "spmp_udc_enable called 5=%08x %08x\n",tmp,llcset0);		
    if( tmp & LCS_VBUS_HIGH)
    {

       if(llcset0 & LCSET0_SOFT_DISC)
		{ 
		   llcset0 &= 0xFE;
        	dprintk(DEBUG_NORMAL, "connect \n");					 
          udc_write(llcset0, UDC_LLCSET0_OFST); 
       }
    }
	else
	{
       if(!(udc_read(UDC_LLCSET0_OFST) & LCSET0_SOFT_DISC))
		{
		   llcset0 |= LCSET0_SOFT_DISC;
          udc_write(llcset0, UDC_LLCSET0_OFST); 
       }	
	}

	tasklet_init(&dev->dma_task, udc_tasklet_read,
			(unsigned long)dev);

	dprintk(DEBUG_NORMAL, "spmp_udc_enable called 5\n");
//    mdelay(500);
	dprintk(DEBUG_NORMAL, "spmp_udc_enable called6\n");	
	/* time to say "hello, world" */
//	if (udc_info && udc_info->udc_command)
//		udc_info->udc_command(0);
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
};

/*
 *	probe - binds to the platform device
 */
static int spmp_udc_probe(struct platform_device *pdev)
{
	struct spmp_udc *udc = &memory;
	struct device *dev = &pdev->dev;
	int retval;
	int irq;

    dprintk(DEBUG_NORMAL, "spmp_udc_probe\n");

	udc_clock = clk_get(NULL, "USB_DEVICE");
	if (IS_ERR(udc_clock)) {
		dev_err(dev, "failed to get udc clock source\n");
		return PTR_ERR(udc_clock);
	}

	clk_enable(udc_clock);

	mdelay(10);

    dprintk(DEBUG_NORMAL, "got and enabled clocks\n");
	spin_lock_init (&udc->lock);
//	udc_info = pdev->dev.platform_data;

	rsrc_start = 0x93006000;
	rsrc_len   = 0x1000;

	if (!request_mem_region(rsrc_start, rsrc_len, gadget_name))
		return -EBUSY;

	base_addr = ioremap(rsrc_start, rsrc_len);
	if (!base_addr) {
		retval = -ENOMEM;
		goto err_mem;
	}

	device_initialize(&udc->gadget.dev);
	udc->gadget.dev.parent = &pdev->dev;
	udc->gadget.dev.dma_mask = pdev->dev.dma_mask;

	the_controller = udc;
	platform_set_drvdata(pdev, udc);

	spmp_udc_disable(udc);
	spmp_udc_reinit(udc);

	/* irq setup after old hardware state is cleaned up */
	retval = request_irq(IRQ_USB_DEV, spmp_udc_irq,
			     IRQF_DISABLED, gadget_name, udc);

	if (retval != 0) {
		dev_err(dev, "cannot get irq %i, err %d\n", IRQ_USB_DEV, retval);
		retval = -EBUSY;
		goto err_map;
	}

	dev_dbg(dev, "got irq %i\n", IRQ_USB_DEV);
    dprintk(DEBUG_NORMAL, "got irq %i\n", IRQ_USB_DEV);
	 if (retval != 0) {
		dev_err(dev, "can't get vbus irq %d, err %d\n",
		irq, retval);
		retval = -EBUSY;
		goto err_int;
	 }
	udc->vbus = 0;
	if (spmp_udc_debugfs_root) {
		udc->regs_info = debugfs_create_file("registers", S_IRUGO,
				spmp_udc_debugfs_root,
				udc, &spmp_udc_debugfs_fops);
		if (!udc->regs_info)
			dev_warn(dev, "debugfs file creation failed\n");
	}

	dev_dbg(dev, "probe ok\n");
    dprintk(DEBUG_NORMAL, "probe ok\n");
	return 0;

err_int:
	free_irq(IRQ_USB_DEV, udc);
err_map:
	iounmap(base_addr);
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
	unsigned int irq;

	dev_dbg(&pdev->dev, "%s()\n", __func__);
	if (udc->driver)
		return -EBUSY;

	debugfs_remove(udc->regs_info);
	free_irq(IRQ_USB_DEV, udc);

	iounmap(base_addr);
	release_mem_region(rsrc_start, rsrc_len);

	platform_set_drvdata(pdev, NULL);

	if (!IS_ERR(udc_clock) && udc_clock != NULL) {
		clk_disable(udc_clock);
		clk_put(udc_clock);
		udc_clock = NULL;
	}

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

	spmp_udc_debugfs_root = debugfs_create_dir(gadget_name, NULL);
	if (IS_ERR(spmp_udc_debugfs_root)) {
		printk(KERN_ERR "%s: debugfs dir creation failed %ld\n",
			gadget_name, PTR_ERR(spmp_udc_debugfs_root));
		spmp_udc_debugfs_root = NULL;
	}

	retval = platform_driver_register(&udc_driver_spmp);
	if (retval)
		goto err;

	return 0;

err:
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
