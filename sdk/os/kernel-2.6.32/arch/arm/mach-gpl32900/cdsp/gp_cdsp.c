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
#include <mach/diag.h>
#include <mach/cdev.h>
#include <linux/cdev.h>

#include <media/v4l2-common.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>

#include <mach/gp_chunkmem.h>
#include <mach/gp_cache.h>
#include <mach/hal/hal_cdsp.h>
#include <mach/hal/hal_clock.h>
#include <mach/gp_cdsp.h>
#include <mach/gp_board.h>
#include <mach/clock_mgr/gp_clock.h>
#include <mach/sensor_mgr.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define C_BUFFER_MAX	6
#define C_DMA_A			1
#define USBPHY_CLK		96000000

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define RETURN(x)	{nRet = x; goto __return;}
#define DERROR 	printk
#if 1
#define DEBUG	printk
#else
#define DEBUG(...)
#endif

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct gpCdspFb_s
{
	unsigned int 	index;
	unsigned int 	flag;
	unsigned int 	addr;
	unsigned int 	phy_addr;
	unsigned int 	length;
}gpCdspFb_t;

typedef struct gpCdspImg_s
{
	unsigned int 	v4l2in_fmt;		/* app format */
	unsigned int 	cdspin_fmt;		/* cdsp format */

	unsigned char	img_src; 		/* image input source, sdram, front, mipi */
	unsigned char 	sensor_flag;	/* sensor flag, 0:sdram, 1:sensor */
	unsigned char	raw_flag;		/* raw flag, 0:yuv, 1:raw */
	unsigned char	raw_format;		/* 0~3, bayer raw format */

	unsigned char	yuv_range;		/* yuv range */
	unsigned char	rgb_path;		/* bypass output raw data */
	unsigned char   reserved0;
	unsigned char   reserved1;

	unsigned char	mapped_idx;
	unsigned char	post_idx;
	unsigned char	ready_idx;
	unsigned char 	fb_total;
	gpCdspFb_t 		frame[C_BUFFER_MAX];

	unsigned short	img_h_size;		/* image/csi h size */
	unsigned short	img_v_size;		/* image/csi v size */
	unsigned short	img_h_offset;	/* image/csi h offset */
	unsigned short	img_v_offset;	/* image/csi v offset */
	unsigned short	img_rb_h_size;	/* output buffer h size */
	unsigned short	img_rb_v_size;	/* output buffer v size */
}gpCdspImg_t;

typedef struct gpCdspModule_s
{
	struct v4l2_interface Interface;
	gpCsiMclk_t			mclk;
	gpCdspImg_t			image;
	gpCdspScalePara_t	scale;
	unsigned int		suppr_mode;	/* suppression mode */
	unsigned int 		hislowcnt;	/* histgm low count */
	unsigned int 		hishicnt;	/* histgm hight count */
	gpCdsp3aResult_t	a3_result;
}gpCdspModule_t;

typedef struct gpCdspDev_s
{
	struct miscdevice 	dev;
	struct v4l2_subdev 	*sd;
	struct semaphore 	sem;
	wait_queue_head_t 	cdsp_wait_queue;
	bool 				cdsp_done;

	callbackfunc_t	*cb_func;
	unsigned char	*port;
	gpCdspModule_t	*cur_module;
	unsigned char 	open_cnt;
	unsigned char 	start_flag;
	unsigned char	update_flag;
	unsigned char	input_index;

	unsigned char 	*aewin_addr[2];
}gpCdspDev_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
unsigned short g_lenscmp_table[] =
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

unsigned char g_gamma_table[512] =
{
	0x00,0x50,0x15,0x00,0x06,0x55,0x11,0x00,0x0d,0x45,0x15,0x00,0x13,0x55,0x14,0x00,
	0x1a,0x51,0x14,0x00,0x1f,0x15,0x05,0x00,0x25,0x51,0x11,0x00,0x2a,0x11,0x05,0x00,
	0x2f,0x14,0x11,0x00,0x33,0x11,0x11,0x00,0x37,0x44,0x10,0x00,0x3a,0x41,0x04,0x00,
	0x3e,0x44,0x10,0x00,0x41,0x41,0x04,0x00,0x44,0x11,0x11,0x00,0x48,0x44,0x04,0x00,
	0x4c,0x10,0x11,0x00,0x4f,0x41,0x04,0x00,0x52,0x11,0x04,0x00,0x56,0x10,0x11,0x00,
	0x59,0x41,0x04,0x00,0x5c,0x11,0x04,0x00,0x60,0x10,0x01,0x00,0x63,0x10,0x01,0x00,
	0x66,0x04,0x01,0x00,0x69,0x04,0x01,0x00,0x6c,0x10,0x01,0x00,0x6f,0x10,0x04,0x00,
	0x72,0x10,0x04,0x00,0x74,0x41,0x10,0x00,0x77,0x04,0x01,0x00,0x7a,0x10,0x04,0x00,
	0x7c,0x41,0x10,0x00,0x7f,0x04,0x01,0x00,0x82,0x40,0x10,0x00,0x84,0x04,0x01,0x00,
	0x86,0x41,0x10,0x00,0x89,0x10,0x04,0x00,0x8b,0x01,0x01,0x00,0x8e,0x40,0x10,0x00,
	0x90,0x10,0x04,0x00,0x92,0x04,0x04,0x00,0x94,0x01,0x01,0x00,0x96,0x01,0x01,0x00,
	0x98,0x41,0x00,0x00,0x9b,0x40,0x00,0x00,0x9d,0x40,0x00,0x00,0x9f,0x40,0x00,0x00,
	0xa0,0x01,0x01,0x00,0xa2,0x01,0x01,0x00,0xa4,0x04,0x04,0x00,0xa6,0x04,0x10,0x00,
	0xa8,0x10,0x00,0x00,0xaa,0x00,0x01,0x00,0xab,0x01,0x04,0x00,0xad,0x10,0x00,0x00,
	0xaf,0x40,0x00,0x00,0xb0,0x01,0x04,0x00,0xb2,0x10,0x00,0x00,0xb3,0x01,0x04,0x00,
	0xb5,0x10,0x00,0x00,0xb7,0x00,0x04,0x00,0xb8,0x10,0x00,0x00,0xb9,0x01,0x04,0x00,
	0xbb,0x40,0x00,0x00,0xbc,0x04,0x00,0x00,0xbe,0x00,0x01,0x00,0xbf,0x10,0x00,0x00,
	0xc0,0x01,0x00,0x00,0xc2,0x00,0x04,0x00,0xc3,0x40,0x00,0x00,0xc4,0x04,0x00,0x00,
	0xc5,0x01,0x00,0x00,0xc7,0x00,0x04,0x00,0xc8,0x00,0x01,0x00,0xc9,0x40,0x00,0x00,
	0xca,0x10,0x00,0x00,0xcb,0x04,0x00,0x00,0xcc,0x01,0x00,0x00,0xcd,0x01,0x04,0x00,
	0xcf,0x10,0x00,0x00,0xd1,0x00,0x01,0x00,0xd2,0x00,0x04,0x00,0xd3,0x00,0x04,0x00,
	0xd4,0x00,0x04,0x00,0xd5,0x00,0x01,0x00,0xd6,0x00,0x01,0x00,0xd7,0x00,0x01,0x00,
	0xd8,0x00,0x01,0x00,0xd9,0x00,0x01,0x00,0xda,0x10,0x00,0x00,0xdc,0x00,0x01,0x00,
	0xdd,0x04,0x10,0x00,0xdf,0x00,0x00,0x00,0xe0,0x00,0x00,0x00,0xe1,0x00,0x00,0x00,
	0xe2,0x00,0x01,0x00,0xe3,0x04,0x00,0x00,0xe5,0x00,0x00,0x00,0xe6,0x00,0x00,0x00,
	0xe6,0x04,0x00,0x00,0xe8,0x00,0x04,0x00,0xe9,0x40,0x00,0x00,0xea,0x00,0x01,0x00,
	0xeb,0x00,0x01,0x00,0xec,0x00,0x01,0x00,0xed,0x40,0x00,0x00,0xee,0x00,0x00,0x00,
	0xef,0x00,0x00,0x00,0xef,0x40,0x00,0x00,0xf0,0x04,0x00,0x00,0xf2,0x00,0x04,0x00,
	0xf3,0x00,0x00,0x00,0xf3,0x10,0x00,0x00,0xf4,0x40,0x00,0x00,0xf5,0x40,0x00,0x00,
	0xf6,0x40,0x00,0x00,0xf7,0x00,0x00,0x00,0xf7,0x01,0x00,0x00,0xf8,0x40,0x00,0x00,
	0xf9,0x04,0x00,0x00,0xfb,0x00,0x04,0x00,0xfc,0x00,0x00,0x00,0xfc,0x10,0x00,0x00,
	0xfd,0x00,0x10,0x00,0xfe,0x00,0x00,0x00,0xfe,0x00,0x10,0x00,0xff,0x00,0x00,0x00,
};

unsigned char g_edge_table[256] =
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

static gpCdspDev_t *p_cdsp_dev;
static sensor_config_t *sensor=NULL;
static struct v4l2_capability g_cdsp_cap= {
	.driver = "/dev/cdsp",
	.card = "Generalplus CDSP",
	.bus_info = "CDSP interface",
	.version = 0x020620,
	.capabilities = V4L2_CAP_VIDEO_CAPTURE
};


/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
static int
gp_cdsp_set_fifo_size(
	unsigned short width
)
{
	while(1) {
		if(width <= 0x100) {
			break;
		}
		width >>= 1;
	}

	DEBUG(KERN_WARNING "fifo=0x%x.\n", width);
	gpHalCdspSetSRAM(ENABLE, width);
	gpHalCdspSetLineInterval(0x28);
	return width;
}

static void
gp_cdsp_clock_enable(
	unsigned int enable
)
{
	unsigned char div;
	int clock_out;
	struct clk *clock;

	clock = clk_get(NULL, "clk_sys_ahb");
    clock_out = clk_get_rate(clock);
	div = clock_out/100000000;
#ifdef CONFIG_PM
	if(enable) {
		/*For CDSP write*/
		gp_enable_clock((int*)"READLTIME_ABT", 1);
		/*For CDSP read*/
		gp_enable_clock((int*)"2DSCAABT", 1);
		gpHalScuClkEnable(SCU_A_PERI_CDSP, SCU_A, 1);
		gpHalCdspSetClk(C_CDSP_CLK_ENABLE, div);
		gpHalCdspSetModuleReset(1);
	} else {
		gpHalCdspSetClk(C_CDSP_CLK_DISABLE, 0);
		gpHalScuClkEnable(SCU_A_PERI_CDSP, SCU_A, 0);
		/*For CDSP write*/
		gp_enable_clock((int*)"READLTIME_ABT", 0);
		/*For CDSP read*/
		gp_enable_clock((int*)"2DSCAABT", 0);
	}
#else
	gpHalScuClkEnable(SCU_A_PERI_CDSP, SCU_A, enable);
	gpHalCdspSetClk(C_CDSP_CLK_ENABLE, div);
	gpHalCdspSetModuleReset(1);
#endif
}

static void
gp_cdsp_set_badpixob(
	gpCdspBadPixOB_t *argp,
	gpCdspWhtBal_t *wh_bal
)
{
	/* bad pixel */
	if(argp->badpixen) {
		gpHalCdspSetBadPixel(argp->bprthr, argp->bpgthr, argp->bpbthr);
		gpHalCdspSetBadPixelEn(argp->badpixen, 0x03); /* enable mirror */
	} else {
		gpHalCdspSetBadPixelEn(DISABLE, 0x03);
	}

	/* optical black */
	if(argp->manuoben || argp->autooben) {
		/* enable white balance offset when use ob */
		if(wh_bal->wboffseten == 0) {
			wh_bal->wboffseten = 1;
		}
		
		gpHalCdspSetWbOffset(wh_bal->wboffseten, wh_bal->roffset, wh_bal->groffset, wh_bal->boffset, wh_bal->gboffset);
		gpHalCdspSetManuOB(argp->manuoben, argp->manuob);
		gpHalCdspSetAutoOB(argp->autooben, argp->obtype, argp->obHOffset, argp->obVOffset);
	} else {
		gpHalCdspSetManuOB(DISABLE, argp->manuob);
		gpHalCdspSetAutoOB(DISABLE, argp->obtype, argp->obHOffset, argp->obVOffset);
	}
}

static void
gp_cdsp_get_badpixob(
	gpCdspBadPixOB_t *argp
)
{
	/* bad pixel */
	gpHalCdspGetBadPixelEn(&argp->badpixen, &argp->reserved0);
	gpHalCdspGetBadPixel(&argp->bprthr, &argp->bpgthr, &argp->bpbthr);

	/* optical black */
	gpHalCdspGetManuOB(&argp->manuoben, &argp->manuob);
	gpHalCdspGetAutoOB(&argp->autooben, &argp->obtype, &argp->obHOffset, &argp->obVOffset);
	gpHalCdspGetAutoOBAvg(&argp->Ravg, &argp->GRavg, &argp->Bavg, &argp->GBavg);
}

static void
gp_cdsp_set_lens_cmp(
	unsigned char raw_flag,
	gpCdspLenCmp_t *argp
)
{
	if(argp->lcen) {
		if(argp->lenscmp_table) {
			gpHalCdspInitLensCmp(argp->lenscmp_table);
		} else {
			gpHalCdspInitLensCmp((unsigned short *)g_lenscmp_table);
		}
		
		gpHalCdspSetLensCmpPos(argp->centx, argp->centy, argp->xmoffset, argp->ymoffset);
		gpHalCdspSetLensCmp(argp->stepfactor, argp->xminc, argp->ymoinc, argp->ymeinc);
		gpHalCdspSetLensCmpEn(argp->lcen);
	} else {
		gpHalCdspSetLensCmpEn(DISABLE);
	}

	if(raw_flag) {
		gpHalCdspSetLensCmpPath(0);
	} else {
		gpHalCdspSetLensCmpPath(1);
	}
}

static void
gp_cdsp_get_lens_cmp(
	gpCdspLenCmp_t *argp
)
{
	gpHalCdspGetLensCmpPos(&argp->centx, &argp->centy, &argp->xmoffset, &argp->ymoffset);
	gpHalCdspGetLensCmp(&argp->stepfactor, &argp->xminc, &argp->ymoinc, &argp->ymeinc);
	argp->lcen = gpHalCdspGetLensCmpEn();
	argp->lenscmp_table = NULL;
}

static void
gp_cdsp_set_white_balance(
	gpCdspWhtBal_t *argp
)
{
	gpHalCdspSetWbOffset(argp->wboffseten,argp->roffset, argp->groffset, argp->boffset, argp->gboffset);
	gpHalCdspSetWbGain(argp->wbgainen, argp->rgain, argp->grgain, argp->bgain, argp->gbgain);
	if(argp->wbgainen) {
		gpHalCdspSetGlobalGain(argp->global_gain);
	}
}

static void
gp_cdsp_get_white_balance(
	gpCdspWhtBal_t *argp
)
{
	gpHalCdspGetWbOffset(&argp->wboffseten, &argp->roffset, &argp->groffset, &argp->boffset, &argp->gboffset);
	gpHalCdspGetWbGain(&argp->wbgainen, &argp->rgain, &argp->grgain, &argp->bgain, &argp->gbgain);
	argp->global_gain =	gpHalCdspGetGlobalGain();
}

static void
gp_cdsp_set_lut_gamma(
	gpCdspGamma_t *argp
)
{
	if(argp->lut_gamma_en)
	{
		if(argp->gamma_table) {
			gpHalCdspInitGamma(argp->gamma_table);
		} else {
			gpHalCdspInitGamma((unsigned int *)g_gamma_table);
		}
		gpHalCdspSetLutGammaEn(argp->lut_gamma_en);
	}
	else
	{
		gpHalCdspSetLutGammaEn(DISABLE);
	}
}

static void
gp_cdsp_get_lut_gamma(
	gpCdspGamma_t *argp
)
{
	argp->lut_gamma_en = gpHalCdspGetLutGammaEn();
	argp->gamma_table = NULL;
}

static void
gp_cdsp_set_intpl(
	unsigned short width,
	gpCdspIntpl_t *argp
)
{
#if 1
	/* down mirror enable */
	gpHalCdspSetIntplMirEn(0xF, 1, 0);
	/* down mirror, need enable extline */
	gpHalCdspSetLineCtrl(0);
	gpHalCdspSetExtLine(width, 0x280);
	gpHalCdspSetExtLinePath(1, 0);
#else
	/* down mirror disable */
	gpHalCdspSetIntplMirEn(0x7, 0, 0);
	gpHalCdspSetLineCtrl(0);
	gpHalCdspSetExtLine(width, 0x280);
	gpHalCdspSetExtLinePath(0, 0);
#endif
	gpHalCdspSetIntplThr(argp->int_low_thr, argp->int_hi_thr);
	gpHalCdspSetRawSpecMode(argp->rawspecmode);

	/* disbale suppression */
	gpHalCdspSetUvSupprEn(DISABLE);
}

static void
gp_cdsp_get_intpl(
	gpCdspIntpl_t *argp
)
{
	gpHalCdspGetIntplThr(&argp->int_low_thr, &argp->int_hi_thr);
	argp->rawspecmode = gpHalCdspGetRawSpecMode();
}

static void
gp_cdsp_set_edge(
	unsigned char raw_flag,
	gpCdspEdge_t *argp
)
{
	if(argp->edgeen) {
		if(argp->edge_table) {
			gpHalCdspInitEdgeLut(argp->edge_table);
		} else {
			gpHalCdspInitEdgeLut((unsigned char *)g_edge_table);
		}
		
		gpHalCdspSetEdgeLutTableEn(argp->eluten);
		gpHalCdspSetEdgeEn(argp->edgeen);
		gpHalCdspSetEdgeLCoring(argp->lhdiv, argp->lhtdiv, argp->lhcoring, argp->lhmode);
		gpHalCdspSetEdgeAmpga(argp->ampga);
		gpHalCdspSetEdgeDomain(argp->edgedomain);
		gpHalCdspSetEdgeQthr(argp->Qthr);

		if(raw_flag) {
			gpHalCdspSetEdgeSrc(0);
		} else {
			gpHalCdspSetEdgeSrc(1);
		}
		
		/*3x3 programing matrix */
		if(argp->lhmode == 0) {
			gpHalCdspSetEdgeFilter(0, argp->lf00, argp->lf01, argp->lf02);
			gpHalCdspSetEdgeFilter(1, argp->lf10, argp->lf11, argp->lf12);
			gpHalCdspSetEdgeFilter(2, argp->lf20, argp->lf21, argp->lf22);
		}
	} else {
		gpHalCdspSetEdgeEn(DISABLE);
		gpHalCdspSetEdgeLutTableEn(DISABLE);
	}
}

static void
gp_cdsp_get_edge(
	gpCdspEdge_t *argp
)
{
	argp->eluten = gpHalCdspGetEdgeLutTableEn();
	argp->edge_table = NULL;
	argp->edgeen = gpHalCdspGetEdgeEn();
	gpHalCdspGetEdgeLCoring(&argp->lhdiv, &argp->lhtdiv, &argp->lhcoring, &argp->lhmode);
	argp->ampga = gpHalCdspGetEdgeAmpga();
	argp->edgedomain = gpHalCdspGetEdgeDomain();
	argp->Qthr = gpHalCdspGetEdgeQCnt();

	gpHalCdspGetEdgeFilter(0, &argp->lf00, &argp->lf01, &argp->lf02);
	gpHalCdspGetEdgeFilter(1, &argp->lf10, &argp->lf11, &argp->lf12);
	gpHalCdspGetEdgeFilter(2, &argp->lf20, &argp->lf21, &argp->lf22);
}

static void
gp_cdsp_set_color_matrix(
	gpCdspCorMatrix_t *argp
)
{
	if(argp->colcorren) {
		gpHalCdspSetColorMatrix(0, argp->a11, argp->a12, argp->a13);
		gpHalCdspSetColorMatrix(1, argp->a21, argp->a22, argp->a23);
		gpHalCdspSetColorMatrix(2, argp->a31, argp->a32, argp->a33);
		gpHalCdspSetColorMatrixEn(argp->colcorren);
	} else {
		gpHalCdspSetColorMatrixEn(DISABLE);
	}
}

static void
gp_cdsp_get_color_matrix(
	gpCdspCorMatrix_t *argp
)
{
	gpHalCdspGetColorMatrix(0, &argp->a11, &argp->a12, &argp->a13);
	gpHalCdspGetColorMatrix(1, &argp->a21, &argp->a22, &argp->a23);
	gpHalCdspGetColorMatrix(2, &argp->a31, &argp->a32, &argp->a33);
	argp->colcorren = gpHalCdspGetColorMatrixEn();
}

static void
gp_cdsp_set_rgbtoyuv(
	gpCdspRgb2Yuv_t *argp
)
{
	gpHalCdspSetPreRBClamp(argp->pre_rbclamp);
	gpHalCdspSetRBClamp(argp->rbclampen, argp->rbclamp);
	if(argp->uvdiven) {
		gpHalCdspSetUvDivideEn(argp->uvdiven);
		gpHalCdspSetUvDivide(argp->Yvalue_T1, argp->Yvalue_T2, argp->Yvalue_T3,
							argp->Yvalue_T4, argp->Yvalue_T5, argp->Yvalue_T6);
	} else {
		gpHalCdspSetUvDivideEn(DISABLE);
	}
}

static void
gp_cdsp_get_rgbtoyuv(
	gpCdspRgb2Yuv_t *argp
)
{
	argp->pre_rbclamp = gpHalCdspGetPreRBClamp();
	gpHalCdspGetRBClamp(&argp->rbclampen, (unsigned char *)&argp->rbclamp);
	argp->uvdiven = gpHalCdspGetUvDivideEn();
	gpHalCdspGetUvDivide(&argp->Yvalue_T1, &argp->Yvalue_T2, &argp->Yvalue_T3,
						&argp->Yvalue_T4, &argp->Yvalue_T5, &argp->Yvalue_T6);
}

static void
gp_cdsp_set_yuv444_insert(
	gpCdspYuvInsert_t *argp
)
{
	unsigned char y_value, u_value, v_value;

	y_value = ((argp->y_corval & 0x0F) << 4)|(argp->y_coring & 0xF);
	u_value = ((argp->u_corval & 0x0F) << 4)|(argp->u_coring & 0xF);
	v_value = ((argp->v_corval & 0x0F) << 4)|(argp->v_coring & 0xF);
	gpHalCdspSetYuv444InsertEn(argp->yuv444_insert);
	gpHalCdspSetYuvCoring(y_value, u_value, v_value);
}

static void
gp_cdsp_get_yuv444_insert(
	gpCdspYuvInsert_t *argp
)
{
	unsigned char y_value, u_value, v_value;

	argp->yuv444_insert = gpHalCdspGetYuv444InsertEn();
	gpHalCdspGetYuvCoring(&y_value, &u_value, &v_value);
	argp->y_coring = y_value & 0x0F;
	argp->y_corval = (y_value >> 4) & 0x0F;
	argp->u_coring = v_value & 0x0F;
	argp->u_corval = (v_value >> 4) & 0x0F;
	argp->v_coring = v_value & 0x0F;
	argp->v_corval = (v_value >> 4) & 0x0F;
}

static void
gp_cdsp_set_yuv_havg(
	gpCdspYuvHAvg_t *argp
)
{
	gpHalCdspSetYuvHAvg(0x03, argp->ytype, argp->utype, argp->vtype);
}

static void
gp_cdsp_get_yuv_havg(
	gpCdspYuvHAvg_t *argp
)
{
	gpHalCdspGetYuvHAvg(&argp->reserved, &argp->ytype, &argp->utype, &argp->vtype);
}

static void
gp_cdsp_set_special_mode(
	gpCdspSpecMod_t *argp
)
{
	gpHalCdspSetYuvSpecModeBinThr(argp->binarthr);
	gpHalCdspSetYuvSpecMode(argp->yuvspecmode);
}

static void
gp_cdsp_get_special_mode(
	gpCdspSpecMod_t *argp
)
{
	argp->binarthr = gpHalCdspGetYuvSpecModeBinThr();
	argp->yuvspecmode = gpHalCdspGetYuvSpecMode();
}

static void
gp_cdsp_set_sat_hue(
	gpCdspSatHue_t *argp
)
{
	gpHalCdspSetYuvSPHue(argp->u_huesindata, argp->u_huecosdata, argp->v_huesindata, argp->v_huecosdata);
	gpHalCdspSetYuvSPEffOffset(argp->y_offset, argp->u_offset, argp->v_offset);
	gpHalCdspSetYuvSPEffScale(argp->y_scale, argp->u_scale, argp->v_scale);
	gpHalCdspSetBriContEn(argp->YbYcEn);
}

static void
gp_cdsp_get_sat_hue(
	gpCdspSatHue_t *argp
)
{
	gpHalCdspGetYuvSPHue(&argp->u_huesindata, &argp->u_huecosdata, &argp->v_huesindata, &argp->v_huecosdata);
	gpHalCdspGetYuvSPEffOffset(&argp->y_offset, &argp->u_offset, &argp->v_offset);
	gpHalCdspGetYuvSPEffScale(&argp->y_scale, &argp->u_scale, &argp->v_scale);
	argp->YbYcEn = gpHalCdspGetBriContEn();
}

static void
gp_cdsp_set_suppression(
	unsigned short width,
	gpCdspSuppression_t *argp,
	gpCdspEdge_t *edge
)
{
	if(argp->suppressen) {
	#if 1
		/* down mirror enable */
		gpHalCdspSetUvSuppr(1, 1, 0xF);
		/* use down mirror must enable extline */
		gpHalCdspSetLineCtrl(1);
		gpHalCdspSetExtLine(width, 0x280);
		gpHalCdspSetExtLinePath(1, 1);
	#else
		/* down mirror disable */
		gpHalCdspSetUvSuppr(1, 1, 0xD);
		gpHalCdspSetLineCtrl(1);
		gpHalCdspSetExtLine(width, 0x280);
		gpHalCdspSetExtLinePath(0, 0);
	#endif
		gpHalCdspSetUvSupprEn(ENABLE);
		if(argp->suppr_mode >  2)
			argp->suppr_mode = 2;

		switch(argp->suppr_mode)
		{
		case 0:
			gpHalCdspSetYDenoiseEn(DISABLE);
			gpHalCdspSetYLPFEn(DISABLE);
			gp_cdsp_set_edge(0, edge);
			break;
		case 1:
			gpHalCdspSetEdgeEn(DISABLE);
			gpHalCdspSetEdgeLutTableEn(DISABLE);
			gpHalCdspSetYLPFEn(DISABLE);
			gpHalCdspSetYDenoise(argp->denoisethrl, argp->denoisethrwth, argp->yhtdiv);
			gpHalCdspSetYDenoiseEn(argp->denoisen);
			break;
		case 2:
			gpHalCdspSetEdgeEn(DISABLE);
			gpHalCdspSetEdgeLutTableEn(DISABLE);
			gpHalCdspSetYDenoiseEn(DISABLE);
			gpHalCdspSetYLPFEn(argp->lowyen);
			break;
		}
	} else {
		/* dow mirror disable */
		gpHalCdspSetUvSuppr(0, 0, 0);
		gpHalCdspSetLineCtrl(0);
		gpHalCdspSetExtLine(width, 0x280);
		gpHalCdspSetExtLinePath(0, 0);

		gpHalCdspSetUvSupprEn(DISABLE);
		gpHalCdspSetEdgeEn(DISABLE);
		gpHalCdspSetEdgeLutTableEn(DISABLE);
		gpHalCdspSetYDenoiseEn(DISABLE);
		gpHalCdspSetYLPFEn(DISABLE);
	}
}

static void
gp_cdsp_get_suppression(
	gpCdspSuppression_t *argp
)
{
	gpHalCdspGetYDenoise(&argp->denoisethrl, &argp->denoisethrwth, &argp->yhtdiv);
	argp->denoisen = gpHalCdspGetYDenoiseEn();
	argp->lowyen = gpHalCdspGetYLPFEn();
}

static void
gp_cdsp_set_raw_win(
	unsigned short width,
	unsigned short height,
	gpCdspRawWin_t *argp
)
{
	unsigned int x, y;

	width -= 8;
	height -= 8;
	if(argp->hwdoffset == 0) {
		argp->hwdoffset = 1;
	}
	
	if(argp->vwdoffset == 0) {
		argp->vwdoffset = 1;
	}
	
	if(argp->aeawb_src == 0) {
		x = argp->hwdoffset + argp->hwdsize * 8;
		y = argp->vwdoffset + argp->vwdsize * 8;
		if(x >= width) {
			x = width - argp->hwdoffset;
			argp->hwdsize = x / 8;
		}

		if(y >= height) {
			y = height - argp->vwdoffset;
			argp->vwdsize = y / 8;
		}
	} else {
		x = argp->hwdoffset*2 + argp->hwdsize*2 * 8;
		y = argp->vwdoffset*2 + argp->vwdsize*2 * 8;
		if(x >= width) {
			x = width - argp->hwdoffset*2;
			argp->hwdsize = x / 8;
			argp->hwdsize >>= 1;
		}

		if(y >= height) {
			y = height - argp->vwdoffset*2;
			argp->vwdsize = y / 8;
			argp->vwdsize >>= 1;
		}
	}

	gpHalCdspSetAeAwbSrc(argp->aeawb_src);
	gpHalCdspSetAeAwbSubSample(argp->subample);
	gpHalCdspSet3ATestWinEn(argp->AeWinTest, argp->AfWinTest);
	gpHalCdspSetRGBWin(argp->hwdoffset, argp->vwdoffset, argp->hwdsize, argp->vwdsize);
	DEBUG(KERN_WARNING "RawWinOffset[%d,%d]\n", argp->hwdoffset, argp->vwdoffset);
	DEBUG(KERN_WARNING "RawWinCellSize[%d,%d]\n", argp->hwdsize, argp->vwdsize);
}

static void
gp_cdsp_get_raw_win(
	gpCdspRawWin_t *argp
)
{
	argp->aeawb_src = gpHalCdspGetAeAwbSrc();
	argp->subample = gpHalCdspGetAeAwbSubSample();
	gpHalCdspGet3ATestWinEn(&argp->AeWinTest, &argp->AfWinTest);
	gpHalCdspGetRGBWin(&argp->hwdoffset, &argp->vwdoffset, &argp->hwdsize, &argp->vwdsize);
}

static void
gp_cdsp_set_af(
	unsigned short width,
	unsigned short height,
	gpCdspAF_t *argp
)
{
	if(argp->af_win_en) {
		unsigned int x, y;
		/* af1 */
		if(argp->af1_hsize >= width) {
			argp->af1_hsize = width;
		}

		if(argp->af1_vsize >= height) {
			argp->af1_hsize = height;
		}
		
		x = argp->af1_hoffset + argp->af1_hsize;
		y = argp->af1_voffset + argp->af1_vsize;
		if(x >= width) {
			argp->af1_hoffset = width - 1 - argp->af1_hsize;
		}
		
		if(y >= height) {
			argp->af1_voffset = height - 1 - argp->af1_vsize;
		}
		
		/* af2 */
		if(argp->af2_hsize >= width) {
			argp->af2_hsize = width;
		}

		if(argp->af2_vsize >= height) {
			argp->af2_hsize = height;
		}

		if(argp->af2_hsize < 64) {
			argp->af2_hsize = 64;
		}
		
		if(argp->af2_vsize < 64) {
			argp->af2_vsize = 64;
		}
		
		x = argp->af2_hoffset + argp->af2_hsize;
		y = argp->af2_voffset + argp->af2_vsize;
		if(x >= width) {
			argp->af2_hoffset = width - 1 - argp->af2_hsize;
		}

		if(y >= height) {
			argp->af2_voffset = height - 1 - argp->af2_vsize;
		}
		
		/* af3 */
		if(argp->af3_hsize >= width) {
			argp->af3_hsize = width;
		}
		
		if(argp->af3_vsize >= height) {
			argp->af3_hsize = height;
		}

		if(argp->af3_hsize < 64) {
			argp->af3_hsize = 64;
		}

		if(argp->af3_vsize < 64) {
			argp->af3_vsize = 64;
		}
		
		x = argp->af3_hoffset + argp->af3_hsize;
		y = argp->af3_voffset + argp->af3_vsize;
		if(x >= width) {
			argp->af3_hoffset = width - 1 - argp->af3_hsize;
		}

		if(y >= height) {
			argp->af3_voffset = height - 1 - argp->af3_vsize;
		}
		
		gpHalCdspSetAfWin1(argp->af1_hoffset, argp->af1_voffset, argp->af1_hsize, argp->af1_hsize);
		gpHalCdspSetAfWin2(argp->af2_hoffset, argp->af2_voffset, argp->af2_hsize, argp->af2_hsize);
		gpHalCdspSetAfWin3(argp->af3_hoffset, argp->af3_voffset, argp->af3_hsize, argp->af3_hsize);
		gpHalCdspSetAFEn(argp->af_win_en, argp->af_win_hold);
		DEBUG(KERN_WARNING "Af1Offset[%d,%d]\n", argp->af1_hoffset, argp->af1_voffset);
		DEBUG(KERN_WARNING "Af1Size[%d,%d]\n", argp->af1_hsize, argp->af1_vsize);
		DEBUG(KERN_WARNING "Af2Offset[%d,%d]\n", argp->af2_hoffset, argp->af2_voffset);
		DEBUG(KERN_WARNING "Af2Size[%d,%d]\n", argp->af2_hsize, argp->af2_vsize);
		DEBUG(KERN_WARNING "Af3Offset[%d,%d]\n", argp->af3_hoffset, argp->af3_voffset);
		DEBUG(KERN_WARNING "Af3Size[%d,%d]\n", argp->af3_hsize, argp->af3_vsize);
	} else {
		gpHalCdspSetAFEn(DISABLE, DISABLE);
	}
}

static void
gp_cdsp_get_af(
	gpCdspAF_t *argp
)
{
	gpHalCdspGetAfWin1(&argp->af1_hoffset, &argp->af1_voffset, &argp->af1_hsize, &argp->af1_hsize);
	gpHalCdspGetAfWin2(&argp->af2_hoffset, &argp->af2_voffset, &argp->af2_hsize, &argp->af2_hsize);
	gpHalCdspGetAfWin3(&argp->af3_hoffset, &argp->af3_voffset, &argp->af3_hsize, &argp->af3_hsize);
	gpHalCdspGetAFEn(&argp->af_win_en, &argp->af_win_hold);
}

static void
gp_cdsp_set_ae(
	unsigned char phaccfactor,
	unsigned char pvaccfactor,
	gpCdspAE_t *argp
)
{
	if(argp->ae_win_en) {
		unsigned int ae0, ae1;
		
		ae0 = (unsigned int)gp_chunk_pa((void *)p_cdsp_dev->aewin_addr[0]);
		ae1 = (unsigned int)gp_chunk_pa((void *)p_cdsp_dev->aewin_addr[1]);
		gpHalCdspSetAEBuffAddr(ae0, ae1);
		gpHalCdspSetAEEn(argp->ae_win_en, argp->ae_win_hold);
		gpHalCdspSetAEWin(phaccfactor, pvaccfactor);
		DEBUG(KERN_WARNING "AeWinAddr0 = 0x%x.\n", ae0);
		DEBUG(KERN_WARNING "AeWinAddr1 = 0x%x.\n", ae1);
	} else {
		gpHalCdspSetAEEn(DISABLE, DISABLE);
	}
}

static void
gp_cdsp_get_ae(
	gpCdspAE_t *argp
)
{
	gpHalCdspGetAEEn(&argp->ae_win_en, &argp->ae_win_hold);
}

static void
gp_cdsp_set_awb(
	gpCdspAWB_t *argp
)
{
	if(argp->awb_win_en) {
		gpHalCdspSetAWB(argp->awbclamp_en, argp->sindata, argp->cosdata, argp->awbwinthr);
		gpHalCdspSetAwbYThr(argp->Ythr0, argp->Ythr1, argp->Ythr2, argp->Ythr3);
		gpHalCdspSetAwbUVThr(1,	argp->UL1N1, argp->UL1P1, argp->VL1N1, argp->VL1P1);
		gpHalCdspSetAwbUVThr(2,	argp->UL1N2, argp->UL1P2, argp->VL1N2, argp->VL1P2);
		gpHalCdspSetAwbUVThr(3,	argp->UL1N3, argp->UL1P3, argp->VL1N3, argp->VL1P3);
		gpHalCdspSetAWBEn(argp->awb_win_en, argp->awb_win_hold);
	} else {
		gpHalCdspSetAWBEn(DISABLE, DISABLE);
	}
}

static void
gp_cdsp_get_awb(
	gpCdspAWB_t *argp
)
{
	gpHalCdspGetAWB(&argp->awbclamp_en, &argp->sindata, &argp->cosdata, &argp->awbwinthr);
	gpHalCdspGetAwbYThr(&argp->Ythr0, &argp->Ythr1, &argp->Ythr2, &argp->Ythr3);
	gpHalCdspGetAwbUVThr(1,	&argp->UL1N1, &argp->UL1P1, &argp->VL1N1, &argp->VL1P1);
	gpHalCdspGetAwbUVThr(2,	&argp->UL1N2, &argp->UL1P2, &argp->VL1N2, &argp->VL1P2);
	gpHalCdspGetAwbUVThr(3,	&argp->UL1N3, &argp->UL1P3, &argp->VL1N3, &argp->VL1P3);
	gpHalCdspGetAWBEn(&argp->awb_win_en, &argp->awb_win_hold);
}

static void
gp_cdsp_set_wbgain2(
	gpCdspWbGain2_t *argp
)
{
	if(argp->wbgain2en) {
		gpHalCdspSetWbGain2(argp->rgain2, argp->ggain2, argp->bgain2);
		gpHalCdspSetWbGain2En(ENABLE);
	} else {
		gpHalCdspSetWbGain2En(DISABLE);
	}
}

static void
gp_cdsp_get_wbgain2(
	gpCdspWbGain2_t *argp
)
{
	gpHalCdspGetWbGain2(&argp->rgain2, &argp->ggain2, &argp->bgain2);
	argp->wbgain2en = gpHalCdspGetWbGain2En();
}

static void
gp_cdsp_set_histgm(
	gpCdspHistgm_t *argp
)
{
	if(argp->his_en) {
		gpHalCdspSetHistgm(argp->hislowthr, argp->hishighthr);
		gpHalCdspSetHistgmEn(ENABLE, argp->his_hold_en);
	} else {
		gpHalCdspSetHistgmEn(DISABLE, DISABLE);
	}
}

static void
gp_cdsp_get_histgm(
	gpCdspHistgm_t *argp
)
{
	gpHalCdspGetHistgm(&argp->hislowthr, &argp->hishighthr);
	gpHalCdspGetHistgmEn(&argp->his_en, &argp->his_hold_en);
}

static int
gp_cdsp_set_scale_crop(
	gpCdspImg_t	*imgp,
	gpCdspScalePara_t *argp
)
{
	unsigned char clamphsizeen;
	unsigned short clamphsize, src_width, src_height;
	unsigned int temp;

	src_width = imgp->img_h_size;
	src_height = imgp->img_v_size;
	clamphsizeen = 0;
	clamphsize = argp->img_rb_h_size;

	/* raw h scale down*/
	if(imgp->raw_flag) {
		if(argp->hscale_en && (src_width > argp->dst_hsize)) {
			DEBUG(KERN_WARNING "HScaleEn\n");
			argp->dst_hsize &= ~(0x1); 	/* 2 align */
			src_width = argp->dst_hsize;
			clamphsize = argp->dst_hsize;
			gpHalCdspSetRawHScale(src_width, argp->dst_hsize);
			gpHalCdspSetRawHScaleEn(argp->hscale_en, argp->hscale_mode);
		} else {
			argp->hscale_en = 0;
			gpHalCdspSetRawHScaleEn(DISABLE, argp->hscale_mode);
		}
	} else {
		argp->hscale_en = 0;
		gpHalCdspSetRawHScaleEn(DISABLE, argp->hscale_mode);
	}

	/* crop */
	if((argp->hscale_en == 0) &&
		argp->crop_en &&
		(src_width > argp->crop_hsize) &&
		(src_height > argp->crop_vsize)) {
		
		DEBUG(KERN_WARNING "CropEn\n");
		if(argp->crop_hoffset == 0) {
			argp->crop_hoffset = 1;
		}

		if(argp->crop_voffset == 0) {
			argp->crop_voffset = 1;
		}
		
		temp = argp->crop_hoffset + argp->crop_hsize;
		if(temp > src_width) {
			argp->crop_hsize = src_width - argp->crop_hoffset;
		}
		
		temp = argp->crop_voffset + argp->crop_vsize;
		if(temp > src_height) {
			argp->crop_vsize = src_height - argp->crop_voffset;
		}
		
		src_width = argp->crop_hsize;
		src_height = argp->crop_vsize;
		clamphsize = argp->crop_hsize;
		gpHalCdspSetCrop(argp->crop_hoffset, argp->crop_voffset, argp->crop_hsize, argp->crop_vsize);
		gpHalCdspSetCropEn(argp->crop_en);
	} else {
		argp->crop_en = 0;
		gpHalCdspSetCropEn(DISABLE);
	}

	/* yuv h scale down*/
	if(argp->yuvhscale_en && (src_width > argp->yuv_dst_hsize)) {
		DEBUG(KERN_WARNING "YuvHScaleEn\n");
		if(argp->yuv_dst_hsize > argp->img_rb_h_size) {
			argp->yuv_dst_hsize = argp->img_rb_h_size;
		}
		
		argp->yuv_dst_hsize &= ~(0x1); 	/* 2 align */
		clamphsizeen = 1;
		clamphsize = argp->yuv_dst_hsize;
		temp = (argp->yuv_dst_hsize<<16)/src_width + 1;
		gpHalCdspSetYuvHScale(temp, temp);
		gpHalCdspSetYuvHScaleEn(argp->yuvhscale_en, argp->yuvhscale_mode);
	} else if(src_width > argp->img_rb_h_size) {
		DEBUG(KERN_WARNING "YuvHScaleEn1\n");
		argp->yuv_dst_hsize &= ~(0x1); 	/* 2 align */
		clamphsizeen = 1;
		clamphsize = argp->img_rb_h_size;
		temp = (argp->img_rb_h_size<<16)/src_width + 1;
		gpHalCdspSetYuvHScale(temp, temp);
		gpHalCdspSetYuvHScaleEn(ENABLE, argp->yuvhscale_mode);
	} else {
		argp->yuvhscale_en = 0;
		gpHalCdspSetYuvHScaleEn(DISABLE, argp->yuvhscale_mode);
	}

	/* yuv v scale down*/
	if(argp->yuvvscale_en && (src_height > argp->yuv_dst_vsize)) {
		DEBUG(KERN_WARNING "YuvVScaleEn\n");
		if(argp->yuv_dst_vsize >  argp->img_rb_v_size) {
			argp->yuv_dst_vsize = argp->img_rb_v_size;
		}
		
		argp->yuv_dst_vsize &= ~(0x1); 	/* 2 align */
		temp = (argp->yuv_dst_vsize<<16)/src_height + 1;
		gpHalCdspSetYuvVScale(temp, temp);
		gpHalCdspSetYuvVScaleEn(argp->yuvvscale_en, argp->yuvvscale_mode);
	} else if(src_height > argp->img_rb_v_size) {
		DEBUG(KERN_WARNING "YuvVScaleEn1\n");
		argp->yuv_dst_vsize &= ~(0x1); 	/* 2 align */
		temp = (argp->img_rb_v_size<<16)/src_height + 1;
		gpHalCdspSetYuvVScale(temp, temp);
		gpHalCdspSetYuvVScaleEn(ENABLE, argp->yuvvscale_mode);
	} else {
		argp->yuvvscale_en = 0;
		gpHalCdspSetYuvVScaleEn(DISABLE, argp->yuvhscale_mode);
	}

	/* set clamp enable and clamp h size */
	gpHalCdspSetClampEn(clamphsizeen, clamphsize);
	imgp->img_rb_h_size = argp->img_rb_h_size;
	imgp->img_rb_v_size = argp->img_rb_v_size;
	p_cdsp_dev->update_flag = 1;
	DEBUG(KERN_WARNING "Clampen = %d\n", clamphsizeen);
	DEBUG(KERN_WARNING "ClampSize = %d\n", clamphsize);
	DEBUG(KERN_WARNING "rb_h_size = %d\n", imgp->img_rb_h_size);
	DEBUG(KERN_WARNING "rb_v_size = %d\n", imgp->img_rb_v_size);
	return 0;
}

static void
gp_cdsp_get_scale_crop(
	gpCdspScalePara_t *argp
)
{
	gpHalCdspGetRawHScaleEn(&argp->hscale_en, &argp->hscale_mode);

	gpHalCdspGetCrop(&argp->crop_hoffset, &argp->crop_voffset, &argp->crop_hsize, &argp->crop_vsize);
	argp->crop_en = gpHalCdspGetCropEn();

	gpHalCdspGetYuvHScaleEn(&argp->yuvhscale_en, &argp->yuvhscale_mode);
	gpHalCdspGetYuvVScaleEn(&argp->yuvvscale_en, &argp->yuvvscale_mode);
}

static void
gp_cdsp_set_scale_ae_af(
	gpCdspModule_t *argp
)
{
	unsigned short w, h;
	gpCdspRawWin_t raw_win;
	gpCdspAE_t ae;
	gpCdspAF_t af;

	if(argp->scale.crop_en) {
		w = argp->scale.crop_hsize;
		h = argp->scale.crop_vsize;
	} else {
		w = argp->image.img_h_size;
		h = argp->image.img_v_size;
	}

	gp_cdsp_get_raw_win(&raw_win);
	gp_cdsp_set_raw_win(w, h, &raw_win);

	gp_cdsp_get_af(&af);
	gp_cdsp_set_af(w, h, &af);

	gp_cdsp_get_ae(&ae);
	gp_cdsp_set_ae(raw_win.hwdsize, raw_win.vwdsize, &ae);
}

static int
gp_cdsp_sdram_start(
	gpCdspModule_t *argp
)
{
	if(argp->image.raw_flag) {
		DEBUG(KERN_WARNING "SDRAMRawFmt\n");
		gpHalCdspSetYuvRange(0x00);
		gpHalCdspSetRawDataFormat(argp->image.raw_format);
		gpHalCdspFrontSetInputFormat(argp->image.cdspin_fmt);
		gpHalCdspSetRawBuff(argp->image.img_rb_h_size,
							argp->image.img_rb_v_size,
							0x00,
							argp->image.frame[0].phy_addr);
		gpHalCdspSetYuvBuffA(argp->image.img_rb_h_size,
							argp->image.img_rb_v_size,
							argp->image.frame[1].phy_addr);
		gpHalCdspSetDmaBuff(RD_A_WR_A);
#ifndef GP_SYNC_OPTION
		/* clean dcache */
		gp_clean_dcache_range(argp->image.frame[0].addr, argp->image.frame[0].length/2);
#else
		GP_SYNC_CACHE();
#endif
	} else {
		DEBUG(KERN_WARNING "SDRAMYuvFmt\n");
		gpHalCdspSetYuvRange(argp->image.yuv_range);
		gpHalCdspSetRawDataFormat(0x00);
		gpHalCdspFrontSetInputFormat(argp->image.cdspin_fmt);
		gpHalCdspSetYuvBuffA(argp->image.img_rb_h_size,
							argp->image.img_rb_v_size,
							argp->image.frame[0].phy_addr);
		gpHalCdspSetYuvBuffB(argp->image.img_rb_h_size,
							argp->image.img_rb_v_size,
							argp->image.frame[1].phy_addr);
		gpHalCdspSetDmaBuff(RD_A_WR_B);
#ifndef GP_SYNC_OPTION
		/* clean dcache */
		gp_clean_dcache_range(argp->image.frame[0].addr, argp->image.frame[0].length);
#else
		GP_SYNC_CACHE();
#endif
	}

	gpHalCdspSetReadBackSize(0x00, 0x00,
							argp->image.img_rb_h_size,
							argp->image.img_rb_v_size);
	DEBUG(KERN_WARNING "InPhyAddr = 0x%x.\n", argp->image.frame[0].phy_addr);
	DEBUG(KERN_WARNING "OutPhyAddr = 0x%x.\n", argp->image.frame[1].phy_addr);
	gpHalCdspClrIntStatus(CDSP_INT_ALL);
	gpHalCdspSetIntEn(ENABLE, CDSP_INT_ALL);
	gpHalCdspDataSource(C_CDSP_SDRAM);
	gpHalCdspRedoTriger(1);
	return 0;
}

static int
gp_cdsp_front_start(
	gpCdspModule_t *argp
)
{
	unsigned int temp;

	if(argp->image.raw_flag) {
		DEBUG(KERN_WARNING "FrontRawFmt\n");
		gpHalCdspSetYuvRange(0x00);
		gpHalCdspSetRawDataFormat(argp->image.raw_format);
	} else {
		DEBUG(KERN_WARNING "FrontYuvFmt\n");
		gpHalCdspSetYuvRange(0x00);
		gpHalCdspSetRawDataFormat(0x00);
	}

	gpHalCdspFrontSetInputFormat(argp->image.cdspin_fmt);
	gpHalCdspFrontSetInterface((argp->Interface.Interface == CCIR656) ? 1 : 0,
								argp->Interface.HsyncAct ? 0 : 1,
								argp->Interface.VsyncAct ? 0 : 1,
								argp->mclk.prvi);
	//gpHalCdspFrontSetInterlace(argp->Interface.Field, argp->Interface.Interlace);
	temp = argp->image.img_v_size;
	if(argp->Interface.Interface == HREF) {
		temp -= 1;
	}
	
	gpHalCdspFrontSetFrameSize(argp->image.img_h_offset,
							argp->image.img_v_offset,
							argp->image.img_h_size,
							temp);
	if(argp->image.rgb_path) {
		if(argp->image.raw_flag) {
			gpHalCdspSetRawPath(argp->image.rgb_path, 1, 1);
		} else {
			gpHalCdspSetRawPath(argp->image.rgb_path, 0, 1);
		}
		
		gpHalCdspSetRawBuff(argp->image.img_rb_h_size,
							argp->image.img_rb_v_size,
							0x00,
							argp->image.frame[0].phy_addr);
	} else {
	#if C_DMA_A == 1
		gpHalCdspSetYuvBuffA(argp->image.img_rb_h_size,
							argp->image.img_rb_v_size,
							argp->image.frame[0].phy_addr);
		gpHalCdspSetDmaBuff(RD_A_WR_A);
	#else
		gpHalCdspSetYuvBuffB(argp->image.img_rb_h_size,
							argp->image.img_rb_v_size,
							argp->image.frame[0].phy_addr);
		gpHalCdspSetDmaBuff(RD_B_WR_B);
	#endif
		argp->image.frame[0].flag = V4L2_BUF_FLAG_MAPPED;
		DEBUG(KERN_WARNING "frame = 0x%x.\n", argp->image.frame[0].phy_addr);
	}

	gpHalCdspClrIntStatus(CDSP_INT_ALL);
	gpHalCdspSetIntEn(ENABLE, CDSP_INT_ALL);
	gpHalCdspDataSource(C_CDSP_FRONT);
	return 0;
}

static int
gp_cdsp_mipi_start(
	gpCdspModule_t *argp
)
{
	if(argp->image.raw_flag) {
		DEBUG(KERN_WARNING "MipiRawFmt\n");
		gpHalCdspSetYuvRange(0x00);
		gpHalCdspSetRawDataFormat(argp->image.raw_format);
	} else {
		DEBUG(KERN_WARNING "MipiYuvFmt\n");
		gpHalCdspSetYuvRange(0x00);
		gpHalCdspSetRawDataFormat(0x00);
	}

	gpHalCdspFrontSetInputFormat(argp->image.cdspin_fmt);
	gpHalCdspFrontSetMipiFrameSize(argp->image.img_h_offset,
								argp->image.img_v_offset,
								argp->image.img_h_size,
								argp->image.img_v_size);
	if(argp->image.rgb_path) {
		if(argp->image.raw_flag) {
			gpHalCdspSetRawPath(argp->image.rgb_path, 1, 1);
		} else {
			gpHalCdspSetRawPath(argp->image.rgb_path, 0, 1);
		}
		
		gpHalCdspSetRawBuff(argp->image.img_rb_h_size,
							argp->image.img_rb_v_size,
							0x00,
							argp->image.frame[0].phy_addr);
	} else {
	#if C_DMA_A == 1
		gpHalCdspSetYuvBuffA(argp->image.img_rb_h_size,
							argp->image.img_rb_v_size,
							argp->image.frame[0].phy_addr);
		gpHalCdspSetDmaBuff(RD_A_WR_A);
	#else
		gpHalCdspSetYuvBuffB(argp->image.img_rb_h_size,
							argp->image.img_rb_v_size,
							argp->image.frame[0].phy_addr);
		gpHalCdspSetDmaBuff(RD_B_WR_B);
	#endif
		argp->image.frame[0].flag = V4L2_BUF_FLAG_MAPPED;
		DEBUG(KERN_WARNING "frame = 0x%x.\n", argp->image.frame[0].phy_addr);
	}

	gpHalCdspClrIntStatus(CDSP_INT_ALL);
	gpHalCdspSetIntEn(ENABLE, CDSP_INT_ALL);
	gpHalCdspDataSource(C_CDSP_MIPI);
	return 0;
}

static int
gp_cdsp_stop(
	void
)
{
	DEBUG(KERN_WARNING "CdspStop\n");
	gpHalCdspSetIntEn(DISABLE, CDSP_INT_ALL);
	gpHalCdspClrIntStatus(CDSP_INT_ALL);
	gpHalCdspDataSource(C_CDSP_SDRAM);
	gpHalCdspRedoTriger(DISABLE);
	return 0;
}

static int
gp_cdsp_start(
	gpCdspModule_t *argp
)
{
	int nRet = 0;
	gpCdspYuvHAvg_t yuv_havg;
	gpCdspLenCmp_t lens_cmp;
	gpCdspEdge_t edge;

	DEBUG(KERN_WARNING "CdspStart\n");
	/* cdsp reset */
	gpHalCdspReset();
	gpHalCdspFrontReset();
	gpHalCdspDataSource(C_CDSP_SDRAM);

	/* set module */
	if(argp->image.raw_flag) {
		gpCdspIntpl_t intpl;

		gp_cdsp_get_intpl(&intpl);
		gp_cdsp_set_intpl(argp->image.img_h_size, &intpl);

		gp_cdsp_get_edge(&edge);
		gp_cdsp_set_edge(argp->image.raw_flag, &edge);
	} else {
		gpCdspSuppression_t suppression;

		gp_cdsp_get_edge(&edge);
		gp_cdsp_set_suppression(argp->image.img_h_size, &suppression, &edge);
	}

	gp_cdsp_get_lens_cmp(&lens_cmp);
	gp_cdsp_set_lens_cmp(argp->image.raw_flag, &lens_cmp);

	gp_cdsp_get_yuv_havg(&yuv_havg);
	gp_cdsp_set_yuv_havg(&yuv_havg);

	/* set scale & crop */
	gp_cdsp_set_scale_crop(&argp->image, &argp->scale);
	gp_cdsp_set_scale_ae_af(argp);

	/* set fifo */
	gp_cdsp_set_fifo_size(argp->image.img_h_size);

	/* start cdsp */
	if(argp->image.img_src == C_CDSP_SDRAM) {
		nRet = gp_cdsp_sdram_start(argp);
	} else if(argp->image.img_src == C_CDSP_FRONT) {
		nRet = gp_cdsp_front_start(argp);
	} else if(argp->image.img_src == C_CDSP_MIPI) {
		nRet = gp_cdsp_mipi_start(argp);
	}

	if(nRet < 0) {
		gp_cdsp_stop();
	}
	return nRet;
}

static int
gp_cdsp_s_fmt(
	gpCdspModule_t *pModule
)
{
	int nRet = 0;

	switch(pModule->image.img_src)
	{
	case C_CDSP_SDRAM:
		pModule->image.sensor_flag = 0;
		if(pModule->image.v4l2in_fmt == V4L2_PIX_FMT_VYUY) {
			pModule->image.cdspin_fmt = C_SDRAM_FMT_VY1UY0;
			pModule->image.raw_flag = 0;
		} else if(pModule->image.v4l2in_fmt == V4L2_PIX_FMT_SBGGR8) {
			pModule->image.cdspin_fmt = C_SDRAM_FMT_RAW8;
			pModule->image.raw_flag = 1;
			pModule->image.raw_format = 2;
		} else if(pModule->image.v4l2in_fmt == V4L2_PIX_FMT_SGBRG8) {
			pModule->image.cdspin_fmt = C_SDRAM_FMT_RAW8;
			pModule->image.raw_flag = 1;
			pModule->image.raw_format = 3;
		} else if(pModule->image.v4l2in_fmt == V4L2_PIX_FMT_SGRBG8) {
			pModule->image.cdspin_fmt = C_SDRAM_FMT_RAW8;
			pModule->image.raw_flag = 1;
			pModule->image.raw_format = 0;
		} else if(pModule->image.v4l2in_fmt == V4L2_PIX_FMT_SGRBG10) {
			pModule->image.cdspin_fmt = C_SDRAM_FMT_RAW10;
			pModule->image.raw_flag = 1;
			pModule->image.raw_format = 1;
		} else {
			RETURN(-EINVAL);
		}
		break;

	case C_CDSP_FRONT:
		pModule->image.sensor_flag = 1;
		if(pModule->image.v4l2in_fmt == V4L2_PIX_FMT_YUYV) {
			pModule->image.cdspin_fmt = C_FRONT_FMT_Y1UY0V;
			pModule->image.raw_flag = 0;
		} else if(pModule->image.v4l2in_fmt == V4L2_PIX_FMT_YVYU) {
			pModule->image.cdspin_fmt = C_FRONT_FMT_Y1VY0U;
			pModule->image.raw_flag = 0;
		} else if(pModule->image.v4l2in_fmt == V4L2_PIX_FMT_UYVY) {
			pModule->image.cdspin_fmt = C_FRONT_FMT_UY1VY0;
			pModule->image.raw_flag = 0;
		} else if(pModule->image.v4l2in_fmt == V4L2_PIX_FMT_VYUY) {
			pModule->image.cdspin_fmt = C_FRONT_FMT_VY1UY0;
			pModule->image.raw_flag = 0;
		} else if(pModule->image.v4l2in_fmt == V4L2_PIX_FMT_SBGGR8) {
			pModule->image.cdspin_fmt = C_FRONT_FMT_RAW8;
			pModule->image.raw_flag = 1;
			pModule->image.raw_format = 2;
		} else if(pModule->image.v4l2in_fmt == V4L2_PIX_FMT_SGBRG8) {
			pModule->image.cdspin_fmt = C_FRONT_FMT_RAW8;
			pModule->image.raw_flag = 1;
			pModule->image.raw_format = 3;
		} else if(pModule->image.v4l2in_fmt == V4L2_PIX_FMT_SGRBG8) {
			pModule->image.cdspin_fmt = C_FRONT_FMT_RAW8;
			pModule->image.raw_flag = 1;
			pModule->image.raw_format = 0;
		} else if(pModule->image.v4l2in_fmt == V4L2_PIX_FMT_SGRBG10) {
			pModule->image.cdspin_fmt = C_FRONT_FMT_RAW10;
			pModule->image.raw_flag = 1;
			pModule->image.raw_format = 0;
		} else {
			RETURN(-EINVAL);
		}
		break;

	case C_CDSP_MIPI:
		pModule->image.sensor_flag = 1;
		if(pModule->image.v4l2in_fmt == V4L2_PIX_FMT_YVYU) {
			pModule->image.cdspin_fmt = C_MIPI_FMT_Y1VY0U;
			pModule->image.raw_flag = 0;
			pModule->image.sensor_flag = 1;
		} else if(pModule->image.v4l2in_fmt == V4L2_PIX_FMT_SBGGR8) {
			pModule->image.cdspin_fmt = C_MIPI_FMT_RAW8;
			pModule->image.raw_flag = 1;
			pModule->image.raw_format = 2;
		} else if(pModule->image.v4l2in_fmt == V4L2_PIX_FMT_SGBRG8) {
			pModule->image.cdspin_fmt = C_MIPI_FMT_RAW8;
			pModule->image.raw_flag = 1;
			pModule->image.raw_format = 3;
		} else if(pModule->image.v4l2in_fmt == V4L2_PIX_FMT_SGRBG8) {
			pModule->image.cdspin_fmt = C_MIPI_FMT_RAW8;
			pModule->image.raw_flag = 1;
			pModule->image.raw_format = 0;
		} else if(pModule->image.v4l2in_fmt == V4L2_PIX_FMT_SGRBG10) {
			pModule->image.cdspin_fmt = C_MIPI_FMT_RAW10;
			pModule->image.raw_flag = 1;
			pModule->image.raw_format = 0;
		} else {
			RETURN(-EINVAL);
		}
		break;
	default:
		RETURN(-EINVAL);
	}

__return:
	return nRet;
}

static int
gp_cdsp_g_ctrl(
	struct v4l2_control *ctrl,
	gpCdspModule_t *argp
)
{
	int nRet = 0;
	struct v4l2_control ctrl_csi;

	switch(ctrl->id)
	{
		case MSG_CDSP_SCALE_CROP:
		{
			gp_cdsp_get_scale_crop(&argp->scale);
			nRet = copy_to_user((void __user*)ctrl->value, (void*)&argp->scale, sizeof(gpCdspScalePara_t));
			break;
		}

		case MSG_CDSP_BADPIX_OB:
		{
			gpCdspBadPixOB_t bad_pixel;

			gp_cdsp_get_badpixob(&bad_pixel);
			nRet = copy_to_user((void __user*)ctrl->value, (void*)&bad_pixel, sizeof(gpCdspBadPixOB_t));
			break;
		}

		case MSG_CDSP_LENS_CMP:
		{
			gpCdspLenCmp_t lens_cmp;

			gp_cdsp_get_lens_cmp(&lens_cmp);
			nRet = copy_to_user((void __user*)ctrl->value, (void*)&lens_cmp, sizeof(gpCdspLenCmp_t));
			break;
		}

		case MSG_CDSP_WBGAIN:
		{
			gpCdspWhtBal_t wht_bal;
			gp_cdsp_get_white_balance(&wht_bal);
			nRet = copy_to_user((void __user*)ctrl->value, (void*)&wht_bal, sizeof(gpCdspWhtBal_t));
			break;
		}

		case MSG_CDSP_LUT_GAMMA:
		{
			gpCdspGamma_t lut_gamma;

			gp_cdsp_get_lut_gamma(&lut_gamma);
			nRet = copy_to_user((void __user*)ctrl->value, (void*)&lut_gamma, sizeof(gpCdspGamma_t));
			break;
		}

		case MSG_CDSP_INTERPOLATION:
		{
			gpCdspIntpl_t intpl;

			gp_cdsp_get_intpl(&intpl);
			nRet = copy_to_user((void __user*)ctrl->value, (void*)&intpl, sizeof(gpCdspIntpl_t));
			break;
		}

		case MSG_CDSP_EDGE:
		{
			gpCdspEdge_t edge;
			gp_cdsp_get_edge(&edge);
			nRet = copy_to_user((void __user*)ctrl->value, (void*)&edge, sizeof(gpCdspEdge_t));
			break;
		}

		case MSG_CDSP_COLOR_MATRIX:
		{
			gpCdspCorMatrix_t matrix;

			gp_cdsp_get_color_matrix(&matrix);
			nRet = copy_to_user((void __user*)ctrl->value, (void*)&matrix, sizeof(gpCdspCorMatrix_t));
			break;
		}

		case MSG_CDSP_POSWB_RGB2YUV:
		{
			gpCdspRgb2Yuv_t rgb2yuv;

			gp_cdsp_get_rgbtoyuv(&rgb2yuv);
			nRet = copy_to_user((void __user*)ctrl->value, (void*)&rgb2yuv, sizeof(gpCdspRgb2Yuv_t));
			break;
		}

		case MSG_CDSP_YUV_INSERT:
		{
			gpCdspYuvInsert_t yuv_insert;

			gp_cdsp_get_yuv444_insert(&yuv_insert);
			nRet = copy_to_user((void __user*)ctrl->value, (void*)&yuv_insert, sizeof(gpCdspYuvInsert_t));
			break;
		}

		case MSG_CDSP_YUV_HAVG:
		{
			gpCdspYuvHAvg_t yuv_havg;

			gp_cdsp_get_yuv_havg(&yuv_havg);
			nRet = copy_to_user((void __user*)ctrl->value, (void*)&yuv_havg, sizeof(gpCdspYuvHAvg_t));
			break;
		}

		case MSG_CDSP_SPEC_MODE:
		{
			gpCdspSpecMod_t spec_mode;

			gp_cdsp_get_special_mode(&spec_mode);
			nRet = copy_to_user((void __user*)ctrl->value, (void*)&spec_mode, sizeof(gpCdspSpecMod_t));
			break;
		}

		case MSG_CDSP_SAT_HUE:
		{
			gpCdspSatHue_t sat_hue;

			gp_cdsp_get_sat_hue(&sat_hue);
			nRet = copy_to_user((void __user*)ctrl->value, (void*)&sat_hue, sizeof(gpCdspSatHue_t));
			break;
		}

		case MSG_CDSP_SUPPRESSION:
		{
			gpCdspSuppression_t suppression;

			gp_cdsp_get_suppression(&suppression);
			nRet = copy_to_user((void __user*)ctrl->value, (void*)&suppression, sizeof(gpCdspSuppression_t));
			break;
		}

		case MSG_CDSP_RAW_WIN:
		{
			gpCdspRawWin_t raw_win;

			gp_cdsp_get_raw_win(&raw_win);
			nRet = copy_to_user((void __user*)ctrl->value, (void*)&raw_win, sizeof(gpCdspRawWin_t));
			break;
		}

		case MSG_CDSP_AE_WIN:
		{
			gpCdspAE_t ae;

			gp_cdsp_get_ae(&ae);
			nRet = copy_to_user((void __user*)ctrl->value, (void*)&ae, sizeof(gpCdspAE_t));
			break;
		}

		case MSG_CDSP_AF_WIN:
		{
			gpCdspAF_t af;

			gp_cdsp_get_af(&af);
			nRet = copy_to_user((void __user*)ctrl->value, (void*)&af, sizeof(gpCdspAF_t));
			break;
		}

		case MSG_CDSP_AWB_WIN:
		{
			gpCdspAWB_t awb;

			gp_cdsp_get_awb(&awb);
			nRet = copy_to_user((void __user*)ctrl->value, (void*)&awb, sizeof(gpCdspAWB_t));
			break;
		}

		case MSG_CDSP_WBGAIN2:
		{
			gpCdspWbGain2_t wbgain2;

			gp_cdsp_get_wbgain2(&wbgain2);
			nRet = copy_to_user((void __user*)ctrl->value, (void*)&wbgain2, sizeof(gpCdspWbGain2_t));
			break;
		}

		case MSG_CDSP_HISTGM:
		{
			gpCdspHistgm_t histgm;

			gp_cdsp_get_histgm(&histgm);
			histgm.hislowcnt = argp->hislowcnt;
			histgm.hishicnt= argp->hishicnt;
			nRet = copy_to_user((void __user*)ctrl->value, (void*)&histgm, sizeof(gpCdspHistgm_t));
			break;
		}

		case MSG_CDSP_3A_STATISTIC:
		{
			nRet = copy_to_user((void __user*)ctrl->value, (void*)&argp->a3_result, sizeof(gpCdsp3aResult_t));
			break;
		}

		case MSG_CDSP_SENSOR:
		{
			nRet = copy_from_user((void*)&ctrl_csi, (void __user*)ctrl->value, sizeof(struct v4l2_control));
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
			
			nRet = p_cdsp_dev->sd->ops->core->g_ctrl(p_cdsp_dev->sd, &ctrl_csi);
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
			
			nRet = copy_to_user((void __user*)ctrl->value, (void*)&ctrl_csi, sizeof(struct v4l2_control));
			break;
		}

		default:
			RETURN(-EINVAL);
			break;
	}

__return:
	return nRet;
}

static int
gp_cdsp_s_ctrl(
	struct v4l2_control *ctrl,
	gpCdspModule_t *argp
)
{
	unsigned short w, h;
	int nRet = 0;

	switch(ctrl->id)
	{
		case MSG_CDSP_SCALE_CROP:
		{
			nRet = copy_from_user((void*)&argp->scale, (void __user*)ctrl->value, sizeof(gpCdspScalePara_t));
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
			
			gp_cdsp_set_scale_crop(&argp->image, &argp->scale);
			gp_cdsp_set_scale_ae_af(argp);
			break;
		}

		case MSG_CDSP_BADPIX_OB:
		{
			gpCdspBadPixOB_t bad_pixel;

			nRet = copy_from_user((void*)&bad_pixel, (void __user*)ctrl->value, sizeof(gpCdspBadPixOB_t));
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
			
			if(argp->image.raw_flag) {
				gpCdspWhtBal_t wht_bal;
				
				gp_cdsp_get_white_balance(&wht_bal);
				gp_cdsp_set_badpixob(&bad_pixel, &wht_bal);
			}
			break;
		}

		case MSG_CDSP_LENS_CMP:
		{
			gpCdspLenCmp_t lens_cmp;

			nRet = copy_from_user((void*)&lens_cmp, (void __user*)ctrl->value, sizeof(gpCdspLenCmp_t));
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
			
			gp_cdsp_set_lens_cmp(argp->image.raw_flag, &lens_cmp);
			break;
		}

		case MSG_CDSP_WBGAIN:
		{
			gpCdspWhtBal_t wht_bal;

			nRet = copy_from_user((void*)&wht_bal, (void __user*)ctrl->value, sizeof(gpCdspWhtBal_t));
			if(nRet < 0) {
				RETURN(-EINVAL);
			}

			if(argp->image.raw_flag) {
				gp_cdsp_set_white_balance(&wht_bal);
			}
			break;
		}

		case MSG_CDSP_LUT_GAMMA:
		{
			gpCdspGamma_t lut_gamma;

			nRet = copy_from_user((void*)&lut_gamma, (void __user*)ctrl->value, sizeof(gpCdspGamma_t));
			if(nRet < 0) {
				RETURN(-EINVAL);
			}

			if(argp->image.raw_flag) {
				gp_cdsp_set_lut_gamma(&lut_gamma);
			}
			break;
		}

		case MSG_CDSP_INTERPOLATION:
		{
			gpCdspIntpl_t intpl;

			nRet = copy_from_user((void*)&intpl, (void __user*)ctrl->value, sizeof(gpCdspIntpl_t));
			if(nRet < 0) {
				RETURN(-EINVAL);
			}

			if(argp->image.raw_flag) {
				gp_cdsp_set_intpl(argp->image.img_h_size, &intpl);
			}
			break;
		}

		case MSG_CDSP_EDGE:
		{
			gpCdspEdge_t edge;

			nRet = copy_from_user((void*)&edge, (void __user*)ctrl->value, sizeof(gpCdspEdge_t));
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
			
			gp_cdsp_set_edge(argp->image.raw_flag, &edge);
		}
		break;

		case MSG_CDSP_COLOR_MATRIX:
		{
			gpCdspCorMatrix_t matrix;
			
			nRet = copy_from_user((void*)&matrix, (void __user*)ctrl->value, sizeof(gpCdspCorMatrix_t));
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
			
			if(argp->image.raw_flag) {
				gp_cdsp_set_color_matrix(&matrix);
			}
		}
		break;

		case MSG_CDSP_POSWB_RGB2YUV:
		{
			gpCdspRgb2Yuv_t rgb2yuv;
			
			nRet = copy_from_user((void*)&rgb2yuv, (void __user*)ctrl->value, sizeof(gpCdspRgb2Yuv_t));
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
			
			if(argp->image.raw_flag) {
				gp_cdsp_set_rgbtoyuv(&rgb2yuv);
			}
			break;
		}

		case MSG_CDSP_YUV_INSERT:
		{
			gpCdspYuvInsert_t yuv_insert;

			nRet = copy_from_user((void*)&yuv_insert, (void __user*)ctrl->value, sizeof(gpCdspYuvInsert_t));
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
			
			gp_cdsp_set_yuv444_insert(&yuv_insert);
			break;
		}

		case MSG_CDSP_YUV_HAVG:
		{
			gpCdspYuvHAvg_t yuv_havg;

			nRet = copy_from_user((void*)&yuv_havg, (void __user*)ctrl->value, sizeof(gpCdspYuvHAvg_t));
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
			
			gp_cdsp_set_yuv_havg(&yuv_havg);
			break;
		}

		case MSG_CDSP_SPEC_MODE:
		{
			gpCdspSpecMod_t spec_mode;

			nRet = copy_from_user((void*)&spec_mode, (void __user*)ctrl->value, sizeof(gpCdspSpecMod_t));
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
			
			gp_cdsp_set_special_mode(&spec_mode);
			break;
		}

		case MSG_CDSP_SAT_HUE:
		{
			gpCdspSpecMod_t spec_mode;
			gpCdspSatHue_t sat_hue;

			nRet = copy_from_user((void*)&sat_hue, (void __user*)ctrl->value, sizeof(gpCdspSatHue_t));
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
			
			spec_mode.yuvspecmode = SP_YUV_YbYcSatHue;
			gp_cdsp_set_special_mode(&spec_mode);
			gp_cdsp_set_sat_hue(&sat_hue);
			break;
		}

		case MSG_CDSP_SUPPRESSION:
		{
			gpCdspSuppression_t suppression;
			gpCdspEdge_t edge;

			nRet = copy_from_user((void*)&suppression, (void __user*)ctrl->value, sizeof(gpCdspSuppression_t));
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
			
			if(argp->image.raw_flag == 0) {
				argp->suppr_mode = suppression.suppr_mode;
				gp_cdsp_get_edge(&edge);
				gp_cdsp_set_suppression(argp->image.img_h_size, &suppression, &edge);
			}
			break;
		}

		case MSG_CDSP_RAW_WIN:
		{
			gpCdspRawWin_t raw_win;

			nRet = copy_from_user((void*)&raw_win, (void __user*)ctrl->value, sizeof(gpCdspRawWin_t));
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
			
			if(argp->scale.crop_en) {
				w = argp->scale.crop_hsize;
				h = argp->scale.crop_vsize;
			} else {
				w = argp->image.img_h_size;
				h = argp->image.img_v_size;
			}
			
			gp_cdsp_set_raw_win(w, h, &raw_win);
			break;
		}

		case MSG_CDSP_AE_WIN:
		{
			gpCdspAE_t ae;
			gpCdspRawWin_t raw_win;

			nRet = copy_from_user((void*)&ae, (void __user*)ctrl->value, sizeof(gpCdspAE_t));
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
			
			gp_cdsp_get_raw_win(&raw_win);
			gp_cdsp_set_ae(raw_win.hwdsize, raw_win.vwdsize, &ae);
			break;
		}

		case MSG_CDSP_AF_WIN:
		{
			gpCdspAF_t af;

			nRet = copy_from_user((void*)&af, (void __user*)ctrl->value, sizeof(gpCdspAF_t));
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
			
			if(argp->scale.crop_en) {
				w = argp->scale.crop_hsize;
				h = argp->scale.crop_vsize;
			} else {
				w = argp->image.img_h_size;
				h = argp->image.img_v_size;
			}
			
			gp_cdsp_set_af(w, h, &af);
			break;
		}

		case MSG_CDSP_AWB_WIN:
		{
			gpCdspAWB_t awb;

			nRet = copy_from_user((void*)&awb, (void __user*)ctrl->value, sizeof(gpCdspAWB_t));
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
			
			gp_cdsp_set_awb(&awb);
			break;
		}

		case MSG_CDSP_WBGAIN2:
		{
			gpCdspWbGain2_t wbgain2;

			nRet = copy_from_user((void*)&wbgain2, (void __user*)ctrl->value, sizeof(gpCdspWbGain2_t));
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
			
			gp_cdsp_set_wbgain2(&wbgain2);
			break;
		}

		case MSG_CDSP_HISTGM:
		{
			gpCdspHistgm_t histgm;

			nRet = copy_from_user((void*)&histgm, (void __user*)ctrl->value, sizeof(gpCdspHistgm_t));
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
			
			gp_cdsp_set_histgm(&histgm);
			break;
		}

		case MSG_CDSP_SENSOR:
		{
			struct v4l2_control ctrl_csi;

			nRet = copy_from_user((void*)&ctrl_csi, (void __user*)ctrl->value, sizeof(struct v4l2_control));
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
			
			nRet = p_cdsp_dev->sd->ops->core->s_ctrl(p_cdsp_dev->sd, &ctrl_csi);
			break;
		}

		default:
			RETURN(-EINVAL);
			break;
	}

__return:
	return nRet;
}

static void
gp_cdsp_af_isr(
	gpCdsp3aResult_t *argp
)
{
	unsigned int temphl, temphh, tempvl, tempvh;

	gpHalCdspGetAFWinVlaue(1, &temphl, &temphh, &tempvl, &tempvh);
	argp->af1_h_value = temphh;
	argp->af1_h_value <<= 16;
	argp->af1_h_value |= temphl;
	argp->af1_v_value = tempvh;
	argp->af1_v_value <<= 16;
	argp->af1_v_value |= tempvl;

	gpHalCdspGetAFWinVlaue(2, &temphl, &temphh, &tempvl, &tempvh);
	argp->af2_h_value = temphh;
	argp->af2_h_value <<= 16;
	argp->af2_h_value |= temphl;
	argp->af2_v_value = tempvh;
	argp->af2_v_value <<= 16;
	argp->af2_v_value |= tempvl;

	gpHalCdspGetAFWinVlaue(3, &temphl, &temphh, &tempvl, &tempvh);
	argp->af3_h_value = temphh;
	argp->af3_h_value <<= 16;
	argp->af3_h_value |= temphl;
	argp->af3_v_value = tempvh;
	argp->af3_v_value <<= 16;
	argp->af3_v_value |= tempvl;
}

static void
gp_cdsp_awb_isr(
	gpCdsp3aResult_t *argp
)
{
	signed int tempsh, tempsl;
	unsigned int cnt, temph, templ;

	gpHalCdspGetAwbSumCnt(1, &cnt);
	argp->sumcnt1 = cnt;
	gpHalCdspGetAwbSumCnt(2, &cnt);
	argp->sumcnt2 = cnt;
	gpHalCdspGetAwbSumCnt(3, &cnt);
	argp->sumcnt3 = cnt;

	gpHalCdspGetAwbSumG(1, &templ, &temph);
	argp->sumg1 = temph;
	argp->sumg1 <<= 16;
	argp->sumg1 |= templ;
	gpHalCdspGetAwbSumG(2, &templ, &temph);
	argp->sumg2 = temph;
	argp->sumg2 <<= 16;
	argp->sumg2 |= templ;
	gpHalCdspGetAwbSumG(3, &templ, &temph);
	argp->sumg3 = temph;
	argp->sumg3 <<= 16;
	argp->sumg3 |= templ;

	gpHalCdspGetAwbSumRG(1, &tempsl, &tempsh);
	argp->sumrg1 = tempsh;
	argp->sumrg1 <<= 16;
	argp->sumrg1 |= tempsl;
	gpHalCdspGetAwbSumRG(2, &tempsl, &tempsh);
	argp->sumrg2 = tempsh;
	argp->sumrg2 <<= 16;
	argp->sumrg2 |= tempsl;
	gpHalCdspGetAwbSumRG(3, &tempsl, &tempsh);
	argp->sumrg3 = tempsh;
	argp->sumrg3 <<= 16;
	argp->sumrg3 |= tempsl;

	gpHalCdspGetAwbSumBG(1, &tempsl, &tempsh);
	argp->sumbg1 = tempsh;
	argp->sumbg1 <<= 16;
	argp->sumbg1 |= tempsl;
	gpHalCdspGetAwbSumBG(2, &tempsl, &tempsh);
	argp->sumbg1 = tempsh;
	argp->sumbg1 <<= 16;
	argp->sumbg1 |= tempsl;
	gpHalCdspGetAwbSumBG(3, &tempsl, &tempsh);
	argp->sumbg1 = tempsh;
	argp->sumbg1 <<= 16;
	argp->sumbg1 |= tempsl;
}

static void
gp_cdsp_ae_isr(
	gpCdsp3aResult_t *argp
)
{
	if(gpHalCdspGetAEActBuff()) {
		memcpy(argp->ae_win, (void*)p_cdsp_dev->aewin_addr[0], 64);
	} else {
		memcpy(argp->ae_win, (void*)p_cdsp_dev->aewin_addr[1], 64);
	}
}

static void
gp_cdsp_ov_isr(
	void
)
{
	DERROR("ovisr\n");
}

static void
gp_cdsp_facwr_isr(
	void
)
{
	DERROR("facwrisr\n");
}

static void
gp_cdsp_eof_isr(
	gpCdspImg_t *argp
)
{
	unsigned char post_flag = 0;
	unsigned char i, index;
	unsigned short width, height;

	if(argp->img_src == C_CDSP_SDRAM) {
		post_flag = 1;
	} else {
		/* 1st. find empty frame buffer */
		for(i=argp->mapped_idx; i<argp->fb_total; i++) {
			if(argp->frame[i].flag == V4L2_BUF_FLAG_QUEUED) {
				index = argp->mapped_idx = i;
				goto __queue_find;
			}
		}
		
		for(i=0; i<argp->mapped_idx; i++) {
			if(argp->frame[i].flag == V4L2_BUF_FLAG_QUEUED) {
				index = argp->mapped_idx = i;
				goto __queue_find;
			}
		}
		
		/* not find queue */
		return;
		
__queue_find:
		/* 2nd. set buffer doing to done flag */
		for(i=0; i<argp->fb_total; i++) {
			if(argp->frame[i].flag == V4L2_BUF_FLAG_MAPPED) {
				argp->frame[i].flag = V4L2_BUF_FLAG_DONE;
				break;
			}
		}

		if(i >= argp->fb_total) {
			DERROR("MapFail\n");
			return;
		}

		/* 3st. check scale/crop flag */
		if(p_cdsp_dev->update_flag) {
			p_cdsp_dev->update_flag = 0;
			width = argp->img_rb_h_size;
			height = argp->img_rb_v_size;
		} else {
		#if C_DMA_A == 1
			gpHalCdspGetYuvBuffASize(&width, &height);
		#else
			gpHalCdspGetYuvBuffBSize(&width, &height);
		#endif
		}

		/* 4th. switch dma buffer */
		post_flag = 1;
		argp->frame[index].flag = V4L2_BUF_FLAG_MAPPED;
	#if C_DMA_A == 1
		gpHalCdspSetYuvBuffA(width, height, argp->frame[index].phy_addr);
		gpHalCdspSetDmaBuff(RD_A_WR_A);
	#else
		gpHalCdspSetYuvBuffB(width, height, argp->frame[index].phy_addr);
		gpHalCdspSetDmaBuff(RD_B_WR_B);
	#endif
	}

	if(post_flag) {
		if(p_cdsp_dev->cdsp_done == 0) {
			p_cdsp_dev->cdsp_done = 1;
			wake_up_interruptible(&p_cdsp_dev->cdsp_wait_queue);
		}
	}
}

static irqreturn_t
gp_cdsp_irq_handler(
	int irq,
	void *dev_id
)
{
	int glb_status;
	int status;
	gpCdspDev_t *devp;
	gpCdspModule_t *modp;

	devp = (gpCdspDev_t *)dev_id;
	modp = (gpCdspModule_t *)devp->cur_module;
	glb_status = gpHalCdspGetGlbIntStatus();
	if(glb_status & CDSP_INT_BIT) {
		status = gpHalCdspGetIntStatus();
		if(status & CDSP_OVERFOLW) {
			gp_cdsp_ov_isr();
			return IRQ_HANDLED;
		}
		
		if(status & CDSP_FACWR) {
			gp_cdsp_facwr_isr();
			return IRQ_HANDLED;
		}
		
		if(status & CDSP_EOF) {
			gp_cdsp_eof_isr(&modp->image);
			/* histgm */
			gpHalCdspGetHistgmCount(&modp->hislowcnt, &modp->hishicnt);
		}
		
		if(status & CDSP_AFWIN_UPDATE) {
			gp_cdsp_af_isr(&modp->a3_result);
		}
		
		if(status & CDSP_AWBWIN_UPDATE) {
			gp_cdsp_awb_isr(&modp->a3_result);
		}
		
		if(status & CDSP_AEWIN_SEND) {
			gp_cdsp_ae_isr(&modp->a3_result);
		}
	} else if(glb_status & FRONT_VD_INT_BIT) {
		status = gpHalCdspGetFrontVdIntStatus();
	} else if(glb_status & FRONT_INT_BIT) {
		status = gpHalCdspGetFrontIntStatus();
	} else {
		return IRQ_NONE;
	}
	return IRQ_HANDLED;
}

static unsigned int
gp_cdsp_poll(
	struct file *filp,
	struct poll_table_struct *poll
)
{
	unsigned int mask = 0;

#if 0
	wait_event_interruptible(p_cdsp_dev->cdsp_wait_queue, (p_cdsp_dev->cdsp_done != 0));
#else
	poll_wait(filp, &p_cdsp_dev->cdsp_wait_queue, poll);
#endif

	if(p_cdsp_dev->cdsp_done == 1) {
		p_cdsp_dev->cdsp_done = 0;
		mask = POLLIN | POLLRDNORM;
	}

	return mask;
}

static long
gp_cdsp_ioctl(
	struct file *filp,
	unsigned int cmd,
	unsigned long arg
)
{
	unsigned char div, idx;
	int i, nRet;
	gpCdspModule_t *modp;
	struct v4l2_fmtdesc fmtdesc;
	struct v4l2_format fmt;
	struct v4l2_requestbuffers req;
	struct v4l2_buffer buf;
	struct v4l2_streamparm stream;
	struct v4l2_input input;
	struct v4l2_control ctrl;
	struct v4l2_queryctrl qctrl;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct clk *clock;

	if(down_interruptible(&p_cdsp_dev->sem) != 0)
		return -ERESTARTSYS;

	nRet = 0;
	modp = (gpCdspModule_t *)filp->private_data;
 	if(modp == NULL) RETURN(-EINVAL);

	switch(cmd)
	{
	case VIDIOC_QUERYCAP:
		nRet = copy_to_user((void __user*)arg, (void*)&g_cdsp_cap, sizeof(struct v4l2_capability));
		if(nRet < 0) { 
			RETURN(-EINVAL);
		}
		break;

	case VIDIOC_ENUMINPUT:
		nRet = copy_from_user((void*)&input, (void __user*)arg, sizeof(struct v4l2_input));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}

		input.type = V4L2_INPUT_TYPE_CAMERA;
		if(input.index == 0) {
			strncpy(input.name, "FB2FB", sizeof(input.name));
		} else if(input.index > 0) {
			unsigned int index;
			struct v4l2_subdev *sd;
			callbackfunc_t *cb;
			char *port;

			index = input.index - 1;
			nRet = gp_get_sensorinfo(index, (int*)&sd, (int*)&cb, (int*)&port, (int*)&sensor);
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
			
			strncpy(input.name, sd->name, sizeof(input.name));
			strncat(input.name, "+", sizeof(input.name));
			strncat(input.name, port, sizeof(input.name));
		}
		
		DEBUG("Input[%d]:%s\n", input.index, input.name);
		nRet = copy_to_user((void __user*)arg, (void*)&input, sizeof(struct v4l2_input));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		break;

	case VIDIOC_S_INPUT:
		modp->image.sensor_flag = 0;
		if(arg == 0) {
			DEBUG("SetInput[0]:FB2FB\n");
			modp->image.img_src = C_CDSP_SDRAM;
		} else if(arg > 0) {
			unsigned int index;

			index = arg - 1;
			nRet = gp_get_sensorinfo(index, (int*)&p_cdsp_dev->sd, (int*)&p_cdsp_dev->cb_func, (int*)&p_cdsp_dev->port, (int*)&sensor);
			if(nRet < 0) { 
				RETURN(-EINVAL);
			}
			
			DEBUG("SetInput[%d]:%s+%s\n", input.index, p_cdsp_dev->sd->name, p_cdsp_dev->port);
			modp->Interface.Interface = sensor->sensor_timing_mode;
			modp->Interface.HsyncAct = sensor->sensor_hsync_mode;
			modp->Interface.VsyncAct = sensor->sensor_vsync_mode;
			modp->Interface.Interlace = sensor->sensor_interlace_mode;
			modp->Interface.SampleEdge = sensor->sensor_pclk_mode;
			modp->Interface.FmtOut = sensor->sensor_data_mode;
			p_cdsp_dev->input_index = input.index;

			if(strcmp("PORT0", p_cdsp_dev->port) == 0) {
				modp->image.img_src = C_CDSP_FRONT;
				modp->image.sensor_flag = 1;
			} else if(strcmp("MIPI", p_cdsp_dev->port) == 0) {
				modp->image.img_src = C_CDSP_MIPI;
				modp->image.sensor_flag = 1;
			} else {
				DEBUG("DrvPortError!\n");
				RETURN(-EINVAL);
			}

			if(p_cdsp_dev->cb_func->powerctl != NULL) {
				p_cdsp_dev->cb_func->powerctl(1);
			}
			
			if(p_cdsp_dev->cb_func->standby != NULL) {
				p_cdsp_dev->cb_func->standby(0); 
			}

			if(p_cdsp_dev->cb_func->set_port != NULL) {
				p_cdsp_dev->cb_func->set_port(p_cdsp_dev->port);
			}

			/* open mclk, some sensor need mclk befor init */
			if(sensor->fmt[0].mclk_src == CSI_CLK_SPLL) {	
				clock = clk_get(NULL, "clk_ref_ceva");
				nRet = clk_get_rate(clock); 
			} else {
				nRet = USBPHY_CLK;
			}
			
			div = nRet / sensor->fmt[0].mclk;
			if((nRet % sensor->fmt[0].mclk) == 0) { 
				div--;
			}
			gpHalCdspSetMclk(sensor->fmt[0].mclk_src, div, 0, 0);
			DEBUG("mclk = %d\n", nRet/(div + 1));

			if(p_cdsp_dev->sd) {
				nRet = p_cdsp_dev->sd->ops->core->reset(p_cdsp_dev->sd, 0);
				nRet = p_cdsp_dev->sd->ops->core->init(p_cdsp_dev->sd, 0);
			}
		}
		break;

	case VIDIOC_G_INPUT:
		nRet = copy_from_user((void*)&input, (void __user*)arg, sizeof(struct v4l2_input));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		
		input.type = V4L2_INPUT_TYPE_CAMERA;
		if(modp->image.img_src == C_CDSP_SDRAM) {
			input.index = 0;
			strncpy(input.name, "FB2FB, SDRAM", sizeof(input.name));
		} else {
			input.index = p_cdsp_dev->input_index;
			strncpy(input.name, p_cdsp_dev->sd->name, sizeof(input.name));
		}
		
		nRet = copy_to_user((void __user*)arg, (void*)&input, sizeof(struct v4l2_input));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		break;

	case VIDIOC_S_FMT:
		nRet = copy_from_user((void*)&fmt, (void __user*)arg, sizeof(struct v4l2_format));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}

	#if 0
		for(i=0; i<sensor->sensor_fmt_num; i++) {
			if((fmt.fmt.pix.width == sensor->fmt[i].hpixel) && 
				(fmt.fmt.pix.height == sensor->fmt[i].vline)) {
				idx = i;
				break;
			}
		}

		if(sensor->sensor_fmt_num == i) {
			RETURN(-EINVAL);
		}
	#else
		if(fmt.fmt.pix.priv >= sensor->sensor_fmt_num) {
			idx = 0;
		} else {
			idx = fmt.fmt.pix.priv;
		}
	#endif

		modp->image.v4l2in_fmt = fmt.fmt.pix.pixelformat;
		modp->image.rgb_path = 0;
		if(modp->image.sensor_flag == 0) {
			fmt.fmt.pix.bytesperline = fmt.fmt.pix.width << 1;
			fmt.fmt.pix.sizeimage = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;

			modp->image.img_h_size = fmt.fmt.pix.width;
			modp->image.img_v_size = fmt.fmt.pix.height;
			modp->image.img_h_offset = 0;
			modp->image.img_v_offset = 0;
			modp->image.img_rb_h_size = fmt.fmt.pix.width;
			modp->image.img_rb_v_size = fmt.fmt.pix.height;
			modp->scale.img_rb_h_size = fmt.fmt.pix.width;
			modp->scale.img_rb_v_size = fmt.fmt.pix.height;
		} else {
			fmt.fmt.pix.bytesperline = sensor->fmt[idx].hpixel << 1;
			fmt.fmt.pix.sizeimage = fmt.fmt.pix.bytesperline * sensor->fmt[idx].vline;
			modp->image.img_h_size = sensor->fmt[idx].hpixel;
			modp->image.img_v_size = sensor->fmt[idx].vline;
			modp->image.img_h_offset = sensor->fmt[idx].hoffset;
			modp->image.img_v_offset = sensor->fmt[idx].voffset;
			modp->image.img_rb_h_size = sensor->fmt[idx].hpixel;
			modp->image.img_rb_v_size = sensor->fmt[idx].vline;
			modp->scale.img_rb_h_size = sensor->fmt[idx].hpixel;
			modp->scale.img_rb_v_size = sensor->fmt[idx].vline;
		}
		
		nRet = gp_cdsp_s_fmt(modp);
		if(nRet < 0) {
			RETURN(nRet);
		}
		
		if(modp->image.sensor_flag) {
			nRet = p_cdsp_dev->sd->ops->video->s_fmt(p_cdsp_dev->sd, &fmt);
			if(nRet < 0) {
				DERROR(KERN_WARNING "SetSensorFmtFail\n");
				RETURN(-EINVAL);
			}

			/* set csi output clock again */
			if(sensor->fmt[idx].mclk_src == CSI_CLK_SPLL) {	
				clock = clk_get(NULL, "clk_ref_ceva");
				nRet = clk_get_rate(clock);
			} else {
				nRet = USBPHY_CLK;
			}

			div = nRet/sensor->fmt[idx].mclk;
			if((nRet % sensor->fmt[idx].mclk) == 0) { 
				div--;
			}
			
			gpHalCdspSetMclk(sensor->fmt[idx].mclk_src, div, 0, 0);
			DEBUG("mclk = %d\n", nRet/(div + 1));
		}
		
		nRet = copy_to_user((void __user*)arg, &fmt, sizeof(struct v4l2_format));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		break;

	case VIDIOC_G_FMT:
		nRet = copy_from_user((void*)&fmt, (void __user*)arg, sizeof(struct v4l2_format));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}

		if(modp->image.sensor_flag) {
			nRet = p_cdsp_dev->sd->ops->video->g_fmt(p_cdsp_dev->sd, &fmt);
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
		} else {
			fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			fmt.fmt.pix.width = modp->image.img_h_size;
			fmt.fmt.pix.height = modp->image.img_v_size;
			fmt.fmt.pix.pixelformat = modp->image.v4l2in_fmt;
			fmt.fmt.pix.field = V4L2_FIELD_NONE;
			if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_SBGGR8 ||
				fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_SGBRG8 ||
				fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_SGRBG8) {
				fmt.fmt.pix.bytesperline = fmt.fmt.pix.width;
			} else {
				fmt.fmt.pix.bytesperline = fmt.fmt.pix.width << 1;
			}
			fmt.fmt.pix.sizeimage = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
		}

		nRet = copy_to_user((void __user*)arg, (void*)&fmt, sizeof(struct v4l2_format));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		break;

	case VIDIOC_TRY_FMT:
		nRet = copy_from_user((void*)&fmt, (void __user*)arg, sizeof(struct v4l2_format));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		
		if(modp->image.sensor_flag) {
			nRet = p_cdsp_dev->sd->ops->video->try_fmt(p_cdsp_dev->sd, &fmt);
			if(nRet < 0)
				RETURN(-EINVAL);
		} else {
			if(fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_VYUY &&
				fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_SBGGR8 &&
				fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_SGBRG8 &&
				fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_SGRBG8 &&
				fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_SGRBG10) {
				RETURN(-EINVAL);
			}
		}
		break;

	case VIDIOC_ENUM_FMT:
		nRet = copy_from_user((void*)&fmtdesc, (void __user*)arg, sizeof(struct v4l2_fmtdesc));
		if(nRet < 0) { 
			RETURN(-EINVAL);
		}
		
		if(modp->image.sensor_flag) {
			nRet = p_cdsp_dev->sd->ops->video->enum_fmt(p_cdsp_dev->sd, &fmtdesc);
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
		} else {
			if(fmtdesc.index == 0) {
				memcpy((void *)fmtdesc.description, "VYUY", 10);
				fmtdesc.pixelformat = V4L2_PIX_FMT_VYUY;
			} else if(fmtdesc.index == 1) {
				memcpy((void *)fmtdesc.description, "SBGGR8", 10);
				fmtdesc.pixelformat = V4L2_PIX_FMT_SBGGR8;
			} else if(fmtdesc.index == 2) {
				memcpy((void *)fmtdesc.description, "SGBRG8", 10);
				fmtdesc.pixelformat = V4L2_PIX_FMT_SGBRG8;
			} else if(fmtdesc.index == 3) {
				memcpy((void *)fmtdesc.description, "SGRBG8", 10);
				fmtdesc.pixelformat = V4L2_PIX_FMT_SGRBG8;
			} else if(fmtdesc.index == 4) {
				memcpy((void *)fmtdesc.description, "SGRBG10", 10);
				fmtdesc.pixelformat = V4L2_PIX_FMT_SGRBG10;
			}
		}

		nRet = copy_to_user((void __user*)arg, (void*)&fmtdesc, sizeof(struct v4l2_fmtdesc));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		break;

	case VIDIOC_QUERYCTRL:
		if(modp->image.sensor_flag) {
			nRet = p_cdsp_dev->sd->ops->core->queryctrl(p_cdsp_dev->sd, &qctrl);
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
		} else {
			DEBUG("PleaseCheckDriver\n");
		}
		
		nRet = copy_to_user((void __user*)arg, (void*)&qctrl, sizeof(struct v4l2_queryctrl));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		break;

	case VIDIOC_G_CTRL:
		nRet = copy_from_user((void*)&ctrl, (void __user*)arg, sizeof(struct v4l2_control));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		
		nRet = gp_cdsp_g_ctrl(&ctrl, modp);
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		break;

	case VIDIOC_S_CTRL:
		nRet = copy_from_user((void*)&ctrl, (void __user*)arg, sizeof(struct v4l2_control));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		
		nRet = gp_cdsp_s_ctrl(&ctrl, modp);
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		break;

	case VIDIOC_G_PARM:
		nRet = copy_from_user((void*)&stream, (void __user*)arg, sizeof(struct v4l2_streamparm));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		break;

	case VIDIOC_S_PARM:
		nRet = copy_from_user((void*)&stream, (void __user*)arg, sizeof(struct v4l2_streamparm));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		break;

	case VIDIOC_S_INTERFACE:
		nRet = copy_from_user((void*)&modp->Interface, (void __user*)arg, sizeof(struct v4l2_interface));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		
		if(modp->Interface.FmtOut != YUVOUT) {
			DERROR("Only Support YUVOUT\n");
			RETURN(-EINVAL);
		}
		
		if(p_cdsp_dev->sd) {
			nRet = p_cdsp_dev->sd->ops->ext->s_interface(p_cdsp_dev->sd, &modp->Interface);
		}
		break;

	case VIDIOC_G_INTERFACE:
		nRet = copy_to_user((void __user*)arg, (void*)&modp->Interface, sizeof(struct v4l2_interface));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		break;

	case VIDIOC_S_MCLK:
		nRet = copy_from_user((void*)&modp->mclk, (void __user*)arg, sizeof(gpCsiMclk_t));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		
		if(modp->mclk.mclk_out == 0) {
			modp->mclk.mclk_sel = div = 0;
			modp->mclk.pclk_dly = 0;
			modp->mclk.pclk_revb = 0;
			DEBUG("mclk = 0\n");
		} else {
			if(modp->mclk.mclk_sel == CSI_CLK_SPLL) {
				clock = clk_get(NULL, "clk_ref_ceva");
				nRet = clk_get_rate(clock);
			} else {
				nRet = USBPHY_CLK;
			}
			div = nRet / modp->mclk.mclk_out;
			if((nRet % modp->mclk.mclk_out) == 0) {
				div--;
			}
			DEBUG("mclk = %d\n", nRet/(div + 1));
		}
		gpHalCdspSetMclk(modp->mclk.mclk_sel, div, modp->mclk.pclk_dly, modp->mclk.pclk_revb);
		break;

	case VIDIOC_G_MCLK:
		gpHalCdspGetMclk(&modp->mclk.mclk_sel, &div, &modp->mclk.pclk_dly, &modp->mclk.pclk_revb);
		if(modp->mclk.mclk_sel == CSI_CLK_SPLL) {
			clock = clk_get(NULL, "clk_ref_ceva");
			nRet = clk_get_rate(clock);
		} else {
			nRet = USBPHY_CLK;
		}

		modp->mclk.mclk_out = nRet/(div + 1);
		nRet = copy_to_user((void __user*)arg, (void*)&modp->mclk, sizeof(gpCsiMclk_t));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		break;

	case VIDIOC_QUERYBUF:
		nRet = copy_from_user((void*)&buf, (void __user*)arg, sizeof(struct v4l2_buffer));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		
		if((buf.index >= C_BUFFER_MAX) ||
			(buf.type != V4L2_BUF_TYPE_VIDEO_CAPTURE) ||
			(buf.memory != V4L2_MEMORY_USERPTR)) {
			RETURN(-EINVAL);
		}

		buf.m.userptr = modp->image.frame[buf.index].addr;
		buf.length = modp->image.frame[buf.index].length;
		nRet = copy_to_user((void __user*)arg, (void*)&buf, sizeof(struct v4l2_buffer));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		break;

	case VIDIOC_REQBUFS:
		nRet = copy_from_user((void*)&req, (void __user*)arg, sizeof(struct v4l2_requestbuffers));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		
		if((req.count < 2) ||
			(req.count >= C_BUFFER_MAX) ||
			(req.type != V4L2_BUF_TYPE_VIDEO_CAPTURE) ||
			(req.memory != V4L2_MEMORY_USERPTR)){
			RETURN(-EINVAL);
		}

		modp->image.mapped_idx = 0;
		modp->image.post_idx = 0;
		modp->image.ready_idx = 0;
		modp->image.fb_total = req.count;
		for(i=0; i<req.count; i++) {
			modp->image.frame[i].index = 0xFF;
			modp->image.frame[i].flag = V4L2_BUF_FLAG_INPUT;
			modp->image.frame[i].addr = 0;
			modp->image.frame[i].phy_addr = 0;
			modp->image.frame[i].length = 0;
		}
		
		nRet = copy_to_user((void __user*)arg, (void*)&req, sizeof(struct v4l2_requestbuffers));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		break;

	case VIDIOC_STREAMON:	
		switch(modp->image.img_src)
		{
		case C_CDSP_SDRAM:
			if(modp->image.raw_flag) {
				gpHalCdspSetClk(C_CDSP_CLK_FB, 0);
			} else {
				gpHalCdspSetClk(C_CDSP_CLK_FB, 1);
			}
			break;

		case C_CDSP_FRONT:
			if(modp->image.raw_flag) {
				gpHalCdspSetClk(C_CDSP_CLK_FRONT, 0);
			} else {
				gpHalCdspSetClk(C_CDSP_CLK_FRONT, 1);
			}
			break;
			
		case C_CDSP_MIPI:
			if(modp->image.raw_flag) {
				gpHalCdspSetClk(C_CDSP_CLK_MIPI, 0);
			} else {
				gpHalCdspSetClk(C_CDSP_CLK_MIPI, 1);
			}
			break;
		}

		nRet = gp_cdsp_start(modp);
		if(nRet < 0) {
			RETURN(-EINVAL)
		}
		
		p_cdsp_dev->start_flag = 1;
		p_cdsp_dev->cur_module = modp;		
		break;

	case VIDIOC_STREAMOFF:
		nRet = gp_cdsp_stop();
		p_cdsp_dev->cdsp_done = 0;
		p_cdsp_dev->start_flag = 0;
		p_cdsp_dev->cur_module = NULL;
		if(modp->image.sensor_flag) {
			gpHalCdspSetMclk(0, 0, 0, 0);
			p_cdsp_dev->sd->ops->ext->suspend(p_cdsp_dev->sd);
			if(p_cdsp_dev->cb_func->standby)
				p_cdsp_dev->cb_func->standby(1);

			if(p_cdsp_dev->cb_func->powerctl)
				p_cdsp_dev->cb_func->powerctl(0);
		}

		/* invalid cache */
		if(modp->image.img_src == C_CDSP_SDRAM) {
		#ifndef GP_SYNC_OPTION
			/* invalid cache */
			nRet = modp->image.img_rb_h_size * modp->image.img_rb_v_size * 2;
			gp_invalidate_dcache_range(modp->image.frame[1].addr, nRet);
		#else
			GP_SYNC_CACHE();
		#endif
		}
		break;

	case VIDIOC_QBUF:
		nRet = copy_from_user((void*)&buf, (void __user*)arg, sizeof(struct v4l2_buffer));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		
		if((buf.type != V4L2_BUF_TYPE_VIDEO_CAPTURE) || (buf.memory != V4L2_MEMORY_USERPTR)) {
			DERROR("QBUFTypeErr\n");
			RETURN(-EINVAL);
		}
		
		if(buf.index >= modp->image.fb_total) {
			DERROR("QBUFIndexErr\n");
			RETURN(-EINVAL);
		}

		/* find input buffer */
		for(i=modp->image.post_idx; i<modp->image.fb_total; i++) {
			if(modp->image.frame[i].flag == V4L2_BUF_FLAG_INPUT) {
				buf.index = modp->image.post_idx = i;
				goto __qbuf_find;
			}
		}
		
		for(i=0; i<modp->image.post_idx; i++) {
			if(modp->image.frame[i].flag == V4L2_BUF_FLAG_INPUT) {
				buf.index = modp->image.post_idx = i;
				goto __qbuf_find;
			}
		}
		DERROR("QBFFindErr\n");
		RETURN(-EINVAL);

__qbuf_find:
		//DEBUG("Q%d\n", buf.index);
		modp->image.frame[buf.index].index = buf.index;
		modp->image.frame[buf.index].addr = buf.m.userptr;
		modp->image.frame[buf.index].phy_addr = gp_user_va_to_pa((void *)modp->image.frame[buf.index].addr);
		modp->image.frame[buf.index].length = buf.length;
		modp->image.frame[buf.index].flag = V4L2_BUF_FLAG_QUEUED;
		nRet = copy_to_user((void __user*)arg, (void*)&buf, sizeof(struct v4l2_buffer));
		if(nRet < 0) { 
			RETURN(-EINVAL);
		}
		break;

	case VIDIOC_DQBUF:
		nRet = copy_from_user((void *)&buf, (void __user*)arg, sizeof(struct v4l2_buffer));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		
		/* find done buffer */
		for(i=modp->image.ready_idx; i<modp->image.fb_total; i++) {
			if(modp->image.frame[i].flag == V4L2_BUF_FLAG_DONE) {
				modp->image.frame[i].flag = V4L2_BUF_FLAG_INPUT;
				modp->image.ready_idx = i;
				goto __dqbuf_find;
			}
		}
		
		for(i=0; i<modp->image.ready_idx; i++) {
			if(modp->image.frame[i].flag == V4L2_BUF_FLAG_DONE) {
				modp->image.frame[i].flag = V4L2_BUF_FLAG_INPUT;
				modp->image.ready_idx = i;
				goto __dqbuf_find;
			}
		}
		DERROR("DQBFindErr\n");
		RETURN(-EINVAL);

__dqbuf_find:
		//DEBUG("DQ%d,", i);
		buf.index = modp->image.frame[i].index;
		buf.m.userptr = modp->image.frame[i].addr;
		buf.length = modp->image.frame[i].length;

	#ifndef GP_SYNC_OPTION
		/* invalid cache */
		gp_invalidate_dcache_range(buf.m.userptr, buf.length);
	#else
		GP_SYNC_CACHE();
	#endif
		nRet = copy_to_user((void __user*)arg, (void*)&buf, sizeof(struct v4l2_buffer));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		break;

	case VIDIOC_CROPCAP:
		nRet = copy_from_user((void*)&cropcap, (void __user*)arg, sizeof(struct v4l2_cropcap));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		
		cropcap.bounds.left = 0;
		cropcap.bounds.top = 0;
		cropcap.bounds.width = modp->image.img_h_size;
		cropcap.bounds.height = modp->image.img_v_size;
		cropcap.defrect.left = modp->scale.crop_hoffset;
		cropcap.defrect.top = modp->scale.crop_voffset;
		cropcap.defrect.width = modp->scale.crop_hsize;
		cropcap.defrect.height = modp->scale.crop_hsize;
		nRet = copy_to_user((void __user*)arg, (void*)&cropcap, sizeof(struct v4l2_cropcap));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		break;

	case VIDIOC_G_CROP:
		nRet = copy_from_user((void*)&crop, (void __user*)arg, sizeof(struct v4l2_crop));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}

		if(modp->scale.crop_en) {
			crop.c.left = modp->scale.crop_hoffset;
			crop.c.top = modp->scale.crop_voffset;
			crop.c.width = modp->scale.crop_hsize;
			crop.c.height = modp->scale.crop_vsize;
		} else {
			crop.c.left = 0;
			crop.c.top = 0;
			crop.c.width = 0;
			crop.c.height = 0;
		}
		
		nRet = copy_to_user((void __user*)arg, (void*)&crop, sizeof(struct v4l2_crop));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		break;

	case VIDIOC_S_CROP:
		nRet = copy_from_user((void*)&crop, (void __user*)arg, sizeof(struct v4l2_crop));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		
		if((crop.c.left == 0) || (crop.c.top == 0)) {
			RETURN(-EINVAL);
		}
		
		modp->scale.crop_hoffset = crop.c.left;
		modp->scale.crop_voffset = crop.c.top;
		modp->scale.crop_hsize = crop.c.width;
		modp->scale.crop_vsize = crop.c.height;
		if(modp->scale.crop_hsize && modp->scale.crop_vsize) {
			modp->scale.crop_en = ENABLE;
		} else {
			modp->scale.crop_en = DISABLE;
		}
		gp_cdsp_set_scale_crop(&modp->image, &modp->scale);
		gp_cdsp_set_scale_ae_af(modp);
		break;

	default:
		RETURN(-ENOTTY);	/* Inappropriate ioctl for device */
	}

__return:
	up(&p_cdsp_dev->sem);
	return nRet;
}

static int
gp_cdsp_open(
	struct inode *inode,
	struct file *filp
)
{
	int nRet = 0;
	gpCdspModule_t *pModule;

	pModule = (gpCdspModule_t *)kzalloc(sizeof(gpCdspModule_t), GFP_KERNEL);
	if(pModule == NULL) {
		DERROR("MemFail\n");
		RETURN(-ENOMEM);
	}

	filp->private_data = (gpCdspModule_t *)pModule;
	if(p_cdsp_dev->open_cnt == 0) {
		gp_cdsp_clock_enable(1);
		gpHalCdspSetModuleReset(1);

		gpHalCdspReset();
		gpHalCdspFrontReset();
		gpHalCdspDataSource(C_CDSP_SDRAM);

		gpHalCdspSetIntEn(DISABLE, CDSP_INT_ALL);
		gpHalCdspClrIntStatus(CDSP_INT_ALL);
	}
	p_cdsp_dev->open_cnt++;

__return:
	if(nRet < 0) {
		DERROR(KERN_WARNING "CdspOpenFail!\n");
	} else {
		DEBUG(KERN_WARNING "CdspOpen.\n");
	}
	return nRet;
}

static int
gp_cdsp_release(
	struct inode *inode,
	struct file *filp
)
{
	gpCdspModule_t *pModule;

	pModule = (gpCdspModule_t *)filp->private_data;
 	if(pModule) {
 		kfree(pModule);
		filp->private_data = NULL;
 	}
	
	p_cdsp_dev->open_cnt--;
	if(p_cdsp_dev->open_cnt == 0) {
		gp_cdsp_stop();
		gpHalCdspSetMclk(0, 0, 0, 0);
		gp_cdsp_clock_enable(0);

		p_cdsp_dev->cdsp_done = 0;
		p_cdsp_dev->start_flag = 0;
		p_cdsp_dev->cur_module = NULL;
	}

	DEBUG(KERN_WARNING "CdspClose.\n");
	return 0;
}

struct file_operations cdsp_fops =
{
	.owner = THIS_MODULE,
	.poll = gp_cdsp_poll,
	.unlocked_ioctl = gp_cdsp_ioctl,
	.open = gp_cdsp_open,
	.release = gp_cdsp_release,
};

static void
gp_cdsp_device_release(
	struct device *dev
)
{
	DIAG_INFO("remove cdsp device ok\n");
}

static struct platform_device gp_cdsp_device = {
	.name = "gp-cdsp",
	.id	= 0,
	.dev =
	{
		.release = gp_cdsp_device_release,
	},
};

#ifdef CONFIG_PM
static int
gp_cdsp_suspend(
	struct platform_device *pdev,
	pm_message_t state
)
{
	if(p_cdsp_dev->open_cnt > 0) {
		gp_cdsp_clock_enable(0);
		if(p_cdsp_dev->cb_func->standby) {
			p_cdsp_dev->cb_func->standby(1);
		}
	}
	return 0;
}

static int
gp_cdsp_resume(
	struct platform_device *pdev
)
{
	if( p_cdsp_dev->open_cnt > 0) {
		gp_cdsp_clock_enable(1);
		gpHalCdspReset();
		gpHalCdspFrontReset();
		if(p_cdsp_dev->cb_func->standby) {
			p_cdsp_dev->cb_func->standby(0);
		}
	}
	return 0;
}
#else
#define gp_cdsp_suspend NULL
#define gp_cdsp_resume NULL
#endif

static struct platform_driver gp_cdsp_driver =
{
	.suspend = gp_cdsp_suspend,
	.resume = gp_cdsp_resume,
	.driver	=
	{
		.owner	= THIS_MODULE,
		.name	= "gp-cdsp"
	},
};

static int __init
cdsp_init_module(
	void
)
{
	int nRet = -ENOMEM;

	DEBUG(KERN_WARNING "ModuleInit: cdsp \n");
	/* memory alloc */
	p_cdsp_dev = (gpCdspDev_t *)kzalloc(sizeof(gpCdspDev_t), GFP_KERNEL);
	if(!p_cdsp_dev) {
		RETURN(-1);
	}
	
	p_cdsp_dev->aewin_addr[0] = (unsigned char *)gp_chunk_malloc(current->tgid, 64*2);
	p_cdsp_dev->aewin_addr[1] = p_cdsp_dev->aewin_addr[0] + 64;
	if(p_cdsp_dev->aewin_addr[0] == 0) {
		RETURN(-1);
	}
	
	/* register irq */
	nRet = request_irq(IRQ_AC97,
					  gp_cdsp_irq_handler,
					  IRQF_DISABLED,
					  "CDSP_IRQ",
					  p_cdsp_dev);
	if(nRet < 0) {
		RETURN(-1);
	}
	
	/* initialize */
	init_MUTEX(&p_cdsp_dev->sem);
	init_waitqueue_head(&p_cdsp_dev->cdsp_wait_queue);

	/* register char device */
	p_cdsp_dev->dev.name  = "cdsp";
	p_cdsp_dev->dev.minor = MISC_DYNAMIC_MINOR;
	p_cdsp_dev->dev.fops  = &cdsp_fops;
	nRet = misc_register(&p_cdsp_dev->dev);
	if(nRet < 0) {
		DERROR("cdsp device register fail\n");
		RETURN(-ENXIO);
	}

	/* register platform device/driver */
	platform_device_register(&gp_cdsp_device);
	platform_driver_register(&gp_cdsp_driver);

__return:
	if(nRet < 0) {
		DERROR(KERN_WARNING "CdspInitFail\n");
		/* free irq */
		free_irq(IRQ_AC97, p_cdsp_dev);

		/* free char device */
		misc_deregister(&p_cdsp_dev->dev);

		/* platform unregister */
		platform_device_unregister(&gp_cdsp_device);
		platform_driver_unregister(&gp_cdsp_driver);

		/* free memory */
		gp_chunk_free((void *)p_cdsp_dev->aewin_addr[0]);
		kfree(p_cdsp_dev);
	}
	return nRet;
}

static void __exit
cdsp_exit_module(
	void
)
{
	DEBUG(KERN_WARNING "ModuleExit: cdsp \n");
	/* free irq */
	free_irq(IRQ_AC97, p_cdsp_dev);

	/* free char device */
	misc_deregister(&p_cdsp_dev->dev);

	/* platform unregister */
	platform_device_unregister(&gp_cdsp_device);
	platform_driver_unregister(&gp_cdsp_driver);

	/* free memory */
	gp_chunk_free((void *)p_cdsp_dev->aewin_addr[0]);
	kfree(p_cdsp_dev);
}

module_init(cdsp_init_module);
module_exit(cdsp_exit_module);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/
MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("Generalplus CDSP Driver");
MODULE_LICENSE_GP;
MODULE_VERSION("1.0");



