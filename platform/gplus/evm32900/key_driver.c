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
 * @file key_driver.c
 * @brief ADC key and GPIO keys driver
 * @author zh.l
 */
#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/cdev.h>
#include <mach/gp_adc.h>
#include <mach/gp_gpio.h>
#include <linux/input-polldev.h>
#include <mach/hal/hal_pwrc.h>
#include "sysconfig.h"
#include <mach/gp_wdt.h>



#define MK_GPIO_INDEX(channel, func, gid, pin) ((channel<<24)|(func<<16)|(gid<<8)|(pin))
#define POWER_KEY_VALUE		128 /*KEY_POWER*/
#undef SYSCONFIG_INTERNAL_ADC

static int config = 0;
static int wdt = 0;
static int poweroff = 5;
static int offcnt = 0;	
static int wdtclose = 0;

/** gpio per key data*/
struct gp_gpio_button_s {
	/* Configuration parameters */
	int code;		/* input event code (KEY_*, SW_*) */
	int pin;		/*key gpio pin index*/
	int active_low;		/*1=>active low,0=>active high*/
	char *desc;
	int pressed;
};

/** adc per key data*/
struct gp_adc_button_s {
	/* Configuration parameters */
	int code;		/* input event code (KEY_*, SW_*) */
	int min;		/*key ad min value*/
	int max;		/*key ad max value*/
	char *desc;
	int pressed;
};

struct gp_adc_channel_data_s {
	int channel;	/** which adc channel*/
	int nr_keys;		/** total keys on this channel*/
	struct gp_adc_button_s *keys;		/** per key info, inclue key value,min/max adc value*/
};

struct gp_keys_platform_data_s {
	int scan_ms;		/** scan interval in ms*/
	int nr_channels;	/** total adc channeles using for adc key*/
	int nr_gpios;		/** total gpio-keys*/
	struct gp_adc_channel_data_s *channel_data;/** per channel info, inclue keys and per-key info*/
	struct gp_gpio_button_s *gpio_data;/** per gpio key info, inclue pin and code*/
};

struct gp_keys_drvdata {
	struct input_polled_dev *input_poll;
	struct gp_keys_platform_data_s *pdata;
	int *handle_gpio;	/* start key GPIO handle*/
	int *handle_adc;	/* handles of adc clients*/
	int handles[0];
};

/*****************************************************************************/
/* Constant Definitions */

static const char MapTbl_32900[4][32][2] =
{
    {
        { 0, 18 }, { 0, 19 }, { 0, 20 }, { 0, 21 }, { 0, 22 }, { 0, 22 }, { 0, 23 }, { 0, 24 },
        { 0, 12 }, { 0, 11 }, { 2,  0 }, { 2,  0 }, { 2,  1 }, { 2,  2 }, { 2,  3 }, { 0,  7 },
        { 0,  0 }, { 0,  0 }, { 0,  0 }, { 0,  0 }, { 2, 26 }, { 2, 27 }, { 2, 28 }, { 2, 29 },
        { 2, 30 }, { 2, 30 }, { 2, 34 }, { 2, 34 }, { 2, 34 }, { 2, 31 }, { 2, 32 }, { 0, 0  }
    },
    {
        { 0,  8 }, { 0,  8 }, { 0,  8 }, { 0,  8 }, { 0,  8 }, { 0,  8 }, { 0,  8 }, { 0,  8 },
        { 2,  5 }, { 2,  5 }, { 2,  5 }, { 2,  5 }, { 2,  5 }, { 2,  5 }, { 2,  5 }, { 2,  5 },
        { 3, 13 }, { 3, 13 }, { 2, 33 }, { 2, 33 }, { 2, 14 }, { 2, 14 }, { 2, 14 }, { 2, 15 },
        { 2, 15 }, { 2, 15 }, { 2, 16 }, { 2, 16 }, { 2, 16 }, { 2, 17 }, { 2, 17 }, { 0,  0 }
    },
    {
        { 2, 4 }, { 2, 4 }, { 2, 4 }, { 2, 4 }, { 2, 4 }, { 2, 4 }, { 2, 4 }, { 2, 4 },
        { 0, 9 }, { 0, 9 }, { 0, 9 }, { 0, 9 }, { 0, 9 }, { 0, 9 }, { 0, 9 }, { 0, 9 },
        { 2, 5 }, { 2, 5 }, { 2, 5 }, { 2, 5 }, { 2, 5 }, { 2, 5 }, { 0, 0 }, { 0, 0 },
        { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }
    },
    {
        { 0,  6 }, { 0,  6 }, { 0,  6 }, { 0,  6 }, { 0,  6 }, { 0,  6 }, { 0,  6 }, { 0,  6 },
        { 0,  6 }, { 0,  6 }, { 0,  6 }, { 2, 25 }, { 2, 25 }, { 2, 25 }, { 2, 25 }, { 2, 25 },
        { 2, 25 }, { 2, 25 }, { 2, 25 }, { 2, 25 }, { 2, 25 }, { 2, 25 }, { 2, 25 }, { 2, 25 },
        { 2, 25 }, { 0,  0 }, { 0,  0 }, { 0,  0 }, { 0,  0 }, { 0,  0 }, { 0,  0 }, { 0,  0 }
    }
};

#define MK_GPIO_INDEX(CHANNEL, FUNC, GID, PIN) (((CHANNEL) << 24)|((FUNC) << 16)|((GID) << 8)|(PIN))

/** watchdog time*/


static int gp_wdt_data = 0;

static int watchdog_init(void)
{
	gp_wdt_data = gp_wdt_request();
	if(gp_wdt_data == 0)
	{
		return -1;
	}

	gp_wdt_set_timeout(gp_wdt_data,wdt);
	gp_wdt_enable(gp_wdt_data);		//enable watchdog
	printk("###watchdog_init###\n");

	return 0;
}
static int watchdog_alive(void)
{
	if(gp_wdt_data == 0)
	{
		return -1;
	}
	gp_wdt_keep_alive(gp_wdt_data);
	//printk("gp_wdt_keep_alive\n");
	return 0;
}
static void watchdog_close(void)
{	
	if(gp_wdt_data == 0)
	{
		return -1;
	}
	if(wdtclose)
	{
		gp_wdt_disable(gp_wdt_data);
		printk("###watchdog_close###\n");
	}
}

static int gpiocfgOut(int channel, int func, int gid, int pin, int level)
{
	int handle;
	gpio_content_t ctx;

	if ( channel == 0xff ) {
		return -1;	/* No Function */
	}

	ctx.pin_index = MK_GPIO_INDEX( channel, func, gid, pin );
	handle = gp_gpio_request( ctx.pin_index, NULL );

	gp_gpio_set_function( handle, func );
	gp_gpio_set_direction( handle, GPIO_DIR_OUTPUT );
	gp_gpio_set_output( handle, level, 0 );
	
	gp_gpio_release( handle );
	return	0;
}
static void systemoff(void)
{
	gpiocfgOut(0, 0, 20, 2, 0);
}
/** read ad data and see any key down?*/
static void gp_keys_report_event(struct input_polled_dev *pdev)
{
	struct gp_keys_drvdata *ddata = (struct gp_keys_drvdata *)(pdev->private);
	struct gp_keys_platform_data_s *pdata = ddata->pdata;
	struct input_dev *input = ddata->input_poll->input;
	struct gp_adc_channel_data_s *pchdata;
	struct gp_adc_button_s *button;
	int result, i, j;
	int changed = 0;
	static int powerKey = 0; /* not pressed */
	static int start_poweroff = 0;

	if(wdt > 0)
	{
		watchdog_alive();
	}


#ifdef SYSCONFIG_INTERNAL_ADC
	for (i = 0; i < pdata->nr_channels; i++) {
		//result = gp_adc_read_timeout(ddata->handle_adc[i], 0);	/* read adc result*/
		result = gp_adc_read(ddata->handle_adc[i]);	/* read adc result*/
		if (IS_ERR((void*)result)) 
			continue;
		pchdata = &pdata->channel_data[i];

		for (j = 0; j < pchdata->nr_keys; j++) { /*ad result match any key data?*/
			button = &(pchdata->keys[j]);
			if( (result >= button->min) && (result <= button->max) ) {/*key pressed*/
				if ( !button->pressed ) {
					input_event(input, EV_KEY, button->code, 1);
					button->pressed = 1;
					changed++;
				}
			} else { /*release state*/
				if ( button->pressed ) {
					input_event(input, EV_KEY, button->code, 0);
					button->pressed = 0;
					changed++;
				}
			}
		}
	}
#endif
	/*check GPIO keys*/
	for (i = 0; i < pdata->nr_gpios; i++ ) {
		if( 0==gp_gpio_get_value( ddata->handle_gpio[i], &result) ) {
			result = (result ? 1 : 0) ^ pdata->gpio_data[i].active_low;
		#if 0
			if ( !!(result) ) {
				if ( !pdata->gpio_data[i].pressed ) {
					input_event( input, EV_KEY, pdata->gpio_data[i].code, 1 );
					button->pressed = 1;
					changed++;
				} else {
					input_event( input, EV_KEY, pdata->gpio_data[i].code, 0 );
					button->pressed = 0;
					changed++;
				}
			}
		#else
			if( result != pdata->gpio_data[i].pressed ) {/*status changed*/
				input_event( input, EV_KEY, pdata->gpio_data[i].code, result );
				pdata->gpio_data[i].pressed = result;
				changed++;
				if((pdata->gpio_data[i].code == 0x74) && (start_poweroff == 0))
				{
					offcnt = poweroff * 50;
					start_poweroff = 1;				
					printk("###	KEY PRESSED [%i]= 0x%x	###\n", i, pdata->gpio_data[i].code);
				}
				
			}
		#endif
		}
	}

	if(start_poweroff)
	{
		if(offcnt <= 0)
		{
			printk("###	systemoff ###\n");
				systemoff();
				for(;;);
		}
		offcnt--;
	}

	/*check for power key*/
	if(gpHalPwron0Get()){
		if (!powerKey) {
			input_event(input, EV_KEY, POWER_KEY_VALUE, 1);
			powerKey = 1;
			changed ++;
		}
	}
	else {
		if (powerKey) {
			input_event(input, EV_KEY, POWER_KEY_VALUE, 0);
			powerKey = 0;
			changed++;
		}
	}

	if (changed) {
		input_sync(input);
	}
}


static int __devinit gp_keys_probe(struct platform_device *pdev)
{
	struct gp_keys_platform_data_s *pdata = pdev->dev.platform_data;
	struct gp_keys_drvdata *ddata;
	struct input_dev *input;
	struct input_polled_dev *input_poll;
	int i, j, error = 0;
	int handle = -1;

	/*alloc ram for data struct*/
	ddata = kzalloc(sizeof(struct gp_keys_drvdata) +
			pdata->nr_gpios * sizeof(int) +		/*memory for adc channel handles*/
			pdata->nr_channels * sizeof(int),	/*memory for adc channel handles*/
			GFP_KERNEL);
	input_poll = input_allocate_polled_device();
	if (!ddata || !input_poll) {
		printk("gp_adc_keys_probe ddata=%p,input_poll=%p\n",ddata,input_poll);
		error = -ENOMEM;
		goto __err_alloc_mem;
	}
	
	ddata->handle_gpio = ddata->handles;
    if (pdata->nr_channels > 0) {
        ddata->handle_adc = ddata->handles + pdata->nr_gpios;
    } else {
        ddata->handle_adc = NULL;
    }
	ddata->input_poll = input_poll;
	ddata->pdata = pdata;
	platform_set_drvdata(pdev, ddata);

	/* set input-polldev handlers */
	input_poll->private = ddata;
	input_poll->poll = gp_keys_report_event;
	input_poll->poll_interval = pdata->scan_ms;

	input = input_poll->input;
	input->name = pdev->name;
	input->id.bustype = BUS_HOST;
	input->dev.parent = &pdev->dev;

#ifdef SYSCONFIG_INTERNAL_ADC
	for (i = 0; i < pdata->nr_channels; i++) {
		struct gp_adc_channel_data_s *ch_data;
		struct gp_adc_button_s *button;
		/* request adc */
		handle = gp_adc_request(0, NULL);
		if (IS_ERR((void*)handle)) {
			pr_err("adc: failed to request adc,"
				" error %d\n", handle);
			error = handle;
			goto __err_request_adc;
		}
		ddata->handle_adc[i] = handle;
		/*setup the key map*/
		ch_data = &pdata->channel_data[i];		
		for (j = 0; j < ch_data->nr_keys; j++) {
			button = &(ch_data->keys[j]);
			input_set_capability(input, EV_KEY, button->code);
		}
		gp_adc_start(handle, ch_data->channel);
	}
#endif	
	for (i = 0; i < pdata->nr_gpios; i++) {
		struct gp_gpio_button_s *button = &(pdata->gpio_data[i]);
		/* request adc */
		handle = gp_gpio_request( button->pin, (button->desc ?:"GPIO"));
		if (IS_ERR((void*)handle)) {
			pr_err("adc: failed to request gpio(0x%x) error %d\n", button->pin, handle);
			error = handle;
			goto __err_request_gpio;
		}
		ddata->handle_gpio[i] = handle;
		/*setup the GPIO pin*/
		gp_gpio_set_input( handle, (button->active_low)? GPIO_PULL_HIGH: GPIO_PULL_LOW);
		/*setup the key map*/
		input_set_capability(input, EV_KEY, button->code);
	}
	
	/*setup the power key*/
	input_set_capability(input, EV_KEY, POWER_KEY_VALUE);

	/*register polling device*/
	error = input_register_polled_device(input_poll);
	if (error) {
		dev_err(&pdev->dev, "could not register input polling device\n");
		goto __err_request_gpio;
	}

	return 0;

__err_request_gpio:
	while(--i >= 0) {
		gp_gpio_release(ddata->handle_gpio[i]);
	}
	i = pdata->nr_channels;
__err_request_adc:
#ifdef SYSCONFIG_INTERNAL_ADC
	while(--i >= 0) {
		gp_adc_release(ddata->handle_adc[i]);
	}
#endif
__err_alloc_mem:
	input_free_polled_device(input_poll);
	platform_set_drvdata(pdev, NULL);
	kfree(ddata);

	return error;
}

static int __devexit gp_keys_remove(struct platform_device *pdev)
{
	struct gp_keys_drvdata *ddata = platform_get_drvdata(pdev);
	struct gp_keys_platform_data_s *pdata = ddata->pdata;
	int i;

	input_unregister_polled_device(ddata->input_poll);
	for (i=0; i<pdata->nr_channels; i++) {
		gp_gpio_release(ddata->handle_gpio[i]);
	}
#ifdef SYSCONFIG_INTERNAL_ADC
	for (i=0; i<pdata->nr_channels; i++) {
		gp_adc_release(ddata->handle_adc[i]);
	}
#endif
	input_free_polled_device(ddata->input_poll);
	platform_set_drvdata(pdev, NULL);
	kfree(ddata);

	return 0;
}

#define VAL_MAX 0x3ff
#define VAL_MIN 0
#define VAL_OFF (VAL_MAX/4)
/*up/down left/right share with wheel*/
/*TPXN 1*/
#ifdef SYSCONFIG_INTERNAL_ADC
static struct gp_adc_button_s gp_adc_keys1[]={
	/*key code, ad channel, ad min, ad max, description*/
	{KEY_DOWN,  VAL_MIN, VAL_OFF,"dn", 0},
	{KEY_UP,  VAL_MAX-VAL_OFF, VAL_MAX,"up", 0}
};

/*TPXP 0*/
static struct gp_adc_button_s gp_adc_keys0[]={
	/*key code, ad channel, ad min, ad max, description*/
	{KEY_LEFT,  VAL_MIN, VAL_OFF,"left", 0},
	{KEY_RIGHT, VAL_MAX-VAL_OFF, VAL_MAX,"right", 0}
};
#endif

/*TPYN*/
#undef VAL_OFF
#define VAL_OFF 30
#ifdef SYSCONFIG_INTERNAL_ADC
static struct gp_adc_button_s gp_adc_keys3[]={
	/*key code, ad channel, ad min, ad max, description*/
	{KEY_R, VAL_MAX*4.7/14.7-VAL_OFF, VAL_MAX*4.7/14.7+VAL_OFF, "KeyR", 0},
	{KEY_ENTER, VAL_MAX*2.0/3-VAL_OFF, VAL_MAX*2.0/3+VAL_OFF, "OK", 0},
	{KEY_SPACE, VAL_MAX*1.0/2-VAL_OFF, VAL_MAX*1.0/2+VAL_OFF, "SEL", 0},
	{KEY_X, VAL_MAX*56.0/66-VAL_OFF, VAL_MAX*56.0/66+VAL_OFF, "KeyX", 0}
};

static struct gp_adc_channel_data_s gp_adc_channel_datas[] = {
	{/*channel TPXN*/
		1, ARRAY_SIZE(gp_adc_keys1), gp_adc_keys1
	},
	{/*channel TPXP*/
		0, ARRAY_SIZE(gp_adc_keys0), gp_adc_keys0
	},
	{/*channel TPYN*/
		3, ARRAY_SIZE(gp_adc_keys3), gp_adc_keys3
	}
};
#endif
static struct gp_gpio_button_s gp_gpio_buttons[] = {
	/*code, index, active_low, desc*/
	{0x83,            MK_GPIO_INDEX(2,2,4,3),1,"Exit"},/* GPC3 */
	{0x81,            MK_GPIO_INDEX(2,2,4,4),1,"Help"},/* GPC4 */
	{0x88,      MK_GPIO_INDEX(2,2,4,5),1,"Vol-"},/* GPC5 */
	{0x89,        MK_GPIO_INDEX(2,2,4,6),1,"Vol+"},/* GPC6 */
	{0x80,MK_GPIO_INDEX(2,2,4,7),1,"Bright"}, /* GPC7 */
	{KEY_POWER,           MK_GPIO_INDEX(0,0,22,4),1,"Power"} /* GPA4 */
};
static struct gp_gpio_button_s gp_gpio_buttons_1[] = {
	/*code, index, active_low, desc*/
	{0x83,            MK_GPIO_INDEX(2,2,4,3),1,"Exit"},/* GPC3 */
	{0x81,            MK_GPIO_INDEX(2,2,4,4),1,"Help"},/* GPC4 */
	{0x90,      MK_GPIO_INDEX(2,2,4,5),1,"Vol-"},/* GPC5 */
	{0x91,        MK_GPIO_INDEX(2,2,4,6),1,"Vol+"},/* GPC6 */
	{0x80,MK_GPIO_INDEX(2,2,4,7),1,"Bright"}, /* GPC7 */
	{KEY_POWER,           MK_GPIO_INDEX(0,0,22,4),1,"Power"} /* GPA4 */
};

static struct gp_keys_platform_data_s gp_keys_platform_data = {
	.scan_ms	= 20,
#ifdef SYSCONFIG_INTERNAL_ADC
	.nr_channels	= ARRAY_SIZE(gp_adc_channel_datas),
#else
	.nr_channels	= 0,
#endif
	.nr_gpios	= ARRAY_SIZE(gp_gpio_buttons),
#ifdef SYSCONFIG_INTERNAL_ADC
	.channel_data	= gp_adc_channel_datas,
#else
	.channel_data	= NULL,
#endif
	.gpio_data	= gp_gpio_buttons
};
static struct gp_keys_platform_data_s gp_keys_platform_data_1 = {
	.scan_ms	= 20,
#ifdef SYSCONFIG_INTERNAL_ADC
	.nr_channels	= ARRAY_SIZE(gp_adc_channel_datas),
#else
	.nr_channels	= 0,
#endif
	.nr_gpios	= ARRAY_SIZE(gp_gpio_buttons_1),
#ifdef SYSCONFIG_INTERNAL_ADC
	.channel_data	= gp_adc_channel_datas,
#else
	.channel_data	= NULL,
#endif
	.gpio_data	= gp_gpio_buttons_1
};

static struct platform_device gp_keys_device = {
	.name	= "gp-keys",
	.id	= -1,
	.dev	= {
		.platform_data = &gp_keys_platform_data
	},
};
static struct platform_device gp_keys_device_1 = {
	.name	= "gp-keys",
	.id	= -1,
	.dev	= {
		.platform_data = &gp_keys_platform_data_1
	},
};

static /*__initdata*/ struct platform_driver gp_keys_driver = {
	.probe		= gp_keys_probe,
	.remove		= __devexit_p(gp_keys_remove),
	.driver		= {
		.name	= "gp-keys",
		.owner	= THIS_MODULE,
	}
};

static int __init gp_keys_init(void)
{
	if(wdt > 0)
	{
		watchdog_init();
	}

	if(config)
	{
		platform_device_register(&gp_keys_device_1);
		return platform_driver_register(&gp_keys_driver);
	}
	else
	{
		platform_device_register(&gp_keys_device);
		return platform_driver_register(&gp_keys_driver);
	}
}

static void __exit gp_keys_exit(void)
{
	if(config)
	{
		platform_driver_unregister(&gp_keys_driver);	
		platform_device_unregister(&gp_keys_device_1);
	}
	else
	{
		platform_driver_unregister(&gp_keys_driver);	
		platform_device_unregister(&gp_keys_device);
	}
	
	if(wdt > 0)
	{
		watchdog_close();
	}
	
}

module_param(wdt, int,S_IRUGO);
module_param(config, int,S_IRUGO);
module_param(poweroff, int, S_IRUGO);
module_param(wdtclose, int, S_IRUGO);
module_init(gp_keys_init);
module_exit(gp_keys_exit);
MODULE_LICENSE_GP;


