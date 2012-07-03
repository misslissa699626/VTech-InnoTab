
//#include <linux/io.h>
//#include <linux/fs.h>
#include <linux/kthread.h>
#include <linux/module.h> 
#if 0
#include <linux/hdreg.h> 		/* HDIO_GETGEO */
#include <linux/blkdev.h>
#include <linux/scatterlist.h>
#include <mach/gp_sd.h>
#include <mach/gp_gpio.h>
#include <mach/hal/hal_sd.h>
#include <mach/gp_apbdma0.h>
#endif

//#include <linux/module.h>
#include <linux/dma-mapping.h>
//#include <linux/init.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
//#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/scatterlist.h>

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/errno.h>

#include <asm/system.h>
#include <asm/io.h>
#include <asm/cacheflush.h>
#include <linux/mmc/host.h>
#include <mach/hardware.h>
#include <mach/regs-scu.h>
#include <mach/regs-interrupt.h>
#include <mach/regs-sd.h>
#include <mach/regs-dma.h>
#include <mach/spmp_dma.h>
#include <mach/sd.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <mach/gp_gpio.h>
#include <mach/hal/hal_gpio.h>
#include <mach/regs-gpio.h>

#include <mach/gp_board.h>
#include <mach/module.h>

#define DRIVER_NAME "spmp-mci"

#define SDIO_TASK_RESP_COM 0x01
#define SDIO_TASK_SEND_PIO  0x02
#define SDIO_TASK_RECV_PIO  0x04
struct pmpmci_host {
	struct mmc_host *mmc;
	struct mmc_request *mrq;

	int flags;
	void __iomem		*iobase;	
	void __iomem		*iobase_phy;		
	struct clk		*clk;
	int			cmd_is_stop;

	int status;

	struct {
		int dir;		
		int sgmap_len;
		int dmatogo;
		int offset; 
		int prexfer ; 
		int totalxfer;		
	} dma;

	struct {
		int index;
		int offset;
		int len;
	} pio;

	int use_dma;

	int irq;

	struct tasklet_struct finish_task;
	struct tasklet_struct data_task;
	struct sd_data_s *platdata;
	struct platform_device *pdev;
	struct resource *ioarea;
	struct request_queue 	*queue;    			/* The device request queue */
	struct task_struct		*thread;
	struct semaphore		thread_sem;
       unsigned int thread_task_flags;
};

/* Status flags used by the host structure */
#define HOST_F_XMIT	0x0001
#define HOST_F_RECV	0x0002
#define HOST_F_DMA	0x0010
#define HOST_F_ACTIVE	0x0100
#define HOST_F_STOP	0x1000

#define HOST_S_IDLE	0x0001
#define HOST_S_CMD	0x0002
#define HOST_S_DATA	0x0003
#define HOST_S_STOP	0x0004
#define HOST_S_DATA_SEND 0x0005
#define HOST_S_CMD_COM 0x0006
#define DBG_LINE() printk("\n%s,%s,%d.", __FILE__, __FUNCTION__, __LINE__);
static void pmpmci_send_request(struct mmc_host *mmc);
extern void wifiPowerOn(int enable);
static void pmpmci_data_complete(struct pmpmci_host *host, unsigned int status);
extern void halSd_DumpReg(struct sd_info_s *sd_info);
/**
 * @brief   Sync cache
 */
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>
static void sync_cache(void)
{
	unsigned long oldIrq;

	local_irq_save(oldIrq);
	flush_cache_all();
	flush_tlb_all();
	local_irq_restore(oldIrq);
}

#define SD_IN_DRIVE
#ifdef SD_IN_DRIVE
/*****************************************************************
 * SD / SDIO Device Info
*****************************************************************/
static struct sd_data_s spmpmci0_platdata = {
	.info = {
	         .device_id = 0,
             .p_addr = 0,
	         .v_addr = 0,
	         .dma_chan = -1,
	         .is_irq = 1,
	         .is_dmairq = 1,
	         .is_detectirq = 0,
	         .is_readonly = 0,
	         .clk_rate = 0,
	         .clk_div = 2,
	         .max_clkdiv = 256,
	         .real_rate = 0,
	         .dma_cb = NULL,
	         .detect_chan = -1,
	         .detect_delay = 20,
	         .detect_cb = NULL,
	},
	.ops = &spmpmci_ops0,
};

static struct resource spmp_resources_mci1[] = {
	[0] = {
		.start	= 0x92B0C000,
		.end	= 0x92B0CFFF,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_SD1,
		.end	= IRQ_SD1,
		.flags	= IORESOURCE_IRQ,
	},
};

static u64 spmp_mci1_dma_mask = DMA_BIT_MASK(32);
struct platform_device spmp_mci1_device = {
	.name	= "spmp-mci",
	.id		= 1,
	.dev	= {
		.platform_data		= &spmpmci0_platdata,
		.dma_mask = &spmp_mci1_dma_mask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
	.num_resources	= ARRAY_SIZE(spmp_resources_mci1),
	.resource	= spmp_resources_mci1,
};

int spmp_regdev_sd1(void)
{
    int ret;
	ret = platform_device_register(&spmp_mci1_device);
	if (ret)
		dev_err(&(spmp_mci1_device.dev), "unable to register device: %d\n", ret);
	return ret;
}

void spmp_initial_sdio_gpio()
{
#if 0   
	UINT32	gpio_info;

	/* USB Host Power Enable : B_NAND15 */
	gpio_info = 0;	/*GPIO0*/
	gpio_info <<= 8;
	gpio_info |= 2;	/* Function : 2*/
	gpio_info <<= 8;
	gpio_info |= 27;	/* GID : 27 */
	gpio_info <<= 8;
	gpio_info |= 21;	/* Pin */
	gpHalGpioSetPadGrp( gpio_info );
	gpHalGpioSetDirection( gpio_info, 0 );
	gpHalGpioSetValue( gpio_info,1 );

       //mdelay(100);
        /*power off*/
	/* WiFi_PWR_ON : B_KEYSCAN1 */
	gpio_info = 0;	/*GPIO0*/
	gpio_info <<= 8;
	gpio_info |= 0;	/* Function : 0 */
	gpio_info <<= 8;
	gpio_info |= 19;	/* GID : 19 */
	gpio_info <<= 8;
	gpio_info |= 1;	/* Pin */
	gpHalGpioSetPadGrp( gpio_info );
	gpHalGpioSetDirection( gpio_info, 0 );
	gpHalGpioSetValue( gpio_info,0 );

      /*disable wlan(high)*/
	/* WL_EN : B_SD_CARD1 */
	gpio_info = 0;	/*GPIO0*/
	gpio_info <<= 8;
	gpio_info |= 2;	/* Function : 2 */
	gpio_info <<= 8;
	gpio_info |= 30;	/* GID : 30 */
	gpio_info <<= 8;
	gpio_info |= 24;	/* Pin */
	gpHalGpioSetPadGrp( gpio_info );
	gpHalGpioSetDirection( gpio_info, 0 );
	gpHalGpioSetValue( gpio_info,1 );

      /*not reset(low)*/
	/* WL_REG_ON : B_SD_CARD3 */
	gpio_info = 0;	/*GPIO0*/
	gpio_info <<= 8;
	gpio_info |= 2;	/* Function : 2 */
	gpio_info <<= 8;
	gpio_info |= 34;	/* GID : 34 */
	gpio_info <<= 8;
	gpio_info |= 26;	/* Pin */
	gpHalGpioSetPadGrp( gpio_info );
	gpHalGpioSetDirection( gpio_info, 0 );
	gpHalGpioSetValue( gpio_info,0 );
        mdelay(200);

        /*power on*/
	/* WiFi_PWR_ON : B_KEYSCAN1 */
	gpio_info = 0;	/*GPIO0*/
	gpio_info <<= 8;
	gpio_info |= 0;	/* Function : 0 */
	gpio_info <<= 8;
	gpio_info |= 19;	/* GID : 19 */
	gpio_info <<= 8;
	gpio_info |= 1;	/* Pin */
	gpHalGpioSetPadGrp( gpio_info );
	gpHalGpioSetDirection( gpio_info, 0 );
	gpHalGpioSetValue( gpio_info,1);
        //mdelay(200);

        /*wlan enable(low)*/
	/* WL_EN : B_SD_CARD1 */
	gpio_info = 0;	/*GPIO0*/
	gpio_info <<= 8;
	gpio_info |= 2;	/* Function : 2 */
	gpio_info <<= 8;
	gpio_info |= 30;	/* GID : 30 */
	gpio_info <<= 8;
	gpio_info |= 24;	/* Pin */
	gpHalGpioSetPadGrp( gpio_info );
	gpHalGpioSetDirection( gpio_info, 0 );
	gpHalGpioSetValue( gpio_info,1 );
        //mdelay(200);

	/* Switch SDIO Function : B_KEYSCAN_2 */
	gpio_info = 0;	/*GPIO0*/
	gpio_info <<= 8;
	gpio_info |= 2;	/* Function : 2 */
	gpio_info <<= 8;
	gpio_info |= 20;	/* GID : 20 */
	gpio_info <<= 8;
	gpio_info |= 2;	/* Pin 2 */
	gpHalGpioSetPadGrp( gpio_info );

	/* Switch SDIO Function : B_KEYSCAN_3 */
	gpio_info = 0;	/*GPIO0*/
	gpio_info <<= 8;
	gpio_info |= 2;	/* Function : 2 */
	gpio_info <<= 8;
	gpio_info |= 21;	/* GID : 21 */
	gpio_info <<= 8;
	gpio_info |= 3;	/* Pin 3 */
	gpHalGpioSetPadGrp( gpio_info );

	/* Switch SDIO Function : B_KEYSCAN_4/5 */
	gpio_info = 0;	/*GPIO0*/
	gpio_info <<= 8;
	gpio_info |= 2;	/* Function : 2 */
	gpio_info <<= 8;
	gpio_info |= 22;	/* GID : 22 */
	gpio_info <<= 8;
	gpio_info |= 4;	/* Pin 4 */
	gpHalGpioSetPadGrp( gpio_info );

	/* Switch SDIO Function : B_KEYSCAN_6 */
	gpio_info = 0;	/*GPIO0*/
	gpio_info <<= 8;
	gpio_info |= 2;	/* Function : 2 */
	gpio_info <<= 8;
	gpio_info |= 23;	/* GID : 23 */
	gpio_info <<= 8;
	gpio_info |= 6;	/* Pin 6 */
	gpHalGpioSetPadGrp( gpio_info );

	/* Switch SDIO Function : B_KEYSCAN_7 */
	gpio_info = 0;	/*GPIO0*/
	gpio_info <<= 8;
	gpio_info |= 2;	/* Function : 2 */
	gpio_info <<= 8;
	gpio_info |= 24;	/* GID : 24 */
	gpio_info <<= 8;
	gpio_info |= 7;	/* Pin 7 */
	gpHalGpioSetPadGrp( gpio_info );
#else
	gp_board_sd_t *sdio_power;

	sdio_power = gp_board_get_config("sd0", gp_board_sd_t );

	if ( sdio_power->get_func() != 1 ) {		/* 0: SD, 1: SDIO, 0xff: no used */
		sdio_power = gp_board_get_config("sd1", gp_board_sd_t );
	}

	//sdio_power->set_standby(1);		/* 0: Normal, 1: Standby */
	//sdio_power->set_power(0);
	mdelay(200);
	
	sdio_power->set_standby(0);		/* 0: Normal, 1: Standby */
	sdio_power->set_power(1);
	sdio_power->set_io( 2, 0 );			/* SD I/F 0: output, 1: input, 2: sd/sdio */
	mdelay(200);
#endif
	spmp_regdev_sd1();

}
#endif

static inline void SEND_STOP(struct pmpmci_host *host)
{
	struct mmc_request *mrq = host->mrq;	
    struct sd_data_s *sd_data= host->platdata;	
	unsigned int cmdinfo = 0,ret ;
	
    printk("\n%s(),%d.", __FUNCTION__, __LINE__);
	host->cmd_is_stop = 1;
	
	host->status = HOST_S_STOP;
    host->flags &= ~(HOST_F_XMIT | HOST_F_RECV);
    cmdinfo  = SD_MMC_RSP_R1B;
	sd_data->ops->intrpt_enable(&(sd_data->info), SD_MMC_INT_CMDCOM | SD_MMC_INT_CMDBUFFULL , 1);		
	ret = sd_data->ops->sendTxCmd(&(sd_data->info), mrq->stop->opcode, mrq->stop->arg, cmdinfo);
//	pmpmci_send_request(host->mmc);
}

static int pmpmci_card_inserted(struct mmc_host *mmc)
{

	struct pmpmci_host *host = mmc_priv(mmc);
    struct sd_data_s *sd_data= host->platdata;
	if(sd_data->info.is_detectirq) 
	{
        mmc->caps &= ~(MMC_CAP_NEEDS_POLL);
	}
	
	if (sd_data && sd_data->ops->is_Insert)
	{
		return !!sd_data->ops->is_Insert(&(sd_data->info));
	}
	return -ENOSYS;	
}

static int pmpmci_card_readonly(struct mmc_host *mmc)
{
	struct pmpmci_host *host = mmc_priv(mmc);
    struct sd_data_s *sd_data= host->platdata;
	if (sd_data && sd_data->ops->is_readonly)
		return !!sd_data->ops->is_readonly(&(sd_data->info));

	return -ENOSYS;
}



static void pmpmci_finish_request(struct pmpmci_host *host)
{
	struct mmc_request *mrq = host->mrq;
	host->mrq = NULL;
	host->flags &= HOST_F_ACTIVE;

	host->dma.sgmap_len = 0;
	host->dma.dir = 0;
	host->dma.dmatogo = 0;
	host->dma.totalxfer = 0;
	host->dma.offset = 0;	
	host->dma.prexfer =0; 	
 	
	
	host->pio.index  = 0;
	host->pio.offset = 0;
	host->pio.len = 0;

    //printk("\n%s(),%d.", __FUNCTION__, __LINE__);
	host->status = HOST_S_IDLE;
	mmc_request_done(host->mmc, mrq);
}

static void pmpmci_tasklet_finish(unsigned long param)
{
	struct pmpmci_host *host = (struct pmpmci_host *) param;
	pmpmci_finish_request(host);
}
static void pmpmci_send_pio(struct pmpmci_host *host)
{
	struct mmc_data *data;
    struct sd_data_s *sd_data= host->platdata;		
	int sg_len, max, count;
	unsigned int *sg_ptr;

	   

	struct scatterlist *sg;
	data = host->mrq->data;

	if (!(host->flags & HOST_F_XMIT))
		return;

	sg = &data->sg[host->pio.index];
	sg_ptr = sg_virt(sg) + host->pio.offset;

	sg_len = data->sg[host->pio.index].length - host->pio.offset;

	max = (sg_len > host->pio.len) ? host->pio.len : sg_len;

    count = sd_data->ops->transData(&(sd_data->info),
                                               SD_DATA_FIFO | SD_DATA_WRITE,
                                               (void *)sg_ptr,
                                               max);	  
	
	host->pio.len -= count;
	host->pio.offset += count;
	if (count == sg_len) {
		host->pio.index++;
		host->pio.offset = 0;
	}

	if (host->pio.len == 0) {
		sd_data->ops->intrpt_enable(&(sd_data->info), 
			         SD_MMC_INT_DATABUFEMPTY ,
			         0);			
        host->flags &= ~(HOST_F_XMIT | HOST_F_RECV);		
    //printk("\n%s(),%d.", __FUNCTION__, __LINE__);
		//tasklet_schedule(&host->data_task);
		pmpmci_data_complete(host, sd_data->ops->getStatus(&(sd_data->info)));
	}
}

static void pmpmci_receive_pio(struct pmpmci_host *host)
{
	struct mmc_data *data;
    struct sd_data_s *sd_data= host->platdata;		
	int max, count, sg_len = 0;
	unsigned int *sg_ptr = NULL;
	struct scatterlist *sg;

	data = host->mrq->data;
	if (!(host->flags & HOST_F_RECV))
		return;

	max = host->pio.len;

	if (host->pio.index < host->dma.sgmap_len) {
		sg = &data->sg[host->pio.index];
		sg_ptr = sg_virt(sg) + host->pio.offset;

		sg_len = sg_dma_len(&data->sg[host->pio.index]) - host->pio.offset;

		if (sg_len < max)
			max = sg_len;
	}
    count = sd_data->ops->transData(&(sd_data->info),
                                               SD_DATA_FIFO | SD_DATA_READ,
                                               (void *)sg_ptr,
                                               max);
	host->pio.len -= count;
	host->pio.offset += count;
	if (sg_len && count == sg_len) {
		host->pio.index++;
		host->pio.offset = 0;
	}

	if (host->pio.len == 0) {
		sd_data->ops->intrpt_enable(&(sd_data->info), 
			         SD_MMC_INT_DATABUFFULL ,
			         0);	
        host->flags &= ~(HOST_F_XMIT | HOST_F_RECV);	
    //printk("\n%s(),%d.", __FUNCTION__, __LINE__);
		//tasklet_schedule(&host->data_task);		
		pmpmci_data_complete(host, sd_data->ops->getStatus(&(sd_data->info)));
	}
}
static int pmpmci_prepare_dma(struct pmpmci_host *host, struct mmc_data *data)
{
	int len;
	void *trans_addr;
    struct sd_data_s *sd_data= host->platdata;	
	
	if (host->dma.dmatogo > host->dma.sgmap_len)
		return -ENOMEM;
	
    len = sg_dma_len(&data->sg[host->dma.dmatogo]);
	len = len - host->dma.offset;
	trans_addr = (void *)sg_dma_address(&data->sg[host->dma.dmatogo]) + host->dma.offset;
	len = len > 4096 ? 4096:len;	
	host->dma.prexfer = len;
	if(host->dma.prexfer == 0) return -EINVAL;	
	if(host->flags & HOST_F_RECV)
	{
        sd_data->ops->transData(&(sd_data->info),
                                               SD_DATA_DMA | SD_DATA_READ,
                                               trans_addr,
                                               len);
	}
    else
    {
         sd_data->ops->transData(&(sd_data->info),
                                               SD_DATA_DMA | SD_DATA_WRITE,
                                               trans_addr,
                                               len);
    }
	 
	return 0;
}

static int pmpmci_send_command(struct pmpmci_host *host,
				struct mmc_command *cmd, struct mmc_data *data)
{
    struct sd_data_s *sd_data= host->platdata;
	unsigned int cmdinfo = 0;
	int ret;
	switch (mmc_resp_type(cmd)) 
	{
	    case MMC_RSP_NONE:
		   cmdinfo = SD_MMC_RSP_NONE;
		break;
    	case MMC_RSP_R1:
		   cmdinfo  = SD_MMC_RSP_R1;
		break;
	    case MMC_RSP_R1B:
	       cmdinfo  = SD_MMC_RSP_R1B;
		break;
    	case MMC_RSP_R2:
    	   cmdinfo  = SD_MMC_RSP_R2;
		break;
	    case MMC_RSP_R3:
		   cmdinfo  = SD_MMC_RSP_R3;
		break;
	    default:
	    	printk("pmpmci: unhandled response type %02x\n",mmc_resp_type(cmd));
		  return -EINVAL;
    }
	
	if (data) {
		cmdinfo |= SD_MMC_CMD_WITH_DATA;		
		if (data->flags & MMC_DATA_WRITE) 
		{
			cmdinfo |= SD_MMC_TRANS_WRITE;
		}
		if (data->blocks > 1)
		{
			cmdinfo |= SD_MMC_MBLK_MULTI;		
		}
	}	
    //printk("\nsend sdio cmd=%d, arg=0x%x.", cmd->opcode, cmd->arg);
        while(sd_data->ops->getStatus(&(sd_data->info)) & MASK_S_BUSY)
            printk("\n!!SD CONTROLLER BUSY.");
	sd_data->ops->intrpt_enable(&(sd_data->info), SD_MMC_INT_CMDCOM | SD_MMC_INT_CMDBUFFULL , 1);	
	ret = sd_data->ops->sendTxCmd(&(sd_data->info), cmd->opcode, cmd->arg, cmdinfo);
	if(ret)
	{
       sd_data->ops->intrpt_enable(&(sd_data->info), SD_MMC_INT_CMDCOM | SD_MMC_INT_CMDBUFFULL , 0);	
	   return -EINVAL;
	}
    #ifdef OLD_DATE_MODE
    if (data) {
       if(host->flags & HOST_F_DMA)
       {  
          //ret = pmpmci_prepare_dma(host,data);       
       }	
	   else
	   {
		if (host->flags & HOST_F_XMIT)
		{
			//pmpmci_send_pio(host);					
    		if (host->flags & HOST_F_XMIT)
 	    	{
        	  sd_data->ops->intrpt_enable(&(sd_data->info), SD_MMC_INT_DATABUFEMPTY , 1);	
		    }
			
		}
		else if (host->flags & HOST_F_RECV)
		{
			//pmpmci_receive_pio(host);	   
         	if (host->flags & HOST_F_RECV)
		    {
        	  sd_data->ops->intrpt_enable(&(sd_data->info), SD_MMC_INT_DATABUFFULL , 1);		   
		    }								
		}
	   }
    }
    #endif
	return ret;

}

static void pmpmci_data_complete(struct pmpmci_host *host, unsigned int status)
{
	struct mmc_request *mrq = host->mrq;
    struct sd_data_s *sd_data= host->platdata;		
	struct mmc_data *data;
	unsigned int crc;
	int timeout = 100000;
    #if 1
//printk("\n%s,%s():%d, enter host stutus=0x%x.", __FILE__, __FUNCTION__, __LINE__, host->status );
	/*WARN_ON((host->status != HOST_S_DATA) && (host->status != HOST_S_STOP));*/
	while ((host->status != HOST_S_DATA_SEND) && (host->status != HOST_S_STOP) && timeout > 0) {
		timeout--;
		//tasklet_schedule(&host->data_task);
	}
	if(timeout <= 0) {		
		printk("\nSD CMD not complete, dma interrupt run, wait timeout!!");		
		return;
	}
#endif
	if (host->mrq == NULL)
		return;

	data = mrq->cmd->data;
	
	data->error = 0;
	dma_unmap_sg(mmc_dev(host->mmc), data->sg, data->sg_len, host->dma.dir);
        //sync_cache();
    /* Process any errors */
	crc = (status & SD_MMC_S_DATA_CRCERR);

	if (crc)
		data->error = -EILSEQ;

	/* Clear the CRC bits */
	sd_data->ops->clearStatus(&(sd_data->info),SD_MMC_S_DATA_CRCERR);

	data->bytes_xfered = 0;

        host->flags &= ~(HOST_F_XMIT | HOST_F_RECV);
	if (!data->error) {
		if (host->flags & HOST_F_DMA) {
			data->bytes_xfered = host->dma.totalxfer;
		} else
			data->bytes_xfered =
				(data->blocks * data->blksz) - host->pio.len;
	}
	sd_data->ops->stopTxCmd(&(sd_data->info));
	if (host->flags & HOST_F_STOP)
	{
		SEND_STOP(host);
    printk("\n%s(),%d.", __FUNCTION__, __LINE__);
		return;
	}
	pmpmci_finish_request(host);
    //printk("\n%s(),%d.", __FUNCTION__, __LINE__);
}

static void pmpmci_tasklet_data(unsigned long param)
{
	struct pmpmci_host *host = (struct pmpmci_host *)param;
    struct sd_data_s *sd_data= host->platdata;	
    unsigned int status = sd_data->ops->getStatus(&(sd_data->info));
    /*printk("\npmpmci_tasklet_data enter.");
    halSd_DumpReg(&sd_data->info);*/
	pmpmci_data_complete(host, status);
}



static void pmpmci_resp_complete(struct pmpmci_host *host, u32 status)
{
	struct mmc_request *mrq = host->mrq;
	struct mmc_command *cmd;
    struct sd_data_s *sd_data= host->platdata;

	cmd = mrq->cmd;
	cmd->error = 0;

	if (cmd->flags & MMC_RSP_PRESENT) {
		if (cmd->flags & MMC_RSP_136) {
			cmd->resp[0] = sd_data->ops->recvRxRsp(&(sd_data->info));
			cmd->resp[1] = sd_data->ops->recvRxRsp(&(sd_data->info));
			cmd->resp[2] = sd_data->ops->recvRxRsp(&(sd_data->info));
			cmd->resp[3] = sd_data->ops->recvRxRsp(&(sd_data->info));			
		} else {
			cmd->resp[0] = sd_data->ops->recvRxRsp(&(sd_data->info));
		}
	}
}

static void pmpmci_cmd_complete(struct pmpmci_host *host, u32 status)
{
	struct mmc_request *mrq = host->mrq;
	struct mmc_command *cmd;
    struct sd_data_s *sd_data= host->platdata;		
	int trans;
	//printk("\n%s,%s():%d, enter host flag=0x%x.", __FILE__, __FUNCTION__, __LINE__, host->flags);
	if (!host->mrq)
		return;

	cmd = mrq->cmd;
	cmd->error = 0;

    /* Figure out errors */
	if (status & (SD_MMC_S_DATA_CRCERR| SD_MMC_S_RSP_IDXERR| SD_MMC_S_RSP_CRCERR))
		cmd->error = -EILSEQ;

	trans = host->flags & (HOST_F_XMIT | HOST_F_RECV);

	if (!trans || cmd->error) {
		sd_data->ops->intrpt_enable(&(sd_data->info), 
			         SD_MMC_INT_DATABUFFULL | SD_MMC_INT_DATABUFEMPTY ,
			         0);	
		//tasklet_schedule(&host->finish_task);
	        sd_data->ops->stopTxCmd(&(sd_data->info));
		pmpmci_finish_request(host);
    //printk("\n%s(),%d.", __FUNCTION__, __LINE__);
		return;
	}

	host->status = HOST_S_DATA;
}




static int pmpmci_prepare_data(struct pmpmci_host *host,
				struct mmc_data *data)
{
    
	int datalen = data->blocks * data->blksz;
    struct sd_data_s *sd_data= host->platdata;		
	static int blksz = 0;
	
	if (data->flags & MMC_DATA_READ)
		host->flags |= HOST_F_RECV;
	else
		host->flags |= HOST_F_XMIT;

	if (host->mrq->stop)
		host->flags |= HOST_F_STOP;

	host->dma.dir = DMA_BIDIRECTIONAL;

	host->dma.sgmap_len = dma_map_sg(mmc_dev(host->mmc), data->sg,
				   data->sg_len, host->dma.dir);
	if (host->dma.sgmap_len == 0)
		return -EINVAL;
  
    if(blksz != data->blksz)
    {
      sd_data->ops->setBlkLen(&(sd_data->info), data->blksz);			  
      blksz = data->blksz;
    }
/*************************************************************/
	if ((host->use_dma) && ((data->blksz % 16) == 0)) {  // dma working size must 16 plus
	    if(~(host->flags & HOST_F_DMA))
	    {
          host->flags |= HOST_F_DMA;		
	    }
        host->dma.dmatogo = 0;
	    host->dma.totalxfer = 0;
    	host->dma.offset = 0;	
    	host->dma.prexfer =0; 			
//      	if(pmpmci_prepare_dma(host,data))
//			goto dataerr; 		
	} 
	else {
	    if(host->flags & HOST_F_DMA)
	    {		
          host->flags &= ~HOST_F_DMA;					  
	    }
		host->pio.index = 0;
		host->pio.offset = 0;
		host->pio.len = datalen;

		
//		if (host->flags & HOST_F_XMIT)
//        	sd_data->ops->intrpt_enable(&(sd_data->info), SD_MMC_INT_DATABUFEMPTY , 1);	
//		else
//        	sd_data->ops->intrpt_enable(&(sd_data->info), SD_MMC_INT_DATABUFFULL , 1);	
	}
	
/***************************************************/

	return 0;

dataerr:
	dma_unmap_sg(mmc_dev(host->mmc), data->sg, data->sg_len,
			host->dma.dir);
	return -ETIMEDOUT;
}

/* This actually starts a command or data transaction */
static void pmpmci_send_request(struct mmc_host *mmc)
{
	struct pmpmci_host *host = mmc_priv(mmc);
	struct mmc_request *mrq = host->mrq;
	struct mmc_command *cmd = host->cmd_is_stop ? mrq->stop : mrq->cmd;
	int ret = 0;
	
	if (cmd->data) { 
   		ret = pmpmci_prepare_data(host, cmd->data);
	}

	if (!ret)
	{
		ret = pmpmci_send_command(host, cmd, cmd->data);
	}
	if (ret) {
		cmd->error = ret;
		cmd->data->error =ret;
		pmpmci_finish_request(host);
    printk("\n%s(),%d.", __FUNCTION__, __LINE__);
	}		
}

static void pmpmci_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct pmpmci_host *host = mmc_priv(mmc);

        if(host->mrq != NULL){
            printk("\nWARNING:SD CMD REQUEST WHEN PREV NOT FINISH!\nNew cmd=%d, arg=0x%x.\nOld cmd=%d, arg=0x%x.", 
                mrq->cmd->opcode, mrq->cmd->arg, host->mrq->cmd->opcode, host->mrq->cmd->arg);
            //SEND_STOP(host);
            return;
        }
            
	host->cmd_is_stop = 0;
	host->mrq = mrq;
	host->status = HOST_S_CMD;
        pmpmci_send_request(mmc);
}

static void pmpmci_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct pmpmci_host *host = mmc_priv(mmc);
    struct sd_data_s *sd_data= host->platdata;	

	switch (ios->power_mode) {
	case MMC_POWER_ON:
	case MMC_POWER_UP:
		sd_data->ops->setClkFreq(&(sd_data->info),ios->clock);
      	switch (ios->bus_width) {
      	case MMC_BUS_WIDTH_4:
        	sd_data->ops->setBusWidth(&(sd_data->info),SD_MMC_BUS_WIDTH_4);		
	    	break;
    	case MMC_BUS_WIDTH_1:
        	sd_data->ops->setBusWidth(&(sd_data->info),SD_MMC_BUS_WIDTH_1);		
	   	 break;
    	}
	break;

	case MMC_POWER_OFF:
	default:
		printk("power off\n");
	}

}

#define STATUS_TIMEOUT (SD_STATUS_RAT | SD_STATUS_DT)
#define STATUS_DATA_IN  (SD_STATUS_NE)
#define STATUS_DATA_OUT (SD_STATUS_TH)

static irqreturn_t pmpmci_irq(int irq, void *dev_id)
{
	struct pmpmci_host *host = dev_id;
    struct sd_data_s *sd_data= host->platdata;		
	struct mmc_command *cmd;	
	u32 status;
    extern void gp_gpio_dump();


	status = sd_data->ops->getStatus(&(sd_data->info));
//printk("\ninterrupt occur:  sd irq status=%x.", status);
//halSd_DumpReg(&sd_data->info);
//gp_gpio_dump(); 
    
	if (status & SD_MMC_S_CARDINT)	/* sdio */
	{
              //printk("\nSDIO interrupt occur:  sd irq status=%x.", status);
		mmc_signal_sdio_irq(host->mmc);		
		goto irq_out;
	}
	
	if (!host->mrq) {
		printk("no active mrq\n");		
		sd_data->ops->intrpt_enable(&(sd_data->info), 
			         SD_MMC_INT_CMDCOM | SD_MMC_INT_DATACOM | SD_MMC_INT_CMDBUFFULL | SD_MMC_INT_DATABUFFULL | SD_MMC_INT_DATABUFEMPTY ,
			         0);	
		goto irq_out;
	}
	
	cmd = host->cmd_is_stop ? host->mrq->stop : host->mrq->cmd;
	
	if (!cmd) {
		printk("no active cmd\n");
		sd_data->ops->intrpt_enable(&(sd_data->info), 
			         SD_MMC_INT_CMDCOM | SD_MMC_INT_DATACOM | SD_MMC_INT_CMDBUFFULL | SD_MMC_INT_DATABUFFULL | SD_MMC_INT_DATABUFEMPTY ,
			         0);	
        goto irq_out;
	}
	
	if (host->mrq && (status & SD_MMC_S_RSP_TIMEOUT)) {
		if (status & SD_MMC_S_CMDCOM)
			host->mrq->cmd->error = -ETIMEDOUT;
		else if (status & SD_MMC_S_DATCOM)
			host->mrq->data->error = -ETIMEDOUT;
		sd_data->ops->intrpt_enable(&(sd_data->info), 
			         SD_MMC_INT_CMDCOM | SD_MMC_INT_DATACOM | SD_MMC_INT_CMDBUFFULL | SD_MMC_INT_DATABUFFULL | SD_MMC_INT_DATABUFEMPTY ,
			         0);	
		//tasklet_schedule(&host->finish_task);	
		pmpmci_finish_request(host);
    printk("\n%s(),%d.", __FUNCTION__, __LINE__);
	}
	else if ((status & SD_MMC_S_CMDCOM) && !(cmd->flags & MMC_RSP_PRESENT))   // Cmd complete without Response
	{
		if (host->status == HOST_S_CMD)
		{
		#if 0
			pmpmci_cmd_complete(host, status);
		    sd_data->ops->intrpt_enable(&(sd_data->info), 
			         SD_MMC_INT_CMDCOM | SD_MMC_INT_CMDBUFFULL,
			         0);		
            #else
		    sd_data->ops->intrpt_enable(&(sd_data->info), 
			         SD_MMC_INT_CMDCOM | SD_MMC_INT_CMDBUFFULL,
			         0);		
                host->status  = HOST_S_CMD_COM;
                if(host->thread_task_flags)
                    printk("\n!!!WARNING:last thread_task_flags not clear!!!.");
                    host->thread_task_flags |= status;
        		wake_up_process(host->thread);
            #endif
		}
	}
	else if ((status & SD_MMC_S_CMDBUFFULL) && (cmd->flags & MMC_RSP_PRESENT)) // Cmd complete with Response
	{
		if ((host->status == HOST_S_CMD) || (host->status == HOST_S_STOP))
		{
		#if 0
     		pmpmci_resp_complete(host, status);	
         	/*while(!(sd_data->ops->getStatus(&(sd_data->info)) & SD_MMC_S_CMDCOM));*/
			pmpmci_cmd_complete(host, status);
		    sd_data->ops->intrpt_enable(&(sd_data->info), 
			         SD_MMC_INT_CMDCOM | SD_MMC_INT_CMDBUFFULL,
			         0);	
		#ifndef OLD_DATA_MODE
        if(host->flags & (HOST_F_XMIT | HOST_F_RECV)) {
            //printk("\nwake dat thread in irq, flags=%x.", host->flags);
        if(host->thread_task_flags)
            printk("\n!!!WARNING:last thread_task_flags not clear!!!.");
            host->thread_task_flags |= status;
		wake_up_process(host->thread);
            }
            #endif
            #else
		    sd_data->ops->intrpt_enable(&(sd_data->info), 
			         SD_MMC_INT_CMDCOM | SD_MMC_INT_CMDBUFFULL,
			         0);	
                host->status  = HOST_S_CMD_COM;
        if(host->thread_task_flags)
            printk("\n!!!WARNING:last thread_task_flags not clear!!!.");
            host->thread_task_flags |= status;
		wake_up_process(host->thread);
            #endif
		}
	} 
    #if 0
	else if (!(host->flags & HOST_F_DMA)) {
		  if ((host->flags & HOST_F_XMIT) && (status & SD_MMC_S_DATABUFEMPTY)){
		#ifndef OLD_DATA_MODE
            host->thread_task_flags |= SDIO_TASK_SEND_PIO;
		wake_up_process(host->thread);
        #else
			pmpmci_send_pio(host);
        #endif
		    }
          else if ((host->flags & HOST_F_RECV) && (status & SD_MMC_S_DATABUFFULL)){
		#ifndef OLD_DATA_MODE
            host->thread_task_flags |= SDIO_TASK_RECV_PIO;
		wake_up_process(host->thread);
        #else
			pmpmci_receive_pio(host);
        #endif
            }

	}
    #endif
irq_out:
   	sd_data->ops->clearStatus(&(sd_data->info),SD_MMC_S_CLRALL);
	return IRQ_HANDLED;
}

static void pmpmci_dma_cbk(int dma, void *devid)
{
	struct pmpmci_host *host = devid;
   	struct mmc_request *mrq; 
    struct sd_data_s *sd_data= host->platdata;	
	int len;
    /**///printk("\n%s,%s():%d, enter.", __FILE__, __FUNCTION__, __LINE__);
	/* Avoid spurious interrupts */
	if (!host->mrq /*|| host->status != HOST_S_DATA_SEND*/)
		return;
	host->status = 	HOST_S_DATA;
        wake_up_process(host->thread);
        #if 0
    mrq = host->mrq;
	len = sg_dma_len(&(mrq->cmd->data->sg[host->dma.dmatogo]));

   	host->dma.offset += host->dma.prexfer;
    host->dma.totalxfer += host->dma.prexfer;	
	if(len == host->dma.offset)
	{
	  host->dma.offset = 0;
   	  host->dma.dmatogo++;	
	}
	if(host->dma.dmatogo == host->dma.sgmap_len)
 	{
	   //tasklet_schedule(&host->data_task);	   
		pmpmci_data_complete(host, sd_data->ops->getStatus(&(sd_data->info)));
	}
	else
	{
	   pmpmci_prepare_dma(host,mrq->cmd->data);
	}
    #endif
}

static void pmpmci_detect_cbk(int irq, void *devid)
{
	struct pmpmci_host *host = devid;
    struct sd_data_s *sd_data= host->platdata;		
    if(host->mmc->caps & MMC_CAP_NEEDS_POLL)
    	host->mmc->caps &= ~(MMC_CAP_NEEDS_POLL);
	mmc_detect_change(host->mmc, sd_data->info.detect_delay);	
}

static void pmpmci_enable_mcio_irq(struct mmc_host *mmc, int en)
{
  
}

void pmpmci_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
    struct pmpmci_host *host = mmc_priv(mmc);
    struct sd_data_s *sd_data= host->platdata;

    /*printk("\npmpmci_enable_sdio_irq=%d.", enable);*/
    sd_data->ops->intrpt_enable(&(sd_data->info), SD_MMC_INT_SDIODETECT, enable);
}

    
/**
* @brief 	Request thread function
* @param 	d[in]: Private data.
* @return 	SUCCESS/ERROR_ID.
*/ 
static int gp_sdiocard_queue_thread(void *d)
{
        
	struct pmpmci_host *host = d;
        unsigned int status;
	//struct request_queue *q = host->queue;
       struct mmc_request *req;
    struct sd_data_s *sd_data= host->platdata;	
	int len;
	
	current->flags |= PF_MEMALLOC;
	
	down(&host->thread_sem);
	do 
	{		
		status = host->thread_task_flags;
                host->thread_task_flags = 0;
		//spin_lock_irq(q->queue_lock);
		set_current_state(TASK_INTERRUPTIBLE);
		/*if (!blk_queue_plugged(q))
			req = blk_fetch_request(q);
		host->req = req;*/
		req = host->mrq;
		//spin_unlock_irq(q->queue_lock);
	    //printk("\ngp_sdiocard_queue_thread host status=%x.", host->status);
		
		if (!req || (host->status != HOST_S_DATA &&  host->status != HOST_S_CMD_COM))
		{
	    //printk("\ngp_sdiocard_queue_thread host status 2=%x.", host->status);
			if (kthread_should_stop())
			{
				set_current_state(TASK_RUNNING);
				break;
			}
			up(&host->thread_sem);
			schedule();
			down(&host->thread_sem);
			continue;
		}
		set_current_state(TASK_RUNNING);
		
		/*gp_sdcard_xfer_request(host, req);*/
            if(host->status == HOST_S_DATA){
               // printk("\ngp_sdio_thread_deal data host flag=0x%x.", host->flags);
       		if (host->flags & HOST_F_DMA)
       		{
                    host->status = HOST_S_DATA_SEND;
                        //printk("\ngp_sdio_thread_deal data DMA.");
       		    //pmpmci_prepare_dma(host,host->mrq->data);
                        	len = sg_dma_len(&(req->cmd->data->sg[host->dma.dmatogo]));

                           	host->dma.offset += host->dma.prexfer;
                            host->dma.totalxfer += host->dma.prexfer;	
                        	if(len == host->dma.offset)
                        	{
                        	  host->dma.offset = 0;
                           	  host->dma.dmatogo++;	
                        	}
                        	if(host->dma.dmatogo == host->dma.sgmap_len)
                         	{
                        	   //tasklet_schedule(&host->data_task);	   
                        		pmpmci_data_complete(host, sd_data->ops->getStatus(&(sd_data->info)));
                                    //host->flags &= ~(HOST_F_XMIT | HOST_F_RECV);
                        	}
                        	else
                        	{
                        	   pmpmci_prepare_dma(host,req->cmd->data);
                        	}
                    //host->flags &= ~(HOST_F_XMIT | HOST_F_RECV);
       		}
                    else if((host->flags & HOST_F_XMIT) /*&& (status & SD_MMC_S_DATABUFEMPTY)*/){
                    host->status = HOST_S_DATA_SEND;
                        //printk("\ngp_sdio_thread_deal data SEND PIO.");
			pmpmci_send_pio(host);
                    //host->flags &= ~(HOST_F_XMIT | HOST_F_RECV);
                    }
                    else if((host->flags & HOST_F_RECV) /*&& (status & SD_MMC_S_DATABUFFULL)*/){
                    host->status = HOST_S_DATA_SEND;
                        //printk("\ngp_sdio_thread_deal data RECV PIO.");
			pmpmci_receive_pio(host);
                    //host->flags &= ~(HOST_F_XMIT | HOST_F_RECV);
                    }	
	    //printk("\ngp_sdiocard_queue_thread host status 3=%x.", host->status);	
            }
            else if(host->status == HOST_S_CMD_COM) {
                if (req->cmd->flags & MMC_RSP_PRESENT){
     		        pmpmci_resp_complete(host, status);	
                 }
         	/*while(!(sd_data->ops->getStatus(&(sd_data->info)) & SD_MMC_S_CMDCOM));*/
			pmpmci_cmd_complete(host, status);
            }
            
	} while (1);
	up(&host->thread_sem);
	return 0;
}

static const struct mmc_host_ops pmpmci_ops = {
	.request	= pmpmci_request,
	.set_ios	= pmpmci_set_ios,
	.get_ro		= pmpmci_card_readonly,
	.get_cd		= pmpmci_card_inserted,
	.enable_sdio_irq = pmpmci_enable_sdio_irq,
};
struct pmpmci_host* g_pmpmci_host = NULL;
extern struct platform_device spmp_mci1_device;
void wifiPowerOn(int enable)
{
#if 0
	UINT32	gpio_info;

	/* WiFi_PWR_ON : B_KEYSCAN1 */
	gpio_info = 0;	/*GPIO0*/
	gpio_info <<= 8;
	gpio_info |= 0;	/* Function : 0 */
	gpio_info <<= 8;
	gpio_info |= 19;	/* GID : 19 */
	gpio_info <<= 8;
	gpio_info |= 1;	/* Pin */
	gpHalGpioSetPadGrp( gpio_info );
	gpHalGpioSetDirection( gpio_info, 0 );
	gpHalGpioSetValue( gpio_info,enable?1:0 );
#else
	UINT32	gpio_info;
	gp_board_sd_t *sdio_power;

	sdio_power = gp_board_get_config("sd1", gp_board_sd_t );
	sdio_power->set_power(enable);
#endif
}
static int __devinit pmpmci_probe(struct platform_device *pdev)
{
	struct mmc_host *mmc;
	struct pmpmci_host *host;
	struct resource *r;
	int ret;

	mmc = mmc_alloc_host(sizeof(struct pmpmci_host), &pdev->dev);
	if (!mmc) {
		dev_err(&pdev->dev, "no memory for mmc_host\n");
		ret = -ENOMEM;
		goto out0;
	}
	host = mmc_priv(mmc);
	host->mmc = mmc;
	host->platdata = pdev->dev.platform_data;
	host->pdev = pdev;
	ret = -ENODEV;

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r) {
		dev_err(&pdev->dev, "no mmio defined\n");
		goto out1;
	}
	host->iobase_phy=r->start;	
	host->ioarea = request_mem_region(r->start, resource_size(r),
					   DRIVER_NAME);
	if (!host->ioarea) {
		dev_err(&pdev->dev, "mmio already in use\n");
		goto out1;
	}

	host->iobase = ioremap(r->start, resource_size(r));		
	if (!host->iobase) {
		dev_err(&pdev->dev, "cannot remap mmio\n");
		goto out2;
	}
	r = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!r) {
		dev_err(&pdev->dev, "no IRQ defined\n");
		goto out3;
	}

	host->irq = r->start;
	host->irq = IRQ_SD1;	
	/* IRQ is shared among both SD controllers */
	ret = request_irq(host->irq, pmpmci_irq, 0,
			  DRIVER_NAME, host);
	if (ret) {
		dev_err(&pdev->dev, "cannot grab IRQ\n");
		goto out3;
	}

	host->clk = clk_get(&pdev->dev, "SD1");
	if (IS_ERR(host->clk)) {
		dev_err(&pdev->dev, "failed to find clock source.\n");
		ret = PTR_ERR(host->clk);
		host->clk = NULL;
		goto out4;
	}

	ret = clk_enable(host->clk);
	if (ret) {
		dev_err(&pdev->dev, "failed to enable clock source.\n");
		goto out4;
	}
	
    host->platdata->ops->init(&(host->platdata->info),(unsigned int)host->iobase_phy,(unsigned int)host->iobase);
	mmc->ops = &pmpmci_ops;
	host->platdata->info.clk_rate= clk_get_rate(host->clk);
	mmc->f_min 	= host->platdata->info.clk_rate / (host->platdata->info.clk_div * host->platdata->info.max_clkdiv);
	mmc->f_max 	= host->platdata->info.clk_rate / host->platdata->info.clk_div;
	
	mmc->max_blk_count = 4095;
	mmc->max_blk_size  = 4095;	
	mmc->max_seg_size  = 4095 * 512;
	mmc->max_req_size  = 4095 * 512;	
	mmc->max_hw_segs   = 128;		
	mmc->max_phys_segs = 128;
	
	mmc->ocr_avail = MMC_VDD_32_33 | MMC_VDD_33_34;
	mmc->caps = MMC_CAP_4_BIT_DATA;
    mmc->caps |= MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED;
	host->status = HOST_S_IDLE;

    host->platdata->ops->setDma(&(host->platdata->info),pmpmci_dma_cbk,host);	
    host->platdata->ops->setDetect(&(host->platdata->info),pmpmci_detect_cbk,host);		
    mmc->caps |= MMC_CAP_NEEDS_POLL;  //first time use poll detect
    mmc->caps |= MMC_CAP_SDIO_IRQ;

/* ----- Enable thread ----- */
		init_MUTEX(&host->thread_sem);
host->thread = kthread_run(gp_sdiocard_queue_thread, host, "sdio-qd");
if (IS_ERR(host->thread)) 
{
	goto out4;
}
	tasklet_init(&host->data_task, pmpmci_tasklet_data,
			(unsigned long)host);

	tasklet_init(&host->finish_task, pmpmci_tasklet_finish,
			(unsigned long)host);
	
	
	ret = mmc_add_host(mmc);
	if (ret) {
		dev_err(&pdev->dev, "cannot add mmc host\n");
		goto out6;
	}
//	host->dma_chan = spmp_request_dma(APBDMAC_SYSTEM,DRIVER_NAME, pmpmci_dma_irq ,host);
//	if (host->dma_chan >= 0) {
//      host->flags |= HOST_F_DMA;				
//	}
//host->dma_chan = -1;
    host->use_dma= host->platdata->ops->is_Dma(&(host->platdata->info));
    if(host->use_dma)
    {
       host->flags |= HOST_F_DMA;				
    }
	platform_set_drvdata(pdev, host);

	printk(KERN_INFO DRIVER_NAME ": MMC Controller %d set up at %8.8X"
		" (mode=%s)\n", pdev->id, host->iobase,
		host->flags & HOST_F_DMA ? "dma" : "pio");

	return 0;	/* all ok */

out6:


	tasklet_kill(&host->data_task);
	tasklet_kill(&host->finish_task);

//	if (host->platdata && host->platdata->cd_setup &&
//	    !(mmc->caps & MMC_CAP_NEEDS_POLL))
//		host->platdata->cd_setup(mmc, 0);
out4:
	free_irq(host->irq, host);
out3:
	iounmap((void *)host->iobase);	
out2:
	release_resource(host->ioarea);
	kfree(host->ioarea);	
out1:
	mmc_free_host(mmc);
out0:
	return ret;
}

static int __devexit pmpmci_remove(struct platform_device *pdev)
{
	struct pmpmci_host *host = platform_get_drvdata(pdev);
    struct sd_data_s *sd_data= host->platdata;	
	if (host) {
		mmc_remove_host(host->mmc);
    	clk_disable(host->clk);
//		if (host->platdata && host->platdata->cd_setup &&
//		    !(host->mmc->caps & MMC_CAP_NEEDS_POLL))
//			host->platdata->cd_setup(host->mmc, 0);
//
        sd_data->ops->deInit(&(sd_data->info));
		tasklet_kill(&host->data_task);
		tasklet_kill(&host->finish_task);

		free_irq(host->irq, host);
		iounmap((void *)host->iobase);
		release_resource(host->ioarea);
		kfree(host->ioarea);

		mmc_free_host(host->mmc);
		platform_set_drvdata(pdev, NULL);
	}
	return 0;
}

#define pmpmci_suspend NULL
#define pmpmci_resume NULL
static struct platform_driver pmpmci_driver = {
	.probe         = pmpmci_probe,
	.remove        = pmpmci_remove,
	.suspend       = pmpmci_suspend,
	.resume        = pmpmci_resume,
	.driver        = {
		.name  = DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

//#define SDIO_KO 1
#ifndef SDIO_KO
static int __init pmpmci_init(void)
#else
int pmpmci_init(void)
#endif
{
	printk("\npmpmci_init ENTER."); 
#ifdef SD_IN_DRIVE
    spmp_initial_sdio_gpio();
#endif
    return platform_driver_register(&pmpmci_driver);
}

#ifndef SDIO_KO
static void __exit pmpmci_exit(void)
#else
void pmpmci_exit(void)
#endif
{
	platform_driver_unregister(&pmpmci_driver);
}

#ifndef SDIO_KO
module_init(pmpmci_init);
module_exit(pmpmci_exit);

MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("Generalplus SDIO Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

#else
EXPORT_SYMBOL(pmpmci_init);
EXPORT_SYMBOL(pmpmci_exit);
#endif


