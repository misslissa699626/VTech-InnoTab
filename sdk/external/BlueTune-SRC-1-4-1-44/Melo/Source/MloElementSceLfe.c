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

/*----------------------------------------------------------------------
|       Includes
+---------------------------------------------------------------------*/
#include "MloBitStream.h"
#include "MloDebug.h"
#include "MloElementSceLfe.h"
#include "MloIndivChnPool.h"
#include "MloIndivChnStream.h"
#include "MloPns.h"
#include "MloUtils.h"

/*----------------------------------------------------------------------
|       Functions
+---------------------------------------------------------------------*/

/*
==============================================================================
Name: MLO_ElementSceLfe_Decode
Description:
   Decodes a single_channel_element or an lfe_channel_element and processes
   spectral data up to Perceptual Noise Substitution (PNS) stage.
   Ref: 4.4.2.1, Table 4.4
Input parameters:
	- fs_index: Valid Sampling Frequency Index for the frame
Input/output parameters:
	- sce_ptr: MLO_ElementSceLfe object
	- bit_ptr: Input bitstream to decode
	- chn_pool_ptr: Channel pool, with room for one more channel.
Returns:
   MLO_SUCCESS if ok
   MLO_ERROR_NO_CHN_AVAILABLE if no room in the channel pool
   MLO_ERROR_LTP_IN_LC if unexpected LTP block is encountered (LC profile)
   MLO_ERROR_UNSUPPORTED_SFI if sample frequency is not supported
   MLO_ERROR_GC_IN_LC if Gain Control block is encountered (LC profile)
   MLO_ERROR_SCALE_FACTOR_RANGE if a scale factor is out of expected range,
   MLO_ERROR_PULSE_SFB_RANGE if pulse_start_sfb is out of expected range,
   MLO_ERROR_TOO_MANY_PULSES if there are too many pulses
   MLO_ERROR_UNEXPECTED_CB_TYPE for an unexpected codebook type in Spectral
      Data (12, reserved)
==============================================================================
*/

MLO_Result	
MLO_ElementSceLfe_Decode (MLO_ElementSceLfe *sce_ptr, MLO_BitStream *bit_ptr, MLO_IndivChnPool *chn_pool_ptr, MLO_SamplingFreq_Index fs_index)
{
   MLO_Result     result = MLO_SUCCESS;
   MLO_IndivChnStream * ics_ptr = 0;
   int            ics_index;

	MLO_ASSERT(sce_ptr != NULL);
	MLO_ASSERT(bit_ptr != NULL);
	MLO_ASSERT(chn_pool_ptr != NULL);
    MLO_ASSERT(fs_index >= 0);
    MLO_CHECK_ARGS(fs_index < MLO_SAMPLING_FREQ_INDEX_NBR_VALID);

    sce_ptr->element_instance_tag = MLO_BitStream_ReadBits (bit_ptr, 4);

   result = MLO_IndivChnPool_AddChn (chn_pool_ptr, &ics_index);
   if (MLO_SUCCEEDED (result))
   {
      ics_ptr = chn_pool_ptr->chn_ptr_arr [ics_index];
      sce_ptr->ics_ptr = ics_ptr;

      result = MLO_IndivChnStream_Decode (
         ics_ptr,
         bit_ptr,
         MLO_FALSE,
         MLO_FALSE,
         fs_index
      );
   }

   /* Other processings */
   if (MLO_SUCCEEDED (result))
   {
      MLO_Pns_ProcessSingle (ics_ptr);
   }

	return (result);
}



