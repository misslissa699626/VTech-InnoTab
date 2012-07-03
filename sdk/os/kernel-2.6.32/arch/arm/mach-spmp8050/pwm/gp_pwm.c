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
 * @file gp_pwm.c
 * @brief pwm driver interface 
 * @author zaimingmeng
 */

#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/cdev.h>
#include <mach/diag.h>

#include <mach/gp_timer.h>
#include <mach/gp_pwm.h>
#include <mach/hal/hal_pwm.h>
#include <mach/gp_board.h>

#define PWM_NAME	"pwm"

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct gp_pwm_s {
	struct miscdevice dev;			/*!< @brief pwm device */

	int id;					/*!< @brief pwm device id */
	int timer_handle;			/*!< @brief pwm request timer handle */
	struct gp_pwm_config_s config;		/*!< @brief pwm config */
	struct list_head list;
} gp_pwm_t;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static LIST_HEAD(gp_pwm_list);
static int gp_pwm_pin = 0;


/**
 * @brief pwm enable function
 * @param handle [in] pwm handle
 * @return success: 0,  erro: ERROR_ID
 */
int gp_pwm_enable(int handle)
{
	struct gp_pwm_s *pwm = NULL;

	if(0 == handle){
		return -EINVAL;
	}
	
	pwm = (struct gp_pwm_s *)handle;
	
	gpHalPwmEn(pwm->id,1);

	return 0;
}
EXPORT_SYMBOL(gp_pwm_enable);

/**
 * @brief pwm disable function
 * @param handle [in] pwm handle
 * @return success: 0,  erro: ERROR_ID
 */
int gp_pwm_disable(int handle)
{	
	struct gp_pwm_s *pwm = NULL;

	if(0 == handle){
		return -EINVAL;
	}
	
	pwm = (struct gp_pwm_s *)handle;

	gpHalPwmEn(pwm->id,0);

	return 0;
}
EXPORT_SYMBOL(gp_pwm_disable);

/**
 * @brief pwm config set function
 * @param handle [in] pwm handle
 * @param config [in] config struct value(freq and duty)
 * @return success: 0,  erro: ERROR_ID
 */
int gp_pwm_set_config(int handle,struct gp_pwm_config_s *config)
{
	struct gp_pwm_s *pwm = NULL;

	if(0 == handle){
		return -EINVAL;
	}
	
	pwm = (struct gp_pwm_s *)handle;

	pwm->config.freq = config->freq;
	pwm->config.duty = config->duty;

	if(config->freq)
		gp_tc_set_freq_duty(pwm->timer_handle,config->freq,config->duty);
	return 0;
}
EXPORT_SYMBOL(gp_pwm_set_config);

/**
 * @brief pwm init function
 * @param handle [in] pwm handle
 * @return success: 0,  erro: ERROR_ID
 */
static int gp_pwm_init(int handle)
{
	struct gp_pwm_s *pwm = NULL;
	if(0 == handle){
		return -EINVAL;
	}
	
	pwm = (struct gp_pwm_s *)handle;

	pwm->config.freq = 0;
	pwm->config.duty = 0;
	
	gp_tc_init(pwm->timer_handle);
	
	gp_tc_enable_output(pwm->timer_handle,1);

	return 0;
    
}

/**
 * @brief pwm request function
 * @param pwm_id [in] pwm channel index
 * @return success: 0,  erro: ERROR_ID
 */
int gp_pwm_request(int pwm_id)
{
	int found = 0;
	struct gp_pwm_s *pwm = NULL;

	list_for_each_entry(pwm, &gp_pwm_list, list){
		if(pwm->id == pwm_id){
			found = 1;
			break;
		}
	}

	if(found){
		return (int)pwm;
	}
	else{
		return 0;
	}
}
EXPORT_SYMBOL(gp_pwm_request);

/**
 *@brief pwm free function
 *@param handle [in] pwm handle
 * @return success: 0,  erro: ERROR_ID
 */
int gp_pwm_release(int handle)
{
	return 0;
}
EXPORT_SYMBOL(gp_pwm_release);

/**
 * @brief   pwm driver open
 */
static int gp_pwm_dev_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	int found = 0;
	int minor = iminor(inode);
	struct gp_pwm_s *pwm = NULL;
	
	list_for_each_entry(pwm, &gp_pwm_list, list){
		if(minor == pwm->dev.minor){
			found = 1;
			break;
		}
	}

	if(!found){
		file->private_data = NULL;
		ret = -EBUSY;
	}
	else{
		file->private_data = pwm;
	}
	
	return ret;
}

/**
 * @brief   pwm driver ioctl
 */
static long gp_pwm_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct gp_pwm_s *pwm = (struct gp_pwm_s *)file->private_data;

	if(NULL == pwm){
		return -EFAULT;
	}

	switch(cmd){
	case PWM_IOCTL_CTRL:
		if(1 == arg){		
			gp_pwm_enable((int)pwm);		
		}
		else{
			gp_pwm_disable((int)pwm);
		}
		break;
		
	case PWM_IOCTL_SET_FREQ:
		pwm->config.freq = arg;
		gp_pwm_set_config((int)pwm, &pwm->config);
		break;
		
	case PWM_IOCTL_SET_DUTY:
		pwm->config.duty = arg;
		gp_pwm_set_config((int)pwm, &pwm->config);
		break;

	default:
		ret = -ENOTTY;
			
	}

	return ret;
}

struct file_operations gp_pwm_fops = {
	.owner          = THIS_MODULE,
	.open		= gp_pwm_dev_open,
	.unlocked_ioctl = gp_pwm_dev_ioctl,
};

/**
 * @brief   pwm driver probe
 */
static int gp_pwm_probe(struct platform_device *pdev)
{

	int ret;
	char *name = NULL;
	struct gp_pwm_s *pwm = NULL;

	pwm = kzalloc(sizeof(gp_pwm_t), GFP_KERNEL);
	if(!pwm)
		return -ENOMEM;
	
	INIT_LIST_HEAD(&pwm->list);

	name = kzalloc(5, GFP_KERNEL);
	if(!name){
		ret = -ENOMEM;
		goto _err_alloc_;
	}
	memset(name, 0, 5);
	sprintf(name, "pwm%d", pdev->id);

	pwm->dev.name  = name;
	pwm->dev.fops  = &gp_pwm_fops;
	pwm->dev.minor  = MISC_DYNAMIC_MINOR;
	pwm->id = pdev->id;

	pwm->timer_handle = gp_tc_request(pwm->id, name);
	
	if(0 == pwm->timer_handle){
		DIAG_ERROR("pwm request timer erro\n");
		goto _err_timer_;
	}
	gp_pwm_init((int)pwm);

	ret = misc_register(&pwm->dev);
	if(ret != 0){
		DIAG_ERROR(KERN_ALERT "pwm probe register fail\n");
		ret = -EINVAL;
		goto _err_reg_;
	}

	list_add(&pwm->list, &gp_pwm_list);
	
	platform_set_drvdata(pdev, pwm);
	
	DIAG_INFO("pwm probe ok\n");
	return 0;
	
_err_reg_:
	gp_tc_release(pwm->timer_handle);

_err_timer_:
	kfree(name);

_err_alloc_:
	kfree(pwm);
	
	return ret;
}

/**
 * @brief   pwm driver remove
 */
static int gp_pwm_remove(struct platform_device *pdev)
{
	int found = 0;
	struct gp_pwm_s *pwm = NULL;
	
	list_for_each_entry(pwm, &gp_pwm_list, list){
		if(pwm->id == pdev->id){
			found = 1;
			break;
		}
	}
	
	if(1 == found){
		misc_deregister(&pwm->dev);
		
		gp_tc_release(pwm->timer_handle);
		if(pwm->dev.name){
			kfree(pwm->dev.name);
		}
		list_del(&pwm->list);
		kfree(pwm);
	}
	return 0;
}

/**
 * @brief   pwm driver define
 */
static struct platform_driver gp_pwm_driver = {
	.probe  = gp_pwm_probe,
	.remove = gp_pwm_remove,
	.driver = {
		.name  = "gp-pwm",
		.owner = THIS_MODULE,
	}
};

/**
 * @brief   wdt device release
 */
static void gp_pwm_device_release(struct device *dev)
{
	DIAG_INFO("remove pwm device ok\n");
}

static struct platform_device gp_pwm_device0 = {
	.name	= "gp-pwm",
	.id	= 0,
	.dev	= {
			.release = gp_pwm_device_release,
	},
};

static struct platform_device gp_pwm_device1 = {
	.name	= "gp-pwm",
	.id	= 1,
	.dev	= {
			.release = gp_pwm_device_release,
	},
};

static struct platform_device gp_pwm_device2 = {
	.name	= "gp-pwm",
	.id	= 2,
	.dev	= {
			.release = gp_pwm_device_release,
	},
};

static struct platform_device *gp_pwm_devices[] = {
	&gp_pwm_device0,
	&gp_pwm_device1,
	&gp_pwm_device2
};

/**
 * @brief pwm dev infomation register function
 * @param pwm_dev [in] pwm dev handle
 * @return success: 0,  erro: ERROR_ID
 */
static int gp_pwm_register_device(void)
{
	int i;
	struct gp_board_pwm_s *pwm_dev_config;

	gp_pwm_pin = gp_board_pin_func_request(GP_PIN_PWM, GP_BOARD_WAIT_FOREVER);
	if (gp_pwm_pin < 0) {
		DIAG_ERROR("[%s:%d] Error!\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}

	pwm_dev_config = gp_board_get_config("pwm",gp_board_pwm_t);
	if(!pwm_dev_config){
		DIAG_ERROR("[%s:%d] Error!\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}

	for(i = 0; i < pwm_dev_config->count; i++){
		if((pwm_dev_config->channel[i] >= 0)&&(pwm_dev_config->channel[i] <= 2))
			platform_device_register(gp_pwm_devices[pwm_dev_config->channel[i]]);
	}
	return 0;
}

/**
 * @brief pwm dev infomation register function
 * @param pwm_dev [in] pwm dev handle
 * @return success: 0,  erro: ERROR_ID
 */
static int gp_pwm_unregister_device(void)
{
	int i;
	struct gp_board_pwm_s *pwm_dev_config;

	gp_board_pin_func_release(gp_pwm_pin);

	pwm_dev_config = gp_board_get_config("pwm",gp_board_pwm_t);
	if(!pwm_dev_config){
		DIAG_ERROR("[%s:%d] Error!\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}

	for(i = 0; i < pwm_dev_config->count; i++){
		if((pwm_dev_config->channel[i] >= 0)&&(pwm_dev_config->channel[i] <= 2))
			platform_device_unregister(gp_pwm_devices[pwm_dev_config->channel[i]]);
	}
	return 0;
}

/**
 * @brief   pwm device register
 */
int gp_pwm_device_register(void)
{
	return gp_pwm_register_device();
}
EXPORT_SYMBOL(gp_pwm_device_register);

/**
 * @brief   pwm device unregister
 */
void gp_pwm_device_unregister(void)
{
	gp_pwm_unregister_device();
}
EXPORT_SYMBOL(gp_pwm_device_unregister);

/**
 * @brief   pwm driver init
 */
static int __init gp_pwm_drv_init(void)
{
	return platform_driver_register(&gp_pwm_driver);
}

/**
 * @brief   pwm driver exit
 */
static void __exit gp_pwm_drv_exit(void)
{
	platform_driver_unregister(&gp_pwm_driver);
}

module_init(gp_pwm_drv_init);
module_exit(gp_pwm_drv_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/
MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("GP pwm Driver");
MODULE_LICENSE_GP;
