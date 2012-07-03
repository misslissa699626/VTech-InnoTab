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

#ifndef _HAL_CSI1_H
#define _HAL_CSI1_H

#include <mach/typedef.h>

// CSI1 MODULE CLOCK
#define CSI_SEL_32900	(1<<6)
#define	PPU_27M_EN		(1<<5)

// CSI1 IRQ bit
#define CSI1_FRAME_END			(1<<6)
#define CSI1_MOTION_DECT		(1<<7)
#define CSI1_POST_HIT			(1<<8)
#define CSI1_MD_UNDER_RUN		(1<<9)
#define CSI1_UNDER_RUN			(1<<15)

// CSI1_CTRL0
#define	CSI1_EN					(1<<0)			// Sensor controller enable bit
#define CSI1_CAP				(1<<1)			// Capture / preview mode
#define CSI1_HREF				(1<<2)			// HREF / HSYNC mode
#define	CSI1_RGB888IN			(0<<3)
#define	CSI1_RGB565IN			(1<<3)			// RGB565 / RGB888 input @YUVIN=0B
#define CSI1_CLKIINV			(1<<4)			// Invert input clock
#define CSI1_YUVIN				(1<<5)			// YUV / RGB input
#define CSI1_YUVOUT				(1<<6)			// YUV / RGB output
#define CSI1_BSEN				(1<<7)			// Enable blue screen effect
#define CSI1_CCIR656			(1<<8)			// CCIR656 / CCIR601 Interface
#define CSI1_FGET_RISE			(1<<9)			// Field latch timing at the rising edge of VSYNC
#define CSI1_HRST_FALL			(0<<10)			// Horizontal counter reset at the rising edge of HSYNC
#define CSI1_HRST_RISE			(1<<10)			// Horizontal counter reset at the rising edge of HSYNC
#define CSI1_VADD_FALL			(0<<11)			// Vertical counter increase at the rising edge of HSYNC
#define CSI1_VADD_RISE			(1<<11)			// Vertical counter increase at the rising edge of HSYNC
#define CSI1_VRST_FALL			(0<<12)			// Vertical counter reset at the rising edge of VSYNC
#define CSI1_VRST_RISE			(1<<12)			// Vertical counter reset at the rising edge of VSYNC
#define CSI1_YUV_YUYV			(1<<13)			// YUYV(GBGR) / UYVY (BGRG) selection
#define CSI1_FIELDINV    		(1<<14)			// Invert field input
#define CSI1_INTERLACE  		(1<<15)			// Interlace / non-interlace mode

// CSI1_CTRL1
#define CSI1_D_TYPE0         	(0<<0)			// Data latch delay 1 clock
#define CSI1_D_TYPE1         	(1<<0)			// Data latch delay 2 clock
#define CSI1_D_TYPE2         	(2<<0)			// Data latch delay 3 clock
#define CSI1_D_TYPE3         	(3<<0)			// Data latch delay 4 clock
#define CSI1_CLKOINV			(1<<3)			// Invert output clock
#define	CSI1_RGB565				(0<<4)
#define CSI1_RGB1555			(1<<4)			// RGB1555 / RGB565 mode output
#define CSI1_INVYUVO          	(1<<6)			// Invert output UV's bit 7
#define CSI1_CLKOEN				(1<<7)			// CSI output clock enable
#define CSI1_CUTEN				(1<<8)			// Screen CUT enable
#define CSI1_INVYUVI			(1<<9)			// Invert input UV's bit 7
#define CSI1_YONLY				(1<<10)			// Only Y output enable
#define CSI1_CLK_SEL27M			(0<<11)			// 27MHz
#define CSI1_CLK_SEL48M			(1<<11)			// 48MHz
#define	CSI1_CELL				(1<<12)
#define	CSI1_CELL32X32			(1<<13)
#define	CSI1_NOSTOP				(1<<14)
#define	CSI1_HIGHPRI			(1<<15)

// CSI1 Format 
#define CSI1_UYVY_IN			0x01
#define CSI1_YUYV_IN			0x02
#define CSI1_BGRG_IN			0x03
#define CSI1_GBGR_IN			0x04
#define CSI1_RGB565_IN			0x05
#define CSI1_VYUY_OUT			0x10
#define CSI1_YUYV_OUT			0x20
#define CSI1_RGB565_OUT			0x30
#define CSI1_RGB1555_OUT		0x40
#define CSI1_YONLY_OUT			0x50

void gpHalCsi1SetMclk(UINT32 clk_sel, UINT32 clko_div, UINT32 pclk_dly, UINT32 pclk_revb);
void gpHalCsi1GetMclk(UINT8 *clk_sel, UINT8 *clko_div, UINT8 *pclk_dly, UINT8 *pclk_revb);
UINT32 gpHalCsi1Clearisr(void);
void gpHalCsi1SetIRQ(UINT32 bits, UINT32 enable);
void gpHalCsi1SetBuf(UINT32 addr);
void gpHalCsi1SetResolution(UINT16 hsize,UINT16 vsize);
void gpHalCsi1SetEnable(UINT32 enable);
void gpHalCsi1Start(void);
void gpHalCsi1Stop(void);
void gpHalCsi1Close(void);
void gpHalCsi1Init(void);
SINT32 gpHalCsi1SetInputFmt(UINT32 fmt);
SINT32 gpHalCsi1SetOutputFmt(UINT32 fmt);
void gpHalCsi1SetHVSync(UINT32 HsyncAct, UINT32 VsyncAct,UINT32 clki_inv);
void gpHalCsi1SetDataLatchTiming(UINT8 d_type);
SINT32 gpHalCsi1SetInterface(UINT32 filed_interlace, UINT32 field_inv, UINT32 iface);
UINT32 gpHalCsi1GetCtrl(UINT32 n);
void gpHalCsi1Suspend(void);
void gpHalCsi1Resume(void);

void gpHalCsi1SetInvYUV(UINT8 inv_yuvin, UINT8 inv_yuvout);
void gpHalCsi1SetHVStart(UINT16 hlstart, UINT16 vl0start, UINT16 vl1start);
SINT32 gpHalCsi1SetScaleDown(UINT16 tar_width, UINT16 tar_height);
void gpHalCsi1SetScreenCut(UINT16 h_cutstartline, UINT16 v_cutstartline, UINT16 h_cutsize, UINT16 v_cutsize);

void gpHalCsi1SetCubic(UINT8 CubicEn,UINT8 Cubic32);
void gpHalCsi1GetCubic(UINT32 *CubicEn, UINT32 *Cubic32);

void gpHalCsi1SetBlackScreen(UINT16 bs_hstart, UINT16 bs_vstart,UINT16 view_hsize,UINT16 view_vsize);
void gpHalCsi1GetBlackScreen(UINT32 *bs_hstart,	UINT32 *bs_vstart, UINT32 *view_hsize, UINT32 *view_vsize);
void gpHalCsi1SetBlueScreen(UINT8 BlueScreenEn, UINT8 r_upper, UINT8 g_upper, UINT8 b_upper,
						UINT8 r_lower, UINT8 g_lower, UINT8 b_lower);
void gpHalCsi1GetBlueScreen(UINT32 *BlueScreenEn, UINT32 *r_upper, UINT32 *g_upper,	UINT32 *b_upper,
						UINT32 *r_lower, UINT32 *g_lower, UINT32 *b_lower);

void gpHalCsi1SetBlending(UINT8 blend_en, UINT8 blend_level);
void gpHalCsi1GetBlending(UINT32 *blend_en,UINT32 *blend_level);

void gpHalCsi1SetMDEn(UINT8 MD_En, UINT8 MD_Frame, UINT8 MD_VGA, UINT8 MD_yuv,
					UINT8 MD_mode, UINT8 MDBlk8x8, UINT8 threshold);
void gpHalCsi1SetMDHVPos(UINT16 md_h_pos, UINT16 md_v_pos);
void gpHalCsi1SetMDFbAddr(UINT32 md_fb_addr); 
#endif
