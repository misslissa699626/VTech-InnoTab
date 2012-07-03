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
#include "MloElementCce.h"
#include "MloFloat.h"
#include "MloHcbScaleFactor.h"
#include "MloIndivChnPool.h"
#include "MloIndivChnStream.h"
#include "MloPns.h"
#include "MloSyntacticElements.h"
#include "MloUtils.h"

/*----------------------------------------------------------------------
|       Types
+---------------------------------------------------------------------*/
typedef  MLO_Float   MLO_ElementCce_LinearGainArray [MLO_DEFS_MAX_NBR_WIN_GRP] [MLO_DEFS_MAX_NUM_SWB + 1]; /* [g] [sfb] */

/*----------------------------------------------------------------------
|       Data
+---------------------------------------------------------------------*/
static const MLO_Float MLO_ElementCce_table_pow2 [MLO_ELEMENT_CCE_POW2_RES] =
{
   MLO_FLOAT_C (1),
   MLO_FLOAT_C (1.09050773),
   MLO_FLOAT_C (1.18920712),
   MLO_FLOAT_C (1.29683955),
   MLO_FLOAT_C (1.41421356),
   MLO_FLOAT_C (1.54221083),
   MLO_FLOAT_C (1.68179283),
   MLO_FLOAT_C (1.83400809)
};

/*----------------------------------------------------------------------
|       Prototypes
+---------------------------------------------------------------------*/
static int  MLO_ElementCce_DecodeTargets (MLO_ElementCce *cce_ptr, MLO_BitStream *bit_ptr);
static void MLO_ElementCce_DecodeGainElementList (MLO_ElementCce_GainElementList *gel_ptr, MLO_BitStream *bit_ptr, const MLO_IcsInfo *ics_info_ptr, MLO_Boolean cge_flag);
static void MLO_ElementCce_CoupleChannel (const MLO_ElementCce *cce_ptr, MLO_IndivChnStream *ics_ptr, int list_index, MLO_ElementCce_Stage stage);
static void MLO_ElementCce_CoupleChannelNoGain (const MLO_ElementCce *cce_ptr, MLO_IndivChnStream *ics_ptr);
static void MLO_ElementCce_CalculateGains (const MLO_ElementCce *cce_ptr, int list_index, MLO_ElementCce_LinearGainArray gain_arr);
static MLO_Float  MLO_ElementCce_CalculateLinearGain (int scale_log, int cge);
static void MLO_ElementCce_ApplyChannelCouplingDep (const MLO_ElementCce *cce_ptr, MLO_IndivChnStream *ics_ptr, MLO_ElementCce_LinearGainArray gain_arr);
static void MLO_ElementCce_ApplyChannelCouplingIndep (const MLO_ElementCce *cce_ptr, MLO_IndivChnStream *ics_ptr, int list_index);

/*----------------------------------------------------------------------
|       Functions
+---------------------------------------------------------------------*/
/*
==============================================================================
Name: MLO_ElementCce_Decode
Description:
   Decodes a coupling_channel_element and processes spectral data up to
   Perceptual Noise Substitution (PNS) stage.
   Ref: 4.4.2.1, Table 4.8
Input parameters:
	- fs_index: Valid Sampling Frequency Index for the frame
Input/output parameters:
	- cce_ptr: MLO_ElementCce structure
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
MLO_ElementCce_Decode (MLO_ElementCce *cce_ptr, MLO_BitStream *bit_ptr, MLO_IndivChnPool *chn_pool_ptr, MLO_SamplingFreq_Index fs_index)
{
    MLO_Result     result = MLO_SUCCESS;
    int            num_gain_element_lists;
    int            ics_index = -1;
    MLO_IndivChnStream * ics_ptr = 0;

	MLO_ASSERT (cce_ptr != NULL);
	MLO_ASSERT (bit_ptr != NULL);
	MLO_ASSERT (chn_pool_ptr != NULL);
    MLO_CHECK_ARGS(fs_index >= 0);
    MLO_CHECK_ARGS(fs_index < MLO_SAMPLING_FREQ_INDEX_NBR_VALID);

   cce_ptr->element_instance_tag = MLO_BitStream_ReadBits (bit_ptr, 4);
   cce_ptr->ind_sw_cce_flag      = MLO_BitStream_ReadBit (bit_ptr);
   cce_ptr->num_coupled_elements = MLO_BitStream_ReadBits (bit_ptr, 3);

   num_gain_element_lists = MLO_ElementCce_DecodeTargets (cce_ptr, bit_ptr);
   cce_ptr->num_gain_element_lists = num_gain_element_lists;

   cce_ptr->cc_domain          = MLO_BitStream_ReadBit (bit_ptr);
   cce_ptr->gain_element_sign  = MLO_BitStream_ReadBit (bit_ptr);
   cce_ptr->gain_element_scale = MLO_BitStream_ReadBits (bit_ptr, 2);

   result = MLO_IndivChnPool_AddChn (chn_pool_ptr, &ics_index);

   if (MLO_SUCCEEDED (result))
   {
      ics_ptr = chn_pool_ptr->chn_ptr_arr [ics_index];
      cce_ptr->ics_ptr = ics_ptr;

      result = MLO_IndivChnStream_Decode (
         ics_ptr,
         bit_ptr,
         MLO_FALSE,
         MLO_FALSE,
         fs_index
      );
   }

   if (MLO_SUCCEEDED (result))
   {
      int            c;
      for (c = 1; c < num_gain_element_lists; ++c)
      {
         MLO_ElementCce_DecodeGainElementList (
            &cce_ptr->gel_arr [c],
            bit_ptr,
            &ics_ptr->ics_info,
            cce_ptr->ind_sw_cce_flag
         );
      }

      /* Other processings */
      MLO_Pns_ProcessSingle (ics_ptr);
   }

   return (result);
}



/* Ref: 4.6.8.3.3 */
MLO_Result  
MLO_ElementCce_Process (const MLO_ElementCce *cce_ptr, struct MLO_SyntacticElements *se_ptr, MLO_ElementCce_Stage stage)
{
   MLO_Result     result = MLO_SUCCESS;
   MLO_Boolean    proc_flag = MLO_FALSE;

   MLO_ASSERT(cce_ptr != NULL);
   MLO_ASSERT(se_ptr != NULL);
   MLO_CHECK_ARGS(stage < MLO_ELEMENT_CCE_STAGE_NBR_ELT);

   switch (stage)
   {
   case  MLO_ELEMENT_CCE_STAGE_DEP_BEFORE_TNS:
		proc_flag = (! cce_ptr->ind_sw_cce_flag && ! cce_ptr->cc_domain);
      break;
   case  MLO_ELEMENT_CCE_STAGE_DEP_AFTER_TNS:
		proc_flag = (! cce_ptr->ind_sw_cce_flag && cce_ptr->cc_domain);
      break;
   case  MLO_ELEMENT_CCE_STAGE_INDEP:
		proc_flag = cce_ptr->ind_sw_cce_flag;
      break;
	default:
		return MLO_ERROR_INVALID_PARAMETERS;
		break;
   }

   if (proc_flag)
   {
      int            list_index = 0;
      int            nbr_targets = cce_ptr->num_coupled_elements + 1;
      int            target;

      for (target = 0
      ;  target < nbr_targets && MLO_SUCCEEDED (result)
      ;  ++target)
      {
         const MLO_ElementCce_CcTarget *  target_ptr =
            &cce_ptr->cc_target_arr [target];
         int            tag = target_ptr->tag_select;

         /* channel_pair_element */
         if (target_ptr->is_cpe)
         {
            MLO_ElementCpe *  cpe_ptr;
            result = MLO_SyntacticElements_UseCpe (se_ptr, tag, &cpe_ptr);
            if (MLO_SUCCEEDED (result))
            {
               /* Shared coupling channle */
               if (! target_ptr->cc_l && ! target_ptr->cc_r)
               {
                  MLO_ElementCce_CoupleChannel (
                     cce_ptr,
                     cpe_ptr->ics_ptr_arr [0],
                     list_index,
                     stage
                  );
                  MLO_ElementCce_CoupleChannel (
                     cce_ptr,
                     cpe_ptr->ics_ptr_arr [1],
                     list_index,
                     stage
                  );
                  ++ list_index;
               }

               /* Coupling info for each channel */
               else
               {
                  if (target_ptr->cc_l)
                  {
                     MLO_ElementCce_CoupleChannel (
                        cce_ptr,
                        cpe_ptr->ics_ptr_arr [0],
                        list_index,
                        stage
                     );
                     ++ list_index;
                  }
                  if (target_ptr->cc_r)
                  {
                     MLO_ElementCce_CoupleChannel (
                        cce_ptr,
                        cpe_ptr->ics_ptr_arr [1],
                        list_index,
                        stage
                     );
                     ++ list_index;
                  }
               }
            }
         }

         /* single_channel_element */
         else
         {
            MLO_ElementSceLfe *  sce_ptr;
            result = MLO_SyntacticElements_UseSce (se_ptr, tag, &sce_ptr);
            if (MLO_SUCCEEDED (result))
            {
               MLO_ElementCce_CoupleChannel (
                  cce_ptr,
                  sce_ptr->ics_ptr,
                  list_index,
                  stage
               );
               ++ list_index;
            }
         }
      }
   }

   return (result);
}



/*
==============================================================================
Name: MLO_ElementCce_DecodeTargets
Description:
   Decode target channel lists
Input/output parameters:
	- cce_ptr: MLO_ElementCce structure
	- bit_ptr: Input bitstream to decode
Returns: Number of gain elements, >= 1
==============================================================================
*/

static int  MLO_ElementCce_DecodeTargets (MLO_ElementCce *cce_ptr, MLO_BitStream *bit_ptr)
{
    int            num_gain_element_lists = 0;
    int            nbr_targets;
    int            target_cnt;

    MLO_ASSERT (cce_ptr != NULL);
	MLO_ASSERT (bit_ptr != NULL);

   nbr_targets = cce_ptr->num_coupled_elements + 1;
   MLO_CHECK_DATA(nbr_targets <= (int)MLO_ARRAY_SIZE (cce_ptr->cc_target_arr));

   for (target_cnt = 0; target_cnt < nbr_targets; ++target_cnt)
   {
      MLO_ElementCce_CcTarget *  target_ptr =
         &cce_ptr->cc_target_arr [target_cnt];
      target_ptr->is_cpe = MLO_BitStream_ReadBit (bit_ptr);
      target_ptr->tag_select = MLO_BitStream_ReadBits (bit_ptr, 4);
      ++ num_gain_element_lists;

      if (target_ptr->is_cpe)
      {
         target_ptr->cc_l = MLO_BitStream_ReadBit (bit_ptr);
         target_ptr->cc_r = MLO_BitStream_ReadBit (bit_ptr);
         if (   target_ptr->cc_l != 0
             && target_ptr->cc_r != 0)
         {
            ++ num_gain_element_lists;
         }
      }
   }

   return (num_gain_element_lists);
}



/*
==============================================================================
Name: MLO_ElementCce_DecodeGainElementList
Description:
Input parameters:
	- ics_info_ptr: current ics_info
	- cge_flag: Indicates if common gain element is present.
Output parameters:
	- gel_ptr: Gain element list to initialise
Input/output parameters:
	- bit_ptr: Input bitstream to decode
==============================================================================
*/

static void
MLO_ElementCce_DecodeGainElementList (MLO_ElementCce_GainElementList *gel_ptr, MLO_BitStream *bit_ptr, const MLO_IcsInfo *ics_info_ptr, MLO_Boolean cge_flag)
{
	MLO_ASSERT(gel_ptr != NULL);
	MLO_ASSERT(bit_ptr != NULL);
	MLO_ASSERT(ics_info_ptr != NULL);

   if (! cge_flag)
   {
      cge_flag = (MLO_Boolean) MLO_BitStream_ReadBit (bit_ptr);
   }
   gel_ptr->cge_flag = cge_flag;

   /* Common */
   if (cge_flag)
   {
      gel_ptr->common_gain_element = MLO_HcbScaleFactor_decode (bit_ptr);
   }

   /* Differential */
   else
   {
      const int      max_sfb = ics_info_ptr->max_sfb;
      const int      num_window_groups = ics_info_ptr->num_window_groups;
      int            g;

      MLO_ASSERT(num_window_groups <= (int)MLO_ARRAY_SIZE (gel_ptr->dpcm_gain_element));
      MLO_ASSERT(max_sfb           <= (int)MLO_ARRAY_SIZE (gel_ptr->dpcm_gain_element [0]));

      for (g = 0; g < num_window_groups; ++g)
      {
         int            sfb;

         for (sfb = 0; sfb < max_sfb; ++sfb)
         {
            gel_ptr->dpcm_gain_element [g] [sfb] =
               MLO_HcbScaleFactor_decode (bit_ptr);
         }
      }
   }
}



/* Ref: 4.6.8.3.3 */
static void
MLO_ElementCce_CoupleChannel (const MLO_ElementCce *cce_ptr, MLO_IndivChnStream *ics_ptr, int list_index, MLO_ElementCce_Stage stage)
{
	MLO_ASSERT(cce_ptr != NULL);
	MLO_ASSERT(ics_ptr != NULL);
	MLO_ASSERT(list_index >= 0);
    MLO_ASSERT(list_index < cce_ptr->num_gain_element_lists);
    MLO_ASSERT(stage < MLO_ELEMENT_CCE_STAGE_NBR_ELT);

   if (list_index == 0)
   {
      MLO_ElementCce_CoupleChannelNoGain (cce_ptr, ics_ptr);
   }
   else
   {
      /* Dependently switched coupling */
      if (stage != MLO_ELEMENT_CCE_STAGE_INDEP)
      {
         MLO_ElementCce_LinearGainArray   gain_arr;

         /* Decode coupling gain elements for this group */
         MLO_ElementCce_CalculateGains (cce_ptr, list_index, gain_arr);

         /* Do coupling onto target channels */
         MLO_ElementCce_ApplyChannelCouplingDep (cce_ptr, ics_ptr, gain_arr);
		}

      /* Independently switched coupling */
      else
      {
         MLO_ElementCce_ApplyChannelCouplingIndep (cce_ptr, ics_ptr, list_index);
      }
   }
}



/* For both spectral and time domains */
static void 
MLO_ElementCce_CoupleChannelNoGain (const MLO_ElementCce *cce_ptr, MLO_IndivChnStream *ics_ptr)
{
   MLO_Float *    cc_coef_arr;
   int            pos;

   MLO_ASSERT (cce_ptr != NULL);
   MLO_ASSERT (ics_ptr != NULL);

   cc_coef_arr = &cce_ptr->ics_ptr->coef_arr [0];
   for (pos = 0; pos < MLO_DEFS_FRAME_LEN_LONG; ++pos)
   {
      ics_ptr->coef_arr [pos] =
         MLO_Float_Add (ics_ptr->coef_arr [pos], cc_coef_arr [pos]);
   }
}



static void
MLO_ElementCce_CalculateGains (const MLO_ElementCce *cce_ptr, int list_index, MLO_ElementCce_LinearGainArray gain_arr)
{
    const MLO_ElementCce_GainElementList * gel_ptr;
    MLO_IndivChnStream * ics_ptr;
    int            scale_log;           /* val = 2 ^ (scale_log/8 * gain) */
    int            num_window_groups;
    int            max_sfb;

    MLO_ASSERT (cce_ptr != NULL);
	MLO_ASSERT (list_index >= 0);
    MLO_ASSERT (list_index < cce_ptr->num_gain_element_lists);
	MLO_ASSERT (gain_arr != 0);

   gel_ptr = &cce_ptr->gel_arr [list_index];
   scale_log = 1 << cce_ptr->gain_element_scale;

   ics_ptr = cce_ptr->ics_ptr;
   num_window_groups = ics_ptr->ics_info.num_window_groups;
   max_sfb = ics_ptr->ics_info.max_sfb;

   /* common_gain_element_present */
   if (gel_ptr->cge_flag)
   {
      const MLO_Float   gain = MLO_ElementCce_CalculateLinearGain (
         scale_log,
         gel_ptr->common_gain_element
      );
      int            g;

      for (g = 0; g < num_window_groups; ++g)
      {
         int            sfb;

         for (sfb = 0; sfb < max_sfb; ++sfb)
         {
            gain_arr [g] [sfb] = gain;
         }
      }
   }

   /* Specified gains for each SFB */
   else
   {
      int            a = 0;
      MLO_Boolean    sign_flag = cce_ptr->gain_element_sign;
      int            g;

      for (g = 0; g < num_window_groups; ++g)
      {
         int            sfb;

         for (sfb = 0; sfb < max_sfb; ++sfb)
         {
            MLO_Float      linear_gain;
            const int      dpcm = gel_ptr->dpcm_gain_element [g] [sfb];
            int            sign = 1;
            if (sign_flag)
            {
               sign = 1 - 2 * (dpcm & 1);
               a += (dpcm >> 1) - (60 >> 1);
            }
            else
            {
               a += dpcm - 60;
            }

            linear_gain = MLO_ElementCce_CalculateLinearGain (scale_log, a);
            gain_arr [g] [sfb] = MLO_Float_MulInt (linear_gain, sign);
         }
      }
   }
}



/* Returns 2 ^ (cge * scale_log / 8) */
static MLO_Float  MLO_ElementCce_CalculateLinearGain (int scale_log, int cge)
{
   const int      cge_s = cge * scale_log;
   const int      cge_s_f = cge_s & (MLO_ELEMENT_CCE_POW2_RES - 1);
   const int      cge_s_i = cge_s >> MLO_ELEMENT_CCE_POW2_RES_L2;
   const MLO_Float   gain = MLO_Float_ScaleP2 (
      MLO_ElementCce_table_pow2 [cge_s_f],
      cge_s_i
   );

   MLO_ASSERT (cge_s >= 0);
   MLO_ASSERT (MLO_SIGN (cge_s_i) == MLO_SIGN (cge_s));

   return (gain);
}



/* gain_arr is actually const */
static void 
MLO_ElementCce_ApplyChannelCouplingDep (const MLO_ElementCce *cce_ptr, MLO_IndivChnStream *ics_ptr, MLO_ElementCce_LinearGainArray gain_arr)
{
    int            num_window_groups;
    int            max_sfb;
    int            group_pos = 0;
    int            g;
    MLO_Float *    cc_coef_arr;

    MLO_ASSERT(cce_ptr != NULL);
	MLO_ASSERT(ics_ptr != NULL);
	MLO_ASSERT(gain_arr != 0);

   cc_coef_arr = &cce_ptr->ics_ptr->coef_arr [0];
   num_window_groups = ics_ptr->ics_info.num_window_groups;
   max_sfb = ics_ptr->ics_info.max_sfb;

   for (g = 0; g < num_window_groups; ++g)
   {
      int            sfb;

      for (sfb = 0; sfb < max_sfb; ++sfb)
      {
         int            sfb_start = ics_ptr->ics_info.swb_offset [sfb];
         int            sfb_len   = ics_ptr->ics_info.swb_offset [sfb + 1] - sfb_start;
         int            win_pos = group_pos + sfb_start;
         int            window_group_length =
            ics_ptr->ics_info.window_group_length [g];
         MLO_Float      gain = gain_arr [g] [sfb];
         int            win;

         for (win = 0; win < window_group_length; ++win)
         {
            int            pos_end = win_pos + sfb_len;
            int            i;

            for (i = win_pos; i < pos_end; ++i)
            {
               const MLO_Float   g = MLO_Float_Mul (cc_coef_arr [i], gain);
               ics_ptr->coef_arr [i] = MLO_Float_Add (ics_ptr->coef_arr [i], g);
            }
            win_pos += MLO_DEFS_FRAME_LEN_SHORT;
         }
      }

      group_pos +=   ics_ptr->ics_info.window_group_length [g]
                   * MLO_DEFS_FRAME_LEN_SHORT;
   }
}

static void 
MLO_ElementCce_ApplyChannelCouplingIndep (const MLO_ElementCce *cce_ptr, MLO_IndivChnStream *ics_ptr, int list_index)
{
    int            scale_log;
    MLO_Float      gain;
    MLO_Float *    cc_coef_arr;
    int            pos = 0;

    MLO_ASSERT(cce_ptr != NULL);
	MLO_ASSERT(ics_ptr != NULL);
	MLO_ASSERT(list_index >= 0);
    MLO_ASSERT(list_index < cce_ptr->num_gain_element_lists);

    scale_log = 1 << cce_ptr->gain_element_scale;
    gain = MLO_ElementCce_CalculateLinearGain (
      scale_log,
      cce_ptr->gel_arr [list_index].common_gain_element
    );

    cc_coef_arr = &cce_ptr->ics_ptr->coef_arr [0];
    for (pos = 0; pos < MLO_DEFS_FRAME_LEN_LONG; ++pos)
    {
        const MLO_Float   g = MLO_Float_Mul (cc_coef_arr [pos], gain);
        ics_ptr->coef_arr [pos] = MLO_Float_Add (ics_ptr->coef_arr [pos], g);
    }
}
