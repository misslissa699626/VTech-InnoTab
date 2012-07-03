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
 * @file    hal_scale.h
 * @brief   Declaration of SPMP8050 Scale HAL API
 * @author  qinjian
 * @since   2010/10/9
 * @date    2010/10/9
 */
#ifndef _HAL_SCALE_H_
#define _HAL_SCALE_H_

#include <mach/hal/hal_common.h>
#include <mach/gp_scale.h>

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
 * @brief   Scale hardware dither setting
 * @param   mode [in] dither mode (SCALE_DITHER_MODE_xxx)
 * @param   seqNum [in] dither sequence number
 * @param   upperPart [in] upper part of dither map
 * @param   lowerPart [in] lower part of dither map
 * @return  SP_OK(0)/SP_FAIL
 * @see
 */
UINT32 gpHalScaleSetDither(UINT32 mode, UINT32 seqNum,
						   UINT32 upperPart, UINT32 lowerPart);

/**
 * @brief   Scale hardware scaling execute
 * @param   srcImg [in] source image bitmap
 * @param   clipRgn [in] source clip region
 * @param   dstImg [in] destination image bitmap
 * @param   scaleRgn [in] destination scale region
 * @return  SP_OK(0)/SP_FAIL
 * @see
 */
UINT32 gpHalScaleExec(gp_bitmap_t *srcImg, gp_rect_t *clipRgn,
					  gp_bitmap_t *dstImg, gp_rect_t *scaleRgn);

/**
 * @brief   Scale hardware interrupt enable/disable
 * @param   enable [in] diable(0)/enable(1)
 * @return  None
 * @see
 */
void gpHalScaleEnableIrq(UINT32 enable);

/**
 * @brief   Scale hardware clear scaling done flag
 * @return  None
 * @see
 */
void gpHalScaleClearDone(void);

/**
 * @brief   Scale hardware check scaling finish
 * @return  not finish(0)/finished(1)
 * @see
 */
UINT32 gpHalScaleDone(void);

/**
 * @brief   Scale clock enable/disable function
 * @param   enable[in]: disable(0)/enable(1)
 * @return  None
 * @see
 */
void gpHalScaleClkEnable(UINT32 enable);

/**
 * @brief   Scale hardware reset function
 */
void gpHalScaleReset(void);

/**
 * @brief   Scale hardware register dump function
 */
void gpHalScaleRegDump(void);

#endif /* _HAL_SCALE_H_ */
