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
#include <linux/slab.h>
#include <media/v4l2-device.h>
#include <mach/module.h>
#include <mach/sensor_mgr.h>
#include <mach/gp_i2c_bus.h>
#include <mach/diag.h>


/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define CMATRIX_LEN		6
#define	SXGA_WIDTH		1280
#define	SXGA_HEIGHT		1024
#define VGA_WIDTH			640
#define VGA_HEIGHT		480
#define QVGA_WIDTH		320
#define QVGA_HEIGHT		240

#define COM7_FMT_VGA	0x00

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
static char *param[] = {"0", "PORT0", "0", "NONE", "0", "NONE"};
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
int ov9655_init(struct v4l2_subdev *sd,	u32 val);
static int ov9655_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc);
static int ov9655_enum_fmt(struct v4l2_subdev *sd, struct v4l2_fmtdesc *fmt);
static int ov9655_try_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt);
static int ov9655_s_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt);
static int ov9655_g_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt);
static int ov9655_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int ov9655_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int ov9655_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param);
static int ov9655_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param);
static int ov9655_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int ov9655_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int ov9655_cropcap(struct v4l2_subdev *sd, struct v4l2_cropcap *cc);
static int ov9655_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *crop);
static int ov9655_s_crop(struct v4l2_subdev *sd, struct v4l2_crop *crop);
static int ov9655_s_interface(struct v4l2_subdev *sd, struct v4l2_interface *interface);
static int ov9655_suspend(struct v4l2_subdev *sd);
static int ov9655_resume(struct v4l2_subdev *sd);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

struct regval_list {
	unsigned char reg_num;
	unsigned char value;
};

static struct regval_list ov9655_default_regs[] = {
	{ 0x12, 0x80 },
	{ 0x00, 0x00 },
//	{ 0x00, 0x1a },
	{ 0x01, 0x80 },
	{ 0x02, 0x80 },
	{ 0x03, 0x00 }, //0x12,
	{ 0x04, 0x03 },
	{ 0x0b, 0x57 },
	{ 0x0e, 0x61 },
	{ 0x0f, 0x43 }, //0x42,
    
	{ 0x11, 0x00 }, //0x80,
	{ 0x12, 0x62 },
	{ 0x13, 0xe7 },
	{ 0x14, 0x2e }, //0x3e,
	{ 0x15, 0x04 },
	{ 0x16, 0x14 }, //0x24, //0x04,
	{ 0x17, 0x16 },
	{ 0x18, 0x02 },
	{ 0x19, 0x01 },
	{ 0x1a, 0x3d },
	{ 0x1e, 0x34 },
//	{ 0x1e, 0x04 },
    
	{ 0x24, 0x43 }, //0x3c,
	{ 0x25, 0x33 },
	{ 0x26, 0x92 }, //0x82, //0xf2, //0x72,
	{ 0x27, 0x08 },
	{ 0x28, 0x08 },
	{ 0x29, 0x00 },
	{ 0x2a, 0x00 }, //dummy pixel number=0x2a[7:4]x256+0x2b[7:0]
	{ 0x2b, 0x19 }, //dummy pixel number=0x2a[7:4]x256+0x2b[7:0]
	{ 0x2c, 0x08 },
    
	{ 0x32, 0x80 }, //0xff,
	{ 0x33, 0x00 },
	{ 0x34, 0x3F },
	{ 0x35, 0x00 },
	{ 0x36, 0xfa },
	{ 0x37, 0x08 },
	{ 0x38, 0x72 },
	{ 0x39, 0x57 },
	{ 0x3a, 0x80 }, //0x82, //0x80,
//	{ 0x3b, 0xa4 }, //0x44,
	{ 0x3b, 0x44 },
	{ 0x3d, 0x99 },
	{ 0x3e, 0x0c },
	{ 0x3f, 0xc1 },
//	{ 0x3f, 0x84 },
    
	{ 0x40, 0xc0 },
	{ 0x41, 0x00 },
	{ 0x42, 0xd1 }, //0xc0,
	{ 0x43, 0x0a },
	{ 0x44, 0xf0 },
	{ 0x45, 0x46 },
	{ 0x46, 0x62 },
	{ 0x47, 0x2a },
	{ 0x48, 0x3c },
	{ 0x4a, 0xfc },
	{ 0x4b, 0xfc },
	{ 0x4c, 0x7f },
	{ 0x4d, 0x7f },
	{ 0x4e, 0x7f },
	             
	// 0.8x saturation
	{ 0x4f, 0x79 },
	{ 0x50, 0x79 },
	{ 0x51, 0x00 },
	{ 0x52, 0x20 },
	{ 0x54, 0x8c },
	{ 0x58, 0x1a },

	{ 0x59, 0x85 },
	{ 0x5a, 0xa9 },
	{ 0x5b, 0x64 },
	{ 0x5c, 0x84 },
	{ 0x5d, 0x53 },
	{ 0x5e, 0x0e },
	{ 0x5f, 0xf0 },

	{ 0x60, 0xf0 },
	{ 0x61, 0xf0 },
	{ 0x62, 0x00 },
	{ 0x63, 0x00 },
	{ 0x64, 0x04 },
	{ 0x65, 0x20 },
	{ 0x66, 0x00 },
	{ 0x69, 0x0a },
	{ 0x6a, 0x02 }, // 50Hz Banding Max AEC Step
	{ 0x6b, 0x4a },
	{ 0x6c, 0x04 },
	{ 0x6d, 0x55 },
	{ 0x6e, 0x00 },
	{ 0x6f, 0x9d },

	{ 0x70, 0x10 }, //0x21,
//	{ 0x70, 0x09 },
	{ 0x71, 0x78 },
	{ 0x72, 0xcc }, //0x00,
	{ 0x73, 0x00 },
	{ 0x74, 0x3a },
	{ 0x75, 0x35 },
	{ 0x76, 0x01 },
	{ 0x77, 0x03 }, //0x02,

	//OV 2
	{ 0x7a, 0x20 },
	{ 0x7b, 0x09 },
	{ 0x7c, 0x18 },
	{ 0x7d, 0x30 },
	{ 0x7e, 0x58 },
	{ 0x7f, 0x66 },
	
	{ 0x80, 0x72 },
	{ 0x81, 0x7d },
	{ 0x82, 0x86 },
	{ 0x83, 0x8f },
	{ 0x84, 0x97 },
	{ 0x85, 0xa5 },
	{ 0x86, 0xb2 },
	{ 0x87, 0xc7 },
	{ 0x88, 0xd8 },
	{ 0x89, 0xe8 },
	
	{ 0x8a, 0x45 }, //0x03,
	{ 0x8c, 0x8d },
	
	{ 0x90, 0x7d },
	{ 0x91, 0x7b },
	{ 0x9d, 0x02 },
	{ 0x9e, 0x02 },
	{ 0x9f, 0x7a },
	
	{ 0xa0, 0x79 },
	{ 0xa1, 0x40 },
	{ 0xa2, 0x96 }, //50Hz Fliker for 30FPS
	{ 0xa3, 0x7d }, //60Hz Fliker for 30FPS
	{ 0xa4, 0x50 },
	{ 0xa5, 0x68 },
	{ 0xa6, 0x4a },
//	{ 0xa6, 0x40 },
	{ 0xa8, 0xc1 },
	{ 0xa9, 0xef },
	{ 0xaa, 0x92 },
	{ 0xab, 0x04 },
	{ 0xac, 0x80 },
	{ 0xad, 0x80 },
	{ 0xae, 0x80 },
	{ 0xaf, 0x80 },
	
	{ 0xb2, 0xf2 },
	{ 0xb3, 0x20 },
	{ 0xb4, 0x03 }, //0x00,
	{ 0xb5, 0x00 },
	{ 0xb6, 0xaf },
	{ 0xbb, 0xae },
	{ 0xbc, 0x7f },
	{ 0xbd, 0x7f },
	{ 0xbe, 0x7f },
	{ 0xbf, 0x7f },

	{ 0xc0, 0xaa },
	{ 0xc1, 0xc0 },
	{ 0xc2, 0x01 },
	{ 0xc3, 0x4e },
	{ 0xc5, 0x02 }, // 60Hz Banding Max AEC Step
	{ 0xc6, 0x05 },
	{ 0xc7, 0x80 },
	{ 0xc9, 0xe0 },
	{ 0xca, 0xe8 },
	{ 0xcb, 0xf0 },
	{ 0xcc, 0xd8 },
	{ 0xcd, 0x93 },

	{ 0x03, 0x00 }, //0x12,
	{ 0x12, 0x62 },
//	{ 0x12, 0x02 },
	{ 0x17, 0x17 },
	{ 0x18, 0x03 },
	{ 0x1a, 0x3d },
	{ 0x2a, 0x00 }, //dummy pixel number=0x2a[7:4]x256+0x2b[7:0]
	{ 0x2b, 0x19 }, //dummy pixel number=0x2a[7:4]x256+0x2b[7:0]
	{ 0x32, 0x80 }, //0x92, //0xa4,
	{ 0x34, 0x3F },
	{ 0x36, 0xfa },
	{ 0x65, 0x00 }, //0x20,
	{ 0x66, 0x01 }, //0x00,
	{ 0x69, 0x0a },
	{ 0x6a, 0x02 }, // 50Hz Banding Max AEC Step
	{ 0x73, 0x00 },
	{ 0x8c, 0x89 },
	{ 0x9d, 0x03 }, //0x02,
	{ 0x9e, 0x04 }, //0x02,
	{ 0xa2, 0x96 }, //50Hz Fliker for 30FPS
	{ 0xa3, 0x7d }, //60Hz Fliker for 30FPS
	{ 0xc5, 0x02 }, // 60Hz Banding Max AEC Step
	{ 0xc0, 0xaa },
	{ 0x14, 0x2e }, //0x3e,
	{ 0x13, 0xe7 },
	
	{ 0xa1, 0x40 },
	{ 0x10, 0x3e },
//	{ 0x10, 0x70 },
	{ 0x04, 0x02 },
	{ 0x2e, 0x00 },
	{ 0x2d, 0x00 },
	{ 0x01, 0x80 },
//	{ 0x01, 0x78 },
	{ 0xa6, 0x80 },
	{ 0x02, 0x80 },
//	{ 0x02, 0x40 },
	{ 0x05, 0x36 },
	{ 0x06, 0x3b },
	{ 0x07, 0x38 },
	{ 0x08, 0x3f },
	{ 0x09, 0x03 },
	{ 0x2f, 0x3b },

	{ 0xff, 0xff }	//end
};

static struct regval_list ov9655_fmt_yuv422[] = {
	{ 0xff, 0xff }
};

static struct regval_list ov9655_fmt_rgb565[] = {
	{ 0xff, 0xff }
};

static struct regval_list ov9655_resume_regs[] = {
	{ 0xc1, 0x00 },
	{ 0x49, 0x48 },
	{ 0x39, 0x57 },
	{ 0x09, 0x03 },
	{ 0xc1, 0xc0 },
	{ 0xff, 0xff }
};

static struct regval_list ov9655_suspend_regs[] = {
	{ 0x13, 0xe0 },
	{ 0x6b, 0x4a },
	{ 0x39, 0x5f },
	{ 0x38, 0x72 },
	{ 0x10, 0x00 },
	{ 0xa1, 0x00 },
	{ 0x04, 0x00 },
	{ 0x00, 0x00 },
	{ 0xc1, 0xc0 },
	{ 0x49, 0x08 },
	{ 0x09, 0x11 },
	{ 0x09, 0x01 },
	{ 0x09, 0x11 },
	{ 0xff, 0xff }
};

static const struct v4l2_subdev_core_ops ov9655_core_ops = {
//	.g_chip_ident = ov9655_g_chip_ident,
	.g_ctrl = ov9655_g_ctrl,
	.s_ctrl = ov9655_s_ctrl,
	.queryctrl = ov9655_queryctrl,
//	.reset = ov9655_reset,
	.init = ov9655_init,
};

static const struct v4l2_subdev_video_ops ov9655_video_ops = {
	.enum_fmt = ov9655_enum_fmt,
	.try_fmt = ov9655_try_fmt,
	.s_fmt = ov9655_s_fmt,
	.g_fmt = ov9655_g_fmt,
	.s_parm = ov9655_s_parm,
	.g_parm = ov9655_g_parm,
	.cropcap = ov9655_cropcap,
	.g_crop = ov9655_g_crop,
	.s_crop = ov9655_s_crop,	
};

static const struct v4l2_subdev_ext_ops ov9655_ext_ops = {
	.s_interface = ov9655_s_interface,
	.suspend = ov9655_suspend,
	.resume = ov9655_resume,
};

static const struct v4l2_subdev_ops ov9655_ops = {
	.core = &ov9655_core_ops,
	.video = &ov9655_video_ops,
	.ext = &ov9655_ext_ops
};

static struct ov9655_format_struct {
	__u8 *desc;
	__u32 pixelformat;
	struct regval_list *regs;
	int cmatrix[CMATRIX_LEN];
	int bpp;   /* Bytes per pixel */
} ov9655_formats[] = {
	{
		.desc		= "YUYV 4:2:2",
		.pixelformat= V4L2_PIX_FMT_YUYV,
		.regs		= ov9655_fmt_yuv422,
		.cmatrix	= { 128, -128, 0, -34, -94, 128 },
		.bpp		= 2,
	},
	{
		.desc		= "RGB 5:6:5",
		.pixelformat= V4L2_PIX_FMT_RGB565,
		.regs		= ov9655_fmt_rgb565,
		.cmatrix	= { 128, -128, 0, -34, -94, 128 },
		.bpp		= 2,
	},
};
#define N_OV9655_FMTS ARRAY_SIZE(ov9655_formats)

static struct ov9655_win_size {
	int	width;
	int	height;
	unsigned char com7_bit;
	int	hstart;		/* Start/stop values for the camera.  Note */
	int	hstop;		/* that they do not always make complete */
	int	vstart;		/* sense to humans, but evidently the sensor */
	int	vstop;		/* will do the right thing... */
	struct regval_list *regs; /* Regs to tweak */
/* h/vref stuff */
} ov9655_win_sizes[] = {		//the size sequence is the bigest to smallest
	/* SXGA */
	{
		.width		= SXGA_WIDTH,
		.height		= SXGA_HEIGHT,
		.com7_bit	= COM7_FMT_VGA,
		.hstart		= 158,		/* These values from */
		.hstop		=  14,		/* Omnivision */
		.vstart		=  10,
		.vstop		= 490,
		.regs 		= NULL,		
	},

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
#define N_WIN_SIZES (ARRAY_SIZE(ov9655_win_sizes))

struct ov9655_format_struct;  /* coming later */
typedef struct ov9655_info_t {
	struct v4l2_subdev sd;
	struct ov9655_format_struct *fmt;  /* Current format */
	struct ov9655_win_size *winsize;
	unsigned char sat;		/* Saturation value */
	int hue;			/* Hue value */
} ov9655_info_s;

static ov9655_info_s *ov9655_info = NULL;
struct i2c_bus_attr_t *i2c_attr = NULL;


/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/

static int ov9655_read(
	unsigned char reg,
	unsigned char *value
)
{
	char data[1];
	int ret;

	data[0]=reg;
	ret = gp_i2c_bus_write((int)i2c_attr, data, 1);
	ret = gp_i2c_bus_read((int)i2c_attr, data, 1);
	*value = data[0];
	return ret;
}

static int ov9655_write(
	unsigned char reg,
	unsigned char value
)
{	
	char data[2];
	
	data[0]=reg;
	data[1]=value;
	
	return gp_i2c_bus_write((int)i2c_attr, data, 2);
}

static int ov9655_write_array(struct regval_list *vals)
{
	int ret;
	int i;
	int k;
	k = 0;

	while (vals->reg_num != 0xff || vals->value != 0xff) {
		printk("printk ov9655 write address:\n");
		ret = ov9655_write(vals->reg_num, vals->value);
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

static int ov9655_s_interface(
	struct v4l2_subdev *sd,
	struct v4l2_interface *interface
)
{
	return 0;
}

static int ov9655_enum_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_fmtdesc *fmt
)
{
	struct ov9655_format_struct *ofmt;

	if (fmt->index >= N_OV9655_FMTS)
		return -EINVAL;

	ofmt = ov9655_formats + fmt->index;
	fmt->flags = 0;
	strcpy(fmt->description, ofmt->desc);
	fmt->pixelformat = ofmt->pixelformat;
	
	return 0;
}

static int ov9655_try_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_format *fmt
)
{
	int index;
	struct ov9655_win_size *wsize;
	struct v4l2_pix_format *pix = &fmt->fmt.pix;
	
	for( index=0; index<N_OV9655_FMTS; index++ )
		if (ov9655_formats[index].pixelformat == pix->pixelformat)
			break;
	if (index >= N_OV9655_FMTS) {
		/* default to first format */
		index = 0;
		pix->pixelformat = ov9655_formats[0].pixelformat;
		printk(KERN_NOTICE "No match format\n");
	}
	
	pix->field = V4L2_FIELD_NONE;
	
	for (wsize = ov9655_win_sizes; wsize < ov9655_win_sizes + N_WIN_SIZES; wsize++)
		if (pix->width >= wsize->width && pix->height >= wsize->height)
			break;
	if (wsize >= ov9655_win_sizes + N_WIN_SIZES)
		wsize--;   /* Take the smallest one */

	/*
	 * Note the size we'll actually handle.
	 */
	pix->width = wsize->width;
	pix->height = wsize->height;
	pix->bytesperline = pix->width*ov9655_formats[index].bpp;
	pix->sizeimage = pix->height*pix->bytesperline;
	return 0;
}

static int ov9655_s_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_format *fmt
)
{
	int index;
//	struct ov9655_win_size *wsize;
	struct v4l2_pix_format *pix = &fmt->fmt.pix;
	
	for( index=0; index<N_OV9655_FMTS; index++ )
		if (ov9655_formats[index].pixelformat == pix->pixelformat)
			break;
	if (index >= N_OV9655_FMTS) {
		/* default to first format */
		printk(KERN_NOTICE "No match format\n");
		return -EINVAL;
	}	
	
	return 0;
}

static int ov9655_g_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_format *fmt
)
{
	copy_to_user((struct v4l2_format*)fmt, &(ov9655_info->fmt), sizeof(struct v4l2_format));
	return 0;
}

static int ov9655_cropcap(
	struct v4l2_subdev *sd,
	struct v4l2_cropcap *cc
)
{
	return 0;
}


static int ov9655_g_crop(
	struct v4l2_subdev *sd,
	struct v4l2_crop *crop
)
{
	return 0;
}

static int ov9655_s_crop(
	struct v4l2_subdev *sd,
	struct v4l2_crop *crop
)
{
	return 0;
}

static int ov9655_g_parm(
	struct v4l2_subdev *sd,
	struct v4l2_streamparm *parms
)
{
	return 0;
}

static int ov9655_s_parm(
	struct v4l2_subdev *sd,
	struct v4l2_streamparm *param
)
{
	return 0;
}

static int ov9655_queryctrl(
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

static int ov9655_g_ctrl(
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

static int ov9655_s_ctrl(
	struct v4l2_subdev *sd,
	struct v4l2_control *ctrl
)
{
	unsigned char data;
	int nRet = 0;
	
	switch (ctrl->id) {
		case V4L2_CID_BRIGHTNESS:
		break;
		case V4L2_CID_CONTRAST:
		break;
		case V4L2_CID_VFLIP:
		nRet = ov9655_read(0x1E, &data);
		if(ctrl->value)
			nRet = ov9655_write(0x1E, data |= 0x10);
		else
			nRet = ov9655_write(0x1E, data &= ~0x10);
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

int ov9655_init(
	struct v4l2_subdev *sd,
	u32 val
)
{
	return ov9655_write_array(ov9655_default_regs);
}

int ov9655_suspend(
	struct v4l2_subdev *sd
)
{
	return ov9655_write_array(ov9655_suspend_regs);
}
EXPORT_SYMBOL(ov9655_suspend);

int ov9655_resume(
	struct v4l2_subdev *sd
)
{
	return ov9655_write_array(ov9655_resume_regs);
}

static int gp_ov9655_init(void)
{
	int ret = 0;
	
	ov9655_info = kmalloc(sizeof(ov9655_info_s), GFP_KERNEL);
	if(ov9655_info==NULL)
		goto fail_kmalloc;

	v4l2_subdev_init(&(ov9655_info->sd), &ov9655_ops);
	
	/*init ov9655 information*/
	strcpy(ov9655_info->sd.name, "ov9655");
	ov9655_info->fmt = &ov9655_formats[0];
	ov9655_info->winsize = &ov9655_win_sizes[0];
	
	i2c_attr = (struct i2c_bus_attr_t *)gp_i2c_bus_request(0x60, 20);
	if(i2c_attr==NULL)
		goto fail_i2c;
		
	ret = register_sensor(&(ov9655_info->sd), (int *)&param[0]);
	if( ret<0 )
		goto fail_register;

	return 0;

fail_register:
	i2c_attr=NULL;
fail_i2c:
	kfree(ov9655_info);
fail_kmalloc:
	ov9655_info=NULL;
	DIAG_ERROR("insmod ov9655 fail\n");
	return ret;
}

static void gp_ov9655_exit(void) {
	unregister_sensor(&(ov9655_info->sd));
	kfree(ov9655_info);
	ov9655_info=NULL;
	i2c_attr=NULL;
	return;
}

module_init(gp_ov9655_init);
module_exit(gp_ov9655_exit);

MODULE_LICENSE_GP;
