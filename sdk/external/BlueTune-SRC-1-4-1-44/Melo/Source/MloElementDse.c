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
#include "MloElementDse.h"
#include "MloDefs.h"

/*----------------------------------------------------------------------
|       Functions
+---------------------------------------------------------------------*/

/*
==============================================================================
Name: MLO_ElementDse_decode
Description:
   Decodes and skips the Data Stream Element.
Input/output parameters:
	- bit_ptr: Input bitstream to decode
Returns:
   MLO_SUCCESS if ok
   MLO_FAILURE for a bitstream error
==============================================================================
*/

MLO_Result	
MLO_ElementDse_Decode (MLO_BitStream *bit_ptr)
{
   MLO_Result     result = MLO_SUCCESS;
   MLO_Boolean    byte_align_flag;
   int            count;

	MLO_ASSERT (bit_ptr != NULL);

   MLO_BitStream_ReadBits (bit_ptr, 4);   /* Ignores element_instance_tag */
   byte_align_flag = (MLO_Boolean) MLO_BitStream_ReadBit (bit_ptr);
   count = MLO_BitStream_ReadBits (bit_ptr, 8);
   if (count == 255)
   {
      count += MLO_BitStream_ReadBits (bit_ptr, 8);
   }

   if (byte_align_flag)
   {
      result = MLO_BitStream_ByteAlign (bit_ptr);
   }

   if (MLO_SUCCEEDED (result))
   {
      MLO_BitStream_SkipBits (bit_ptr, count * 8);
   }

	return (result);
}



