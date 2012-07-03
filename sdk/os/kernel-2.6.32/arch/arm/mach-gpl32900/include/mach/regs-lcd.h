/*
 * arch/arm/mach-spmp8000/include/mach/regs-usbhost.h
 *
 * Copyright (C) 2007 Sunplus MultiMedia
 *
 * USBHOST - System peripherals regsters.
 *
 */

#define LCDC_BASE			IO3_ADDRESS(0x000000)

//For LCD display freq setup
//#define     DEV_SCUALCDCTRL         ((volatile UINT32 *)(MMP_SCUA_BASE+0x10))
//#define     DEV_SCUALCDRATIO        ((volatile UINT32 *)(MMP_SCUA_BASE+0x80))
#define     DEV_LCDC_TFT_CTRL       ((volatile UINT32 *)(LCDC_BASE+0x0000))
#define     DEV_LCDC_DATA_FMT       ((volatile UINT32 *)(LCDC_BASE+0x0004))

#define     DEV_LCDC_HOR_OUTPIX     ((volatile UINT32 *)(LCDC_BASE+0x0008))
#define     DEV_LCDC_HOR_FBLK       ((volatile UINT32 *)(LCDC_BASE+0x000C))
#define     DEV_LCDC_HOR_BBLK       ((volatile UINT32 *)(LCDC_BASE+0x0010))
#define     DEV_LCDC_HOR_SYNCW      ((volatile UINT32 *)(LCDC_BASE+0x0014))

#define     DEV_LCDC_VER_OUTPIX     ((volatile UINT32 *)(LCDC_BASE+0x0018))
#define     DEV_LCDC_VER_FBLK       ((volatile UINT32 *)(LCDC_BASE+0x001C))
#define     DEV_LCDC_VER_BBLK       ((volatile UINT32 *)(LCDC_BASE+0x0020))
#define     DEV_LCDC_VER_SYNCW      ((volatile UINT32 *)(LCDC_BASE+0x0024))

#define     DEV_LCDC_FB_ATTRIB      ((volatile UINT32 *)(LCDC_BASE+0x0028))
#define     DEV_LCDC_STR_LNO        ((volatile UINT32 *)(LCDC_BASE+0x002C))
#define     DEV_LCDC_STR_PNO        ((volatile UINT32 *)(LCDC_BASE+0x0030))
#define     DEV_LCDC_SRC_HPIX       ((volatile UINT32 *)(LCDC_BASE+0x0034))
#define     DEV_LCDC_DUMY_PIX       ((volatile UINT32 *)(LCDC_BASE+0x0038))
#define     DEV_LCDC_TFT_ST         ((volatile UINT32 *)(LCDC_BASE+0x003C))
#define     DEV_LCDC_DATA_SEQ       ((volatile UINT32 *)(LCDC_BASE+0x0040))
#define     DEV_LCDC_SRC_VPIX       ((volatile UINT32 *)(LCDC_BASE+0x0044))

#define     DEV_LCDC_TFT_INT        ((volatile UINT32 *)(LCDC_BASE+0x0050))
#define     DEV_LCDC_TFT_ADDR       ((volatile UINT32 *)(LCDC_BASE+0x0054))

#define     DEV_LCDC_BLK_LEFT       ((volatile UINT32 *)(LCDC_BASE+0x0058))
#define     DEV_LCDC_BLK_RIGHT      ((volatile UINT32 *)(LCDC_BASE+0x005C))
#define     DEV_LCDC_BLK_TOP        ((volatile UINT32 *)(LCDC_BASE+0x0060))
#define     DEV_LCDC_BLK_BOTTOM     ((volatile UINT32 *)(LCDC_BASE+0x0064))
#define     DEV_LCDC_BLK_PATTERN    ((volatile UINT32 *)(LCDC_BASE+0x0068))

#define     DEV_LCDC_AHB_CTRL       ((volatile UINT32 *)(LCDC_BASE+0x006C))
#define     DEV_LCDC_HOR_SCALE      ((volatile UINT32 *)(LCDC_BASE+0x0070))
#define     DEV_LCDC_VER_SCALE      ((volatile UINT32 *)(LCDC_BASE+0x0074))
#define     DEV_LCDC_UPDATE_REG     ((volatile UINT32 *)(LCDC_BASE+0x0078))
#define     DEV_LCDC_FILL_COLOR     ((volatile UINT32 *)(LCDC_BASE+0x007C))

#define     DEV_LCDC_OSD_FB_ATTRIB  ((volatile UINT32 *)(LCDC_BASE+0x0080))
#define     DEV_LCDC_OSD_BASE_ADDR  ((volatile UINT32 *)(LCDC_BASE+0x0084))
#define     DEV_LCDC_OSD_SRC_HPIX   ((volatile UINT32 *)(LCDC_BASE+0x0088))
#define     DEV_LCDC_OSD_SRC_VPIX   ((volatile UINT32 *)(LCDC_BASE+0x008C))
#define     DEV_LCDC_OSD_STR_LNO    ((volatile UINT32 *)(LCDC_BASE+0x0090))
#define     DEV_LCDC_OSD_STR_PNO    ((volatile UINT32 *)(LCDC_BASE+0x0094))
#define     DEV_LCDC_OSD_DMY_PIX    ((volatile UINT32 *)(LCDC_BASE+0x0098))
#define     DEV_LCDC_OSD_ST         ((volatile UINT32 *)(LCDC_BASE+0x009C))
#define     DEV_LCDC_OSD_OUT_LNO    ((volatile UINT32 *)(LCDC_BASE+0x00A0))
#define     DEV_LCDC_OSD_OUT_PNO    ((volatile UINT32 *)(LCDC_BASE+0x00A4))
#define     DEV_LCDC_OSD_M_FACTOR   ((volatile UINT32 *)(LCDC_BASE+0x00A8))
#define     DEV_LCDC_OSD_TBL_IDX_B  ((volatile UINT32 *)(LCDC_BASE+0x0400))
#define     DEV_LCDC_OSD_TBL_IDX_E  ((volatile UINT32 *)(LCDC_BASE+0x07FC))

#define P_TFT_CTRL     0x0000 
#define P_DATA_FMT     0x0004

#define P_HOR_OUTPIX   0x0008
#define P_HOR_FBLK     0x000C
#define P_HOR_BBLK     0x0010
#define P_HOR_SYNCW    0x0014

#define P_VER_OUTPIX   0x0018
#define P_VER_FBLK     0x001C
#define P_VER_BBLK     0x0020
#define P_VER_SYNCW    0x0024

#define P_FB_ATTRIB    0x0028
#define P_STR_LNO      0x002C
#define P_STR_PNO      0x0030
#define P_SRC_HPIX     0x0034
#define P_DUMY_PIX     0x0038
#define P_TFT_ST       0x003C
#define P_DATA_SEQ     0x0040
#define P_SRC_VPIX     0x0044

#define P_TFT_INT      0x0050
#define P_TFT_ADDR     0x0054

#define P_BLK_LEFT     0x0058
#define P_BLK_RIGHT    0x005C
#define P_BLK_TOP      0x0060
#define P_BLK_BOTTOM   0x0064
#define P_BLK_PATTERN  0x0068

#define P_AHB_CTRL     0x006C
#define P_HOR_SCALE    0x0070
#define P_VER_SCALE    0x0074
#define P_UPDATE       0x0078
#define P_FILL_COLOR   0x007C

#define P_OSD_FB_ATTRIB 0x0080
#define P_OSD_BASE_ADDR 0x0084
#define P_OSD_SRC_HPIX  0x0088
#define P_OSD_SRC_VPIX  0x008C
#define P_OSD_STR_LNO   0x0090
#define P_OSD_STR_PNO   0x0094
#define P_OSD_DMY_PIX   0x0098
#define P_OSD_ST        0x009C
#define P_OSD_OUT_LNO   0x00A0
#define P_OSD_OUT_PNO   0x00A4
#define P_OSD_M_FACTOR  0x00A8
#define P_OSD_TBL_IDX_B 0x0400
#define P_OSD_TBL_IDX_E 0x07FC



// P_TFT_CTRL 
#define LCD_CTRL_TFT_DISP_EN            0x01000000
#define LCD_CTRL_OSD_DISP_EN            0x00100000
#define LCD_CTRL_ON                     0x00010000
#define LCD_CTRL_OFF                    0x00000000
#define LCD_CTRL_VSCALE_EN              0x00000200
#define LCD_CTRL_HSCALE_EN              0x00000100
#define LCD_CTRL_SCALE_MSK              0x00000300
#define LCD_CTRL_1KB_BOUNDARY_CROSS_EN  0x00000010
#define LCD_CTRL_CLK_SEL_SYS            0x00000000
#define LCD_CTRL_CLK_SEL_D2             0x00000001   //clk /2 
#define LCD_CTRL_CLK_SEL_D4             0x00000002   //clk /4 
#define LCD_CTRL_CLK_SEL_D8             0x00000003   //clk /8 
#define LCD_CTRL_CLK_SEL_MSK            0x00000003

// P_DATA_FMT
#define LCD_DATA_FMT_CCIR656_M          0x00000100
#define LCD_DATA_FMT_PARALLEL_RGB       0x00000000
#define LCD_DATA_FMT_SERIAL_RGB         0x00000001
#define LCD_DATA_FMT_SERIAL_RGBDm       0x00000002
#define LCD_DATA_FMT_CCIR601            0x00000003
#define LCD_DATA_FMT_CCIR656            0x00000004
#define LCD_DATA_FMT_RGB24B             0x00000005

// P_FB_ATTRIB
#define LCD_FB_ATTRIB_YCbCr_YCbCr       0x00000000
#define LCD_FB_ATTRIB_YCbCr_YUV         0x00010000
#define LCD_FB_ATTRIB_BIG_ENDIAN        0x00000010
#define LCD_FB_ATTRIB_LITTLE_ENDIAN     0x00000000
#define LCD_FB_ATTRIB_FMT_RGB565        0x00000000        ///// little endian
#define LCD_FB_ATTRIB_FMT_RGB1555       0x00000001        ///// big endian
#define LCD_FB_ATTRIB_FMT_YUYV          0x00000002        ///// little endian
#define LCD_FB_ATTRIB_FMT_4Y4U4Y4V      0x00000003        ///// little endian
#define LCD_FB_ATTRIB_FMT_RGB888        0x00000004

//P_TFT_ST
#define LCD_TFT_ST_BUF_OVERFLOW         0x01000000        ///// TFT LCD buffer underflow
#define LCD_TFT_ST_BUF_UNDERFLOW        0x02000000        ///// TFT LCD buffer underflow
#define LCD_TFT_ST_FINISH               0x00010000        ///// TFT controller complete internal operation
#define LCD_TFT_ST_CLEAR                0x00000001        ///// TFT controller complete internal operation

//P_TFT_INT
#define LCD_TFT_INT_ENABLE              0x01000000        
#define LCD_TFT_INT_VBLK_INT            0x00010000        

//P_DATA_SEQ
#define LCD_ATTR_DATA_SEQ_YCbCr         0x00000000
#define LCD_ATTR_DATA_SEQ_YUV           0x01000000
#define LCD_ATTR_DATA_SEQ_Y0U0Y1V0      0x00000000
#define LCD_ATTR_DATA_SEQ_Y0V0Y1U0      0x00010000
#define LCD_ATTR_DATA_SEQ_U0Y0V0Y1      0x00020000
#define LCD_ATTR_DATA_SEQ_V0Y0U0Y1      0x00030000
#define LCD_ATTR_DATA_SEQ_Y1U0Y0V0      0x00040000
#define LCD_ATTR_DATA_SEQ_Y1V0Y0U0      0x00050000
#define LCD_ATTR_DATA_SEQ_U0Y1V0Y0      0x00060000
#define LCD_ATTR_DATA_SEQ_V0Y1U0Y0      0x00070000

#define LCD_ATTR_DATA_SEQ_RGB_OL_RGB    0x00000000
#define LCD_ATTR_DATA_SEQ_RGB_OL_GBR    0x00000100
#define LCD_ATTR_DATA_SEQ_RGB_OL_BRG    0x00000200
#define LCD_ATTR_DATA_SEQ_RGB_OL_RBG    0x00000300
#define LCD_ATTR_DATA_SEQ_RGB_OL_BGR    0x00000400
#define LCD_ATTR_DATA_SEQ_RGB_OL_GRB    0x00000500
                                            
#define LCD_ATTR_DATA_SEQ_RGB_EL_RGB    0x00000000
#define LCD_ATTR_DATA_SEQ_RGB_EL_GBR    0x00000001
#define LCD_ATTR_DATA_SEQ_RGB_EL_BRG    0x00000002
#define LCD_ATTR_DATA_SEQ_RGB_EL_RBG    0x00000003
#define LCD_ATTR_DATA_SEQ_RGB_EL_BGR    0x00000004
#define LCD_ATTR_DATA_SEQ_RGB_EL_GRB    0x00000005

//P_AHB_CTRL
#define     LCD_AHB_CTRL_BURST_MSK          0x00000001
#define     LCD_AHB_CTRL_BURST_16           0x00000000
#define     LCD_AHB_CTRL_BURST_4            0x00000001

//P_HOR_SCALE
#define     LCD_HOR_SCALE_INIT_MSK          0xFFFF0000
#define     LCD_HOR_SCALE_FACTOR_MSK        0x0000FFFF

//P_VER_SCALE
#define     LCD_VER_SCALE_INIT_MSK          0xFFFF0000
#define     LCD_VER_SCALE_FACTOR_MSK        0x0000FFFF

//P_UPDATE
#define     LCD_ATTR_UPDATE_REG_FLAG        0x00000001

//O_FB_ATTRIB
#define     LCD_OSD_FB_ATTR_OFFSET_MSK      0x0000FF00
#define     LCD_OSD_FB_ATTR_ENDIAN_MSK      0x00000010
#define     LCD_OSD_FB_ATTR_BIG_ENDIAN      0x00000010
#define     LCD_OSD_FB_ATTR_LITTLE_ENDIAN   0x00000000
#define     LCD_OSD_FB_ATTR_FB_FMT_MSK      0x00000003
#define     LCD_OSD_FB_ATTR_FB_FMT_RGB565   0x00000000
#define     LCD_OSD_FB_ATTR_FB_FMT_RGB1555  0x00000001
#define     LCD_OSD_FB_ATTR_FB_FMT_8B_IDX   0x00000002
#define     LCD_OSD_FB_ATTR_FB_FMT_4B_IDX   0x00000003

//O_ST
#define     LCD_OSD_ST_BUFF_OVERFLOW_MSK    0x02000000
#define     LCD_OSD_ST_BUFF_OVERFLOW        0x02000000
#define     LCD_OSD_ST_BUFF_UNDERFLOW_MSK   0x01000000
#define     LCD_OSD_ST_BUFF_UNDERFLOW       0x01000000

//O_M_FACTOR
#define     LCD_OSD_M_FACTOR_MSK_MSK        0x00000100
#define     LCD_OSD_M_FACTOR_MSK_EN         0x00000100
#define     LCD_OSD_M_FACTOR_MSK_MU         0x00000000
#define     LCD_OSD_M_FACTOR_MIX_FACTOR_MSK 0x0000007F

