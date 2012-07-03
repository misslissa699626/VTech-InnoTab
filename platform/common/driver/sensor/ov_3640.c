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
#define VGA_WIDTH		1024
#define VGA_HEIGHT		768
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
int ov3640_init(struct v4l2_subdev *sd,	u32 val);
static int ov3640_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc);
static int ov3640_enum_fmt(struct v4l2_subdev *sd, struct v4l2_fmtdesc *fmt);
static int ov3640_try_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt);
static int ov3640_s_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt);
static int ov3640_g_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt);
static int ov3640_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int ov3640_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int ov3640_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param);
static int ov3640_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param);
static int ov3640_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int ov3640_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int ov3640_cropcap(struct v4l2_subdev *sd, struct v4l2_cropcap *cc);
static int ov3640_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *crop);
static int ov3640_s_crop(struct v4l2_subdev *sd, struct v4l2_crop *crop);
static int ov3640_s_interface(struct v4l2_subdev *sd, struct v4l2_interface *interface);
static int ov3640_suspend(struct v4l2_subdev *sd);
static int ov3640_resume(struct v4l2_subdev *sd);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

struct regval_list {
	unsigned char reg_num_h;
	unsigned char reg_num_l;
	unsigned char value_l;
};

static struct regval_list ov3640_default_regs[] = {
	{0x30, 0x12, 0x90}, // [7]:Reset; [6:4]=001->XGA mode
	{0x30, 0xa9, 0xdb}, // for 1.5V
	{0x30, 0x4d, 0x45},
	{0x30, 0x87, 0x16},
	{0x30, 0x9c, 0x1a},                 
	{0x30, 0xa2, 0xe4},                 
	{0x30, 0xaa, 0x42},                 
	{0x30, 0xb0, 0xff},                 
	{0x30, 0xb1, 0xff},
	{0x30, 0xb2, 0x10},
	{0x30, 0x0e, 0x32},
	{0x30, 0x0f, 0x21},
	{0x30, 0x10, 0x20},
	{0x30, 0x11, 0x01},
	{0x30, 0x4c, 0x82},
	{0x30, 0xd7, 0x10},                 
	{0x30, 0xd9, 0x0d},                 
	{0x30, 0xdb, 0x08},                 
	{0x30, 0x16, 0x82},                 
	{0x30, 0x18, 0x48}, // Luminance High Range=72 after Gamma=0x86=134; 0x40->134
	{0x30, 0x19, 0x40}, // Luminance Low Range=64 after Gamma=0x8f=143; 0x38->125
	{0x30, 0x1a, 0x82},                 
	{0x30, 0x7d, 0x00},                 
	{0x30, 0x87, 0x02},                 
	{0x30, 0x82, 0x20},
	{0x30, 0x70, 0x00}, // 50Hz Banding MSB
	{0x30, 0x71, 0x72}, // 50Hz Banding LSB
	{0x30, 0x72, 0x00}, // 60Hz Banding MSB
	{0x30, 0x73, 0xa6}, // 60Hz Banding LSB
	{0x30, 0x1c, 0x07}, //max_band_step_50hz
	{0x30, 0x1d, 0x08}, //max_band_step_60hz
	{0x30, 0x15, 0x12}, // [6:4]:1 dummy frame; [2:0]:AGC gain 8x
	{0x30, 0x14, 0x84}, //[7]:50hz; [6]:auto banding detection disable; [3]:night modedisable
	{0x30, 0x13, 0xf7}, //AE_en
	{0x30, 0x30, 0x11}, // Avg_win_Weight0
	{0x30, 0x31, 0x11}, // Avg_win_Weight1
	{0x30, 0x32, 0x11}, // Avg_win_Weight2
	{0x30, 0x33, 0x11}, // Avg_win_Weight3
	{0x30, 0x34, 0x11}, // Avg_win_Weight4
	{0x30, 0x35, 0x11}, // Avg_win_Weight5
	{0x30, 0x36, 0x11}, // Avg_win_Weight6
	{0x30, 0x37, 0x11}, // Avg_win_Weight7
	{0x30, 0x38, 0x01}, // Avg_Win_Hstart=285
	{0x30, 0x39, 0x1d}, // Avg_Win_Hstart=285
	{0x30, 0x3a, 0x00}, // Avg_Win_Vstart=10
	{0x30, 0x3b, 0x0a}, // Avg_Win_Vstart=10
	{0x30, 0x3c, 0x02}, // Avg_Win_Width=512x4=2048
	{0x30, 0x3d, 0x00}, // Avg_Win_Width=512x4=2048
	{0x30, 0x3e, 0x01}, // Avg_Win_Height=384x4=1536
	{0x30, 0x3f, 0x80}, // Avg_Win_Height=384x4=1536
	{0x30, 0x47, 0x00}, // [7]:avg_based AE
	{0x30, 0xb8, 0x20},
	{0x30, 0xb9, 0x17},
	{0x30, 0xba, 0x04},
	{0x30, 0xbb, 0x08},
	{0x30, 0xa9, 0xdb}, // for 1.5V
	{0x31, 0x04, 0x02},                 
	{0x31, 0x05, 0xfd},                 
	{0x31, 0x06, 0x00},                 
	{0x31, 0x07, 0xff},                 
	{0x31, 0x00, 0x02},
	{0x33, 0x00, 0x13}, // [0]: LENC disable; [1]: AF enable
	{0x33, 0x01, 0xde}, // [1]: BC_en; [2]: WC_en; [4]: CMX_en
	{0x33, 0x02, 0xcf}, //[0]: AWB_en; [1]: AWB_gain_en; [2]: Gamma_en; [7]: Special_Effect_en
	{0x33, 0x04, 0xfc}, // [4]: Add bias to gamma result; [5]: Enable Gamma bias function
	{0x33, 0x06, 0x5c}, // Reserved ???
	{0x33, 0x07, 0x11}, // Reserved ???
	{0x33, 0x08, 0x00}, // [7]: AWB_mode=advanced
	{0x33, 0x0b, 0x1c}, // Reserved ???
	{0x33, 0x0c, 0x18}, // Reserved ???
	{0x33, 0x0d, 0x18}, // Reserved ???
	{0x33, 0x0e, 0x56}, // Reserved ???
	{0x33, 0x0f, 0x5c}, // Reserved ???
	{0x33, 0x10, 0xd0}, // Reserved ???
	{0x33, 0x11, 0xbd}, // Reserved ???
	{0x33, 0x12, 0x26}, // Reserved ???
	{0x33, 0x13, 0x2b}, // Reserved ???
	{0x33, 0x14, 0x42}, // Reserved ???
	{0x33, 0x15, 0x42}, // Reserved ???
	{0x33, 0x1b, 0x09}, // Gamma YST1
	{0x33, 0x1c, 0x18}, // Gamma YST2
	{0x33, 0x1d, 0x30}, // Gamma YST3
	{0x33, 0x1e, 0x58}, // Gamma YST4
	{0x33, 0x1f, 0x66}, // Gamma YST5
	{0x33, 0x20, 0x72}, // Gamma YST6
	{0x33, 0x21, 0x7d}, // Gamma YST7
	{0x33, 0x22, 0x86}, // Gamma YST8
	{0x33, 0x23, 0x8f}, // Gamma YST9
	{0x33, 0x24, 0x97}, // Gamma YST10
	{0x33, 0x25, 0xa5}, // Gamma YST11
	{0x33, 0x26, 0xb2}, // Gamma YST12
	{0x33, 0x27, 0xc7}, // Gamma YST13
	{0x33, 0x28, 0xd8}, // Gamma YST14
	{0x33, 0x29, 0xe8}, // Gamma YST15
	{0x33, 0x2a, 0x20}, // Gamma YSLP15
	{0x33, 0x2b, 0x00}, // [3]: WB_mode=auto
	{0x33, 0x2d, 0x64}, // [6]:de-noise auto mode; [5]:edge auto mode; [4:0]:edge threshold
	{0x33, 0x55, 0x06}, // Special_Effect_CTRL: [1]:Sat_en; [2]: Cont_Y_en
	{0x33, 0x58, 0x40}, // Special_Effect_Sat_U
	{0x33, 0x59, 0x40}, // Special_Effect_Sat_V
	{0x33, 0x6a, 0x52}, // LENC R_A1
	{0x33, 0x70, 0x46}, // LENC G_A1
	{0x33, 0x76, 0x38}, // LENC B_A1
	{0x34, 0x00, 0x00}, // [2:0];Format input source=DSP TUV444
	{0x34, 0x03, 0x42}, // DVP Win Addr
	{0x34, 0x04, 0x00}, // [5:0]: yuyv
	{0x35, 0x07, 0x06}, // ???
	{0x35, 0x0a, 0x4f}, // ???
	{0x36, 0x00, 0xc0}, // VSYNC_CTRL
	{0x33, 0x02, 0xcf}, //[0]: AWB_enable
	{0x30, 0x0d, 0x01}, // PCLK/2
	{0x30, 0x12, 0x10}, // [6:4]=001->XGA mode
	{0x30, 0x13, 0xf7}, //AE_enable
	{0x30, 0x20, 0x01}, // HS=285
	{0x30, 0x21, 0x1d}, // HS=285
	{0x30, 0x22, 0x00}, // VS = 6
	{0x30, 0x23, 0x06}, // VS = 6
	{0x30, 0x24, 0x08}, // HW=2072
	{0x30, 0x25, 0x18}, // HW=2072
	{0x30, 0x26, 0x03}, // VW=772
	{0x30, 0x27, 0x04}, // VW=772
	{0x30, 0x28, 0x09}, //HTotalSize=2375
	{0x30, 0x29, 0x47}, //HTotalSize=2375
	{0x30, 0x2a, 0x03}, //VTotalSize=784
	{0x30, 0x2b, 0x10}, //VTotalSize=784
	{0x30, 0x4c, 0x82},
	{0x30, 0x75, 0x24}, // VSYNCOPT
	{0x30, 0x86, 0x00}, // Sleep/Wakeup
	{0x30, 0x88, 0x04}, //x_output_size=1024
	{0x30, 0x89, 0x00}, //x_output_size=1024
	{0x30, 0x8a, 0x03}, //y_output_size=768
	{0x30, 0x8b, 0x00}, //y_output_size=768
	{0x30, 0x8d, 0x04},
	{0x30, 0xd7, 0x90}, //???
	{0x33, 0x02, 0xef}, // [5]: Scale_en, [0]: AWB_enable
	{0x33, 0x5f, 0x34}, // Scale_VH_in
	{0x33, 0x60, 0x0c}, // Scale_H_in = 0x40c = 1036
	{0x33, 0x61, 0x04}, // Scale_V_in = 0x304 = 772
	{0x33, 0x62, 0x34}, // Scale_VH_out
	{0x33, 0x63, 0x08}, // Scale_H_out = 0x408 = 1032
	{0x33, 0x64, 0x04}, // Scale_V_out = 0x304 = 772
	{0x30, 0x0e, 0x32},
	{0x30, 0x0f, 0x21},
	{0x30, 0x11, 0x00}, // for 30 FPS
	{0x30, 0x4c, 0x82},

	{0xff, 0xff, 0xff}	//end
};

static struct regval_list ov3640_fmt_yuv422[] = {
	{0xff, 0xff, 0xff}	//end
};

static struct regval_list ov3640_fmt_rgb565[] = {
	{0xff, 0xff, 0xff}	//end
};

static struct regval_list ov3640_resume_regs[] = {
	{0xff, 0xff, 0xff}	//end
};

static struct regval_list ov3640_suspend_regs[] = {
	{0xff, 0xff, 0xff}	//end
};

static const struct v4l2_subdev_core_ops ov3640_core_ops = {
//	.g_chip_ident = ov3640_g_chip_ident,
	.g_ctrl = ov3640_g_ctrl,
	.s_ctrl = ov3640_s_ctrl,
	.queryctrl = ov3640_queryctrl,
//	.reset = ov3640_reset,
	.init = ov3640_init,
};

static const struct v4l2_subdev_video_ops ov3640_video_ops = {
	.enum_fmt = ov3640_enum_fmt,
	.try_fmt = ov3640_try_fmt,
	.s_fmt = ov3640_s_fmt,
	.g_fmt = ov3640_g_fmt,
	.s_parm = ov3640_s_parm,
	.g_parm = ov3640_g_parm,
	.cropcap = ov3640_cropcap,
	.g_crop = ov3640_g_crop,
	.s_crop = ov3640_s_crop,	
};

static const struct v4l2_subdev_ext_ops ov3640_ext_ops = {
	.s_interface = ov3640_s_interface,
	.suspend = ov3640_suspend,
	.resume = ov3640_resume,
};

static const struct v4l2_subdev_ops ov3640_ops = {
	.core = &ov3640_core_ops,
	.video = &ov3640_video_ops,
	.ext = &ov3640_ext_ops
};

static struct ov3640_format_struct {
	__u8 *desc;
	__u32 pixelformat;
	struct regval_list *regs;
	int cmatrix[CMATRIX_LEN];
	int bpp;   /* Bytes per pixel */
} ov3640_formats[] = {
	{
		.desc		= "YUYV 4:2:2",
		.pixelformat= V4L2_PIX_FMT_YUYV,
		.regs		= ov3640_fmt_yuv422,
		.cmatrix	= { 128, -128, 0, -34, -94, 128 },
		.bpp		= 2,
	},
	{
		.desc		= "RGB 5-6-5",
		.pixelformat= V4L2_PIX_FMT_RGB565,
		.regs		= ov3640_fmt_rgb565,
		.cmatrix	= { 128, -128, 0, -34, -94, 128 },
		.bpp		= 2,
	},
};
#define N_ov3640_FMTS ARRAY_SIZE(ov3640_formats)

static struct ov3640_win_size {
	int	width;
	int	height;
	unsigned char com7_bit;
	int	hstart;		/* Start/stop values for the camera.  Note */
	int	hstop;		/* that they do not always make complete */
	int	vstart;		/* sense to humans, but evidently the sensor */
	int	vstop;		/* will do the right thing... */
	struct regval_list *regs; /* Regs to tweak */
/* h/vref stuff */
} ov3640_win_sizes[] = {
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
#define N_WIN_SIZES (ARRAY_SIZE(ov3640_win_sizes))

struct ov3640_format_struct;  /* coming later */
typedef struct ov3640_info_t {
	struct v4l2_subdev sd;
	struct ov3640_format_struct *fmt;  /* Current format */
	unsigned char sat;		/* Saturation value */
	int hue;			/* Hue value */
}ov3640_info_s;

static ov3640_info_s ov3640_info;
struct i2c_bus_attr_t *i2c_attr;


/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/

static int ov3640_read(
	unsigned char reg_num_h,
	unsigned char reg_num_l,
	unsigned char *value
)
{
	char data[2];
	int ret;

	data[0]=reg_num_h;
	data[1]=reg_num_l;
	ret = gp_i2c_bus_write((int)i2c_attr, data, 2);
	ret = gp_i2c_bus_read((int)i2c_attr, data, 1);
	*value = data[0];
	return ret;
}

static int ov3640_write(
	unsigned char reg_num_h,
	unsigned char reg_num_l,
	unsigned char value_l
)
{	
//	i2c_handlep -> slaveAddr = 0x60;
//	i2c_handlep -> regAddr = reg;	
//	i2c_handlep -> clkRate = 0;
	char data[3];
	
	data[0]=reg_num_h;
	data[1]=reg_num_l;
	data[2]=value_l;
	return gp_i2c_bus_write((int)i2c_attr, data, 3);
}

static int ov3640_write_array(struct regval_list *vals)
{
	int ret;
	int k;
	k = 0;

	while (vals->reg_num_h != 0xff||vals->reg_num_l != 0xff|| vals->value_l != 0xff) {
		printk("reg=0x%02x%02x, value=0x%02x\n", vals->reg_num_h,vals->reg_num_l,vals->value_l);
		ret = ov3640_write(vals->reg_num_h,vals->reg_num_l,vals->value_l);
		if (ret < 0)
			return ret;
		vals++;
	}
	return 0;
 }

static int ov3640_s_interface(
	struct v4l2_subdev *sd,
	struct v4l2_interface *interface
)
{
	return 0;
}

static int ov3640_enum_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_fmtdesc *fmt
)
{
	struct ov3640_format_struct *ofmt;

	if (fmt->index >= N_ov3640_FMTS)
		return -EINVAL;

	ofmt = ov3640_formats + fmt->index;
	fmt->flags = 0;
	strcpy(fmt->description, ofmt->desc);
	fmt->pixelformat = ofmt->pixelformat;
	
	return 0;
}

static int ov3640_try_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_format *fmt
)
{
	int index;
	struct ov3640_win_size *wsize;
	struct v4l2_pix_format *pix = &fmt->fmt.pix;
	
	for( index=0; index<N_ov3640_FMTS; index++ )
		if (ov3640_formats[index].pixelformat == pix->pixelformat)
			break;
	if (index >= N_ov3640_FMTS) {
		/* default to first format */
		index = 0;
		pix->pixelformat = ov3640_formats[0].pixelformat;
		printk(KERN_NOTICE "No match format\n");
	}
	
	pix->field = V4L2_FIELD_NONE;
	
	for (wsize = ov3640_win_sizes; wsize < ov3640_win_sizes + N_WIN_SIZES; wsize++)
		if (pix->width >= wsize->width && pix->height >= wsize->height)
			break;
	if (wsize >= ov3640_win_sizes + N_WIN_SIZES)
		wsize--;   /* Take the smallest one */

	/*
	 * Note the size we'll actually handle.
	 */
	pix->width = wsize->width;
	pix->height = wsize->height;
	pix->bytesperline = pix->width*ov3640_formats[index].bpp;
	pix->sizeimage = pix->height*pix->bytesperline;
	return 0;
}

static int ov3640_s_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_format *fmt
)
{
	return 0;
}

static int ov3640_g_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_format *fmt
)
{
	return 0;
}

static int ov3640_cropcap(
	struct v4l2_subdev *sd,
	struct v4l2_cropcap *cc
)
{
	return 0;
}


static int ov3640_g_crop(
	struct v4l2_subdev *sd,
	struct v4l2_crop *crop
)
{
	return 0;
}

static int ov3640_s_crop(
	struct v4l2_subdev *sd,
	struct v4l2_crop *crop
)
{
	return 0;
}

static int ov3640_g_parm(
	struct v4l2_subdev *sd,
	struct v4l2_streamparm *parms
)
{
	return 0;
}

static int ov3640_s_parm(
	struct v4l2_subdev *sd,
	struct v4l2_streamparm *param
)
{
	return 0;
}

static int ov3640_queryctrl(
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

static int ov3640_g_ctrl(
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

static int ov3640_s_ctrl(
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
		nRet = ov3640_read(0x30, 0x7c, (unsigned char *)&data);
		if(ctrl->value)
			nRet = ov3640_write(0x30, 0x7c, data |= 0x01);
		else
			nRet = ov3640_write(0x30, 0x7c, data &= ~0x01);
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

int ov3640_init(
	struct v4l2_subdev *sd,
	u32 val
)
{
	return ov3640_write_array(ov3640_default_regs);
}

int ov3640_suspend(
	struct v4l2_subdev *sd
)
{
	return ov3640_write_array(ov3640_suspend_regs);
}

int ov3640_resume(
	struct v4l2_subdev *sd
)
{
	return ov3640_write_array(ov3640_resume_regs);
}

static int gp_ov3640_init(void)
{
	int ret;

	v4l2_subdev_init(&(ov3640_info.sd), &ov3640_ops);
	
	strcpy(ov3640_info.sd.name, "ov3640");
	
	ret = register_sensor(&(ov3640_info.sd), (int *)&param[0]);
	if( ret<0 )
		return ret;

	i2c_attr = (struct i2c_bus_attr_t *)gp_i2c_bus_request(0x78, 20);
	return 0;
}

static void gp_ov3640_exit(void) {
	unregister_sensor(&(ov3640_info.sd));
	return;
}

module_init(gp_ov3640_init);
module_exit(gp_ov3640_exit);

MODULE_LICENSE_GP;
