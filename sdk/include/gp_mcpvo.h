
/***************************************************************************
 * Name: gp_mcpvo.h
 *
 * Purpose:
 *
 * Developer:
 *     zhoulu, 2010-8-18
 *
 * Copyright (c) 2010-2011 by Sunplus mMobile Inc.
 ***************************************************************************/

#ifndef _GP_MCPVO_H_
#define _GP_MCPVO_H_

/***************************************************************************
 * Header Files
 ***************************************************************************/
#include "typedef.h"

/***************************************************************************
 * Constants
 ***************************************************************************/

/***************************************************************************
 * Macros
 ***************************************************************************/
/***************************************************************************
 * Data Types
 ***************************************************************************/
/** @brief MCPlayer video out type */
typedef enum mcpVoType_e {
	VO_TYPE_DISP0 = 0,
	VO_TYPE_TV0,
	VO_TYPE_TV1,
	VO_TYPE_DISP1,
	VO_TYPE_DISP2,
	VO_TYPE_FB,
	VO_TYPE_IMAGE,
	VO_TYPE_SHARE,
	VO_TYPE_MAX,
}mcpVoType_t;	

typedef enum mcpVoParam_e {
	VO_PRARM_LAYER = 0,   /* set display layer*/
	VO_PRARM_MODE,        /* set  display mode */
	VO_PRARM_COLOR,       /* set color space format */
	VO_PRARM_MAPINFO,     /* get vo width/height/type information */
	VO_PARAM_END
}mcpVoParam_t;	

/***************************************************************************
 * Inline Function Definitions
 ***************************************************************************/
	
/***************************************************************************
 * Global Data
  ***************************************************************************/

/***************************************************************************
 * Function Declarations
 ***************************************************************************/
void *voOpen(mcpVoType_t type);
void voClose(void *hd);
SINT32 voSet(void *hd, UINT32 param, UINT32 value);
SINT32 voGet(void *hd, UINT32 param, UINT32 *value);
SINT32 voOut(void *hd, gp_bitmap_t *img);
#endif   /* _GP_MCPVO_H_  */

/***************************************************************************
 * The gp_mcpvo.h file end
 ***************************************************************************/

