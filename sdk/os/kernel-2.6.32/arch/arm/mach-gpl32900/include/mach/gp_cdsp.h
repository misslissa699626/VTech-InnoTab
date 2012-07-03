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
#ifndef _GP_CDSP_H_
#define _GP_CDSP_H_

#include <mach/gp_csi.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#ifndef ENABLE
#define ENABLE		1
#endif
#ifndef DISABLE
#define DISABLE		0
#endif

/* cdsp interrupt */
#define C_ISR_AF			0x0
#define C_ISR_AWB			0x1
#define C_ISR_AE			0x2
#define C_ISR_OVERFLOW		0x3
#define C_ISR_EOF			0x4
#define C_ISR_FACWR			0x5
#define C_ISR_MAX			0x6

/* cdsp input source */
#define C_CDSP_FRONT		0x0
#define C_CDSP_SDRAM		0x1
#define C_CDSP_MIPI			0x2

/* yuv special mode */
#define SP_YUV_NONE			0x0
#define SP_YUV_NEGATIVE		0x1
#define SP_YUV_BINARIZE		0x2
#define SP_YUV_YbYcSatHue	0x3
#define SP_YUV_EMBOSSMENT	0x4
#define SP_YUV_EMBOSSMENTx2	0x5
#define SP_YUV_EMBOSSMENTx4	0x6
#define SP_YUV_EMBOSSMENTx8	0x7

/* raw spec mode */
#define SP_RAW_NONE			0x0
#define SP_RAW_NEGATIVE		0x1
#define SP_RAW_SOL_ARISE	0x2
#define SP_RAW_EMBOSSMENT	0x3
#define SP_RAW_BINARIZE		0x4
#define SP_RAW_SEPIA		0x5
#define SP_RAW_BLACK_WHITE	0x6

/* rgb path */
#define RGB_NONE			0x0
#define RGB_PATH1			0x1
#define RGB_PATH2			0x3
#define RGB_PATH3			0x5
/* H/V Polarity */
#define C_CCIR601			0
#define C_CCIR656 			1
#define	C_HSYNC_HACT		0
#define	C_HSYNC_LACT		1
#define	C_VSYNC_HACT		0
#define	C_VSYNC_LACT		1
/* Interface setting */
#define CCIR601				0
#define CCIR656 			1
#define HREF 				2
#define	HSYNC_LACT			0
#define	HSYNC_HACT			1
#define	VSYNC_LACT			0
#define	VSYNC_HACT			1
#define	FIELD_ODDL			0
#define	FIELD_ODDH			1
#define	NON_INTERLACE		0
#define	INTERLACE			1
#define	RGBIN				0
#define	YUVIN				1
#define RGBOUT				0
#define	YUVOUT				1
#define	INSEQ_UYVU			0
#define	INSEQ_YUYV			1
#define	EVERYFRM			0
#define	ODDFIELD			1
#define	EVENFIELD			2
#define	SAMPLE_POSI			0
#define	SAMPLE_NEG			1

/* csi clock source */
#define CSI_CLK_SPLL		0
#define CSI_CLK_USBPHY		1

/* VIDIOC_G_CTRL / VIDIOC_S_CTRL ID*/
typedef enum
{	
	MSG_CDSP_FRONT_CLKO = 0x10000000,
	MSG_CDSP_SCALE_CROP,
	
	MSG_CDSP_BADPIX_OB,
	MSG_CDSP_LENS_CMP,
	MSG_CDSP_WBGAIN,
	MSG_CDSP_LUT_GAMMA,
	MSG_CDSP_INTERPOLATION,
	MSG_CDSP_EDGE,
	MSG_CDSP_COLOR_MATRIX,
	MSG_CDSP_POSWB_RGB2YUV,

	MSG_CDSP_YUV_INSERT,
	MSG_CDSP_YUV_HAVG,
	MSG_CDSP_SPEC_MODE,
	MSG_CDSP_SAT_HUE,
	MSG_CDSP_SUPPRESSION,

	MSG_CDSP_RAW_WIN,
	MSG_CDSP_AE_WIN,
	MSG_CDSP_AF_WIN,
	MSG_CDSP_AWB_WIN,
	MSG_CDSP_WBGAIN2,
	MSG_CDSP_HISTGM,
	
	MSG_CDSP_3A_STATISTIC,
	MSG_CDSP_SENSOR,
	MSG_CDSP_MAX
}CDSP_MSG_CTRL_ID;

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct gpCdspFront_s
{
	unsigned char	Interface;		/* 0:CCIR601, fix CCIR601 */
	unsigned char	HsyncAct;		/* 0:HACT, 1:LACT */ 
	unsigned char	VsyncAct;		/* 0:HACT, 1:LACT */
	unsigned char 	CsiSyncEn;		/* 0:disable, 1:enable, do sync after csi datain */

	unsigned int 	mclk_out;		/* MCU output to sensor clock */	
	unsigned char	pclk_reverse;	/* 0:disable, 1:enable, MCU latch pclk reserse */ 
	unsigned char 	pclk_dly;		/* 0~0xF, MCU latch pclk delay */
	
	unsigned char	yuv_range;		/* 0~7, yuv range */ 
	unsigned char	rgb_path;		/* 0~3, rgb path */
}gpCdspFront_t;

typedef struct gpCdspScalePara_s 
{
	/* raw hscale */
	unsigned char 	hscale_en;		/* 0:disable, 1:enable */ 		
	unsigned char 	hscale_mode;	/* 0:drop, 1:filter */ 
	unsigned short	dst_hsize;		/* 0~0xFFF */

	/* crop function */ 
	unsigned int	crop_en;		/* 0:disable, 1:enable */ 
	unsigned short	crop_hoffset;	/* 1~0xFFF */
	unsigned short	crop_voffset;	/* 1~0xFFF */
	unsigned short	crop_hsize;		/* 1~0xFFF */	
	unsigned short	crop_vsize;		/* 1~0xFFF */
	
	/* yuv h/v scale down */
	unsigned char	yuvhscale_en;	/* 0:disable, 1:enable */ 
	unsigned char	yuvvscale_en;	/* 0:disable, 1:enable */ 
	unsigned char	yuvhscale_mode; /* 0:drop, 1:filter */ 
	unsigned char	yuvvscale_mode; /* 0:drop, 1:filter */ 
	unsigned short	yuv_dst_hsize;	/* 0~0xFFF */
	unsigned short	yuv_dst_vsize;	/* 0~0xFFF */

	/* read back size */
	unsigned short	img_rb_h_size;		/* 0~0xFFF */
	unsigned short	img_rb_v_size;		/* 0~0xFFF */
}gpCdspScalePara_t;

typedef struct gpCdspBadPixOB_s
{
	/* bad pixel */
	unsigned char   badpixen; 		/* 0:disable, 1:enable */
	unsigned char   reserved0;
	unsigned short	bprthr;			/* 0~1023 */
	unsigned short	bpgthr;			/* 0~1023 */
	unsigned short	bpbthr;			/* 0~1023 */
	
	/* optical black */
	unsigned char	manuoben;		/* 0:disable, 1:enable */
	unsigned char   reserved1;
	signed short	manuob;			/* -1024~1023 */
	
	unsigned char   reserved2;
	unsigned char   reserved3;
	unsigned char	autooben;		/* 0:disable, 1:enable */
	unsigned char	obtype;			/* 0:2x256,1:4x256,2:8x256,3:16x256,4:256x2,5:256x4,6:256x8,7:256x16 */
	
	unsigned short	obHOffset;		/* 0~0xFFF */
	unsigned short	obVOffset;		/* 0~0xFFF */
	unsigned short	Ravg;			/* read back auto ob r avg */ 
	unsigned short	GRavg;			/* read back auto ob gr avg */
	unsigned short	Bavg;			/* read back auto ob b avg */
	unsigned short	GBavg;			/* read back auto ob gb avg */
}gpCdspBadPixOB_t;

typedef struct gpCdspLenCmp_s
{
	/* lens compensation */
	unsigned int	lcen;			/* 0:disable, 1:enable */
	unsigned short 	*lenscmp_table; /* lenscmp_table, size 512 byte */
	
	unsigned char	stepfactor;		/* 0:2^6,1:2^7,2:2^8,3:2^9,4:2^10,5:2^11,6:2^12,7:2^13 */
	unsigned char	xminc;			/* 0~0xF */
	unsigned char  	ymoinc;			/* 0~0xF */
	unsigned char  	ymeinc;			/* 0~0xF */
	unsigned char 	reserved0;
	unsigned char 	reserved1;
	unsigned char	reserved2;

	unsigned short	centx;			/* 0~0x1FFF */	
	unsigned short	centy;			/* 0~0x1FFF */
	unsigned short	xmoffset;		/* 0~0xFFF */
	unsigned short	ymoffset;		/* 0~0xFFF */
}gpCdspLenCmp_t;

typedef struct gpCdspWhtBal_s
{
	/* white balance */
	unsigned char	wboffseten;		/* 0:disable, 1:enable */
	signed char		roffset;		/* -128 ~ 127 */
	signed char		groffset;		/* -128 ~ 127 */
	signed char		boffset;		/* -128 ~ 127 */
	signed char		gboffset;		/* -128 ~ 127 */
	
	unsigned char	wbgainen;		/* 0:disable, 1:enable */
	unsigned char	global_gain;	/* 0~0xFF */
	unsigned char 	reserved;
	
	unsigned short	rgain;			/* 3.6 bit, 0/64~511/64 */
	unsigned short	grgain;			/* 3.6 bit, 0/64~511/64 */
	unsigned short	bgain;			/* 3.6 bit, 0/64~511/64 */
	unsigned short	gbgain;			/* 3.6 bit, 0/64~511/64 */
}gpCdspWhtBal_t;

typedef struct gpCdspIntpl_s
{
	/* interpolation */
	unsigned char	int_low_thr;	/* 0~0xFF */	
	unsigned char	int_hi_thr;		/* 0~0xFF */
	/* raw special mode */
	unsigned char	rawspecmode;	/* 0~6 */
	unsigned char	reserved;
}gpCdspIntpl_t;

typedef struct gpCdspEdge_s
{
	/* edge lut table */
	unsigned char	eluten;			/* 0:disable, 1:enable */
	unsigned char 	*edge_table;	/* edge table, 256byte */

	/* edge enhance */
	unsigned char	edgeen;			/* 0:disable, 1:enable */
	unsigned char	lhdiv;			/* 1/lhdiv,  1, 2, 4, 8, 16, 32, 64, 128 */
	unsigned char	lhtdiv;			/* 1/lhtdiv,  1, 2, 4, 8, 16, 32, 64, 128 */
	
	unsigned char	lhcoring;		/* 0~0xFF */
	unsigned char	ampga;			/* 1, 2, 3, 4 */
	unsigned char	edgedomain;		/* 0:add y value, 1:add RGB value */
	unsigned char	lhmode;			/* 0:USE HPF LF00 matrix, 1:default matrix */ 

	unsigned char	Qthr;			/* edge thr set */
	/* edge filter */
	unsigned char	lf00;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	unsigned char	lf01;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	unsigned char	lf02;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */

	unsigned char	lf10;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	unsigned char	lf11;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	unsigned char	lf12;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	unsigned char	lf20;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */

	unsigned char	lf21;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	unsigned char	lf22;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	unsigned char	reserved0;
	unsigned char	reserved1;

	unsigned int 	Qcnt;			/* read back edge thr count */
}gpCdspEdge_t;

typedef struct gpCdspCorMatrix_s
{
	/* color matrix */
	unsigned char	colcorren;		/* 0:disable, 1:enable */
	unsigned char	reserved0;
	
	signed short	a11;			/* 1 sign + 3.6 bit, -512/64~511/64 */
	signed short	a12;			/* 1 sign + 3.6 bit, -512/64~511/64 */
	signed short	a13;			/* 1 sign + 3.6 bit, -512/64~511/64 */
	signed short	a21;			/* 1 sign + 3.6 bit, -512/64~511/64 */
	signed short	a22;			/* 1 sign + 3.6 bit, -512/64~511/64 */
	signed short	a23;			/* 1 sign + 3.6 bit, -512/64~511/64 */
	signed short	a31;			/* 1 sign + 3.6 bit, -512/64~511/64 */
	signed short	a32;			/* 1 sign + 3.6 bit, -512/64~511/64 */
	signed short	a33;			/* 1 sign + 3.6 bit, -512/64~511/64 */
}gpCdspCorMatrix_t;

typedef struct gpCdspRgb2Yuv_s
{
	/* pre rb clamp */
	unsigned char  	pre_rbclamp;
	/* rb clamp */
	unsigned char  	rbclampen;
	unsigned short 	rbclamp;
	
	/* Uv divider */
	unsigned char	uvdiven;		/* 0:disable, 1:enable */
	unsigned char 	reserved;
	unsigned char	Yvalue_T1;		/* 0~0xFF, when Y=T1, UV divide 1/8 */
	unsigned char	Yvalue_T2;		/* 0~0xFF, when Y=T2, UV divide 2/8 */
	unsigned char	Yvalue_T3;		/* 0~0xFF, when Y=T3, UV divide 3/8 */
	unsigned char	Yvalue_T4;		/* 0~0xFF, when Y=T4, UV divide 4/8 */
	unsigned char	Yvalue_T5;		/* 0~0xFF, when Y=T5, UV divide 5/8 */
	unsigned char	Yvalue_T6;		/* 0~0xFF, when Y=T6, UV divide 6/8 */
}gpCdspRgb2Yuv_t;

typedef struct gpCdspGamma_s
{
	unsigned int	lut_gamma_en;	/* 0:disable, 1:enable */
	unsigned int 	*gamma_table;	/* gamma table, 512 byte */ 
}gpCdspGamma_t;

typedef struct gpCdspSpecMod_s
{
	/* yuv special mode */
	unsigned char	yuvspecmode;	/* 0~7 */
	unsigned char	binarthr;		/* 0~0xFF */
	unsigned char	reserved0;
	unsigned char	reserved1;
}gpCdspSpecMod_t;

typedef struct gpCdspYuvInsert_s
{
	/* yuv 422 to 444 */
	unsigned char	yuv444_insert;	/* 0:disable, 1:enable */
	/* yuv coring threshold */
	unsigned char	y_coring;		/* 0~0xF */
	unsigned char	u_coring;		/* 0~0xF */
	unsigned char	v_coring;		/* 0~0xF */
	/* yuv coring subtraction */
	unsigned char	y_corval;		/* 0~0xF */
	unsigned char	u_corval;		/* 0~0xF */
	unsigned char	v_corval;		/* 0~0xF */
	unsigned char	reserved;
}gpCdspYuvInsert_t;

typedef struct gpCdspYuvHAvg_s
{
	/* h average */
	unsigned char	ytype;			/* 0:disable, 1:3tap, 2:5tap */
	unsigned char	utype;			/* 0:disable, 1:3tap, 2:5tap */
	unsigned char	vtype;			/* 0:disable, 1:3tap, 2:5tap */
	unsigned char   reserved;
}gpCdspYuvHAvg_t;

typedef struct gpCdspSatHue_s
{
	unsigned char	YbYcEn;			/* 0:disable, 1:enable */
	/* brightness & constract */
	signed char		y_offset;		/* -128 ~ 127 */
	signed char		u_offset;		/* -128 ~ 127 */
	signed char		v_offset;		/* -128 ~ 127 */
	
	unsigned char	y_scale;		/* 3.5 bit, 0/32~255/32 */	
	unsigned char	u_scale;		/* 3.5 bit, 0/32~255/32 */
	unsigned char	v_scale;		/* 3.5 bit, 0/32~255/32 */
	unsigned char	reserved;
	
	/* saturation, hue */
	signed char		u_huesindata;	/* 1 sign + 1.6 bit , -128/64~127/64 */
	signed char		u_huecosdata;	/* 1 sign + 1.6 bit , -128/64~127/64 */
	signed char		v_huesindata;	/* 1 sign + 1.6 bit , -128/64~127/64 */
	signed char		v_huecosdata;	/* 1 sign + 1.6 bit , -128/64~127/64 */
}gpCdspSatHue_t;

typedef struct gpCdspSuppression_s
{
	/* suppression */
	unsigned char	suppressen;		/* 0:disable, 1:enable */
	unsigned char   suppr_mode;		/* 0:edge, 1:denoise, 2:YLPF */
	
	/* y denoise */
	unsigned char 	denoisen;		/* 0:disable, 1:enable */
	unsigned char	denoisethrl;	/* 0~0xFF */
	unsigned char	denoisethrwth;	/* 1, 2, 4, 8, 16, 32, 64, 128 */
	unsigned char	yhtdiv;			/* 1, 2, 4, 8, 16, 32, 64, 128 */

	/* y lpf */
	unsigned char	lowyen;			/* 0:disable, 1:enable */
	unsigned char   reserved;
}gpCdspSuppression_t;

typedef struct gpCdspRawWin_s
{
	unsigned char	aeawb_src;		/* 0:from poswb, 1:form awb line ctrl */
	unsigned char	subample; 		/* 0:disable, 1:1/2, 2:1/4 subsample */
	unsigned char 	AeWinTest;		/* 0:disable, 1:enable */
	unsigned char 	AfWinTest;		/* 0:disable, 1:enable */
	/* rgb window set */
	unsigned short	hwdoffset;		/* 0~0x3FFF */
	unsigned short	vwdoffset;		/* 0~0x3FFF */
	unsigned short	hwdsize;		/* 0~0x1FF, h cell size */
	unsigned short	vwdsize;		/* 0~0x1FF, v cell size */
}gpCdspRawWin_t;

typedef struct gpCdspAF_s
{
	/* auto focuse */
	unsigned char	af_win_en;		/* 0:disable, 1:enable */
	unsigned char	af_win_hold;	/* 0:disable, 1:enable */
	unsigned char	reserved00;
	unsigned char	reserved01;
	/* af window 1 */
	unsigned short	af1_hoffset;	/* 0~0xFFF */
	unsigned short 	af1_voffset;	/* 0~0xFFF */
	unsigned short 	af1_hsize;		/* 0~0xFFF */
	unsigned short	af1_vsize;		/* 0~0xFFF */
	/* af window 2 */
	unsigned short	af2_hoffset;	/* 0~0xFFF */
	unsigned short 	af2_voffset;	/* 0~0xFFF */
	unsigned short 	af2_hsize;		/* 64, 256, 512, 1024, 2048 */
	unsigned short	af2_vsize;		/* 64, 256, 512, 1024, 2048 */
	/* af window 3 */
	unsigned short	af3_hoffset;	/* 0~0xFFF */
	unsigned short 	af3_voffset;	/* 0~0xFFF */
	unsigned short 	af3_hsize;		/* 64, 256, 512, 1024, 2048 */
	unsigned short	af3_vsize;		/* 64, 256, 512, 1024, 2048 */
}gpCdspAF_t;

typedef struct gpCdspAE_s
{
	/* auto expore */
	unsigned char	ae_win_en;		/* 0:disable, 1:enable */
	unsigned char	ae_win_hold;	/* 0:disable, 1:enable */
	unsigned char	reserved0;		
	unsigned char	reserved1;		
}gpCdspAE_t;

typedef struct gpCdspAWB_s
{
	/* auto white balance */
	unsigned char	awb_win_en;		/* 0:disable, 1:enable */
	unsigned char	awb_win_hold;	/* 0:disable, 1:enable */
	unsigned char	awbclamp_en;	/* 0:disable, 1:enable */
	unsigned char	awbwinthr;		/* 0~0xFF */
	unsigned char	sindata;		/* 1 sign + 1.6bits, -128/64~127/64 */
	unsigned char	cosdata;		/* 1 sign + 1.6bits, -128/64~127/64 */
	unsigned char	reserved0;
	unsigned char	reserved1;
	
	unsigned char	Ythr0;			/* 0~0xFF */
	unsigned char	Ythr1;			/* 0~0xFF */
	unsigned char	Ythr2;			/* 0~0xFF */
	unsigned char	Ythr3;			/* 0~0xFF */
	
	signed short	UL1N1;			/* -256 ~ 255 */
	signed short	UL1P1;			/* -256 ~ 255 */
	signed short	VL1N1;			/* -256 ~ 255 */
	signed short	VL1P1;			/* -256 ~ 255 */
	
	signed short	UL1N2;			/* -256 ~ 255 */
	signed short	UL1P2;			/* -256 ~ 255 */
	signed short	VL1N2;			/* -256 ~ 255 */
	signed short	VL1P2;			/* -256 ~ 255 */

	signed short	UL1N3;			/* -256 ~ 255 */
	signed short	UL1P3;			/* -256 ~ 255 */
	signed short	VL1N3;			/* -256 ~ 255 */
	signed short	VL1P3;			/* -256 ~ 255 */

}gpCdspAWB_t;

typedef struct gpCdspWbGain2_s
{
	/* wb gain2 */
	unsigned char	wbgain2en;		/* 0:disable, 1:enable */
	unsigned char	reserved;
	unsigned short	rgain2;			/* 0~0x1FF */
	unsigned short	ggain2;			/* 0~0x1FF */
	unsigned short	bgain2;			/* 0~0x1FF */
}gpCdspWbGain2_t;

typedef struct gpCdspHistgm_s
{
	/* histgm */
	unsigned char	his_en;			/* 0:disable, 1:enable */
	unsigned char	his_hold_en;	/* 0:disable, 1:enable */
	unsigned char	hislowthr;		/* 0~0xFF */
	unsigned char	hishighthr;		/* 0~0xFF */

	unsigned int 	hislowcnt;		/* read back result low count */
	unsigned int 	hishicnt;		/* read back result hight count */
}gpCdspHistgm_t;

typedef struct gpCdsp3aResult_s
{
	/* af */
	unsigned long long 	af1_h_value;
	unsigned long long 	af1_v_value;
	unsigned long long 	af2_h_value;
	unsigned long long 	af2_v_value;
	unsigned long long 	af3_h_value;
	unsigned long long 	af3_v_value;

	/* awb */
	unsigned int 		sumcnt1;
	unsigned long long 	sumg1;
	signed long long	sumrg1;
	signed long	long	sumbg1;

	unsigned int 		sumcnt2;
	unsigned long long 	sumg2;
	signed long long 	sumrg2;
	signed long long 	sumbg2;

	unsigned int 		sumcnt3;
	unsigned long long 	sumg3;
	signed long long	sumrg3;
	signed long long	sumbg3;

	/* ae */
	unsigned char ae_win[64];
}gpCdsp3aResult_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
/**
* @brief	cdsp register sensor use.
* @param	sd [in]: struct v4l2_subdev pointer
* @param	pedge [in]: edge table pointer
* @param	plenscmp [in]: lenscmp table pointer
* @param	pgamma [in]: gamma table pointer
* @return	SUCCESS/ERROR_ID
*/
int gp_cdsp_register_sensor(
	void *sd, 
	unsigned char *pedge, 
	unsigned short *plenscmp, 
	unsigned int *pgamma);

#endif /* _GP_CDSP_H_ */
