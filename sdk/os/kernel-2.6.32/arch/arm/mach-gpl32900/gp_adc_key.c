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
 * @file gp_adc_key.c
 * @brief ADC key driver
 * @author zh.l
 */
#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/gp_adc_key.h>
#include <mach/gp_adc.h>
struct gp_adc_keys_drvdata {
	struct input_dev *input;
	struct timer_list timer;	/** key scan timer*/
	struct work_struct work;	/** working queue*/
	int handle;	/** handle of adc client*/
	int nr_keys;	/** number of adc keys*/
	struct gp_adc_keys_button data[0];	/** key datas*/
};

/** read ad data and see any key down?*/
static void gp_adc_keys_report_event(struct work_struct *work)
{
	struct gp_adc_keys_drvdata *ddata =
	    container_of(work, struct gp_adc_keys_drvdata, work);
	struct gp_adc_keys_button *button = NULL;
	struct input_dev *input = ddata->input;
	int ad_result, i;
	ad_result = gp_adc_read(ddata->handle);	/* read adc result */
	if (!IS_ERR((void *)ad_result)) {
		for (i = 0; i < ddata->nr_keys; i++) {	/*ad result match any key data? */
			button = &ddata->data[i];
			if ((ad_result >= button->min) && (ad_result <= button->max)) {	/*key pressed */
				if ( !button->pressed ) {
					input_event(input, EV_KEY, button->code, 1);
					input_sync(input);
					button->pressed = 1;
				}
			} else {	/*release state */
				if ( button->presse ) {
					input_event(input, EV_KEY, button->code, 0);
					input_sync(input);
					button->pressed = 0;
				}
			}
		}
	}

	/*next scan, 20ms */
	mod_timer(&ddata->timer, jiffies + HZ / 50);
}

static void gp_adc_keys_timer(unsigned long _data)
{
	struct gp_adc_keys_drvdata *data = (struct gp_adc_keys_drvdata *)_data;
	schedule_work(&data->work);
}

static int __devinit gp_adc_keys_probe(struct platform_device *pdev)
{
	struct gp_adc_keys_platform_data *pdata = pdev->dev.platform_data;
	struct gp_adc_keys_drvdata *ddata;
	struct input_dev *input;
	int i, error = 0;
	int handle = -1;

	/*alloc ram for data struct */
	ddata =
	    kzalloc(sizeof(struct gp_adc_keys_drvdata) +
		    pdata->nr_buttons * sizeof(struct gp_adc_keys_button),
		    GFP_KERNEL);
	input = input_allocate_device();
	printk("drvdata=%p,input=%p", ddata, input);
	if (!ddata || !input) {
		error = -ENOMEM;
		goto fail1;
	}
	input->name = pdev->name;
	input->phys = "adc-keys/input0";
	input->dev.parent = &pdev->dev;
	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0100;
	ddata->input = input;

	/* request adc */
	handle = gp_adc_request(0, NULL);
	if (IS_ERR((void *)handle)) {
		pr_err("gpio-keys: failed to request adc,"
		       " error %d\n", handle);
		goto fail1;
	}
	ddata->handle = handle;

	/*setup the key map */
	for (i = 0; i < pdata->nr_buttons; i++) {
		struct gp_adc_keys_button *button = &pdata->buttons[i];
		struct gp_adc_keys_button *bdata = &ddata->data[i];
		bdata->code = button->code;
		bdata->min = button->min;
		bdata->max = button->max;
		bdata->pressed = 0;
		input_set_capability(input, EV_KEY, button->code);
	}

	/*init timer and work queue */
	setup_timer(&ddata->timer, gp_adc_keys_timer, (unsigned long)ddata);
	INIT_WORK(&ddata->work, gp_adc_keys_report_event);
	mod_timer(&ddata->timer, jiffies + HZ);	//start scan after 1S

	/*register input device */
	error = input_register_device(input);
	if (error) {
		pr_err("gpio-keys: Unable to register input device, "
		       "error: %d\n", error);
		goto fail2;
	}

	/*start ad conversion */
	gp_adc_start(ddata->handle, pdata->channel);
	return 0;
fail2:
	del_timer_sync(&ddata->timer);
	cancel_work_sync(&ddata->work);
	gp_adc_release(ddata->handle);
fail1:
	input_free_device(input);
	kfree(ddata);

	return error;
}

static int __devexit gp_adc_keys_remove(struct platform_device *pdev)
{
	/*struct gp_adc_keys_platform_data *pdata = pdev->dev.platform_data; */
	struct gp_adc_keys_drvdata *ddata = platform_get_drvdata(pdev);
	struct input_dev *input = ddata->input;
	device_init_wakeup(&pdev->dev, 0);
	del_timer_sync(&ddata->timer);
	cancel_work_sync(&ddata->work);
	gp_adc_release(ddata->handle);
	input_unregister_device(input);
	kfree(ddata);

	return 0;
}

static __initdata struct gp_adc_keys_button gp_adc_keys[] = {
	/*key code, ad channel, ad min, ad max, description */
	{KEY_UP, 100, 200, "up"},
	{KEY_DOWN, 300, 400, "down"},
	{KEY_LEFT, 500, 600, "left"},
	{KEY_RIGHT, 700, 800, "right"},
};

static __initdata struct gp_adc_keys_platform_data gp_adc_keys_data = {
	.buttons = gp_adc_keys,
	.nr_buttons = ARRAY_SIZE(gp_adc_keys),
	.channel = 4;		/*AUX1 channel */
};

static __initdata struct platform_device gp_adc_keys_device = {
	.name = "adc-keys",
	.id = -1,
	.dev = {
		.platform_data = &gp_adc_keys_data},
};

static struct platform_driver gp_adc_keys_driver = {
	.probe = gp_adc_keys_probe,
	.remove = __devexit_p(gp_adc_keys_remove),
	.driver = {
		   .name = "adc-keys",
		   .owner = THIS_MODULE,
		   }
};

static int __init gp_adc_keys_init(void)
{
	platform_device_register(&gp_adc_keys_device);
	return platform_driver_register(&gp_adc_keys_driver);
}

static void __exit gp_adc_keys_exit(void)
{
	platform_driver_unregister(&gp_adc_keys_driver);
} module_init(gp_adc_keys_init);

module_exit(gp_adc_keys_exit);
MODULE_LICENSE_GP;
