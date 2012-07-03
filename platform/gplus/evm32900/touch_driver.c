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

/**
 * @file gp_ts.c
 * @brief Gerneralplus touch screen driver
 * @author zh.l
 */

#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/cdev.h>
#include <mach/diag.h>
#include <mach/gp_adc.h>
//#include <mach/gp_ts.h>
#include <mach/typedef.h>

#define GP_TP_DEBOUNCE 2
/*
 * Per-touchscreen data.
 */

typedef struct gp_tp_s {
	struct input_dev *dev;
	int client;
	unsigned int xp;
	unsigned int yp;
	char phys[32];
}gp_tp_t;

static gp_tp_t ts;

/* Added by Kevin */
#define TS_BUF_SIZE	10
typedef struct ts_filter_s {
	unsigned int x_buf[TS_BUF_SIZE];
	unsigned int y_buf[TS_BUF_SIZE];
	unsigned int data_count;
}ts_filter_t;
static volatile ts_filter_t ts_filter;

/**
 * @brief touch panel request fucntion
 * @return : handle of the requested touch panel(<0 invalid handle)
 */
int gp_tp_request(void)
{
	return (int)(&ts);
}

/**
 * @brief touch panel free fucntion
 * @param handle[in] : handle of touch panel to release
 * @return : SP_OK(0)/ERROR_ID
 */
int gp_tp_release(int handle)
{
	return 0;
}

/**
 * @brief get touch panel x/y position
 * @param handle[in] : resource handle returned by gp_tp_request()
 * @param ptArray[out] : x/y coordinate of touch panel
 * @param num[in] : number of array elements
		   num[out] : valid points in array
 * @return : SP_OK(0)/ERROR_ID
 */
int gp_tp_get_xy(int handle, struct gp_point_s *ptArray, int* num)
{
	gp_tp_t *pdev;

	if((handle != (int)(&ts)) || (NULL==ptArray) || (0==num) )
		return -EINVAL;

	pdev = (gp_tp_t *)handle;
	ptArray[0].x = 	pdev->xp;
	ptArray[0].y = 	pdev->yp;
	*num = 1;
	return 0;
}


static void insert_sort(volatile unsigned int buf[], unsigned int data, unsigned int data_count)
{
	int i;
	if((!buf) || (data_count >= TS_BUF_SIZE)) {
		return;
	}

	for(i=0;i < data_count;i++){
		if(data < buf[i]){
			memmove(&buf[i+1], &buf[i], (data_count - i)*sizeof(unsigned int));
			buf[i] = data;
			return;
		}
	}

	buf[i] = data;
}

static void ts_filter_input(unsigned int x, unsigned int y)
{
	insert_sort(ts_filter.x_buf, x, ts_filter.data_count);
	insert_sort(ts_filter.y_buf, y, ts_filter.data_count);
	ts_filter.data_count ++;
}

static void ts_filter_output(unsigned int *px, unsigned int *py)
{
	unsigned int i, sum_x, sum_y;
	
	if(!px || !py ) return;

	sum_x = sum_y = 0;
	for(i=2;i<8;i++){
		sum_x += ts_filter.x_buf[i];
		sum_y += ts_filter.y_buf[i];
	}
	*px = sum_x / 6;
	*py = sum_y / 6;
}

/**
 * interrupt callback
 */
void gp_ts_callback(int client, unsigned val, unsigned event)
{
	static int pen_down=0;

	if(event & 4/*ADC_INTPNL*/) {

		ts_filter_input(0x3ff & (unsigned short)(val  + 1024), 0x3ff & (unsigned short)( (val >> 16) + 1024));
		if(ts_filter.data_count >= TS_BUF_SIZE) {

			ts_filter_output(&ts.xp, &ts.yp);

			input_report_abs(ts.dev, ABS_X, ts.xp);
			input_report_abs(ts.dev, ABS_Y, ts.yp);
	 		input_report_key(ts.dev, BTN_TOUCH, 1);		
	 		input_report_abs(ts.dev, ABS_PRESSURE, 1);
	 		input_sync(ts.dev);

			pen_down = GP_TP_DEBOUNCE;
			ts_filter.data_count = 0;
		} 

/*		pen_down++;
		if( pen_down >= GP_TP_DEBOUNCE ) {
			pen_down = GP_TP_DEBOUNCE;		
			ts.xp = 0x3ff & (unsigned short)(val  + 1024);
			ts.yp = 0x3ff & (unsigned short)( (val >> 16)    + 1024);
			input_report_abs(ts.dev, ABS_X, ts.xp);
			input_report_abs(ts.dev, ABS_Y, ts.yp);
	 		input_report_key(ts.dev, BTN_TOUCH, 1);		
	 		input_report_abs(ts.dev, ABS_PRESSURE, 1);
	 		input_sync(ts.dev);   
		}*/
	}

	if(event & 2/*ADC_INTPENUP*/) {
		if( pen_down >= GP_TP_DEBOUNCE ) {
	 		input_report_key(ts.dev, BTN_TOUCH, 0);
 			input_report_abs(ts.dev, ABS_PRESSURE, 0);
 			input_sync(ts.dev);
			pen_down = 0;
		}
		ts_filter.data_count = 0;
	}	
}

/** device driver probe*/
static int __init gp_tp_probe(struct platform_device *pdev)
{
	int rc;
	int ret = 0;

	DIAG_DEBUG("Entering gp_tp_probe\n");

	memset(&ts_filter, 0, sizeof(ts_filter));

	memset(&ts, 0, sizeof(gp_tp_t));
	ts.dev = input_allocate_device();
	if ( NULL==ts.dev ){
		DIAG_ERROR("Unable to alloc input device\n");
		ret = -ENOMEM;
		goto __err_alloc;
	}

	ts.client = gp_adc_request(1, gp_ts_callback);
	if(IS_ERR((void*)ts.client)) {
		DIAG_ERROR("%s unable to register client\n", __func__);
		ret = -ENOMEM;
		goto __err_register;
	}

	ts.dev->evbit[0] = BIT_MASK(EV_SYN)| BIT_MASK(EV_KEY) |BIT_MASK(EV_ABS);
	ts.dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	set_bit(EV_SW, ts.dev->evbit);
	bitmap_fill(ts.dev->swbit, SW_MAX);    

	input_set_abs_params(ts.dev, ABS_X, 0, 0x3ff, 0, 0);
	input_set_abs_params(ts.dev, ABS_Y, 0, 0x3ff, 0, 0);
	input_set_abs_params(ts.dev, ABS_PRESSURE, 0, 1, 0, 0);

	ts.dev->name = "GeneralPlus touch panel controller";
	ts.dev->phys = "gp_ts";
	ts.dev->id.bustype = BUS_RS232;
	ts.dev->id.vendor  = 0xDEAD;
	ts.dev->id.product = 0xBEEF;
	ts.dev->id.version = 0x0101;
	/* All went ok, so register to the input system */
	rc = input_register_device(ts.dev);
	if (rc) {
		ret = -EIO;
		goto __err_reg_input;
	}

	/*start touch panel conversion*/
	ret = gp_adc_start(ts.client, 0);
	DIAG_DEBUG("gp_tp driver loaded(%d)\n", ret);
	return 0;


__err_reg_input:
	gp_adc_release(ts.client);
__err_register:
	input_free_device(ts.dev);
__err_alloc:
	return ret;
}

/** device driver remove*/
static int gp_tp_remove(struct platform_device *pdev)
{
	input_unregister_device(ts.dev);
	input_free_device(ts.dev);	
	gp_adc_release(ts.client);
	return 0;
}

#ifdef CONFIG_PM
static int gp_tp_suspend(struct platform_device *pdev, pm_message_t state)
{
	int ret = 0;
	ret = gp_adc_stop(ts.client);
	DIAG_DEBUG("gp_tp driver suspend(%d)\n", ret);	
	return 0;
	return 0;
}

static int gp_tp_resume(struct platform_device *pdev)
{
	int ret = 0;
	ret = gp_adc_start(ts.client, 0);
	DIAG_DEBUG("gp_tp driver resume(%d)\n", ret);	
	return 0;
}

#else
#define gp_tp_suspend NULL
#define gp_tp_resume  NULL
#endif

static struct platform_device gp_tp_device = {
	.name = "gp_tp",
	.id   = -1,
};

static struct platform_driver gp_tp_driver = {
       .driver         = {
	       .name   = "gp_tp",
	       .owner  = THIS_MODULE,
       },
       .probe          = gp_tp_probe,
       .remove         = gp_tp_remove,
       .suspend        = gp_tp_suspend,
       .resume         = gp_tp_resume,

};

static int __init gp_tp_module_init(void)
{
	int rc;

	platform_device_register(&gp_tp_device);
	rc = platform_driver_register(&gp_tp_driver);
	return rc;
}

static void __exit gp_tp_module_exit(void)
{
	platform_device_unregister(&gp_tp_device);
	platform_driver_unregister(&gp_tp_driver);
}

module_init(gp_tp_module_init);
module_exit(gp_tp_module_exit);
MODULE_LICENSE_GP;

