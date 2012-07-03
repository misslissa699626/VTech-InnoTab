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
#define	OV3640_ID					0x78
#define OV3640_WIDTH				2048
#define OV3640_HEIGHT				1536

#define OV3640_VYUY 				0x00
#define OV3640_UYVY					0x02
#define OV3640_BGGR					0x18
#define OV3640_GBRG					0x19
#define OV3640_GRBG					0x1A
#define OV3640_RGGB					0x1B

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

struct ov3640_fmt_s
{
	__u8 *desc;
	__u32 pixelformat;
	struct regval_list_s *pInitRegs;
	struct regval_list_s *pScaleRegs;
	int bpp;					/* Bytes per pixel */
}; 

typedef struct ov3640_dev_s 
{
	struct v4l2_subdev sd;
	struct ov3640_fmt_s *fmt;	/* Current format */
	short width;
	short height;
	int hue;					/* Hue value */
	unsigned char sat;			/* Saturation value */
	unsigned char reserved0;
	unsigned char reserved1;
	unsigned char reserved2;
}ov3640_dev_t;

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
	{ 0x304d, 0x41 }, 
	{ 0x3087, 0x16 }, 
	{ 0x30aa, 0x45 }, 
	{ 0x30b0, 0xff }, 
	{ 0x30b1, 0xff }, 
	{ 0x30b2, 0x10 }, 
	{ 0x30d7, 0x10 }, 
	
	{ 0x309e, 0x00 }, 
	{ 0x3602, 0x26 }, /* SOL/EOL on */	
	{ 0x3603, 0x4D }, /* ecc */
	{ 0x364c, 0x04 }, /* ecc */
	{ 0x360c, 0x12 }, /* irtual channel 0 */
	{ 0x361e, 0x00 }, 
	{ 0x361f, 0x11 }, /* pclk_period, terry */
	{ 0x3633, 0x32 }, /* increase hs_prepare */
	{ 0x3629, 0x3c }, /* increase clk_prepare */

	{ 0x300e, 0x38 },
	{ 0x300f, 0xa2 }, /*a1:1.5 , a2:2 */
#if 1
	{ 0x3010, 0x80 }, /* high mipi spd, 81 */
#else
	{ 0x3010, 0xa0 }, /* 2 lane */
#endif
	{ 0x3011, 0x00 }, 
	{ 0x304c, 0x81 }, 
	
	{ 0x3018, 0x38 }, /* aec */
	{ 0x3019, 0x30 }, 
	{ 0x301a, 0x61 }, 
	{ 0x307d, 0x00 }, 
	{ 0x3087, 0x02 }, 
	{ 0x3082, 0x20 },
	
	{ 0x303c, 0x08 }, /* aec weight */
	{ 0x303d, 0x18 }, 
	{ 0x303e, 0x06 }, 
	{ 0x303f, 0x0c }, 
	{ 0x3030, 0x62 }, 	
	{ 0x3031, 0x26 }, 
	{ 0x3032, 0xe6 }, 
	{ 0x3033, 0x6e }, 
	{ 0x3034, 0xea }, 
	{ 0x3035, 0xae }, 
	{ 0x3036, 0xa6 }, 
	{ 0x3037, 0x6a },
	
	{ 0x3015, 0x12 },	
	{ 0x3014, 0x04 },	
	{ 0x3013, 0xf7 },
	
	{ 0x3104, 0x02 }, 
	{ 0x3105, 0xfd }, 
	{ 0x3106, 0x00 }, 
	{ 0x3107, 0xff }, 
	{ 0x3308, 0xa5 }, 
	{ 0x3316, 0xff }, 
	{ 0x3317, 0x00 }, 
	{ 0x3087, 0x02 }, 
	{ 0x3082, 0x20 }, 
	{ 0x3300, 0x13 }, 
	{ 0x3301, 0xde }, 
	{ 0x3302, 0xef },
	
	{ 0x30b8, 0x20 },  
	{ 0x30b9, 0x17 }, 
	{ 0x30ba, 0x04 },	
	{ 0x30bb, 0x08 },     

	{ 0x3100, 0x22 }, /* set raw format */
	{ 0x3304, 0x01 }, 
 	{ 0x3400, 0x03 },
 	{ 0x3600, 0xC4 },
 	{ 0x3404, OV3640_BGGR },
 	
	{ 0x3020, 0x01 }, /* Size, 2048x1536, QXGA */
	{ 0x3021, 0x1d },
	{ 0x3022, 0x00 },
	{ 0x3023, 0x0a },
	{ 0x3024, 0x08 },
	{ 0x3025, 0x18 },
	{ 0x3026, 0x06 },
	{ 0x3027, 0x0c },
	{ 0x335f, 0x68 },
	{ 0x3360, 0x18 },
	{ 0x3361, 0x0c },
	{ 0x3362, 0x68 },
	{ 0x3363, 0x08 },
	{ 0x3364, 0x04 },
	{ 0x3403, 0x42 },
	{ 0x3088, 0x08 },
	{ 0x3089, 0x00 },
	{ 0x308a, 0x06 },
	{ 0x308b, 0x00 },

	{ 0x3507, 0x06 },
	{ 0x350a, 0x4f },
	{ 0x3600, 0xc4 },
#if COLOR_BAR_EN == 1
	{ 0x307B, 0x4a }, /* color bar[1:0] */
	{ 0x307D, 0xa0 }, /* color bar[7] */
	{ 0x306C, 0x00 }, /* color bar[4] */
	{ 0x3080, 0x11 }, /* color bar[7] enable */
#endif
	{ 0xffff, 0xff },
};

static struct regval_list_s mipi_yuv_qxga[] = 
{
	{ 0x304d, 0x41 }, 
	{ 0x3087, 0x16 }, 
	{ 0x30aa, 0x45 }, 
	{ 0x30b0, 0xff }, 
	{ 0x30b1, 0xff }, 
	{ 0x30b2, 0x10 }, 
	{ 0x30d7, 0x10 }, 
	
	{ 0x309e, 0x00 }, 
	{ 0x3602, 0x26 }, /* SOL/EOL on */	
	{ 0x3603, 0x4D }, /* ecc */
	{ 0x364c, 0x04 }, /* ecc */
	{ 0x360c, 0x12 }, /* irtual channel 0 */
	{ 0x361e, 0x00 }, 
	{ 0x361f, 0x11 }, /* pclk_period, terry */
	{ 0x3633, 0x32 }, /* increase hs_prepare */
	{ 0x3629, 0x3c }, /* increase clk_prepare */

	{ 0x300e, 0x38 },
	{ 0x300f, 0xa2 }, /*a1:1.5 , a2:2 */
#if 1
	{ 0x3010, 0x80 }, /* high mipi spd, 81 */
#else
	{ 0x3010, 0xa0 }, /* 2 lane */
#endif
	{ 0x3011, 0x00 }, 
	{ 0x304c, 0x81 }, 

	{ 0x3018, 0x38 }, /* aec */
	{ 0x3019, 0x30 }, 
	{ 0x301a, 0x61 }, 
	{ 0x307d, 0x00 }, 
	{ 0x3087, 0x02 }, 
	{ 0x3082, 0x20 },
	
	{ 0x303c, 0x08 }, /* aec weight */
	{ 0x303d, 0x18 }, 
	{ 0x303e, 0x06 }, 
	{ 0x303f, 0x0c }, 
	{ 0x3030, 0x62 }, 	
	{ 0x3031, 0x26 }, 
	{ 0x3032, 0xe6 }, 
	{ 0x3033, 0x6e }, 
	{ 0x3034, 0xea }, 
	{ 0x3035, 0xae }, 
	{ 0x3036, 0xa6 }, 
	{ 0x3037, 0x6a },
	
	{ 0x3015, 0x12 },	
	{ 0x3014, 0x04 },	
	{ 0x3013, 0xf7 },

	{ 0x3104, 0x02 }, 
	{ 0x3105, 0xfd }, 
	{ 0x3106, 0x00 }, 
	{ 0x3107, 0xff }, 
	{ 0x3308, 0xa5 }, 
	{ 0x3316, 0xff }, 
	{ 0x3317, 0x00 }, 
	{ 0x3087, 0x02 }, 
	{ 0x3082, 0x20 }, 
	{ 0x3300, 0x13 }, 
	{ 0x3301, 0xde }, 
	{ 0x3302, 0xef },
	
	{ 0x30b8, 0x20 },  
	{ 0x30b9, 0x17 }, 
	{ 0x30ba, 0x04 },	
	{ 0x30bb, 0x08 },     

	{ 0x3100, 0x02 }, /* set yuv format */
	{ 0x3304, 0x00 },
	{ 0x3400, 0x00 },
	{ 0x3404, OV3640_UYVY}, 
	
	{ 0x3020, 0x01 }, /* Size, 2048x1536, QXGA */
	{ 0x3021, 0x1d },
	{ 0x3022, 0x00 },
	{ 0x3023, 0x0a },
	{ 0x3024, 0x08 },
	{ 0x3025, 0x18 },
	{ 0x3026, 0x06 },
	{ 0x3027, 0x0c },
	{ 0x335f, 0x68 },
	{ 0x3360, 0x18 },
	{ 0x3361, 0x0c },
	{ 0x3362, 0x68 },
	{ 0x3363, 0x08 },
	{ 0x3364, 0x04 },
	{ 0x3403, 0x42 },
	{ 0x3088, 0x08 },
	{ 0x3089, 0x00 },
	{ 0x308a, 0x06 },
	{ 0x308b, 0x00 },

	{ 0x3507, 0x06 },
	{ 0x350a, 0x4f },
	{ 0x3600, 0xc4 },
#if COLOR_BAR_EN == 1
	{ 0x307B, 0x4a }, /* color bar[1:0] */
	{ 0x307D, 0xa0 }, /* color bar[7] */
	{ 0x306C, 0x00 }, /* color bar[4] */
	{ 0x3080, 0x11 }, /* color bar[7] enable */
#endif
	{ 0xffff, 0xff },
};

static struct regval_list_s mipi_scale_vga[] =
{
	{ 0x3012, 0x10 },
	{ 0x3020, 0x01 },
	{ 0x3021, 0x1d },
	{ 0x3022, 0x00 },
	{ 0x3023, 0x06 },
	{ 0x3024, 0x08 },
	{ 0x3025, 0x18 },
	{ 0x3026, 0x03 },
	{ 0x3027, 0x04 },
	{ 0x302a, 0x03 },
	{ 0x302b, 0x10 },
	{ 0x3075, 0x24 },
	{ 0x300d, 0x01 },
	{ 0x30d7, 0x90 },
	{ 0x3069, 0x04 },
	
	{ 0x3302, 0xef },
	{ 0x335f, 0x34 },
	{ 0x3360, 0x0c },
	{ 0x3361, 0x04 },
	{ 0x3362, 0x12 },
	{ 0x3363, 0x88 },
	{ 0x3364, 0xe4 },
	{ 0x3403, 0x42 },

	{ 0x302c, 0x0e }, /* EXHTS */
	{ 0x302d, 0x00 }, /* EXVTS[15:8] */
	{ 0x302e, 0x10 }, /* EXVTS[7:0] */
	
	{ 0x3088, 0x02 },
	{ 0x3089, 0x80 },
	{ 0x308a, 0x01 },
	{ 0x308b, 0xe0 },
	{ 0xffff, 0xff },
};

static struct regval_list_s ccir601_raw_qxga[] = 
{
	{ 0x304d, 0x45 },//44 ;Rev2A
	{ 0x30a7, 0x5e },//Rev2C mi
	{ 0x3087, 0x16 },//Rev2A
	{ 0x309C, 0x1a },//Rev2C 18
	{ 0x30a2, 0xe4 },//Rev2C E8
	{ 0x30aa, 0x42 },//Rev2C 45
	{ 0x30b0, 0xff },//Rev2A
	{ 0x30b1, 0xff },
	{ 0x30b2, 0x10 },//driving

	{ 0x300e, 0x32 }, //21MHz, 5.8fps
	//{ 0x300e, 0x27 },//40Mhz 10fps
	{ 0x300f, 0x21 },//051007 (a1)
	{ 0x3010, 0x20 },//Rev2A 82
	{ 0x3011, 0x01 },//Rev2A default 7.5fps
	{ 0x304c, 0x82 },//Rev2A
	{ 0x30d7, 0x10 },//Rev2A 08212007
	{ 0x30d9, 0x0d },//Rev2C
	{ 0x30db, 0x08 },//Rev2C
	{ 0x3016, 0x82 },//Rev2C

	//aec/agc auto setting
	{ 0x3018, 0x38 },//aec
	{ 0x3019, 0x30 },//06142007
	{ 0x301a, 0x61 },//06142007
	{ 0x307d, 0x00 },//aec isp 06142007
	{ 0x3087, 0x02 },//06142007
	{ 0x3082, 0x20 },//06142007

	{ 0x3070, 0x00 },//50Hz Banding MSB
	{ 0x3071, 0xaf },//50Hz Banding LSB
	{ 0x3072, 0x00 },//60Hz Banding MSB
	{ 0x3073, 0xa6 },//60Hz Banding LSB
	{ 0x301c, 0x07 },//max_band_step_50hz
	{ 0x301d, 0x08 },//max_band_step_60hz

	{ 0x3015, 0x12 },//07182007 8x gain, auto 1/2
	{ 0x3014, 0x05 },//0x0c },//06142007 auto frame on
	{ 0x3013, 0xf7 },//07182007

	//aecweight
	{ 0x303c, 0x08 },
	{ 0x303d, 0x18 },
	{ 0x303e, 0x06 },
	{ 0x303F, 0x0c },
	
	{ 0x3030, 0x62 },
	{ 0x3031, 0x26 },
	{ 0x3032, 0xe6 },
	{ 0x3033, 0x6e },
	{ 0x3034, 0xea },
	{ 0x3035, 0xae },
	{ 0x3036, 0xa6 },
	{ 0x3037, 0x6a },
	
	//ISP Common 
	{ 0x3104, 0x02 },//isp system control
	{ 0x3105, 0xfd },
	{ 0x3106, 0x00 },
	{ 0x3107, 0xff },
	{ 0x3300, 0x13 },//052207
	{ 0x3301, 0xde },//aec gamma- 06142007

	//ISP setting
	{ 0x3302, 0xcf },//sde, uv_adj, gam, awb

	//AWB
	{ 0x3312, 0x26 },
	{ 0x3314, 0x42 },
	{ 0x3313, 0x2b },
	{ 0x3315, 0x42 },
	{ 0x3310, 0xd0 },
	{ 0x3311, 0xbd },
	{ 0x330c, 0x18 },
	{ 0x330d, 0x18 },
	{ 0x330e, 0x56 },
	{ 0x330f, 0x5c },
	{ 0x330b, 0x1c },
	{ 0x3306, 0x5c },
	{ 0x3307, 0x11 },
	//{ 0x3308, 0x00 }, // [7]: AWB_mode=advanced
	
	//gamma
	{ 0x331b, 0x09 }, // Gamma YST1
	{ 0x331c, 0x18 }, // Gamma YST2
	{ 0x331d, 0x30 }, // Gamma YST3
	{ 0x331e, 0x58 }, // Gamma YST4
	{ 0x331f, 0x66 }, // Gamma YST5
	{ 0x3320, 0x72 }, // Gamma YST6
	{ 0x3321, 0x7d }, // Gamma YST7
	{ 0x3322, 0x86 }, // Gamma YST8
	{ 0x3323, 0x8f }, // Gamma YST9
	{ 0x3324, 0x97 }, // Gamma YST10
	{ 0x3325, 0xa5 }, // Gamma YST11
	{ 0x3326, 0xb2 }, // Gamma YST12
	{ 0x3327, 0xc7 }, // Gamma YST13
	{ 0x3328, 0xd8 }, // Gamma YST14
	{ 0x3329, 0xe8 }, // Gamma YST15
	{ 0x332a, 0x20 }, // Gamma YSLP15
	{ 0x332b, 0x00 }, // [3]: WB_mode=auto
	{ 0x332d, 0x64 }, // [6]:de-noise auto mode; [5]:edge auto mode; [4:0]:edge threshold
	{ 0x3355, 0x06 }, // Special_Effect_CTRL: [1]:Sat_en; [2]: Cont_Y_en

	//Sat
    { 0x3358, 0x40 }, // Special_Effect_Sat_U
	{ 0x3359, 0x40 }, // Special_Effect_Sat_V
	
	//Lens correction
	{ 0x336a, 0x52 },// LENC R_A1
	{ 0x3370, 0x46 },// LENC G_A1
	{ 0x3376, 0x38 },// LENC B_A1
	{ 0x3300, 0x13 }, 

	//UV adjust  
	{ 0x30b8, 0x20 },   
	{ 0x30b9, 0x17 },
	{ 0x30ba, 0x04 },
	{ 0x30bb, 0x08 },

	//Compression
	{ 0x3507, 0x06 },
	{ 0x350a, 0x4f },

	//Output format
	{ 0x3100, 0x32 },
	{ 0x3304, 0x00 },
	{ 0x3400, 0x02 },
	{ 0x3404, 0x22 },
	{ 0x3500, 0x00 },
	{ 0x3600, 0xC0 }, 
	{ 0x3610, 0x60 },
	{ 0x350a, 0x4f },
	
	//DVP QXGA
	{ 0x3088, 0x08 },
	{ 0x3089, 0x00 },
	{ 0x308a, 0x06 },
	{ 0x308b, 0x00 },

	//CIP Raw
	{ 0x3100, 0x22 },
	{ 0x3304, 0x01 },
	{ 0x3400, 0x03 },

	//vsync width 
	{ 0x302d, 0x00 }, /* EXVTS[15:8] */
	{ 0x302e, 0x00 }, /* EXVTS[7:0] */

	//hsync mode
	{ 0x3600, 0xC4 },
	{ 0x3646, 0x40 },

	{ 0x308d, 0x04 }, //Rev2A
	{ 0x3086, 0x03 }, //Rev2A
	{ 0x3086, 0x00 }, //Rev2A
	{ 0xffff, 0xff },
};

static struct regval_list_s ccir601_yuv_qxga[] =
{
	{ 0x304d, 0x45 },//44 ;Rev2A
	{ 0x30a7, 0x5e },//Rev2C mi
	{ 0x3087, 0x16 },//Rev2A
	{ 0x309C, 0x1a },//Rev2C 18
	{ 0x30a2, 0xe4 },//Rev2C E8
	{ 0x30aa, 0x42 },//Rev2C 45
	{ 0x30b0, 0xff },//Rev2A
	{ 0x30b1, 0xff },
	{ 0x30b2, 0x10 },//driving

	{ 0x300e, 0x32 },//(3c)48Mhz 7.5fps
	//{ 0x300e, 0x27 },//40Mhz 10fps
	{ 0x300f, 0x21 },//051007 (a1)
	{ 0x3010, 0x20 },//Rev2A 82
	{ 0x3011, 0x01 },//Rev2A default 7.5fps
	{ 0x304c, 0x82 },//Rev2A
	{ 0x30d7, 0x10 },//Rev2A 08212007
	{ 0x30d9, 0x0d },//Rev2C
	{ 0x30db, 0x08 },//Rev2C
	{ 0x3016, 0x82 },//Rev2C

	//aec/agc auto setting
	{ 0x3018, 0x38 },//aec
	{ 0x3019, 0x30 },//06142007
	{ 0x301a, 0x61 },//06142007
	{ 0x307d, 0x00 },//aec isp 06142007
	{ 0x3087, 0x02 },//06142007
	{ 0x3082, 0x20 },//06142007

	{ 0x3070, 0x00 },//50Hz Banding MSB
	{ 0x3071, 0xaf },//50Hz Banding LSB
	{ 0x3072, 0x00 },//60Hz Banding MSB
	{ 0x3073, 0xa6 },//60Hz Banding LSB
	{ 0x301c, 0x07 },//max_band_step_50hz
	{ 0x301d, 0x08 },//max_band_step_60hz

	{ 0x3015, 0x12 },//07182007 8x gain, auto 1/2
	{ 0x3014, 0x05 },//0x0c },//06142007 auto frame on
	{ 0x3013, 0xf7 },//07182007

	//aecweight
	{ 0x303c, 0x08 },
	{ 0x303d, 0x18 },
	{ 0x303e, 0x06 },
	{ 0x303F, 0x0c },
	
	{ 0x3030, 0x62 },
	{ 0x3031, 0x26 },
	{ 0x3032, 0xe6 },
	{ 0x3033, 0x6e },
	{ 0x3034, 0xea },
	{ 0x3035, 0xae },
	{ 0x3036, 0xa6 },
	{ 0x3037, 0x6a },
	
	//ISP Common 
	{ 0x3104, 0x02 },//isp system control
	{ 0x3105, 0xfd },
	{ 0x3106, 0x00 },
	{ 0x3107, 0xff },
	{ 0x3300, 0x13 },//052207
	{ 0x3301, 0xde },//aec gamma- 06142007

	//ISP setting
	{ 0x3302, 0xcf },//sde, uv_adj, gam, awb

	//AWB
	{ 0x3312, 0x26 },
	{ 0x3314, 0x42 },
	{ 0x3313, 0x2b },
	{ 0x3315, 0x42 },
	{ 0x3310, 0xd0 },
	{ 0x3311, 0xbd },
	{ 0x330c, 0x18 },
	{ 0x330d, 0x18 },
	{ 0x330e, 0x56 },
	{ 0x330f, 0x5c },
	{ 0x330b, 0x1c },
	{ 0x3306, 0x5c },
	{ 0x3307, 0x11 },
	//{ 0x3308, 0x00 }, // [7]: AWB_mode=advanced
	
	//gamma
	{ 0x331b, 0x09 }, // Gamma YST1
	{ 0x331c, 0x18 }, // Gamma YST2
	{ 0x331d, 0x30 }, // Gamma YST3
	{ 0x331e, 0x58 }, // Gamma YST4
	{ 0x331f, 0x66 }, // Gamma YST5
	{ 0x3320, 0x72 }, // Gamma YST6
	{ 0x3321, 0x7d }, // Gamma YST7
	{ 0x3322, 0x86 }, // Gamma YST8
	{ 0x3323, 0x8f }, // Gamma YST9
	{ 0x3324, 0x97 }, // Gamma YST10
	{ 0x3325, 0xa5 }, // Gamma YST11
	{ 0x3326, 0xb2 }, // Gamma YST12
	{ 0x3327, 0xc7 }, // Gamma YST13
	{ 0x3328, 0xd8 }, // Gamma YST14
	{ 0x3329, 0xe8 }, // Gamma YST15
	{ 0x332a, 0x20 }, // Gamma YSLP15
	{ 0x332b, 0x00 }, // [3]: WB_mode=auto
	{ 0x332d, 0x64 }, // [6]:de-noise auto mode; [5]:edge auto mode; [4:0]:edge threshold
	{ 0x3355, 0x06 }, // Special_Effect_CTRL: [1]:Sat_en; [2]: Cont_Y_en

	//Sat
    { 0x3358, 0x40 }, // Special_Effect_Sat_U
	{ 0x3359, 0x40 }, // Special_Effect_Sat_V
	
	//Lens correction
	{ 0x336a, 0x52 },// LENC R_A1
	{ 0x3370, 0x46 },// LENC G_A1
	{ 0x3376, 0x38 },// LENC B_A1
	{ 0x3300, 0x13 }, 

	//UV adjust  
	{ 0x30b8, 0x20 },   
	{ 0x30b9, 0x17 },
	{ 0x30ba, 0x04 },
	{ 0x30bb, 0x08 },

	//Compression
	{ 0x3507, 0x06 },
	{ 0x350a, 0x4f },

	//Output format
	{ 0x3100, 0x32 },
	{ 0x3304, 0x00 },
	{ 0x3400, 0x02 },
	{ 0x3404, 0x22 },
	{ 0x3500, 0x00 },
	{ 0x3600, 0xC0 }, 
	{ 0x3610, 0x60 },
	{ 0x350a, 0x4f },

	//DVP QXGA
	{ 0x3088, 0x08 },
	{ 0x3089, 0x00 },
	{ 0x308a, 0x06 },
	{ 0x308b, 0x00 },

	//SET YUV
	{ 0x3100, 0x02 },
	{ 0x3301, 0x10 }, //0x30
	{ 0x3304, 0x00 }, //0x03
	{ 0x3400, 0x00 },
	{ 0x3404, 0x00 },

	//vsync width 
	{ 0x302d, 0x00 }, /* EXVTS[15:8] */
	{ 0x302e, 0x00 }, /* EXVTS[7:0] */

	//hsync mode
	{ 0x3600, 0xC4 },
	{ 0x3646, 0x40 },
	 
	{ 0x304c, 0x81 }, 
	{ 0x3011, 0x01 }, //3.75fps
	
	{ 0x308d, 0x04 }, //Rev2A
	{ 0x3086, 0x03 }, //Rev2A
	{ 0x3086, 0x00 }, //Rev2A
	{ 0xffff, 0xff },
};

static struct regval_list_s ccir601_scale_xga[] =
{
	{ 0x3012, 0x10 },
	{ 0x3023, 0x06 }, //05
	{ 0x3026, 0x03 },
	{ 0x3027, 0x04 },
	{ 0x302a, 0x03 },
	{ 0x302b, 0x10 },
	{ 0x3075, 0x24 },
	{ 0x300d, 0x01 }, //pclk reverse set 
	{ 0x30d7, 0x90 },
	{ 0x3069, 0x04 },
	{ 0x303e, 0x00 },
	{ 0x303f, 0xc0 },
	
	{ 0x3302, 0xef },
	{ 0x335f, 0x34 },//Zoom_in output size
	{ 0x3360, 0x0c },
	{ 0x3361, 0x04 },
	{ 0x3362, 0x34 },//Zoom_out output size
	{ 0x3363, 0x08 },
	{ 0x3364, 0x04 },
	{ 0x3403, 0x42 },//x, y start
	{ 0x3088, 0x14 },//x_output_size
	{ 0x3089, 0x04 },//0x00, hsize 0x404
	{ 0x308a, 0x03 },//y_output_size
	{ 0x308b, 0x04 },//vsize 0x304

	{ 0x3011, 0x00 },//30fps
	{ 0xffff, 0xff },
};

static struct regval_list_s ccir601_scale_vga[] =
{
	{ 0x3012, 0x10 },
	{ 0x3023, 0x06 }, //05
	{ 0x3026, 0x03 },
	{ 0x3027, 0x04 },
	{ 0x302a, 0x03 },
	{ 0x302b, 0x10 },
	{ 0x3075, 0x24 },
	{ 0x300d, 0x01 }, //pclk reverse set 
	{ 0x30d7, 0x90 },
	{ 0x3069, 0x04 },
	{ 0x303e, 0x00 },
	{ 0x303f, 0xc0 },
	
	{ 0x3302, 0xef },
	{ 0x335f, 0x34 },//Zoom_in output size
	{ 0x3360, 0x0c },
	{ 0x3361, 0x04 },
	{ 0x3362, 0x12 },//Zoom_out output size
	{ 0x3363, 0x88 },
	{ 0x3364, 0xE4 },
	{ 0x3403, 0x42 },//x, y start
	{ 0x3088, 0x12 },//x_output_size
	{ 0x3089, 0x84 },//0x00, hsize 0x284
	{ 0x308a, 0x01 },//y_output_size
	{ 0x308b, 0xE4 },//vsize 0x1E4

	{ 0x3600, 0xCC },
	{ 0x3011, 0x00 },//30fps
	{ 0xffff, 0xff },
};

static struct regval_list_s ccir601_yuv_xga[] = 
{
	{ 0x3012, 0x90 }, // [7]:Reset; [6:4]=001->XGA mode
	{ 0x30a9, 0xdb }, // for 1.5V
	{ 0x304d, 0x45 },
	{ 0x3087, 0x16 },
	{ 0x309c, 0x1a },                 
	{ 0x30a2, 0xe4 },                 
	{ 0x30aa, 0x42 },                 
	{ 0x30b0, 0xff },                 
	{ 0x30b1, 0xff },
	{ 0x30b2, 0x10 },
	{ 0x300e, 0x32 },
	{ 0x300f, 0x21 },
	{ 0x3010, 0x20 },
	{ 0x3011, 0x01 },
	{ 0x304c, 0x82 },
	{ 0x30d7, 0x10 },                 
	{ 0x30d9, 0x0d },                 
	{ 0x30db, 0x08 },                 
	{ 0x3016, 0x82 },                 
	{ 0x3018, 0x48 }, // Luminance High Range=72 after Gamma=0x86=134; 0x40->134
	{ 0x3019, 0x40 }, // Luminance Low Range=64 after Gamma=0x8f=143; 0x38->125
	{ 0x301a, 0x82 },                 
	{ 0x307d, 0x00 },                 
	{ 0x3087, 0x02 },                 
	{ 0x3082, 0x20 },
	{ 0x3070, 0x00 }, // 50Hz Banding MSB
	{ 0x3071, 0x72 }, // 50Hz Banding LSB
	{ 0x3072, 0x00 }, // 60Hz Banding MSB
	{ 0x3073, 0xa6 }, // 60Hz Banding LSB
	{ 0x301c, 0x07 }, //max_band_step_50hz
	{ 0x301d, 0x08 }, //max_band_step_60hz
	{ 0x3015, 0x12 }, // [6:4]:1 dummy frame; [2:0]:AGC gain 8x
	{ 0x3014, 0x84 }, //[7]:50hz; [6]:auto banding detection disable; [3]:night modedisable
	{ 0x3013, 0xf7 }, //AE_en
	{ 0x3030, 0x11 }, // Avg_win_Weight0
	{ 0x3031, 0x11 }, // Avg_win_Weight1
	{ 0x3032, 0x11 }, // Avg_win_Weight2
	{ 0x3033, 0x11 }, // Avg_win_Weight3
	{ 0x3034, 0x11 }, // Avg_win_Weight4
	{ 0x3035, 0x11 }, // Avg_win_Weight5
	{ 0x3036, 0x11 }, // Avg_win_Weight6
	{ 0x3037, 0x11 }, // Avg_win_Weight7
	{ 0x3038, 0x01 }, // Avg_Win_Hstart=285
	{ 0x3039, 0x1d }, // Avg_Win_Hstart=285
	{ 0x303a, 0x00 }, // Avg_Win_Vstart=10
	{ 0x303b, 0x0a }, // Avg_Win_Vstart=10
	{ 0x303c, 0x02 }, // Avg_Win_Width=512x4=2048
	{ 0x303d, 0x00 }, // Avg_Win_Width=512x4=2048
	{ 0x303e, 0x01 }, // Avg_Win_Height=384x4=1536
	{ 0x303f, 0x80 }, // Avg_Win_Height=384x4=1536
	{ 0x3047, 0x00 }, // [7]:avg_based AE
	{ 0x30b8, 0x20 },
	{ 0x30b9, 0x17 },
	{ 0x30ba, 0x04 },
	{ 0x30bb, 0x08 },
	{ 0x30a9, 0xdb }, // for 1.5V
	{ 0x3104, 0x02 },                 
	{ 0x3105, 0xfd },                 
	{ 0x3106, 0x00 },                 
	{ 0x3107, 0xff },                 
	{ 0x3100, 0x02 },
	{ 0x3300, 0x13 }, // [0]: LENC disable; [1]: AF enable
	{ 0x3301, 0xde }, // [1]: BC_en; [2]: WC_en; [4]: CMX_en
	{ 0x3302, 0xcf }, //[0]: AWB_en; [1]: AWB_gain_en; [2]: Gamma_en; [7]: Special_Effect_en
	{ 0x3304, 0xfc }, // [4]: Add bias to gamma result; [5]: Enable Gamma bias function
	{ 0x3306, 0x5c }, // Reserved ???
	{ 0x3307, 0x11 }, // Reserved ???
	{ 0x3308, 0x00 }, // [7]: AWB_mode=advanced
	{ 0x330b, 0x1c }, // Reserved ???
	{ 0x330c, 0x18 }, // Reserved ???
	{ 0x330d, 0x18 }, // Reserved ???
	{ 0x330e, 0x56 }, // Reserved ???
	{ 0x330f, 0x5c }, // Reserved ???
	{ 0x3310, 0xd0 }, // Reserved ???
	{ 0x3311, 0xbd }, // Reserved ???
	{ 0x3312, 0x26 }, // Reserved ???
	{ 0x3313, 0x2b }, // Reserved ???
	{ 0x3314, 0x42 }, // Reserved ???
	{ 0x3315, 0x42 }, // Reserved ???
	{ 0x331b, 0x09 }, // Gamma YST1
	{ 0x331c, 0x18 }, // Gamma YST2
	{ 0x331d, 0x30 }, // Gamma YST3
	{ 0x331e, 0x58 }, // Gamma YST4
	{ 0x331f, 0x66 }, // Gamma YST5
	{ 0x3320, 0x72 }, // Gamma YST6
	{ 0x3321, 0x7d }, // Gamma YST7
	{ 0x3322, 0x86 }, // Gamma YST8
	{ 0x3323, 0x8f }, // Gamma YST9
	{ 0x3324, 0x97 }, // Gamma YST10
	{ 0x3325, 0xa5 }, // Gamma YST11
	{ 0x3326, 0xb2 }, // Gamma YST12
	{ 0x3327, 0xc7 }, // Gamma YST13
	{ 0x3328, 0xd8 }, // Gamma YST14
	{ 0x3329, 0xe8 }, // Gamma YST15
	{ 0x332a, 0x20 }, // Gamma YSLP15
	{ 0x332b, 0x00 }, // [3]: WB_mode=auto
	{ 0x332d, 0x64 }, // [6]:de-noise auto mode; [5]:edge auto mode; [4:0]:edge threshold
	{ 0x3355, 0x06 }, // Special_Effect_CTRL: [1]:Sat_en; [2]: Cont_Y_en
	{ 0x3358, 0x40 }, // Special_Effect_Sat_U
	{ 0x3359, 0x40 }, // Special_Effect_Sat_V
	{ 0x336a, 0x52 }, // LENC R_A1
	{ 0x3370, 0x46 }, // LENC G_A1
	{ 0x3376, 0x38 }, // LENC B_A1
	{ 0x3400, 0x00 }, // [2:0];Format input source=DSP TUV444
	{ 0x3403, 0x42 }, // DVP Win Addr
	{ 0x3404, 0x00 }, // [5:0]: yuyv
	{ 0x3507, 0x06 }, // ???
	{ 0x350a, 0x4f }, // ???
	{ 0x3600, 0xc4 },	// VSYNC_CTRL
	{ 0x3646, 0x40 },
	{ 0x3302, 0xcf }, //[0]: AWB_enable
	{ 0x300d, 0x01 }, // PCLK/2
	{ 0x3012, 0x10 }, // [6:4]=001->XGA mode
	{ 0x3013, 0xf7 }, //AE_enable
	{ 0x3020, 0x01 }, // HS=285
	{ 0x3021, 0x1d }, // HS=285
	{ 0x3022, 0x00 }, // VS = 6
	{ 0x3023, 0x06 }, // VS = 6
	{ 0x3024, 0x08 }, // HW=2072
	{ 0x3025, 0x18 }, // HW=2072
	{ 0x3026, 0x03 }, // VW=772
	{ 0x3027, 0x04 }, // VW=772
	{ 0x3028, 0x09 }, //HTotalSize=2375
	{ 0x3029, 0x47 }, //HTotalSize=2375
	{ 0x302a, 0x03 }, //VTotalSize=784
	{ 0x302b, 0x10 }, //VTotalSize=784
	{ 0x304c, 0x82 },
	{ 0x3075, 0x24 }, // VSYNCOPT
	{ 0x3086, 0x00 }, // Sleep/Wakeup
	{ 0x3088, 0x04 }, //x_output_size=1024
	{ 0x3089, 0x08 }, //x_output_size=1024
	{ 0x308a, 0x03 }, //y_output_size=768
	{ 0x308b, 0x04 }, //y_output_size=768
	{ 0x308d, 0x04 },
	{ 0x30d7, 0x90 }, //???
	{ 0x3302, 0xef }, // [5]: Scale_en, [0]: AWB_enable
	{ 0x335f, 0x34 }, // Scale_VH_in
	{ 0x3360, 0x0c }, // Scale_H_in = 0x40c = 1036
	{ 0x3361, 0x04 }, // Scale_V_in = 0x304 = 772
	{ 0x3362, 0x34 }, // Scale_VH_out
	{ 0x3363, 0x08 }, // Scale_H_out = 0x408 = 1032
	{ 0x3364, 0x04 }, // Scale_V_out = 0x304 = 772
	{ 0x300e, 0x32 },
	{ 0x300f, 0x21 },
	{ 0x3011, 0x00 }, // for 30 FPS
	{ 0x304c, 0x82 },
	{ 0xffff, 0xff }, //end
};

static struct regval_list_s ccir601_yuv_vga[] = 
{
	{ 0x3012, 0x90 }, // [7]:Reset; [6:4]=001->XGA mode
	{ 0x30a9, 0xdb }, // for 1.5V
	{ 0x304d, 0x45 },
	{ 0x3087, 0x16 },
	{ 0x309c, 0x1a },                 
	{ 0x30a2, 0xe4 },                 
	{ 0x30aa, 0x42 },                 
	{ 0x30b0, 0xff },                 
	{ 0x30b1, 0xff },
	{ 0x30b2, 0x10 },
	{ 0x300e, 0x32 },
	{ 0x300f, 0x21 },
	{ 0x3010, 0x20 },
	{ 0x3011, 0x01 },
	{ 0x304c, 0x82 },
	{ 0x30d7, 0x10 },                 
	{ 0x30d9, 0x0d },                 
	{ 0x30db, 0x08 },                 
	{ 0x3016, 0x82 },                 
	{ 0x3018, 0x48 }, // Luminance High Range=72 after Gamma=0x86=134; 0x40->134
	{ 0x3019, 0x40 }, // Luminance Low Range=64 after Gamma=0x8f=143; 0x38->125
	{ 0x301a, 0x82 },                 
	{ 0x307d, 0x00 },                 
	{ 0x3087, 0x02 },                 
	{ 0x3082, 0x20 },
	{ 0x3070, 0x00 }, // 50Hz Banding MSB
	{ 0x3071, 0x72 }, // 50Hz Banding LSB
	{ 0x3072, 0x00 }, // 60Hz Banding MSB
	{ 0x3073, 0xa6 }, // 60Hz Banding LSB
	{ 0x301c, 0x07 }, //max_band_step_50hz
	{ 0x301d, 0x08 }, //max_band_step_60hz
	{ 0x3015, 0x12 }, // [6:4]:1 dummy frame; [2:0]:AGC gain 8x
	{ 0x3014, 0x84 }, //[7]:50hz; [6]:auto banding detection disable; [3]:night modedisable
	{ 0x3013, 0xf7 }, //AE_en
	{ 0x3030, 0x11 }, // Avg_win_Weight0
	{ 0x3031, 0x11 }, // Avg_win_Weight1
	{ 0x3032, 0x11 }, // Avg_win_Weight2
	{ 0x3033, 0x11 }, // Avg_win_Weight3
	{ 0x3034, 0x11 }, // Avg_win_Weight4
	{ 0x3035, 0x11 }, // Avg_win_Weight5
	{ 0x3036, 0x11 }, // Avg_win_Weight6
	{ 0x3037, 0x11 }, // Avg_win_Weight7
	{ 0x3038, 0x01 }, // Avg_Win_Hstart=285
	{ 0x3039, 0x1d }, // Avg_Win_Hstart=285
	{ 0x303a, 0x00 }, // Avg_Win_Vstart=10
	{ 0x303b, 0x0a }, // Avg_Win_Vstart=10
	{ 0x303c, 0x02 }, // Avg_Win_Width=512x4=2048
	{ 0x303d, 0x00 }, // Avg_Win_Width=512x4=2048
	{ 0x303e, 0x01 }, // Avg_Win_Height=384x4=1536
	{ 0x303f, 0x80 }, // Avg_Win_Height=384x4=1536
	{ 0x3047, 0x00 }, // [7]:avg_based AE
	{ 0x30b8, 0x20 },
	{ 0x30b9, 0x17 },
	{ 0x30ba, 0x04 },
	{ 0x30bb, 0x08 },
	{ 0x30a9, 0xdb }, // for 1.5V
	{ 0x3104, 0x02 },                 
	{ 0x3105, 0xfd },                 
	{ 0x3106, 0x00 },                 
	{ 0x3107, 0xff },                 
	{ 0x3100, 0x02 },
	{ 0x3300, 0x13 }, // [0]: LENC disable; [1]: AF enable
	{ 0x3301, 0xde }, // [1]: BC_en; [2]: WC_en; [4]: CMX_en
	{ 0x3302, 0xcf }, //[0]: AWB_en; [1]: AWB_gain_en; [2]: Gamma_en; [7]: Special_Effect_en
	{ 0x3304, 0xfc }, // [4]: Add bias to gamma result; [5]: Enable Gamma bias function
	{ 0x3306, 0x5c }, // Reserved ???
	{ 0x3307, 0x11 }, // Reserved ???
	{ 0x3308, 0x00 }, // [7]: AWB_mode=advanced
	{ 0x330b, 0x1c }, // Reserved ???
	{ 0x330c, 0x18 }, // Reserved ???
	{ 0x330d, 0x18 }, // Reserved ???
	{ 0x330e, 0x56 }, // Reserved ???
	{ 0x330f, 0x5c }, // Reserved ???
	{ 0x3310, 0xd0 }, // Reserved ???
	{ 0x3311, 0xbd }, // Reserved ???
	{ 0x3312, 0x26 }, // Reserved ???
	{ 0x3313, 0x2b }, // Reserved ???
	{ 0x3314, 0x42 }, // Reserved ???
	{ 0x3315, 0x42 }, // Reserved ???
	{ 0x331b, 0x09 }, // Gamma YST1
	{ 0x331c, 0x18 }, // Gamma YST2
	{ 0x331d, 0x30 }, // Gamma YST3
	{ 0x331e, 0x58 }, // Gamma YST4
	{ 0x331f, 0x66 }, // Gamma YST5
	{ 0x3320, 0x72 }, // Gamma YST6
	{ 0x3321, 0x7d }, // Gamma YST7
	{ 0x3322, 0x86 }, // Gamma YST8
	{ 0x3323, 0x8f }, // Gamma YST9
	{ 0x3324, 0x97 }, // Gamma YST10
	{ 0x3325, 0xa5 }, // Gamma YST11
	{ 0x3326, 0xb2 }, // Gamma YST12
	{ 0x3327, 0xc7 }, // Gamma YST13
	{ 0x3328, 0xd8 }, // Gamma YST14
	{ 0x3329, 0xe8 }, // Gamma YST15
	{ 0x332a, 0x20 }, // Gamma YSLP15
	{ 0x332b, 0x00 }, // [3]: WB_mode=auto
	{ 0x332d, 0x64 }, // [6]:de-noise auto mode; [5]:edge auto mode; [4:0]:edge threshold
	{ 0x3355, 0x06 }, // Special_Effect_CTRL: [1]:Sat_en; [2]: Cont_Y_en
	{ 0x3358, 0x40 }, // Special_Effect_Sat_U
	{ 0x3359, 0x40 }, // Special_Effect_Sat_V
	{ 0x336a, 0x52 }, // LENC R_A1
	{ 0x3370, 0x46 }, // LENC G_A1
	{ 0x3376, 0x38 }, // LENC B_A1
	{ 0x3400, 0x00 }, // [2:0];Format input source=DSP TUV444
	{ 0x3403, 0x42 }, // DVP Win Addr
	{ 0x3404, 0x00 }, // [5:0]: yuyv
	{ 0x3507, 0x06 }, // ???
	{ 0x350a, 0x4f }, // ???
	{ 0x3600, 0xc4 },	// VSYNC_CTRL
	{ 0x3646, 0x40 },
	{ 0x3302, 0xcf }, //[0]: AWB_enable
	{ 0x300d, 0x01 }, // PCLK/2
	{ 0x3012, 0x10 }, // [6:4]=001->XGA mode
	{ 0x3013, 0xf7 }, //AE_enable
	{ 0x3020, 0x01 }, // HS=285
	{ 0x3021, 0x1d }, // HS=285
	{ 0x3022, 0x00 }, // VS = 6
	{ 0x3023, 0x06 }, // VS = 6
	{ 0x3024, 0x08 }, // HW=2072
	{ 0x3025, 0x18 }, // HW=2072
	{ 0x3026, 0x03 }, // VW=772
	{ 0x3027, 0x04 }, // VW=772
	{ 0x3028, 0x09 }, //HTotalSize=2375
	{ 0x3029, 0x47 }, //HTotalSize=2375
	{ 0x302a, 0x03 }, //VTotalSize=784
	{ 0x302b, 0x10 }, //VTotalSize=784
	{ 0x304c, 0x82 },
	{ 0x3075, 0x24 }, // VSYNCOPT
	{ 0x3086, 0x00 }, // Sleep/Wakeup
	{ 0x3088, 0x02 }, //x_output_size=648
	{ 0x3089, 0x88 }, //x_output_size=648
	{ 0x308a, 0x01 }, //y_output_size=484
	{ 0x308b, 0xE4 }, //y_output_size=484
	{ 0x308d, 0x04 },
	{ 0x30d7, 0x90 }, //???
	{ 0x3302, 0xef }, // [5]: Scale_en, [0]: AWB_enable
	{ 0x335f, 0x12 }, // Scale_VH_in
	{ 0x3360, 0x88 }, // Scale_H_in = 0x40c = 648
	{ 0x3361, 0xE4 }, // Scale_V_in = 0x304 = 484
	{ 0x3362, 0x12 }, // Scale_VH_out
	{ 0x3363, 0x88 }, // Scale_H_out = 0x288 = 648
	{ 0x3364, 0xE4 }, // Scale_V_out = 0x1E4 = 484
	{ 0x300e, 0x32 },
	{ 0x300f, 0x21 },
	{ 0x3011, 0x00 }, // for 30 FPS
	{ 0x304c, 0x82 },
	{ 0xffff, 0xff }, //end
};

#define C_OV3640_FMT_MAX	sizeof(g_ov3640_fmt)/sizeof(struct ov3640_fmt_s)
static struct ov3640_fmt_s g_ov3640_fmt[] = 
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
		.pixelformat = V4L2_PIX_FMT_YVYU,
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
		.pixelformat = V4L2_PIX_FMT_YVYU,
		.pInitRegs 	= ccir601_yuv_xga,
		.pScaleRegs = NULL,
		.bpp		= 2,
	},
	/*8*/
	{
		.desc		= "CCIR601 RAW VGA(640x480)",
		.pixelformat = V4L2_PIX_FMT_SGRBG8,
		.pInitRegs 	= ccir601_raw_qxga,
		.pScaleRegs = ccir601_scale_vga,
		.bpp		= 2,
	},
	/*9*/
	{
		.desc		= "CCIR601 YUV VGA(640x480)",
		.pixelformat = V4L2_PIX_FMT_YVYU,
		.pInitRegs 	= ccir601_yuv_vga,
		.pScaleRegs = NULL,
		.bpp		= 2,
	},
};

static ov3640_dev_t	g_ov3640_dev;
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
	unsigned int reg,
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

static int
ov3640_write_table(
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
gp_ov3640_s_interface(
	struct v4l2_subdev *sd,
	struct v4l2_interface *interface
)
{
	return 0;
}

static int 
gp_ov3640_enum_fmt(
	struct v4l2_subdev *sd, 
	struct v4l2_fmtdesc *fmtdesc
)
{
	if(fmtdesc->index >= C_OV3640_FMT_MAX)
		return -EINVAL;

	fmtdesc->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	memcpy((void *)fmtdesc->description, (void *)g_ov3640_fmt[fmtdesc->index].desc, 10);
	fmtdesc->pixelformat = g_ov3640_fmt[fmtdesc->index].pixelformat;
	return 0;
}


static int gp_ov3640_try_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_format *fmt
)
{
	int i;
	struct v4l2_pix_format *pix = &fmt->fmt.pix;

	for(i=0; i<C_OV3640_FMT_MAX; i++)
	{
		if (g_ov3640_fmt[i].pixelformat == pix->pixelformat)
			break;
	}
	
	if(i == C_OV3640_FMT_MAX)
		return -1;
	
	pix->width = OV3640_WIDTH;
	pix->height = OV3640_HEIGHT;
	pix->bytesperline = OV3640_WIDTH * g_ov3640_fmt[i].bpp;
	pix->sizeimage = pix->bytesperline * OV3640_HEIGHT;
	return 0;
}

static int 
gp_ov3640_s_fmt(
	struct v4l2_subdev *sd, 
	struct v4l2_format *fmt
)
{
	int nRet = 0, N = -1;

	g_ov3640_dev.width = fmt->fmt.pix.width;
	g_ov3640_dev.height = fmt->fmt.pix.height;
	if(fmt->fmt.pix.priv == C_CDSP_MIPI)
	{
		switch(fmt->fmt.pix.pixelformat)
		{
		case V4L2_PIX_FMT_SBGGR8:
			if(g_ov3640_dev.width == OV3640_WIDTH) N = 0;
			else if(g_ov3640_dev.width == 640) N = 2;
			else nRet = -EINVAL;
			break;
			
		case V4L2_PIX_FMT_YVYU:
			if(g_ov3640_dev.width == OV3640_WIDTH) N = 1;
			else if(g_ov3640_dev.width == 640) N = 3;
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
			if(g_ov3640_dev.width <= 640) N = 8;
			else if(g_ov3640_dev.width <= 1024) N = 6;
			else if(g_ov3640_dev.width <= OV3640_WIDTH) N = 4;
			else nRet = -EINVAL;
			break;

		case V4L2_PIX_FMT_YVYU:
			if(g_ov3640_dev.width <= 640) N = 9;
			else if(g_ov3640_dev.width <= 1024) N = 7;
			else if(g_ov3640_dev.width <= OV3640_WIDTH) N = 5;
			else nRet = -EINVAL;
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
		printk("OV3640Fmt:%s\n", g_ov3640_fmt[N].desc);
		g_ov3640_dev.fmt = &g_ov3640_fmt[N];
	}
	
	return nRet;
}

static int 
gp_ov3640_g_fmt(
	struct v4l2_subdev *sd, 
	struct v4l2_format *fmt
)
{
	fmt->fmt.pix.width = g_ov3640_dev.width;
	fmt->fmt.pix.height = g_ov3640_dev.height;
	fmt->fmt.pix.pixelformat = g_ov3640_dev.fmt->pixelformat;
	fmt->fmt.pix.field = V4L2_FIELD_NONE;
	fmt->fmt.pix.bytesperline = g_ov3640_dev.width * g_ov3640_dev.fmt->bpp;
	fmt->fmt.pix.sizeimage = fmt->fmt.pix.bytesperline * g_ov3640_dev.height;
	return 0;
}

static int 
gp_ov3640_cropcap(
	struct v4l2_subdev *sd,
	struct v4l2_cropcap *cc
)
{
	return 0;
}


static int 
gp_ov3640_g_crop(
	struct v4l2_subdev *sd,
	struct v4l2_crop *crop
)
{
	return 0;
}

static int 
gp_ov3640_s_crop(
	struct v4l2_subdev *sd,
	struct v4l2_crop *crop
)
{
	return 0;
}

static int 
gp_ov3640_g_parm(
	struct v4l2_subdev *sd,
	struct v4l2_streamparm *parms
)
{
	return 0;
}

static int 
gp_ov3640_s_parm(
	struct v4l2_subdev *sd,
	struct v4l2_streamparm *param
)
{
	return 0;
}

static int 
gp_ov3640_queryctrl(
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
gp_ov3640_g_ctrl(
	struct v4l2_subdev *sd, 
	struct v4l2_control *ctrl
)
{	
	unsigned char data;
	int nRet = 0;
	
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
gp_ov3640_s_ctrl(
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
			nRet = sensor_read(0x332b, (unsigned char *)&data);			
			nRet = sensor_write(0x332b, (data &= ~0x8));
		}
		else
		{
			nRet = sensor_read(0x332b, (unsigned char *)&data);
			nRet = sensor_write(0x332b, data |= 0x08);
			nRet = sensor_write(0x33a7, 0x40);
			nRet = sensor_write(0x33a8, 0x40);
			nRet = sensor_write(0x33a9, 0x40);
		}
		break;
	
	case V4L2_CID_POWER_LINE_FREQUENCY:
		printk("PL = %d\n", ctrl->value);
		if(ctrl->value == V4L2_CID_POWER_LINE_FREQUENCY_DISABLED)
		{

		}
		else if(ctrl->value == V4L2_CID_POWER_LINE_FREQUENCY_50HZ)
		{
			nRet = sensor_read(0x3014, (unsigned char *)&data);
			nRet = sensor_write(0x3014, data |= 0x80);		
		}
		else if(ctrl->value == V4L2_CID_POWER_LINE_FREQUENCY_60HZ)
		{
			nRet = sensor_read(0x3014, (unsigned char *)&data);
			nRet = sensor_write(0x3014, data &= ~0x80);	
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
			nRet = sensor_read(0x332b, (unsigned char *)&data);
			nRet = sensor_write(0x332b, data |= 0x08);
			nRet = sensor_write(0x33a7, 0x5a);
			nRet = sensor_write(0x33a8, 0x40);
			nRet = sensor_write(0x33a9, 0x48);
		}
		else if(ctrl->value == 1)//CLOUDY
		{
			nRet = sensor_read(0x332b, (unsigned char *)&data);
			nRet = sensor_write(0x332b, data |= 0x08);
			nRet = sensor_write(0x33a7, 0x68);
			nRet = sensor_write(0x33a8, 0x40);
			nRet = sensor_write(0x33a9, 0x50);
		}
		else if(ctrl->value == 2)//FLUORESCENCE
		{
			nRet = sensor_read(0x332b, (unsigned char *)&data);
			nRet = sensor_write(0x332b, data |= 0x08);
			nRet = sensor_write(0x33a7, 0x52);
			nRet = sensor_write(0x33a8, 0x40);
			nRet = sensor_write(0x33a9, 0x5a);
		}
		else if(ctrl->value == 3)//INCANDESCENCE
		{
			nRet = sensor_read(0x332b, (unsigned char *)&data);
			nRet = sensor_write(0x332b, data |= 0x08);
			nRet = sensor_write(0x33a7, 0x40);
			nRet = sensor_write(0x33a8, 0x40);
			nRet = sensor_write(0x33a9, 0x64);
		}
		break;

	case V4L2_CID_BACKLIGHT_COMPENSATION:
		printk("NightMode = %d\n", ctrl->value);
		if(ctrl->value)
		{
			nRet = sensor_read(0x3014, (unsigned char *)&data);
			nRet = sensor_write(0x3014, data |= 0x08);
			
			nRet = sensor_write(0x3015, 0x03);//16x gain, no dummy
			nRet = sensor_write(0x3011, 0x01);//set fps/2
			//nRet = sensor_write(0x3011, 0x03);//set fps/4
		}
		else
		{
			nRet = sensor_read(0x3014, (unsigned char *)&data);
			nRet = sensor_write(0x3014, data &= ~0x08);
			
			nRet = sensor_write(0x3015, 0x12);//8x gain, auto 1/2
			nRet = sensor_write(0x3011, 0x00);//set fps
		}
		break;
		
	default:
		return -EINVAL;
	}
	
	return nRet; 
}

static int 
gp_ov3640_reset(
	struct v4l2_subdev *sd, 
	u32 val
)
{
	sensor_write(0x3012, 0x80);
	sccb_delay(val);
	return 0;
}

static int 
gp_ov3640_init(
	struct v4l2_subdev *sd,
	u32 val
)
{
	int nRet;

	if(g_ov3640_dev.fmt->pInitRegs)
	{
		printk("ov3640InitReg\n");
		nRet = ov3640_write_table(g_ov3640_dev.fmt->pInitRegs);
		if(nRet < 0) return -1;
	}
	
	if(g_ov3640_dev.fmt->pScaleRegs)
	{
		printk("ov3640ScaleReg\n");
		nRet = ov3640_write_table(g_ov3640_dev.fmt->pScaleRegs);
		if(nRet < 0) return -1;
	}
	return 0;
}

static int 
gp_ov3640_suspend(
	struct v4l2_subdev *sd
)
{
	return 0;
}

static int 
gp_ov3640_resume(
	struct v4l2_subdev *sd
)
{
	return 0;
}

static const struct v4l2_subdev_core_ops cdsp_ov3640_core_ops = 
{
	.g_ctrl = gp_ov3640_g_ctrl,
	.s_ctrl = gp_ov3640_s_ctrl,
	.queryctrl = gp_ov3640_queryctrl,
	.reset = gp_ov3640_reset,
	.init = gp_ov3640_init,
};

static const struct v4l2_subdev_video_ops cdsp_ov3640_video_ops = 
{
	.enum_fmt = gp_ov3640_enum_fmt,
	.try_fmt = gp_ov3640_try_fmt,
	.g_fmt = gp_ov3640_g_fmt,
	.s_fmt = gp_ov3640_s_fmt,
	.s_parm = gp_ov3640_s_parm,
	.g_parm = gp_ov3640_g_parm,
	.cropcap = gp_ov3640_cropcap,
	.g_crop = gp_ov3640_g_crop,
	.s_crop = gp_ov3640_s_crop,	
};

static const struct v4l2_subdev_ext_ops cdsp_ov3640_ext_ops = 
{
	.s_interface = gp_ov3640_s_interface,
	.suspend = gp_ov3640_suspend,
	.resume = gp_ov3640_resume,
};

static const struct v4l2_subdev_ops cdsp_ov3640_ops = 
{
	.core = &cdsp_ov3640_core_ops,
	.video = &cdsp_ov3640_video_ops,
	.ext = &cdsp_ov3640_ext_ops
};

static int __init 
ov3640_init_module(
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
	g_i2c_handle = gp_i2c_bus_request(OV3640_ID, 100);	/*100KHZ*/
	if((g_i2c_handle == 0) ||(g_i2c_handle == -ENOMEM))
	{
		printk(KERN_WARNING "i2cReqFail %d\n", g_i2c_handle);
		return -1;
	}
#endif

	printk(KERN_WARNING "ModuleInit: cdsp_ov3640 \n");
	g_ov3640_dev.fmt = &g_ov3640_fmt[0];
	g_ov3640_dev.width = OV3640_WIDTH;
	g_ov3640_dev.height = OV3640_HEIGHT;
	v4l2_subdev_init(&(g_ov3640_dev.sd), &cdsp_ov3640_ops);
	strcpy(g_ov3640_dev.sd.name, "cdsp_ov3640");
	register_sensor(&g_ov3640_dev.sd, (int *)&param[0]);
	return 0;
}

static void __exit
ov3640_module_exit(
		void
) 
{
#if I2C_USE_GPIO == 1	
	gp_gpio_release(g_scl_handle);
	gp_gpio_release(g_sda_handle);	
#else
	gp_i2c_bus_release(g_i2c_handle);
#endif	
	unregister_sensor(&(g_ov3640_dev.sd));
}

module_init(ov3640_init_module);
module_exit(ov3640_module_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/
MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("Generalplus cdsp ov3640 Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");



