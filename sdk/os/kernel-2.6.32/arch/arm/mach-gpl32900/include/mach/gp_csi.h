 /**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Technology Co., Ltd.         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Technology Co., Ltd. All rights are reserved by Generalplus Technology*
 *  Co., Ltd. This software may only be used in accordance with the       *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Technology Co., Ltd. reserves the right to modify this    *
 *  software without notice.                                              *
 *                                                                        *
 *  Generalplus Technology Co., Ltd.                                      *
 *  3F, No.8, Dusing Rd., Science-Based Industrial Park,                  *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 **************************************************************************/
#ifndef _GP_CSI_H
#define _GP_CSI_H

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define CCIR601			0
#define CCIR656 		1
#define	HSYNC_LACT		0
#define	HSYNC_HACT		1
#define	VSYNC_LACT		0
#define	VSYNC_HACT		1
#define	FIELD_ODDL		0
#define	FIELD_ODDH		1
#define	NON_INTERLACE	0
#define	INTERLACE		1
#define	RGBIN			0
#define	YUVIN			1
#define RGBOUT			0
#define	YUVOUT			1
#define	INSEQ_UYVU		0
#define	INSEQ_YUYV		1
#define	EVERYFRM		0
#define	ODDFIELD		1
#define	EVENFIELD		2
#define	SAMPLE_POSI		0
#define	SAMPLE_NEG		1

#define	CSI_CCIR601				(0<<1)
#define	CSI_CCIR656				(1<<1)
#define	CSI_HSYNC_LACT			(0<<2)
#define	CSI_HSYNC_HACT			(1<<2)
#define	CSI_VSYNC_LACT			(0<<3)
#define	CSI_VSYNC_HACT			(1<<3)
#define CSI_FIELD_ODDL			(0<<4)
#define	CSI_FIELD_ODDH			(1<<4)
#define	CSI_NON_INTERLACE		(0<<5)
#define	CSI_INTERLACE			(1<<5)
#define	CSI_RGBIN				(0<<8)
#define	CSI_YUVIN				(1<<8)
#define	CSI_RGBOUT				(0<<9)
#define	CSI_YUVOUT				(1<<9)
#define	CSI_INSEQ_UYVY			(0<<10)
#define	CSI_INSEQ_YUYV			(1<<10)
#define	CSI_RGB565				(0<<15)
#define	CSI_RGB1555				(1<<15)
#define	CSI_FMMODE_EVERYFRM		(0<<20)
#define	CSI_FMMODE_ODDFIELD		(1<<20)
#define	CSI_FMMODE_EVENFIELD	(2<<20)
#define CSI_SAMPLE_POS			(0<<22)
#define	CSI_SAMPLE_NEG			(1<<22)
#define	CSI_FRAME_DIS			(1<<27)

/* csi clock source */
#define CSI_CLK_SPLL			0
#define CSI_CLK_USBPHY			1

typedef enum
{	
	MSG_CSI_SENSOR = 0x10000000,
	MSG_CSI_MAX
}MSG_CSI_CTRL_ID;

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct gpCsiMclk_s
{
	unsigned int 	mclk_out;		/* MCU output to sensor clock */
	unsigned char	mclk_sel;		/* 0: spll(cpx), 1: usb */
	unsigned char	pclk_revb;		/* 0:disable, 1:enable, MCU latch pclk reserse */
	unsigned char 	pclk_dly;		/* 0~0xF, MCU latch pclk delay */
	unsigned char 	prvi;			/* csi0: not use,
									   csi1: csi1 data latch timing, 0 ~ 3 delay pclk, 
									   csi2: csi2 do sync after data in, 0:disable, 1:enable, */
}gpCsiMclk_t;

typedef struct gpCsiCapture_s
{
	unsigned short width;
	unsigned short height;
	unsigned int pixelformat;
	unsigned int priv;
	unsigned int buffaddr;
	unsigned int waitcnt;
}gpCsiCapture_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
#define VIDIOC_S_INTERFACE	_IOWR('V', BASE_VIDIOC_PRIVATE+1, struct v4l2_interface)
#define VIDIOC_G_INTERFACE	_IOWR('V', BASE_VIDIOC_PRIVATE+2, struct v4l2_interface)
#define VIDIOC_S_MCLK		_IOWR('V', BASE_VIDIOC_PRIVATE+3, gpCsiMclk_t)
#define VIDIOC_G_MCLK		_IOWR('V', BASE_VIDIOC_PRIVATE+4, gpCsiMclk_t)
#define VIDIOC_CAPTURE		_IOWR('V', BASE_VIDIOC_PRIVATE+5, gpCsiCapture_t)
#endif
