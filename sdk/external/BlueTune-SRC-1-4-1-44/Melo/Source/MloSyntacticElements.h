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

#ifndef _MLO_SYNTACTIC_ELEMENTS_H_
#define _MLO_SYNTACTIC_ELEMENTS_H_



/*----------------------------------------------------------------------
|       Includes
+---------------------------------------------------------------------*/



#include "MloSamplingFreq.h"
#include "MloBitStream.h"
#include "MloElementCce.h"
#include "MloElementCpe.h"
#include "MloElementFil.h"
#include "MloElementPce.h"
#include "MloElementSceLfe.h"
#include	"MloFilterBank.h"
#include "MloIndivChnPool.h"
#include "MloSampleBuffer.h"



/*----------------------------------------------------------------------
|       Constants
+---------------------------------------------------------------------*/



#define  MLO_ERROR_SCE_TAG_DUPLICATED  (MLO_ERROR_BASE_SYNTACTIC_ELEMENTS-0)
#define  MLO_ERROR_CPE_TAG_DUPLICATED  (MLO_ERROR_BASE_SYNTACTIC_ELEMENTS-1)
#define  MLO_ERROR_SCE_TAG_UNKNOWN     (MLO_ERROR_BASE_SYNTACTIC_ELEMENTS-2)
#define  MLO_ERROR_CPE_TAG_UNKNOWN     (MLO_ERROR_BASE_SYNTACTIC_ELEMENTS-3)



/*----------------------------------------------------------------------
|       Types
+---------------------------------------------------------------------*/



typedef enum MLO_SyntacticElements_ContentType
{
   MLO_SYNTACTIC_ELEMENTS_CONTENT_TYPE_SCE = 0,
   MLO_SYNTACTIC_ELEMENTS_CONTENT_TYPE_CPE,
   MLO_SYNTACTIC_ELEMENTS_CONTENT_TYPE_LFE,

   MLO_SYNTACTIC_ELEMENTS_CONTENT_TYPE_SCE_NBR_ELT
}  MLO_SyntacticElements_ContentType;



typedef struct MLO_SyntacticElements_ContentRef
{
   MLO_SyntacticElements_ContentType
                  type;
   int            index;
}  MLO_SyntacticElements_ContentRef;



typedef  int   MLO_SyntacticElements_TagMap [16];  /* < 0: empty tag, >= 0: element index */



typedef struct MLO_SyntacticElements
{
   MLO_ElementPce pce;           /* program_config_element */

   MLO_IndivChnPool
                  chn_pool;

   int            nbr_fil;
   MLO_ElementFil fil_arr [16];  /* fill_element */

   int            nbr_sce;
   MLO_ElementSceLfe             /* single_channel_element */
                  sce_arr [16];
   MLO_SyntacticElements_TagMap
                  sce_tag_map;

   int            nbr_lfe;
   MLO_ElementSceLfe             /* lfe_channel_element */
                  lfe_arr [16];

   int            nbr_cpe;
   MLO_ElementCpe cpe_arr [16];  /* channel_pair_element */
   MLO_SyntacticElements_TagMap
                  cpe_tag_map;

   int            nbr_cce;
   MLO_ElementCce cce_arr [16];  /* coupling_channel_element */

   int            nbr_received_elements;
   MLO_SyntacticElements_ContentRef
                  order_arr [64];   /* Order of receipt */
}  MLO_SyntacticElements;



/*----------------------------------------------------------------------
|       Function prototypes
+---------------------------------------------------------------------*/



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



MLO_Result  MLO_SyntacticElements_Init (MLO_SyntacticElements *se_ptr);
void  MLO_SyntacticElements_Restore (MLO_SyntacticElements *se_ptr);
MLO_Result  MLO_SyntacticElements_SetNbrChn (MLO_SyntacticElements *se_ptr, int nbr_chn);

void  MLO_SyntacticElements_StartNewFrame (MLO_SyntacticElements *se_ptr, MLO_SamplingFreq_Index fs_index);

MLO_Result  MLO_SyntacticElements_Decode (MLO_SyntacticElements *se_ptr, MLO_BitStream *bit_ptr);
MLO_Result  MLO_SyntacticElements_FinishSpectralProc (MLO_SyntacticElements *se_ptr);
MLO_Result  MLO_SyntacticElements_ConvertSpectralToTime (MLO_SyntacticElements *se_ptr, MLO_FilterBank *fb_ptr);
MLO_Result  MLO_SyntacticElements_SendToOutput (const MLO_SyntacticElements *se_ptr, MLO_SampleBuffer *outbuf_ptr);

MLO_Result  MLO_SyntacticElements_UseSce (MLO_SyntacticElements *se_ptr, int tag, MLO_ElementSceLfe **sce_ptr_ptr);
MLO_Result  MLO_SyntacticElements_UseCpe (MLO_SyntacticElements *se_ptr, int tag, MLO_ElementCpe **cpe_ptr_ptr);


#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif /* _MLO_SYNTACTIC_ELEMENTS_H_ */
