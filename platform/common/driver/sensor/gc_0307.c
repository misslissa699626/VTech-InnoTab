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
#include <linux/delay.h> 	/* udelay/mdelay */
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
#define VGA_WIDTH		640
#define VGA_HEIGHT		480
#define QVGA_WIDTH		320
#define QVGA_HEIGHT		240

#define COM7_FMT_VGA	0x00

#define SENSOR_I2C_SLAVE_ADDR 0x42

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
int GC0307_init(struct v4l2_subdev *sd,	u32 val);
static int GC0307_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc);
static int GC0307_enum_fmt(struct v4l2_subdev *sd, struct v4l2_fmtdesc *fmt);
static int GC0307_try_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt);
static int GC0307_s_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt);
static int GC0307_g_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt);
static int GC0307_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int GC0307_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int GC0307_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param);
static int GC0307_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param);
static int GC0307_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int GC0307_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);

static int GC0307_cropcap(struct v4l2_subdev *sd, struct v4l2_cropcap *cc);
static int GC0307_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *crop);
static int GC0307_s_crop(struct v4l2_subdev *sd, struct v4l2_crop *crop);


static int GC0307_s_interface(struct v4l2_subdev *sd, struct v4l2_interface *interface);
static int GC0307_suspend(struct v4l2_subdev *sd);
static int GC0307_resume(struct v4l2_subdev *sd);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

typedef struct{
	unsigned char reg_num;
	unsigned char value;
}regval_list;

static regval_list GC0307_default_regs[] = {
	{0x43  ,0x00},
	{0x44  ,0xa2},
	{0x40  ,0x10},
	{0x41  ,0x00},
	{0x42  ,0x10},
	{0x47  ,0x00},  //mode1,
	{0x48  ,0xc3},  //mode2,
	{0x49  ,0x00},  //dither_mode
	{0x4a  ,0x00},//clock_gating_en
	{0x4b  ,0x00},  //mode_reg3
	{0x4E  ,0x22},  //sync mode
	{0x4F  ,0x01},  //AWB, AEC, every N frame
	{0x01  ,0x74}, //HB
	{0x02  ,0x70}, //VB
	{0x1C  ,0x00}, //Vs_st
	{0x1D  ,0x00}, //Vs_et
	{0x10  ,0x00}, //high 4 bits of VB, HB
	{0x11  ,0x05}, //row_tail,  AD_pipe_number
	{0x05  ,0x00}, //row_start
	{0x06  ,0x00},
	{0x07  ,0x00}, //col start
	{0x08  ,0x00},
	{0x09  ,0x01}, //win height
	{0x0A  ,0xE8},
	{0x0B  ,0x02}, //win width, pixel array only 640
	{0x0C  ,0x80},
	{0x0D  ,0x22}, //rsh_width
	{0x0E  ,0x02}, //CISCTL mode2,
	{0x0F  ,0x82}, //CISCTL mode1
	{0x12  ,0x70}, //7 hrst, 6_4 darsg,
	{0x13  ,0x00}, //7 CISCTL_restart, 0 apwd
	{0x14  ,0x00}, //NA
	{0x15  ,0xba}, //7_4 vref
	{0x16  ,0x13}, //5to4 _coln_r,  __1to0__da18
	{0x17  ,0x52}, //opa_r, ref_r, sRef_r
	{0x18  ,0xc0}, //analog_mode, best case for left band.
	{0x1E  ,0x0d}, //tsp_width
	{0x1F  ,0x32}, //sh_delay
	{0x47  ,0x00},  //7__test_image, __6__fixed_pga, __5__auto_DN, __4__CbCr_fix,
	{0x19  ,0x06},  //pga_o
	{0x1a  ,0x06},  //pga_e
	{0x31  ,0x00},  //4	//pga_oFFset ,	 high 8bits of 11bits
	{0x3B  ,0x00},  //global_oFFset, low 8bits of 11bits
	{0x59  ,0x0f},  //offset_mode
	{0x58  ,0x88},  //DARK_VALUE_RATIO_G,  DARK_VALUE_RATIO_RB
	{0x57  ,0x08},  //DARK_CURRENT_RATE
	{0x56  ,0x77},  //PGA_OFFSET_EVEN_RATIO, PGA_OFFSET_ODD_RATIO
	{0x35  ,0xd8},  //blk_mode
	{0x36  ,0x40},
	{0x3C  ,0x00},
	{0x3D  ,0x00},
	{0x3E  ,0x00},
	{0x3F  ,0x00},
	{0xb5  ,0x70},
	{0xb6  ,0x40},
	{0xb7  ,0x00},
	{0xb8  ,0x38},
	{0xb9  ,0xc3},
	{0xba  ,0x0f},
	{0x7e  ,0x35},
	{0x7f  ,0x86},
	{0x5c  ,0x68}, //78
	{0x5d  ,0x78}, //88
	{0x61  ,0x80}, //manual_gain_g1
	{0x63  ,0x80}, //manual_gain_r
	{0x65  ,0x98}, //manual_gai_b, 0xa0=1.25, 0x98=1.1875
	{0x67  ,0x80}, //manual_gain_g2
	{0x68  ,0x18}, //global_manual_gain	 2.4bits
	{0x69  ,0x58},  //54
	{0x6A  ,0xf6},  //ff
	{0x6B  ,0xfb},  //fe
	{0x6C  ,0xf4},  //ff
	{0x6D  ,0x5a},  //5f
	{0x6E  ,0xe6},  //e1
	{0x6f  ,0x00},
	{0x70  ,0x14},
	{0x71  ,0x1c},
	{0x72  ,0x20},
	{0x73  ,0x10},
	{0x74  ,0x3c},
	{0x75  ,0x52},
	{0x7d  ,0x2f},  //dn_mode
	{0x80  ,0x0c}, //when auto_dn, check 7e,7f
	{0x81  ,0x0c},
	{0x82  ,0x44},
	{0x83  ,0x18},  //DD_TH1
	{0x84  ,0x18},  //DD_TH2
	{0x85  ,0x04},  //DD_TH3
	{0x87  ,0x34},  //32 b DNDD_low_range X16,  DNDD_low_range_C_weight_center
	{0x88  ,0x04},
	{0x89  ,0x01},
	{0x8a  ,0x50},//60
	{0x8b  ,0x50},//60
	{0x8c  ,0x07},
	{0x50  ,0x0c},
	{0x5f  ,0x3c},
	{0x8e  ,0x02},
	{0x86  ,0x02},
	{0x51  ,0x20},
	{0x52  ,0x08},
	{0x53  ,0x00},
	{0x77  ,0x80}, //contrast_center
	{0x78  ,0x00}, //fixed_Cb
	{0x79  ,0x00}, //fixed_Cr
	{0x7a  ,0x00}, //luma_offset
	{0x7b  ,0x40}, //hue_cos
	{0x7c  ,0x00}, //hue_sin
	{0xa0  ,0x40}, //global_saturation
	{0xa1  ,0x40}, //luma_contrast
	{0xa2  ,0x34}, //saturation_Cb
	{0xa3  ,0x34}, //saturation_Cr
	{0xa4  ,0xc8},
	{0xa5  ,0x02},
	{0xa6  ,0x28},
	{0xa7  ,0x02},
	{0xa8  ,0xee},
	{0xa9  ,0x12},
	{0xaa  ,0x01},
	{0xab  ,0x20},
	{0xac  ,0xf0},
	{0xad  ,0x10},
	{0xae  ,0x18},
	{0xaf  ,0x74},
	{0xb0  ,0xe0},
	{0xb1  ,0x20},
	{0xb2  ,0x6c},
	{0xb3  ,0x40},
	{0xb4  ,0x04},
	{0xbb  ,0x42},
	{0xbc  ,0x60},
	{0xbd  ,0x50},
	{0xbe  ,0x50},
	{0xbf  ,0x0c},
	{0xc0  ,0x06},
	{0xc1  ,0x70},
	{0xc2  ,0xf1},  //f4
	{0xc3  ,0x40},
	{0xc4  ,0x20}, //18
	{0xc5  ,0x33},
	{0xc6  ,0x1d},
	{0xca  ,0x80},// 70
	{0xcb  ,0x80}, // 70
	{0xcc  ,0x80}, // 78
	{0xcd  ,0x80}, //R_ratio
	{0xce  ,0x80}, //G_ratio  , cold_white white
	{0xcf  ,0x80}, //B_ratio
	{0x20  ,0x02},
	{0x21  ,0xc0},
	{0x22  ,0x60},
	{0x23  ,0x88},
	{0x24  ,0x96},
	{0x25  ,0x30},
	{0x26  ,0xd0},
	{0x27  ,0x00},
	{0x28  ,0x02}, //AEC_exp_level_1bit11to8
	{0x29  ,0x58}, //AEC_exp_level_1bit7to0
	{0x2a  ,0x03}, //AEC_exp_level_2bit11to8
	{0x2b  ,0x84}, //AEC_exp_level_2bit7to0
	{0x2c  ,0x09}, //AEC_exp_level_3bit11to8   659 - 8FPS,  8ca - 6FPS  //
	{0x2d  ,0x60}, //AEC_exp_level_3bit7to0
	{0x2e  ,0x0a}, //AEC_exp_level_4bit11to8   4FPS
	{0x2f  ,0x8c}, //AEC_exp_level_4bit7to0
	{0x30  ,0x20},
	{0x31  ,0x00},
	{0x32  ,0x1c},
	{0x33  ,0x90},
	{0x34  ,0x10},
	{0xd0  ,0x34},
	{0xd1  ,0x50}, //AEC_target_Y
	{0xd2  ,0xf2},
	{0xd4  ,0x96},
	{0xd5  ,0x10},
	{0xd6  ,0x96}, //antiflicker_step
	{0xd7  ,0x10}, //AEC_exp_time_min
	{0xd8  ,0x02},
	{0xdd  ,0x12},
	{0xe0  ,0x03},
	{0xe1  ,0x02},
	{0xe2  ,0x27},
	{0xe3  ,0x1e},
	{0xe8  ,0x3b},
	{0xe9  ,0x6e},
	{0xea  ,0x2c},
	{0xeb  ,0x50},
	{0xec  ,0x73},
	{0xed  ,0x00}, //close_frame_num1 ,can be use to reduce FPS
	{0xee  ,0x00}, //close_frame_num2
	{0xef  ,0x00}, //close_frame_num
	{0xf0  ,0x01}, //select page1
	{0x00  ,0x20},
	{0x01  ,0x20},
	{0x02  ,0x20},
	{0x03  ,0x20},
	{0x04  ,0x78},
	{0x05  ,0x78},
	{0x06  ,0x78},
	{0x07  ,0x78},
	{0x10  ,0x04},
	{0x11  ,0x04},
	{0x12  ,0x04},
	{0x13  ,0x04},
	{0x14  ,0x01},
	{0x15  ,0x01},
	{0x16  ,0x01},
	{0x17  ,0x01},
	{0x20  ,0x00},
	{0x21  ,0x00},
	{0x22  ,0x00},
	{0x23  ,0x00},
	{0x24  ,0x00},
	{0x25  ,0x00},
	{0x26  ,0x00},
	{0x27  ,0x00},
	{0x40  ,0x11},
	{0x45  ,0x06},
	{0x46  ,0x06},
	{0x47  ,0x05},
	{0x48  ,0x04},
	{0x49  ,0x03},
	{0x4a  ,0x03},
	{0x62  ,0xd8},
	{0x63  ,0x24},
	{0x64  ,0x24},
	{0x65  ,0x24},
	{0x66  ,0xd8},
	{0x67  ,0x24},
	{0x5a  ,0x00},
	{0x5b  ,0x00},
	{0x5c  ,0x00},
	{0x5d  ,0x00},
	{0x5e  ,0x00},
	{0x5f  ,0x00},
	{0x69  ,0x03}, //cc_mode
	{0x70  ,0x5d},
	{0x71  ,0xed},
	{0x72  ,0xff},
	{0x73  ,0xe5},
	{0x74  ,0x5f},
	{0x75  ,0xe6},
	{0x76  ,0x41},
	{0x77  ,0xef},
	{0x78  ,0xff},
	{0x79  ,0xff},
	{0x7a  ,0x5f},
	{0x7b  ,0xfa},
	{0x7e  ,0x00},
	{0x7f  ,0x00},
	{0x80  ,0xc8},
	{0x81  ,0x06},
	{0x82  ,0x08},
	{0x83  ,0x23},
	{0x84  ,0x38},
	{0x85  ,0x4F},
	{0x86  ,0x61},
	{0x87  ,0x72},
	{0x88  ,0x80},
	{0x89  ,0x8D},
	{0x8a  ,0xA2},
	{0x8b  ,0xB2},
	{0x8c  ,0xC0},
	{0x8d  ,0xCA},
	{0x8e  ,0xD3},
	{0x8f  ,0xDB},
	{0x90  ,0xE2},
	{0x91  ,0xED},
	{0x92  ,0xF6},
	{0x93  ,0xFD},
	{0x94  ,0x04},
	{0x95  ,0x0E},
	{0x96  ,0x1B},
	{0x97  ,0x28},
	{0x98  ,0x35},
	{0x99  ,0x41},
	{0x9a  ,0x4E},
	{0x9b  ,0x67},
	{0x9c  ,0x7E},
	{0x9d  ,0x94},
	{0x9e  ,0xA7},
	{0x9f  ,0xBA},
	{0xa0  ,0xC8},
	{0xa1  ,0xD4},
	{0xa2  ,0xE7},
	{0xa3  ,0xF4},
	{0xa4  ,0xFA},
	{0xf0  ,0x00}, //set back to page0
	{0x45  ,0x24},
	{0x40  ,0x7e},
	{0x41  ,0x2F},
	{0x47  ,0x20},
	{0x43  ,0x40},
	{0x44  ,0xE2},
	{0xff  ,0xff},
};
//=========================

static regval_list GC0307_fmt_yuv422[] = {
	{0xff, 0xff}	//end
};
//*=========================
static regval_list GC0307_fmt_rgb565[] = {
	{0xff, 0xff}	//end
};
//-=========================
static regval_list GC0307_resume_regs[] = {
	{0xff, 0xff}	//end
};
//*=========================
static regval_list GC0307_suspend_regs[] = {
	{0xff, 0xff}	//end
};
//-=========================
static const struct v4l2_subdev_core_ops GC0307_core_ops = {
//	.g_chip_ident = GC0307_g_chip_ident,
	.g_ctrl = GC0307_g_ctrl,
	.s_ctrl = GC0307_s_ctrl,
	.queryctrl = GC0307_queryctrl,
//	.reset = GC0307_reset,
	.init = GC0307_init,
};
//*=========================
static const struct v4l2_subdev_video_ops GC0307_video_ops = {
	.enum_fmt = GC0307_enum_fmt,
	.try_fmt = GC0307_try_fmt,
	.s_fmt = GC0307_s_fmt,
	.g_fmt = GC0307_g_fmt,
	.s_parm = GC0307_s_parm,
	.g_parm = GC0307_g_parm,
	.cropcap = GC0307_cropcap,
	.g_crop = GC0307_g_crop,
	.s_crop = GC0307_s_crop,
};
//-=========================
static const struct v4l2_subdev_ext_ops GC0307_ext_ops = {
	.s_interface = GC0307_s_interface,
	.suspend = GC0307_suspend,
	.resume = GC0307_resume,
};
//--------------------------
static const struct v4l2_subdev_ops GC0307_ops = {
	.core = &GC0307_core_ops,
	.video = &GC0307_video_ops,
	.ext = &GC0307_ext_ops
};
//=========================
static struct GC0307_format_struct {
	__u8 *desc;
	__u32 pixelformat;
	regval_list *regs;
	int cmatrix[CMATRIX_LEN];
	int bpp;   /* Bytes per pixel */
} GC0307_formats[] = {
	{
		.desc		= "YUYV 4:2:2",
		.pixelformat= V4L2_PIX_FMT_YUYV,
		.regs		= GC0307_fmt_yuv422,
		.cmatrix	= { 128, -128, 0, -34, -94, 128 },
		.bpp		= 2,
	},
	{
		.desc		= "RGB 5-6-5",
		.pixelformat= V4L2_PIX_FMT_RGB565,
		.regs		= GC0307_fmt_rgb565,
		.cmatrix	= { 128, -128, 0, -34, -94, 128 },
		.bpp		= 2,
	},
};
#define N_GC0307_FMTS ARRAY_SIZE(GC0307_formats)

static struct GC0307_win_size {
	int	width;
	int	height;
	unsigned char com7_bit;
	int	hstart;		/* Start/stop values for the camera.  Note */
	int	hstop;		/* that they do not always make complete */
	int	vstart;		/* sense to humans, but evidently the sensor */
	int	vstop;		/* will do the right thing... */
	regval_list *regs; /* Regs to tweak */
/* h/vref stuff */
} GC0307_win_sizes[] = {
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
#define N_WIN_SIZES (ARRAY_SIZE(GC0307_win_sizes))

struct GC0307_format_struct;  /* coming later */
typedef struct GC0307_info_t {
	struct v4l2_subdev sd;
	struct GC0307_format_struct *fmt;  /* Current format */
	unsigned char sat;		/* Saturation value */
	int hue;			/* Hue value */
}GC0307_info_s;

static GC0307_info_s GC0307_info;
struct i2c_bus_attr_t *i2c_attr;


/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
/*
static int GC0307_read(
	unsigned char reg,
	unsigned char *value
)
{
	char data[2];
	int ret;

	data[0] = reg;
	ret = gp_i2c_bus_read((int)i2c_attr, data, 2);
	*value = data[1];

	return ret;
}
*/
static int GC0307_write(unsigned char *pvals)
{
	return gp_i2c_bus_write((int)i2c_attr, pvals, sizeof(regval_list));
}

static int GC0307_write_array(regval_list *vals)
{
	int ret;
	int i,k;
	unsigned char *pvals;

	pvals = (unsigned char*) vals;

	k = sizeof(regval_list);

	while (1) {
		for(i=0;i<k;i++)
		{
			if(*(pvals+i) != 0xff)
			{
				goto snedi2cdata;
			}
		}
		break;//all vals is 0xff,reach end, then break
snedi2cdata:
		mdelay(1);
		ret = GC0307_write(pvals);
		if (ret < 0)
			return ret;
		vals++;
		pvals = (unsigned char*) vals;//get next vals
	}
	return 0;
}

static int GC0307_s_interface(
	struct v4l2_subdev *sd,
	struct v4l2_interface *interface
)
{
	return 0;
}

static int GC0307_enum_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_fmtdesc *fmt
)
{
	struct GC0307_format_struct *ofmt;

	if (fmt->index >= N_GC0307_FMTS)
		return -EINVAL;

	ofmt = GC0307_formats + fmt->index;
	fmt->flags = 0;
	strcpy(fmt->description, ofmt->desc);
	fmt->pixelformat = ofmt->pixelformat;

	return 0;
}

static int GC0307_try_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_format *fmt
)
{


	int index;
	struct GC0307_win_size *wsize;
	struct v4l2_pix_format *pix = &fmt->fmt.pix;

	for( index=0; index<N_GC0307_FMTS; index++ )
		if (GC0307_formats[index].pixelformat == pix->pixelformat)
			break;
	if (index >= N_GC0307_FMTS) {
		/* default to first format */
		index = 0;
		pix->pixelformat = GC0307_formats[0].pixelformat;
		printk(KERN_NOTICE "No match format\n");
	}

	pix->field = V4L2_FIELD_NONE;

	for (wsize = GC0307_win_sizes; wsize < GC0307_win_sizes + N_WIN_SIZES; wsize++)
		if (pix->width >= wsize->width && pix->height >= wsize->height)
			break;
	if (wsize >= GC0307_win_sizes + N_WIN_SIZES)
		wsize--;   /* Take the smallest one */

	/*
	 * Note the size we'll actually handle.
	 */
	pix->width = wsize->width;
	pix->height = wsize->height;
	pix->bytesperline = pix->width*GC0307_formats[index].bpp;
	pix->sizeimage = pix->height*pix->bytesperline;
	return 0;
}

static int GC0307_s_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_format *fmt
)
{
	return 0;
}

static int GC0307_g_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_format *fmt
)
{
	return 0;
}

static int GC0307_cropcap(
	struct v4l2_subdev *sd,
	struct v4l2_cropcap *cc
)
{
	return 0;
}


static int GC0307_g_crop(
	struct v4l2_subdev *sd,
	struct v4l2_crop *crop
)
{
	return 0;
}

static int GC0307_s_crop(
	struct v4l2_subdev *sd,
	struct v4l2_crop *crop
)
{
	return 0;
}

static int GC0307_g_parm(
	struct v4l2_subdev *sd,
	struct v4l2_streamparm *parms
)
{
	return 0;
}

static int GC0307_s_parm(
	struct v4l2_subdev *sd,
	struct v4l2_streamparm *param
)
{
	return 0;
}

static int GC0307_queryctrl(
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

static int GC0307_g_ctrl(
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

static int GC0307_s_ctrl(
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

int GC0307_init(
	struct v4l2_subdev *sd,
	u32 val
)
{
	return GC0307_write_array(GC0307_default_regs);
}

int GC0307_suspend(
	struct v4l2_subdev *sd
)
{
	return GC0307_write_array(GC0307_suspend_regs);
}

int GC0307_resume(
	struct v4l2_subdev *sd
)
{
	return GC0307_write_array(GC0307_resume_regs);
}

static int gp_GC0307_init(void)
{
	int ret;

	v4l2_subdev_init(&(GC0307_info.sd), &GC0307_ops);

	strcpy(GC0307_info.sd.name, "GC0307");
	
	ret = register_sensor(&(GC0307_info.sd), (int *)&param[0]);
	if( ret<0 )
		return ret;

	i2c_attr = (struct i2c_bus_attr_t *)gp_i2c_bus_request(SENSOR_I2C_SLAVE_ADDR, 20);
	return 0;
}

static void gp_GC0307_exit(void) {
	unregister_sensor(&(GC0307_info.sd));
	return;
}

module_init(gp_GC0307_init);
module_exit(gp_GC0307_exit);

MODULE_LICENSE_GP;
