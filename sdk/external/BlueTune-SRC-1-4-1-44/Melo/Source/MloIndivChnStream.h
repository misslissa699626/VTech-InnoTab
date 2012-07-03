/*****************************************************************
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

/*
Individual Channel Stream

Ref:
4.4.2.7, Table 4.44
4.5.2.3
*/

#ifndef _MLO_INDIV_CHN_STREAM_H_
#define _MLO_INDIV_CHN_STREAM_H_



/*----------------------------------------------------------------------
|       Includes
+---------------------------------------------------------------------*/



#include "MloSamplingFreq.h"
#include "MloBitStream.h"
#include "MloDefs.h"
#include "MloFloat.h"
#include "MloIcsInfo.h"
#include "MloScaleFactor.h"
#include "MloSectionData.h"
#include "MloTns.h"
#include "MloTypes.h"



/*----------------------------------------------------------------------
|       Constants
+---------------------------------------------------------------------*/



#define  MLO_ERROR_GC_IN_LC            (MLO_ERROR_BASE_INDIV_CHN_STREAM-0)
#define  MLO_ERROR_PULSE_SFB_RANGE     (MLO_ERROR_BASE_INDIV_CHN_STREAM-1)
#define  MLO_ERROR_TOO_MANY_PULSES     (MLO_ERROR_BASE_INDIV_CHN_STREAM-2)



/*----------------------------------------------------------------------
|       Types
+---------------------------------------------------------------------*/



typedef struct MLO_IndivChnStream_Pulse
{
   MLO_UInt8      offset;
   MLO_UInt8      amp;
}  MLO_IndivChnStream_Pulse;



typedef struct MLO_IndivChnStream
{
   int            global_gain;
   MLO_IcsInfo    ics_info;
   MLO_Boolean    pulse_data_present_flag;
   MLO_Boolean    tns_data_present_flag;

   /* Section data */
   MLO_SectionData
                  section_data;

   /* Scale factors */
   MLO_ScaleFactor
                  sf_data;

   /* Pulse */
   int            number_pulse;
   int            pulse_start_sfb;
   MLO_IndivChnStream_Pulse
                  pulse [4];

   /* Temporal Noise Shapping */
   MLO_Tns        tns;

   /* Spectral coefficients */
   MLO_Int16      data [MLO_DEFS_FRAME_LEN_LONG];
   /* sfb/win are interleaved until the end of the decoding and inverse quantisation part. */
   MLO_Float      coef_arr [MLO_DEFS_FRAME_LEN_LONG];    /* Hosts spectral coefficients during the spectral processing, and time samples after filterbank processing */
   MLO_Float      prev_frame [MLO_DEFS_FRAME_LEN_LONG];  /* Samples of the previous frame */

}  MLO_IndivChnStream;



/*----------------------------------------------------------------------
|       Function prototypes
+---------------------------------------------------------------------*/



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



void  MLO_IndivChnStream_ClearBuffers (MLO_IndivChnStream *ics_ptr);
MLO_Result  MLO_IndivChnStream_Decode (MLO_IndivChnStream *ics_ptr, MLO_BitStream *bit_ptr, MLO_Boolean common_win_flag, MLO_Boolean scale_flag, MLO_SamplingFreq_Index fs_index);



#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif /* _MLO_INDIV_CHN_STREAM_H_ */
