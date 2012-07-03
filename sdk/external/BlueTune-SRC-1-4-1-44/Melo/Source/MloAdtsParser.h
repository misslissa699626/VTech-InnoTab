/*****************************************************************
|
|    Melo - ADTS Parser
|
|    Copyright 2004-2006 Axiomatic Systems LLC
|
|    This file is part of Melo (Melo AAC Decoder).
|
|    Unless you have obtained Melo under a difference license,
|    this version of Melo is Melo|GPL.
|    Melo|GPL is free software; you can redistribute it and/or modify
|    it under the terms of the GNU General Public License as published by
|    the Free Software Foundation; either version 2, or (at your option)
|    any later version.
|
|    Melo|GPL is distributed in the hope that it will be useful,
|    but WITHOUT ANY WARRANTY; without even the implied warranty of
|    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|    GNU General Public License for more details.
|
|    You should have received a copy of the GNU General Public License
|    along with Melo|GPL; see the file COPYING.  If not, write to the
|    Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
|    02111-1307, USA.
|
 ****************************************************************/
/** @file
 * Melo - Frames
 */

#ifndef _MLO_ADTS_PARSER_H_
#define _MLO_ADTS_PARSER_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "MloTypes.h"

/*----------------------------------------------------------------------
|       types
+---------------------------------------------------------------------*/
typedef struct MLO_AdtsParser MLO_AdtsParser;

/*----------------------------------------------------------------------
|       function prototypes
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    
MLO_Result MLO_AdtsParser_Create(MLO_AdtsParser** parser);
MLO_Result MLO_AdrsParser_Destroy(MLO_AdtsParser* parser);
MLO_Result MLO_AdtsParser_Feed(MLO_AdtsParser* parser, 
                               MLO_ByteBuffer  buffer, 
                               MLO_Size*       size,
                               MLO_Flags       flags);
MLO_Result MLO_AdtsParser_FindFrame(MLO_AdtsParser* parser,
                                    MLO_FrameData*  frame);
MLO_Result MLO_AdtsParser_Skip(MLO_AdtsParser* parser,
                               MLO_Size        bytes);
unsigned int   MLO_AdtsParser_GetBytesFree (const MLO_AdtsParser * parser_ptr);
unsigned int   MLO_AdtsParser_GetBytesAvailable (const MLO_AdtsParser * parser_ptr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _MLO_ADTS_PARSER_H_ */
