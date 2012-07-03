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
#include "MloConfig.h"
#include "MloDebug.h"
#include "MloDefs.h"
#include "MloFloat.h"
#include "MloHcb.h"
#include "MloHcbScaleFactor.h"
#include "MloIcsInfo.h"
#include "MloScaleFactor.h"
#include "MloSectionData.h"
#include "MloUtils.h"

#if defined(MLO_CONFIG_HAVE_MATH_H)
#include <math.h>
#endif

/*----------------------------------------------------------------------
|       Data
+---------------------------------------------------------------------*/
static const MLO_Float MLO_ScaleFactor_table_pow2  [1 << 2] =
{
   MLO_FLOAT_C (1),
   MLO_FLOAT_C (1.18920712),
   MLO_FLOAT_C (1.41421356),
   MLO_FLOAT_C (1.68179283)
};

/*----------------------------------------------------------------------
|       Functions
+---------------------------------------------------------------------*/

/*
==============================================================================
Name: MLO_ScaleFactor_Decode
Description:
   Decodes scale_factor_data section.
   Perceptual Noise Substitution (PNS) and Intensity Stereo (IS) data are
   directly converted to PCM.
   Ref:
      4.4.2.7, Table 4.47
      4.6.2
      4.6.8.1.4
Input parameters:
	- ics_ptr: ics_info data
   - sec_ptr: section_data content (for the codebook index list)
   - global_gain: gain for the channel
Input/output parameters:
	- bits_ptr: Input bitstream to decode
Output parameters:
   - sf_ptr: MLO_ScaleFactor object to be decoded
Returns:
   MLO_SUCCESS if ok
   MLO_ERROR_SCALE_FACTOR_RANGE if a scale factor is out of expected range.
==============================================================================
*/

MLO_Result  MLO_ScaleFactor_Decode (MLO_ScaleFactor *sf_ptr, const MLO_IcsInfo *ics_ptr, const MLO_SectionData *sec_ptr, MLO_BitStream *bit_ptr, int global_gain)
{
   MLO_Result     result = MLO_SUCCESS;
   MLO_Boolean    noise_pcm_flag = MLO_TRUE;
   int            is_position = 0;
   int            sf = global_gain;
   int            noise_nrg = global_gain - 90;
   int            g;
   int            num_windows_groups;

   MLO_ASSERT (sf_ptr != NULL);
   MLO_ASSERT (ics_ptr != NULL);
   MLO_ASSERT (sec_ptr != NULL);
   MLO_ASSERT (bit_ptr != NULL);
   MLO_ASSERT (global_gain >= 0);
   MLO_ASSERT (global_gain <= 255);

   num_windows_groups = ics_ptr->num_window_groups;
   for (g = 0; g < num_windows_groups && MLO_SUCCEEDED (result); ++g)
   {
      int            sfb;
      int            dpcm;

      for (sfb = 0
      ;  sfb < ics_ptr->max_sfb && MLO_SUCCEEDED (result)
      ;  ++ sfb)
      {
         switch (sec_ptr->sfb_cb [g] [sfb])
         {
         case  MLO_HCB_ZERO_HCB:
            sf_ptr->scale_factors [g] [sfb] = 0;
            break;

         case  MLO_HCB_INTENSITY_HCB2:
         case  MLO_HCB_INTENSITY_HCB:
            dpcm = MLO_HcbScaleFactor_decode (bit_ptr) - 60;
            is_position += dpcm;
            sf_ptr->scale_factors [g] [sfb] = is_position;
            break;

         case  MLO_HCB_NOISE_HCB:
            if (noise_pcm_flag)
            {
               noise_pcm_flag = MLO_FALSE;
               dpcm = MLO_BitStream_ReadBits (bit_ptr, 9) - 256;
            }
            else
            {
               dpcm = MLO_HcbScaleFactor_decode (bit_ptr) - 60;
            }
            noise_nrg += dpcm;
            sf_ptr->scale_factors [g] [sfb] = noise_nrg;
            break;

         default:
            dpcm = MLO_HcbScaleFactor_decode (bit_ptr) - 60;
            sf += dpcm;
            if (   sf < MLO_SCALE_FACTOR_MIN_VAL
                || sf > MLO_SCALE_FACTOR_MAX_VAL)
            {
               result = MLO_ERROR_SCALE_FACTOR_RANGE;
            }
            else
            {
               sf_ptr->scale_factors [g] [sfb] = sf;
            }
            break;
         }
      }
   }

   return (result);
}



/*
==============================================================================
Name: MLO_ScaleFactor_ScaleCoefficients
Description:
   Applies scaling to spectrum coefficients of a individual_channel_stream.
   Ref:
      4.6.2.3.3
Input parameters:
	- sf_ptr: Scale factor data
	- ics_ptr: ics_info of the single_channel_element
Input/output parameters:
	- coef_ptr: Array of scaled coefficients of the individual_channel_stream,
      with SFB/window interleaved
==============================================================================
*/

void  MLO_ScaleFactor_ScaleCoefficients (const MLO_ScaleFactor *sf_ptr, const MLO_IcsInfo *ics_ptr, MLO_Float coef_ptr [])
{
   int            g;
   int            group_pos = 0;
   int            num_window_groups;
   int            max_sfb;

   MLO_ASSERT (sf_ptr != NULL);
   MLO_ASSERT (ics_ptr != NULL);
   MLO_ASSERT (coef_ptr != NULL);

   num_window_groups = ics_ptr->num_window_groups;
   max_sfb = ics_ptr->max_sfb;

   for (g = 0; g < num_window_groups; ++g)
   {
      int            coef_pos = group_pos * MLO_DEFS_FRAME_LEN_SHORT;
      int            sfb;

      for (sfb = 0; sfb < max_sfb; ++sfb)
      {
         const int      sf = sf_ptr->scale_factors [g] [sfb];
         const MLO_Float   gain = MLO_ScaleFactor_ComputeGain (sf);
         const int      end =
              group_pos * MLO_DEFS_FRAME_LEN_SHORT
            + ics_ptr->sect_sfb_offset [g] [sfb + 1];

         MLO_ASSERT (end <= MLO_DEFS_FRAME_LEN_LONG);

         while (coef_pos < end)
         {
            coef_ptr [coef_pos] = MLO_Float_Mul (coef_ptr [coef_pos], gain);
            ++ coef_pos;
         }
      }

      group_pos += ics_ptr->window_group_length [g];
   }
}



/*
==============================================================================
Name: MLO_ScaleFactor_ComputeGain
Description:
   Computes the gain for a Scale Factor:
      gain = 2 ^ (0.25 * (sf - 100))
   The fuction can be used to compute other gains by means of an offset,
   for Perceptual Noise Substitution (PNS) or Intensity Stereo (IS).
Input parameters:
	- sf: Scale Factor, range [0 ; 255]. 100 gives the unity gain.
Returns: gain value, range [2.980e-8 ; 4.623e+11]
==============================================================================
*/

MLO_Float   MLO_ScaleFactor_ComputeGain (int sf)
{
   MLO_Float      r;
   int            s;

   MLO_ASSERT (sf >= MLO_SCALE_FACTOR_MIN_VAL);
   MLO_ASSERT (sf <= MLO_SCALE_FACTOR_MAX_VAL);

	r = MLO_ScaleFactor_table_pow2 [sf & 3];
   s = (sf >> 2) - (MLO_SCALE_FACTOR_UNITY_GAIN >> 2);
   r = MLO_Float_ScaleP2 (r, s);

   MLO_ASSERT (r > 0);

	return (r);
}
