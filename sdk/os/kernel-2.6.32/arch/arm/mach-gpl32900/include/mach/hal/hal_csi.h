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

#ifndef _HAL_CSI_H
#define _HAL_CSI_H

#include <mach/typedef.h>

#define CSI_CLKEN		(1<<10)
#define CSI_CNT_EN		(1<<8)
#define CSI_UPDATESET	(1<<31)
#define	CSI_SEL_8050	(1<<15)

#define	CSI_EN				(1<<0)
#define	CSI_CCIR601			(0<<1)
#define	CSI_CCIR656			(1<<1)
#define	CSI_HSYNC_LACT		(0<<2)
#define	CSI_HSYNC_HACT		(1<<2)
#define	CSI_VSYNC_LACT		(0<<3)
#define	CSI_VSYNC_HACT		(1<<3)
#define CSI_FIELD_ODDL		(0<<4)
#define	CSI_FIELD_ODDH		(1<<4)
#define	CSI_NON_INTERLACE		(0<<5)
#define	CSI_INTERLACE	(1<<5)
#define	CSI_RGBIN			(0<<8)
#define	CSI_YUVIN			(1<<8)
#define	CSI_RGBOUT			(0<<9)
#define	CSI_YUVOUT			(1<<9)
#define	CSI_INSEQ_UYVY		(0<<10)
#define	CSI_INSEQ_YUYV		(1<<10)
#define	CSI_RGB565				(0<<15)
#define	CSI_RGB1555				(1<<15)
#define	CSI_FMMODE_EVERYFRM	(0<<20)
#define	CSI_FMMODE_ODDFIELD	(1<<20)
#define	CSI_FMMODE_EVENFIELD	(2<<20)
#define CSI_SAMPLE_POS			(0<<22)
#define	CSI_SAMPLE_NEG			(1<<22)
#define	CSI_FRAME_DIS		(1<<27)

void gpHalCsiSetMclk(UINT32 clk_sel, UINT32 clko_div, UINT32 pclk_dly, UINT32 pclk_revb);
void gpHalCsiGetMclk(UINT8 *clk_sel, UINT8 *clko_div, UINT8 *pclk_dly, UINT8 *pclk_revb);
void gpHalCsiInit(void);
void gpHalCsiSetBuf(UINT32 addr);
UINT32 gpHalCsiClearisr(void);//UINT32 idx, UINT32 addr);
void gpHalCsiSetFormat( UINT32 fmt );
void gpHalCsiSetResolution(	UINT32 hset, UINT32 vset, UINT32 stp, UINT32 hoffset, UINT32 voffset);
void gpHalCsiStart(void);
void gpHalCsiSetCtrl( UINT32 ctrl );
void gpHalCsiGetCtrl( UINT32 *ctrl );
void gpHalCsiSuspend(void);
void gpHalCsiResume(void);
void gpHalCsiStop(void);
void gpHalCsiClose(void);

#endif
