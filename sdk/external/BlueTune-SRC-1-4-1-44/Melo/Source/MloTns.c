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
#include "MloFloat.h"
#include "MloIcsInfo.h"
#include "MloIndivChnStream.h"
#include "MloTns.h"
#include "MloUtils.h"

/*----------------------------------------------------------------------
|       Constants
+---------------------------------------------------------------------*/
typedef enum MLO_Tns_WinSize
{
   MLO_TNS_WIN_SIZE_SHORT = 0,
   MLO_TNS_WIN_SIZE_LONG,

   MLO_TNS_WIN_SIZE_NBR_ELT
}  MLO_Tns_WinSize;

/* 4.6.9.4, Table 4.137 */
static const int  MLO_Tns_max_order [MLO_TNS_WIN_SIZE_NBR_ELT] = { 7, 12 };

/* 4.6.9.4, Table 4.138 */
static const int  MLO_Tns_max_bands [MLO_SAMPLING_FREQ_INDEX_NBR_ELT] [MLO_TNS_WIN_SIZE_NBR_ELT] =
{
   /* Short / Long */
   {  9, 31 }, /* 96000 */
   {  9, 31 }, /* 88200 */
   { 10, 34 }, /* 64000 */
   { 14, 40 }, /* 48000 */
   { 14, 42 }, /* 44100 */
   { 14, 51 }, /* 32000 */
   { 14, 46 }, /* 24000 */
   { 14, 46 }, /* 22050 */
   { 14, 42 }, /* 16000 */
   { 14, 42 }, /* 12000 */
   { 14, 42 }, /* 11025 */
   { 14, 39 }, /*  8000 */
   {  0,  0 }, /*  7350 */
   {  0,  0 },
   {  0,  0 },
   {  0,  0 }
};

/* [resol] [compress] [coef] */
static const MLO_Float MLO_Tns_table_coef [2] [2] [1 << MLO_TNS_MAX_COEF_RES] =
{
   {
      {
         MLO_FLOAT_C (0),
         MLO_FLOAT_C (0.433883739),
         MLO_FLOAT_C (0.781831482),
         MLO_FLOAT_C (0.974927912),
         MLO_FLOAT_C (-0.984807753),
         MLO_FLOAT_C (-0.866025404),
         MLO_FLOAT_C (-0.64278761),
         MLO_FLOAT_C (-0.342020143)
      },
      {
         MLO_FLOAT_C (0),
         MLO_FLOAT_C (0.433883739),
         MLO_FLOAT_C (-0.64278761),
         MLO_FLOAT_C (-0.342020143)
      }
   },
   {
      {
         MLO_FLOAT_C (0),
         MLO_FLOAT_C (0.207911691),
         MLO_FLOAT_C (0.406736643),
         MLO_FLOAT_C (0.587785252),
         MLO_FLOAT_C (0.743144825),
         MLO_FLOAT_C (0.866025404),
         MLO_FLOAT_C (0.951056516),
         MLO_FLOAT_C (0.994521895),
         MLO_FLOAT_C (-0.995734176),
         MLO_FLOAT_C (-0.961825643),
         MLO_FLOAT_C (-0.895163291),
         MLO_FLOAT_C (-0.798017227),
         MLO_FLOAT_C (-0.673695644),
         MLO_FLOAT_C (-0.526432163),
         MLO_FLOAT_C (-0.361241666),
         MLO_FLOAT_C (-0.183749518)
      },
      {
         MLO_FLOAT_C (0),
         MLO_FLOAT_C (0.207911691),
         MLO_FLOAT_C (0.406736643),
         MLO_FLOAT_C (0.587785252),
         MLO_FLOAT_C (-0.673695644),
         MLO_FLOAT_C (-0.526432163),
         MLO_FLOAT_C (-0.361241666),
         MLO_FLOAT_C (-0.183749518)
      }
   }
};



/*----------------------------------------------------------------------
|       Prototypes
+---------------------------------------------------------------------*/



static void MLO_Tns_ProcessFilter (MLO_IndivChnStream *ics_ptr, int win, int filter, int *bottom_ptr);
static void MLO_Tns_CalculateLpcCoef (const MLO_Tns_Filter *filter_ptr, int resol, MLO_Float lpc_coef_arr [1 + MLO_TNS_MAX_ORDER]);
static void MLO_Tns_InvQuantCoef (const MLO_Tns_Filter *filter_ptr, int resol, MLO_Float iq_arr [MLO_TNS_MAX_ORDER]);
static void MLO_Tns_ConvCoefToLpc (MLO_Float lpc_coef_arr [1 + MLO_TNS_MAX_ORDER], MLO_Float iq_arr [MLO_TNS_MAX_ORDER], int order);
static void MLO_Tns_RunFilter (MLO_Float *coef_ptr, int length, int inc, const MLO_Float lpc_coef_arr [1 + MLO_TNS_MAX_ORDER], int order);



/*----------------------------------------------------------------------
|       Functions
+---------------------------------------------------------------------*/



/*
==============================================================================
Name: MLO_Tns_Decode
Description:
   Decodes Temporal Noise Shapping information.
   Ref:
      4.4.2.7, Table 4.48
      4.6.9
Input parameters:
	- ics_info_ptr: Information for the channel
Output parameters:
   - tns_ptr: Decoded TNS information
Input/output parameters:
	- bits_ptr: Input bitstream to decode
==============================================================================
*/

MLO_Result
MLO_Tns_Decode (MLO_Tns *tns_ptr, const MLO_IcsInfo *ics_info_ptr, MLO_BitStream *bit_ptr)
{
   int            bd_n_filt = 2;
   int            bd_length = 6;
   int            bd_order = 5;
   int            w;

   MLO_ASSERT (tns_ptr != NULL);
   MLO_ASSERT (ics_info_ptr != NULL);
   MLO_ASSERT (bit_ptr != NULL);

   if (ics_info_ptr->window_sequence == MLO_ICS_INFO_WIN_EIGHT_SHORT_SEQUENCE)
   {
      bd_n_filt = 1;
      bd_length = 4;
      bd_order = 3;
   }

   MLO_ASSERT (ics_info_ptr->num_windows <= (int)MLO_ARRAY_SIZE (tns_ptr->win_arr));
   for (w = 0; w < ics_info_ptr->num_windows; ++ w)
   {
      MLO_Tns_Window *  win_ptr = &tns_ptr->win_arr [w];
      int            bd_coef_base = 3;
      int            f;

      win_ptr->n_filt = MLO_BitStream_ReadBits (bit_ptr, bd_n_filt);
      MLO_CHECK_DATA (win_ptr->n_filt <= (int)MLO_ARRAY_SIZE (win_ptr->filter));

      if (win_ptr->n_filt > 0)
      {
         win_ptr->coef_res = MLO_BitStream_ReadBit (bit_ptr);
         if (win_ptr->coef_res != 0)
         {
            bd_coef_base = 4;
         }
      }

      for (f = 0; f < win_ptr->n_filt; ++ f)
      {
         MLO_Tns_Filter *  filter_ptr = &win_ptr->filter [f];

         filter_ptr->length = MLO_BitStream_ReadBits (bit_ptr, bd_length);
         filter_ptr->order  = MLO_BitStream_ReadBits (bit_ptr, bd_order);
         MLO_CHECK_DATA (filter_ptr->order <= MLO_ARRAY_SIZE (filter_ptr->coef));

         if (filter_ptr->order > 0)
         {
            int            c;
            int            bd_coef = bd_coef_base;

            filter_ptr->direction = MLO_BitStream_ReadBit (bit_ptr);
            filter_ptr->compress = MLO_BitStream_ReadBit (bit_ptr);
            bd_coef -= filter_ptr->compress;
            MLO_CHECK_DATA (bd_coef > 0);
            MLO_CHECK_DATA (bd_coef <= MLO_TNS_MAX_COEF_RES);

            for (c = 0; c < filter_ptr->order; ++c)
            {
               filter_ptr->coef [c] =
                  MLO_BitStream_ReadBits (bit_ptr, bd_coef);
            }
         }
      }
   }
   
   return MLO_SUCCESS;
}



/*
==============================================================================
Name: MLO_Tns_Process
Description:
   Does the Temporal Noise Shaping for an individual_channel_stream.
Input/output parameters:
	- ics_ptr: individual_channel_stream to process.
==============================================================================
*/

MLO_Result
MLO_Tns_Process (struct MLO_IndivChnStream *ics_ptr)
{
   MLO_ASSERT (ics_ptr != NULL);

   if (ics_ptr->tns_data_present_flag)
   {
      int            num_windows = ics_ptr->ics_info.num_windows;
      int            num_swb = ics_ptr->ics_info.num_swb;
      int            win;

      for (win = 0; win < num_windows; ++ win)
      {
         const MLO_Tns_Window *  win_ptr = &ics_ptr->tns.win_arr [win];
         int         bottom = num_swb;
         int         nbr_filters = win_ptr->n_filt;
         int         filter;

         for (filter = 0; filter < nbr_filters; ++filter)
         {
            MLO_Tns_ProcessFilter (ics_ptr, win, filter, &bottom);
         }
      }
   }
   
   return MLO_SUCCESS;
}



/*
==============================================================================
Name: MLO_Tns_ProcessFilter
Description:
   Process one filter for a given window.
Input parameters:
	- win: window index, [0 ; num_windows[
	- filter: filter index for this window, [0 ; n_filt[
Input/output parameters:
	- ics_ptr: individual_channel_stream where operation takes place.
	- bottom_ptr: SWB index after the last SWB to process. Returned as the
      first SWB processed (and ready for the subsequent call to the function).
==============================================================================
*/

static void MLO_Tns_ProcessFilter (MLO_IndivChnStream *ics_ptr, int win, int filter, int *bottom_ptr)
{
   const MLO_Tns_Filter *  filter_ptr;
   int            top;
   int            tns_order;
   int            tns_max_order;
   MLO_Tns_WinSize   win_size = MLO_TNS_WIN_SIZE_LONG;

   MLO_ASSERT (ics_ptr != NULL);
   MLO_ASSERT (win >= 0);
   MLO_ASSERT (win < ics_ptr->ics_info.num_windows);
   MLO_ASSERT (filter >= 0);
   MLO_ASSERT (filter < ics_ptr->tns.win_arr [win].n_filt);
   MLO_ASSERT (bottom_ptr != NULL);
   MLO_ASSERT (*bottom_ptr > 0);
   MLO_ASSERT (*bottom_ptr <= ics_ptr->ics_info.num_swb);

   if (ics_ptr->ics_info.window_sequence == MLO_ICS_INFO_WIN_EIGHT_SHORT_SEQUENCE)
   {
      win_size = MLO_TNS_WIN_SIZE_SHORT;
   }
   filter_ptr = &ics_ptr->tns.win_arr [win].filter [filter];

   top = *bottom_ptr;
   *bottom_ptr = MLO_MAX (top - filter_ptr->length, 0);

   tns_max_order = MLO_Tns_max_order [win_size];
   tns_order = MLO_MIN (filter_ptr->order, tns_max_order);
   if (tns_order > 0)
   {
      int            tns_max_bands =
         MLO_Tns_max_bands [ics_ptr->ics_info.fs_index] [win_size];
      int            max_sfb = ics_ptr->ics_info.max_sfb;
      int            min_max_sfb_max_bands = MLO_MIN (tns_max_bands, max_sfb);
      int            swb_bot = MLO_MIN (*bottom_ptr, min_max_sfb_max_bands);
      int            swb_top = MLO_MIN (top, min_max_sfb_max_bands);
      int            start = ics_ptr->ics_info.swb_offset [swb_bot];
      int            end   = ics_ptr->ics_info.swb_offset [swb_top];
      int            length = end - start;

      if (length > 0)
      {
         MLO_Float      lpc_coef_arr [1 + MLO_TNS_MAX_ORDER];
         int            inc = 1;

         MLO_Tns_CalculateLpcCoef (
            filter_ptr,
            ics_ptr->tns.win_arr [win].coef_res,
            lpc_coef_arr
         );

         if (filter_ptr->direction != 0)
         {
            inc = -1;
            start = end - 1;
         }

         MLO_Tns_RunFilter (
            &ics_ptr->coef_arr [win * MLO_DEFS_FRAME_LEN_SHORT + start],
            length,
            inc,
            &lpc_coef_arr [0],
            tns_order
         );
      }
   }
}



/*
==============================================================================
Name: MLO_Tns_CalculateLpcCoef
Description:
   Prepare LPC coefficients for filtering.
Input parameters:
	- filter_ptr: Filter object
	- resol: Coefficient resolution coef_res, [0 ; 1]
Output parameters:
	- lpc_coef_arr: array to fill with coefficients. Should be big enough to
      host them all (depends on the filter order).
==============================================================================
*/

static void MLO_Tns_CalculateLpcCoef (const MLO_Tns_Filter *filter_ptr, int resol, MLO_Float lpc_coef_arr [1 + MLO_TNS_MAX_ORDER])
{
   MLO_Float      iq_arr [MLO_TNS_MAX_ORDER];

   MLO_ASSERT (resol >= 0);
   MLO_ASSERT (resol <= 1);

   MLO_Tns_InvQuantCoef (filter_ptr, resol, iq_arr);
   MLO_Tns_ConvCoefToLpc (lpc_coef_arr, iq_arr, filter_ptr->order);
}



/*
==============================================================================
Name: MLO_Tns_InvQuantCoef
Description:
   Does the inverse quantisation on filter data.
Input parameters:
	- filter_ptr: Filter object
	- resol: Coefficient resolution coef_res, [0 ; 1]
Output parameters:
	- iq_arr: inverse-quantised output data. Should be big enough to host
   them all (depends on the filter order).
==============================================================================
*/

static void MLO_Tns_InvQuantCoef (const MLO_Tns_Filter *filter_ptr, int resol, MLO_Float iq_arr [MLO_TNS_MAX_ORDER])
{
   const int      order = filter_ptr->order;
   const int      compress = filter_ptr->compress;
   int            i;

   for (i = 0; i < order; ++i)
   {
      const int      coef = filter_ptr->coef [i];
      iq_arr [i] = MLO_Tns_table_coef [resol] [compress] [coef];
   }
}



/*
==============================================================================
Name: MLO_Tns_ConvCoefToLpc
Description:
   Converts inverse-quantised data to usable LPC coefficients.
Input parameters:
	- iq_arr: filter data, not quantised.
	- order: Actual order of the filter, > 0.
Output parameters:
	- lpc_coef_arr: array to fill with coefficients. Should be big enough to
      host them all (order + 1).
==============================================================================
*/

static void MLO_Tns_ConvCoefToLpc (MLO_Float lpc_coef_arr [1 + MLO_TNS_MAX_ORDER], MLO_Float iq_arr [MLO_TNS_MAX_ORDER], int order)
{
   int            m;

   MLO_ASSERT (order > 0);

   /* Conversion to LPC coefficients */
   lpc_coef_arr [0] = 1;
   for (m = 1; m <= order; ++m)
   {
      MLO_Float      b [MLO_TNS_MAX_ORDER];
      int            i;

      /* loop only while i<m */
      for (i = 1; i < m; ++i)
      {
         b [i] = MLO_Float_Add (
            lpc_coef_arr [i],
            MLO_Float_Mul (iq_arr [m - 1], lpc_coef_arr [m - i])
         );
      }

      /* loop only while i<m */
      for (i = 1; i < m; ++i)
      {
         lpc_coef_arr [i] = b [i];
      }

      /* changed */
      lpc_coef_arr [m] = iq_arr [m - 1];
   }
}



/*
==============================================================================
Name: MLO_Tns_RunFilter
Description:
   Filter spectral data with LPC. Filtering is done in-place.
Input parameters:
	- length: Number of sample to process, > 0
	- inc: Filter direction, +1 = forward, -1 = backward.
	- lpc_coef_arr: coefficient of the LPC filter, there should be order + 1 of
      them (first one is actually ignored).
	- order: Order of the LPC filter. 
Input/output parameters:
	- coef_ptr: spectral data to filter. Processed data extends before or
      after this point, depending on inc parameter.
==============================================================================
*/

static void MLO_Tns_RunFilter (MLO_Float *coef_ptr, int length, int inc, const MLO_Float lpc_coef_arr [1 + MLO_TNS_MAX_ORDER], int order)
{
   int            pos = 1;

   MLO_ASSERT (coef_ptr != NULL);
   MLO_ASSERT (length > 0);
   MLO_ASSERT (inc == 1 || inc == -1);
   MLO_ASSERT (lpc_coef_arr != 0);
   MLO_ASSERT (order > 0);
   MLO_ASSERT (order <= MLO_TNS_MAX_ORDER);

   /* As filter state is 0, first data is unchanged */
   coef_ptr += inc;

   while (pos < length)
   {
      /* Runs only with "existing" past output data */
      int            conv_len = MLO_MIN (order, pos);
      MLO_Float      sum = *coef_ptr;

      int            c = 1;
      int            s = -inc;
      do
      {
         const MLO_Float   coef = lpc_coef_arr [c];
         const MLO_Float   state = coef_ptr [s];
         sum = MLO_Float_Sub (sum, MLO_Float_Mul (state, coef));
         ++ c;
         s -= inc;
      }
      while (c <= conv_len);

      *coef_ptr = sum;

      coef_ptr += inc;
      ++ pos;
   }
}
