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
#include <mach/gp_csi.h>
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


/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/


/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/


/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
int GC0309_init(struct v4l2_subdev *sd,	u32 val);
static int GC0309_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc);
static int GC0309_enum_fmt(struct v4l2_subdev *sd, struct v4l2_fmtdesc *fmt);
static int GC0309_try_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt);
static int GC0309_s_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt);
static int GC0309_g_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt);
static int GC0309_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int GC0309_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int GC0309_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param);
static int GC0309_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param);
static int GC0309_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int GC0309_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);

static int GC0309_cropcap(struct v4l2_subdev *sd, struct v4l2_cropcap *cc);
static int GC0309_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *crop);
static int GC0309_s_crop(struct v4l2_subdev *sd, struct v4l2_crop *crop);


static int GC0309_s_interface(struct v4l2_subdev *sd, struct v4l2_interface *interface);
static int GC0309_suspend(struct v4l2_subdev *sd);
static int GC0309_resume(struct v4l2_subdev *sd);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

typedef struct{
	unsigned char reg_num;
	unsigned char value;
}regval_list;

static regval_list GC0309_default_regs[] = {
	{0xfe,0x80},    	
		
	{0xfe,0x00},        // set page0
	
	{0x1a,0x16},    	
	{0xd2,0x10},    // close AEC
	{0x22,0x55},    //close AWB	//daniel 100209

	{0x5a,0x56},  
	{0x5b,0x40}, 
	{0x5c,0x4a}, 		
	
	{0x22,0x57},  
		
	{0x01,0x6a},  
	{0x02,0x25},  
	{0x0f,0x00}, 

	{0x03,0x01},  
	{0x04,0x2c},  

	{0xe2,0x00},  
	{0xe3,0x4b},  
		
	{0xe4,0x02},  
	{0xe5,0x0d},  
	{0xe6,0x02},  
	{0xe7,0x0d},  
	{0xe8,0x02},  
	{0xe9,0x0d},  
	{0xea,0x05},  
	{0xeb,0xdc},  

	{0xd2,0x90},   // Open AEC


	{0x05,0x00}, 
	{0x06,0x00}, 
	{0x07,0x00},  
	{0x08,0x00},  
	{0x09,0x01},  
	{0x0a,0xe8},  
	{0x0b,0x02},  
	{0x0c,0x88},  
	{0x0d,0x02},  
	{0x0e,0x02},  
	{0x10,0x22},  
	{0x11,0x0d},  
	{0x12,0x2a},  
	{0x13,0x00},  
	{0x15,0x0a},  
	{0x16,0x05},  
	{0x17,0x01},  

	{0x1b,0x00},  
	{0x1c,0xc1},  
	{0x1d,0x08},  
	{0x1e,0x20},  //0x60
	{0x1f,0x16},  

	{0x20,0xff},  
	{0x21,0xf8},  
	{0x24,0xa2},  
	{0x25,0x0f}, 
	//output sync_mode
	{0x26,0x02},  
	{0x2f,0x01},  
	/////////////////////////////////////////////////////////////////////
	/////////////////////////// grab_t ////////////////////////////////
	/////////////////////////////////////////////////////////////////////
	{0x30,0xf7},  // a7  //daniel 100209
	{0x31,0x40}, 
	{0x32,0x00},  
	{0x39,0x04},    //daniel 100209
	{0x3a,0x20},  //10 
	{0x3b,0x20},  // 20 //daniel 100209
	{0x3c,0x02},  
	{0x3d,0x02},  
	{0x3e,0x02}, 
	{0x3f,0x02},  
	
	//gain
	{0x50,0x24},  
	
	{0x53,0x82},  
	{0x54,0x80},  
	{0x55,0x80},  
	{0x56,0x82},  
	
	/////////////////////////////////////////////////////////////////////
	/////////////////////////// LSC_t  ////////////////////////////////
	/////////////////////////////////////////////////////////////////////
	{0x8b,0x20},  
	{0x8c,0x20},  
	{0x8d,0x20},  
	{0x8e,0x10},  
	{0x8f,0x10},  
	{0x90,0x10},  
	{0x91,0x3c},  
	{0x92,0x50},  
	{0x5d,0x12},  
	{0x5e,0x1a},  
	{0x5f,0x24},  
	/////////////////////////////////////////////////////////////////////
	/////////////////////////// DNDD_t  ///////////////////////////////
	/////////////////////////////////////////////////////////////////////
	{0x60,0x07},  
	{0x61,0x0e},  
	{0x62,0x0c},  
	{0x64,0x03},  
	{0x66,0xe8},  
	{0x67,0x86},  
	{0x68,0xa2},  
	
	/////////////////////////////////////////////////////////////////////
	/////////////////////////// asde_t ///////////////////////////////
	/////////////////////////////////////////////////////////////////////
	{0x69,0x20},  
	{0x6a,0x0f},  
	{0x6b,0x00},  //daniel 100209
	{0x6c,0x53},  
	{0x6d,0x83},  
	{0x6e,0xac},  
	{0x6f,0xac},  
	{0x70,0x15},  
	{0x71,0x33},  
	/////////////////////////////////////////////////////////////////////
	/////////////////////////// eeintp_t///////////////////////////////
	/////////////////////////////////////////////////////////////////////
	{0x72,0xdc},   //daniel 100209
	{0x73,0x80},   
	//for high resolution in light scene
	{0x74,0x02},  
	{0x75,0x3f},  
	{0x76,0x02},  
	{0x77,0x54},  //daniel 100209
	{0x78,0x88},  
	{0x79,0x81},  
	{0x7a,0x81},  
	{0x7b,0x22},  
	{0x7c,0xff}, 
	
	
	/////////////////////////////////////////////////////////////////////
	///////////////////////////CC_t///////////////////////////////
	/////////////////////////////////////////////////////////////////////
	{0x93,0x45},  //daniel 100209
	{0x94,0x00},  
	{0x95,0x00},  //daniel 100209
	{0x96,0x00},  
	{0x97,0x45},  
	{0x98,0xf0},  
	{0x9c,0x00},  
	{0x9d,0x03},  
	{0x9e,0x00},  
	
	
	/////////////////////////////////////////////////////////////////////
	///////////////////////////YCP_t///////////////////////////////
	/////////////////////////////////////////////////////////////////////
	{0xb1,0x40},  
	{0xb2,0x40},  
	{0xb8,0x20},  
	{0xbe,0x36},  
	{0xbf,0x00},  
	/////////////////////////////////////////////////////////////////////
	///////////////////////////AEC_t///////////////////////////////
	/////////////////////////////////////////////////////////////////////
	{0xd0,0xc9},   //daniel 100209
	{0xd1,0x10},   

	{0xd3,0x80},   //daniel 100209
	{0xd5,0xf2},  
	{0xd6,0x16},     //daniel 100209
	{0xdb,0x92},  
	{0xdc,0xa5},   
	{0xdf,0x23},    
	{0xd9,0x00},   
	{0xda,0x00},   
	{0xe0,0x09}, 

	{0xec,0x20},   
	{0xed,0x04},   
	{0xee,0xa0},   
	{0xef,0x40},   
	///////////////////////////////////////////////////////////////////
	///////////////////////////GAMMA//////////////////////////////////
	///////////////////////////////////////////////////////////////////
#if 1	
	{0x9F,0x10},            
	{0xA0,0x20},   
	{0xA1,0x38}, 
	{0xA2,0x4e}, 
	{0xA3,0x63}, 
	{0xA4,0x76}, 
	{0xA5,0x87}, 
	{0xA6,0xa2}, 
	{0xA7,0xb8}, 
	{0xA8,0xca}, 
	{0xA9,0xd8}, 
	{0xAA,0xe3}, 
	{0xAB,0xeb}, 
	{0xAC,0xf0}, 
	{0xAD,0xf8}, 
	{0xAE,0xfd}, 
	{0xAF,0xff}, 
#endif
	//Y_gamma
	{0xc0,0x00}, 
	{0xc1,0x0B}, 
	{0xc2,0x15}, 
	{0xc3,0x27}, 
	{0xc4,0x39}, 
	{0xc5,0x49}, 
	{0xc6,0x5A}, 
	{0xc7,0x6A}, 
	{0xc8,0x89}, 
	{0xc9,0xA8}, 
	{0xca,0xC6}, 
	{0xcb,0xE3}, 
	{0xcc,0xFF}, 

	/////////////////////////////////////////////////////////////////
	/////////////////////////// ABS_t ///////////////////////////////
	/////////////////////////////////////////////////////////////////
	{0xf0,0x02}, 
	{0xf1,0x01}, 
	{0xf2,0x00},   //0x04
	{0xf3,0x30},  
	
	/////////////////////////////////////////////////////////////////
	/////////////////////////// Measure Window ///////////////////////
	/////////////////////////////////////////////////////////////////
	{0xf7,0x04},  
	{0xf8,0x02},  
	{0xf9,0x9f}, 
	{0xfa,0x78}, 

	//---------------------------------------------------------------
	{0xfe,0x01}, 
	
	/////////////////////////////////////////////////////////////////
	///////////////////////////AWB_p/////////////////////////////////
	/////////////////////////////////////////////////////////////////
	{0x00,0xf5},  
	{0x01,0x0a},   
	{0x02,0x1a},  
	{0x0a,0xa0},  
	{0x0b,0x64},  
	{0x0c,0x08}, 
	{0x0e,0x4c},  
	{0x0f,0x39},  
	{0x11,0x3f},  
	{0x13,0x11},  
	{0x14,0x40},   
	{0x15,0x40},  
	{0x16,0xc2},  
	{0x17,0xa8},  
	{0x18,0x18},   
	{0x19,0x40},   
	{0x1a,0xd0},  
	{0x1b,0xf5},   

	{0x70,0x43},  
	{0x71,0x58},   
	{0x72,0x30},   
	{0x73,0x48},   
	{0x74,0x20},   
	{0x75,0x60},   
	
	{0xfe,0x00}, 


	{0x23,0x00},    // effect normal mode
	{0x2d,0x0a},  
	{0x20,0xff},  
	{0xd2,0x90},  
	{0x73,0x00},  
	{0x77,0x38}, 
	{0xb3,0x40},  
	{0xb4,0x80},  
	{0xba,0x00},  
	{0xbb,0x00},  

	

	//{0x28,0x00},  clk  24M
	{0x28,0x11}, //11  clk  12M
					
	//delay 100ms
	{0x14,0x10},   // normal

	// {0x14,0x13,0,0}, 180¶È

	{0xff,0xff},
};
//=========================

static regval_list GC0309_fmt_yuv422[] = {
	{0xff, 0xff}	//end
};
//*=========================
static regval_list GC0309_fmt_rgb565[] = {
	{0xff, 0xff}	//end
};
//-=========================
static regval_list GC0309_resume_regs[] = {
	{0xff, 0xff}	//end
};
//*=========================
static regval_list GC0309_suspend_regs[] = {
	{0xff, 0xff}	//end
};
//-=========================
static const struct v4l2_subdev_core_ops GC0309_core_ops = {
//	.g_chip_ident = GC0309_g_chip_ident,
	.g_ctrl = GC0309_g_ctrl,
	.s_ctrl = GC0309_s_ctrl,
	.queryctrl = GC0309_queryctrl,
//	.reset = GC0309_reset,
	.init = GC0309_init,
};
//*=========================
static const struct v4l2_subdev_video_ops GC0309_video_ops = {
	.enum_fmt = GC0309_enum_fmt,
	.try_fmt = GC0309_try_fmt,
	.s_fmt = GC0309_s_fmt,
	.g_fmt = GC0309_g_fmt,
	.s_parm = GC0309_s_parm,
	.g_parm = GC0309_g_parm,
	.cropcap = GC0309_cropcap,
	.g_crop = GC0309_g_crop,
	.s_crop = GC0309_s_crop,
};
//-=========================
static const struct v4l2_subdev_ext_ops GC0309_ext_ops = {
	.s_interface = GC0309_s_interface,
	.suspend = GC0309_suspend,
	.resume = GC0309_resume,
};
//--------------------------
static const struct v4l2_subdev_ops GC0309_ops = {
	.core = &GC0309_core_ops,
	.video = &GC0309_video_ops,
	.ext = &GC0309_ext_ops
};
//=========================
static struct GC0309_format_struct {
	__u8 *desc;
	__u32 pixelformat;
	regval_list *regs;
	int cmatrix[CMATRIX_LEN];
	int bpp;   /* Bytes per pixel */
} GC0309_formats[] = {
	{
		.desc		= "YUYV 4:2:2",
		.pixelformat= V4L2_PIX_FMT_YUYV,
		.regs		= GC0309_fmt_yuv422,
		.cmatrix	= { 128, -128, 0, -34, -94, 128 },
		.bpp		= 2,
	},
	{
		.desc		= "RGB 5-6-5",
		.pixelformat= V4L2_PIX_FMT_RGB565,
		.regs		= GC0309_fmt_rgb565,
		.cmatrix	= { 128, -128, 0, -34, -94, 128 },
		.bpp		= 2,
	},
};
#define N_GC0309_FMTS ARRAY_SIZE(GC0309_formats)

static struct GC0309_win_size {
	int	width;
	int	height;
	unsigned char com7_bit;
	int	hstart;		/* Start/stop values for the camera.  Note */
	int	hstop;		/* that they do not always make complete */
	int	vstart;		/* sense to humans, but evidently the sensor */
	int	vstop;		/* will do the right thing... */
	regval_list *regs; /* Regs to tweak */
/* h/vref stuff */
} GC0309_win_sizes[] = {
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
#define N_WIN_SIZES (ARRAY_SIZE(GC0309_win_sizes))

struct GC0309_format_struct;  /* coming later */
typedef struct GC0309_info_t {
	struct v4l2_subdev sd;
	struct GC0309_format_struct *fmt;  /* Current format */
	unsigned char sat;		/* Saturation value */
	int hue;			/* Hue value */
}GC0309_info_s;

static GC0309_info_s GC0309_info;
struct i2c_bus_attr_t *i2c_attr;


/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/

static int GC0309_read(
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

static int GC0309_write(unsigned char *pvals)
{
	return gp_i2c_bus_write((int)i2c_attr, pvals, sizeof(regval_list));
}

static int GC0309_write_array(regval_list *vals)
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
		//printk("reg=0x%02x, value=0x%02x\n", *pvals,*(pvals+1));
		ret = GC0309_write(pvals);
		if (ret < 0)
			return ret;
		vals++;
		pvals = (unsigned char*) vals;//get next vals
	}
	return 0;
}

static int GC0309_s_interface(
	struct v4l2_subdev *sd,
	struct v4l2_interface *interface
)
{
	return 0;
}

static int GC0309_enum_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_fmtdesc *fmt
)
{
	struct GC0309_format_struct *ofmt;

	if (fmt->index >= N_GC0309_FMTS)
		return -EINVAL;

	ofmt = GC0309_formats + fmt->index;
	fmt->flags = 0;
	strcpy(fmt->description, ofmt->desc);
	fmt->pixelformat = ofmt->pixelformat;

	return 0;
}

static int GC0309_try_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_format *fmt
)
{
	int index;
	struct GC0309_win_size *wsize;
	struct v4l2_pix_format *pix = &fmt->fmt.pix;

	for( index=0; index<N_GC0309_FMTS; index++ )
		if (GC0309_formats[index].pixelformat == pix->pixelformat)
			break;
	if (index >= N_GC0309_FMTS) {
		/* default to first format */
		index = 0;
		pix->pixelformat = GC0309_formats[0].pixelformat;
		printk(KERN_NOTICE "No match format\n");
	}

	pix->field = V4L2_FIELD_NONE;

	for (wsize = GC0309_win_sizes; wsize < GC0309_win_sizes + N_WIN_SIZES; wsize++)
		if (pix->width >= wsize->width && pix->height >= wsize->height)
			break;
	if (wsize >= GC0309_win_sizes + N_WIN_SIZES)
		wsize--;   /* Take the smallest one */

	/*
	 * Note the size we'll actually handle.
	 */
	pix->width = wsize->width;
	pix->height = wsize->height;
	pix->bytesperline = pix->width*GC0309_formats[index].bpp;
	pix->sizeimage = pix->height*pix->bytesperline;
	return 0;
}

static int GC0309_s_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_format *fmt
)
{
	return 0;
}

static int GC0309_g_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_format *fmt
)
{
	return 0;
}

static int GC0309_cropcap(
	struct v4l2_subdev *sd,
	struct v4l2_cropcap *cc
)
{
	return 0;
}


static int GC0309_g_crop(
	struct v4l2_subdev *sd,
	struct v4l2_crop *crop
)
{
	return 0;
}

static int GC0309_s_crop(
	struct v4l2_subdev *sd,
	struct v4l2_crop *crop
)
{
	return 0;
}

static int GC0309_g_parm(
	struct v4l2_subdev *sd,
	struct v4l2_streamparm *parms
)
{
	return 0;
}

static int GC0309_s_parm(
	struct v4l2_subdev *sd,
	struct v4l2_streamparm *param
)
{
	return 0;
}

static int GC0309_queryctrl(
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

static int GC0309_g_ctrl(
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

static int GC0309_s_ctrl(
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

int GC0309_init(
	struct v4l2_subdev *sd,
	u32 val
)
{
	return GC0309_write_array(GC0309_default_regs);
}

int GC0309_suspend(
	struct v4l2_subdev *sd
)
{
	return GC0309_write_array(GC0309_suspend_regs);
}

int GC0309_resume(
	struct v4l2_subdev *sd
)
{
	return GC0309_write_array(GC0309_resume_regs);
}

static int gp_GC0309_init(void)
{
	int ret;

	v4l2_subdev_init(&(GC0309_info.sd), &GC0309_ops);

	strcpy(GC0309_info.sd.name, "GC0309");

	ret = register_sensor(&(GC0309_info.sd));
	if( ret<0 )
		return ret;

	i2c_attr = (struct i2c_bus_attr_t *)gp_i2c_bus_request(SENSOR_I2C_SLAVE_ADDR, 20);

	ret = GC0309_init(&(GC0309_info.sd), 0);
	if(ret<0)
	{
		DIAG_ERROR("GC0309 init fail\n");
		return ret;
	}
	return 0;
}

static void gp_GC0309_exit(void) {
	unregister_sensor(&(GC0309_info.sd));
	return;
}

module_init(gp_GC0309_init);
module_exit(gp_GC0309_exit);

MODULE_LICENSE_GP;
