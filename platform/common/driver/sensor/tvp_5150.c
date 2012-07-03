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
#include <linux/fs.h> /* everything... */
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <mach/module.h>
#include <mach/sensor_mgr.h>
#include <mach/gp_i2c_bus.h>
#include <mach/diag.h>
#include <mach/gp_csi.h>


/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define CMATRIX_LEN		6
#define VGA_WIDTH		720
#define VGA_HEIGHT		480
#define QVGA_WIDTH		320
#define QVGA_HEIGHT		240

#define COM7_FMT_VGA	0x00

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
static char *param[] = {"1", "PORT0", "0", "NONE", "0", "NONE"};
static int nstrs = 6;
module_param_array(param, charp, &nstrs, S_IRUGO);

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/


/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/


/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
int tvp5150_init(struct v4l2_subdev *sd,	u32 val);
static int tvp5150_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc);
static int tvp5150_enum_fmt(struct v4l2_subdev *sd, struct v4l2_fmtdesc *fmt);
static int tvp5150_try_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt);
static int tvp5150_s_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt);
static int tvp5150_g_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt);
static int tvp5150_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int tvp5150_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int tvp5150_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param);
static int tvp5150_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param);
static int tvp5150_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int tvp5150_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int tvp5150_cropcap(struct v4l2_subdev *sd, struct v4l2_cropcap *cc);
static int tvp5150_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *crop);
static int tvp5150_s_crop(struct v4l2_subdev *sd, struct v4l2_crop *crop);
static int tvp5150_s_interface(struct v4l2_subdev *sd, struct v4l2_interface *interface);
static int tvp5150_suspend(struct v4l2_subdev *sd);
static int tvp5150_resume(struct v4l2_subdev *sd);
static int tvp5150_querystd(struct v4l2_subdev *sd, v4l2_std_id *id);
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

struct regval_list {
	unsigned char reg_num;
	unsigned char value;
};

static struct regval_list tvp5150_default_regs[] = {
	//PAL & INIT
	{0x05, 0x01},
	{0x05, 0x00},
	{0x02, 0x00},
	{0xD0, 0xFF},
	{0x0f, 0x0a},
	{0x03, 0x6d},
	{0x0d, 0x47},
//NTSC
#if 0
	{0x11, 0xff},
	{0x12, 0x07},
	{0x03, 0x0d},
	{0x0d, 0x47},
#endif
	{ 0xff, 0xff }	//end
};

static struct regval_list tvp5150_fmt_yuv422[] = {
	{ 0xff, 0xff }
};

static struct regval_list tvp5150_fmt_rgb565[] = {
	{ 0xff, 0xff }
};

static struct regval_list tvp5150_resume_regs[] = {

	{ 0xff, 0xff }
};

static struct regval_list tvp5150_suspend_regs[] = {

	{ 0xff, 0xff }
};

static const struct v4l2_subdev_core_ops tvp5150_core_ops = {
//	.g_chip_ident = tvp5150_g_chip_ident,
	.g_ctrl = tvp5150_g_ctrl,
	.s_ctrl = tvp5150_s_ctrl,
	.queryctrl = tvp5150_queryctrl,
//	.reset = tvp5150_reset,
	.init = tvp5150_init,
};

static const struct v4l2_subdev_video_ops tvp5150_video_ops = {
	.enum_fmt = tvp5150_enum_fmt,
	.try_fmt = tvp5150_try_fmt,
	.s_fmt = tvp5150_s_fmt,
	.g_fmt = tvp5150_g_fmt,
	.s_parm = tvp5150_s_parm,
	.g_parm = tvp5150_g_parm,
	.cropcap = tvp5150_cropcap,
	.g_crop = tvp5150_g_crop,
	.s_crop = tvp5150_s_crop,
	.querystd = tvp5150_querystd,	
};

static const struct v4l2_subdev_ext_ops tvp5150_ext_ops = {
	.s_interface = tvp5150_s_interface,
	.suspend = tvp5150_suspend,
	.resume = tvp5150_resume,
};

static const struct v4l2_subdev_ops tvp5150_ops = {
	.core = &tvp5150_core_ops,
	.video = &tvp5150_video_ops,
	.ext = &tvp5150_ext_ops
};

static struct tvp5150_format_struct {
	__u8 *desc;
	__u32 pixelformat;
	struct regval_list *regs;
	int cmatrix[CMATRIX_LEN];
	int bpp;   /* Bytes per pixel */
} tvp5150_formats[] = {
	{
		.desc		= "YUYV 4:2:2",
		.pixelformat= V4L2_PIX_FMT_YUYV,
		.regs		= tvp5150_fmt_yuv422,
		.cmatrix	= { 128, -128, 0, -34, -94, 128 },
		.bpp		= 2,
	},
	{
		.desc		= "RGB 5-6-5",
		.pixelformat= V4L2_PIX_FMT_RGB565,
		.regs		= tvp5150_fmt_rgb565,
		.cmatrix	= { 128, -128, 0, -34, -94, 128 },
		.bpp		= 2,
	},
};
#define N_tvp5150_FMTS ARRAY_SIZE(tvp5150_formats)

static struct tvp5150_win_size {
	int	width;
	int	height;
	unsigned char com7_bit;
	int	hstart;		/* Start/stop values for the camera.  Note */
	int	hstop;		/* that they do not always make complete */
	int	vstart;		/* sense to humans, but evidently the sensor */
	int	vstop;		/* will do the right thing... */
	struct regval_list *regs; /* Regs to tweak */
/* h/vref stuff */
} tvp5150_win_sizes[] = {
	/* VGA */
	{
		.width		= VGA_WIDTH,
		.height		= VGA_HEIGHT,
		.com7_bit	= COM7_FMT_VGA,
		.hstart		= 158,		/* These values from */
		.hstop		=  14,		/* Omnivision */
		.vstart		=  10,
		.vstop		= 490,
		.regs 		= NULL,
	},
};
#define N_WIN_SIZES (ARRAY_SIZE(tvp5150_win_sizes))

struct tvp5150_format_struct;  /* coming later */
typedef struct tvp5150_info_t {
	struct v4l2_subdev sd;
	struct tvp5150_format_struct *fmt;  /* Current format */
	unsigned char sat;		/* Saturation value */
	int hue;			/* Hue value */
}tvp5150_info_s;

static tvp5150_info_s tvp5150_info;
struct i2c_bus_attr_t *i2c_attr;


/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/

static int tvp5150_read(
	unsigned char reg,
	unsigned char *value
)
{
	unsigned char data;
	int ret;
	
	data = reg;	
	ret = gp_i2c_bus_write((int)i2c_attr, &data, 1);
	if ( ret < 0 ) 
		return ret;

	ret = gp_i2c_bus_read((int)i2c_attr, &data, 1);
	*value = data;

	return ret;
}

static int tvp5150_write(
	unsigned char reg,
	unsigned char value
)
{	
	char data[2];
	
	data[0]=reg;
	data[1]=value;
	
	return gp_i2c_bus_write((int)i2c_attr, data, 2);
}

static int tvp5150_write_array(struct regval_list *vals)
{
	int ret;
	int i;
	int k;
	k = 0;

	while (vals->reg_num != 0xff || vals->value != 0xff) {
		ret = tvp5150_write(vals->reg_num, vals->value);
		if (ret < 0)
			return ret;
		if(vals->reg_num == 0x12)
		{
			for(i = 0; i < 4000; i = i + 1)
	  			k = k + 1;
		}
		vals++;
	}
	return 0;
}

static int tvp5150_s_interface(
	struct v4l2_subdev *sd,
	struct v4l2_interface *interface
)
{
	return 0;
}

static int tvp5150_enum_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_fmtdesc *fmt
)
{
	struct tvp5150_format_struct *ofmt;

	if (fmt->index >= N_tvp5150_FMTS)
		return -EINVAL;

	ofmt = tvp5150_formats + fmt->index;
	fmt->flags = 0;
	strcpy(fmt->description, ofmt->desc);
	fmt->pixelformat = ofmt->pixelformat;
	
	return 0;
}

static int tvp5150_try_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_format *fmt
)
{
	int index;
	struct tvp5150_win_size *wsize;
	struct v4l2_pix_format *pix = &fmt->fmt.pix;
	
	for( index=0; index<N_tvp5150_FMTS; index++ )
		if (tvp5150_formats[index].pixelformat == pix->pixelformat)
			break;
	if (index >= N_tvp5150_FMTS) {
		/* default to first format */
		index = 0;
		pix->pixelformat = tvp5150_formats[0].pixelformat;
		printk(KERN_NOTICE "No match format\n");
	}
	
	pix->field = V4L2_FIELD_NONE;
	
	for (wsize = tvp5150_win_sizes; wsize < tvp5150_win_sizes + N_WIN_SIZES; wsize++)
		if (pix->width >= wsize->width && pix->height >= wsize->height)
			break;
	if (wsize >= tvp5150_win_sizes + N_WIN_SIZES)
		wsize--;   /* Take the smallest one */

	/*
	 * Note the size we'll actually handle.
	 */
	pix->width = wsize->width;
	pix->height = wsize->height;
	pix->bytesperline = pix->width*tvp5150_formats[index].bpp;
	pix->sizeimage = pix->height*pix->bytesperline;
	return 0;
}

static int tvp5150_s_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_format *fmt
)
{
	return 0;
}

static int tvp5150_g_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_format *fmt
)
{
	return 0;
}

static int tvp5150_cropcap(
	struct v4l2_subdev *sd,
	struct v4l2_cropcap *cc
)
{
	return 0;
}


static int tvp5150_g_crop(
	struct v4l2_subdev *sd,
	struct v4l2_crop *crop
)
{
	return 0;
}

static int tvp5150_s_crop(
	struct v4l2_subdev *sd,
	struct v4l2_crop *crop
)
{
	return 0;
}

static int tvp5150_g_parm(
	struct v4l2_subdev *sd,
	struct v4l2_streamparm *parms
)
{
	return 0;
}

static int tvp5150_s_parm(
	struct v4l2_subdev *sd,
	struct v4l2_streamparm *param
)
{
	return 0;
}

static int tvp5150_queryctrl(
	struct v4l2_subdev *sd,
	struct v4l2_queryctrl *qc
)
{
	/* Fill in min, max, step and default value for these controls. */
	switch (qc->id) {
		case V4L2_CID_BRIGHTNESS:
			qc->minimum = 0;
			qc->maximum = 255;
			qc->step = 1;
			qc->default_value = 128;
		break;
				
		case V4L2_CID_CONTRAST:
			qc->minimum = 0;
			qc->maximum = 127;
			qc->step = 1;
			qc->default_value = 64;
		break;

		case V4L2_CID_VFLIP:
			qc->minimum = 0;
			qc->maximum = 1;
			qc->step = 1;
			qc->default_value = 0;
		break;
		
		case V4L2_CID_HFLIP:
			qc->minimum = 0;
			qc->maximum = 1;
			qc->step = 1;
			qc->default_value = 0;
		break;
		
		case V4L2_CID_SATURATION:
			qc->minimum = 0;
			qc->maximum = 256;
			qc->step = 1;
			qc->default_value = 128;
		break;
		
		case V4L2_CID_HUE:
			qc->minimum = -180;
			qc->maximum = 180;
			qc->step = 5;
			qc->default_value = 0;
		break;
	}
	return -EINVAL;
}

static int tvp5150_g_ctrl(
	struct v4l2_subdev *sd,
	struct v4l2_control *ctrl
)
{
	switch (ctrl->id) {
		case V4L2_CID_BRIGHTNESS:
		break;
		case V4L2_CID_CONTRAST:
		break;
		case V4L2_CID_VFLIP:
		break;
		case V4L2_CID_HFLIP:
		break;
		case V4L2_CID_SATURATION:
		break;
		case V4L2_CID_HUE:
		break;
	}
	return 0;	
}

static int tvp5150_s_ctrl(
	struct v4l2_subdev *sd,
	struct v4l2_control *ctrl
)
{
	switch (ctrl->id) {
		case V4L2_CID_BRIGHTNESS:
		break;
		case V4L2_CID_CONTRAST:
		break;
		case V4L2_CID_VFLIP:
		break;
		case V4L2_CID_HFLIP:
		break;
		case V4L2_CID_SATURATION:
		break;
		case V4L2_CID_HUE:
		break;
	}
	return 0;	
}

int tvp5150_init(
	struct v4l2_subdev *sd,
	u32 val
)
{
	printk("tvp5150_init\n");
	return tvp5150_write_array(tvp5150_default_regs);
}

int tvp5150_querystd(
	struct v4l2_subdev *sd,
	v4l2_std_id *std
)
{
	int ret = 0;
	unsigned char value;
	ret = tvp5150_read(0x8c, &value);
	if (ret < 0) {
		ret = -EIO;
	}
	else {
		if (value & 0x06) {
			*std = V4L2_STD_PAL;
		}
		else {
			*std = V4L2_STD_NTSC;
		} 
	}
	return ret;
}

int tvp5150_suspend(
	struct v4l2_subdev *sd
)
{
	return tvp5150_write_array(tvp5150_suspend_regs);
}

int tvp5150_resume(
	struct v4l2_subdev *sd
)
{
	return tvp5150_write_array(tvp5150_resume_regs);
}

static int gp_tvp5150_init(void)
{
	int ret;

	v4l2_subdev_init(&(tvp5150_info.sd), &tvp5150_ops);
	
	strcpy(tvp5150_info.sd.name, "tvp5150");
	
	ret = register_sensor(&(tvp5150_info.sd),  (int *)&param[0]);
	if( ret<0 )
		return ret;

	i2c_attr = (struct i2c_bus_attr_t *)gp_i2c_bus_request(0xb8, 300); /*100KHZ*/
/*
	ret = tvp5150_init(&(tvp5150_info.sd), 0);
	if(ret<0)
	{
		DIAG_ERROR("tvp5150 init fail\n");
		return ret;
	}
*/
	return 0;
}

static void gp_tvp5150_exit(void) {
	unregister_sensor(&(tvp5150_info.sd));
	return;
}

module_init(gp_tvp5150_init);
module_exit(gp_tvp5150_exit);

MODULE_LICENSE_GP;
