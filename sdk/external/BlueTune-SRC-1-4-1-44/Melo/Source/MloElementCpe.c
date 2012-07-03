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
#include "MloElementCpe.h"
#include "MloIndivChnPool.h"
#include "MloIndivChnStream.h"
#include "MloIs.h"
#include "MloMs.h"
#include "MloPns.h"
#include "MloUtils.h"

/*----------------------------------------------------------------------
|       Functions
+---------------------------------------------------------------------*/
/*
==============================================================================
Name: MLO_ElementCpe_Decode
Description:
   Decodes a coupling_channel_element and processes spectral data up to
   Intensity Stereo (IS) stage.
   Ref: 4.4.2.1, Table 4.5
Input parameters:
	- fs_index: Valid Sampling Frequency Index for the frame
Input/output parameters:
	- cpe_ptr: MLO_ElementCpe object
	- bit_ptr: Input bitstream to decode
	- chn_pool_ptr: Channel pool, with room for 2 more channel.
Returns:
   MLO_SUCESS if ok
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
MLO_ElementCpe_Decode (MLO_ElementCpe *cpe_ptr, MLO_BitStream *bit_ptr, MLO_IndivChnPool *chn_pool_ptr, MLO_SamplingFreq_Index fs_index)
{
   MLO_Result     result = MLO_SUCCESS;
   MLO_Boolean    common_window_flag = MLO_FALSE;
   int            ics_index_arr [2] = { -1, -1 };

	MLO_ASSERT(cpe_ptr != NULL);
	MLO_ASSERT(bit_ptr != NULL);
	MLO_ASSERT(chn_pool_ptr != NULL);
    MLO_ASSERT(fs_index >= 0);
    MLO_CHECK_ARGS(fs_index < MLO_SAMPLING_FREQ_INDEX_NBR_VALID);

   cpe_ptr->element_instance_tag = MLO_BitStream_ReadBits (bit_ptr, 4);

   if (MLO_SUCCEEDED (result))
   {
      result = MLO_IndivChnPool_AddChn (
         chn_pool_ptr,
         &ics_index_arr [0]
      );
   }
   if (MLO_SUCCEEDED (result))
   {
      result = MLO_IndivChnPool_AddChn (
         chn_pool_ptr,
         &ics_index_arr [1]
      );
   }

   if (MLO_SUCCEEDED (result))
   {
      cpe_ptr->ics_ptr_arr [0] = chn_pool_ptr->chn_ptr_arr [ics_index_arr [0]];
      cpe_ptr->ics_ptr_arr [1] = chn_pool_ptr->chn_ptr_arr [ics_index_arr [1]];

      common_window_flag = MLO_BitStream_ReadBit (bit_ptr);
      cpe_ptr->common_window_flag = common_window_flag;
      if (common_window_flag)
      {
         result = MLO_IcsInfo_Decode (
            &cpe_ptr->ics_ptr_arr [0]->ics_info,
            bit_ptr,
            fs_index
         );
         cpe_ptr->ics_ptr_arr [1]->ics_info = cpe_ptr->ics_ptr_arr [0]->ics_info;
      }
      else
      {
         /* If ics_info is not common, there is not M/S thing. Setting
            this value avoids to check common_window_flag everytime
            ms_mask_present has to be tested. */
         cpe_ptr->ms_mask_present = MLO_ELEMENT_CPE_MS_MASK_TYPE_ALL_0;
      }
   }

   if (MLO_SUCCEEDED (result) && common_window_flag)
   {
      cpe_ptr->ms_mask_present =
         (MLO_ElementCpe_MsMaskType) MLO_BitStream_ReadBits (bit_ptr, 2);

      if (cpe_ptr->ms_mask_present == MLO_ELEMENT_CPE_MS_MASK_TYPE_USED)
      {
         /* Collects mask flags */
         const MLO_IcsInfo *  ics_info_ptr =
            &cpe_ptr->ics_ptr_arr [0]->ics_info;
         const int      max_sfb = ics_info_ptr->max_sfb;
         const int      num_window_groups = ics_info_ptr->num_window_groups;
         int            g;

         MLO_CHECK_DATA(num_window_groups <= (int)MLO_ARRAY_SIZE (cpe_ptr->ms_used));
         MLO_CHECK_DATA(max_sfb           <= (int)MLO_ARRAY_SIZE (cpe_ptr->ms_used [0]));

         for (g = 0; g < num_window_groups; ++g)
         {
            int            sfb;

            for (sfb = 0; sfb < max_sfb; ++sfb)
            {
               cpe_ptr->ms_used [g] [sfb] = MLO_BitStream_ReadBit (bit_ptr);
            }
         }
      }
   }

   /* Decodes channel streams */
   if (MLO_SUCCEEDED (result))
   {
      result = MLO_IndivChnStream_Decode (
         cpe_ptr->ics_ptr_arr [0],
         bit_ptr,
         common_window_flag,
         MLO_FALSE,
         fs_index
      );
   }
   if (MLO_SUCCEEDED (result))
   {
      result = MLO_IndivChnStream_Decode (
         cpe_ptr->ics_ptr_arr [1],
         bit_ptr,
         common_window_flag,
         MLO_FALSE,
         fs_index
      );
   }

   /* Other processings */
   if (MLO_SUCCEEDED (result))
   {
      /* Mid/Side coding */
      MLO_Ms_Process (cpe_ptr);

      /* Perceptual Noise Substitution */
      MLO_Pns_ProcessPair (cpe_ptr);

      /* Intensity Stereo */
      MLO_Is_Process (cpe_ptr);
   }

	return (result);
}
