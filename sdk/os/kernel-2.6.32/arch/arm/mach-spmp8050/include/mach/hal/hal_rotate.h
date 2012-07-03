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
 * @file    hal_rotate.h
 * @brief   Declaration of SPMP8050 Rotate HAL API.
 * @author  qinjian
 * @since   2010/10/14
 * @date    2010/10/14
 */
#ifndef _HAL_ROTATE_H_
#define _HAL_ROTATE_H_

#include <mach/hal/hal_common.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**
 * @brief   Rotate hardware rotating execute
 * @param   srcImg [in] source image bitmap
 * @param   clipRgn [in] source clip region
 * @param   dstImg [in] destination image bitmap
 * @param   dstPos [in] destination position
 * @param   opt [in] rotate option
 * @return  SP_OK(0)/SP_FAIL
 * @see
 */
UINT32 gpHalRotateExec(gp_bitmap_t *srcImg, gp_rect_t *clipRgn,
					  gp_bitmap_t *dstImg, gp_point_t *dstPos, UINT32 opt);

/**
 * @brief   Rotate hardware interrupt enable/disable
 * @param   enable [in] diable(0)/enable(1)
 * @return  None
 * @see
 */
void gpHalRotateEnableIrq(UINT32 enable);

/**
 * @brief   Rotate hardware clear rotating done flag
 * @return  None
 * @see
 */
void gpHalRotateClearDone(void);

/**
 * @brief   Rotate hardware check rotating finish
 * @return  not finish(0)/finished(1)
 * @see
 */
UINT32 gpHalRotateDone(void);

/**
 * @brief   Rotate clock enable/disable function
 * @param   enable[in]: disable(0)/enable(1)
 * @return  None
 * @see
 */
void gpHalRotateClkEnable(UINT32 enable);

#endif /* _HAL_ROTATE_H_ */
