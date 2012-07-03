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
 *  Hsinchu City 300    {  0x, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
 
/**************************************************************************
 *                         H E A D E R   F I L E S												*
 **************************************************************************/
#include <linux/module.h>
#include <linux/fs.h> /* everything... */
#include <linux/videodev2.h>
#include <media/v4l2-device.h>

#include <linux/delay.h>
#include <mach/gp_cdsp.h>
#include <mach/gp_gpio.h>
#include <mach/gp_i2c_bus.h>
#include <mach/sensor_mgr.h>


/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
/* max resolution: 2048*1536 */
#define	GT2005_ID					0x78
#define GT2005_WIDTH				1600
#define GT2005_HEIGHT				1200

#define GT2005_VYUY 				0x00
#define GT2005_UYVY					0x02
#define GT2005_BGGR					0x18
#define GT2005_GBRG					0x19
#define GT2005_GRBG					0x1A
#define GT2005_RGGB					0x1B

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define COLOR_BAR_EN 				0
#define I2C_USE_GPIO				0
#define I2C_SCL_IO					0x0
#define I2C_SDA_IO					0x1

static char *param[] = {"0", "PORT0", "0", "NONE", "0", "NONE"};
static int nstrs = 6;
module_param_array(param, charp, &nstrs, S_IRUGO);

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
struct regval_list_s 
{
	unsigned short reg_num;
	unsigned char value;
};

struct GT2005_fmt_s
{
	__u8 *desc;
	__u32 pixelformat;
	struct regval_list_s *pInitRegs;
	struct regval_list_s *pScaleRegs;
	int bpp;					/* Bytes per pixel */
}; 

typedef struct GT2005_dev_s 
{
	struct v4l2_subdev sd;
	struct GT2005_fmt_s *fmt;	/* Current format */
	short width;
	short height;
	int hue;					/* Hue value */
	unsigned char sat;			/* Saturation value */
	unsigned char reserved0;
	unsigned char reserved1;
	unsigned char reserved2;
}GT2005_dev_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static struct regval_list_s mipi_raw_qxga[] = 
{
	{ 0xffff, 0xff },
};

static struct regval_list_s mipi_yuv_qxga[] = 
{
	{ 0xffff, 0xff },
};

static struct regval_list_s mipi_scale_vga[] =//800*600
{
	{ 0xffff, 0xff },
};

static struct regval_list_s ccir601_raw_qxga[] = 
{
	{ 0xffff, 0xff },
};

static struct regval_list_s ccir601_yuv_qxga[] =    //1600*1200
{

	{0x0101 , 0x00},
	{0x0103 , 0x00},

	//Hcount&Vcount
	{0x0105 , 0x00},
	{0x0106 , 0xF0},
	{0x0107 , 0x00},
	{0x0108 , 0x1C},

	//Binning&Resoultion
	{0x0109 , 0x01},
	{0x010A , 0x00},
	{0x010B , 0x00},
	{0x010C , 0x00},
	{0x010D , 0x08},
	{0x010E , 0x00},
	{0x010F , 0x08},
	{0x0110 , 0x06},
	{0x0111 , 0x40},
	{0x0112 , 0x04},
	{0x0113 , 0xB0},
	/*
	{0x0109 , 0x01},
	{0x010A , 0x00},
	{0x010B , 0x00},
	{0x010C , 0x00},
	{0x010D , 0x08},
	{0x010E , 0x00},
	{0x010F , 0x08},
	{0x0110 , 0x06},
	{0x0111 , 0x40},
	{0x0112 , 0x04},
	{0x0113 , 0xB0},
	*/

	//YUV Mode
	{0x0114 , 0x00},

	//Picture Effect
	{0x0115 , 0x00},

	//PLL&Frame Rate
	{0x0116 , 0x02},
	{0x0117 , 0x00},
	{0x0118 , 0xdc},
	{0x0119 , 0x01},
	{0x011A , 0x04},
	{0x011B , 0x00},

	//DCLK Polarity
	{0x011C , 0x00},

	//Do not change
	{0x011D , 0x02},
	{0x011E , 0x00},
	
	{0x011F , 0x00},
	{0x0120 , 0x1C},
	{0x0121 , 0x00},
	{0x0122 , 0x04},
	{0x0123 , 0x00},
	{0x0124 , 0x00},
	{0x0125 , 0x00},
	{0x0126 , 0x00},
	{0x0127 , 0x00},
	{0x0128 , 0x00},

	//Contrast
	{0x0200 , 0x00},

	//Brightness
	{0x0201 , 0x00},

	//Saturation
	{0x0202 , 0x40},

	//Do not change
	{0x0203 , 0x00},
	{0x0204 , 0x03},
	{0x0205 , 0x1F},
	{0x0206 , 0x0B},
	{0x0207 , 0x20},
	{0x0208 , 0x00},
	{0x0209 , 0x2A},
	{0x020A , 0x01},

	//Sharpness
	{0x020B , 0x48},
	{0x020C , 0x64},

	//Do not change
	{0x020D , 0xC8},
	{0x020E , 0xBC},
	{0x020F , 0x08},
	{0x0210 , 0xD6},
	{0x0211 , 0x00},
	{0x0212 , 0x20},
	{0x0213 , 0x81},
	{0x0214 , 0x15},
	{0x0215 , 0x00},
	{0x0216 , 0x00},
	{0x0217 , 0x00},
	{0x0218 , 0x46},
	{0x0219 , 0x30},
	{0x021A , 0x03},
	{0x021B , 0x28},
	{0x021C , 0x02},
	{0x021D , 0x60},
	{0x021E , 0x00},
	{0x021F , 0x00},
	{0x0220 , 0x08},
	{0x0221 , 0x08},
	{0x0222 , 0x04},
	{0x0223 , 0x00},
	{0x0224 , 0x1F},
	{0x0225 , 0x1E},
	{0x0226 , 0x18},
	{0x0227 , 0x1D},
	{0x0228 , 0x1F},
	{0x0229 , 0x1F},
	{0x022A , 0x01},
	{0x022B , 0x04},
	{0x022C , 0x05},
	{0x022D , 0x05},
	{0x022E , 0x04},
	{0x022F , 0x03},
	{0x0230 , 0x02},
	{0x0231 , 0x1F},
	{0x0232 , 0x1A},
	{0x0233 , 0x19},
	{0x0234 , 0x19},
	{0x0235 , 0x1B},
	{0x0236 , 0x1F},
	{0x0237 , 0x04},
	{0x0238 , 0xEE},
	{0x0239 , 0xFF},
	{0x023A , 0x00},
	{0x023B , 0x00},
	{0x023C , 0x00},
	{0x023D , 0x00},
	{0x023E , 0x00},
	{0x023F , 0x00},
	{0x0240 , 0x00},
	{0x0241 , 0x00},
	{0x0242 , 0x00},
	{0x0243 , 0x21},
	{0x0244 , 0x42},
	{0x0245 , 0x53},
	{0x0246 , 0x54},
	{0x0247 , 0x54},
	{0x0248 , 0x54},
	{0x0249 , 0x33},
	{0x024A , 0x11},
	{0x024B , 0x00},
	{0x024C , 0x00},
	{0x024D , 0xFF},
	{0x024E , 0xEE},
	{0x024F , 0xDD},
	{0x0250 , 0x00},
	{0x0251 , 0x00},
	{0x0252 , 0x00},
	{0x0253 , 0x00},
	{0x0254 , 0x00},
	{0x0255 , 0x00},
	{0x0256 , 0x00},
	{0x0257 , 0x00},
	{0x0258 , 0x00},
	{0x0259 , 0x00},
	{0x025A , 0x00},
	{0x025B , 0x00},
	{0x025C , 0x00},
	{0x025D , 0x00},
	{0x025E , 0x00},
	{0x025F , 0x00},
	{0x0260 , 0x00},
	{0x0261 , 0x00},
	{0x0262 , 0x00},
	{0x0263 , 0x00},
	{0x0264 , 0x00},
	{0x0265 , 0x00},
	{0x0266 , 0x00},
	{0x0267 , 0x00},
	{0x0268 , 0x8F},
	{0x0269 , 0xA3},
	{0x026A , 0xB4},
	{0x026B , 0x90},
	{0x026C , 0x00},
	{0x026D , 0xD0},
	{0x026E , 0x60},
	{0x026F , 0xA0},
	{0x0270 , 0x40},
	{0x0300 , 0x81},
	{0x0301 , 0x80},
	{0x0302 , 0x22},
	{0x0303 , 0x06},
	{0x0304 , 0x03},
	{0x0305 , 0x83},
	{0x0306 , 0x00},
	{0x0307 , 0x22},
	{0x0308 , 0x00},
	{0x0309 , 0x55},
	{0x030A , 0x55},
	{0x030B , 0x55},
	{0x030C , 0x54},
	{0x030D , 0x1F},
	{0x030E , 0x0A},
	{0x030F , 0x10},
	{0x0310 , 0x04},
	{0x0311 , 0xFF},
	{0x0312 , 0x08},
	{0x0313 , 0x38},
	{0x0314 , 0xe0},
	{0x0315 , 0x96},
	{0x0316 , 0x26},
	{0x0317 , 0x02},
	{0x0318 , 0x08},
	{0x0319 , 0x0C},

#if 0 
	//Normal AWB Setting
	{0x031A , 0x81},
	{0x031B , 0x00},
	{0x031C , 0x3D},
	{0x031D , 0x00},
	{0x031E , 0xF9},
	{0x031F , 0x00},
	{0x0320 , 0x24},
	{0x0321 , 0x14},
	{0x0322 , 0x1A},
	{0x0323 , 0x24},
	{0x0324 , 0x08},
	{0x0325 , 0xF0},
	{0x0326 , 0x30},
	{0x0327 , 0x17},
	{0x0328 , 0x11},
	{0x0329 , 0x22},
	{0x032A , 0x2F},
	{0x032B , 0x21},
	{0x032C , 0xDA},
	{0x032D , 0x10},
	{0x032E , 0xEA},
	{0x032F , 0x18},
	{0x0330 , 0x29},
	{0x0331 , 0x25},
	{0x0332 , 0x12},
	{0x0333 , 0x0F},
	{0x0334 , 0xE0},
	{0x0335 , 0x13},
	{0x0336 , 0xFF},
	{0x0337 , 0x20},
	{0x0338 , 0x46},
	{0x0339 , 0x04},
	{0x033A , 0x04},
	{0x033B , 0xFF},
	{0x033C , 0x01},
	{0x033D , 0x00},

#else
	//A LIGHT CORRECTION
	{0x031A , 0x81},
	{0x031B , 0x00},
	{0x031C , 0x1D},
	{0x031D , 0x00},
	{0x031E , 0xFD},
	{0x031F , 0x00},
	{0x0320 , 0xE1},
	{0x0321 , 0x1A},
	{0x0322 , 0xDE},
	{0x0323 , 0x11},
	{0x0324 , 0x1A},
	{0x0325 , 0xEE},
	{0x0326 , 0x50},
	{0x0327 , 0x18},
	{0x0328 , 0x25},
	{0x0329 , 0x37},
	{0x032A , 0x24},
	{0x032B , 0x32},
	{0x032C , 0xA9},
	{0x032D , 0x32},
	{0x032E , 0xFF},
	{0x032F , 0x7F},
	{0x0330 , 0xBA},
	{0x0331 , 0x7F},
	{0x0332 , 0x7F},
	{0x0333 , 0x14},
	{0x0334 , 0x81},
	{0x0335 , 0x14},
	{0x0336 , 0xFF},
	{0x0337 , 0x20},
	{0x0338 , 0x46},
	{0x0339 , 0x04},
	{0x033A , 0x04},
	{0x033B , 0x00},
	{0x033C , 0x00},
	{0x033D , 0x00},
#endif

	//Do not change
	{0x033E , 0x03},
	{0x033F , 0x28},
	{0x0340 , 0x02},
	{0x0341 , 0x60},
	{0x0342 , 0xAC},
	{0x0343 , 0x97},
	{0x0344 , 0x7F},
	{0x0400 , 0xE8},
	{0x0401 , 0x40},
	{0x0402 , 0x00},
	{0x0403 , 0x00},
	{0x0404 , 0xF8},
	{0x0405 , 0x03},
	{0x0406 , 0x03},
	{0x0407 , 0x85},
	{0x0408 , 0x44},
	{0x0409 , 0x1F},
	{0x040A , 0x40},
	{0x040B , 0x33},

	//Lens Shading Correction
	{0x040C , 0xA0},
	{0x040D , 0x00},
	{0x040E , 0x00},
	{0x040F , 0x00},
	{0x0410 , 0x0D},
	{0x0411 , 0x0D},
	{0x0412 , 0x0C},
	{0x0413 , 0x04},
	{0x0414 , 0x00},
	{0x0415 , 0x00},
	{0x0416 , 0x07},
	{0x0417 , 0x09},
	{0x0418 , 0x16},
	{0x0419 , 0x14},
	{0x041A , 0x11},
	{0x041B , 0x14},
	{0x041C , 0x07},
	{0x041D , 0x07},
	{0x041E , 0x06},
	{0x041F , 0x02},
	{0x0420 , 0x42},
	{0x0421 , 0x42},
	{0x0422 , 0x47},
	{0x0423 , 0x39},
	{0x0424 , 0x3E},
	{0x0425 , 0x4D},
	{0x0426 , 0x46},
	{0x0427 , 0x3A},
	{0x0428 , 0x21},
	{0x0429 , 0x21},
	{0x042A , 0x26},
	{0x042B , 0x1C},
	{0x042C , 0x25},
	{0x042D , 0x25},
	{0x042E , 0x28},
	{0x042F , 0x20},
	{0x0430 , 0x3E},
	{0x0431 , 0x3E},
	{0x0432 , 0x33},
	{0x0433 , 0x2E},
	{0x0434 , 0x54},
	{0x0435 , 0x53},
	{0x0436 , 0x3C},
	{0x0437 , 0x51},
	{0x0438 , 0x2B},
	{0x0439 , 0x2B},
	{0x043A , 0x38},
	{0x043B , 0x22},
	{0x043C , 0x3B},
	{0x043D , 0x3B},
	{0x043E , 0x31},
	{0x043F , 0x37},

	//PWB Gain
	{0x0440 , 0x00},
	{0x0441 , 0x4B},
	{0x0442 , 0x00},
	{0x0443 , 0x00},
	{0x0444 , 0x31},

	{0x0445 , 0x00},
	{0x0446 , 0x00},
	{0x0447 , 0x00},
	{0x0448 , 0x00},
	{0x0449 , 0x00},
	{0x044A , 0x00},
	{0x044D , 0xE0},
	{0x044E , 0x05},
	{0x044F , 0x07},
	{0x0450 , 0x00},
	{0x0451 , 0x00},
	{0x0452 , 0x00},
	{0x0453 , 0x00},
	{0x0454 , 0x00},
	{0x0455 , 0x00},
	{0x0456 , 0x00},
	{0x0457 , 0x00},
	{0x0458 , 0x00},
	{0x0459 , 0x00},
	{0x045A , 0x00},
	{0x045B , 0x00},
	{0x045C , 0x00},
	{0x045D , 0x00},
	{0x045E , 0x00},
	{0x045F , 0x00},

	//GAMMA Correction
	{0x0460 , 0x80},
	{0x0461 , 0x10},
	{0x0462 , 0x10},
	{0x0463 , 0x10},
	{0x0464 , 0x08},
	{0x0465 , 0x08},
	{0x0466 , 0x11},
	{0x0467 , 0x09},
	{0x0468 , 0x23},
	{0x0469 , 0x2A},
	{0x046A , 0x2A},
	{0x046B , 0x47},
	{0x046C , 0x52},
	{0x046D , 0x42},
	{0x046E , 0x36},
	{0x046F , 0x46},
	{0x0470 , 0x3A},
	{0x0471 , 0x32},
	{0x0472 , 0x32},
	{0x0473 , 0x38},
	{0x0474 , 0x3D},
	{0x0475 , 0x2F},
	{0x0476 , 0x29},
	{0x0477 , 0x48},

	//Do not change
	/*
	{0x0600 , 0x00},
	{0x0601 , 0x24},
	{0x0602 , 0x45},
	{0x0603 , 0x0E},
	{0x0604 , 0x14},
	{0x0605 , 0x2F},
	{0x0606 , 0x01},
	{0x0607 , 0x0E},
	{0x0608 , 0x0E},
	{0x0609 , 0x37},
	{0x060A , 0x18},
	{0x060B , 0xA0},
	{0x060C , 0x20},
	{0x060D , 0x07},
	{0x060E , 0x47},
	{0x060F , 0x90},
	{0x0610 , 0x06},
	{0x0611 , 0x0C},
	{0x0612 , 0x28},
	{0x0613 , 0x13},
	{0x0614 , 0x0B},
	{0x0615 , 0x10},
	{0x0616 , 0x14},
	{0x0617 , 0x19},
	{0x0618 , 0x52},
	{0x0619 , 0xA0},
	{0x061A , 0x11},
	{0x061B , 0x33},
	{0x061C , 0x56},
	{0x061D , 0x20},
	{0x061E , 0x28},
	{0x061F , 0x2B},
	{0x0620 , 0x22},
	{0x0621 , 0x11},
	{0x0622 , 0x75},
	{0x0623 , 0x49},
	{0x0624 , 0x6E},
	{0x0625 , 0x80},
	{0x0626 , 0x02},
	{0x0627 , 0x0C},
	{0x0628 , 0x51},
	{0x0629 , 0x25},
	{0x062A , 0x01},
	{0x062B , 0x3D},
	{0x062C , 0x04},
	{0x062D , 0x01},
	{0x062E , 0x0C},
	{0x062F , 0x2C},
	{0x0630 , 0x0D},
	{0x0631 , 0x14},
	{0x0632 , 0x12},
	{0x0633 , 0x34},
	{0x0634 , 0x00},
	{0x0635 , 0x00},
	{0x0636 , 0x00},
	{0x0637 , 0xB1},
	{0x0638 , 0x22},
	{0x0639 , 0x32},
	{0x063A , 0x0E},
	{0x063B , 0x18},
	{0x063C , 0x88},
	{0x0640 , 0xB2},
	{0x0641 , 0xC0},
	{0x0642 , 0x01},
	{0x0643 , 0x26},
	{0x0644 , 0x13},
	{0x0645 , 0x88},
	{0x0646 , 0x64},
	{0x0647 , 0x00},
	{0x0681 , 0x1B},
	{0x0682 , 0xA0},
	{0x0683 , 0x28},
	{0x0684 , 0x00},
	{0x0685 , 0xB0},
	{0x0686 , 0x6F},
	{0x0687 , 0x33},
	{0x0688 , 0x1F},
	{0x0689 , 0x44},
	{0x068A , 0xA8},
	{0x068B , 0x44},
	{0x068C , 0x08},
	{0x068D , 0x08},
	{0x068E , 0x00},
	{0x068F , 0x00},
	{0x0690 , 0x01},
	{0x0691 , 0x00},
	{0x0692 , 0x01},
	{0x0693 , 0x00},
	{0x0694 , 0x00},
	{0x0695 , 0x00},
	{0x0696 , 0x00},
	{0x0697 , 0x00},
	{0x0698 , 0x2A},
	{0x0699 , 0x80},
	{0x069A , 0x1F},
	{0x069B , 0x00},
	{0x069C , 0x02},
	{0x069D , 0xF5},
	{0x069E , 0x03},
	{0x069F , 0x6D},
	{0x06A0 , 0x0C},
	{0x06A1 , 0xB8},
	{0x06A2 , 0x0D},
	{0x06A3 , 0x74},
	{0x06A4 , 0x00},
	{0x06A5 , 0x2F},
	{0x06A6 , 0x00},
	{0x06A7 , 0x2F},
	{0x0F00 , 0x00},
	{0x0F01 , 0x00},
	*/
	{0x0686 , 0x6F},

	//Output Enable
	{0x0100 , 0x01},
	{0x0102 , 0x02},
	{0x0104 , 0x03},

	//GT2005_H_V_Switch
	{0x0101 , 0x00},

	 {0x011c , 0x01},		//pclk 

	{ 0xffff, 0xff },
};

static struct regval_list_s ccir601_scale_xga[] =
{
	{ 0xffff, 0xff },
};

static struct regval_list_s ccir601_scale_vga[] = //800*600
{

	{0x0101 , 0x00},
	{0x0103 , 0x00},

	//Hcount&Vcount
	{0x0105 , 0x00},
	{0x0106 , 0xF0},
	{0x0107 , 0x00},
	{0x0108 , 0x1C},

	//Binning&Resoultion
	{0x0109 , 0x00},
	{0x010A , 0x04},
	{0x010B , 0x0f},
	{0x010C , 0x00},
	{0x010D , 0x08},
	{0x010E , 0x00},
	{0x010F , 0x08},
	{0x0110 , 0x03},
	{0x0111 , 0x20},
	{0x0112 , 0x02},
	{0x0113 , 0x58},


	//YUV Mode
	{0x0114 , 0x00},

	//Picture Effect
	{0x0115 , 0x00},

	//PLL&Frame Rate
	{0x0116 , 0x02},
	{0x0117 , 0x00},
	{0x0118 , 0xAC},
	{0x0119 , 0x01},
	{0x011A , 0x04},
	{0x011B , 0x00},

	//DCLK Polarity
	{0x011C , 0x01},

	//Do not change
	{0x011D , 0x02},
	{0x011E , 0x00},
	
	{0x011F , 0x00},
	{0x0120 , 0x1C},
	{0x0121 , 0x00},
	{0x0122 , 0x04},
	{0x0123 , 0x00},
	{0x0124 , 0x00},
	{0x0125 , 0x00},
	{0x0126 , 0x00},
	{0x0127 , 0x00},
	{0x0128 , 0x00},

	//Contrast
	{0x0200 , 0x00},

	//Brightness
	{0x0201 , 0x00},

	//Saturation
	{0x0202 , 0x40},

	//Do not change
	{0x0203 , 0x00},
	{0x0204 , 0x03},
	{0x0205 , 0x1F},
	{0x0206 , 0x0B},
	{0x0207 , 0x20},
	{0x0208 , 0x00},
	{0x0209 , 0x2A},
	{0x020A , 0x01},

	//Sharpness
	{0x020B , 0x48},
	{0x020C , 0x64},

	//Do not change
	{0x020D , 0xC8},
	{0x020E , 0xBC},
	{0x020F , 0x08},
	{0x0210 , 0xD6},
	{0x0211 , 0x00},
	{0x0212 , 0x20},
	{0x0213 , 0x81},
	{0x0214 , 0x15},
	{0x0215 , 0x00},
	{0x0216 , 0x00},
	{0x0217 , 0x00},
	{0x0218 , 0x46},
	{0x0219 , 0x30},
	{0x021A , 0x03},
	{0x021B , 0x28},
	{0x021C , 0x02},
	{0x021D , 0x60},
	{0x021E , 0x00},
	{0x021F , 0x00},
	{0x0220 , 0x08},
	{0x0221 , 0x08},
	{0x0222 , 0x04},
	{0x0223 , 0x00},
	{0x0224 , 0x1F},
	{0x0225 , 0x1E},
	{0x0226 , 0x18},
	{0x0227 , 0x1D},
	{0x0228 , 0x1F},
	{0x0229 , 0x1F},
	{0x022A , 0x01},
	{0x022B , 0x04},
	{0x022C , 0x05},
	{0x022D , 0x05},
	{0x022E , 0x04},
	{0x022F , 0x03},
	{0x0230 , 0x02},
	{0x0231 , 0x1F},
	{0x0232 , 0x1A},
	{0x0233 , 0x19},
	{0x0234 , 0x19},
	{0x0235 , 0x1B},
	{0x0236 , 0x1F},
	{0x0237 , 0x04},
	{0x0238 , 0xEE},
	{0x0239 , 0xFF},
	{0x023A , 0x00},
	{0x023B , 0x00},
	{0x023C , 0x00},
	{0x023D , 0x00},
	{0x023E , 0x00},
	{0x023F , 0x00},
	{0x0240 , 0x00},
	{0x0241 , 0x00},
	{0x0242 , 0x00},
	{0x0243 , 0x21},
	{0x0244 , 0x42},
	{0x0245 , 0x53},
	{0x0246 , 0x54},
	{0x0247 , 0x54},
	{0x0248 , 0x54},
	{0x0249 , 0x33},
	{0x024A , 0x11},
	{0x024B , 0x00},
	{0x024C , 0x00},
	{0x024D , 0xFF},
	{0x024E , 0xEE},
	{0x024F , 0xDD},
	{0x0250 , 0x00},
	{0x0251 , 0x00},
	{0x0252 , 0x00},
	{0x0253 , 0x00},
	{0x0254 , 0x00},
	{0x0255 , 0x00},
	{0x0256 , 0x00},
	{0x0257 , 0x00},
	{0x0258 , 0x00},
	{0x0259 , 0x00},
	{0x025A , 0x00},
	{0x025B , 0x00},
	{0x025C , 0x00},
	{0x025D , 0x00},
	{0x025E , 0x00},
	{0x025F , 0x00},
	{0x0260 , 0x00},
	{0x0261 , 0x00},
	{0x0262 , 0x00},
	{0x0263 , 0x00},
	{0x0264 , 0x00},
	{0x0265 , 0x00},
	{0x0266 , 0x00},
	{0x0267 , 0x00},
	{0x0268 , 0x8F},
	{0x0269 , 0xA3},
	{0x026A , 0xB4},
	{0x026B , 0x90},
	{0x026C , 0x00},
	{0x026D , 0xD0},
	{0x026E , 0x60},
	{0x026F , 0xA0},
	{0x0270 , 0x40},
	{0x0300 , 0x81},
	{0x0301 , 0x80},
	{0x0302 , 0x22},
	{0x0303 , 0x06},
	{0x0304 , 0x03},
	{0x0305 , 0x83},
	{0x0306 , 0x00},
	{0x0307 , 0x22},
	{0x0308 , 0x00},
	{0x0309 , 0x55},
	{0x030A , 0x55},
	{0x030B , 0x55},
	{0x030C , 0x54},
	{0x030D , 0x1F},
	{0x030E , 0x0A},
	{0x030F , 0x10},
	{0x0310 , 0x04},
	{0x0311 , 0xFF},
	{0x0312 , 0x08},
	{0x0313 , 0x26},
	{0x0314 , 0xFF},
	{0x0315 , 0x96},
	{0x0316 , 0x26},
	{0x0317 , 0x02},
	{0x0318 , 0x08},
	{0x0319 , 0x0C},

	//A LIGHT CORRECTION
	{0x031A , 0x81},
	{0x031B , 0x00},
	{0x031C , 0x1D},
	{0x031D , 0x00},
	{0x031E , 0xFD},
	{0x031F , 0x00},
	{0x0320 , 0xE1},
	{0x0321 , 0x1A},
	{0x0322 , 0xDE},
	{0x0323 , 0x11},
	{0x0324 , 0x1A},
	{0x0325 , 0xEE},
	{0x0326 , 0x50},
	{0x0327 , 0x18},
	{0x0328 , 0x25},
	{0x0329 , 0x37},
	{0x032A , 0x24},
	{0x032B , 0x32},
	{0x032C , 0xA9},
	{0x032D , 0x32},
	{0x032E , 0xFF},
	{0x032F , 0x7F},
	{0x0330 , 0xBA},
	{0x0331 , 0x7F},
	{0x0332 , 0x7F},
	{0x0333 , 0x14},
	{0x0334 , 0x81},
	{0x0335 , 0x14},
	{0x0336 , 0xFF},
	{0x0337 , 0x20},
	{0x0338 , 0x46},
	{0x0339 , 0x04},
	{0x033A , 0x04},
	{0x033B , 0x00},
	{0x033C , 0x00},
	{0x033D , 0x00},

	//Do not change
	{0x033E , 0x03},
	{0x033F , 0x28},
	{0x0340 , 0x02},
	{0x0341 , 0x60},
	{0x0342 , 0xAC},
	{0x0343 , 0x97},
	{0x0344 , 0x7F},
	{0x0400 , 0xE8},
	{0x0401 , 0x40},
	{0x0402 , 0x00},
	{0x0403 , 0x00},
	{0x0404 , 0xF8},
	{0x0405 , 0x03},
	{0x0406 , 0x03},
	{0x0407 , 0x85},
	{0x0408 , 0x44},
	{0x0409 , 0x1F},
	{0x040A , 0x40},
	{0x040B , 0x33},

	//Lens Shading Correction
	{0x040C , 0xA0},
	{0x040D , 0x00},
	{0x040E , 0x00},
	{0x040F , 0x00},
	{0x0410 , 0x0D},
	{0x0411 , 0x0D},
	{0x0412 , 0x0C},
	{0x0413 , 0x04},
	{0x0414 , 0x00},
	{0x0415 , 0x00},
	{0x0416 , 0x07},
	{0x0417 , 0x09},
	{0x0418 , 0x16},
	{0x0419 , 0x14},
	{0x041A , 0x11},
	{0x041B , 0x14},
	{0x041C , 0x07},
	{0x041D , 0x07},
	{0x041E , 0x06},
	{0x041F , 0x02},
	{0x0420 , 0x42},
	{0x0421 , 0x42},
	{0x0422 , 0x47},
	{0x0423 , 0x39},
	{0x0424 , 0x3E},
	{0x0425 , 0x4D},
	{0x0426 , 0x46},
	{0x0427 , 0x3A},
	{0x0428 , 0x21},
	{0x0429 , 0x21},
	{0x042A , 0x26},
	{0x042B , 0x1C},
	{0x042C , 0x25},
	{0x042D , 0x25},
	{0x042E , 0x28},
	{0x042F , 0x20},
	{0x0430 , 0x3E},
	{0x0431 , 0x3E},
	{0x0432 , 0x33},
	{0x0433 , 0x2E},
	{0x0434 , 0x54},
	{0x0435 , 0x53},
	{0x0436 , 0x3C},
	{0x0437 , 0x51},
	{0x0438 , 0x2B},
	{0x0439 , 0x2B},
	{0x043A , 0x38},
	{0x043B , 0x22},
	{0x043C , 0x3B},
	{0x043D , 0x3B},
	{0x043E , 0x31},
	{0x043F , 0x37},

	//PWB Gain
	{0x0440 , 0x00},
	{0x0441 , 0x4B},
	{0x0442 , 0x00},
	{0x0443 , 0x00},
	{0x0444 , 0x31},

	{0x0445 , 0x00},
	{0x0446 , 0x00},
	{0x0447 , 0x00},
	{0x0448 , 0x00},
	{0x0449 , 0x00},
	{0x044A , 0x00},
	{0x044D , 0xE0},
	{0x044E , 0x05},
	{0x044F , 0x07},
	{0x0450 , 0x00},
	{0x0451 , 0x00},
	{0x0452 , 0x00},
	{0x0453 , 0x00},
	{0x0454 , 0x00},
	{0x0455 , 0x00},
	{0x0456 , 0x00},
	{0x0457 , 0x00},
	{0x0458 , 0x00},
	{0x0459 , 0x00},
	{0x045A , 0x00},
	{0x045B , 0x00},
	{0x045C , 0x00},
	{0x045D , 0x00},
	{0x045E , 0x00},
	{0x045F , 0x00},

	//GAMMA Correction
	{0x0460 , 0x80},
	{0x0461 , 0x10},
	{0x0462 , 0x10},
	{0x0463 , 0x10},
	{0x0464 , 0x08},
	{0x0465 , 0x08},
	{0x0466 , 0x11},
	{0x0467 , 0x09},
	{0x0468 , 0x23},
	{0x0469 , 0x2A},
	{0x046A , 0x2A},
	{0x046B , 0x47},
	{0x046C , 0x52},
	{0x046D , 0x42},
	{0x046E , 0x36},
	{0x046F , 0x46},
	{0x0470 , 0x3A},
	{0x0471 , 0x32},
	{0x0472 , 0x32},
	{0x0473 , 0x38},
	{0x0474 , 0x3D},
	{0x0475 , 0x2F},
	{0x0476 , 0x29},
	{0x0477 , 0x48},

	{0x0686 , 0x6F},

	//Output Enable
	{0x0100 , 0x01},
	{0x0102 , 0x02},
	{0x0104 , 0x03},

	//GT2005_H_V_Switch
	{0x0101 , 0x00},


	{ 0xffff, 0xff },
};

#define C_GT2005_FMT_MAX	sizeof(g_GT2005_fmt)/sizeof(struct GT2005_fmt_s)
static struct GT2005_fmt_s g_GT2005_fmt[] = 
{
	/*0*/
	{
		.desc		= "MIPI Raw QXGA(2048x1536)",
		.pixelformat = V4L2_PIX_FMT_SBGGR8,
		.pInitRegs 	= mipi_raw_qxga,
		.pScaleRegs = NULL,
		.bpp 		= 2,
	},
	/*1*/
	{
		.desc		= "MIPI YVYU QXGA(2048x1536)",
		.pixelformat = V4L2_PIX_FMT_YVYU,
		.pInitRegs	= mipi_yuv_qxga,
		.pScaleRegs = NULL,
		.bpp		= 2,
	},
	/*2*/
	{
		.desc		= "MIPI Raw VGA(640x480)",
		.pixelformat = V4L2_PIX_FMT_SBGGR8,
		.pInitRegs 	= mipi_raw_qxga,
		.pScaleRegs = mipi_scale_vga,
		.bpp 		= 2,
	},
	/*3*/
	{
		.desc		= "MIPI YVYU VGA(640x480)",
		.pixelformat = V4L2_PIX_FMT_YVYU,
		.pInitRegs	= mipi_yuv_qxga,
		.pScaleRegs = mipi_scale_vga,
		.bpp		= 2,
	},
	/*4*/	
	{
		.desc		= "CCIR601 RAW QXGA(2048x1536)",
		.pixelformat = V4L2_PIX_FMT_SGRBG8,
		.pInitRegs 	= ccir601_raw_qxga,
		.pScaleRegs = NULL,
		.bpp		= 2,
	},
	/*5*/
	{
		.desc		= "CCIR601 YUV QXGA(2048x1536)",
		.pixelformat = V4L2_PIX_FMT_UYVY,
		.pInitRegs 	= ccir601_yuv_qxga,
		.pScaleRegs = NULL,
		.bpp		= 2,
	},
	/*6*/
	{
		.desc		= "CCIR601 RAW XGA(1024x768)",
		.pixelformat = V4L2_PIX_FMT_SGRBG8,
		.pInitRegs 	= ccir601_raw_qxga,
		.pScaleRegs = ccir601_scale_xga,
		.bpp		= 2,
	},
	/*7*/
	{
		.desc		= "CCIR601 YUV XGA(1024x768)",
		.pixelformat = V4L2_PIX_FMT_UYVY,
		.pInitRegs 	= ccir601_yuv_qxga,
		.pScaleRegs = NULL,//ccir601_scale_xga,
		.bpp		= 2,
	},
	/*8*/
	{
		.desc		= "CCIR601 RAW VGA(640x480)",//800*600
		.pixelformat = V4L2_PIX_FMT_SGRBG8,
		.pInitRegs 	= NULL,//ccir601_yuv_qxga,
		.pScaleRegs = ccir601_scale_vga,
		.bpp		= 2,
	},
};

static GT2005_dev_t	g_GT2005_dev;
#if I2C_USE_GPIO == 1
static int g_scl_handle, g_sda_handle;
#else
static int g_i2c_handle;
#endif

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
static void 
sccb_delay (
	int i
) 
{
	udelay(i*10);
}

#if I2C_USE_GPIO == 1
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

static int
sccb_read (
	unsigned short id,			
	unsigned short addr,		
	unsigned char *value
) 
{
	int nRet = 0;
	
	/* Data re-verification */
	id &= 0xFF;
	addr &= 0xFFFF;

	/* Serial bus output mode initialization */
	gp_gpio_set_output(g_scl_handle, 1, 0);
	gp_gpio_set_output(g_sda_handle, 1, 0);	

	/* 2-Phase write transmission cycle is starting now ...*/
	gp_gpio_set_value(g_scl_handle, 1);		/* SCL1	*/	
	gp_gpio_set_value(g_sda_handle, 0);		/* SDA0 */
	
	sccb_start ();							/* Transmission start */
	nRet = sccb_w_phase (id);				/* Phase 1 */
	if(nRet < 0) goto Return;
	nRet = sccb_w_phase(addr >> 8);			/* Phase 2 */
	if(nRet < 0) goto Return;
	nRet = sccb_w_phase(addr & 0xFF);						
	if(nRet < 0) goto Return;
	sccb_stop ();							/* Transmission stop */

	/* 2-Phase read transmission cycle is starting now ... */
	sccb_start ();							/* Transmission start */
	nRet = sccb_w_phase (id | 0x01);		/* Phase 1 (read) */
	if(nRet < 0) goto Return;
	*value = sccb_r_phase();				/* Phase 2 */

Return:
	sccb_stop ();							/* Transmission stop */
	return nRet;
}

static int 
sccb_write (
	unsigned short id,
	unsigned short addr,
	unsigned char data
) 
{
	int nRet = 0;
	
	/* Data re-verification */
	id &= 0xFF;
	addr &= 0xFFFF;
	data &= 0xFF;
	
	/* Serial bus output mode initialization */
	gp_gpio_set_output(g_scl_handle, 1, 0);
	gp_gpio_set_output(g_sda_handle, 1, 0);

	/* 3-Phase write transmission cycle is starting now ... */
	gp_gpio_set_value(g_scl_handle, 1);		/* SCL1 */		
	gp_gpio_set_value(g_sda_handle, 0);		/* SDA0 */
	sccb_start();							/* Transmission start */

	nRet = sccb_w_phase(id);				/* Phase 1 */
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

static int 
sensor_read(
	unsigned int reg,
	unsigned char *value
)
{
#if I2C_USE_GPIO == 1
	return sccb_read(GT2005_ID, reg, value);
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
	unsigned int reg,
	unsigned char value
)
{	
#if I2C_USE_GPIO == 1
	return sccb_write(GT2005_ID, reg, value);
#else
	char data[3];
	
	data[0] = (reg >> 8) & 0xFF;
	data[1] = reg & 0xFF;
	data[2] = value;	
	return gp_i2c_bus_write(g_i2c_handle, data, 3);
#endif
}

static int
GT2005_write_table(
	struct regval_list_s *vals
)
{
	int i, nRet;
	
	while (vals->reg_num != 0xffff || vals->value != 0xff) 
	{
		for(i = 0; i< 10; i++)
		{
			nRet = sensor_write(vals->reg_num, vals->value);
			if(nRet >= 0)
			{
			#if 0
				unsigned char value;
				sensor_read(vals->reg_num, &value);
				printk("0x%x, 0x%x\n", vals->reg_num, value);
			#endif
				break;
			}
			else
				printk("I2C Fail\n");
		}
		if(i == 10) return -1;
		vals++;
	}
	return 0;
}

static int 
gp_GT2005_s_interface(
	struct v4l2_subdev *sd,
	struct v4l2_interface *interface
)
{
	return 0;
}

static int 
gp_GT2005_enum_fmt(
	struct v4l2_subdev *sd, 
	struct v4l2_fmtdesc *fmtdesc
)
{
	if(fmtdesc->index >= C_GT2005_FMT_MAX)
		return -EINVAL;

	fmtdesc->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	memcpy((void *)fmtdesc->description, (void *)g_GT2005_fmt[fmtdesc->index].desc, 10);
	fmtdesc->pixelformat = g_GT2005_fmt[fmtdesc->index].pixelformat;
	return 0;
}


static int gp_GT2005_try_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_format *fmt
)
{
	int i;
	struct v4l2_pix_format *pix = &fmt->fmt.pix;

	for(i=0; i<C_GT2005_FMT_MAX; i++)
	{
		if (g_GT2005_fmt[i].pixelformat == pix->pixelformat)
			break;
	}
	
	if(i == C_GT2005_FMT_MAX)
		return -1;
	
	pix->width = GT2005_WIDTH;
	pix->height = GT2005_HEIGHT;
	pix->bytesperline = GT2005_WIDTH * g_GT2005_fmt[i].bpp;
	pix->sizeimage = pix->bytesperline * GT2005_HEIGHT;
	return 0;
}

static int 
gp_GT2005_s_fmt(
	struct v4l2_subdev *sd, 
	struct v4l2_format *fmt
)
{
	int nRet = 0, N = -1;

	g_GT2005_dev.width = fmt->fmt.pix.width;
	g_GT2005_dev.height = fmt->fmt.pix.height;
	if(fmt->fmt.pix.priv == C_CDSP_MIPI)
	{
		switch(fmt->fmt.pix.pixelformat)
		{
		case V4L2_PIX_FMT_SBGGR8:
			if(g_GT2005_dev.width == GT2005_WIDTH) N = 0;
			else if(g_GT2005_dev.width == 640) N = 2;
			else nRet = -EINVAL;
			break;
			
		case V4L2_PIX_FMT_YVYU:
			if(g_GT2005_dev.width == GT2005_WIDTH) N = 1;
			else if(g_GT2005_dev.width == 640) N = 3;
			else nRet = -EINVAL;
			break;
			
		default:
			nRet = -EINVAL;
		}
	}
	else if(fmt->fmt.pix.priv == C_CDSP_FRONT)
	{
		switch(fmt->fmt.pix.pixelformat)
		{
		case V4L2_PIX_FMT_SGRBG8:
			if(g_GT2005_dev.width == GT2005_WIDTH) N = 4;
			else if(g_GT2005_dev.width == 1024) N = 6;
			else if(g_GT2005_dev.width == 640) N = 8;
			else nRet = -EINVAL;
			break;

		case V4L2_PIX_FMT_UYVY:
			if(g_GT2005_dev.width == GT2005_WIDTH) N = 7;
			else if(g_GT2005_dev.width == 1024) N = 5;
			else if(g_GT2005_dev.width == 640) N = 8;
			else nRet = -EINVAL;
			printk("g_GT2005_dev.width=%d\n",g_GT2005_dev.width);
			break;

		default:
			nRet = -EINVAL;
		}
	}
	else
	{
		nRet = -EINVAL;
	}

	if(N >= 0)
	{
		printk("GT2005Fmt:%s\n", g_GT2005_fmt[N].desc);
		g_GT2005_dev.fmt = &g_GT2005_fmt[N];
	}
	
	return nRet;
}

static int 
gp_GT2005_g_fmt(
	struct v4l2_subdev *sd, 
	struct v4l2_format *fmt
)
{
	fmt->fmt.pix.width = g_GT2005_dev.width;
	fmt->fmt.pix.height = g_GT2005_dev.height;
	fmt->fmt.pix.pixelformat = g_GT2005_dev.fmt->pixelformat;
	fmt->fmt.pix.field = V4L2_FIELD_NONE;
	fmt->fmt.pix.bytesperline = g_GT2005_dev.width * g_GT2005_dev.fmt->bpp;
	fmt->fmt.pix.sizeimage = fmt->fmt.pix.bytesperline * g_GT2005_dev.height;
	return 0;
}

static int 
gp_GT2005_cropcap(
	struct v4l2_subdev *sd,
	struct v4l2_cropcap *cc
)
{
	return 0;
}


static int 
gp_GT2005_g_crop(
	struct v4l2_subdev *sd,
	struct v4l2_crop *crop
)
{
	return 0;
}

static int 
gp_GT2005_s_crop(
	struct v4l2_subdev *sd,
	struct v4l2_crop *crop
)
{
	return 0;
}

static int 
gp_GT2005_g_parm(
	struct v4l2_subdev *sd,
	struct v4l2_streamparm *parms
)
{
	return 0;
}

static int 
gp_GT2005_s_parm(
	struct v4l2_subdev *sd,
	struct v4l2_streamparm *param
)
{
	return 0;
}

static int 
gp_GT2005_queryctrl(
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
gp_GT2005_g_ctrl(
	struct v4l2_subdev *sd, 
	struct v4l2_control *ctrl
)
{	
	unsigned char data;
	int nRet = 0;
	printk("_____gp_GT2005_g_ctrl___\n");
	return 0;
	switch(ctrl->id)
	{
	case V4L2_CID_AUTO_WHITE_BALANCE:
		printk("WBAUTO.\n");
		nRet = sensor_read(0x3302, (unsigned char *)&data);
		printk("0x3302 =%x\n", data);
		if(data & 0x01)
		{
			ctrl->value = 1;
		}
		else
		{
			ctrl->value = 0;
		}
		break;
	
	case V4L2_CID_POWER_LINE_FREQUENCY:
		printk("BandingMode.\n");
		nRet = sensor_read(0x3014, (unsigned char *)&data);
		printk("0x3014 =%x\n", data);
		if(data & 0x80)
		{
			ctrl->value = V4L2_CID_POWER_LINE_FREQUENCY_50HZ;	
		}
		else
		{
			ctrl->value = V4L2_CID_POWER_LINE_FREQUENCY_60HZ;	
		}
		break;
			
	case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
		
		break;

	case V4L2_CID_BACKLIGHT_COMPENSATION:	
		printk("NightMode.\n");
		nRet = sensor_read(0x3014, (unsigned char *)&data);
		printk("0x3014 =%x\n", data);
		if(data & 0x08)
		{
			ctrl->value = 1; 
		}
		else
		{
			ctrl->value = 0; 
		}
		break;
		
	default:
		return -EINVAL;
	}
	return nRet; 
}

static int 
gp_GT2005_s_ctrl(
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
		if(ctrl->value)
		{
			nRet = sensor_write(0x031a,0x81);   // select Auto WB
			nRet = sensor_write(0x0320,0x24);//  WBGRMAX[7:0];
			nRet = sensor_write(0x0321,0x14);//  WBGRMIN[7:0];
			nRet = sensor_write(0x0322,0x24);//  WBGBMAX[7:0];
			nRet = sensor_write(0x0323,0x1a);//  WBGBMIN[7:0];
			nRet = sensor_write(0x0441,0x4B);//  PWBGAINR[7:0];
			nRet = sensor_write(0x0442,0x00);//  PWBGAINGR[7:0];
			nRet = sensor_write(0x0443,0x00);//  PWBGAINGB[7:0];
			nRet = sensor_write(0x0444,0x31);//  PWBGAINB[7:0];
		}
		else
		{
			nRet = sensor_read(0x031a, (unsigned char *)&data);			
			nRet = sensor_write(0x031a, (data &= ~0x80));
		}
		break;

	case V4L2_CID_POWER_LINE_FREQUENCY:
		printk("PL = %d\n", ctrl->value);
		if(ctrl->value == V4L2_CID_POWER_LINE_FREQUENCY_DISABLED)
		{

		}
		else if(ctrl->value == V4L2_CID_POWER_LINE_FREQUENCY_50HZ)
		{
			nRet = sensor_write(0x0315, 0x16);                  			
			nRet = sensor_write(0x0313, 0x35); 
			nRet = sensor_write(0x0314, 0x36); 
		}
		else if(ctrl->value == V4L2_CID_POWER_LINE_FREQUENCY_60HZ)
		{
			nRet = sensor_write(0x0315, 0x56);                  			
			nRet = sensor_write(0x0313, 0x35); 
			nRet = sensor_write(0x0314, 0x36); 
		}
		else 
		{
			return -EINVAL;
		}
		break;

	case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
		printk("WB = %d\n", ctrl->value);
		if(ctrl->value == 0) //SUNSHINE
		{
			nRet = sensor_write(0x0320 ,0x02);
			nRet = sensor_write(0x0321 ,0x02);
			nRet = sensor_write(0x0322 ,0x02);
			nRet = sensor_write(0x0323 ,0x02);
			nRet = sensor_write(0x0441 ,0x60);
			nRet = sensor_write(0x0442 ,0x00);
			nRet = sensor_write(0x0443 ,0x00);
			nRet = sensor_write(0x0444 ,0x14);
		}
		else if(ctrl->value == 1)//CLOUDY
		{
			nRet = sensor_write(0x0320 ,0x02);//  WBGRMAX[7:0];
			nRet = sensor_write(0x0321 ,0x02);//  WBGRMIN[7:0];
			nRet = sensor_write(0x0322 ,0x02);//  WBGBMAX[7:0];
			nRet = sensor_write(0x0323 ,0x02);//  WBGBMIN[7:0];
			nRet = sensor_write(0x0441 ,0x72);//  PWBGAINR[7:0];
			nRet = sensor_write(0x0442 ,0x00);//  PWBGAINGR[7:0];
			nRet = sensor_write(0x0443 ,0x00);//  PWBGAINGB[7:0];
			nRet = sensor_write(0x0444 ,0x0D);//  PWBGAINB[7:0];
		}
		else if(ctrl->value == 2)//FLUORESCENCE
		{
		
		}
		else if(ctrl->value == 3)//INCANDESCENCE
		{
		
		}
		break;

	case V4L2_CID_BACKLIGHT_COMPENSATION:
		printk("NightMode = %d\n", ctrl->value);
		if(ctrl->value)
		{
			nRet = sensor_write(0x0312 , 0x98);//set fps/2
		}
		else
		{
			nRet = sensor_write(0x0312 , 0x08); //Disable night mode  Frame rate do not change);//set fps
		}
		break;
		
	default:
		return -EINVAL;
	}
	
	return nRet; 
}

static int 
gp_GT2005_reset(
	struct v4l2_subdev *sd, 
	u32 val
)
{

	return 0;
}

static int 
gp_GT2005_init(
	struct v4l2_subdev *sd,
	u32 val
)
{
	int nRet;

	if(g_GT2005_dev.fmt->pInitRegs)
	{
		printk("GT2005InitReg\n");
		nRet = GT2005_write_table(g_GT2005_dev.fmt->pInitRegs);
		if(nRet < 0) return -1;
	}
	
	if(g_GT2005_dev.fmt->pScaleRegs)
	{
		printk("GT2005ScaleReg\n");
		nRet = GT2005_write_table(g_GT2005_dev.fmt->pScaleRegs);
		if(nRet < 0) return -1;
	}
	return 0;
}

static int 
gp_GT2005_suspend(
	struct v4l2_subdev *sd
)
{
	return 0;
}

static int 
gp_GT2005_resume(
	struct v4l2_subdev *sd
)
{
	return 0;
}

static const struct v4l2_subdev_core_ops cdsp_GT2005_core_ops = 
{
	.g_ctrl = gp_GT2005_g_ctrl,
	.s_ctrl = gp_GT2005_s_ctrl,
	.queryctrl = gp_GT2005_queryctrl,
	.reset = gp_GT2005_reset,
	.init = gp_GT2005_init,
};

static const struct v4l2_subdev_video_ops cdsp_GT2005_video_ops = 
{
	.enum_fmt = gp_GT2005_enum_fmt,
	.try_fmt = gp_GT2005_try_fmt,
	.g_fmt = gp_GT2005_g_fmt,
	.s_fmt = gp_GT2005_s_fmt,
	.s_parm = gp_GT2005_s_parm,
	.g_parm = gp_GT2005_g_parm,
	.cropcap = gp_GT2005_cropcap,
	.g_crop = gp_GT2005_g_crop,
	.s_crop = gp_GT2005_s_crop,	
};

static const struct v4l2_subdev_ext_ops cdsp_GT2005_ext_ops = 
{
	.s_interface = gp_GT2005_s_interface,
	.suspend = gp_GT2005_suspend,
	.resume = gp_GT2005_resume,
};

static const struct v4l2_subdev_ops cdsp_GT2005_ops = 
{
	.core = &cdsp_GT2005_core_ops,
	.video = &cdsp_GT2005_video_ops,
	.ext = &cdsp_GT2005_ext_ops
};

static int __init 
GT2005_init_module(
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
	g_i2c_handle = gp_i2c_bus_request(GT2005_ID, 100);	/*100KHZ*/
	if((g_i2c_handle == 0) ||(g_i2c_handle == -ENOMEM))
	{
		printk(KERN_WARNING "i2cReqFail %d\n", g_i2c_handle);
		return -1;
	}
#endif

	printk(KERN_WARNING "ModuleInit: cdsp_GT2005 \n");
	g_GT2005_dev.fmt = &g_GT2005_fmt[0];
	g_GT2005_dev.width = GT2005_WIDTH;
	g_GT2005_dev.height = GT2005_HEIGHT;
	v4l2_subdev_init(&(g_GT2005_dev.sd), &cdsp_GT2005_ops);
	strcpy(g_GT2005_dev.sd.name, "cdsp_GT2005");
	register_sensor(&g_GT2005_dev.sd, (int *)&param[0]);
	return 0;
}

static void __exit
GT2005_module_exit(
		void
) 
{
#if I2C_USE_GPIO == 1	
	gp_gpio_release(g_scl_handle);
	gp_gpio_release(g_sda_handle);	
#else
	gp_i2c_bus_release(g_i2c_handle);
#endif	
	unregister_sensor(&(g_GT2005_dev.sd));
}

module_init(GT2005_init_module);
module_exit(GT2005_module_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/
MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("Generalplus cdsp GT2005 Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");



