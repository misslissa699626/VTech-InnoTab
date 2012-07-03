/*
 * arch/arm/mach-spmp8000/include/mach/regs-scu.h
 *
 * Copyright (C) 2007 Sunplus MultiMedia
 *
 * Syetem Unit Control Unit
 *
 */

#define     MMP_SCALE_ENGINE_BASE			IO2_ADDRESS(0x7000) 

//----------------------------------------------------------------------------
#define 	  DEV_SCALE_ENG_CTRL              ((volatile unsigned int *)(MMP_SCALE_ENGINE_BASE+0x0000))
#define     DEV_SCALE_ENG_FORMAT            ((volatile unsigned int *)(MMP_SCALE_ENGINE_BASE+0x0004))
#define     DEV_SCALE_ENG_SRC_HPIX          ((volatile unsigned int *)(MMP_SCALE_ENGINE_BASE+0x0008))
#define     DEV_SCALE_ENG_SRC_VPIX          ((volatile unsigned int *)(MMP_SCALE_ENGINE_BASE+0x000C))
#define     DEV_SCALE_ENG_HOR_OUTPIX        ((volatile unsigned int *)(MMP_SCALE_ENGINE_BASE+0x0010))
#define     DEV_SCALE_ENG_VER_OUTPIX        ((volatile unsigned int *)(MMP_SCALE_ENGINE_BASE+0x0014))
#define     DEV_SCALE_ENG_DATA_BYTE         ((volatile unsigned int *)(MMP_SCALE_ENGINE_BASE+0x0018))
#define     DEV_SCALE_ENG_CLIP_BYTE_Y       ((volatile unsigned int *)(MMP_SCALE_ENGINE_BASE+0x001C))
#define     DEV_SCALE_ENG_CLIP_BYTE_UV      ((volatile unsigned int *)(MMP_SCALE_ENGINE_BASE+0x0020))
#define     DEV_SCALE_ENG_BASE_RD_ADDR      ((volatile unsigned int *)(MMP_SCALE_ENGINE_BASE+0x0024))
#define     DEV_SCALE_ENG_BASE_WR_ADDR      ((volatile unsigned int *)(MMP_SCALE_ENGINE_BASE+0x0028))
#define     DEV_SCALE_ENG_HOR_SCALE         ((volatile unsigned int *)(MMP_SCALE_ENGINE_BASE+0x002C))
#define     DEV_SCALE_ENG_VER_SCALE         ((volatile unsigned int *)(MMP_SCALE_ENGINE_BASE+0x0030))
#define     DEV_SCALE_ENG_HOR_INI           ((volatile unsigned int *)(MMP_SCALE_ENGINE_BASE+0x0034))
#define     DEV_SCALE_ENG_VER_INI           ((volatile unsigned int *)(MMP_SCALE_ENGINE_BASE+0x0038))
#define     DEV_SCALE_ENG_BASE_RD_ADDR_U    ((volatile unsigned int *)(MMP_SCALE_ENGINE_BASE+0x003C))
#define     DEV_SCALE_ENG_BASE_RD_ADDR_V    ((volatile unsigned int *)(MMP_SCALE_ENGINE_BASE+0x0040))
#define     DEV_SCALE_ENG_VERSION           ((volatile unsigned int *)(MMP_SCALE_ENGINE_BASE+0x0044))

//----------------------------------------------------------------------------

#define 	SCALE_ENG_CTRL_OFST                       0x0000
#define   SCALE_ENG_FORMAT_OFST                  0x0004
#define   SCALE_ENG_SRC_HPIX_OFST               0x0008
#define   SCALE_ENG_SRC_VPIX_OFST               0x000C 
#define   SCALE_ENG_HOR_OUTPIX_OFST          0x0010  
#define   SCALE_ENG_VER_OUTPIX_OFST           0x0014
#define   SCALE_ENG_DATA_BYTE_OFST      0x0018
#define   SCALE_ENG_CLIP_BYTE_Y_OFST    0x001C
#define   SCALE_ENG_CLIP_BYTE_UV_OFST   0x0020
#define   SCALE_ENG_BASE_RD_ADDR_OFST  0x0024
#define   SCALE_ENG_BASE_WR_ADDR_OFST 0x0028
#define   SCALE_ENG_HOR_SCALE_OFST         0x002C
#define   SCALE_ENG_VER_SCALE_OFST        0x0030
#define   SCALE_ENG_HOR_INI_OFST           0x0034
#define   SCALE_ENG_VER_INI_OFST           0x0038
#define   SCALE_ENG_BASE_RD_ADDR_U_OFST 0x003C
#define   SCALE_ENG_BASE_RD_ADDR_V_OFST 0x0040
#define   SCALE_ENG_VERSION_OFST    0x0044       
//----------------------------------------------------------------------------
#define     SCALE_CTRL_TRIGGER              0x00000001
#define     SCALE_CTRL_RESET                0x00000010
#define     SCALE_CTRL_ON                   0x00000100
#define     SCALE_CTRL_INT_CLEAR            0x00001000
//----------------------------------------------------------------------------
#define     SCALE_FMT_IN_MSK                0x00000107
#define     SCALE_FMT_IN_RGB565             0x00000000
#define     SCALE_FMT_IN_RGB1555            0x00000001
#define     SCALE_FMT_IN_RGB888             0x00000002
#define     SCALE_FMT_IN_YCbCr422           0x00000004
#define     SCALE_FMT_IN_4Y4Cb4Y4Cr         0x00000005
#define     SCALE_FMT_IN_YCbCr420           0x00000006
#define     SCALE_FMT_IN_YUV422             0x00000104
#define     SCALE_FMT_IN_4Y4U4Y4V           0x00000105
#define     SCALE_FMT_IN_YUV420             0x00000106

#define     SCALE_FMT_OUT_MSK               0x00001070
#define     SCALE_FMT_OUT_RGB565            0x00000000
#define     SCALE_FMT_OUT_RGB1555           0x00000010
#define     SCALE_FMT_OUT_RGB888            0x00000020
#define     SCALE_FMT_OUT_YCbCr422          0x00000040
#define     SCALE_FMT_OUT_4Y4Cb4Y4Cr        0x00000050
#define     SCALE_FMT_OUT_YCbCr420          0x00000060
#define     SCALE_FMT_OUT_YUV422            0x00001040
#define     SCALE_FMT_OUT_4Y4U4Y4V          0x00001050
#define     SCALE_FMT_OUT_YUV420            0x00001060

#define     SCALE_FMT_ENDIAN_BIG            0x00010000

#define     SCALE_FMT_64_BYTE_ALIGN         0x00000000
#define     SCALE_FMT_32_BYTE_ALIGN         0x00100000
#define     SCALE_FMT_BYTE_ALIGN_MSK        0x00100000
//----------------------------------------------------------------------------
#define     SCALE_SRC_HPIX_NUM_MSK          0x00001FFF
//----------------------------------------------------------------------------
#define     SCALE_SRC_VPIX_NUM_MSK          0x00001FFF
//----------------------------------------------------------------------------
#define     SCALE_HOR_OUTPIX_NUM_MSK        0x000007FF
//----------------------------------------------------------------------------
#define     SCALE_VER_OUTPIX_NUM_MSK        0x000007FF
//----------------------------------------------------------------------------
#define     SCALE_STR_LNO_MSK               0x000007FF
//----------------------------------------------------------------------------
#define     SCALE_STR_PNO_MSK               0x000007FF
//----------------------------------------------------------------------------
#define     SCALE_DMY_PIX_MSK               0x000007FF
//----------------------------------------------------------------------------
#define     SCALE_HOR_SCALE_FACTOR_MSK      0x0000FFFF

#define     SCALE_HOR_SCALE_FUNC_MSK        0x00110000
#define     SCALE_HOR_SCALE_FUNC_UP         0x00010000
#define     SCALE_HOR_SCALE_FUNC_DOWN       0x00100000
//----------------------------------------------------------------------------
#define     SCALE_VER_SCALE_FACTOR_MSK      0x0000FFFF

#define     SCALE_VER_SCALE_FUNC_MSK        0x00110000
#define     SCALE_VER_SCALE_FUNC_UP         0x00010000
#define     SCALE_VER_SCALE_FUNC_DOWN       0x00100000
//----------------------------------------------------------------------------
#define     SCALE_HOR_INI_MSK               0x0000FFFF
//----------------------------------------------------------------------------
#define     SCALE_VER_INI_MSK               0x0000FFFF
