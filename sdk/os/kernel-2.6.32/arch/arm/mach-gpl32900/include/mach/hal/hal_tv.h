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
/**
 * @file    hal_ppu.h
 * @brief   Implement of PPU HAL API header file.
 * @author  Cater Chen
 * @since   2010-10-27
 * @date    2010-10-27
 */

#ifndef HAL_TV_H
#define HAL_TV_H

/**************************************************************************
 *                         H E A D E R   F I L E S                        *
 **************************************************************************/
#include <mach/typedef.h>
#include <mach/gp_tv.h>
 
/**************************************************************************
*                           C O N S T A N T S                             *
 **************************************************************************/
//  P_TV_CTRL        -   BIT[6:5]        -   Resolution
#define B_RESOLUTION                    5
//  P_TV_CTRL        -   BIT[4]          -   NONINTL
#define B_NONINTL                       4
//  P_TV_CTRL        -   BIT[3:1]        -   TVSTD
#define B_TVSTD                         1
//  P_TV_CTRL        -   BIT[0]          -   TVEN
#define B_TVEN                          0
#define TV_DISABLE                      (((UINT32)0)<<B_TVEN)
#define TV_ENABLE                       (((UINT32)1)<<B_TVEN)

//  P_TV_CTRL21        -   BIT[17]          -   PAL576EN
#define B_REVERSEEN                     17
#define TV1_REVERSE_ENABLE              (((UINT32)1)<<B_REVERSEEN)

//  P_TV_CTRL2        -   BIT[1]          -   PAL576EN
#define B_PAL576EN                      1
#define TV1_PAL576_DISABLE              (((UINT32)0)<<B_PAL576EN)
#define TV1_PAL576_ENABLE               (((UINT32)1)<<B_PAL576EN)

//  P_TV_CTRL3
#define	B_RGBG_YUYV_TYPE			          20
#define	MASK_RGBG_YUYV_TYPE	            (((UINT32) 0x7)<<B_RGBG_YUYV_TYPE)
#define	TV1_BGRG_VYUY_TYPE	            (((UINT32) 0x0)<<B_RGBG_YUYV_TYPE)
#define	TV1_GBGR_YVYU_TYPE	            (((UINT32) 0x1)<<B_RGBG_YUYV_TYPE)
#define	TV1_RGBG_UYVY_TYPE	            (((UINT32) 0x2)<<B_RGBG_YUYV_TYPE)
#define	TV1_GRGB_YUYV_TYPE	            (((UINT32) 0x3)<<B_RGBG_YUYV_TYPE)
#define	B_TV1_D1_SET					          16
#define	MASK_TV1_D1_SET				          (((UINT32) 0x7)<<B_TV1_D1_SET)
#define	TV1_D1					                (((UINT32) 0x4)<<B_TV1_D1_SET)
#define	TV1_576					                (((UINT32) 0x1)<<B_TV1_D1_SET)
#define	B_RGBG_YUYV_SET				          10
#define	MASK_RGBG_YUYV_SET		          (((UINT32) 0x3)<<B_RGBG_YUYV_SET)
#define	TV1_RGBG_SET		                (((UINT32) 0x0)<<B_RGBG_YUYV_SET)
#define	TV1_YUYV_SET		                (((UINT32) 0x1)<<B_RGBG_YUYV_SET)
#define	TV1_RGBA_SET		                (((UINT32) 0x2)<<B_RGBG_YUYV_SET)
#define	B_TV_COLOR_FORMAT			          8
#define	TV1_RGB565						          (((UINT32) 0)<<B_TV_COLOR_FORMAT)
#define	TV1_RGBG_YUYV				            (((UINT32) 1)<<B_TV_COLOR_FORMAT)
#define	B_TV1_DISPLAY_EN		            7
#define	TV1_DISPLAY_EN				          (((UINT32) 1)<<B_TV1_DISPLAY_EN)
#define	B_TV1_VGA_EN				            4
#define	TV1_QVGA						            (((UINT32) 0)<<B_TV1_VGA_EN)
#define	TV1_VGA							            (((UINT32) 1)<<B_TV1_VGA_EN)

//  P_TV_CTRL4
#define	B_INTEL_TYPE			              31
#define	B_H_SIZE			                  16
#define TV_640                          (((UINT32) 640)<<B_H_SIZE)
#define TV_720                          (((UINT32) 720)<<B_H_SIZE)
#define TV_720P_H                       (((UINT32) 1280)<<B_H_SIZE)
#define TV_720P_V                       720
#define TV_576                          576


//  P_TV1_IRQEN
//#define	B_TV1_IRQ_EN				            11
#define	TV1_IRQ_EN						          (((UINT32) 1)<<B_TV1_IRQ_EN)

/**************************************************************************
*                          D A T A    T Y P E S
**************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define TV_BIT_VALUE_SET(__BIT_N__,__VALUE_SET__)  \
(((UINT32)(__VALUE_SET__))<<(__BIT_N__)) 
/*******************************************************************************
*               F U N C T I O N    D E C L A R A T I O N S
*******************************************************************************/

/******************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 *****************************************************************************/
/**
* @brief	       gp tv1 initial
* @param 	none
* @return 	none
*/
extern void gpHalTVinit(void);

/**
* @brief	gp tv1 start
* @param 	nTvStd[in]:tv type ,ntsc/pal.
* @param 	nResolution[in]:tv resolution ,qvga/vga/d1.
* @param 	nNonInterlace[in]:tv noninterlace ,0=disable,1=enable.
* @return 	none
*/
extern void gpHalTVstart(SINT32 nTvStd, SINT32 nResolution, SINT32 nNonInterlace);

/**
* @brief	 none ppu display color set
* @param 	 SHOW_TYPE[in]:display color type.
* @return  success=0,fail=-1.
*/
extern void gpHalPicDisplaycolor(BUFFER_COLOR_FORMAT COLOR_MODE);

/**
* @brief	  tv1 display buffer set
* @param 	  display_buffer[in]:display buffer.
* @return 	none.
*/
extern void gpHalTvFramebufferset(UINT32 display_buffer);

/**
* @brief	  tv1 display reverse set
* @param 	  enable[in]:0:disable, 1:enable.
* @return 	none.
*/
extern void gpHalTvFramebufferReverse(UINT32 enable);

/**
* @brief	  tv1 display isr
* @return 	isr_number.
*/
extern SINT32 gpHalTvIsr(void);

/**
* @brief	  tv1 display buffer state
* @return 	display buffer state, success=0,fail=-1.
*/
extern signed int gpHalTvFramebufferstate(void);

/**
* @brief	 none ppu display Y/Pb/Pr set
* @param 	 mode[in]:0:480i,1:720p.
* @param 	 enable[in]:0:disable, 1:enable.
* @return  none.
*/
extern void gpHalYbpbcrenable(UINT32 mode,UINT32 enable);

#endif
