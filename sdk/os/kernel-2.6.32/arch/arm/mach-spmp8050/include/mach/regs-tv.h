/*
 * arch/arm/mach-spmp8000/include/mach/regs-tv.h
 *
 * Copyright (C) 2007 Sunplus MultiMedia
 *
 * Syetem Unit Control Unit
 *
 */
#ifndef _TV_OUT_H_
#define _TV_OUT_H_

#define TV_OUT_BASE			          IO3_ADDRESS(0x2000)

//----------------------------------------------------------------------------
#define 	 TV_OUT_CTRL							((volatile unsigned int *)(TV_OUT_BASE+0x00))
#define	 TV_OUT_SATURATION			((volatile unsigned int *)(TV_OUT_BASE+0x0004))
#define	 TV_OUT_HUE							((volatile unsigned int *)(TV_OUT_BASE+0x0008))
#define	 TV_OUT_FADE_CTRL				((volatile unsigned int *)(TV_OUT_BASE+0x000C))
#define	 TV_OUT_FILTER_SEL				((volatile unsigned int *)(TV_OUT_BASE+0x0010))
#define	 TV_OUT_LUM_OFFSET			((volatile unsigned int *)(TV_OUT_BASE+0x0014))
#define	 TV_OUT_FRAME_END_CTRL	((volatile unsigned int *)(TV_OUT_BASE+0x0018))
#define	 TV_OUT_STATUS					((volatile unsigned int *)(TV_OUT_BASE+0x001C))
#define	 TV_OUT_LINE_COUNTER		((volatile unsigned int *)(TV_OUT_BASE+0x0020))
#define	 TV_OUT_FIFO_STATUS			((volatile unsigned int *)(TV_OUT_BASE+0x0024))
#define    TV_OUT_TV_IRQ_CTRL			((volatile unsigned int *)(TV_OUT_BASE+0x0040))
#define 	 TV_OUT_TV_IRQ_STATUS		((volatile unsigned int *)(TV_OUT_BASE+0x0044))
#define	 TV_OUT_VIDEO_DAC_CTRL	((volatile unsigned int *)(TV_OUT_BASE+0x0050))
#define	 TV_OUT_TEST_MODE				((volatile unsigned int *)(TV_OUT_BASE+0x0054))
#define	 TV_OUT_VDAC_TEST_DATA	((volatile unsigned int *)(TV_OUT_BASE+0x0058))
#define	 TV_OUT_RESERVED2_REG	((volatile unsigned int *)(TV_OUT_BASE+0x005C))
#define	 TV_OUT_VER_ADJ					((volatile unsigned int *)(TV_OUT_BASE+0x0060))
#define	 TV_OUT_HOR_ADJ					((volatile unsigned int *)(TV_OUT_BASE+0x0064))
#define	 TV_OUT_PAL_DISP_MODE	((volatile unsigned int *)(TV_OUT_BASE+0x0068))
#define	 TV_OUT_FINISH_FLAG			((volatile unsigned int *)(TV_OUT_BASE+0x0070))
#define	 TV_OUT_EDTV_CTRL				((volatile unsigned int *)(TV_OUT_BASE+0x0078))
#define	 TV_OUT_Y_SYNC						((volatile unsigned int *)(TV_OUT_BASE+0x007C))
#define	 TV_OUT_Y_BLANK					((volatile unsigned int *)(TV_OUT_BASE+0x0080))
#define	 TV_OUT_Y_SETUP					((volatile unsigned int *)(TV_OUT_BASE+0x0084))
#define	 TV_OUT_YPbPr_SEL				((volatile unsigned int *)(TV_OUT_BASE+0x0088))
#define	 TV_OUT_MONITOR_TYPE		((volatile unsigned int *)(TV_OUT_BASE+0x008C))
#define	 TV_OUT_SPMP8K_CTRL			((volatile unsigned int *)(TV_OUT_BASE+0x00C0))
#define	 TV_OUT_BASE_ADDR				((volatile unsigned int *)(TV_OUT_BASE+0x00C4))
#define	 TV_OUT_HPIX_NUM				((volatile unsigned int *)(TV_OUT_BASE+0x00C8))
#define	 TV_OUT_VPIX_NUM				((volatile unsigned int *)(TV_OUT_BASE+0x00CC))
#define	 TV_OUT_STR_LNO					((volatile unsigned int *)(TV_OUT_BASE+0x00D0))
#define	 TV_OUT_STR_PNO					((volatile unsigned int *)(TV_OUT_BASE+0x00D4))
#define	 TV_OUT_DUMMY_PIX				((volatile unsigned int *)(TV_OUT_BASE+0x00D8))
#define	 TV_OUT_BLANK_LEFT			((volatile unsigned int *)(TV_OUT_BASE+0x00DC))
#define	 TV_OUT_BLANK_RIGHT			((volatile unsigned int *)(TV_OUT_BASE+0x00E0))
#define	 TV_OUT_BLANK_TOP				((volatile unsigned int *)(TV_OUT_BASE+0x00E4))
#define	 TV_OUT_BLANK_BOTTOM		((volatile unsigned int *)(TV_OUT_BASE+0x00E8))
#define	 TV_OUT_BLANK_PATTERN	((volatile unsigned int *)(TV_OUT_BASE+0x00EC))
#define	 TV_OUT_HOR_SCALE_UP_CFG		((volatile unsigned int *)(TV_OUT_BASE+0x00F0))
#define	 TV_OUT_HOR_INIT_VAL		((volatile unsigned int *)(TV_OUT_BASE+0x00F4))
#define	 TV_OUT_OUT_HPIX_NUM		((volatile unsigned int *)(TV_OUT_BASE+0x00F8))
//----------------------------------------------------------------------------
#define 	 TV_OUT_CTRL_OFST						0x0000          
#define	 TV_OUT_SATURATION_OFST		0x0004
#define	 TV_OUT_HUE_OFST						0x0008
#define	 TV_OUT_FADE_CTRL_OFST			0x000C
#define	 TV_OUT_FILTER_SEL_OFST			0x0010
#define	 TV_OUT_LUM_OFFSET_OFST			0x0014
#define	 TV_OUT_FRAME_END_CTRL_OFST	0x0018
#define	 TV_OUT_STATUS_OFST					0x001C
#define	 TV_OUT_LINE_COUNTER_OFST		0x0020
#define	 TV_OUT_FIFO_STATUS_OFST			0x0024
#define    TV_OUT_TV_IRQ_CTRL_OFST			0x0040
#define 	 TV_OUT_TV_IRQ_STATUS_OFST		0x0044
#define	 TV_OUT_VIDEO_DAC_CTR_OFST	0x0050
#define	 TV_OUT_TEST_MODE_OFST				0x0054
#define	 TV_OUT_VDAC_TEST_DAT_OFST		0x0058
#define	 TV_OUT_RESERVED2_REG_OFST		0x005C
#define	 TV_OUT_VER_ADJ_OFST					0x0060
#define	 TV_OUT_HOR_ADJ_OFST					0x0064
#define	 TV_OUT_PAL_DISP_MODE_OFST		0x0068
#define	 TV_OUT_FINISH_FLAG_OFST			0x0070
#define	 TV_OUT_EDTV_CTRL_OFST				0x0078
#define	 TV_OUT_Y_SYNC_OFST					0x007C
#define	 TV_OUT_Y_BLANK_OFST					0x0080
#define	 TV_OUT_Y_SETUP_OFST					0x0084
#define	 TV_OUT_YPbPr_SEL_OFST				0x0088
#define	 TV_OUT_MONITOR_TYPE_OFST		0x008C
#define	 TV_OUT_SPMP8K_CTRL_OFST			0x00C0
#define	 TV_OUT_BASE_ADDR_OFST				0x00C4
#define	 TV_OUT_HPIX_NUM_OFST				0x00C8
#define	 TV_OUT_VPIX_NUM_OFST				0x00CC
#define	 TV_OUT_STR_LNO_OFST					0x00D0
#define	 TV_OUT_STR_PNO_OFST					0x00D4
#define	 TV_OUT_DUMMY_PIX_OFST				0x00D8
#define	 TV_OUT_BLANK_LEFT_OFST				0x00DC
#define	 TV_OUT_BLANK_RIGHT_OFST			0x00E0
#define	 TV_OUT_BLANK_TOP_OFST				0x00E4
#define	 TV_OUT_BLANK_BOTTOM_OFST		0x00E8
#define	 TV_OUT_BLANK_PATTERN_OFST		0x00EC
#define	 TV_OUT_HOR_SCALE_UP_CFG_OFST	0x00F0
#define	 TV_OUT_HOR_INIT_VAL_OFST				0x00F4
#define	 TV_OUT_OUT_HPIX_NUM_OFST			0x00F8

#define	 TV_OUT_ATTR_CTRL_INTERLACE				0x00000001
#define	 TV_OUT_ATTR_CTRL_NON_INTERLACE		0x00000000
#define	 TV_OUT_ATTR_CTRL_INTERLACE_MSK		0x00000001

#define	 TV_OUT_ATTR_CTRL_TYPE_NTSC				0x00000000
#define	 TV_OUT_ATTR_CTRL_TYPE_NTSC_J			0x00000040
#define	 TV_OUT_ATTR_CTRL_TYPE_NTSC443		0x00000080
#define	 TV_OUT_ATTR_CTRL_TYPE_PAL					0x00000002 
#define	 TV_OUT_ATTR_CTRL_TYPE_PAL_Nc			0x00000012
#define	 TV_OUT_ATTR_CTRL_TYPE_PAL_M			0x00000022
#define	 TV_OUT_ATTR_CTRL_TYPE_MSK					0x000000F2

#define	 TV_OUT_ATTR_CTRL_RES_QVGA					0x00000000  // 320x240
#define	 TV_OUT_ATTR_CTRL_RES_VGA					0x00000004  // 640x480
#define	 TV_OUT_ATTR_CTRL_RES_HVGA					0x00000008  // 640x240
#define	 TV_OUT_ATTR_CTRL_RES_QVGA_				0x0000000C  // 320x240
#define	 TV_OUT_ATTR_CTRL_RES_MSK					0x0000000C  // 320x240

#define	 TV_OUT_ATTR_CTRL_FMT_YCbCr422		0x00000000
#define	 TV_OUT_ATTR_CTRL_FMT_4Y4Cb4Y4Cr	0x00000800
#define	 TV_OUT_ATTR_CTRL_FMT_RGB565			0x00000100
#define	 TV_OUT_ATTR_CTRL_FMT_RGB888			0x00000100
#define	 TV_OUT_ATTR_CTRL_FMT_RGB555			0x00000500
#define	 TV_OUT_ATTR_CTRL_FMT_RGB_DOMAIN	0x00000100
#define	 TV_OUT_ATTR_CTRL_FMT_MSK					0x00000D00

#define	 TV_OUT_ATTR_CTRL_FMT_ENDIAN_LITTLE		0x00000000
#define	 TV_OUT_ATTR_CTRL_FMT_ENDIAN_BIG		0x00000200
#define	 TV_OUT_ATTR_CTRL_FMT_ENDIAN_MSK		0x00000200

#define	 TV_OUT_ATTR_CTRL_EN_ON				0x00001000
#define	 TV_OUT_ATTR_CTRL_EN_OFF				0x00000000
#define	 TV_OUT_ATTR_CTRL_EN_MSK				0x00001000
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_FILTER_SEL_Y_FILTER_NO			0x00000000
#define	 TV_OUT_ATTR_FILTER_SEL_Y_FILTER_9_TAP	0x00000001
#define	 TV_OUT_ATTR_FILTER_SEL_Y_FILTER_MSK		0x00000003

#define	 TV_OUT_ATTR_FILTER_SEL_Y_UPS_FILTER_2_TAP		0x00000000
#define	 TV_OUT_ATTR_FILTER_SEL_Y_UPS_FILTER_3_TAP		0x00000004
#define	 TV_OUT_ATTR_FILTER_SEL_Y_UPS_FILTER_7_TAP		0x00000008
#define	 TV_OUT_ATTR_FILTER_SEL_Y_UPS_FILTER_MSK			0x0000000C

#define	 TV_OUT_ATTR_FILTER_SEL_UV_FILTER_NO			0x00000000
#define	 TV_OUT_ATTR_FILTER_SEL_UV_FILTER_9_TAP		0x00000010
#define	 TV_OUT_ATTR_FILTER_SEL_UV_FILTER_MSK			0x00000030

#define	 TV_OUT_ATTR_FILTER_SEL_UV_UPS_FILTER_2_TAP	0x00000000
#define	 TV_OUT_ATTR_FILTER_SEL_UV_UPS_FILTER_3_TAP	0x00000040
#define	 TV_OUT_ATTR_FILTER_SEL_UV_UPS_FILTER_7_TAP	0x00000080
#define	 TV_OUT_ATTR_FILTER_SEL_UV_UPS_FILTER_MSK		0x000000C0
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_FRAME_END_CTRL_AUTO_SWAP				0x00000000
#define	 TV_OUT_ATTR_FRAME_END_CTRL_FIELD_END_SWAP	0x00000001
#define	 TV_OUT_ATTR_FRAME_END_CTRL_EVEN_END_SWAP	0x00000002
#define	 TV_OUT_ATTR_FRAME_END_CTRL_ODD_END_SWAP		0x00000003
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_STATUS_FIELD_MSK				0x00000001
#define	 TV_OUT_ATTR_STATUS_HSYNC_MSK				0x00000002
#define	 TV_OUT_ATTR_STATUS_VSYNC_MSK				0x00000004
#define	 TV_OUT_ATTR_STATUS_VBLANK_MSK			0x00000008
#define	 TV_OUT_ATTR_STATUS_ACTIVE_MSK			0x00000010
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_FIFO_STATUS_UNDERRUN_MSK			0x00000001
#define	 TV_OUT_ATTR_FIFO_STATUS_UNDERRUN_CLEAR		0x00000001
#define	 TV_OUT_ATTR_FIFO_STATUS_OVERFLOW_MSK			0x00000002
#define	 TV_OUT_ATTR_FIFO_STATUS_OVERFLOW_CLEAR		0x00000002
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_IRQ_VBLANK_DIS						0x00000000
#define	 TV_OUT_ATTR_IRQ_VBLANK_BY_FIELD_DIS	0x00000000
#define	 TV_OUT_ATTR_IRQ_VBLANK_BY_FIELD_EN		0x00000001
#define	 TV_OUT_ATTR_IRQ_VBLANK_BY_FRAME_DIS	0x00000100
#define	 TV_OUT_ATTR_IRQ_VBLANK_BY_FRAME_EN	0x00000101
#define	 TV_OUT_ATTR_IRQ_VBLANK_MSK						0x00000101
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_IRQ_STATUS_MSK						0x00000001
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_VIDEO_DAC_CTRL_EN				0x00000000
#define	 TV_OUT_ATTR_VIDEO_DAC_CTRL_DIS				0x00000001
#define	 TV_OUT_ATTR_VIDEO_DAC_CTRL_MSK				0x00000001

#define	 TV_OUT_ATTR_VIDEO_DAC_CTRL_VREF_EN	0x00000000
#define	 TV_OUT_ATTR_VIDEO_DAC_CTRL_VREF_DIS	0x00000002
#define	 TV_OUT_ATTR_VIDEO_DAC_CTRL_VREF_MSK	0x00000002
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_VER_ADJUST_MSK						0x0000001F
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_HOR_ADJUST_MSK						0x0000003F
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_PAL_DISP_MODE_240				0x00000000
#define	 TV_OUT_ATTR_PAL_DISP_MODE_288				0x00000001
#define	 TV_OUT_ATTR_PAL_DISP_MODE_MSK				0x00000001
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_EDTV_CTRL_SDTV						0x00000000
#define	 TV_OUT_ATTR_EDTV_CTRL_EDTV						0x00000001
#define	 TV_OUT_ATTR_EDTV_CTRL_MSK							0x00000001

#define	 TV_OUT_ATTR_EDTV_CTRL_V_576P				0x00000000
#define	 TV_OUT_ATTR_EDTV_CTRL_V_480P				0x00000002
#define	 TV_OUT_ATTR_EDTV_CTRL_V_MSK				0x00000002

#define	 TV_OUT_ATTR_EDTV_CTRL_H_720_DIS		0x00000000
#define	 TV_OUT_ATTR_EDTV_CTRL_H_720_EN			0x00000004
#define	 TV_OUT_ATTR_EDTV_CTRL_H_720_MSK		0x00000004
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_Y_SYNC_MSK			0x000003FF
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_Y_BLANK_MSK			0x000007FF
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_Y_SETUP_0mv		0x00000000
#define	 TV_OUT_ATTR_Y_SETUP_54mv		0x00000001
#define	 TV_OUT_ATTR_Y_SETUP_MSK			0x00000001
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_YPbPr_SEL_COMPOSITE_OUT		0x00000000
#define	 TV_OUT_ATTR_YPbPr_SEL_YPbPr_OUT				0x00000001
#define	 TV_OUT_ATTR_YPbPr_SEL_MSK								0x00000001
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_MONITOR_TYPE_263_PER_FIELD	0x00000000
#define	 TV_OUT_ATTR_MONITOR_TYPE_262_PER_FIELD	0x00000001
#define	 TV_OUT_ATTR_MONITOR_TYPE_MSK							0x00000001
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_SPMP8K_CTRL_RGB888_DIS		0x00000000
#define	 TV_OUT_ATTR_SPMP8K_CTRL_RGB888_EN			0x00000001
#define	 TV_OUT_ATTR_SPMP8K_CTRL_RGB888_MSK		0x00000001

#define	 TV_OUT_ATTR_SPMP8K_CTRL_MST_LINE			0x00000010

#define	 TV_OUT_ATTR_SPMP8K_CTRL_BURST_16			0x00000000
#define	 TV_OUT_ATTR_SPMP8K_CTRL_BURST_4			0x00000100
#define	 TV_OUT_ATTR_SPMP8K_CTRL_BURST_MSK		0x00000100

#define	 TV_OUT_ATTR_SPMP8K_CTRL_BOUND_CROSS_1K_EN		0x00000000
#define	 TV_OUT_ATTR_SPMP8K_CTRL_BOUND_CROSS_1K_DIS		0x00010000
#define	 TV_OUT_ATTR_SPMP8K_CTRL_BOUND_CROSS_1K_MSK	0x00010000

#define	 TV_OUT_ATTR_SPMP8K_CTRL_TYPE_YCbCr		0x00000000
#define	 TV_OUT_ATTR_SPMP8K_CTRL_TYPE_YUV			0x01000000
#define	 TV_OUT_ATTR_SPMP8K_CTRL_TYPE_MSK		0x01000000
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_HPIX_NUM_MSK			0x000003FF
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_VPIX_NUM_MSK			0x000003FF
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_STR_LNO_MSK				0x000003FF
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_STR_PNO_MSK				0x000003FF
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_DMY_PIX_MSK				0x000003FF
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_BLANK_LEFT_MSK		0x000003FF
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_BLANK_RIGHT_MSK	0x000003FF
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_BLANK_TOP_MSK		0x000003FF
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_BLANK_BOTTOM_MSK		0x000003FF
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_BLANK_PATTERN_MSK		0x00FFFFFF
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_HOR_SCALE_UP_FACTOR		0x0000FFFF
#define	 TV_OUT_ATTR_HOR_SCALE_UP_EN				0x00010000
#define	 TV_OUT_ATTR_HOR_SCALE_UP_MSK				0x0001FFFF
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_HOR_INIT_VAL_MSK				0x0000FC00
//----------------------------------------------------------------------------
#define	 TV_OUT_ATTR_OUT_HPIX_NUM_MSK			0x000003FF
//----------------------------------------------------------------------------



#endif

