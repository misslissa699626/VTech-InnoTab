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
 
/**************************************************************************
 *                         H E A D E R   F I L E S						  *
 **************************************************************************/
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <linux/delay.h>
#include <mach/gp_gpio.h>
#include <mach/gp_i2c_bus.h>
#include <mach/sensor_mgr.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define MODE_CCIR_601				0
#define MODE_CCIR_656				1
#define MODE_CCIR_HREF				2

#define MODE_POSITIVE_EDGE			0
#define MODE_NEGATIVE_EDGE			1

#define MODE_ACTIVE_LOW				0
#define MODE_ACTIVE_HIGH			1

#define MODE_NONE_INTERLACE			0
#define MODE_INTERLACE				1

#define MODE_DATA_RGB				0
#define MODE_DATA_YUV				1

#define MODE_MCLK_SRC_320M			0	/* src is 320M, can not output 24M */
#define MODE_MCLK_SRC_96M			1	/* src is 96M, can output 24M */

#define I2C_USE_GPIO				0
#define I2C_SCL_IO					0x0
#define I2C_SDA_IO					0x1

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define	SENSOR_ID					0x60
#define	I2C_CLK						100

#define	SENSOR_REG_ADDR_BITS		8
#define	SENSOR_REG_DATA_BITS		8

#define	SENSOR_RESET_LEVEL			0
#define	SENSOR_STANDBY_LEVEL		1

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct sensor_dev_s 
{
	struct v4l2_subdev sd;
	sensor_fmt_t *fmt;	/* Current format */
}sensor_dev_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static sensor_dev_t	g_sensor_dev;
#if I2C_USE_GPIO == 1
static int g_scl_handle, g_sda_handle;
#else
static int g_i2c_handle;
#endif
static char *param[] = {"0", "PORT0", "0", "NONE", "0", "NONE"};
static int nstrs = 6;
module_param_array(param, charp, &nstrs, S_IRUGO);

static sensor_fmt_t g_fmt_table[] =
{
	/* preview mode */
	{
		.desc		= "preview=800*600",
		.pixelformat = V4L2_PIX_FMT_YUYV,
		.bpp 		= 2,
		.mclk_src = MODE_MCLK_SRC_96M,
		.mclk = 24000000,
		.hpixel = 800,
		.hoffset = 0,
		.vline = 600,
		.voffset = 0,
	},
	/* capature mode */
	{
		.desc		= "capture=1600*1200",
		.pixelformat = V4L2_PIX_FMT_YUYV,
		.bpp 		= 2,
		.mclk_src = MODE_MCLK_SRC_96M,
		.mclk = 24000000,
		.hpixel = 1600,
		.hoffset = 0,
		.vline = 1200,
		.voffset = 0,
	},
	/* record mode */
	{
		.desc		= "record=800*600",
		.pixelformat = V4L2_PIX_FMT_YUYV,
		.bpp 		= 2,
		.mclk_src = MODE_MCLK_SRC_96M,
		.mclk = 24000000,
		.hpixel = 800,
		.hoffset = 0,
		.vline = 600,
		.voffset = 0,
	},
};

#define C_SENSOR_FMT_MAX	sizeof(g_fmt_table)/sizeof(sensor_fmt_t)

static sensor_config_t config_table =
{
	.sensor_timing_mode = MODE_CCIR_HREF,
	.sensor_data_mode = MODE_DATA_YUV,
	.sensor_interlace_mode = MODE_NONE_INTERLACE,
	.sensor_pclk_mode = MODE_POSITIVE_EDGE,
	.sensor_hsync_mode = MODE_ACTIVE_HIGH,
	.sensor_vsync_mode = MODE_ACTIVE_HIGH,
	.sensor_fmt_num = C_SENSOR_FMT_MAX,
	.fmt = g_fmt_table,
};

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
#if (I2C_USE_GPIO == 1)		// use i2c gpio
static void 
sccb_delay (
	int i
) 
{
	udelay(i*10);
}

static void 
sccb_start(
	void
)
{
	gp_gpio_set_value(g_scl_handle, 1);
	sccb_delay (2);
	gp_gpio_set_value(g_sda_handle, 1);
	sccb_delay (2);
	gp_gpio_set_value(g_sda_handle, 0);	
	sccb_delay (2);
}

static void 
sccb_stop(
	void
)
{
	sccb_delay (2);
	gp_gpio_set_value(g_sda_handle, 0);					
	sccb_delay (2);
	gp_gpio_set_value(g_scl_handle, 1);					
	sccb_delay (2);
	gp_gpio_set_value(g_sda_handle, 1);					
	sccb_delay (2);
}

static int 
sccb_w_phase(
	unsigned short value
)
{
	int i, nRet = 0;

	for(i=0;i<8;i++)
	{
		gp_gpio_set_value(g_scl_handle,0);		/* SCL0 */
		sccb_delay (2);
		if (value & 0x80)
			gp_gpio_set_value(g_sda_handle, 1);	/* SDA1 */
		else
			gp_gpio_set_value(g_sda_handle, 0);	/* SDA0 */
		gp_gpio_set_value(g_scl_handle, 1);		/* SCL1 */
		sccb_delay(2);
		value <<= 1;
	}
	/* The 9th bit transmission */
	gp_gpio_set_value(g_scl_handle, 0);				/* SCL0 */
	gp_gpio_set_input(g_sda_handle, GPIO_PULL_HIGH);/* SDA is Hi-Z mode */
	
	sccb_delay(2);
	gp_gpio_set_value(g_scl_handle,1);				/* SCL1 */
	sccb_delay(2);
	gp_gpio_set_value(g_scl_handle,0);				/* SCL0 */

	gp_gpio_get_value(g_sda_handle, &i);			/* check ack */
	if(i != 0) nRet = -1;
	gp_gpio_set_output(g_sda_handle, 1, 0);			/* SDA is output */
	return nRet;
}

static int 
sccb_r_phase(
	void
)
{
	int i;
	int data, temp;

	gp_gpio_set_input(g_sda_handle, GPIO_PULL_HIGH);/* SDA is Hi-Z mode */
	data = 0x00;
	for (i=0;i<8;i++)
	{
		gp_gpio_set_value(g_scl_handle,0);			/* SCL0 */
		sccb_delay(2);
		gp_gpio_set_value(g_scl_handle,1);			/* SCL1 */
		gp_gpio_get_value(g_sda_handle, &temp);
		data <<= 1;
		data |= temp;
		sccb_delay(2);
	}
	/* The 9th bit transmission */
	gp_gpio_set_value(g_scl_handle, 0);				/* SCL0 */
	gp_gpio_set_output(g_sda_handle, 1, 0);			/* SDA is output mode */
	gp_gpio_set_value(g_sda_handle, 1);				/* SDA0, the nighth bit is NA must be 1 */

	sccb_delay(2);
	gp_gpio_set_value(g_scl_handle,1);				/* SCL1 */
	sccb_delay(2);
	gp_gpio_set_value(g_scl_handle,0);				/* SCL0 */
	return data;		
}

#if (SENSOR_REG_ADDR_BITS == 16)
#if (SENSOR_REG_DATA_BITS == 16)

#endif
#endif

#if (SENSOR_REG_ADDR_BITS == 16)
#if (SENSOR_REG_DATA_BITS == 8)
static int
sensor_read (
	unsigned short addr,		
	unsigned char *value
)
{
	int nRet = 0;

	/* Data re-verification */
	addr &= 0xFFFF;

	/* Serial bus output mode initialization */
	gp_gpio_set_output(g_scl_handle, 1, 0);
	gp_gpio_set_output(g_sda_handle, 1, 0);	

	/* 2-Phase write transmission cycle is starting now ...*/
	gp_gpio_set_value(g_scl_handle, 1);		/* SCL1	*/	
	gp_gpio_set_value(g_sda_handle, 0);		/* SDA0 */

	sccb_start ();							/* Transmission start */
	nRet = sccb_w_phase (SENSOR_ID);				/* Phase 1 */
	if(nRet < 0) goto Return;
	nRet = sccb_w_phase(addr >> 8);			/* Phase 2 */
	if(nRet < 0) goto Return;
	nRet = sccb_w_phase(addr & 0xFF);						
	if(nRet < 0) goto Return;
	sccb_stop ();							/* Transmission stop */

	/* 2-Phase read transmission cycle is starting now ... */
	sccb_start ();							/* Transmission start */
	nRet = sccb_w_phase (SENSOR_ID | 0x01);		/* Phase 1 (read) */
	if(nRet < 0) goto Return;
	*value = sccb_r_phase();				/* Phase 2 */

Return:
	sccb_stop ();							/* Transmission stop */
	return nRet;
}

static int 
sensor_write (
	unsigned short addr,
	unsigned char data
)
{
	int nRet = 0;

	/* Data re-verification */
	addr &= 0xFFFF;
	data &= 0xFF;

	/* Serial bus output mode initialization */
	gp_gpio_set_output(g_scl_handle, 1, 0);
	gp_gpio_set_output(g_sda_handle, 1, 0);

	/* 3-Phase write transmission cycle is starting now ... */
	gp_gpio_set_value(g_scl_handle, 1);		/* SCL1 */		
	gp_gpio_set_value(g_sda_handle, 0);		/* SDA0 */
	sccb_start();							/* Transmission start */

	nRet = sccb_w_phase(SENSOR_ID);				/* Phase 1 */
	if(nRet < 0) goto Return;
	nRet = sccb_w_phase((addr >> 8)& 0xFF);	/* Phase 2 */
	if(nRet < 0) goto Return;
	nRet = sccb_w_phase(addr & 0xFF);
	if(nRet < 0) goto Return;
	nRet = sccb_w_phase(data);				/* Phase 3 */
	if(nRet < 0) goto Return;
	
Return:
	sccb_stop();							/* Transmission stop */
	return nRet;
}
#endif
#endif

#if (SENSOR_REG_ADDR_BITS == 8)
#if (SENSOR_REG_DATA_BITS == 16)

#endif
#endif

#if (SENSOR_REG_ADDR_BITS == 8)
#if (SENSOR_REG_DATA_BITS == 8)

#endif
#endif

#else	// use i2c controller

#if (SENSOR_REG_ADDR_BITS == 16)
#if (SENSOR_REG_DATA_BITS == 16)

#endif
#endif

#if (SENSOR_REG_ADDR_BITS == 16)
#if (SENSOR_REG_DATA_BITS == 8)
static int 
sensor_read(
	unsigned short reg,
	unsigned char *value
)
{
#if I2C_USE_GPIO == 1
	return sccb_read(OV3640_ID, reg, value);
#else
	char addr[2], data[0];
	int nRet;
	
	addr[0] = (reg >> 8) & 0xFF;
	addr[1] = reg & 0xFF;
	nRet = gp_i2c_bus_write(g_i2c_handle, addr, 2);
	nRet = gp_i2c_bus_read(g_i2c_handle, data, 1);
	*value = data[0];
	return nRet;
#endif
}

static int 
sensor_write(
	unsigned short reg,
	unsigned char value
)
{
#if I2C_USE_GPIO == 1
	return sccb_write(OV3640_ID, reg, value);
#else
	char data[3];
	
	data[0] = (reg >> 8) & 0xFF;
	data[1] = reg & 0xFF;
	data[2] = value;	
	return gp_i2c_bus_write(g_i2c_handle, data, 3);
#endif
}
#endif
#endif

#if (SENSOR_REG_ADDR_BITS == 8)
#if (SENSOR_REG_DATA_BITS == 16)

#endif
#endif

#if (SENSOR_REG_ADDR_BITS == 8)
#if (SENSOR_REG_DATA_BITS == 8)
static int 
sensor_read(
	unsigned char reg,
	unsigned char *value
)
{
#if I2C_USE_GPIO == 1
	return sccb_read(OV2643_ID, reg, value);
#else
	char addr[0], data[0];
	int nRet;
	
	addr[0] = reg ;
	nRet = gp_i2c_bus_write(g_i2c_handle, addr, 1);
	nRet = gp_i2c_bus_read(g_i2c_handle, data, 1);
	*value = data[0];
	return nRet;
#endif
}

static int 
sensor_write(
	unsigned char reg,
	unsigned char value
)
{
#if I2C_USE_GPIO == 1
	return sccb_write(OV2643_ID, reg, value);
#else
	char data[2];
	data[0] = reg ;
	data[1] = value;	
	return gp_i2c_bus_write(g_i2c_handle, data, 2);
#endif
}
#endif
#endif

#endif

static int select_fmt = 0;

static int 
sensor_init(
	struct v4l2_subdev *sd,
	u32 val
)
{
	printk("%s\n", __FUNCTION__);
	sensor_write(0x12,0x80); //SW reset sensor
	msleep(100);
	sensor_write(0xc3, 0x1f);
	sensor_write(0xc4, 0xff);
	sensor_write(0x3d, 0x48); 
	sensor_write(0xdd, 0xa5);
	sensor_write(0x0e, 0x10);
	sensor_write(0x10, 0x0a);
	sensor_write(0x11, 0x00);
	sensor_write(0x0f, 0x14);
	sensor_write(0x0e, 0x10);
		
	sensor_write(0x20, 0x01);
	sensor_write(0x21, 0x98);
	sensor_write(0x22, 0x00);
	sensor_write(0x23, 0x06);
	sensor_write(0x24, 0x32);
	sensor_write(0x26, 0x25);
	sensor_write(0x27, 0x84);
	sensor_write(0x29, 0x05);
	sensor_write(0x2a, 0xdc);
	sensor_write(0x2b, 0x03);
	sensor_write(0x2c, 0x20);
	sensor_write(0x1d, 0x04);
	sensor_write(0x25, 0x04);
	sensor_write(0x27, 0x84);
	sensor_write(0x28, 0x40);
	sensor_write(0x12, 0x39);	//mirror and flip
	sensor_write(0x39, 0xd0);
	sensor_write(0xcd, 0x13);	
		
	sensor_write(0x13, 0xff);
	sensor_write(0x14, 0xa7);
	sensor_write(0x15, 0x42);
	sensor_write(0x3c, 0xa4);
	sensor_write(0x18, 0x60);
	sensor_write(0x19, 0x50);
	sensor_write(0x1a, 0xe2);
	sensor_write(0x37, 0xe8);
	sensor_write(0x16, 0x90);
	sensor_write(0x43, 0x00);
	sensor_write(0x40, 0xfb);
	sensor_write(0xa9, 0x44);
	sensor_write(0x2f, 0xec);
	sensor_write(0x35, 0x10);
	sensor_write(0x36, 0x10);
	sensor_write(0x0c, 0x00);
	sensor_write(0x0d, 0x00);
	sensor_write(0xd0, 0x93);
	sensor_write(0xdc, 0x2b);
	sensor_write(0xd9, 0x41);
	sensor_write(0xd3, 0x02);
		
	sensor_write(0xde, 0x7c);
		
	sensor_write(0x3d, 0x08);
	sensor_write(0x0c, 0x00);
	sensor_write(0x18, 0x2c);
	sensor_write(0x19, 0x24);
	sensor_write(0x1a, 0x71);
	sensor_write(0x9b, 0x69);
	sensor_write(0x9c, 0x7d);
	sensor_write(0x9d, 0x7d);
	sensor_write(0x9e, 0x69);
	sensor_write(0x35, 0x04);
	sensor_write(0x36, 0x04);
	sensor_write(0x65, 0x12);
	sensor_write(0x66, 0x20);
	sensor_write(0x67, 0x39);
	sensor_write(0x68, 0x4e);
	sensor_write(0x69, 0x62);
	sensor_write(0x6a, 0x74);
	sensor_write(0x6b, 0x85);
	sensor_write(0x6c, 0x92);
	sensor_write(0x6d, 0x9e);
	sensor_write(0x6e, 0xb2);
	sensor_write(0x6f, 0xc0);
	sensor_write(0x70, 0xcc);
	sensor_write(0x71, 0xe0);
	sensor_write(0x72, 0xee);
	sensor_write(0x73, 0xf6);
	sensor_write(0x74, 0x11);
	sensor_write(0xab, 0x20);
	sensor_write(0xac, 0x5b);
	sensor_write(0xad, 0x05);
	sensor_write(0xae, 0x1b);
	sensor_write(0xaf, 0x76);
	sensor_write(0xb0, 0x90);
	sensor_write(0xb1, 0x90);
	sensor_write(0xb2, 0x8c);
	sensor_write(0xb3, 0x04);
	sensor_write(0xb4, 0x98);
	sensor_write(0xbC, 0x03);
	sensor_write(0x4d, 0x30);
	sensor_write(0x4e, 0x02);
	sensor_write(0x4f, 0x5c);
	sensor_write(0x50, 0x56);
	sensor_write(0x51, 0x00);
	sensor_write(0x52, 0x66);
	sensor_write(0x53, 0x03);
	sensor_write(0x54, 0x30);
	sensor_write(0x55, 0x02);
	sensor_write(0x56, 0x5c);
	sensor_write(0x57, 0x40);
	sensor_write(0x58, 0x00);
	sensor_write(0x59, 0x66);
	sensor_write(0x5a, 0x03);
	sensor_write(0x5b, 0x20);
	sensor_write(0x5c, 0x02);
	sensor_write(0x5d, 0x5c);
	sensor_write(0x5e, 0x3a);
	sensor_write(0x5f, 0x00);
		
	sensor_write(0x60, 0x66);
	sensor_write(0x41, 0x1f);
	sensor_write(0xb5, 0x01);
	sensor_write(0xb6, 0x02);
	sensor_write(0xb9, 0x40);
	sensor_write(0xba, 0x28);
	sensor_write(0xbf, 0x0c);
	sensor_write(0xc0, 0x3e);
	sensor_write(0xa3, 0x0a);
	sensor_write(0xa4, 0x0f);
	sensor_write(0xa5, 0x09);
	sensor_write(0xa6, 0x16);
	sensor_write(0x9f, 0x0a);
	sensor_write(0xa0, 0x0f);
	sensor_write(0xa7, 0x0a);
	sensor_write(0xa8, 0x0f);
	sensor_write(0xa1, 0x10);
	sensor_write(0xa2, 0x04);
	sensor_write(0xa9, 0x04);
	sensor_write(0xaa, 0xa6);
	sensor_write(0x75, 0x6a);
	sensor_write(0x76, 0x11);
	sensor_write(0x77, 0x92);
	sensor_write(0x78, 0x21);
	sensor_write(0x79, 0xe1);
	sensor_write(0x7a, 0x02);
	sensor_write(0x7c, 0x05);
	sensor_write(0x7d, 0x08);
	sensor_write(0x7e, 0x08);
	sensor_write(0x7f, 0x7c);
	sensor_write(0x80, 0x58);
	sensor_write(0x81, 0x2a);
	sensor_write(0x82, 0xc5);
	sensor_write(0x83, 0x46);
	sensor_write(0x84, 0x3a);
	sensor_write(0x85, 0x54);
	sensor_write(0x86, 0x44);
	sensor_write(0x87, 0xf8);
	sensor_write(0x88, 0x08);
	sensor_write(0x89, 0x70);
	sensor_write(0x8a, 0xf0);
	sensor_write(0x8b, 0xf0);
	sensor_write(0x0f, 0x24); //15fps
	//driving capability for 30fps
	//sensor_write(0xc3, 0xdf); 
}

static int 
sensor_preview(void)
{
	printk("%s\n", __FUNCTION__);

	select_fmt = 0;
	sensor_write(0x12,0x80); //SW reset sensor
	msleep(100);
#if 1
	sensor_write(0xc3, 0x1f);
	sensor_write(0xc4, 0xff);
	sensor_write(0x3d, 0x48); 
	sensor_write(0xdd, 0xa5);
	sensor_write(0x0e, 0x10);
	sensor_write(0x10, 0x0a);
	sensor_write(0x11, 0x00);
	sensor_write(0x0f, 0x14);
	sensor_write(0x0e, 0x10);
		
	sensor_write(0x20, 0x01);
	sensor_write(0x21, 0x98);
	sensor_write(0x22, 0x00);
	sensor_write(0x23, 0x06);
	sensor_write(0x24, 0x32);
	sensor_write(0x26, 0x25);
	sensor_write(0x27, 0x84);
	sensor_write(0x29, 0x05);
	sensor_write(0x2a, 0xdc);
	sensor_write(0x2b, 0x03);
	sensor_write(0x2c, 0x20);
	sensor_write(0x1d, 0x04);
	sensor_write(0x25, 0x04);
	sensor_write(0x27, 0x84);
	sensor_write(0x28, 0x40);
	sensor_write(0x12, 0x39);	//mirror and flip
	sensor_write(0x39, 0xd0);
	sensor_write(0xcd, 0x13);	
		
	sensor_write(0x13, 0xff);
	sensor_write(0x14, 0xa7);
	sensor_write(0x15, 0x42);
	sensor_write(0x3c, 0xa4);
	sensor_write(0x18, 0x60);
	sensor_write(0x19, 0x50);
	sensor_write(0x1a, 0xe2);
	sensor_write(0x37, 0xe8);
	sensor_write(0x16, 0x90);
	sensor_write(0x43, 0x00);
	sensor_write(0x40, 0xfb);
	sensor_write(0xa9, 0x44);
	sensor_write(0x2f, 0xec);
	sensor_write(0x35, 0x10);
	sensor_write(0x36, 0x10);
	sensor_write(0x0c, 0x00);
	sensor_write(0x0d, 0x00);
	sensor_write(0xd0, 0x93);
	sensor_write(0xdc, 0x2b);
	sensor_write(0xd9, 0x41);
	sensor_write(0xd3, 0x02);
		
	sensor_write(0xde, 0x7c);
		
	sensor_write(0x3d, 0x08);
	sensor_write(0x0c, 0x00);
	sensor_write(0x18, 0x2c);
	sensor_write(0x19, 0x24);
	sensor_write(0x1a, 0x71);
	sensor_write(0x9b, 0x69);
	sensor_write(0x9c, 0x7d);
	sensor_write(0x9d, 0x7d);
	sensor_write(0x9e, 0x69);
	sensor_write(0x35, 0x04);
	sensor_write(0x36, 0x04);
	sensor_write(0x65, 0x12);
	sensor_write(0x66, 0x20);
	sensor_write(0x67, 0x39);
	sensor_write(0x68, 0x4e);
	sensor_write(0x69, 0x62);
	sensor_write(0x6a, 0x74);
	sensor_write(0x6b, 0x85);
	sensor_write(0x6c, 0x92);
	sensor_write(0x6d, 0x9e);
	sensor_write(0x6e, 0xb2);
	sensor_write(0x6f, 0xc0);
	sensor_write(0x70, 0xcc);
	sensor_write(0x71, 0xe0);
	sensor_write(0x72, 0xee);
	sensor_write(0x73, 0xf6);
	sensor_write(0x74, 0x11);
	sensor_write(0xab, 0x20);
	sensor_write(0xac, 0x5b);
	sensor_write(0xad, 0x05);
	sensor_write(0xae, 0x1b);
	sensor_write(0xaf, 0x76);
	sensor_write(0xb0, 0x90);
	sensor_write(0xb1, 0x90);
	sensor_write(0xb2, 0x8c);
	sensor_write(0xb3, 0x04);
	sensor_write(0xb4, 0x98);
	sensor_write(0xbC, 0x03);
	sensor_write(0x4d, 0x30);
	sensor_write(0x4e, 0x02);
	sensor_write(0x4f, 0x5c);
	sensor_write(0x50, 0x56);
	sensor_write(0x51, 0x00);
	sensor_write(0x52, 0x66);
	sensor_write(0x53, 0x03);
	sensor_write(0x54, 0x30);
	sensor_write(0x55, 0x02);
	sensor_write(0x56, 0x5c);
	sensor_write(0x57, 0x40);
	sensor_write(0x58, 0x00);
	sensor_write(0x59, 0x66);
	sensor_write(0x5a, 0x03);
	sensor_write(0x5b, 0x20);
	sensor_write(0x5c, 0x02);
	sensor_write(0x5d, 0x5c);
	sensor_write(0x5e, 0x3a);
	sensor_write(0x5f, 0x00);
		
	sensor_write(0x60, 0x66);
	sensor_write(0x41, 0x1f);
	sensor_write(0xb5, 0x01);
	sensor_write(0xb6, 0x02);
	sensor_write(0xb9, 0x40);
	sensor_write(0xba, 0x28);
	sensor_write(0xbf, 0x0c);
	sensor_write(0xc0, 0x3e);
	sensor_write(0xa3, 0x0a);
	sensor_write(0xa4, 0x0f);
	sensor_write(0xa5, 0x09);
	sensor_write(0xa6, 0x16);
	sensor_write(0x9f, 0x0a);
	sensor_write(0xa0, 0x0f);
	sensor_write(0xa7, 0x0a);
	sensor_write(0xa8, 0x0f);
	sensor_write(0xa1, 0x10);
	sensor_write(0xa2, 0x04);
	sensor_write(0xa9, 0x04);
	sensor_write(0xaa, 0xa6);
	sensor_write(0x75, 0x6a);
	sensor_write(0x76, 0x11);
	sensor_write(0x77, 0x92);
	sensor_write(0x78, 0x21);
	sensor_write(0x79, 0xe1);
	sensor_write(0x7a, 0x02);
	sensor_write(0x7c, 0x05);
	sensor_write(0x7d, 0x08);
	sensor_write(0x7e, 0x08);
	sensor_write(0x7f, 0x7c);
	sensor_write(0x80, 0x58);
	sensor_write(0x81, 0x2a);
	sensor_write(0x82, 0xc5);
	sensor_write(0x83, 0x46);
	sensor_write(0x84, 0x3a);
	sensor_write(0x85, 0x54);
	sensor_write(0x86, 0x44);
	sensor_write(0x87, 0xf8);
	sensor_write(0x88, 0x08);
	sensor_write(0x89, 0x70);
	sensor_write(0x8a, 0xf0);
	sensor_write(0x8b, 0xf0);
	//sensor_write(0x0f, 0x24); //15fps
	//driving capability for 30fps
	sensor_write(0xc3, 0xdf); 
#endif



	return select_fmt;	
/*
	sensor_write(0xff, 0x00);	// page 0
	sensor_write(0xc0, 0xc8);
	sensor_write(0xc1, 0x96);
	sensor_write(0x86, 0x3d);
	sensor_write(0x50, 0x89);
	sensor_write(0x51, 0x90);
	sensor_write(0x52, 0x2c);
	sensor_write(0x53, 0x00);
	sensor_write(0x54, 0x00);
	sensor_write(0x55, 0x88);
	sensor_write(0x57, 0x00);
	sensor_write(0x5a, 0xc8);
	sensor_write(0x5b, 0x96);
	sensor_write(0x5c, 0x00);
	return sensor_write(0xd3, 0x82);
*/
}

static int 
sensor_capture(void)
{
	printk("%s\n", __FUNCTION__);

	select_fmt = 1;
	sensor_write(0x12,0x80); //SW reset sensor
	msleep(100);	
#if 0
	sensor_write(0xc3, 0x1f);
	sensor_write(0xc4, 0xff);
	sensor_write(0x3d, 0x48); 
	sensor_write(0xdd, 0xa5);
	sensor_write(0x0e, 0x10);
	sensor_write(0x10, 0x0a);
	sensor_write(0x11, 0x00);
	sensor_write(0x0f, 0x14);
	sensor_write(0x0e, 0x10);
		
	sensor_write(0x20, 0x01);
	sensor_write(0x21, 0x25);
	sensor_write(0x22, 0x00);
	sensor_write(0x23, 0x0c);
	sensor_write(0x24, 0x50);
	sensor_write(0x26, 0x2d);
	sensor_write(0x27, 0x04);
	sensor_write(0x29, 0x06);
	sensor_write(0x2a, 0x40);
	sensor_write(0x2b, 0x02);
	sensor_write(0x2c, 0xee);
	sensor_write(0x1d, 0x04);
	sensor_write(0x25, 0x04);
	sensor_write(0x27, 0x04);
	sensor_write(0x28, 0x40);
	sensor_write(0x12, 0x78);	//mirror and flip
	sensor_write(0x39, 0x10);
	sensor_write(0xcd, 0x12);	
		
	sensor_write(0x13, 0xff);
	sensor_write(0x14, 0xa7);
	sensor_write(0x15, 0x42);
	sensor_write(0x3c, 0xa4);
	sensor_write(0x18, 0x60);
	sensor_write(0x19, 0x50);
	sensor_write(0x1a, 0xe2);
	sensor_write(0x37, 0xe8);
	sensor_write(0x16, 0x90);
	sensor_write(0x43, 0x00);
	sensor_write(0x40, 0xfb);
	sensor_write(0xa9, 0x44);
	sensor_write(0x2f, 0xec);
	sensor_write(0x35, 0x10);
	sensor_write(0x36, 0x10);
	sensor_write(0x0c, 0x00);
	sensor_write(0x0d, 0x00);
	sensor_write(0xd0, 0x93);
	sensor_write(0xdc, 0x2b);
	sensor_write(0xd9, 0x41);
	sensor_write(0xd3, 0x02);
		
	sensor_write(0x3d, 0x08);
	sensor_write(0x0c, 0x00);
	sensor_write(0x18, 0x2c);
	sensor_write(0x19, 0x24);
	sensor_write(0x1a, 0x71);
	sensor_write(0x9b, 0x69);
	sensor_write(0x9c, 0x7d);
	sensor_write(0x9d, 0x7d);
	sensor_write(0x9e, 0x69);
	sensor_write(0x35, 0x04);
	sensor_write(0x36, 0x04);
	sensor_write(0x65, 0x12);
	sensor_write(0x66, 0x20);
	sensor_write(0x67, 0x39);
	sensor_write(0x68, 0x4e);
	sensor_write(0x69, 0x62);
	sensor_write(0x6a, 0x74);
	sensor_write(0x6b, 0x85);
	sensor_write(0x6c, 0x92);
	sensor_write(0x6d, 0x9e);
	sensor_write(0x6e, 0xb2);
	sensor_write(0x6f, 0xc0);
	sensor_write(0x70, 0xcc);
	sensor_write(0x71, 0xe0);
	sensor_write(0x72, 0xee);
	sensor_write(0x73, 0xf6);
	sensor_write(0x74, 0x11);
	sensor_write(0xab, 0x20);
	sensor_write(0xac, 0x5b);
	sensor_write(0xad, 0x05);
	sensor_write(0xae, 0x1b);
	sensor_write(0xaf, 0x76);
	sensor_write(0xb0, 0x90);
	sensor_write(0xb1, 0x90);
	sensor_write(0xb2, 0x8c);
	sensor_write(0xb3, 0x04);
	sensor_write(0xb4, 0x98);
	sensor_write(0xbC, 0x03);
	sensor_write(0x4d, 0x30);
	sensor_write(0x4e, 0x02);
	sensor_write(0x4f, 0x5c);
	sensor_write(0x50, 0x56);
	sensor_write(0x51, 0x00);
	sensor_write(0x52, 0x66);
	sensor_write(0x53, 0x03);
	sensor_write(0x54, 0x30);
	sensor_write(0x55, 0x02);
	sensor_write(0x56, 0x5c);
	sensor_write(0x57, 0x40);
	sensor_write(0x58, 0x00);
	sensor_write(0x59, 0x66);
	sensor_write(0x5a, 0x03);
	sensor_write(0x5b, 0x20);
	sensor_write(0x5c, 0x02);
	sensor_write(0x5d, 0x5c);
	sensor_write(0x5e, 0x3a);
	sensor_write(0x5f, 0x00);
		
	sensor_write(0x60, 0x66);
	sensor_write(0x41, 0x1f);
	sensor_write(0xb5, 0x01);
	sensor_write(0xb6, 0x02);
	sensor_write(0xb9, 0x40);
	sensor_write(0xba, 0x28);
	sensor_write(0xbf, 0x0c);
	sensor_write(0xc0, 0x3e);
	sensor_write(0xa3, 0x0a);
	sensor_write(0xa4, 0x0f);
	sensor_write(0xa5, 0x09);
	sensor_write(0xa6, 0x16);
	sensor_write(0x9f, 0x0a);
	sensor_write(0xa0, 0x0f);
	sensor_write(0xa7, 0x0a);
	sensor_write(0xa8, 0x0f);
	sensor_write(0xa1, 0x10);
	sensor_write(0xa2, 0x04);
	sensor_write(0xa9, 0x04);
	sensor_write(0xaa, 0xa6);
	sensor_write(0x75, 0x6a);
	sensor_write(0x76, 0x11);
	sensor_write(0x77, 0x92);
	sensor_write(0x78, 0x21);
	sensor_write(0x79, 0xe1);
	sensor_write(0x7a, 0x02);
	sensor_write(0x7c, 0x05);
	sensor_write(0x7d, 0x08);
	sensor_write(0x7e, 0x08);
	sensor_write(0x7f, 0x7c);
	sensor_write(0x80, 0x58);
	sensor_write(0x81, 0x2a);
	sensor_write(0x82, 0xc5);
	sensor_write(0x83, 0x46);
	sensor_write(0x84, 0x3a);
	sensor_write(0x85, 0x54);
	sensor_write(0x86, 0x44);
	sensor_write(0x87, 0xf8);
	sensor_write(0x88, 0x08);
	sensor_write(0x89, 0x70);
	sensor_write(0x8a, 0xf0);
	sensor_write(0x8b, 0xf0);
	sensor_write(0x0f, 0x34); //10fps
	//driving capability for 30fps
	sensor_write(0xc3, 0xdf); 
#endif	
#if 1//UXGA
	sensor_write(0xc3, 0x1f);
	sensor_write(0xc4, 0xff);
	sensor_write(0x3d, 0x48);
	sensor_write(0xdd, 0xa5);
	sensor_write(0x0e, 0x10);
	sensor_write(0x10, 0x0a);
	sensor_write(0x11, 0x00);
	sensor_write(0x0f, 0x14);
	sensor_write(0x0e, 0x10);
	sensor_write(0x21, 0x25);
	sensor_write(0x23, 0x0c);
	sensor_write(0x12, 0x38);	//mirror and flip
	sensor_write(0x39, 0x10);
	sensor_write(0xcd, 0x12);	
	sensor_write(0x13, 0xff);
	sensor_write(0x14, 0xa7);
	sensor_write(0x15, 0x42);
	sensor_write(0x3c, 0xa4);
	sensor_write(0x18, 0x60);
	sensor_write(0x19, 0x50);
	sensor_write(0x1a, 0xe2);
	sensor_write(0x37, 0xe8);
	sensor_write(0x16, 0x90);
	sensor_write(0x43, 0x00);
	sensor_write(0x40, 0xfb);
	sensor_write(0xa9, 0x44);
	sensor_write(0x2f, 0xec);
	sensor_write(0x35, 0x10);
	sensor_write(0x36, 0x10);
	sensor_write(0x0c, 0x00);
	sensor_write(0x0d, 0x00);
	sensor_write(0xd0, 0x93);
	sensor_write(0xdc, 0x2b);
	sensor_write(0xd9, 0x41);
	sensor_write(0xd3, 0x02);
		
	sensor_write(0x3d, 0x08);
	sensor_write(0x0c, 0x00);
	sensor_write(0x18, 0x2c);
	sensor_write(0x19, 0x24);
	sensor_write(0x1a, 0x71);
	sensor_write(0x9b, 0x69);
	sensor_write(0x9c, 0x7d);
	sensor_write(0x9d, 0x7d);
	sensor_write(0x9e, 0x69);
	sensor_write(0x35, 0x04);
	sensor_write(0x36, 0x04);
	sensor_write(0x65, 0x12);
	sensor_write(0x66, 0x20);
	sensor_write(0x67, 0x39);
	sensor_write(0x68, 0x4e);
	sensor_write(0x69, 0x62);
	sensor_write(0x6a, 0x74);
	sensor_write(0x6b, 0x85);
	sensor_write(0x6c, 0x92);
	sensor_write(0x6d, 0x9e);
	sensor_write(0x6e, 0xb2);
	sensor_write(0x6f, 0xc0);
	sensor_write(0x70, 0xcc);
	sensor_write(0x71, 0xe0);
	sensor_write(0x72, 0xee);
	sensor_write(0x73, 0xf6);
	sensor_write(0x74, 0x11);
	sensor_write(0xab, 0x20);
	sensor_write(0xac, 0x5b);
	sensor_write(0xad, 0x05);
	sensor_write(0xae, 0x1b);
	sensor_write(0xaf, 0x76);
	sensor_write(0xb0, 0x90);
	sensor_write(0xb1, 0x90);
	sensor_write(0xb2, 0x8c);
	sensor_write(0xb3, 0x04);
	sensor_write(0xb4, 0x98);
	sensor_write(0xbC, 0x03);
	sensor_write(0x4d, 0x30);
	sensor_write(0x4e, 0x02);
	sensor_write(0x4f, 0x5c);
	sensor_write(0x50, 0x56);
	sensor_write(0x51, 0x00);
	sensor_write(0x52, 0x66);
	sensor_write(0x53, 0x03);
	sensor_write(0x54, 0x30);
	sensor_write(0x55, 0x02);
	sensor_write(0x56, 0x5c);
	sensor_write(0x57, 0x40);
	sensor_write(0x58, 0x00);
	sensor_write(0x59, 0x66);
	sensor_write(0x5a, 0x03);
	sensor_write(0x5b, 0x20);
	sensor_write(0x5c, 0x02);
	sensor_write(0x5d, 0x5c);
	sensor_write(0x5e, 0x3a);
	sensor_write(0x5f, 0x00);
		
	sensor_write(0x60, 0x66);
	sensor_write(0x41, 0x1f);
	sensor_write(0xb5, 0x01);
	sensor_write(0xb6, 0x02);
	sensor_write(0xb9, 0x40);
	sensor_write(0xba, 0x28);
	sensor_write(0xbf, 0x0c);
	sensor_write(0xc0, 0x3e);
	sensor_write(0xa3, 0x0a);
	sensor_write(0xa4, 0x0f);
	sensor_write(0xa5, 0x09);
	sensor_write(0xa6, 0x16);
	sensor_write(0x9f, 0x0a);
	sensor_write(0xa0, 0x0f);
	sensor_write(0xa7, 0x0a);
	sensor_write(0xa8, 0x0f);
	sensor_write(0xa1, 0x10);
	sensor_write(0xa2, 0x04);
	sensor_write(0xa9, 0x04);
	sensor_write(0xaa, 0xa6);
	sensor_write(0x75, 0x6a);
	sensor_write(0x76, 0x11);
	sensor_write(0x77, 0x92);
	sensor_write(0x78, 0x21);
	sensor_write(0x79, 0xe1);
	sensor_write(0x7a, 0x02);
	sensor_write(0x7c, 0x05);
	sensor_write(0x7d, 0x08);
	sensor_write(0x7e, 0x08);
	sensor_write(0x7f, 0x7c);
	sensor_write(0x80, 0x58);
	sensor_write(0x81, 0x2a);
	sensor_write(0x82, 0xc5);
	sensor_write(0x83, 0x46);
	sensor_write(0x84, 0x3a);
	sensor_write(0x85, 0x54);
	sensor_write(0x86, 0x44);
	sensor_write(0x87, 0xf8);
	sensor_write(0x88, 0x08);
	sensor_write(0x89, 0x70);
	sensor_write(0x8a, 0xf0);
	sensor_write(0x8b, 0xf0);
	//sensor_write(0x0f, 0x34); //5fps
	//driving capability for 15fps
	sensor_write(0xc3, 0xdf);
#endif
	return select_fmt;
/*
	sensor_write(0xff, 0x00);	// page 0
	sensor_write(0xc0, 0xc8);
	sensor_write(0xc1, 0x96);
	sensor_write(0x86, 0x3d);
	sensor_write(0x50, 0x00);
	sensor_write(0x51, 0x90);
	sensor_write(0x52, 0x2c);
	sensor_write(0x53, 0x00);
	sensor_write(0x54, 0x00);
	sensor_write(0x55, 0x88);
	sensor_write(0x57, 0x00);
	sensor_write(0x5a, 0x90);
	sensor_write(0x5b, 0x2c);
	sensor_write(0x5c, 0x05);
	return sensor_write(0xd3, 0x82);
*/	
}

static int 
sensor_record(void)
{
	printk("%s\n", __FUNCTION__);
	select_fmt = 2;
/*
	sensor_write(0xff, 0x01);
	sensor_write(0x13, 0xf7);	//turn on AGC/AEC
	sensor_write(0x12, 0x40);
	sensor_write(0x11, 0x00);
	sensor_write(0x17, 0x11);
	sensor_write(0x18, 0x43);
	sensor_write(0x19, 0x00);
	sensor_write(0x1a, 0x4b);
	sensor_write(0x32, 0x09);
	sensor_write(0x37, 0xc0);
	//sensor_write(0x46, 0x5e);	//22.8m
	sensor_write(0x46, 0x87);	//24m
	sensor_write(0x4f, 0xca);
	sensor_write(0x50, 0xa8);
	sensor_write(0x5a, 0x34);
	sensor_write(0x6d, 0x00);
	sensor_write(0x3d, 0x38);
	sensor_write(0x39, 0x12);
	sensor_write(0x35, 0xda);
	sensor_write(0x22, 0x19);
	sensor_write(0x37, 0xc3);
	sensor_write(0x23, 0x00);
	sensor_write(0x34, 0xc0);
	sensor_write(0x36, 0x1a);
	sensor_write(0x06, 0x88);
	sensor_write(0x07, 0xc0);
	sensor_write(0x0d, 0x87);
	sensor_write(0x0e, 0x41);
	sensor_write(0x4c, 0x00);
	sensor_write(0xff, 0x00);
	sensor_write(0xe0, 0x04);
	sensor_write(0xc0, 0x64);
	sensor_write(0xc1, 0x4B);
	sensor_write(0x8c, 0x00);
	sensor_write(0x86, 0x1D);
	sensor_write(0x50, 0x00);
	sensor_write(0x51, 0xC8);
	sensor_write(0x52, 0x96);
	sensor_write(0x53, 0x00);
	sensor_write(0x54, 0x00);
	sensor_write(0x55, 0x00);
	sensor_write(0x5a, 0xC8);
	sensor_write(0x5b, 0x96);
	sensor_write(0x5c, 0x00);
	sensor_write(0xd3, 0x82);
	return sensor_write(0xe0, 0x00);
*/
}

static int 
sensor_reset(
	struct v4l2_subdev *sd, 
	u32 val
)
{
	return 0;
}

static int 
sensor_queryctrl(
	struct v4l2_subdev *sd,
	struct v4l2_queryctrl *qc
)
{
	/* Fill in min, max, step and default value for these controls. */
	switch(qc->id)
	{
	case V4L2_CID_AUTO_WHITE_BALANCE:
		qc->minimum = 0;
		qc->maximum = 1;
		qc->step = 1;
		qc->default_value = 1;
		break;

	case V4L2_CID_POWER_LINE_FREQUENCY:
		qc->minimum = 50;
		qc->maximum = 60;
		qc->step = 10;
		qc->default_value = 50;
		break;
	
	case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
		qc->minimum = 0;
		qc->maximum = 3;
		qc->step = 1;
		qc->default_value = 0;
		break;

	case V4L2_CID_BACKLIGHT_COMPENSATION:
		qc->minimum = 0;
		qc->maximum = 1;
		qc->step = 1;
		qc->default_value = 0;
		break;

	default:
		return -EINVAL;
	}
	return 0;
}

static int 
sensor_g_ctrl(
	struct v4l2_subdev *sd, 
	struct v4l2_control *ctrl
)
{
	switch(ctrl->id)
	{
	case V4L2_CID_AUTO_WHITE_BALANCE:
		break;

	case V4L2_CID_POWER_LINE_FREQUENCY:
		break;

	case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
		break;

	case V4L2_CID_BACKLIGHT_COMPENSATION:	
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int 
sensor_s_ctrl(
	struct v4l2_subdev *sd, 
	struct v4l2_control *ctrl
)
{
	unsigned char data;
	int nRet = 0;
	switch(ctrl->id)
	{
	case V4L2_CID_AUTO_WHITE_BALANCE:
		printk("WBAUTO = %d\n", ctrl->value);
		nRet = sensor_write(0xff, 0x00);
		nRet = sensor_read(0xc7,(unsigned char *)&data); 					
		if(ctrl->value) {	// Enable Auto AWB
			  nRet = sensor_write(0xc7,data& ~0x40);
		}else{	// Disable Auto AWB
			
		}
		break;

	case V4L2_CID_POWER_LINE_FREQUENCY:
		printk("PL = %d\n", ctrl->value);
		if(ctrl->value == V4L2_CID_POWER_LINE_FREQUENCY_DISABLED) {

		}else if(ctrl->value == V4L2_CID_POWER_LINE_FREQUENCY_50HZ) {
			nRet = sensor_write(0xff, 0x01);	
			nRet = sensor_write(0x0c, 0x38);
			if(select_fmt == 0){	// previre mode and capture mode
				nRet = sensor_write(0x46, 0x3f);
			}else if(select_fmt == 2){	// record mode
				nRet = sensor_write(0x46, 0x87);
			}
		}else if(ctrl->value == V4L2_CID_POWER_LINE_FREQUENCY_60HZ) {
			nRet = sensor_write(0xff, 0x01);	
			nRet = sensor_write(0x0c, 0x3c);
			nRet = sensor_write(0x46, 0x00);
		}else {
			return -EINVAL;
		}
		break;
		
	case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
		printk("WB = %d\n", ctrl->value);
		nRet = sensor_write(0xff, 0x00);
		nRet = sensor_read(0xc7, (unsigned char *)&data);
		if(ctrl->value == 0) {	//SUNSHINE
			nRet = sensor_write(0xc7, data|0x40);	
			nRet = sensor_write(0xCC, 0x4e);
			nRet = sensor_write(0xCD, 0x40);
			nRet = sensor_write(0xCE, 0x48);		
		}else if(ctrl->value == 1) {	//CLOUDY
			  nRet = sensor_write(0xc7,data|0x40);  // Manual AWB mode
			  nRet = sensor_write(0xCC, 0x38);
			  nRet = sensor_write(0xCD, 0x40);
			  nRet = sensor_write(0xCE, 0x58);
		}else if(ctrl->value == 2) {	//FLUORESCENCE
			  nRet = sensor_write(0xc7,data|0x40);  // Manual AWB mode
			  nRet = sensor_write(0xCC, 0x40);
			  nRet = sensor_write(0xCD, 0x40);
			  nRet = sensor_write(0xCE, 0x50);		
		}else if(ctrl->value == 3) {	//INCANDESCENCE
			nRet = sensor_write(0xc7,data|0x40);  
			nRet = sensor_write(0xCC, 0x30);
			nRet = sensor_write(0xCD, 0x40);
			nRet = sensor_write(0xCE, 0x66);
		}
		break; 
 
	case V4L2_CID_BACKLIGHT_COMPENSATION:
		printk("NightMode = %d\n", ctrl->value);
		if(ctrl->value) {
			nRet = sensor_write(0xff, 0x01);
			nRet = sensor_write(0x0f, 0x4b);
			nRet = sensor_write(0x03, 0x4f);
		}else {
			nRet = sensor_write(0xff, 0x01);
			nRet = sensor_write(0x0f, 0x43);
			nRet = sensor_write(0x03, 0x0f);
			nRet = sensor_write(0x2d, 0x00);
			nRet = sensor_write(0x2e, 0x00);
		}
		break;

	default:
		return -EINVAL;
	}

	return nRet; 
}

static int 
sensor_querystd(
	struct v4l2_subdev *sd,
	v4l2_std_id *std
)
{
	return 0;
}

static int 
sensor_enum_fmt(
	struct v4l2_subdev *sd, 
	struct v4l2_fmtdesc *fmtdesc
)
{
	printk("%s\n", __FUNCTION__);
	if(fmtdesc->index >= C_SENSOR_FMT_MAX)
		return -EINVAL;

	fmtdesc->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	memcpy((void *)fmtdesc->description, (void *)g_fmt_table[fmtdesc->index].desc, 32);
	fmtdesc->pixelformat = g_fmt_table[fmtdesc->index].pixelformat;
	return 0;
}

static int 
sensor_g_fmt(
	struct v4l2_subdev *sd, 
	struct v4l2_format *fmt
)
{
	printk("%s\n", __FUNCTION__);
	fmt->fmt.pix.width = g_sensor_dev.fmt->hpixel;
	fmt->fmt.pix.height = g_sensor_dev.fmt->vline;
	fmt->fmt.pix.pixelformat = g_sensor_dev.fmt->pixelformat;
	fmt->fmt.pix.field = V4L2_FIELD_NONE;
	fmt->fmt.pix.bytesperline = g_sensor_dev.fmt->hpixel * g_sensor_dev.fmt->bpp;
	fmt->fmt.pix.sizeimage = fmt->fmt.pix.bytesperline * g_sensor_dev.fmt->vline;

	return 0;
}

static int 
sensor_try_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_format *fmt
)
{

	return 0;
}

static int 
sensor_s_fmt(
	struct v4l2_subdev *sd, 
	struct v4l2_format *fmt
)
{
	int ret,i;

	printk("%s\n", __FUNCTION__);
	for(i=0; i<config_table.sensor_fmt_num; i++) {	
		if( (fmt->fmt.pix.width == config_table.fmt[i].hpixel) && (fmt->fmt.pix.height == config_table.fmt[i].vline) ) {
				printk("sensor mode = %d \n", i);
				if(0 == i) {
					ret = sensor_preview();
				}else if (1 == i) {
					ret = sensor_capture();
				}else if (2 == i) {
					ret = sensor_record();
				}else {
					ret = -1;
			}
	
		//g_sensor_dev.fmt = &g_fmt_table[fmt->fmt.pix.priv];
		g_sensor_dev.fmt = &g_fmt_table[i];
		printk("ret = %d /n",ret);
		return ret;
		}
	}
	return -1;
}

static int 
sensor_cropcap(
	struct v4l2_subdev *sd,
	struct v4l2_cropcap *cc
)
{
	return 0;
}

static int 
sensor_g_crop(
	struct v4l2_subdev *sd,
	struct v4l2_crop *crop
)
{
	return 0;
}

static int 
sensor_s_crop(
	struct v4l2_subdev *sd,
	struct v4l2_crop *crop
)
{
	return 0;
}

static int 
sensor_g_parm(
	struct v4l2_subdev *sd,
	struct v4l2_streamparm *parms
)
{
	return 0;
}

static int 
sensor_s_parm(
	struct v4l2_subdev *sd,
	struct v4l2_streamparm *param
)
{
	return 0;
}

static int 
sensor_s_interface(
	struct v4l2_subdev *sd,
	struct v4l2_interface *interface
)
{
	return 0;
}

static int 
sensor_suspend(
	struct v4l2_subdev *sd
)
{
	// need implement
	//sensor_write(0x3306, 0x02);
	return 0;
}

static int 
sensor_resume(
	struct v4l2_subdev *sd
)
{
	// need implement
	//sensor_write(0x3306, 0x02);
	return 0;
}

static const struct v4l2_subdev_core_ops sensor_core_ops = 
{
	.init = sensor_init,
	.reset = sensor_reset,
	.queryctrl = sensor_queryctrl,
	.g_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
};

static const struct v4l2_subdev_video_ops sensor_video_ops = 
{
	.querystd = sensor_querystd,
	.enum_fmt = sensor_enum_fmt,
	.g_fmt = sensor_g_fmt,
	.try_fmt = sensor_try_fmt,
	.s_fmt = sensor_s_fmt,
	.cropcap = sensor_cropcap,
	.g_crop = sensor_g_crop,
	.s_crop = sensor_s_crop,
	.g_parm = sensor_g_parm,
	.s_parm = sensor_s_parm,
};

static const struct v4l2_subdev_ext_ops sensor_ext_ops = 
{
	.s_interface = sensor_s_interface,
	.suspend = sensor_suspend,
	.resume = sensor_resume,
};

static const struct v4l2_subdev_ops sensor_ops = 
{
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
	.ext = &sensor_ext_ops
};

static int __init 
sensor_module_init(
		void
)
{
#if I2C_USE_GPIO == 1
	g_scl_handle = gp_gpio_request(I2C_SCL_IO, "SCL"); 
	g_sda_handle = gp_gpio_request(I2C_SDA_IO, "SDA");
	if((g_scl_handle == 0) || (g_scl_handle == -EINVAL) || (g_scl_handle == -ENOMEM)||
		(g_sda_handle == 0) || (g_sda_handle == -EINVAL) || (g_sda_handle == -ENOMEM))
	{
		printk(KERN_WARNING "GpioReqFail %d, %d\n", g_scl_handle, g_sda_handle);
		gp_gpio_release(g_scl_handle);
		gp_gpio_release(g_sda_handle);	
		return -1;
	}
	gp_gpio_set_output(g_scl_handle, 1, 0);
	gp_gpio_set_output(g_sda_handle, 1, 0);
#else
	g_i2c_handle = gp_i2c_bus_request(SENSOR_ID, I2C_CLK);	/*100KHZ*/
	if((g_i2c_handle == 0) ||(g_i2c_handle == -ENOMEM))
	{
		printk(KERN_WARNING "i2cReqFail %d\n", g_i2c_handle);
		return -1;
	}
#endif

	printk(KERN_WARNING "ModuleInit: ov2643 \n");
	g_sensor_dev.fmt = &g_fmt_table[0];
	v4l2_subdev_init(&(g_sensor_dev.sd), &sensor_ops);
	strcpy(g_sensor_dev.sd.name, "sensor_ov2643");
	register_sensor(&g_sensor_dev.sd, (int *)&param[0], &config_table);
	return 0;
}

static void __exit
sensor_module_exit(
		void
)
{
#if I2C_USE_GPIO == 1
	gp_gpio_release(g_scl_handle);
	gp_gpio_release(g_sda_handle);	
#else
	gp_i2c_bus_release(g_i2c_handle);
#endif
	unregister_sensor(&(g_sensor_dev.sd));
}

module_init(sensor_module_init);
module_exit(sensor_module_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/
MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("Generalplus ov2643 Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

