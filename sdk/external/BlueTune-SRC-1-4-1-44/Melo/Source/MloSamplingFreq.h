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

#ifndef _MLO_SAMPLING_FREQ_H_
#define _MLO_SAMPLING_FREQ_H_

/*----------------------------------------------------------------------
|    include
+---------------------------------------------------------------------*/
#include "MloConfig.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* 1.6.3.4, Table 1.16 */
typedef enum MLO_SamplingFreq_Index
{
   MLO_SAMPLING_FREQ_INDEX_INVALID = -1,

   MLO_SAMPLING_FREQ_96000,
   MLO_SAMPLING_FREQ_88200,
   MLO_SAMPLING_FREQ_64000,
   MLO_SAMPLING_FREQ_48000,
   MLO_SAMPLING_FREQ_44100,
   MLO_SAMPLING_FREQ_32000,
   MLO_SAMPLING_FREQ_24000,
   MLO_SAMPLING_FREQ_22050,
   MLO_SAMPLING_FREQ_16000,
   MLO_SAMPLING_FREQ_12000,
   MLO_SAMPLING_FREQ_11025,
   MLO_SAMPLING_FREQ_8000,
   MLO_SAMPLING_FREQ_7350,

   MLO_SAMPLING_FREQ_INDEX_NBR_SUPPORTED = 12,   /* 7350 Hz is not supported by some parts of the specs */
   MLO_SAMPLING_FREQ_INDEX_NBR_VALID = 13,
   MLO_SAMPLING_FREQ_INDEX_RESERVED = MLO_SAMPLING_FREQ_INDEX_NBR_VALID,
   MLO_SAMPLING_FREQ_INDEX_ESCAPE = 15,

   MLO_SAMPLING_FREQ_INDEX_NBR_ELT
}  MLO_SamplingFreq_Index;



extern const MLO_UInt32 
MLO_SamplingFreq_table [MLO_SAMPLING_FREQ_INDEX_NBR_ELT];


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* _MLO_SAMPLING_FREQ_H_ */
