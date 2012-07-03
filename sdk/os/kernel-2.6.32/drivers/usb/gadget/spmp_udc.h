#ifndef _SPMP_UDC_H
#define _SPMP_UDC_H

struct spmp_ep {
	struct list_head		queue;
	unsigned long			last_io;	/* jiffies timestamp */
	struct usb_gadget		*gadget;
	struct spmp_udc		*dev;
	const struct usb_endpoint_descriptor *desc;
	struct usb_ep			ep;
	u8				num;

	unsigned short			fifo_size;
	u8				bEndpointAddress;
	u8				bmAttributes;

	unsigned			halted : 1;
	unsigned			already_seen : 1;
	unsigned			setup_stage : 1;
};


/* Warning : ep0 has a fifo of 16 bytes */
/* Don't try to set 32 or 64            */
/* also testusb 14 fails  wit 16 but is */
/* fine with 8                          */
#define EP0_FIFO_SIZE		 64    // control Endpoint
#define EP_FIFO_SIZE		 64   

#define EP12_FIFO_SIZE64		64    //full speed
#define EP12_FIFO_SIZE512		512  //high speed
#define EP3_FIFO_SIZE		64          //interrupt in Endpoint
#define DEFAULT_POWER_STATE	0x00

#define S3C2440_EP_FIFO_SIZE	128

static const char ep0name [] = "ep0";


#define SPMP_MAXENDPOINTS      6

struct spmp_request {
	struct list_head		queue;		/* ep's requests */
	struct usb_request		req;
};

enum ep0_state {
        EP0_IDLE,
        EP0_IN_DATA_PHASE,
        EP0_OUT_DATA_PHASE,
        EP0_END_XFER,
        EP0_STALL,
};

static const char *ep0states[]= {
        "EP0_IDLE",
        "EP0_IN_DATA_PHASE",
        "EP0_OUT_DATA_PHASE",
        "EP0_END_XFER",
        "EP0_STALL",
};

struct spmp_udc {
	spinlock_t			lock;

	struct spmp_ep		ep[SPMP_MAXENDPOINTS];
	int				address;
	struct usb_gadget		gadget;
	struct usb_gadget_driver	*driver;
	struct spmp_request		fifo_req;
	u8				fifo_buf[EP_FIFO_SIZE];
	u16				devstatus;

	u32				port_status;
	int				ep0state;

	unsigned			got_irq : 1;

	unsigned			req_std : 1;
	unsigned			req_config : 1;
	unsigned			req_pending : 1;
	u8				vbus;
	struct dentry			*regs_info;
	struct tasklet_struct dma_task;	
};

#endif
