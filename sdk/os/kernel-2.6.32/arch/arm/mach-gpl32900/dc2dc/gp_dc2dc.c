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
 *  Generalplus Inc.                                                      *
 *  3F, No.8, Dusing Rd., Hsinchu Science Park,                           *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/

 /**
 * @file gp_dc2dc.c
 * @brief dc2dc driver interface 
 * @author Daniel Huang
 */

#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/cdev.h>
#include <mach/diag.h>

#include <mach/hal/hal_dc2dc.h>
#include <mach/gp_dc2dc.h>
#include <mach/gp_board.h>

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define DC2DC_NAME	"dc2dc"

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct gp_dc2dc_s {
	struct miscdevice dev;     	 /*!< @brief dc2dc device */
	unsigned int period;		 /*!< @brief timeout period */
	unsigned long isOpened;
	int pin_handle;
} gp_dc2dc_t;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static gp_dc2dc_t gp_dc2dc_data;


/**
 * @brief dc2dc handle is valid
 * @return success: 1,  erro: 0
 */
static int valid_handle(int handle)
{
	return (handle == (int)&gp_dc2dc_data);
}

/**
 * @brief dc2dc request function
 * @return success: dc2dc handle,  erro: NULL
 */
int gp_dc2dc_request(void)
{
	if(test_and_set_bit(0,&gp_dc2dc_data.isOpened))
		return 0;

	return (int)&gp_dc2dc_data;
}
EXPORT_SYMBOL(gp_dc2dc_request);

/**
 * @brief dc2dc release function
 * @param handle [in] dc2dc handle
 * @return success: 0,  erro: erro_id
 */
int gp_dc2dc_release(int handle)
{
	if(!valid_handle(handle))
		return -EINVAL;

	clear_bit(0,&gp_dc2dc_data.isOpened);
	//gp_dc2dc_disable((int)&gp_dc2dc_data);
	return 0;
}
EXPORT_SYMBOL(gp_dc2dc_release);

/**
 * @brief dc2dc enable pwm0 function
 * @param handle [in] dc2dc handle
 * @return success: 0,  erro: erro_id
 */
int gp_dc2dc_enable_pwm0(int handle)
{
	if(!valid_handle(handle))
		return -EINVAL;

	gpHalDc2dcEnablePWM0(1);
	return 0;
}
EXPORT_SYMBOL(gp_dc2dc_enable_pwm0);

/**
 * @brief dc2dc disable pwm0 function
 * @param handle [in] dc2dc handle
 * @return success: 0,  erro: erro_id
 */
int gp_dc2dc_disable_pwm0(int handle)
{
	if(!valid_handle(handle))
		return -EINVAL;

	gpHalDc2dcEnablePWM0(0);
	return 0;
}
EXPORT_SYMBOL(gp_dc2dc_disable_pwm0);

/**
 * @brief dc2dc enable pwm1 function
 * @param handle [in] dc2dc handle
 * @return success: 0,  erro: erro_id
 */
int gp_dc2dc_enable_pwm1(int handle)
{
	if(!valid_handle(handle))
		return -EINVAL;

	gpHalDc2dcEnablePWM1(1);
	return 0;
}
EXPORT_SYMBOL(gp_dc2dc_enable_pwm1);

/**
 * @brief dc2dc disable pwm1 function
 * @param handle [in] dc2dc handle
 * @return success: 0,  erro: erro_id
 */
int gp_dc2dc_disable_pwm1(int handle)
{
	if(!valid_handle(handle))
		return -EINVAL;

	gpHalDc2dcEnablePWM1(0);
	return 0;
}
EXPORT_SYMBOL(gp_dc2dc_disable_pwm1);

/**
 * @brief dc2dc disable function
 * @param handle [in] dc2dc handle
 * @return success: 0,  erro: erro_id
 */
int gp_dc2dc_disable(int handle)
{
	if(!valid_handle(handle))
		return -EINVAL;

	gpHalDc2dcDisable();
	return 0;
}
EXPORT_SYMBOL(gp_dc2dc_disable);

/**
 * @brief   dc2dc device open
 */
static int gp_dc2dc_drv_open(struct inode *inode, struct file *file)
{
	if(test_and_set_bit(0,&gp_dc2dc_data.isOpened))
		return -EBUSY;
	else
		return 0;
}

/**
 * @brief   dc2dc driver release
 */
static int gp_dc2dc_drv_release(struct inode *inode, struct file *file)
{
	clear_bit(0,&gp_dc2dc_data.isOpened);
	gp_dc2dc_disable((int)&gp_dc2dc_data);
	return 0;
}

/**
 * @brief   dc2dc driver ioctl
 */
static long gp_dc2dc_drv_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch(cmd)
	{
            	case DC2DC_IOCTL_ENABLE_PWM0:
            		if(arg)
            		{
            			gp_dc2dc_enable_pwm0((int)&gp_dc2dc_data);
            		}
            		else
            		{
            			gp_dc2dc_disable_pwm0((int)&gp_dc2dc_data);
            		}
            		break;
            		
            	case DC2DC_IOCTL_ENABLE_PWM1:
            		if(arg)
            		{
            			gp_dc2dc_enable_pwm1((int)&gp_dc2dc_data);
            		}
            		else
            		{
            			gp_dc2dc_disable_pwm1((int)&gp_dc2dc_data);
            		}
            		break;            		
            		
            	case DC2DC_IOCTL_SET_FB_VOLTAGE:
            	
            		    gpHalDc2dcSetFeedbackVoltage(arg);
            		
            		break;            		
            		
	}
	return ret;
}

struct file_operations gp_dc2dc_fops = {
	.owner          = THIS_MODULE,
	.open           = gp_dc2dc_drv_open,
	.release        = gp_dc2dc_drv_release,
	.unlocked_ioctl = gp_dc2dc_drv_ioctl,
};

/**
 * @brief   dc2dc driver probe
 */
static int __init gp_dc2dc_probe(struct platform_device *pdev)
{
	int ret;

	gp_dc2dc_data.pin_handle = gp_board_pin_func_request(GP_PIN_DC2DC, GP_BOARD_WAIT_FOREVER);
	if (gp_dc2dc_data.pin_handle < 0) {
		DIAG_ERROR("[%s:%d] Error!\n", __FUNCTION__, __LINE__);
		ret = -EINVAL;
		goto fail_pin;
	}	
	
	gp_dc2dc_data.dev.name = DC2DC_NAME;
	gp_dc2dc_data.dev.minor  = MISC_DYNAMIC_MINOR;
	gp_dc2dc_data.dev.fops  = &gp_dc2dc_fops;

	ret = misc_register(&gp_dc2dc_data.dev);
	if(ret != 0){
		DIAG_ERROR("watchdog probe register fail\n");
		goto err_reg;
	}

	return 0;
err_reg:
	return ret;
	
fail_pin:
	kfree(&gp_dc2dc_data);	
	return ret;
}

#ifdef CONFIG_PM
static int gp_dc2dc_suspend(struct platform_device *pdev, pm_message_t state){

	gpHalDc2dcEnableCLK6M(0);
	return 0;
}

static int gp_dc2dc_resume(struct platform_device *pdev){

	gpHalDc2dcEnableCLK6M(1);
	return 0;
}
#else
#define gp_dc2dc_suspend      NULL
#define gp_dc2dc_resume       NULL
#endif

/**
 * @brief   dc2dc driver remove
 */
static int gp_dc2dc_remove(struct platform_device *pdev)
{
	gp_dc2dc_disable((int)&gp_dc2dc_data);
	misc_deregister(&gp_dc2dc_data.dev);
	return 0;
}

/**
 * @brief   dc2dc driver define
 */
static struct platform_driver gp_dc2dc_driver = {
	.probe	= gp_dc2dc_probe,
	.remove	= gp_dc2dc_remove,
	.suspend	= gp_dc2dc_suspend,
	.resume	= gp_dc2dc_resume,
	.driver	= {
		.owner	= THIS_MODULE,
		.name	= "gp-dc2dc"
	},
};


/**
 * @brief   dc2dc device release
 */
static void gp_dc2dc_device_release(struct device *dev)
{
	DIAG_INFO("remove watchdog device ok\n");
}

/**
 * @brief   dc2dc device resource
 */
static struct platform_device gp_dc2dc_device = {
	.name	= "gp-dc2dc",
	.id	= -1,
	.dev	= {
			.release = gp_dc2dc_device_release,
	},
};

/**
 * @brief   dc2dc driver init
 */
static int __init gp_dc2dc_drv_init(void)
{
	platform_device_register(&gp_dc2dc_device);
	return platform_driver_register(&gp_dc2dc_driver);
}

/**
 * @brief   dc2dc driver exit
 */
static void __exit gp_dc2dc_drv_exit(void)
{
	platform_device_unregister(&gp_dc2dc_device);
	platform_driver_unregister(&gp_dc2dc_driver);	
}

module_init(gp_dc2dc_drv_init);
module_exit(gp_dc2dc_drv_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/
MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("GP DC2DC Driver");
MODULE_LICENSE_GP;
