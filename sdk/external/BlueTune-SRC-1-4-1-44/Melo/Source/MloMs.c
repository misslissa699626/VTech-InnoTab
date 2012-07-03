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
#include "MloFloat.h"
#include "MloElementCpe.h"
#include "MloIndivChnStream.h"
#include "MloMs.h"
#include "MloUtils.h"

/*----------------------------------------------------------------------
|       Prototypes
+---------------------------------------------------------------------*/
static void MLO_Ms_ProcessSfb (MLO_IndivChnStream *ics_l_ptr, MLO_IndivChnStream *ics_r_ptr, int g, int sfb, int win_pos);

/*----------------------------------------------------------------------
|       Functions
+---------------------------------------------------------------------*/

/*
==============================================================================
Name: MLO_Ms_Process
Description:
   Does Mid/Side decoding if required.
   This function is called after coefficient rescalling.
Input/output parameters:
	- cpe_ptr: MLO_ElementCpe target object
==============================================================================
*/

void	MLO_Ms_Process (MLO_ElementCpe *cpe_ptr)
{
   MLO_ElementCpe_MsMaskType  ms_mask_present;
   MLO_ASSERT (cpe_ptr != NULL);

   ms_mask_present = cpe_ptr->ms_mask_present;
   if (ms_mask_present != MLO_ELEMENT_CPE_MS_MASK_TYPE_ALL_0)
   {
      MLO_IndivChnStream * ics_l_ptr = cpe_ptr->ics_ptr_arr [0];
      MLO_IndivChnStream * ics_r_ptr = cpe_ptr->ics_ptr_arr [1];
      int            max_sfb = ics_r_ptr->ics_info.max_sfb;
      int            num_window_groups =
         ics_r_ptr->ics_info.num_window_groups;
      int            group_pos = 0;
      int            g;

      for (g = 0; g < num_window_groups; ++g)
      {
         int            sfb;

         for (sfb = 0; sfb < max_sfb; ++sfb)
         {
            if (   (   ms_mask_present == MLO_ELEMENT_CPE_MS_MASK_TYPE_ALL_1
                    || cpe_ptr->ms_used [g] [sfb] != 0)
                /*** To do: optimise the following tests ***/
                && ! MLO_SectionData_IsIntensity (&ics_r_ptr->section_data, g, sfb)
                && ! MLO_SectionData_IsNoise (&ics_r_ptr->section_data, g, sfb))
            {
               MLO_Ms_ProcessSfb (ics_l_ptr, ics_r_ptr, g, sfb, group_pos);
            }
         }

         group_pos +=   ics_r_ptr->ics_info.window_group_length [g]
                      * MLO_DEFS_FRAME_LEN_SHORT;
      }
   }
}



static void MLO_Ms_ProcessSfb (MLO_IndivChnStream *ics_l_ptr, MLO_IndivChnStream *ics_r_ptr, int g, int sfb, int win_pos)
{
   int            sfb_start;
   int            sfb_len;
   int            window_group_length;
   int            win;

   MLO_ASSERT (ics_l_ptr != NULL);
   MLO_ASSERT (ics_r_ptr != NULL);
   MLO_ASSERT (g >= 0);
   MLO_ASSERT (g < 8);
   MLO_ASSERT (sfb >= 0);
   MLO_ASSERT (sfb < ics_r_ptr->ics_info.max_sfb);
   MLO_ASSERT (win_pos >= 0);
   MLO_ASSERT (win_pos < MLO_DEFS_FRAME_LEN_LONG);

   sfb_start = ics_r_ptr->ics_info.swb_offset [sfb];
   sfb_len   = ics_r_ptr->ics_info.swb_offset [sfb + 1] - sfb_start;

   win_pos += sfb_start;
   window_group_length = ics_r_ptr->ics_info.window_group_length [g];

   for (win = 0; win < window_group_length; ++win)
   {
      int            pos_end = win_pos + sfb_len;
      int            i;

      for (i = win_pos; i < pos_end; ++i)
      {
         const MLO_Float   l = ics_l_ptr->coef_arr [i];
         const MLO_Float   r = ics_r_ptr->coef_arr [i];

         ics_l_ptr->coef_arr [i] = MLO_Float_Add (l, r);
         ics_r_ptr->coef_arr [i] = MLO_Float_Sub (l, r);
      }

      win_pos += MLO_DEFS_FRAME_LEN_SHORT;
   }
}
