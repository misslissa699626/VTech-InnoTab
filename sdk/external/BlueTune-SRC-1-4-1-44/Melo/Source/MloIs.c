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
#include "MloConfig.h"
#include "MloDebug.h"
#include "MloElementCpe.h"
#include "MloFloat.h"
#include "MloIndivChnStream.h"
#include "MloIs.h"
#include "MloScaleFactor.h"
#include "MloUtils.h"

/*----------------------------------------------------------------------
|       Prototypes
+---------------------------------------------------------------------*/
static void MLO_Is_ProcessSfb (const MLO_IndivChnStream *ics_l_ptr, MLO_IndivChnStream *ics_r_ptr, int g, int sfb, int win_pos, MLO_Float scale);
static MLO_Float  MLO_Is_ConvIsPositionToGain (int is_position);

/*----------------------------------------------------------------------
|       Functions
+---------------------------------------------------------------------*/
/*
==============================================================================
Name: MLO_Is_Process
Description:
   Does the Intensity Stereo processing.
   Ref:
      4.6.8.2
Input/output parameters:
	- cpe_ptr: channel_pair_element to process
==============================================================================
*/

void  MLO_Is_Process (MLO_ElementCpe *cpe_ptr)
{
   MLO_IndivChnStream *
                  ics_l_ptr;
   MLO_IndivChnStream *
                  ics_r_ptr;
   MLO_Boolean    ms_mask_flag;            
   int            num_window_groups;
   int            max_sfb;
   int            group_pos = 0;
   int            g;

   MLO_ASSERT (cpe_ptr != NULL);

   ics_l_ptr = cpe_ptr->ics_ptr_arr [0];
   ics_r_ptr = cpe_ptr->ics_ptr_arr [1];
   MLO_ASSERT (ics_l_ptr != NULL);
   MLO_ASSERT (ics_r_ptr != NULL);

   ms_mask_flag =
      (cpe_ptr->ms_mask_present == MLO_ELEMENT_CPE_MS_MASK_TYPE_USED);

   num_window_groups = ics_l_ptr->ics_info.num_window_groups;
   max_sfb = ics_l_ptr->ics_info.max_sfb;

   for (g = 0; g < num_window_groups; ++g)
   {
      int            sfb;

      for (sfb = 0; sfb < max_sfb; ++sfb)
      {
         int            is_val =
            MLO_SectionData_IsIntensity (&ics_r_ptr->section_data, g, sfb);
         if (is_val != 0)
         {
            int            pre_scale = is_val;
            int            is = ics_r_ptr->sf_data.scale_factors [g] [sfb];
            MLO_Float      scale = MLO_Is_ConvIsPositionToGain (is);
            if (ms_mask_flag)
            {
               pre_scale *= 1 - 2 * cpe_ptr->ms_used [g] [sfb];
            }
            scale = MLO_Float_MulInt (scale, pre_scale);

            MLO_Is_ProcessSfb (ics_l_ptr, ics_r_ptr, g, sfb, group_pos, scale);
         }
      }

      group_pos +=   ics_l_ptr->ics_info.window_group_length [g]
                   * MLO_DEFS_FRAME_LEN_SHORT;
   }
}



static void MLO_Is_ProcessSfb (const MLO_IndivChnStream *ics_l_ptr, MLO_IndivChnStream *ics_r_ptr, int g, int sfb, int win_pos, MLO_Float scale)
{
   int            win;
   int            sfb_start;
   int            sfb_len;
   int            window_group_length;

   MLO_ASSERT (ics_l_ptr != NULL);
   MLO_ASSERT (ics_r_ptr != NULL);
   MLO_ASSERT (g >= 0);
   MLO_ASSERT (g < 8);
   MLO_ASSERT (sfb >= 0);
   MLO_ASSERT (sfb < ics_l_ptr->ics_info.max_sfb);
   MLO_ASSERT (win_pos >= 0);
   MLO_ASSERT (win_pos < MLO_DEFS_FRAME_LEN_LONG);
   MLO_ASSERT (MLO_SectionData_IsIntensity (&ics_l_ptr->section_data, g, sfb));

   sfb_start = ics_l_ptr->ics_info.swb_offset [sfb];
   sfb_len   = ics_l_ptr->ics_info.swb_offset [sfb + 1] - sfb_start;

   win_pos += sfb_start;
   window_group_length = ics_l_ptr->ics_info.window_group_length [g];

   for (win = 0; win < window_group_length; ++win)
   {
      int            pos_end = win_pos + sfb_len;
      int            i;

      for (i = win_pos; i < pos_end; ++i)
      {
         ics_r_ptr->coef_arr [i] = MLO_Float_Mul (
            ics_l_ptr->coef_arr [i],
            scale
         );
      }
      win_pos += MLO_DEFS_FRAME_LEN_SHORT;
   }
}



/*
==============================================================================
Name: MLO_Is_ConvIsPositionToGain
Description:
   Converts the is_position value to a gain (without sign change).
   gain = 0.5 ^ (0.25 * is_position [g] [sfb]).
   The specs don't mention any input range. To simplify implementation,
   extreme input values are clipped to the range [-125 ; +100].
   The function is quite similare to scale factor decoding.
   Ref:
      4.6.8.2.3
Input parameters:
	- is_position: is_position, PCM value of dpcm_is_position.
Returns:
   Gain value, range [2.98e-8, 2.56e+9].
==============================================================================
*/

static MLO_Float  MLO_Is_ConvIsPositionToGain (int is_position)
{
   MLO_Float      gain;

   is_position = MLO_SCALE_FACTOR_UNITY_GAIN - is_position;
   is_position = MLO_BOUND (
      is_position,
      MLO_SCALE_FACTOR_MIN_VAL,
      MLO_SCALE_FACTOR_MAX_VAL
   );

   gain = MLO_ScaleFactor_ComputeGain (is_position);

   return (gain);
}
