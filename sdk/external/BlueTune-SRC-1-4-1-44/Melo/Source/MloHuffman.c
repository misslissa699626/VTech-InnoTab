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
#include "MloDebug.h"
#include "MloHcbPair.h"
#include "MloHcbQuad.h"
#include "MloHuffman.h"
#include "MloUtils.h"

/*----------------------------------------------------------------------
|       Prototypes
+---------------------------------------------------------------------*/
static void MLO_Huffman_decode_spectral_data_pair (MLO_Int16 data_ptr [2], MLO_BitStream *bit_ptr, MLO_Hcb hcb);
static void MLO_Huffman_decode_spectral_data_quad (MLO_Int16 data_ptr [4], MLO_BitStream *bit_ptr, MLO_Hcb hcb);
static void MLO_Huffman_decode_spectral_data_esc (MLO_Int16 data_ptr [2], MLO_BitStream *bit_ptr);

static void MLO_Huffman_decode_binary_pair_sign (MLO_Int16 data_ptr [2], MLO_BitStream *bit_ptr, MLO_Hcb hcb);
static void MLO_Huffman_decode_2steps_pair_sign (MLO_Int16 data_ptr [2], MLO_BitStream *bit_ptr, MLO_Hcb hcb);
static void MLO_Huffman_decode_binary_quad_sign (MLO_Int16 data_ptr [4], MLO_BitStream *bit_ptr, MLO_Hcb hcb);
static void MLO_Huffman_decode_2steps_quad_sign (MLO_Int16 data_ptr [4], MLO_BitStream *bit_ptr, MLO_Hcb hcb);

static MLO_Int16  MLO_Huffman_apply_sign (MLO_Int16 data, MLO_BitStream *bit_ptr);
static MLO_Int16  MLO_Huffman_decode_escape_code (MLO_Int16 data, MLO_BitStream *bit_ptr);

/*----------------------------------------------------------------------
|       Functions
+---------------------------------------------------------------------*/

/*
==============================================================================
Name: MLO_Huffman_decode_spectral_data
Description:
   Decodes a pair or quad of Huffman-compressed values of spectral data.
Input parameters:
	- data_ptr: Points on the first value location in the spectral data array.
	- hcb: Index of the Huffman codebook that will be used to decode the data.
Input/output parameters:
	- bit_ptr: Input bitstream to decode.
Returns:
   MLO_SUCCESS if ok
   MLO_ERROR_UNEXPECTED_CB_TYPE if the specified codebook is not supported.
==============================================================================
*/

MLO_Result	
MLO_Huffman_decode_spectral_data (MLO_Int16 data_ptr [], MLO_BitStream *bit_ptr, MLO_Hcb hcb)
{
   MLO_Result     result = MLO_SUCCESS;

   MLO_ASSERT (data_ptr != NULL);
   MLO_ASSERT (bit_ptr != NULL);
   MLO_ASSERT (hcb > MLO_HCB_ZERO_HCB);
   MLO_ASSERT (hcb <= MLO_HCB_ESC_HCB);

   /* Quad */
   if (hcb < MLO_HCB_FIRST_PAIR_HCB)
   {
      MLO_Huffman_decode_spectral_data_quad (data_ptr, bit_ptr, hcb);
   }

   /* Pair */
   else if (hcb < MLO_HCB_ESC_HCB)
   {
      MLO_Huffman_decode_spectral_data_pair (data_ptr, bit_ptr, hcb);
   }

   /* Escape code */
   else if (hcb == MLO_HCB_ESC_HCB)
   {
      MLO_Huffman_decode_spectral_data_esc (data_ptr, bit_ptr);
   }

   /* Reserved codebook, not handled */
   else
   {
      result = MLO_ERROR_UNEXPECTED_CB_TYPE;
   }

	return (result);
}



void  
MLO_Huffman_decode_spectral_data_pair (MLO_Int16 data_ptr [2], MLO_BitStream *bit_ptr, MLO_Hcb hcb)
{
	MLO_ASSERT (data_ptr != NULL);
	MLO_ASSERT (bit_ptr != NULL);
	MLO_ASSERT (hcb >= MLO_HCB_FIRST_PAIR_HCB);
    MLO_ASSERT (hcb < MLO_HCB_ESC_HCB);

   switch (hcb)
   {
   case  5:
      MLO_HcbPair_decode_binary (data_ptr, bit_ptr, hcb);
      break;

   case  6:
      MLO_HcbPair_decode_2steps (data_ptr, bit_ptr, hcb);
      break;

   case  7:
   case  9:
      MLO_Huffman_decode_binary_pair_sign (data_ptr, bit_ptr, hcb);
      break;

   case  8:
   case  10:
      MLO_Huffman_decode_2steps_pair_sign (data_ptr, bit_ptr, hcb);
      break;

   default:
      MLO_ASSERT (MLO_FALSE);
      break;
   }
}

void  
MLO_Huffman_decode_spectral_data_quad (MLO_Int16 data_ptr [4], MLO_BitStream *bit_ptr, MLO_Hcb hcb)
{
	MLO_ASSERT (data_ptr != NULL);
	MLO_ASSERT (bit_ptr != NULL);
    MLO_ASSERT (hcb > MLO_HCB_ZERO_HCB);
	MLO_ASSERT (hcb < MLO_HCB_FIRST_PAIR_HCB);

   switch (hcb)
   {
   case  1:
   case  2:
      MLO_HcbQuad_decode_2steps (data_ptr, bit_ptr, hcb);
      break;

   case  3:
      MLO_Huffman_decode_binary_quad_sign (data_ptr, bit_ptr, hcb);
      break;

   case  4:
      MLO_Huffman_decode_2steps_quad_sign (data_ptr, bit_ptr, hcb);
      break;

   default:
      MLO_ASSERT (MLO_FALSE);
      break;
   }
}



void  
MLO_Huffman_decode_spectral_data_esc (MLO_Int16 data_ptr [2], MLO_BitStream *bit_ptr)
{
	MLO_ASSERT (data_ptr != NULL);
	MLO_ASSERT (bit_ptr != NULL);

   MLO_Huffman_decode_2steps_pair_sign (data_ptr, bit_ptr, MLO_HCB_ESC_HCB);
   data_ptr [0] = MLO_Huffman_decode_escape_code (data_ptr [0], bit_ptr);
   data_ptr [1] = MLO_Huffman_decode_escape_code (data_ptr [1], bit_ptr);
}



void  MLO_Huffman_decode_binary_pair_sign (MLO_Int16 data_ptr [2], MLO_BitStream *bit_ptr, MLO_Hcb hcb)
{
   MLO_HcbPair_decode_binary (data_ptr, bit_ptr, hcb);

   data_ptr [0] = MLO_Huffman_apply_sign (data_ptr [0], bit_ptr);
   data_ptr [1] = MLO_Huffman_apply_sign (data_ptr [1], bit_ptr);
}



void  MLO_Huffman_decode_2steps_pair_sign (MLO_Int16 data_ptr [2], MLO_BitStream *bit_ptr, MLO_Hcb hcb)
{
   MLO_HcbPair_decode_2steps (data_ptr, bit_ptr, hcb);

   data_ptr [0] = MLO_Huffman_apply_sign (data_ptr [0], bit_ptr);
   data_ptr [1] = MLO_Huffman_apply_sign (data_ptr [1], bit_ptr);
}



void  MLO_Huffman_decode_binary_quad_sign (MLO_Int16 data_ptr [4], MLO_BitStream *bit_ptr, MLO_Hcb hcb)
{
   MLO_HcbQuad_decode_binary (data_ptr, bit_ptr, hcb);

   data_ptr [0] = MLO_Huffman_apply_sign (data_ptr [0], bit_ptr);
   data_ptr [1] = MLO_Huffman_apply_sign (data_ptr [1], bit_ptr);
   data_ptr [2] = MLO_Huffman_apply_sign (data_ptr [2], bit_ptr);
   data_ptr [3] = MLO_Huffman_apply_sign (data_ptr [3], bit_ptr);
}



void  MLO_Huffman_decode_2steps_quad_sign (MLO_Int16 data_ptr [4], MLO_BitStream *bit_ptr, MLO_Hcb hcb)
{
   MLO_HcbQuad_decode_2steps (data_ptr, bit_ptr, hcb);

   data_ptr [0] = MLO_Huffman_apply_sign (data_ptr [0], bit_ptr);
   data_ptr [1] = MLO_Huffman_apply_sign (data_ptr [1], bit_ptr);
   data_ptr [2] = MLO_Huffman_apply_sign (data_ptr [2], bit_ptr);
   data_ptr [3] = MLO_Huffman_apply_sign (data_ptr [3], bit_ptr);
}



/*
==============================================================================
Name: MLO_Huffman_apply_sign
Description:
   Optionnally decodes the sign of a single value, if non-zero.
Input parameters:
	- data: Read value, positive or null.
Input/output parameters:
	- bit_ptr: Input bitstream to decode.
Returns: Signed value.
==============================================================================
*/

MLO_Int16	
MLO_Huffman_apply_sign (MLO_Int16 data, MLO_BitStream *bit_ptr)
{
	MLO_ASSERT (data >= 0);
	MLO_ASSERT (bit_ptr != NULL);

   if (data != 0)
   {
      if (MLO_BitStream_ReadBit (bit_ptr) != 0)
      {
         data = -data;
      }
   }

	return (data);
}



/*
==============================================================================
Name: MLO_Huffman_decode_escape_code
Description:
   Optionnally decodes a Huffman escape code following a signed pair from the
   MLO_HCB_ESC_HCB codebook (11). The function decides if there is an escape
   code or not depending on the input value. One call per value.
   Ref: 4.6.3.3
Input parameters:
	- data: Read value, with its sign.
Input/output parameters:
	- bit_ptr: Input bitstream to decode. Sign has been read already.
Returns: The final data.
==============================================================================
*/

MLO_Int16	
MLO_Huffman_decode_escape_code (MLO_Int16 data, MLO_BitStream *bit_ptr)
{
	MLO_ASSERT (bit_ptr != NULL);

   /* If escape sequence is present */
   if (MLO_ABS (data) == 16)
   {
      int         sign = MLO_SIGN (data);
      int         n = 0;
      int         esc_word;

      /* Counts the 1's and extracts the terminal 0 */
      while (MLO_BitStream_ReadBit (bit_ptr) != 0)
      {
         ++ n;
      }

      /* Get the n + 4 mantissa bits */
      esc_word = MLO_BitStream_ReadBits (bit_ptr, n + 4);

      /* Compute the number */
      data = (1 << (n + 4)) + esc_word;

      /* Applies the sign */
      data *= sign;
   }

	return (data);
}
