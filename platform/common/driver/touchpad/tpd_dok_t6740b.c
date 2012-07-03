/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2011 by Generalplus Technology Co., Ltd.         *
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
 *  antion @2011.03.09                                                    *
 *                                                                        *
 **************************************************************************/

 /**
 * @file gp_touchpad.c
 * @brief used IIC driver interface 
 * @author antion
 */

#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/cdev.h>
#include <mach/diag.h>
#include <mach/hardware.h>

#include <mach/hal/hal_i2c_bus.h>
#include <mach/gp_i2c_bus.h>
#include <mach/gp_board.h>
//----------------------------------
#include <linux/init.h>
#include <linux/fs.h> 
#include <mach/gp_display.h>
#include <mach/general.h>

#include <mach/gp_gpio.h>
#include <linux/delay.h> 
#include <mach/hal/hal_gpio.h>
#include <linux/input.h>
//*************************************************************************
/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

#define TOUCHPAD_NAME	"touchpad"

#define TP_YOF		0x80
#define TP_XOF		0x40
#define TP_YSIGN	0x20
#define TP_XSIGN	0x10
#define TP_MBTN		0x04
#define TP_RBTN		0x02
#define TP_LBTN		0x01

#define tp_debug_print		1

#define	touchpad_set_scl_channel		1
#define	touchpad_set_scl_func		3
#define	touchpad_set_scl_gid			13
#define	touchpad_set_scl_pin			17
#define	touchpad_set_scl_level		1

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
//static gpio_content_t cmd_rx = {0};
typedef struct gp_touchpad_s{
	struct miscdevice dev; 	 /*!< @brief spi device */

	struct semaphore touchpad_sem;
	int pin_handle;
	struct input_dev *input;
}gp_touchpad_t;

typedef void (*irq_callback)(void *);

typedef struct gpio_isr_s
{
    irq_callback cbk;           /*!< @brief callback function */
	void *priv_data;            /*!< @brief private data */
} gpio_isr_t;

typedef struct gpio_handle_s {
	unsigned int pin_index;     /*!< @brief pin index */
	char *name;                 /*!< @brief owner name */
	gpio_isr_t isr;             /*!< @brief pin isr */
} gpio_handle_t;

typedef struct gp_touchpad_irq_s {	
	int pin_scl;
	int pin_sda;
	int gpiohd;      
	struct input_dev *input;
} gp_touchpad_irq_t;
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static struct gp_touchpad_s *gp_touchpad_data;     	 /*!< @brief spi device */
static struct gp_touchpad_irq_s *gp_touchpad_irq_data;
static int iic_handle;

extern int gp_gpio_request_irq(int gpio_id, char *name,
						void (*irq_handler)(int, void *), void *data);

/*
static int
gpiocfgOut(
	int channel,
	int func,
	int gid,
	int pin,
	int level
)
{
	int handle;
	gpio_content_t ctx;

	if ( channel == 0xff ) {
		return -1;	
	}

	ctx.pin_index = MK_GPIO_INDEX( channel, func, gid, pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );

	gp_gpio_set_function( handle, func );
	gp_gpio_set_direction( handle, GPIO_DIR_OUTPUT );
	gp_gpio_set_output( handle, level, 0 );
	
	gp_gpio_release( handle );
	return	0;
}*/

static int gp_touchpad_request_gpio(void)
{
	//int ret = 0;
	//int value;
	printk("begin to request gpio\n");
	gp_touchpad_irq_data->gpiohd = gp_gpio_request(MK_GPIO_INDEX( touchpad_set_scl_channel, touchpad_set_scl_func, touchpad_set_scl_gid, touchpad_set_scl_pin ), "touchpad");
	if(IS_ERR_VALUE(gp_touchpad_irq_data->gpiohd)){
		DIAG_ERROR("touchpad request gpio clk = %d\n", gp_touchpad_irq_data->gpiohd);
		return -EINVAL;
	}
	
	gp_gpio_set_function(gp_touchpad_irq_data->gpiohd, GPIO_FUNC_GPIO);
	printk("touchpad set gpio function handle=0x%x\n",gp_touchpad_irq_data->gpiohd);
	gp_gpio_set_direction(gp_touchpad_irq_data->gpiohd, GPIO_DIR_INPUT);
	printk("touchpad set gpio direction\n");
	return 0;
}

/**
 * @brief touchpad device open
 */
static int gp_touchpad_dev_open(struct inode *inode, struct file *file)
{
	return 0;
}

/**
 * @brief touchpad device close
 */
static int gp_touchpad_dev_release(struct inode *inode, struct file *file)
{
	return 0;
}

/**
 * @brief touchpad device ioctl
 */
static long gp_touchpad_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return 0;
}

/**
 * @brief touchpad device read
 */
static ssize_t gp_touchpad_dev_read(struct file *file, char __user *buf,size_t count, loff_t *oppos)
{
	return 0;
}

/**
 * @brief touchpad device write
 */
static ssize_t gp_touchpad_dev_write(struct file *file, const char __user *buf,size_t count, loff_t *oppos)
{
	return 0;
}

/**
 * @brief touchpad device gpio ext irq
 */

static void gp_touchpad_gpioIrq(void *data)
{
	int ret,tmp;
	char datax[3];
	int *p;
	if (down_interruptible(&gp_touchpad_data->touchpad_sem) != 0) 
	{
		return;
	}
	gp_gpio_enable_irq(gp_touchpad_irq_data->gpiohd, 0);
	p = (int*)IO0_ADDRESS(0x5080);
	*p &= 0xf3ffffff;
	ret = gp_i2c_bus_read(iic_handle, (unsigned char*)&datax, 3);
	*p |= 0x0c000000;

	if((datax[1]&0x80) == 0)
	{
		tmp = (int)(datax[1]);
	}
	else
	{
		
		tmp = (int)(datax[1]);
		tmp -= 0xff;
	}
	input_report_rel((gp_touchpad_data->input), REL_X, tmp);
#if tp_debug_print
	printk("Xm:%3d-%x\n",tmp,tmp);
#endif

	if((datax[2]&0x80) == 0)
	{
		tmp = (int)(datax[2]);
	}
	else
	{
		tmp = (int)(datax[2]);
		tmp -= 0xff;
	}
	input_report_rel((gp_touchpad_data->input), REL_Y, tmp);
#if tp_debug_print
	printk("Ym:%3d-%x\n",tmp,tmp);
#endif

	//left button
	if(datax[0]&TP_LBTN)
	{
		input_report_key(gp_touchpad_data->input, BTN_LEFT,   1);
#if tp_debug_print
		printk("LB:+++\n");
#endif
	}
	else
	{
		input_report_key(gp_touchpad_data->input, BTN_LEFT,   0);
#if tp_debug_print
		printk("LB:...\n");
#endif
	}
	//right button
	if(datax[0]&TP_RBTN)
	{
		input_report_key(gp_touchpad_data->input, BTN_RIGHT,   1);
#if tp_debug_print
		printk("RB:+++\n");
#endif
	}
	else
	{
		input_report_key(gp_touchpad_data->input, BTN_RIGHT,   0);
#if tp_debug_print
		printk("RB:...\n");
#endif
	}
	//middle button
	if(datax[0]&TP_MBTN)
	{
		input_report_key(gp_touchpad_data->input, BTN_MIDDLE,   1);
#if tp_debug_print
		printk("MB:+++\n");
#endif
	}
	else
	{
		input_report_key(gp_touchpad_data->input, BTN_MIDDLE,   0);
#if tp_debug_print
		printk("MB:...\n");
#endif
	}

	input_sync(gp_touchpad_data->input);
#if tp_debug_print
	printk("cmd:%u,data0:%d,data1:%d\n",datax[0],datax[1],datax[2]);
#endif
	gp_gpio_set_direction(gp_touchpad_irq_data->gpiohd, GPIO_DIR_INPUT);
	gp_gpio_set_input(gp_touchpad_irq_data->gpiohd, 1);
	gp_gpio_enable_irq(gp_touchpad_irq_data->gpiohd, 1);
	up(&gp_touchpad_data->touchpad_sem);
	return;
}

/**
 * @brief touchpad device iic init
 */
static int gp_touchpad_iic_init(void)
{
	iic_handle = gp_i2c_bus_request(0xa, 0x1);
	if(iic_handle == -ENOMEM)
	{
		printk("gp_i2c_bus_request(0x5, 0x1) ret=0x%x\n",iic_handle);
		return -1;
	}
	return 0;
}

struct file_operations touchpad_fops = 
{
	.owner          = THIS_MODULE,
	.open           = gp_touchpad_dev_open,
	.release        = gp_touchpad_dev_release,
	.unlocked_ioctl = gp_touchpad_dev_ioctl,
	.read			= gp_touchpad_dev_read,
	.write			= gp_touchpad_dev_write,
};



/**
 * @brief touchpad device resource
 */
static struct platform_device gp_touchpad_device = {
	.name	= "gp-touchpad",
	.id	= 0,
};

/**
 * @brief touchpad device probe
 */
static int gp_touchpad_probe(struct platform_device *pdev)
{
	int ret = 0;
	int value = 0;
	char *data;
	struct input_dev *input;

	printk("K-gp_touchpad_probe0....\n");
	gp_touchpad_irq_data = kzalloc(sizeof(gp_touchpad_irq_t), GFP_KERNEL);
	if(!gp_touchpad_irq_data){
		return -ENOMEM;
	}
	memset(gp_touchpad_irq_data, 0, sizeof(gp_touchpad_irq_t));
	printk("K-gp_touchpad_probe1....\n");
	data = kzalloc(8, GFP_KERNEL);
	if(!data){
		return -ENOMEM;
	}
	memset(data, 0, 8);
	printk("K-gp_touchpad_probe2....\n");
	gp_touchpad_data = kzalloc(sizeof(gp_touchpad_t), GFP_KERNEL);
	if(!gp_touchpad_data){
		return -ENOMEM;
	}
	memset(gp_touchpad_data, 0, sizeof(gp_touchpad_t));

	init_MUTEX(&gp_touchpad_data->touchpad_sem);

	
	printk("touch pad allocate input device\n");
	input = input_allocate_device();
	if (!input) {
		ret = -ENOMEM;
		printk("input alloc erro\n");
		goto erro_req;
	}

	input->name = pdev->name;
	input->phys = "touchpad/input0";
	input->dev.parent = &pdev->dev;

	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0100;
	input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
	input->keybit[BIT_WORD(BTN_LEFT)] = BIT_MASK(BTN_LEFT) |BIT_MASK(BTN_RIGHT);
	input->relbit[0] = BIT_MASK(REL_X) | BIT_MASK(REL_Y) | BIT_MASK(REL_WHEEL);

	gp_touchpad_data->input = input;
	printk("touch pad register input drv data\n");
	input_set_drvdata(input, &gp_touchpad_data);
	printk("touch pad register device\n");
	ret = input_register_device(input);
	if (ret){
		goto erro_input;
	}

	gp_touchpad_data->dev.name  = TOUCHPAD_NAME;
	gp_touchpad_data->dev.minor = MISC_DYNAMIC_MINOR;
	gp_touchpad_data->dev.fops  = &touchpad_fops;
	printk("touch pad register dev\n");
	ret = misc_register(&gp_touchpad_data->dev);
	if(ret != 0){
		DIAG_ERROR("touchpad probe register fail\n");
		return -1;
	}
	printk("touch pad iic init\n");
	ret = gp_touchpad_iic_init();
	if(ret != 0){
		DIAG_ERROR("touchpad probe iic init fail\n");
		return -1;
	}
	printk("set drvdata\n");
	platform_set_drvdata(pdev,&gp_touchpad_data);
	printk("init touchpad sem\n");
	init_MUTEX(&gp_touchpad_data->touchpad_sem);
	printk("enable gpio isr\n");

	gp_touchpad_request_gpio();
	printk("touchpad request gpio finish.\n");
	gp_gpio_register_isr(gp_touchpad_irq_data->gpiohd, &gp_touchpad_gpioIrq, "touchpad_isr"); 
	printk("touchpad register isr,cb=0x%x\n",(int)&gp_touchpad_gpioIrq);

	gp_gpio_irq_property(gp_touchpad_irq_data->gpiohd, GPIO_IRQ_EDGE_TRIGGER|GPIO_IRQ_ACTIVE_FALING, (int*)&value);
	printk("touchpad gpio irq property,val=0x%x\n",(int)&value);
	printk("gp_touchpad_probe finish!\n");
	return 0;
erro_input:
	input_free_device(input);
erro_req:
	printk("gp_touchpad_probe error!\n");
	return -1;
}



/**
 * @brief touchpad device remove
 */
static int gp_touchpad_remove(struct platform_device *pdev)
{
	gp_gpio_enable_irq(gp_touchpad_irq_data->pin_scl, 0);
	misc_deregister(&gp_touchpad_data->dev);
	gp_i2c_bus_release(iic_handle);
	kfree(gp_touchpad_data);
	kfree(gp_touchpad_irq_data);
	return 0;
}




static struct platform_driver gp_touchpad_driver = {
	.probe	= gp_touchpad_probe,
	.remove	= gp_touchpad_remove,
	.driver	= 
	{
		.owner	= THIS_MODULE,
		.name	= "gp-touchpad"
	},
};



/**
 * @brief touchpad driver init
 */
static int __init gp_touchpad_drv_init(void)
{
	printk("gp_touchpad_drv_init\n");
	platform_device_register(&gp_touchpad_device);
	return platform_driver_register(&gp_touchpad_driver);
}

/**
 * @brief touchpad driver exit
 */
static void __exit gp_touchpad_drv_exit(void)
{
	printk("gp_touchpad_drv_exit\n");
	platform_device_unregister(&gp_touchpad_device);
	platform_driver_unregister(&gp_touchpad_driver);
}


module_init(gp_touchpad_drv_init);
module_exit(gp_touchpad_drv_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/
MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("GP TouchPad Driver");
MODULE_LICENSE_GP;
