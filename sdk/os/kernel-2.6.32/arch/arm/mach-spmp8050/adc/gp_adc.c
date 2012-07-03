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
 * @file arch/arm/mach-spmp8050/gp_adc.c
 * @brief ADC device driver
 * @author zh.l
 */

#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/cdev.h>
#include <mach/diag.h>
#include <mach/gp_adc.h>
#include <mach/hal/hal_adc.h>
#include <mach/hal/regmap/reg_adc.h>

/* This driver is designed to control the usage of the ADC block between
 * the touchscreen and any other drivers that may need to use it, such as
 * the sar key driver.
 *
 * Each user registers to get a client block which uniquely identifies it
 * and stores information such as the necessary functions to callback when
 * action is required.
 */
 
/**
 *@brife the ADC client data struct
 */
typedef struct gp_adc_client_s {
	struct list_head	pend;	/*!<for list iteration*/
	wait_queue_head_t	wait;	/*!<wait queue list head*/

	int			result;	/*!<last ad result of this client*/
	unsigned int		channel;/*!<current channel of this client*/
	unsigned int		is_ts;	/*!<is this client an touch screen?*/
	/*!<conversion end callback function, need for touch screen client*/
	void	(*convert_cb)(int handle, unsigned val, unsigned event);
}gp_adc_client_t;

/**
 *@brife the ADC device data struct
 */
typedef struct gp_adc_device_s {
	struct miscdevice dev;      /*!< @brief gpio device */
	struct platform_device	*pdev;
	struct gp_adc_client_s	*cur;	/*!< current ad client*/
	struct gp_adc_client_s	*ts;	/*!< touch screen client*/
	unsigned int	irq;		/*!< irq number*/
} gp_adc_device_t;


static struct gp_adc_device_s *adc_dev = NULL;
/*pending list of ad conversions*/
static LIST_HEAD(gp_adc_pending);

/*
 *round-robin adc scheduler
 */
static void gp_adc_sched(struct gp_adc_device_s *adc)
{
	struct gp_adc_client_s *next;

	if(NULL == adc)
		return;

	/*any AD request in pending list*/
	if (!list_empty(&gp_adc_pending)) { 
		next = list_first_entry(&gp_adc_pending, struct gp_adc_client_s, pend);
		/*start ad request*/
		adc->cur = next;
		gpHalAdcStartConv(MODE_AUX, next->channel);
		gpHalAdcSetIntEn(ADC_INTAUX, ADC_INTAUX);
		/*request done, move it to the list tail*/
		list_move_tail(&next->pend, &gp_adc_pending);
	}
	else { /*no more pending request, stop manual conversion*/
		gpHalAdcStopConv(MODE_AUX);
		gpHalAdcSetIntEn(ADC_INTAUX, 0);
	}
}

/**
 *@brief  start specified channel ADC conversion
 *@param  handle: handle of the adc client
 *@param  channel: specify which channel as AUX ADC input
 *   This parameter can be one of the following values:
 *     @arg ADC_CHANNEL_TPXP: touch pannle X positive input
 *     @arg ADC_CHANNEL_TPXN: touch pannle X negative input
 *     @arg ADC_CHANNEL_TPYP: touch pannle Y positive input
 *     @arg ADC_CHANNEL_TPYN: touch pannle Y negative input
 *     @arg ADC_CHANNEL_AUX1: AUX1 as ADC input
 *     @arg ADC_CHANNEL_AUX2: AUX2 as ADC input
 *     @arg ADC_CHANNEL_AUX3: AUX3 as ADC input
 *     @arg ADC_CHANNEL_AUX4: AUX4 as ADC input
 *@return 0-OK
 *        -EINVAL invalid parameter
 *        -EAGAIN client have alreay started
 */
int gp_adc_start(int handle, unsigned int channel)
{
	struct gp_adc_device_s *adc = adc_dev;
	unsigned long flags;
	struct gp_adc_client_s *client = (struct gp_adc_client_s *)handle;

	if (NULL==adc || NULL==client) {
		DIAG_ERROR("%s: failed to find adc or client\n", __func__);
		return -EINVAL;
	}

	/*only one touch screen client can be started*/
	if (client->is_ts && adc->ts) {
		DIAG_INFO("adc touchscreen client already started!\n");
		return -EAGAIN;
	}

	local_irq_save(flags);

	if (client->is_ts) {/*ts client*/
		adc->ts = client;
		gpHalAdcSetIntEn(ADC_INTPNL | ADC_INTPENUP | ADC_INTPENDN, 
				ADC_INTPNL | ADC_INTPENUP);
		gpHalAdcStartConv(MODE_TP_AUTO, 0);
	}
	else {/*general ADC clients, add to pending list*/
		struct list_head *p, *n;
		struct gp_adc_client_s *tmp;
		unsigned found = 0;

		client->channel = channel;
		client->result = -1;
		/*client already in pending list?*/
		list_for_each_safe(p, n, &gp_adc_pending) {
			tmp = list_entry(p, struct gp_adc_client_s, pend);
			if (tmp == client) {
				found = 1;
				break;
			}
		}
		/*not in list, add it to tail*/
		if(0==found) {
			list_add_tail(&client->pend, &gp_adc_pending);	
		}		
	}

	/*try to schedule ADC conversion request in the list*/
	if (!adc->cur)
		gp_adc_sched(adc);	
	local_irq_restore(flags);

	return 0;
}
EXPORT_SYMBOL(gp_adc_start);

/**
 *@brief  manual stop specified AD client conversion
 *@param  handle: handle of the adc client
 *@return none
 */
int gp_adc_stop(int handle)
{
	struct list_head *p, *n;
	struct gp_adc_client_s *tmp;
	struct gp_adc_client_s *client = (struct gp_adc_client_s *)handle;
	
	if (adc_dev->cur == client)
		adc_dev->cur = NULL;

	if (adc_dev->ts == client) { /*touch screen client*/
		adc_dev->ts = NULL;
		gpHalAdcStopConv(MODE_TP_AUTO);
		gpHalAdcSetIntEn(ADC_INTPNL | ADC_INTPENUP | ADC_INTPENDN, 0);
	}
	else { /*remove this client from pending list*/
		list_for_each_safe(p, n, &gp_adc_pending) {
			tmp = list_entry(p, struct gp_adc_client_s, pend);
			if (tmp == client) {
				list_del(&tmp->pend);
				break;
			}
		}
	}

	if (adc_dev->cur == NULL)
		gp_adc_sched(adc_dev);
	return 0;
}
EXPORT_SYMBOL(gp_adc_stop);

/** callback function for AUX conversion end*/
static void gp_convert_done(int handle, unsigned int val, unsigned int event)
{
	struct gp_adc_client_s *client = (struct gp_adc_client_s *)handle;
	if(NULL != client) {
		client->result = 0x3ff & (val + 1024);
		wake_up(&client->wait);
	}
}

/**
 *@brief  Returns the last ADC conversion result data for AUX channel.
 *@param  handle: handle of the adc resource
 *@return -ETIMEOUT timeout, no valid result
 *        >=0, AD result
 */
int gp_adc_read(int handle)
{
	int ret;
	struct gp_adc_client_s *client = (struct gp_adc_client_s *)handle;

	if(NULL == client) {
		DIAG_ERROR("%s: called with NULL client\n", __func__);
		ret = -EINVAL;
		goto err_client;
	}

	ret = wait_event_timeout(client->wait, client->result >= 0, HZ / 10);	/*wait conversion down, or timeout 100ms*/
	if (client->result < 0) {
		ret = -ETIMEDOUT;
		goto err_client;
	}

	ret = client->result;
	client->result = -1;	/*data read, reset to invalid state*/

err_client:
	return ret;
}
EXPORT_SYMBOL(gp_adc_read);

/**
 *@brife request an ADC client
 *@param is_ts[in]: 0-this client is not touch screen, others-touch screen client
 *@param ts_cb[in]: callback function for touch screen conversion end,
 *                  not used for none touch screent client
 *@return -ENOMEM no memory for new client, request failed
 *        others: handle of the new adc client
 */
int gp_adc_request(
	unsigned int is_ts,
	void (*ts_cb)(int client,  unsigned int val, unsigned int event)
)
{
	struct gp_adc_client_s *client;

	client = kzalloc(sizeof(struct gp_adc_client_s), GFP_KERNEL);
	if (!client) {
		DIAG_ERROR("no memory for adc client\n");
		return -ENOMEM;
	}

	/*init client data*/
	client->is_ts = is_ts;
	client->convert_cb = is_ts ? ts_cb : gp_convert_done;
	init_waitqueue_head(&client->wait);

	return (int)client;
}
EXPORT_SYMBOL(gp_adc_request);

/**
 *@brife release ADC client
 *@param handle[in]: the ADC client to release
 *@return none
 */
void gp_adc_release(int handle)
{
	struct gp_adc_client_s *client = (struct gp_adc_client_s *)handle;

	gp_adc_stop(handle);
	kfree(client);
}
EXPORT_SYMBOL(gp_adc_release);

/*
 *adc interrupt handler
 */
static irqreturn_t gp_adc_irq(int irq, void *pw)
{
	struct gp_adc_device_s *adc = pw;
	struct gp_adc_client_s *client = adc->cur;
	unsigned int int_flags, data;

	int_flags = gpHalAdcGetIntFlag();
	data = gpHalAdcGetPNL();

	if(adc->ts && adc->ts->convert_cb) {
		/*PNL interrupt*/
		if(int_flags & ADC_INTPNL) {
			(adc->ts->convert_cb)((int)client, data, ADC_INTPNL);
		}
		/*Pen up interrupt*/
		if(int_flags & ADC_INTPENUP) {
			(adc->ts->convert_cb)((int)client, 0, ADC_INTPENUP);
		}
		/*pen down, only in touch screen manual mode*/
		if(int_flags & ADC_INTPENDN) {
			gpHalAdcStartConv(MODE_TP_MANUAL, 1);
		}
	}
	/*AUX data conversion end*/
	if((int_flags & ADC_INTAUX)) {
		data = gpHalAdcGetAUX();
		if(client && client->convert_cb)
			(client->convert_cb)((int)client, data, ADC_INTAUX);
		/*call the ADC scheduler*/
		adc->cur = NULL;
		gp_adc_sched(adc);
	}

	return IRQ_HANDLED;
}

/*
 *@brife /dev/adc handling (file operations)
 */
ssize_t gp_adc_fops_read(struct file* file, char __user *user,size_t len,loff_t* pos)
{
	int client = (int)(file->private_data);
	int temp;

	if(len<2) {/*user buffer size too small */
		return -EFAULT;
	}
	/*may blocking*/
	temp = gp_adc_read(client);

	if( temp<0 )
		return temp;

	len = 2;
	if(copy_to_user(user, &temp, len)) {
		return -EFAULT;
	}
	return len;
}

static int gp_adc_fops_open(struct inode *inode, struct file *file)
{
	int client;

	client = gp_adc_request(0,NULL);
	if(IS_ERR((void*)client))
		return client;

	file->private_data = (void*)client;
	return nonseekable_open(inode, file);
}

static int gp_adc_fops_release(struct inode *inode, struct file *file)
{
	int client = (int)file->private_data;

	gp_adc_release(client);
	return 0;
}

static long gp_adc_fops_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int client = (int)file->private_data;
	long ret = -ENOTTY;

	switch (cmd) {
	case IOCTL_GP_ADC_START:		
		ret = gp_adc_start(client, arg);	/*arg here used as channel*/
		break;
	case IOCTL_GP_ADC_STOP:
		ret = gp_adc_stop(client);
		break;
	default:
		break;
	}
	return ret;
}

static struct file_operations gp_adc_fops = {
	.owner		= THIS_MODULE,
	.read		= gp_adc_fops_read,
	.open		= gp_adc_fops_open,
	.release	= gp_adc_fops_release,
	.unlocked_ioctl = gp_adc_fops_ioctl,
};
/*
 *device driver probe
 */
static int gp_adc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct gp_adc_device_s *adc;
	adc_init_t adc_init;

	int ret;

	adc = kzalloc(sizeof(struct gp_adc_device_s), GFP_KERNEL);
	if (adc == NULL) {
		DIAG_ERROR("failed to allocate adc device\n");
		return -ENOMEM;
	}
	
	adc->pdev = pdev;

	adc->irq = IRQ_SAACC;
	ret = request_irq(adc->irq, gp_adc_irq, 0, dev_name(dev), adc);
	if (ret < 0) {
		DIAG_ERROR( "failed to attach adc irq\n");
		goto err_alloc;
	}
	
	gpHalAdcClkEnable(1);

	/* register misc device */
	adc->dev.name  = "adc";
	adc->dev.minor = MISC_DYNAMIC_MINOR;
	adc->dev.fops  = &gp_adc_fops;
	ret = misc_register(&adc->dev);
	if (ret != 0) {
		DIAG_ERROR("gp_adc device register fail\n");
		goto err_irq;
	}
	
	/*setup adc default settings*/
	adc_init.clk_tog_en = 0;	/*1-always toggling clock,0-toggling only in measurement*/
	adc_init.conv_dly = 0x20;	/*2 frame time*/
	adc_init.chkdly = 0;
	adc_init.x2y_dly = 0x10;	/*a frame time*/
	adc_init.interval_dly = 0x800*5;/*about 2msx5*/
	adc_init.debounce_dly = 0x800;/*about 2ms*/
	adc_init.clock_rate = 1000000ul;/*1.0MHz*/
	gpHalAdcInit(&adc_init);

	DIAG_INFO("attached adc driver\n");

	platform_set_drvdata(pdev, adc);
	adc_dev = adc;

	return 0;
err_irq:
	free_irq(adc->irq, adc);
err_alloc:
	kfree(adc);
	return ret;
}

static int gp_adc_remove(struct platform_device *pdev)
{
	struct gp_adc_device_s *adc = platform_get_drvdata(pdev);

	gpHalAdcClkEnable(0);
	free_irq(adc->irq, adc);
	kfree(adc);

	return 0;
}

#ifdef CONFIG_PM
static int gp_adc_suspend(struct platform_device *pdev, pm_message_t state)
{
	/*struct gp_adc_device_s *adc = platform_get_drvdata(pdev);*/

	return 0;
}

static int gp_adc_resume(struct platform_device *pdev)
{
	/*struct gp_adc_device_s *adc = platform_get_drvdata(pdev);*/

	return 0;
}

#else
#define gp_adc_suspend NULL
#define gp_adc_resume NULL
#endif

static struct platform_device gp_adc_device = {
	.name	= "gp-adc",
	.id	= -1,
};

static struct platform_driver gp_adc_driver = {
	.driver		= {
		.name	= "gp-adc",
		.owner	= THIS_MODULE,
	},
	.probe		= gp_adc_probe,
	.remove		= __devexit_p(gp_adc_remove),
	.suspend	= gp_adc_suspend,
	.resume		= gp_adc_resume,
};

static int __init gp_adc_module_init(void)
{
	int ret;
	
	platform_device_register(&gp_adc_device);
	ret = platform_driver_register(&gp_adc_driver);
	if (ret)
		DIAG_ERROR("%s: failed to add adc driver\n", __func__);

	return ret;
}

static void __exit gp_adc_module_exit(void)
{
	platform_device_unregister(&gp_adc_device);
	platform_driver_unregister(&gp_adc_driver);
}

module_init(gp_adc_module_init);
module_exit(gp_adc_module_exit);
MODULE_LICENSE_GP;
