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
#include "MloIndivChnStream.h"
#include "MloPns.h"
#include "MloScaleFactor.h"
#include "MloUtils.h"

/*----------------------------------------------------------------------
|       Variables
+---------------------------------------------------------------------*/
static MLO_UInt32 MLO_Pns_rand_state = 1;

/*----------------------------------------------------------------------
|       Prototypes
+---------------------------------------------------------------------*/
static void MLO_Pns_ProcessCommon (MLO_IndivChnStream *ics_l_ptr, MLO_IndivChnStream *ics_r_ptr, MLO_ElementCpe *cpe_ptr);
static void MLO_Pns_ProcessSfbSingle (MLO_IndivChnStream *ics_ptr, int g, int sfb, int win_pos);
static void MLO_Pns_CopySfbNoise (const MLO_IndivChnStream *ics_l_ptr, MLO_IndivChnStream *ics_r_ptr, int g, int sfb, int win_pos);
static void MLO_Pns_GenRandVect (MLO_Float *coef_ptr, int len, MLO_Float noise_nrg_gain);
static MLO_Float  MLO_Pns_GenRandVal (void);
static MLO_Int32  MLO_Pns_GenRandValInt (void);
static MLO_Float  MLO_Pns_ConvNoiseNrgToGain (int noise_nrg);

/*----------------------------------------------------------------------
|       Functions
+---------------------------------------------------------------------*/
/*
==============================================================================
Name: MLO_Pns_ProcessSingle
Description:
   Does the Perceptual Noise Shapping part of the spectral processing for a
   single channel (SCE, LFE and CCE)
   Ref:
      4.6.13
Input/output parameters:
	- ics_ptr: individual_channel_stream to process.
==============================================================================
*/

void  MLO_Pns_ProcessSingle (MLO_IndivChnStream *ics_ptr)
{
   MLO_ASSERT (ics_ptr != NULL);

   MLO_Pns_ProcessCommon (ics_ptr, 0, 0);
}



/*
==============================================================================
Name: MLO_Pns_ProcessPair
Description:
   Does the Perceptual Noise Shapping part of the spectral processing for a
   CPE.
   Ref:
      4.6.13
Input/output parameters:
	- cpe_ptr: channel_pair_element to process
==============================================================================
*/

void  MLO_Pns_ProcessPair (MLO_ElementCpe *cpe_ptr)
{
   MLO_ASSERT (cpe_ptr != NULL);
   MLO_ASSERT (cpe_ptr->ics_ptr_arr [0] != 0);
   MLO_ASSERT (cpe_ptr->ics_ptr_arr [1] != 0);

   MLO_Pns_ProcessCommon (cpe_ptr->ics_ptr_arr [0], cpe_ptr->ics_ptr_arr [1], cpe_ptr);
}



/*
==============================================================================
Name: MLO_Pns_ProcessCommon
Description:
   Does the Perceptual Noise Shapping part of the spectral processing. This
   function is common for both single channels and channel pairs.
   Ref:
      4.6.13
Input/output parameters:
	- ics_l_ptr: Single channel or Left channel of a pair.
	- ics_r_ptr: Right channel of a pair. Required only to process channel
      pairs. Must be 0 for single channels.
	- cpe_ptr: channel_pair_element. Required only to process channel pairs.
      Must be 0 for single channels.
==============================================================================
*/

static void  MLO_Pns_ProcessCommon (MLO_IndivChnStream *ics_l_ptr, MLO_IndivChnStream *ics_r_ptr, MLO_ElementCpe *cpe_ptr)
{
   int            num_window_groups;
   int            max_sfb;
   int            group_pos = 0;
   int            g;
   MLO_ElementCpe_MsMaskType
                  ms_mask_present = MLO_ELEMENT_CPE_MS_MASK_TYPE_ALL_0;

   MLO_ASSERT (ics_l_ptr != NULL);
   MLO_ASSERT (ics_r_ptr == NULL || cpe_ptr != NULL);

   num_window_groups = ics_l_ptr->ics_info.num_window_groups;
   max_sfb = ics_l_ptr->ics_info.max_sfb;

   if (cpe_ptr != 0)
   {
      ms_mask_present = cpe_ptr->ms_mask_present;
   }

   for (g = 0; g < num_window_groups; ++g)
   {
      int            sfb;

      for (sfb = 0; sfb < max_sfb; ++sfb)
      {
         MLO_Boolean    noise_l_flag =
            MLO_SectionData_IsNoise (&ics_l_ptr->section_data, g, sfb);
         if (noise_l_flag)
         {
            MLO_Pns_ProcessSfbSingle (ics_l_ptr, g, sfb, group_pos);
         }

         if (   ics_r_ptr != 0
             && MLO_SectionData_IsNoise (&ics_r_ptr->section_data, g, sfb))
         {
            if (   (   ms_mask_present == MLO_ELEMENT_CPE_MS_MASK_TYPE_ALL_1
                    || (   ms_mask_present == MLO_ELEMENT_CPE_MS_MASK_TYPE_USED
                        && cpe_ptr->ms_used [g] [sfb]))
                && noise_l_flag)
            {
               MLO_Pns_CopySfbNoise (ics_l_ptr, ics_r_ptr, g, sfb, group_pos);
            }
            else
            {
               MLO_Pns_ProcessSfbSingle (ics_r_ptr, g, sfb, group_pos);
            }
         }
      }

      group_pos +=   ics_l_ptr->ics_info.window_group_length [g]
                   * MLO_DEFS_FRAME_LEN_SHORT;
   }
}



/*
==============================================================================
Name: MLO_Pns_ProcessSfbSingle
Description:
   Generates noise for all the windows of a specified group/SFB.
Input parameters:
	- g: Window group, [0 ; num_window_groups[
	- sfb: [0 ; max_sfb[
	- win_pos: Position of the beginning of the first window of the group, in
      the spectral coefficients.
Input/output parameters:
	- ics_ptr: individual_channel_stream where noise should be generated.
==============================================================================
*/

static void MLO_Pns_ProcessSfbSingle (MLO_IndivChnStream *ics_ptr, int g, int sfb, int win_pos)
{
   int            win;
   int            sfb_start;
   int            sfb_len;
   int            noise_nrg;
   MLO_Float      noise_nrg_gain;
   int            window_group_length;

   MLO_ASSERT (ics_ptr != NULL);
   MLO_ASSERT (g >= 0);
   MLO_ASSERT (g < 8);
   MLO_ASSERT (sfb >= 0);
   MLO_ASSERT (sfb < ics_ptr->ics_info.max_sfb);
   MLO_ASSERT (win_pos >= 0);
   MLO_ASSERT (win_pos < MLO_DEFS_FRAME_LEN_LONG);
   MLO_ASSERT (MLO_SectionData_IsNoise (&ics_ptr->section_data, g, sfb));

   noise_nrg = ics_ptr->sf_data.scale_factors [g] [sfb];
   noise_nrg_gain = MLO_Pns_ConvNoiseNrgToGain (noise_nrg);

   sfb_start = ics_ptr->ics_info.swb_offset [sfb];
   sfb_len   = ics_ptr->ics_info.swb_offset [sfb + 1] - sfb_start;

   win_pos += sfb_start;
   window_group_length = ics_ptr->ics_info.window_group_length [g];

   for (win = 0; win < window_group_length; ++win)
   {
      MLO_Pns_GenRandVect (
         &ics_ptr->coef_arr [win_pos],
         sfb_len,
         noise_nrg_gain
      );
      win_pos += MLO_DEFS_FRAME_LEN_SHORT;
   }
}



/*
==============================================================================
Name: MLO_Pns_CopySfbNoise
Description:
   Copies noise of the left channel on the right channel, for all the windows
   of a specified group/SFB.
Input parameters:
	- ics_l_ptr: Left individual_channel_stream
	- g: Window group, [0 ; num_window_groups[
	- sfb: [0 ; max_sfb[
	- win_pos: Position of the beginning of the first window of the group, in
      the spectral coefficients.
Input/output parameters:
	- ics_r_ptr: Right individual_channel_stream
==============================================================================
*/

static void MLO_Pns_CopySfbNoise (const MLO_IndivChnStream *ics_l_ptr, MLO_IndivChnStream *ics_r_ptr, int g, int sfb, int win_pos)
{
   int            win;
   int            sfb_start;
   int            sfb_len;
   int            window_group_length;

   MLO_ASSERT (ics_l_ptr != NULL);
   MLO_ASSERT (ics_r_ptr != NULL);
   MLO_ASSERT (g >= 0);
   MLO_ASSERT (g < ics_l_ptr->ics_info.num_window_groups);
   MLO_ASSERT (sfb >= 0);
   MLO_ASSERT (sfb < ics_l_ptr->ics_info.max_sfb);
   MLO_ASSERT (win_pos >= 0);
   MLO_ASSERT (win_pos < MLO_DEFS_FRAME_LEN_LONG);
   MLO_ASSERT (MLO_SectionData_IsNoise (&ics_l_ptr->section_data, g, sfb));

   sfb_start = ics_l_ptr->ics_info.swb_offset [sfb];
   sfb_len   = ics_l_ptr->ics_info.swb_offset [sfb + 1] - sfb_start;

   win_pos += sfb_start;
   window_group_length = ics_l_ptr->ics_info.window_group_length [g];

   for (win = 0; win < window_group_length; ++win)
   {
      MLO_CopyMemory (
         &ics_r_ptr->coef_arr [win_pos],
         &ics_l_ptr->coef_arr [win_pos],
         sfb_len * sizeof (ics_r_ptr->coef_arr [win_pos])
      );
      win_pos += MLO_DEFS_FRAME_LEN_SHORT;
   }
}



/*
==============================================================================
Name: MLO_Pns_GenRandVect
Description:
   Generate a vector containing random values. Energy of this vector is
   specified by the caller.
Input parameters:
	- len: Number of sample to generate.
	- noise_nrg_gain: Desired energy (sum of squares) for the vector
Output parameters:
	- coef_ptr: Array containing the generated values.
==============================================================================
*/

static void MLO_Pns_GenRandVect (MLO_Float *coef_ptr, int len, MLO_Float noise_nrg_gain)
{
   int            i;
   MLO_Float      energy_sum = 0;
   MLO_Float      rms;
   MLO_Float      scale;

   MLO_ASSERT (coef_ptr != NULL);
   MLO_ASSERT (len > 0);
   MLO_ASSERT (len <= MLO_DEFS_FRAME_LEN_LONG);

   /* Generates random vector */
   for (i = 0; i < len; ++i)
   {
      const MLO_Float   coef = MLO_Pns_GenRandVal ();
      coef_ptr [i] = coef;
      energy_sum = MLO_Float_Add (energy_sum, MLO_Float_Mul (coef, coef));
   }

   rms = MLO_Float_Sqrt (energy_sum);

   /* Rescales vector to match required energy */
   scale = MLO_Float_Div (noise_nrg_gain, rms);
   for (i = 0; i < len; ++i)
   {
      coef_ptr [i] = MLO_Float_Mul (coef_ptr [i], scale);
   }
}




/*
==============================================================================
Name: MLO_Pns_GenRandVal
Description:
   Generates a floating-point random value.
   Scaling has no importance, we just should be able to store and accumulate
   the square of the result into a MLO_Float.
Returns:
   The random value, here in range [-2^31 ; 2^31-1] for floating point
   implementation, [-0.5 ; 0.5] for fixed point.
==============================================================================
*/

static MLO_Float  MLO_Pns_GenRandVal ()
{
#if defined (MLO_CONFIG_FIXED)
   return ((MLO_Float) MLO_Pns_GenRandValInt ());
#else
   return (MLO_Float_ConvIntToFloat (MLO_Pns_GenRandValInt ()));
#endif
}



/*
==============================================================================
Name: MLO_Pns_GenRandValInt
Description:
   Generates an integer random value. Period of the generator is 2^32-1,
   which should be enough for the purpose.
Returns:
   The random value, range [-2^31 ; 2^31-1]
==============================================================================
*/

static MLO_Int32  MLO_Pns_GenRandValInt ()
{
   MLO_Int32      r = (MLO_Int32) MLO_Pns_rand_state;
   MLO_Pns_rand_state = MLO_Pns_rand_state * 1234567UL + 890123UL;

   return (r);
}



/*
==============================================================================
Name: MLO_Pns_ConvNoiseNrgToGain
Description:
   Converts the noise_nrg value into a gain.
   gain = 2.0 ^ (0.25 * noise_nrg [g] [sfb])
   The specs don't mention any input range. To simplify implementation,
   extreme input values are clipped to the range [-100 ; +125].
   The function is quite similare to scale factor decoding.
   Ref:
      4.6.13.3
Input parameters:
	- noise_nrg: noise_nrg value for a group/SFB
Returns:
   Gain value, range [2.98e-8, 2.56e+9].
==============================================================================
*/

static MLO_Float  MLO_Pns_ConvNoiseNrgToGain (int noise_nrg)
{
   MLO_Float      gain;

   noise_nrg += MLO_SCALE_FACTOR_UNITY_GAIN;
   noise_nrg = MLO_BOUND (
      noise_nrg,
      MLO_SCALE_FACTOR_MIN_VAL,
      MLO_SCALE_FACTOR_MAX_VAL
   );

   gain = MLO_ScaleFactor_ComputeGain (noise_nrg);

   return (gain);
}
