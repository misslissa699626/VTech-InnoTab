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
 * @file    2d.H
 * @brief   Declare 2d api wrap for device driver.
 * @author  clhuang
 * @since   2010-12-03
 * @date    2010-12-03
 */
#ifndef _2D_H_
#define _2D_H_

#include <stdint.h>
#include <mach/typedef.h>
#include <mach/gp_2d.h>
#include <mach/gp_scale.h>

#define NONE_TRASPARENT 0XFF000000
/**
* @brief wrap 2d scale ioctrl api.
* @param pdstBitmap [out] : destination bitmap data
* @param dstRect [in] : destination scale rectangle
* @param psrcBitmap [in] : source bitmap
* @param srcRect [in] : source scale rectangle
* @return : SP_OK(0)/SP_FAIL
*/
SINT32 gp2dScale(spBitmap_t *pdstBitmap, spRect_t dstRect, spBitmap_t *psrcBitmap, spRect_t srcRect);

/**
* @brief wrap 2d blending ioctrl api.
* @param pdstBitmap [in/out] : destination bitmap data
* @param psrcBitmap [in] : source bitmap
* @param opRect [in] : operation rectangle
* @param alphaFmt [in] : alpha format
* @param dstAlpha [in] : dest alpha value
* @param srcAlpha [in] : source alpha value
* @return : SP_OK(0)/SP_FAIL
*/
SINT32 gp2dBlend(spBitmap_t *pdstBitmap, spBitmap_t *psrcBitmap, 
    spRect_t opRect, UINT32 alphaFmt, UINT32 dstAlpha, UINT32 srcAlpha);

/**
* @brief wrap 2d scale,transparent, rotate,flip  ioctrl api.
* @param pdstBitmap [out] : destination bitmap data
* @param dstRect [in] : destination scale rectangle
* @param psrcBitmap [in] : source bitmap
* @param srcRect [in] : source scale rectangle
* @param loKeyColor[in] : the low color of the range of  srcBitmap transparent color,
*                                  make by THE MACRO MAKE_RGB
* @param hiKeyColor[in] : the high color of the range of  srcBitmap transparent color,
*                                  make by THE MACRO MAKE_RGB
* @param rotate[in] : the value of rotation,only use G2D_ROTATE_0,G2D_ROTATE_90,
*                            G2D_ROTATE_180,G2D_ROTATE_270
* @param flip[in] : the value of flip,only use 0 and 1 
* @return : SP_OK(0)/SP_FAIL
*/
SINT32 gp2dBitblt(spBitmap_t *pdstBitmap, spRect_t dstRect, spBitmap_t *psrcBitmap,  spRect_t srcRect,
    SINT32 loKeyColor, SINT32 hiKeyColor, SINT32 rotate, SINT32 flip);

/**
* @brief set scalar dithering param or disable.
* @param dither [in] : pointer to dither param
* @return : SP_OK(0)/SP_FAIL
*/
SINT32 gp2dSetDither(scale_dither_t* dither);

#endif /* _2D_H_ */

