/*****************************************************************
|
|    Melo - Frames
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

#ifndef _MLO_FRAME_H_
#define _MLO_FRAME_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "MloSamplingFreq.h"
#include "MloBitStream.h"
#include "MloTypes.h"

/*----------------------------------------------------------------------
|       types
+---------------------------------------------------------------------*/
typedef enum {
    MLO_AAC_STANDARD_MPEG2,
    MLO_AAC_STANDARD_MPEG4
} MLO_AacStandard;

typedef enum {
    MLO_AAC_PROFILE_MAIN,
    MLO_AAC_PROFILE_LC,
    MLO_AAC_PROFILE_SSR,
    MLO_AAC_PROFILE_LTP
} MLO_AacProfile;

typedef struct {
    MLO_AacStandard standard;
    MLO_AacProfile  profile;
    MLO_SamplingFreq_Index
                    sampling_frequency_index;
    unsigned long   sampling_frequency;
    unsigned int    channel_configuration;
    unsigned int    frame_length;            /* With headers and check, bytes */
} MLO_FrameInfo;

typedef struct {
    MLO_BitStream* source;
    MLO_FrameInfo  info;
} MLO_FrameData;

#endif /* _MLO_FRAME_H_ */
