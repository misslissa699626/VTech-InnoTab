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
#include "MloDefs.h"
#include "MloElementFil.h"
#include "MloUtils.h"

/*----------------------------------------------------------------------
|       Types
+---------------------------------------------------------------------*/
/* 4.5.2.9.3, Table 4.105 */
typedef enum MLO_ElementFil_Ext
{
   MLO_ELEMENT_FIL_EXT_FILL          = 0,
   MLO_ELEMENT_FIL_EXT_FILL_DATA     = 1,
   MLO_ELEMENT_FIL_EXT_DATA_ELEMENT  = 2,
   MLO_ELEMENT_FIL_EXT_DYNAMIC_RANGE = 11,
   MLO_ELEMENT_FIL_EXT_SBR_DATA      = 13,
   MLO_ELEMENT_FIL_EXT_SBR_DATA_CRC  = 14
}  MLO_ElementFil_Ext;

/* 4.5.2.9.4, Table 4.106 */
typedef enum MLO_ElementFil_DataElementVersion
{
   MLO_ELEMENT_FIL_DATA_ELEMENT_VERSION_ANC_DATA = 0
}  MLO_ElementFil_DataElementVersion;

/*----------------------------------------------------------------------
|       Prototypes
+---------------------------------------------------------------------*/
static MLO_Result MLO_ElementFil_DecodeExtensionPayload (MLO_ElementFil *fil_ptr, MLO_BitStream *bit_ptr, unsigned int *count_ptr);
static void MLO_ElementFil_DecodeFill (MLO_BitStream *bit_ptr, unsigned int count);
static MLO_Result MLO_ElementFil_DecodeFillData (MLO_BitStream *bit_ptr, unsigned int count);
static void MLO_ElementFil_DecodeDataElement (MLO_BitStream *bit_ptr, unsigned int *count_ptr);
static MLO_Result MLO_ElementFil_DecodeDynamicRangeInfo(MLO_ElementFil_DynamicRangeInfo *drc_ptr, MLO_BitStream *bit_ptr, unsigned int* bytes_used);
static MLO_Result MLO_ElementFil_DecodeExcludedChannels(MLO_ElementFil_DynamicRangeInfo *drc_ptr, MLO_BitStream *bit_ptr, unsigned int* bytes_used);

/*----------------------------------------------------------------------
|       Functions
+---------------------------------------------------------------------*/

/*
==============================================================================
Name: MLO_ElementFil_Decode
Description:
   Decodes the Fill Element (FIL).
   Ref: 4.4.1.1, Table 4.11
Output parameters:
	- fil_ptr: FIL object to build with the input bitstream
Input/output parameters:
	- bits_ptr: Input bitstream to decode
Returns:
   MLO_SUCCESS if ok
   MLO_FAILURE for a bitstream error.
==============================================================================
*/

MLO_Result	
MLO_ElementFil_Decode (MLO_ElementFil *fil_ptr, MLO_BitStream *bit_ptr)
{
   MLO_Result   result = MLO_SUCCESS;
   unsigned int count;

	MLO_ASSERT(fil_ptr != NULL);
	MLO_ASSERT(bit_ptr != NULL);

   count = MLO_BitStream_ReadBits (bit_ptr, 4);
   if (count == 15)
   {
      unsigned int esc_count = MLO_BitStream_ReadBits (bit_ptr, 8);
      count += esc_count - 1;
   }

   while (count > 0 && MLO_SUCCEEDED (result))
   {
      unsigned int cnt = count;
      result = MLO_ElementFil_DecodeExtensionPayload (fil_ptr, bit_ptr, &cnt);
      count -= cnt;
   }

   return (result);
}



/*
==============================================================================
Name: MLO_ElementFil_DecodeExtensionPayload
Description:
   Decodes one extension payload section of the FIL element.
   Ref: 4.4.2.7, Table 4.51
   4.5.2.9
Output parameters:
	- fil_ptr: FIL object to build with the input bitstream
Input/output parameters:
	- bits_ptr: Input bitstream to decode
	- count_ptr:
      input: Number of bytes to read
      output: Number of bytes actually read on the stream
Returns:
   MLO_SUCCESS if ok
   MLO_ERROR_SBR_IN_LC if an unexpected SBR section is encountered
   MLO_ERROR_WRONG_FILL_NIBBLE if wrong fill_nibble value
   MLO_ERROR_WRONG_FILL_BYTE if wrong fill_byte value
==============================================================================
*/

MLO_Result	
MLO_ElementFil_DecodeExtensionPayload (MLO_ElementFil *fil_ptr, MLO_BitStream *bit_ptr, unsigned int* count_ptr)
{
    MLO_Result     result = MLO_SUCCESS;
    MLO_ElementFil_Ext   extension_type;

    MLO_ASSERT(fil_ptr != NULL);
	MLO_ASSERT(bit_ptr != NULL);
	MLO_ASSERT(count_ptr != NULL);
    MLO_CHECK_ARGS(*count_ptr > 0);

    extension_type = (MLO_ElementFil_Ext) MLO_BitStream_ReadBits (bit_ptr, 4);
    switch (extension_type)
   {
   case  MLO_ELEMENT_FIL_EXT_FILL:
      MLO_ElementFil_DecodeFill (bit_ptr, *count_ptr);
      break;

   case  MLO_ELEMENT_FIL_EXT_FILL_DATA:
      result = MLO_ElementFil_DecodeFillData (bit_ptr, *count_ptr);
      break;

   case  MLO_ELEMENT_FIL_EXT_DATA_ELEMENT:
      MLO_ElementFil_DecodeDataElement (bit_ptr, count_ptr);
      break;

   case  MLO_ELEMENT_FIL_EXT_DYNAMIC_RANGE:
      result = MLO_ElementFil_DecodeDynamicRangeInfo (
         &fil_ptr->dynamic_range_info,
         bit_ptr,
         count_ptr
      );
      break;

   default:
      MLO_ElementFil_DecodeFill (bit_ptr, *count_ptr);
      break;
   }

	return (result);
}



/*
==============================================================================
Name: MLO_ElementFil_DecodeFill
Description:
   Skip bits in the input stream
Input parameters:
	- count: Number of bytes to skip (including the 4-bit previous nibble)
Input/output parameters:
	- bits_ptr: Input bitstream to decode
==============================================================================
*/

void	
MLO_ElementFil_DecodeFill (MLO_BitStream *bit_ptr, unsigned int count)
{
    int            align = 4;
    int            bits_to_skip = (count - 1) * 8 + align;

	MLO_ASSERT(bit_ptr != NULL);

    MLO_BitStream_SkipBits (bit_ptr, bits_to_skip);
}



/*
==============================================================================
Name: MLO_ElementFil_DecodeFillData
Description:
   Check fill data in the input stream.
Input parameters:
	- count: Number of bytes to skip (including the 4-bit previous nibble)
Input/output parameters:
	- bits_ptr: Input bitstream to decode
Returns:
   MLO_SUCCESS if ok
   MLO_ERROR_WRONG_FILL_NIBBLE if wrong fill_nibble value
   MLO_ERROR_WRONG_FILL_BYTE if wrong fill_byte value
==============================================================================
*/

MLO_Result	
MLO_ElementFil_DecodeFillData (MLO_BitStream *bit_ptr, unsigned int count)
{
    MLO_Result     result = MLO_SUCCESS;
    int            data;
    unsigned int   index;

	MLO_ASSERT(bit_ptr != NULL);

    data = MLO_BitStream_ReadBits (bit_ptr, 4);
    if (data != 0)
    {
       result = MLO_ERROR_WRONG_FILL_NIBBLE;
    }

    -- count;
    for (index = 0; index < count && MLO_SUCCEEDED (result); ++index)
    {
        data = MLO_BitStream_ReadBits (bit_ptr, 8);
        if (data != 0xA5)
        {
            result = MLO_ERROR_WRONG_FILL_BYTE;
        }
    }

 	return (result);
}



/*
==============================================================================
Name: MLO_ElementFil_DecodeDataElement
Description:
   Skips the EXT_DATA_ELEMENT section
Input/Output parameters:
	- bits_ptr: Input bitstream to decode
Output parameters:
	- count_ptr: Number of skipped bytes
==============================================================================
*/

void	
MLO_ElementFil_DecodeDataElement (MLO_BitStream *bit_ptr, unsigned int *count_ptr)
{
    MLO_ElementFil_DataElementVersion   data_element_version;

    MLO_ASSERT(bit_ptr != NULL);

   data_element_version =
      (MLO_ElementFil_DataElementVersion) MLO_BitStream_ReadBits (bit_ptr, 4);
   if (data_element_version == MLO_ELEMENT_FIL_DATA_ELEMENT_VERSION_ANC_DATA)
   {
      int            loop_counter = 0;
      int            data_element_length = 0;
      int            data_element_length_part;
      do
      {
         data_element_length_part = MLO_BitStream_ReadBits (bit_ptr, 8);
         data_element_length += data_element_length_part;
         ++ loop_counter;
      }
      while (data_element_length_part == 255);

      MLO_BitStream_SkipBits (bit_ptr, data_element_length * 8);

      *count_ptr = data_element_length + loop_counter + 1;
   }
}



/*
==============================================================================
Name: MLO_ElementFil_DecodeDynamicRangeInfo
Description:
   Decodes the Dynamic Range Control (CRC) information section from the
   bitstream.
   Ref: 4.4.2.7, Table 4.52
Input/output parameters:
	- drc_ptr: Pointer on the DRC structure to fill. Should have been properly
      initialized before entering the function.
	- bits_ptr: Input bitstream to decode
Returns: Number of bytes read from the stream, > 0
==============================================================================
*/

MLO_Result
MLO_ElementFil_DecodeDynamicRangeInfo (MLO_ElementFil_DynamicRangeInfo *drc_ptr, 
                                       MLO_BitStream                   *bit_ptr, 
                                       unsigned int                    *bytes_used)
{
    int index;

    MLO_ASSERT(drc_ptr != NULL);
	MLO_ASSERT(bit_ptr != NULL);

    *bytes_used = 1;
    drc_ptr->num_bands = 1;

   if (MLO_BitStream_ReadBit (bit_ptr) != 0)
   {
      drc_ptr->pce_instance_tag = MLO_BitStream_ReadBits (bit_ptr, 4);
      drc_ptr->tag_reserved_bits = MLO_BitStream_ReadBits (bit_ptr, 4);
      ++*bytes_used;
   }

   drc_ptr->excluded_chns_present_flag =
      (MLO_Boolean) MLO_BitStream_ReadBit (bit_ptr);
   if (drc_ptr->excluded_chns_present_flag)
   {
       unsigned int used = 0;
       MLO_Result result = MLO_ElementFil_DecodeExcludedChannels (drc_ptr, bit_ptr, &used);
       if (MLO_FAILED(result)) return result;
       *bytes_used += used;
   }

   if (MLO_BitStream_ReadBit (bit_ptr) != 0)
   {
      int            band_inc = MLO_BitStream_ReadBits (bit_ptr, 4);
      MLO_BitStream_SkipBits (bit_ptr, 4);
      drc_ptr->num_bands += band_inc;

      for (index = 0; index < drc_ptr->num_bands; ++index)
      {
         drc_ptr->band_top_arr [index] = MLO_BitStream_ReadBits (bit_ptr, 8);
      }

      *bytes_used += drc_ptr->num_bands + 1;
   }

   if (MLO_BitStream_ReadBit (bit_ptr) != 0)
   {
      drc_ptr->prog_ref_level = MLO_BitStream_ReadBits (bit_ptr, 7);
      MLO_BitStream_SkipBit (bit_ptr);
      ++*bytes_used;
   }

   for (index = 0; index < drc_ptr->num_bands; ++index)
   {
      drc_ptr->dyn_rng_sgn [index] =
         1 - (MLO_BitStream_ReadBit (bit_ptr) << 1);
      drc_ptr->dyn_rng_ctl [index] = MLO_BitStream_ReadBits (bit_ptr, 7);
   }
   *bytes_used += drc_ptr->num_bands;

   return MLO_SUCCESS;
}



/*
==============================================================================
Name: MLO_ElementFil_DecodeExcludedChannels
Description:
   Decodes the channel exclusion list for the Dynamic Range Control (DRC)
   Ref: 4.4.2.7, Table 4.53
Input/output parameters:
	- drc_ptr: Pointer on the DRC structure to fill.
	- bits_ptr: Input bitstream to decode
Returns: Number of bytes read from the stream, > 0
==============================================================================
*/

MLO_Result
MLO_ElementFil_DecodeExcludedChannels (MLO_ElementFil_DynamicRangeInfo *drc_ptr, MLO_BitStream *bit_ptr, unsigned int* bytes_read)
{
   MLO_CHECK_CST (
      ExcludedMaskBitDepth,
      MLO_BIT_DEPTH (drc_ptr->excluded_mask) >= MLO_DEFS_MAX_CHN
   );

   int continue_flag = 1;
   int offset = 0;

   MLO_ASSERT(drc_ptr != NULL);
   MLO_ASSERT(bit_ptr != NULL);
   *bytes_read = 0;
   drc_ptr->excluded_mask = 0;

   do
   {
      int index;
      for (index = 0; index < 7; ++index)
      {
         int            bit = MLO_BitStream_ReadBit (bit_ptr);
         int            channel = offset + index;
         MLO_CHECK_DATA(channel < (int)MLO_BIT_DEPTH (drc_ptr->excluded_mask));

         drc_ptr->excluded_mask |= (bit << channel);
      }
      continue_flag = MLO_BitStream_ReadBit (bit_ptr);

      ++*bytes_read;
      offset += 7;
   }
   while (continue_flag != 0);

    return MLO_SUCCESS;
}
