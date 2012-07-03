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
#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/cdev.h>
#include <mach/diag.h>
#include <mach/hal/hal_aes.h>
#include <mach/hal/hal_clock.h>
#include <mach/gp_chunkmem.h>
#include <mach/gp_cache.h>
#include <mach/gp_aes.h>
#include <mach/clock_mgr/gp_clock.h>
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define RETURN(x)	{nRet = x; goto __return;}
#define DERROR	printk 
#if 0
#define DEBUG	printk
#else
#define DEBUG(...)
#endif

/* AES timeout (ms) */
#define AES_TIMEOUT   3000

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct gpAesDev_s 
{
	struct miscdevice dev;          /*!< @brief aes device */
	struct semaphore sem;           /*!< @brief mutex semaphore for aes ops */
	wait_queue_head_t wait_queue;   /*!< @brief aes done wait queue */
	bool done;                      /*!< @brief aes done flag */
	unsigned int open_cnt;
} gpAesDev_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static gpAesDev_t *pAesDev = NULL;

/**
 * @brief   Ceva clock enable/disable
 * @param   enable [in] 0:disable, 1:enable
 * @return  None
 * @see
 */
static void aes_clock_enable(int enable)
{
#ifdef CONFIG_PM
	if( enable ){
		gp_enable_clock((int*)"SYS_A", 1);
		gpHalScuClkEnable( SCU_A_PERI_AES, SCU_A, enable);
		gpHalAesModuleReset(1);
	}
	else{
		gpHalScuClkEnable( SCU_A_PERI_AES, SCU_A, enable);
		gp_enable_clock((int*)"SYS_A", 0);
	}
#else
	gpHalScuClkEnable( SCU_A_PERI_AES, SCU_A, enable);
	gpHalAesModuleReset(1);
#endif
}

static int
gp_aes_set_dma(
	UINT32 src_addr,
	UINT32 dst_addr,
	UINT32 cblen
)
{
	unsigned int ctrl, len;
	unsigned int src_phy_addr, dst_phy_addr;
	
	if(	((src_addr & 0x03) == 0) && 
		((dst_addr & 0x03) == 0) && 
		((cblen % 4) == 0))
	{
		/* DMA_DATA_WIDTH_4BYTE */
		ctrl = C_DMA_CTRL_32BIT | C_DMA_CTRL_INT | C_DMA_CTRL_NORMAL_INT | C_DMA_CTRL_ENABLE;
		len = cblen >> 2; 
	}
	else if(((src_addr & 0x01) == 0) && 
			((dst_addr & 0x01) == 0) && 
			((cblen % 2) == 0))
	{
		/* DMA_DATA_WIDTH_2BYTE */
		ctrl = C_DMA_CTRL_16BIT | C_DMA_CTRL_INT | C_DMA_CTRL_NORMAL_INT | C_DMA_CTRL_ENABLE;
		len = cblen >> 1; 
	}
	else
	{
		/* DMA_DATA_WIDTH_1BYTE */
		ctrl = C_DMA_CTRL_8BIT | C_DMA_CTRL_INT | C_DMA_CTRL_NORMAL_INT | C_DMA_CTRL_ENABLE;
		len = cblen; 
	}

	/* Memory to memory */
	ctrl |= C_DMA_CTRL_SINGLE_TRANS | C_DMA_CTRL_SRC_INCREASE | C_DMA_CTRL_DEST_INCREASE | C_DMA_CTRL_SOFTWARE;
	
	/* Check whether burst mode can be used */
	if((cblen & 0x3) == 0) 
	{
		ctrl |= C_DMA_CTRL_BURST4_ACCESS;
	} 
	else 
	{
		ctrl |= C_DMA_CTRL_SINGLE_ACCESS;
	}

	src_phy_addr = gp_user_va_to_pa((void *)src_addr);
	dst_phy_addr = gp_user_va_to_pa((void *)dst_addr);
	if(src_phy_addr == 0 || dst_phy_addr == 0)
		return -1;
		
	DEBUG("SrcAddr = 0x%x\n", src_phy_addr);
	DEBUG("DstAddr = 0x%x\n", dst_phy_addr);
	DEBUG("len = 0x%x\n", len);
	DEBUG("DmaCtrl = 0x%x\n", ctrl);
	
	gpHalAesSetDmaInit();
	gpHalAesSetDmaAddr(src_phy_addr, dst_phy_addr, len);
	gpHalAesSetDmaCtrl(ctrl);
	return 0;
}

static int 
gp_aes_trigger(
	unsigned int encrypt_en,
	gpAesPara_t *argp
)
{
	int nRet = 0;

	if(down_interruptible(&pAesDev->sem) != 0)
		return -ERESTARTSYS;

#ifndef GP_SYNC_OPTION
	/* update cache to ram */
	gp_clean_dcache_range(argp->input_buf_addr, argp->cblen);
#else
	GP_SYNC_CACHE();	
#endif

	/* load key */
	nRet = gpHalAesSetKey(argp->key0, argp->key1, argp->key2, argp->key3);
	if(nRet < 0)
	{
		DERROR("load key fail..\n");
		RETURN(-1);
	}
	
	/* enable aes */
	if(encrypt_en)
	{
		DEBUG("Encrypt.\n");
		gpHalAesSetEncrypt();
	}
	else
	{
		DEBUG("Decrypt.\n");
		gpHalAesSetDecrypt();
	}

	/* enable dma */
	pAesDev->done = 0;
	nRet = gp_aes_set_dma(argp->input_buf_addr, argp->output_buf_addr, argp->cblen);
	if(nRet < 0) 
	{
		DERROR("dma set fail..\n");
		RETURN(-2);
	}	
	
	/* waiting for done */
	DEBUG("wait.\n");
	if(wait_event_timeout(pAesDev->wait_queue, pAesDev->done, AES_TIMEOUT) == 0)
	{
		DERROR("AesTimeOut!\n");
		RETURN(-3);
	}

#ifndef GP_SYNC_OPTION	
	/* update ram to cache */
	gp_invalidate_dcache_range(argp->output_buf_addr, argp->cblen);
#else
	GP_SYNC_CACHE();	
#endif
__return:
	gpHalAesSetStop();
	up(&pAesDev->sem);
	return nRet;
}

static irqreturn_t 
gp_aes_irq_handler(
	int irq, 
	void *dev_id
)
{
	gpAesDev_t *pdev = (gpAesDev_t *)dev_id;

	DEBUG("aesirq\n");
	if(gpHalAeGetDmaStatus() == 0 && pdev->done == 0)
	{
		pdev->done = 1;
		wake_up(&pdev->wait_queue);
	}
	return IRQ_HANDLED;
}

static long 
gp_aes_ioctl(
	struct file *filp, 
	unsigned int cmd, 
	unsigned long arg
)
{
	long nRet = 0;
	gpAesPara_t *hd;

	hd = (gpAesPara_t *)filp->private_data;
 	if(hd == NULL) RETURN(-EINVAL);
	
	switch(cmd) 
	{
	case AES_IOCTL_S_DECRYPT:	
		nRet = copy_from_user((void *)hd, (void __user*)arg, sizeof(gpAesPara_t));
		if(nRet < 0) RETURN(-EINVAL);
		nRet = gp_aes_trigger(0, hd);
		break;

	case AES_IOCTL_S_ENCRYPT:
		nRet = copy_from_user((void *)hd, (void __user*)arg, sizeof(gpAesPara_t));
		if(nRet < 0) RETURN(-EINVAL);
		nRet = gp_aes_trigger(1, hd);
		break;
	
	default:
		RETURN(-ENOTTY);	/* Inappropriate ioctl for device */
		break;
	}

__return:
	return nRet; 
}


static int 
gp_aes_open	(
	struct inode *inode, 
	struct file *filp
)
{
	int nRet = 0;
	gpAesPara_t *hd = NULL;

	if(down_interruptible(&pAesDev->sem) != 0)
		return -ERESTARTSYS;

	hd = (gpAesPara_t *)kzalloc(sizeof(gpAesPara_t), GFP_KERNEL);
	if(hd == NULL) RETURN(-ENOMEM);

	filp->private_data = (gpAesPara_t *)hd;
	if(pAesDev->open_cnt == 0) 
		aes_clock_enable(1);

	pAesDev->open_cnt++;
__return:
	up(&pAesDev->sem);
	return nRet;
}

static int 
gp_aes_release(
	struct inode *inode, 
	struct file *filp
)
{
	int nRet = 0;
	gpAesPara_t *hd;

	if(down_interruptible(&pAesDev->sem) != 0)
		return -ERESTARTSYS;
	
	hd = (gpAesPara_t *)filp->private_data;
 	if(hd == NULL) RETURN(-ENOMEM);

	kfree(hd);
	filp->private_data = NULL;
	pAesDev->open_cnt--;
	if(pAesDev->open_cnt == 0) 
		aes_clock_enable(0);

__return:
	up(&pAesDev->sem);
	return nRet;
}


static const struct file_operations aes_fops = 
{
	.owner          = THIS_MODULE,
	.open           = gp_aes_open,
	.release        = gp_aes_release,
	.unlocked_ioctl = gp_aes_ioctl,
};

static void 
gp_aes_device_release(
	struct device *dev
)                       
{                                                                           
	DIAG_INFO("remove aes device ok\n");                                      
}                                                                           
                                                                            
static struct platform_device gp_aes_device = {                             
	.name = "gp-aes",                                                         
	.id	= 0,                                                                  
	.dev = 
	{                                                                 
		.release = gp_aes_device_release,                                       
	},                                                                        
};

#ifdef CONFIG_PM
static int 
gp_aes_suspend(
	struct platform_device *pdev, 
	pm_message_t state)
{
	if(pAesDev->open_cnt > 0)
	{
		if(down_interruptible(&pAesDev->sem) != 0)
			return -ERESTARTSYS;
	
		aes_clock_enable(0);
		up(&pAesDev->sem);
	}
	return 0;
}

static int 
gp_aes_resume(
	struct platform_device *pdev
)
{
	if(pAesDev->open_cnt > 0)
	{
		aes_clock_enable(1);
	}
	return 0;
}
#else
#define gp_aes_suspend NULL
#define gp_aes_resume NULL
#endif

static struct platform_driver gp_aes_driver = 
{
	.suspend = gp_aes_suspend,
	.resume = gp_aes_resume,
	.driver	= 
	{
		.owner	= THIS_MODULE,
		.name	= "gp-aes"
	},
};

static int __init 
aes_init_module(
	void
)
{
	int nRet = 0;

	pAesDev = (gpAesDev_t *)kzalloc(sizeof(gpAesDev_t), GFP_KERNEL);
	if(pAesDev == NULL) 
	{
		DERROR("aes kmalloc fail\n");
		RETURN(-ENOMEM);
	}

	nRet = request_irq(IRQ_UART_C1,
					  gp_aes_irq_handler,
					  IRQF_DISABLED,
					  "AES_IRQ",
					  pAesDev);
	if(nRet < 0) 
	{
		DERROR("aes request irq fail\n");
		RETURN(-ENXIO);
	}

	/* initialize */
	init_MUTEX(&pAesDev->sem);
	init_waitqueue_head(&pAesDev->wait_queue);

	pAesDev->dev.name  = "aes";
	pAesDev->dev.minor = MISC_DYNAMIC_MINOR;
	pAesDev->dev.fops  = &aes_fops;
	pAesDev->open_cnt = 0;
	/* register device */
	nRet = misc_register(&pAesDev->dev);
	if(nRet != 0) 
	{
		DERROR("scalar device register fail\n");
		RETURN(-ENXIO);
	}
	
	/* register platform driver */
	platform_device_register(&gp_aes_device);
	platform_driver_register(&gp_aes_driver);
__return:
	if(nRet < 0)
	{
		free_irq(IRQ_UART_C1, pAesDev);
		kfree(pAesDev);
		pAesDev = NULL;
	}
	return nRet;
}

static void __exit 
aes_exit_module(
	void
)
{
	misc_deregister(&pAesDev->dev);
	platform_device_unregister(&gp_aes_device);
	platform_driver_unregister(&gp_aes_driver);
	
	free_irq(IRQ_UART_C1, pAesDev);
	kfree(pAesDev);
	pAesDev= NULL;
}

module_init(aes_init_module);
module_exit(aes_exit_module);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("Generalplus AES Engine Driver");
MODULE_LICENSE_GP;
