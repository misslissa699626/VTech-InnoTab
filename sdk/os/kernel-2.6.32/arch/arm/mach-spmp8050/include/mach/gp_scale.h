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
 * @file    gp_scale.h
 * @brief   Declaration of scale driver.
 * @author  qinjian
 * @since   2010/10/9
 * @date    2010/10/9
 */
#ifndef _GP_SCALE_H_
#define _GP_SCALE_H_

#include <mach/typedef.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/* ioctl for device node definition */
#define SCALE_IOCTL_ID          'S'
#define SCALE_IOCTL_TRIGGER     _IOW(SCALE_IOCTL_ID, 0, scale_content_t)
#define SCALE_IOCTL_DITHER      _IOW(SCALE_IOCTL_ID, 1, scale_dither_t)

/* scale dither mode */
enum {
	SCALE_DITHER_MODE_NONE = 0,
	SCALE_DITHER_MODE_ORDER,    /* Ordered Dithering */
	SCALE_DITHER_MODE_HERRDIFF  /* Horizontal Error Diffusion */
};

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct sclae_dither_s {
	unsigned int mode;          /*!< @brief dither mode: SCALE_DITHER_MODE_XXX */
	unsigned int sequence;      /*!< @brief dither sequence number */
	unsigned int map_upper;     /*!< @brief upper part of dither map */
	unsigned int map_lower;     /*!< @brief lower part of dither map */
} scale_dither_t;

typedef struct scale_content_s {
	gp_bitmap_t  src_img;       /*!< @brief source image bitmap */
	gp_rect_t    clip_rgn;      /*!< @brief source clip region */
	gp_bitmap_t  dst_img;       /*!< @brief destination image bitmap */
	gp_rect_t    scale_rgn;     /*!< @brief destination scale region */
	unsigned int timeout;       /*!< @brief scaling timeout (ms), 0 for default timeout value (3000ms)*/
} scale_content_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

#endif /* _GP_SCALE_H_ */
