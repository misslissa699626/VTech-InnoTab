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
 * @file    gp_codec.h
 * @brief   Declaration of video codec.
 * @author  zhoulu
 * @since   2010/10/13
 * @date    2010/10/13
 */
#ifndef _GP_VCODEC_H_
#define _GP_VCODEC_H_

#include "gp_avcodec.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define MCP_CODEC_FLAG_SW           0
#define MCP_CODEC_FLAG_CEVA         1
#define MCP_CODEC_FLAG_OTHER        2


/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct video_codec_s {
	UINT32 format;                         /*!< @brief codec format */
	UINT32 flag;                       /*!< @brief codec speciality flag */
	UINT8 fcc[4];                      /*!< @brief codec four cc */
	SINT32 (*instance_size)(void);     /*!< @brief codec need work buffer size */
	SINT32 (*init)(void *cdp);         /*!< @brief codec initialize */
	SINT32 (*codec)(void *cdp);        /*!< @brief codec execute code/decode */
	SINT32 (*uninit)(void *cdp);       /*!< @brief codec free */
	SINT8 *description;                    /*!< @brief codec four cc */
} video_codec_t;



/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

video_codec_t *video_codec_load(UINT32 format);
void video_codec_unload(video_codec_t *vcodec);

#endif /* _GP_VCODEC_H_ */

