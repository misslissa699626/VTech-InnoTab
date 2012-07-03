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
#include "MloElementDse.h"
#include "MloFloat.h"
#include "MloSyntacticElements.h"
#include "MloTns.h"
#include "MloUtils.h"
#include "MloDecoder.h"

/*----------------------------------------------------------------------
|       Types
+---------------------------------------------------------------------*/

/* Syntactic elements, 4.5.2.2.1, Table 4.71 */
typedef enum MLO_SyntacticElements_Type
{
   MLO_SYNTACTIC_ELEMENTS_TYPE_SCE = 0,
   MLO_SYNTACTIC_ELEMENTS_TYPE_CPE,
   MLO_SYNTACTIC_ELEMENTS_TYPE_CCE,
   MLO_SYNTACTIC_ELEMENTS_TYPE_LFE,
   MLO_SYNTACTIC_ELEMENTS_TYPE_DSE,
   MLO_SYNTACTIC_ELEMENTS_TYPE_PCE,
   MLO_SYNTACTIC_ELEMENTS_TYPE_FIL,
   MLO_SYNTACTIC_ELEMENTS_TYPE_END,

   MLO_SYNTACTIC_ELEMENTS_TYPE_NBR_ELT
}  MLO_SyntacticElements_Type;

/*----------------------------------------------------------------------
|       Prototypes
+---------------------------------------------------------------------*/
void  MLO_SyntacticElements_ClearTagMaps (MLO_SyntacticElements *se_ptr);
static MLO_Result MLO_SyntacticElements_DecodeSce (MLO_SyntacticElements *se_ptr, MLO_BitStream *bit_ptr);
static MLO_Result MLO_SyntacticElements_DecodeCpe (MLO_SyntacticElements *se_ptr, MLO_BitStream *bit_ptr);
static MLO_Result MLO_SyntacticElements_DecodeCce (MLO_SyntacticElements *se_ptr, MLO_BitStream *bit_ptr);
static MLO_Result MLO_SyntacticElements_DecodeLfe (MLO_SyntacticElements *se_ptr, MLO_BitStream *bit_ptr);
static MLO_Result MLO_SyntacticElements_DecodeDse (MLO_SyntacticElements *se_ptr, MLO_BitStream *bit_ptr);
static MLO_Result MLO_SyntacticElements_DecodePce (MLO_SyntacticElements *se_ptr, MLO_BitStream *bit_ptr);
static MLO_Result MLO_SyntacticElements_DecodeFil (MLO_SyntacticElements *se_ptr, MLO_BitStream *bit_ptr);
static void MLO_SyntacticElements_MemorizeElement (MLO_SyntacticElements *se_ptr, MLO_SyntacticElements_ContentType type, int index);
static void MLO_SyntacticElements_InterleaveAndConvertChannel (MLO_SampleBuffer *outbuf_ptr, int chn, const MLO_Float in_ptr []);



/*----------------------------------------------------------------------
|       Functions
+---------------------------------------------------------------------*/



/*************************************************************************************************************/
/*** Debug ***/
#if 0
#include <stdio.h>
#include "MloIndivChnStream.h"

static int  MLO_SyntacticElements_frame_index = 0;

void MLO_SyntacticElements_DumpFrame (int channel, const MLO_Float *data_ptr, long len)
{
   if (MLO_SyntacticElements_frame_index < 100)
   {
      long           pos;
      FILE *         f_ptr;
      const char *   fname_0 = "D:\\Ohm Force\\dev\\test\\frames_test.txt";

      if (MLO_SyntacticElements_frame_index == 0 && channel == 0)
      {
         remove (fname_0);
      }
      f_ptr = fopen (fname_0, "a");

      fprintf (f_ptr, "Frame # %06d, channel # %02d\n", MLO_SyntacticElements_frame_index, channel);
      for (pos = 0; pos < len; ++pos)
      {
         fprintf (f_ptr, "%.6g\n", data_ptr [pos]);
      }
      fprintf (f_ptr, "\n");

      fclose (f_ptr);
   }
}
#endif
/*************************************************************************************************************/



/*
==============================================================================
Name: MLO_SyntacticElements_Init
Description:
   Initialise the object. Call this function only once, before any other.
   Default configuration: 2 channels.
Output parameters:
	- se_ptr: MLO_SyntacticElements to initialise
Returns:
   MLO_SUCCESS if ok
   MLO_ERROR_OUT_OF_MEMORY if required memory cannot be allocated
==============================================================================
*/

MLO_Result	MLO_SyntacticElements_Init (MLO_SyntacticElements *se_ptr)
{
   MLO_Result     result = MLO_SUCCESS;

	MLO_ASSERT (se_ptr != NULL);

   result = MLO_IndivChnPool_Create (&se_ptr->chn_pool, 2);
   if (MLO_SUCCEEDED (result))
   {
      /* Init the PCE? */

   	/*** To do ***/
      se_ptr->pce.sampling_frequency_index = 0/*** To do ***/;

   }

   if (MLO_SUCCEEDED (result))
   {
      MLO_SyntacticElements_StartNewFrame (
         se_ptr,
         se_ptr->pce.sampling_frequency_index
      );
   }

	return (result);
}



/*
==============================================================================
Name: MLO_SyntacticElements_Restore
Description:
   Release all occupied resources. Call this function when the object is no
   longer used, but only if call to MLO_SyntacticElements_Init() was
   successful.
Input/output parameters:
	- se_ptr: MLO_SyntacticElements to destroy
==============================================================================
*/

void	MLO_SyntacticElements_Restore (MLO_SyntacticElements *se_ptr)
{
	MLO_ASSERT (se_ptr != NULL);

   MLO_IndivChnPool_Destroy (&se_ptr->chn_pool);
}



/*
==============================================================================
Name: MLO_SyntacticElements_SetNbrChn
Description:
   Call this function before decoding frames to change the channel
   configuration.
Input parameters:
	- nbr_chn: Maximum number of supported channels, in ]0, MLO_DEFS_MAX_CHN]
Input/output parameters:
	- se_ptr: MLO_SyntactifElements object
Returns:
   MLO_SUCCESS if ok
   MLO_ERROR_OUT_OF_MEMORY if out of memory.
==============================================================================
*/

MLO_Result	
MLO_SyntacticElements_SetNbrChn (MLO_SyntacticElements *se_ptr, int nbr_chn)
{
   MLO_Result     result = MLO_SUCCESS;

   MLO_ASSERT (se_ptr != NULL);
   MLO_ASSERT (nbr_chn > 0);
   MLO_ASSERT (nbr_chn <= MLO_DEFS_MAX_CHN);

   result = MLO_IndivChnPool_Allocate (&se_ptr->chn_pool, nbr_chn);
   if (MLO_SUCCEEDED (result))
   {
      MLO_SyntacticElements_StartNewFrame (
         se_ptr,
         se_ptr->pce.sampling_frequency_index
      );
   }

	return (result);
}



/*
==============================================================================
Name: MLO_SyntacticElements_StartNewFrame
Description:
   Call this function at the beginning of the frame, before decoding any
   raw_data_block.
Input parameters:
	- fs_index: Valid Sampling Frequency Index
Input/output parameters:
	- se_ptr: MLO_SyntactifElements object
==============================================================================
*/

void	MLO_SyntacticElements_StartNewFrame (MLO_SyntacticElements *se_ptr, MLO_SamplingFreq_Index fs_index)
{
   int            tag;

   MLO_ASSERT (se_ptr != NULL);
   MLO_ASSERT (fs_index >= 0);
   MLO_ASSERT (fs_index < MLO_SAMPLING_FREQ_INDEX_NBR_VALID);

   MLO_IndivChnPool_Clear (&se_ptr->chn_pool);

   se_ptr->nbr_fil = 0;
   se_ptr->nbr_sce = 0;
   se_ptr->nbr_cpe = 0;
   se_ptr->nbr_cce = 0;
   se_ptr->nbr_received_elements = 0;

   for (tag = 0; tag < (int)MLO_ARRAY_SIZE (se_ptr->sce_tag_map); ++tag)
   {
      se_ptr->sce_tag_map [tag] = -1;
      se_ptr->cpe_tag_map [tag] = -1;
   }

   se_ptr->pce.sampling_frequency_index = fs_index;
}



/*
==============================================================================
Name: MLO_SyntacticElements_Decode
Description:
   Decodes a raw_data_block and does spectral processing up to Dependently
   Switched Coupling (not included).
   Ref: 4.4.2.1, Table 4.3
Input/output parameters:
	- se_ptr: MLO_SyntactifElements object.
	- bit_ptr: Input bitstream to decode
Returns:
   MLO_SUCCESS if ok
   ... if any error
==============================================================================
*/

MLO_Result	
MLO_SyntacticElements_Decode (MLO_SyntacticElements *se_ptr, MLO_BitStream *bit_ptr)
{
   MLO_Result     result = MLO_SUCCESS;
   MLO_Boolean    cont_flag = MLO_TRUE;

	MLO_ASSERT (se_ptr != NULL);
	MLO_ASSERT (bit_ptr != NULL);

   /* Element selection */
   do
   {
      MLO_SyntacticElements_Type id_syn_ele =
         (MLO_SyntacticElements_Type) MLO_BitStream_ReadBits (bit_ptr, 3);
      switch (id_syn_ele)
      {
      case  MLO_SYNTACTIC_ELEMENTS_TYPE_SCE:
         result = MLO_SyntacticElements_DecodeSce (se_ptr, bit_ptr);
         break;

      case  MLO_SYNTACTIC_ELEMENTS_TYPE_CPE:
         result = MLO_SyntacticElements_DecodeCpe (se_ptr, bit_ptr);
         break;

      case  MLO_SYNTACTIC_ELEMENTS_TYPE_CCE:
         result = MLO_SyntacticElements_DecodeCce (se_ptr, bit_ptr);
         break;

      case  MLO_SYNTACTIC_ELEMENTS_TYPE_LFE:
         result = MLO_SyntacticElements_DecodeLfe (se_ptr, bit_ptr);
         break;

      case  MLO_SYNTACTIC_ELEMENTS_TYPE_DSE:
         result = MLO_SyntacticElements_DecodeDse (se_ptr, bit_ptr);
         break;

      case  MLO_SYNTACTIC_ELEMENTS_TYPE_PCE:
         result = MLO_SyntacticElements_DecodePce (se_ptr, bit_ptr);
         break;

      case  MLO_SYNTACTIC_ELEMENTS_TYPE_FIL:
         result = MLO_SyntacticElements_DecodeFil (se_ptr, bit_ptr);
         break;

      case  MLO_SYNTACTIC_ELEMENTS_TYPE_END:
         cont_flag = MLO_FALSE;
         break;

      /* Shouldn't happen, all cases are covered for the 3-bit id_syn_ele */
      default:
         MLO_ASSERT (MLO_FALSE);
         break;
      }

      if (MLO_FAILED (result))
      {
         cont_flag = MLO_FALSE;
      }
   }
   while (cont_flag);

   if (MLO_SUCCEEDED (result))
   {
      result = MLO_BitStream_ByteAlign (bit_ptr);
   }

	return (result);
}



/*
==============================================================================
Name: MLO_SyntacticElements_FinishSpectralProc
Description:
   Does the second part of the specral processing, because some operations
   require to be done on multiple channels (coupling). This function processes
   all channels.
   To be called after MLO_SyntacticElements_Decode().
Output parameters:
Input/output parameters:
	- se_ptr: MLO_SyntactifElements object
Returns:
   MLO_SUCCESS if ok
   MLO_ERROR_SCE_TAG_UNKNOWN if a referenced SCE was not existing
   MLO_ERROR_CPE_TAG_UNKNOWN if a referenced CPE was not existing
==============================================================================
*/

MLO_Result  
MLO_SyntacticElements_FinishSpectralProc (MLO_SyntacticElements *se_ptr)
{
   MLO_Result     result = MLO_SUCCESS;
   int            cce;
   int            chn;
   int            nbr_cce;
   int            nbr_chn;

	MLO_ASSERT (se_ptr != NULL);

   nbr_cce = se_ptr->nbr_cce;
   nbr_chn = se_ptr->chn_pool.nbr_chn;

   /* Dependently Switched Coupling (before TNS) */
   for (cce = 0; cce < nbr_cce && MLO_SUCCEEDED (result); ++cce)
   {
      result = MLO_ElementCce_Process (
         &se_ptr->cce_arr [cce],
         se_ptr,
         MLO_ELEMENT_CCE_STAGE_DEP_BEFORE_TNS
      );
   }

   /* Temporal Noise Shapping */
   if (MLO_SUCCEEDED (result))
   {
      for (chn = 0; chn < nbr_chn; ++chn)
      {
         MLO_Tns_Process (se_ptr->chn_pool.chn_ptr_arr [chn]);
      }
   }

   /* Dependently Switched Coupling (after TNS) */
   for (cce = 0; cce < nbr_cce && MLO_SUCCEEDED (result); ++cce)
   {
      result = MLO_ElementCce_Process (
         &se_ptr->cce_arr [cce],
         se_ptr,
         MLO_ELEMENT_CCE_STAGE_DEP_AFTER_TNS
      );
   }

/*************************************************************************************************************/
/*** Debug ***/
#if 0
   for (chn = 0; chn < nbr_chn && MLO_SUCCEEDED (result); ++chn)
   {
      MLO_SyntacticElements_DumpFrame (
         chn,
         se_ptr->chn_pool.chn_ptr_arr [chn]->coef_arr,
         MLO_DEFS_FRAME_LEN_LONG
      );
   }
   ++ MLO_SyntacticElements_frame_index;
#endif
/*************************************************************************************************************/

   return (result);
}



/*
==============================================================================
Name: MLO_SyntacticElements_ConvertSpectralToTime
Description:
	Converts spectral data to time domain and finishes time processing.
	Then content of coef_arr[] for each channel is ready to copied to the
	output buffer.
Input/output parameters:
	- se_ptr: MLO_SyntactifElements object
	- fb_ptr: Filterbank object used to do the IMDCT for each channel
Returns:
   MLO_SUCCESS if ok
   MLO_ERROR_SCE_TAG_UNKNOWN if a referenced SCE was not existing
   MLO_ERROR_CPE_TAG_UNKNOWN if a referenced CPE was not existing
==============================================================================
*/

MLO_Result  
MLO_SyntacticElements_ConvertSpectralToTime (MLO_SyntacticElements *se_ptr, MLO_FilterBank *fb_ptr)
{
   MLO_Result     result = MLO_SUCCESS;
   int            cce;
   int            chn;
   int            nbr_cce;
   int            nbr_chn;

   MLO_ASSERT (se_ptr != NULL);
   MLO_ASSERT (fb_ptr != NULL);

   nbr_cce = se_ptr->nbr_cce;
   nbr_chn = se_ptr->chn_pool.nbr_chn;

	/* Conversion from spectral to time domain */
   for (chn = 0; chn < nbr_chn; ++chn)
   {
      MLO_IndivChnStream *	ics_ptr = se_ptr->chn_pool.chn_ptr_arr [chn];
      MLO_FilterBank_ConvertSpectralToTime (fb_ptr, ics_ptr);
   }

   /* Independently Switched Coupling */
   for (cce = 0; cce < nbr_cce && MLO_SUCCEEDED (result); ++cce)
   {
      result = MLO_ElementCce_Process (
         &se_ptr->cce_arr [cce],
         se_ptr,
         MLO_ELEMENT_CCE_STAGE_INDEP
      );
   }

	return (result);
}



/*
==============================================================================
Name: MLO_SyntacticElements_SendToOutput
Description:
   Copy PCM data to output buffer.
Input parameters:
	- se_ptr: MLO_SyntacticElement object, frame fully decoded.
Input/output parameters:
	- outbuf_ptr: Final buffer. Should have enough allocated memory to store
      the frame, with the right number of channels.
==============================================================================
*/

MLO_Result
MLO_SyntacticElements_SendToOutput (const MLO_SyntacticElements *se_ptr, MLO_SampleBuffer *outbuf_ptr)
{
   int chn = 0;
   int elt;
   int nbr_received_elements;
   int channel_count = (int) MLO_SampleBuffer_GetFormat(outbuf_ptr)->channel_count;

   MLO_ASSERT (se_ptr != NULL);
   MLO_ASSERT (outbuf_ptr != NULL);

   nbr_received_elements = se_ptr->nbr_received_elements;

   for (elt = 0; elt < nbr_received_elements; ++elt)
   {
      const int index = se_ptr->order_arr [elt].index;

      switch (se_ptr->order_arr [elt].type)
      {
      /* SCE */
      case  MLO_SYNTACTIC_ELEMENTS_CONTENT_TYPE_SCE:
         if (chn+1 > channel_count) return MLO_ERROR_DECODER_INVALID_CHANNEL_CONFIGURATION;
         MLO_SyntacticElements_InterleaveAndConvertChannel (
            outbuf_ptr,
            chn,
            &se_ptr->sce_arr [index].ics_ptr->coef_arr [0]
         );
         ++ chn;
         break;

      /* CPE */
      case  MLO_SYNTACTIC_ELEMENTS_CONTENT_TYPE_CPE:
         if (chn+2 > channel_count) return MLO_ERROR_DECODER_INVALID_CHANNEL_CONFIGURATION;
         MLO_SyntacticElements_InterleaveAndConvertChannel (
            outbuf_ptr,
            chn + 0,
            &se_ptr->cpe_arr [index].ics_ptr_arr [0]->coef_arr [0]
         );
         MLO_SyntacticElements_InterleaveAndConvertChannel (
            outbuf_ptr,
            chn + 1,
            &se_ptr->cpe_arr [index].ics_ptr_arr [1]->coef_arr [0]
         );
         chn += 2;
         break;

      /* LFE */
      case  MLO_SYNTACTIC_ELEMENTS_CONTENT_TYPE_LFE:
         if (chn+1 > channel_count) return MLO_ERROR_DECODER_INVALID_CHANNEL_CONFIGURATION;
         MLO_SyntacticElements_InterleaveAndConvertChannel (
            outbuf_ptr,
            chn,
            &se_ptr->lfe_arr [index].ics_ptr->coef_arr [0]
         );
         ++ chn;
         break;

      default:
         MLO_ASSERT (MLO_FALSE);
      }
   }

   return MLO_SUCCESS;
}



/*
==============================================================================
Name: MLO_SyntacticElements_UseSce
Description:
   Finds a single_channel_element from a given element_tag.
Input parameters:
	- tag: The SCE identifier, should be at least in the valid tag range.
Output parameters:
	- sce_ptr_ptr: pointer to fill with the SCE address, if found.
Input/output parameters:
	- se_ptr: MLO_SyntactifElements object
Returns:
   MLO_SUCCESS if ok
   MLO_ERROR_SCE_TAG_UNKNOWN if the referenced SCE was not existing
==============================================================================
*/

MLO_Result  MLO_SyntacticElements_UseSce (MLO_SyntacticElements *se_ptr, int tag, MLO_ElementSceLfe **sce_ptr_ptr)
{
   MLO_Result     result = MLO_SUCCESS;
   int            index;

	MLO_ASSERT (se_ptr != NULL);
    MLO_ASSERT (tag >= 0);
    MLO_ASSERT (tag < (int)MLO_ARRAY_SIZE (se_ptr->sce_tag_map));
	MLO_ASSERT (sce_ptr_ptr != NULL);

   index = se_ptr->sce_tag_map [tag];
   if (index < 0)
   {
      result = MLO_ERROR_SCE_TAG_UNKNOWN;
   }
   else
   {
      *sce_ptr_ptr = &se_ptr->sce_arr [index];
   }

   return (result);
}



/*
==============================================================================
Name: MLO_SyntacticElements_UseCpe
Description:
   Finds a channel_pair_element from a given element_tag.
Input parameters:
	- tag: The SCE identifier, should be at least in the valid tag range.
Output parameters:
	- cpe_ptr_ptr: pointer to fill with the CPE address, if found.
Input/output parameters:
	- se_ptr: MLO_SyntactifElements object
Returns:
   MLO_SUCCESS if ok
   MLO_ERROR_CPE_TAG_UNKNOWN if the referenced CPE was not existing
==============================================================================
*/

MLO_Result  MLO_SyntacticElements_UseCpe (MLO_SyntacticElements *se_ptr, int tag, MLO_ElementCpe **cpe_ptr_ptr)
{
   MLO_Result     result = MLO_SUCCESS;
   int            index;

	MLO_ASSERT (se_ptr != NULL);
    MLO_ASSERT (tag >= 0);
    MLO_ASSERT (tag < (int)MLO_ARRAY_SIZE (se_ptr->cpe_tag_map));
	MLO_ASSERT (cpe_ptr_ptr != NULL);

   index = se_ptr->cpe_tag_map [tag];
   if (index < 0)
   {
      result = MLO_ERROR_CPE_TAG_UNKNOWN;
   }
   else
   {
      *cpe_ptr_ptr = &se_ptr->cpe_arr [index];
   }

   return (result);
}



static MLO_Result MLO_SyntacticElements_DecodeSce (MLO_SyntacticElements *se_ptr, MLO_BitStream *bit_ptr)
{
   MLO_Result     result = MLO_SUCCESS;
   int            pos;

	MLO_ASSERT (se_ptr != NULL);
	MLO_ASSERT (bit_ptr != NULL);

   pos = se_ptr->nbr_sce;
   if (pos >= (int)MLO_ARRAY_SIZE (se_ptr->sce_arr))
   {
      result = MLO_FAILURE;
   }

   else
   {
      result = MLO_ElementSceLfe_Decode (
         &se_ptr->sce_arr [pos],
         bit_ptr,
         &se_ptr->chn_pool,
         se_ptr->pce.sampling_frequency_index
      );
   }

   if (MLO_SUCCEEDED (result))
   {
      int            tag =
         se_ptr->sce_arr [pos].element_instance_tag;
      MLO_ASSERT (tag >= 0);
      MLO_ASSERT (tag < (int)MLO_ARRAY_SIZE (se_ptr->sce_tag_map));
      if (se_ptr->sce_tag_map [tag] >= 0)
      {
         result = MLO_ERROR_SCE_TAG_DUPLICATED;
      }
      else
      {
         se_ptr->sce_tag_map [tag] = pos;
         MLO_SyntacticElements_MemorizeElement (
            se_ptr,
            MLO_SYNTACTIC_ELEMENTS_CONTENT_TYPE_SCE,
            pos
         );

         ++ se_ptr->nbr_sce;
      }
   }

	return (result);
}



static MLO_Result MLO_SyntacticElements_DecodeCpe (MLO_SyntacticElements *se_ptr, MLO_BitStream *bit_ptr)
{
   MLO_Result     result = MLO_SUCCESS;
   int            pos;

	MLO_ASSERT (se_ptr != NULL);
	MLO_ASSERT (bit_ptr != NULL);

   pos = se_ptr->nbr_cpe;
   if (pos >= (int)MLO_ARRAY_SIZE (se_ptr->cpe_arr))
   {
      result = MLO_FAILURE;
   }

   else
   {
      result = MLO_ElementCpe_Decode (
         &se_ptr->cpe_arr [pos],
         bit_ptr,
         &se_ptr->chn_pool,
         se_ptr->pce.sampling_frequency_index
      );
   }

   if (MLO_SUCCEEDED (result))
   {
      int            tag =
         se_ptr->cpe_arr [pos].element_instance_tag;
      MLO_ASSERT (tag >= 0);
      MLO_ASSERT (tag < (int)MLO_ARRAY_SIZE (se_ptr->cpe_tag_map));
      if (se_ptr->cpe_tag_map [tag] >= 0)
      {
         result = MLO_ERROR_CPE_TAG_DUPLICATED;
      }
      else
      {
         se_ptr->cpe_tag_map [tag] = pos;
         MLO_SyntacticElements_MemorizeElement (
            se_ptr,
            MLO_SYNTACTIC_ELEMENTS_CONTENT_TYPE_CPE,
            pos
         );

         ++ se_ptr->nbr_cpe;
      }
   }

	return (result);
}



static MLO_Result MLO_SyntacticElements_DecodeCce (MLO_SyntacticElements *se_ptr, MLO_BitStream *bit_ptr)
{
   MLO_Result     result = MLO_SUCCESS;

	MLO_ASSERT (se_ptr != NULL);
	MLO_ASSERT (bit_ptr != NULL);

   if (se_ptr->nbr_cce >= (int)MLO_ARRAY_SIZE (se_ptr->cce_arr))
   {
      result = MLO_FAILURE;
   }

   else
   {
      result = MLO_ElementCce_Decode (
         &se_ptr->cce_arr [se_ptr->nbr_cce],
         bit_ptr,
         &se_ptr->chn_pool,
         se_ptr->pce.sampling_frequency_index
      );
   }

   if (MLO_SUCCEEDED (result))
   {
      ++ se_ptr->nbr_cce;
   }

	return (result);
}



static MLO_Result MLO_SyntacticElements_DecodeLfe (MLO_SyntacticElements *se_ptr, MLO_BitStream *bit_ptr)
{
   MLO_Result     result = MLO_SUCCESS;
   int            pos;

	MLO_ASSERT (se_ptr != NULL);
	MLO_ASSERT (bit_ptr != NULL);

   pos = se_ptr->nbr_lfe;
   if (pos >= (int)MLO_ARRAY_SIZE (se_ptr->lfe_arr))
   {
      result = MLO_FAILURE;
   }

   else
   {
      result = MLO_ElementSceLfe_Decode (
         &se_ptr->lfe_arr [pos],
         bit_ptr,
         &se_ptr->chn_pool,
         se_ptr->pce.sampling_frequency_index
      );
   }

   if (MLO_SUCCEEDED (result))
   {
      MLO_SyntacticElements_MemorizeElement (
         se_ptr,
         MLO_SYNTACTIC_ELEMENTS_CONTENT_TYPE_LFE,
         pos
      );

      ++ se_ptr->nbr_lfe;
   }

	return (result);
}



static MLO_Result MLO_SyntacticElements_DecodeDse (MLO_SyntacticElements *se_ptr, MLO_BitStream *bit_ptr)
{
    MLO_Result     result = MLO_SUCCESS;

	MLO_ASSERT (se_ptr != NULL);
	MLO_ASSERT (bit_ptr != NULL);
    (void)se_ptr; /* unused */

    result = MLO_ElementDse_Decode (bit_ptr);

	return (result);
}



static MLO_Result MLO_SyntacticElements_DecodePce (MLO_SyntacticElements *se_ptr, MLO_BitStream *bit_ptr)
{
   MLO_Result     result = MLO_SUCCESS;

	MLO_ASSERT (se_ptr != NULL);
	MLO_ASSERT (bit_ptr != NULL);

   result = MLO_ElementPce_Decode (&se_ptr->pce, bit_ptr);

	return (result);
}



static MLO_Result MLO_SyntacticElements_DecodeFil (MLO_SyntacticElements *se_ptr, MLO_BitStream *bit_ptr)
{
   MLO_Result     result = MLO_SUCCESS;

	MLO_ASSERT (se_ptr != NULL);
	MLO_ASSERT (bit_ptr != NULL);

   if (se_ptr->nbr_fil >= (int)MLO_ARRAY_SIZE (se_ptr->fil_arr))
   {
      result = MLO_FAILURE;
   }

   else
   {
      result = MLO_ElementFil_Decode (
         &se_ptr->fil_arr [se_ptr->nbr_fil],
         bit_ptr
      );
   }

   if (MLO_SUCCEEDED (result))
   {
      ++ se_ptr->nbr_fil;
   }

	return (result);
}



static void MLO_SyntacticElements_MemorizeElement (MLO_SyntacticElements *se_ptr, MLO_SyntacticElements_ContentType type, int index)
{
   int            pos;

   MLO_ASSERT (se_ptr != NULL);
   MLO_ASSERT (type < MLO_SYNTACTIC_ELEMENTS_CONTENT_TYPE_SCE_NBR_ELT);
   MLO_ASSERT (index >= 0);

   pos = se_ptr->nbr_received_elements;
   MLO_ASSERT (pos < (int)MLO_ARRAY_SIZE (se_ptr->order_arr));

   se_ptr->order_arr [pos].type  = type;
   se_ptr->order_arr [pos].index = index;

   ++ se_ptr->nbr_received_elements;
}

/*
==============================================================================
Name: MLO_SyntacticElements_InterleaveAndConvertChannel
Description:
   Converts the internal overlapped buffer to the final output buffer.
   This is where the sample format conversion takes place.
   Note: ensure that all samples formats are supported.
Input parameters:
	- chn: channel index in the output buffer, >= 0
	- in_ptr: overlapped data for this frame/channel (length == long frame)
Input/output parameters:
	- outbuf_ptr: The final output buffer where the rendering takes place.
==============================================================================
*/

static void MLO_SyntacticElements_InterleaveAndConvertChannel (MLO_SampleBuffer *outbuf_ptr, int chn, const MLO_Float in_ptr [])
{
   int                     nbr_chn;
   const MLO_SampleFormat* format;

   MLO_ASSERT (outbuf_ptr != NULL);
   MLO_ASSERT (chn >= 0);
   MLO_ASSERT (in_ptr != NULL);

   format = MLO_SampleBuffer_GetFormat(outbuf_ptr);
   MLO_ASSERT (format->type == MLO_SAMPLE_TYPE_INTERLACED_SIGNED);
   nbr_chn = format->channel_count;
   MLO_ASSERT (chn < nbr_chn);

   if (format->bits_per_sample == 16)
   {
      int            pos;
      MLO_Int16 *    out_ptr = (MLO_Int16 *) MLO_SampleBuffer_UseSamples(outbuf_ptr);
      out_ptr += chn;
      for (pos = 0; pos < MLO_DEFS_FRAME_LEN_LONG; ++pos, out_ptr += nbr_chn)
      {
         int sample = MLO_Float_RoundInt(in_ptr[pos]);
         *out_ptr = MLO_BOUND(sample, -32768, 32767);
      }
   }

   else
   {
      /*** To do ***/
      MLO_ASSERT (MLO_FALSE);
   }
}
