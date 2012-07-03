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
#include "MloElementPce.h"
#include "MloDecoder.h"

/*----------------------------------------------------------------------
|       Prototypes
+---------------------------------------------------------------------*/
static void  MLO_ElementPce_ReadTaggedElementArray (MLO_ElementPce_TaggedElt tag_elt_ptr [], MLO_BitStream *bit_ptr, int nbr_elt);

/*----------------------------------------------------------------------
|       Functions
+---------------------------------------------------------------------*/

/*
==============================================================================
Name: MLO_ElementPce_Decode
Description:
   Decode a Program Config Element
Output parameters:
	- pce_ptr: Object to build with the decoded input stream
Input/output parameters:
	- bits_ptr: Input bitstream to decode
Returns:
   MLO_SUCCESS if ok
   MLO_FAILURE for a bitstream error, a non-LC profile or an invalid
      sampling rate index.
==============================================================================
*/

MLO_Result	
MLO_ElementPce_Decode (MLO_ElementPce *pce_ptr, MLO_BitStream *bit_ptr)
{
   MLO_Result     result = MLO_SUCCESS;
   int            index;

	MLO_ASSERT(pce_ptr != NULL);
	MLO_ASSERT(bit_ptr != NULL);

   pce_ptr->element_instance_tag = MLO_BitStream_ReadBits (bit_ptr, 4);
   pce_ptr->object_type = (MLO_AacProfile) MLO_BitStream_ReadBits (bit_ptr, 2);
   if (pce_ptr->object_type != MLO_AAC_PROFILE_LC)
   {
      result = MLO_ERROR_DECODER_UNSUPPORTED_CONFIG;
   }

   if (MLO_SUCCEEDED (result))
   {
      pce_ptr->sampling_frequency_index =
         (MLO_SamplingFreq_Index) MLO_BitStream_ReadBits (bit_ptr, 4);
      if (pce_ptr->sampling_frequency_index >= MLO_SAMPLING_FREQ_INDEX_NBR_VALID)
      {
         result = MLO_ERROR_DECODER_INVALID_DATA;
      }
   }

   if (MLO_SUCCEEDED (result))
   {
      pce_ptr->num_front_channel_elements = MLO_BitStream_ReadBits (bit_ptr, 4);
      pce_ptr->num_side_channel_elements = MLO_BitStream_ReadBits (bit_ptr, 4);
      pce_ptr->num_back_channel_elements = MLO_BitStream_ReadBits (bit_ptr, 4);
      pce_ptr->num_lfe_channel_elements = MLO_BitStream_ReadBits (bit_ptr, 2);
      pce_ptr->num_assoc_data_elements = MLO_BitStream_ReadBits (bit_ptr, 3);
      pce_ptr->num_valid_cc_elements = MLO_BitStream_ReadBits (bit_ptr, 4);

      pce_ptr->mono_mixdown_present =
         (MLO_Boolean) MLO_BitStream_ReadBit (bit_ptr);
      if (pce_ptr->mono_mixdown_present)
      {
         pce_ptr->mono_mixdown_element_number =
            MLO_BitStream_ReadBits (bit_ptr, 4);
      }

      pce_ptr->stereo_mixdown_present =
         (MLO_Boolean) MLO_BitStream_ReadBit (bit_ptr);
      if (pce_ptr->stereo_mixdown_present)
      {
         pce_ptr->stereo_mixdown_element_number =
            MLO_BitStream_ReadBits (bit_ptr, 4);
      }

      pce_ptr->matrix_mixdown_idx_present =
         (MLO_Boolean) MLO_BitStream_ReadBit (bit_ptr);
      if (pce_ptr->matrix_mixdown_idx_present)
      {
         pce_ptr->matrix_mixdown_idx = MLO_BitStream_ReadBits (bit_ptr, 2);
         pce_ptr->pseudo_surround_enable =
            (MLO_Boolean) MLO_BitStream_ReadBit (bit_ptr);
      }

      MLO_ElementPce_ReadTaggedElementArray (
         pce_ptr->front_element_arr,
         bit_ptr,
         pce_ptr->num_front_channel_elements
      );

      MLO_ElementPce_ReadTaggedElementArray (
         pce_ptr->side_element_arr,
         bit_ptr,
         pce_ptr->num_side_channel_elements
      );

      MLO_ElementPce_ReadTaggedElementArray (
         pce_ptr->back_element_arr,
         bit_ptr,
         pce_ptr->num_back_channel_elements
      );

      for (index = 0; index < pce_ptr->num_lfe_channel_elements; ++index)
      {
         pce_ptr->lfe_element_tag_arr [index] =
            MLO_BitStream_ReadBits (bit_ptr, 4);
      }

      for (index = 0; index < pce_ptr->num_assoc_data_elements; ++index)
      {
         pce_ptr->assoc_data_element_tag_arr [index] =
            MLO_BitStream_ReadBits (bit_ptr, 4);
      }

      for (index = 0; index < pce_ptr->num_valid_cc_elements; ++index)
      {
         pce_ptr->cc_element_arr [index].is_ind_sw_flag =
            (MLO_Boolean) MLO_BitStream_ReadBit (bit_ptr);
         pce_ptr->cc_element_arr [index].tag =
            MLO_BitStream_ReadBits (bit_ptr, 4);
      }

      result = MLO_BitStream_ByteAlign (bit_ptr);
   }

   if (MLO_SUCCEEDED (result))
   {
      int comment_field_bytes = MLO_BitStream_ReadBits(bit_ptr, 8);
      MLO_BitStream_SkipBits(bit_ptr, 8*comment_field_bytes);
   }

	return (result);
}



/*
==============================================================================
Name: MLO_ElementPce_ReadTaggedElementArray
Description:
   Decode an array of MLO_ElementPce_TaggedElt from the bitstream.
Input parameters:
	- nbr_elt: Number of elements to decode, >= 0, limited by the array size
Output parameters:
	- tag_elt_ptr: start of the array to fill
Input/output parameters:
	- bits_ptr: Input bitstream to decode
==============================================================================
*/

void
MLO_ElementPce_ReadTaggedElementArray (MLO_ElementPce_TaggedElt tag_elt_ptr [], MLO_BitStream *bit_ptr, int nbr_elt)
{
    int index;

	MLO_ASSERT(tag_elt_ptr != NULL);
	MLO_ASSERT(bit_ptr != NULL);
	MLO_ASSERT(nbr_elt >= 0);

    for (index = 0; index < nbr_elt; ++index)
    {
        tag_elt_ptr [index].is_cpe_flag =
            (MLO_Boolean) MLO_BitStream_ReadBit (bit_ptr);
        tag_elt_ptr [index].tag = MLO_BitStream_ReadBits (bit_ptr, 4);
    }
}



