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
 * @file    gp_tv.h
 * @brief   Declaration of PPU base driver.
 * @author  Cater Chen
 * @since   2010-10-27
 * @date    2010-10-27
 */
 
#ifndef _GP_TV_H_
#define _GP_TV_H_
/**************************************************************************
 *                         H E A D E R   F I L E S                        *
 **************************************************************************/

/**************************************************************************
*                           C O N S T A N T S                             *
 **************************************************************************/
// PPU Hardware Module 
#define GP_TV1_DISABLE                        0
#define GP_TV1_ENABLE                         1
#define GP_TV1_HARDWARE_MODULE                GP_TV1_ENABLE  

//  ypbpr mode
#define TV_480I                         0
#define TV_720P                         1

//  nTvStd
#define TVSTD_NTSC_M                    0
#define TVSTD_NTSC_J                    1
#define TVSTD_NTSC_N                    2
#define TVSTD_PAL_M                     3
#define TVSTD_PAL_B                     4
#define TVSTD_PAL_N                     5
#define TVSTD_PAL_NC                    6
#define TVSTD_NTSC_J_NONINTL            7
#define TVSTD_PAL_B_NONINTL             8
//  nResolution
#define TV_QVGA                         0
#define TV_HVGA                         1
#define TV_D1                           2
#define TV_HVGA_576                     3
#define TV_D1_576                       4
//  nNonInterlace
#define TV_INTERLACE                    0
#define TV_NON_INTERLACE                1

//  GP_TV1_IRQEN
#define	B_TV1_IRQ_EN				            11
#define TV1_IRQ_START                   (((unsigned int) 1)<<B_TV1_IRQ_EN)                  

/*
 * ioctl calls that are permitted to the /dev/tv interface, if
 * any of the ppu moudle are enabled.
 */
#define TV_INIT	                      _IOW('D', 0xFF, unsigned int)			  
#define TV_ENABLE_START	              _IOW('D', 0xFE, unsigned int)			  
#define TV_SET_BUFFER	                _IOW('D', 0xFD, unsigned int)			 
#define TV_SET_COLOR	                _IOW('D', 0xFC, unsigned int)
#define TV_SET_REVERSE	              _IOW('D', 0xFB, unsigned int)			 
#define TV_BUFFER_STATE	              _IOW('D', 0xFA, unsigned int)	
#define TV_SET_YPBPR	                _IOW('D', 0xF9, unsigned int)	

/*******************************************************************************
*                          D A T A    T Y P E S
*******************************************************************************/

typedef struct {
	unsigned short	TV1_type_mode;                                
	unsigned short	TV1_resolution_mode;                          
	unsigned short	TV1_noninterlace_mode;
	unsigned short	TV1_buffer_color_mode;                                
	unsigned int  	TV1_display_buffer_ptr; 	             
} TV1_MOUDLE_STRUCT;

typedef enum
{
		BUFFER_COLOR_FORMAT_RGB565=0x10,
		BUFFER_COLOR_FORMAT_BGRG,
		BUFFER_COLOR_FORMAT_GBGR,
		BUFFER_COLOR_FORMAT_RGBG,
		BUFFER_COLOR_FORMAT_GRGB,				
		BUFFER_COLOR_FORMAT_VYUY,
    BUFFER_COLOR_FORMAT_YVYU,	    
 		BUFFER_COLOR_FORMAT_UYVY,
    BUFFER_COLOR_FORMAT_YUYV,
    BUFFER_COLOR_FORMAT_RGBA	   		
} BUFFER_COLOR_FORMAT;

/*******************************************************************************
*               F U N C T I O N    D E C L A R A T I O N S
*******************************************************************************/

/******************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 *****************************************************************************/
/**
* @brief	       gp tv1 initial
* @param 	none
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_tv1_init(void);

/**
* @brief	       gp tv1 set display buffer
* @param 	buffer_ptr[in]:TV display buffer set.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_tv1_set_buffer(unsigned int buffer_ptr);

/**
* @brief	       gp tv1 set display color
* @param 	buffer_color_mode[in]:TV display buffer color type set.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_tv1_set_color(BUFFER_COLOR_FORMAT buffer_color_mode);

/**
* @brief	       gp tv1 start
* @param 	nTvStd[in]:TV display type set.0:TVSTD_NTSC_M, 1:TVSTD_NTSC_J, 2:TVSTD_NTSC_N, 3:TVSTD_PAL_M, 4:TVSTD_PAL_B, 5:TVSTD_PAL_N, 6:TVSTD_PAL_NC, 7:TVSTD_NTSC_J_NONINTL, 8:TVSTD_PAL_B_NONINTL 
* @param 	nTvStd[in]:TV display resolution set.0:TV_QVGA, 1:TV_HVGA, 2:TV_D1
* @param 	nTvStd[in]:TV display non-interlace set.0:TV_INTERLACE, 1:TV_NON_INTERLACE
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_tv1_start(signed int nTvStd, signed int nResolution, signed int nNonInterlace);

/**
* @brief	  gp tv1 display to reverse
* @param 	  mode[in]:0:disable,1:enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_tv1_set_reverse(unsigned int mode);

/**
* @brief	  gp tv1 ypbpr set
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_tv1_set_ypbpr(unsigned int mode,unsigned int enable);

#endif /* _GP_TV_H_ */