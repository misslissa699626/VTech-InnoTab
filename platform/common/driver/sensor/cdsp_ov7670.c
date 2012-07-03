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
 *                         H E A D E R   F I L E S												*
 **************************************************************************/
#include <linux/module.h>
#include <linux/fs.h> /* everything... */
#include <linux/videodev2.h>
#include <media/v4l2-device.h>

#include <linux/delay.h>
#include <mach/gp_gpio.h>
#include <mach/gp_i2c_bus.h>
#include <mach/sensor_mgr.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define OV7670_ID					0x42
#define OV7670_WIDTH				640
#define OV7670_HEIGHT				480

#define OV7670_YUYV					0x00
#define OV7670_BAYER_RAW			0x01
#define COLOR_BAR					0x02
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
	unsigned char reg_num;
	unsigned char value;
};

struct ov7670_fmt_s
{
	__u8 *desc;
	__u32 pixelformat;
	struct regval_list_s *regs;
	int cmatrix[6];
	int bpp;					/* Bytes per pixel */
}; 

typedef struct ov7670_dev_s 
{
	struct v4l2_subdev sd;
	struct ov7670_fmt_s *fmt;	/* Current format */
	short width;
	short height;
	unsigned char sat;			/* Saturation value */
	int hue;					/* Hue value */
}ov7670_dev_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
#if 0
static unsigned char OV7670_Gamma_TBLValue[512] =
{ 
	0x00, 0x50, 0x15, 0x00,
	0x06, 0x55, 0x11, 0x00,
	0x0d, 0x45, 0x15, 0x00,
	0x13, 0x55, 0x14, 0x00,
	0x1a, 0x51, 0x14, 0x00,
	0x1f, 0x15, 0x05, 0x00,
	0x25, 0x51, 0x11, 0x00,
	0x2a, 0x11, 0x05, 0x00,
	0x2f, 0x14, 0x11, 0x00,
	0x33, 0x11, 0x11, 0x00,
	0x37, 0x44, 0x10, 0x00,
	0x3a, 0x41, 0x04, 0x00,
	0x3e, 0x44, 0x10, 0x00,
	0x41, 0x41, 0x04, 0x00,
	0x44, 0x11, 0x11, 0x00,
	0x48, 0x44, 0x04, 0x00,
	0x4c, 0x10, 0x11, 0x00,
	0x4f, 0x41, 0x04, 0x00,
	0x52, 0x11, 0x04, 0x00,
	0x56, 0x10, 0x11, 0x00,
	0x59, 0x41, 0x04, 0x00,
	0x5c, 0x11, 0x04, 0x00,
	0x60, 0x10, 0x01, 0x00,
	0x63, 0x10, 0x01, 0x00,
	0x66, 0x04, 0x01, 0x00,
	0x69, 0x04, 0x01, 0x00,
	0x6c, 0x10, 0x01, 0x00,
	0x6f, 0x10, 0x04, 0x00,
	0x72, 0x10, 0x04, 0x00,
	0x74, 0x41, 0x10, 0x00,
	0x77, 0x04, 0x01, 0x00,
	0x7a, 0x10, 0x04, 0x00,
	0x7c, 0x41, 0x10, 0x00,
	0x7f, 0x04, 0x01, 0x00,
	0x82, 0x40, 0x10, 0x00,
	0x84, 0x04, 0x01, 0x00,
	0x86, 0x41, 0x10, 0x00,
	0x89, 0x10, 0x04, 0x00,
	0x8b, 0x01, 0x01, 0x00,
	0x8e, 0x40, 0x10, 0x00,
	0x90, 0x10, 0x04, 0x00,
	0x92, 0x04, 0x04, 0x00,
	0x94, 0x01, 0x01, 0x00,
	0x96, 0x01, 0x01, 0x00,
	0x98, 0x41, 0x00, 0x00,
	0x9b, 0x40, 0x00, 0x00,
	0x9d, 0x40, 0x00, 0x00,
	0x9f, 0x40, 0x00, 0x00,
	0xa0, 0x01, 0x01, 0x00,
	0xa2, 0x01, 0x01, 0x00,
	0xa4, 0x04, 0x04, 0x00,
	0xa6, 0x04, 0x10, 0x00,
	0xa8, 0x10, 0x00, 0x00,
	0xaa, 0x00, 0x01, 0x00,
	0xab, 0x01, 0x04, 0x00,
	0xad, 0x10, 0x00, 0x00,
	0xaf, 0x40, 0x00, 0x00,
	0xb0, 0x01, 0x04, 0x00,
	0xb2, 0x10, 0x00, 0x00,
	0xb3, 0x01, 0x04, 0x00,
	0xb5, 0x10, 0x00, 0x00,
	0xb7, 0x00, 0x04, 0x00,
	0xb8, 0x10, 0x00, 0x00,
	0xb9, 0x01, 0x04, 0x00,
	0xbb, 0x40, 0x00, 0x00,
	0xbc, 0x04, 0x00, 0x00,
	0xbe, 0x00, 0x01, 0x00,
	0xbf, 0x10, 0x00, 0x00,
	0xc0, 0x01, 0x00, 0x00,
	0xc2, 0x00, 0x04, 0x00,
	0xc3, 0x40, 0x00, 0x00,
	0xc4, 0x04, 0x00, 0x00,
	0xc5, 0x01, 0x00, 0x00,
	0xc7, 0x00, 0x04, 0x00,
	0xc8, 0x00, 0x01, 0x00,
	0xc9, 0x40, 0x00, 0x00,
	0xca, 0x10, 0x00, 0x00,
	0xcb, 0x04, 0x00, 0x00,
	0xcc, 0x01, 0x00, 0x00,
	0xcd, 0x01, 0x04, 0x00,
	0xcf, 0x10, 0x00, 0x00,
	0xd1, 0x00, 0x01, 0x00,
	0xd2, 0x00, 0x04, 0x00,
	0xd3, 0x00, 0x04, 0x00,
	0xd4, 0x00, 0x04, 0x00,
	0xd5, 0x00, 0x01, 0x00,
	0xd6, 0x00, 0x01, 0x00,
	0xd7, 0x00, 0x01, 0x00,
	0xd8, 0x00, 0x01, 0x00,
	0xd9, 0x00, 0x01, 0x00,
	0xda, 0x10, 0x00, 0x00,
	0xdc, 0x00, 0x01, 0x00,
	0xdd, 0x04, 0x10, 0x00,
	0xdf, 0x00, 0x00, 0x00,
	0xe0, 0x00, 0x00, 0x00,
	0xe1, 0x00, 0x00, 0x00,
	0xe2, 0x00, 0x01, 0x00,
	0xe3, 0x04, 0x00, 0x00,
	0xe5, 0x00, 0x00, 0x00,
	0xe6, 0x00, 0x00, 0x00,
	0xe6, 0x04, 0x00, 0x00,
	0xe8, 0x00, 0x04, 0x00,
	0xe9, 0x40, 0x00, 0x00,
	0xea, 0x00, 0x01, 0x00,
	0xeb, 0x00, 0x01, 0x00,
	0xec, 0x00, 0x01, 0x00,
	0xed, 0x40, 0x00, 0x00,
	0xee, 0x00, 0x00, 0x00,
	0xef, 0x00, 0x00, 0x00,
	0xef, 0x40, 0x00, 0x00,
	0xf0, 0x04, 0x00, 0x00,
	0xf2, 0x00, 0x04, 0x00,
	0xf3, 0x00, 0x00, 0x00,
	0xf3, 0x10, 0x00, 0x00,
	0xf4, 0x40, 0x00, 0x00,
	0xf5, 0x40, 0x00, 0x00,
	0xf6, 0x40, 0x00, 0x00,
	0xf7, 0x00, 0x00, 0x00,
	0xf7, 0x01, 0x00, 0x00,
	0xf8, 0x40, 0x00, 0x00,
	0xf9, 0x04, 0x00, 0x00,
	0xfb, 0x00, 0x04, 0x00,
	0xfc, 0x00, 0x00, 0x00,
	0xfc, 0x10, 0x00, 0x00,
	0xfd, 0x00, 0x10, 0x00,
	0xfe, 0x00, 0x00, 0x00,
	0xfe, 0x00, 0x10, 0x00,
	0xff, 0x00, 0x00, 0x00,
};

static unsigned char OV7670_Edge_TBLValue[256] = 
{
	0x00,0x01,0x02,0x04,0x07,0x0a,0x0e,0x12,0x17,0x1d,0x23,0x29,0x31,0x38,0x41,0x4a,
	0x53,0x5d,0x68,0x73,0x7f,0x8b,0x98,0xa5,0xb3,0xc2,0xd0,0xe0,0xef,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfd,0xfb,0xf8,0xf6,0xf4,0xf1,0xef,0xed,
	0xeb,0xe9,0xe6,0xe4,0xe2,0xe0,0xde,0xdc,0xda,0xd8,0xd6,0xd5,0xd3,0xd1,0xcf,0xcd,
	0xcb,0xca,0xc8,0xc6,0xc5,0xc3,0xc1,0xc0,0xbe,0xbc,0xbb,0xb9,0xb8,0xb6,0xb5,0xb3,
	0xb2,0xb0,0xaf,0xae,0xac,0xab,0xa9,0xa8,0xa7,0xa5,0xa4,0xa3,0xa2,0xa0,0x9f,0x9e
};

static unsigned short Ov7670_LensCmp_TblValue[256]=
{	
	0x100,0x101,0x102,0x103,0x104,0x105,0x106,0x107,0x108,0x109,0x10A,0x10B,0x10C,0x10D,0x10E,0x10F,
	0x110,0x111,0x112,0x113,0x114,0x115,0x116,0x117,0x118,0x119,0x11A,0x11B,0x11C,0x11D,0x11E,0x11F,
	0x120,0x121,0x122,0x123,0x124,0x125,0x126,0x127,0x128,0x129,0x12A,0x12B,0x12C,0x12D,0x12E,0x12F,
	0x130,0x131,0x132,0x133,0x134,0x135,0x136,0x137,0x138,0x139,0x13A,0x13B,0x13C,0x13D,0x13E,0x13F,
	0x140,0x141,0x142,0x143,0x144,0x145,0x146,0x147,0x148,0x149,0x14A,0x14B,0x14C,0x14D,0x14E,0x14F,
	0x150,0x151,0x152,0x153,0x154,0x155,0x156,0x157,0x158,0x159,0x15A,0x15B,0x15C,0x15D,0x15E,0x15F,
	0x160,0x161,0x162,0x163,0x164,0x165,0x166,0x167,0x168,0x169,0x16A,0x16B,0x16C,0x16D,0x16E,0x16F,
	0x170,0x171,0x172,0x173,0x174,0x175,0x176,0x177,0x178,0x179,0x17A,0x17B,0x17C,0x17D,0x17E,0x17F,
	0x180,0x181,0x182,0x183,0x184,0x185,0x186,0x187,0x188,0x189,0x18A,0x18B,0x18C,0x18D,0x18E,0x18F,
	0x190,0x191,0x192,0x193,0x194,0x195,0x196,0x197,0x198,0x199,0x19A,0x19B,0x19C,0x19D,0x19E,0x19F,
	0x1A0,0x1A1,0x1A2,0x1A3,0x1A4,0x1A5,0x1A6,0x1A7,0x1A8,0x1A9,0x1AA,0x1AB,0x1AC,0x1AD,0x1AE,0x1AF,
	0x1B0,0x1B1,0x1B2,0x1B3,0x1B4,0x1B5,0x1B6,0x1B7,0x1B8,0x1B9,0x1BA,0x1BB,0x1BC,0x1BD,0x1BE,0x1BF,
	0x1C0,0x1C1,0x1C2,0x1C3,0x1C4,0x1C5,0x1C6,0x1C7,0x1C8,0x1C9,0x1CA,0x1CB,0x1CC,0x1CD,0x1CE,0x1CF,
	0x1D0,0x1D1,0x1D2,0x1D3,0x1D4,0x1D5,0x1D6,0x1D7,0x1D8,0x1D9,0x1DA,0x1DB,0x1DC,0x1DD,0x1DE,0x1DF,
	0x1E0,0x1E1,0x1E2,0x1E3,0x1E4,0x1E5,0x1E6,0x1E7,0x1E8,0x1E9,0x1EA,0x1EB,0x1EC,0x1ED,0x1EE,0x1EF,
	0x1F0,0x1F1,0x1F2,0x1F3,0x1F4,0x1F5,0x1F6,0x1F7,0x1F8,0x1F9,0x1FA,0x1FB,0x1FC,0x1FD,0x1FE,0x1FF,
};
#endif

static struct regval_list_s ov7670_init_table[] = 
{
#if 1
	{ 0x11, 0x80 },	/* pclk bypass */
#else
	{ 0x11, 0x00 },	/* pclk div */
#endif
	{ 0x15, 0x46 },	/* use Hsync mode and reverse Vsync */
#if COLOR_BAR_EN == 1
	{ 0x42, 0x08 },
	{ 0x12, OV7670_BAYER_RAW | COLOR_BAR },
#else
	{ 0x12, OV7670_BAYER_RAW },
#endif
	{ 0x17, 0x12 }, /* h set */	
	{ 0x18, 0x01 },
	{ 0x32, 0x04 },
	
	{ 0x19, 0x01 }, /* v set */
	{ 0x1a, 0x7a },
	{ 0x03, 0xce },
	{ 0x2b, 0x08 }, /* horizontal insert dummy pixel */
	
	{ 0x0c, 0x00 },
	{ 0x3e, 0x00 },
	{ 0x70, 0x3a },
	{ 0x71, 0x35 },
	{ 0x72, 0x11 },
	{ 0x73, 0xf0 },
	{ 0xa2, 0x29 },
	
	{ 0x7a, 0x24 },
	{ 0x7b, 0x04 },
	{ 0x7c, 0x0a },
	{ 0x7d, 0x17 },
	{ 0x7e, 0x32 },
	{ 0x7f, 0x3f },
	{ 0x80, 0x4c },
	{ 0x81, 0x58 },
	{ 0x82, 0x64 },
	{ 0x83, 0x6f },
	{ 0x84, 0x7a },
	{ 0x85, 0x8c },
	{ 0x86, 0x9e },
	{ 0x87, 0xbb },
	{ 0x88, 0xd2 },
	{ 0x89, 0xe5 },
	
	{ 0x13, 0xe0 },
	{ 0x00, 0x00 },
	{ 0x10, 0x00 },
	{ 0x0d, 0x40 },
	{ 0x14, 0x18 },
	{ 0xa5, 0x02 },
	{ 0xab, 0x03 },
	{ 0x24, 0x95 },
	{ 0x25, 0x33 },
	{ 0x26, 0xe3 },
	{ 0x9f, 0x78 },
	{ 0xa0, 0x68 },
	{ 0xa1, 0x03 },
	{ 0xa6, 0xd8 },
	{ 0xa7, 0xd8 },
	{ 0xa8, 0xf0 },
	{ 0xa9, 0x90 },
	{ 0xaa, 0x94 },
	{ 0x13, 0xe5 },
	
	{ 0x0e, 0x61 },
	{ 0x0f, 0x4b },
	{ 0x16, 0x02 },
	{ 0x1e, 0x3f },	/* Flip & Mirror */
	{ 0x21, 0x02 },
	{ 0x22, 0x91 },
	{ 0x29, 0x07 },
	{ 0x33, 0x0b },
	{ 0x35, 0x0b },
	{ 0x37, 0x1d },
	{ 0x38, 0x71 },
	{ 0x39, 0x2a },
	{ 0x3c, 0x78 },
	{ 0x4d, 0x40 },
	{ 0x4e, 0x20 },
	{ 0x69, 0x00 },	
#if 1	
	{ 0x6b, 0x0a },		
#elif 0
	{ 0x6b, 0x4a },	/* pclk*4 */	
#elif 0
	{ 0x6b, 0x8a },	/* pclk*6 */	
#else
	{ 0x6b, 0xca },	/* pclk*8 */	
#endif
	{ 0x74, 0x10 },
	{ 0x8d, 0x4f },
	{ 0x8e, 0x00 },
	{ 0x8f, 0x00 },
	{ 0x90, 0x00 },
	{ 0x91, 0x00 },
	{ 0x96, 0x00 },
	{ 0x9a, 0x80 },
	{ 0xb0, 0x84 },
	{ 0xb1, 0x0c },
	{ 0xb2, 0x0e },
	{ 0xb3, 0x82 },
	{ 0xb8, 0x0a },
	{ 0x43, 0x0a },
	{ 0x44, 0xf0 },
	{ 0x45, 0x44 },
	{ 0x46, 0x7a },
	{ 0x47, 0x27 },
	{ 0x48, 0x3c },
	{ 0x59, 0xbc },
	{ 0x5a, 0xde },
	{ 0x5b, 0x54 },
	{ 0x5c, 0x8a },
	{ 0x5d, 0x4b },
	{ 0x5e, 0x0f },
	{ 0x6c, 0x0a },
	{ 0x6d, 0x55 },
	{ 0x6e, 0x11 },
	{ 0x6f, 0x9e },
	{ 0x6a, 0x40 },
	{ 0x01, 0x40 },
	{ 0x02, 0x40 },
	{ 0x13, 0xe7 },
	{ 0x4f, 0x80 },
	{ 0x50, 0x80 },
	{ 0x51, 0x00 },
	{ 0x52, 0x22 },
	{ 0x53, 0x5e },
	{ 0x54, 0x80 },
	{ 0x58, 0x9e },
	{ 0x62, 0x08 },
	{ 0x63, 0x8f },
	{ 0x65, 0x00 },
	{ 0x64, 0x08 },
	{ 0x94, 0x08 },
	{ 0x95, 0x0c },
	{ 0x66, 0x05 },
	{ 0x41, 0x08 },
	{ 0x3f, 0x00 },
	{ 0x75, 0x05 },
	{ 0x76, 0xe1 },
	{ 0x4c, 0x00 },
	{ 0x77, 0x01 },
	{ 0x3d, 0x08 },	/* disable gamma and uv saturation */
	{ 0x4b, 0x09 },
	{ 0xc9, 0x60 },
	{ 0x41, 0x38 },
	{ 0x56, 0x40 },
	{ 0x34, 0x11 },
	{ 0x3b, 0x4a },	/* disable night mode */
	{ 0xa4, 0x88 },
	{ 0x96, 0x00 },
	{ 0x97, 0x30 },
	{ 0x98, 0x20 },
	{ 0x99, 0x30 },
	{ 0x9a, 0x84 },
	{ 0x9b, 0x29 },
	{ 0x9c, 0x03 },
	{ 0x9d, 0x98 },
	{ 0x9e, 0x7f },
	{ 0x78, 0x04 },
	{ 0x79, 0x01 },
	{ 0xc8, 0xf0 },
	{ 0x79, 0x0f },
	{ 0xc8, 0x00 },
	{ 0x79, 0x10 },
	{ 0xc8, 0x7e },
	{ 0x79, 0x0a },
	{ 0xc8, 0x80 },
	{ 0x79, 0x0b },
	{ 0xc8, 0x01 },
	{ 0x79, 0x0c },
	{ 0xc8, 0x0f },
	{ 0x79, 0x0d },
	{ 0xc8, 0x20 },
	{ 0x79, 0x09 },
	{ 0xc8, 0x80 },
	{ 0x79, 0x02 },
	{ 0xc8, 0xc0 },
	{ 0x79, 0x03 },
	{ 0xc8, 0x40 },
	{ 0x79, 0x05 },
	{ 0xc8, 0x30 },
	{ 0x79, 0x26 },
	{ 0x3b, 0x08 },/* 60Hz = 0x00, 50Hz = 0x08 */
	{ 0xff, 0xff },
};

static struct regval_list_s ccir601_raw[] = 
{
	{ 0x12, OV7670_BAYER_RAW },
	{ 0xff, 0xff },
};

static struct regval_list_s ccir601_yuv422[] = 
{
	{ 0x12, OV7670_YUYV },  /* Selects YUV mode */
	{ 0xff, 0xff },
};

#define C_OV7670_FMT_MAX	2
static struct ov7670_fmt_s g_ov7670_fmt[] = 
{
	{
		.desc		= "Bayer Raw",
		.pixelformat = V4L2_PIX_FMT_SBGGR8,
		.regs 		= ccir601_raw,
		.cmatrix	= { 0, 0, 0, 0, 0, 0 },
		.bpp 		= 1
	},
	{
		.desc		= "YUYV422",
		.pixelformat = V4L2_PIX_FMT_YUYV,
		.regs 		= ccir601_yuv422,
		.cmatrix	= { 128, -128, 0, -34, -94, 128 },
		.bpp		= 2,
	},
};

static ov7670_dev_t g_ov7670_dev;
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
	unsigned char value
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
	// The 9th bit transmission
	gp_gpio_set_value(g_scl_handle, 0);				/* SCL0 */
	gp_gpio_set_input(g_sda_handle, GPIO_PULL_HIGH);/* SDA is Hi-Z mode */
	
	sccb_delay(2);
	gp_gpio_set_value(g_scl_handle,1);				/* SCL1 */
	sccb_delay(2);
	gp_gpio_set_value(g_scl_handle,0);				/* SCL0 */

	gp_gpio_get_value(g_sda_handle, &i);			/* check ACK */
	if(i != 0) nRet = -1;
	gp_gpio_set_output(g_sda_handle, 1, 0);			/* SDA is Hi-Z mode */
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
		gp_gpio_set_value(g_scl_handle,0);		/* SCL0 */
		sccb_delay(2);
		gp_gpio_set_value(g_scl_handle,1);		/* SCL1 */
		gp_gpio_get_value(g_sda_handle, &temp);
		data <<= 1;
		data |= temp;
		sccb_delay(2);
	}
	// The 9th bit transmission
	gp_gpio_set_value(g_scl_handle, 0);		/* SCL0 */
	gp_gpio_set_output(g_sda_handle, 1, 0);	/* SDA is output mode */
	gp_gpio_set_value(g_sda_handle, 1);		/* SDA0, the nighth bit is NA must be 1 */

	sccb_delay(2);
	gp_gpio_set_value(g_scl_handle,1);		/* SCL1 */
	sccb_delay(2);
	gp_gpio_set_value(g_scl_handle,0);		/* SCL0 */
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
	addr &= 0xFF;

	/* Serial bus output mode initialization */
	gp_gpio_set_output(g_scl_handle, 1, 0);
	gp_gpio_set_output(g_sda_handle, 1, 0);	

	/* 2-Phase write transmission cycle is starting now ...*/
	gp_gpio_set_value(g_scl_handle, 1);		/* SCL1 */		
	gp_gpio_set_value(g_sda_handle, 0);		/* SDA0 */
	
	sccb_start ();							/* Transmission start */
	nRet = sccb_w_phase (id);				/* Phase 1 */
	if(nRet < 0) goto Return;
	nRet = sccb_w_phase (addr);				/* Phase 2 */
	if(nRet < 0) goto Return;
	sccb_stop ();							/* Transmission stop */

	/* 2-Phase read transmission cycle is starting now ...*/
	sccb_start ();							/* Transmission start*/
	nRet = sccb_w_phase (id | 0x01);		/* Phase 1 (read)*/
	if(nRet < 0) goto Return;
	*value = sccb_r_phase();				/* Phase 2*/

Return:
	sccb_stop ();							/* Transmission stop*/
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
	data &= 0xFF;

	/* Serial bus output mode initialization */
	gp_gpio_set_output(g_scl_handle, 1, 0);
	gp_gpio_set_output(g_sda_handle, 1, 0);

	/* 3-Phase write transmission cycle is starting now ...*/
	gp_gpio_set_value(g_scl_handle, 1);	/* SCL1 */	
	gp_gpio_set_value(g_sda_handle, 0);	/* SDA0 */
	sccb_start();						/* Transmission start */

	nRet = sccb_w_phase(id);			/* Phase 1 */
	if(nRet < 0) goto Return;
	nRet = sccb_w_phase(addr);			/* Phase 2 */
	if(nRet < 0) goto Return;
	nRet = sccb_w_phase(data);			/* Phase 3 */
	if(nRet < 0) goto Return;
	
Return:
	sccb_stop();						/* Transmission stop */
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
	return sccb_read(OV7670_ID, reg, value);
#else
	char addr[0], data[0];
	int nRet;
	
	addr[0] = reg & 0xFF;
	nRet = gp_i2c_bus_write(g_i2c_handle, addr, 1);
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
	return sccb_write(OV7670_ID, reg, value);
#else
	char data[3];
	
	data[0] = reg & 0xFF;
	data[1] = value;	
	return gp_i2c_bus_write(g_i2c_handle, data, 2);
#endif
}

static int
ov7670_write_table(
	struct regval_list_s *vals
)
{
	int i, nRet;
	
	while (vals->reg_num != 0xff || vals->value != 0xff) 
	{
		for(i=0; i<10; i++)
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
				printk("I2C fail\n");
		}	
		if(i == 10) return -1;
		vals++;
	}
	return 0;
}

static int 
gp_ov7670_s_interface(
	struct v4l2_subdev *sd,
	struct v4l2_interface *interface
)
{
	return 0;
}

static int 
gp_ov7670_enum_fmt(
	struct v4l2_subdev *sd, 
	struct v4l2_fmtdesc *fmtdesc
)
{
	if(fmtdesc->index >= C_OV7670_FMT_MAX)
		return -EINVAL;

	fmtdesc->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	memcpy((void *)fmtdesc->description, (void *)g_ov7670_fmt[fmtdesc->index].desc, 10);
	fmtdesc->pixelformat = g_ov7670_fmt[fmtdesc->index].pixelformat;
	return 0;
}

static int gp_ov7670_try_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_format *fmt
)
{
	int i;
	struct v4l2_pix_format *pix = &fmt->fmt.pix;

	for(i=0; i<C_OV7670_FMT_MAX; i++)
	{
		if (g_ov7670_fmt[i].pixelformat == pix->pixelformat)
			break;
	}
	
	if(i == C_OV7670_FMT_MAX)
		return -1;
	
	pix->width = OV7670_WIDTH;
	pix->height = OV7670_HEIGHT;
	pix->bytesperline = OV7670_WIDTH * g_ov7670_fmt[i].bpp;
	pix->sizeimage = pix->bytesperline * OV7670_HEIGHT;
	return 0;
}

static int 
gp_ov7670_s_fmt(
	struct v4l2_subdev *sd, 
	struct v4l2_format *fmt
)
{
	int i;
	
	printk(KERN_WARNING "Ov7670_S_Fmt\n");
	for(i=0; i<C_OV7670_FMT_MAX; i++)
	{
		if(fmt->fmt.pix.pixelformat == g_ov7670_fmt[i].pixelformat)
		{
			g_ov7670_dev.fmt = &g_ov7670_fmt[i];
			g_ov7670_dev.width = fmt->fmt.pix.width;
			g_ov7670_dev.height = fmt->fmt.pix.height;
			break;
		}
	}

	if(i == C_OV7670_FMT_MAX)
		return -EINVAL;
	
	return 0;
}

static int 
gp_ov7670_g_fmt(
	struct v4l2_subdev *sd, 
	struct v4l2_format *fmt
)
{
	printk(KERN_WARNING "Ov7670_G_Fmt\n");
	fmt->fmt.pix.width = g_ov7670_dev.width;
	fmt->fmt.pix.height = g_ov7670_dev.height;
	fmt->fmt.pix.pixelformat = g_ov7670_dev.fmt->pixelformat;
	fmt->fmt.pix.field = V4L2_FIELD_NONE;
	fmt->fmt.pix.bytesperline = g_ov7670_dev.width * g_ov7670_dev.fmt->bpp;
	fmt->fmt.pix.sizeimage = fmt->fmt.pix.bytesperline * g_ov7670_dev.height;
	return 0;
}

static int 
gp_ov7670_cropcap(
	struct v4l2_subdev *sd,
	struct v4l2_cropcap *cc
)
{
	return 0;
}


static int 
gp_ov7670_g_crop(
	struct v4l2_subdev *sd,
	struct v4l2_crop *crop
)
{
	return 0;
}

static int 
gp_ov7670_s_crop(
	struct v4l2_subdev *sd,
	struct v4l2_crop *crop
)
{
	return 0;
}

static int 
gp_ov7670_g_parm(
	struct v4l2_subdev *sd,
	struct v4l2_streamparm *parms
)
{
	return 0;
}

static int 
gp_ov7670_s_parm(
	struct v4l2_subdev *sd,
	struct v4l2_streamparm *param
)
{
	return 0;
}

static int 
gp_ov7670_queryctrl(
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
gp_ov7670_g_ctrl(
	struct v4l2_subdev *sd, 
	struct v4l2_control *ctrl
)
{
	unsigned char data;
	int nRet = 0;

	switch(ctrl->id)
	{
	case V4L2_CID_AUTO_WHITE_BALANCE:
		nRet = sensor_read(0x13, &data);
		if(data &= 0x02)
			ctrl->value = 1;
		else 
			ctrl->value = 0;
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
	
	return nRet; 
}

static int 
gp_ov7670_s_ctrl(
	struct v4l2_subdev *sd, 
	struct v4l2_control *ctrl
)
{
	unsigned char data;
	int nRet = 0;
	
	switch(ctrl->id)
	{
	case V4L2_CID_AUTO_WHITE_BALANCE:
		nRet = sensor_read(0x13, &data);
		if(ctrl->value)
			sensor_write(0x13, data |= 0x02);
		else
			sensor_write(0x13, data &= ~0x02);
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
	return nRet; 
}

static int 
gp_ov7670_reset(
	struct v4l2_subdev *sd, 
	u32 val
)
{
	sensor_write(0x12, 0x80);
	sccb_delay(val);
	return 0;
}

static int 
gp_ov7670_init(
	struct v4l2_subdev *sd,
	u32 val
)
{
	int nRet;

	printk("ov7670_init.\n");
	nRet = ov7670_write_table(ov7670_init_table);
	if(nRet < 0) return -1;
	nRet = ov7670_write_table(g_ov7670_dev.fmt->regs);
	if(nRet < 0) return -1;
	return 0;
}

static int 
gp_ov7670_suspend(
	struct v4l2_subdev *sd
)
{
	return 0;
}

static int 
gp_ov7670_resume(
	struct v4l2_subdev *sd
)
{
	return 0;
}

static const struct v4l2_subdev_core_ops cdsp_ov7670_core_ops = 
{
	.g_ctrl = gp_ov7670_g_ctrl,
	.s_ctrl = gp_ov7670_s_ctrl,
	.queryctrl = gp_ov7670_queryctrl,
	.reset = gp_ov7670_reset,
	.init = gp_ov7670_init,
};

static const struct v4l2_subdev_video_ops cdsp_ov7670_video_ops = 
{
	.enum_fmt = gp_ov7670_enum_fmt,
	.try_fmt = gp_ov7670_try_fmt,
	.g_fmt = gp_ov7670_g_fmt,
	.s_fmt = gp_ov7670_s_fmt,
	.s_parm = gp_ov7670_s_parm,
	.g_parm = gp_ov7670_g_parm,
	.cropcap = gp_ov7670_cropcap,
	.g_crop = gp_ov7670_g_crop,
	.s_crop = gp_ov7670_s_crop,	
};

static const struct v4l2_subdev_ext_ops cdsp_ov7670_ext_ops = 
{
	.s_interface = gp_ov7670_s_interface,
	.suspend = gp_ov7670_suspend,
	.resume = gp_ov7670_resume,
};


static const struct v4l2_subdev_ops cdsp_ov7670_ops = 
{
	.core = &cdsp_ov7670_core_ops,
	.video = &cdsp_ov7670_video_ops,
	.ext = &cdsp_ov7670_ext_ops
};

static int __init 
ov7670_init_module(
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
	g_i2c_handle = gp_i2c_bus_request(OV7670_ID, 100);	/*100KHZ*/
	if((g_i2c_handle == 0) ||(g_i2c_handle == -ENOMEM))
	{
		printk(KERN_WARNING "i2cReqFail %d\n", g_i2c_handle);
		return -1;
	}
#endif

	printk(KERN_WARNING "ModuleInit: cdsp_ov7670\n");
	g_ov7670_dev.fmt = &g_ov7670_fmt[0];
	g_ov7670_dev.width = 640;
	g_ov7670_dev.height = 480;
	v4l2_subdev_init(&(g_ov7670_dev.sd), &cdsp_ov7670_ops);
	strcpy(g_ov7670_dev.sd.name, "cdsp_ov7670");
	register_sensor(&g_ov7670_dev.sd, (int *)&param[0]);
	return 0;
}

static void __exit
ov7670_module_exit(
		void
) 
{
#if I2C_USE_GPIO == 1	
	gp_gpio_release(g_scl_handle);
	gp_gpio_release(g_sda_handle);	
#else
	gp_i2c_bus_release(g_i2c_handle);
#endif
	unregister_sensor(&g_ov7670_dev.sd);
}

module_init(ov7670_init_module);
module_exit(ov7670_module_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/
MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("Generalplus ov7670 Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");



