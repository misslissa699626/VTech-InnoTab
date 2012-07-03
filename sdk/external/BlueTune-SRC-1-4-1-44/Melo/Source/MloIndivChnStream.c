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
#include "MloSamplingFreq.h"
#include "MloBitStream.h"
#include "MloDebug.h"
#include "MloDefs.h"
#include "MloHcb.h"
#include "MloHuffman.h"
#include "MloIndivChnStream.h"
#include "MloInvQuant.h"
#include "MloUtils.h"

/*----------------------------------------------------------------------
|       Function prototypes
+---------------------------------------------------------------------*/
static MLO_Result MLO_IndivChnStream_DecodePulseData (MLO_IndivChnStream *ics_ptr, MLO_BitStream *bit_ptr);
static MLO_Result MLO_IndivChnStream_DecodeSpectralData (MLO_IndivChnStream *ics_ptr, MLO_BitStream *bit_ptr);

/*----------------------------------------------------------------------
|       Functions
+---------------------------------------------------------------------*/
/*
==============================================================================
Name: MLO_IndivChnStream_ClearBuffers
Description:
   Clears memory and past of the channel. To be called at least once before
   decoding a new stream.
Input/output parameters:
	- ics_ptr: channel to clear.
==============================================================================
*/

void  MLO_IndivChnStream_ClearBuffers (MLO_IndivChnStream *ics_ptr)
{
   MLO_ASSERT (ics_ptr != NULL);

   MLO_IcsInfo_ClearBuffers (&ics_ptr->ics_info);

   MLO_SetMemory (
      &ics_ptr->prev_frame [0],
      0,
      sizeof (ics_ptr->prev_frame)
   );
}



/*
==============================================================================
Name: MLO_IndivChnStream_Decode
Description:
   Decodes the individual_channel_stream section.
   Ref:
   4.4.2.7, Table 4.44
   4.5.2.3
Input parameters:
	- common_win_flag: Indicates if the ics_info is common for both channels
      (should have been provided before)
	- scale_flag: Doesn't contain ics_info, pulse_data and tns_data
   - fs_index: Valid Sampling Frequency Index for the frame
Input/output parameters:
	- ics_ptr: depending on the common_win_flag and scale_flag, ics_info should
      have been (or not) initialized before calling the function.
	- bit_ptr: Input bitstream to decode
Returns:
   MLO_SUCESS if ok
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

MLO_Result	MLO_IndivChnStream_Decode (MLO_IndivChnStream *ics_ptr, MLO_BitStream *bit_ptr, MLO_Boolean common_win_flag, MLO_Boolean scale_flag, MLO_SamplingFreq_Index fs_index)
{
   int            result = MLO_SUCCESS;

   MLO_ASSERT (ics_ptr != NULL);
   MLO_ASSERT (bit_ptr != NULL);
   MLO_ASSERT (fs_index >= 0);
   MLO_ASSERT (fs_index < MLO_SAMPLING_FREQ_INDEX_NBR_VALID);

   ics_ptr->global_gain = MLO_BitStream_ReadBits (bit_ptr, 8);

   if (! common_win_flag && ! scale_flag)
   {
      /* ICS info */
      result = MLO_IcsInfo_Decode (&ics_ptr->ics_info, bit_ptr, fs_index);
   }

   if (MLO_SUCCEEDED (result))
   {
      /* Section data */
      MLO_SectionData_Decode (
         &ics_ptr->section_data,
         &ics_ptr->ics_info,
         bit_ptr
      );

      /* Scale factor data */
      result = MLO_ScaleFactor_Decode (
         &ics_ptr->sf_data,
         &ics_ptr->ics_info,
         &ics_ptr->section_data,
         bit_ptr,
         ics_ptr->global_gain
      );
   }

   if (MLO_SUCCEEDED (result) && ! scale_flag)
   {
      /* Pulse */
      ics_ptr->pulse_data_present_flag =
         (MLO_Boolean) MLO_BitStream_ReadBit (bit_ptr);
      if (ics_ptr->pulse_data_present_flag)
      {
         result = MLO_IndivChnStream_DecodePulseData (ics_ptr, bit_ptr);
      }
   }

   if (MLO_SUCCEEDED (result) && ! scale_flag)
   {
      /* TNS */
      ics_ptr->tns_data_present_flag =
         (MLO_Boolean) MLO_BitStream_ReadBit (bit_ptr);
      if (ics_ptr->tns_data_present_flag)
      {
         MLO_Tns_Decode (&ics_ptr->tns, &ics_ptr->ics_info, bit_ptr);
      }

      /* Gain Control */
      if (MLO_BitStream_ReadBit (bit_ptr) != 0)
      {
         result = MLO_ERROR_GC_IN_LC;   /* No Gain Control in LC profile */
      }
   }

   if (MLO_SUCCEEDED (result))
   {
      /* Spectral Data */
      result = MLO_IndivChnStream_DecodeSpectralData (ics_ptr, bit_ptr);
   }

   if (MLO_SUCCEEDED (result))
   {
      /* Finishes uncompression of the spectral data */
      MLO_InvQuant_ProcessChannel (ics_ptr);
      MLO_ScaleFactor_ScaleCoefficients (
         &ics_ptr->sf_data,
         &ics_ptr->ics_info,
         &ics_ptr->coef_arr [0]
      );

      /* Deinterleave window coefficients */
      MLO_IcsInfo_DeinterleaveCoefficients (
         &ics_ptr->ics_info,
         &ics_ptr->coef_arr [0]
      );
   }

	return (result);
}



/*
==============================================================================
Name: MLO_IndivChnStream_DecodePulseData
Description:
   Decodes pulse_data section.
   Ref:
      4.4.2.7, Table 4.??
      4.6.3
Input/output parameters:
	- ics_ptr: Information for the channel. Requires ics_info and window
      grouping information to be filled.
	- bits_ptr: Input bitstream to decode
Returns:
   MLO_SUCCESS if ok
   MLO_ERROR_TOO_MANY_PULSES if there are too many pulses
   MLO_ERROR_PULSE_SFB_RANGE if pulse_start_sfb is out of expected range,
==============================================================================
*/

MLO_Result  MLO_IndivChnStream_DecodePulseData (MLO_IndivChnStream *ics_ptr, MLO_BitStream *bit_ptr)
{
   MLO_Result     result = MLO_SUCCESS;
   int            nbr_pulses;

   MLO_ASSERT (ics_ptr != NULL);
   MLO_ASSERT (bit_ptr != NULL);

   ics_ptr->number_pulse = MLO_BitStream_ReadBits (bit_ptr, 2);
   ics_ptr->pulse_start_sfb = MLO_BitStream_ReadBits (bit_ptr, 6);

   nbr_pulses = ics_ptr->number_pulse + 1;

   if (nbr_pulses > (int)MLO_ARRAY_SIZE (ics_ptr->pulse))
   {
      result = MLO_ERROR_TOO_MANY_PULSES;
   }
   else if (ics_ptr->pulse_start_sfb > ics_ptr->ics_info.num_swb)
   {
      result = MLO_ERROR_PULSE_SFB_RANGE;
   }

   else
   {
      int            p;
      for (p = 0; p < nbr_pulses; ++ p)
      {
         MLO_IndivChnStream_Pulse * pulse_ptr = &ics_ptr->pulse [p];
         pulse_ptr->offset = MLO_BitStream_ReadBits (bit_ptr, 5);
         pulse_ptr->amp = MLO_BitStream_ReadBits (bit_ptr, 4);
      }
   }

   return (result);
}



/*
==============================================================================
Name: MLO_IndivChnStream_DecodeSpectralData
Description:
   Decodes Huffman-compressed spectral data.
   Ref:
      4.4.2.7, Table 4.50
      4.5.2.3.5
      4.6.3
      4.6.9
Input/output parameters:
	- ics_ptr: Information for the channel. Requires ics_info, window grouping
      information and Section Data to be filled.
	- bits_ptr: Input bitstream to decode
Returns:
   MLO_SUCCESS if ok
   MLO_ERROR_UNEXPECTED_CB_TYPE for an unexpected codebook type (12, reserved).
==============================================================================
*/

MLO_Result  MLO_IndivChnStream_DecodeSpectralData (MLO_IndivChnStream *ics_ptr, MLO_BitStream *bit_ptr)
{
   MLO_Result     result = MLO_SUCCESS;
   int            g;
   int            group_pos = 0;
   int            num_window_groups;

   MLO_ASSERT (ics_ptr != NULL);
   MLO_ASSERT (bit_ptr != NULL);

   /* Spectral array should be cleaned before entering the decoding stage
      because non-transmitted info above max_sfb is implicitely 0 (4.6.3.3). */
   MLO_SetMemory (&ics_ptr->data [0], 0, sizeof (ics_ptr->data));

   num_window_groups = ics_ptr->ics_info.num_window_groups;
   for (g = 0; g < num_window_groups && MLO_SUCCEEDED (result); ++g)
   {
      int            data_pos = group_pos * MLO_DEFS_FRAME_LEN_SHORT;
      int            i;

      for (i = 0
      ;  i < ics_ptr->section_data.num_sec [g] && MLO_SUCCEEDED (result)
      ;  ++i)
      {
         const MLO_Hcb  sect_cb = ics_ptr->section_data.sect_cb [g] [i];
         const int      sect_start = ics_ptr->section_data.sect_start [g] [i];
         const int      sect_end   = ics_ptr->section_data.sect_end   [g] [i];
         const int      beg = ics_ptr->ics_info.sect_sfb_offset [g] [sect_start];
         const int      end = ics_ptr->ics_info.sect_sfb_offset [g] [sect_end  ];
         MLO_ASSERT (beg <= end);

         if (   sect_cb != MLO_HCB_ZERO_HCB
             && sect_cb != MLO_HCB_NOISE_HCB
             && sect_cb != MLO_HCB_INTENSITY_HCB
             && sect_cb != MLO_HCB_INTENSITY_HCB2)
         {
            const int      inc = (sect_cb >= MLO_HCB_FIRST_PAIR_HCB) ? 2 : 4;
            int            k;

            for (k = beg; k < end && MLO_SUCCEEDED (result); k += inc)
            {
               MLO_ASSERT (data_pos < (int)MLO_ARRAY_SIZE (ics_ptr->data));

               result = MLO_Huffman_decode_spectral_data (
                  &ics_ptr->data [data_pos],
                  bit_ptr,
                  sect_cb
               );

               data_pos += inc;
            }
         }

         else
         {
            data_pos += end - beg;
         }
      }

      group_pos += ics_ptr->ics_info.window_group_length [g];
   }

   return (result);
}
