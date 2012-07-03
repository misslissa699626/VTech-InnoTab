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
 *  3F, No.8, Dusing Rd., Science-Based Industrial Park,                  *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 **************************************************************************/
 
 /**
 * @file joystick.c
 * @brief joystick interface
 * @author Simon Hsu
 */

#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/diag.h>
#include <mach/gp_gpio.h>
#include <mach/diag.h>
#include <linux/input-polldev.h>
#include <mach/hal/hal_pwrc.h>
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
															
/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#if 0
	#define DEBUG	DIAG_ERROR
#else
	#define DEBUG(...)
#endif

#define POWER_KEY_VALUE		KEY_POWER

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
struct gp_js_platform_data_s {
	int scan_ms;	/*scan interval(unit: ms)*/
	int pinClk;	/*pin index of clock*/
	int pinDat;	/*pin index of data*/
	int pinStb;	/*pin index of strobe*/
	int nr_keys;	/*number of keys*/
	const int *code;	/*key code*/
};

typedef struct gp_js_dev_s {
	struct input_polled_dev *input_poll;
	int nr_keys;
	const int *key_code;
	int handle_clk;
	int handle_dat;
	int handle_stb;
} gp_js_dev_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/


/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/

/**
 * @brief   charater device ioctl function
 * @return  success: 0
 * @see
 */ 
static void gp_js_report_event(struct input_polled_dev *pdev)
{
	struct gp_js_dev_s *ddata = pdev->private;
	int32_t i;
	int32_t state;

	gp_gpio_set_output( ddata->handle_clk, 1, 0 );		/*CLK high*/
	gp_gpio_set_output( ddata->handle_stb, 1, 0 );		/*STB low*/
	udelay(2);
	gp_gpio_set_output( ddata->handle_stb, 0, 0 );

	for(i = 0; i < ddata->nr_keys; i++) {
		gp_gpio_set_output( ddata->handle_clk, 1, 0 );		/*CLK hight*/
		udelay(3);
		gp_gpio_set_output( ddata->handle_clk, 0, 0 );		/*CLK low*/
		udelay(2);
		gp_gpio_get_value( ddata->handle_dat, &state);
		input_event(ddata->input_poll->input, EV_KEY, ddata->key_code[i], !state);
	}

	/*check for power key*/
	if(gpHalPwron0Get()){
		input_event(ddata->input_poll->input, EV_KEY, POWER_KEY_VALUE, 1);
	}
	else{
		input_event(ddata->input_poll->input, EV_KEY, POWER_KEY_VALUE, 0);
	}
	
	input_sync(ddata->input_poll->input);
}

/**
 * @brief   character device module exit function
 * @see
 */ 
static int __exit gp_js_remove(struct platform_device *pdev)
{
	struct gp_js_dev_s *ddata = platform_get_drvdata(pdev);
		
	if( !IS_ERR_VALUE(ddata->handle_clk)) {
		gp_gpio_release(ddata->handle_clk);
	}
	if( !IS_ERR_VALUE(ddata->handle_dat)) {
		gp_gpio_release(ddata->handle_dat);
	}
	if( !IS_ERR_VALUE(ddata->handle_stb)) {
		gp_gpio_release(ddata->handle_stb);
	}

	if( ddata->input_poll ) {
		input_free_polled_device(ddata->input_poll);
	}
	if( ddata ) {
		platform_set_drvdata(pdev, NULL);
		kfree(ddata);
	}
	return 0;
}

/**
 * @brief   character device module init function
 * @return  success: 0
 * @see
 */ 
static int32_t gp_js_probe(struct platform_device *pdev)
{
	struct gp_js_platform_data_s *pdata = pdev->dev.platform_data;
	struct gp_js_dev_s *ddata;
	struct input_dev *input;
	struct input_polled_dev *input_poll;
	int i, error = 0;
	
	/*alloc ram for data struct*/
	ddata = kzalloc(sizeof(struct gp_js_dev_s), GFP_KERNEL); 
	input_poll = input_allocate_polled_device();
	if (!ddata || !input_poll) {
		DIAG_ERROR("gp_js_probe ddata=%p,input_poll=%p\n",ddata, input_poll);
		error = -ENOMEM;
		goto __err_alloc_mem;
	}
	platform_set_drvdata(pdev, ddata);
	ddata->nr_keys = pdata->nr_keys;
	ddata->key_code = pdata->code;
	ddata->input_poll = input_poll;

	/* set input-polldev handlers */
	input_poll->private = ddata;
	input_poll->poll = gp_js_report_event;
	input_poll->poll_interval = pdata->scan_ms;

	input = input_poll->input;
	input->name = pdev->name;
	input->id.bustype = BUS_HOST;
	input->dev.parent = &pdev->dev;

	/* ----- Set pin IO ----- */
	ddata->handle_clk = gp_gpio_request(pdata->pinClk, "js_clk");
	ddata->handle_dat = gp_gpio_request(pdata->pinDat, "js_dat");
	ddata->handle_stb = gp_gpio_request(pdata->pinStb, "js_stb");
	if( IS_ERR_VALUE(ddata->handle_clk)
		|| IS_ERR_VALUE(ddata->handle_dat)
		|| IS_ERR_VALUE(ddata->handle_stb) ) {
		DIAG_ERROR("gp_js_probe request GPIO failed\n");
		error = -EIO;
		goto __err_request_gpio;
	}

	gp_gpio_set_input(ddata->handle_dat, GPIO_PULL_HIGH);
	gp_gpio_set_output(ddata->handle_stb, 0, 0);
	gp_gpio_set_output(ddata->handle_clk, 0, 0);
	
	for( i = 0; i < pdata->nr_keys; i++ ) {
		input_set_capability(input, EV_KEY, pdata->code[i]);
	}
	/*setup the power key*/
	input_set_capability(input, EV_KEY, POWER_KEY_VALUE);


	/*register polling device*/
	error = input_register_polled_device(input_poll);
	if (error) {
		DIAG_ERROR("could not register device: gp-js\n");
		error = -EIO;
		goto __err_request_gpio;
	}
	DIAG_INFO("load gp-js OK, handle clk=%x,dat=%x,stb=%x,nr_keys=%d\n",
		ddata->handle_clk,ddata->handle_dat,ddata->handle_stb,ddata->nr_keys);
	return 0;

__err_request_gpio:
	if( !IS_ERR_VALUE(ddata->handle_clk)) {
		gp_gpio_release(ddata->handle_clk);
	}
	if( !IS_ERR_VALUE(ddata->handle_dat)) {
		gp_gpio_release(ddata->handle_dat);
	}
	if( !IS_ERR_VALUE(ddata->handle_stb)) {
		gp_gpio_release(ddata->handle_stb);
	}
__err_alloc_mem:
	if( input_poll ) {
		input_free_polled_device(input_poll);
	}
	if( ddata ) {
		platform_set_drvdata(pdev, NULL);
		kfree(ddata);
	}
	return error;
}

/**/
static const int key_codes[] = {KEY_B, KEY_A, KEY_ENTER, KEY_ESC, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT};
static struct gp_js_platform_data_s gp_js_platform_data = {
	.scan_ms = 20,	/*scan every 20ms*/
#if 0
	.pinDat = ((2<<24)|(2<<16)|(4<<8)|0),	/* JOYSTICK DATA pin 5*/ 
	.pinStb = ((2<<24)|(2<<16)|(4<<8)|2),	/* JOYSTICK STB pin 6*/  
	.pinClk = ((2<<24)|(2<<16)|(4<<8)|4),	/* JOYSTICK CLK pin 7*/
#else
	.pinDat = ((0<<24)|(0<<16)|(23<<8)|6),	/* IOA6*/ 
	.pinStb = ((0<<24)|(0<<16)|(22<<8)|5),	/* IOA5*/  
	.pinClk = ((0<<24)|(0<<16)|(22<<8)|4),	/* IOA4*/
#endif
	//.pinDat = ((0<<24)|(0<<16)|(22<<8)|5),	/* JOYSTICK DATA IOA5-pin180*/ 
	//.pinStb = ((0<<24)|(0<<16)|(22<<8)|4),	/* JOYSTICK STB IOA4-pin181*/  
	//.pinClk = ((0<<24)|(0<<16)|(21<<8)|3),	/* JOYSTICK CLK IOA3-pin182*/
	.nr_keys = ARRAY_SIZE( key_codes ),
	.code = key_codes,
};


void gp_js_suspend_set( void ){
}
EXPORT_SYMBOL(gp_js_suspend_set);

void gp_js_resume_set( void ){
}
EXPORT_SYMBOL(gp_js_resume_set);

#ifdef CONFIG_PM
static int gp_js_suspend(struct platform_device *pdev, pm_message_t state){
	gp_js_suspend_set();
	return 0;
}

static int gp_js_resume(struct platform_device *pdev){
	gp_js_resume_set();
	return 0;
}
#else
#define gp_js_suspend NULL
#define gp_js_resume NULL
#endif

static void gp_js_device_release(struct device * dev)
{
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
}

static struct platform_device gp_js_device = {
	.name	= "gp-js",
	.id	= -1,
	.dev	= {
		.platform_data = &gp_js_platform_data,
		.release = &gp_js_device_release
	},
};

static struct platform_driver gp_js_driver = {
	.probe		= gp_js_probe,
	.remove		= __devexit_p(gp_js_remove),
	.suspend = gp_js_suspend,
	.resume = gp_js_resume,
	.driver		= {
		.name	= "gp-js",
		.owner	= THIS_MODULE,
	}
};

static int __init gp_js_init(void)
{
	platform_device_register(&gp_js_device);
	return platform_driver_register(&gp_js_driver);
}

static void __exit gp_js_exit(void)
{
	platform_driver_unregister(&gp_js_driver);	
	platform_device_unregister(&gp_js_device);
}

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/
module_init(gp_js_init);
module_exit(gp_js_exit);
MODULE_LICENSE_GP;


