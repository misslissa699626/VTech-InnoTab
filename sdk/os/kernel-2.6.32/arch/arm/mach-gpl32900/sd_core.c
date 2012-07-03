#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/errno.h>

#include <asm/system.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/spmp_dma.h>
#include <mach/spmp_gpio.h>
#include <mach/regs-scu.h>
#include <mach/regs-sd.h>
#include <mach/regs-dma.h>
#include <mach/regs-gpio.h>
#include <mach/sd.h>
#include <mach/gp_apbdma0.h>
#include <mach/regs-dma.h>
#include <mach/spmp_dma.h>
#include <linux/dma-mapping.h>
#define HAL_NAME "SD_HAL"
#define MAX_DMA_SIZE 4096   
#define DBG_LINE() printk("\n%s,%s,%d.", __FILE__, __FUNCTION__, __LINE__);
#define SD_GPIO0_PIN (5)

const struct sd_ops_s spmpmci_ops0 = {	
	.init = halSd_Init,
	.deInit = halSd_DeInit,
	.is_readonly = halSd_is_readonly,
	.is_Insert = halSd_is_Insert,
	.is_Dma = halSd_is_Dma,
	.setClkFreq = halSd_SetClkFreq,
	.setBusWidth = halSd_SetBusWidth,
	.setBlkLen = halSd_SetBlkLen,
	.setDma = halSd_SetDma,
	.setDetect = halSd_SetDetect,
	.sendTxCmd = halSd_SendTxCmd,
	.stopTxCmd = halSd_StopTxCmd,
	.transData = halSd_TransData,
	.getStatus = halSd_GetStatus,
	.clearStatus = halSd_ClearStatus,
	.intrpt_enable = halSd_intrpt_enable,
	.recvRxRsp = halSd_RecvRxRsp,
};
void halSd_DumpReg(struct sd_info_s *sd_info);
static void hal_sdinsert_event(int pendirq, void *devid)
{
   UINT32 status;
   UINT32 tmpx;
   UINT32 gpio_status,gpio_polarity;   
   struct sd_info_s *sd_info = devid;
   tmpx = (UINT32) pendirq;
   if(tmpx & (1 << SD_GPIO0_PIN))
   {   
     halGpioGetFuncData(GPIO_STATUS,0,SD_GPIO0_PIN,&gpio_status);
     halGpioGetFuncData(GPIO_POLARITY,0,SD_GPIO0_PIN,&gpio_polarity);	
      if(gpio_polarity)	 
      	{
          halGpioSetFuncData(GPIO_POLARITY,0,SD_GPIO0_PIN,0);	
      	}
	   else
	   	{	   	
          halGpioSetFuncData(GPIO_POLARITY,0,SD_GPIO0_PIN,1);
   	   }
      if(sd_info->detect_cb)
         sd_info->detect_cb(0, sd_info->cli_detdata);
   }
}

BOOL  halSd_is_readonly(struct sd_info_s *sd_info)
{
	UINT32 status_info;
	BOOL retval = 0;
    status_info = READ32(sd_info->v_addr + SDX_STATUS);		
	retval = (status_info & MASK_S_CARDWP)? 1:0;
	printk("readonly = %08x\n",retval);
	return retval; 
}
BOOL  halSd_is_Insert(struct sd_info_s *sd_info)
{
     UINT32 gpio_status,gpio_polarity;
     UINT32 tmpx;	 

     return 1;
     halGpioGetFuncData(GPIO_STATUS,0,SD_GPIO0_PIN,&gpio_status);
     halGpioGetFuncData(GPIO_POLARITY,0,SD_GPIO0_PIN,&gpio_polarity);	  
//     printk("status polarity =%08x  %08x\n",gpio_status,gpio_polarity);
//	 sd_info->inserted = gpio_status ^ gpio_polarity;
//     return (gpio_status ^ gpio_polarity) ? 1: 0;			 
     return gpio_status ? 0: 1;
}
BOOL  halSd_is_Dma(struct sd_info_s *sd_info)
{
	 if(sd_info->dma_chan >=0) return TRUE;
	 else return FALSE;
}
BOOL  halSd_Init(struct sd_info_s *sd_info,UINT32 p_addr,UINT32 v_addr)
{
	sd_info->p_addr = p_addr;
	sd_info->v_addr = v_addr;
	sd_info->clk_rate = 0;	
	sd_info->is_readonly = FALSE;
	sd_info->is_dmairq = TRUE;
	sd_info->is_detectirq = TRUE;//clhuang TRUE;
	sd_info->is_irq = TRUE;
	sd_info->dma_chan = -1;
	sd_info->clk_div = 2;
	WRITE32(sd_info->v_addr + SDX_CONTROL,0);
	return TRUE;		
}
BOOL  halSd_DeInit(struct sd_info_s *sd_info)
{
    if (sd_info->dma_chan >= 0) {
       spmp_free_dma(APBDMAC_SYSTEM,sd_info->dma_chan);			
	   sd_info->dma_chan = -1;
    }
	return 0;
}
UINT32 halSd_SetClkFreq(struct sd_info_s *sd_info, UINT32 sd_freq)
{
   UINT32 con_val;
   UINT32 mci_psc;
   con_val = READ32(sd_info->v_addr + SDX_CONTROL);
   con_val &= ~(0xFF);   
	/* Set clock */
	for (mci_psc = 0; mci_psc < 255; mci_psc++) {
		sd_info->real_rate = sd_info->clk_rate / (sd_info->clk_div*(mci_psc+1));

		if (sd_info->real_rate <= sd_freq)
			break;
	}

	if (mci_psc > 255)
		mci_psc = 255;	

    WRITE32(sd_info->v_addr + SDX_CONTROL,mci_psc | con_val);   
	/* If requested clock is 0, real_rate will be 0, too */
	printk("\nclock = %d %d %d\n",sd_freq, sd_info->real_rate,sd_info->clk_rate);
	if (sd_freq == 0)
		sd_info->real_rate = 0;   
   return 0;
}

UINT32 halSd_SetBusWidth(struct sd_info_s *sd_info,	UINT32 busWidth)
{
   UINT32 con_val;  
   con_val = READ32(sd_info->v_addr + SDX_CONTROL);
   con_val &= ~(0x1<<8);
   switch (busWidth)
   {
     case SD_MMC_BUS_WIDTH_1:
		con_val |= MASK_C_BUSWIDTH_1;	 	
	 	break;
	 case SD_MMC_BUS_WIDTH_4:
 		con_val |= MASK_C_BUSWIDTH_4;	 		 	
	 	break;
	 default:
	 	return -1;
   }
   WRITE32(sd_info->v_addr + SDX_CONTROL,con_val);   
   return 0;
}

UINT32 halSd_SetBlkLen(struct sd_info_s *sd_info,	UINT32 blklen)
{
   UINT32 con_val;   
   con_val = READ32(sd_info->v_addr + SDX_CONTROL);
   if(blklen >= 32)
     con_val |= 0xF000;   //change Fifo level
   else
     con_val &= ~(0xF000); 
   con_val &= ~(0xFFF<<16);
   if(blklen > 0x1000) return -1;
   con_val |= (blklen<<16);
   WRITE32(sd_info->v_addr + SDX_CONTROL,con_val);   
   return 0;
}


UINT32 halSd_SetDma  (struct sd_info_s *sd_info,	dma_callback dma_cbk,void *pri_data)
{
    if(sd_info->is_dmairq)
    {
       sd_info->dma_chan = spmp_request_dma(APBDMAC_SYSTEM, HAL_NAME, dma_cbk ,pri_data);
	   sd_info->max_dma_size = MAX_DMA_SIZE;
	   if(sd_info->dma_chan >= 0)
	   {
	      sd_info->is_dmairq = 1;
		  sd_info->detect_cb = dma_cbk;
          sd_info->cli_dmadata = pri_data;	 		  
	   }
    }
	else
	{
	   sd_info->dma_chan = 0;
	   sd_info->dma_cb = dma_cbk;
       sd_info->cli_dmadata = pri_data;	 	   
	}
	return 0;
}

UINT32 halSd_SetDetect  (struct sd_info_s *sd_info,	detect_callback detect_cbk,void *pri_data)
{  
  int status;
#if 0
    SCUB_GPIO0_IE |=  (0x3F<<23);

  if((sd_info->detect_chan = spmp_request_gpio_irq(0, "sd_mmc", hal_sdinsert_event ,sd_info))>=0)
  {
     sd_info->detect_cb = detect_cbk;
     sd_info->cli_detdata = pri_data;	 
     sd_info->is_detectirq = TRUE;	
	//Enable detect sd insert remove function	 
//	halGpioSetPGS(22,0);		
    SCUB_GPIO0_IE &=  ~(0x1<<SD_GPIO0_PIN);	
	halGpioSetFuncData(GPIO_DIR,0,SD_GPIO0_PIN,0);		
    SCUB_GPIO0_IE |=  (0x1<<SD_GPIO0_PIN);	
	halGpioSetFuncData(GPIO_DIR,0,SD_GPIO0_PIN,1);				
	halGpioGetFuncData(GPIO_STATUS,0,SD_GPIO0_PIN,&status);		
	if(status)
	{  	
      halGpioSetFuncData(GPIO_POLARITY,0,SD_GPIO0_PIN,0);				
	}
	else
	{
      halGpioSetFuncData(GPIO_POLARITY,0,SD_GPIO0_PIN,1);
	}
    halGpioSetFuncData(GPIO_INT_EN,0,SD_GPIO0_PIN,1);	
    halGpioSetFuncData(GPIO_ENABLE,0,SD_GPIO0_PIN,1);	

    SCUB_GPIO0_PE	&= ~(0x1<<SD_GPIO0_PIN);			
  }	
  #endif
  return 0;
}
UINT32 halSd_SendTxCmd(struct sd_info_s *sd_info,	UINT32 cmdId, UINT32 arg, UINT32 cmdinfo)
{
    UINT32 mmcinfo;
    UINT32 mmccmd;
	mmccmd = cmdId;
	mmcinfo = cmdinfo & SD_MMC_RSP_MASK;
  	
    //printk("\n\nhalSd_SendTxCmd,cmd=%d,arg=0x%x,cmdinfo=0x%x.", cmdId, arg, cmdinfo);
	switch (mmcinfo) 
	{
	    case SD_MMC_RSP_NONE:			
		   mmccmd |= MASK_RESPTYPE0;				
		break;
    	case SD_MMC_RSP_R1:
    	case SD_MMC_RSP_R5:						
    	case SD_MMC_RSP_R7:			
		   mmccmd |= MASK_RESPTYPE1;
           break;
	    case SD_MMC_RSP_R2:
	       mmccmd |= MASK_RESPTYPE2;
		   break;
    	case SD_MMC_RSP_R3:
    	case SD_MMC_RSP_R4:			
           mmccmd |= MASK_RESPTYPE3;
     	   break;
	    case SD_MMC_RSP_R6:
		   mmccmd |= MASK_RESPTYPE6;
		   break;
	    case SD_MMC_RSP_R1B:
		   mmccmd |= MASK_RESPTYPE1b;
		   break;		
	    default:
	    	printk("pmpmci: unhandled response type %02x\n",mmcinfo);
		  return -1;
    }

    if(cmdinfo & SD_MMC_CMD_WITH_DATA)
    {
	   mmccmd |= MASK_CMD_WITHDATA;
       if(cmdinfo & SD_MMC_TRANS_WRITE)
       {
	      mmccmd |= MASK_TRANSDATA;
       }	
       if(cmdinfo & SD_MMC_MBLK_MULTI)
       {
	      mmccmd |= MASK_MULTIBLOCK;
       }			   
    }
    WRITE32(sd_info->v_addr + SDX_ARGUMENT,arg); 	
    WRITE32(sd_info->v_addr + SDX_COMMAND,mmccmd | MASK_CMD_RUN); 		
	return 0;
}

UINT32 halSd_StopTxCmd(struct sd_info_s *sd_info)
{  
    WRITE32(sd_info->v_addr + SDX_COMMAND,MASK_CMD_STOP);  
	while (READ32(sd_info->v_addr+ SDX_STATUS) & MASK_S_BUSY);
    //printk("\nhalSd_StopTxCmd out."); 
	return 0;
}
UINT32 halSd_TransData(struct sd_info_s *sd_info,UINT32 sendInfo,void *addr,UINT32 len)
{
    if(sendInfo & SD_DATA_DMA)
    {
      UINT32 dma_len;
      UINT32 con_val;
	  dma_len = len > MAX_DMA_SIZE ? MAX_DMA_SIZE:len;	  
	  
	 if(dma_len < 16) return 0;
	  
      con_val = READ32(sd_info->v_addr + SDX_CONTROL);
      con_val |= MASK_C_DMAMODE;
      WRITE32(sd_info->v_addr + SDX_CONTROL,con_val);   
      spmp_request_disable(APBDMAC_SYSTEM,   //force stop dma
        sd_info->dma_chan);
      spmp_request_address(APBDMAC_SYSTEM,
             	sd_info->dma_chan,
                DMA_BUFFERA,
                addr, dma_len);
	  if(sd_info->is_dmairq == TRUE)
	  {	  
        if(sendInfo & SD_DATA_WRITE)
        {
		    spmp_request_enable(APBDMAC_SYSTEM,
			sd_info->dma_chan,
		    (void *) (sd_info->p_addr+ SDX_DATATX),
		    DMA_M2P|DMA_REQ|DMA_FIX|DMA_SINGLE_BUF|DMA_32BIT_BURST|DMA_IRQON|DMA_ON);
        }
	    else
	    {
		    spmp_request_enable(APBDMAC_SYSTEM,
			sd_info->dma_chan,
		    (void *) (sd_info->p_addr + SDX_DATARX),
		    DMA_P2M|DMA_REQ|DMA_FIX|DMA_SINGLE_BUF|DMA_32BIT_BURST|DMA_IRQON|DMA_ON);

	    }
		return dma_len;
	  }
      else
      {
	    //block dma need add some code here
	    printk("\n\n!!!WARNING:%s,%d.",__FUNCTION__, __LINE__);
	    return 0;
	  }      
    }
	else
	{
      UINT32 con_val;
      con_val = READ32(sd_info->v_addr + SDX_CONTROL);
      con_val &= ~(MASK_C_DMAMODE);
      WRITE32(sd_info->v_addr + SDX_CONTROL,con_val);   
	  
      if(sendInfo & SD_DATA_WRITE)
      {
       	UINT32 count; 
		UINT32 *sg_ptr;
    	UINT32 status,val;
        int timeout= 600000;
		
		sg_ptr = (unsigned int *) addr;

        for (count = 0; count < (len >> 2); count++) 
		{
		  //status = READ32(sd_info->v_addr + SDX_STATUS);
                timeout= 600000;
          while (timeout>0 && !((status = READ32(sd_info->v_addr + SDX_STATUS)) & MASK_S_DATABUFEMPTY))
			timeout--;//break;
		  val = *sg_ptr++;
		  WRITE32(sd_info->v_addr + SDX_DATATX,val);
	    } 

		return (count << 2);
      }
	  else
	  {
       	UINT32 count; 
		UINT32 *sg_ptr;
    	UINT32 status,val;
        int timeout= 600000;
		
		sg_ptr = (unsigned int *) addr;

	    for (count = 0; count < (len>>2); count++) 
		{
		   //status = READ32(sd_info->v_addr + SDX_STATUS);

                timeout= 600000;
		   while (timeout>0 && !((status = READ32(sd_info->v_addr + SDX_STATUS)) & MASK_S_DATABUFFULL))
			  timeout--;//break;
		   val = READ32(sd_info->v_addr + SDX_DATARX);

		   if (sg_ptr)
			*sg_ptr++ = val;
	    }

		return (count << 2);	  
	  }	
	}
}
UINT32 halSd_GetStatus(struct sd_info_s *sd_info)
{
    UINT32 status_info,retval = 0;
    status_info = READ32(sd_info->v_addr + SDX_STATUS);		
	retval = status_info & 0x7FFF;		
	return retval;
}
UINT32 halSd_ClearStatus(struct sd_info_s *sd_info,UINT32 status_info)
{
    UINT32 retval = 0;
	WRITE32(sd_info->v_addr + SDX_STATUS,status_info);
	return retval;
}
UINT32 halSd_intrpt_enable(struct sd_info_s *sd_info,UINT32 intinfo,BOOL en)
{
   	UINT32 val,mask = 0;

    if(intinfo & SD_MMC_INT_CMDCOM)
    {
      mask |= INTEN0;
    }
    if(intinfo & SD_MMC_INT_DATACOM)
    {
      mask |= INTEN1;
    }
    if(intinfo & SD_MMC_INT_CMDBUFFULL)
    {
      mask |= INTEN2;    
    }
    if(intinfo & SD_MMC_INT_DATABUFFULL)
    {
      mask |= INTEN3;    
    }
    if(intinfo & SD_MMC_INT_DATABUFEMPTY)
    {
      mask |= INTEN4;    
    }
    if(intinfo & SD_MMC_INT_SDIODETECT)
    {
      mask |= INTEN6;    
    }
    
	val = READ32(sd_info->v_addr + SDX_INTEN);
	if(en)
      val |= mask;
	else
	  val &= (~mask);		
	WRITE32(sd_info->v_addr + SDX_INTEN,val);

    if(intinfo & SD_MMC_INT_SDIODETECT)
    {
      UINT32 con_val;
      con_val = READ32(sd_info->v_addr + SDX_CONTROL);
      if(en)
        con_val |= (MASK_C_IOMODE|MASK_C_ENSDBUS);
      else
        con_val &= ~(MASK_C_IOMODE/*|MASK_C_ENSDBUS*/);
      WRITE32(sd_info->v_addr + SDX_CONTROL,con_val);
      /*printk("\nhalSd enable SDIO interrupt");*/
    }
    
	return 0;
}
UINT32 halSd_RecvRxRsp(struct sd_info_s *sd_info)
{
   UINT32 resp; 	   
    while(!(READ32(sd_info->v_addr + SDX_STATUS) & MASK_S_CMDBUFFULL)); 	
    resp = READ32(sd_info->v_addr + SDX_RESPONSE);	
    //printk("\nhalSd_RecvRxRsp=%x.", resp);
    //halSd_DumpReg(sd_info);
   return resp;
}

void gp_hexdump(unsigned char *buf, unsigned int len)
{
    int i = 0;
    while (len--) {
        if (i % 16 == 0)
            printk("\n");
        if (i % 4 == 0)
            printk("  ");
        printk("%02x", *buf++);
        i++;        
    }
    printk("\n");
}
void halSd_DumpReg(struct sd_info_s *sd_info)
{
    DBG_LINE();
    gp_hexdump(sd_info->v_addr, 32);
}
