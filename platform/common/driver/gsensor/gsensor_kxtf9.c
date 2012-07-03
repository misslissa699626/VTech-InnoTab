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
#include <mach/module.h>
#include <mach/kernel.h>
#include <mach/cdev.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <mach/gp_i2c_bus.h>
#include <mach/gsensor_kxtf9.h>


/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
/* gsensor id */
#define SLAVE_ADDR	0x1E
#define I2C_FREQ	300 //300 KHz

/* kxtf9 gsensor register */
#define XOUT_HPF_L			0x00
#define XOUT_HPF_H			0x01
#define YOUT_HPF_L			0x02
#define YOUT_HPF_H			0x03
#define ZOUT_HPF_L			0x04
#define ZOUT_HPF_H			0x05
#define XOUT_L				0x06
#define XOUT_H				0x07
#define YOUT_L				0x08
#define YOUT_H				0x09
#define ZOUT_L				0x0A
#define ZOUT_H				0x0B
#define DCST_RESP			0x0C
#define WHO_I_AM			0x0F
#define TILT_POS_CUR 		0x10
#define TILT_POS_PRE		0x11
#define INT_SRC_REG1		0x15
#define INT_SRC_REG2		0x16
#define STATUS_REG			0x18
#define INT_REL				0x1A
#define CTRL_REG1			0x1B
#define CTRL_REG2			0x1C
#define CTRL_REG3			0x1D
#define INT_CTRL_REG1		0x1E
#define INT_CTRL_REG2		0x1F
#define INT_CTRL_REG3   	0x20
#define DATA_CTRL_REG		0x21
#define TILT_TIMER			0x28
#define WUF_TIMER			0x2B
#define TDT_H_THRESH		0x2C
#define TDT_L_THRESH		0x2D
#define TDT_TAP_TIMER		0x21
#define TDT_TOTAL_TIMER		0x2F
#define TDT_LATENCY_TIMER	0x30
#define TDT_WINDOW_TIMER	0x31
#define SELF_TEST			0x3A
#define WUF_THRESH			0x5A
#define TILT_ANGLE			0x5C
#define HYST_SET			0x5F


/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
//#define GSENSOR_ADD_FAIL_COUNT

#define GSENSOR_DATA_12BIT
//#define GSENSOR_DATA_8BIT

#define	GSENSOR_RANGE_2G
//#define	GSENSOR_RANGE_4G
//#define	GSENSOR_RANGE_8G

#if defined(GSENSOR_DATA_8BIT)
	#define RANGE_OF_COUNT	2048
#elif defined(GSENSOR_DATA_12BIT)
	#define RANGE_OF_COUNT	128
#else
#error Wrong configuration
#endif

#if defined(GSENSOR_RANGE_2G)
	#if defined(GSENSOR_DATA_8BIT)
		#define COUNTS_PER_G	64
	#elif defined(GSENSOR_DATA_12BIT)
		#define COUNTS_PER_G	1024
	#else
		#error Wrong configuration
	#endif
#elif defined(GSENSOR_RANGE_4G)
	#if defined(GSENSOR_DATA_8BIT)
		#define COUNTS_PER_G	32
	#elif defined(GSENSOR_DATA_12BIT)
		#define COUNTS_PER_G	512
	#else
		#error Wrong configuration
	#endif
#elif defined(GSENSOR_RANGE_8G)
	#if defined(GSENSOR_DATA_8BIT)
		#define COUNTS_PER_G	16
	#elif defined(GSENSOR_DATA_12BIT)
		#define COUNTS_PER_G	256
	#else
		#error Wrong configuration
	#endif
#else
	#error Wrong configuration
#endif


#define GSENSOR_FILTER_LEVEL	5
#define GSENSOR_UPDATE_INTERVAL	(HZ/50)
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct gsensor_info_s 
{	
	struct miscdevice dev;
	int i2c_handle;
	int open_cnt;
}gsensor_info_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static gsensor_info_t gsensor_data;


typedef struct
{
	int x_buf[GSENSOR_FILTER_LEVEL];
	int y_buf[GSENSOR_FILTER_LEVEL];
	int z_buf[GSENSOR_FILTER_LEVEL];
	int tilt[GSENSOR_FILTER_LEVEL];
	unsigned int counter;
}gsensor_filter_t;
static volatile gsensor_filter_t gsensor_filter;
static struct timer_list gsensor_timer;
static volatile gsensor_data_t gsensor_data_buf;
static int need_rotate = 0;
static gsensor_cal_data_t g_cal_data = {0, 0, 0};

#ifdef GSENSOR_ADD_FAIL_COUNT
#define FAIL_MAX_COUNT	10
static volatile int i2c_trans_fail_count;
#endif


static int kxtf9_write( unsigned char *pdata, int len)
{
	int ret = 0;
	
	ret = gp_i2c_bus_write(gsensor_data.i2c_handle, pdata, len);
	
	return ret;
}

static int kxtf9_read(unsigned char reg, unsigned char *pdata, int len)
{
	int ret = 0;
	
	gp_i2c_bus_write(gsensor_data.i2c_handle, &reg, 1);
	
	ret = gp_i2c_bus_read(gsensor_data.i2c_handle, pdata, len);

	return ret;
}

static int kxtf9_init(void)
{
	unsigned char value, data[10];
	int i, ret = 0;
	
	/* software reset */
#ifdef GSENSOR_ADD_FAIL_COUNT
	do {
		ret = kxtf9_read(CTRL_REG3, &value, 1);
		if (ret != 1) {
			i2c_trans_fail_count ++;
			if (i2c_trans_fail_count >= FAIL_MAX_COUNT) {
				return -1;
			}
		} else {
			i2c_trans_fail_count = 0;
		}
	} while (ret != 1);
#else
	ret = kxtf9_read(CTRL_REG3, &value, 1);
#endif

	data[0] = CTRL_REG3;
	data[1] = value | (1 << 7);
#ifdef GSENSOR_ADD_FAIL_COUNT
	do {
		ret = kxtf9_write(data, 2);
		if (ret != 2) {
			i2c_trans_fail_count ++;
			if (i2c_trans_fail_count >= FAIL_MAX_COUNT) {
				return -1;
			}
		} else {
			i2c_trans_fail_count = 0;
		}
	} while (ret != 2);
#else
	ret = kxtf9_write(data, 2);
#endif
	for(i=0; i<10; i++)
	{
		msleep(1);
		ret = kxtf9_read(CTRL_REG3, &value, 1);
#ifdef GSENSOR_ADD_FAIL_COUNT
		if (ret != 1) {
			i2c_trans_fail_count ++;
			if (i2c_trans_fail_count >= FAIL_MAX_COUNT) {
				return -1;
			}
		} else {
			i2c_trans_fail_count = 0;
			if ((value & (1 << 7)) == 0)
				break;
		}
#else
	if ((value & (1 << 7)) == 0)
		break;
#endif
	}
	if(i==10) return -1;

	data[0] = CTRL_REG1;

#ifdef GSENSOR_RANGE_2G
	#ifdef GSENSOR_DATA_8BIT
		data[1] = 0x01;//CTRL_REG1 - 8bit valid, range +/-2g, enable tilt position detect
	#else
		data[1] = 0x41;//CTRL_REG1 - 12bit valid, range +/-2g, enable tilt position detect
	#endif
#elif defined(GSENSOR_RANGE_4G)
	#ifdef GSENSOR_DATA_8BIT
		data[1] = 0x09;//CTRL_REG1 - 8bit valid, range +/-4g, enable tilt position detect
	#else
		data[1] = 0x49;//CTRL_REG1 - 12bit valid, range +/-4g, enable tilt position detect
	#endif
#else
	#ifdef GSENSOR_DATA_8BIT
		data[1] = 0x11;//CTRL_REG1 - 8bit valid, range +/-8g, enable tilt position detect
	#else
		data[1] = 0x51;//CTRL_REG1 - 12bit valid, range +/-8g, enable tilt position detect
	#endif
#endif


	data[2] = 0x3F;//CTRL_REG2 - enable tilt position report
	data[3] = 0x62;//CTRL_REG3 - Set output data rate for general motion detect to 100Hz, tilt position data rate 50Hz
	data[4] = 0x00;//INT_CTRL_REG1 - Disable interrupt
	data[5] = 0x00;//INT_CTRL_REG2 - Disable interrupt
	data[6] = 0x00;//INT_CTRL_REG3 - Disable interrupt
	data[7] = 0x13;//DATA_CTRL_REG - Set output data rate to 100Hz

#ifdef GSENSOR_ADD_FAIL_COUNT
	do {
		ret = kxtf9_write(data, 8);
		if (ret != 8) {
			i2c_trans_fail_count ++;
			if (i2c_trans_fail_count >= FAIL_MAX_COUNT) {
				return -1;
			}
		} else {
			i2c_trans_fail_count = 0;
		}
	} while (ret != 8);
#else
	ret = kxtf9_write(data, 8);
#endif

	/* start */
	data[0] = CTRL_REG1;
	data[1] = 0xC1;

#ifdef GSENSOR_ADD_FAIL_COUNT
	do {
		ret = kxtf9_write(data, 2);
		if (ret != 2) {
			i2c_trans_fail_count ++;
			if (i2c_trans_fail_count >= FAIL_MAX_COUNT) {
				return -1;
			}
		} else {
			i2c_trans_fail_count = 0;
		}
	} while (ret != 2);
#else
	ret = kxtf9_write(data, 2);
#endif

	return 0;
}


static int get_gsensor_data(int *px, int *py, int *pz, int *ptilt)
{
	unsigned char data_tmp[10], tilt_tmp = 0;
	int x_tmp = 0, y_tmp = 0, z_tmp = 0, ret;
	int rotate_x, rotate_y, rotate_z;
	unsigned char rotate_tilt;

#ifdef GSENSOR_ADD_FAIL_COUNT
	if (i2c_trans_fail_count >= FAIL_MAX_COUNT) {
		return -EFAULT;
	}
#endif

	memset(&data_tmp, 0, sizeof(data_tmp));

	ret = kxtf9_read(TILT_POS_CUR, &tilt_tmp, 1);
	if(ret <= 0) {
#ifdef GSENSOR_ADD_FAIL_COUNT
		i2c_trans_fail_count ++;
#endif
		return -EFAULT;
	}

	ret = kxtf9_read(XOUT_L, data_tmp, 6);
	if(ret < 6) {
#ifdef GSENSOR_ADD_FAIL_COUNT
		i2c_trans_fail_count ++;
#endif
		return -EFAULT;
	}
#ifdef GSENSOR_DATA_8BIT
	x_tmp = data_tmp[1];
	if(x_tmp & 0x80) {
		x_tmp |= 0xFFFFFF00;
	}
	y_tmp = data_tmp[3];
	if(y_tmp & 0x80) {
		y_tmp |= 0xFFFFFF00;
	}
	z_tmp = data_tmp[5];
	if(z_tmp & 0x80) {
		z_tmp |= 0xFFFFFF00;
	}
#else
	x_tmp = (data_tmp[1]<<4)|(data_tmp[0]>>4);
	if(x_tmp&0x800)
	{
		x_tmp |= 0xFFFFF000;//12bit signed to 32bit signed
	}

	y_tmp = (data_tmp[3]<<4)|(data_tmp[2]>>4);
	if(y_tmp&0x800)
	{
		y_tmp |= 0xFFFFF000;//12bit signed to 32bit signed
	}

	z_tmp = (data_tmp[5]<<4)|(data_tmp[4]>>4);
	if(z_tmp&0x800)
	{
		z_tmp |= 0xFFFFF000;//12bit signed to 32bit signed
	}
#endif

	if (need_rotate) {
/*		rotate_x = y_tmp;
		rotate_y = - x_tmp;
		rotate_z = z_tmp;
		rotate_tilt = 0;
		// bit 5->2, 4->3, 3->5, 2->4, 1->1, 0->0
		rotate_tilt = ((tilt_tmp&(1<<5)) >> 3) | ((tilt_tmp&(1<<4)) >> 1) | ((tilt_tmp&(1<<3)) << 2) | ((tilt_tmp&(1<<2)) << 2) | (tilt_tmp&0x3);
		*px = rotate_x;
		*py = rotate_y;
		*pz = rotate_z;
		*ptilt = rotate_tilt;	
*/
		rotate_x = - y_tmp;
		rotate_y = x_tmp;
		rotate_z = z_tmp;
		rotate_tilt = 0;
		// bit 5->3, 4->2, 3->4, 2->5, 1->1, 0->0
		rotate_tilt = ((tilt_tmp&(1<<5)) >> 2) | ((tilt_tmp&(1<<4)) >> 2) | ((tilt_tmp&(1<<3)) << 1) | ((tilt_tmp&(1<<2)) << 3) | (tilt_tmp&0x3);

		*px = rotate_x - g_cal_data.deltaX;
		*py = rotate_y - g_cal_data.deltaY;
		*pz = rotate_z - g_cal_data.deltaZ;
		*ptilt = rotate_tilt;	
	} else {
		*px = x_tmp - g_cal_data.deltaX;
		*py = y_tmp - g_cal_data.deltaY;
		*pz = z_tmp - g_cal_data.deltaZ;
		*ptilt = tilt_tmp;
	}
	
	return 0;
}

static void update_gsensor_data(unsigned long arg)
{
	int sum[3], index = 0, tilt_tmp;

	index = gsensor_filter.counter%GSENSOR_FILTER_LEVEL;

	if(get_gsensor_data((int*)&gsensor_filter.x_buf[index], \
		(int*)&gsensor_filter.y_buf[index], \
		(int*)&gsensor_filter.z_buf[index], \
		(int*)&gsensor_filter.tilt[index]) == 0) {
#ifdef GSENSOR_ADD_FAIL_COUNT
		i2c_trans_fail_count = 0;
#endif
		gsensor_filter.counter ++;
		if(gsensor_filter.counter >= GSENSOR_FILTER_LEVEL) {
			memset(sum, 0, sizeof(sum));
			tilt_tmp = 0x3F;
			for(index=0; index<GSENSOR_FILTER_LEVEL; index++) {
				sum[0] += gsensor_filter.x_buf[index];
				sum[1] += gsensor_filter.y_buf[index];
				sum[2] += gsensor_filter.z_buf[index];
				tilt_tmp &= gsensor_filter.tilt[index];
			}
			if(tilt_tmp) gsensor_data_buf.tilt = tilt_tmp;

			gsensor_data_buf.x = sum[0] / GSENSOR_FILTER_LEVEL;
			gsensor_data_buf.y = sum[1] / GSENSOR_FILTER_LEVEL;
			gsensor_data_buf.z = sum[2] / GSENSOR_FILTER_LEVEL;
			gsensor_data_buf.counts_per_g = COUNTS_PER_G;
			gsensor_data_buf.range_of_count = RANGE_OF_COUNT;
		}
	} else {
//		printk(KERN_ERR "read data fail\n");
	}
#ifdef GSENSOR_ADD_FAIL_COUNT
	if (i2c_trans_fail_count < FAIL_MAX_COUNT) {
		mod_timer(&gsensor_timer, jiffies + GSENSOR_UPDATE_INTERVAL);
	}
#else
	mod_timer(&gsensor_timer, jiffies + GSENSOR_UPDATE_INTERVAL);
#endif
}


static long gsensor_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	gsensor_data_t	data_tmp;
	gsensor_cal_data_t cal_data_tmp = {0, 0, 0};

	switch(cmd)
	{
	case GSENSOR_GET_DATA:
#ifdef GSENSOR_ADD_FAIL_COUNT
		if (i2c_trans_fail_count >= FAIL_MAX_COUNT) {
			return -EFAULT;
		}
#endif
		memset(&data_tmp, 0, sizeof(data_tmp));
		
		if(get_gsensor_data(&data_tmp.x, &data_tmp.y, &data_tmp.z, &data_tmp.tilt)) {
			return -EFAULT;
		}
		data_tmp.counts_per_g = COUNTS_PER_G;
		data_tmp.range_of_count = RANGE_OF_COUNT;

		if(copy_to_user((void __user *)arg, (const void *)&data_tmp, sizeof(data_tmp)))
			return -EFAULT;

		break;

	case GSENSOR_GET_FILTERED_DATA:
#ifdef GSENSOR_ADD_FAIL_COUNT
		if (i2c_trans_fail_count >= FAIL_MAX_COUNT) {
			return -EFAULT;
		}
#endif
		if(copy_to_user((void __user *)arg, (const void *)&gsensor_data_buf, sizeof(gsensor_data_buf)))
			return -EFAULT;
		break;

	case GSENSOR_SET_CAL_DATA:
		if(copy_from_user((void *)&cal_data_tmp, (void __user *)arg, sizeof(cal_data_tmp)))
			return -EFAULT;
		memcpy(&g_cal_data, &cal_data_tmp, sizeof(gsensor_cal_data_t));
		printk(KERN_INFO"[GSENSOR_DRV] Set gsensor cal data! deltaX: %d, deltaY: %d, deltaZ: %d\n", 
			g_cal_data.deltaX, g_cal_data.deltaY, g_cal_data.deltaZ);
		break;

	default:
		return -EINVAL;	
	}
	return 0;
}

static int gsensor_dev_open(struct inode *inode, struct file *fop)
{
	if(gsensor_data.open_cnt == 0)
	{
		gsensor_timer.expires = jiffies + GSENSOR_UPDATE_INTERVAL;
		add_timer(&gsensor_timer);
	}

//	gsensor_data.open_cnt ++;
	gsensor_data.open_cnt = 1;//TBD

	return 0;
}

static int gsensor_dev_release(struct inode *inode, struct file *fop)
{
	//TBD
/*	if(gsensor_data.open_cnt > 0)
	{
		gsensor_data.open_cnt --;
		if(gsensor_data.open_cnt == 0)
		{
			del_timer(&gsensor_timer);
		}
	}*/

	return 0;
}

struct file_operations gsensor_fops = 
{
	.owner			= THIS_MODULE,
	.unlocked_ioctl = gsensor_dev_ioctl,
	.open			= gsensor_dev_open,
	.release		= gsensor_dev_release
};

static int gp_gsensor_probe(struct platform_device *pdev)
{
	int nRet;

	gsensor_data.i2c_handle = gp_i2c_bus_request(SLAVE_ADDR,I2C_FREQ);
	if(!gsensor_data.i2c_handle)
		return -ENOMEM;

	gsensor_data.dev.name = "gsensor";
	gsensor_data.dev.fops = &gsensor_fops;
	gsensor_data.dev.minor = MISC_DYNAMIC_MINOR;
	gsensor_data.open_cnt = 0;
	
	nRet = misc_register(&gsensor_data.dev);
	if(nRet)
	{
		printk(KERN_ALERT"Gsensor probe register failed\n");
		goto err_reg;
	}
	
	platform_set_drvdata(pdev,&gsensor_data);

#ifdef GSENSOR_ADD_FAIL_COUNT
	i2c_trans_fail_count = 0;
#endif

	if (kxtf9_init()) {
		nRet = -EFAULT;
		printk(KERN_ALERT"Init gsensor kxtf9 failed\n");
		goto err_reg;
	}
	
	//initial gsensor_data_buf
	memset((void *)&gsensor_data_buf, 0, sizeof(gsensor_data_buf));
	memset((void *)&gsensor_filter, 0, sizeof(gsensor_filter));
	get_gsensor_data((int*)&gsensor_data_buf.x, (int*)&gsensor_data_buf.y, (int*)&gsensor_data_buf.z, (int*)&gsensor_data_buf.tilt);
	gsensor_data_buf.counts_per_g = COUNTS_PER_G;
	gsensor_data_buf.range_of_count = RANGE_OF_COUNT;

	init_timer(&gsensor_timer);
	gsensor_timer.function = update_gsensor_data;	/* timer handler */
	gsensor_timer.data = 1;
//	gsensor_timer.expires = jiffies + GSENSOR_UPDATE_INTERVAL;
//	add_timer(&gsensor_timer);
	
	printk(KERN_ALERT"gsensor kxtf9 probe ok(need_rotate=%d)\n", need_rotate);
	return 0;

err_reg:
	gp_i2c_bus_release(gsensor_data.i2c_handle);
	return nRet;
}

static int gp_gsensor_remove(struct platform_device *pdev)
{
	gsensor_data.open_cnt = 0;
	del_timer(&gsensor_timer);
	gp_i2c_bus_release(gsensor_data.i2c_handle);
	return 0;
}


static struct platform_device gp_gsensor_device = 
{
	.name	= "gsensor",
	.id	= -1,
};

static struct platform_driver gp_gsensor_driver = 
{
	.probe		= gp_gsensor_probe,
	.remove		= __devexit_p(gp_gsensor_remove),
	.driver		= 
	{
		.name	= "gsensor",
		.owner	= THIS_MODULE,
	}
};

static int gp_gsensor_init(void)
{
	platform_device_register(&gp_gsensor_device);
	return platform_driver_register(&gp_gsensor_driver);
}

static void gp_gsensor_free(void)
{	
	platform_driver_unregister(&gp_gsensor_driver);	
}

module_init(gp_gsensor_init);
module_exit(gp_gsensor_free);
module_param(need_rotate, int, S_IRUGO);
MODULE_AUTHOR("Generalplus(Kevin modified)");
MODULE_DESCRIPTION("Generalplus G-Sensor Driver");
MODULE_LICENSE_GP;
MODULE_VERSION("1.0");
