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
#ifndef _GP_CSI1_H
#define _GP_CSI1_H

#include <mach/gp_csi.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define CCIR601			0
#define CCIR656 		1
#define HREF			2
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

/* csi clock source */
#define CSI_CLK_SPLL	0
#define CSI_CLK_USBPHY	1

/* CUBIC MODE SET */
#define CUBIC_64X64		0
#define CUBIC_32X32		1

/* motion detect */
#define MD_16x16		0
#define MD_8x8			1

#define MD_AVG			0
#define MD_SINGLE		1
#define MD_AVG_IIR		2
#define MD_SINGLE_IIR	3

#define MD_DATA_RGB		0
#define MD_DATA_YUV		1

#define MD_QVGA_SIZE	0
#define MD_VGA_SIZE		1

#define MD_EVERY1		0
#define MD_EVERY2		1
#define MD_EVERY4		2
#define MD_EVERY8		3

typedef enum
{	
	MSG_CSI1_CUBIC = 0x20000000,
	MSG_CSI1_BLACKSCREEN,
	MSG_CSI1_BLENDING,
	MSG_CSI1_MD,
	MSG_CSI1_SENSOR,
	MSG_CSI1_MAX
}MSG_CSI1_CTRL_ID;

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct gpCsi1Cubic_s
{
	unsigned int CubicEn;
	unsigned int CubicMode;
}gpCsi1Cubic_t;

typedef struct gpCsi1BlackScreen_s
{
	unsigned int BlueScreenEn;
	unsigned int hstart;
	unsigned int vstart;
	unsigned int view_hsize;
	unsigned int view_vsize;
	unsigned int r_upper;	
	unsigned int g_upper;
	unsigned int b_upper;
	unsigned int r_lower;	
	unsigned int g_lower;
	unsigned int b_lower;
}gpCsi1BlackScreen_t;

typedef struct gpCsi1Blending_s
{
	unsigned int BlendEn;
	unsigned int BlendLevel;	/* 0 ~ 63 */	
}gpCsi1Blending_t;

typedef struct gpCsi1MD_s
{
	unsigned int MDEn;
	unsigned int MDBlk8x8;	
	unsigned int MDMode;
	unsigned int MDYUV;	
	unsigned int MDVGA;
	unsigned int MDFrame;	
	unsigned int threshold;

	unsigned int hpos;
	unsigned int vpos;
	unsigned int md_fbaddr[3];
}gpCsi1MD_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

#endif
