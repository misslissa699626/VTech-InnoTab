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
 * @file    gp_rotate.h
 * @brief   Declaration of rotate driver.
 * @author  qinjian
 * @since   2010/10/15
 * @date    2010/10/15
 */
#ifndef _GP_ROTATE_H_
#define _GP_ROTATE_H_

#include <mach/typedef.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/* Ioctl for device node definition */
#define ROTATE_IOCTL_ID         'R'
#define ROTATE_IOCTL_TRIGGER    _IOW(ROTATE_IOCTL_ID, 0, rotate_content_t)

/* Rotation options */
#define ROTATE_OPT_NONE             0
#define ROTATE_OPT_DEGREE_0         0
#define ROTATE_OPT_DEGREE_90        1
#define ROTATE_OPT_DEGREE_180       2
#define ROTATE_OPT_DEGREE_270       3
#define ROTATE_OPT_MIRROR           4
#define ROTATE_OPT_FLIP             8

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct rotate_content_s {
	gp_bitmap_t src_img;        /*!< @brief source image bitmap */
	gp_rect_t   clip_rgn;       /*!< @brief source clip region, width & height must mutiple by 16 */
	gp_bitmap_t dst_img;        /*!< @brief destination image bitmap */
	gp_point_t  dst_pos;        /*!< @brief destination position (left,top) */
	unsigned int opt;           /*!< @brief rotate option (ex: ROTATE_OPT_DEGREE_90|ROTATE_OPT_FLIP) */
} rotate_content_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

#endif /* _GP_ROTATE_H_ */
