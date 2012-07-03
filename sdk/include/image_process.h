/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2007 by Sunplus mMedia Inc.                      *
 *                                                                        *
 *  This software is copyrighted by and is the property of Sunplus        *
 *  mMedia Inc. All rights are reserved by Sunplus mMedia Inc.            *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Sunplus Technology Co., Ltd.                       *
 *                                                                        *
 *  Sunplus mMedia Inc. reserves the right to modify this software        *
 *  without notice.                                                       *
 *                                                                        *
 *  Sunplus mMedia Inc.                                                   *
 *  19, Innovation First Road, Science-Based Industrial Park,             *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 *                                                                        *
 **************************************************************************/
/**
 * @file image_process.h
 * @brief Process images using hardware
 * @author Billy Shieh
 * @since 2008-01-10
 * @date 2008-01-10
 */

#ifndef _IMAGE_PROCESS_H_
#define _IMAGE_PROCESS_H_

#include "typedef.h"
//#include "spgui.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct spImageProcess_s spImageProcess_t;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
/**
 *
 * @brief Initial image process.
 * @retval SP_OK if success. Otherwise fail.
 *
 * This function creates a new image reader.
 *
 */
spImageProcess_t* imageProcessCreate(void);

SINT32 imageProcessDestroy(spImageProcess_t* pImp);

SINT32 imageProcessAddFillRectOp(spImageProcess_t* pImp, spBitmap_t *pDst, spRect_t dstRoi, spRgbQuad_t color);

SINT32 imageProcessAddBitBltOp(spImageProcess_t* pImp, spBitmap_t *pDst, spRect_t dstRoi, spBitmap_t *pSrc, spRect_t srcRoi, spRgbQuad_t colorkey);

UINT8 transformAlpha(UINT8 alpha);/* transform spgui alpha for 2D driver, range 0~255*/

SINT32 imageProcessAddBlendingOp(spImageProcess_t* pImp, spBitmap_t *pDst, spPoint_t dstPos, spBitmap_t *pSrc1, spRect_t srcRoi1, UINT8 alpha1, spBitmap_t *pSrc2, spRect_t srcRoi2, UINT8 alpha2);

SINT32 imageProcessAdd1bppConvertOp(spImageProcess_t* pImp, spBitmap_t *pDst, spPoint_t dstPos, spBitmap_t *pSrc, spRect_t srcRoi);

SINT32 imageProcessAddYOffsetOp(spImageProcess_t* pImp, spBitmap_t *pDst,  spRect_t dstRoi, spBitmap_t *pSrc, spRect_t srcRoi,UINT8 yoffset);
SINT32 imageProcessAddOsdEncodeOp(spImageProcess_t* pImp, spBitmap_t *pDst, spBitmap_t *pSrc);
SINT32 imageProcessAddIPMOp(spImageProcess_t* pImp, spBitmap_t *pDst, spRect_t dstRoi, spPoint_t *pDstPnt, spBitmap_t *pSrc, spRect_t srcRoi, spRgbQuad_t *pDefaultColor);
SINT32 imageProcessFlush(spImageProcess_t* pImp);


#endif

