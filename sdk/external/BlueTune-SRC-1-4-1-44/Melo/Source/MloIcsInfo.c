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
#include "MloIcsInfo.h"
#include "MloUtils.h"

/*----------------------------------------------------------------------
|       Function prototypes
+---------------------------------------------------------------------*/
static MLO_Result MLO_IcsInfo_ComputeWindowGroupingInfo (MLO_IcsInfo *ics_ptr);
static void MLO_IcsInfo_ComputeWindowGroupingInfoLong (MLO_IcsInfo *ics_ptr);
static void MLO_IcsInfo_ComputeWindowGroupingInfoShort (MLO_IcsInfo *ics_ptr);

/*----------------------------------------------------------------------
|       Constants
+---------------------------------------------------------------------*/
/*
Ref: 4.5.4
If size of the swb_offset* arrays are changed, don't forget to change
the sizes in MLO_IcsInfo too.
*/

static const int  MLO_IcsInfo_num_swb_long_window_1024 [MLO_SAMPLING_FREQ_INDEX_NBR_ELT] =
{
   41, 41, 47, 49, 49, 51, 47, 47, 43, 43, 43, 40, 0, 0, 0, 0
};

static const int  MLO_IcsInfo_swb_offset_1024_96 [41 + 1 + 1] =
{
   0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56,
   64, 72, 80, 88, 96, 108, 120, 132, 144, 156, 172, 188, 212, 240,
   276, 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960, 1024,
   -1
};

static const int MLO_IcsInfo_swb_offset_1024_64 [47 + 1 + 1] =
{
   0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56,
   64, 72, 80, 88, 100, 112, 124, 140, 156, 172, 192, 216, 240, 268,
   304, 344, 384, 424, 464, 504, 544, 584, 624, 664, 704, 744, 784, 824,
   864, 904, 944, 984, 1024,
   -1
};

static const int MLO_IcsInfo_swb_offset_1024_48 [49 + 1 + 1] =
{
   0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72,
   80, 88, 96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292,
   320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736,
   768, 800, 832, 864, 896, 928, 1024,
   -1
};

static const int MLO_IcsInfo_swb_offset_1024_32 [51 + 1 + 1] =
{
   0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72,
   80, 88, 96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292,
   320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736,
   768, 800, 832, 864, 896, 928, 960, 992, 1024,
   -1
};

static const int MLO_IcsInfo_swb_offset_1024_24 [47 + 1 + 1] =
{
   0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 52, 60, 68,
   76, 84, 92, 100, 108, 116, 124, 136, 148, 160, 172, 188, 204, 220,
   240, 260, 284, 308, 336, 364, 396, 432, 468, 508, 552, 600, 652, 704,
   768, 832, 896, 960, 1024,
   -1
};

static const int MLO_IcsInfo_swb_offset_1024_16 [43 + 1 + 1] =
{
   0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 100, 112, 124,
   136, 148, 160, 172, 184, 196, 212, 228, 244, 260, 280, 300, 320, 344,
   368, 396, 424, 456, 492, 532, 572, 616, 664, 716, 772, 832, 896, 960, 1024,
   -1
};

static const int MLO_IcsInfo_swb_offset_1024_8 [40 + 1 + 1] =
{
   0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 172,
   188, 204, 220, 236, 252, 268, 288, 308, 328, 348, 372, 396, 420, 448,
   476, 508, 544, 580, 620, 664, 712, 764, 820, 880, 944, 1024,
   -1
};

static const int* MLO_IcsInfo_swb_offset_long_window_1024 [MLO_SAMPLING_FREQ_INDEX_NBR_ELT] =
{
   MLO_IcsInfo_swb_offset_1024_96,  /* 96000 */
   MLO_IcsInfo_swb_offset_1024_96,  /* 88200 */
   MLO_IcsInfo_swb_offset_1024_64,  /* 64000 */
   MLO_IcsInfo_swb_offset_1024_48,  /* 48000 */
   MLO_IcsInfo_swb_offset_1024_48,  /* 44100 */
   MLO_IcsInfo_swb_offset_1024_32,  /* 32000 */
   MLO_IcsInfo_swb_offset_1024_24,  /* 24000 */
   MLO_IcsInfo_swb_offset_1024_24,  /* 22050 */
   MLO_IcsInfo_swb_offset_1024_16,  /* 16000 */
   MLO_IcsInfo_swb_offset_1024_16,  /* 12000 */
   MLO_IcsInfo_swb_offset_1024_16,  /* 11025 */
   MLO_IcsInfo_swb_offset_1024_8,   /*  8000 */
   0,                               /*  7350 */
   0,
   0,
   0
};



static const int  MLO_IcsInfo_num_swb_short_window_128 [MLO_SAMPLING_FREQ_INDEX_NBR_ELT] =
{
    12, 12, 12, 14, 14, 14, 15, 15, 15, 15, 15, 15, 0, 0, 0, 0
};

static const int  MLO_IcsInfo_swb_offset_128_96 [12 + 1 + 1] =
{
    0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 128,
    -1
};

static const int  MLO_IcsInfo_swb_offset_128_64 [12 + 1 + 1] =
{
    0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 128,
    -1
};

static const int  MLO_IcsInfo_swb_offset_128_48 [14 + 1 + 1] =
{
    0, 4, 8, 12, 16, 20, 28, 36, 44, 56, 68, 80, 96, 112, 128,
    -1
};

static const int  MLO_IcsInfo_swb_offset_128_24 [15 + 1 + 1] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64, 76, 92, 108, 128,
    -1
};

static const int  MLO_IcsInfo_swb_offset_128_16 [15 + 1 + 1] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 60, 72, 88, 108, 128,
    -1
};

static const int  MLO_IcsInfo_swb_offset_128_8 [15 + 1 + 1] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 60, 72, 88, 108, 128,
    -1
};

static const int* MLO_IcsInfo_swb_offset_short_window_128 [MLO_SAMPLING_FREQ_INDEX_NBR_ELT] =
{
   MLO_IcsInfo_swb_offset_128_96,   /* 96000 */
   MLO_IcsInfo_swb_offset_128_96,   /* 88200 */
   MLO_IcsInfo_swb_offset_128_64,   /* 64000 */
   MLO_IcsInfo_swb_offset_128_48,   /* 48000 */
   MLO_IcsInfo_swb_offset_128_48,   /* 44100 */
   MLO_IcsInfo_swb_offset_128_48,   /* 32000 */
   MLO_IcsInfo_swb_offset_128_24,   /* 24000 */
   MLO_IcsInfo_swb_offset_128_24,   /* 22050 */
   MLO_IcsInfo_swb_offset_128_16,   /* 16000 */
   MLO_IcsInfo_swb_offset_128_16,   /* 12000 */
   MLO_IcsInfo_swb_offset_128_16,   /* 11025 */
   MLO_IcsInfo_swb_offset_128_8,    /*  8000 */
   0,                               /*  7350 */
   0,
   0,
   0
};



/*----------------------------------------------------------------------
|       Functions
+---------------------------------------------------------------------*/



/*
==============================================================================
Name: MLO_IcsInfo_ClearBuffers
Description:
   Clears memory and past of the channel. To be called at least once before
   decoding a new stream.
Input/output parameters:
	- ics_ptr: channel to clear.
==============================================================================
*/

void	
MLO_IcsInfo_ClearBuffers (MLO_IcsInfo *ics_ptr)
{
   int            pos;

	MLO_ASSERT (ics_ptr != NULL);

   for (pos = 0; pos <MLO_IcsInfo_WSIndex_NBR_ELT; ++pos)
   {
      ics_ptr->window_shape [pos] = MLO_ICS_INFO_WINDOW_SHAPE_SINE;
   }
}



/*
==============================================================================
Name: MLO_IcsInfo_Decode
Description:
   Decodes ICS Info block from the bitstream.
   Ref:
   4.4.2.1, Table 4.6
   4.5.2.3
Output parameters:
Input/output parameters:
	- ics_ptr: ICS Info structure to fill
	- bits_ptr: Input bitstream to decode
   - fs_index: Valid Sampling Frequency Index for the frame
Returns:
   MLO_SUCCESS if ok
   MLO_ERROR_LTP_IN_LC if unexpected LTP block is encountered (LC profile)
   MLO_ERROR_UNSUPPORTED_SFI if sample frequency is not supported
==============================================================================
*/

MLO_Result	
MLO_IcsInfo_Decode (MLO_IcsInfo *ics_ptr, MLO_BitStream *bit_ptr, MLO_SamplingFreq_Index fs_index)
{
   MLO_Result     result = MLO_SUCCESS;

   MLO_ASSERT (ics_ptr != NULL);
   MLO_ASSERT (bit_ptr != NULL);
   MLO_ASSERT (fs_index >= 0);
   MLO_ASSERT (fs_index < MLO_SAMPLING_FREQ_INDEX_NBR_VALID);

   /* Saves previous frame's state */
   ics_ptr->window_shape [MLO_IcsInfo_WSIndex_PREV] =
      ics_ptr->window_shape [MLO_IcsInfo_WSIndex_CUR];

   /* Now, read the new frame */
   ics_ptr->fs_index = fs_index;

   MLO_BitStream_SkipBit (bit_ptr);
   ics_ptr->window_sequence =
      (MLO_IcsInfo_WinSeq) MLO_BitStream_ReadBits (bit_ptr, 2);
   ics_ptr->window_shape [MLO_IcsInfo_WSIndex_CUR] =
      (MLO_IcsInfo_WindowShape) MLO_BitStream_ReadBit (bit_ptr);

   if (ics_ptr->window_sequence == MLO_ICS_INFO_WIN_EIGHT_SHORT_SEQUENCE)
   {
      ics_ptr->max_sfb = MLO_BitStream_ReadBits (bit_ptr, 4);
      ics_ptr->scale_factor_grouping = MLO_BitStream_ReadBits (bit_ptr, 7);
   }

   else
   {
      ics_ptr->max_sfb = MLO_BitStream_ReadBits (bit_ptr, 6);

      if (MLO_BitStream_ReadBit (bit_ptr) != 0)
      {
         /* There should be no predictor for LC profile: audioObjectType != 1
            and there is no Long Term Prediction (LTP). */
         result = MLO_ERROR_LTP_IN_LC;
      }
   }

   if (MLO_SUCCEEDED (result))
   {
      result = MLO_IcsInfo_ComputeWindowGroupingInfo (ics_ptr);
   }

	return (result);
}



/*
==============================================================================
Name: MLO_IcsInfo_DeinterleaveCoefficients
Description:
   Reorder coefficients after decoding, inverse quantisation and rescaling:
   sfb / window => window / sfb.
   The operation is done in-place.
   Function collects data in the source (interleaved) order to maximise cache
   efficiency.
Input parameters:
	- ics_ptr: ICS Info
	- coef_ptr: coefficient array.
==============================================================================
*/

void MLO_IcsInfo_DeinterleaveCoefficients (const MLO_IcsInfo *ics_ptr, MLO_Float coef_ptr [])
{
   MLO_Float      tmp_arr [MLO_DEFS_FRAME_LEN_LONG];
   int            group_pos = 0;
   int            win_dest_size;
   int            num_window_groups;
   int            num_swb;
   int            g;

   MLO_ASSERT (ics_ptr != NULL);
   MLO_ASSERT (coef_ptr != NULL);

   num_window_groups = ics_ptr->num_window_groups;
   num_swb = ics_ptr->num_swb;
   win_dest_size = ics_ptr->swb_offset [num_swb];
   for (g = 0; g < num_window_groups; ++g)
   {
      int            nbr_win = ics_ptr->window_group_length [g];
      int            group_len = nbr_win * win_dest_size;
      int            src_pos = group_pos;
      int            sfb;

      for (sfb = 0; sfb < num_swb; ++sfb)
      {
         int            dest_pos = ics_ptr->swb_offset [sfb];
         int            win_src_size =
            ics_ptr->swb_offset [sfb + 1] - ics_ptr->swb_offset [sfb];
         int            win;

         for (win = 0; win < nbr_win; ++win)
         {
            MLO_CopyMemory (
               &tmp_arr [dest_pos],
               &coef_ptr [src_pos],
               win_src_size * sizeof (tmp_arr [0])
            );
            src_pos += win_src_size;
            dest_pos += win_dest_size;
         }

         MLO_ASSERT (dest_pos == ics_ptr->swb_offset [sfb] + group_len);
      }

      MLO_CopyMemory (
         &coef_ptr [group_pos],
         &tmp_arr [0],
         group_len * sizeof (coef_ptr [0])
      );

      group_pos += group_len;
      MLO_ASSERT (src_pos == group_pos);
   }
}



/*
==============================================================================
Name: MLO_IcsInfo_ComputeWindowGroupingInfo
Description:
   Computes and collects window informations, which are required for further
   data collection.
   Ref: 4.5.2.3.4
Input/output parameters:
	- ics_ptr: pointer on ics_info structure. It should have been initialised
      before calling the function.
Returns:
   MLO_SUCCESS if everything is ok
   MLO_ERROR_UNSUPPORTED_SFI if sample frequency is not supported.
==============================================================================
*/

MLO_Result  MLO_IcsInfo_ComputeWindowGroupingInfo (MLO_IcsInfo *ics_ptr)
{
   int            result = MLO_SUCCESS;

   MLO_ASSERT (ics_ptr != NULL);
   MLO_ASSERT (ics_ptr->fs_index >= 0);
   MLO_ASSERT (ics_ptr->fs_index < MLO_SAMPLING_FREQ_INDEX_NBR_VALID);

   /* Sampling rate below 8 kHz is not supported by this section. */
   if (ics_ptr->fs_index >= MLO_SAMPLING_FREQ_INDEX_NBR_SUPPORTED)
   {
      result = MLO_ERROR_UNSUPPORTED_SFI;
   }

   if (MLO_SUCCEEDED (result))
   {
      switch (ics_ptr->window_sequence)
      {
      case  MLO_ICS_INFO_WIN_ONLY_LONG_SEQUENCE:
      case  MLO_ICS_INFO_WIN_LONG_START_SEQUENCE:
      case  MLO_ICS_INFO_WIN_LONG_STOP_SEQUENCE:
         MLO_IcsInfo_ComputeWindowGroupingInfoLong (ics_ptr);
         break;

      case  MLO_ICS_INFO_WIN_EIGHT_SHORT_SEQUENCE:
         MLO_IcsInfo_ComputeWindowGroupingInfoShort (ics_ptr);
         break;

      default:
         MLO_ASSERT (MLO_FALSE);
         break;
      }
   }

   return (result);
}



void	
MLO_IcsInfo_ComputeWindowGroupingInfoLong (MLO_IcsInfo *ics_ptr)
{
   MLO_SamplingFreq_Index fs_index;
   int            nbr_swb;
   int            i;

   MLO_ASSERT (ics_ptr != NULL);
   MLO_ASSERT (ics_ptr->fs_index >= 0);
   MLO_ASSERT (ics_ptr->fs_index < MLO_SAMPLING_FREQ_INDEX_NBR_SUPPORTED);

   fs_index = ics_ptr->fs_index;

   ics_ptr->num_windows = 1;
   ics_ptr->num_window_groups = 1;
   ics_ptr->window_group_length [ics_ptr->num_window_groups - 1] = 1;

   nbr_swb = MLO_IcsInfo_num_swb_long_window_1024 [fs_index];
   MLO_ASSERT (nbr_swb + 1 <= (int)MLO_ARRAY_SIZE (ics_ptr->swb_offset));
   MLO_ASSERT (nbr_swb + 1 <= (int)MLO_ARRAY_SIZE (ics_ptr->sect_sfb_offset [0]));
   ics_ptr->num_swb = nbr_swb;

   /* Preparation of sect_sfb_offset for long blocks.
      Also copy the last value! */
   for (i = 0; i < nbr_swb + 1; ++i)
   {
      const int      offset =
         MLO_IcsInfo_swb_offset_long_window_1024 [fs_index] [i];
      MLO_ASSERT (offset >= 0);
      MLO_ASSERT (MLO_IcsInfo_swb_offset_long_window_1024 [fs_index] != 0);
      ics_ptr->sect_sfb_offset [0] [i] = offset;
      ics_ptr->swb_offset [i] = offset;
   }

   /* We support only 1024-sample frames */
   MLO_ASSERT (ics_ptr->sect_sfb_offset [0] [nbr_swb] == MLO_DEFS_FRAME_LEN_LONG);
}



void	
MLO_IcsInfo_ComputeWindowGroupingInfoShort (MLO_IcsInfo *ics_ptr)
{
   MLO_SamplingFreq_Index fs_index;
   int            nbr_swb;
   int            i;
   int            g;

   MLO_ASSERT (ics_ptr != NULL);
   MLO_ASSERT (ics_ptr->fs_index >= 0);
   MLO_ASSERT (ics_ptr->fs_index < MLO_SAMPLING_FREQ_INDEX_NBR_SUPPORTED);

   fs_index = ics_ptr->fs_index;

   ics_ptr->num_windows = 8;
   ics_ptr->num_window_groups = 1;
   ics_ptr->window_group_length [ics_ptr->num_window_groups - 1] = 1;

   nbr_swb = MLO_IcsInfo_num_swb_short_window_128 [fs_index];
   ics_ptr->num_swb = nbr_swb;

   for (i = 0; i < nbr_swb + 1; ++i)
   {
      ics_ptr->swb_offset [i] =
         MLO_IcsInfo_swb_offset_short_window_128 [fs_index] [i];
   }

   for (i = 0; i < ics_ptr->num_windows - 1; ++i)
   {
      const int      bit = 1 << (6 - i);
      if ((ics_ptr->scale_factor_grouping & bit) == 0)
      {
         ++ ics_ptr->num_window_groups;
         ics_ptr->window_group_length [ics_ptr->num_window_groups - 1] = 1;
      }
      else
      {
         ++ ics_ptr->window_group_length [ics_ptr->num_window_groups - 1];
      }
   }

   /* Preparation of sect_sfb_offset for short blocks */
   for (g = 0; g < ics_ptr->num_window_groups; ++g)
   {
      int            offset = 0;
      int            cur_swb_off =
         MLO_IcsInfo_swb_offset_short_window_128 [fs_index] [0];

      MLO_ASSERT (cur_swb_off >= 0);
      MLO_ASSERT (MLO_IcsInfo_swb_offset_short_window_128 [fs_index] != 0);

      for (i = 0; i < nbr_swb; ++i)
      {
         const int      next_swb_off =
            MLO_IcsInfo_swb_offset_short_window_128 [fs_index] [i + 1];
         int            width = next_swb_off - cur_swb_off;

         /* We support only 128-sample frames */
         MLO_ASSERT (next_swb_off == MLO_DEFS_FRAME_LEN_SHORT || i < nbr_swb);
         MLO_ASSERT (width > 0);

         width *= ics_ptr->window_group_length [g];
         ics_ptr->sect_sfb_offset [g] [i] = offset;

         offset += width;
         cur_swb_off = next_swb_off;
      }
      ics_ptr->sect_sfb_offset [g] [nbr_swb] = offset;
   }
}
