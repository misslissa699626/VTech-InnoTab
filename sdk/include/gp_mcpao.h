
/***************************************************************************
 * Name: gp_mcpao.h
 *
 * Purpose:
 *
 * Developer:
 *     zhoulu, 2010-8-18
 *
 * Copyright (c) 2010-2011 by Generalplus Inc.
 ***************************************************************************/

#ifndef _GP_MCPAO_H_
#define _GP_MCPAO_H_

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
typedef enum mcpAoParam_e {
	AO_FORMAT = 0,	 /* get/set pcm format */
	AO_SAMPLERATE,	 /* get/set sample rate */
	AO_CHANNELS,	 /* get/set channels */
	AO_FRAGMENT,	 /* get/set fragment */
	AO_VOLUME,		 /* get/set vlomue */
	AO_PARAM_END
}mcpAoParam_t;	

typedef enum mcpAoType_e {
	AO_TYPE_OSS = 0,
	AO_TYPE_MIXER,	/* libaudiomixer */
	AO_TYPE_ALSA,
	AO_TYPE_ESOUND,	
	AO_TYPE_END
}mcpAoType_t;	

/***************************************************************************
 * Inline Function Definitions
 ***************************************************************************/
	
/***************************************************************************
 * Global Data
  ***************************************************************************/

/***************************************************************************
 * Function Declarations
 ***************************************************************************/
void *aoOpen(mcpAoType_t type);
void aoClose(void *hd);
SINT32 aoOut(void *hd, UINT8 *data, UINT32 size);
SINT32 aoSpace(void *hd);
SINT32 aoDelay(void *hd);
SINT32 aoPause(void *hd);
SINT32 aoResume(void *hd);
SINT32 aoClean(void *hd);
SINT32 aoState(void *hd);
SINT32 aoSync(void *hd);
SINT32 aoSet(void *hd, UINT32 param, UINT32 value);
SINT32 aoGet(void *hd, UINT32 param, UINT32 *value);

#endif   /* _GP_MCPAO_H_  */

/***************************************************************************
 * The gp_mcpao.h file end
 ***************************************************************************/

