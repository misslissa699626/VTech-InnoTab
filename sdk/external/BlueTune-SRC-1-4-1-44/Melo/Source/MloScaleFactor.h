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
Scaling of spectral data

Ref:
4.6.2
*/

#ifndef _MLO_SCALE_FACTOR_H_
#define _MLO_SCALE_FACTOR_H_



/*----------------------------------------------------------------------
|       Includes
+---------------------------------------------------------------------*/

#include "MloBitStream.h"
#include "MloDefs.h"
#include "MloIcsInfo.h"
#include "MloResults.h"
#include "MloSectionData.h"
#include "MloTypes.h"



/*----------------------------------------------------------------------
|       Constants
+---------------------------------------------------------------------*/



#define  MLO_ERROR_SCALE_FACTOR_RANGE  (MLO_ERROR_BASE_SCALE_FACTOR-0)

/* Scale Factor value for 0 dB */
enum {   MLO_SCALE_FACTOR_UNITY_GAIN   = 100 };
enum {   MLO_SCALE_FACTOR_MIN_VAL      = 0   };
enum {   MLO_SCALE_FACTOR_MAX_VAL      = 255 };



/*----------------------------------------------------------------------
|       Types
+---------------------------------------------------------------------*/



typedef struct MLO_ScaleFactor
{
   /* For noise/intensity: array directly contains final
      noise_nrg/is_position values. */
   MLO_UInt16     scale_factors [MLO_DEFS_MAX_NBR_WIN_GRP] [MLO_DEFS_MAX_NUM_SWB];
}  MLO_ScaleFactor;



/*----------------------------------------------------------------------
|       Function prototypes
+---------------------------------------------------------------------*/



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



MLO_Result  MLO_ScaleFactor_Decode (MLO_ScaleFactor *sf_ptr, const MLO_IcsInfo *ics_ptr, const MLO_SectionData *sec_ptr, MLO_BitStream *bit_ptr, int global_gain);
void  MLO_ScaleFactor_ScaleCoefficients (const MLO_ScaleFactor *sf_ptr, const MLO_IcsInfo *ics_ptr, MLO_Float coef_ptr []);
MLO_Float   MLO_ScaleFactor_ComputeGain (int sf);



#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif /* _MLO_SCALE_FACTOR_H_ */
