/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Technology Co., Ltd.         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Technology Co., Ltd. All rights are reserved by Generalplus Technology*
 *  Co., Ltd. This software may only be used in accordance with the       *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Technology Co., Ltd. reserves the right to modify this    *
 *  software without notice.                                              *
 *                                                                        *
 *  Generalplus Technology Co., Ltd.                                      *
 *  19, Innovation First Road, Science-Based Industrial Park,             *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 **************************************************************************/

 /**
 * @file gp_sdio.c
 * @brief SDIO driver interface 
 * @author clhuang
 */
 
 /*Linux header include*/
#include <mach/module.h> 
#include <mach/kernel.h>
#include <linux/platform_device.h>
#include <linux/mmc/host.h>
#include <linux/mmc/core.h>
#include <linux/scatterlist.h>
/*GP header include*/
#include <mach/diag.h>
#include <mach/hal/regmap/reg_sd.h>
#include <mach/hal/hal_sd.h>
#include <mach/gp_sd.h>
#include <mach/gp_gpio.h>

#include <mach/regs-dma.h>
#include <mach/spmp_dma.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
/* ----- SD IO define ----- */
#define GP_SDIO_DEV_ID 0x01
#define GP_SDIO_PIN_NUM 6

/*system clock define*/
#define SYS_APB_CLK 371	/* unit: 100KHz */
#define SD_CLK_UNIT 100000 /*100KHz*/

/**************************************************************************
 *                               M A C R O S                       *
 **************************************************************************/
#define DRIVER_NAME "gp_sdio"
//#define OLD_DMA
#undef DIAG_INFO
#define DIAG_INFO(...)/**/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct gp_sdio_info_s{
    int device_id;
    int power_pin;
    int wl_reg_on_pin;
    int wl_en_pin;
    int sdio_pin[GP_SDIO_PIN_NUM];
    int is_readonly;
    int inserted;	
    int clk_rate;
    int clk_div;
    int max_clkdiv;	
    int real_rate;
}gp_sdio_info_t;

typedef struct gp_sdio_host_s {
    struct mmc_host *mmc;
    struct mmc_request *mrq;
    gp_sdio_info_t *platdata;
    void *pdev;
    int irq;
    int blk_sz;
}gp_sdio_host_t;


/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static int gp_sdio_inserted(struct mmc_host *mmc);
static int gp_sdio_readonly(struct mmc_host *mmc);
static void gp_sdio_request(struct mmc_host *mmc, struct mmc_request *mrq);
static void gp_sdio_set_ios(struct mmc_host *mmc, struct mmc_ios *ios);
static void gp_sdio_enable_irq(struct mmc_host *mmc, int enable);
static int __devinit gp_sdio_probe(struct platform_device *pdev);
static int __devexit gp_sdio_remove(struct platform_device *pdev);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
/*WIFI POWER (B_KEYSCAN1), GPIO0, Function 0, GID 19, PIN 1*/  
static const unsigned int gp_sdio_power_pin = ((0<<24)|(0<<16)|(19<<8)|1);
/*WLAN_EN (B_SD_CARD1), GPIO0, Function 2, GID 30, PIN 24*/  
static const unsigned int gp_wl_en_pin = ((0<<24)|(2<<16)|(30<<8)|24);
/*WL_REG_ON (B_SD_CARD3), GPIO0, Function 2, GID 34, PIN 26*/  
static const unsigned int gp_reg_on_pin = ((0<<24)|(2<<16)|(34<<8)|26);
static const unsigned int gp_sdio_pin[GP_SDIO_PIN_NUM] = {
    ((0<<24)|(2<<16)|(20<<8)|2), /*SD_WIFI_CLK(B_KEYSCAN_2), GPIO0, Function 2, GID 20, PIN 2*/  
    ((0<<24)|(2<<16)|(21<<8)|3), /*SD_WIFI_CMD(B_KEYSCAN_3), GPIO0, Function 2, GID 21, PIN 3*/  
    ((0<<24)|(2<<16)|(22<<8)|4), /*SD_WIFI_DAT[0](B_KEYSCAN_4), GPIO0, Function 2, GID 22, PIN 4*/  
    ((0<<24)|(2<<16)|(22<<8)|5), /*SD_WIFI_DAT[1](B_KEYSCAN_5), GPIO0, Function 2, GID 22, PIN 5*/  
    ((0<<24)|(2<<16)|(23<<8)|6), /*SD_WIFI_DAT[2](B_KEYSCAN_6), GPIO0, Function 2, GID 23, PIN 6*/  
    ((0<<24)|(2<<16)|(24<<8)|7), /*SD_WIFI_DAT[3](B_KEYSCAN_7), GPIO0, Function 2, GID 24, PIN 7*/ 
    };
gp_sdio_info_t *gp_sdio_info = NULL;

/*****************************************************************
 * SD / SDIO Device Info
*****************************************************************/
static gp_sdio_info_t gp_sdio_platdata = {
	         .device_id = 1,
	         .clk_rate = 0,
	         .clk_div = 2,
	         .max_clkdiv = 256,
	         .real_rate = 0
};

static long long gp_sdio_dma_mask = DMA_BIT_MASK(32);
struct platform_device gp_sdio_device = {
	.name	= "gp_sdio",
	.id		= 1,
	.dev	= {
		.platform_data		= &gp_sdio_platdata,
		.dma_mask = &gp_sdio_dma_mask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	}
};

static const struct mmc_host_ops gp_sdio_ops = {
	.request	= gp_sdio_request,
	.set_ios	= gp_sdio_set_ios,
	.get_ro = gp_sdio_readonly,
	.get_cd = gp_sdio_inserted,
	.enable_sdio_irq = gp_sdio_enable_irq,
};

static struct platform_driver gp_sdio_driver = {
	.probe         = gp_sdio_probe,
	.remove        = gp_sdio_remove,
	.suspend       = NULL,
	.resume        = NULL,
	.driver        = {
		.name  = DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

/**************************************************************************
 *                               Function implement                                                                    *
 **************************************************************************/
/**
 * @brief   SDIO device check insert function
 */
static int gp_sdio_inserted(struct mmc_host *mmc)
{
    /*gp_sdio_host_t *host = mmc_priv(mmc);
    gp_sdio_info_t  *sd_data= host->platdata;*/
    
    DIAG_INFO("\n%s,%s() ENTER.", __FILE__, __FUNCTION__);
    return SP_TRUE;
}

/**
 * @brief   SDIO device check readonly function
 */
static int gp_sdio_readonly(struct mmc_host *mmc)
{
    /*sdio card not need check readonly*/
    /*gp_sdio_host_t *host = mmc_priv(mmc);
    gp_sdio_info_t  *sd_data= host->platdata;
    unsigned int status;

    DIAG_INFO("\n%s,%s() ENTER.", __FILE__, __FUNCTION__);
    status = gpHalSDGetStatus(sd_data->device_id);
    if (status & MASK_S_CARDWP) {
        return SP_TRUE;
    }*/

    return SP_FALSE;
}

/**
 * @brief   SDIO device send command function
 */
static int gp_sdio_send_command(gp_sdio_host_t *host, struct mmc_command *cmd, struct mmc_data *data)
{
    gp_sdio_info_t *sd_data= host->platdata;    
    unsigned int cmd_info = 0;
    int ret;
    gpHalSDCmd_t sdCmd;
    int sg_map_len = 0;
    int sg_len;
    int iter;
    unsigned char *sg_ptr = NULL;
    gpApbdma0Param_t dma_param = {0};
    int dma_handle = -1;
    int dma_dir = 0;
    static int dma_cnt = 0;

    /*DIAG_INFO("\n%s,%s() ENTER, %x,%x,%x.", __FILE__, __FUNCTION__, host, cmd, data);*/
    
    switch (mmc_resp_type(cmd)) {
    case MMC_RSP_NONE:
        cmd_info = SDC_RESPTYPE_NONE;
        break;
    case MMC_RSP_R1:
        cmd_info  = SDC_RESPTYPE_R1;
        break;
    case MMC_RSP_R1B:
        cmd_info  = SDC_RESPTYPE_R1b;
        break;
    case MMC_RSP_R2:
        cmd_info  = SDC_RESPTYPE_R2;
        break;
    case MMC_RSP_R3:
        cmd_info  = SDC_RESPTYPE_R3;
        break;
    default:
        DIAG_ERROR("pmpmci: unhandled response type %02x\n",mmc_resp_type(cmd));
        ret = -EINVAL;
        goto out_send_cmd1;
    }

    DIAG_INFO("\nsend sdio cmd=%d, arg=0x%x, data blocks=%d, blksz=%d.", 
        cmd->opcode, cmd->arg, data?data->blocks:0, data?data->blksz:0);
    sdCmd.cmd = cmd->opcode;
    sdCmd.resp_type = cmd_info;
    if (data != NULL) {
        sdCmd.with_data = 1;		
        if (data->blocks > 1) {
            sdCmd.with_data |= 0x02;
        }
        if (data->flags & MMC_DATA_WRITE) {
            sdCmd.dir = SDC_WRITE;
            dma_dir = DMA_TO_DEVICE;
        }
        else {
            sdCmd.dir = SDC_READ;
            dma_dir = DMA_FROM_DEVICE;
        }   
        
        if (data->blksz % 16 != 0) {
            gpHalSDEnableDma(sd_data->device_id, SP_FALSE);
        }
        else {
            gpHalSDEnableDma(sd_data->device_id, SP_TRUE);
        }
        if (data->blksz != host->blk_sz) {
            gpHalSDSetBlk(sd_data->device_id, data->blksz);
            DIAG_INFO("\nSet SDIO block size=%d.", data->blksz);
            host->blk_sz = data->blksz;
        }
        sg_map_len = dma_map_sg(mmc_dev(host->mmc), data->sg, data->sg_len, dma_dir);
        if (sg_map_len <= 0) {
            DIAG_ERROR("sdio sg map zero!\n");
            ret = -1;
            goto out_send_cmd;
        }
    }	
    else {
        sdCmd.with_data = 0;
        sdCmd.dir = 0;
        sg_map_len = 0;
        gpHalSDEnableDma(sd_data->device_id, SP_FALSE);
    }
    
    ret = gpHalSDSendCmd(sd_data->device_id, sdCmd, cmd->arg, &cmd->resp[0], sizeof(cmd->resp));
    if (ret == SP_FALSE) {
        DIAG_ERROR("\n%s,%s() %d, send cmd=%d failed!\narg=0x%x, status=0x%x, with_data=0x%x, resp type=%d.\n",
            __FILE__, __FUNCTION__, __LINE__, sdCmd.cmd, cmd->arg, gpHalSDGetStatus(sd_data->device_id), sdCmd.with_data, sdCmd.resp_type);
        ret = -1;
        goto out_send_cmd1;
    }

    DIAG_INFO("\t response=0x%8x.", cmd->resp[0]);
    /*no data, cmd finished*/
    if (data == NULL) {
        ret = 0;
        goto out_send_cmd1;
    }    
    
    if (data->blksz % 16 != 0) {
        /*pio mode*/
        /*DIAG_INFO("\n%s,%s() %d.", __FILE__, __FUNCTION__, __LINE__);*/
        for (iter = 0; iter < sg_map_len; iter++) {
            sg_ptr = sg_virt(&data->sg[iter]);
            sg_len = sg_dma_len(&data->sg[iter]);
            ret = gpHalSDDataTxRx(sd_data->device_id, sg_ptr, sg_len, sdCmd.dir);
            if(!ret) {
                DIAG_ERROR("PIO error: %d\n",ret);
                ret = -1;
                goto out_send_cmd;
            }
        }
    }
    else {
        /*dma mode*/
        /*DIAG_INFO("\n%s,%s() %d.", __FILE__, __FUNCTION__, __LINE__);*/
        dma_cnt++;
        dma_handle = gp_apbdma0_request(100);
        if (dma_handle < 0) {
            DIAG_ERROR("%s,%s() %d, request DMA failed!\n",  __FILE__, __FUNCTION__, __LINE__);
            ret = -1;
            goto out_send_cmd;
        }
        for (iter = 0; iter < sg_map_len; iter++) {
            dma_param.buf0 = (unsigned char*)sg_phys(&data->sg[iter]);
            dma_param.ln0 = sg_dma_len(&data->sg[iter]);
            dma_param.dir = sdCmd.dir;
            dma_param.module = (sd_data->device_id == 0) ? SD0: SD1;    
            dma_param.buf1 = NULL;
            dma_param.ln1 = 0;
            
            gp_apbdma0_en(dma_handle, dma_param);
		/* ----- Wait dma finish ----- */
            ret = gp_apbdma0_wait(dma_handle, 100);
            if(ret != 0) {
                gp_apbdma0_stop(dma_handle);
                DIAG_ERROR("DMA error: %d\n",ret);
                ret = -1;
                goto out_send_cmd;
            }
            DIAG_INFO("\ndma out");  
           if (sdCmd.dir == SDC_WRITE) {
                ret = gpHalSDWaitDataComplete(sd_data->device_id);
               /* if(ret != SP_TRUE) {
                    gpHalSDDump(sd_data->device_id);
                    DIAG_ERROR("DMA data complete wait time out!\ncmd=%d, arg=0x%x, status=0x%x.\n",
                        cmd->opcode, cmd->arg, gpHalSDGetStatus(sd_data->device_id));
                    printk("\ndma count=%d.", dma_cnt);
                    ret = -1;
                    goto out_send_cmd;
                }*/
            } 
            gpHalSDClearStatus(sd_data->device_id);
            /*DIAG_INFO("\ndma start buf=%x, len=%d, dir=%d.", dma_param.buf0, dma_param.ln0, dma_param.dir); */
        }
    }
    
    cmd->data->error = 0;
    ret = SP_SUCCESS;
    
out_send_cmd:        
    if (dma_handle >= 0) {
        gp_apbdma0_release(dma_handle);
    }    
    if (data != NULL && sg_map_len > 0) {
        dma_unmap_sg(mmc_dev(host->mmc), data->sg, data->sg_len, dma_dir);
    }
    
out_send_cmd1:
    /*if (data != NULL)*/ {
        gpHalSDStop(sd_data->device_id);
    }
    
    return ret;
}

/**
 * @brief   SDIO device request function
 */
static void gp_sdio_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
    gp_sdio_host_t *host = mmc_priv(mmc); 
    struct mmc_command *cmd = mrq->cmd;
   
    /*DIAG_INFO("\n%s,%s() stop=0x%x, ENTER.", __FILE__, __FUNCTION__, mrq->stop);*/
    if(host->mrq != NULL){
        DIAG_ERROR("\nWARNING:SD CMD REQUEST WHEN PREV NOT FINISH!\nNew cmd=%d, arg=0x%x.\nOld cmd=%d, arg=0x%x.", 
            mrq->cmd->opcode, mrq->cmd->arg, host->mrq->cmd->opcode, host->mrq->cmd->arg);
        return;
    }
    
    host->mrq = mrq;
    gp_sdio_send_command(host, cmd, cmd->data);     
    if (mrq->stop != NULL) {
        DIAG_INFO("\n%s,%s() send stop cmd.", __FILE__, __FUNCTION__);
        gp_sdio_send_command(host, mrq->stop, mrq->stop->data);    
    }
    mmc_request_done(host->mmc, mrq);
    host->mrq = NULL;

    return;
}

/**
 * @brief   SDIO device IO Setting function
 */
static void gp_sdio_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
    gp_sdio_host_t *host = mmc_priv(mmc);
    gp_sdio_info_t *sd_data= host->platdata;	

    switch (ios->power_mode) {
    case MMC_POWER_ON:
        /*power on same with power up*/
        /*not need break*/
    case MMC_POWER_UP:
        sd_data->real_rate = gpHalSDSetClk(sd_data->device_id, SYS_APB_CLK,  (ios->clock+SD_CLK_UNIT)/SD_CLK_UNIT);
        sd_data->real_rate *= SD_CLK_UNIT;
        DIAG_INFO("\n%s,%s() clock=%d, real clock=%d.", __FILE__, __FUNCTION__, ios->clock, sd_data->real_rate);
        
        switch (ios->bus_width) {
        case MMC_BUS_WIDTH_4:
           gpHalSDSetBus(sd_data->device_id, MASK_C_BUSWIDTH_4);	
            break;
        case MMC_BUS_WIDTH_1:
           gpHalSDSetBus(sd_data->device_id, MASK_C_BUSWIDTH_1);			
            break;
        }
        break;

    case MMC_POWER_OFF:
        DIAG_INFO("sdio card power off\n");
        break;
        
    default:
        break;
    }

    return;
}

/**
 * @brief   SDIO device irq callback function
 */
static irqreturn_t gp_sdio_irq(int irq, void *dev_id)
{
    gp_sdio_host_t *host = dev_id;
    gp_sdio_info_t *sd_data= host->platdata;	
    int status;

    status = gpHalSDGetStatus(sd_data->device_id);
    DIAG_INFO("\ninterrupt occur:  sd irq status=%x.", status);
    
    if (status & MASK_S_SdioInterrupt) { /* sdio */
        DIAG_INFO("\nSDIO interrupt occur:  sd irq status=%x.", status);
        mmc_signal_sdio_irq(host->mmc);		
        goto irq_out;
    }
	
irq_out:
   	gpHalSDClearStatus(sd_data->device_id);
	return IRQ_HANDLED;
}

/**
 * @brief   SDIO device enable sdio interrupt function
 */
static void gp_sdio_enable_irq(struct mmc_host *mmc, int enable)
{
    gp_sdio_host_t *host = mmc_priv(mmc);
    gp_sdio_info_t *sd_data= host->platdata;	

    DIAG_INFO("\npmpmci_enable_sdio_irq=%d.", enable);
    gpHalSDEnableInterrupt(sd_data->device_id,  SD_MMC_INT_SDIODETECT, enable);
}

/**
 * @brief   SDIO device probe function
 */
static int __devinit gp_sdio_probe(struct platform_device *pdev)
{
    struct mmc_host *mmc_host;
    gp_sdio_host_t *sdio_host;
    gp_sdio_info_t *sd_data;
    int ret;

    mmc_host = mmc_alloc_host(sizeof(gp_sdio_host_t), &pdev->dev);
    if (mmc_host == NULL) {
        DIAG_ERROR("no memory for mmc_host\n");
        ret = -ENOMEM;
        goto err0_sdio_probe;
    }
    sdio_host = mmc_priv(mmc_host);
    sdio_host->mmc = mmc_host;
    sdio_host->platdata = pdev->dev.platform_data;
    sd_data = (gp_sdio_info_t *)sdio_host->platdata;
    sdio_host->pdev = pdev;    
    ret = -ENODEV;

    if (sd_data->device_id == 0) {
        sdio_host->irq = IRQ_SD0;
    }
    else {
        sdio_host->irq = IRQ_SD1;
    }

    /* IRQ is shared among both SD controllers */
    ret = request_irq(sdio_host->irq, gp_sdio_irq, 0, DRIVER_NAME, sdio_host);
    if (ret) {
        DIAG_ERROR("cannot grab SDIO IRQ\n");
        goto err1_sdio_probe;
    }
    
    gpHalSDInit(sd_data->device_id);
    mmc_host->ops = &gp_sdio_ops;
    sdio_host->platdata->clk_rate = 37125000;
    mmc_host->f_min = sdio_host->platdata->clk_rate / (sdio_host->platdata->clk_div * sdio_host->platdata->max_clkdiv);
    mmc_host->f_max = sdio_host->platdata->clk_rate / sdio_host->platdata->clk_div;
    mmc_host->max_blk_count = 4095;
    mmc_host->max_blk_size  = 4095;	
    mmc_host->max_seg_size  = 4095 * 512;
    mmc_host->max_req_size  = 4095 * 512;	
    mmc_host->max_hw_segs   = 128;		
    mmc_host->max_phys_segs = 128;
	
    mmc_host->ocr_avail = MMC_VDD_32_33 | MMC_VDD_33_34;
    mmc_host->caps = MMC_CAP_4_BIT_DATA;
    mmc_host->caps |= MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED;
    //mmc_host->caps |= MMC_CAP_NEEDS_POLL;  //first time use poll detect
    mmc_host->caps |= MMC_CAP_SDIO_IRQ;
    
    ret = mmc_add_host(mmc_host);
    if (ret) {
        DIAG_ERROR("cannot add mmc host\n");
        goto err2_sdio_probe;
     }
    
    platform_set_drvdata(pdev, sdio_host);
    DIAG_INFO("MMC Controller %d set up\n", sd_data->device_id);

    return 0;	/* all ok */

err2_sdio_probe:
    free_irq(sdio_host->irq, sdio_host);
    
err1_sdio_probe:
    mmc_free_host(mmc_host);
    
err0_sdio_probe:
    return ret;
}

/**
 * @brief   SDIO device remove function
 */
static int __devexit gp_sdio_remove(struct platform_device *pdev)
{
    gp_sdio_host_t *host = platform_get_drvdata(pdev);
    /*gp_sdio_info_t *sd_data= host->platdata;*/
    
    if (host != NULL) {
        mmc_remove_host(host->mmc);
        free_irq(host->irq, host);
        mmc_free_host(host->mmc);
        platform_set_drvdata(pdev, NULL);
    }
    
    return 0;
}

/**
 * @brief   SDIO driver init
 */
static int __init gp_sdio_init(void)
{
    int iter;
    int ret;
    
    /*gp_sdio_info = kmalloc(sizeof (gp_sdio_info_t), GFP_KERNEL);
    if (gp_sdio_info == NULL) {
        goto err_sdio_init;
    }*/
    
    gp_sdio_info = &gp_sdio_platdata;
    
    DIAG_INFO("\n%s,%s(),%d.", __FILE__, __FUNCTION__, __LINE__);
    
    /*WL_EN pull low*/
    gp_sdio_info->wl_en_pin = gp_gpio_request(gp_wl_en_pin,"wl_en");
    gp_gpio_set_direction(gp_sdio_info->wl_en_pin, GPIO_DIR_OUTPUT);
    gp_gpio_set_value(gp_sdio_info->wl_en_pin, GPIO_PULL_LOW);/**/
    
    /*WL_REG_ON pull low*/
    gp_sdio_info->wl_reg_on_pin = gp_gpio_request(gp_reg_on_pin,"wl_reg_on");
    gp_gpio_set_direction(gp_sdio_info->wl_reg_on_pin, GPIO_DIR_OUTPUT);
    gp_gpio_set_value(gp_sdio_info->wl_reg_on_pin, GPIO_PULL_LOW);

    /*SDIO power on*/
    gp_sdio_info->power_pin = gp_gpio_request(gp_sdio_power_pin,"sdio_power");
    gp_gpio_set_direction(gp_sdio_info->power_pin, GPIO_DIR_OUTPUT);
    gp_gpio_set_value(gp_sdio_info->power_pin, GPIO_PULL_HIGH);
    mdelay(100); 
        
    /* ----- initial sdio pins ----- */
    for(iter=0; iter<GP_SDIO_PIN_NUM; iter++)
    {
    	gp_sdio_info->sdio_pin[iter] = gp_gpio_request(gp_sdio_pin[iter],"sdio_pin");	
    }     
    
    /*WL_EN pull high*/
    gp_gpio_set_direction(gp_sdio_info->wl_en_pin, GPIO_DIR_OUTPUT);
    gp_gpio_set_value(gp_sdio_info->wl_en_pin, GPIO_PULL_HIGH);    
    mdelay(200);     
    
    DIAG_INFO("\n%s,%s(),%d.", __FILE__, __FUNCTION__, __LINE__);
    
    ret = platform_device_register(&gp_sdio_device);
    if (ret) {
        goto err_sdio_init;
    }
    
    return platform_driver_register(&gp_sdio_driver);

    err_sdio_init:
        return -ENOMEM;
}
/**
 * @brief   SDIO driver exit
 */
static void __exit gp_sdio_exit(void)
{
    int iter;
    
    platform_driver_unregister(&gp_sdio_driver);

    gp_gpio_set_value(gp_sdio_info->power_pin, GPIO_PULL_LOW);
    gp_gpio_release(gp_sdio_info->wl_en_pin);
    gp_gpio_release(gp_sdio_info->wl_reg_on_pin);
    gp_gpio_release(gp_sdio_info->power_pin);
    for(iter=0; iter<GP_SDIO_PIN_NUM; iter++)
    {
        gp_gpio_release(gp_sdio_info->sdio_pin[iter]);
    }     
}

module_init(gp_sdio_init);
module_exit(gp_sdio_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/
MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("GP SDIO Driver");
MODULE_LICENSE_GP;

